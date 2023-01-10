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

namespace bipc = boost::interprocess;

typedef bipc::managed_shared_memory::segment_manager segment_manager_t;
typedef boost::container::scoped_allocator_adaptor<
bipc::allocator<void, segment_manager_t>>
  void_allocator;

extern std::unique_ptr<void_allocator> alloc_inst_g;
extern std::unique_ptr<bipc::managed_shared_memory> segment_g;

#endif //LABSTOR_BENCHMARK_DATA_STRUCTURE_TEST_INIT_H_
