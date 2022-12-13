//
// Created by lukemartinlogan on 11/27/22.
//

#ifndef LABSTOR_TEST_UNIT_DATA_STRUCTURES_TEST_INIT_H_
#define LABSTOR_TEST_UNIT_DATA_STRUCTURES_TEST_INIT_H_

#include "labstor/memory/allocator/page_allocator.h"

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::Pointer;

extern Allocator *alloc_g;
void Pretest(AllocatorType type);
void Posttest();

#endif //LABSTOR_TEST_UNIT_DATA_STRUCTURES_TEST_INIT_H_
