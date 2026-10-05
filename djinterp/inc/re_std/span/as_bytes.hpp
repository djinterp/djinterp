/*******************************************************************************
* djinterp [re_std]                                                 as_bytes.hpp
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
* path:      /inc/re_std/span/as_bytes.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SPAN_AS_BYTES_HPP
#define RE_STD_SPAN_AS_BYTES_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // size_t, byte

#include "re_std/span/dynamic_extent.hpp"
#include "re_std/span/span.hpp"

namespace re_std
{

    // as_bytes
    //   function: view the elements of _s as immutable bytes. The result
    //   extent is (Extent * sizeof(element_type)) when fixed, else
    //   dynamic_extent.
    template<typename Type, std::size_t Extent>
    span<const std::byte,
         (Extent == dynamic_extent ? dynamic_extent
                                    : sizeof(Type) * Extent)>
    as_bytes(span<Type, Extent> _s) noexcept
    {
        return span<const std::byte,
                    (Extent == dynamic_extent
                         ? dynamic_extent
                         : sizeof(Type) * Extent)>(
            reinterpret_cast<const std::byte*>(_s.data()), _s.size_bytes());
    }

}  // re_std
#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER

#endif  // RE_STD_SPAN_AS_BYTES_HPP
