/***********************************************************************
* restd                                            as_writable_bytes.hpp
*
* free function as_writable_bytes:
*   Reinterprets a span over non-const elements as a writable span of
* bytes. Mirrors std::as_writable_bytes (C++20), back-ported to C++17
* (the lowest tier with std::byte). Constrained to non-const element
* types via restd::is_const, matching std.
*
*   Not constexpr on any tier (reinterpret_cast).
*
*
* path:      /inc/restd/span/as_writable_bytes.hpp
* link(s):   TBA
* author(s): restd contributors                        date: 2026.06.04
***********************************************************************/

#ifndef RESTD_SPAN_AS_WRITABLE_BYTES_
#define RESTD_SPAN_AS_WRITABLE_BYTES_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#include <cstddef>  // size_t, byte

#include "restd/type_traits/type_traits.hpp"  // enable_if, is_const
#include "restd/span/dynamic_extent.hpp"
#include "restd/span/span.hpp"

namespace restd
{

    // as_writable_bytes
    //   function: view the elements of _s as mutable bytes. Disabled when
    //   element_type is const (you cannot obtain a writable byte view of
    //   immutable storage). Result extent mirrors as_bytes.
    template<typename _Type, std::size_t _Extent,
             typename restd::enable_if<!restd::is_const<_Type>::value,
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

}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // RESTD_SPAN_AS_WRITABLE_BYTES_
