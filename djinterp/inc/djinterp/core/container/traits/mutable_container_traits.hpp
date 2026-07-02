/******************************************************************************
* djinterp [container]                            mutable_container_traits.hpp
*
*   SFINAE structural traits for mutability classification - WHAT mutation a
* container's public interface admits.  Two independent capabilities are
* distinguished, then summarised:
*
*     ELEMENT mutation     overwriting an EXISTING element in place, through a
*                          non-const operator[] or a non-const data() - the set
*                          of elements is unchanged, only their values;
*     STRUCTURAL mutation  changing the SET of elements - push / pop / insert /
*                          erase / clear / resize.
*
*   The two are genuinely orthogonal.  A fixed inline array overwrites elements
* but cannot grow (element-only); an ordered set inserts and erases but exposes
* no writable element handle (structural-only); a vector does both.  The four
* corners are named by the `mutability` grade:
*
*     immutable           neither - only const observation;
*     element_mutable     element mutation, no structural;
*     structural_mutable  structural mutation, no element;
*     fully_mutable       both.
*
*   "Mutable" (the umbrella) is element OR structural; "immutable" is a type
* that LOOKS like a container (it has a size()) yet exposes neither.
*
*   AXIS ORTHOGONALITY:
*   Mutability is independent of the lifetime, storage, and memory-discipline
* axes - no entailment is drawn here.  A literal type may be mutated within a
* constexpr context (so compile-time lifetime does NOT imply immutability), and a
* bounded inline buffer may be grown structurally within its capacity (so
* structural mutation does NOT imply dynamic storage).  Read-only / write-only /
* read-write ACCESS is a separate, orthogonal concern - a capability OVERLAY
* imposed by the container access wrappers, not an intrinsic grade - and lives
* with those wrappers, not in this file.
*
*   PORTABILITY:
*   C++11 baseline; the `_v` companions the D_TYPE_TRAIT_* engine emits degrade
* with the language exactly as the rest of the trait family's do.
*
*
* path:      /inc/djinterp/core/container/traits/mutable_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
*                                                          revised: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_MUTABLE_CONTAINER_TRAITS_
#define DJINTERP_MUTABLE_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"            // clean_t, NS_*, D_ENV_* feature macros
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_* detection macros


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
    decltype(std::declval<clean_t<_Type>&>().push_back(
        std::declval<typename clean_t<_Type>::value_type>())))

// has_push_front_signal
D_TYPE_TRAIT_TRUE(has_push_front_signal,
    decltype(std::declval<clean_t<_Type>&>().push_front(
        std::declval<typename clean_t<_Type>::value_type>())))

// has_insert_signal
D_TYPE_TRAIT_TRUE(has_insert_signal,
    decltype(std::declval<clean_t<_Type>&>().insert(
        std::declval<typename clean_t<_Type>::value_type>())))

// has_erase_signal
D_TYPE_TRAIT_TRUE(has_erase_signal,
    decltype(std::declval<clean_t<_Type>&>().erase(
        std::declval<clean_t<_Type>&>().begin())))

// has_clear_signal
D_TYPE_TRAIT_TRUE(has_clear_signal,
    decltype(std::declval<clean_t<_Type>&>().clear()))

// has_resize_signal
D_TYPE_TRAIT_TRUE(has_resize_signal,
    decltype(std::declval<clean_t<_Type>&>().resize(std::size_t{})))

// --- element mutators (overwrite an EXISTING element in place) ---

// has_mutable_subscript_signal
//   trait: operator[] yields a non-const lvalue (the subscript is assignable).
D_TYPE_TRAIT_TRUE(has_mutable_subscript_signal,
    decltype(std::declval<clean_t<_Type>&>()[std::size_t{}] =
             std::declval<typename clean_t<_Type>::value_type>()))

// has_mutable_data_signal
//   trait: data() yields a non-const pointer (the pointee is assignable).
D_TYPE_TRAIT_TRUE(has_mutable_data_signal,
    decltype(*std::declval<clean_t<_Type>&>().data() =
             std::declval<typename clean_t<_Type>::value_type>()))

// --- const observation (the "looks like a container" marker) ---

// has_size_signal
//   trait: a const-callable size() - the minimal evidence of container-hood,
// used to tell an immutable container from a merely mutator-less type.
D_TYPE_TRAIT_TRUE(has_size_signal,
    decltype(std::declval<const clean_t<_Type>&>().size()))


// ===========================================================================
// II.  Capability umbrellas
// ===========================================================================

// is_element_mutable_container
//   trait: true iff an existing element can be overwritten in place.
template<typename _Type>
struct is_element_mutable_container
    : std::integral_constant<bool,
          (    has_mutable_subscript_signal<clean_t<_Type>>::value
            || has_mutable_data_signal<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_element_mutable_container)

// is_structurally_mutable_container
//   trait: true iff the set of elements can be changed.
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

// is_mutable_container
//   trait: true iff ANY mutator is present - element OR structural.
template<typename _Type>
struct is_mutable_container
    : std::integral_constant<bool,
          (    is_element_mutable_container<clean_t<_Type>>::value
            || is_structurally_mutable_container<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_mutable_container)

// is_immutable_container
//   trait: true iff the type LOOKS like a container (it has a size()) but
// exposes NONE of the mutation signals.
template<typename _Type>
struct is_immutable_container
    : std::integral_constant<bool,
          (    has_size_signal<clean_t<_Type>>::value
            && !is_mutable_container<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_immutable_container)


// ===========================================================================
// III. The mutability grade
// ===========================================================================

// mutability
//   enum: the summary of the two independent capabilities - the four corners of
// the (element, structural) square.
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
//   function: true iff the grade permits overwriting elements (element OR full).
constexpr bool
grade_admits_element_mutation(mutability _m) noexcept
{
    return ( ( _m == mutability::element_mutable ) ||
             ( _m == mutability::fully_mutable ) );
}

// grade_admits_structural_mutation
//   function: true iff the grade permits structural change (structural OR full).
constexpr bool
grade_admits_structural_mutation(mutability _m) noexcept
{
    return ( ( _m == mutability::structural_mutable ) ||
             ( _m == mutability::fully_mutable ) );
}

// mutability_of
//   trait: the mutability grade of a container, from its two capability bits.
// (A type with no mutators reads as immutable whether or not it is a container;
//  pair with is_immutable_container when container-hood must be confirmed.)
template<typename _Type>
struct mutability_of
{
private:
    using clean_type = clean_t<_Type>;
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
template<typename _Type>
using mutability_of_t = typename mutability_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr mutability mutability_of_v = mutability_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr mutability mutability_of_v = mutability_of<_Type>::value;
#endif


// ===========================================================================
// IV.  Aggregate snapshot
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
};


NS_END  // djinterp


#endif  // DJINTERP_MUTABLE_CONTAINER_TRAITS_
