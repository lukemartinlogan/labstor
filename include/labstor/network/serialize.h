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
  void *data_;                /**< The virtual address of data on the node */
  size_t data_size_;          /**< The amount of data to transfer */
  DomainId node_id_;          /**< The node data is located */

  /** Serialize a data transfer object */
  template<typename Ar>
  void save(Ar &ar) const {
    u32 flags = flags_.bits_;
    ar(flags, (size_t)data_, data_size_, node_id_);
  }

  /** Deserialize a data transfer object */
  template<typename Ar>
  void load(Ar &ar) {
    u32 flags;
    ar(flags, (size_t)data_, data_size_, node_id_);
    flags_.bits_ = flags;
  }

  /** Default constructor */
  DataTransferBase() = default;

  /** Emplace constructor */
  DataTransferBase(u32 flags, void *data, size_t data_size,
                   const DomainId &node_id = DomainId::GetLocal()) :
  flags_(flags), data_(data), data_size_(data_size), node_id_(node_id) {}

  /** Copy constructor */
  DataTransferBase(const DataTransferBase &xfer) :
  flags_(xfer.flags_), data_(xfer.data_),
  data_size_(xfer.data_size_), node_id_(xfer.node_id_) {}

  /** Copy assignment */
  DataTransferBase& operator=(const DataTransferBase &xfer) {
    flags_ = xfer.flags_;
    data_ = xfer.data_;
    data_size_ = xfer.data_size_;
    node_id_ = xfer.node_id_;
    return *this;
  }

  /** Move constructor */
  DataTransferBase(DataTransferBase &&xfer) noexcept :
  flags_(xfer.flags_), data_(xfer.data_),
  data_size_(xfer.data_size_), node_id_(xfer.node_id_) {}

  /** Equality operator */
  bool operator==(const DataTransferBase &other) const {
    return flags_.bits_ == other.flags_.bits_ &&
         data_ == other.data_ &&
         data_size_ == other.data_size_ &&
         node_id_ == other.node_id_;
  }
};

using DataTransfer = DataTransferBase<true>;
using PassDataTransfer = DataTransferBase<false>;

/** Label a class as supporting bulk serialization methods */
struct BulkSerializeable {};

/** Serialize a data structure */
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

  /** Serialize using call */
  template<typename T, typename ...Args>
  BinaryOutputArchive& operator()(T &var, Args &&...args) {
    return Serialize(var, std::forward<Args>(args)...);
  }

  /** Serialize using left shift */
  template<typename T>
  BinaryOutputArchive& operator<<(T &var) {
    return Serialize(var);
  }

  /** Serialize using ampersand */
  template<typename T>
  BinaryOutputArchive& operator&(T &var) {
    return Serialize(var);
  }

  /** Serialize using left shift */
  template<typename T>
  BinaryOutputArchive& operator<<(T &&var) {
    return Serialize(var);
  }

  /** Serialize using ampersand */
  template<typename T>
  BinaryOutputArchive& operator&(T &&var) {
    return Serialize(var);
  }

  /** Serialize a parameter */
  template<typename T, typename ...Args>
  BinaryOutputArchive& Serialize(T &var, Args&& ...args) {
    if constexpr (std::is_base_of<BulkSerializeable, T>::value) {
      if constexpr (std::is_same_v<decltype(var.serialize(*this)), void>) {
        var.serialize(*this);
      } else if constexpr (std::is_same_v<decltype(var.save(*this)), void>) {
        var.save(*this);
      }
    } else if constexpr (std::is_same_v<T, DataTransfer>){
      var.node_id_ = node_id_;
      xfer_.emplace_back(var);
    } else {
      ar_ << var;
    }
    return Serialize(std::forward<Args>(args)...);
  }

  /** End serialization recursion */
  BinaryOutputArchive& Serialize() {
    return *this;
  }

  /** Get serialized data */
  std::vector<DataTransfer> Get() {
    // Serialize metadata parameters
    std::string str = ss_.str();
    void *data = nullptr;
    if (str.size() > 0) {
      data = malloc(str.size());
      memcpy(data, str.data(), str.size());
      ss_.clear();
    }
    xfer_.emplace_back(DT_RECEIVER_READ | DT_FREE_DATA, data, str.size(), node_id_);

    // Return transfer buffers
    return std::move(xfer_);
  }
};

/** Desrialize a data structure */
class BinaryInputArchive {
 public:
  std::vector<DataTransfer> xfer_;
  std::stringstream ss_;
  cereal::BinaryInputArchive ar_;
  int xfer_off_;

 public:
  /** Default constructor */
  BinaryInputArchive(std::vector<DataTransfer> &xfer)
  : xfer_(std::move(xfer)), xfer_off_(0), ss_(), ar_(ss_) {
    auto &param_xfer = xfer_.back();
    ss_.str(std::string((char*)param_xfer.data_, param_xfer.data_size_));
  }

  /** Deserialize using call */
  template<typename T, typename ...Args>
  BinaryInputArchive& operator()(T &var, Args &&...args) {
    return Deserialize(var, std::forward<Args>(args)...);
  }

  /** Deserialize using right shift */
  template<typename T>
  BinaryInputArchive& operator>>(T &var) {
    return Deserialize(var);
  }

  /** Deserialize using ampersand */
  template<typename T>
  BinaryInputArchive& operator&(T &var) {
    return Deserialize(var);
  }

  /** Serialize a parameter */
  template<typename T, typename ...Args>
  BinaryInputArchive& Deserialize(T &var, Args&& ...args) {
    if constexpr (std::is_same_v<T, DataTransfer>) {
      var = xfer_[xfer_off_++];
    } else if constexpr (std::is_base_of<BulkSerializeable, T>::value) {
      if constexpr (std::is_same_v<decltype(var.serialize(*this)), void>) {
        var.serialize(*this);
      } else if constexpr (std::is_same_v<decltype(var.load(*this)), void>) {
        var.load(*this);
      }
    } else {
      ar_ >> var;
    }
    return Deserialize(std::forward<Args>(args)...);
  }

  /** End deserialize recursion */
  BinaryInputArchive& Deserialize() {
    return *this;
  }
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_NETWORK_SERIALIZE_H_
