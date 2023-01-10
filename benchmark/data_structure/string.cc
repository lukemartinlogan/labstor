//
// Created by lukemartinlogan on 1/3/23.
//

#include "basic_test.h"
#include "test_init.h"

#include <string>
#include <labstor/data_structures/string.h>

template<typename T>
class StringTest {
 public:
  std::string str_type_;

  StringTest() {
    if constexpr(std::is_same_v<std::string, T>) {
      str_type_ = "std::string";
    } else if constexpr(std::is_same_v<lipc::string, T>) {
      str_type_ = "lipc::string";
    }
  }

  void TestOutput(const std::string &test_name, Timer &t) {
    printf("%s, %s, %lf\n",
           test_name.c_str(),
           str_type_.c_str(),
           t.GetMsec());
  }

  void ConstructDestructTest(int count, int length) {
    char *data = (char *) malloc(length + 1);
    data[length] = 0;

    Timer t;
    t.Resume();
    for (int i = 0; i < count; ++i) {
      T hello(data);
    }
    t.Pause();

    TestOutput("ConstructDestructTest", t);
  }
};

void FullStringTest() {
  StringTest<std::string>().ConstructDestructTest(1000000, 10);
  StringTest<lipc::string>().ConstructDestructTest(1000000, 10);
}

TEST_CASE("StringBenchmark") {
  FullStringTest();
}