/*******************************************************************************
* djinterp [core]                                   mutable_container_traits.hpp
*
* SFINAE structural traits for mutability classification - WHAT mutation a
* container's public interface admits.
*
*   Two independent capabilities are distinguished, then summarised: ELEMENT
* mutation overwriting an EXISTING element in place, through a non-const
* operator[] or a non-const data() - the set of elements is unchanged, only
* their values; STRUCTURAL mutation changing the SET of elements - push / pop
* / insert / erase / clear / resize. The two are genuinely orthogonal. A fixed
* inline array overwrites elements but cannot grow (element-only); an ordered
* set inserts and erases but exposes no writable element handle
* (structural-only); a vector does both. The four corners are named by the
* `mutability` grade: immutable neither - only const observation;
* element_mutable element mutation, no structural; structural_mutable
* structural mutation, no element; fully_mutable both. "Mutable" (the
* umbrella) is element OR structural; "immutable" is a type that LOOKS like a
* container (it has a size()) yet exposes neither. THE ACCESS DIMENSION (a 2-D
* model). The grade is one dimension - WHICH KINDS of change are admitted
* (fixed / value / structure / both). A second, orthogonal dimension records,
* for the element-mutation capability, WHICH COORDINATE the discipline leaves
* settable: an overlay may hold one coordinate const while freeing another - a
* set / multiset freezes its ELEMENT, a map / multimap freezes its KEY while
* freeing the VALUE, a plain sequence freezes neither - so that a non-const
* iteration reaches only the unconstrained coordinate (Iterability).
* mutability_of returns the grade; access_restriction_of returns the
* coordinate the discipline constrains; the two together locate a container in
* the model. The dimension is read from the discipline with the same local
* signals the overlay axis uses (a nested key_type, a pair value_type),
* keeping this module self- contained - free of the overlay / container_traits
* include graph.
*
*   AXIS ORTHOGONALITY:
*   Mutability is independent of the lifetime, storage, and memory-discipline
* axes - no entailment is drawn here. A literal type may be mutated within a
* constexpr context (so compile-time lifetime does NOT imply immutability),
* and a bounded inline buffer may be grown structurally within its capacity
* (so structural mutation does NOT imply dynamic storage). The read-only /
* write-only / read-write ACCESS WRAPPERS are a further, separate concern - a
* capability overlay imposed on a whole container, distinct from the
* per-coordinate access restriction above - and live with those wrappers, not
* in this file.
*
*   PORTABILITY:
*   C++11 baseline; the `_v` companions the D_TYPE_TRAIT_* engine emits
* degrade with the language exactly as the rest of the trait family's do.
*
*
* path:      /inc/djinterp/core/container/traits/mutable_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.25
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRAITS_MUTABLE_CONTAINER_TRAITS_HPP
#define DJINTERP_CONTAINER_TRAITS_MUTABLE_CONTAINER_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"    // clean_t, NS_*, D_ENV_* macros
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_* detection macros
#include "./bounded_container_traits.hpp"  // has_size_signal (single owner)


NS_DJINTERP


// ===========================================================================
// I.   SFINAE mutator detection
// ===========================================================================
//   Each detector is named with a `_signal` suffix to avoid clashing with any
// homonym elsewhere in the trait family.  A "signal" is one piece of evidence;
// the umbrellas below combine them.  Probes strip cv-ref via clean_t, so the
// answer agrees for C, const C, C&.

// --- structural mutators (change the SET of elements) ---

// has_push_back_signal
D_TYPE_TRAIT_TRUE(has_push_back_signal,
    decltype(std::declval<clean_t<Type>&>().push_back(
        std::declval<typename clean_t<Type>::value_type>())))

// has_push_front_signal
D_TYPE_TRAIT_TRUE(has_push_front_signal,
    decltype(std::declval<clean_t<Type>&>().push_front(
        std::declval<typename clean_t<Type>::value_type>())))

// has_insert_signal
D_TYPE_TRAIT_TRUE(has_insert_signal,
    decltype(std::declval<clean_t<Type>&>().insert(
        std::declval<typename clean_t<Type>::value_type>())))

// has_erase_signal
D_TYPE_TRAIT_TRUE(has_erase_signal,
    decltype(std::declval<clean_t<Type>&>().erase(
        std::declval<clean_t<Type>&>().begin())))

// has_clear_signal
D_TYPE_TRAIT_TRUE(has_clear_signal,
    decltype(std::declval<clean_t<Type>&>().clear()))

// has_resize_signal
D_TYPE_TRAIT_TRUE(has_resize_signal,
    decltype(std::declval<clean_t<Type>&>().resize(std::size_t{})))

// --- element mutators (overwrite an EXISTING element in place) ---

// has_mutable_subscript_signal
//   trait: operator[] yields a non-const lvalue (the subscript is assignable).
D_TYPE_TRAIT_TRUE(has_mutable_subscript_signal,
    decltype(std::declval<clean_t<Type>&>()[std::size_t{}] =
             std::declval<typename clean_t<Type>::value_type>()))

// has_mutable_data_signal
//   trait: data() yields a non-const pointer (the pointee is assignable).
D_TYPE_TRAIT_TRUE(has_mutable_data_signal,
    decltype(*std::declval<clean_t<Type>&>().data() =
             std::declval<typename clean_t<Type>::value_type>()))

// --- const observation (the "looks like a container" marker) ---
//   has_size_signal now has a single owner in bounded_container_traits.hpp
// (included above); this file consumes it from there.


// ===========================================================================
// II.  Capability umbrellas
// ===========================================================================

// is_element_mutable_container
//   trait: true iff an existing element can be overwritten in place.
template<typename Type>
struct is_element_mutable_container
    : std::integral_constant<bool,
          (    has_mutable_subscript_signal<clean_t<Type>>::value
            || has_mutable_data_signal<clean_t<Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_element_mutable_container)

// is_structurally_mutable_container
//   trait: true iff the set of elements can be changed.
template<typename Type>
struct is_structurally_mutable_container
    : std::integral_constant<bool,
          (    has_push_back_signal<clean_t<Type>>::value
            || has_push_front_signal<clean_t<Type>>::value
            || has_insert_signal<clean_t<Type>>::value
            || has_erase_signal<clean_t<Type>>::value
            || has_clear_signal<clean_t<Type>>::value
            || has_resize_signal<clean_t<Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_structurally_mutable_container)

// is_mutable_container
//   trait: true iff ANY mutator is present - element OR structural.
template<typename Type>
struct is_mutable_container
    : std::integral_constant<bool,
          (    is_element_mutable_container<clean_t<Type>>::value
            || is_structurally_mutable_container<clean_t<Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_mutable_container)

// is_immutable_container
//   trait: true iff the type LOOKS like a container (it has a size()) but
// exposes NONE of the mutation signals.
template<typename Type>
struct is_immutable_container
    : std::integral_constant<bool,
          (    has_size_signal<clean_t<Type>>::value
            && !is_mutable_container<clean_t<Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_immutable_container)


// ===========================================================================
// III. The mutability grade
// ===========================================================================

// mutability
//   enum: the summary of the two independent capabilities - the four corners
// of the (element, structural) square.
enum class mutability
{
    immutable,          // neither capability
    element_mutable,    // element only
    structural_mutable, // structural only
    fully_mutable       // both
};

// mutability_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
mutability_name(mutability _m) noexcept
{
    return ( _m == mutability::immutable          ? "immutable"
           : _m == mutability::element_mutable    ? "element_mutable"
           : _m == mutability::structural_mutable ? "structural_mutable"
           :                                        "fully_mutable" );
}

// is_immutable_grade / is_element_mutable_grade / ...
//   function: value-level predicates over a mutability grade.
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

// grade_admits_element_mutation
//   function: true iff the grade permits overwriting elements (element OR
// full).
constexpr bool
grade_admits_element_mutation(mutability _m) noexcept
{
    return ( ( _m == mutability::element_mutable ) ||
             ( _m == mutability::fully_mutable ) );
}

// grade_admits_structural_mutation
//   function: true iff the grade permits structural change (structural OR
// full).
constexpr bool
grade_admits_structural_mutation(mutability _m) noexcept
{
    return ( ( _m == mutability::structural_mutable ) ||
             ( _m == mutability::fully_mutable ) );
}

// mutability_of
//   trait: the mutability grade of a container, from its two capability bits.
// (A type with no mutators reads as immutable whether or not it is a
// container;
//  pair with is_immutable_container when container-hood must be confirmed.)
template<typename Type>
struct mutability_of
{
private:
    using clean_type = clean_t<Type>;
    static constexpr bool element    =
        is_element_mutable_container<clean_type>::value;
    static constexpr bool structural =
        is_structurally_mutable_container<clean_type>::value;

public:
    static constexpr mutability value =
        ( element && structural ) ? mutability::fully_mutable
      : ( structural )            ? mutability::structural_mutable
      : ( element )               ? mutability::element_mutable
      :                             mutability::immutable;

    using type = std::integral_constant<mutability, value>;
};

// mutability_of_t / mutability_of_v
//   type / value: the carrier and, where the language permits, the value
// companion of mutability_of.
template<typename Type>
using mutability_of_t = typename mutability_of<Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Type>
    inline constexpr mutability mutability_of_v = mutability_of<Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr mutability mutability_of_v = mutability_of<Type>::value;
#endif

// mutability_grade / mutability_grade_of
//   alias: the GRADE dimension of the 2-D model is exactly the existing
// `mutability` enum and `mutability_of` trait; these name them by their
// dimension, for callers that pair the grade with the access dimension below.
using mutability_grade = mutability;

template<typename Type>
using mutability_grade_of = mutability_of<Type>;


// ===========================================================================
// IV.  The access dimension  (the 2-D model)
// ===========================================================================
// The grade above answers WHICH KINDS of change a container admits. This
// second, orthogonal dimension answers, for the element-mutation capability,
// WHICH COORDINATE the discipline leaves settable. An overlay may hold one
// coordinate const while freeing another (Overlays): a set or multiset holds
// its ELEMENT const - mutating it mid-traversal could break uniqueness or
// sortedness, so an in-place value change is barred and mutation is structural;
// a map or multimap holds its KEY const while leaving the VALUE settable; a
// plain sequence constrains neither. A non-const iteration therefore reaches
// only the unconstrained coordinate (Iterability). The coordinate is read from
// the discipline with the same local signals the overlay axis uses - a nested
// key_type is the duplicate-equivalence mark, a pair value_type the keyed mark
// - so this module needs no dependency on the overlay header.

NS_INTERNAL

    // has_equivalence_helper
    //   trait: a nested key_type - the duplicate-equivalence mark of an
    // associative discipline (set / multiset / map / multimap), as against a
    // comparator-less sequence which carries none.
    D_TYPE_TRAIT_TRUE(has_equivalence_helper,
        typename clean_t<Type>::key_type)

    // has_keyed_value_helper
    //   trait: the value_type is a pair (first_type / second_type) - the keyed
    // mark (map / multimap), mirroring is_keyed_container of the overlay axis.
    D_TYPE_TRAIT_TRUE(has_keyed_value_helper,
        typename clean_t<Type>::value_type::first_type,
        typename clean_t<Type>::value_type::second_type)

NS_END  // internal

// access_restriction
//   enum: which coordinate the discipline holds const - the access dimension.
// `none` for a plain sequence (the whole element is settable in place, if the
// grade allows); `element_const` for a set / multiset (the element is frozen,
// so no in-place value write - change is structural); `key_const` for a map /
// multimap (the key is frozen, the value settable).
enum class access_restriction
{
    none,           // no coordinate held const by the discipline
    element_const,  // the element coordinate is held const (set / multiset)
    key_const       // the key held const, the value settable (map / multimap)
};

// access_restriction_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
access_restriction_name(access_restriction _a) noexcept
{
    return ( _a == access_restriction::none          ? "none"
           : _a == access_restriction::element_const ? "element_const"
           :                                           "key_const" );
}

// access_restriction_of
//   trait: the coordinate a type's discipline holds const. A keyed (pair)
// value type freezes the KEY (map / multimap); else a duplicate-equivalence
// (key_type) freezes the ELEMENT (set / multiset); else nothing is frozen (a
// sequence, or a non-container, which constrains no coordinate).
template<typename Type>
struct access_restriction_of
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr access_restriction value =
        (  internal::has_keyed_value_helper<clean_type>::value )
              ? access_restriction::key_const
      : (  internal::has_equivalence_helper<clean_type>::value )
              ? access_restriction::element_const
      :         access_restriction::none;

    using type = std::integral_constant<access_restriction, value>;
};

// access_restriction_of_t / access_restriction_of_v
//   type / value: the carrier and, where the language permits, the value
// companion of access_restriction_of.
template<typename Type>
using access_restriction_of_t = typename access_restriction_of<Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Type>
    inline constexpr access_restriction access_restriction_of_v =
        access_restriction_of<Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr access_restriction access_restriction_of_v =
        access_restriction_of<Type>::value;
#endif

// constrains_element
//   trait: the discipline holds the ELEMENT const - a set or multiset, whose
// element is the constrained coordinate.
template<typename Type>
struct constrains_element
    : std::integral_constant<bool,
          access_restriction_of<clean_t<Type>>::value
              == access_restriction::element_const>
{};

D_TYPE_TRAIT_VALUE_BOOL(constrains_element)

// constrains_key
//   trait: the discipline holds the KEY const while freeing the value - a map
// or multimap.
template<typename Type>
struct constrains_key
    : std::integral_constant<bool,
          access_restriction_of<clean_t<Type>>::value
              == access_restriction::key_const>
{};

D_TYPE_TRAIT_VALUE_BOOL(constrains_key)


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

template<typename Type>
struct mutable_container_class
{
private:
    using clean_type = clean_t<Type>;

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

    // capabilities and grade
    static constexpr bool       is_element_mutable =
        is_element_mutable_container<clean_type>::value;
    static constexpr bool       is_structurally_mutable =
        is_structurally_mutable_container<clean_type>::value;
    static constexpr bool       is_mutable =
        is_mutable_container<clean_type>::value;
    static constexpr bool       is_immutable =
        is_immutable_container<clean_type>::value;
    static constexpr mutability grade =
        mutability_of<clean_type>::value;
    static constexpr const char* grade_name =
        mutability_name(grade);

    // access dimension (the 2-D model's second coordinate)
    static constexpr access_restriction access =
        access_restriction_of<clean_type>::value;
    static constexpr const char* access_name =
        access_restriction_name(access);
    static constexpr bool constrains_element =
        djinterp::constrains_element<clean_type>::value;
    static constexpr bool constrains_key =
        djinterp::constrains_key<clean_type>::value;
};




// ===========================================================================
//  FOLDED IN FROM mutable_container_traits_revised.hpp  (now deleted)
// ===========================================================================
//   The "revised" file was a parallel rework of this one.  Its load-bearing
// API -- mutability, mutability_of, mutability_name, is_mutable_container,
// is_immutable_container, and the 2-D access model (access_restriction_of,
// constrains_element, constrains_key) -- is what the rest of the framework
// consumes (container_traits, the concept modules, the audit), so THIS file
// stays canonical and keeps all of it unchanged.
//
//   What the revised file added is folded in below as ADDITIONS, not
// replacements:
// - refined_mutability_grade: a four-grade refinement (fixed / value_mutable /
// structure_mutable / fully_mutable) with its _name and predicates -- a finer
// companion to `mutability` above (the name `mutability_grade` is already taken
// -- it aliases `mutability` as the grade DIMENSION of the 2-D model), which
// stays because existing callers name it. - settable_coordinate_of +
// coordinate_access_restriction: a coordinate-indexed view of the access
// dimension. Left as the documented stub it was, pending is_keyed_container /
// is_unique_container from container_overlay_traits.hpp. The revised file also
// re-declared has_size_signal locally; this file does NOT -- it takes
// has_size_signal from its single owner, bounded_container_traits.hpp, to avoid
// the duplicate-definition bug.


// III. The mutability grade (revised enum)
// ===========================================================================
//   Four grades in a chain of increasing freedom:
//     fixed < value_mutable < fully_mutable
//   (structure_mutable is orthogonal, degenerate, not shown here)

enum class refined_mutability_grade
{
    fixed,              // no change
    value_mutable,      // values at positions may change; structure fixed
    structure_mutable,  // structure (positions) may change; values const
    fully_mutable       // both kinds change permitted
};

constexpr const char*
refined_grade_name(refined_mutability_grade _g) noexcept
{
    using G = refined_mutability_grade;
    return ( _g == G::fixed              ? "fixed"
           : _g == G::value_mutable      ? "value_mutable"
           : _g == G::structure_mutable  ? "structure_mutable"
           :                               "fully_mutable" );
}

constexpr bool
is_fixed_grade(refined_mutability_grade _g) noexcept
{
    return ( _g == refined_mutability_grade::fixed );
}

constexpr bool
is_value_mutable_grade(refined_mutability_grade _g) noexcept
{
    return ( _g == refined_mutability_grade::value_mutable );
}

constexpr bool
is_structure_mutable_grade(refined_mutability_grade _g) noexcept
{
    return ( _g == refined_mutability_grade::structure_mutable );
}

constexpr bool
is_fully_mutable_grade(refined_mutability_grade _g) noexcept
{
    return ( _g == refined_mutability_grade::fully_mutable );
}

constexpr bool
grade_permits_element_mutation(refined_mutability_grade _g) noexcept
{
    return ( ( _g == refined_mutability_grade::value_mutable ) ||
             ( _g == refined_mutability_grade::fully_mutable ) );
}

constexpr bool
grade_permits_structural_mutation(refined_mutability_grade _g) noexcept
{
    return ( ( _g == refined_mutability_grade::structure_mutable ) ||
             ( _g == refined_mutability_grade::fully_mutable ) );
}


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
    using R = coordinate_access_restriction;
    return ( _r == R::none          ? "none"
           : _r == R::element_const ? "element_const"
           : _r == R::key_const     ? "key_const"
           :                          "both_const" );
}

// coordinate_settable
//   function: true iff a coordinate is NOT constrained and may be assigned.
constexpr bool
is_coordinate_settable(coordinate_access_restriction _r,
                       int                            _coordinate)
    noexcept
{
    // _coordinate: 0 = element (or key for keyed), 1 = value (for keyed)
    using R = coordinate_access_restriction;
    return ( ( _r == R::none )
          || ( _r == R::key_const && _coordinate == 1 ) );
}

// settable_coordinate_of
//   trait: the access restriction imposed by an overlay on a container. Keyed
// containers: key_const (keys are unique/identity). Sets / uniqueness-guarded:
// element_const (element uniqueness). Sequences, multisets: none
// (unrestricted).
template<typename Type, typename = void>
struct settable_coordinate_of
    : std::integral_constant<coordinate_access_restriction,
                             coordinate_access_restriction::none>
{};

// Specialization hook for keyed detection (requires overlay_of or
// is_keyed_container). NOTE: This is left as a stub; the real implementation
// depends on is_keyed_container and is_unique_container probes from
// container_overlay_traits.hpp.


// ===========================================================================


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TRAITS_MUTABLE_CONTAINER_TRAITS_HPP
