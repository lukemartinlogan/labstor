//
// Created by lukemartinlogan on 1/26/23.
//

#include "basic_test.h"
#include <labstor/types/tuple_base.h>
#include <utility>

void test_argpack0_pass() {
  std::cout << "HERE0" << std::endl;
}

void test_argpack0() {
  labstor::PassArgPack::Call(labstor::ArgPack<>(), test_argpack0_pass);
}

template<typename T1, typename T2, typename T3>
void test_argpack3_pass(T1 x, T2 y, T3 z) {
  REQUIRE(x == 0);
  REQUIRE(y == 1);
  REQUIRE(z == 0);
  std::cout << "HERE3" << std::endl;
}

void test_product1(int b, int c) {
  REQUIRE(b == 1);
  REQUIRE(c == 2);
}

void test_product2(double d, double e) {
  REQUIRE(d == 3);
  REQUIRE(e == 4);
}

template<typename Pack1, typename Pack2>
void test_product(int a, Pack1 &&pack1, int a2, Pack2 &&pack2) {
  REQUIRE(a == 0);
  REQUIRE(a2 == 0);
  labstor::PassArgPack::Call(
    std::forward<Pack1>(pack1),
    test_product1);
  labstor::PassArgPack::Call(
    std::forward<Pack2>(pack2),
    test_product2);
}

template<typename T1, typename T2, typename T3>
void test_argpack3() {
  // Pass an argpack to a function
  {
    labstor::PassArgPack::Call(
      labstor::ArgPack<T1, T2, T3>(0, 1, 0),
      test_argpack3_pass<T1, T2, T3>);
  }

  // Create a 3-tuple
  {
    labstor::tuple<T1, T2, T3> x(0, 1, 0);
    REQUIRE(x.template Get<0>() == 0);
    REQUIRE(x.template Get<1>() == 1);
    REQUIRE(x.template Get<2>() == 0);
    REQUIRE(x.template Get<2>() == 0);
    REQUIRE(x.Size() == 3);

#ifdef TEST_COMPILER_ERROR
    std::cout << x.Get<3>() << std::endl;
#endif
  }

  // Iterate over a tuple
  {
    labstor::tuple<T1, T2, T3> x(0, 1, 0);
    labstor::ForwardIterateTuple::Apply(
      x,
      [](size_t i, auto &arg) constexpr {
        std::cout << "lambda: " << i << std::endl;
      });
  }

  // Merge two argpacks into a single pack
  {
    size_t y = labstor::MergeArgPacks::Merge(
      labstor::ArgPack<T1>(0),
      labstor::ArgPack<T2, T3>(1, 0)).Size();
    REQUIRE(y == 3);
  }

  // Pass a merged argpack to a function
  {
    labstor::PassArgPack::Call(
      labstor::MergeArgPacks::Merge(
        labstor::ArgPack<T1>(0),
        labstor::ArgPack<T2, T3>(1, 0)),
      test_argpack3_pass<T1, T2, T3>);
  }

  // Product an argpack
  {
    auto&& pack = labstor::ProductArgPacks::Product(
      labstor::ArgPack<int>(0),
      labstor::ArgPack<int, int>(1, 2),
      labstor::ArgPack<double, double>(3, 4));
    REQUIRE(pack.Size() == 4);
  }

  // Product an argpack
  {
    labstor::PassArgPack::Call(
      labstor::ProductArgPacks::Product(
        (int)0,
        labstor::ArgPack<int, int>(1, 2),
        labstor::ArgPack<double, double>(3, 4)),
      test_product<
        labstor::ArgPack<int, int>,
        labstor::ArgPack<double, double>>);
  }
}

TEST_CASE("TestArgpack") {
  test_argpack0();
  test_argpack3<int, double, float>();
}