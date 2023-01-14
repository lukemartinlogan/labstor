/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

// Boost interprocess
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

// Boost private
#include <boost/container/scoped_allocator.hpp>
#include <boost/container/string.hpp>
#include <boost/container/vector.hpp>

// Std
#include <string>
#include <vector>

// LabStor
#include <labstor/data_structures/string.h>
#include <labstor/data_structures/thread_unsafe/vector.h>

#include "basic_test.h"
#include "test_init.h"

namespace bipc = boost::interprocess;

#define SET_VAR_TO_INT_OR_STRING(TYPE, VAR, VAL)\
  if constexpr(std::is_same_v<TYPE, lipc::string>) {\
    VAR = lipc::string(std::to_string(VAL));\
  } else if constexpr(std::is_same_v<TYPE, std::string>) {\
    VAR = std::string(std::to_string(VAL));\
  }\
  else {\
    VAR = VAL;\
  }

/**
 * A series of performance tests for vectors
 * OUTPUT:
 * [test_name] [vec_type] [internal_type] [time_ms]
 * */
template<typename T, typename VecT>
class VectorTest {
 public:
  std::string vec_type;
  std::string internal_type;
  VecT *vec;

  VectorTest() {
    if constexpr(std::is_same_v<std::vector<T>, VecT>) {
      vec = new VecT();
      vec_type = "std::vector";
    } else if constexpr(std::is_same_v<lipc::vector<T>, VecT>) {
      vec = new VecT();
      vec_type = "lipc::vector";
    } else if constexpr(std::is_same_v<boost::container::vector<T>, VecT>) {
      vec = new VecT();
      vec_type = "boost::vector";
    } else if constexpr(std::is_same_v<bipc::vector<T>, VecT>) {
      vec = BoostIpcVector();
      vec_type = "bipc::vector";
    } else {
      std::cout << "INVALID: none of the vector tests matched" << std::endl;
      return;
    }

    if constexpr(std::is_same_v<T, lipc::string>) {
      internal_type = "lipc::string";
    } else if constexpr(std::is_same_v<T, std::string>) {
      internal_type = "std::string";
    } else if constexpr(std::is_same_v<T, int>) {
      internal_type = "int";
    }
  }

  void TestOutput(const std::string &test_name, Timer &t) {
    printf("%s, %s, %s, %lf\n",
           test_name.c_str(),
           vec_type.c_str(),
           internal_type.c_str(),
           t.GetMsec());
  }

  void ResizeTest(VecT &vec, int count) {
    Timer t;
    T var;

    SET_VAR_TO_INT_OR_STRING(T, var, 124);

    t.Resume();
    vec.resize(count);
    t.Pause();

    TestOutput("FixedResize", t);
  }

  void ReserveEmplaceTest(VecT &vec, int count) {
    Timer t;
    T var;
    SET_VAR_TO_INT_OR_STRING(T, var, 124);

    t.Resume();
    vec.reserve(count);
    for (int i = 0; i < count; ++i) {
      vec.emplace_back(var);
    }
    t.Pause();

    TestOutput("FixedEmplace", t);
  }

  void GetTest(VecT &vec, int count) {
    Timer t;
    T var;
    SET_VAR_TO_INT_OR_STRING(T, var, 124);

    vec.reserve(count);
    for (int i = 0; i < count; ++i) {
      vec.emplace_back(var);
    }

    t.Resume();
    for (int i = 0; i < count; ++i) {
      auto x = vec[i];
    }
    t.Pause();

    TestOutput("FixedGet", t);
  }

  void ForwardIteratorTest(VecT &vec, int count) {
    Timer t;
    T var;
    SET_VAR_TO_INT_OR_STRING(T, var, 124);

    vec.reserve(count);
    for (int i = 0; i < count; ++i) {
      vec.emplace_back(var);
    }

    t.Resume();
    int i = 0;
    for (auto x : vec) {
      ++i;
    }
    t.Pause();

    TestOutput("ForwardIterator", t);
  }

  void CopyTest(VecT &vec, int count) {
    Timer t;
    T var;
    SET_VAR_TO_INT_OR_STRING(T, var, 124);

    vec.reserve(count);
    for (int i = 0; i < count; ++i) {
      vec.emplace_back(var);
    }

    t.Resume();
    VecT vec2(vec);
    t.Pause();

    TestOutput("Copy", t);
  }

  void MoveTest(VecT &vec, int count) {
    Timer t;
    T var;
    SET_VAR_TO_INT_OR_STRING(T, var, 124);

    vec.reserve(count);
    for (int i = 0; i < count; ++i) {
      vec.emplace_back(var);
    }

    t.Resume();
    VecT vec2(std::move(vec));
    t.Pause();

    TestOutput("Move", t);
  }

  VecT *BoostIpcVector() {
    void_allocator &alloc_inst = *alloc_inst_g;
    bipc::managed_shared_memory &segment = *segment_g;
    VecT *vec = segment.construct<VecT>("BoostVector")(alloc_inst);
    return vec;
  }

  void Test() {
    ResizeTest(*vec, 1000000);
    ReserveEmplaceTest(*vec, 1000000);
    GetTest(*vec, 1000000);
    ForwardIteratorTest(*vec, 1000000);
    CopyTest(*vec, 1000000);
    MoveTest(*vec, 1000000);
  }
};

void FullVectorTest() {
  // std::vector tests
  /*VectorTest<int, std::vector<int>>().Test();
  VectorTest<std::string, std::vector<std::string>>().Test();
  VectorTest<lipc::string, std::vector<lipc::string>>().Test();

  // boost::vector tests
  VectorTest<int, boost::container::vector<int>>().Test();
  VectorTest<std::string, boost::container::vector<std::string>>().Test();
  VectorTest<lipc::string, boost::container::vector<lipc::string>>().Test();*/

  // boost::ipc::vector tests
  VectorTest<int, bipc::vector<int>>().Test();
  // VectorTest<std::string, bipc::vector<std::string>>().Test();
  // VectorTest<lipc::string, bipc::vector<lipc::string>>().Test();

  // lipc::vector tests
  VectorTest<int, lipc::vector<int>>().Test();
  // VectorTest<std::string, lipc::vector<std::string>>().Test();
  // VectorTest<lipc::string, lipc::vector<lipc::string>>().Test();
}

TEST_CASE("VectorBenchmark") {
  FullVectorTest();
}