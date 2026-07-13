/******************************************************************************
* djinterp [container]                                          byte_size.hpp
*
*   The byte footprint of a container, split as the model splits its storage
* (Storage axis): a STATIC part - the container object itself, whose inline cells
* and bookkeeping sit within its own footprint - and a DYNAMIC part - the cells
* an allocator sites out of line on the heap.  For nested element containers the
* dynamic part recurses, so a vector<vector<int>> counts the inner buffers too.
*
*   Grounding.  Per Storage, a container's cells are sited statically (inline,
* never acquired, no allocator) or dynamically (out of line, through an
* allocator named by a handle).  The static footprint is therefore exact -
* sizeof of the object - while the dynamic footprint is the size of the acquired
* region.  Its cell count is read from the container's own public surface: a
* contiguous buffer from capacity() (Boundedness: the current allocation, not the
* type-level kappa), a node-based store from size().  Siting itself is inferred
* from the same signals the Storage-axis traits use - an allocator_type or a
* reserve() marks dynamic storage; their absence, static (0 dynamic bytes).
*
*   HONESTY.  The static part is EXACT.  The dynamic part is an ESTIMATE:
*     - it counts capacity() * sizeof(cell) for a contiguous store and
*       size() * sizeof(cell) for a node store, and so EXCLUDES per-node
*       allocator bookkeeping (link pointers, colour bits, control blocks),
*       which is implementation-defined and not legible from the surface;
*     - it cannot see small-buffer optimisation (hybrid siting is not legible
*       from a public surface), so an SBO string reported as dynamic may in fact
*       hold its bytes inline, and vice versa;
*     - a nested text buffer (a c_str()-bearing element) is treated as a leaf
*       atom, as the frontier of content_equality.hpp treats it, so its own heap
*       payload is not descended into.
*   The estimate is deliberately simple and surface-derived; where an exact
* figure is wanted the allocator must be asked, which this module does not do.
*
*   PORTABILITY:
*   C++11 baseline.  static_byte_size is a compile-time trait; dynamic_byte_size
* and total_byte_size are runtime function templates (they read capacity()/size()
* and walk the container), not constexpr, matching content_equality.hpp.
*
*
* path:      /inc/djinterp/core/container/byte_size.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.02
******************************************************************************/

#ifndef DJINTERP_BYTE_SIZE_
#define DJINTERP_BYTE_SIZE_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"                          // clean_t, NS_*, feature macros
#include "../meta/trait_detect.hpp"                 // D_VOID_T, D_TYPE_TRAIT_*
#include "./traits/element_relation_traits.hpp"     // element_type_of_t (the cell type)
#include "./traits/iterable_container_traits.hpp"    // is_iterable_container (descent gate)
#include "./traits/iterator_category_traits.hpp"    // has_data_accessor (contiguous mark)


NS_DJINTERP


// ===========================================================================
// I.   Static footprint (exact)
// ===========================================================================

// static_byte_size
//   trait: the exact size in bytes of the container OBJECT - its inline cells
// and bookkeeping - as sizeof(clean_t<_Container>).  Independent of contents;
// a handle-holding dynamic container has a small, fixed static footprint.
template<typename _Container>
struct static_byte_size
    : std::integral_constant<std::size_t, sizeof(clean_t<_Container>)>
{};

// static_byte_size_v
//   value: the `_v` companion (a std::size_t, not a bool, so emitted by hand
// under the same gate the trait-detect macros use for their value companions).
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Container>
    inline constexpr std::size_t static_byte_size_v =
        static_byte_size<_Container>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Container>
    constexpr std::size_t static_byte_size_v =
        static_byte_size<_Container>::value;
#endif


// ===========================================================================
// II.  Storage-shape signals
// ===========================================================================
//   The surface probes that decide a container's dynamic footprint.  Kept as
// local `_helper` detectors so byte-sizing carries its own siting evidence: the
// dynamic signals mirror the Storage-axis has_dynamic_storage_signal (allocator
// or reserve), and the capacity()/size() probes mirror the Boundedness
// accessors, without taking a dependency that would drag the full trait
// umbrella into a footprint query.

