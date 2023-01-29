//
// Created by lukemartinlogan on 1/28/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_TYPES_ARGPACK_H_
#define LABSTOR_INCLUDE_LABSTOR_TYPES_ARGPACK_H_

#include "basic.h"

namespace labstor {

/** The "End Recurrence" type */
struct EndTemplateRecurrence {};

/** Recurrence used to create argument pack */
template<
  size_t idx,
  typename T=EndTemplateRecurrence,
  typename ...Args>
struct ArgPackRecur {
  T&& arg_; /**< The element stored */
  ArgPackRecur<idx + 1, Args...>
    recur_; /**< Remaining args */

  /** Default constructor */
  ArgPackRecur() = default;

  /** Constructor. Const reference. */
  explicit ArgPackRecur(const T &arg, Args&& ...args)
    : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Constructor. Lvalue reference. */
  explicit ArgPackRecur(T& arg, Args&& ...args)
    : arg_(std::forward<T>(arg)), recur_(std::forward<Args>(args)...) {}

  /** Constructor. Rvalue reference. */
  explicit ArgPackRecur(T&& arg, Args&& ...args)
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
};

/** Terminator of the ArgPack recurrence */
template<size_t idx>
struct ArgPackRecur<idx, EndTemplateRecurrence> {
  /** Default constructor */
  ArgPackRecur() = default;

  /** Forward an rvalue reference (only if argpack) */
  template<size_t i>
  void Forward() {
    throw std::logic_error("(Forward) ArgPack index outside of range");
  }
};

/** Used to semantically pack arguments */
template<typename ...Args>
struct ArgPack {
  /** Variable argument pack */
  ArgPackRecur<0, Args...> recur_;

  /** General Constructor. */
  explicit ArgPack(Args&& ...args)
    : recur_(std::forward<Args>(args)...) {}

  /** Move constructor */
  ArgPack(ArgPack &&other) noexcept
    : recur_(std::move(other.objs_)) {}

  /** Move assignment operator */
  ArgPack& operator=(ArgPack &&other) noexcept {
    if (this != &other) {
      recur_ = std::move(other.recur_);
    }
    return *this;
  }

  /** Copy constructor */
  ArgPack(const ArgPack &other)
    : recur_(other.recur_) {}

  /** Copy assignment operator */
  ArgPack& operator=(const ArgPack &other) {
    if (this != &other) {
      recur_ = other.recur_;
    }
    return *this;
  }

  /** Get forward reference */
  template<size_t idx>
  auto&& Forward() {
    return recur_.template Forward<idx>();
  }

  /** Size */
  constexpr size_t Size() {
    return sizeof...(Args);
  }
};

/** Get the type of the forward for \a pack pack at \a index i */
#define FORWARD_ARGPACK_TYPE(pack, i)\
  decltype(pack.template Forward<i>())

/** Forward the param for \a pack pack at \a index i */
#define FORWARD_ARGPACK_PARAM(pack, i)\
  std::forward<FORWARD_ARGPACK_TYPE(pack, i)>(\
    pack.template Forward<i>())

/** Used to pass an argument pack to a function or class method */
class PassArgPack {
 public:
  /** Call function with ArgPack */
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
          std::forward<CurArgs>(args)...,
          std::forward<decltype(pack.template Forward<i>())>(
            pack.template Get<i>()));
      } else {
        return _CallRecur<i + 1, ArgPackT, PackSize, F>(
          std::forward<F>(f),
          pack,
          std::forward<CurArgs>(args)...,
          std::forward<decltype(pack.template Forward<i>())>(
            pack.template Forward<i>()));
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

/** Combine multiple argpacks into a single argpack */
class MergeArgPacks {
 public:
  /** Call function with ArgPack */
  template<typename ...ArgPacks>
  static decltype(auto) Merge(ArgPacks&& ...packs) {
    return _MergePacksRecur<0>(
      ArgPack<ArgPacks...>(std::forward<ArgPacks>(packs)...));
  }

 private:
  /** Unpacks the C++ parameter pack of ArgPacks */
  template<size_t cur_pack, typename ArgPacksT,
    typename ...CurArgs>
  static decltype(auto) _MergePacksRecur(ArgPacksT &&packs,
                                         CurArgs&& ...args) {
    if constexpr(cur_pack < packs.Size()) {
      return _MergeRecur<
        cur_pack, ArgPacksT,
        0, decltype(packs.template Forward<cur_pack>())>(
        // End template parameters
        std::forward<ArgPacksT>(packs),
        FORWARD_ARGPACK_PARAM(packs, cur_pack),
        std::forward<CurArgs>(args)...
      );
    } else {
      return ArgPack<CurArgs...>(std::forward<CurArgs>(args)...);
    }
  }

  /** Unpacks the C++ parameter pack of ArgPacks */
  template<
    size_t cur_pack, typename ArgPacksT,
    size_t i, typename ArgPackT,
    typename ...CurArgs>
  static decltype(auto) _MergeRecur(ArgPacksT &&packs,
                                    ArgPackT &&pack,
                                    CurArgs&& ...args) {
    if constexpr(i < pack.Size()) {
      return _MergeRecur<cur_pack, ArgPacksT, i + 1, ArgPackT>(
        std::forward<ArgPacksT>(packs),
        std::forward<ArgPackT>(pack),
        std::forward<CurArgs>(args)..., FORWARD_ARGPACK_PARAM(pack, i));
    } else {
      return _MergePacksRecur<cur_pack + 1, ArgPacksT>(
        std::forward<ArgPacksT>(packs),
        std::forward<CurArgs>(args)...);
    }
  }
};

/** Insert an argpack at the head of each pack in a set of ArgPacks */
class ProductArgPacks {
 public:
  /** The product function */
  template<typename ProductPackT, typename ...ArgPacks>
  static decltype(auto) Product(ProductPackT &&prod_pack,
                                ArgPacks&& ...packs) {
    return _ProductPacksRecur<0>(
      std::forward<ProductPackT>(prod_pack),
      ArgPack<ArgPacks...>(std::forward<ArgPacks>(packs)...));
  }

 private:
  /** Prepend \a ArgPack prod_pack to every ArgPack in orig_packs */
  template<
    size_t cur_pack,
    typename ProductPackT,
    typename OrigPacksT,
    typename ...NewPacks>
  static decltype(auto) _ProductPacksRecur(ProductPackT &&prod_pack,
                                           OrigPacksT &&orig_packs,
                                           NewPacks&& ...packs) {
    if constexpr(cur_pack < orig_packs.Size()) {
      return _ProductPacksRecur<cur_pack + 1>(
        std::forward<ProductPackT>(prod_pack),
        std::forward<OrigPacksT>(orig_packs),
        std::forward<NewPacks>(packs)...,
        std::forward<ProductPackT>(prod_pack),
        FORWARD_ARGPACK_PARAM(orig_packs, cur_pack));
    } else {
      return ArgPack<NewPacks...>(std::forward<NewPacks>(packs)...);
    }
  }
};

}  // namespace labstor

#endif //LABSTOR_INCLUDE_LABSTOR_TYPES_ARGPACK_H_
