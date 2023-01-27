//
// Created by lukemartinlogan on 1/26/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_TupleBase_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_TupleBase_H_

#include <utility>
#include "basic.h"

namespace labstor {

/** The "End Recurrence" type */
struct EndTemplateRecurrence {};

/** Recurrence used to create argument pack */
template<bool is_argpack,
  size_t idx, typename T=EndTemplateRecurrence, typename ...Args>
struct TupleBaseRecur {
  /** Whether to store element as rvalue reference or raw element */
  typedef typename std::conditional<is_argpack, T&&, T>::type ElementT;
  ElementT arg_; /**< The element stored */
  TupleBaseRecur<is_argpack, idx + 1, Args...> recur_; /**< Remaining args */

  /** Default constructor */
  TupleBaseRecur() = default;

  /** Constructor. Construct arg in-place. */
  explicit TupleBaseRecur(const T &arg, Args&& ...args)
    : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Constructor. Construct arg in-place. */
  explicit TupleBaseRecur(T& arg, Args&& ...args)
  : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Constructor. Rvalue reference. */
  explicit TupleBaseRecur(T&& arg, Args&& ...args)
  : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Forward an rvalue reference (only if argpack) */
  template<size_t i>
  auto&& Forward() {
    if constexpr(i == idx) {
      return std::forward<T>(arg_);
    } else {
      return recur_.template
        Forward<i>();
    }
  }

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
};

/** Terminator of the TupleBase recurrence */
template<bool is_argpack, size_t idx>
struct TupleBaseRecur<is_argpack, idx, EndTemplateRecurrence> {
  /** Default constructor */
  TupleBaseRecur() = default;

  /** Forward an rvalue reference (only if argpack) */
  template<size_t i>
  void Forward() {
    throw std::logic_error("(Forward) TupleBase index outside of range");
  }

  /** Getter */
  template<size_t i>
  void Get() {
    throw std::logic_error("(Get) TupleBase index outside of range");
  }
};

/** Used to semantically pack arguments */
template<bool is_argpack, typename ...Args>
struct TupleBase {
  /** Variable argument pack */
  TupleBaseRecur<is_argpack, 0, Args...> recur_;

  /** Constructor. */
  TupleBase(Args&& ...args)
  : recur_(std::forward<Args>(args)...) {}

  /** Getter */
  template<size_t idx>
  auto&& Forward() {
    return recur_.template Forward<idx>();
  }

  /** Getter */
  template<size_t idx>
  auto& Get() {
    return recur_.template Get<idx>();
  }

  /** Size */
  constexpr size_t Size() {
    return sizeof...(Args);
  }
};

/** ArgPack definition */
template<typename ...Args>
using ArgPack = TupleBase<true, Args...>;

/** Tuple definition */
template<typename ...Args>
using tuple = TupleBase<false, Args...>;

/** Used to pass an argument pack to a function or class method */
class PassArgPack {
 public:
  /** Call function with TupleBase */
  template<typename F, typename ...Args>
  static decltype(auto) Call(ArgPack<Args...> &&pack, F &&f) {
    return _CallRecur<0, ArgPack<Args...>,
      sizeof...(Args), F>(std::forward<F>(f), pack);
  }

  /** Call function with std::tuple */
  template<typename F, typename ...Args>
  static decltype(auto) Call(std::tuple<Args...> &pack, F &&f) {
    return _CallRecur<0, std::tuple<Args...>,
      sizeof...(Args), F>(std::forward<F>(f), pack);
  }

  private:
  /** Unpacks the TupleBase and passes it to the function */
  template<size_t i, typename TupleBaseT, size_t PackSize,
    typename F, typename ...CurArgs>
  static decltype(auto) _CallRecur(F &&f,
                                   TupleBaseT &pack,
                                   CurArgs&& ...args) {
    typedef typename std::result_of<F(CurArgs...)> RetT;

    if constexpr(i < PackSize) {
      if constexpr(std::is_same_v<RetT, void>) {
        _CallRecur<i + 1, TupleBaseT, PackSize, F>(
          std::forward<F>(f),
          pack,
          std::forward<CurArgs>(args)...,
          std::forward<decltype(pack.template Forward<i>())>(
            pack.template Get<i>()));
      } else {
        return _CallRecur<i + 1, TupleBaseT, PackSize, F>(
          std::forward<F>(f),
          pack,
          std::forward<CurArgs>(args)...,
          std::forward<decltype(pack.template Forward<i>())>(
            pack.template Get<i>()));;
      }
    } else {
      if constexpr(std::is_same_v<RetT, void>) {
        f(std::forward<CurArgs>(args)...);
      } else {
        return f(std::forward<CurArgs>(args)...);
      }
    }
  }
};

/** Apply a function over an entire TupleBase / tuple */
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
      f(i, pack.template Get<i>());
      _Apply<i + 1, TupleT, TupleSize, F>(pack, std::forward<F>(f));
    }
  }
};

}  // namespace labstor

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_TupleBase_H_
