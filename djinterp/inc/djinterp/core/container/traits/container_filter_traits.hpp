/******************************************************************************
* djinterp [container]                             container_filter_traits.hpp
*
*   The Filterability axis: whether a container may be FILTERED - reduced to the
* sub-container of elements satisfying a predicate - and what of its anatomy
* survives.  Filterability is the composite intrinsic axis: it holds where a READ
* capability (iterability, to visit and test) and a BUILD capability (mutability,
* to gather the survivors) align over a predicate on the element type.
*
*   A type is FILTERABLE when selection is closed on it - the result is again of
* its type, which needs both read and build.  A type that can be read and tested
* but not grown is a FILTER SOURCE only: it feeds a selection whose result is
* built in another type.  The STRATEGY names the strongest realization the type's
* iterability grants (native, indexed, bidirectional, forward, external); the
* STAGE names when a selection may run (compile-time, as a functional selection,
* or runtime).
*
*   The weight of the axis is PRESERVATION.  Because selection removes elements
* without reordering or transforming survivors, it preserves the whole anatomy of
* the survivors save one bound: arrangement (order, structure) and sortedness
* carry over, multiplicity can only fall so uniqueness is kept, and the capacity
* CEILING holds since the result is no larger.  What it does not preserve is a
* capacity FLOOR - the constant-false predicate empties the container - the single
* invariant selection can break.
*
*   These preservation traits are read of a container: because selection keeps the
* survivors' arrangement, the result is ordered / sorted / unique exactly when the
* source is (the trait reports that surviving status), the ceiling always holds,
* and a floor never does.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language, as elsewhere.
*
*
* path:      /inc/djinterp/core/container/traits/container_filter_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_FILTER_TRAITS_
#define DJINTERP_CONTAINER_FILTER_TRAITS_ 1

// std
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"              // D_VOID_T, D_TYPE_TRAIT_VALUE_BOOL
#include "../../meta/lifetime.hpp"                  // is_compile_time
#include "./constexpr_container_traits.hpp"         // container_lifetime (filter stage)
#include "./iterable_container_traits.hpp"          // is_iterable_container (read)
#include "./element_relation_traits.hpp"            // element_type_of_t (value_type)
#include "./container_multiplicity_traits.hpp"      // is_unique_container (uniqueness)
#include "./bounded_container_traits.hpp"           // is_bounded_container (capacity)
#include "./ordered_container_traits.hpp"                 // is_ordered_container (order)
#include "./sorted_container_traits.hpp"                  // is_sorted_container (sortedness)
#include "./iterator_category_traits.hpp"           // is_*_iterable, iterator_category_of (moved here)


NS_DJINTERP


// ===========================================================================
// I.   Capability detection
// ===========================================================================

NS_INTERNAL

    // has_value_type_helper
    //   helper: whether a container exposes a value_type (a void element type
    // from element_type_of means there is none).
    template<typename _Container>
    struct has_value_type_helper
        : std::integral_constant<bool,
              !std::is_void<element_type_of_t<_Container>>::value>
    {};

    // has_push_back_helper / has_insert_value_helper
    //   helper: the BUILD capability - the container can grow by an element, by
    // sequence push_back or associative insert.
    template<typename _Container, typename = void>
    struct has_push_back_helper : std::false_type {};
    template<typename _Container>
    struct has_push_back_helper<_Container,
        D_VOID_T<decltype(std::declval<_Container&>().push_back(
            std::declval<typename _Container::value_type>()))>>
        : std::true_type {};

    template<typename _Container, typename = void>
    struct has_insert_value_helper : std::false_type {};
    template<typename _Container>
    struct has_insert_value_helper<_Container,
        D_VOID_T<decltype(std::declval<_Container&>().insert(
            std::declval<typename _Container::value_type>()))>>
        : std::true_type {};

    // build_capable_helper
    //   helper: the container can receive survivors (push_back or insert).
    template<typename _Container>
    struct build_capable_helper
        : std::integral_constant<bool,
                has_push_back_helper<_Container>::value
             || has_insert_value_helper<_Container>::value>
    {};

    // native_filter_helper
    //   helper: the container exposes its own filter(predicate) primitive, taking
    // a bool(const value_type&) test.
    template<typename _Container, typename = void>
    struct native_filter_helper : std::false_type {};
    template<typename _Container>
    struct native_filter_helper<_Container,
        D_VOID_T<decltype(std::declval<const _Container&>().filter(
            std::declval<bool(*)(const typename _Container::value_type&)>()))>>
        : std::true_type {};

NS_END  // internal

// is_filter_source
//   trait: the container can supply elements to a selection - it is iterable and
// typed (the READ capability), whether or not it can build a result.
template<typename _Type>
struct is_filter_source
    : std::integral_constant<bool,
            is_iterable_container<clean_t<_Type>>::value
         && internal::has_value_type_helper<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_filter_source)

// is_container_filterable
//   trait: selection is closed on the container - it can be read AND can gather
// the survivors into a container of its own type (read plus build).
template<typename _Type>
struct is_container_filterable
    : std::integral_constant<bool,
            is_filter_source<clean_t<_Type>>::value
         && internal::build_capable_helper<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_container_filterable)

// is_filter_input_only
//   trait: the container can feed a selection but not receive its result - a
// source without the build capability, so filtering it needs an external target.
template<typename _Type>
struct is_filter_input_only
    : std::integral_constant<bool,
            is_filter_source<clean_t<_Type>>::value
         && !internal::build_capable_helper<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_filter_input_only)

// has_native_filter
//   trait: the container exposes a filter(predicate) member of its own.
template<typename _Type>
struct has_native_filter
    : internal::native_filter_helper<clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_native_filter)


// ===========================================================================
// II.  Strategy (the realization)
// ===========================================================================

// filter_strategy
//   enum: the strongest selection realization a type's iterability grants.  All
// compute the same result; the strategy is realization, not meaning.
enum class filter_strategy
{
    native,         // the type's own filter() primitive
    indexed,        // contiguous store: index-based selection
    bidirectional,  // bidirectional iteration
    forward,        // single-pass forward iteration
    external,       // a source only: gather into a separate result
    unsupported     // not even a source
};

// filter_strategy_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
filter_strategy_name(filter_strategy _s) noexcept
{
    return ( _s == filter_strategy::native        ? "native"
           : _s == filter_strategy::indexed        ? "indexed"
           : _s == filter_strategy::bidirectional  ? "bidirectional"
           : _s == filter_strategy::forward        ? "forward"
           : _s == filter_strategy::external       ? "external"
           :                                         "unsupported" );
}

// container_filter_strategy
//   trait: the selection strategy for a container - native first, then a source-
// only container filters externally, and a filterable one by the strongest
// category its iterator grants (indexed, bidirectional, forward).
template<typename _Type>
struct container_filter_strategy
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr filter_strategy value =
        ( !is_filter_source<clean_type>::value )
              ? filter_strategy::unsupported
      : ( has_native_filter<clean_type>::value )
              ? filter_strategy::native
      : ( !is_container_filterable<clean_type>::value )
              ? filter_strategy::external
      : ( has_data_accessor<clean_type>::value
       && is_random_access_iterable<clean_type>::value )
              ? filter_strategy::indexed
      : ( is_bidirectional_iterable<clean_type>::value )
              ? filter_strategy::bidirectional
      :         filter_strategy::forward;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr filter_strategy container_filter_strategy_v =
        container_filter_strategy<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr filter_strategy container_filter_strategy_v =
        container_filter_strategy<_Type>::value;
#endif


// ===========================================================================
// III. Stage (when a selection may run)
// ===========================================================================

// filter_stage
//   enum: the stage at which a container admits selection.  Compile-time
// selection is a functional selection over a statically-iterable source; runtime
// selection runs during execution; none, where the type is not even a source.
enum class filter_stage
{
    none,
    runtime,
    compile_time
};

// filter_stage_name
constexpr const char*
filter_stage_name(filter_stage _s) noexcept
{
    return ( _s == filter_stage::none         ? "none"
           : _s == filter_stage::runtime       ? "runtime"
           :                                     "compile_time" );
}

// filter_stage_of
//   trait: the stage a container can be selected at - compile-time when its
// data are compile-staged (a functional selection, the predicate assumed
// statically evaluable), else runtime; none for a non-source.
template<typename _Type>
struct filter_stage_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr filter_stage value =
        ( !is_filter_source<clean_type>::value )
              ? filter_stage::none
      : ( is_compile_time(container_lifetime<clean_type>::value) )
              ? filter_stage::compile_time
      :         filter_stage::runtime;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr filter_stage filter_stage_of_v =
        filter_stage_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr filter_stage filter_stage_of_v =
        filter_stage_of<_Type>::value;
#endif


// ===========================================================================
// IV.  Preservation (the weight of the axis)
// ===========================================================================

// filter_preserves_order
//   trait: selection keeps the survivors' arrangement, so the result is ordered
// exactly when the source is.
template<typename _Type>
struct filter_preserves_order
    : is_ordered_container<clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(filter_preserves_order)

// filter_preserves_sortedness
//   trait: a sub-enumeration of a monotone enumeration is monotone, so the
// result is sorted exactly when the source is.
template<typename _Type>
struct filter_preserves_sortedness
    : is_sorted_container<clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(filter_preserves_sortedness)

// filter_preserves_uniqueness
//   trait: multiplicity can only fall under selection, so uniqueness is kept -
// the result is unique exactly when the source is.
template<typename _Type>
struct filter_preserves_uniqueness
    : is_unique_container<clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(filter_preserves_uniqueness)

// filter_preserves_capacity_ceiling
//   trait: the result is no larger than the source, so a capacity ceiling always
// survives selection - unconditionally true.
template<typename _Type>
struct filter_preserves_capacity_ceiling : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool filter_preserves_capacity_ceiling_v = true;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool filter_preserves_capacity_ceiling_v = true;
#endif

// filter_preserves_capacity_floor
//   trait: the constant-false predicate empties the container, so no positive
// lower bound on size survives - unconditionally false.  The single asymmetry.
template<typename _Type>
struct filter_preserves_capacity_floor : std::false_type {};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool filter_preserves_capacity_floor_v = false;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool filter_preserves_capacity_floor_v = false;
#endif


// ===========================================================================
// V.   Result type
// ===========================================================================

NS_INTERNAL

    // filter_result_type_helper
    //   helper: the container a selection builds.  A filterable type selects into
    // its own type; a source-only type into a std::vector of its value_type.
    template<typename _Container,
             bool _Filterable = is_container_filterable<_Container>::value,
             bool _InputOnly  = is_filter_input_only<_Container>::value>
    struct filter_result_type_helper
    {
        using type = void;
    };

    template<typename _Container>
    struct filter_result_type_helper<_Container, true, false>
    {
        using type = _Container;
    };

    template<typename _Container>
    struct filter_result_type_helper<_Container, false, true>
    {
        using type = std::vector<typename _Container::value_type>;
    };

NS_END  // internal

// filter_result_type
//   trait: the output container type of a selection over _Type.
template<typename _Type>
struct filter_result_type
{
    using type =
        typename internal::filter_result_type_helper<clean_t<_Type>>::type;
};

template<typename _Type>
using filter_result_type_t = typename filter_result_type<_Type>::type;


// ===========================================================================
// VI.  Combined classification
// ===========================================================================

// container_filter_class
//   trait: the assembled Filterability of a container - its capability, strategy,
// stage, and the invariants a selection preserves.
template<typename _Type>
struct container_filter_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // capability
    static constexpr bool is_filterable =
        is_container_filterable<clean_type>::value;
    static constexpr bool is_source =
        is_filter_source<clean_type>::value;
    static constexpr bool is_input_only =
        is_filter_input_only<clean_type>::value;
    static constexpr bool has_native =
        has_native_filter<clean_type>::value;

    // realization
    static constexpr filter_strategy strategy =
        container_filter_strategy<clean_type>::value;
    static constexpr filter_stage stage =
        filter_stage_of<clean_type>::value;

    // preservation
    static constexpr bool preserves_order =
        filter_preserves_order<clean_type>::value;
    static constexpr bool preserves_sortedness =
        filter_preserves_sortedness<clean_type>::value;
    static constexpr bool preserves_uniqueness =
        filter_preserves_uniqueness<clean_type>::value;
    static constexpr bool preserves_capacity_ceiling = true;
    static constexpr bool preserves_capacity_floor   = false;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_FILTER_TRAITS_
