//
// Created by lukemartinlogan on 1/30/23.
//

#include "basic_test.h"
#include "test_init.h"
#include "labstor/data_structures/string.h"
#include "labstor/data_structures/thread_unsafe/list.h"
#include "labstor/data_structures/thread_unsafe/vector.h"

template<typename T, typename ContainerT>
void ListVecTest(size_t count) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  Allocator *alloc = alloc_g;
  Pointer *header = alloc->GetCustomHeader<Pointer>();
  ContainerT obj;

  if (rank == 0) {
    obj.shm_init(alloc);
    obj >> (*header);
  } else {
    obj.shm_deserialize(*header);
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Write 100 objects from rank 0
  {
    if (rank == 0) {
      for (int i = 0; i < count; ++i) {
        CREATE_SET_VAR_TO_INT_OR_STRING(T, var, i);
        obj.emplace_back(var);
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  // Read 100 objects from every rank
  {
    REQUIRE(obj.size() == count);
    int i = 0;
    for (lipc::ShmRef<T> var : obj) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, orig, i);
      REQUIRE(*var == orig);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  // Modify an object in rank 0
  {
    CREATE_SET_VAR_TO_INT_OR_STRING(T, update, count);
    lipc::ShmRef<T> first = *obj.begin();
    (*first) = update;
    MPI_Barrier(MPI_COMM_WORLD);
  }

  // Check if modification received
  {
    CREATE_SET_VAR_TO_INT_OR_STRING(T, update, count);
    lipc::ShmRef<T> first = *obj.begin();
    REQUIRE((*first) == update);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
  }
}

TEST_CASE("ListOfIntMpi") {
  ListVecTest<int, lipc::list<int>>(100);
}

TEST_CASE("ListOfStringMpi") {
  ListVecTest<lipc::string, lipc::list<lipc::string>>(100);
}

TEST_CASE("VectorOfIntMpi") {
  ListVecTest<int, lipc::vector<int>>(100);
}

TEST_CASE("VectorOfStringMpi") {
  ListVecTest<lipc::string, lipc::vector<lipc::string>>(100);
}