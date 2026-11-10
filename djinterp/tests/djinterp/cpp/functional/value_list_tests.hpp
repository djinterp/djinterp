/*******************************************************************************
* djinterp [test]                                           value_list_tests.hpp
*
*   Unit-test declarations for core/meta/value_list.hpp -- the NTTP sequence, the
* value-domain counterpart of dtuple's std::tuple type sequence.  One declaration
* group per section of the module under test:
*
*     value_list_tests_list.cpp       -- I.   DValueList<auto...> + size()
*     value_list_tests_detection.cpp  -- II.  is_value_list (+ the ValueList face)
*     value_list_tests_access.cpp     -- III. value_list_size, value_list_at, at
*     value_list_tests_growth.cpp     -- IV.  append / prepend / concat
*     value_list_tests_transform.cpp  -- V.   transform
*     value_list_tests_fold.cpp       -- VI.  fold
*
*   TIER.  DValueList is an auto-NTTP facility, so the whole module is gated to
* C++17; below that the header declares NOTHING.  Every fixture and every test
* body here is gated to match, which means this suite still COMPILES under C++11
* and C++14 -- and that it does, having included the header, is precisely the
* check that the header is inert at those floors.  The concept face is gated
* further to C++20.
*
*   FIXTURES.  The growth / transform / fold ops all take value carriers (val_t)
* and are carrier-callable leaves -- val_t<V> -> val_t<f(V)> for the unary ones,
* (acc, val_t<V>) -> acc for the binary ones -- exactly the leaf shape compose and
* the transducer spine use.  Two of them are chosen to expose contracts the module
* only states in prose:
*
*     DDeclOnlyOp   is DECLARED and never DEFINED, and its operator() is neither
*                    const nor constexpr.  transform still works with it, because
*                    transform only probes the op in an unevaluated context and
*                    uses its RESULT TYPE.  fold, which actually invokes its op,
*                    could not.
*     DDigitsOp      is non-commutative ((acc * 10) + v), so folding 1,2,3 from
*                    seed 0 yields 123 only if the fold is LEFT-associative.
*
*   DPrependOp and DAppendOp take a DValueList as the accumulator rather than a
* carrier -- the header says the seed may be "any object, typically a carrier or
* another DValueList" -- which makes fold+append a copy and fold+prepend a
* REVERSE, pinning the traversal order a second way.
*
*   All tests are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/cpp/functional/value_list_tests.hpp
* link(s):   TBA
* author(s): teer                                            created: 2026.07.12
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (carrier-callable leaves, look-alikes, an enum for heterogeneity)
      ---------------------------------------------------------------------------

I.    THE SEQUENCE     (DValueList, size)
      -----------------------------------

II.   DETECTION        (is_value_list, ValueList)
      -------------------------------------------

III.  SIZE + ACCESS    (value_list_size, value_list_at, at)
      -----------------------------------------------------

IV.   GROWTH           (append, prepend, concat)
      ------------------------------------------

V.    TRANSFORMATION   (transform)
      ----------------------------

VI.   REDUCTION        (fold)
      -----------------------
*/


#ifndef DJINTERP_TEST_VALUE_LIST_TESTS_
#define DJINTERP_TEST_VALUE_LIST_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
// djinterp (module under test)
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/meta/value_list.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////
//   DValueList is a C++17 facility; below its tier the module declares nothing,
// so the fixtures are gated to match.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// -- a non-integral NTTP kind, for heterogeneity --------------------------

enum class color
{
    red   = 1,
    green = 2
};


// -- unary carrier-callable leaves:  val_t<V> -> val_t<f(V)> --------------

// DDoubleOp: V -> V * 2.
struct DDoubleOp
{
    template<auto _Value>
    constexpr val_t<_Value * 2>
    operator()(val_t<_Value>) const { return {}; }
};

// DAdd3Op: V -> V + 3  (paired with DDoubleOp for the composition law).
struct DAdd3Op
{
    template<auto _Value>
    constexpr val_t<_Value + 3>
    operator()(val_t<_Value>) const { return {}; }
};

// DDoubleThenAdd3Op: the composite g . f, written directly.
struct DDoubleThenAdd3Op
{
    template<auto _Value>
    constexpr val_t<(_Value * 2) + 3>
    operator()(val_t<_Value>) const { return {}; }
};

// DIdentOp: V -> V  (the functor identity law).
struct DIdentOp
{
    template<auto _Value>
    constexpr val_t<_Value>
    operator()(val_t<_Value>) const { return {}; }
};

