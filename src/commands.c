#include "commands.h"
#include "utils.h"
#include "registry.h"
#include "vpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

static void print_meta_box(const VpiPackageMeta *meta) {
    print_separator('=', 52);
    printf("  \033[1;36m%-14s\033[0m \033[1;37m%s\033[0m\n", "Package:", meta->name);
    printf("  \033[1;36m%-14s\033[0m %s\n", "Version:", meta->version);
    if (meta->developer[0]) printf("  \033[1;36m%-14s\033[0m %s\n", "Developer:", meta->developer);
    if (meta->license[0])   printf("  \033[1;36m%-14s\033[0m %s\n", "License:", meta->license);
    if (meta->description[0]) printf("  \033[1;36m%-14s\033[0m %s\n", "Description:", meta->description);
    if (meta->homepage[0])  printf("  \033[1;36m%-14s\033[0m %s\n", "Homepage:", meta->homepage);
    if (meta->install_date[0]) printf("  \033[1;36m%-14s\033[0m %s\n", "Installed:", meta->install_date);
    if (meta->installed_size > 0) {
        char sz[64]; format_bytes(meta->installed_size, sz, sizeof(sz));
        printf("  \033[1;36m%-14s\033[0m %s\n", "Size:", sz);
    }
    print_separator('=', 52);
}

static int find_vers_json(const char *extract_dir, char *out, size_t out_sz) {
    char root_path[VPI_MAX_PATH];
    path_join(root_path, sizeof(root_path), extract_dir, "vers.json");
    if (file_exists(root_path)) {
        strncpy(out, root_path, out_sz - 1);
        out[out_sz - 1] = '\0';
        return 0;
    }
#ifdef _WIN32
    char pattern[VPI_MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", extract_dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            char sub[VPI_MAX_PATH], candidate[VPI_MAX_PATH];
            snprintf(sub, sizeof(sub), "%s\\%s", extract_dir, fd.cFileName);
            snprintf(candidate, sizeof(candidate), "%s\\vers.json", sub);
            if (file_exists(candidate)) {
                strncpy(out, candidate, out_sz - 1);
                out[out_sz - 1] = '\0';
                FindClose(h);
                return 0;
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR *d = opendir(extract_dir);
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            char sub[VPI_MAX_PATH], candidate[VPI_MAX_PATH];
            path_join(sub, sizeof(sub), extract_dir, ent->d_name);
            path_join(candidate, sizeof(candidate), sub, "vers.json");
            struct stat st;
            if (stat(sub, &st) == 0 && S_ISDIR(st.st_mode) && file_exists(candidate)) {
                strncpy(out, candidate, out_sz - 1);
                out[out_sz - 1] = '\0';
                closedir(d);
                return 0;
            }
        }
        closedir(d);
    }
#endif
    return -1;
}

static int parse_vers_json(const char *pkg_dir, VpiPackageMeta *meta) {
    char json_path[VPI_MAX_PATH];
    if (find_vers_json(pkg_dir, json_path, sizeof(json_path)) != 0) {
        fprintf(stderr, "\033[1;31mError:\033[0m vers.json not found inside package archive.\n");
        fprintf(stderr, "       A valid .vers package MUST contain vers.json at root or one level deep.\n");
        return -1;
    }
    char *data = NULL;
    size_t len = 0;
    if (file_read_all(json_path, &data, &len) != 0) {
        fprintf(stderr, "\033[1;31mError:\033[0m Failed to read vers.json.\n");
        return -1;
    }
    char *v;
    v = json_get_string(data, "nama");
    if (!v) v = json_get_string(data, "name");
    if (!v) { fprintf(stderr, "\033[1;31mError:\033[0m vers.json missing required field: 'nama' (name).\n"); free(data); return -1; }
    strncpy(meta->name, v, sizeof(meta->name)-1); free(v);
    v = json_get_string(data, "versi");
    if (!v) v = json_get_string(data, "version");
    if (!v) { fprintf(stderr, "\033[1;31mError:\033[0m vers.json missing required field: 'versi' (version).\n"); free(data); return -1; }
    strncpy(meta->version, v, sizeof(meta->version)-1); free(v);
    v = json_get_string(data, "developer");
    if (!v) v = json_get_string(data, "author");
    if (v) { strncpy(meta->developer, v, sizeof(meta->developer)-1); free(v); }
    else { fprintf(stderr, "\033[1;31mError:\033[0m vers.json missing required field: 'developer'.\n"); free(data); return -1; }
    v = json_get_string(data, "lisensi");
    if (!v) v = json_get_string(data, "license");
    if (v) { strncpy(meta->license, v, sizeof(meta->license)-1); free(v); }
    else { fprintf(stderr, "\033[1;31mError:\033[0m vers.json missing required field: 'lisensi' (license).\n"); free(data); return -1; }
    v = json_get_string(data, "description");
    if (!v) v = json_get_string(data, "deskripsi");
    if (v) { strncpy(meta->description, v, sizeof(meta->description)-1); free(v); }
    v = json_get_string(data, "homepage");
    if (!v) v = json_get_string(data, "website");
    if (v) { strncpy(meta->homepage, v, sizeof(meta->homepage)-1); free(v); }
    free(data);
    return 0;
}