NS_INTERNAL

    // byte_has_capacity_helper
    //   helper: detects a capacity() accessor on a const lvalue - the current
    // allocation of a contiguous store.
    template<typename _Type,
             typename = void>
    struct byte_has_capacity_helper : std::false_type
    {};

    template<typename _Type>
    struct byte_has_capacity_helper<_Type,
        D_VOID_T<decltype(std::declval<const clean_t<_Type>&>().capacity())>>
        : std::true_type
    {};

    // byte_has_size_helper
    //   helper: detects a size() accessor on a const lvalue - the element count
    // of a node store (absent on forward_list, which is counted by distance).
    template<typename _Type,
             typename = void>
    struct byte_has_size_helper : std::false_type
    {};

    template<typename _Type>
    struct byte_has_size_helper<_Type,
        D_VOID_T<decltype(std::declval<const clean_t<_Type>&>().size())>>
        : std::true_type
    {};

    // byte_has_allocator_helper
    //   helper: detects an allocator_type alias - decisive evidence of dynamic
    // (out-of-line) storage, since static storage takes no allocator.
    template<typename _Type,
             typename = void>
    struct byte_has_allocator_helper : std::false_type
    {};

    template<typename _Type>
    struct byte_has_allocator_helper<_Type,
        D_VOID_T<typename clean_t<_Type>::allocator_type>>
        : std::true_type
    {};

    // byte_has_reserve_helper
    //   helper: detects a reserve(size_type) call - a growable, out-of-line
    // store, so also decisive evidence of dynamic storage.
    template<typename _Type,
             typename = void>
    struct byte_has_reserve_helper : std::false_type
    {};

    template<typename _Type>
    struct byte_has_reserve_helper<_Type,
        D_VOID_T<decltype(std::declval<clean_t<_Type>&>().reserve(
            std::declval<std::size_t>()))>>
        : std::true_type
    {};

    // byte_has_c_str_helper
    //   helper: detects a c_str() accessor - a text buffer, treated as a leaf
    // atom by the descent rather than a sequence to recurse into.
    template<typename _Type,
             typename = void>
    struct byte_has_c_str_helper : std::false_type
    {};

    template<typename _Type>
    struct byte_has_c_str_helper<_Type,
        D_VOID_T<decltype(std::declval<const clean_t<_Type>&>().c_str())>>
        : std::true_type
    {};

    // byte_is_dynamically_sited_helper
    //   helper: whether the container's cells are sited dynamically - an
    // allocator_type or a reserve() is decisive; absent both, siting is static
    // and the dynamic footprint is zero.
    template<typename _Type>
    struct byte_is_dynamically_sited_helper
        : std::integral_constant<bool,
              (    byte_has_allocator_helper<clean_t<_Type>>::value
                || byte_has_reserve_helper<clean_t<_Type>>::value )>
    {};

    // byte_recurse_into_element_helper
    //   helper: whether the descent enters an element - true for a nested
    // container, false for a leaf or a text buffer.  Mirrors the descent
    // predicate of content_equality.hpp so both flatten nesting identically.
    template<typename _Elem>
    struct byte_recurse_into_element_helper
        : std::integral_constant<bool,
              (    is_iterable_container<clean_t<_Elem>>::value
                && !byte_has_c_str_helper<clean_t<_Elem>>::value )>
    {};

NS_END  // internal


// ===========================================================================
// III. Own dynamic footprint (this level's acquired region)
// ===========================================================================
//   The heap bytes of ONE container level, before descending into elements.
// Dispatched on a compile-time shape so the hazardous sizeof(cell) is never
// named for a static-sited type (whose cell type may be void): the tag selects
// exactly one overload, and only that overload is instantiated.