// DGt1Op: V -> (V > 1)  -- an int list becomes a bool list.
struct DGt1Op
{
    template<auto _Value>
    constexpr val_t<(_Value > 1)>
    operator()(val_t<_Value>) const { return {}; }
};

// DTruthyOp: V -> bool(V)  -- works across a heterogeneous list.
struct DTruthyOp
{
    template<auto _Value>
    constexpr val_t<static_cast<bool>(_Value)>
    operator()(val_t<_Value>) const { return {}; }
};

// DDeclOnlyOp
//   fixture leaf: DECLARED, never DEFINED -- and its operator() is neither const
// nor constexpr. transform nevertheless works with it, because transform probes
// the op in an UNEVALUATED context and needs only its result type. A leaf like
// this could not drive fold, which really invokes its op.
struct DDeclOnlyOp
{
    template<auto _Value>
    val_t<_Value * 3> operator()(
        val_t<_Value>
    );      // no definition, by design
};


// -- binary carrier-callable leaves:  (acc, val_t<V>) -> acc --------------

// DSumOp: a carrier accumulator.
struct DSumOp
{
    template<auto _Acc,
             auto _Value>
    constexpr val_t<_Acc + _Value>
    operator()(val_t<_Acc>, val_t<_Value>) const { return {}; }
};

// DDigitsOp
//   fixture reducer: NON-COMMUTATIVE -- (acc * 10) + v. Folding 1, 2, 3 from a
// seed of 0 yields 123 under a LEFT fold and something else under any other
// order, so it pins the traversal.
struct DDigitsOp
{
    template<auto _Acc,
             auto _Value>
    constexpr val_t<(_Acc * 10) + _Value>
    operator()(val_t<_Acc>, val_t<_Value>) const { return {}; }
};

// DAppendOp: a DValueList accumulator -- fold with it copies the list.
struct DAppendOp
{
    template<auto... _Acc,
             auto    _Value>
    constexpr
    auto operator()(
        DValueList<_Acc...> _acc,
        val_t<_Value>       _v
    ) const
    {
        return append(_acc, _v);
    }
};

// DPrependOp: a DValueList accumulator -- fold with it REVERSES the list.
struct DPrependOp
{
    template<auto... _Acc,
             auto    _Value>
    constexpr
    auto operator()(
        DValueList<_Acc...> _acc,
        val_t<_Value>       _v
    ) const
    {
        return prepend(_v, _acc);
    }
};

// DIntSumOp
//   fixture reducer: a PLAIN-VALUE accumulator. val_t and DValueList are empty
// types -- their value lives in the type -- so a fold over them cannot tell an
// accumulator that is passed through from one that is default-constructed afresh.
// An int accumulator can, and the header does permit "any object" as the seed.
struct DIntSumOp
{
    template<auto _Value>
    constexpr
    int operator()(
        int _acc,
        val_t<_Value>
    ) const
    {
        return _acc + static_cast<int>(_Value);
    }
};

// DCountOp: an accumulator that ignores the element -- fold as a counter.
struct DCountOp
{
    template<auto _Acc,
             auto _Value>
    constexpr val_t<_Acc + 1>
    operator()(val_t<_Acc>, val_t<_Value>) const { return {}; }
};


// -- negatives / look-alikes ----------------------------------------------

// DFakeList: mimics the interface but is NOT a DValueList specialization --
// detection is specialization-based, so this must be refused.
struct DFakeList
{
    static constexpr std::size_t
    size() noexcept { return 3; }
};

// DDerivedList: DERIVES from a DValueList -- still not a specialization of one.
struct DDerivedList : DValueList<1, 2>
{
};

// DNotAList: unrelated.
struct DNotAList
{
    int x;
};


// -- completeness probe ---------------------------------------------------

// is_complete
//   fixture trait: true when _Type is a COMPLETE type. value_list_size and
// value_list_at leave their primary template declared-but-undefined, so a query
// they do not have a specialization for is an INCOMPLETE type rather than a
// false answer. This probe reads that SFINAE-safely, which lets the suite pin
// which spellings they accept -- notably that, unlike is_value_list, they do NOT
// apply clean_t.
template<typename _Type,
         typename = void>
struct is_complete : std::false_type
{
};

template<typename _Type>
struct is_complete<_Type, decltype(void(sizeof(_Type)))> : std::true_type
{
};


///////////////////////////////////////////////////////////////////////////////
///             CONCEPT-FACING HELPERS  (C++20)                             ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// list_size_of: constrained on ValueList -- takes any list, refuses anything else.
template<ValueList _List>
constexpr
std::size_t list_size_of(
    _List
)
{
    return value_list_size_v<_List>;
}

