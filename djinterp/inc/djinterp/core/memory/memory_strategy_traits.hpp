/******************************************************************************
* djinterp [memory]                                  memory_strategy_traits.hpp
*
* SFINAE structural traits for the memory-strategy axis  (CORE).
*   A "memory strategy" is any type that DECLARES - and optionally SUPPLIES -
* the storage backing a set of objects.  This header is the strategy- and
* container-agnostic core of the axis: it names no concrete provider (no pool,
* no allocator, no array) and reasons about no container shape.  Concrete
* providers are adapted in their own modules; container resolution lives in
* container_memory_strategy.hpp.
*
THE CONTRACT (two layers, mirroring the pool-trait layering):
*   DESCRIPTIVE  (is_memory_strategy)
*     The single universal signal: the type declares its storage discipline
*     through the `strategy_storage_kind` constant.  Every strategy - static
*     or dynamic, element- or byte-typed, allocating or not - declares this.
*     Static, descriptive-only strategies live here and stop here.
*   OPERATIONAL  (is_allocating_strategy)
*     A refinement that ALSO supplies and reclaims storage at runtime,
*     through one of two typed surfaces (below).
* THE TWO TYPINGS  (fully orthogonal; complete and total support for both):
*   ELEMENT-TYPED   (is_element_strategy)   - std::allocator-shaped
*     - nested `value_type`                    the element type
*     - allocate(size_t n)        -> value_type*    n objects
*     - deallocate(value_type*, size_t n)
*   BYTE-TYPED      (is_byte_strategy)       - std::pmr::memory_resource-shaped
*     - allocate(size_t bytes, size_t align)        -> void*
*     - deallocate(void*, size_t bytes, size_t align)
*     (no value_type required; value_type, if present, is void/byte)
*   Both typings are detected, classified, concept-constrained (see the
* concepts header), instantiable (see the concrete strategy modules), and
* interconvertible via the bridges in section VIII:
*     - element_strategy_view<Byte, T>   any byte strategy  -> element of T
*     - byte_strategy_view<Element>      byte-granular element -> byte surface
* The first direction is universal (this is how pmr builds typed allocators);
* the second is only meaningful when the element type is byte-granular
* (sizeof == 1), which is enforced, not silently faked.
*
*   STORAGE DISCIPLINE is read solely from the declared constant - no shape
* inference here.  static / fixed / dynamic confirmation is therefore exact
* and provider-independent.
*
* PORTABILITY:
*   C++11 baseline.  All `_v` aliases gated on
* D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES.
*
* DEPENDENCIES:
*   type_traits.hpp  - detection macros, clean_t, detected_or_t
*   (NONE on container or pool modules - this is the agnostic core.)
*
* NOTE ON storage_kind:
*   This header owns the `storage_kind` vocabulary.  container_storage_traits.hpp
* currently declares an identical enum; the two should be unified by having that
* header include this one.  Until then, do not include both in one TU.
*
*
* path:      /inc/djinterp/core/memory/memory_strategy_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.29
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    storage_kind vocabulary
II.   constant detection
III.  typed-surface detection
IV.   typing classification
V.    descriptive / operational classification
VI.   storage-discipline confirmation
VII.  stability and release refinements
VIII. element <-> byte bridges
IX.   uniform accessor (customization point)
X.    aggregate snapshot
*/

#ifndef DJINTERP_MEMORY_STRATEGY_TRAITS_
#define DJINTERP_MEMORY_STRATEGY_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../env/cpp/env_cpp_features.h"


NS_DJINTERP


// ===========================================================================
// I.   storage_kind vocabulary
// ===========================================================================

// storage_kind
//   enum: the storage discipline a strategy declares.  Shared storage
// vocabulary (see header note on unification with container_storage_traits).
enum class storage_kind
{
    static_storage,    // compile-time fixed capacity, in-object
    fixed_storage,     // runtime-fixed capacity, non-growable
    dynamic_storage,   // heap-allocated, growable
    unknown            // not a recognizable strategy
};


NS_INTERNAL
    // value_type_of_alias
    //   op alias for detected_or_t: yields _Type::value_type when present.
    template<typename _Type>
    using value_type_of_alias = typename _Type::value_type;
NS_END  // internal


