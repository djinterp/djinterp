/*******************************************************************************
* djinterp [core]                                                      array.hpp
*
* Concrete array container.
*   `array<typename... Options>` is the framework's options-pack form
* of a fixed- or dynamic-extent contiguous container.  It satisfies
* the options-container contract: inheriting from
* `options_container_base<Options...>` surfaces the four contract
* members (`options_type`, `option_count`, `has_option_v<>`,
* `option_t<>`) and the four positional axes that array<>'s storage
* depends on (element type, extent, lifetime, iterability) are
* resolved internally from the same option_set.
*
*   Six configuration cells from the lifetime / iterability cube
* are reachable through the option pack:
*
*               | iterable               | non-iterable
*   ------------+------------------------+--------------------------
*   constexpr   | constexpr+iterable     | constexpr+non_iterable
*   ------------+------------------------+--------------------------
*   immutable   | immutable+iterable     | immutable+non_iterable
*   ------------+------------------------+--------------------------
*   mutable     | mutable+iterable       | mutable+non_iterable
*
* combined with the {static-extent vs dynamic-extent} axis to give
* eight named cells.  See array_options.hpp for the option keys
* and canonical aliases used to select among them.
*
*   PORTABILITY:
*   C++11 baseline.  Members marked with `D_INTERNAL_ARRAY_CONSTEXPR`
* are constexpr from C++14 onward (relaxed constexpr) and inline
* otherwise.
*
*
* path:      /inc/djinterp/core/container/array/array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.25
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Policy enums and tag dispatch
      -----------------------------

II.   Universal-to-Array translation helpers
      --------------------------------------

III.  array primary template (options-pack)
      -------------------------------------

IV.   Convenience aliases
      -------------------

V.    Free-function factories
      -----------------------

VI.   Free-function bulk algorithms
      -----------------------------
*/

#ifndef DJINTERP_CONTAINER_ARRAY_ARRAY_HPP
#define DJINTERP_CONTAINER_ARRAY_ARRAY_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../option/option.hpp"
#include "../container_options.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../iterator/constexpr_iterator.hpp"
#include "./array_traits.hpp"
#include "./array_iterator.hpp"


// D_INTERNAL_ARRAY_CONSTEXPR
//   macro: relaxed-constexpr (C++14+) for mutator member functions.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_INTERNAL_ARRAY_CONSTEXPR constexpr
#else
    #define D_INTERNAL_ARRAY_CONSTEXPR
#endif


NS_DJINTERP


// ===========================================================================
// I.   Policy enums and tag dispatch
// ===========================================================================

// array_iterability and array_lifetime are defined in array_traits.hpp.
// Re-exported by inclusion above.

// array_storage_kind
//   enum: distinguishes a static-extent array (capacity baked into the type)
// from a dynamic-extent one (capacity tracked at run-time against a maximum).
enum class array_storage_kind
{
    static_extent,
    dynamic_extent
};


// dynamic_extent
//   constant: sentinel value for the extent option indicating dynamic-extent
// storage. Mirrors std::dynamic_extent.
static constexpr std::size_t dynamic_extent =
    static_cast<std::size_t>(-1);


// array_type_key, array_extent_key
//   constants: the array's own two option keys -- the element type (the
// option's value position is the type, as for container_axis::lock_policy)
// and the extent (an integral_constant<size_t, N>). option_set takes one key
// type per set, and an array's pack mixes these with the universal axes
// (lifetime, iterability), so both are container_axis enumerators, named
// here as the module has always named them: `option<array_type_key, T>` is
// what array_opt_type<T> spells.
D_CONSTEXPR_INLINE_VAR container_axis array_type_key =
    container_axis::element_type;
D_CONSTEXPR_INLINE_VAR container_axis array_extent_key =
    container_axis::extent;


NS_INTERNAL


    // is_dynamic_extent
    template<std::size_t N>
    struct is_dynamic_extent
        : std::integral_constant<bool, (N == dynamic_extent)>
    {};


    // array_assert_size
    template<typename SizeType>
    inline D_INTERNAL_ARRAY_CONSTEXPR
    void array_assert_size(SizeType _index, SizeType _size)
    {
        (void)_index;
        (void)_size;
    }


    // array_is_self_arg
    //   trait: true iff the pack is exactly one argument whose decayed type is
    // Self -- the copy or move that array<>'s element-list constructor must
    // leave to the implicit constructors. Chosen by the pack's shape, so an
    // empty pack never asks for its first element (std::tuple_element<0,
    // std::tuple<>> is a hard error, not a substitution failure).
    template<typename    Self,
             typename... Args>
    struct array_is_self_arg : std::false_type
    {};

    template<typename Self,
             typename Arg>
    struct array_is_self_arg<Self, Arg>
        : std::is_same<typename std::decay<Arg>::type, Self>
    {};


