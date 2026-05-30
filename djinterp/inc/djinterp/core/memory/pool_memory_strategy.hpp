/******************************************************************************
* djinterp [memory]                                     pool_memory_strategy.hpp
*
* Concrete element-typed memory strategy backed by a pool_resource.
*   This is a SPECIFIC strategy module: it adapts the djinterp pool layer to
* the agnostic memory-strategy core.  The core never mentions pools; this
* module is where pool knowledge lives.
*   ёpool_memory_strategy<Pool>ё presents a pool_resource under the canonical
* element-typed surface (allocate(n) / deallocate(p,n)), mapping them onto the
* pool's acquire() / release().  It declares the descriptive constants the core
* contract requires (a pool is dynamic storage), forwarding pointer-stability
* and release semantics from the pool's own policy constants.
*   It is a non-owning handle: copying copies the binding, not the pool.
*
* DEPENDENCIES:
*   memory_strategy_traits.hpp - core contract + storage_kind
*   pool_traits.hpp            - pool policy-constant forwarding
*   pool.hpp                   - pool_resource (the wrapped type)
*
*
* path:      /inc/djinterp/core/memory/pool_memory_strategy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.29
******************************************************************************/

#ifndef DJINTERP_MEMORY_POOL_STRATEGY_
#define DJINTERP_MEMORY_POOL_STRATEGY_ 1

// std
#include <cstddef>
#include <new>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "./pool/pool_traits.hpp"
#include "./memory_strategy_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   pool_memory_strategy
// ===========================================================================

// pool_memory_strategy
//   class: element-typed strategy drawing storage from a pool_resource.
template<typename _Pool>
class pool_memory_strategy
{
public:
    using pool_type  = clean_t<_Pool>;
    using value_type = typename pool_type::value_type;

    // --- descriptive constants (core contract) ---
    static constexpr storage_kind strategy_storage_kind       = storage_kind::dynamic_storage;
    static constexpr bool         pointer_stable              = is_pointer_stable_pool_v<pool_type>;
    static constexpr bool         supports_individual_release = supports_individual_release_v<pool_type>;
    static constexpr bool         supports_generational_sweep = has_generational_sweep_v<pool_type>;

    explicit
    pool_memory_strategy(
        pool_type& _pool
    ) noexcept
        : m_pool(&_pool)
    {}

    // --- element-typed surface ---

    // allocate
    //   maps onto pool acquire().  _n is expected to be 1 (pools vend one slot
    // at a time); _n > 1 falls back to ::operator new, matching pool_allocator.
    value_type*
    allocate(
        std::size_t _n
    )
    {
        if (_n == 1)
        {
            void* slot = m_pool->acquire();

            if (!slot)
            {
                throw std::bad_alloc();
            }

            return static_cast<value_type*>(slot);
        }

        return static_cast<value_type*>(::operator new(_n * sizeof(value_type)));
    }

    // deallocate
    //   maps onto pool release() for _n == 1; ::operator delete otherwise.
    void
    deallocate(
        value_type* _p,
        std::size_t _n
    ) noexcept
    {
        if (!_p)
        {
            return;
        }

        if (_n == 1)
        {
            m_pool->release(static_cast<void*>(_p));
        }
        else
        {
            ::operator delete(static_cast<void*>(_p));
        }

        return;
    }

    // resource
    //   exposes the wrapped pool (parity with pool_allocator::resource()).
    pool_type*
    resource() const noexcept
    {
        return m_pool;
    }

private:
    pool_type* m_pool;
};


// ===========================================================================
// II.  Recognition trait + concept
// ===========================================================================
// (Folded into this strategy module rather than a separate traits/concepts
// pair, since the module already defines a concrete type.  Split out if the
// project prefers strict one-trait-file-per-concepts-file parity.)
NS_INTERNAL
    template<typename _Type,
             typename = void>
    struct is_pool_strategy_check : std::false_type
    {};

    // a pool strategy is an element strategy exposing resource() whose return
    // type is a pool_resource.
    template<typename _Type>
    struct is_pool_strategy_check<_Type, void_t<
        decltype(std::declval<const _Type&>().resource())
    >> : std::integral_constant<bool,
             is_element_strategy<_Type>::value &&
             is_pool_resource_v<std::remove_pointer_t<
                 decltype(std::declval<const _Type&>().resource())>>>
    {};
NS_END  // internal

// is_pool_memory_strategy
//   trait: true if _Type is an element strategy backed by a pool resource.
template<typename _Type>
struct is_pool_memory_strategy
    : internal::is_pool_strategy_check<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_pool_memory_strategy_v =
        is_pool_memory_strategy<_Type>::value;
#endif


#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)
    // pool_memory_strategy_c
    //   concept: constrains element strategies backed by a pool resource.
    template<typename _Type>
    concept pool_memory_strategy_c =
        is_pool_memory_strategy_v<_Type>;
#endif


// ===========================================================================
// III. Convenience factory
// ===========================================================================

// make_pool_strategy
//   factory: deduces the pool type and binds a strategy to it.
template<typename _Pool>
pool_memory_strategy<clean_t<_Pool>>
make_pool_strategy(
    _Pool& _pool
) noexcept
{
    return pool_memory_strategy<clean_t<_Pool>>(_pool);
}


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_POOL_STRATEGY_