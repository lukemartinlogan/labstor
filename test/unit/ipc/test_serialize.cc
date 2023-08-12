//
// Created by lukemartinlogan on 8/7/23.
//

#include "basic_test.h"
#include "labstor/network/serialize.h"
#include "labstor/task_registry/task.h"

using labstor::DomainId;
using labstor::BinaryOutputArchive;
using labstor::BinaryInputArchive;
using labstor::DataTransfer;
using labstor::Task;

struct TestObj : public Task {
  std::vector <char> data_;
  DataTransfer xfer1_;
  DataTransfer xfer2_;
  int a_, b_, c_;

  TestObj(int a, int b, int c, const std::vector<char> &data)
  : a_(a), b_(b), c_(c), data_(data),
    xfer1_(DT_RECEIVER_READ, data_.data(), data_.size()),
    xfer2_(DT_RECEIVER_WRITE, data_.data(), data_.size()),
    Task(nullptr)  {}

  template<typename Ar>
  void serialize(Ar &ar) {

  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(a_, b_, xfer1_, c_, xfer2_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }

  bool operator==(const TestObj &other) const {
    return a_ == other.a_ && b_ == other.b_ && c_ == other.c_ &&
           xfer1_ == other.xfer1_ && xfer2_ == other.xfer2_;
  }
};

TEST_CASE("TestSerialize") {
  std::vector<char> data(256);
  BinaryOutputArchive<true> out(DomainId::GetLocal());
  TestObj obj(25, 30, 35, std::vector<char>(256, 10));
  out << obj;
  std::vector<DataTransfer> xfer = out.Get();

  BinaryInputArchive<true> in(xfer);
  TestObj obj2(40, 50, 60, std::vector<char>(256, 0));
  in >> obj2;

  REQUIRE(obj == obj2);
}