int cmd_install(const char *package_name, const VpiFlags *flags) {
    printf("\033[1;37m==> Installing package:\033[0m \033[1;36m%s\033[0m\n", package_name);
    printf("    Registry: \033[0;90m%s\033[0m\n\n", VPI_REGISTRY_BASE);
    if (registry_exists(package_name)) {
        VpiPackageMeta existing;
        registry_load(package_name, &existing);
        if (!flags->force) {
            printf("\033[1;33m[WARN]\033[0m Package '%s' v%s is already installed.\n", package_name, existing.version);
            printf("       Use 'vpi update %s' to upgrade, or '--force' to reinstall.\n", package_name);
            return 1;
        }
        printf("\033[1;33m[WARN]\033[0m Reinstalling (--force)...\n");
    }
    char vers_filename[VPI_MAX_NAME + 8];
    char url[VPI_MAX_PATH];
    if (registry_index_lookup(package_name, flags, vers_filename, sizeof(vers_filename), url, sizeof(url)) != 0) {
        fprintf(stderr, "\033[1;33m[WARN]\033[0m Registry index lookup failed. Falling back to default URL.\n");
        snprintf(vers_filename, sizeof(vers_filename), "%s.vers", package_name);
        snprintf(url, sizeof(url), VPI_PACKAGE_URL_FMT, package_name);
    }
    printf("\033[1;34m[INFO]\033[0m Fetching from: \033[0;90m%s\033[0m\n", url);
    char cache_path[VPI_MAX_PATH];
    path_join(cache_path, sizeof(cache_path), vpi_get_cache_dir(), vers_filename);
    if (flags->dry_run) {
        printf("\033[1;33m[DRY-RUN]\033[0m Would download: %s\n", url);
        printf("\033[1;33m[DRY-RUN]\033[0m Would extract to: %s/%s\n", vpi_get_install_dir(), package_name);
        return 0;
    }
    printf("\033[1;34m[1/4]\033[0m Downloading .vers package...\n");
    if (http_download_file(url, cache_path, flags) != 0) {
        fprintf(stderr, "\033[1;31mFailed:\033[0m Could not download '%s' from registry.\n", package_name);
        fprintf(stderr, "        URL attempted: %s\n", url);
        return 1;
    }
    print_status("OK", "Download complete.", 0);
    char extract_tmp[VPI_MAX_PATH];
    char tmp_dir_name[VPI_MAX_NAME + 16];
    snprintf(tmp_dir_name, sizeof(tmp_dir_name), ".tmp_%s", package_name);
    path_join(extract_tmp, sizeof(extract_tmp), vpi_get_cache_dir(), tmp_dir_name);
    dir_create_recursive(extract_tmp);
    printf("\033[1;34m[2/4]\033[0m Extracting archive...\n");
    if (zip_extract(cache_path, extract_tmp, flags) != 0) {
        fprintf(stderr, "\033[1;31mFailed:\033[0m Archive extraction failed. File may be corrupt.\n");
        dir_remove_recursive(extract_tmp);
        remove(cache_path);
        return 1;
    }
    print_status("OK", "Extraction complete.", 0);
    printf("\033[1;34m[3/4]\033[0m Validating vers.json metadata...\n");
    VpiPackageMeta meta;
    memset(&meta, 0, sizeof(meta));
    if (parse_vers_json(extract_tmp, &meta) != 0) {
        dir_remove_recursive(extract_tmp);
        remove(cache_path);
        return 1;
    }
    printf("  \033[1;32m✓\033[0m Name:      %s\n", meta.name);
    printf("  \033[1;32m✓\033[0m Version:   %s\n", meta.version);
    printf("  \033[1;32m✓\033[0m Developer: %s\n", meta.developer);
    printf("  \033[1;32m✓\033[0m License:   %s\n", meta.license);
    print_status("OK", "Metadata verified.", 0);
    printf("\033[1;34m[4/4]\033[0m Installing files...\n");
    const char *install_name = meta.name[0] ? meta.name : package_name;
    char install_path[VPI_MAX_PATH];
    path_join(install_path, sizeof(install_path), vpi_get_install_dir(), install_name);
    if (dir_exists(install_path)) dir_remove_recursive(install_path);
    dir_create_recursive(install_path);
#ifdef _WIN32
    char mv_cmd[VPI_MAX_PATH * 2 + 32];
    snprintf(mv_cmd, sizeof(mv_cmd), "xcopy /E /I /Q \"%s\" \"%s\" >nul 2>&1", extract_tmp, install_path);
    system(mv_cmd);
    dir_remove_recursive(extract_tmp);
#else
    char mv_cmd[VPI_MAX_PATH * 2 + 32];
    snprintf(mv_cmd, sizeof(mv_cmd), "cp -a \"%s/.\" \"%s/\"", extract_tmp, install_path);
    system(mv_cmd);
    dir_remove_recursive(extract_tmp);
#endif
    char *dt = get_current_datetime();
    if (dt) { strncpy(meta.install_date, dt, sizeof(meta.install_date)-1); free(dt); }
    meta.installed_size = dir_size(install_path);
    if (meta.name[0] == '\0') strncpy(meta.name, install_name, sizeof(meta.name)-1);
    registry_save(&meta);
    remove(cache_path);
    printf("\n");
    print_status("SUCCESS", "Package installed successfully!", 0);
    printf("\n");
    print_meta_box(&meta);
    printf("\n  Install path: \033[0;90m%s\033[0m\n\n", install_path);
    return 0;
}

