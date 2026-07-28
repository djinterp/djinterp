/******************************************************************************
* djinterp [restd]                                              make_optional.hpp
*
* make_optional factory functions:
*   Three overloads mirror std::make_optional from C++17:
*     1. make_optional(T&&)               -- value-deduced single arg
*     2. make_optional<T>(Args&&...)      -- explicit type, in-place
*     3. make_optional<T>(initializer_list<U>, Args&&...) -- with il
*
*   STANDARD STATUS:
*   Introduced in C++17. restd provides on C++11+ since the rest of the
* optional module targets that floor.
*
*   PORTABILITY:
*   Available on C++11 and later. C++98/03 omits the entire optional
* module, so make_optional is unavailable there too.
*
*
* path:      /inc/djinterp/restd/optional/make_optional.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_OPTIONAL_MAKE_OPTIONAL_
#define DJINTERP_RESTD_OPTIONAL_MAKE_OPTIONAL_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if    D_ENV_LANG_IS_CPP11_OR_HIGHER \
    && D_ENV_CPP98_HAS_NEW

#include <initializer_list>

// restd
#include "./optional.hpp"
#include "../utility/in_place.hpp"
#include "../type_traits/decay.hpp"


NS_RESTD


    // make_optional(T&&)
    //   function: value-deduced overload. The result type is
    //             optional<decay<T>::type>, and the value is
    //             forward-constructed in place.
    template<typename _T>
    D_CONSTEXPR optional<typename decay<_T>::type>
    make_optional(_T&& value)
    {
        return optional<typename decay<_T>::type>(
            in_place,
            static_cast<_T&&>(value));
    }


    // make_optional<T>(Args&&...)
    //   function: explicit-type, in-place constructor. The args are
    //             forwarded to T's constructor inside the optional.
    template<typename _T, typename... _Args>
    D_CONSTEXPR optional<_T>
    make_optional(_Args&&... args)
    {
        return optional<_T>(
            in_place,
            static_cast<_Args&&>(args)...);
    }


    // make_optional<T>(initializer_list<U>, Args&&...)
    //   function: explicit-type, in-place constructor accepting an
    //             initializer_list as the first argument.
    template<typename _T, typename _U, typename... _Args>
    D_CONSTEXPR optional<_T>
    make_optional(std::initializer_list<_U> il, _Args&&... args)
    {
        return optional<_T>(
            in_place,
            il,
            static_cast<_Args&&>(args)...);
    }


NS_END  // restd


#endif  // CPP11+ && HAS_NEW

#endif  // DJINTERP_RESTD_OPTIONAL_MAKE_OPTIONAL_
