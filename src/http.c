#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
    const VpiFlags *flags;
    long content_length;
    const char *label;
} CurlBuffer;

static size_t curl_write_memory(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real = size * nmemb;
    CurlBuffer *buf = (CurlBuffer *)userp;
    if (buf->size + real + 1 > buf->capacity) {
        size_t new_cap = buf->capacity == 0 ? 65536 : buf->capacity * 2;
        while (new_cap < buf->size + real + 1) new_cap *= 2;
        char *tmp = (char *)realloc(buf->data, new_cap);
        if (!tmp) return 0;
        buf->data = tmp;
        buf->capacity = new_cap;
    }
    memcpy(buf->data + buf->size, contents, real);
    buf->size += real;
    buf->data[buf->size] = '\0';
    return real;
}

static size_t curl_write_file(void *contents, size_t size, size_t nmemb, void *userp) {
    CurlBuffer *buf = (CurlBuffer *)userp;
    size_t real = size * nmemb;
    FILE *f = (FILE *)(buf->data);
    size_t written = fwrite(contents, 1, real, f);
    buf->size += written;
    if (buf->flags && !buf->flags->dry_run) {
        print_progress(buf->label ? buf->label : "Downloading", (long)buf->size, buf->content_length);
    }
    return written;
}

static int curl_progress_cb(void *userp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ult, curl_off_t uln) {
    (void)ult; (void)uln;
    CurlBuffer *buf = (CurlBuffer *)userp;
    if (buf->content_length <= 0 && dltotal > 0) buf->content_length = (long)dltotal;
    return 0;
}

char *http_download(const char *url, size_t *out_size, const VpiFlags *flags) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;
    CurlBuffer buf = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "vpi/" VPI_VERSION " (Versa Package Installer)");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    if (flags && flags->verbose) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "\n\033[1;31mNetwork error:\033[0m %s\n", curl_easy_strerror(res));
        free(buf.data);
        return NULL;
    }
    if (http_code != 200) {
        fprintf(stderr, "\n\033[1;31mHTTP error:\033[0m Server returned %ld for %s\n", http_code, url);
        free(buf.data);
        return NULL;
    }
    if (out_size) *out_size = buf.size;
    return buf.data;
}

int http_download_file(const char *url, const char *dest_path, const VpiFlags *flags) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;
    FILE *f = fopen(dest_path, "wb");
    if (!f) { curl_easy_cleanup(curl); return -1; }
    CurlBuffer ctx = {0};
    ctx.data = (char *)f;
    ctx.flags = flags;
    ctx.label = "Downloading";
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_progress_cb);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "vpi/" VPI_VERSION " (Versa Package Installer)");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    if (flags && flags->verbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    fclose(f);
    printf("\n");
    if (res != CURLE_OK) {
        fprintf(stderr, "\033[1;31mNetwork error:\033[0m %s\n", curl_easy_strerror(res));
        remove(dest_path);
        return -1;
    }
    if (http_code != 200) {
        fprintf(stderr, "\033[1;31mHTTP %ld:\033[0m Package not found on server.\n", http_code);
        remove(dest_path);
        return -1;
    }
    return 0;
}
