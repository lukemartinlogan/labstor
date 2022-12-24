/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#ifndef LABSTOR_DATA_STRUCTURES_LOCKLESS_STRING_H_
#define LABSTOR_DATA_STRUCTURES_LOCKLESS_STRING_H_

#include "data_structure.h"
#include <string>

namespace labstor::ipc {

/** forward declaration for string */
class string;

/** string shared-memory header */
struct string_header {
  size_t length_;
  char text_[];
};

/**
 * MACROS used to simplify the string namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME string
#define TYPED_CLASS string
#define TYPED_HEADER string_header

/**
 * A string of characters.
 * */
class string : public ShmContainer<TYPED_CLASS, TYPED_HEADER> {
 public:
  BASIC_SHM_CONTAINER_TEMPLATE

 public:
  /** Default constructor */
  string() = default;

  /** Destructor */
  ~string() {
    if (destructable_) {
      shm_destroy();
    }
  }

  /** Construct for a C-style string in shared memory */
  explicit string(const char *text) {
    shm_init(text);
  }

  /** Construct from a C-style string with allocator in shared memory */
  explicit string(Allocator *alloc, const char *text) {
    shm_init(alloc, text);
  }

  /** Construct for a std::string in shared-memory */
  explicit string(const std::string &text) {
    shm_init(text);
  }

  /** Construct for an std::string with allocator in shared-memory */
  explicit string(Allocator *alloc, const std::string &text) {
    shm_init(alloc, text);
  }

  /** Construct by concatenating two string in shared-memory */
  explicit string(Allocator *alloc, string &text1, string &text2) {
    shm_init(alloc, text1, text2);
  }

  /** Construct a string of a specific length in shared memory */
  explicit string(size_t length) {
    shm_init(length);
  }

  /** Construct a string of specific length and allocator in shared memory */
  explicit string(Allocator *alloc, size_t length) {
    shm_init(alloc, length);
  }

  /** Default constructor */
  void shm_init() {}

  /** Construct for a C-style string in shared memory */
  void shm_init(const char *text) {
    size_t length = strlen(text);
    shm_init(nullptr, length);
    _create_str(text, length);
  }

  /** Construct from a C-style string with allocator in shared memory */
  void shm_init(Allocator *alloc, const char *text) {
    size_t length = strlen(text);
    shm_init(alloc, length);
    _create_str(text, length);
  }

  /** Construct for a std::string in shared-memory */
  void shm_init(const std::string &text) {
    shm_init(nullptr, text.size());
    _create_str(text.data(), text.size());
  }

  /** Construct for an std::string with allocator in shared-memory */
  void shm_init(Allocator *alloc, const std::string &text) {
    shm_init(alloc, text.size());
    _create_str(text.data(), text.size());
  }

  /** Copy constructor */
  void StrongCopy(const string &other) {
    shm_init(other.alloc_, other.size());
    _create_str(other.data(), other.size());
  }

  /** Construct by concatenating two string in shared-memory */
  void shm_init(Allocator *alloc, string &text1, string &text2) {
    size_t length = text1.size() + text2.size();
    shm_init(alloc, length);
    memcpy(header_->text_,
           text1.data(), text1.size());
    memcpy(header_->text_ + text1.size(),
           text2.data(), text2.size());
    header_->text_[length] = 0;
  }

  /** Construct a string of a specific length in shared memory */
  void shm_init(size_t length) {
    shm_init(nullptr, length);
    header_->text_[length] = 0;
  }

  /**
   * Construct a string of specific length and allocator in shared memory
   * */
  void shm_init(Allocator *alloc, size_t length) {
    ShmContainer<TYPED_CLASS, TYPED_HEADER>::shm_init(alloc);
    header_ = alloc_->template
      AllocatePtr<TYPED_HEADER>(
      sizeof(TYPED_HEADER) + length + 1,
      header_ptr_);
    header_->length_ = length;
    header_->text_[length] = 0;
  }

  /**
   * Destroy the shared-memory data.
   * */
  void shm_destroy() {
    if (IsNull()) { return; }
    alloc_->Free(header_ptr_);
    SetNull();
  }

  /** Get character at index i in the string */
  char& operator[](size_t i) const {
    return header_->text_[i];
  }

  /** Convert into a std::string */
  std::string str() const {
    return {header_->text_, header_->length_};
  }

  /** Add two strings together */
  string operator+(string &other) {
    return string(GetAllocator(), *this, other);
  }

  /** Get the size of the current string */
  size_t size() const {
    if (header_ == nullptr) {
      return 0;
    }
    return header_->length_;
  }

  /** Get a constant reference to the C-style string */
  char* c_str() const {
    return header_->text_;
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
   * Comparison operators
   * */

#define LABSTOR_STR_CMP_OPERATOR(op) \
  bool operator op(const char *other) const { \
    return strncmp(data(), other, std::max(size(), strlen(other))) op 0; \
  } \
  bool operator op(const std::string &other) const { \
    return strncmp(data(), other.data(), std::max(size(), other.size())) op 0; \
  } \
  bool operator op(const string &other) const { \
    return strncmp(data(), other.data(), std::max(size(), other.size())) op 0; \
  }

  LABSTOR_STR_CMP_OPERATOR(==)
  LABSTOR_STR_CMP_OPERATOR(!=)
  LABSTOR_STR_CMP_OPERATOR(<)
  LABSTOR_STR_CMP_OPERATOR(>)
  LABSTOR_STR_CMP_OPERATOR(<=)
  LABSTOR_STR_CMP_OPERATOR(>=)

 private:
  inline void _create_str(const char *text, size_t length) {
    memcpy(header_->text_, text, length);
    header_->text_[length] = 0;
  }
};

/** Consider the string as an uniterpreted set of bytes */
typedef string charbuf;

}  // namespace labstor::ipc

namespace std {

/** Hash function for string */
template<>
struct hash<labstor::ipc::string> {
  size_t operator()(const labstor::ipc::string &text) const {
    size_t sum = 0;
    for (size_t i = 0; i < text.size(); ++i) {
      auto shift = static_cast<size_t>(i % sizeof(size_t));
      auto c = static_cast<size_t>((unsigned char)text[i]);
      sum = 31*sum + (c << shift);
    }
    return sum;
  }
};

}  // namespace std

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif  // LABSTOR_DATA_STRUCTURES_LOCKLESS_STRING_H_
