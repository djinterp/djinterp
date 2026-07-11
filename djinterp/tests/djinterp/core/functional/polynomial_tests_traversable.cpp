// djinterp [test]  polynomial_tests_traversable.cpp
//   Section IV -- traversable_traits specialisations, traverse, and sequence.
//   The effect threaded is the header's test_maybe<T>.

// std
#include <type_traits>
// djinterp
#include "polynomial_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_traversable_poly
  Confirms the carriers that ARE Traversable.
  Tests the following:
  - is_traversable true for poly_var / poly_unit / poly_const
  - is_traversable_v agrees (where variable templates exist)
*/
bool
tests_is_traversable_poly()
{
    bool ok = true;

    ok = ok && (is_traversable<poly_var<int> >::value);
    ok = ok && (is_traversable<poly_unit<int> >::value);
    ok = ok && (is_traversable<poly_const<char, int> >::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_traversable_v<poly_var<int> > ==
                is_traversable<poly_var<int> >::value);
#endif

    return ok;
}


/*
tests_is_traversable_negative
  Confirms the functor / traversable asymmetry: poly_sum and poly_product are
  Functors (Section III) but were deliberately NOT given traversable_traits, so
  they are not Traversable.  Also covers non-carriers.
  Tests the following:
  - is_traversable false for poly_sum and poly_product
  - those same types are nonetheless Functors (contrast)
  - is_traversable false for an unrelated struct and a scalar
*/
bool
tests_is_traversable_negative()
{
    bool ok = true;

    typedef poly_sum<poly_var<int>, poly_unit<int> >         sum_t;
    typedef poly_product<poly_var<int>, poly_const<char,int> > prod_t;

    // the asymmetry: Functor yes, Traversable no.
    ok = ok && (!is_traversable<sum_t>::value);
    ok = ok && (!is_traversable<prod_t>::value);
    ok = ok && (is_functor<sum_t>::value);
    ok = ok && (is_functor<prod_t>::value);

    // non-carriers.
    ok = ok && (!is_traversable<not_a_functor>::value);
    ok = ok && (!is_traversable<int>::value);

    return ok;
}


/*
tests_traversable_value_type
  Confirms traversable_value_type_t reports the inner X of each Traversable
  carrier.
  Tests the following:
  - the inner value type of poly_var / poly_unit / poly_const
*/
bool
tests_traversable_value_type()
{
    static_assert(std::is_same<traversable_value_type_t<poly_var<int> >,
                               int>::value, "var");
    static_assert(std::is_same<traversable_value_type_t<poly_unit<int> >,
                               int>::value, "unit");
    static_assert(std::is_same<traversable_value_type_t<poly_const<char,int> >,
                               int>::value, "const");

    return true;
}


/*
tests_traverse_poly_var
  Exercises traverse over the recursive position, threading test_maybe.
  Tests the following:
  - a present effect lifts the mapped value into F<poly_var<B>>
  - an absent effect short-circuits (nothing propagates)
  - a type-changing effect rebinds the carrier under the effect
*/
bool
tests_traverse_poly_var()
{
    bool ok = true;

    // present effect -> just(poly_var{mapped}).
    auto r = traverse(poly_var<int>(5),
                      [](int _x){ return t_just(_x + 1); });
    ok = ok && (std::is_same<decltype(r),
                             test_maybe<poly_var<int> > >::value);
    ok = ok && (r.has == true);
    ok = ok && (r.val.content == 6);

    // absent effect -> nothing.
    auto rn = traverse(poly_var<int>(5),
                       [](int _x){ (void)_x; return t_nothing<int>(); });
    ok = ok && (rn.has == false);

    // type-changing effect.
    auto rd = traverse(poly_var<int>(5),
                       [](int _x){
                           return t_just(static_cast<double>(_x) + 0.5);
                       });
    ok = ok && (std::is_same<decltype(rd),
                             test_maybe<poly_var<double> > >::value);
    ok = ok && (rd.has == true);
    ok = ok && close_enough(rd.val.content, 5.5);

    return ok;
}


/*
tests_traverse_poly_unit
  Exercises traverse over the unit carrier.
  Tests the following:
  - the effect function is never invoked (no X inhabitants)
  - the result is the empty unit lifted via pure -- present regardless of what
    the (uncalled) function would have produced
  - the result type is F<poly_unit<inner>> keyed on the function's return
*/
bool
tests_traverse_poly_unit()
{
    bool ok = true;

    bool called = false;
    auto r = traverse(poly_unit<int>(),
                      [&called](int _x){
                          called = true;
                          (void)_x;
                          return t_nothing<double>();   // would be 'absent'...
                      });
    ok = ok && (called == false);                       // ...but never called
    ok = ok && (std::is_same<decltype(r),
                             test_maybe<poly_unit<double> > >::value);
    ok = ok && (r.has == true);                         // pure => present

    return ok;
}


/*
tests_traverse_poly_const
  Exercises traverse over the constant carrier.
  Tests the following:
  - the effect function is never invoked (X is phantom)
  - the constant payload threads through unchanged
  - the result is present (pure) with type F<poly_const<C, inner>>
*/
bool
tests_traverse_poly_const()
{
    bool ok = true;

    bool called = false;
    auto r = traverse(poly_const<char, int>('z'),
                      [&called](int _x){
                          called = true;
                          (void)_x;
                          return t_nothing<double>();
                      });
    ok = ok && (called == false);
    ok = ok && (std::is_same<decltype(r),
                             test_maybe<poly_const<char, double> > >::value);
    ok = ok && (r.has == true);
    ok = ok && (r.val.content == 'z');

    return ok;
}


/*
tests_sequence_poly_var
  Exercises sequence = traverse(., id) over the recursive position, inverting
  poly_var<F<A>> into F<poly_var<A>>.
  Tests the following:
  - a present inner effect inverts to a present F holding poly_var<A>
  - an absent inner effect short-circuits
*/
bool
tests_sequence_poly_var()
{
    bool ok = true;

    // present -> just(poly_var{7}).
    auto r = sequence(poly_var<test_maybe<int> >(t_just(7)));
    ok = ok && (std::is_same<decltype(r),
                             test_maybe<poly_var<int> > >::value);
    ok = ok && (r.has == true);
    ok = ok && (r.val.content == 7);

    // absent -> nothing.
    auto rn = sequence(poly_var<test_maybe<int> >(t_nothing<int>()));
    ok = ok && (rn.has == false);

    return ok;
}


/*
tests_sequence_poly_unit_const
  Exercises sequence over the effect-free carriers.
  Tests the following:
  - sequence(poly_unit<F<A>>) is a present F holding poly_unit<A>
  - sequence(poly_const<C, F<A>>) is a present F, constant preserved
*/
bool
tests_sequence_poly_unit_const()
{
    bool ok = true;

    // unit: no effect to sequence -> present.
    auto ru = sequence(poly_unit<test_maybe<int> >());
    ok = ok && (std::is_same<decltype(ru),
                             test_maybe<poly_unit<int> > >::value);
    ok = ok && (ru.has == true);

    // const: constant threads through, present.
    auto rc = sequence(poly_const<char, test_maybe<int> >('k'));
    ok = ok && (std::is_same<decltype(rc),
                             test_maybe<poly_const<char, int> > >::value);
    ok = ok && (rc.has == true);
    ok = ok && (rc.val.content == 'k');

    return ok;
}


NS_END  // testing
NS_END  // djinterp
