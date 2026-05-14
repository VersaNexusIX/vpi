#include "registry.h"
#include "utils.h"
#include "vpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

static void get_meta_path(const char *name, char *out, size_t out_sz) {
    char meta_dir[VPI_MAX_PATH];
    path_join(meta_dir, sizeof(meta_dir), vpi_get_install_dir(), ".meta");
    dir_create_recursive(meta_dir);
    char filename[VPI_MAX_NAME + 8];
    snprintf(filename, sizeof(filename), "%s.json", name);
    path_join(out, out_sz, meta_dir, filename);
}

int registry_save(const VpiPackageMeta *meta) {
    char meta_path[VPI_MAX_PATH];
    get_meta_path(meta->name, meta_path, sizeof(meta_path));
    char buf[VPI_MAX_FIELD * 8];
    snprintf(buf, sizeof(buf),
        "{\n"
        "  \"name\": \"%s\",\n"
        "  \"version\": \"%s\",\n"
        "  \"developer\": \"%s\",\n"
        "  \"license\": \"%s\",\n"
        "  \"description\": \"%s\",\n"
        "  \"homepage\": \"%s\",\n"
        "  \"install_date\": \"%s\",\n"
        "  \"installed_size\": %zu\n"
        "}\n",
        meta->name, meta->version, meta->developer,
        meta->license, meta->description, meta->homepage,
        meta->install_date, meta->installed_size);
    return file_write_all(meta_path, buf, strlen(buf));
}

int registry_load(const char *name, VpiPackageMeta *meta) {
    char meta_path[VPI_MAX_PATH];
    get_meta_path(name, meta_path, sizeof(meta_path));
    char *data = NULL;
    size_t len = 0;
    if (file_read_all(meta_path, &data, &len) != 0) return -1;
    memset(meta, 0, sizeof(VpiPackageMeta));
    char *v;
    v = json_get_string(data, "name"); if (v) { strncpy(meta->name, v, sizeof(meta->name)-1); free(v); }
    v = json_get_string(data, "version"); if (v) { strncpy(meta->version, v, sizeof(meta->version)-1); free(v); }
    v = json_get_string(data, "developer"); if (v) { strncpy(meta->developer, v, sizeof(meta->developer)-1); free(v); }
    v = json_get_string(data, "license"); if (v) { strncpy(meta->license, v, sizeof(meta->license)-1); free(v); }
    v = json_get_string(data, "description"); if (v) { strncpy(meta->description, v, sizeof(meta->description)-1); free(v); }
    v = json_get_string(data, "homepage"); if (v) { strncpy(meta->homepage, v, sizeof(meta->homepage)-1); free(v); }
    v = json_get_string(data, "install_date"); if (v) { strncpy(meta->install_date, v, sizeof(meta->install_date)-1); free(v); }
    long sz = json_get_long(data, "installed_size");
    if (sz >= 0) meta->installed_size = (size_t)sz;
    free(data);
    return 0;
}

int registry_remove(const char *name) {
    char meta_path[VPI_MAX_PATH];
    get_meta_path(name, meta_path, sizeof(meta_path));
    return remove(meta_path);
}

int registry_exists(const char *name) {
    char meta_path[VPI_MAX_PATH];
    get_meta_path(name, meta_path, sizeof(meta_path));
    return file_exists(meta_path);
}

int registry_list(VpiPackageMeta **out, int *count) {
    char meta_dir[VPI_MAX_PATH];
    path_join(meta_dir, sizeof(meta_dir), vpi_get_install_dir(), ".meta");
    *count = 0;
    *out = NULL;
    int cap = 16;
    VpiPackageMeta *list = (VpiPackageMeta *)malloc((size_t)cap * sizeof(VpiPackageMeta));
    if (!list) return -1;
#ifdef _WIN32
    char pattern[VPI_MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*.json", meta_dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            char full[VPI_MAX_PATH];
            snprintf(full, sizeof(full), "%s\\%s", meta_dir, fd.cFileName);
            char *data = NULL; size_t len = 0;
            if (file_read_all(full, &data, &len) == 0) {
                if (*count >= cap) { cap *= 2; list = (VpiPackageMeta *)realloc(list, (size_t)cap * sizeof(VpiPackageMeta)); }
                VpiPackageMeta *m = &list[*count];
                memset(m, 0, sizeof(*m));
                char *v;
                v = json_get_string(data, "name"); if (v) { strncpy(m->name, v, sizeof(m->name)-1); free(v); }
                v = json_get_string(data, "version"); if (v) { strncpy(m->version, v, sizeof(m->version)-1); free(v); }
                v = json_get_string(data, "developer"); if (v) { strncpy(m->developer, v, sizeof(m->developer)-1); free(v); }
                v = json_get_string(data, "license"); if (v) { strncpy(m->license, v, sizeof(m->license)-1); free(v); }
                v = json_get_string(data, "description"); if (v) { strncpy(m->description, v, sizeof(m->description)-1); free(v); }
                v = json_get_string(data, "install_date"); if (v) { strncpy(m->install_date, v, sizeof(m->install_date)-1); free(v); }
                long sz = json_get_long(data, "installed_size"); if (sz >= 0) m->installed_size = (size_t)sz;
                (*count)++;
                free(data);
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR *d = opendir(meta_dir);
    if (!d) { free(list); return 0; }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        size_t nlen = strlen(ent->d_name);
        if (nlen < 5 || strcmp(ent->d_name + nlen - 5, ".json") != 0) continue;
        char full[VPI_MAX_PATH];
        path_join(full, sizeof(full), meta_dir, ent->d_name);
        char *data = NULL; size_t len = 0;
        if (file_read_all(full, &data, &len) == 0) {
            if (*count >= cap) { cap *= 2; list = (VpiPackageMeta *)realloc(list, (size_t)cap * sizeof(VpiPackageMeta)); }
            VpiPackageMeta *m = &list[*count];
            memset(m, 0, sizeof(*m));
            char *v;
            v = json_get_string(data, "name"); if (v) { strncpy(m->name, v, sizeof(m->name)-1); free(v); }
            v = json_get_string(data, "version"); if (v) { strncpy(m->version, v, sizeof(m->version)-1); free(v); }
            v = json_get_string(data, "developer"); if (v) { strncpy(m->developer, v, sizeof(m->developer)-1); free(v); }
            v = json_get_string(data, "license"); if (v) { strncpy(m->license, v, sizeof(m->license)-1); free(v); }
            v = json_get_string(data, "description"); if (v) { strncpy(m->description, v, sizeof(m->description)-1); free(v); }
            v = json_get_string(data, "install_date"); if (v) { strncpy(m->install_date, v, sizeof(m->install_date)-1); free(v); }
            long sz = json_get_long(data, "installed_size"); if (sz >= 0) m->installed_size = (size_t)sz;
            (*count)++;
            free(data);
        }
    }
    closedir(d);
#endif
    *out = list;
    return 0;
}

void registry_free_list(VpiPackageMeta *list, int count) {
    (void)count;
    free(list);
}
