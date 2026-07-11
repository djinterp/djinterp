/******************************************************************************
* djinterp [container]                                                 set.hpp
*
*   The djinterp SET family, in one header: the set OVERLAY traits, their C++20
* concept faces, and the four concrete set containers.  A set is the canonical
* overlay {mu_1^E} (the spec, Overlays): an UNKEYED, UNIQUE discipline worn by an
* otherwise arbitrary backing.  Its multiset sibling relaxes the uniqueness bound
* to {mu_m^E}, m > 1.  Neither names its backing; conformance is a property of a
* container's CONTENTS, not its construction.
*
*   THIS HEADER MERGES what were formerly set_traits.hpp, set_concepts.hpp, and
* set.hpp, and re-bases them on the current container subframework.  Two changes
* are load-bearing and worth stating up front:
*
*   1. NAMESPACE.  The subframework is flat: axes, overlays, and concrete
*      containers all live directly in `djinterp` (no `container` / `traits`
*      sub-namespaces).  Everything here follows suit.
*
*   2. THE ORDER-AXIS RENAME.  In the current framework `is_ordered_container`
*      means POSITIONAL identity - a container with a position function - and a
*      set, keyed and permutation-invariant, is therefore UNORDERED (its identity
*      is its bag).  A comparator-equipped set is MONOTONE: it has no positions,
*      but enumerates in comparator order "by construction" (sorted_container_
*      traits.hpp).  The old set traits used "ordered"/"unordered" for the
*      comparator/hash split, which now collides with the axis.  They are renamed:
*          old is_ordered_set    (std::set, comparator)      -> is_monotone_set
*          old is_unordered_set  (std::unordered_set, hash)  -> is_hashed_set
*      The set layer DEFERS the shared verdicts (multiplicity, sortedness, overlay,
*      mutability) to the subframework rather than re-deriving them, so a set is
*      classified consistently with every other container.
*
*   THE FOUR CONTAINERS delegate to the corresponding standard container while
* exposing the structural surface the classifier reads (key_type / value_type, a
* comparator or hasher, a uniqueness-revealing insert, a const traversal, size).
* They carry `structure_category = flat` (a set is flat - depth 1 - whatever its
* element type), and their element access is const by construction (std::set's
* iterator dereferences to a const element), so the framework reads them as
* structure-mutable with an element-const access restriction, exactly the spec's
* Mutability row for a set.
*
*   PORTABILITY:
*   The TRAITS are a C++11 baseline: each `_v` companion is emitted through the
* trait_detect macros (inline variable on C++17+, variable template on C++14,
* absent on C++11).  The CONCEPTS are C++20-only (Part II self-suppresses below
* it).  The CONTAINERS track their underlying std counterparts - std::set /
* std::unordered_set are C++11, but the node-handle surface they re-expose
* (node_type, extract, merge) is C++17, so the containers require C++17 in full;
* the has_set_extract / has_set_merge traits report this correctly by SFINAE.
*
*
* path:      /inc/djinterp/core/container/set/set.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.06
*                                                          revised: 2026.07.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
PART I  -- TRAITS
    I.    core structural detection          (is_set_like)
    II.   backing / enumeration axis         (monotone vs hashed)
    III.  multiplicity axis                  (unique vs multi)
    IV.   flat-set backing detection
    V.    compound classifications
    VI.   overlay bridge                     (canonical_overlay)
    VII.  set-specific method detection
    VIII. strategy enums
    IX.   set_class aggregate snapshot
    X.    type extractors

PART II -- CONCEPTS (C++20)

PART III -- CONTAINERS
    set / multiset / unordered_set / unordered_multiset
*/

#ifndef DJINTERP_CONTAINER_SET_
#define DJINTERP_CONTAINER_SET_ 1

#ifndef __cplusplus
    #error "set.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <utility>
// djinterp
#include "../../djinterp.hpp"              // clean_t, NS_*, D_ENV_* feature macros
#include "../../meta/trait_detect.hpp"     // D_TYPE_TRAIT_* detection macros, D_VOID_T
#include "../../meta/member_types.hpp"     // has_key_type / has_value_type / has_mapped_type
#include "../../meta/multiplicity.hpp"     // multiplicity_kind vocabulary
#include "../traits/container_traits.hpp"  // container_class + the ordered / sorted /
                                           //   multiplicity / overlay / mutable / iterable axes
#include "../structure/flat.hpp"           // flat tag (structure_category opt-in)


NS_DJINTERP


// ###########################################################################
// ##  PART I  --  TRAITS                                                    ##
// ###########################################################################


// ===========================================================================
// I.   Core structural detection
// ===========================================================================
//   A type is SET-LIKE when it presents the associative surface of a set: it
// carries a key_type AND a value_type, is iterable, and does NOT expose a
// mapped_type (which would make it map-like - keyed, its elements not the keys).
// This is the structural gate every set trait below stands on; the richer
// verdicts (which multiplicity, which enumeration, which overlay) are deferred to
// the subframework, keyed off this gate.

// is_set_like
//   trait: true iff _Type is a set-like associative container - key_type and
// value_type present, a const begin()/end() traversal, and no mapped_type.
template<typename _Type,
         typename = void>
struct is_set_like : std::false_type
{};

template<typename _Type>
struct is_set_like<_Type, D_VOID_T<
    typename clean_t<_Type>::key_type,
    typename clean_t<_Type>::value_type,
    decltype(std::declval<const clean_t<_Type>&>().begin()),
    decltype(std::declval<const clean_t<_Type>&>().end())
>> : std::integral_constant<bool,
         !has_mapped_type<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_set_like)


// ===========================================================================
// II.  Backing / enumeration axis  (monotone vs hashed)
// ===========================================================================
//   A set's backing is deliberately forgotten by the overlay, but a set library
// must still tell the comparator-ordered form from the hash-ordered one, because
// only the former enumerates in comparator order and offers range lookups.  The
// distinction rides the subframework's SORTEDNESS axis, not the (positional)
// order axis: a comparator-equipped set is MONOTONE (sorted-by-construction
// enumeration); a hash-based set is unordered with no such guarantee.
//
//   RENAME NOTE: is_monotone_set was is_ordered_set; is_hashed_set was
// is_unordered_set.  See the file header for why.

// is_monotone_set
//   trait: true iff _Type is a set-like container whose enumeration is monotone -
// it is comparator-equipped (a key_compare, no hasher), so it enumerates in
// comparator order by construction.  The std::set / std::multiset / flat_set
// family, and any structural equivalent maintaining sorted order.
template<typename _Type>
struct is_monotone_set
    : std::integral_constant<bool,
            is_set_like<clean_t<_Type>>::value
         && is_monotone_container<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_monotone_set)

// is_hashed_set
//   trait: true iff _Type is a set-like container backed by hashing - it exposes
// a hasher alias and so enumerates in no comparator order.  The
// std::unordered_set / std::unordered_multiset family, or any hash-based
// equivalent.
template<typename _Type>
struct is_hashed_set
    : std::integral_constant<bool,
            is_set_like<clean_t<_Type>>::value
         && has_hasher_type<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_hashed_set)


