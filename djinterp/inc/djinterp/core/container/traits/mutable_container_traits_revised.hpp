/******************************************************************************
* djinterp [container]                    mutable_container_traits_revised.hpp
*
*   REVISED -- SFINAE structural traits for mutability classification.  The
* mutability axis now spans two dimensions: GRADE (what may change -- nothing /
* values / structure / both) and ACCESS RESTRICTION (whether an overlay forces
* const on a coordinate).  The grade is intrinsic; the access restriction is
* imposed by overlays (sets, maps, keyed containers).
*
*   GRADE (intrinsic to the type):
*   ======
*     fixed               no change; only const observation.
*     value-mutable       values at positions may be overwritten; structure fixed.
*     structure-mutable   positions (structure) may change; values const.
*                         (degenerate; rarely occurs without value mutation)
*     fully-mutable       both kinds of change permitted.
*
*   ACCESS RESTRICTION (from overlays):
*   ===================================
*   A keyed or uniqueness-preserving container may force const access on one or
*   more coordinates to protect its invariants:
*     - A set: element coordinate is const (uniqueness guard).
*     - A map: key coordinate is const (uniqueness of keys); value is settable.
*     - A sequence / multiset: no coordinate is constrained (value settable).
*
*   CONNECTION TO ITERABILITY:
*   ==========================
*   Value mutation is exactly what a non-const traversal grants at each position
*   (Iterability).  Access restrictions modulate this: a set's non-const iterator
*   exposes a const-qualified element, forbidding assignment by overlay.  The two
*   are orthogonal axes that interact.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with language features.
*
*
* path:      /inc/djinterp/core/container/traits/mutable_container_traits_revised.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                    created: 2026.07.03
*                                                        revised: 2026.07.03
******************************************************************************/

#ifndef DJINTERP_MUTABLE_CONTAINER_TRAITS_REVISED_
#define DJINTERP_MUTABLE_CONTAINER_TRAITS_REVISED_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"            // clean_t, NS_*, D_ENV_* feature macros
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_* detection macros


NS_DJINTERP


// ===========================================================================
// I.   SFINAE mutator detection (unchanged from prior version)
// ===========================================================================
//   Signals for element mutation (value-level change) and structural mutation
// (set-level change).  A "signal" is evidence; umbrellas below combine them.

// --- structural mutators (change the SET of elements) ---

D_TYPE_TRAIT_TRUE(has_push_back_signal,
    decltype(std::declval<clean_t<_Type>&>().push_back(
        std::declval<typename clean_t<_Type>::value_type>())))

D_TYPE_TRAIT_TRUE(has_push_front_signal,
    decltype(std::declval<clean_t<_Type>&>().push_front(
        std::declval<typename clean_t<_Type>::value_type>())))

D_TYPE_TRAIT_TRUE(has_insert_signal,
    decltype(std::declval<clean_t<_Type>&>().insert(
        std::declval<typename clean_t<_Type>::value_type>())))

D_TYPE_TRAIT_TRUE(has_erase_signal,
    decltype(std::declval<clean_t<_Type>&>().erase(
        std::declval<clean_t<_Type>&>().begin())))

D_TYPE_TRAIT_TRUE(has_clear_signal,
    decltype(std::declval<clean_t<_Type>&>().clear()))

D_TYPE_TRAIT_TRUE(has_resize_signal,
    decltype(std::declval<clean_t<_Type>&>().resize(std::size_t{})))

// --- element mutators (overwrite EXISTING element in place) ---

D_TYPE_TRAIT_TRUE(has_mutable_subscript_signal,
    decltype(std::declval<clean_t<_Type>&>()[std::size_t{}] =
             std::declval<typename clean_t<_Type>::value_type>()))

D_TYPE_TRAIT_TRUE(has_mutable_data_signal,
    decltype(*std::declval<clean_t<_Type>&>().data() =
             std::declval<typename clean_t<_Type>::value_type>()))

// --- const observation (marks container-like types) ---

D_TYPE_TRAIT_TRUE(has_size_signal,
    decltype(std::declval<const clean_t<_Type>&>().size()))


// ===========================================================================
// II.  Capability umbrellas
// ===========================================================================

template<typename _Type>
struct is_element_mutable_container
    : std::integral_constant<bool,
          (    has_mutable_subscript_signal<clean_t<_Type>>::value
            || has_mutable_data_signal<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_element_mutable_container)

