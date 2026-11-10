/*******************************************************************************
* djinterp [test]                                               reduce_tests.hpp
*
*   Unit-test declarations for core/functional/reduce.hpp -- the DRIVERS, the
* iteration half of the step/driver split.  One declaration group per section of
* the module under test:
*
*     reduce_tests_rt.cpp       -- reduce_rt      (the runtime loop; both forms)
*     reduce_tests_value.cpp    -- reduce_ct      (the value domain: DValueList)
*     reduce_tests_type.cpp     -- reduce_ct      (the type domain: std::tuple)
*     reduce_tests_unified.cpp  --                (the step/driver thesis, and
*                                                   the C++17 tier)
*
*   FIXTURES.  The header's whole claim is that "the same reducer body serves all
* three domains; only the driver (loop vs. recursion) and the leaf differ."  The
* fixtures are built to put that claim on trial rather than take it on faith.
*
*     count_all      is ONE reducer body -- (acc, anything) -> acc + 1 -- passed
*                    UNCHANGED to all three drivers.  If the thesis holds it
*                    returns the element count in every domain, and it does.
*
*     DSumWith<Leaf> is one reducer body parameterised by a LEAF.  Three leaves --
*                    DRtLeaf (a runtime value), DValLeaf (a val_t carrier),
*                    DTypeLeaf (a type_t carrier) -- are the only thing that
*                    changes between the domains, which is exactly what the
*                    header says should change.
*
*     digits_*       are non-commutative ((acc * 10) + x), so folding 1, 2, 3 from
*                    a seed of 0 yields 123 only under a LEFT fold.  Each domain
*                    gets one, so the traversal order is pinned three times over.
*
*   THE DRIVERS ARE NOT INTERCHANGEABLE, and the suite pins the difference:
* reduce_rt assigns into its accumulator (`_acc = _rf(_acc, *_it)`) and returns
* _Acc, so the accumulator TYPE IS FIXED across the fold -- widen proves it, by
* returning a long that still comes back as an int.  Both reduce_ct overloads
* recompute the accumulator type at every step, so it may EVOLVE -- DAppendVal and
* DSizesIntoList prove it, by growing a DValueList whose type changes on each
* element.
*
*   THE TWO TYPE-DOMAIN ENTRIES DIFFER, and the suite pins that too: the preferred
* type_c<std::tuple<Ts...>> form walks element TYPES and never builds a tuple, so
* it folds a tuple of NON-default-constructible elements; the convenience
* std::tuple<Ts...>{} form cannot even be called on one.  can_value_entry reads
* that difference SFINAE-safely.
*
*   DCountIter is a generated source with no container behind it, and DAdlRange is
* found only through free begin/end -- between them they check the two things
* reduce_rt's iterable form promises: that lazy sources are fine, and that the
* `using std::begin;` idiom really does admit ADL.
*
*   All tests are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/cpp/functional/reduce_tests.hpp
* link(s):   TBA
* author(s): teer                                            created: 2026.07.12
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (one-body reducers, three leaves, lazy sources, probes)
      -----------------------------------------------------------------

I.    reduce_rt        (the runtime driver)
      -------------------------------------

II.   reduce_ct        (the value domain: DValueList)
      -----------------------------------------------

III.  reduce_ct        (the type domain: std::tuple)
      ----------------------------------------------

IV.   THE STEP/DRIVER SPLIT  (one reducer, three domains; the tier)
      -------------------------------------------------------------
*/


#ifndef DJINTERP_TEST_REDUCE_TESTS_
#define DJINTERP_TEST_REDUCE_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp
// djinterp (module under test)
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/reduce.hpp"
#include "../../../../inc/djinterp/cpp/functional/structural_traits.hpp"   // Reducer (C++20 face)
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
//   reduce_ct and the value carriers are C++17 facilities, so the module -- and
// therefore every fixture and every test body here -- is gated to that floor.
// That this suite still COMPILES under C++11 and C++14, having included the
// header, is precisely the check that the header is inert below its tier.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// -- ONE reducer body, run unchanged by all three drivers -----------------

