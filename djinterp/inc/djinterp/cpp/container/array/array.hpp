/*******************************************************************************
* djinterp [cpp][container]                                            array.hpp
*
* Concrete array container.
*   `DArray<typename... Options>` is the framework's options-pack form
* of a fixed-extent contiguous container.  It satisfies
* the options-container contract: inheriting from
* `DOptionsContainerBase<Options...>` surfaces the four contract
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
* each with a static extent.  `DArrayStorageKind::dynamic_extent` is
* reserved: no cell stores a dynamic extent yet.  See array_options.hpp
* for the option keys and canonical aliases used to select among them.
*
*   Every cell is contiguous, so is_contiguous_array holds for a
* non-iterable array too (section III); non-iterability withholds
* begin()/end(), not the storage.
*
*   PORTABILITY:
*   The header compiles from C++17 (the floor, for now).  Members
* marked with `D_INTERNAL_ARRAY_CONSTEXPR` are constexpr from C++14
* onward (relaxed constexpr) and inline otherwise.
*
*
* path:      /inc/djinterp/cpp/container/array/array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.25
*                                                            revised: 2026.11.14
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
#include "../../djinterp.hpp"
#include "../../../../re_std/cpp/span/dynamic_extent.hpp"  // dynamic_extent
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

// DArrayIterability and DArrayLifetime are defined in array_traits.hpp.
// Re-exported by inclusion above.

// DArrayStorageKind
//   enum: distinguishes a static-extent array (capacity baked into the type)
// from a dynamic-extent one (capacity tracked at run-time against a maximum).
enum class DArrayStorageKind
{
    static_extent,
    dynamic_extent
};


// dynamic_extent
//   constant: sentinel value for the extent option indicating dynamic-extent
// storage. It is re_std's (equal to std::dynamic_extent), the one table_shape
// and re_std::span use too, so that every extent in the framework compares
// against the same object.
using re_std::dynamic_extent;


// array_type_key, array_extent_key
//   constants: the array's own two option keys - the element type (the
// option's value position is the type, as for container_axis::lock_policy)
// and the extent (an integral_constant<size_t, N>). option_set takes one key
// type per set, and an array's pack mixes these with the universal axes
// (lifetime, iterability), so both are container_axis enumerators, named
// here as the module has always named them: `option<array_type_key, T>` is
// what array_opt_type<T> spells.
D_CONSTEXPR_INLINE_VAR DContainerAxis array_type_key =
    DContainerAxis::element_type;
D_CONSTEXPR_INLINE_VAR DContainerAxis array_extent_key =
    DContainerAxis::extent;


NS_INTERNAL


    // is_dynamic_extent
    template<std::size_t N>
    struct is_dynamic_extent
        : std::integral_constant<bool, (N == dynamic_extent)>
    {};


    // array_assert_size
    template<typename SizeType>
    inline D_INTERNAL_ARRAY_CONSTEXPR
    void array_assert_size(
        SizeType _index,
        SizeType _size
    )
    {
        (void)_index;
        (void)_size;
    }


    // array_is_self_arg
    //   trait: true iff the pack is exactly one argument whose decayed type is
    // Self - the copy or move that array<>'s element-list constructor must
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

    // array_arg_casts
    //   trait: true when `static_cast<Value>(std::declval<Arg>())` is
    // well-formed: what array<>'s element-list constructor does with each
    // argument.
    template<typename Value,
             typename Arg,
             typename = void>
    struct array_arg_casts : std::false_type
    {};

    // array_arg_casts<Value, Arg, void_t<the cast>>
    //   trait: the case where the cast is well-formed; it reports true.
    template<typename Value,
             typename Arg>
    struct array_arg_casts<Value, Arg, void_t<
        decltype(static_cast<Value>(std::declval<Arg>()))
    >> : std::true_type
    {};

    // array_args_cast
    //   trait: true when every argument of the pack casts to Value.
    template<typename    Value,
             typename... Args>
    struct array_args_cast : std::true_type
    {};

    // array_args_cast<Value, First, Rest...>
    //   trait: the non-empty case; it checks First, then the rest.
    template<typename    Value,
             typename    First,
             typename... Rest>
    struct array_args_cast<Value, First, Rest...>
        : std::integral_constant<bool,
              ( array_arg_casts<Value, First>::value &&
                array_args_cast<Value, Rest...>::value )>
    {};

    // array_args_nothrow
    //   trait: true when no argument's cast to Value can throw.
    template<typename    Value,
             typename... Args>
    struct array_args_nothrow : std::true_type
    {};

    // array_args_nothrow<Value, First, Rest...>
    //   trait: the non-empty case; it checks First, then the rest.
    template<typename    Value,
             typename    First,
             typename... Rest>
    struct array_args_nothrow<Value, First, Rest...>
        : std::integral_constant<bool,
              ( noexcept(static_cast<Value>(std::declval<First>())) &&
                array_args_nothrow<Value, Rest...>::value )>
    {};


// ===========================================================================
// II.  Universal-to-Array translation helpers
// ===========================================================================

    // to_array_lifetime
    //   helper: translates a universal `DContainerLifetime` to the
    // corresponding `DArrayLifetime`.
    inline D_CONSTEXPR
    DArrayLifetime to_array_lifetime(
        DContainerLifetime _l
    ) noexcept
    {
        return (_l == DContainerLifetime::constexpr_storage)
                   ? DArrayLifetime::constexpr_lifetime
                   : (_l == DContainerLifetime::immutable)
                       ? DArrayLifetime::immutable_lifetime
                       : DArrayLifetime::mutable_lifetime;
    }

    // to_array_iterability
    //   helper: translates a universal `DContainerIterability` to the
    // per-array `DArrayIterability` enum.
    inline D_CONSTEXPR
    DArrayIterability to_array_iterability(
        DContainerIterability _i
    ) noexcept
    {
        return (_i == DContainerIterability::iterable)
                   ? DArrayIterability::iterable
                   : DArrayIterability::non_iterable;
    }


    // DArrayAxesResolver
    //   helper: reads the four positional values that array<>'s storage and
    // SFINAE depend on out of the pack's option_set, each falling back to its
    // default when the pack leaves the axis out.
    template<typename    Set,
             std::size_t ExtentDefault>
    struct DArrayAxesResolver
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
        static constexpr DArrayLifetime lifetime =
            to_array_lifetime(
                container_axis_value_v<Set,
                                       DContainerAxis::lifetime,
                                       DContainerLifetime::mutable_storage>);

        // iterability (enum)
        //   value: read from the universal iterability axis, then translated
        // to the per-array `DArrayIterability` enum. Defaults to iterable.
        static constexpr DArrayIterability iterability =
            to_array_iterability(
                container_axis_value_v<Set,
                                       DContainerAxis::iterability,
                                       DContainerIterability::iterable>);

        // iterable (bool convenience, derived)
        //   value: true iff `iterability == DArrayIterability::iterable`.
        // Surfaced so SFINAE predicates that just want a yes/no answer don't
        // have to compare enum values.
        static constexpr bool iterable =
            (iterability == DArrayIterability::iterable);
    };


NS_END  // internal


// ===========================================================================
// III. array primary template (options-pack)
// ===========================================================================

// DArray
//   class template: fixed- or dynamic-extent contiguous container.
// The single template parameter is a pack of options consumed by
// `with_options_pack` (via `DOptionsContainerBase`) and by an internal
// resolver that reads the four positional axes.
//
// Required options:
//   - `array_type_key` (or `DArrayOptType<T>`): element type.
//
// Optional options:
//   - `array_extent_key` (or `DArrayOptExtent<N>`): extent;
//     default 0.
//   - `container_lifetime_key` (or `DContainerOptLifetime<L>`):
//     lifetime; default `mutable_storage`.
//   - `container_iterability_key`
//     (or `DContainerOptIterability<I>`): iterability;
//     default `iterable`.
template<typename... Options>
class DArray : public DOptionsContainerBase<Options...>
{
private:
    using contract_base = DOptionsContainerBase<Options...>;

    using resolver = internal::DArrayAxesResolver<
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
    static constexpr DArrayLifetime    lifetime    = resolver::lifetime;
    static constexpr DArrayIterability iterability = resolver::iterability;
    static constexpr bool              iterable    = resolver::iterable;

private:
    // Storage
    static constexpr size_type _storage_n = (extent == 0 ? 1 : extent);

    typename std::conditional<
        (lifetime == DArrayLifetime::immutable_lifetime),
        const value_type[_storage_n],
        value_type[_storage_n]
    >::type m_data;

public:
    // ---------------------------------------------------------------
    //  Construction
    // ---------------------------------------------------------------

    constexpr
    DArray() D_NOEXCEPT
        : m_data{}
    {}

    // element-list constructor: at most `extent` values, each cast to
    // value_type; the rest are value-initialized.  Constrained on exactly
    // that, so std::is_constructible tells the truth about it, and noexcept
    // when no cast can throw.
    template<typename... Args,
             typename = typename std::enable_if<
                 ( !internal::array_is_self_arg<DArray, Args...>::value  &&
                   (sizeof...(Args) <= extent)                          &&
                   internal::array_args_cast<value_type,
                                             Args...>::value )
             >::type>
    constexpr
    DArray(Args&&... _args)
        D_NOEXCEPT_IF((internal::array_args_nothrow<value_type,
                                                    Args...>::value))
        : m_data{static_cast<value_type>(
                     std::forward<Args>(_args))...}
    {}

    // ---------------------------------------------------------------
    //  Element access
    // ---------------------------------------------------------------

    template<bool Enabled = (lifetime != DArrayLifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    operator[](size_type _i) D_NOEXCEPT
    {
        return m_data[_i];
    }

    constexpr
    const_reference operator[](
        size_type _i
    ) const D_NOEXCEPT
    {
        return m_data[_i];
    }

    template<bool Enabled =
                 (lifetime != DArrayLifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    at(size_type _i) D_NOEXCEPT
    {
        return m_data[_i];
    }

    constexpr
    const_reference at(
        size_type _i
    ) const D_NOEXCEPT
    {
        return m_data[_i];
    }

    template<bool Enabled =
                 (lifetime != DArrayLifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    front() D_NOEXCEPT
    {
        return m_data[0];
    }

    constexpr
    const_reference front() const D_NOEXCEPT
    {
        return m_data[0];
    }

    template<bool Enabled =
                 (lifetime != DArrayLifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    reference
    back() D_NOEXCEPT
    {
        return m_data[extent - 1];
    }

    constexpr
    const_reference back() const D_NOEXCEPT
    {
        return m_data[extent - 1];
    }

    template<bool Enabled =
                 (lifetime != DArrayLifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    pointer
    data() D_NOEXCEPT
    {
        return &m_data[0];
    }

    constexpr
    const_pointer data() const D_NOEXCEPT
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
                   (lifetime != DArrayLifetime::immutable_lifetime) ),
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
                   (lifetime != DArrayLifetime::immutable_lifetime) ),
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
                   (lifetime != DArrayLifetime::immutable_lifetime) ),
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
                   (lifetime != DArrayLifetime::immutable_lifetime) ),
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
                 (lifetime != DArrayLifetime::immutable_lifetime),
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
                 (lifetime != DArrayLifetime::immutable_lifetime),
             typename = typename std::enable_if<Enabled>::type>
    D_INTERNAL_ARRAY_CONSTEXPR
    void
    swap(DArray& _other) D_NOEXCEPT
    {
        for (size_type i = 0; i < extent; ++i)
        {
            value_type tmp   = std::move(m_data[i]);
            m_data[i]        = std::move(_other.m_data[i]);
            _other.m_data[i] = std::move(tmp);
        }
    }
};

// old name, until 0.9.0-rc.1
template<typename... Options>
using array = DArray<Options...>;

// is_contiguous_array<array<Options...>>
//   trait: every cell of array<> stores its elements in one contiguous
// block reached through data(), iterable or not; it reports true.
template<typename... Options>
struct is_contiguous_array<DArray<Options...>>
    : std::true_type
{};


// ===========================================================================
// IV.  Convenience aliases
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    template<typename Type, std::size_t N>
    using DMutableIterableArray = DArray<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        DContainerOptLifetime<DContainerLifetime::mutable_storage>,
        DContainerOptIterability<DContainerIterability::iterable>>;

    template<typename Type, std::size_t N>
    using DMutableNonIterableArray = DArray<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        DContainerOptLifetime<DContainerLifetime::mutable_storage>,
        DContainerOptIterability<DContainerIterability::non_iterable>>;

    template<typename Type, std::size_t N>
    using DImmutableIterableArray = DArray<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        DContainerOptLifetime<DContainerLifetime::immutable>,
        DContainerOptIterability<DContainerIterability::iterable>>;

    template<typename Type, std::size_t N>
    using DImmutableNonIterableArray = DArray<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        DContainerOptLifetime<DContainerLifetime::immutable>,
        DContainerOptIterability<DContainerIterability::non_iterable>>;

    template<typename Type, std::size_t N>
    using DConstexprIterableArray = DArray<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        DContainerOptLifetime<DContainerLifetime::constexpr_storage>,
        DContainerOptIterability<DContainerIterability::iterable>>;

    template<typename Type, std::size_t N>
    using DConstexprNonIterableArray = DArray<
        option<array_type_key,   Type>,
        option<array_extent_key,
            std::integral_constant<std::size_t, N>>,
        DContainerOptLifetime<DContainerLifetime::constexpr_storage>,
        DContainerOptIterability<DContainerIterability::non_iterable>>;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


// ===========================================================================
// V.   Free-function factories
// ===========================================================================

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES \
        && D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES)

    template<typename Type, typename... Args>
    constexpr
    DMutableIterableArray<Type, sizeof...(Args) + 1>
    make_array(Type _first, Args&&... _rest)
    {
        return DMutableIterableArray<Type, sizeof...(Args) + 1>(
            _first,
            std::forward<Args>(_rest)...);
    }

    template<typename Type, typename... Args>
    constexpr
    DImmutableIterableArray<Type, sizeof...(Args) + 1>
    make_immutable_array(Type _first, Args&&... _rest)
    {
        return DImmutableIterableArray<Type, sizeof...(Args) + 1>(
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
bool array_equal(
    const DArray<OptsA...>& _a,
    const DArray<OptsB...>& _b
) D_NOEXCEPT
{
    static_assert(
        std::is_same<typename DArray<OptsA...>::value_type,
                     typename DArray<OptsB...>::value_type>::value,
        "array_equal: element types must match");
    static_assert(
        DArray<OptsA...>::extent == DArray<OptsB...>::extent,
        "array_equal: extents must match");

    return constexpr_equal(
        _a.data(),
        _a.data() + DArray<OptsA...>::extent,
        _b.data());
}


// array_copy
template<typename... OptsSrc,
         typename... OptsDst,
         typename = typename std::enable_if<
             (DArray<OptsDst...>::lifetime !=
              DArrayLifetime::immutable_lifetime)
         >::type>
D_INTERNAL_ARRAY_CONSTEXPR
void
array_copy(
    const DArray<OptsSrc...>& _src,
    DArray<OptsDst...>&       _dst)
{
    static_assert(
        std::is_same<typename DArray<OptsSrc...>::value_type,
                     typename DArray<OptsDst...>::value_type>::value,
        "array_copy: element types must match");
    static_assert(
        DArray<OptsSrc...>::extent == DArray<OptsDst...>::extent,
        "array_copy: extents must match");

    for (std::size_t i = 0; i < DArray<OptsSrc...>::extent; ++i)
    {
        _dst.data()[i] = _src.data()[i];
    }
}


// array_swap
template<typename... OptsA,
         typename... OptsB,
         typename = typename std::enable_if<
                (DArray<OptsA...>::lifetime !=
                 DArrayLifetime::immutable_lifetime)
             && (DArray<OptsB...>::lifetime !=
                 DArrayLifetime::immutable_lifetime)
         >::type>
D_INTERNAL_ARRAY_CONSTEXPR
void
array_swap(
    DArray<OptsA...>& _a,
    DArray<OptsB...>& _b)
{
    static_assert(
        std::is_same<typename DArray<OptsA...>::value_type,
                     typename DArray<OptsB...>::value_type>::value,
        "array_swap: element types must match");
    static_assert(
        DArray<OptsA...>::extent == DArray<OptsB...>::extent,
        "array_swap: extents must match");

    using value_type = typename DArray<OptsA...>::value_type;

    for (std::size_t i = 0; i < DArray<OptsA...>::extent; ++i)
    {
        value_type tmp = std::move(_a.data()[i]);
        _a.data()[i]   = std::move(_b.data()[i]);
        _b.data()[i]   = std::move(tmp);
    }
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_ARRAY_HPP
