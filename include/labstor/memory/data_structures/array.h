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


#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_DATA_STRUCTURES_ARRAY_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_DATA_STRUCTURES_ARRAY_H_

#include <cstdint>
#include <labstor/types/basic.h>
#include <labstor/util/errors.h>

namespace labstor::memory {

struct ArrayHeader {
  size_t length_;
  size_t max_length_;
};

template<typename T>
class Array;

template<typename T>
struct ArrayIterator {
  Array<T> &array_;
  size_t i_;

  explicit ArrayIterator(Array<T> &array) : array_(array), i_(0) {}
  explicit ArrayIterator(Array<T> &array, size_t i) : array_(array), i_(i) {}

  T& operator*() const { return array_[i_]; }
  T* operator->() const { return &array_[i_]; }

  ArrayIterator& operator++() {
    ++i_;
    return *this;
  }

  ArrayIterator operator++(int) {
    return ArrayIterator(array_, i_ + 1);
  }

  ArrayIterator operator+(size_t i) {
    if (i_ > array_.size()) {
      return array_.end();
    }
    return ArrayIterator(array_, i_ + i);
  }

  friend bool operator==(const ArrayIterator &a, const ArrayIterator &b) {
    return a.i_ == b.i_;
  }

  friend bool operator!=(const ArrayIterator &a, const ArrayIterator &b) {
    return a.i_ != b.i_;
  }
};

template <typename T>
class Array : public BufferAttachable {
 private:
  ArrayHeader *header_;
  T *array_;

 public:
  Array() : header_(nullptr), array_(nullptr) {}

  bool IsInitialized() {
    return header_ != nullptr;
  }

  bool Create(void *buffer, size_t size) override {
    header_ = reinterpret_cast<ArrayHeader*>(buffer);
    header_->length_ = 0;
    header_->max_length_ = (size - sizeof(ArrayHeader)) / sizeof(T);
    array_ = reinterpret_cast<T*>(header_ + 1);
    return true;
  }

  bool Attach(void *buffer) override {
    header_ = reinterpret_cast<ArrayHeader*>(buffer);
    array_ = reinterpret_cast<T*>(header_ + 1);
    return true;
  }

  T& operator[](const size_t i) {
    return array_[i];
  }

  template<typename... Args>
  void emplace_back(Args&&... args) {
    if (header_->length_ == header_->max_length_) {
      throw ARRAY_OUT_OF_BOUNDS.format("array::emplace_back");
    }
    array_[header_->length_++] = std::move(T(args...));
  }

  size_t size() {
    return header_->length_;
  }

  ArrayIterator<T> begin() {
    return ArrayIterator<T>(*this, 0);
  }

  ArrayIterator<T> end() {
    return ArrayIterator<T>(*this, size());
  }
};

}  // namespace labstor::memory

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_DATA_STRUCTURES_ARRAY_H_
