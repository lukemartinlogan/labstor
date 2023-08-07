//
// Created by lukemartinlogan on 8/7/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_NETWORK_SERIALIZE_H_
#define LABSTOR_INCLUDE_LABSTOR_NETWORK_SERIALIZE_H_

#include "labstor/labstor_types.h"
#include <sstream>
#include <cereal/archives/binary.hpp>

namespace labstor {

/** Receiver will read from data_ */
#define DT_RECEIVER_READ BIT_OPT(u32, 0)

/** Receiver will write to data_ */
#define DT_RECEIVER_WRITE BIT_OPT(u32, 1)

/** Free data_ when the data transfer is complete */
#define DT_FREE_DATA BIT_OPT(u32, 2)

/** Indicate how data should be transferred over network */
template<bool NO_XFER>
struct DataTransferBase {
  hshm::bitfield32_t flags_;  /**< Indicates how data will be accessed */
  DomainId node_id_;          /**< The node data is located */
  void *data_;                /**< The virtual address of data on the node */
  size_t data_size_;          /**< The amount of data to transfer */

  /** Serialize a data transfer object */
  template<typename Ar>
  void serialize(Ar &ar) {
    ar(flags_, node_id_, (size_t)data_, data_size_);
  }

  /** Default constructor */
  DataTransferBase() = default;

  /** Emplace constructor */
  DataTransferBase(u32 flags, const DomainId &node_id, void *data, size_t data_size) :
  flags_(flags), node_id_(node_id), data_(data), data_size_(data_size) {}

  /** Copy constructor */
  DataTransferBase(const DataTransferBase &xfer) :
  flags_(xfer.flags_), node_id_(xfer.node_id_), data_(xfer.data_),
  data_size_(xfer.data_size_) {}

  /** Copy assignment */
  DataTransferBase& operator=(const DataTransferBase &xfer) {
    flags_ = xfer.flags_;
    node_id_ = xfer.node_id_;
    data_ = xfer.data_;
    data_size_ = xfer.data_size_;
    return *this;
  }

  /** Move constructor */
  DataTransferBase(DataTransferBase &&xfer) noexcept :
  flags_(xfer.flags_), node_id_(xfer.node_id_),
  data_(xfer.data_), data_size_(xfer.data_size_) {}
};

using DataTransfer = DataTransferBase<true>;
using PassDataTransfer = DataTransferBase<false>;

class BinaryOutputArchive {
 public:
  std::vector<DataTransfer> xfer_;
  std::stringstream ss_;
  cereal::BinaryOutputArchive ar_;
  DomainId node_id_;

 public:
  /** Default constructor */
  BinaryOutputArchive(const DomainId &node_id)
  : node_id_(node_id), ar_(ss_) {}

  /** Serialize using left shift */
  template<typename T>
  BinaryOutputArchive& operator<<(const T &var) {
    return Serialize(var);
  }

  /** Serialize using ampersand */
  template<typename T>
  BinaryOutputArchive& operator&(const T &var) {
    return Serialize(var);
  }

  /** Serialize a parameter */
  template<typename T>
  BinaryOutputArchive& Serialize(const T &var) {
    if constexpr(std::is_same_v<T, DataTransfer>) {
      SolidfyStringStream();
      xfer_.push_back(var);
    } else {
      ar_ << var;
    }
    return *this;
  }

  /** Get the final output */
  std::vector<DataTransfer> Get() {
    SolidfyStringStream();
    return std::move(xfer_);
  }

 private:
  void SolidfyStringStream() {
    std::string str = ss_.str();
    if (str.size() > 0) {
      void *data = malloc(str.size());
      memcpy(data, str.c_str(), str.size());
      xfer_.emplace_back(DT_RECEIVER_READ | DT_FREE_DATA, node_id_, data, str.size());
      ss_.clear();
    }
  }
};

class BinaryInputArchive {
 public:
  std::vector<DataTransfer> xfer_;
  std::stringstream ss_;
  cereal::BinaryInputArchive ar_;
  int xfer_off_;
  int count_;

 public:
  /** Default constructor */
  BinaryInputArchive(std::vector<DataTransfer> &xfer)
  : xfer_(std::move(xfer)), xfer_off_(0), count_(0), ar_(ss_) {}

  /** Deserialize using right shift */
  template<typename T>
  BinaryInputArchive& operator>>(const T &var) {
    return Deserialize(var);
  }

  /** Deserialize using ampersand */
  template<typename T>
  BinaryInputArchive& operator&(const T &var) {
    return Deserialize(var);
  }

  /** Serialize a parameter */
  template<typename T>
  BinaryInputArchive& Deserialize(const T &var) {
    if constexpr(std::is_same_v<T, DataTransfer>) {
      if (count_) {
        xfer_off_ += 1;
        count_ = 0;
      }
      var = xfer_[xfer_off_++];
    } else {
      ar_ >> var;
      count_ += 1;
    }
    return *this;
  }
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_NETWORK_SERIALIZE_H_
