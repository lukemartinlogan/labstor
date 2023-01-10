//
// Created by lukemartinlogan on 1/10/23.
//

#ifndef LABSTOR_BENCHMARK_DATA_STRUCTURE_TEST_INIT_H_
#define LABSTOR_BENCHMARK_DATA_STRUCTURE_TEST_INIT_H_

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <boost/container/scoped_allocator.hpp>

#include "labstor/data_structures/data_structure.h"
#include <labstor/util/timer.h>

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::Pointer;

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;

using Timer = labstor::HighResMonotonicTimer;

extern std::string shm_url;

#endif //LABSTOR_BENCHMARK_DATA_STRUCTURE_TEST_INIT_H_