// ===========================================================================
// II.  Constant detection
// ===========================================================================
// The descriptive surface is a small vocabulary of static constants.  Core
// detects them by name only; it never asks where they came from.

// has_strategy_kind_constant
//   trait: detects the mandatory `strategy_storage_kind` constant.
D_TRAIT_IS_DETECTED(has_strategy_kind_constant,
                    decltype(_Type::strategy_storage_kind))

// has_strategy_pointer_stable_constant
//   trait: detects the optional `pointer_stable` constant.
D_TRAIT_IS_DETECTED(has_strategy_pointer_stable_constant,
                    decltype(_Type::pointer_stable))

// has_strategy_individual_release_constant
//   trait: detects the optional `supports_individual_release` constant.
D_TRAIT_IS_DETECTED(has_strategy_individual_release_constant,
                    decltype(_Type::supports_individual_release))

// has_strategy_generational_constant
//   trait: detects the optional `supports_generational_sweep` constant.
D_TRAIT_IS_DETECTED(has_strategy_generational_constant,
                    decltype(_Type::supports_generational_sweep))

// has_strategy_value_type
//   trait: detects a nested `value_type` alias.
D_TRAIT_HAS_TYPE(has_strategy_value_type, value_type)


// ===========================================================================
// III. Typed-surface detection
// ===========================================================================

// --- element-typed surface ---

// has_element_allocate
//   trait: detects allocate(size_t) - the one-argument element supply verb.
D_TRAIT_IS_DETECTED(has_element_allocate,
                    decltype(std::declval<_Type&>().allocate(
                        std::declval<std::size_t>())))

// has_element_deallocate
//   trait: detects deallocate(value_type*, size_t).  Probed only for types
// with a value_type (guarded by the internal check).
NS_INTERNAL
    template<typename _Type,
             typename = void>
    struct element_deallocate_check : std::false_type
    {};

    template<typename _Type>
    struct element_deallocate_check<_Type, void_t<
        decltype(std::declval<_Type&>().deallocate(
            std::declval<typename _Type::value_type*>(),
            std::declval<std::size_t>()))
    >> : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct has_element_deallocate
    : internal::element_deallocate_check<clean_t<_Type>>
{};

// --- byte-typed surface ---

// has_byte_allocate
//   trait: detects allocate(size_t bytes, size_t align) -> the two-argument
// byte supply verb (pmr-shaped).
D_TRAIT_IS_DETECTED(has_byte_allocate,
                    decltype(std::declval<_Type&>().allocate(
                        std::declval<std::size_t>(),
                        std::declval<std::size_t>())))

// has_byte_deallocate
//   trait: detects deallocate(void*, size_t bytes, size_t align).
D_TRAIT_IS_DETECTED(has_byte_deallocate,
                    decltype(std::declval<_Type&>().deallocate(
                        std::declval<void*>(),
                        std::declval<std::size_t>(),
                        std::declval<std::size_t>())))


#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_element_deallocate_v =
        has_element_deallocate<_Type>::value;
#endif


// ===========================================================================
// IV.  Typing classification
// ===========================================================================

// strategy_typing
//   enum: which typed surface a strategy operates through.
enum class strategy_typing
{
    element_typed,    // value_type + allocate(n)/deallocate(p,n)
    byte_typed,       // allocate(bytes,align)/deallocate(p,bytes,align)
    none              // descriptive-only, or not a strategy
};

// is_element_strategy
//   trait: true if _Type exposes the complete element-typed surface.
template<typename _Type>
struct is_element_strategy
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_strategy_value_type<clean_type>::value &&
            !is_void<detected_or_t<void,
                                   internal::value_type_of_alias, clean_type>>::value &&
            has_element_allocate<clean_type>::value &&
            has_element_deallocate<clean_type>::value );
};

// is_byte_strategy
//   trait: true if _Type exposes the complete byte-typed surface.
template<typename _Type>
struct is_byte_strategy
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_byte_allocate<clean_type>::value &&
          has_byte_deallocate<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_element_strategy_v = is_element_strategy<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_byte_strategy_v = is_byte_strategy<_Type>::value;
#endif

