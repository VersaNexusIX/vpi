#include "utils.h"
#include "miniz.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir(a,b) _mkdir(a)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

static int ensure_parent_dir(const char *filepath) {
    char tmp[VPI_MAX_PATH];
    strncpy(tmp, filepath, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char save = *p;
            *p = '\0';
            if (!dir_exists(tmp)) {
#ifdef _WIN32
                _mkdir(tmp);
#else
                mkdir(tmp, 0755);
#endif
            }
            *p = save;
        }
    }
    return 0;
}

int zip_extract(const char *zip_path, const char *dest_dir, const VpiFlags *flags) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zip_path, 0)) {
        fprintf(stderr, "\033[1;31mError:\033[0m Failed to open archive: %s\n", zip_path);
        return -1;
    }
    int num_files = (int)mz_zip_reader_get_num_files(&zip);
    int ok = 0;
    for (int i = 0; i < num_files; i++) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, (mz_uint)i, &stat)) continue;
        char dest_path[VPI_MAX_PATH];
        path_join(dest_path, sizeof(dest_path), dest_dir, stat.m_filename);
        for (char *p = dest_path; *p; p++) {
            if (*p == '\\') *p = '/';
        }
        if (mz_zip_reader_is_file_a_directory(&zip, (mz_uint)i)) {
            if (!dir_exists(dest_path)) dir_create_recursive(dest_path);
            continue;
        }
        ensure_parent_dir(dest_path);
        if (flags && flags->verbose) {
            printf("  \033[0;90mExtracting:\033[0m %s\n", stat.m_filename);
        }
        if (!mz_zip_reader_extract_to_file(&zip, (mz_uint)i, dest_path, 0)) {
            fprintf(stderr, "\033[1;31mError:\033[0m Failed to extract: %s\n", stat.m_filename);
            ok = -1;
        }
    }
    mz_zip_reader_end(&zip);
    return ok;
}
