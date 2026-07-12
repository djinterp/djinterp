/******************************************************************************
* djinterp [test]                                            reduce_tests.hpp
*
*   Unit-test declarations for core/functional/reduce.hpp -- the DRIVERS, the
* iteration half of the step/driver split.  One declaration group per section of
* the module under test:
*
*     reduce_tests_rt.cpp       -- reduce_rt      (the runtime loop; both forms)
*     reduce_tests_value.cpp    -- reduce_ct      (the value domain: value_list)
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
*     sum_with<Leaf> is one reducer body parameterised by a LEAF.  Three leaves --
*                    rt_leaf (a runtime value), val_leaf (a val_t carrier),
*                    type_leaf (a type_t carrier) -- are the only thing that
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
* recompute the accumulator type at every step, so it may EVOLVE -- append_val and
* sizes_into_list prove it, by growing a value_list whose type changes on each
* element.
*
*   THE TWO TYPE-DOMAIN ENTRIES DIFFER, and the suite pins that too: the preferred
* type_c<std::tuple<Ts...>> form walks element TYPES and never builds a tuple, so
* it folds a tuple of NON-default-constructible elements; the convenience
* std::tuple<Ts...>{} form cannot even be called on one.  can_value_entry reads
* that difference SFINAE-safely.
*
*   count_iter is a generated source with no container behind it, and adl_range is
* found only through free begin/end -- between them they check the two things
* reduce_rt's iterable form promises: that lazy sources are fine, and that the
* `using std::begin;` idiom really does admit ADL.
*
*   All tests are flat in djinterp::testing.
*
* path:      /inc/djinterp/test/functional/reduce_tests.hpp
* link(s):   TBA
* author(s): teer                                          created: 2026.07.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (one-body reducers, three leaves, lazy sources, probes)
I.    reduce_rt        (the runtime driver)
II.   reduce_ct        (the value domain: value_list)
III.  reduce_ct        (the type domain: std::tuple)
IV.   THE STEP/DRIVER SPLIT  (one reducer, three domains; the tier)
*/


#ifndef DJINTERP_TEST_REDUCE_TESTS_
#define DJINTERP_TEST_REDUCE_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp (module under test)
#include "../../core/functional/reduce.hpp"
#include "../../core/functional/structural_traits.hpp"   // Reducer (C++20 face)


NS_DJINTERP
NS_TESTING


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
// to reduce_ct over a value_list, and to reduce_ct over a tuple. If the
// step/driver split is real, it returns the element count in all three.
struct count_all
{
    template<typename _Element>
    D_CONSTEXPR int operator()(int _acc, _Element) const { return _acc + 1; }
};


// -- one reducer body + three LEAVES (only the leaf differs) --------------

// rt_leaf:   a runtime value  -> itself.
struct rt_leaf
{
    D_CONSTEXPR int operator()(int _x) const { return _x; }
};

// val_leaf:  a val_t carrier  -> the value it carries.
struct val_leaf
{
    template<auto _Value>
    D_CONSTEXPR int operator()(val_t<_Value>) const
    {
        return static_cast<int>(_Value);
    }
};

// type_leaf: a type_t carrier -> a fact about the type it carries.
struct type_leaf
{
    template<typename _Type>
    D_CONSTEXPR int operator()(type_t<_Type>) const
    {
        return static_cast<int>(sizeof(_Type));
    }
};

// sum_with
//   fixture reducer: the SAME body in every domain -- (acc, x) -> acc + leaf(x).
// Only the leaf changes.
template<typename _Leaf>
struct sum_with
{
    _Leaf leaf;

    template<typename _Element>
    D_CONSTEXPR int operator()(int _acc, _Element _x) const
    {
        return _acc + leaf(_x);
    }
};


// -- non-commutative reducers: they pin the LEFT fold in each domain ------

struct digits_rt
{
    D_CONSTEXPR int operator()(int _acc, int _x) const
    {
        return (_acc * 10) + _x;
    }
};

struct digits_val
{
    template<auto _Value>
    D_CONSTEXPR int operator()(int _acc, val_t<_Value>) const
    {
        return (_acc * 10) + static_cast<int>(_Value);
    }
};

struct digits_type
{
    template<typename _Type>
    D_CONSTEXPR int operator()(int _acc, type_t<_Type>) const
    {
        return (_acc * 10) + static_cast<int>(sizeof(_Type));
    }
};


// -- accumulator-type-EVOLVING reducers (the reduce_ct drivers only) ------

// append_val: the accumulator is a value_list that GROWS -- so its type changes
// at every step, which reduce_rt could not express.
struct append_val
{
    template<auto... _Acc,
             auto    _Value>
    D_CONSTEXPR auto operator()(value_list<_Acc...> _acc, val_t<_Value>) const
    {
        return append(_acc, val<_Value>);
    }
};

// prepend_val: the same, at the front -- so reduce_ct with it REVERSES the list.
struct prepend_val
{
    template<auto... _Acc,
             auto    _Value>
    D_CONSTEXPR auto operator()(value_list<_Acc...> _acc, val_t<_Value>) const
    {
        return prepend(val<_Value>, _acc);
    }
};

// sizes_into_list: the type domain's evolving accumulator -- each element TYPE
// contributes its size to a growing value_list.
struct sizes_into_list
{
    template<auto... _Acc,
             typename _Type>
    D_CONSTEXPR auto operator()(value_list<_Acc...> _acc, type_t<_Type>) const
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
    D_CONSTEXPR long operator()(int _acc, int _x) const
    {
        return static_cast<long>(_acc) + _x;
    }
};


// -- counting reducers: they make the visit count observable --------------

// counting_any
//   fixture reducer: records how many times it was called, in ANY domain. The
// drivers take the reducer by value, so the count is kept behind a pointer.
struct counting_any
{
    int* calls;

    template<typename _Element>
    int operator()(int _acc, _Element) const
    {
        ++(*calls);
        return _acc + 1;
    }
};


// -- sources for reduce_rt's iterable form --------------------------------

// count_iter
//   fixture iterator: a GENERATED source -- there is no container behind it. It
// checks the header's claim that lazy sources are fine: the driver pulls until
// the range ends and needs nothing more than *, ++ and !=.
struct count_iter
{
    int n;

    D_CONSTEXPR int          operator*() const { return n; }
    D_CONSTEXPR count_iter&  operator++()      { ++n; return *this; }
    D_CONSTEXPR bool operator!=(const count_iter& _other) const
    {
        return n != _other.n;
    }
};

// adl_range
//   fixture range: exposes NO member begin/end -- only the free functions below,
// which reduce_rt can reach solely because it says `using std::begin;` and then
// calls begin() unqualified.
struct adl_range
{
    const int*  data;
    std::size_t count;
};

D_CONSTEXPR const int*
begin(const adl_range& _r)
{
    return _r.data;
}

D_CONSTEXPR const int*
end(const adl_range& _r)
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
D_CONSTEXPR int
sum_fn(int _acc, int _x)
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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_REDUCE_TESTS_
