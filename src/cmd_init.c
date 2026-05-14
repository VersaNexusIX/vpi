#include "commands.h"
#include "utils.h"
#include "vpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#define INIT_MAX_INPUT 512

static const char *LICENSES[] = {
    "MIT", "Apache-2.0", "GPL-2.0", "GPL-3.0", "LGPL-2.1", "LGPL-3.0",
    "BSD-2-Clause", "BSD-3-Clause", "ISC", "MPL-2.0", "CDDL-1.0",
    "EPL-2.0", "AGPL-3.0", "Unlicense", "CC0-1.0", "Proprietary", NULL
};

static void print_init_header(void) {
    printf("\033[1;36m");
    printf("  ╔══════════════════════════════════════╗\n");
    printf("  ║      VPI Package Initializer         ║\n");
    printf("  ║   Generates vers.json for your pkg   ║\n");
    printf("  ╚══════════════════════════════════════╝\n");
    printf("\033[0m\n");
    printf("\033[0;90mThis will create a vers.json in the current directory.\n");
    printf("Press Enter to accept [default values].\033[0m\n\n");
}

static void prompt_field(const char *label, const char *hint, const char *def,
                          char *out, size_t out_sz, int required) {
    while (1) {
        if (def && def[0]) {
            printf("  \033[1;36m%-18s\033[0m\033[0;90m(%s)\033[0m \033[0;90m[%s]:\033[0m ", label, hint, def);
        } else {
            printf("  \033[1;36m%-18s\033[0m\033[0;90m(%s):\033[0m ", label, hint);
        }
        fflush(stdout);
        char buf[INIT_MAX_INPUT];
        if (!fgets(buf, sizeof(buf), stdin)) { buf[0] = '\0'; }
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[--len] = '\0';
        char *trimmed = str_trim(buf);
        if (trimmed[0] == '\0') {
            if (def && def[0]) {
                strncpy(out, def, out_sz - 1);
                out[out_sz - 1] = '\0';
                printf("  \033[0;90m  → Using default: %s\033[0m\n", def);
                return;
            }
            if (required) {
                printf("  \033[1;31m  ✗ This field is required. Please enter a value.\033[0m\n");
                continue;
            }
            out[0] = '\0';
            return;
        }
        strncpy(out, trimmed, out_sz - 1);
        out[out_sz - 1] = '\0';
        printf("  \033[1;32m  ✓\033[0m\n");
        return;
    }
}

static void prompt_version(const char *def, char *out, size_t out_sz) {
    while (1) {
        printf("  \033[1;36m%-18s\033[0m\033[0;90m(semver: X.Y.Z)\033[0m \033[0;90m[%s]:\033[0m ", "Versi:", def);
        fflush(stdout);
        char buf[64];
        if (!fgets(buf, sizeof(buf), stdin)) { buf[0] = '\0'; }
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[--len] = '\0';
        char *trimmed = str_trim(buf);
        if (trimmed[0] == '\0') {
            strncpy(out, def, out_sz - 1);
            out[out_sz-1] = '\0';
            printf("  \033[0;90m  → Using default: %s\033[0m\n", def);
            return;
        }
        int dots = 0, valid = 1;
        for (char *p = trimmed; *p; p++) {
            if (*p == '.') dots++;
            else if (!isdigit((unsigned char)*p)) { valid = 0; break; }
        }
        if (!valid || dots != 2) {
            printf("  \033[1;31m  ✗ Format versi tidak valid. Gunakan X.Y.Z (contoh: 1.0.0)\033[0m\n");
            continue;
        }
        strncpy(out, trimmed, out_sz - 1);
        out[out_sz-1] = '\0';
        printf("  \033[1;32m  ✓\033[0m\n");
        return;
    }
}

