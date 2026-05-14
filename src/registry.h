#ifndef REGISTRY_H
#define REGISTRY_H
#include "vpi.h"
int registry_save(const VpiPackageMeta *meta);
int registry_load(const char *name, VpiPackageMeta *meta);
int registry_remove(const char *name);
int registry_exists(const char *name);
int registry_list(VpiPackageMeta **out, int *count);
void registry_free_list(VpiPackageMeta *list, int count);
#endif