// strategy_typing_of
//   trait: the typing discipline of a strategy.  Element wins when both
// surfaces are present (a typed view is the more specific contract).
template<typename _Type>
struct strategy_typing_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr strategy_typing value =
        ( is_element_strategy<clean_type>::value 
              ? strategy_typing::element_typed
              : is_byte_strategy<clean_type>::value 
                  ? strategy_typing::byte_typed
                  : strategy_typing::none );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr strategy_typing strategy_typing_of_v = strategy_typing_of<_Type>::value;
#endif


// ===========================================================================
// V.   Descriptive / operational classification
// ===========================================================================

// is_memory_strategy
//   trait: descriptive contract.  True iff _Type declares its storage
// discipline.  This is the single, provider-independent membership test.
template<typename _Type>
struct is_memory_strategy
    : std::integral_constant<bool,
          has_strategy_kind_constant<clean_t<_Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_memory_strategy_v =
        is_memory_strategy<_Type>::value;
#endif

// is_non_memory_strategy
//   trait: explicit negation for disjoint requires-clauses / SFINAE branches.
template<typename _Type>
struct is_non_memory_strategy
    : std::integral_constant<bool,
          !is_memory_strategy<_Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_non_memory_strategy_v =
        is_non_memory_strategy<_Type>::value;
#endif

// is_allocating_strategy
//   trait: operational refinement.  A memory strategy that also supplies and
// reclaims storage through either typed surface.
template<typename _Type>
struct is_allocating_strategy
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_memory_strategy<clean_type>::value &&
            (  is_element_strategy<clean_type>::value ||
               is_byte_strategy<clean_type>::value ) 
        );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_allocating_strategy_v =
        is_allocating_strategy<_Type>::value;
#endif

// is_descriptive_only_strategy
//   trait: a memory strategy that declares discipline but does not allocate
// (static / in-object storage that the container fills directly).
template<typename _Type>
struct is_descriptive_only_strategy
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_memory_strategy<clean_type>::value &&
          !is_allocating_strategy<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_descriptive_only_strategy_v =
        is_descriptive_only_strategy<_Type>::value;
#endif


// ===========================================================================
// VI.  Storage-discipline confirmation
// ===========================================================================
// Read solely from the declared constant - exact and provider-independent.

NS_INTERNAL
    template<typename _Type,
             bool _HasConstant =
                 has_strategy_kind_constant_v<clean_t<_Type>>>
    struct kind_of : std::integral_constant<storage_kind,
                         storage_kind::unknown>
    {};

    template<typename _Type>
    struct kind_of<_Type, true>
        : std::integral_constant<storage_kind,
              clean_t<_Type>::strategy_storage_kind>
    {};
NS_END  // internal

// memory_strategy_kind_of
//   trait: the declared storage_kind of a strategy.
template<typename _Type>
struct memory_strategy_kind_of
    : std::integral_constant<storage_kind,
          internal::kind_of<clean_t<_Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr storage_kind memory_strategy_kind_of_v =
        memory_strategy_kind_of<_Type>::value;
#endif

// is_static_strategy / is_fixed_strategy / is_dynamic_strategy
template<typename _Type>
struct is_static_strategy
    : std::integral_constant<bool,
          ( internal::kind_of<clean_t<_Type>>::value
                == storage_kind::static_storage )>
{};

template<typename _Type>
struct is_fixed_strategy
    : std::integral_constant<bool,
          ( internal::kind_of<clean_t<_Type>>::value
                == storage_kind::fixed_storage )>
{};

template<typename _Type>
struct is_dynamic_strategy
    : std::integral_constant<bool,
          ( internal::kind_of<clean_t<_Type>>::value
                == storage_kind::dynamic_storage )>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_static_strategy_v =
        is_static_strategy<_Type>::value;
    template<typename _Type>
    inline constexpr bool is_fixed_strategy_v =
        is_fixed_strategy<_Type>::value;
    template<typename _Type>
    inline constexpr bool is_dynamic_strategy_v =
        is_dynamic_strategy<_Type>::value;
#endif


// ===========================================================================
// VII. Stability and release refinements
// ===========================================================================
// All read from declared constants; absent constants default to the
// conservative answer (not stable, not individually releasing, not
// generational -> hence monotonic).