// count_all
//   fixture reducer: (acc, anything) -> acc + 1. Domain-agnostic by construction
// -- it never looks at the element -- so the SAME object is handed to reduce_rt,
// to reduce_ct over a DValueList, and to reduce_ct over a tuple. If the
// step/driver split is real, it returns the element count in all three.
struct count_all
{
    template<typename _Element>
    D_CONSTEXPR int operator()(int _acc, _Element) const { return _acc + 1; }
};


// -- one reducer body + three LEAVES (only the leaf differs) --------------

// DRtLeaf:   a runtime value  -> itself.
struct DRtLeaf
{
    D_CONSTEXPR int operator()(int _x) const { return _x; }
};

// DValLeaf:  a val_t carrier  -> the value it carries.
struct DValLeaf
{
    template<auto _Value>
    D_CONSTEXPR
    int operator()(
        val_t<_Value>
    ) const
    {
        return static_cast<int>(_Value);
    }
};

// DTypeLeaf: a type_t carrier -> a fact about the type it carries.
struct DTypeLeaf
{
    template<typename _Type>
    D_CONSTEXPR
    int operator()(
        type_t<_Type>
    ) const
    {
        return static_cast<int>(sizeof(_Type));
    }
};

// DSumWith
//   fixture reducer: the SAME body in every domain -- (acc, x) -> acc + leaf(x).
// Only the leaf changes.
template<typename _Leaf>
struct DSumWith
{
    _Leaf leaf;

    template<typename _Element>
    D_CONSTEXPR
    int operator()(
        int      _acc,
        _Element _x
    ) const
    {
        return _acc + leaf(_x);
    }
};


// -- non-commutative reducers: they pin the LEFT fold in each domain ------

struct DDigitsRt
{
    D_CONSTEXPR
    int operator()(
        int _acc,
        int _x
    ) const
    {
        return (_acc * 10) + _x;
    }
};

struct DDigitsVal
{
    template<auto _Value>
    D_CONSTEXPR
    int operator()(
        int _acc,
        val_t<_Value>
    ) const
    {
        return (_acc * 10) + static_cast<int>(_Value);
    }
};

struct digits_type
{
    template<typename _Type>
    D_CONSTEXPR
    int operator()(
        int _acc,
        type_t<_Type>
    ) const
    {
        return (_acc * 10) + static_cast<int>(sizeof(_Type));
    }
};


// -- accumulator-type-EVOLVING reducers (the reduce_ct drivers only) ------

// DAppendVal: the accumulator is a DValueList that GROWS -- so its type changes
// at every step, which reduce_rt could not express.
struct DAppendVal
{
    template<auto... _Acc,
             auto    _Value>
    D_CONSTEXPR
    auto operator()(
        DValueList<_Acc...> _acc,
        val_t<_Value>
    ) const
    {
        return append(_acc, val<_Value>);
    }
};

// DPrependVal: the same, at the front -- so reduce_ct with it REVERSES the list.
struct DPrependVal
{
    template<auto... _Acc,
             auto    _Value>
    D_CONSTEXPR
    auto operator()(
        DValueList<_Acc...> _acc,
        val_t<_Value>
    ) const
    {
        return prepend(val<_Value>, _acc);
    }
};

// DSizesIntoList: the type domain's evolving accumulator -- each element TYPE
// contributes its size to a growing DValueList.
struct DSizesIntoList
{
    template<auto... _Acc,
             typename _Type>
    D_CONSTEXPR
    auto operator()(
        DValueList<_Acc...> _acc,
        type_t<_Type>
    ) const
    {
        return append(_acc, val<sizeof(_Type)>);
    }
};


// -- the accumulator type is FIXED in reduce_rt ---------------------------