template<typename _Type>
struct is_structurally_mutable_container
    : std::integral_constant<bool,
          (    has_push_back_signal<clean_t<_Type>>::value
            || has_push_front_signal<clean_t<_Type>>::value
            || has_insert_signal<clean_t<_Type>>::value
            || has_erase_signal<clean_t<_Type>>::value
            || has_clear_signal<clean_t<_Type>>::value
            || has_resize_signal<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_structurally_mutable_container)

template<typename _Type>
struct is_mutable_container
    : std::integral_constant<bool,
          (    is_element_mutable_container<clean_t<_Type>>::value
            || is_structurally_mutable_container<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_mutable_container)

template<typename _Type>
struct is_immutable_container
    : std::integral_constant<bool,
          (    has_size_signal<clean_t<_Type>>::value
            && !is_mutable_container<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_immutable_container)


// ===========================================================================
// III. The mutability grade (revised enum)
// ===========================================================================
//   Four grades in a chain of increasing freedom:
//     fixed < value_mutable < fully_mutable
//   (structure_mutable is orthogonal, degenerate, not shown here)

enum class mutability_grade
{
    fixed,              // no change
    value_mutable,      // values at positions may change; structure fixed
    structure_mutable,  // structure (positions) may change; values const
    fully_mutable       // both kinds change permitted
};

constexpr const char*
mutability_grade_name(mutability_grade _g) noexcept
{
    return ( _g == mutability_grade::fixed               ? "fixed"
           : _g == mutability_grade::value_mutable      ? "value_mutable"
           : _g == mutability_grade::structure_mutable  ? "structure_mutable"
           :                                              "fully_mutable" );
}

constexpr bool
is_fixed_grade(mutability_grade _g) noexcept
{
    return ( _g == mutability_grade::fixed );
}

constexpr bool
is_value_mutable_grade(mutability_grade _g) noexcept
{
    return ( _g == mutability_grade::value_mutable );
}

constexpr bool
is_structure_mutable_grade(mutability_grade _g) noexcept
{
    return ( _g == mutability_grade::structure_mutable );
}

constexpr bool
is_fully_mutable_grade(mutability_grade _g) noexcept
{
    return ( _g == mutability_grade::fully_mutable );
}

constexpr bool
grade_permits_element_mutation(mutability_grade _g) noexcept
{
    return ( ( _g == mutability_grade::value_mutable ) ||
             ( _g == mutability_grade::fully_mutable ) );
}

constexpr bool
grade_permits_structural_mutation(mutability_grade _g) noexcept
{
    return ( ( _g == mutability_grade::structure_mutable ) ||
             ( _g == mutability_grade::fully_mutable ) );
}

// mutability_of
//   trait: the grade of a container from its two capability bits.
template<typename _Type>
struct mutability_of
{
private:
    using clean_type = clean_t<_Type>;
    static constexpr bool element =
        is_element_mutable_container<clean_type>::value;
    static constexpr bool structural =
        is_structurally_mutable_container<clean_type>::value;

public:
    static constexpr mutability_grade value =
        ( element && structural ) ? mutability_grade::fully_mutable
      : ( structural )            ? mutability_grade::structure_mutable
      : ( element )               ? mutability_grade::value_mutable
      :                             mutability_grade::fixed;