NS_INTERNAL

    template<typename _Type,
             bool = has_strategy_pointer_stable_constant_v<clean_t<_Type>>>
    struct ptr_stable_read : std::false_type
    {};

    template<typename _Type>
    struct ptr_stable_read<_Type, true>
        : std::bool_constant<clean_t<_Type>::pointer_stable> {};

    template<typename _Type,
             bool = has_strategy_individual_release_constant_v<clean_t<_Type>>>
    struct indiv_release_read : std::false_type {};
    template<typename _Type>
    struct indiv_release_read<_Type, true>
        : std::bool_constant<clean_t<_Type>::supports_individual_release> {};

    template<typename _Type,
             bool = has_strategy_generational_constant_v<clean_t<_Type>>>
    struct generational_read : std::false_type {};
    template<typename _Type>
    struct generational_read<_Type, true>
        : std::bool_constant<clean_t<_Type>::supports_generational_sweep> {};

NS_END  // internal

// is_pointer_stable_strategy
template<typename _Type>
struct is_pointer_stable_strategy
    : std::integral_constant<bool,
          internal::ptr_stable_read<_Type>::value>
{};

// supports_individual_release_strategy
template<typename _Type>
struct supports_individual_release_strategy
    : std::integral_constant<bool,
          internal::indiv_release_read<_Type>::value>
{};

// is_monotonic_strategy
//   trait: an allocating strategy that does NOT support individual release -
// memory is reclaimed only on reset / destruction.
template<typename _Type>
struct is_monotonic_strategy
    : std::integral_constant<bool,
          (    is_allocating_strategy<clean_t<_Type>>::value
            && !internal::indiv_release_read<_Type>::value )>
{};

// is_generational_strategy
template<typename _Type>
struct is_generational_strategy
    : std::integral_constant<bool,
          internal::generational_read<_Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_pointer_stable_strategy_v =
        is_pointer_stable_strategy<_Type>::value;
    template<typename _Type>
    inline constexpr bool supports_individual_release_strategy_v =
        supports_individual_release_strategy<_Type>::value;
    template<typename _Type>
    inline constexpr bool is_monotonic_strategy_v =
        is_monotonic_strategy<_Type>::value;
    template<typename _Type>
    inline constexpr bool is_generational_strategy_v =
        is_generational_strategy<_Type>::value;
#endif


// ===========================================================================
// VIII. element <-> byte bridges
// ===========================================================================
// These make dual-typing total: any byte strategy can present an element
// surface for any T; a byte-granular element strategy can present a byte
// surface.  Both are zero-state handles (a pointer to the wrapped strategy).

// element_strategy_view
//   adapter: presents byte strategy _Byte under the element-typed surface for
// _Type.  This is the universal direction (how pmr builds typed allocators).
template<typename _Byte,
         typename _Type>
class element_strategy_view
{
public:
    using value_type  = _Type;
    using byte_type   = clean_t<_Byte>;

    static constexpr storage_kind strategy_storage_kind =
        memory_strategy_kind_of<byte_type>::value;
    static constexpr bool pointer_stable =
        is_pointer_stable_strategy<byte_type>::value;
    static constexpr bool supports_individual_release =
        supports_individual_release_strategy<byte_type>::value;
    static constexpr bool supports_generational_sweep =
        is_generational_strategy<byte_type>::value;

    explicit
    element_strategy_view(
        byte_type& _bytes
    ) noexcept
        : m_bytes(&_bytes)
    {}

    value_type*
    allocate(
        std::size_t _n
    )
    {
        return static_cast<value_type*>(
            m_bytes->allocate(_n * sizeof(value_type), alignof(value_type)));
    }

    void
    deallocate(
        value_type* _p,
        std::size_t _n
    ) noexcept
    {
        m_bytes->deallocate(static_cast<void*>(_p),
                            _n * sizeof(value_type),
                            alignof(value_type));

        return;
    }

    byte_type*
    underlying() const noexcept
    {
        return m_bytes;
    }

private:
    byte_type* m_bytes;
};


// byte_strategy_view
//   adapter: presents a BYTE-GRANULAR element strategy (value_type of size 1)
// under the byte-typed surface.  Enabled only when sizeof(value_type) == 1, so
// the conversion is exact rather than silently lossy.  For wider element types
// a byte view is not well defined (a typed pool cannot honor arbitrary
// alignment / byte counts) and is intentionally not provided.
template<typename _Element,
         typename _Enable = void>
