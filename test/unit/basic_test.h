
#ifndef LABSTOR_TEST_UNIT_BASIC_TEST_H_
#define LABSTOR_TEST_UNIT_BASIC_TEST_H_

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>

namespace cl = Catch::Clara;
cl::Parser define_options();

static bool VerifyBuffer(char *ptr, size_t size, char nonce) {
  for (size_t i = 0; i < size; ++i) {
    if (ptr[i] != nonce) {
      return false;
    }
  }
  return true;
}

#endif //LABSTOR_TEST_UNIT_BASIC_TEST_H_
