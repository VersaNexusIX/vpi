#ifndef MINIZ_H
#define MINIZ_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef unsigned long mz_ulong;
typedef unsigned char mz_uint8;
typedef unsigned short mz_uint16;
typedef unsigned int mz_uint32;
typedef unsigned int mz_uint;
typedef int64_t mz_int64;
typedef uint64_t mz_uint64;
typedef int mz_bool;
#define MZ_FALSE (0)
#define MZ_TRUE (1)
#define MZ_OK (0)
#define MZ_STREAM_END (1)
#define MZ_NEED_DICT (2)
#define MZ_ERRNO (-1)
#define MZ_STREAM_ERROR (-2)
#define MZ_DATA_ERROR (-3)
#define MZ_MEM_ERROR (-4)
#define MZ_BUF_ERROR (-5)
#define MZ_VERSION_ERROR (-6)
#define MZ_PARAM_ERROR (-10000)
#define MZ_DEFAULT_COMPRESSION (-1)
#define MZ_NO_COMPRESSION 0
#define MZ_BEST_SPEED 1
#define MZ_BEST_COMPRESSION 9
#define MZ_UBER_COMPRESSION 10
#define MZ_DEFAULT_LEVEL 6
#define MZ_DEFAULT_STRATEGY 0
#define MZ_FILTERED 1
#define MZ_HUFFMAN_ONLY 2
#define MZ_RLE 3
#define MZ_FIXED 4
#define MZ_DEFLATED 8
#define MZ_ZIP_MAX_IO_BUF_SIZE (64 * 1024)
#define MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE (512)
#define MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE (512)
typedef struct {
    mz_uint32 m_file_index;
    mz_uint32 m_central_dir_ofs;
    mz_uint16 m_version_made_by;
    mz_uint16 m_version_needed;
    mz_uint16 m_bit_flag;
    mz_uint16 m_method;
    mz_uint32 m_crc32;
    mz_uint64 m_comp_size;
    mz_uint64 m_uncomp_size;
    mz_uint16 m_internal_attr;
    mz_uint32 m_external_attr;
    mz_uint64 m_local_header_ofs;
    mz_uint32 m_comment_size;
    mz_bool m_is_directory;
    mz_bool m_is_utf8;
    mz_bool m_is_supported;
    char m_filename[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
    char m_comment[MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE];
    mz_uint16 m_time;
    mz_uint16 m_date;
} mz_zip_archive_file_stat;
typedef size_t (*mz_file_read_func)(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n);
typedef size_t (*mz_file_write_func)(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n);
typedef void *(*mz_alloc_func)(void *pOpaque, size_t items, size_t size);
typedef void (*mz_free_func)(void *pOpaque, void *p);
typedef void *(*mz_realloc_func)(void *pOpaque, void *p, size_t items, size_t size);
typedef void (*mz_file_needs_keepalive)(void *pOpaque);
typedef enum {
    MZ_ZIP_MODE_INVALID = 0,
    MZ_ZIP_MODE_READING,
    MZ_ZIP_MODE_WRITING,
    MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED
} mz_zip_mode;
typedef enum {
    MZ_ZIP_TYPE_INVALID = 0,
    MZ_ZIP_TYPE_USER,
    MZ_ZIP_TYPE_MEMORY,
    MZ_ZIP_TYPE_HEAP,
    MZ_ZIP_TYPE_FILE,
    MZ_ZIP_TYPE_CFILE,
    MZ_ZIP_TOTAL_TYPES
} mz_zip_type;
typedef enum {
    MZ_ZIP_NO_ERROR = 0,
    MZ_ZIP_UNDEFINED_ERROR,
    MZ_ZIP_TOO_MANY_FILES,
    MZ_ZIP_FILE_TOO_LARGE,
    MZ_ZIP_UNSUPPORTED_METHOD,
    MZ_ZIP_UNSUPPORTED_ENCRYPTION,
    MZ_ZIP_UNSUPPORTED_FEATURE,
    MZ_ZIP_FAILED_FINDING_CENTRAL_DIR,
    MZ_ZIP_NOT_AN_ARCHIVE,
    MZ_ZIP_INVALID_HEADER_OR_CORRUPTED,
    MZ_ZIP_UNSUPPORTED_MULTIDISK,
    MZ_ZIP_DECOMPRESSION_FAILED,
    MZ_ZIP_COMPRESSION_FAILED,
    MZ_ZIP_UNEXPECTED_DECOMPRESSED_SIZE,
    MZ_ZIP_CRC_CHECK_FAILED,
    MZ_ZIP_UNSUPPORTED_CDIR_SIZE,
    MZ_ZIP_ALLOC_FAILED,
    MZ_ZIP_FILE_OPEN_FAILED,
    MZ_ZIP_FILE_CREATE_FAILED,
    MZ_ZIP_FILE_WRITE_FAILED,
    MZ_ZIP_FILE_READ_FAILED,
    MZ_ZIP_FILE_CLOSE_FAILED,
    MZ_ZIP_FILE_SEEK_FAILED,
    MZ_ZIP_FILE_STAT_FAILED,
    MZ_ZIP_INVALID_PARAMETER,
    MZ_ZIP_INVALID_FILENAME,
    MZ_ZIP_BUF_TOO_SMALL,
    MZ_ZIP_INTERNAL_ERROR,
    MZ_ZIP_FILE_NOT_FOUND,
    MZ_ZIP_ARCHIVE_TOO_LARGE,
    MZ_ZIP_VALIDATION_FAILED,
    MZ_ZIP_WRITE_CALLBACK_FAILED,
    MZ_ZIP_TOTAL_ERRORS
} mz_zip_error;
typedef struct {
    mz_uint64 m_archive_size;
    mz_uint64 m_central_directory_file_ofs;
    mz_uint32 m_total_files;
    mz_zip_mode m_zip_mode;
    mz_zip_type m_zip_type;
    mz_zip_error m_last_error;
    mz_uint64 m_file_offset_alignment;
    mz_alloc_func m_pAlloc;
    mz_free_func m_pFree;
    mz_realloc_func m_pRealloc;
    void *m_pAlloc_opaque;
    mz_file_read_func m_pRead;
    mz_file_write_func m_pWrite;
    mz_file_needs_keepalive m_pNeeds_keepalive;
    void *m_pIO_opaque;
    struct mz_zip_internal_state_tag *m_pState;
} mz_zip_archive;
mz_bool mz_zip_reader_init_file(mz_zip_archive *pZip, const char *pFilename, mz_uint32 flags);
mz_bool mz_zip_reader_init_mem(mz_zip_archive *pZip, const void *pMem, size_t size, mz_uint32 flags);
mz_uint mz_zip_reader_get_num_files(mz_zip_archive *pZip);
mz_bool mz_zip_reader_file_stat(mz_zip_archive *pZip, mz_uint file_index, mz_zip_archive_file_stat *pStat);
mz_bool mz_zip_reader_is_file_a_directory(mz_zip_archive *pZip, mz_uint file_index);
mz_bool mz_zip_reader_extract_to_file(mz_zip_archive *pZip, mz_uint file_index, const char *pDst_filename, mz_uint flags);
mz_bool mz_zip_reader_extract_file_to_file(mz_zip_archive *pZip, const char *pArchive_filename, const char *pDst_filename, mz_uint flags);
mz_bool mz_zip_reader_end(mz_zip_archive *pZip);
const char *mz_zip_get_error_string(mz_zip_error mz_err);
#ifdef __cplusplus
}
#endif
#endif
