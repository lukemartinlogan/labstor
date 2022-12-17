//
// Created by lukemartinlogan on 12/16/22.
//

#ifndef LABSTOR_TEST_UNIT_ALLOCATORS_TEST_INIT_H_
#define LABSTOR_TEST_UNIT_ALLOCATORS_TEST_INIT_H_

#include "basic_test.h"
#include "omp.h"
#include "labstor/data_structures/thread_unsafe/vector.h"
#include "labstor/memory/allocator/page_allocator.h"

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;

Allocator* Pretest(AllocatorType type);
void Posttest();
void PageAllocationTest(Allocator *alloc);

#endif //LABSTOR_TEST_UNIT_ALLOCATORS_TEST_INIT_H_