// ===========================================================================
// II.  Universal-to-Array translation helpers
// ===========================================================================

    // to_array_lifetime
    //   helper: translates a universal `container_lifetime` to the
    // corresponding `array_lifetime`.
    inline D_CONSTEXPR array_lifetime
    to_array_lifetime(
        container_lifetime _l
    ) noexcept
    {
        return (_l == container_lifetime::constexpr_storage)
                   ? array_lifetime::constexpr_lifetime
                   : (_l == container_lifetime::immutable)
                       ? array_lifetime::immutable_lifetime
                       : array_lifetime::mutable_lifetime;
    }

    // to_array_iterability
    //   helper: translates a universal `container_iterability` to the
    // per-array `array_iterability` enum.
    inline D_CONSTEXPR array_iterability
    to_array_iterability(
        container_iterability _i
    ) noexcept
    {
        return (_i == container_iterability::iterable)
                   ? array_iterability::iterable
                   : array_iterability::non_iterable;
    }


    // array_axes_resolver
    //   helper: reads the four positional values that array<>'s storage and
    // SFINAE depend on out of the pack's option_set, each falling back to its
    // default when the pack leaves the axis out.
    template<typename    Set,
             std::size_t ExtentDefault>
    struct array_axes_resolver
    {
        // element_type
        //   type: the type under array_type_key; void when absent, which
        // array<> rejects with a static_assert.
        using element_type = container_axis_type_t<
            Set,
            array_type_key,
            void>;

        // extent
        static constexpr std::size_t extent =
            container_axis_type_t<
                Set,
                array_extent_key,
                std::integral_constant<std::size_t,
                                       ExtentDefault>>::value;

        // lifetime
        static constexpr array_lifetime lifetime =
            to_array_lifetime(
                container_axis_value_v<Set,
                                       container_axis::lifetime,
                                       container_lifetime::mutable_storage>);

        // iterability (enum)
        //   value: read from the universal iterability axis, then translated
        // to the per-array `array_iterability` enum. Defaults to iterable.
        static constexpr array_iterability iterability =
            to_array_iterability(
                container_axis_value_v<Set,
                                       container_axis::iterability,
                                       container_iterability::iterable>);

        // iterable (bool convenience, derived)
        //   value: true iff `iterability == array_iterability::iterable`.
        // Surfaced so SFINAE predicates that just want a yes/no answer don't
        // have to compare enum values.
        static constexpr bool iterable =
            (iterability == array_iterability::iterable);
    };


NS_END  // internal


// ===========================================================================
// III. array primary template (options-pack)
// ===========================================================================

// array
//   class template: fixed- or dynamic-extent contiguous container.
// The single template parameter is a pack of options consumed by
// `with_options_pack` (via `options_container_base`) and by an internal
// resolver that reads the four positional axes.
//
// Required options:
//   - `array_type_key` (or `array_opt_type<T>`): element type.
//
// Optional options:
//   - `array_extent_key` (or `array_opt_extent<N>`): extent;
//     default 0.
//   - `container_lifetime_key` (or `container_opt_lifetime<L>`):
//     lifetime; default `mutable_storage`.
//   - `container_iterability_key`
//     (or `container_opt_iterability<I>`): iterability;
//     default `iterable`.
template<typename... Options>
class array : public options_container_base<Options...>
{
private:
    using contract_base = options_container_base<Options...>;

    using resolver = internal::array_axes_resolver<
        typename contract_base::options_type,
        0>;

    static_assert(
        !std::is_same<typename resolver::element_type, void>::value,
        "djinterp::array: an `array_type_key` option (or "
        "`array_opt_type<T>`) must be present in the options pack "
        "to identify the element type.");

public:
    // ---------------------------------------------------------------
    //  Type aliases (resolved positional axes)
    // ---------------------------------------------------------------
    using value_type             = typename resolver::element_type;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = value_type*;
    using const_iterator         = const value_type*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_type         extent      = resolver::extent;
    static constexpr array_lifetime    lifetime    = resolver::lifetime;
    static constexpr array_iterability iterability = resolver::iterability;
    static constexpr bool              iterable    = resolver::iterable;

private:
    // Storage
    static constexpr size_type _storage_n = (extent == 0 ? 1 : extent);

    typename std::conditional<
        (lifetime == array_lifetime::immutable_lifetime),
        const value_type[_storage_n],
        value_type[_storage_n]
    >::type m_data;

public:
    // ---------------------------------------------------------------
    //  Construction
    // ---------------------------------------------------------------

    constexpr
    array() D_NOEXCEPT
        : m_data{}
    {}

    template<typename... Args,
             typename = typename std::enable_if<
                 !internal::array_is_self_arg<array, Args...>::value
             >::type>
    constexpr
    array(Args&&... _args) D_NOEXCEPT
        : m_data{static_cast<value_type>(
                     std::forward<Args>(_args))...}
    {}

