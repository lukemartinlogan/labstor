//
// Created by lukemartinlogan on 1/26/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_TupleBase_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_TupleBase_H_

#include <utility>
#include "basic.h"
#include "argpack.h"

namespace labstor {

/** The null container wrapper */
template<typename T>
using NullWrap = T;

/** Recurrence used to create argument pack */
template<
  template<typename> typename Wrap,
  size_t idx,
  typename T=EndTemplateRecurrence,
  typename ...Args>
struct TupleBaseRecur {
  Wrap<T> arg_; /**< The element stored */
  TupleBaseRecur<Wrap, idx + 1, Args...>
    recur_; /**< Remaining args */

  /** Default constructor */
  TupleBaseRecur() = default;

  /** Constructor. Const reference. */
  explicit TupleBaseRecur(const T &arg, Args&& ...args)
    : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Constructor. Lvalue reference. */
  explicit TupleBaseRecur(T& arg, Args&& ...args)
  : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Constructor. Rvalue reference. */
  explicit TupleBaseRecur(T&& arg, Args&& ...args)
  : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Move constructor */
  TupleBaseRecur(TupleBaseRecur &&other) noexcept
  : arg_(std::move(other.arg_)), recur_(std::move(other.recur_)) {}

  /** Move assignment operator */
  TupleBaseRecur& operator=(TupleBaseRecur &&other) {
    if (this != &other) {
      arg_ = std::move(other.arg_);
      recur_ = std::move(other.recur_);
    }
    return *this;
  }

  /** Copy constructor */
  TupleBaseRecur(const TupleBaseRecur &other)
  : arg_(other.arg_), recur_(other.recur_) {}

  /** Copy assignment operator */
  TupleBaseRecur& operator=(const TupleBaseRecur &other) {
    if (this != &other) {
      arg_ = other.arg_;
      recur_ = other.recur_;
    }
    return *this;
  }

  /** Solidification constructor */
  template<typename ...CArgs>
  explicit TupleBaseRecur(ArgPack<CArgs...> &&other)
    : arg_(other.template Forward<idx>()),
      recur_(std::forward<ArgPack<CArgs...>>(other)) {}

  /** Get reference to internal variable (only if tuple) */
  template<size_t i>
  auto& Get() {
    if constexpr(i == idx) {
      return arg_;
    } else {
      return recur_.template
        Get<i>();
    }
  }

  /** Get reference to internal variable (only if tuple, const) */
  template<size_t i>
  auto& Get() const {
    if constexpr(i == idx) {
      return arg_;
    } else {
      return recur_.template
        Get<i>();
    }
  }
};

/** Terminator of the TupleBase recurrence */
template<
  template<typename> typename Wrap,
  size_t idx>
struct TupleBaseRecur<Wrap, idx, EndTemplateRecurrence> {
  /** Default constructor */
  TupleBaseRecur() = default;

  /** Solidification constructor */
  template<typename ...CArgs>
  explicit TupleBaseRecur(ArgPack<CArgs...> &&other) {}

  /** Getter */
  template<size_t i>
  void Get() {
    throw std::logic_error("(Get) TupleBase index outside of range");
  }

  /** Getter */
  template<size_t i>
  void Get() const {
    throw std::logic_error("(Get) TupleBase index outside of range");
  }
};

/** Used to semantically pack arguments */
template<
  bool is_argpack,
  template<typename> typename Wrap,
  typename ...Args>
struct TupleBase {
  /** Variable argument pack */
  TupleBaseRecur<Wrap, 0, Args...> recur_;

  /** Default constructor */
  TupleBase() = default;

  /** General Constructor. */
  template<typename ...CArgs>
  explicit TupleBase(Args&& ...args)
  : recur_(std::forward<Args>(args)...) {}

  /** Move constructor */
  TupleBase(TupleBase &&other) noexcept
  : recur_(std::move(other.recur_)) {}

  /** Move assignment operator */
  TupleBase& operator=(TupleBase &&other) noexcept {
    if (this != &other) {
      recur_ = std::move(other.recur_);
    }
    return *this;
  }

  /** Copy constructor */
  TupleBase(const TupleBase &other)
  : recur_(other.recur_) {}

  /** Copy assignment operator */
  TupleBase& operator=(const TupleBase &other) {
    if (this != &other) {
      recur_ = other.recur_;
    }
    return *this;
  }

  /** Solidification constructor */
  template<typename ...CArgs>
  explicit TupleBase(ArgPack<CArgs...> &&other)
  : recur_(std::forward<ArgPack<CArgs...>>(other)) {}

  /** Getter */
  template<size_t idx>
  auto& Get() {
    return recur_.template Get<idx>();
  }

  /** Getter (const) */
  template<size_t idx>
  auto& Get() const {
    return recur_.template Get<idx>();
  }

  /** Size */
  constexpr size_t Size() {
    return sizeof...(Args);
  }
};

/** Tuple definition */
template<typename ...Containers>
using tuple = TupleBase<false, NullWrap, Containers...>;

/** Tuple Wrapper Definition */
template<template<typename> typename Wrap, typename ...Containers>
using tuple_wrap = TupleBase<false, Wrap, Containers...>;

/** Apply a function over an entire TupleBase / tuple */
template<bool reverse>
class IterateTuple {
 public:
  /** Apply a function to every element of a tuple */
  template<typename F, typename ...Args>
  static void Apply(tuple<Args...> &pack, F &&f) {
    _Apply<0, tuple<Args...>,
      sizeof...(Args), F>(pack, std::forward<F>(f));
  }

  /** Apply a function to every element of a std::tuple */
  template<typename F, typename ...Args>
  static void Apply(std::tuple<Args...> &pack, F &&f) {
    _Apply<0, std::tuple<Args...>,
      sizeof...(Args), F>(pack, std::forward<F>(f));
  }

 private:
  /** Apply the function recursively */
  template<size_t i, typename TupleT, size_t TupleSize, typename F>
  static void _Apply(TupleT &pack, F &&f) {
    if constexpr(i < TupleSize) {
      if constexpr(reverse) {
        _Apply<i + 1, TupleT, TupleSize, F>(pack, std::forward<F>(f));
        f(i, pack.template Get<i>());
      } else {
        f(i, pack.template Get<i>());
        _Apply<i + 1, TupleT, TupleSize, F>(pack, std::forward<F>(f));
      }
    }
  }
};

/** Forward iterate over tuple and apply function  */
using ForwardIterateTuple = IterateTuple<false>;

/** Reverse iterate over tuple and apply function */
using ReverseIterateTuple = IterateTuple<true>;

}  // namespace labstor

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_TupleBase_H_
