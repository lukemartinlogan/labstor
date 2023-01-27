//
// Created by lukemartinlogan on 1/26/23.
//

#include "basic_test.h"
#include <labstor/types/argpack.h>
#include <utility>

void test_argpack0_pass() {
  std::cout << "HERE0" << std::endl;
}

void test_argpack0() {
  labstor::ArgPack args;
  labstor::PassArgPack::Call(args, test_argpack0_pass);
}

template<typename T1, typename T2, typename T3>
void test_argpack3_pass(T1 x, T2 y, T3 z) {
  REQUIRE(x == 0);
  REQUIRE(y == 1);
  REQUIRE(z == 0);
  std::cout << "HERE3" << std::endl;
}

template<typename T1, typename T2, typename T3>
void test_argpack3() {
  labstor::ArgPack<T1, T2, T3> x(0, 1, 0);
  REQUIRE(x.template Get<0>() == 0);
  REQUIRE(x.template Get<1>() == 1);
  REQUIRE(x.template Get<2>() == 0);
  REQUIRE(x.template Get<2>() == 0);
  REQUIRE(x.Size() == 3);

#ifdef TEST_COMPILER_ERROR
  std::cout << x.Get<3>() << std::endl;
#endif

  labstor::PassArgPack::Call(x, test_argpack3_pass<T1, T2, T3>);

  labstor::IterateTuple::Apply(x,
    [](size_t i, auto &arg) constexpr {
      std::cout << "lambda: " << i << std::endl;
    });
}

TEST_CASE("TestArgpack") {
  test_argpack0();
  test_argpack3<int, double, float>();
}