#ifndef COMMANDS_H
#define COMMANDS_H
#include "vpi.h"
int cmd_install(const char *package_name, const VpiFlags *flags);
int cmd_remove(const char *package_name, const VpiFlags *flags);
int cmd_update(const char *package_name, const VpiFlags *flags);
int cmd_list(const VpiFlags *flags);
int cmd_view(const char *package_name, const VpiFlags *flags);
int cmd_search(const char *query, const VpiFlags *flags);
int cmd_info(const char *package_name, const VpiFlags *flags);
int cmd_cache(int argc, char *argv[], const VpiFlags *flags);
int cmd_init(const VpiFlags *flags);
#endif
