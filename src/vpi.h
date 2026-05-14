#ifndef VPI_H
#define VPI_H
#include <stddef.h>
#define VPI_VERSION "1.0.0"
#define VPI_REGISTRY_BASE    "https://versas.my.id"
#define VPI_REGISTRY_INDEX   "https://versas.my.id/vers.json"
#define VPI_PACKAGE_BASE_URL "https://versas.my.id/package/"
#define VPI_PACKAGE_URL_FMT  "https://versas.my.id/package/%s.vers"
#define VPI_SEARCH_URL_FMT   "https://versas.my.id/api/search?q=%s"
#define VPI_INFO_URL_FMT     "https://versas.my.id/api/package/%s"
#define VPI_MAX_PATH 4096
#define VPI_MAX_NAME 256
#define VPI_MAX_VERSION 64
#define VPI_MAX_FIELD 1024
typedef struct {
    int verbose;
    int no_color;
    int force;
    int dry_run;
} VpiFlags;
typedef struct {
    char name[VPI_MAX_NAME];
    char version[VPI_MAX_VERSION];
    char developer[VPI_MAX_FIELD];
    char license[VPI_MAX_FIELD];
    char description[VPI_MAX_FIELD];
    char homepage[VPI_MAX_FIELD];
    char install_date[VPI_MAX_FIELD];
    size_t installed_size;
} VpiPackageMeta;
void vpi_init(void);
void vpi_cleanup(void);
const char *vpi_get_install_dir(void);
const char *vpi_get_cache_dir(void);
#endif