static void prompt_license(char *out, size_t out_sz) {
    printf("\n  \033[1;37mPilih Lisensi:\033[0m\n");
    for (int i = 0; LICENSES[i] != NULL; i++) {
        if (i % 4 == 0) printf("  ");
        printf("  \033[0;90m[%2d]\033[0m %-14s", i + 1, LICENSES[i]);
        if ((i + 1) % 4 == 0) printf("\n");
    }
    printf("\n\n");
    while (1) {
        printf("  \033[1;36m%-18s\033[0m\033[0;90m(nomor atau ketik sendiri)\033[0m \033[0;90m[1]:\033[0m ", "Lisensi:");
        fflush(stdout);
        char buf[INIT_MAX_INPUT];
        if (!fgets(buf, sizeof(buf), stdin)) { buf[0] = '\0'; }
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[--len] = '\0';
        char *trimmed = str_trim(buf);
        if (trimmed[0] == '\0') {
            strncpy(out, LICENSES[0], out_sz - 1);
            out[out_sz-1] = '\0';
            printf("  \033[0;90m  → Using default: MIT\033[0m\n");
            return;
        }
        int all_digits = 1;
        for (char *p = trimmed; *p; p++) {
            if (!isdigit((unsigned char)*p)) { all_digits = 0; break; }
        }
        if (all_digits) {
            int choice = atoi(trimmed);
            if (choice >= 1) {
                int count = 0;
                while (LICENSES[count]) count++;
                if (choice <= count) {
                    strncpy(out, LICENSES[choice - 1], out_sz - 1);
                    out[out_sz-1] = '\0';
                    printf("  \033[1;32m  ✓ %s\033[0m\n", out);
                    return;
                }
            }
            printf("  \033[1;31m  ✗ Nomor tidak valid (1-%d)\033[0m\n", 16);
            continue;
        }
        strncpy(out, trimmed, out_sz - 1);
        out[out_sz-1] = '\0';
        printf("  \033[1;32m  ✓\033[0m\n");
        return;
    }
}

static void prompt_entry_point(char *out, size_t out_sz) {
    printf("\n  \033[1;37mTipe Package:\033[0m\n");
    printf("    \033[0;90m[1]\033[0m library      - Hanya library, tidak ada executable\n");
    printf("    \033[0;90m[2]\033[0m binary        - Ada file executable\n");
    printf("    \033[0;90m[3]\033[0m mixed         - Library + executable\n");
    printf("    \033[0;90m[4]\033[0m script        - Script (shell/python/dll)\n\n");
    printf("  \033[1;36m%-18s\033[0m\033[0;90m(pilih 1-4)\033[0m \033[0;90m[1]:\033[0m ", "Tipe:");
    fflush(stdout);
    char buf[16];
    if (!fgets(buf, sizeof(buf), stdin)) buf[0] = '1';
    char c = buf[0];
    if (c == '2') strncpy(out, "binary", out_sz - 1);
    else if (c == '3') strncpy(out, "mixed", out_sz - 1);
    else if (c == '4') strncpy(out, "script", out_sz - 1);
    else strncpy(out, "library", out_sz - 1);
    out[out_sz-1] = '\0';
    printf("  \033[1;32m  ✓ %s\033[0m\n", out);
}

static char *json_escape(const char *s, char *buf, size_t bufsz) {
    size_t i = 0, o = 0;
    while (s[i] && o + 2 < bufsz) {
        if (s[i] == '"')       { buf[o++] = '\\'; buf[o++] = '"'; }
        else if (s[i] == '\\') { buf[o++] = '\\'; buf[o++] = '\\'; }
        else if (s[i] == '\n') { buf[o++] = '\\'; buf[o++] = 'n'; }
        else if (s[i] == '\t') { buf[o++] = '\\'; buf[o++] = 't'; }
        else { buf[o++] = s[i]; }
        i++;
    }
    buf[o] = '\0';
    return buf;
}

