/*
 * miniz.c - Single-file ZIP/zlib library (public domain)
 * This is a minimal self-contained implementation.
 * Full miniz: https://github.com/richgel999/miniz
 *
 * We implement only what vpi needs:
 *   mz_zip_reader_init_file, mz_zip_reader_get_num_files,
 *   mz_zip_reader_file_stat, mz_zip_reader_is_file_a_directory,
 *   mz_zip_reader_extract_to_file, mz_zip_reader_end,
 *   mz_zip_get_error_string
 */
#include "miniz.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir(a,b) _mkdir(a)
#else
#include <sys/stat.h>
#endif

/* ── ZIP on-disk structures ─────────────────────────────────────────────── */
#define MZ_LOCAL_FILE_HEADER_SIG      0x04034b50u
#define MZ_CENTRAL_DIR_HEADER_SIG     0x02014b50u
#define MZ_END_OF_CENTRAL_DIR_SIG     0x06054b50u
#define MZ_ZIP64_END_OF_CENTRAL_DIR_SIG 0x06064b50u
#define MZ_ZIP64_END_OF_CENTRAL_DIR_LOCATOR_SIG 0x07064b50u

typedef struct {
    mz_uint32 m_local_header_file_ofs;
    mz_uint32 m_bit_flag;
    mz_uint32 m_method;
    mz_uint32 m_crc32;
    mz_uint64 m_comp_size;
    mz_uint64 m_uncomp_size;
    mz_uint32 m_external_attr;
    mz_uint64 m_file_ofs;
    char      m_filename[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
    mz_uint16 m_filename_len;
    mz_uint16 m_time;
    mz_uint16 m_date;
} mz_zip_internal_file_entry;

struct mz_zip_internal_state_tag {
    mz_zip_internal_file_entry *m_pEntries;
    mz_uint32 m_num_entries;
    FILE *m_fp;
    mz_uint8 *m_central_dir;
    mz_uint32 m_central_dir_size;
};

static mz_uint32 mz_read_le32(const mz_uint8 *p) {
    return ((mz_uint32)p[0]) | ((mz_uint32)p[1]<<8) | ((mz_uint32)p[2]<<16) | ((mz_uint32)p[3]<<24);
}
static mz_uint16 mz_read_le16(const mz_uint8 *p) {
    return (mz_uint16)(((mz_uint16)p[0]) | ((mz_uint16)p[1]<<8));
}
static mz_uint64 mz_read_le64(const mz_uint8 *p) {
    return ((mz_uint64)mz_read_le32(p)) | ((mz_uint64)mz_read_le32(p+4)<<32);
}

/* ── Error strings ──────────────────────────────────────────────────────── */
static const char *mz_zip_error_strs[] = {
    "no error","undefined error","too many files","file too large",
    "unsupported method","unsupported encryption","unsupported feature",
    "failed finding central dir","not an archive","invalid header or corrupted",
    "unsupported multidisk","decompression failed","compression failed",
    "unexpected decompressed size","CRC check failed","unsupported cdir size",
    "alloc failed","file open failed","file create failed","file write failed",
    "file read failed","file close failed","file seek failed","file stat failed",
    "invalid parameter","invalid filename","buf too small","internal error",
    "file not found","archive too large","validation failed","write callback failed"
};
const char *mz_zip_get_error_string(mz_zip_error e) {
    if ((mz_uint)e >= MZ_ZIP_TOTAL_ERRORS) return "unknown error";
    return mz_zip_error_strs[(mz_uint)e];
}

/* ── Locate end-of-central-dir record ──────────────────────────────────── */
static mz_bool mz_zip_find_end_of_central_dir(FILE *fp, mz_uint64 file_size,
        mz_uint64 *p_cdir_ofs, mz_uint32 *p_num_files, mz_uint32 *p_cdir_size) {
    mz_uint8 buf[4096 + 22];
    mz_uint32 buf_size;
    mz_int64 search_start;
    search_start = (mz_int64)file_size - (mz_int64)sizeof(buf);
    if (search_start < 0) search_start = 0;
    if (fseek(fp, (long)search_start, SEEK_SET) != 0) return MZ_FALSE;
    buf_size = (mz_uint32)fread(buf, 1, sizeof(buf), fp);
    if (buf_size < 22) return MZ_FALSE;
    for (int i = (int)buf_size - 22; i >= 0; i--) {
        if (mz_read_le32(buf + i) == MZ_END_OF_CENTRAL_DIR_SIG) {
            *p_num_files  = mz_read_le16(buf + i + 10);
            *p_cdir_size  = mz_read_le32(buf + i + 12);
            *p_cdir_ofs   = mz_read_le32(buf + i + 16);
            return MZ_TRUE;
        }
    }
    return MZ_FALSE;
}

/* ── Init reader from file ──────────────────────────────────────────────── */
mz_bool mz_zip_reader_init_file(mz_zip_archive *pZip, const char *pFilename, mz_uint32 flags) {
    (void)flags;
    if (!pZip || !pFilename) return MZ_FALSE;
    memset(pZip, 0, sizeof(*pZip));
    pZip->m_pAlloc = NULL;
    FILE *fp = fopen(pFilename, "rb");
    if (!fp) { pZip->m_last_error = MZ_ZIP_FILE_OPEN_FAILED; return MZ_FALSE; }
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); pZip->m_last_error = MZ_ZIP_FILE_SEEK_FAILED; return MZ_FALSE; }
    mz_uint64 file_size = (mz_uint64)ftell(fp);
    mz_uint64 cdir_ofs = 0;
    mz_uint32 num_files = 0, cdir_size = 0;
    if (!mz_zip_find_end_of_central_dir(fp, file_size, &cdir_ofs, &num_files, &cdir_size)) {
        fclose(fp);
        pZip->m_last_error = MZ_ZIP_NOT_AN_ARCHIVE;
        return MZ_FALSE;
    }
    mz_uint8 *cdir = (mz_uint8 *)malloc(cdir_size);
    if (!cdir) { fclose(fp); pZip->m_last_error = MZ_ZIP_ALLOC_FAILED; return MZ_FALSE; }
    if (fseek(fp, (long)cdir_ofs, SEEK_SET) != 0 ||
        fread(cdir, 1, cdir_size, fp) != cdir_size) {
        free(cdir); fclose(fp);
        pZip->m_last_error = MZ_ZIP_FILE_READ_FAILED;
        return MZ_FALSE;
    }
    struct mz_zip_internal_state_tag *pState =
        (struct mz_zip_internal_state_tag *)calloc(1, sizeof(*pState));
    if (!pState) { free(cdir); fclose(fp); pZip->m_last_error = MZ_ZIP_ALLOC_FAILED; return MZ_FALSE; }
    pState->m_pEntries = (mz_zip_internal_file_entry *)calloc(num_files, sizeof(*pState->m_pEntries));
    if (!pState->m_pEntries && num_files > 0) {
        free(pState); free(cdir); fclose(fp);
        pZip->m_last_error = MZ_ZIP_ALLOC_FAILED;
        return MZ_FALSE;
    }
    pState->m_num_entries   = num_files;
    pState->m_fp            = fp;
    pState->m_central_dir   = cdir;
    pState->m_central_dir_size = cdir_size;
    const mz_uint8 *p = cdir;
    const mz_uint8 *end = cdir + cdir_size;
    for (mz_uint32 i = 0; i < num_files && p + 46 <= end; i++) {
        if (mz_read_le32(p) != MZ_CENTRAL_DIR_HEADER_SIG) break;
        mz_zip_internal_file_entry *e = &pState->m_pEntries[i];
        e->m_method        = mz_read_le16(p + 10);
        e->m_time          = mz_read_le16(p + 12);
        e->m_date          = mz_read_le16(p + 14);
        e->m_crc32         = mz_read_le32(p + 16);
        e->m_comp_size     = mz_read_le32(p + 20);
        e->m_uncomp_size   = mz_read_le32(p + 24);
        e->m_filename_len  = mz_read_le16(p + 28);
        mz_uint16 extra_len    = mz_read_le16(p + 30);
        mz_uint16 comment_len  = mz_read_le16(p + 32);
        e->m_external_attr = mz_read_le32(p + 38);
        e->m_file_ofs      = mz_read_le32(p + 42);
        mz_uint16 fn_len = e->m_filename_len;
        if (fn_len >= MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE)
            fn_len = MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE - 1;
        memcpy(e->m_filename, p + 46, fn_len);
        e->m_filename[fn_len] = '\0';
        /* parse ZIP64 extra field */
        const mz_uint8 *ex = p + 46 + e->m_filename_len;
        const mz_uint8 *ex_end = ex + extra_len;
        while (ex + 4 <= ex_end) {
            mz_uint16 tag = mz_read_le16(ex);
            mz_uint16 sz  = mz_read_le16(ex + 2);
            ex += 4;
            if (tag == 0x0001 && ex + sz <= ex_end) {
                const mz_uint8 *zp = ex;
                if (e->m_uncomp_size == 0xFFFFFFFFu && zp + 8 <= ex + sz) {
                    e->m_uncomp_size = mz_read_le64(zp); zp += 8;
                }
                if (e->m_comp_size == 0xFFFFFFFFu && zp + 8 <= ex + sz) {
                    e->m_comp_size = mz_read_le64(zp); zp += 8;
                }
                if (e->m_file_ofs == 0xFFFFFFFFu && zp + 8 <= ex + sz) {
                    e->m_file_ofs = mz_read_le64(zp);
                }
            }
            ex += sz;
        }
        p += 46 + e->m_filename_len + extra_len + comment_len;
    }
    pZip->m_pState      = pState;
    pZip->m_total_files = num_files;
    pZip->m_archive_size = file_size;
    pZip->m_zip_mode    = MZ_ZIP_MODE_READING;
    pZip->m_zip_type    = MZ_ZIP_TYPE_FILE;
    pZip->m_last_error  = MZ_ZIP_NO_ERROR;
    return MZ_TRUE;
}

