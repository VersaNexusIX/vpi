#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vpi.h"
#include "commands.h"
#include "utils.h"

static void print_banner(void) {
    printf("\033[1;36m");
    printf(" __ ___ __ _\n");
    printf(" \\ V / '_ \\| |\n");
    printf("  \\_/| .__/|_|\n");
    printf("     |_|      \033[0m\033[1;37mv%s\033[0m\n", VPI_VERSION);
    printf("\033[0;90mVersa Package Installer - versas.my.id\033[0m\n\n");
}

static void print_help(void) {
    print_banner();
    printf("\033[1;37mUSAGE:\033[0m\n");
    printf("  vpi <command> [options]\n\n");
    printf("\033[1;37mCOMMANDS:\033[0m\n");
    printf("  \033[1;32minstall\033[0m <package>   Install a package from versas.my.id\n");
    printf("  \033[1;32mremove\033[0m  <package>   Remove an installed package\n");
    printf("  \033[1;32mupdate\033[0m  <package>   Update a package to latest version\n");
    printf("  \033[1;32mlist\033[0m                List all installed packages\n");
    printf("  \033[1;32mview\033[0m    <package>   View details of an installed package\n");
    printf("  \033[1;32msearch\033[0m  <query>     Search packages on versas.my.id\n");
    printf("  \033[1;32minfo\033[0m    <package>   Show remote package info before install\n");
    printf("  \033[1;32minit\033[0m                Create vers.json interactively\n");
    printf("  \033[1;32mcache\033[0m               Manage local package cache\n");
    printf("  \033[1;32mversion\033[0m             Show VPI version\n");
    printf("  \033[1;32mhelp\033[0m                Show this help message\n\n");
    printf("\033[1;37mOPTIONS:\033[0m\n");
    printf("  \033[1;33m--verbose\033[0m           Enable verbose output\n");
    printf("  \033[1;33m--no-color\033[0m          Disable colored output\n");
    printf("  \033[1;33m--force\033[0m             Force operation (skip confirmations)\n");
    printf("  \033[1;33m--dry-run\033[0m           Simulate without making changes\n\n");
    printf("\033[0;90mExamples:\033[0m\n");
    printf("  vpi install mypackage\n");
    printf("  vpi list\n");
    printf("  vpi view mypackage\n");
    printf("  vpi remove mypackage\n");
    printf("  vpi init\n\n");
}

static int is_flag(const char *s) {
    return strncmp(s, "--", 2) == 0;
}

static VpiFlags parse_flags(int argc, char *argv[], int *cmd_idx) {
    VpiFlags flags = {0};
    *cmd_idx = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0) flags.verbose = 1;
        else if (strcmp(argv[i], "--no-color") == 0) flags.no_color = 1;
        else if (strcmp(argv[i], "--force") == 0) flags.force = 1;
        else if (strcmp(argv[i], "--dry-run") == 0) flags.dry_run = 1;
        else if (!is_flag(argv[i]) && *cmd_idx == -1) *cmd_idx = i;
    }
    if (*cmd_idx == -1) *cmd_idx = 1;
    return flags;
}

static const char *next_arg(int argc, char *argv[], int after) {
    for (int i = after + 1; i < argc; i++) {
        if (!is_flag(argv[i])) return argv[i];
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }
    int cmd_idx = 1;
    VpiFlags flags = parse_flags(argc, argv, &cmd_idx);
    if (flags.no_color) disable_color();
    vpi_init();
    const char *cmd = argv[cmd_idx];
    int ret = 0;
    if (strcmp(cmd, "install") == 0) {
        const char *pkg = next_arg(argc, argv, cmd_idx);
        if (!pkg) { fprintf(stderr, "\033[1;31mError:\033[0m Package name required.\n"); return 1; }
        ret = cmd_install(pkg, &flags);
    } else if (strcmp(cmd, "remove") == 0) {
        const char *pkg = next_arg(argc, argv, cmd_idx);
        if (!pkg) { fprintf(stderr, "\033[1;31mError:\033[0m Package name required.\n"); return 1; }
        ret = cmd_remove(pkg, &flags);
    } else if (strcmp(cmd, "update") == 0) {
        const char *pkg = next_arg(argc, argv, cmd_idx);
        if (!pkg) { fprintf(stderr, "\033[1;31mError:\033[0m Package name required.\n"); return 1; }
        ret = cmd_update(pkg, &flags);
    } else if (strcmp(cmd, "list") == 0) {
        ret = cmd_list(&flags);
    } else if (strcmp(cmd, "view") == 0) {
        const char *pkg = next_arg(argc, argv, cmd_idx);
        if (!pkg) { fprintf(stderr, "\033[1;31mError:\033[0m Package name required.\n"); return 1; }
        ret = cmd_view(pkg, &flags);
    } else if (strcmp(cmd, "search") == 0) {
        const char *pkg = next_arg(argc, argv, cmd_idx);
        if (!pkg) { fprintf(stderr, "\033[1;31mError:\033[0m Search query required.\n"); return 1; }
        ret = cmd_search(pkg, &flags);
    } else if (strcmp(cmd, "info") == 0) {
        const char *pkg = next_arg(argc, argv, cmd_idx);
        if (!pkg) { fprintf(stderr, "\033[1;31mError:\033[0m Package name required.\n"); return 1; }
        ret = cmd_info(pkg, &flags);
    } else if (strcmp(cmd, "cache") == 0) {
        ret = cmd_cache(argc - cmd_idx - 1, argv + cmd_idx + 1, &flags);
    } else if (strcmp(cmd, "init") == 0) {
        ret = cmd_init(&flags);
    } else if (strcmp(cmd, "version") == 0) {
        print_banner();
    } else if (strcmp(cmd, "help") == 0) {
        print_help();
    } else {
        fprintf(stderr, "\033[1;31mError:\033[0m Unknown command '%s'. Run 'vpi help' for usage.\n", cmd);
        ret = 1;
    }
    vpi_cleanup();
    return ret;
}
