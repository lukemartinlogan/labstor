//
// Created by lukemartinlogan on 1/26/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_ARGPACK_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_ARGPACK_H_

#include <utility>
#include "basic.h"

namespace labstor {

/** The "End Recurrence" type */
struct EndTemplateRecurrence {};

/** Recurrence used to create argument pack */
template<size_t idx, typename T=EndTemplateRecurrence, typename ...Args>
struct ArgPackRecur {
  T arg_; /**< The argument */
  ArgPackRecur<idx + 1, Args...> recur_; /**< The remaining argument */

  /** Default constructor */
  ArgPackRecur() = default;

  /** Constructor. Construct arg in-place. */
  ArgPackRecur(T arg, Args&& ...args)
  : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Getter */
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

/** Terminator of the ArgPack recurrence */
template<size_t idx>
struct ArgPackRecur<idx, EndTemplateRecurrence> {
  /** Default constructor */
  ArgPackRecur() = default;

  /** Getter */
  template<size_t i>
  void Get() {
    throw std::logic_error("Argpack index outside of range");
  }
};

/** Used to semantically pack arguments */
template<typename ...Args>
struct ArgPack {
  /** Variable argument pack */
  ArgPackRecur<0, Args...> recur_;

  /** Constructor. */
  ArgPack(Args&& ...args)
  : recur_(std::forward<Args>(args)...) {}

  /** Getter */
  template<size_t idx>
  decltype(auto) Get() {
    return recur_.template Get<idx>();
  }

  /** Size */
  constexpr size_t Size() {
    return sizeof...(Args);
  }
};

/** Used to pass an argument pack to a function or class method */
class PassArgPack {
 public:
  /** Call function with ArgPack */
  template<typename F, typename ...Args>
  static decltype(auto) Call(ArgPack<Args...> &pack, F &&f) {
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
  /** Unpacks the ArgPack and passes it to the function */
  template<size_t i, typename ArgPackT, size_t PackSize,
    typename F, typename ...CurArgs>
  static decltype(auto) _CallRecur(F &&f,
                                   ArgPackT &pack,
                                   CurArgs&& ...args) {
    typedef typename std::result_of<F(CurArgs...)> RetT;

    if constexpr(i < PackSize) {
      if constexpr(std::is_same_v<RetT, void>) {
        _CallRecur<i + 1, ArgPackT, PackSize, F>(
          std::forward<F>(f),
          pack,
          std::forward<CurArgs>(args)..., pack.template Get<i>());
      } else {
        return _CallRecur<i + 1, ArgPackT, PackSize, F>(
          std::forward<F>(f),
          pack,
          std::forward<CurArgs>(args)..., pack.template Get<i>());
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

/** Refers to the argpack as a form of tuple */
template<typename ...Args>
using tuple = ArgPack<Args...>;

/** Apply a function over an entire argpack / tuple */
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

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_ARGPACK_H_