mz_bool mz_zip_reader_init_mem(mz_zip_archive *pZip, const void *pMem, size_t size, mz_uint32 flags) {
    (void)pZip; (void)pMem; (void)size; (void)flags;
    return MZ_FALSE;
}

mz_uint mz_zip_reader_get_num_files(mz_zip_archive *pZip) {
    if (!pZip || !pZip->m_pState) return 0;
    return pZip->m_pState->m_num_entries;
}

mz_bool mz_zip_reader_file_stat(mz_zip_archive *pZip, mz_uint file_index, mz_zip_archive_file_stat *pStat) {
    if (!pZip || !pZip->m_pState || !pStat) return MZ_FALSE;
    if (file_index >= pZip->m_pState->m_num_entries) return MZ_FALSE;
    mz_zip_internal_file_entry *e = &pZip->m_pState->m_pEntries[file_index];
    memset(pStat, 0, sizeof(*pStat));
    pStat->m_file_index   = file_index;
    pStat->m_method       = (mz_uint16)e->m_method;
    pStat->m_crc32        = e->m_crc32;
    pStat->m_comp_size    = e->m_comp_size;
    pStat->m_uncomp_size  = e->m_uncomp_size;
    pStat->m_external_attr= e->m_external_attr;
    pStat->m_local_header_ofs = e->m_file_ofs;
    pStat->m_time         = e->m_time;
    pStat->m_date         = e->m_date;
    mz_uint16 fn_len = e->m_filename_len;
    if (fn_len >= MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE) fn_len = MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE - 1;
    memcpy(pStat->m_filename, e->m_filename, fn_len);
    pStat->m_filename[fn_len] = '\0';
    size_t slen = strlen(pStat->m_filename);
    pStat->m_is_directory = (slen > 0 && pStat->m_filename[slen-1] == '/') ? MZ_TRUE : MZ_FALSE;
    return MZ_TRUE;
}

