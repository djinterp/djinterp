/******************************************************************************
* djinterp [test]                                         value_list_tests.hpp
*
*   Unit-test declarations for core/meta/value_list.hpp -- the NTTP sequence, the
* value-domain counterpart of dtuple's std::tuple type sequence.  One declaration
* group per section of the module under test:
*
*     value_list_tests_list.cpp       -- I.   value_list<auto...> + size()
*     value_list_tests_detection.cpp  -- II.  is_value_list (+ the ValueList face)
*     value_list_tests_access.cpp     -- III. value_list_size, value_list_at, at
*     value_list_tests_growth.cpp     -- IV.  append / prepend / concat
*     value_list_tests_transform.cpp  -- V.   transform
*     value_list_tests_fold.cpp       -- VI.  fold
*
*   TIER.  value_list is an auto-NTTP facility, so the whole module is gated to
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
*     decl_only_op   is DECLARED and never DEFINED, and its operator() is neither
*                    const nor constexpr.  transform still works with it, because
*                    transform only probes the op in an unevaluated context and
*                    uses its RESULT TYPE.  fold, which actually invokes its op,
*                    could not.
*     digits_op      is non-commutative ((acc * 10) + v), so folding 1,2,3 from
*                    seed 0 yields 123 only if the fold is LEFT-associative.
*
*   prepend_op and append_op take a value_list as the accumulator rather than a
* carrier -- the header says the seed may be "any object, typically a carrier or
* another value_list" -- which makes fold+append a copy and fold+prepend a
* REVERSE, pinning the traversal order a second way.
*
*   All tests are flat in djinterp::testing.
*
* path:      /inc/djinterp/test/meta/value_list_tests.hpp
* link(s):   TBA
* author(s): teer                                          created: 2026.07.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (carrier-callable leaves, look-alikes, an enum for heterogeneity)
I.    THE SEQUENCE     (value_list, size)
II.   DETECTION        (is_value_list, ValueList)
III.  SIZE + ACCESS    (value_list_size, value_list_at, at)
IV.   GROWTH           (append, prepend, concat)
V.    TRANSFORMATION   (transform)
VI.   REDUCTION        (fold)
*/


#ifndef DJINTERP_TEST_VALUE_LIST_TESTS_
#define DJINTERP_TEST_VALUE_LIST_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp (module under test)
#include "../../core/meta/value_list.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////
//   value_list is a C++17 facility; below its tier the module declares nothing,
// so the fixtures are gated to match.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// -- a non-integral NTTP kind, for heterogeneity --------------------------

enum class color
{
    red   = 1,
    green = 2
};


// -- unary carrier-callable leaves:  val_t<V> -> val_t<f(V)> --------------

// double_op: V -> V * 2.
struct double_op
{
    template<auto _Value>
    constexpr val_t<_Value * 2>
    operator()(val_t<_Value>) const { return {}; }
};

// add3_op: V -> V + 3  (paired with double_op for the composition law).
struct add3_op
{
    template<auto _Value>
    constexpr val_t<_Value + 3>
    operator()(val_t<_Value>) const { return {}; }
};

// double_then_add3_op: the composite g . f, written directly.
struct double_then_add3_op
{
    template<auto _Value>
    constexpr val_t<(_Value * 2) + 3>
    operator()(val_t<_Value>) const { return {}; }
};

// ident_op: V -> V  (the functor identity law).
struct ident_op
{
    template<auto _Value>
    constexpr val_t<_Value>
    operator()(val_t<_Value>) const { return {}; }
};

// gt1_op: V -> (V > 1)  -- an int list becomes a bool list.
struct gt1_op
{
    template<auto _Value>
    constexpr val_t<(_Value > 1)>
    operator()(val_t<_Value>) const { return {}; }
};

// truthy_op: V -> bool(V)  -- works across a heterogeneous list.
struct truthy_op
{
    template<auto _Value>
    constexpr val_t<static_cast<bool>(_Value)>
    operator()(val_t<_Value>) const { return {}; }
};

// decl_only_op
//   fixture leaf: DECLARED, never DEFINED -- and its operator() is neither const
// nor constexpr. transform nevertheless works with it, because transform probes
// the op in an UNEVALUATED context and needs only its result type. A leaf like
// this could not drive fold, which really invokes its op.
struct decl_only_op
{
    template<auto _Value>
    val_t<_Value * 3>
    operator()(val_t<_Value>);      // no definition, by design
};


// -- binary carrier-callable leaves:  (acc, val_t<V>) -> acc --------------

// sum_op: a carrier accumulator.
struct sum_op
{
    template<auto _Acc,
             auto _Value>
    constexpr val_t<_Acc + _Value>
    operator()(val_t<_Acc>, val_t<_Value>) const { return {}; }
};

// digits_op
//   fixture reducer: NON-COMMUTATIVE -- (acc * 10) + v. Folding 1, 2, 3 from a
// seed of 0 yields 123 under a LEFT fold and something else under any other
// order, so it pins the traversal.
struct digits_op
{
    template<auto _Acc,
             auto _Value>
    constexpr val_t<(_Acc * 10) + _Value>
    operator()(val_t<_Acc>, val_t<_Value>) const { return {}; }
};

// append_op: a value_list accumulator -- fold with it copies the list.
struct append_op
{
    template<auto... _Acc,
             auto    _Value>
    constexpr auto
    operator()(value_list<_Acc...> _acc, val_t<_Value> _v) const
    {
        return append(_acc, _v);
    }
};

// prepend_op: a value_list accumulator -- fold with it REVERSES the list.
struct prepend_op
{
    template<auto... _Acc,
             auto    _Value>
    constexpr auto
    operator()(value_list<_Acc...> _acc, val_t<_Value> _v) const
    {
        return prepend(_v, _acc);
    }
};

// int_sum_op
//   fixture reducer: a PLAIN-VALUE accumulator. val_t and value_list are empty
// types -- their value lives in the type -- so a fold over them cannot tell an
// accumulator that is passed through from one that is default-constructed afresh.
// An int accumulator can, and the header does permit "any object" as the seed.
struct int_sum_op
{
    template<auto _Value>
    constexpr int
    operator()(int _acc, val_t<_Value>) const
    {
        return _acc + static_cast<int>(_Value);
    }
};

// count_op: an accumulator that ignores the element -- fold as a counter.
struct count_op
{
    template<auto _Acc,
             auto _Value>
    constexpr val_t<_Acc + 1>
    operator()(val_t<_Acc>, val_t<_Value>) const { return {}; }
};


// -- negatives / look-alikes ----------------------------------------------

// fake_list: mimics the interface but is NOT a value_list specialization --
// detection is specialization-based, so this must be refused.
struct fake_list
{
    static constexpr std::size_t
    size() noexcept { return 3; }
};

// derived_list: DERIVES from a value_list -- still not a specialization of one.
struct derived_list : value_list<1, 2>
{
};

// not_a_list: unrelated.
struct not_a_list
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
constexpr std::size_t
list_size_of(_List)
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
