//
// Created by lukemartinlogan on 1/26/23.
//

#include "basic_test.h"
#include <labstor/types/argpack.h>

TEST_CASE("TestArgpack") {
  labstor::ArgPack<int, double, float> x(0, 1, 0);
  REQUIRE(x.Get<0>() == 0);
  REQUIRE(x.Get<1>() == 1);
  REQUIRE(x.Get<2>() == 0);
  REQUIRE(x.Size() == 3);

#ifdef TEST_COMPILER_ERROR
  std::cout << x.Get<3>() << std::endl;
#endif
}