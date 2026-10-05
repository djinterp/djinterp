/*******************************************************************************
* djinterp [re_std]                                        as_writable_bytes.hpp
*
* free function as_writable_bytes:
*   Reinterprets a span over non-const elements as a writable span of
* bytes. Mirrors std::as_writable_bytes (C++20), back-ported to C++17
* (the lowest tier with std::byte). Constrained to non-const element
* types via re_std::is_const, matching std.
*
*   Not constexpr on any tier (reinterpret_cast).
*
*
* path:      /inc/re_std/span/as_writable_bytes.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SPAN_AS_WRITABLE_BYTES_HPP
#define RE_STD_SPAN_AS_WRITABLE_BYTES_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // size_t, byte

#include "re_std/type_traits/type_traits.hpp"           // enable_if, is_const
#include "re_std/span/dynamic_extent.hpp"
#include "re_std/span/span.hpp"

namespace re_std
{

    // as_writable_bytes
    //   function: view the elements of _s as mutable bytes. Disabled when
    //   element_type is const (you cannot obtain a writable byte view of
    //   immutable storage). Result extent mirrors as_bytes.
    template<typename Type, std::size_t Extent,
             typename re_std::enable_if<!re_std::is_const<Type>::value,
                                       int>::type = 0>
    span<std::byte,
         (Extent == dynamic_extent ? dynamic_extent
                                    : sizeof(Type) * Extent)>
    as_writable_bytes(span<Type, Extent> _s) noexcept
    {
        return span<std::byte,
                    (Extent == dynamic_extent
                         ? dynamic_extent
                         : sizeof(Type) * Extent)>(
            reinterpret_cast<std::byte*>(_s.data()), _s.size_bytes());
    }

}  // re_std
#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER

#endif  // RE_STD_SPAN_AS_WRITABLE_BYTES_HPP