class byte_strategy_view
{
    static_assert(sizeof(_Element) == 0,
        "byte_strategy_view requires a byte-granular element strategy "
        "(value_type of size 1). A byte surface over a wider element type "
        "is not well defined; use the element_strategy_view direction, or a "
        "native byte strategy.");
};

template<typename _Element>
class byte_strategy_view<_Element,
    typename std::enable_if<
        ( sizeof(typename clean_t<_Element>::value_type) == 1 )
    >::type>
{
public:
    using element_type = clean_t<_Element>;
    using value_type   = void;
    using cell_type    = typename element_type::value_type;  // 1-byte cell

    static constexpr storage_kind strategy_storage_kind =
        memory_strategy_kind_of<element_type>::value;
    static constexpr bool pointer_stable =
        is_pointer_stable_strategy<element_type>::value;

    explicit
    byte_strategy_view(
        element_type& _elem
    ) noexcept
        : m_elem(&_elem)
    {}

    void*
    allocate(
        std::size_t _bytes,
        std::size_t /*_align*/
    )
    {
        // 1-byte cells: byte count == cell count; alignment of a 1-byte cell
        // is 1, so any requested alignment <= the platform's guarantee for
        // the underlying storage is satisfied by the element strategy.
        return static_cast<void*>(m_elem->allocate(_bytes));
    }

    void
    deallocate(
        void*       _p,
        std::size_t _bytes,
        std::size_t /*_align*/
    ) noexcept
    {
        m_elem->deallocate(static_cast<cell_type*>(_p), _bytes);

        return;
    }

    element_type*
    underlying() const noexcept
    {
        return m_elem;
    }

private:
    element_type* m_elem;
};


// ===========================================================================
// IX.  Uniform accessor (customization point)
// ===========================================================================
// memory_strategy_traits<S> exposes the descriptive surface uniformly for both
// typings.  Detection elsewhere stays structural; this exists so foreign /
// non-conforming strategies can be adapted by SPECIALIZING this one struct
// rather than by being modified.

template<typename _Type>
struct memory_strategy_traits
{
private:
    using clean_type = clean_t<_Type>;

public:
    using strategy_type = clean_type;
    using value_type    =
        detected_or_t<void, internal::value_type_of_alias, clean_type>;

    static constexpr strategy_typing typing =
        strategy_typing_of<clean_type>::value;
    static constexpr storage_kind kind =
        memory_strategy_kind_of<clean_type>::value;

    static constexpr bool is_element    = ( typing == strategy_typing::element_typed );
    static constexpr bool is_byte       = ( typing == strategy_typing::byte_typed );
    static constexpr bool allocates     = is_allocating_strategy<clean_type>::value;
    static constexpr bool pointer_stable = is_pointer_stable_strategy<clean_type>::value;
    static constexpr bool monotonic     = is_monotonic_strategy<clean_type>::value;
    static constexpr bool generational  = is_generational_strategy<clean_type>::value;
};


// ===========================================================================
// X.   Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct memory_strategy_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool is_strategy =
        is_memory_strategy<clean_type>::value;
    static constexpr bool is_allocating =
        is_allocating_strategy<clean_type>::value;
    static constexpr bool is_descriptive_only =
        is_descriptive_only_strategy<clean_type>::value;
    static constexpr bool is_element =
        is_element_strategy<clean_type>::value;
    static constexpr bool is_byte =
        is_byte_strategy<clean_type>::value;
    static constexpr bool pointer_stable =
        is_pointer_stable_strategy<clean_type>::value;
    static constexpr bool individual_release =
        supports_individual_release_strategy<clean_type>::value;
    static constexpr bool monotonic =
        is_monotonic_strategy<clean_type>::value;
    static constexpr bool generational =
        is_generational_strategy<clean_type>::value;
    static constexpr bool is_static =
        is_static_strategy<clean_type>::value;
    static constexpr bool is_fixed =
        is_fixed_strategy<clean_type>::value;
    static constexpr bool is_dynamic =
        is_dynamic_strategy<clean_type>::value;
    static constexpr strategy_typing typing =
        strategy_typing_of<clean_type>::value;
    static constexpr storage_kind kind =
        memory_strategy_kind_of<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_STRATEGY_TRAITS_
