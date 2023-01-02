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

/**
 * MACROS used to simplify the string namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME string
#define TYPED_CLASS string

/** string shared-memory header */
template<>
struct ShmHeader<TYPED_CLASS> : public ShmBaseHeader {
  size_t length_;
  Pointer text_;
};

/**
 * A string of characters.
 * */
class string : public SHM_CONTAINER(TYPED_CLASS) {
 public:
  BASIC_SHM_CONTAINER_TEMPLATE
  char *text_;
  size_t length_;

 public:
  /** Default constructor */
  string() : length_(0) {}

  /** Default shm constructor */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc) {
    shm_init_header(ar, alloc);
  }

  /** Construct from a C-style string with allocator in shared memory */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc, const char *text) {
    size_t length = strlen(text);
    shm_init_main(ar, alloc, length);
    _create_str(text, length);
  }

  /** Construct for an std::string with allocator in shared-memory */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc, const std::string &text) {
    shm_init_main(ar, alloc, text.size());
    _create_str(text.data(), text.size());
  }

  /** Move constructor */
  void WeakMove(string &other) {
    SHM_WEAK_MOVE_START(SHM_WEAK_MOVE_DEFAULT)
    header_->length_ = other.header_->length_;
    header_->text_ = other.header_->text_;
    length_ = other.length_;
    text_ = other.text_;
    alloc_ = other.alloc_;
    SHM_WEAK_MOVE_END()
  }

  /** Copy constructor */
  void StrongCopy(const string &other) {
    SHM_STRONG_COPY_START(SHM_STRONG_COPY_DEFAULT, other.size())
    _create_str(other.data(), other.size());
    SHM_STRONG_COPY_END()
  }

  /** Construct by concatenating two string in shared-memory */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc,
                     const string &text1, const string &text2) {
    size_t length = text1.size() + text2.size();
    shm_init_main(ar, alloc, length);
    memcpy(text_,
           text1.data(), text1.size());
    memcpy(text_ + text1.size(),
           text2.data(), text2.size());
    text_[length] = 0;
  }

  /**
   * Construct a string of specific length and allocator in shared memory
   * */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc, size_t length) {
    shm_init_header(ar, alloc);
    text_ = alloc_->template
      AllocatePtr<char>(
        length + 1,
        header_->text_);
    header_->length_ = length;
    length_ = length;
    text_[length] = 0;
  }

  /**
   * Destroy the shared-memory data.
   * */
  void shm_destroy(bool destroy_header = true) {
    SHM_DESTROY_DATA_START
    alloc_->Free(header_->text_);
    SHM_DESTROY_DATA_END
    SHM_DESTROY_END
  }

  /** Store into shared memory */
  void shm_serialize(ShmArchive<TYPED_CLASS> &ar) const {
    shm_serialize_header(ar.header_ptr_);
  }

  /** Load from shared memory */
  void shm_deserialize(const ShmArchive<TYPED_CLASS> &ar) {
    shm_deserialize_header(ar.header_ptr_);
    text_ = alloc_->template
      Convert<char>(header_->text_);
    length_ = header_->length_;
  }

  /** Get character at index i in the string */
  char& operator[](size_t i) const {
    return text_[i];
  }

  /** Convert into a std::string */
  std::string str() const {
    return {text_, length_};
  }

  /** Add two strings together */
  string operator+(const std::string &other) {
    string tmp(other);
    return string(GetAllocator(), *this, tmp);
  }

  /** Add two strings together */
  string operator+(const string &other) {
    return string(GetAllocator(), *this, other);
  }

  /** Get the size of the current string */
  size_t size() const {
    return length_;
  }

  /** Get a constant reference to the C-style string */
  char* c_str() const {
    return text_;
  }

  /** Get a constant reference to the C-style string */
  char* data() const {
    return text_;
  }

  /** Get a mutable reference to the C-style string */
  char* data_mutable() {
    return text_;
  }

  /**
   * Comparison operators
   * */

  int _strncmp(const char *a, size_t len_a,
               const char *b, size_t len_b) const {
    if (len_a != len_b) {
      return int((int64_t)len_a - (int64_t)len_b);
    }
    int sum = 0;
    for (size_t i = 0; i < len_a; ++i) {
      sum += a[i] - b[i];
    }
    return sum;
  }

#define LABSTOR_STR_CMP_OPERATOR(op) \
  bool operator op(const char *other) const { \
    return _strncmp(data(), size(), other, strlen(other)) op 0; \
  } \
  bool operator op(const std::string &other) const { \
    return _strncmp(data(), size(), other.data(), other.size()) op 0; \
  } \
  bool operator op(const string &other) const { \
    return _strncmp(data(), size(), other.data(), other.size()) op 0; \
  }

  LABSTOR_STR_CMP_OPERATOR(==)
  LABSTOR_STR_CMP_OPERATOR(!=)
  LABSTOR_STR_CMP_OPERATOR(<)
  LABSTOR_STR_CMP_OPERATOR(>)
  LABSTOR_STR_CMP_OPERATOR(<=)
  LABSTOR_STR_CMP_OPERATOR(>=)

 private:
  inline void _create_str(const char *text, size_t length) {
    memcpy(text_, text, length);
    text_[length] = 0;
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