// widen
//   fixture reducer: returns a LONG, though the seed is an int. reduce_rt
// assigns the result back into its _Acc and returns _Acc, so the result comes
// back an int -- the accumulator type is fixed across the fold.
struct widen
{
    D_CONSTEXPR
    long operator()(
        int _acc,
        int _x
    ) const
    {
        return static_cast<long>(_acc) + _x;
    }
};


// -- counting reducers: they make the visit count observable --------------

// DCountingAny
//   fixture reducer: records how many times it was called, in ANY domain. The
// drivers take the reducer by value, so the count is kept behind a pointer.
struct DCountingAny
{
    int* calls;

    template<typename _Element>
    int operator()(
        int _acc,
        _Element
    ) const
    {
        ++(*calls);
        return _acc + 1;
    }
};


// -- sources for reduce_rt's iterable form --------------------------------

// DCountIter
//   fixture iterator: a GENERATED source -- there is no container behind it. It
// checks the header's claim that lazy sources are fine: the driver pulls until
// the range ends and needs nothing more than *, ++ and !=.
struct DCountIter
{
    int n;

    D_CONSTEXPR int          operator*() const { return n; }
    D_CONSTEXPR DCountIter&  operator++()      { ++n; return *this; }
    D_CONSTEXPR
    bool operator!=(
        const DCountIter& _other
    ) const
    {
        return n != _other.n;
    }
};

// DAdlRange
//   fixture range: exposes NO member begin/end -- only the free functions below,
// which reduce_rt can reach solely because it says `using std::begin;` and then
// calls begin() unqualified.
struct DAdlRange
{
    const int*  data;
    std::size_t count;
};

D_CONSTEXPR
const int* begin(
    const DAdlRange& _r
)
{
    return _r.data;
}

D_CONSTEXPR
const int* end(
    const DAdlRange& _r
)
{
    return _r.data + _r.count;
}


// -- a NON-default-constructible element type ----------------------------

// no_default
//   fixture: cannot be default-constructed, so std::tuple<no_default, ...>
// cannot be either. The preferred type_c entry folds it anyway (it walks TYPES
// and builds no tuple value); the convenience value entry cannot be called at
// all.
struct no_default
{
    int v;

    D_CONSTEXPR explicit no_default(int _x) : v(_x) {}

    no_default() = delete;
};


// -- probes ---------------------------------------------------------------

// can_value_entry
//   probe: is reduce_ct's std::tuple<Ts...> VALUE overload callable for this
// tuple? False whenever an element is not default-constructible -- which is the
// whole reason the type_c form is the preferred entry.
template<typename _Rf,
         typename _Acc,
         typename _Tuple,
         typename = void>
struct can_value_entry : std::false_type
{
};

template<typename _Rf,
         typename _Acc,
         typename _Tuple>
struct can_value_entry<
    _Rf,
    _Acc,
    _Tuple,
    void_t<decltype(::djinterp::reduce_ct(std::declval<_Rf>(),
                                          std::declval<_Acc>(),
                                          _Tuple{}))> >
    : std::true_type
{
};

// a free function reducer, to show the drivers accept any callable.
D_CONSTEXPR
int sum_fn(
    int _acc,
    int _x
)
{
    return _acc + _x;
}


///////////////////////////////////////////////////////////////////////////////
///             CONCEPT-FACING HELPER  (C++20)                              ///
///////////////////////////////////////////////////////////////////////////////
//   The drivers are deliberately UNCONSTRAINED so one body serves every domain.
// The header notes that a C++20 caller MAY layer the Reducer concept at the call
// site instead -- this is that call site, compiled.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Rf>
    requires Reducer<_Rf, int, int>