    // ---------------------------------------------------------------
    //  Element access
    // ---------------------------------------------------------------

    template<bool Enabled = (lifetime != array_lifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    operator[](size_type _i) D_NOEXCEPT
    {
        return m_data[_i];
    }

    constexpr
    const_reference
    operator[](size_type _i) const D_NOEXCEPT
    {
        return m_data[_i];
    }

    template<bool Enabled =
                 (lifetime != array_lifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    at(size_type _i) D_NOEXCEPT
    {
        return m_data[_i];
    }

    constexpr
    const_reference
    at(size_type _i) const D_NOEXCEPT
    {
        return m_data[_i];
    }

    template<bool Enabled =
                 (lifetime != array_lifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    front() D_NOEXCEPT
    {
        return m_data[0];
    }

    constexpr
    const_reference
    front() const D_NOEXCEPT
    {
        return m_data[0];
    }

    template<bool Enabled =
                 (lifetime != array_lifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    back() D_NOEXCEPT
    {
        return m_data[extent - 1];
    }

    constexpr
    const_reference
    back() const D_NOEXCEPT
    {
        return m_data[extent - 1];
    }

    template<bool Enabled =
                 (lifetime != array_lifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    pointer
    data() D_NOEXCEPT
    {
        return &m_data[0];
    }

    constexpr
    const_pointer
    data() const D_NOEXCEPT
    {
        return &m_data[0];
    }

    // ---------------------------------------------------------------
    //  Capacity
    // ---------------------------------------------------------------

    constexpr size_type size()     const D_NOEXCEPT { return extent;       }
    constexpr size_type max_size() const D_NOEXCEPT { return extent;       }
    constexpr size_type capacity() const D_NOEXCEPT { return extent;       }
    constexpr bool      empty()    const D_NOEXCEPT { return extent == 0;  }

    // ---------------------------------------------------------------
    //  Iteration  (only present when iterable)
    // ---------------------------------------------------------------

    template<bool Enabled =
                 ( iterable &&
                   (lifetime != array_lifetime::immutable_lifetime) ),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    iterator
    begin() D_NOEXCEPT
    {
        return &m_data[0];
    }

    template<bool Enabled = iterable,
             typename = typename std::enable_if<Enabled>::type>
    constexpr
    const_iterator
    begin() const D_NOEXCEPT
    {
        return &m_data[0];
    }

    template<bool Enabled = iterable,
             typename = typename std::enable_if<Enabled>::type>
    constexpr
    const_iterator
    cbegin() const D_NOEXCEPT
    {
        return &m_data[0];
    }

    template<bool Enabled =
                 ( iterable &&
                   (lifetime != array_lifetime::immutable_lifetime) ),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    iterator
    end() D_NOEXCEPT
    {
        return &m_data[0] + extent;
    }

    template<bool Enabled = iterable,
             typename = typename std::enable_if<Enabled>::type>
    constexpr
    const_iterator
    end() const D_NOEXCEPT
    {
        return &m_data[0] + extent;
    }

    template<bool Enabled = iterable,
             typename = typename std::enable_if<Enabled>::type>
    constexpr
    const_iterator
    cend() const D_NOEXCEPT
    {
        return &m_data[0] + extent;
    }

    template<bool Enabled =
                 ( iterable &&
                   (lifetime != array_lifetime::immutable_lifetime) ),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reverse_iterator
    rbegin() D_NOEXCEPT
    {
        return reverse_iterator(end());
    }

    template<bool Enabled = iterable,
             typename = typename std::enable_if<Enabled>::type>
    constexpr
    const_reverse_iterator
    rbegin() const D_NOEXCEPT
    {
        return const_reverse_iterator(end());
    }

    template<bool Enabled =
                 ( iterable &&
                   (lifetime != array_lifetime::immutable_lifetime) ),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reverse_iterator
    rend() D_NOEXCEPT
    {
        return reverse_iterator(begin());
    }

    template<bool Enabled = iterable,
             typename = typename std::enable_if<Enabled>::type>
    constexpr
    const_reverse_iterator
    rend() const D_NOEXCEPT
    {
        return const_reverse_iterator(begin());
    }

    // ---------------------------------------------------------------
    //  Mutation  (SFINAE-disabled when immutable)
    // ---------------------------------------------------------------

    template<bool Enabled =
                 (lifetime != array_lifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    void
    fill(const value_type& _v)
    {
        for (size_type i = 0; i < extent; ++i)
        {
            m_data[i] = _v;
        }
    }

    template<bool Enabled =
                 (lifetime != array_lifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    void
    swap(array& _other) D_NOEXCEPT
    {
        for (size_type i = 0; i < extent; ++i)
        {
            value_type tmp   = std::move(m_data[i]);
            m_data[i]        = std::move(_other.m_data[i]);
            _other.m_data[i] = std::move(tmp);
        }
    }
};


// ===========================================================================
// IV.  Convenience aliases
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    template<typename Type, std::size_t N>
    using mutable_iterable_array = array<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        container_opt_lifetime<container_lifetime::mutable_storage>,
        container_opt_iterability<container_iterability::iterable>>;

    template<typename Type, std::size_t N>
    using mutable_non_iterable_array = array<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        container_opt_lifetime<container_lifetime::mutable_storage>,
        container_opt_iterability<container_iterability::non_iterable>>;

    template<typename Type, std::size_t N>
    using immutable_iterable_array = array<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        container_opt_lifetime<container_lifetime::immutable>,
        container_opt_iterability<container_iterability::iterable>>;

    template<typename Type, std::size_t N>
    using immutable_non_iterable_array = array<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        container_opt_lifetime<container_lifetime::immutable>,
        container_opt_iterability<container_iterability::non_iterable>>;

    template<typename Type, std::size_t N>
    using constexpr_iterable_array = array<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        container_opt_lifetime<container_lifetime::constexpr_storage>,
        container_opt_iterability<container_iterability::iterable>>;

    template<typename Type, std::size_t N>
    using constexpr_non_iterable_array = array<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        container_opt_lifetime<container_lifetime::constexpr_storage>,
        container_opt_iterability<container_iterability::non_iterable>>;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


// ===========================================================================
// V.   Free-function factories
// ===========================================================================

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES \
        && D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES)

    template<typename Type, typename... Args>
    constexpr
    mutable_iterable_array<Type, sizeof...(Args) + 1>
    make_array(Type _first, Args&&... _rest)
    {
        return mutable_iterable_array<Type, sizeof...(Args) + 1>(
            _first,
            std::forward<Args>(_rest)...);
    }

    template<typename Type, typename... Args>
    constexpr
    immutable_iterable_array<Type, sizeof...(Args) + 1>
    make_immutable_array(Type _first, Args&&... _rest)
    {
        return immutable_iterable_array<Type, sizeof...(Args) + 1>(
            _first,
            std::forward<Args>(_rest)...);
    }

#endif


// ===========================================================================
// VI.  Free-function bulk algorithms
// ===========================================================================

// array_equal
template<typename... OptsA,
         typename... OptsB>
constexpr
bool
array_equal(
    const array<OptsA...>& _a,
    const array<OptsB...>& _b
) D_NOEXCEPT
{
    static_assert(
        std::is_same<typename array<OptsA...>::value_type,
                     typename array<OptsB...>::value_type>::value,
        "array_equal: element types must match");
    static_assert(
        array<OptsA...>::extent == array<OptsB...>::extent,
        "array_equal: extents must match");

    return constexpr_equal(
        _a.data(),
        _a.data() + array<OptsA...>::extent,
        _b.data());
}


// array_copy
template<typename... OptsSrc,
         typename... OptsDst,
         typename = typename std::enable_if<
             (array<OptsDst...>::lifetime !=
              array_lifetime::immutable_lifetime)
         >::type>
D_INTERNAL_ARRAY_CONSTEXPR
void
array_copy(
    const array<OptsSrc...>& _src,
    array<OptsDst...>&       _dst)
{
    static_assert(
        std::is_same<typename array<OptsSrc...>::value_type,
                     typename array<OptsDst...>::value_type>::value,
        "array_copy: element types must match");
    static_assert(
        array<OptsSrc...>::extent == array<OptsDst...>::extent,
        "array_copy: extents must match");

    for (std::size_t i = 0; i < array<OptsSrc...>::extent; ++i)
    {
        _dst.data()[i] = _src.data()[i];
    }
}


// array_swap
template<typename... OptsA,
         typename... OptsB,
         typename = typename std::enable_if<
                (array<OptsA...>::lifetime !=
                 array_lifetime::immutable_lifetime)
             && (array<OptsB...>::lifetime !=
                 array_lifetime::immutable_lifetime)
         >::type>
D_INTERNAL_ARRAY_CONSTEXPR
void
array_swap(
    array<OptsA...>& _a,
    array<OptsB...>& _b)
{
    static_assert(
        std::is_same<typename array<OptsA...>::value_type,
                     typename array<OptsB...>::value_type>::value,
        "array_swap: element types must match");
    static_assert(
        array<OptsA...>::extent == array<OptsB...>::extent,
        "array_swap: extents must match");

    using value_type = typename array<OptsA...>::value_type;

    for (std::size_t i = 0; i < array<OptsA...>::extent; ++i)
    {
        value_type tmp = std::move(_a.data()[i]);
        _a.data()[i]   = std::move(_b.data()[i]);
        _b.data()[i]   = std::move(tmp);
    }
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_ARRAY_HPP
