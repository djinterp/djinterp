/******************************************************************************
* djinterp [memory]                                allocator_memory_strategy.hpp
*
* Concrete element-typed memory strategy backed by a standard Allocator.
*   A SPECIFIC strategy module adapting any type satisfying the C++ Allocator
* named requirements (std::allocator, pmr::polymorphic_allocator, a custom
* allocator) to the agnostic memory-strategy core - without modifying it.
*
*   allocator_memory_strategy<Alloc> owns an Alloc by value and forwards the
* canonical element-typed surface to it.  It declares dynamic storage and no
* pointer stability (heap reallocation may move elements).
*
* DEPENDENCIES:
*   memory_strategy_traits.hpp  - core contract + storage_kind
*
*
* path:      /inc/djinterp/core/memory/strategy/allocator_memory_strategy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.29
******************************************************************************/

#ifndef DJINTERP_MEMORY_ALLOCATOR_STRATEGY_
#define DJINTERP_MEMORY_ALLOCATOR_STRATEGY_ 1

// std
#include <cstddef>
#include <memory>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "./memory_strategy_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   allocator_memory_strategy
// ===========================================================================

// allocator_memory_strategy
//   class: element-typed strategy wrapping a standard Allocator.
template<typename _Alloc>
class allocator_memory_strategy
{
public:
    using allocator_type = clean_t<_Alloc>;
    using traits_type    = std::allocator_traits<allocator_type>;
    using value_type     = typename traits_type::value_type;

    // --- descriptive constants (core contract) ---
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;
    static constexpr bool pointer_stable               = false;
    static constexpr bool supports_individual_release  = true;

    explicit
    allocator_memory_strategy(
        const allocator_type& _alloc = allocator_type()
    ) noexcept
        : m_alloc(_alloc)
    {}

    // --- element-typed surface ---

    value_type*
    allocate(
        std::size_t _n
    )
    {
        return traits_type::allocate(m_alloc, _n);
    }

    void
    deallocate(
        value_type* _p,
        std::size_t _n
    ) noexcept
    {
        traits_type::deallocate(m_alloc, _p, _n);

        return;
    }

    // allocator
    //   exposes the wrapped allocator.
    allocator_type
    allocator() const noexcept
    {
        return m_alloc;
    }

private:
    allocator_type m_alloc;
};


// ===========================================================================
// II.  Recognition trait + concept
// ===========================================================================

NS_INTERNAL
    template<typename _Type,
             typename = void>
    struct is_allocator_strategy_check : std::false_type
    {};

    template<typename _Type>
    struct is_allocator_strategy_check<_Type, void_t<
        typename _Type::allocator_type
    >> : is_element_strategy<_Type>
    {};
NS_END  // internal

// is_allocator_memory_strategy
//   trait: true if _Type is an element strategy exposing an allocator_type.
template<typename _Type>
struct is_allocator_memory_strategy
    : internal::is_allocator_strategy_check<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_allocator_memory_strategy_v =
        is_allocator_memory_strategy<_Type>::value;
#endif


#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)
    template<typename _Type>
    concept allocator_memory_strategy_c =
        is_allocator_memory_strategy_v<_Type>;
#endif


// ===========================================================================
// III. Convenience factory
// ===========================================================================

// make_allocator_strategy
//   factory: deduces the allocator type and wraps it.
template<typename _Alloc>
allocator_memory_strategy<clean_t<_Alloc>>
make_allocator_strategy(
    const _Alloc& _alloc
) noexcept
{
    return allocator_memory_strategy<clean_t<_Alloc>>(_alloc);
}


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_ALLOCATOR_STRATEGY_
