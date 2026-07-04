/******************************************************************************
* djinterp [container]                          hierarchical_container_traits.hpp
*
*   SFINAE structural traits for the HIERARCHICAL side of the structure axis -
* whether a container NESTS.  A container type factors as T = tau + F[T]: a leaf
* summand (an element of the base type tau) and an optional NODE summand F[T] (a
* sub-container), present exactly where nesting occurs.  The depth d counts
* nodes; a container is HIERARCHICAL when d >= 2.
*
*   DETECTION - STRUCTURAL FIRST, TAG IN CONJUNCTION:
*   The verdict is reached by combining signals of differing strength, with the
* structural ones deciding wherever they are conclusive:
*
*     1. node_type (container-shaped) the STRONG structural tell - a type advertising
*                                   the F[T] summand (a sub-container) IS nesting.
*                                   Authoritative; the container-shape requirement
*                                   excludes the std node-handle node_type (C++17+).
*     2. container-shaped value_type  the WEAK structural heuristic - container_depth
*                                   walks the value_type chain (vector<vector<T>> ->
*                                   depth 2).  It reads a container-shaped element
*                                   (a string) as a nested level, so it can over-
*                                   report; an opt-in flat tag may suppress it.
*     3. structure_category tag     the OPT-IN supplement (flat / hierarchical) -
*                                   the hierarchical tag asserts nesting the chain
*                                   cannot expose; the flat tag suppresses signal 2.
*
*   The combination: a node_type or a hierarchical tag yields hierarchy outright
* (an O(1) check that also SHORT-CIRCUITS the recursive depth walk); otherwise,
* absent a flat tag, the depth heuristic decides.  A flat tag never overrides the
* strong node_type signal - one cannot un-nest a declared node summand.
*
*   This header owns the structure axis's shared machinery - the container-shape
* guard, container_depth, the structural/tag detection, and the structure_kind
* summary - which flat_container_traits.hpp consumes to define flatness as the
* complement.  The axis is orthogonal to the other intrinsic axes.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/structure/hierarchical_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
*                                                          revised: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_HIERARCHICAL_CONTAINER_TRAITS_
#define DJINTERP_HIERARCHICAL_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"            // clean_t, NS_*, D_VOID_T-via-trait_detect, feature macros
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_* detection macros, D_VOID_T
#include "../../meta/hierarchical.hpp"            // hierarchical tag (structure_category opt-in)
#include "../../meta/flat.hpp"                    // flat tag (structure_category opt-in)


NS_DJINTERP


// ===========================================================================
// I.   Structural detection signals
// ===========================================================================

// is_container_shape
//   trait: true iff _Type quacks like a container - a nested value_type AND a
// const-callable size().  The recursion guard, and the structure axis's shared
// "is this a container at all" primitive (consumed by flat_container_traits.hpp).
D_TYPE_TRAIT_TRUE(is_container_shape,
    typename clean_t<_Type>::value_type,
    decltype(std::declval<const clean_t<_Type>&>().size()))

// has_node_summand
//   trait: detects the F[T] node summand - a nested `node_type` alias that is
// ITSELF container-shaped - the STRONG (authoritative) structural tell for
// hierarchy.  The container-shape requirement is what makes this the formal F[T]
// ("a container of components"), and incidentally keeps it from firing on the
// standard library's node-handle `node_type` (added to the associative /
// unordered containers in C++17), which carries a value_type but no size() and
// so is not a container.
template<typename _Type,
         typename = void>
struct has_node_summand : std::false_type
{};

template<typename _Type>
struct has_node_summand<_Type,
    D_VOID_T<typename clean_t<_Type>::node_type>>
    : is_container_shape<typename clean_t<_Type>::node_type>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_node_summand)

// has_structure_category
//   trait: detects an opt-in `structure_category` alias (a flat / hierarchical
// tag).  Exposed for thoroughness; the verdict reads the tag's `nests` bit below.
D_TYPE_TRAIT_HAS_TYPE(has_structure_category, structure_category)


// ===========================================================================
// II.  Recursive depth (the weak structural heuristic)
// ===========================================================================

NS_INTERNAL

    // container_depth_helper
    //   helper: counts nested-container value_types, bounded by _Remaining -
    // 0 at a non-container leaf, 1 + recurse(value_type) at a container node.
    template<typename _Type,
             std::size_t _Remaining,
             typename = void>
    struct container_depth_helper
        : std::integral_constant<std::size_t, 0>
    {};

    template<typename _Type,
             std::size_t _Remaining>
    struct container_depth_helper<_Type, _Remaining,
        typename std::enable_if<
                (_Remaining > 0)
             && is_container_shape<_Type>::value
        >::type>
        : std::integral_constant<std::size_t,
              1 + container_depth_helper<
                  typename clean_t<_Type>::value_type,
                  _Remaining - 1
              >::value>
    {};

NS_END  // internal

// container_depth
//   trait: the value_type-chain nesting depth - 1 for a flat container,
// 2 for vector<vector<T>>, 0 for a non-container.  _MaxDepth caps the recursion
// (default 32) against pathologically self-referential types.  This measures the
// REGULAR (uniform) nesting visible in the type; composite hierarchy advertised
// only through node_type is caught by is_hierarchical_container, not counted here.
template<typename _Type,
         std::size_t _MaxDepth = 32>
struct container_depth
    : std::integral_constant<std::size_t,
          internal::container_depth_helper<clean_t<_Type>, _MaxDepth>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type,
             std::size_t _MaxDepth = 32>
    constexpr std::size_t container_depth_v =
        container_depth<_Type, _MaxDepth>::value;
#endif

// max_depth_of
//   trait: alias of container_depth, for parity with the other "_of" names.
template<typename _Type,
         std::size_t _MaxDepth = 32>
struct max_depth_of
    : container_depth<_Type, _MaxDepth>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type,
             std::size_t _MaxDepth = 32>
    constexpr std::size_t max_depth_of_v =
        max_depth_of<_Type, _MaxDepth>::value;
#endif


// ===========================================================================
// III. Tag reading + verdict
// ===========================================================================

NS_INTERNAL

    // structure_tag_hierarchical_helper / structure_tag_flat_helper
    //   helper: read the opt-in structure_category tag's `nests` bit - true only
    // when the tag is present and carries the corresponding kind.
    template<typename _Type,
             typename = void>
    struct structure_tag_hierarchical_helper : std::false_type
    {};

    template<typename _Type>
    struct structure_tag_hierarchical_helper<_Type,
        D_VOID_T<decltype(clean_t<_Type>::structure_category::nests)>>
        : std::integral_constant<bool, clean_t<_Type>::structure_category::nests>
    {};

    template<typename _Type,
             typename = void>
    struct structure_tag_flat_helper : std::false_type
    {};

    template<typename _Type>
    struct structure_tag_flat_helper<_Type,
        D_VOID_T<decltype(clean_t<_Type>::structure_category::nests)>>
        : std::integral_constant<bool, !clean_t<_Type>::structure_category::nests>
    {};

    // hierarchical_resolve_helper
    //   helper: combines the signals with their precedence, and - by carrying the
    // O(1) outcomes as template arguments - instantiates the recursive depth walk
    // ONLY in the case that actually needs it (no strong signal, no flat tag).
    //   _Fast    = node_type OR hierarchical tag  -> hierarchical outright.
    //   _TagFlat = flat tag                        -> suppress the depth heuristic.
    template<typename _Type,
             bool _Fast =
                 ( has_node_summand<_Type>::value ||
                   structure_tag_hierarchical_helper<_Type>::value ),
             bool _TagFlat = structure_tag_flat_helper<_Type>::value>
    struct hierarchical_resolve_helper
        : std::integral_constant<bool, (container_depth<_Type>::value >= 2)>
    {};

    template<typename _Type, bool _TagFlat>
    struct hierarchical_resolve_helper<_Type, true, _TagFlat>
        : std::true_type
    {};

    template<typename _Type>
    struct hierarchical_resolve_helper<_Type, false, true>
        : std::false_type
    {};

NS_END  // internal

// is_hierarchical_container
//   trait: true iff the container nests - the strong node_type / hierarchical-tag
// signals decide outright, else (absent a flat tag) the depth heuristic does.
template<typename _Type>
struct is_hierarchical_container
    : std::integral_constant<bool,
          internal::hierarchical_resolve_helper<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_hierarchical_container)

// is_depth_bounded_container
//   trait: true iff the container is hierarchical AND its value_type-chain depth
// is <= _N - for constraining functions to a recursion budget.
template<typename _Type,
         std::size_t _N>
struct is_depth_bounded_container
    : std::integral_constant<bool,
            is_hierarchical_container<_Type>::value
         && (container_depth<_Type>::value <= _N)>
{};


// ===========================================================================
// IV.  Structure-kind summary
// ===========================================================================

// structure_kind
//   enum: the container's position on the structure axis.  Ordered by nesting:
// a non-container, then flat (depth 1), then hierarchical (depth >= 2).
enum class structure_kind
{
    non_container,  // not a container
    flat,           // container, depth 1 (leaves only)
    hierarchical    // container, depth >= 2 (nests)
};

// structure_kind_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
structure_kind_name(structure_kind _k) noexcept
{
    return ( _k == structure_kind::non_container ? "non_container"
           : _k == structure_kind::flat          ? "flat"
           :                                        "hierarchical" );
}

// structure_kind_of
//   trait: classifies a type - non_container when it is not container-shaped,
// hierarchical when it nests, else flat.
template<typename _Type>
struct structure_kind_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr structure_kind value =
        ( !is_container_shape<clean_type>::value )
              ? structure_kind::non_container
      : (  is_hierarchical_container<clean_type>::value )
              ? structure_kind::hierarchical
      :         structure_kind::flat;

    using type = std::integral_constant<structure_kind, value>;
};

// structure_kind_of_t / structure_kind_of_v
template<typename _Type>
using structure_kind_of_t = typename structure_kind_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr structure_kind structure_kind_of_v =
        structure_kind_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr structure_kind structure_kind_of_v =
        structure_kind_of<_Type>::value;
#endif


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct hierarchical_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // detection signals
    static constexpr bool        node_summand =
        has_node_summand<clean_type>::value;
    static constexpr bool        has_structure_tag =
        has_structure_category<clean_type>::value;
    static constexpr bool        is_container =
        is_container_shape<clean_type>::value;

    // depth + verdict
    static constexpr std::size_t depth =
        container_depth<clean_type>::value;
    static constexpr bool        is_hierarchical =
        is_hierarchical_container<clean_type>::value;

    // summary
    static constexpr structure_kind kind =
        structure_kind_of<clean_type>::value;
    static constexpr const char*    kind_name =
        structure_kind_name(kind);
};


NS_END  // djinterp


#endif  // DJINTERP_HIERARCHICAL_CONTAINER_TRAITS_