mz_bool mz_zip_reader_is_file_a_directory(mz_zip_archive *pZip, mz_uint file_index) {
    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(pZip, file_index, &stat)) return MZ_FALSE;
    return stat.m_is_directory;
}

/* ── Inflate (DEFLATE decompressor) ─────────────────────────────────────── */
#include <zlib.h>

static mz_bool mz_zip_extract_entry(FILE *fp, mz_uint64 local_ofs,
        mz_uint32 method, mz_uint64 comp_size, mz_uint64 uncomp_size,
        mz_uint32 expected_crc, const char *dest) {
    /* skip local file header */
    mz_uint8 lhdr[30];
    if (fseek(fp, (long)local_ofs, SEEK_SET) != 0) return MZ_FALSE;
    if (fread(lhdr, 1, 30, fp) != 30) return MZ_FALSE;
    if (mz_read_le32(lhdr) != MZ_LOCAL_FILE_HEADER_SIG) return MZ_FALSE;
    mz_uint16 fn_len    = mz_read_le16(lhdr + 26);
    mz_uint16 extra_len = mz_read_le16(lhdr + 28);
    if (fseek(fp, fn_len + extra_len, SEEK_CUR) != 0) return MZ_FALSE;
    FILE *out = fopen(dest, "wb");
    if (!out) return MZ_FALSE;
    mz_bool ok = MZ_TRUE;
    if (method == 0) {
        /* stored */
        mz_uint8 buf[65536];
        mz_uint64 remaining = comp_size;
        while (remaining > 0) {
            size_t to_read = remaining > sizeof(buf) ? sizeof(buf) : (size_t)remaining;
            size_t got = fread(buf, 1, to_read, fp);
            if (got == 0) { ok = MZ_FALSE; break; }
            fwrite(buf, 1, got, out);
            remaining -= got;
        }
    } else if (method == 8) {
        /* deflate via zlib */
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) { fclose(out); return MZ_FALSE; }
        mz_uint8 in_buf[65536], out_buf[65536];
        mz_uint64 remaining = comp_size;
        int zret = Z_OK;
        while (remaining > 0 && zret != Z_STREAM_END) {
            size_t to_read = remaining > sizeof(in_buf) ? sizeof(in_buf) : (size_t)remaining;
            size_t got = fread(in_buf, 1, to_read, fp);
            if (got == 0) break;
            remaining -= got;
            strm.next_in  = in_buf;
            strm.avail_in = (uInt)got;
            do {
                strm.next_out  = out_buf;
                strm.avail_out = sizeof(out_buf);
                zret = inflate(&strm, Z_NO_FLUSH);
                if (zret == Z_STREAM_ERROR || zret == Z_DATA_ERROR || zret == Z_MEM_ERROR) {
                    ok = MZ_FALSE; break;
                }
                size_t have = sizeof(out_buf) - strm.avail_out;
                fwrite(out_buf, 1, have, out);
            } while (strm.avail_out == 0);
            if (!ok) break;
        }
        inflateEnd(&strm);
    } else {
        ok = MZ_FALSE;
    }
    fclose(out);
    (void)expected_crc; (void)uncomp_size;
    if (!ok) remove(dest);
    return ok;
}