D_CONSTEXPR int
reduce_rt_checked(
    _Rf         _rf,
    int         _acc,
    const int*  _first,
    const int*  _last
)
{
    return ::djinterp::reduce_rt(_rf, _acc, _first, _last);
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///             I.    reduce_rt  (the runtime driver)                       ///
///////////////////////////////////////////////////////////////////////////////

bool tests_rt_range_form();
bool tests_rt_iterable_form();
bool tests_rt_both_forms_agree();
bool tests_rt_empty_returns_the_seed();
bool tests_rt_is_a_left_fold();
bool tests_rt_single_element();
bool tests_rt_constexpr_over_a_constexpr_range();
bool tests_rt_accumulator_type_is_fixed();
bool tests_rt_pulls_a_lazy_generated_source();
bool tests_rt_finds_begin_end_by_adl();
bool tests_rt_visits_each_element_once();
bool tests_rt_does_not_consume_its_source();


///////////////////////////////////////////////////////////////////////////////
///             II.   reduce_ct  (the value domain)                         ///
///////////////////////////////////////////////////////////////////////////////

bool tests_ct_value_list_folds();
bool tests_ct_value_list_empty_returns_the_seed();
bool tests_ct_value_list_is_a_left_fold();
bool tests_ct_value_list_delegates_to_fold();
bool tests_ct_value_list_accumulator_may_evolve();
bool tests_ct_value_list_heterogeneous();
bool tests_ct_value_list_single_element();
bool tests_ct_value_list_visits_each_element_once();


///////////////////////////////////////////////////////////////////////////////
///             III.  reduce_ct  (the type domain)                          ///
///////////////////////////////////////////////////////////////////////////////

bool tests_ct_tuple_type_c_entry();
bool tests_ct_tuple_value_entry();
bool tests_ct_tuple_both_entries_agree();
bool tests_ct_tuple_empty_returns_the_seed();
bool tests_ct_tuple_is_a_left_fold();
bool tests_ct_tuple_walks_types_not_values();
bool tests_ct_tuple_feeds_each_type_as_a_carrier();
bool tests_ct_tuple_duplicate_types();
bool tests_ct_tuple_accumulator_may_evolve();
bool tests_ct_tuple_single_element();
bool tests_ct_tuple_heterogeneous_sizes();
bool tests_ct_tuple_visits_each_element_once();


///////////////////////////////////////////////////////////////////////////////
///             IV.   THE STEP/DRIVER SPLIT                                 ///
///////////////////////////////////////////////////////////////////////////////

bool tests_one_reducer_body_three_domains();
bool tests_only_the_driver_and_the_leaf_differ();
bool tests_the_drivers_differ_on_the_accumulator();
bool tests_the_drivers_agree_on_the_same_data();
bool tests_the_drivers_are_unconstrained();
bool tests_module_tier_gate();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// reduce_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
reduce_spec()
{
    return dt::module_spec{
        "reduce",
        "the iterator-range form, the iterable form, and constexpr folding",
        {
            dt::block_spec{
                "rt",
                "Rt",
                {
                    { "tests_rt_range_form",
                      "The iterator-range form folds [_first, _last)",
                      &tests_rt_range_form },
                    { "tests_rt_iterable_form",
                      "The convenience form takes anything with begin()/end() "
                      "and wraps the range form",
                      &tests_rt_iterable_form },
                    { "tests_rt_both_forms_agree",
                      "The iterable form is documented as a wrapper around "
                      "the range form, so the two must give the same answer "
                      "for the same data and reducer",
                      &tests_rt_both_forms_agree },
                    { "tests_rt_empty_returns_the_seed",
                      "An empty range folds to the accumulator, untouched -- "
                      "the reducer is never invoked",
                      &tests_rt_empty_returns_the_seed },
                    { "tests_rt_is_a_left_fold",
                      "rt is a left fold",
                      &tests_rt_is_a_left_fold },
                    { "tests_rt_single_element",
                      "The one-element case: exactly one turn of the loop",
                      &tests_rt_single_element },
                    { "tests_rt_constexpr_over_a_constexpr_range",
                      "The header's claim: reduce_rt is constexpr, \"so it "
                      "also folds at compile time over a constexpr range.\"",
                      &tests_rt_constexpr_over_a_constexpr_range },
                    { "tests_rt_accumulator_type_is_fixed",
                      "reduce_rt assigns the reducer's result back into its "
                      "accumulator and returns _Acc, so the accumulator TYPE "
                      "IS FIXED across the fold -- it is the seed's type, "
                      "whatever the reducer returns",
                      &tests_rt_accumulator_type_is_fixed },
                    { "tests_rt_pulls_a_lazy_generated_source",
                      "rt pulls a lazy generated source",
                      &tests_rt_pulls_a_lazy_generated_source },
                    { "tests_rt_finds_begin_end_by_adl",
                      "The iterable form says `using std::begin; using "
                      "std::end;` and then calls them unqualified, so a range "
                      "that exposes only FREE begin/end -- no members at all "
                      "-- is still admitted, found by argument-dependent "
                      "lookup",
                      &tests_rt_finds_begin_end_by_adl },
                    { "tests_rt_visits_each_element_once",
                      "The loop pulls each element exactly once, in order -- "
                      "no element is skipped and none is folded twice",
                      &tests_rt_visits_each_element_once },
                    { "tests_rt_does_not_consume_its_source",
                      "The iterable is taken by const reference and only "
                      "walked, so the source survives the fold unchanged and "
                      "may be folded again",
                      &tests_rt_does_not_consume_its_source },
                }
            },
            dt::block_spec{
                "value",
                "Value",
                {
                    { "tests_ct_value_list_folds",
                      "The compile-time driver folds an NTTP sequence, "
                      "feeding each element to the reducer as a val_t carrier",
                      &tests_ct_value_list_folds },
                    { "tests_ct_value_list_empty_returns_the_seed",
                      "The empty list folds to the accumulator, untouched",
                      &tests_ct_value_list_empty_returns_the_seed },
                    { "tests_ct_value_list_is_a_left_fold",
                      "ct value list is a left fold",
                      &tests_ct_value_list_is_a_left_fold },
                    { "tests_ct_value_list_delegates_to_fold",
                      "The value-domain recursion lives in DValueList's own "
                      "fold; reduce_ct is purely the unified entry point",
                      &tests_ct_value_list_delegates_to_fold },
                    { "tests_ct_value_list_accumulator_may_evolve",
                      "Unlike reduce_rt, the compile-time driver does not fix "
                      "the accumulator type: the reducer's result becomes the "
                      "next accumulator, so its TYPE may change at every step",
                      &tests_ct_value_list_accumulator_may_evolve },
                    { "tests_ct_value_list_heterogeneous",
                      "The pack is auto..., so the elements need not share a "
                      "type; the reducer sees each one as a val_t over its "
                      "own type",
                      &tests_ct_value_list_heterogeneous },
                    { "tests_ct_value_list_single_element",
                      "The one-element case: exactly one turn of the "
                      "recursion before the base case",
                      &tests_ct_value_list_single_element },
                    { "tests_ct_value_list_visits_each_element_once",
                      "The recursion peels exactly one head per step -- no "
                      "element is skipped and none is folded twice",
                      &tests_ct_value_list_visits_each_element_once },
                }
            },
            dt::block_spec{
                "type",
                "Type",
                {
                    { "tests_ct_tuple_type_c_entry",
                      "The PREFERRED type-domain entry: the sequence is "
                      "carried as a type, type_c<std::tuple<Ts...>>",
                      &tests_ct_tuple_type_c_entry },
                    { "tests_ct_tuple_value_entry",
                      "The CONVENIENCE entry, matching the value-passing "
                      "style: a std::tuple value is passed and immediately "
                      "discarded (the parameter is unnamed)",
                      &tests_ct_tuple_value_entry },
                    { "tests_ct_tuple_both_entries_agree",
                      "Both entries route to the same recursion, so they must "
                      "agree exactly",
                      &tests_ct_tuple_both_entries_agree },
                    { "tests_ct_tuple_empty_returns_the_seed",
                      "The base case: the empty tuple folds to the "
                      "accumulator, untouched -- the reducer is never invoked",
                      &tests_ct_tuple_empty_returns_the_seed },
                    { "tests_ct_tuple_is_a_left_fold",
                      "The recursion folds the HEAD type into the accumulator "
                      "and recurses on the tail, so the element types arrive "
                      "first-to-last",
                      &tests_ct_tuple_is_a_left_fold },
                    { "tests_ct_tuple_walks_types_not_values",
                      "The headline advantage of the preferred entry: the "
                      "recursion walks element TYPES and never builds a tuple "
                      "value, so the element types need NOT be "
                      "default-constructible",
                      &tests_ct_tuple_walks_types_not_values },
                    { "tests_ct_tuple_feeds_each_type_as_a_carrier",
                      "Each element type is handed to the reducer as a type_c "
                      "carrier, so the reducer is a type-domain leaf: it "
                      "recovers the type from the carrier, never from a value",
                      &tests_ct_tuple_feeds_each_type_as_a_carrier },
                    { "tests_ct_tuple_duplicate_types",
                      "The recursion walks POSITIONS, not distinct types: a "
                      "repeated element type is visited once per occurrence",
                      &tests_ct_tuple_duplicate_types },
                    { "tests_ct_tuple_accumulator_may_evolve",
                      "The recursion recomputes the accumulator type at every "
                      "step -- it is decltype(rf(acc, type_c<T0>)) -- so, as "
                      "in the value domain and unlike reduce_rt, the "
                      "accumulator's TYPE may change as the fold proceeds",
                      &tests_ct_tuple_accumulator_may_evolve },
                    { "tests_ct_tuple_single_element",
                      "The one-element case: exactly one turn of the "
                      "recursion before the base case",
                      &tests_ct_tuple_single_element },
                    { "tests_ct_tuple_heterogeneous_sizes",
                      "The tuple is a sequence of unrelated types, and the "
                      "leaf reads a fact about each one in turn",
                      &tests_ct_tuple_heterogeneous_sizes },
                    { "tests_ct_tuple_visits_each_element_once",
                      "The recursion peels exactly one head type per step -- "
                      "none skipped, none folded twice",
                      &tests_ct_tuple_visits_each_element_once },
                }
            },
            dt::block_spec{
                "unified",
                "Unified",
                {
                    { "tests_one_reducer_body_three_domains",
                      "THE THESIS",
                      &tests_one_reducer_body_three_domains },
                    { "tests_only_the_driver_and_the_leaf_differ",
                      "The finer form of the thesis: where a reducer must "
                      "actually LOOK at its element, the body still does not "
                      "change -- only the LEAF does",
                      &tests_only_the_driver_and_the_leaf_differ },
                    { "tests_the_drivers_differ_on_the_accumulator",
                      "Where the two drivers genuinely part company",
                      &tests_the_drivers_differ_on_the_accumulator },
                    { "tests_the_drivers_agree_on_the_same_data",
                      "The same numbers, expressed once as a runtime range "
                      "and once as an NTTP sequence, fold to the same answer "
                      "under the same reducer body -- which is what it means "
                      "for the domains to be two views of one computation",
                      &tests_the_drivers_agree_on_the_same_data },
                    { "tests_the_drivers_are_unconstrained",
                      "\"The drivers are deliberately left unconstrained so "
                      "one body serves every domain; a C++20 caller may layer "
                      "the Reducer / Transducer concepts at the call site.\" "
                      "Both halves are checked here",
                      &tests_the_drivers_are_unconstrained },
                    { "tests_module_tier_gate",
                      "The TIER claim: reduce_ct and the value carriers are "
                      "C++17 facilities, so the module is gated to that floor "
                      "and \"under C++11 this header is empty.\" That this "
                      "translation unit includes the header and still "
                      "compiles under -std=c++11 and -std=c++14 IS the check "
                      "-- nothing below the gate may name reduce_rt or "
                      "reduce_ct at all",
                      &tests_module_tier_gate },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_REDUCE_TESTS_
