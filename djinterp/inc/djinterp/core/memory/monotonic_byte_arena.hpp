/******************************************************************************
* djinterp [memory]                                    monotonic_byte_arena.hpp
*
* Concrete BYTE-typed memory strategy: a monotonic, aligned bump arena.
*   A SPECIFIC strategy module demonstrating that byte-typed strategies are
* first-class and instantiable, not merely detectable.  monotonic_byte_arena<N>
* owns an inline buffer of N bytes and vends aligned byte blocks by bump
* pointer through the pmr-shaped byte surface:
*     allocate(bytes, align)            -> void*
*     deallocate(void*, bytes, align)   -> no-op (monotonic)
*
*   Because it speaks the byte surface, it carries no value_type.  Pair it with
* core's element_strategy_view<arena, T> to obtain a typed element strategy for
* any T - the universal byte -> element bridge.
*
*   It declares STATIC storage (compile-time-fixed N) and monotonic release.
* For a runtime-sized arena, instantiate dynamic_byte_arena (below), which
* declares FIXED storage (capacity set at construction, non-growable).
*
* DEPENDENCIES:
*   storage_strategy_traits.hpp  - core contract + storage_kind
*
*
* path:      /inc/djinterp/core/memory/strategy/monotonic_byte_arena.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.29
******************************************************************************/

#ifndef DJINTERP_MEMORY_MONOTONIC_BYTE_ARENA_
#define DJINTERP_MEMORY_MONOTONIC_BYTE_ARENA_ 1

// std
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "./storage_strategy_traits.hpp"


NS_DJINTERP


NS_INTERNAL
    // align_up
    //   helper: rounds _n up to the next multiple of _align (power of two).
    inline std::size_t
    align_up(
        std::size_t _n,
        std::size_t _align
    ) noexcept
    {
        return ( (_n + _align - 1) & ~(_align - 1) );
    }
NS_END  // internal


// ===========================================================================
// I.   monotonic_byte_arena  (static storage)
// ===========================================================================

// monotonic_byte_arena
//   class: byte-typed monotonic bump arena over N inline bytes.
template<std::size_t _Bytes,
         std::size_t _MaxAlign = alignof(std::max_align_t)>
class monotonic_byte_arena
{
public:
    using value_type = void;

    // --- descriptive constants (core contract) ---
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::static_storage;
    static constexpr bool         pointer_stable              = true;
    static constexpr bool         supports_individual_release = false;

    monotonic_byte_arena() noexcept
        : m_head(0)
    {}

    monotonic_byte_arena(const monotonic_byte_arena&)            = delete;
    monotonic_byte_arena& operator=(const monotonic_byte_arena&) = delete;

    // --- byte-typed surface ---

    void*
    allocate(
        std::size_t _bytes,
        std::size_t _align = _MaxAlign
    )
    {
        std::size_t aligned = internal::align_up(m_head, _align);

        if (aligned + _bytes > _Bytes)
        {
            throw std::bad_alloc();
        }

        void* p = static_cast<void*>(m_storage + aligned);
        m_head  = aligned + _bytes;

        return p;
    }

    void
    deallocate(
        void*       /*_p*/,
        std::size_t /*_bytes*/,
        std::size_t /*_align*/ = _MaxAlign
    ) noexcept
    {
        return;  // monotonic
    }

    void
    reset() noexcept
    {
        m_head = 0;

        return;
    }

    std::size_t bytes_capacity() const noexcept { return _Bytes; }
    std::size_t bytes_in_use()   const noexcept { return m_head; }

private:
    alignas(_MaxAlign) unsigned char m_storage[_Bytes];
    std::size_t                      m_head;
};


// ===========================================================================
// II.  dynamic_byte_arena  (runtime-fixed storage)
// ===========================================================================

// dynamic_byte_arena
//   class: byte-typed monotonic bump arena over a heap buffer whose size is
// fixed at construction (non-growable -> fixed_storage).
class dynamic_byte_arena
{
public:
    using value_type = void;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::fixed_storage;
    static constexpr bool         pointer_stable              = true;
    static constexpr bool         supports_individual_release = false;

    explicit
    dynamic_byte_arena(
        std::size_t _bytes,
        std::size_t _max_align = alignof(std::max_align_t)
    )
        : m_base(static_cast<unsigned char*>(
                     ::operator new(_bytes, std::align_val_t(_max_align)))),
          m_size(_bytes),
          m_head(0),
          m_max_align(_max_align)
    {}

    ~dynamic_byte_arena()
    {
        ::operator delete(static_cast<void*>(m_base),
                          std::align_val_t(m_max_align));
    }

    dynamic_byte_arena(const dynamic_byte_arena&)            = delete;
    dynamic_byte_arena& operator=(const dynamic_byte_arena&) = delete;

    void*
    allocate(
        std::size_t _bytes,
        std::size_t _align = alignof(std::max_align_t)
    )
    {
        std::size_t aligned = internal::align_up(m_head, _align);

        if (aligned + _bytes > m_size)
        {
            throw std::bad_alloc();
        }

        void* p = static_cast<void*>(m_base + aligned);
        m_head  = aligned + _bytes;

        return p;
    }

    void
    deallocate(
        void*       /*_p*/,
        std::size_t /*_bytes*/,
        std::size_t /*_align*/ = alignof(std::max_align_t)
    ) noexcept
    {
        return;  // monotonic
    }

    void reset() noexcept { m_head = 0; }

    std::size_t bytes_capacity() const noexcept { return m_size; }
    std::size_t bytes_in_use()   const noexcept { return m_head; }

private:
    unsigned char* m_base;
    std::size_t    m_size;
    std::size_t    m_head;
    std::size_t    m_max_align;
};


// ===========================================================================
// III. Recognition concept
// ===========================================================================

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)
    // byte_arena_strategy_c
    //   concept: a byte strategy that is monotonic (no individual release).
    template<typename _Type>
    concept byte_arena_strategy_c =
        is_byte_strategy_v<_Type> &&
        is_monotonic_strategy_v<_Type>;
#endif


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_MONOTONIC_BYTE_ARENA_
