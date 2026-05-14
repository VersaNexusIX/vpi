#ifndef UTILS_H
#define UTILS_H
#include "vpi.h"
#include <stddef.h>
void disable_color(void);
int color_enabled(void);
void print_separator(char c, int width);
void print_kv(const char *key, const char *value);
void print_kv_size(const char *key, size_t bytes);
void print_status(const char *status, const char *msg, int type);
void print_progress(const char *label, long downloaded, long total);
char *str_trim(char *s);
char *str_replace(const char *src, const char *from, const char *to);
int path_join(char *out, size_t out_size, const char *a, const char *b);
int dir_exists(const char *path);
int file_exists(const char *path);
int dir_create_recursive(const char *path);
int dir_remove_recursive(const char *path);
size_t dir_size(const char *path);
int file_read_all(const char *path, char **out, size_t *len);
int file_write_all(const char *path, const char *data, size_t len);
char *json_get_string(const char *json, const char *key);
long json_get_long(const char *json, const char *key);
int registry_index_lookup(const char *package_name, const VpiFlags *flags,
                          char *out_filename, size_t fn_sz,
                          char *out_url, size_t url_sz);
char *http_download(const char *url, size_t *out_size, const VpiFlags *flags);
int http_download_file(const char *url, const char *dest_path, const VpiFlags *flags);
int zip_extract(const char *zip_path, const char *dest_dir, const VpiFlags *flags);
char *get_current_datetime(void);
void format_bytes(size_t bytes, char *out, size_t out_sz);
#endif
