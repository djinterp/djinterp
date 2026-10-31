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
*                                                            revised: 2026.09.21
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
#include "../../../../inc/djinterp/cpp/meta/value_list.hpp"


NS_DJINTERP
NS_TESTING


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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_VALUE_LIST_TESTS_