int cmd_remove(const char *package_name, const VpiFlags *flags) {
    if (!registry_exists(package_name)) {
        fprintf(stderr, "\033[1;31mError:\033[0m Package '%s' is not installed.\n", package_name);
        return 1;
    }
    VpiPackageMeta meta;
    registry_load(package_name, &meta);
    printf("\033[1;37m==> Removing package:\033[0m \033[1;36m%s\033[0m v%s\n\n", meta.name, meta.version);
    if (!flags->force) {
        printf("  Developer : %s\n", meta.developer);
        printf("  License   : %s\n", meta.license);
        char sz[64]; format_bytes(meta.installed_size, sz, sizeof(sz));
        printf("  Size      : %s\n\n", sz);
        printf("Are you sure you want to remove this package? [y/N]: ");
        int c = getchar();
        if (c != 'y' && c != 'Y') { printf("\nAborted.\n"); return 0; }
        printf("\n");
    }
    if (flags->dry_run) {
        printf("\033[1;33m[DRY-RUN]\033[0m Would remove: %s/%s\n", vpi_get_install_dir(), package_name);
        return 0;
    }
    char install_path[VPI_MAX_PATH];
    path_join(install_path, sizeof(install_path), vpi_get_install_dir(), package_name);
    printf("\033[1;34m[1/2]\033[0m Removing files from: \033[0;90m%s\033[0m\n", install_path);
    if (dir_remove_recursive(install_path) != 0) {
        fprintf(stderr, "\033[1;31mWarning:\033[0m Could not fully remove install directory.\n");
    }
    printf("\033[1;34m[2/2]\033[0m Cleaning registry entry...\n");
    registry_remove(package_name);
    print_status("SUCCESS", "Package removed successfully.", 0);
    printf("\n");
    return 0;
}