// which_list: constrained overload + unconstrained fallback, proving the concept
// GATES resolution rather than merely evaluating to a bool.
template<typename _Type>
    requires ValueList<_Type>
constexpr int which_list(_Type) { return 1; }

template<typename _Type>
constexpr int which_list(_Type) { return 0; }

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///             I.    THE SEQUENCE                                          ///
///////////////////////////////////////////////////////////////////////////////

bool tests_value_list_size_member();
bool tests_value_list_is_empty_object();
bool tests_value_list_heterogeneous();
bool tests_value_list_size_noexcept();
bool tests_value_list_duplicates();
bool tests_value_list_type_identity();
bool tests_value_list_tier_gate();


///////////////////////////////////////////////////////////////////////////////
///             II.   DETECTION                                             ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_value_list_positive();
bool tests_is_value_list_negative();
bool tests_is_value_list_exact();
bool tests_is_value_list_cvref();
bool tests_is_value_list_v_agrees();
bool tests_concept_value_list();
bool tests_concept_value_list_gating();


///////////////////////////////////////////////////////////////////////////////
///             III.  SIZE + ACCESS                                         ///
///////////////////////////////////////////////////////////////////////////////

bool tests_value_list_size_trait();
bool tests_value_list_size_integral_constant();
bool tests_value_list_size_v_agrees();
bool tests_value_list_at_positions();
bool tests_value_list_at_preserves_value_type();
bool tests_value_list_at_deep();
bool tests_value_list_at_v_agrees();
bool tests_at_returns_carrier();
bool tests_at_feeds_the_carrier_pipeline();
bool tests_traits_require_an_unqualified_list();


///////////////////////////////////////////////////////////////////////////////
///             IV.   GROWTH                                                ///
///////////////////////////////////////////////////////////////////////////////

bool tests_append();
bool tests_append_to_empty();
bool tests_prepend();
bool tests_prepend_to_empty();
bool tests_growth_heterogeneous();
bool tests_concat_nullary_and_unary();
bool tests_concat_binary();
bool tests_concat_variadic();
bool tests_concat_empty_lists();
bool tests_concat_monoid_laws();


///////////////////////////////////////////////////////////////////////////////
///             V.    TRANSFORMATION                                        ///
///////////////////////////////////////////////////////////////////////////////

bool tests_transform_maps_every_element();
bool tests_transform_empty();
bool tests_transform_law_identity();
bool tests_transform_law_composition();
bool tests_transform_changes_value_type();
bool tests_transform_unevaluated_probe();
bool tests_transform_heterogeneous();
bool tests_transform_preserves_length();


///////////////////////////////////////////////////////////////////////////////
///             VI.   REDUCTION                                             ///
///////////////////////////////////////////////////////////////////////////////

