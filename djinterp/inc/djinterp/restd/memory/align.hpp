/***********************************************************************
* restd                                                              align.hpp
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
* added in std C++11; restd matches the C++11 signature.
*
*
* path:      /inc/restd/memory/align.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_ALIGN_
#define RESTD_MEMORY_ALIGN_ 1

#include "djinterp.hpp"

#include <cstddef>
#include <cstdint>


namespace restd
{

inline void* align(std::size_t _alignment,
                   std::size_t _size,
                   void*&      _ptr,
                   std::size_t& _space) D_NOEXCEPT
{
    // Compute the offset needed to bring _ptr up to alignment.
    // Mask works because _alignment is required to be a power of two.
    const std::uintptr_t _addr = reinterpret_cast<std::uintptr_t>(_ptr);
    const std::uintptr_t _aligned_addr =
        (_addr + _alignment - 1) & ~(static_cast<std::uintptr_t>(_alignment) - 1);
    const std::size_t _padding = static_cast<std::size_t>(_aligned_addr - _addr);

    if (_padding + _size > _space)
    {
        return 0;  // not enough space
    }

    _ptr   = reinterpret_cast<void*>(_aligned_addr);
    _space -= _padding;
    return _ptr;
}


}  // namespace restd

#endif  // RESTD_MEMORY_ALIGN_
