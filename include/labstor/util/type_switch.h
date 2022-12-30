//
// Created by lukemartinlogan on 12/29/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_UTIL_TYPE_SWITCH_H_
#define LABSTOR_INCLUDE_LABSTOR_UTIL_TYPE_SWITCH_H_

#include <type_traits>

namespace labstor {

/**
 * USAGE: Determine which base class a type belongs to
 *
 * static_switch<T,
 *  static_case<BaseClass1, RetClass1>,
 *  static_case<BaseClass2, RetClass2>,
 *  static_case<RetClass3> // The default case
 * >
 * */

template<class BaseClassT, class RetTypeT = BaseClassT>
struct static_case {
  using BaseClass = BaseClassT;
  using RetType = RetTypeT;
};

template<class T, class Case1, class... OtherCases>
struct static_switch{
  using type = typename std::conditional<
    std::is_base_of<typename Case1::BaseClass, T>::value,
    typename Case1::RetType,
    typename static_switch<T, OtherCases...>::type
  >::type;
};

template<class T, class DefaultCase>
struct static_switch<T, DefaultCase> {
  using type = typename DefaultCase::RetType;
};

}  // namespace labstor

#endif //LABSTOR_INCLUDE_LABSTOR_UTIL_TYPE_SWITCH_H_
