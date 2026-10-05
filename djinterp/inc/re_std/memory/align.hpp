/*******************************************************************************
* djinterp [re_std]                                                    align.hpp
*
* runtime alignment helper:
*   align(_alignment, _size, _ptr, _space) attempts to advance _ptr
* to the next address that is aligned to _alignment, such that at least
* _size bytes are available starting at the new address. _space is
* updated to reflect the remaining bytes after the advance.
*
* return value:
*   the aligned pointer (= the new value of _ptr) on success.
*   nullptr on failure (insufficient space). Both _ptr and _space are
*   left unchanged on failure.
*
* _alignment must be a power of two — undefined behaviour otherwise.
*
* added in std C++11; re_std matches the C++11 signature.
*
*
* path:      /inc/re_std/memory/align.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_ALIGN_HPP
#define RE_STD_MEMORY_ALIGN_HPP 1

// std
#include <cstddef>                 // size_t
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "../cstdint/cstdint.hpp"  // uintptr_t, and INTPTR_MAX when it exists


namespace re_std
{

//   align needs an integer that round-trips a pointer. re_std's cstdint
// provides uintptr_t where the target has one (INTPTR_MAX) -- from C++11
// today, and at C++98 once it rests on dstdint.h (decision 4.7) -- and
// align is absent where it does not.
#ifdef INTPTR_MAX

inline void* align(std::size_t _alignment,
                   std::size_t _size,
                   void*&      _ptr,
                   std::size_t& _space) RE_STD_NOEXCEPT
{
    // Compute the offset needed to bring _ptr up to alignment.
    // Mask works because _alignment is required to be a power of two.
    const uintptr_t _addr = reinterpret_cast<uintptr_t>(_ptr);
    const uintptr_t _aligned_addr =
        (_addr + _alignment - 1) & ~(static_cast<uintptr_t>(_alignment) - 1);
    const std::size_t _padding = static_cast<std::size_t>(_aligned_addr - _addr);

    if (_padding + _size > _space)
    {
        return 0;  // not enough space
    }

    _ptr   = reinterpret_cast<void*>(_aligned_addr);
    _space -= _padding;
    return _ptr;
}


#endif  // INTPTR_MAX

}  // re_std
#endif  // RE_STD_MEMORY_ALIGN_HPP
