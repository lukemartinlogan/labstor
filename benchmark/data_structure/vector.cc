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

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/container/string.hpp>
#include <boost/container/vector.hpp>

#include <string>
#include <vector>

#include <labstor/data_structures/string.h>
#include <labstor/data_structures/thread_unsafe/vector.h>

#include <labstor/util/timer.h>
using Timer = labstor::HighResMonotonicTimer;

#define SET_VAR_TO_INT_OR_STRING(TYPE, VAR, VAL)\
  if constexpr(std::is_same_v<TYPE, lipc::string>) {\
    VAR = lipc::string(std::to_string(VAL));\
  } else if constexpr(std::is_same_v<TYPE, std::string>) {\
    VAR = std::string(std::to_string(VAL));\
  }\
  else {\
    VAR = VAL;\
  }

template<typename T, typename VecT>
void ResizeTest(int count) {
  Timer t;
  VecT vec;
  T var;

  SET_VAR_TO_INT_OR_STRING(T, var, 124);

  t.Resume();
  vec.resize(count);
  t.Pause();

  std::cout << "FixedResize: " << t.GetSec() << "s" << std::endl;
}

template<typename T, typename VecT>
void ReserveEmplaceTest(int count) {
  Timer t;
  VecT vec;
  T var;
  SET_VAR_TO_INT_OR_STRING(T, var, 124);

  t.Resume();
  vec.reserve(count);
  for(int i = 0; i < count; ++i) {
    vec.emplace_back(var);
  }
  t.Pause();

  std::cout << "FixedEmplace: " << t.GetSec() << "s" << std::endl;
}

template<typename T, typename VecT>
void GetTest(int count) {
  Timer t;
  VecT vec;
  T var;
  SET_VAR_TO_INT_OR_STRING(T, var, 124);

  vec.reserve(count);
  for(int i = 0; i < count; ++i) {
    vec.emplace_back(var);
  }

  t.Resume();
  for(int i = 0; i < count; ++i) {
    auto x = vec[i];
  }
  t.Pause();

  std::cout << "FixedGet: " << t.GetSec() << "s" << std::endl;
}

template<typename T, typename VecT>
void ForwardIteratorTest(int count) {
  Timer t;
  VecT vec;
  T var;
  SET_VAR_TO_INT_OR_STRING(T, var, 124);

  vec.reserve(count);
  for(int i = 0; i < count; ++i) {
    vec.emplace_back(var);
  }

  t.Resume();
  for(auto x : vec) {}
  t.Pause();

  std::cout << "ForwardIterator: " << t.GetSec() << "s" << std::endl;
}

template<typename T, typename VecT>
void CopyTest(int count) {
  Timer t;
  VecT vec;
  T var;
  SET_VAR_TO_INT_OR_STRING(T, var, 124);

  vec.reserve(count);
  for(int i = 0; i < count; ++i) {
    vec.emplace_back(var);
  }

  t.Resume();
  lipc::vector vec2(vec);
  t.Pause();

  std::cout << "Copy: " << t.GetSec() << "s" << std::endl;
}

template<typename T, typename VecT>
void MoveTest(int count) {
  Timer t;
  VecT vec;
  T var;
  SET_VAR_TO_INT_OR_STRING(T, var, 124);

  vec.reserve(count);
  for(int i = 0; i < count; ++i) {
    vec.emplace_back(var);
  }

  t.Resume();
  lipc::vector vec2(std::move(vec));
  t.Pause();

  std::cout << "Move: " << t.GetSec() << "s" << std::endl;
}

template<typename T, typename VecT>
void VectorTest(const std::string &name) {
  if constexpr(std::is_same_v<T, lipc::string>) {
    std::cout << name << ": lipc::string" << std::endl;
  } else if constexpr(std::is_same_v<T, std::string>) {
    std::cout << name << ": std::string" << std::endl;
  }
  else {
    std::cout << name << ": int" << std::endl;
  }

  ResizeTest<T, VecT>(1024);
  ReserveEmplaceTest<T, VecT>(1024);
  GetTest<T, VecT>(1024);
  ForwardIteratorTest<T, VecT>(1024);
  CopyTest<T, VecT>(1024);
  MoveTest<T, VecT>(1024);

  std::cout << "Finished" << std::endl;
  std::cout << std::endl;
}

void FullVectorTest() {
  // std::vector tests
  VectorTest<int, std::vector<int>>("std::vector");
  VectorTest<std::string, std::vector<lipc::string>>("std::vector");
  VectorTest<lipc::string, std::vector<lipc::string>>("std::vector");

  // lipc::vector tests
  VectorTest<int, lipc::vector<int>>("lipc::vector");
  VectorTest<std::string, lipc::vector<lipc::string>>("lipc::vector");
  VectorTest<lipc::string, lipc::vector<lipc::string>>("lipc::vector");

  // boost::vector tests
  VectorTest<int, boost::vector<int>>("boost::vector");
  VectorTest<std::string, boost::vector<lipc::string>>("boost::vector");
  VectorTest<lipc::string, boost::vector<lipc::string>>("boost::vector");

  // boost::ipc::vector tests
  VectorTest<int, boost::ipc::vector<int>>("boost::ipc::vector");
  VectorTest<std::string, boost::ipc::vector<lipc::string>>(
    "boost::ipc::vector");
  VectorTest<lipc::string, boost::ipc::vector<lipc::string>>(
    "boost::ipc::vector");
}

int main() {
  FullTest();
}