// ===========================================================================
// III. Multiplicity axis  (unique vs multi)
// ===========================================================================
//   Whether a set caps each value-class at one occurrence (mu_1, set semantics)
// or admits repeats (mu_m, m > 1, multiset semantics).  The subframework decides
// this structurally - a unique associative's single-element insert returns a
// pair<iterator,bool>, a multiset's a plain iterator - so the set layer reads the
// verdict off multiplicity_kind_of rather than re-probing.

// is_unique_set
//   trait: true iff _Type is a set-like container enforcing unique keys (mu = 1).
template<typename _Type>
struct is_unique_set
    : std::integral_constant<bool,
            is_set_like<clean_t<_Type>>::value
         && is_unique_container<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_unique_set)

// is_multi_set
//   trait: true iff _Type is a set-like container admitting duplicate keys
// (mu > 1).
template<typename _Type>
struct is_multi_set
    : std::integral_constant<bool,
            is_set_like<clean_t<_Type>>::value
         && is_multiset_container<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_multi_set)


// ===========================================================================
// IV.  Flat-set backing detection
// ===========================================================================

// is_flat_set
//   trait: true iff _Type is a monotone set backed by another container - the
// flat_set / flat_multiset adaptor pattern (a sorted sequence viewed as a set).
// Detected via the framework's underlying-container signal: a flat set adaptor
// exposes `underlying_container_type`, the canonical mark of a container that
// delegates its storage rather than owning it.
template<typename _Type>
struct is_flat_set
    : std::integral_constant<bool,
            is_monotone_set<clean_t<_Type>>::value
         && is_underlying_container<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_flat_set)


// ===========================================================================
// V.   Compound classifications
// ===========================================================================
//   The backing axis crossed with the multiplicity axis: the six concrete
// set shapes the standard library and the flat adaptors realise.

// is_monotone_unique_set
//   trait: sorted + unique (std::set, and structural equivalents).
template<typename _Type>
struct is_monotone_unique_set
    : std::integral_constant<bool,
            is_monotone_set<clean_t<_Type>>::value
         && is_unique_set<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_monotone_unique_set)

// is_monotone_multi_set
//   trait: sorted + duplicates (std::multiset).
template<typename _Type>
struct is_monotone_multi_set
    : std::integral_constant<bool,
            is_monotone_set<clean_t<_Type>>::value
         && is_multi_set<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_monotone_multi_set)

// is_hashed_unique_set
//   trait: hashed + unique (std::unordered_set).
template<typename _Type>
struct is_hashed_unique_set
    : std::integral_constant<bool,
            is_hashed_set<clean_t<_Type>>::value
         && is_unique_set<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_hashed_unique_set)

// is_hashed_multi_set
//   trait: hashed + duplicates (std::unordered_multiset).
template<typename _Type>
struct is_hashed_multi_set
    : std::integral_constant<bool,
            is_hashed_set<clean_t<_Type>>::value
         && is_multi_set<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_hashed_multi_set)

// is_flat_unique_set
//   trait: flat backed + sorted + unique (flat_set).
template<typename _Type>
struct is_flat_unique_set
    : std::integral_constant<bool,
            is_flat_set<clean_t<_Type>>::value
         && is_unique_set<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_flat_unique_set)

// is_flat_multi_set
//   trait: flat backed + sorted + duplicates (flat_multiset).
template<typename _Type>
struct is_flat_multi_set
    : std::integral_constant<bool,
            is_flat_set<clean_t<_Type>>::value
         && is_multi_set<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_flat_multi_set)


// ===========================================================================
// VI.  Overlay bridge  (canonical_overlay)
// ===========================================================================
//   A set IS a canonical overlay; these traits tie the set-local classification
// to the subframework's overlay verdict (container_overlay_traits.hpp), so a
// caller reasoning in overlay terms and one reasoning in set terms agree.  The
// bridge keys on canonical_overlay_of, which classifies by (multiplicity, keyed):
// an unkeyed unique container wears `set`, an unkeyed repeatable one `multiset`.

// wears_set_overlay
//   trait: true iff _Type wears the canonical set overlay {mu_1^E} - unkeyed,
// unique.  (The subframework's own name is canonical_overlay::set.)
template<typename _Type>
struct wears_set_overlay
    : std::integral_constant<bool,
          canonical_overlay_of<clean_t<_Type>>::value
              == canonical_overlay::set>
{};

D_TYPE_TRAIT_VALUE_BOOL(wears_set_overlay)

// wears_multiset_overlay
//   trait: true iff _Type wears the canonical multiset overlay {mu_m^E}, m > 1 -
// unkeyed, repeatable.
template<typename _Type>
struct wears_multiset_overlay
    : std::integral_constant<bool,
          canonical_overlay_of<clean_t<_Type>>::value
              == canonical_overlay::multiset>
{};

D_TYPE_TRAIT_VALUE_BOOL(wears_multiset_overlay)


// ===========================================================================
// VII. Set-specific method detection
// ===========================================================================
//   The associative operations a set may expose.  These remain set-local (they
// are the set's own lookup / insert / erase surface); the strategy enums of
// section VIII pick the strongest available at compile time.  Probes strip cv-ref
// via clean_t, matching the rest of the trait family.

