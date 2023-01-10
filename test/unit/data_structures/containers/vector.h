//
// Created by lukemartinlogan on 1/10/23.
//

#ifndef LABSTOR_TEST_UNIT_DATA_STRUCTURES_CONTAINERS_VECTOR_H_
#define LABSTOR_TEST_UNIT_DATA_STRUCTURES_CONTAINERS_VECTOR_H_

#include "list.h"

template<typename T, typename Container>
class VectorTestSuite : public ListTestSuite<T, Container> {
 public:
  using ListTestSuite<T, Container>::obj_;

 public:
  /// Constructor
  VectorTestSuite(Container &obj, Allocator *alloc)
  : ListTestSuite<T, Container>(obj, alloc) {}

  /// Test vector index operator
  void IndexTest() {
    for (int i = 0; i < obj_.size(); ++i) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, var, i);
      REQUIRE(*obj_[i] == var);
    }
  }
};

#endif //LABSTOR_TEST_UNIT_DATA_STRUCTURES_CONTAINERS_VECTOR_H_
