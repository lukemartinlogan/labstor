//
// Created by lukemartinlogan on 10/29/21.
//

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/kobject.h>

#include "labstor/kernel/server/module_manager.h"

#define MAX_LABSTOR_MODULES 64
struct labstor_module *modules[MAX_LABSTOR_MODULES];

void init_labstor_module_manager(void) {
    memset(modules, 0, MAX_LABSTOR_MODULES * sizeof(struct labstor_module *));
}

void register_labstor_module(struct labstor_module *pkg) {
    if(0 <= pkg->runtime_id && pkg->runtime_id < MAX_LABSTOR_MODULES) {
        modules[pkg->runtime_id] = pkg;
        return;
    }
    pr_err("Module %s has invalid runtime id %d\n", pkg->module_id.key, pkg->runtime_id);
}
EXPORT_SYMBOL(register_labstor_module);

void unregister_labstor_module(struct labstor_module *pkg) {
    modules[pkg->runtime_id] = 0;
}
EXPORT_SYMBOL(unregister_labstor_module);

struct labstor_module *get_labstor_module_by_runtime_id(int runtime_id) {
    if(0 <= runtime_id && runtime_id < MAX_LABSTOR_MODULES) {
        return modules[runtime_id];
    }
    return NULL;
}
EXPORT_SYMBOL(get_labstor_module_by_runtime_id);

void free_labstor_module_manager(void) {
}
EXPORT_SYMBOL(free_labstor_module_manager);