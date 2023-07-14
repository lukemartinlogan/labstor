//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_ram_bdev_H_
#define LABSTOR_ram_bdev_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "bdev/bdev.h"

namespace hermes::ram_bdev {

/** The set of methods in the admin task */
using bdev::Method;
using bdev::ConstructTask;
using bdev::DestructTask;
using bdev::AllocTask;
using bdev::FreeTask;
using bdev::ReadTask;
using bdev::WriteTask;

/** Create admin requests */
using bdev::Client;

}  // namespace labstor

#endif  // LABSTOR_ram_bdev_H_
