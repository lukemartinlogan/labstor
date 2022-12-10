//
// Created by lukemartinlogan on 11/29/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_STRING_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_STRING_H_

#include "data_structure.h"
#include <string>

namespace labstor::ipc {

class string;

struct string_header {
  size_t length_;
  char text_[];
};

#define CLASS_NAME string
#define TYPED_CLASS string
#define TYPED_HEADER string_header

class string : public ShmDataStructure<TYPED_CLASS, TYPED_HEADER> {
 public:
  SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)

 public:
  /** Default constructor */
  string() : ShmDataStructure<TYPED_CLASS, TYPED_HEADER>() {}

  /** Construct for a C-style string in shared memory */
  explicit string(const char *text) {
    _create_str(text);
  }

  /** Construct from a C-style string with allocator in shared memory */
  explicit string(const char *text, Allocator *alloc) :
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    _create_str(text);
  }

  /** Construct for a std::string in shared-memory */
  explicit string(const std::string &text) {
    _create_str(text.data(), text.size());
  }

  /** Construct for an std::string with allocator in shared-memory */
  explicit string(const std::string &text, Allocator *alloc) :
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    _create_str(text.data(), text.size());
  }

  /** Disable implicit copy */
  explicit string(const string &other) = delete;

  /** Construct by concatenating two string in shared-memory */
  explicit string(string &text1, string &text2, Allocator *alloc) :
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    size_t length = text1.size() + text2.size();
    _create_header(length);
    memcpy(header_->text_,
           text1.data(), text1.size());
    memcpy(header_->text_ + size(),
           text2.data(), text2.size());
    header_->text_[length] = 0;
  }

  /** Construct a string of a specific length in shared memory */
  explicit string(size_t length) {
    _create_header(length);
    header_->text_[length] = 0;
  }

  /** Construct a string of specific length and allocator in shared memory */
  explicit string(size_t length, Allocator *alloc) :
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    _create_header(length);
    header_->text_[length] = 0;
  }

  /** Moves one string into another */
  string(string&& source) {
    header_ptr_ = source.header_ptr_;
    header_ = source.header_;
    source.header_ptr_.set_null();
  }

  /** Disable assignment (avoids copies) */
  string& operator=(const string &other) = delete;

  /** Get character at index i in the string */
  char& operator[](size_t i) const {
    return header_->text_[i];
  }

  /** Convert into a std::string */
  std::string str() {
    return {header_->text_, header_->length_};
  }

  /** Add two strings together */
  string operator+(string &other) {
    return string(*this, other, other.GetAllocator());
  }

  /** Get the size of the current string */
  size_t size() const {
    if (header_ == nullptr) {
      return 0;
    }
    return header_->length_;
  }

  /** Get a constant reference to the C-style string */
  char* data() const {
    return header_->text_;
  }

  /** Get a mutable reference to the C-style string */
  char* data_mutable() {
    return header_->text_;
  }

  /**
   * Initialize shared-memory, does nothing as it is always handled in
   * constructors
   * */
  void shm_init() {}

  /**
   * Destroy the shared-memory data.
   * */
  void shm_destroy() {
    if (header_ptr_.is_null()) { return; }
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
      AllocatePtr<TYPED_HEADER>(
      sizeof(TYPED_HEADER) + length + 1,
      header_ptr_);
  }
  inline void _create_str(const char *text, size_t length) {
    _create_header(length);
    memcpy(header_->text_, text, length);
    header_->text_[length] = 0;
    header_->length_ = length;
  }
};

/** Consider the string as an uniterpreted set of bytes */
typedef string charbuf;

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_STRING_H_