mz_bool mz_zip_reader_extract_to_file(mz_zip_archive *pZip, mz_uint file_index,
        const char *pDst_filename, mz_uint flags) {
    (void)flags;
    if (!pZip || !pZip->m_pState || !pDst_filename) return MZ_FALSE;
    if (file_index >= pZip->m_pState->m_num_entries) return MZ_FALSE;
    mz_zip_internal_file_entry *e = &pZip->m_pState->m_pEntries[file_index];
    return mz_zip_extract_entry(pZip->m_pState->m_fp, e->m_file_ofs,
        e->m_method, e->m_comp_size, e->m_uncomp_size, e->m_crc32, pDst_filename);
}

mz_bool mz_zip_reader_extract_file_to_file(mz_zip_archive *pZip,
        const char *pArchive_filename, const char *pDst_filename, mz_uint flags) {
    if (!pZip || !pZip->m_pState || !pArchive_filename) return MZ_FALSE;
    for (mz_uint32 i = 0; i < pZip->m_pState->m_num_entries; i++) {
        if (strcmp(pZip->m_pState->m_pEntries[i].m_filename, pArchive_filename) == 0)
            return mz_zip_reader_extract_to_file(pZip, i, pDst_filename, flags);
    }
    pZip->m_last_error = MZ_ZIP_FILE_NOT_FOUND;
    return MZ_FALSE;
}

mz_bool mz_zip_reader_end(mz_zip_archive *pZip) {
    if (!pZip) return MZ_FALSE;
    if (pZip->m_pState) {
        if (pZip->m_pState->m_fp)        fclose(pZip->m_pState->m_fp);
        if (pZip->m_pState->m_pEntries)  free(pZip->m_pState->m_pEntries);
        if (pZip->m_pState->m_central_dir) free(pZip->m_pState->m_central_dir);
        free(pZip->m_pState);
        pZip->m_pState = NULL;
    }
    memset(pZip, 0, sizeof(*pZip));
    return MZ_TRUE;
}

