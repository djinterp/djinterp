/******************************************************************************
* djinterp [memory]                                   static_buffer_strategy.hpp
*
* Concrete element-typed memory strategy over compile-time-fixed inline storage.
*   A SPECIFIC strategy module.  static_buffer_strategy<T, N> owns an in-object,
* suitably-aligned buffer for N objects of T and vends slots by bump pointer.
* It declares STATIC storage (capacity fixed at compile time) and MONOTONIC
* release (deallocate is a no-op; storage is reclaimed only by reset()).
* It is pointer-stable: the buffer never moves.
*
*   This module demonstrates that the static-storage axis and the allocating
* (operational) layer are independent: a strategy can be both static AND
* allocating.  For a purely descriptive static tag with no allocation, use
* static_extent_strategy below.
*
* DEPENDENCIES:
*   storage_strategy_traits.hpp  - core contract + storage_kind
*
*
* path:      /inc/djinterp/core/memory/strategy/static_buffer_strategy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.29
******************************************************************************/

#ifndef DJINTERP_MEMORY_STATIC_BUFFER_STRATEGY_
#define DJINTERP_MEMORY_STATIC_BUFFER_STRATEGY_ 1

// std
#include <cstddef>
#include <new>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "./storage_strategy_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   static_buffer_strategy  (operational: static + monotonic)
// ===========================================================================

// static_buffer_strategy
//   class: element-typed monotonic bump strategy over an inline buffer of N
// objects of _Type.
template<typename     _Type,
         std::size_t  _Count>
class static_buffer_strategy
{
public:
    using value_type = _Type;

    // --- descriptive constants (core contract) ---
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::static_storage;
    static constexpr bool         pointer_stable              = true;
    static constexpr bool         supports_individual_release = false;
    static constexpr std::size_t  extent                      = _Count;

    static_buffer_strategy() noexcept
        : m_head(0)
    {}

    // non-copyable: the buffer is in-object, so copying would alias slots.
    static_buffer_strategy(const static_buffer_strategy&)            = delete;
    static_buffer_strategy& operator=(const static_buffer_strategy&) = delete;

    // --- element-typed surface ---

    // allocate
    //   bump-allocates _n contiguous objects; throws bad_alloc on overflow.
    value_type*
    allocate(
        std::size_t _n
    )
    {
        if (m_head + _n > _Count)
        {
            throw std::bad_alloc();
        }

        value_type* slot = reinterpret_cast<value_type*>(m_storage)
                         + m_head;
        m_head += _n;

        return slot;
    }

    // deallocate
    //   monotonic: a no-op.  Storage is reclaimed only by reset().
    void
    deallocate(
        value_type* /*_p*/,
        std::size_t /*_n*/
    ) noexcept
    {
        return;
    }

    // reset
    //   reclaims all storage (does NOT run destructors).
    void
    reset() noexcept
    {
        m_head = 0;

        return;
    }

    // capacity / size
    std::size_t
    capacity() const noexcept
    {
        return _Count;
    }

    std::size_t
    size() const noexcept
    {
        return m_head;
    }

private:
    alignas(_Type) unsigned char m_storage[_Count * sizeof(_Type)];
    std::size_t                  m_head;
};


// ===========================================================================
// II.  static_extent_strategy  (descriptive-only)
// ===========================================================================

// static_extent_strategy
//   struct: a descriptive-only static strategy - carries value_type and the
// static_storage kind but performs no allocation (the container embeds the
// storage and fills it directly).
template<typename     _Type,
         std::size_t  _Count>
struct static_extent_strategy
{
    using value_type = _Type;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::static_storage;
    static constexpr bool         pointer_stable = true;
    static constexpr std::size_t  extent         = _Count;
};


// ===========================================================================
// III. Recognition trait + concept
// ===========================================================================

// is_static_buffer_strategy
//   trait: true if _Type is a static, descriptive-or-allocating strategy that
// advertises a compile-time extent.
NS_INTERNAL
    template<typename _Type,
             typename = void>
    struct has_extent_member : std::false_type
    {};

    template<typename _Type>
    struct has_extent_member<_Type, void_t<
        decltype(_Type::extent)
    >> : std::true_type
    {};
NS_END  // internal

template<typename _Type>
struct is_static_buffer_strategy
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_storage_strategy<clean_type>::value &&
          is_static_strategy<clean_type>::value &&
          internal::has_extent_member<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_static_buffer_strategy_v =
        is_static_buffer_strategy<_Type>::value;
#endif


#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)
    template<typename _Type>
    concept static_buffer_strategy_c =
        is_static_buffer_strategy_v<_Type>;
#endif


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_STATIC_BUFFER_STRATEGY_
