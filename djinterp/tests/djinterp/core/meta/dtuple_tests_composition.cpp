/******************************************************************************
* djinterp [testing]                               dtuple_tests_composition.cpp
*
*   Composition-group test definitions for the dtuple test suite:
* wrap_all (+ the bundled modifier functors) and tuple_join.  These
* are the operations that BUILD tuples from component parts — by
* composing unary type modifiers (wrap_all) or by concatenating
* existing tuples (tuple_join).
*
*   The tuple_join test is deliberately expanded beyond the original
* two-operand form to pin the behavior of explicit N > 2 joins
* (tuple_join<A, B, C, D>).  A binary-recursive implementation could
* differ subtly from a direct N-ary expansion at the associativity
* seams; the 3- and 4-operand tests cover both.
*
*   See dtuple_tests.hpp for the framework-feature manifest.
*
*
* path:      /tests/djinterp/core/meta/dtuple_tests_composition.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING


// =========================================================================
// I.   WRAP_ALL AND MODIFIERS
// =========================================================================

/*
tests_dtuple_wrap_all_and_modifiers
  Verifies wrap_all composes unary type modifiers right-to-left
  and that the supplied to_lvalue_reference / to_rvalue_reference /
  to_pointer / to_type modifiers behave as advertised.
  Tests the following:
  - empty modifier pack is the identity (pins empty-composition
    semantics)
  - single modifier round-trips through std::add_pointer_t
  - composition of two modifiers pins the left-to-right order
  - identity_trait (hand-rolled std::type_identity analogue) does
    not perturb the input type
  - to_pointer / to_lvalue_reference / to_rvalue_reference functor
    aliases match their std equivalents
  - to_type is the identity functor and its _t alias agrees
*/
bool
tests_dtuple_wrap_all_and_modifiers(
    test_handler& _handler
)
{
    // empty modifier pack -> identity (lower priority but
    // essential: pins the empty-composition contract)
    D_TEST_TYPE_EQ(wrap_all_t<int>,      int);
    D_TEST_TYPE_EQ(wrap_all_t<const char*>,
                   const char*);

    // single modifier
    D_TEST_TYPE_EQ(wrap_all_t<int, std::add_pointer>, int*);

    // identity-trait composition: wrap_all<T, identity_trait> == T
    D_TEST_TYPE_EQ(wrap_all_t<int, internal::identity_trait>,
                   int);
    D_TEST_TYPE_EQ(wrap_all_t<const volatile char*,
                              internal::identity_trait>,
                   const volatile char*);

    // identity composed with a real modifier should be the same as
    // just the real modifier
    D_TEST_TYPE_EQ(
        wrap_all_t<int, std::add_pointer, internal::identity_trait>,
        int*);

    // composition: order is left-to-right (outer first per the
    // helper's recursive expansion).  Pin the actual behavior so
    // callers can reason about it.
    D_TEST_TYPE_EQ(
        wrap_all_t<int, std::add_pointer, std::add_const>,
        const int*);

    // functor-style modifiers
    D_TEST_TYPE_EQ(to_pointer::type<int>,           int*);
    D_TEST_TYPE_EQ(to_lvalue_reference::type<int>,  int&);
    D_TEST_TYPE_EQ(to_rvalue_reference::type<int>,  int&&);

    // to_type as identity
    D_TEST_TYPE_EQ(to_type_t<int>, int);
    D_TEST_TYPE_EQ(to_type_t<const volatile char*>,
                   const volatile char*);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "wrap_all_and_modifiers/all-checks");

    return true;
}


// =========================================================================
// II.  TUPLE_JOIN
// =========================================================================

/*
tests_dtuple_tuple_join
  Verifies tuple_join concatenates an arbitrary number of tuples
  while obeying the algebraic identity laws of concatenation.
  Tests the following:
  - joining nothing yields the empty tuple
  - joining a single tuple is identity
  - joining two tuples concatenates them
  - the empty tuple is a left, right, and interior identity
  - joining is associative across three tuples
  - explicit 3- and 4-operand joins (tuple_join<A,B,C,D>) agree
    with their left- and right-associated binary decompositions
    (catches fold-direction discrepancies in N-ary recursion)
  - joining many empty tuples yields the empty tuple
*/
bool
tests_dtuple_tuple_join(
    test_handler& _handler
)
{
    typedef std::tuple<>                 t_empty;
    typedef std::tuple<int>              t_i;
    typedef std::tuple<char>             t_c;
    typedef std::tuple<long>             t_l;
    typedef std::tuple<double>           t_d;
    typedef std::tuple<int, char>        t_ic;
    typedef std::tuple<int, char, long>  t_icl;
    typedef std::tuple<int, char, long, double>
                                         t_icld;

    // identity laws
    D_TEST_TYPE_EQ(typename tuple_join<>::type,             t_empty);
    D_TEST_TYPE_EQ(typename tuple_join<t_i>::type,          t_i);
    D_TEST_TYPE_EQ(typename tuple_join<t_empty, t_i>::type, t_i);
    D_TEST_TYPE_EQ(typename tuple_join<t_i, t_empty>::type, t_i);

    // basic concatenation
    D_TEST_TYPE_EQ(typename tuple_join<t_i, t_c>::type, t_ic);

    // associativity: (i ++ c) ++ l  ==  i ++ (c ++ l)
    typedef typename tuple_join<typename tuple_join<t_i, t_c>::type,
                                t_l>::type
        left_assoc_3;
    typedef typename tuple_join<t_i,
                                typename tuple_join<t_c, t_l>::type
                               >::type
        right_assoc_3;
    D_TEST_TYPE_EQ(left_assoc_3,  t_icl);
    D_TEST_TYPE_EQ(right_assoc_3, t_icl);
    D_TEST_TYPE_EQ(left_assoc_3,  right_assoc_3);

    // explicit 3-operand join — catches N-ary decomposition bugs
    // that do not show up in binary tests
    D_TEST_TYPE_EQ(typename tuple_join<t_i, t_c, t_l>::type, t_icl);

    // explicit 4-operand join against both association orders
    typedef typename tuple_join<
                typename tuple_join<
                    typename tuple_join<t_i, t_c>::type,
                    t_l>::type,
                t_d>::type
        left_assoc_4;
    typedef typename tuple_join<
                t_i,
                typename tuple_join<
                    t_c,
                    typename tuple_join<t_l, t_d>::type>::type
                >::type
        right_assoc_4;
    D_TEST_TYPE_EQ(typename tuple_join<t_i, t_c, t_l, t_d>::type,
                   t_icld);
    D_TEST_TYPE_EQ(left_assoc_4,  t_icld);
    D_TEST_TYPE_EQ(right_assoc_4, t_icld);

    // empty tuple is an interior identity across N operands
    D_TEST_TYPE_EQ(typename tuple_join<t_i, t_empty, t_c>::type,
                   t_ic);
    D_TEST_TYPE_EQ(typename tuple_join<t_empty, t_i, t_empty,
                                        t_c, t_empty>::type,
                   t_ic);

    // joining many empty tuples yields the empty tuple
    D_TEST_TYPE_EQ(typename tuple_join<t_empty, t_empty, t_empty,
                                        t_empty>::type,
                   t_empty);

    D_TEST_FIRE(_handler, on_compile_check, "tuple_join/all-checks");

    return true;
}


NS_END  // testing
NS_END  // djinterp