int cmd_update(const char *package_name, const VpiFlags *flags) {
    if (!registry_exists(package_name)) {
        fprintf(stderr, "\033[1;31mError:\033[0m Package '%s' is not installed. Use 'vpi install %s' first.\n", package_name, package_name);
        return 1;
    }
    VpiPackageMeta old_meta;
    registry_load(package_name, &old_meta);
    printf("\033[1;37m==> Updating package:\033[0m \033[1;36m%s\033[0m\n", package_name);
    printf("    Current version: \033[1;33m%s\033[0m\n\n", old_meta.version);
    VpiFlags force_flags = *flags;
    force_flags.force = 1;
    int ret = cmd_install(package_name, &force_flags);
    if (ret == 0) {
        VpiPackageMeta new_meta;
        if (registry_load(package_name, &new_meta) == 0) {
            if (strcmp(old_meta.version, new_meta.version) == 0) {
                printf("\033[1;33m[INFO]\033[0m Already at latest version (%s). No update needed.\n", new_meta.version);
            } else {
                printf("\033[1;32m[UPDATE]\033[0m %s -> %s\n", old_meta.version, new_meta.version);
            }
        }
    }
    return ret;
}

int cmd_list(const VpiFlags *flags) {
    VpiPackageMeta *list = NULL;
    int count = 0;
    registry_list(&list, &count);
    printf("\033[1;37m==> Installed Packages\033[0m \033[0;90m(%s)\033[0m\n\n", vpi_get_install_dir());
    if (count == 0) {
        printf("  No packages installed yet.\n");
        printf("  Use '\033[1;36mvpi install <package>\033[0m' to install from versas.my.id\n\n");
        return 0;
    }
    print_separator('-', 72);
    printf("  \033[1;37m%-24s %-12s %-16s %s\033[0m\n", "Package", "Version", "Developer", "Size");
    print_separator('-', 72);
    size_t total_size = 0;
    for (int i = 0; i < count; i++) {
        char sz[32];
        format_bytes(list[i].installed_size, sz, sizeof(sz));
        printf("  \033[1;36m%-24s\033[0m %-12s %-16s \033[0;90m%s\033[0m\n",
            list[i].name, list[i].version, list[i].developer, sz);
        total_size += list[i].installed_size;
        if (flags->verbose && list[i].license[0]) {
            printf("    \033[0;90mLicense: %s  |  Installed: %s\033[0m\n", list[i].license, list[i].install_date);
        }
    }
    print_separator('-', 72);
    char total_sz[64];
    format_bytes(total_size, total_sz, sizeof(total_sz));
    printf("  \033[1;37mTotal: %d package%s  |  %s\033[0m\n\n", count, count == 1 ? "" : "s", total_sz);
    registry_free_list(list, count);
    return 0;
}

int cmd_view(const char *package_name, const VpiFlags *flags) {
    if (!registry_exists(package_name)) {
        fprintf(stderr, "\033[1;31mError:\033[0m Package '%s' is not installed.\n", package_name);
        fprintf(stderr, "       Run 'vpi list' to see installed packages.\n");
        return 1;
    }
    VpiPackageMeta meta;
    if (registry_load(package_name, &meta) != 0) {
        fprintf(stderr, "\033[1;31mError:\033[0m Failed to load metadata for '%s'.\n", package_name);
        return 1;
    }
    printf("\n\033[1;37m  Package Details\033[0m\n");
    print_meta_box(&meta);
    char install_path[VPI_MAX_PATH];
    path_join(install_path, sizeof(install_path), vpi_get_install_dir(), package_name);
    printf("\n");
    print_kv("Install Path:", install_path);
    print_kv("Registry:", VPI_REGISTRY_BASE);
    if (flags->verbose) {
        printf("\n  \033[1;37mInstalled Files:\033[0m\n");
#ifdef _WIN32
        char cmd[VPI_MAX_PATH + 32];
        snprintf(cmd, sizeof(cmd), "dir /B /S \"%s\"", install_path);
        system(cmd);
#else
        char cmd[VPI_MAX_PATH + 32];
        snprintf(cmd, sizeof(cmd), "find \"%s\" -type f | sed 's|%s/||'", install_path, install_path);
        system(cmd);
#endif
    }
    printf("\n");
    return 0;
}

