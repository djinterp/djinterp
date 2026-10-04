/***********************************************************************
* re_std                                                    as_bytes.hpp
*
* free function as_bytes:
*   Reinterprets a span as a read-only span of bytes. Mirrors
* std::as_bytes (C++20). Back-ported to C++17 — the lowest tier at which
* std::byte exists. On C++11/14 std::byte is unavailable, so this
* overload is omitted (see failure_reason in the coverage data).
*
*   Not constexpr on any tier: the implementation relies on
* reinterpret_cast, which is never a constant expression.
*
*
* path:      /inc/djinterp/re_std/span/as_bytes.hpp
* link(s):   TBA
* author(s): re_std contributors                       date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_SPAN_AS_BYTES_
#define DJINTERP_RE_STD_SPAN_AS_BYTES_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#include <cstddef>  // size_t, byte

#include "re_std/span/dynamic_extent.hpp"
#include "re_std/span/span.hpp"

namespace re_std
{

    // as_bytes
    //   function: view the elements of _s as immutable bytes. The result
    //   extent is (Extent * sizeof(element_type)) when fixed, else
    //   dynamic_extent.
    template<typename _Type, std::size_t _Extent>
    span<const std::byte,
         (_Extent == dynamic_extent ? dynamic_extent
                                    : sizeof(_Type) * _Extent)>
    as_bytes(span<_Type, _Extent> _s) noexcept
    {
        return span<const std::byte,
                    (_Extent == dynamic_extent
                         ? dynamic_extent
                         : sizeof(_Type) * _Extent)>(
            reinterpret_cast<const std::byte*>(_s.data()), _s.size_bytes());
    }

}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_RE_STD_SPAN_AS_BYTES_