// has_set_find
//   trait: true iff _Type has find(key_type).
D_TYPE_TRAIT_TRUE(has_set_find,
    decltype(std::declval<const clean_t<_Type>&>().find(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_count
//   trait: true iff _Type has count(key_type).
D_TYPE_TRAIT_TRUE(has_set_count,
    decltype(std::declval<const clean_t<_Type>&>().count(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_contains
//   trait: true iff _Type has contains(key_type) (C++20).
D_TYPE_TRAIT_TRUE(has_set_contains,
    decltype(std::declval<const clean_t<_Type>&>().contains(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_equal_range
//   trait: true iff _Type has equal_range(key_type).
D_TYPE_TRAIT_TRUE(has_set_equal_range,
    decltype(std::declval<const clean_t<_Type>&>().equal_range(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_lower_bound
//   trait: true iff _Type has lower_bound(key_type) (monotone sets only).
D_TYPE_TRAIT_TRUE(has_set_lower_bound,
    decltype(std::declval<const clean_t<_Type>&>().lower_bound(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_upper_bound
//   trait: true iff _Type has upper_bound(key_type) (monotone sets only).
D_TYPE_TRAIT_TRUE(has_set_upper_bound,
    decltype(std::declval<const clean_t<_Type>&>().upper_bound(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_insert
//   trait: true iff _Type has insert(value_type).
D_TYPE_TRAIT_TRUE(has_set_insert,
    decltype(std::declval<clean_t<_Type>&>().insert(
        std::declval<typename clean_t<_Type>::value_type>())))

// has_set_emplace
//   trait: true iff _Type has emplace(...).
D_TYPE_TRAIT_TRUE(has_set_emplace,
    decltype(std::declval<clean_t<_Type>&>().emplace(
        std::declval<typename clean_t<_Type>::value_type>())))

// has_set_erase_key
//   trait: true iff _Type has erase(key_type) (key-based removal).
D_TYPE_TRAIT_TRUE(has_set_erase_key,
    decltype(std::declval<clean_t<_Type>&>().erase(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_erase_iterator
//   trait: true iff _Type has erase(const_iterator) - the fallback removal path
// after a lookup, when no key-based erase is offered.
D_TYPE_TRAIT_TRUE(has_set_erase_iterator,
    decltype(std::declval<clean_t<_Type>&>().erase(
        std::declval<typename clean_t<_Type>::const_iterator>())))

// has_set_extract
//   trait: true iff _Type has extract(key_type) (node extraction, C++17).
D_TYPE_TRAIT_TRUE(has_set_extract,
    decltype(std::declval<clean_t<_Type>&>().extract(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_merge
//   trait: true iff _Type has merge(other) (node splice, C++17).
D_TYPE_TRAIT_TRUE(has_set_merge,
    decltype(std::declval<clean_t<_Type>&>().merge(
        std::declval<clean_t<_Type>&>())))


// ===========================================================================
// VIII. Strategy enums
// ===========================================================================
//   The dispatch a holder should use for the three set operations, each selected
// top-to-bottom (first match wins) by set_class in section IX.

// set_lookup_strategy
//   enum: how to test membership.
enum class set_lookup_strategy
{
    contains,      // contains() - the direct membership test (C++20)
    find,          // find() != end()
    count,         // count() > 0
    linear,        // linear scan via iterators (fallback)
    unsupported    // no lookup capability detected
};

// set_insert_strategy
//   enum: how to add an element.
enum class set_insert_strategy
{
    emplace,       // emplace() - in-place construction
    insert,        // insert(value_type)
    unsupported    // no insertion capability detected
};

// set_erase_strategy
//   enum: how to remove an element.
enum class set_erase_strategy
{
    erase_key,       // erase(key_type) - direct key removal
    erase_iterator,  // erase(iterator) after a lookup
    unsupported      // no erasure capability detected
};

// set_lookup_strategy_name / set_insert_strategy_name / set_erase_strategy_name
//   functions: stable spellings, for diagnostics and agent-facing summaries.
constexpr const char*
set_lookup_strategy_name(set_lookup_strategy _s) noexcept
{
    return ( _s == set_lookup_strategy::contains ? "contains"
           : _s == set_lookup_strategy::find     ? "find"
           : _s == set_lookup_strategy::count    ? "count"
           : _s == set_lookup_strategy::linear   ? "linear"
           :                                       "unsupported" );
}

constexpr const char*
set_insert_strategy_name(set_insert_strategy _s) noexcept
{
    return ( _s == set_insert_strategy::emplace ? "emplace"
           : _s == set_insert_strategy::insert  ? "insert"
           :                                      "unsupported" );
}

constexpr const char*
set_erase_strategy_name(set_erase_strategy _s) noexcept
{
    return ( _s == set_erase_strategy::erase_key      ? "erase_key"
           : _s == set_erase_strategy::erase_iterator ? "erase_iterator"
           :                                            "unsupported" );
}


// ===========================================================================
// IX.  set_class aggregate snapshot
// ===========================================================================
//   The full set-side classification of a type in one place - query this instead
// of the individual traits when the whole picture is wanted.  The final block
// cross-references the shared subframework verdicts, so a set's location on the
// common axes (multiplicity, sortedness, overlay, mutability, iteration) sits
// beside its set-local classification.

template<typename _Type>
struct set_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // --- core classification ---
    static constexpr bool is_set               = is_set_like<clean_type>::value;
    static constexpr bool is_monotone          = is_monotone_set<clean_type>::value;
    static constexpr bool is_hashed            = is_hashed_set<clean_type>::value;
    static constexpr bool is_unique            = is_unique_set<clean_type>::value;
    static constexpr bool is_multi             = is_multi_set<clean_type>::value;
    static constexpr bool is_flat              = is_flat_set<clean_type>::value;

    // --- compound classifications ---
    static constexpr bool is_monotone_unique   = is_monotone_unique_set<clean_type>::value;
    static constexpr bool is_monotone_multi    = is_monotone_multi_set<clean_type>::value;
    static constexpr bool is_hashed_unique     = is_hashed_unique_set<clean_type>::value;
    static constexpr bool is_hashed_multi      = is_hashed_multi_set<clean_type>::value;
    static constexpr bool is_flat_unique       = is_flat_unique_set<clean_type>::value;
    static constexpr bool is_flat_multi        = is_flat_multi_set<clean_type>::value;

    // --- method availability ---
    static constexpr bool has_find             = has_set_find<clean_type>::value;
    static constexpr bool has_count            = has_set_count<clean_type>::value;
    static constexpr bool has_contains         = has_set_contains<clean_type>::value;
    static constexpr bool has_equal_range      = has_set_equal_range<clean_type>::value;
    static constexpr bool has_lower_bound      = has_set_lower_bound<clean_type>::value;
    static constexpr bool has_upper_bound      = has_set_upper_bound<clean_type>::value;
    static constexpr bool has_insert           = has_set_insert<clean_type>::value;
    static constexpr bool has_emplace          = has_set_emplace<clean_type>::value;
    static constexpr bool has_erase_key        = has_set_erase_key<clean_type>::value;
    static constexpr bool has_erase_iterator   = has_set_erase_iterator<clean_type>::value;
    static constexpr bool has_extract          = has_set_extract<clean_type>::value;
    static constexpr bool has_merge            = has_set_merge<clean_type>::value;

    // --- derived lookup support ---
    static constexpr bool has_range_lookup =
        ( has_lower_bound &&
          has_upper_bound );

    static constexpr bool has_full_lookup =
        ( has_find    &&
          has_count   &&
          has_equal_range );

    // --- strategies (first match wins) ---
    static constexpr set_lookup_strategy lookup_strategy =
        ( has_contains ? set_lookup_strategy::contains :
          has_find     ? set_lookup_strategy::find     :
          has_count    ? set_lookup_strategy::count    :
          is_set       ? set_lookup_strategy::linear   :
                         set_lookup_strategy::unsupported );

    static constexpr set_insert_strategy insert_strategy =
        ( has_emplace ? set_insert_strategy::emplace :
          has_insert  ? set_insert_strategy::insert  :
                        set_insert_strategy::unsupported );

    static constexpr set_erase_strategy erase_strategy =
        ( has_erase_key                     ? set_erase_strategy::erase_key      :
          ( has_find && has_erase_iterator ) ? set_erase_strategy::erase_iterator :
                                              set_erase_strategy::unsupported );

    // --- aggregate capability ---
    static constexpr bool is_readable =
        ( lookup_strategy != set_lookup_strategy::unsupported );
    static constexpr bool is_writable =
        ( insert_strategy != set_insert_strategy::unsupported );
    static constexpr bool is_erasable =
        ( erase_strategy != set_erase_strategy::unsupported );
    static constexpr bool is_fully_mutable =
        ( is_writable &&
          is_erasable );

    // --- shared subframework cross-reference ---
    //   The set's location on the common container axes, taken straight from the
    // subframework so a set is described in the same vocabulary as every other
    // container.  (Member names are chosen to not shadow the namespace-scope
    // enums / traits they draw from - e.g. `canonical` for the overlay, `grade`
    // for the mutability enum.)
    static constexpr multiplicity_kind  multiplicity =
        multiplicity_kind_of<clean_type>::value;
    static constexpr sortedness         sorted_kind =
        sortedness_of<clean_type>::value;
    static constexpr bool               sorted_enumeration =
        djinterp::admits_sorted_enumeration<clean_type>::value;
    static constexpr canonical_overlay  canonical =
        canonical_overlay_of<clean_type>::value;
    static constexpr mutability         grade =
        mutability_of<clean_type>::value;
    static constexpr access_restriction access =
        access_restriction_of<clean_type>::value;
    static constexpr iteration_mode     iteration =
        iteration_mode_of<clean_type>::value;

    static constexpr const char*        overlay_name =
        canonical_overlay_name(canonical);
    static constexpr const char*        multiplicity_name =
        multiplicity_kind_name(multiplicity);
};


// ===========================================================================
// X.   Type extractors
// ===========================================================================
//   SFINAE-safe extraction of a set's characteristic member types, each yielding
// void when the type does not carry it.  Built through the framework's
// extract-or-fall-back macro (emits `<name>` and `<name>_t`).

// set_key_type_of / _t
//   the element (== key) type of a set, or void.
D_TYPE_TRAIT_MEMBER_TYPE_OR(set_key_type_of, key_type, void)

// set_value_type_of / _t
//   the value type of a set (== key_type for a genuine set), or void.
D_TYPE_TRAIT_MEMBER_TYPE_OR(set_value_type_of, value_type, void)

// set_key_compare_of / _t
//   the comparator of a monotone set, or void.
D_TYPE_TRAIT_MEMBER_TYPE_OR(set_key_compare_of, key_compare, void)

// set_hasher_of / _t
//   the hasher of a hashed set, or void.
D_TYPE_TRAIT_MEMBER_TYPE_OR(set_hasher_of, hasher, void)


// ###########################################################################
// ##  PART II  --  CONCEPTS  (C++20)                                        ##
// ###########################################################################
//   The concept faces of the Part I traits.  Named in PascalCase after their
// trait (leading is_/has_ dropped), per the framework's concept-naming
// convention; each is a thin face over the corresponding `_v` shorthand, for
// call sites that prefer concept syntax to the SFINAE traits.  Empty below C++20.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// --- core / backing / multiplicity ----------------------------------------

// SetLike           face of is_set_like.
template<typename _Type>
concept SetLike = is_set_like_v<_Type>;

// MonotoneSet       face of is_monotone_set (comparator-ordered; was ordered_set).
template<typename _Type>
concept MonotoneSet = is_monotone_set_v<_Type>;

// HashedSet         face of is_hashed_set (hash-ordered; was unordered_set).
template<typename _Type>
concept HashedSet = is_hashed_set_v<_Type>;

// UniqueSet         face of is_unique_set.
template<typename _Type>
concept UniqueSet = is_unique_set_v<_Type>;

// MultiSet          face of is_multi_set.
template<typename _Type>
concept MultiSet = is_multi_set_v<_Type>;

// FlatSet           face of is_flat_set.
template<typename _Type>
concept FlatSet = is_flat_set_v<_Type>;

// --- compound -------------------------------------------------------------

// MonotoneUniqueSet face of is_monotone_unique_set (std::set).
template<typename _Type>
concept MonotoneUniqueSet = is_monotone_unique_set_v<_Type>;

// MonotoneMultiSet  face of is_monotone_multi_set (std::multiset).
template<typename _Type>
concept MonotoneMultiSet = is_monotone_multi_set_v<_Type>;

// HashedUniqueSet   face of is_hashed_unique_set (std::unordered_set).
template<typename _Type>
concept HashedUniqueSet = is_hashed_unique_set_v<_Type>;

// HashedMultiSet    face of is_hashed_multi_set (std::unordered_multiset).
template<typename _Type>
concept HashedMultiSet = is_hashed_multi_set_v<_Type>;

// FlatUniqueSet     face of is_flat_unique_set (flat_set).
template<typename _Type>
concept FlatUniqueSet = is_flat_unique_set_v<_Type>;

// FlatMultiSet      face of is_flat_multi_set (flat_multiset).
template<typename _Type>
concept FlatMultiSet = is_flat_multi_set_v<_Type>;

// --- overlay bridge -------------------------------------------------------

// SetOverlay        face of wears_set_overlay (canonical_overlay::set).
template<typename _Type>
concept SetOverlay = wears_set_overlay_v<_Type>;

// MultisetOverlay   face of wears_multiset_overlay (canonical_overlay::multiset).
template<typename _Type>
concept MultisetOverlay = wears_multiset_overlay_v<_Type>;

// --- method availability --------------------------------------------------

// SetFind           face of has_set_find.
template<typename _Type>
concept SetFind = has_set_find_v<_Type>;

// SetCount          face of has_set_count.
template<typename _Type>
concept SetCount = has_set_count_v<_Type>;

// SetContains       face of has_set_contains.
template<typename _Type>
concept SetContains = has_set_contains_v<_Type>;

// SetEqualRange     face of has_set_equal_range.
template<typename _Type>
concept SetEqualRange = has_set_equal_range_v<_Type>;

// SetLowerBound     face of has_set_lower_bound.
template<typename _Type>
concept SetLowerBound = has_set_lower_bound_v<_Type>;

// SetUpperBound     face of has_set_upper_bound.
template<typename _Type>
concept SetUpperBound = has_set_upper_bound_v<_Type>;

// SetInsert         face of has_set_insert.
template<typename _Type>
concept SetInsert = has_set_insert_v<_Type>;

// SetEmplace        face of has_set_emplace.
template<typename _Type>
concept SetEmplace = has_set_emplace_v<_Type>;

// SetEraseKey       face of has_set_erase_key.
template<typename _Type>
concept SetEraseKey = has_set_erase_key_v<_Type>;

// SetExtract        face of has_set_extract.
template<typename _Type>
concept SetExtract = has_set_extract_v<_Type>;

// SetMerge          face of has_set_merge.
template<typename _Type>
concept SetMerge = has_set_merge_v<_Type>;

// RangeLookupSet    lower_bound AND upper_bound (a derived capability).
template<typename _Type>
concept RangeLookupSet = set_class<_Type>::has_range_lookup;

// FullLookupSet     find AND count AND equal_range.
template<typename _Type>
concept FullLookupSet = set_class<_Type>::has_full_lookup;

// --- strategy faces -------------------------------------------------------

// ContainsLookupSet lookup strategy prefers contains().
template<typename _Type>
concept ContainsLookupSet =
    ( set_class<_Type>::lookup_strategy == set_lookup_strategy::contains );

// FindLookupSet     lookup strategy prefers find().
template<typename _Type>
concept FindLookupSet =
    ( set_class<_Type>::lookup_strategy == set_lookup_strategy::find );

// CountLookupSet    lookup strategy prefers count().
template<typename _Type>
concept CountLookupSet =
    ( set_class<_Type>::lookup_strategy == set_lookup_strategy::count );

// LinearLookupSet   lookup strategy falls back to a linear scan.
template<typename _Type>
concept LinearLookupSet =
    ( set_class<_Type>::lookup_strategy == set_lookup_strategy::linear );

// EmplacingSet      insertion strategy prefers emplace().
template<typename _Type>
concept EmplacingSet =
    ( set_class<_Type>::insert_strategy == set_insert_strategy::emplace );

// InsertingSet      insertion strategy prefers insert(value_type).
template<typename _Type>
concept InsertingSet =
    ( set_class<_Type>::insert_strategy == set_insert_strategy::insert );

// KeyEraseSet       erasure strategy prefers erase(key_type).
template<typename _Type>
concept KeyEraseSet =
    ( set_class<_Type>::erase_strategy == set_erase_strategy::erase_key );

// IteratorEraseSet  erasure strategy uses erase(iterator) after a lookup.
template<typename _Type>
concept IteratorEraseSet =
    ( set_class<_Type>::erase_strategy == set_erase_strategy::erase_iterator );

// --- aggregate capability -------------------------------------------------

// ReadableSet       has a supported lookup strategy.
template<typename _Type>
concept ReadableSet = set_class<_Type>::is_readable;

// WritableSet       has a supported insertion strategy.
template<typename _Type>
concept WritableSet = set_class<_Type>::is_writable;

// ErasableSet       has a supported erasure strategy.
template<typename _Type>
concept ErasableSet = set_class<_Type>::is_erasable;

// FullyMutableSet   supports both insertion and erasure.
template<typename _Type>
concept FullyMutableSet = set_class<_Type>::is_fully_mutable;

// ClassifiedSet     shorthand for any type recognised as set-like by set_class.
template<typename _Type>
concept ClassifiedSet = set_class<_Type>::is_set;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// ###########################################################################
// ##  PART III  --  CONTAINERS                                              ##
// ###########################################################################
//   The four concrete sets.  Each delegates to the corresponding standard
// container and re-exposes the surface the classifier reads: the member types,
// a comparator or hasher, a uniqueness-revealing insert (a pair<iterator,bool>
// for the unique sets, a plain iterator for the multi sets), a const traversal,
// and size().  `structure_category = flat` marks them depth-1 whatever the
// element type (the spec's Structure row for a set), and the standard
// containers' const element iterators give them the element-const access
// restriction of the spec's Mutability row for free.


// ===========================================================================
// I.   set  --  monotone unique
// ===========================================================================

// set
//   class: a sorted, unique-key associative container - the canonical set
// overlay {mu_1^E} on a balanced-tree backing.  Elements are the keys.
// Classifies as: set-like, unordered (bag identity) yet monotone (sorted-by-
// construction enumeration), unique, flat, dynamic storage, structure-mutable
// with an element-const access restriction, bidirectional const iteration.
template<typename _Key,
         typename _Compare   = std::less<_Key>,
         typename _Allocator = std::allocator<_Key>>
class set
{
private:
    using underlying_type = std::set<_Key, _Compare, _Allocator>;

    underlying_type m_data;

public:
    // --- structural classification surface ---
    using structure_category     = flat;

    // --- member types ---
    using key_type               = _Key;
    using value_type             = _Key;
    using key_compare            = _Compare;
    using value_compare          = _Compare;
    using allocator_type         = _Allocator;
    using size_type              = typename underlying_type::size_type;
    using difference_type        = typename underlying_type::difference_type;
    using reference              = typename underlying_type::reference;
    using const_reference        = typename underlying_type::const_reference;
    using iterator               = typename underlying_type::iterator;
    using const_iterator         = typename underlying_type::const_iterator;
    using reverse_iterator       = typename underlying_type::reverse_iterator;
    using const_reverse_iterator = typename underlying_type::const_reverse_iterator;
    using node_type              = typename underlying_type::node_type;

    // --- constructors ---

    set() = default;

    explicit set(const _Compare&   _comp,
                 const _Allocator& _allocator = _Allocator())
        : m_data(_comp, _allocator)
    {}

    template<typename _InputIt>
    set(_InputIt          _first,
        _InputIt          _last,
        const _Compare&   _comp      = _Compare(),
        const _Allocator& _allocator = _Allocator())
        : m_data(_first, _last, _comp, _allocator)
    {}

    set(std::initializer_list<_Key> _init,
        const _Compare&             _comp      = _Compare(),
        const _Allocator&           _allocator = _Allocator())
        : m_data(_init, _comp, _allocator)
    {}

    set(const set&) = default;
    set(set&&)      = default;

    set& operator=(const set&) = default;
    set& operator=(set&&)      = default;

    // --- iteration (const traversal only; the element is frozen) ---

    iterator               begin()         noexcept { return m_data.begin();   }
    const_iterator         begin()   const noexcept { return m_data.begin();   }
    iterator               end()           noexcept { return m_data.end();     }
    const_iterator         end()     const noexcept { return m_data.end();     }
    const_iterator         cbegin()  const noexcept { return m_data.cbegin();  }
    const_iterator         cend()    const noexcept { return m_data.cend();    }
    reverse_iterator       rbegin()        noexcept { return m_data.rbegin();  }
    const_reverse_iterator rbegin()  const noexcept { return m_data.rbegin();  }
    reverse_iterator       rend()          noexcept { return m_data.rend();    }
    const_reverse_iterator rend()    const noexcept { return m_data.rend();    }

    // --- capacity ---

    bool      empty()    const noexcept { return m_data.empty();    }
    size_type size()     const noexcept { return m_data.size();     }
    size_type max_size() const noexcept { return m_data.max_size(); }

    // --- modifiers ---

    void clear() noexcept { m_data.clear(); }

    std::pair<iterator, bool> insert(const value_type& _value)
    {
        return m_data.insert(_value);
    }

    std::pair<iterator, bool> insert(value_type&& _value)
    {
        return m_data.insert(std::move(_value));
    }

    iterator insert(const_iterator    _hint,
                    const value_type& _value)
    {
        return m_data.insert(_hint, _value);
    }

    template<typename _InputIt>
    void insert(_InputIt _first,
                _InputIt _last)
    {
        m_data.insert(_first, _last);
    }

    void insert(std::initializer_list<value_type> _init)
    {
        m_data.insert(_init);
    }

    template<typename... _Args>
    std::pair<iterator, bool> emplace(_Args&&... _args)
    {
        return m_data.emplace(std::forward<_Args>(_args)...);
    }

    template<typename... _Args>
    iterator emplace_hint(const_iterator _hint,
                          _Args&&...     _args)
    {
        return m_data.emplace_hint(_hint, std::forward<_Args>(_args)...);
    }

    iterator  erase(const_iterator _pos)               { return m_data.erase(_pos);          }
    iterator  erase(const_iterator _first,
                    const_iterator _last)              { return m_data.erase(_first, _last); }
    size_type erase(const key_type& _key)              { return m_data.erase(_key);          }

    void swap(set& _other) noexcept { m_data.swap(_other.m_data); }

    node_type extract(const_iterator _pos)             { return m_data.extract(_pos); }
    node_type extract(const key_type& _key)            { return m_data.extract(_key); }

    template<typename _Comp2>
    void merge(std::set<_Key, _Comp2, _Allocator>& _source)
    {
        m_data.merge(_source);
    }

    void merge(set& _source) { m_data.merge(_source.m_data); }

    // --- lookup ---

    size_type      count(const key_type& _key)    const { return m_data.count(_key);     }
    iterator       find(const key_type& _key)           { return m_data.find(_key);      }
    const_iterator find(const key_type& _key)     const { return m_data.find(_key);      }
    bool           contains(const key_type& _key) const { return m_data.count(_key) > 0; }

    std::pair<iterator, iterator>
    equal_range(const key_type& _key)                   { return m_data.equal_range(_key); }
    std::pair<const_iterator, const_iterator>
    equal_range(const key_type& _key)             const { return m_data.equal_range(_key); }

    iterator       lower_bound(const key_type& _key)       { return m_data.lower_bound(_key); }
    const_iterator lower_bound(const key_type& _key) const { return m_data.lower_bound(_key); }
    iterator       upper_bound(const key_type& _key)       { return m_data.upper_bound(_key); }
    const_iterator upper_bound(const key_type& _key) const { return m_data.upper_bound(_key); }

    // --- observers ---

    key_compare   key_comp()   const { return m_data.key_comp();   }
    value_compare value_comp() const { return m_data.value_comp(); }

    // --- comparison operators ---

    friend bool operator==(const set& _lhs, const set& _rhs) { return _lhs.m_data == _rhs.m_data; }
    friend bool operator!=(const set& _lhs, const set& _rhs) { return _lhs.m_data != _rhs.m_data; }
    friend bool operator< (const set& _lhs, const set& _rhs) { return _lhs.m_data <  _rhs.m_data; }
    friend bool operator<=(const set& _lhs, const set& _rhs) { return _lhs.m_data <= _rhs.m_data; }
    friend bool operator> (const set& _lhs, const set& _rhs) { return _lhs.m_data >  _rhs.m_data; }
    friend bool operator>=(const set& _lhs, const set& _rhs) { return _lhs.m_data >= _rhs.m_data; }
};


// ===========================================================================
// II.  multiset  --  monotone multi
// ===========================================================================

// multiset
//   class: a sorted associative container admitting duplicate keys - the
// canonical multiset overlay {mu_m^E}, m > 1, on a balanced-tree backing.
// Classifies as: set-like, monotone, multi, flat, dynamic, structure-mutable
// (element-const), bidirectional const iteration.
template<typename _Key,
         typename _Compare   = std::less<_Key>,
         typename _Allocator = std::allocator<_Key>>
class multiset
{
private:
    using underlying_type = std::multiset<_Key, _Compare, _Allocator>;

    underlying_type m_data;

public:
    // --- structural classification surface ---
    using structure_category     = flat;

    // --- member types ---
    using key_type               = _Key;
    using value_type             = _Key;
    using key_compare            = _Compare;
    using value_compare          = _Compare;
    using allocator_type         = _Allocator;
    using size_type              = typename underlying_type::size_type;
    using difference_type        = typename underlying_type::difference_type;
    using reference              = typename underlying_type::reference;
    using const_reference        = typename underlying_type::const_reference;
    using iterator               = typename underlying_type::iterator;
    using const_iterator         = typename underlying_type::const_iterator;
    using reverse_iterator       = typename underlying_type::reverse_iterator;
    using const_reverse_iterator = typename underlying_type::const_reverse_iterator;
    using node_type              = typename underlying_type::node_type;

    // --- constructors ---

    multiset() = default;

    explicit multiset(const _Compare&   _comp,
                      const _Allocator& _allocator = _Allocator())
        : m_data(_comp, _allocator)
    {}

    template<typename _InputIt>
    multiset(_InputIt          _first,
             _InputIt          _last,
             const _Compare&   _comp      = _Compare(),
             const _Allocator& _allocator = _Allocator())
        : m_data(_first, _last, _comp, _allocator)
    {}

    multiset(std::initializer_list<_Key> _init,
             const _Compare&             _comp      = _Compare(),
             const _Allocator&           _allocator = _Allocator())
        : m_data(_init, _comp, _allocator)
    {}

    multiset(const multiset&) = default;
    multiset(multiset&&)      = default;

    multiset& operator=(const multiset&) = default;
    multiset& operator=(multiset&&)      = default;

    // --- iteration ---

    iterator               begin()         noexcept { return m_data.begin();   }
    const_iterator         begin()   const noexcept { return m_data.begin();   }
    iterator               end()           noexcept { return m_data.end();     }
    const_iterator         end()     const noexcept { return m_data.end();     }
    const_iterator         cbegin()  const noexcept { return m_data.cbegin();  }
    const_iterator         cend()    const noexcept { return m_data.cend();    }
    reverse_iterator       rbegin()        noexcept { return m_data.rbegin();  }
    const_reverse_iterator rbegin()  const noexcept { return m_data.rbegin();  }
    reverse_iterator       rend()          noexcept { return m_data.rend();    }
    const_reverse_iterator rend()    const noexcept { return m_data.rend();    }

    // --- capacity ---

    bool      empty()    const noexcept { return m_data.empty();    }
    size_type size()     const noexcept { return m_data.size();     }
    size_type max_size() const noexcept { return m_data.max_size(); }

    // --- modifiers ---
    //   NOTE: the single-element insert returns a plain iterator (no bool), the
    // structural mark that separates multiset from set semantics for the
    // multiplicity classifier (has_unique_insert).

    void clear() noexcept { m_data.clear(); }

    iterator insert(const value_type& _value)         { return m_data.insert(_value);            }
    iterator insert(value_type&& _value)              { return m_data.insert(std::move(_value));  }
    iterator insert(const_iterator    _hint,
                    const value_type& _value)         { return m_data.insert(_hint, _value);     }

    template<typename _InputIt>
    void insert(_InputIt _first,
                _InputIt _last)                       { m_data.insert(_first, _last); }

    void insert(std::initializer_list<value_type> _init) { m_data.insert(_init); }

    template<typename... _Args>
    iterator emplace(_Args&&... _args)
    {
        return m_data.emplace(std::forward<_Args>(_args)...);
    }

    template<typename... _Args>
    iterator emplace_hint(const_iterator _hint,
                          _Args&&...     _args)
    {
        return m_data.emplace_hint(_hint, std::forward<_Args>(_args)...);
    }

    iterator  erase(const_iterator _pos)               { return m_data.erase(_pos);          }
    iterator  erase(const_iterator _first,
                    const_iterator _last)              { return m_data.erase(_first, _last); }
    size_type erase(const key_type& _key)              { return m_data.erase(_key);          }

    void swap(multiset& _other) noexcept { m_data.swap(_other.m_data); }

    node_type extract(const_iterator _pos)             { return m_data.extract(_pos); }
    node_type extract(const key_type& _key)            { return m_data.extract(_key); }

    template<typename _Comp2>
    void merge(std::multiset<_Key, _Comp2, _Allocator>& _source)
    {
        m_data.merge(_source);
    }

    void merge(multiset& _source) { m_data.merge(_source.m_data); }

    // --- lookup ---

    size_type      count(const key_type& _key)    const { return m_data.count(_key);     }
    iterator       find(const key_type& _key)           { return m_data.find(_key);      }
    const_iterator find(const key_type& _key)     const { return m_data.find(_key);      }
    bool           contains(const key_type& _key) const { return m_data.count(_key) > 0; }

    std::pair<iterator, iterator>
    equal_range(const key_type& _key)                   { return m_data.equal_range(_key); }
    std::pair<const_iterator, const_iterator>
    equal_range(const key_type& _key)             const { return m_data.equal_range(_key); }

    iterator       lower_bound(const key_type& _key)       { return m_data.lower_bound(_key); }
    const_iterator lower_bound(const key_type& _key) const { return m_data.lower_bound(_key); }
    iterator       upper_bound(const key_type& _key)       { return m_data.upper_bound(_key); }
    const_iterator upper_bound(const key_type& _key) const { return m_data.upper_bound(_key); }

    // --- observers ---

    key_compare   key_comp()   const { return m_data.key_comp();   }
    value_compare value_comp() const { return m_data.value_comp(); }

    // --- comparison operators ---

    friend bool operator==(const multiset& _lhs, const multiset& _rhs) { return _lhs.m_data == _rhs.m_data; }
    friend bool operator!=(const multiset& _lhs, const multiset& _rhs) { return _lhs.m_data != _rhs.m_data; }
    friend bool operator< (const multiset& _lhs, const multiset& _rhs) { return _lhs.m_data <  _rhs.m_data; }
    friend bool operator<=(const multiset& _lhs, const multiset& _rhs) { return _lhs.m_data <= _rhs.m_data; }
    friend bool operator> (const multiset& _lhs, const multiset& _rhs) { return _lhs.m_data >  _rhs.m_data; }
    friend bool operator>=(const multiset& _lhs, const multiset& _rhs) { return _lhs.m_data >= _rhs.m_data; }
};


// ===========================================================================
// III. unordered_set  --  hashed unique
// ===========================================================================

// unordered_set
//   class: a hash-based unique-key associative container - the set overlay
// {mu_1^E} on a hash-table backing.  Classifies as: set-like, unordered (bag
// identity, NO monotone enumeration - hash-ordered), unique, flat, dynamic,
// structure-mutable (element-const), forward const iteration.
template<typename _Key,
         typename _Hash      = std::hash<_Key>,
         typename _KeyEqual  = std::equal_to<_Key>,
         typename _Allocator = std::allocator<_Key>>
class unordered_set
{
private:
    using underlying_type = std::unordered_set<_Key, _Hash, _KeyEqual, _Allocator>;

    underlying_type m_data;

public:
    // --- structural classification surface ---
    using structure_category = flat;

    // --- member types ---
    using key_type        = _Key;
    using value_type      = _Key;
    using hasher          = _Hash;
    using key_equal       = _KeyEqual;
    using allocator_type  = _Allocator;
    using size_type       = typename underlying_type::size_type;
    using difference_type = typename underlying_type::difference_type;
    using reference       = typename underlying_type::reference;
    using const_reference = typename underlying_type::const_reference;
    using iterator        = typename underlying_type::iterator;
    using const_iterator  = typename underlying_type::const_iterator;
    using node_type       = typename underlying_type::node_type;

    // --- constructors ---

    unordered_set() = default;

    explicit unordered_set(
        size_type         _bucket_count,
        const _Hash&      _hash      = _Hash(),
        const _KeyEqual&  _equal     = _KeyEqual(),
        const _Allocator& _allocator = _Allocator())
        : m_data(_bucket_count, _hash, _equal, _allocator)
    {}

    template<typename _InputIt>
    unordered_set(
        _InputIt          _first,
        _InputIt          _last,
        size_type         _bucket_count = 0,
        const _Hash&      _hash         = _Hash(),
        const _KeyEqual&  _equal        = _KeyEqual(),
        const _Allocator& _allocator    = _Allocator())
        : m_data(_first, _last, _bucket_count, _hash, _equal, _allocator)
    {}

    unordered_set(
        std::initializer_list<_Key> _init,
        size_type                   _bucket_count = 0,
        const _Hash&                _hash         = _Hash(),
        const _KeyEqual&            _equal        = _KeyEqual(),
        const _Allocator&           _allocator    = _Allocator())
        : m_data(_init, _bucket_count, _hash, _equal, _allocator)
    {}

    unordered_set(const unordered_set&) = default;
    unordered_set(unordered_set&&)      = default;

    unordered_set& operator=(const unordered_set&) = default;
    unordered_set& operator=(unordered_set&&)      = default;

    // --- iteration ---

    iterator       begin()        noexcept { return m_data.begin();  }
    const_iterator begin()  const noexcept { return m_data.begin();  }
    iterator       end()          noexcept { return m_data.end();    }
    const_iterator end()    const noexcept { return m_data.end();    }
    const_iterator cbegin() const noexcept { return m_data.cbegin(); }
    const_iterator cend()   const noexcept { return m_data.cend();   }

    // --- capacity ---

    bool      empty()    const noexcept { return m_data.empty();    }
    size_type size()     const noexcept { return m_data.size();     }
    size_type max_size() const noexcept { return m_data.max_size(); }

    // --- modifiers ---

    void clear() noexcept { m_data.clear(); }

    std::pair<iterator, bool> insert(const value_type& _value) { return m_data.insert(_value);            }
    std::pair<iterator, bool> insert(value_type&& _value)      { return m_data.insert(std::move(_value)); }
    iterator insert(const_iterator    _hint,
                    const value_type& _value)                  { return m_data.insert(_hint, _value);     }

    template<typename _InputIt>
    void insert(_InputIt _first,
                _InputIt _last)                                { m_data.insert(_first, _last); }

    void insert(std::initializer_list<value_type> _init) { m_data.insert(_init); }

    template<typename... _Args>
    std::pair<iterator, bool> emplace(_Args&&... _args)
    {
        return m_data.emplace(std::forward<_Args>(_args)...);
    }

    template<typename... _Args>
    iterator emplace_hint(const_iterator _hint,
                          _Args&&...     _args)
    {
        return m_data.emplace_hint(_hint, std::forward<_Args>(_args)...);
    }

    iterator  erase(const_iterator _pos)               { return m_data.erase(_pos);          }
    iterator  erase(const_iterator _first,
                    const_iterator _last)              { return m_data.erase(_first, _last); }
    size_type erase(const key_type& _key)              { return m_data.erase(_key);          }

    void swap(unordered_set& _other) noexcept { m_data.swap(_other.m_data); }

    node_type extract(const_iterator _pos)             { return m_data.extract(_pos); }
    node_type extract(const key_type& _key)            { return m_data.extract(_key); }

    void merge(unordered_set& _source) { m_data.merge(_source.m_data); }

    // --- lookup ---

    size_type      count(const key_type& _key)    const { return m_data.count(_key);     }
    iterator       find(const key_type& _key)           { return m_data.find(_key);      }
    const_iterator find(const key_type& _key)     const { return m_data.find(_key);      }
    bool           contains(const key_type& _key) const { return m_data.count(_key) > 0; }

    std::pair<iterator, iterator>
    equal_range(const key_type& _key)                   { return m_data.equal_range(_key); }
    std::pair<const_iterator, const_iterator>
    equal_range(const key_type& _key)             const { return m_data.equal_range(_key); }

    // --- bucket interface ---

    size_type bucket_count()               const { return m_data.bucket_count();     }
    size_type max_bucket_count()           const { return m_data.max_bucket_count(); }
    size_type bucket_size(size_type _n)    const { return m_data.bucket_size(_n);    }
    size_type bucket(const key_type& _key) const { return m_data.bucket(_key);       }

    // --- hash policy ---

    float load_factor()               const { return m_data.load_factor();     }
    float max_load_factor()           const { return m_data.max_load_factor(); }
    void  max_load_factor(float _mlf)       { m_data.max_load_factor(_mlf);    }
    void  rehash(size_type _count)          { m_data.rehash(_count);           }
    void  reserve(size_type _count)         { m_data.reserve(_count);          }

    // --- observers ---

    hasher    hash_function() const { return m_data.hash_function(); }
    key_equal key_eq()        const { return m_data.key_eq();        }

    // --- comparison operators ---

    friend bool operator==(const unordered_set& _lhs, const unordered_set& _rhs) { return _lhs.m_data == _rhs.m_data; }
    friend bool operator!=(const unordered_set& _lhs, const unordered_set& _rhs) { return _lhs.m_data != _rhs.m_data; }
};


// ===========================================================================
// IV.  unordered_multiset  --  hashed multi
// ===========================================================================

// unordered_multiset
//   class: a hash-based associative container admitting duplicate keys - the
// multiset overlay {mu_m^E}, m > 1, on a hash-table backing.  Classifies as:
// set-like, unordered (hash-ordered), multi, flat, dynamic, structure-mutable
// (element-const), forward const iteration.
template<typename _Key,
         typename _Hash      = std::hash<_Key>,
         typename _KeyEqual  = std::equal_to<_Key>,
         typename _Allocator = std::allocator<_Key>>
class unordered_multiset
{
private:
    using underlying_type = std::unordered_multiset<_Key, _Hash, _KeyEqual, _Allocator>;

    underlying_type m_data;

public:
    // --- structural classification surface ---
    using structure_category = flat;

    // --- member types ---
    using key_type        = _Key;
    using value_type      = _Key;
    using hasher          = _Hash;
    using key_equal       = _KeyEqual;
    using allocator_type  = _Allocator;
    using size_type       = typename underlying_type::size_type;
    using difference_type = typename underlying_type::difference_type;
    using reference       = typename underlying_type::reference;
    using const_reference = typename underlying_type::const_reference;
    using iterator        = typename underlying_type::iterator;
    using const_iterator  = typename underlying_type::const_iterator;
    using node_type       = typename underlying_type::node_type;

    // --- constructors ---

    unordered_multiset() = default;

    explicit unordered_multiset(
        size_type         _bucket_count,
        const _Hash&      _hash      = _Hash(),
        const _KeyEqual&  _equal     = _KeyEqual(),
        const _Allocator& _allocator = _Allocator())
        : m_data(_bucket_count, _hash, _equal, _allocator)
    {}

    template<typename _InputIt>
    unordered_multiset(
        _InputIt          _first,
        _InputIt          _last,
        size_type         _bucket_count = 0,
        const _Hash&      _hash         = _Hash(),
        const _KeyEqual&  _equal        = _KeyEqual(),
        const _Allocator& _allocator    = _Allocator())
        : m_data(_first, _last, _bucket_count, _hash, _equal, _allocator)
    {}

    unordered_multiset(
        std::initializer_list<_Key> _init,
        size_type                   _bucket_count = 0,
        const _Hash&                _hash         = _Hash(),
        const _KeyEqual&            _equal        = _KeyEqual(),
        const _Allocator&           _allocator    = _Allocator())
        : m_data(_init, _bucket_count, _hash, _equal, _allocator)
    {}

    unordered_multiset(const unordered_multiset&) = default;
    unordered_multiset(unordered_multiset&&)      = default;

    unordered_multiset& operator=(const unordered_multiset&) = default;
    unordered_multiset& operator=(unordered_multiset&&)      = default;

    // --- iteration ---

    iterator       begin()        noexcept { return m_data.begin();  }
    const_iterator begin()  const noexcept { return m_data.begin();  }
    iterator       end()          noexcept { return m_data.end();    }
    const_iterator end()    const noexcept { return m_data.end();    }
    const_iterator cbegin() const noexcept { return m_data.cbegin(); }
    const_iterator cend()   const noexcept { return m_data.cend();   }

    // --- capacity ---

    bool      empty()    const noexcept { return m_data.empty();    }
    size_type size()     const noexcept { return m_data.size();     }
    size_type max_size() const noexcept { return m_data.max_size(); }

    // --- modifiers (single-element insert returns a plain iterator) ---

    void clear() noexcept { m_data.clear(); }

    iterator insert(const value_type& _value)         { return m_data.insert(_value);            }
    iterator insert(value_type&& _value)              { return m_data.insert(std::move(_value));  }
    iterator insert(const_iterator    _hint,
                    const value_type& _value)         { return m_data.insert(_hint, _value);     }

    template<typename _InputIt>
    void insert(_InputIt _first,
                _InputIt _last)                       { m_data.insert(_first, _last); }

    void insert(std::initializer_list<value_type> _init) { m_data.insert(_init); }

    template<typename... _Args>
    iterator emplace(_Args&&... _args)
    {
        return m_data.emplace(std::forward<_Args>(_args)...);
    }

    template<typename... _Args>
    iterator emplace_hint(const_iterator _hint,
                          _Args&&...     _args)
    {
        return m_data.emplace_hint(_hint, std::forward<_Args>(_args)...);
    }

    iterator  erase(const_iterator _pos)               { return m_data.erase(_pos);          }
    iterator  erase(const_iterator _first,
                    const_iterator _last)              { return m_data.erase(_first, _last); }
    size_type erase(const key_type& _key)              { return m_data.erase(_key);          }

    void swap(unordered_multiset& _other) noexcept { m_data.swap(_other.m_data); }

    node_type extract(const_iterator _pos)             { return m_data.extract(_pos); }
    node_type extract(const key_type& _key)            { return m_data.extract(_key); }

    void merge(unordered_multiset& _source) { m_data.merge(_source.m_data); }

    // --- lookup ---

    size_type      count(const key_type& _key)    const { return m_data.count(_key);     }
    iterator       find(const key_type& _key)           { return m_data.find(_key);      }
    const_iterator find(const key_type& _key)     const { return m_data.find(_key);      }
    bool           contains(const key_type& _key) const { return m_data.count(_key) > 0; }

    std::pair<iterator, iterator>
    equal_range(const key_type& _key)                   { return m_data.equal_range(_key); }
    std::pair<const_iterator, const_iterator>
    equal_range(const key_type& _key)             const { return m_data.equal_range(_key); }

    // --- bucket interface ---

    size_type bucket_count()               const { return m_data.bucket_count();     }
    size_type max_bucket_count()           const { return m_data.max_bucket_count(); }
    size_type bucket_size(size_type _n)    const { return m_data.bucket_size(_n);    }
    size_type bucket(const key_type& _key) const { return m_data.bucket(_key);       }

    // --- hash policy ---

    float load_factor()               const { return m_data.load_factor();     }
    float max_load_factor()           const { return m_data.max_load_factor(); }
    void  max_load_factor(float _mlf)       { m_data.max_load_factor(_mlf);    }
    void  rehash(size_type _count)          { m_data.rehash(_count);           }
    void  reserve(size_type _count)         { m_data.reserve(_count);          }

    // --- observers ---

    hasher    hash_function() const { return m_data.hash_function(); }
    key_equal key_eq()        const { return m_data.key_eq();        }

    // --- comparison operators ---

    friend bool operator==(const unordered_multiset& _lhs, const unordered_multiset& _rhs) { return _lhs.m_data == _rhs.m_data; }
    friend bool operator!=(const unordered_multiset& _lhs, const unordered_multiset& _rhs) { return _lhs.m_data != _rhs.m_data; }
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_SET_