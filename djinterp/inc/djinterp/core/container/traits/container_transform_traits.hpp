/******************************************************************************
* djinterp [container]                          container_transform_traits.hpp
*
*   The Transformability axis: whether a container may be TRANSFORMED - rewritten
* position by position by a map on its element type, every element replaced by its
* image - and what of its anatomy survives.  Transformability is the twin of
* Filterability: the same composite alignment of a READ capability (iterability,
* to visit each element) and a BUILD capability (mutability, to gather the images),
* exercised now over a map f : tau -> sigma rather than a predicate.
*
*   A type is TRANSFORMABLE when mapping is closed up to its element type - the
* image is a container of its shape over sigma (its own type when sigma = tau, its
* constructor rebound to sigma otherwise), which needs both read and build.  A type
* that can be read but not grown is a TRANSFORM SOURCE only: it feeds a mapping
* whose result is built in another type.  The STRATEGY names the strongest
* realization the type's iterability grants (native, indexed, bidirectional,
* forward, external); the STAGE names when a mapping may run (compile-time, as a
* functional transformation, or runtime).
*
*   The weight of the axis is PRESERVATION, and it is the exact complement of
* selection's.  Because mapping keeps every position and its arrangement but
* REWRITES the value at it, it preserves the value-INDEPENDENT anatomy and may
* break the value-DEPENDENT: arrangement (order, structure) carries over, and size
* is preserved entirely - both the capacity CEILING and any capacity FLOOR, since
* no count changes - while sortedness need not survive (a map may invert an order)
* and uniqueness need not survive (a non-injective map may collide distinct
* values).  Selection keeps sortedness and uniqueness but breaks the floor; mapping
* keeps the floor but breaks sortedness and uniqueness.  Each keeps what the other
* drops.
*
*   These preservation traits are read of a container: arrangement gives
* transform_preserves_order = is_ordered (the result is ordered when the source is);
* size gives ceiling AND floor preserved, unconditionally; and sortedness and
* uniqueness are unconditionally NOT preserved, since the type cannot constrain an
* arbitrary map to be monotone or injective.
*
*   RESULT TYPE.  transform_result_type<C, Sigma> is the container a mapping into
* Sigma builds.  Where C is a simple element-parametric constructor it is rebound
* by rebind_element (its first type argument replaced by Sigma); otherwise - a
* standard container, whose allocator is coupled to its element type, or a
* non type-pack template such as a fixed array - a std::vector<Sigma> is offered
* instead.  rebind is confined to this result helper ON PURPOSE: the capability,
* strategy, stage, and preservation traits reason about the container alone and
* never form a rebound type, so an element type that cannot be safely rebound costs
* only the precision of the result, never the axis.  (This mirrors functor.hpp,
* which likewise keeps rebind out of the functor protocol, mapped result types
* being function-dependent.)
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language, as elsewhere.
*
*   NOTE (stage source).  The stage mirrors filter's is_compile_time(...) form but
* reads the compile-time lifetime from the meta-level lifetime_of rather than the
* container-trait container_lifetime, because constexpr_container_traits.hpp cannot
* at present be included alongside iterator_category_traits.hpp (the two iterator-
* category layers define overlapping symbols).  The two classifiers agree on every
* container tested; swapping in container_lifetime is a one-line change once that
* overlap is reconciled.
*
*
* path:      /inc/djinterp/core/container/traits/container_transform_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.02
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRANSFORM_TRAITS_
#define DJINTERP_CONTAINER_TRANSFORM_TRAITS_ 1

// std
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"              // D_VOID_T, D_TYPE_TRAIT_VALUE_BOOL
#include "../../meta/lifetime.hpp"                  // lifetime_of, is_compile_time (stage)
#include "./iterable_container_traits.hpp"          // is_iterable_container (read)
#include "./element_relation_traits.hpp"            // element_type_of_t (value_type)
#include "./iterator_category_traits.hpp"           // data accessor + iterator category
#include "./ordered_container_traits.hpp"                 // is_ordered_container (order)


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
    //   helper: the container can receive images (push_back or insert).
    template<typename _Container>
    struct build_capable_helper
        : std::integral_constant<bool,
                has_push_back_helper<_Container>::value
             || has_insert_value_helper<_Container>::value>
    {};

    // native_transform_helper
    //   helper: the container exposes its own transform(map) primitive, taking a
    // value_type(const value_type&) map (the element-preserving representative).
    template<typename _Container, typename = void>
    struct native_transform_helper : std::false_type {};
    template<typename _Container>
    struct native_transform_helper<_Container,
        D_VOID_T<decltype(std::declval<const _Container&>().transform(
            std::declval<typename _Container::value_type(*)(
                const typename _Container::value_type&)>()))>>
        : std::true_type {};

NS_END  // internal

// is_transform_source
//   trait: the container can supply elements to a mapping - it is iterable and
// typed (the READ capability), whether or not it can build a result.
template<typename _Type>
struct is_transform_source
    : std::integral_constant<bool,
            is_iterable_container<clean_t<_Type>>::value
         && internal::has_value_type_helper<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_transform_source)

// is_container_transformable
//   trait: mapping is closed on the container up to its element type - it can be
// read AND can gather the images into a container of its own shape (read plus
// build).
template<typename _Type>
struct is_container_transformable
    : std::integral_constant<bool,
            is_transform_source<clean_t<_Type>>::value
         && internal::build_capable_helper<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_container_transformable)

// is_transform_input_only
//   trait: the container can feed a mapping but not receive its result - a source
// without the build capability, so transforming it needs an external target.
template<typename _Type>
struct is_transform_input_only
    : std::integral_constant<bool,
            is_transform_source<clean_t<_Type>>::value
         && !internal::build_capable_helper<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_transform_input_only)

// has_native_transform
//   trait: the container exposes a transform(map) member of its own.
template<typename _Type>
struct has_native_transform
    : internal::native_transform_helper<clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_native_transform)


// ===========================================================================
// II.  Strategy (the realization)
// ===========================================================================

// transform_strategy
//   enum: the strongest mapping realization a type's iterability grants.  All
// compute the same result; the strategy is realization, not meaning.
enum class transform_strategy
{
    native,         // the type's own transform() primitive
    indexed,        // contiguous store: index-based mapping
    bidirectional,  // bidirectional iteration
    forward,        // single-pass forward iteration
    external,       // a source only: gather into a separate result
    unsupported     // not even a source
};

// transform_strategy_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
transform_strategy_name(transform_strategy _s) noexcept
{
    return ( _s == transform_strategy::native        ? "native"
           : _s == transform_strategy::indexed        ? "indexed"
           : _s == transform_strategy::bidirectional  ? "bidirectional"
           : _s == transform_strategy::forward        ? "forward"
           : _s == transform_strategy::external       ? "external"
           :                                            "unsupported" );
}

// container_transform_strategy
//   trait: the mapping strategy for a container - native first, then a source-only
// container maps externally, and a transformable one by the strongest category its
// iterator grants (indexed, bidirectional, forward).
template<typename _Type>
struct container_transform_strategy
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr transform_strategy value =
        ( !is_transform_source<clean_type>::value )
              ? transform_strategy::unsupported
      : ( has_native_transform<clean_type>::value )
              ? transform_strategy::native
      : ( !is_container_transformable<clean_type>::value )
              ? transform_strategy::external
      : ( has_data_accessor<clean_type>::value
       && is_random_access_iterable<clean_type>::value )
              ? transform_strategy::indexed
      : ( is_bidirectional_iterable<clean_type>::value )
              ? transform_strategy::bidirectional
      :         transform_strategy::forward;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr transform_strategy container_transform_strategy_v =
        container_transform_strategy<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr transform_strategy container_transform_strategy_v =
        container_transform_strategy<_Type>::value;
#endif


// ===========================================================================
// III. Stage (when a mapping may run)
// ===========================================================================

// transform_stage
//   enum: the stage at which a container admits mapping.  Compile-time mapping is
// a functional transformation over a statically-iterable source; runtime mapping
// runs during execution; none, where the type is not even a source.
enum class transform_stage
{
    none,
    runtime,
    compile_time
};

// transform_stage_name
constexpr const char*
transform_stage_name(transform_stage _s) noexcept
{
    return ( _s == transform_stage::none         ? "none"
           : _s == transform_stage::runtime       ? "runtime"
           :                                        "compile_time" );
}

// transform_stage_of
//   trait: the stage a container can be mapped at - compile-time when its data are
// compile-staged (a functional transformation, the map assumed statically
// evaluable), else runtime; none for a non-source.
template<typename _Type>
struct transform_stage_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr transform_stage value =
        ( !is_transform_source<clean_type>::value )
              ? transform_stage::none
      : ( is_compile_time(lifetime_of<clean_type>::value) )
              ? transform_stage::compile_time
      :         transform_stage::runtime;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr transform_stage transform_stage_of_v =
        transform_stage_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr transform_stage transform_stage_of_v =
        transform_stage_of<_Type>::value;
#endif


// ===========================================================================
// IV.  Preservation (the weight of the axis)
// ===========================================================================

// transform_preserves_order
//   trait: mapping keeps every position in its place, so the result is ordered
// exactly when the source is.
template<typename _Type>
struct transform_preserves_order
    : is_ordered_container<clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(transform_preserves_order)

// transform_preserves_size
//   trait: mapping rewrites values but adds and drops no position, so the size is
// unchanged - unconditionally true.
template<typename _Type>
struct transform_preserves_size : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool transform_preserves_size_v = true;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool transform_preserves_size_v = true;
#endif

// transform_preserves_capacity_ceiling
//   trait: the size is unchanged, so a capacity ceiling always survives mapping -
// unconditionally true.
template<typename _Type>
struct transform_preserves_capacity_ceiling : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool transform_preserves_capacity_ceiling_v = true;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool transform_preserves_capacity_ceiling_v = true;
#endif

// transform_preserves_capacity_floor
//   trait: the size is unchanged, so a capacity floor survives too - mapping keeps
// every position, and so cannot empty a container the way selection can.
// Unconditionally true; the exact complement of selection's single asymmetry.
template<typename _Type>
struct transform_preserves_capacity_floor : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool transform_preserves_capacity_floor_v = true;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool transform_preserves_capacity_floor_v = true;
#endif

// transform_preserves_sortedness
//   trait: a map need not be monotone, so it may invert an order - the image of a
// sorted container need not be sorted.  Unconditionally false; the type cannot
// constrain an arbitrary map to monotone.
template<typename _Type>
struct transform_preserves_sortedness : std::false_type {};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool transform_preserves_sortedness_v = false;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool transform_preserves_sortedness_v = false;
#endif

// transform_preserves_uniqueness
//   trait: a map need not be injective, so distinct values may collide - the image
// of a unique container need not be unique.  Unconditionally false; the type cannot
// constrain an arbitrary map to injective.
template<typename _Type>
struct transform_preserves_uniqueness : std::false_type {};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool transform_preserves_uniqueness_v = false;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool transform_preserves_uniqueness_v = false;
#endif


// ===========================================================================
// V.   Result type and element rebind
// ===========================================================================

NS_INTERNAL

    // is_type_pack_template_helper
    //   helper: whether _Container is a specialization of a template all of whose
    // parameters are types - the shape rebind_element can act on.
    template<typename _Container>
    struct is_type_pack_template_helper : std::false_type {};
    template<template<typename...> class _Tmpl, typename... _Args>
    struct is_type_pack_template_helper<_Tmpl<_Args...>> : std::true_type {};

    // has_allocator_helper
    //   helper: whether the container couples an allocator to its element type -
    // the mark of a standard container, whose first-argument rebind alone would
    // leave the allocator mistyped.
    template<typename _Container, typename = void>
    struct has_allocator_helper : std::false_type {};
    template<typename _Container>
    struct has_allocator_helper<_Container,
        D_VOID_T<typename _Container::allocator_type>>
        : std::true_type {};

    // rebind_element_impl
    //   helper: the container's constructor with its FIRST type argument replaced
    // by _Sigma; void for a non type-pack template.
    template<typename _Container, typename _Sigma>
    struct rebind_element_impl
    {
        using type = void;
    };
    template<template<typename...> class _Tmpl,
             typename    _First,
             typename... _Rest,
             typename    _Sigma>
    struct rebind_element_impl<_Tmpl<_First, _Rest...>, _Sigma>
    {
        using type = _Tmpl<_Sigma, _Rest...>;
    };

    // cleanly_rebindable_helper
    //   helper: a type-pack template with no allocator coupling - one whose
    // first-argument rebind yields a well-formed same-shape container.
    template<typename _Container>
    struct cleanly_rebindable_helper
        : std::integral_constant<bool,
                is_type_pack_template_helper<_Container>::value
             && !has_allocator_helper<_Container>::value>
    {};

NS_END  // internal

// rebind_element
//   trait: the container type _Container with its element (first type argument)
// replaced by _Sigma - Box<int> rebound by double is Box<double>.  void when
// _Container is not a type-pack template.
template<typename _Container,
         typename _Sigma>
struct rebind_element
{
    using type =
        typename internal::rebind_element_impl<clean_t<_Container>, _Sigma>::type;
};

template<typename _Container,
         typename _Sigma>
using rebind_element_t = typename rebind_element<_Container, _Sigma>::type;

NS_INTERNAL

    // transform_result_type_helper
    //   helper: the container a mapping into _Sigma builds.  A source that is a
    // simple element-parametric constructor rebinds to _Sigma; any other source -
    // a standard container, or a non type-pack template - builds a std::vector of
    // _Sigma; a non-source has no result.
    template<typename _Container,
             typename _Sigma,
             bool _Source      = is_transform_source<_Container>::value,
             bool _Rebindable  =
                 internal::cleanly_rebindable_helper<_Container>::value>
    struct transform_result_type_helper
    {
        using type = void;
    };

    template<typename _Container, typename _Sigma>
    struct transform_result_type_helper<_Container, _Sigma, true, true>
    {
        using type =
            typename internal::rebind_element_impl<_Container, _Sigma>::type;
    };

    template<typename _Container, typename _Sigma>
    struct transform_result_type_helper<_Container, _Sigma, true, false>
    {
        using type = std::vector<_Sigma>;
    };

NS_END  // internal

// transform_result_type
//   trait: the output container type of a mapping of _Type into _Sigma.
template<typename _Type,
         typename _Sigma>
struct transform_result_type
{
    using type =
        typename internal::transform_result_type_helper<
            clean_t<_Type>, _Sigma>::type;
};

template<typename _Type,
         typename _Sigma>
using transform_result_type_t =
    typename transform_result_type<_Type, _Sigma>::type;


// ===========================================================================
// VI.  Combined classification
// ===========================================================================

// container_transform_class
//   trait: the assembled Transformability of a container - its capability,
// strategy, stage, and the invariants a mapping preserves (and those it may
// break).
template<typename _Type>
struct container_transform_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // capability
    static constexpr bool is_transformable =
        is_container_transformable<clean_type>::value;
    static constexpr bool is_source =
        is_transform_source<clean_type>::value;
    static constexpr bool is_input_only =
        is_transform_input_only<clean_type>::value;
    static constexpr bool has_native =
        has_native_transform<clean_type>::value;

    // realization
    static constexpr transform_strategy strategy =
        container_transform_strategy<clean_type>::value;
    static constexpr transform_stage stage =
        transform_stage_of<clean_type>::value;

    // preservation
    static constexpr bool preserves_order =
        transform_preserves_order<clean_type>::value;
    static constexpr bool preserves_size             = true;
    static constexpr bool preserves_capacity_ceiling = true;
    static constexpr bool preserves_capacity_floor   = true;
    static constexpr bool preserves_sortedness       = false;
    static constexpr bool preserves_uniqueness       = false;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TRANSFORM_TRAITS_