NS_INTERNAL

    // byte_shape
    //   enum: the footprint shape of a container level - a contiguous dynamic
    // buffer sized by capacity(), a node dynamic store sized by the element
    // count, or a static-sited object with no heap region.
    enum class byte_shape
    {
        contiguous_dynamic,
        node_dynamic,
        static_sited
    };

    // byte_shape_of
    //   function: the shape of _Container.  A dynamically-sited store with a
    // data() accessor and a capacity() is contiguous; any other dynamically-
    // sited store is treated as node-based; everything else is static-sited.
    template<typename _Container>
    constexpr byte_shape byte_shape_of()
    {
        return
            ( byte_is_dynamically_sited_helper<_Container>::value
           && has_data_accessor<clean_t<_Container>>::value
           && byte_has_capacity_helper<_Container>::value )
                ? byte_shape::contiguous_dynamic
          : byte_is_dynamically_sited_helper<_Container>::value
                ? byte_shape::node_dynamic
                : byte_shape::static_sited;
    }

    // byte_shape_tag
    //   type: a compile-time carrier of a byte_shape, for overload dispatch.
    template<byte_shape _Shape>
    using byte_shape_tag = std::integral_constant<byte_shape, _Shape>;

    // byte_node_count
    //   function: the element count of a node store - size() when present, else
    // the iterator distance (the forward_list case, which has no size()).
    template<typename _Container>
    std::size_t byte_node_count(const _Container& _container, std::true_type)
    {
        return static_cast<std::size_t>(_container.size());
    }

    template<typename _Container>
    std::size_t byte_node_count(const _Container& _container, std::false_type)
    {
        return static_cast<std::size_t>(
            std::distance(std::begin(_container), std::end(_container)));
    }

    // byte_own_dynamic - contiguous: capacity() cells of the element type.
    template<typename _Container>
    std::size_t
    byte_own_dynamic(const _Container& _container,
                     byte_shape_tag<byte_shape::contiguous_dynamic>)
    {
        return static_cast<std::size_t>(_container.capacity())
             * sizeof(element_type_of_t<_Container>);
    }

    // byte_own_dynamic - node: element-count cells of the element type (the
    // per-node bookkeeping is excluded, see the file header).
    template<typename _Container>
    std::size_t
    byte_own_dynamic(const _Container& _container,
                     byte_shape_tag<byte_shape::node_dynamic>)
    {
        return byte_node_count(_container,
                   byte_has_size_helper<_Container>{})
             * sizeof(element_type_of_t<_Container>);
    }

    // byte_own_dynamic - static-sited: cells are inline, so no heap region.
    template<typename _Container>
    std::size_t
    byte_own_dynamic(const _Container&,
                     byte_shape_tag<byte_shape::static_sited>)
    {
        return 0;
    }

NS_END  // internal


// ===========================================================================
// IV.  Recursive dynamic descent
// ===========================================================================

NS_INTERNAL

    // byte_dynamic_helper
    //   helper: the dynamic footprint of a container - this level's own region,
    // plus (when elements are themselves containers) the dynamic footprint of
    // every element.  The primary stops at the own region; the recursing
    // specialization sums the elements.
    template<typename _Container,
             bool _Recurse =
                 byte_recurse_into_element_helper<
                     element_type_of_t<_Container>>::value>
    struct byte_dynamic_helper
    {
        static std::size_t compute(const _Container& _container)
        {
            return byte_own_dynamic(_container,
                byte_shape_tag<byte_shape_of<_Container>()>{});
        }
    };

    template<typename _Container>
    struct byte_dynamic_helper<_Container, true>
    {
        static std::size_t compute(const _Container& _container)
        {
            std::size_t _total = byte_own_dynamic(_container,
                byte_shape_tag<byte_shape_of<_Container>()>{});

            for (const auto& _element : _container)
            {
                _total += byte_dynamic_helper<
                    clean_t<element_type_of_t<_Container>>>::compute(_element);
            }

            return _total;
        }
    };

NS_END  // internal


// ===========================================================================
// V.   Public footprint functions
// ===========================================================================

// dynamic_byte_size
//   function: the heap bytes a container holds out of line - zero for a
// static-sited container, capacity()*sizeof(cell) for a contiguous store,
// size()*sizeof(cell) for a node store, summed recursively over nested element
// containers.  An estimate (see the file header); the static part is exact.
template<typename _Container>
std::size_t
dynamic_byte_size(const _Container& _container)
{
    return internal::byte_dynamic_helper<clean_t<_Container>>
        ::compute(_container);
}

// total_byte_size
//   function: the whole footprint of a container - its exact static object
// size plus its (estimated, recursive) dynamic bytes.
template<typename _Container>
std::size_t
total_byte_size(const _Container& _container)
{
    return static_byte_size<_Container>::value
         + dynamic_byte_size(_container);
}


NS_END  // djinterp


#endif  // DJINTERP_BYTE_SIZE_
