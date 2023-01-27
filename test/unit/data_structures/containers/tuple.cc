//
// Created by lukemartinlogan on 1/15/23.
//

#include "basic_test.h"
#include "test_init.h"
#include "labstor/data_structures/tuple.h"
#include "labstor/data_structures/string.h"

template<typename FirstT, typename SecondT>
void TupleTest() {
  Allocator *alloc = alloc_g;

  // Construct test
  {
    CREATE_SET_VAR_TO_INT_OR_STRING(FirstT, first, 124);
    CREATE_SET_VAR_TO_INT_OR_STRING(SecondT, second, 130);
    lipc::tuple<FirstT, SecondT> data(alloc, first, second);
    REQUIRE(*data.template Get<0>() == first);
    REQUIRE(*data.template Get<1>() == second);
  }

  // Copy test
  {
    CREATE_SET_VAR_TO_INT_OR_STRING(FirstT, first, 124);
    CREATE_SET_VAR_TO_INT_OR_STRING(SecondT, second, 130);
    lipc::tuple<FirstT, SecondT> data(alloc, first, second);
    lipc::tuple<FirstT, SecondT> cpy(data);
    REQUIRE(*cpy.template Get<0>() == first);
    REQUIRE(*cpy.template Get<1>() == second);
  }

  // Move test
  {
    CREATE_SET_VAR_TO_INT_OR_STRING(FirstT, first, 124);
    CREATE_SET_VAR_TO_INT_OR_STRING(SecondT, second, 130);
    lipc::tuple<FirstT, SecondT> data(alloc, first, second);
    lipc::tuple<FirstT, SecondT> cpy(std::move(data));
    REQUIRE(*cpy.template Get<0>() == first);
    REQUIRE(*cpy.template Get<1>() == second);
  }
}

TEST_CASE("PairOfIntInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  TupleTest<int, int>();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("PairOfIntString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  // TupleTest<int, lipc::string>();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}