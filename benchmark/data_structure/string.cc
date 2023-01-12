//
// Created by lukemartinlogan on 1/3/23.
//

#include "basic_test.h"
#include "test_init.h"

#include <string>
#include <labstor/data_structures/string.h>

template<typename T>
class StringTestSuite {
 public:
  std::string str_type_;

  /////////////////
  /// Test Cases
  /////////////////

  /** Constructor */
  StringTestSuite() {
    if constexpr(std::is_same_v<std::string, T>) {
      str_type_ = "std::string";
    } else if constexpr(std::is_same_v<lipc::string, T>) {
      str_type_ = "lipc::string";
    }
  }

  /** Construct + destruct in a loop */
  void ConstructDestructTest(int count, int length) {
    char *data = (char *) malloc(length + 1);
    data[length] = 0;
    memset(data, 1, length);

    Timer t;
    t.Resume();
    for (int i = 0; i < count; ++i) {
      T hello(data);
    }
    t.Pause();

    TestOutput("ConstructDestructTest", t);
  }

  /** Construct in a loop, and then destruct in a loop */
  void ConstructThenDestructTest(int count, int length) {
    char *data = (char *) malloc(length + 1);
    data[length] = 0;
    memset(data, 1, length);

    std::vector<T> vec(count);

    Timer t;
    t.Resume();
    for (int i = 0; i < count; ++i) {
      vec[i] = T(data);
    }
    vec.clear();
    t.Pause();

    TestOutput("ConstructThenDestructTest", t);
  }

  /////////////////
  /// Test Output
  /////////////////

  /** Output test results */
  void TestOutput(const std::string &test_name, Timer &t) {
    printf("%s, %s, %lf\n",
           test_name.c_str(),
           str_type_.c_str(),
           t.GetMsec());
  }
};

template<typename T>
void StringTest() {
  StringTestSuite<T>().ConstructDestructTest(1000000, 10);
  // StringTestSuite<T>().ConstructThenDestructTest(1000000, MEGABYTES(1));
}

void FullStringTest() {
  StringTest<std::string>();
  StringTest<lipc::string>();
}

TEST_CASE("StringBenchmark") {
  FullStringTest();
}