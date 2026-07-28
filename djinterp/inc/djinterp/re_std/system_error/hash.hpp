/***********************************************************************
* restd                                                         hash.hpp
*
* restd::hash specialisations for error_code / error_condition:
*   the functional module defers hash<error_code> and
*   hash<error_condition> to ship with their owning header; this is that
*   header. Both specialise restd::hash (empty primary from
*   restd/functional/hash.hpp). std ships hash<error_code> at C++11 and
*   hash<error_condition> at C++17; restd ships BOTH at C++11 — a
*   six-year back-port of the error_condition specialisation. Both combine
*   the integer value() with the address-identity of the category via
*   reinterpret_cast, so neither is constexpr on any tier (matching std
*   and restd's pointer-hash specialisation).
*
*
* path:      /inc/restd/system_error/hash.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_SYSTEM_ERROR_HASH_
#define RESTD_SYSTEM_ERROR_HASH_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>
#include <cstddef>                  // size_t
// restd
#include "../functional/hash.hpp"   // restd::hash primary template
#include "error_code.hpp"
#include "error_condition.hpp"

NS_RESTD

    // hash<error_code>
    //   struct: specialisation of restd::hash for error_code (std: C++11).
    template<>
    struct hash<error_code>
    {
        std::size_t
        operator()(
            const error_code& _code
        ) const D_NOEXCEPT
        {
            std::size_t value_hash = static_cast<std::size_t>(
                static_cast<unsigned int>(_code.value()));
            std::size_t cat_hash = reinterpret_cast<std::size_t>(
                &_code.category());

            return ( value_hash ^
                     ( cat_hash + 0x9e3779b9u +
                       (value_hash << 6) + (value_hash >> 2) ) );
        }
    };

    // hash<error_condition>
    //   struct: specialisation of restd::hash for error_condition. std ships
    // this at C++17; restd back-ports it to C++11.
    template<>
    struct hash<error_condition>
    {
        std::size_t
        operator()(
            const error_condition& _cond
        ) const D_NOEXCEPT
        {
            std::size_t value_hash = static_cast<std::size_t>(
                static_cast<unsigned int>(_cond.value()));
            std::size_t cat_hash = reinterpret_cast<std::size_t>(
                &_cond.category());

            return ( value_hash ^
                     ( cat_hash + 0x9e3779b9u +
                       (value_hash << 6) + (value_hash >> 2) ) );
        }
    };

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_SYSTEM_ERROR_HASH_
