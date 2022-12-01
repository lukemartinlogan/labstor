//
// Created by lukemartinlogan on 11/29/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_STRING_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_STRING_H_

#include <labstor/data_structures/data_structure.h>
#include <string>

namespace labstor::ipc::lockless {
class string;
}  // namespace labstor::ipc::lockless

namespace labstor::ipc {

template<>
struct ShmArchive<lockless::string> {
  Pointer header_ptr_;
};

template<>
struct ShmHeader<lockless::string> {
  size_t length_;
  char text_[];
};

}  // namespace labstor::ipc

namespace labstor::ipc::lockless {

class string : public ShmDataStructure<string> {
 SHM_DATA_STRUCTURE_TEMPLATE0(string)

 public:
  string() : ShmDataStructure<string>() {}
  string(const char *text) {
    _create_str(text);
  }
  string(const char *text, Allocator *alloc) :
    ShmDataStructure<string>(alloc) {
    _create_str(text);
  }
  string(const std::string &text) {
    _create_str(text.data(), text.size());
  }
  string(const std::string &text, Allocator *alloc) :
    ShmDataStructure<string>(alloc) {
    _create_str(text.data(), text.size());
  }
  string(string &text1, string &text2, Allocator *alloc) :
    ShmDataStructure<string>(alloc) {
    size_t length = text1.size() + text2.size();
    _create_header(length);
    memcpy(header_->text_,
           text1.data(), text1.size());
    memcpy(header_->text_ + size(),
           text2.data(), text2.size());
    header_->text_[length] = 0;
  }

  char& operator[](size_t i) const {
    return header_->text_[i];
  }

  std::string str() {
    return {header_->text_, header_->length_};
  }

  string operator+(string &other) {
    return string(*this, other, other.GetAllocator());
  }

  size_t size() const {
    if (header_ == nullptr) {
      return 0;
    }
    return header_->length_;
  }

  char* data() const {
    return header_->text_;
  }

  void shm_serialize(ShmArchive<string> &ar) {
    ar.header_ptr_ = header_ptr_;
  }

  void shm_deserialize(ShmArchive<string> &ar) {
    InitDataStructure(ar.header_ptr_);
  }

  void shm_init() {}

  void shm_destroy() {
    alloc_->Free(header_ptr_);
  }

  /**
   * Comparison operators
   * */

#define LABSTOR_STR_CMP_OPERATOR(op) \
  bool operator op(const char *other) const { \
    return strncmp(data(), other, size()) op 0; \
  } \
  bool operator op(const std::string &other) const { \
    return strncmp(data(), other.data(), size()) op 0; \
  } \
  bool operator op(const string &other) const { \
    return strncmp(data(), other.data(), size()) op 0; \
  }

  LABSTOR_STR_CMP_OPERATOR(==)
  LABSTOR_STR_CMP_OPERATOR(!=)
  LABSTOR_STR_CMP_OPERATOR(<)
  LABSTOR_STR_CMP_OPERATOR(>)
  LABSTOR_STR_CMP_OPERATOR(<=)
  LABSTOR_STR_CMP_OPERATOR(>=)

 private:
  inline void _create_str(const char *text) {
    size_t length = strlen(text);
    _create_str(text, length);
  }
  inline void _create_header(size_t length) {
    header_ = alloc_->template
      AllocatePtr<ShmHeader<string>>(
      sizeof(ShmHeader<string>) + length + 1,
      header_ptr_);
  }
  inline void _create_str(const char *text, size_t length) {
    _create_header(length);
    memcpy(header_->text_, text, length);
    header_->text_[length] = 0;
    header_->length_ = length;
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_STRING_H_