bool tests_fold_reduces();
bool tests_fold_empty_returns_seed();
bool tests_fold_is_left_associative();
bool tests_fold_single_element();
bool tests_fold_accumulator_may_be_a_list();
bool tests_fold_reverses_with_prepend();
bool tests_fold_deep();
bool tests_fold_seed_may_be_a_plain_value();
bool tests_fold_and_transform_compose();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// value_list_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
value_list_spec()
{
    return dt::module_spec{
        "value_list",
        "size(), the empty-object layout, heterogeneity, the C++17 tier",
        {
            dt::block_spec{
                "list",
                "List",
                {
                    { "tests_value_list_size_member",
                      "The list reports its own length",
                      &tests_value_list_size_member },
                    { "tests_value_list_is_empty_object",
                      "The values live in the TYPE, so the list carries no "
                      "storage -- it is an empty struct, cheap to pass by "
                      "value into the constexpr free-function ops",
                      &tests_value_list_is_empty_object },
                    { "tests_value_list_heterogeneous",
                      "The pack is auto..., so the elements need not share a "
                      "type -- this is the value-domain analog of a "
                      "std::tuple's heterogeneity",
                      &tests_value_list_heterogeneous },
                    { "tests_value_list_size_noexcept",
                      "size() is a static constexpr noexcept function -- "
                      "usable anywhere a constant is needed, and never "
                      "throwing",
                      &tests_value_list_size_noexcept },
                    { "tests_value_list_duplicates",
                      "A list is a sequence, not a set: repeated values are "
                      "distinct elements, each addressable",
                      &tests_value_list_duplicates },
                    { "tests_value_list_type_identity",
                      "Two lists are the same type exactly when their packs "
                      "match -- so ORDER matters, and so does the TYPE of "
                      "each value, since an NTTP carries its type with it",
                      &tests_value_list_type_identity },
                    { "tests_value_list_tier_gate",
                      "The TIER claim: DValueList is an auto-NTTP facility, "
                      "so the module is gated to C++17 and below that the "
                      "header declares NOTHING (it adds no trait-floor "
                      "surface)",
                      &tests_value_list_tier_gate },
                }
            },
            dt::block_spec{
                "detection",
                "Detection",
                {
                    { "tests_is_value_list_positive",
                      "Any DValueList specialization is recognised",
                      &tests_is_value_list_positive },
                    { "tests_is_value_list_negative",
                      "Everything else is refused -- notably std::tuple, "
                      "which is the TYPE-domain sequence this module is the "
                      "value-domain counterpart of",
                      &tests_is_value_list_negative },
                    { "tests_is_value_list_exact",
                      "Detection is specialization-based, so it is EXACT: "
                      "mimicking the interface does not qualify, and neither "
                      "does deriving from a list",
                      &tests_is_value_list_exact },
                    { "tests_is_value_list_cvref",
                      "clean_t is applied first, so cv-qualifiers and "
                      "references are stripped before the match",
                      &tests_is_value_list_cvref },
                    { "tests_is_value_list_v_agrees",
                      "The _v companion (emitted by D_TYPE_TRAIT_VALUE_BOOL) "
                      "is exactly the trait's value",
                      &tests_is_value_list_v_agrees },
                    { "tests_concept_value_list",
                      "ValueList is the concept face of is_value_list, and it "
                      "really GATES resolution -- it is a constraint, not "
                      "merely a bool that happens to be true",
                      &tests_concept_value_list },
                    { "tests_concept_value_list_gating",
                      "The concept face is gated to C++20, one tier above the "
                      "module's own C++17 floor, leaving is_value_list as the "
                      "always-present face in between",
                      &tests_concept_value_list_gating },
                }
            },
            dt::block_spec{
                "access",
                "Access",
                {
                    { "tests_value_list_size_trait",
                      "value_list_size is the value-domain analog of "
                      "std::tuple_size",
                      &tests_value_list_size_trait },
                    { "tests_value_list_size_integral_constant",
                      "It IS an integral_constant, so it carries that whole "
                      "surface -- the same as std::tuple_size",
                      &tests_value_list_size_integral_constant },
                    { "tests_value_list_size_v_agrees",
                      "The _v shorthand is exactly the trait's value",
                      &tests_value_list_size_v_agrees },
                    { "tests_value_list_at_positions",
                      "value_list_at yields the I-th value, 0-based",
                      &tests_value_list_at_positions },
                    { "tests_value_list_at_preserves_value_type",
                      "An NTTP carries its type with it, so the extracted "
                      "value keeps that type -- an int stays an int, a char a "
                      "char, an enum an enum",
                      &tests_value_list_at_preserves_value_type },
                    { "tests_value_list_at_deep",
                      "The accessor peels one head per index step, so a deep "
                      "index exercises that recursion",
                      &tests_value_list_at_deep },
                    { "tests_value_list_at_v_agrees",
                      "The _v shorthand is exactly the trait's value, at "
                      "every index",
                      &tests_value_list_at_v_agrees },
                    { "tests_at_returns_carrier",
                      "at<I>(list) is the object-domain accessor: it hands "
                      "back the element as a val_t carrier, so it slots "
                      "straight into the carrier pipeline",
                      &tests_at_returns_carrier },
                    { "tests_at_feeds_the_carrier_pipeline",
                      "The point of returning a carrier: at()'s result is "
                      "exactly what append / prepend consume, so element "
                      "access composes with growth without an unwrap step",
                      &tests_at_feeds_the_carrier_pipeline },
                    { "tests_traits_require_an_unqualified_list",
                      "A sharp edge, pinned",
                      &tests_traits_require_an_unqualified_list },
                }
            },
            dt::block_spec{
                "growth",
                "Growth",
                {
                    { "tests_append",
                      "append puts the new element at the END",
                      &tests_append },
                    { "tests_append_to_empty",
                      "Appending to the empty list yields a singleton -- the "
                      "base of the collect step",
                      &tests_append_to_empty },
                    { "tests_prepend",
                      "prepend puts the new element at the FRONT -- the "
                      "mirror of append, and the reason fold with it reverses "
                      "a list",
                      &tests_prepend },
                    { "tests_prepend_to_empty",
                      "Prepending to the empty list yields a singleton",
                      &tests_prepend_to_empty },
                    { "tests_growth_heterogeneous",
                      "Growth does not homogenise: an element keeps its own "
                      "type as it joins the list",
                      &tests_growth_heterogeneous },
                    { "tests_concat_nullary_and_unary",
                      "The two degenerate arities: concat of nothing is the "
                      "empty list, and concat of one list is that list (the "
                      "identity overload, which returns its argument)",
                      &tests_concat_nullary_and_unary },
                    { "tests_concat_binary",
                      "The two-list base case, which every longer concat "
                      "folds down to",
                      &tests_concat_binary },
                    { "tests_concat_variadic",
                      "Three or more lists take the >= 3 overload, which "
                      "folds pairwise from the left",
                      &tests_concat_variadic },
                    { "tests_concat_empty_lists",
                      "Empty lists contribute nothing, wherever they sit",
                      &tests_concat_empty_lists },
                    { "tests_concat_monoid_laws",
                      "concat is a monoid over value_lists: associative, with "
                      "the empty list as its identity element on both sides",
                      &tests_concat_monoid_laws },
                }
            },
            dt::block_spec{
                "transform",
                "Transform",
                {
                    { "tests_transform_maps_every_element",
                      "transform applies the leaf to each element and returns "
                      "a new list -- the value-domain analog of dtuple's "
                      "tuple_apply_all",
                      &tests_transform_maps_every_element },
                    { "tests_transform_empty",
                      "Mapping over the empty list yields the empty list -- "
                      "the pack expansion has nothing to expand, so the leaf "
                      "is never even probed",
                      &tests_transform_empty },
                    { "tests_transform_law_identity",
                      "The functor identity law: mapping the identity leaf "
                      "changes nothing",
                      &tests_transform_law_identity },
                    { "tests_transform_law_composition",
                      "The functor composition law: mapping f then g is "
                      "mapping (g ",
                      &tests_transform_law_composition },
                    { "tests_transform_changes_value_type",
                      "The leaf may change the value's TYPE, not merely its "
                      "value -- an int list can come back a bool list",
                      &tests_transform_changes_value_type },
                    { "tests_transform_unevaluated_probe",
                      "transform needs only the leaf's RESULT TYPE, so it "
                      "probes it in an UNEVALUATED context: "
                      "decltype(declval<_Op&>()(val<V>))::value",
                      &tests_transform_unevaluated_probe },
                    { "tests_transform_heterogeneous",
                      "A generic leaf maps a mixed list, each element "
                      "entering at its own type",
                      &tests_transform_heterogeneous },
                    { "tests_transform_preserves_length",
                      "transform is a map, not a filter: the result has "
                      "exactly as many elements as the source, always",
                      &tests_transform_preserves_length },
                }
            },
            dt::block_spec{
                "fold",
                "Fold",
                {
                    { "tests_fold_reduces",
                      "fold threads a binary carrier-callable reducer over "
                      "the list, from a seed",
                      &tests_fold_reduces },
                    { "tests_fold_empty_returns_seed",
                      "The base case: an empty list folds to the accumulator, "
                      "untouched -- the reducer is never invoked",
                      &tests_fold_empty_returns_seed },
                    { "tests_fold_is_left_associative",
                      "fold is left associative",
                      &tests_fold_is_left_associative },
                    { "tests_fold_single_element",
                      "The one-element case exercises exactly one turn of the "
                      "recursion before the base case",
                      &tests_fold_single_element },
                    { "tests_fold_accumulator_may_be_a_list",
                      "The seed is \"any object\" -- typically a carrier, but "
                      "a DValueList serves just as well",
                      &tests_fold_accumulator_may_be_a_list },
                    { "tests_fold_reverses_with_prepend",
                      "The same fold with prepend instead of append REVERSES "
                      "the list -- each element arrives at the front, so the "
                      "last one processed ends up first",
                      &tests_fold_reverses_with_prepend },
                    { "tests_fold_deep",
                      "A long list exercises the recursion to depth",
                      &tests_fold_deep },
                    { "tests_fold_seed_may_be_a_plain_value",
                      "The seed is \"any object\" -- not only a carrier or a "
                      "list",
                      &tests_fold_seed_may_be_a_plain_value },
                    { "tests_fold_and_transform_compose",
                      "The two drivers compose: map, then reduce",
                      &tests_fold_and_transform_compose },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_VALUE_LIST_TESTS_
