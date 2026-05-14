#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#define mkdir(a,b) _mkdir(a)
#define access _access
#define F_OK 0
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#endif

static int g_color_enabled = 1;
void disable_color(void) { g_color_enabled = 0; }
int color_enabled(void) { return g_color_enabled; }

static const char *C_RESET  = "\033[0m";
static const char *C_RED    = "\033[1;31m";
static const char *C_GREEN  = "\033[1;32m";
static const char *C_YELLOW = "\033[1;33m";
static const char *C_CYAN   = "\033[1;36m";
static const char *C_GRAY   = "\033[0;90m";
static const char *C_WHITE  = "\033[1;37m";

void print_separator(char c, int width) {
    if (g_color_enabled) printf("%s", C_GRAY);
    for (int i = 0; i < width; i++) putchar(c);
    if (g_color_enabled) printf("%s", C_RESET);
    putchar('\n');
}

void print_kv(const char *key, const char *value) {
    if (!value || value[0] == '\0') return;
    if (g_color_enabled)
        printf("  %s%-16s%s %s\n", C_CYAN, key, C_RESET, value);
    else
        printf("  %-16s %s\n", key, value);
}

void format_bytes(size_t bytes, char *out, size_t out_sz) {
    if (bytes >= 1073741824) snprintf(out, out_sz, "%.2f GB", (double)bytes / 1073741824.0);
    else if (bytes >= 1048576) snprintf(out, out_sz, "%.2f MB", (double)bytes / 1048576.0);
    else if (bytes >= 1024) snprintf(out, out_sz, "%.2f KB", (double)bytes / 1024.0);
    else snprintf(out, out_sz, "%zu B", bytes);
}

void print_kv_size(const char *key, size_t bytes) {
    char buf[64];
    format_bytes(bytes, buf, sizeof(buf));
    print_kv(key, buf);
}

void print_status(const char *status, const char *msg, int type) {
    const char *color = C_WHITE;
    if (!g_color_enabled) { printf("[%s] %s\n", status, msg); return; }
    if (type == 0) color = C_GREEN;
    else if (type == 1) color = C_YELLOW;
    else if (type == 2) color = C_RED;
    else if (type == 3) color = C_CYAN;
    printf("%s[%s]%s %s\n", color, status, C_RESET, msg);
}

void print_progress(const char *label, long downloaded, long total) {
    if (total <= 0) {
        char sz[64]; format_bytes((size_t)downloaded, sz, sizeof(sz));
        printf("\r  %s%-20s%s %s downloaded...", g_color_enabled ? C_CYAN : "", label, g_color_enabled ? C_RESET : "", sz);
        fflush(stdout); return;
    }
    int pct = (int)((double)downloaded / (double)total * 100.0);
    int bar_w = 30;
    int filled = (int)((double)bar_w * pct / 100.0);
    char dz[64], tz[64];
    format_bytes((size_t)downloaded, dz, sizeof(dz));
    format_bytes((size_t)total, tz, sizeof(tz));
    printf("\r  %s%-12s%s [", g_color_enabled ? C_CYAN : "", label, g_color_enabled ? C_RESET : "");
    if (g_color_enabled) printf("%s", C_GREEN);
    for (int i = 0; i < filled; i++) putchar('#');
    if (g_color_enabled) printf("%s", C_GRAY);
    for (int i = filled; i < bar_w; i++) putchar('-');
    if (g_color_enabled) printf("%s", C_RESET);
    printf("] %3d%% %s/%s", pct, dz, tz);
    fflush(stdout);
}

char *str_trim(char *s) {
    if (!s) return s;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

char *str_replace(const char *src, const char *from, const char *to) {
    size_t from_len = strlen(from), to_len = strlen(to);
    int count = 0;
    const char *p = src;
    while ((p = strstr(p, from)) != NULL) { count++; p += from_len; }
    size_t new_len = strlen(src) + count * (to_len - from_len) + 1;
    char *result = (char *)malloc(new_len);
    if (!result) return NULL;
    char *q = result;
    p = src;
    while (*p) {
        if (strncmp(p, from, from_len) == 0) { memcpy(q, to, to_len); q += to_len; p += from_len; }
        else *q++ = *p++;
    }
    *q = '\0';
    return result;
}

int path_join(char *out, size_t out_size, const char *a, const char *b) {
    size_t la = strlen(a);
    char sep = '/';
#ifdef _WIN32
    sep = '\\';
#endif
    if (la > 0 && (a[la-1] == '/' || a[la-1] == '\\')) la--;
    int n = snprintf(out, out_size, "%.*s%c%s", (int)la, a, sep, b);
    return (n > 0 && (size_t)n < out_size) ? 0 : -1;
}

int dir_exists(const char *path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
#endif
}

int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int dir_create_recursive(const char *path) {
    char tmp[VPI_MAX_PATH];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp)-1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            if (!dir_exists(tmp)) {
#ifdef _WIN32
                _mkdir(tmp);
#else
                mkdir(tmp, 0755);
#endif
            }
            *p = '/';
        }
    }
#ifdef _WIN32
    return _mkdir(tmp);
#else
    return mkdir(tmp, 0755);
#endif
}

