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

  /** Constructor. */
  ArgPackRecur(const T &arg, Args&& ...args)
  : arg_(arg), recur_(idx + 1, std::forward<Args>(args)...) {}

  /** Constructor. Construct arg in-place. */
  ArgPackRecur(T &&arg, Args&& ...args)
  : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Getter */
  template<size_t i>
  auto& Get() {
    if constexpr(i < 0) {
      throw std::logic_error("Cannot have tuple index less than 0");
    } else if constexpr(i == idx) {
      return arg_;
    } else if constexpr(i > idx) {
      return recur_.template
        Get<i>();
    }
  }

  /** Size */
  size_t Size() {
    return recur_.Size();
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
    throw std::logic_error("Tuple index outside of range");
  }

  /** Size */
  size_t Size() {
    return idx;
  }
};

/** Used to semantically pack arguments */
template<typename T, typename ...Args>
struct ArgPack {
  /** Variable argument pack */
  ArgPackRecur<0, T, Args...> recur_;

  /** Default constructor */
  ArgPack() = default;

  /** Constructor. Copy arg. */
  ArgPack(const T &arg, Args&& ...args)
  : recur_(arg, std::forward<Args>(args)...) {}

  /** Constructor. Construct arg in-place. */
  ArgPack(T &&arg, Args&& ...args)
  : recur_(std::forward<T>(arg), std::forward<Args>(args)...) {}

  /** Getter */
  template<size_t idx>
  auto& Get() {
    return recur_.template
      Get<idx>();
  }

  /** Size */
  size_t Size() {
    return recur_.template
      Size();
  }
};

}  // namespace labstor

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_ARGPACK_H_