int cmd_search(const char *query, const VpiFlags *flags) {
    printf("\033[1;37m==> Searching:\033[0m \033[1;36m%s\033[0m\n", query);
    printf("    Registry: \033[0;90m%s\033[0m\n\n", VPI_REGISTRY_BASE);
    char url[VPI_MAX_PATH];
    snprintf(url, sizeof(url), VPI_SEARCH_URL_FMT, query);
    if (flags->verbose) printf("  Querying: %s\n", url);
    size_t sz = 0;
    char *response = http_download(url, &sz, flags);
    if (!response) {
        fprintf(stderr, "\033[1;31mError:\033[0m Search request failed.\n");
        fprintf(stderr, "       Check your internet connection or visit %s\n", VPI_REGISTRY_BASE);
        return 1;
    }
    const char *results = strstr(response, "\"results\"");
    if (!results || strstr(response, "\"results\":[]") != NULL) {
        printf("  No packages found matching '\033[1;36m%s\033[0m'.\n\n", query);
        free(response);
        return 0;
    }
    print_separator('-', 64);
    printf("  \033[1;37m%-22s %-12s %s\033[0m\n", "Package", "Version", "Developer");
    print_separator('-', 64);
    const char *p = results;
    int found = 0;
    while ((p = strstr(p, "\"name\"")) != NULL) {
        VpiPackageMeta m; memset(&m, 0, sizeof(m));
        char *v;
        v = json_get_string(p, "name"); if (v) { strncpy(m.name, v, sizeof(m.name)-1); free(v); }
        v = json_get_string(p, "version"); if (v) { strncpy(m.version, v, sizeof(m.version)-1); free(v); }
        v = json_get_string(p, "developer"); if (v) { strncpy(m.developer, v, sizeof(m.developer)-1); free(v); }
        v = json_get_string(p, "description"); if (v) { strncpy(m.description, v, sizeof(m.description)-1); free(v); }
        if (m.name[0]) {
            int inst = registry_exists(m.name);
            printf("  \033[1;36m%-22s\033[0m %-12s %s", m.name, m.version, m.developer);
            if (inst) printf(" \033[1;32m[installed]\033[0m");
            printf("\n");
            if (flags->verbose && m.description[0]) printf("    \033[0;90m%s\033[0m\n", m.description);
            found++;
        }
        p += 6;
    }
    print_separator('-', 64);
    if (found > 0) printf("  Found %d package%s. Install with: \033[1;36mvpi install <name>\033[0m\n\n", found, found == 1 ? "" : "s");
    free(response);
    return 0;
}

