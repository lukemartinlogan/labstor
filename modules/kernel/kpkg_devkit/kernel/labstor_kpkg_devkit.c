//
// Created by lukemartinlogan on 5/6/21.
//

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include "request_queue.h"
#include "simple_allocator.h"
#include "types.h"
#include "unordered_map.h"

MODULE_AUTHOR("Luke Logan <llogan@hawk.iit.edu>");
MODULE_DESCRIPTION("The labstor kernel module devkit");
MODULE_LICENSE("GPL");
MODULE_ALIAS("labstor_kpkg_devkit");

static int __init init_labstor_kpkg_devkit(void) {
    return 0;
}

static void __exit exit_labstor_kpkg_devkit(void) {
}

module_init(init_labstor_kpkg_devkit)
module_exit(exit_labstor_kpkg_devkit)
