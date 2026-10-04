/***********************************************************************
* re_std                                           as_writable_bytes.hpp
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
* path:      /inc/djinterp/re_std/span/as_writable_bytes.hpp
* link(s):   TBA
* author(s): re_std contributors                       date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_SPAN_AS_WRITABLE_BYTES_
#define DJINTERP_RE_STD_SPAN_AS_WRITABLE_BYTES_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#include <cstddef>  // size_t, byte

#include "re_std/type_traits/type_traits.hpp"  // enable_if, is_const
#include "re_std/span/dynamic_extent.hpp"
#include "re_std/span/span.hpp"

namespace re_std
{

    // as_writable_bytes
    //   function: view the elements of _s as mutable bytes. Disabled when
    //   element_type is const (you cannot obtain a writable byte view of
    //   immutable storage). Result extent mirrors as_bytes.
    template<typename _Type, std::size_t _Extent,
             typename re_std::enable_if<!re_std::is_const<_Type>::value,
                                       int>::type = 0>
    span<std::byte,
         (_Extent == dynamic_extent ? dynamic_extent
                                    : sizeof(_Type) * _Extent)>
    as_writable_bytes(span<_Type, _Extent> _s) noexcept
    {
        return span<std::byte,
                    (_Extent == dynamic_extent
                         ? dynamic_extent
                         : sizeof(_Type) * _Extent)>(
            reinterpret_cast<std::byte*>(_s.data()), _s.size_bytes());
    }

}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_RE_STD_SPAN_AS_WRITABLE_BYTES_