int cmd_info(const char *package_name, const VpiFlags *flags) {
    printf("\033[1;37m==> Remote info:\033[0m \033[1;36m%s\033[0m\n", package_name);
    printf("    Source: \033[0;90m" VPI_PACKAGE_URL_FMT "\033[0m\n\n", package_name);
    char url[VPI_MAX_PATH];
    snprintf(url, sizeof(url), VPI_INFO_URL_FMT, package_name);
    size_t sz = 0;
    char *response = http_download(url, &sz, flags);
    if (!response) {
        printf("  \033[1;33m[NOTE]\033[0m API endpoint not available. Attempting package probe...\n");
        char vers_url[VPI_MAX_PATH];
        snprintf(vers_url, sizeof(vers_url), VPI_PACKAGE_URL_FMT, package_name);
        char cache_path[VPI_MAX_PATH];
        char tmp_name[VPI_MAX_NAME + 16];
        snprintf(tmp_name, sizeof(tmp_name), ".probe_%s.vers", package_name);
        path_join(cache_path, sizeof(cache_path), vpi_get_cache_dir(), tmp_name);
        if (http_download_file(vers_url, cache_path, flags) != 0) {
            fprintf(stderr, "\033[1;31mError:\033[0m Package '%s' not found on registry.\n", package_name);
            return 1;
        }
        char extract_tmp[VPI_MAX_PATH];
        char tmp_dir[VPI_MAX_NAME + 16];
        snprintf(tmp_dir, sizeof(tmp_dir), ".probe_dir_%s", package_name);
        path_join(extract_tmp, sizeof(extract_tmp), vpi_get_cache_dir(), tmp_dir);
        dir_create_recursive(extract_tmp);
        if (zip_extract(cache_path, extract_tmp, flags) == 0) {
            VpiPackageMeta meta; memset(&meta, 0, sizeof(meta));
            if (parse_vers_json(extract_tmp, &meta) == 0) {
                printf("\n  \033[1;37mRemote Package Metadata:\033[0m\n");
                print_meta_box(&meta);
                int inst = registry_exists(package_name);
                if (inst) printf("  \033[1;32m[INSTALLED]\033[0m This package is currently installed.\n");
                else printf("  Install with: \033[1;36mvpi install %s\033[0m\n", package_name);
            }
        }
        dir_remove_recursive(extract_tmp);
        remove(cache_path);
        return 0;
    }
    VpiPackageMeta meta; memset(&meta, 0, sizeof(meta));
    char *v;
    v = json_get_string(response, "name"); if (v) { strncpy(meta.name, v, sizeof(meta.name)-1); free(v); }
    v = json_get_string(response, "version"); if (v) { strncpy(meta.version, v, sizeof(meta.version)-1); free(v); }
    v = json_get_string(response, "developer"); if (v) { strncpy(meta.developer, v, sizeof(meta.developer)-1); free(v); }
    v = json_get_string(response, "license"); if (v) { strncpy(meta.license, v, sizeof(meta.license)-1); free(v); }
    v = json_get_string(response, "description"); if (v) { strncpy(meta.description, v, sizeof(meta.description)-1); free(v); }
    v = json_get_string(response, "homepage"); if (v) { strncpy(meta.homepage, v, sizeof(meta.homepage)-1); free(v); }
    if (meta.name[0] == '\0') strncpy(meta.name, package_name, sizeof(meta.name)-1);
    printf("  \033[1;37mRemote Package Metadata:\033[0m\n");
    print_meta_box(&meta);
    int inst = registry_exists(package_name);
    if (inst) {
        VpiPackageMeta local; registry_load(package_name, &local);
        printf("  \033[1;32m[INSTALLED]\033[0m Local version: %s\n", local.version);
        if (strcmp(local.version, meta.version) != 0)
            printf("  \033[1;33m[UPDATE AVAILABLE]\033[0m %s -> %s\n", local.version, meta.version);
    } else {
        printf("  Install with: \033[1;36mvpi install %s\033[0m\n", package_name);
    }
    printf("\n");
    free(response);
    return 0;
}

int cmd_cache(int argc, char *argv[], const VpiFlags *flags) {
    const char *sub = (argc > 0) ? argv[0] : "list";
    if (strcmp(sub, "clear") == 0 || strcmp(sub, "clean") == 0) {
        printf("\033[1;37m==> Clearing cache:\033[0m \033[0;90m%s\033[0m\n", vpi_get_cache_dir());
        if (flags->dry_run) { printf("\033[1;33m[DRY-RUN]\033[0m Would clear cache directory.\n"); return 0; }
        size_t sz = dir_size(vpi_get_cache_dir());
        char sz_str[64]; format_bytes(sz, sz_str, sizeof(sz_str));
        dir_remove_recursive(vpi_get_cache_dir());
        dir_create_recursive(vpi_get_cache_dir());
        print_status("OK", "Cache cleared.", 0);
        printf("  Freed: %s\n\n", sz_str);
    } else if (strcmp(sub, "size") == 0) {
        size_t sz = dir_size(vpi_get_cache_dir());
        char sz_str[64]; format_bytes(sz, sz_str, sizeof(sz_str));
        printf("\033[1;37mCache size:\033[0m %s\n", sz_str);
        printf("\033[0;90mPath: %s\033[0m\n\n", vpi_get_cache_dir());
    } else {
        printf("\033[1;37m==> Cache Directory:\033[0m \033[0;90m%s\033[0m\n\n", vpi_get_cache_dir());
        size_t total = dir_size(vpi_get_cache_dir());
        char sz_str[64]; format_bytes(total, sz_str, sizeof(sz_str));
        printf("  Total size: %s\n\n", sz_str);
        printf("  Commands:\n");
        printf("    vpi cache clear   - Remove all cached files\n");
        printf("    vpi cache size    - Show cache size\n\n");
    }
    return 0;
}
