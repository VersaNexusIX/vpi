#include "vpi.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

static char g_install_dir[VPI_MAX_PATH];
static char g_cache_dir[VPI_MAX_PATH];

static void resolve_dirs(void) {
#ifdef _WIN32
    char appdata[VPI_MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        snprintf(g_install_dir, sizeof(g_install_dir), "%s\\vpi\\packages", appdata);
        snprintf(g_cache_dir, sizeof(g_cache_dir), "%s\\vpi\\cache", appdata);
    } else {
        snprintf(g_install_dir, sizeof(g_install_dir), "C:\\vpi\\packages");
        snprintf(g_cache_dir, sizeof(g_cache_dir), "C:\\vpi\\cache");
    }
#elif defined(__APPLE__)
    const char *home = getenv("HOME");
    if (!home) { struct passwd *pw = getpwuid(getuid()); home = pw ? pw->pw_dir : "/tmp"; }
    snprintf(g_install_dir, sizeof(g_install_dir), "%s/Library/Application Support/vpi/packages", home);
    snprintf(g_cache_dir, sizeof(g_cache_dir), "%s/Library/Caches/vpi", home);
#else
    const char *home = getenv("HOME");
    if (!home) { struct passwd *pw = getpwuid(getuid()); home = pw ? pw->pw_dir : "/tmp"; }
    const char *xdg_data = getenv("XDG_DATA_HOME");
    const char *xdg_cache = getenv("XDG_CACHE_HOME");
    if (xdg_data) snprintf(g_install_dir, sizeof(g_install_dir), "%s/vpi/packages", xdg_data);
    else snprintf(g_install_dir, sizeof(g_install_dir), "%s/.local/share/vpi/packages", home);
    if (xdg_cache) snprintf(g_cache_dir, sizeof(g_cache_dir), "%s/vpi", xdg_cache);
    else snprintf(g_cache_dir, sizeof(g_cache_dir), "%s/.cache/vpi", home);
#endif
}

void vpi_init(void) {
    resolve_dirs();
    dir_create_recursive(g_install_dir);
    dir_create_recursive(g_cache_dir);
}

void vpi_cleanup(void) {}

const char *vpi_get_install_dir(void) { return g_install_dir; }
const char *vpi_get_cache_dir(void) { return g_cache_dir; }