static char *get_current_dirname(char *out, size_t sz) {
#ifdef _WIN32
    if (!_getcwd(out, (int)sz)) { strncpy(out, "mypackage", sz-1); return out; }
    char *last = strrchr(out, '\\');
    if (last) memmove(out, last + 1, strlen(last));
#else
    if (!getcwd(out, sz)) { strncpy(out, "mypackage", sz-1); return out; }
    char *last = strrchr(out, '/');
    if (last) memmove(out, last + 1, strlen(last));
#endif
    return out;
}

static void make_slug(char *s) {
    for (char *p = s; *p; p++) {
        if (isspace((unsigned char)*p) || *p == '_') *p = '-';
        else *p = (char)tolower((unsigned char)*p);
        if (!isalnum((unsigned char)*p) && *p != '-' && *p != '.') *p = '-';
    }
}

int cmd_init(const VpiFlags *flags) {
    (void)flags;
    print_init_header();
    char cwd_name[VPI_MAX_NAME];
    get_current_dirname(cwd_name, sizeof(cwd_name));
    make_slug(cwd_name);
    char nama[INIT_MAX_INPUT]        = {0};
    char versi[64]                   = {0};
    char developer[INIT_MAX_INPUT]   = {0};
    char email[INIT_MAX_INPUT]       = {0};
    char lisensi[INIT_MAX_INPUT]     = {0};
    char description[INIT_MAX_INPUT] = {0};
    char homepage[INIT_MAX_INPUT]    = {0};
    char keywords[INIT_MAX_INPUT]    = {0};
    char pkg_type[32]                = {0};
    char entry[INIT_MAX_INPUT]       = {0};
    printf("  \033[1;37m── Identitas Paket ──────────────────────\033[0m\n\n");
    prompt_field("Nama Paket:", "huruf kecil, boleh tanda -", cwd_name, nama, sizeof(nama), 1);
    make_slug(nama);
    prompt_version("1.0.0", versi, sizeof(versi));
    printf("\n  \033[1;37m── Informasi Developer ──────────────────\033[0m\n\n");
    prompt_field("Developer/Organisasi:", "nama lengkap atau org", "", developer, sizeof(developer), 1);
    prompt_field("Email:", "opsional", "", email, sizeof(email), 0);
    prompt_field("Homepage/URL:", "https://...", "", homepage, sizeof(homepage), 0);
    printf("\n  \033[1;37m── Deskripsi Paket ──────────────────────\033[0m\n\n");
    prompt_field("Deskripsi:", "satu kalimat singkat", "", description, sizeof(description), 0);
    prompt_field("Keywords:", "pisah dengan koma", "", keywords, sizeof(keywords), 0);
    prompt_license(lisensi, sizeof(lisensi));
    prompt_entry_point(pkg_type, sizeof(pkg_type));
    if (strcmp(pkg_type, "library") != 0) {
        printf("\n");
        prompt_field("Entry Point:", "misal: bin/myapp atau main.sh", "", entry, sizeof(entry), 0);
    }
    char eb1[INIT_MAX_INPUT*2], eb2[INIT_MAX_INPUT*2], eb3[INIT_MAX_INPUT*2];
    char eb4[INIT_MAX_INPUT*2], eb5[INIT_MAX_INPUT*2], eb6[INIT_MAX_INPUT*2];
    char eb7[INIT_MAX_INPUT*2], eb8[INIT_MAX_INPUT*2];
    json_escape(nama, eb1, sizeof(eb1));
    json_escape(versi, eb2, sizeof(eb2));
    json_escape(developer, eb3, sizeof(eb3));
    json_escape(lisensi, eb4, sizeof(eb4));
    json_escape(description, eb5, sizeof(eb5));
    json_escape(homepage, eb6, sizeof(eb6));
    json_escape(keywords, eb7, sizeof(eb7));
    json_escape(email, eb8, sizeof(eb8));
    char json_buf[4096];
    int n = 0;
    n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "{\n");
    n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "  \"nama\": \"%s\",\n", eb1);
    n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "  \"versi\": \"%s\",\n", eb2);
    n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "  \"developer\": \"%s\",\n", eb3);
    if (email[0])
        n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "  \"email\": \"%s\",\n", eb8);
    n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "  \"lisensi\": \"%s\",\n", eb4);
    if (description[0])
        n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "  \"description\": \"%s\",\n", eb5);
    if (homepage[0])
        n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "  \"homepage\": \"%s\",\n", eb6);
    if (keywords[0])
        n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "  \"keywords\": \"%s\",\n", eb7);
    n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "  \"type\": \"%s\"", pkg_type);
    if (entry[0]) {
        char eb9[INIT_MAX_INPUT*2];
        json_escape(entry, eb9, sizeof(eb9));
        n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, ",\n  \"entry\": \"%s\"", eb9);
    }
    n += snprintf(json_buf + n, sizeof(json_buf) - (size_t)n, "\n}\n");
    printf("\n");
    print_separator('-', 50);
    printf("\n  \033[1;37mPreview vers.json:\033[0m\n\n");
    const char *lines_start = json_buf;
    int line_num = 1;
    while (*lines_start) {
        const char *nl = strchr(lines_start, '\n');
        size_t line_len = nl ? (size_t)(nl - lines_start) : strlen(lines_start);
        printf("  \033[0;90m%2d │\033[0m \033[1;33m%.*s\033[0m\n", line_num++, (int)line_len, lines_start);
        lines_start += line_len + (nl ? 1 : 0);
        if (!nl) break;
    }
    printf("\n");
    print_separator('-', 50);
    printf("\n");
    if (!flags->force) {
        printf("  \033[1;37mSimpan sebagai \033[1;36mvers.json\033[1;37m? [Y/n]:\033[0m ");
        fflush(stdout);
        char ans[8];
        if (!fgets(ans, sizeof(ans), stdin)) ans[0] = 'y';
        if (ans[0] == 'n' || ans[0] == 'N') {
            printf("\n  \033[1;33mDibatalkan.\033[0m Tidak ada file yang dibuat.\n\n");
            return 0;
        }
    }
    if (flags->dry_run) {
        printf("  \033[1;33m[DRY-RUN]\033[0m Tidak ada file yang ditulis.\n\n");
        return 0;
    }
    if (file_exists("vers.json") && !flags->force) {
        printf("  \033[1;33m[WARN]\033[0m vers.json sudah ada. Timpa? [y/N]: ");
        fflush(stdout);
        char ans2[8];
        if (!fgets(ans2, sizeof(ans2), stdin)) ans2[0] = 'n';
        if (ans2[0] != 'y' && ans2[0] != 'Y') {
            printf("  \033[1;33mDibatalkan.\033[0m File lama dipertahankan.\n\n");
            return 0;
        }
    }
    if (file_write_all("vers.json", json_buf, (size_t)n) != 0) {
        fprintf(stderr, "  \033[1;31mError:\033[0m Gagal menulis vers.json. Periksa izin direktori.\n");
        return 1;
    }
    printf("\n");
    printf("  \033[1;32m✓\033[0m \033[1;37mvers.json berhasil dibuat!\033[0m\n\n");
    printf("  \033[0;90mLangkah selanjutnya:\033[0m\n");
    printf("  \033[0;90m1.\033[0m Tambahkan file paket kamu ke direktori ini\n");
    printf("  \033[0;90m2.\033[0m Buat archive:\033[0m \033[1;36mzip -r %s.vers . && mv %s.vers /ke/server\033[0m\n", nama, nama);
    printf("  \033[0;90m3.\033[0m Upload ke:\033[0m \033[1;36mhttps://versas.my.id/package/%s.vers\033[0m\n", nama);
    printf("  \033[0;90m4.\033[0m Test install:\033[0m \033[1;36mvpi install %s\033[0m\n\n", nama);
    return 0;
}