    using type = std::integral_constant<mutability_grade, value>;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr mutability_grade mutability_of_v = mutability_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr mutability_grade mutability_of_v = mutability_of<_Type>::value;
#endif


// ===========================================================================
// IV.  Access restriction (from overlays)
// ===========================================================================
//   For keyed and uniqueness-guarded containers, one or more coordinates are
// forced const by the overlay invariant.  This is orthogonal to grade.

enum class coordinate_access_restriction
{
    none,               // no coordinate is constrained (sequence, multiset)
    element_const,      // element is const (set, unique-element containers)
    key_const,          // key is const, value settable (map, multimap)
    both_const          // both key and value const (rare; very restrictive)
};

constexpr const char*
coordinate_access_restriction_name(coordinate_access_restriction _r) noexcept
{
    return ( _r == coordinate_access_restriction::none          ? "none"
           : _r == coordinate_access_restriction::element_const ? "element_const"
           : _r == coordinate_access_restriction::key_const     ? "key_const"
           :                                                      "both_const" );
}

// coordinate_settable
//   function: true iff a coordinate is NOT constrained and may be assigned.
constexpr bool
is_coordinate_settable(coordinate_access_restriction _r,
                       int                            _coordinate)
    noexcept
{
    // _coordinate: 0 = element (or key for keyed), 1 = value (for keyed)
    return ( ( _r == coordinate_access_restriction::none ) ||
             ( _r == coordinate_access_restriction::key_const && _coordinate == 1 ) );
}

// settable_coordinate_of
//   trait: the access restriction imposed by an overlay on a container.
// Keyed containers: key_const (keys are unique/identity).
// Sets / uniqueness-guarded: element_const (element uniqueness).
// Sequences, multisets: none (unrestricted).
template<typename _Type, typename = void>
struct settable_coordinate_of
    : std::integral_constant<coordinate_access_restriction,
                             coordinate_access_restriction::none>
{};

// Specialization hook for keyed detection (requires overlay_of or is_keyed_container).
// NOTE: This is left as a stub; the real implementation depends on is_keyed_container
// and is_unique_container probes from container_overlay_traits.hpp.


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct mutable_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // individual signals
    static constexpr bool can_push_back =
        has_push_back_signal<clean_type>::value;
    static constexpr bool can_push_front =
        has_push_front_signal<clean_type>::value;
    static constexpr bool can_insert =
        has_insert_signal<clean_type>::value;
    static constexpr bool can_erase =
        has_erase_signal<clean_type>::value;
    static constexpr bool can_clear =
        has_clear_signal<clean_type>::value;
    static constexpr bool can_resize =
        has_resize_signal<clean_type>::value;
    static constexpr bool can_overwrite_element =
        is_element_mutable_container<clean_type>::value;

    // capabilities
    static constexpr bool       is_element_mutable =
        is_element_mutable_container<clean_type>::value;
    static constexpr bool       is_structurally_mutable =
        is_structurally_mutable_container<clean_type>::value;
    static constexpr bool       is_mutable =
        is_mutable_container<clean_type>::value;
    static constexpr bool       is_immutable =
        is_immutable_container<clean_type>::value;

    // grade (revised)
    static constexpr mutability_grade grade =
        mutability_of<clean_type>::value;
    static constexpr const char* grade_name =
        mutability_grade_name(grade);

    // access restriction (stub; specializations override)
    static constexpr coordinate_access_restriction access_restriction =
        settable_coordinate_of<clean_type>::value;
    static constexpr const char* access_restriction_name =
        coordinate_access_restriction_name(access_restriction);
};


// ===========================================================================
// VI.  Backward compatibility: map old mutability enum to new grade
// ===========================================================================
//   For existing code using the older binary enum, provide a conversion.
//   NOTE: This conversion is conservative and loses information.

enum class mutability  // old enum (DEPRECATED, for compat only)
{
    immutable,
    element_mutable,
    structural_mutable,
    fully_mutable
};

constexpr const char*
mutability_name(mutability _m) noexcept  // old function (for compat)
{
    return ( _m == mutability::immutable          ? "immutable"
           : _m == mutability::element_mutable    ? "element_mutable"
           : _m == mutability::structural_mutable ? "structural_mutable"
           :                                        "fully_mutable" );
}

// Old value-level predicates (DEPRECATED, for compat)
constexpr bool
is_immutable_grade(mutability _m) noexcept
{
    return ( _m == mutability::immutable );
}

constexpr bool
is_element_mutable_grade(mutability _m) noexcept
{
    return ( _m == mutability::element_mutable );
}

constexpr bool
is_structural_mutable_grade(mutability _m) noexcept
{
    return ( _m == mutability::structural_mutable );
}

constexpr bool
is_fully_mutable_grade(mutability _m) noexcept
{
    return ( _m == mutability::fully_mutable );
}

constexpr bool
grade_admits_element_mutation(mutability _m) noexcept
{
    return ( ( _m == mutability::element_mutable ) ||
             ( _m == mutability::fully_mutable ) );
}

constexpr bool
grade_admits_structural_mutation(mutability _m) noexcept
{
    return ( ( _m == mutability::structural_mutable ) ||
             ( _m == mutability::fully_mutable ) );
}


NS_END  // djinterp


#endif  // DJINTERP_MUTABLE_CONTAINER_TRAITS_REVISED_
