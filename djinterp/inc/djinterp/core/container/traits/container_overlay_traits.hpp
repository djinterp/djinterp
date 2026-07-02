/******************************************************************************
* djinterp [container]                             container_overlay_traits.hpp
*
*   SFINAE traits for the OVERLAY view of a container (the spec, Restrictions /
* Overlays).  An overlay is the synthesis the earlier axes were building toward:
* a finite SET of restrictions a container's contents satisfy, with the backing
* (vector, list, tree, array) deliberately forgotten.  The named containers are
* canonical overlays - set, multiset, map, multimap - and bounded or interval
* variants are mere conjunctions.
*
*   RESTRICTIONS AND SCOPE.  A restriction is a predicate on containers; its scope
* records what it inspects - STATIC (the value type alone), BAG-LEVEL (the value
* bag, blind to order and construction), or SEQUENCE-LEVEL (the ordered sequence).
* The vocabulary, drawn from the earlier axes:
*     multiplicity  mu_m^E  (bag-level)   #_E(c,x) <= m per class
*     capacity      gamma_k (bag-level)   |c| <= kappa
*     domain        delta_I (bag-level)   every value in <I>
*     sorted        sigma   (sequence-level)  ordered along positions
*     keyed         eta     (static)      the value type is a pair Key x Val
*
*   AN OVERLAY is order-blind when every restriction it bears is bag-level or
* static - it then sees a container only through its bag, so a list, a tree, an
* array, and a vector with the same contents wear it alike (conformance is a
* property of CONTENTS, not CONSTRUCTION).  The canonical overlays are all order-
* blind; a comparator merely supplies a sorted PRESENTATION without that order
* joining the overlay's identity.  Sorted (sigma) is therefore an opt-in addition
* here - NOT inferred from a container being associative - and is the proper
* concern of the forthcoming Sortedness axis, to which this hook defers.
*
*   COMPOSITION AND STRENGTH.  Overlays form a meet-semilattice: the meet unions
* restrictions (extensions intersect), the top is the empty overlay worn by
* everything, and one overlay is stronger than another (subsumes it) exactly when
* its extension is contained - it admits fewer containers.  overlay_meet and
* overlay_subsumes realise these; the latter is the strength order container
* comparison consumes.
*
*   This header SYNTHESISES the multiplicity and boundedness axes - it sources
* mu / gamma / delta from them - and adds the eta (keyed) and sigma (sorted)
* signals locally.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/traits/container_overlay_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_OVERLAY_TRAITS_
#define DJINTERP_CONTAINER_OVERLAY_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"              // D_TYPE_TRAIT_* detection macros, D_VOID_T
#include "../../meta/multiplicity.hpp"               // multiplicity_kind + ranks
#include "./container_multiplicity_traits.hpp"       // multiplicity_kind_of (mu, the E+m parameter)
#include "./bounded_container_traits.hpp"            // is_bounded_container (gamma), is_domain_bounded_container (delta)


NS_DJINTERP


// ===========================================================================
// I.   Restriction vocabulary
// ===========================================================================

// restriction_scope
//   enum: what part of a container a restriction inspects.
enum class restriction_scope
{
    static_scope,    // the value type alone (checkable from the type)
    bag_level,       // the value bag (order- and construction-blind)
    sequence_level   // the ordered sequence
};

// restriction_kind
//   enum: the basic restrictions furnished by the earlier axes.
enum class restriction_kind
{
    multiplicity,    // mu_m^E
    capacity,        // gamma_kappa
    domain,          // delta_I
    sorted,          // sigma
    keyed            // eta_{Key,Val}
};

// restriction_scope_name / restriction_kind_name
constexpr const char*
restriction_scope_name(restriction_scope _s) noexcept
{
    return ( _s == restriction_scope::static_scope   ? "static"
           : _s == restriction_scope::bag_level      ? "bag_level"
           :                                           "sequence_level" );
}

constexpr const char*
restriction_kind_name(restriction_kind _k) noexcept
{
    return ( _k == restriction_kind::multiplicity ? "multiplicity"
           : _k == restriction_kind::capacity     ? "capacity"
           : _k == restriction_kind::domain       ? "domain"
           : _k == restriction_kind::sorted       ? "sorted"
           :                                        "keyed" );
}

// scope_of_restriction
//   function: the scope each restriction kind carries.  Only sorted is sequence-
// level (the rest are bag-level or static), which is what makes an overlay
// without it order-blind.
constexpr restriction_scope
scope_of_restriction(restriction_kind _k) noexcept
{
    return ( _k == restriction_kind::keyed  ? restriction_scope::static_scope
           : _k == restriction_kind::sorted ? restriction_scope::sequence_level
           :                                  restriction_scope::bag_level );
}


// ===========================================================================
// II.  Overlay-specific signals (eta, sigma)
// ===========================================================================

// is_keyed_container
//   trait: the keyed restriction eta (static) - the value type is a pair
// Key x Val, detected by its first_type / second_type aliases.  A keyed value
// type re-bases the duplicate-equivalence onto the key, but eta itself asserts
// only the pair SHAPE and so is independent of uniqueness (a sequence of pairs is
// keyed yet wears no mu_1).
D_TYPE_TRAIT_TRUE(is_keyed_container,
    typename clean_t<_Type>::value_type::first_type,
    typename clean_t<_Type>::value_type::second_type)


NS_INTERNAL

    // sorted_invariant_helper
    //   helper: the sorted restriction sigma (sequence-level) is opt-in - a
    // container asserts sorted order as part of its IDENTITY via a static
    // `sorted_invariant` constant.  It is NOT inferred from associativity, whose
    // sorting is mere presentation.  The Sortedness axis will own this; until
    // then the hook reads the opt-in and otherwise reports false (order-blind).
    template<typename _Type,
             typename = void>
    struct sorted_invariant_helper
    {
        static constexpr bool value = false;
    };

    template<typename _Type>
    struct sorted_invariant_helper<_Type,
        D_VOID_T<decltype(clean_t<_Type>::sorted_invariant)>>
    {
        static constexpr bool value =
            static_cast<bool>(clean_t<_Type>::sorted_invariant);
    };

NS_END  // internal


// ===========================================================================
// III. The overlay descriptor + algebra
// ===========================================================================

// overlay
//   value type: the restrictions a container's contents bear.  mu is carried as
// its multiplicity_kind (the E + m parameterisation - sequence is the vacuous
// cap); the remaining restrictions are present/absent flags.  A literal type, so
// it composes and compares at compile time.
struct overlay
{
    multiplicity_kind mult;             // mu  (bag-level): the E + m parameter
    bool              keyed;            // eta (static)
    bool              sorted;           // sigma (sequence-level)
    bool              capacity_bounded; // gamma (bag-level)
    bool              domain_bounded;   // delta (bag-level)
};

// overlay_top
//   function: the empty overlay (top) - no restrictions, worn by everything.
constexpr overlay
overlay_top() noexcept
{
    return overlay{ multiplicity_kind::sequence, false, false, false, false };
}

// is_order_blind
//   function: true iff the overlay bears no sequence-level restriction - i.e. it
// is not sorted, so it observes a container only through its bag.
constexpr bool
is_order_blind(overlay _o) noexcept
{
    return !_o.sorted;
}

// overlay_subsumes
//   function: the strength order - true iff _a <= _b, i.e. [_a] is contained in
// [_b] (_a is the MORE restrictive, admitting fewer containers).  Component-wise:
// _a's multiplicity is at least as tight (smaller rank), and _a bears every
// flag-restriction _b requires.
constexpr bool
overlay_subsumes(overlay _a, overlay _b) noexcept
{
    return
        ( ( multiplicity_kind_rank(_a.mult)
                <= multiplicity_kind_rank(_b.mult) )     &&
          ( !_b.keyed            || _a.keyed )            &&
          ( !_b.sorted           || _a.sorted )           &&
          ( !_b.capacity_bounded || _a.capacity_bounded ) &&
          ( !_b.domain_bounded   || _a.domain_bounded ) );
}

// overlay_meet
//   function: the composition (meet) - the union of two overlays' restrictions,
// the stronger combined overlay.  Multiplicity takes the tighter (smaller-rank)
// kind; the flag-restrictions take the disjunction.
constexpr overlay
overlay_meet(overlay _a, overlay _b) noexcept
{
    return overlay{
        (   multiplicity_kind_rank(_a.mult) <= multiplicity_kind_rank(_b.mult)
                ? _a.mult : _b.mult ),
        _a.keyed            || _b.keyed,
        _a.sorted           || _b.sorted,
        _a.capacity_bounded || _b.capacity_bounded,
        _a.domain_bounded   || _b.domain_bounded
    };
}


// ===========================================================================
// IV.  The overlay a container wears
// ===========================================================================

// overlay_of
//   trait: assembles the overlay a type wears from the per-axis verdicts -
// multiplicity from the multiplicity axis, capacity and domain from boundedness,
// keyed and sorted from the local signals.  Each restriction is exposed as a
// static member, and value() materialises the descriptor for the algebra.
template<typename _Type>
struct overlay_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr multiplicity_kind mult =
        multiplicity_kind_of<clean_type>::value;
    static constexpr bool keyed =
        is_keyed_container<clean_type>::value;
    static constexpr bool sorted =
        internal::sorted_invariant_helper<clean_type>::value;
    static constexpr bool capacity_bounded =
        is_bounded_container<clean_type>::value;
    static constexpr bool domain_bounded =
        is_domain_bounded_container<clean_type>::value;

    // value
    //   the assembled descriptor (a function, to sidestep an out-of-line
    // definition for a static member of class type pre-C++17).
    static constexpr overlay value() noexcept
    {
        return overlay{ mult, keyed, sorted, capacity_bounded, domain_bounded };
    }
};

// is_order_blind_overlay
//   trait: true iff the overlay a type wears is order-blind (bears no sigma).
template<typename _Type>
struct is_order_blind_overlay
    : std::integral_constant<bool, !overlay_of<clean_t<_Type>>::sorted>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_order_blind_overlay)

// wears_overlay
//   function: true iff a type's overlay is at least as strong as a target - i.e.
// the type conforms to (wears) the target overlay.  A function template - the
// target is a constexpr argument, not a class-type non-type parameter, which the
// C++11 baseline does not admit - constexpr-evaluable at usage sites.
template<typename _Type>
constexpr bool
wears_overlay(overlay _target) noexcept
{
    return overlay_subsumes(
               overlay_of<clean_t<_Type>>::value(), _target );
}


// ===========================================================================
// V.   Canonical overlays
// ===========================================================================

// canonical_overlay
//   enum: the named overlays - the 2x2 of keyed? against may-repeat?  A type that
// wears none of them (a plain sequence, or a non-container) is `none`.
enum class canonical_overlay
{
    none,        // wears the trivial overlay (a sequence) / not a container
    set,         // {mu_1^E}                 unkeyed, unique
    multiset,    // {mu_m^E}, m>1            unkeyed, repeatable
    map,         // {eta, mu_1^E_key}        keyed, unique
    multimap     // {eta, mu_m^E_key}, m>1   keyed, repeatable
};

// canonical_overlay_name
constexpr const char*
canonical_overlay_name(canonical_overlay _c) noexcept
{
    return ( _c == canonical_overlay::none     ? "none"
           : _c == canonical_overlay::set      ? "set"
           : _c == canonical_overlay::multiset ? "multiset"
           : _c == canonical_overlay::map      ? "map"
           :                                     "multimap" );
}

// classify_canonical
//   function: the named overlay from the two distinguishing axes - the
// multiplicity (unique vs repeatable) and whether the value type is keyed.  A
// sequence (the vacuous cap, no equivalence) or an unknown wears none.
constexpr canonical_overlay
classify_canonical(multiplicity_kind _m, bool _keyed) noexcept
{
    return ( _m == multiplicity_kind::unique
                 ? ( _keyed ? canonical_overlay::map : canonical_overlay::set )
           : is_multiset_kind(_m)
                 ? ( _keyed ? canonical_overlay::multimap : canonical_overlay::multiset )
           :       canonical_overlay::none );
}

// canonical_overlay_of
//   trait: the named overlay a type wears (its multiplicity-canonical base;
// bounded / domain variants are recorded separately in overlay_of).
template<typename _Type>
struct canonical_overlay_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr canonical_overlay value =
        classify_canonical(
            multiplicity_kind_of<clean_type>::value,
            is_keyed_container<clean_type>::value );

    using type = std::integral_constant<canonical_overlay, value>;
};

template<typename _Type>
using canonical_overlay_of_t = typename canonical_overlay_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr canonical_overlay canonical_overlay_of_v =
        canonical_overlay_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr canonical_overlay canonical_overlay_of_v =
        canonical_overlay_of<_Type>::value;
#endif


// ===========================================================================
// VI.  Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct overlay_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // borne restrictions
    static constexpr multiplicity_kind mult =
        overlay_of<clean_type>::mult;
    static constexpr bool keyed =
        overlay_of<clean_type>::keyed;
    static constexpr bool sorted =
        overlay_of<clean_type>::sorted;
    static constexpr bool capacity_bounded =
        overlay_of<clean_type>::capacity_bounded;
    static constexpr bool domain_bounded =
        overlay_of<clean_type>::domain_bounded;

    // derived
    static constexpr bool order_blind =
        is_order_blind_overlay<clean_type>::value;
    static constexpr canonical_overlay canonical =
        canonical_overlay_of<clean_type>::value;
    static constexpr const char* canonical_name =
        canonical_overlay_name(canonical);
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_OVERLAY_TRAITS_