#ifdef _WIN32
static int rmdir_recursive_win(const char *path) {
    char pattern[VPI_MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
            char full[VPI_MAX_PATH];
            snprintf(full, sizeof(full), "%s\\%s", path, fd.cFileName);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) rmdir_recursive_win(full);
            else DeleteFileA(full);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    return RemoveDirectoryA(path) ? 0 : -1;
}
#else
static int rmdir_recursive_posix(const char *path) {
    DIR *d = opendir(path);
    if (!d) return -1;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char full[VPI_MAX_PATH];
        path_join(full, sizeof(full), path, ent->d_name);
        struct stat st;
        if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) rmdir_recursive_posix(full);
            else unlink(full);
        }
    }
    closedir(d);
    return rmdir(path);
}
#endif

int dir_remove_recursive(const char *path) {
#ifdef _WIN32
    return rmdir_recursive_win(path);
#else
    return rmdir_recursive_posix(path);
#endif
}

size_t dir_size(const char *path) {
    size_t total = 0;
#ifdef _WIN32
    char pattern[VPI_MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
            char full[VPI_MAX_PATH];
            snprintf(full, sizeof(full), "%s\\%s", path, fd.cFileName);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) total += dir_size(full);
            else total += ((size_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR *d = opendir(path);
    if (!d) return 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char full[VPI_MAX_PATH];
        path_join(full, sizeof(full), path, ent->d_name);
        struct stat st;
        if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) total += dir_size(full);
            else total += (size_t)st.st_size;
        }
    }
    closedir(d);
#endif
    return total;
}

int file_read_all(const char *path, char **out, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return -1; }
    *out = (char *)malloc((size_t)sz + 1);
    if (!*out) { fclose(f); return -1; }
    *len = fread(*out, 1, (size_t)sz, f);
    (*out)[*len] = '\0';
    fclose(f);
    return 0;
}

int file_write_all(const char *path, const char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t w = fwrite(data, 1, len, f);
    fclose(f);
    return (w == len) ? 0 : -1;
}

char *json_get_string(const char *json, const char *key) {
    if (!json || !key) return NULL;
    char search[VPI_MAX_NAME + 4];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(json, search);
    if (!pos) return NULL;
    pos += strlen(search);
    while (*pos == ' ' || *pos == '\t' || *pos == ':' || *pos == ' ') pos++;
    if (*pos != '"') return NULL;
    pos++;
    const char *start = pos;
    while (*pos && *pos != '"') {
        if (*pos == '\\') pos++;
        pos++;
    }
    size_t len = (size_t)(pos - start);
    char *result = (char *)malloc(len + 1);
    if (!result) return NULL;
    size_t ri = 0;
    for (size_t i = 0; i < len; i++) {
        if (start[i] == '\\' && i + 1 < len) {
            i++;
            if (start[i] == 'n') result[ri++] = '\n';
            else if (start[i] == 't') result[ri++] = '\t';
            else result[ri++] = start[i];
        } else {
            result[ri++] = start[i];
        }
    }
    result[ri] = '\0';
    return result;
}

long json_get_long(const char *json, const char *key) {
    if (!json || !key) return -1;
    char search[VPI_MAX_NAME + 4];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(json, search);
    if (!pos) return -1;
    pos += strlen(search);
    while (*pos == ' ' || *pos == ':') pos++;
    if (!isdigit((unsigned char)*pos) && *pos != '-') return -1;
    return atol(pos);
}

char *get_current_datetime(void) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char *buf = (char *)malloc(32);
    if (!buf) return NULL;
    strftime(buf, 32, "%Y-%m-%d %H:%M:%S", tm_info);
    return buf;
}

int registry_index_lookup(const char *package_name, const VpiFlags *flags,
                          char *out_filename, size_t fn_sz,
                          char *out_url, size_t url_sz) {
    printf("\033[1;34m[idx]\033[0m Fetching registry index: \033[0;90m%s\033[0m\n", VPI_REGISTRY_INDEX);
    size_t sz = 0;
    char *data = http_download(VPI_REGISTRY_INDEX, &sz, flags);
    if (!data) {
        fprintf(stderr, "\033[1;31mError:\033[0m Failed to fetch registry index.\n");
        return -1;
    }
    const char *p = data;
    int found = 0;
    while ((p = strstr(p, "\"nama\"")) != NULL) {
        char *nama = json_get_string(p, "nama");
        if (!nama) { p++; continue; }
        if (strcmp(nama, package_name) != 0) {
            free(nama);
            const char *next = strstr(p + 1, "{");
            if (!next) break;
            p = next;
            continue;
        }
        free(nama);
        char *filename = json_get_string(p, "_filename");
        if (!filename) {
            fprintf(stderr, "\033[1;31mError:\033[0m Package found in index but missing '_filename'.\n");
            free(data);
            return -1;
        }
        strncpy(out_filename, filename, fn_sz - 1);
        out_filename[fn_sz - 1] = '\0';
        free(filename);
        snprintf(out_url, url_sz, "%s%s", VPI_PACKAGE_BASE_URL, out_filename);
        found = 1;
        break;
    }
    free(data);
    if (!found) {
        fprintf(stderr, "\033[1;31mError:\033[0m Package '%s' not found in registry index.\n", package_name);
        return -1;
    }
    return 0;
}
