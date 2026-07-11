// djinterp [test]  polynomial_tests_functor.cpp
//   Section III -- functor_traits specialisations, functor_map, the functor
//   laws, and the cata / ana integration those instances exist to serve.

// std
#include <type_traits>
// djinterp
#include "polynomial_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_functor_poly
  Confirms every poly_* carrier is a Functor.
  Tests the following:
  - is_functor true for var / unit / const / sum / product
  - is_functor strips cv / ref before testing
  - is_functor_v agrees with the trait (where variable templates exist)
*/
bool
tests_is_functor_poly()
{
    bool ok = true;

    ok = ok && (is_functor<poly_var<int> >::value);
    ok = ok && (is_functor<poly_unit<int> >::value);
    ok = ok && (is_functor<poly_const<char, int> >::value);
    ok = ok && (is_functor<poly_sum<poly_var<int>, poly_unit<int> > >::value);
    ok = ok && (is_functor<
                    poly_product<poly_var<int>,
                                 poly_const<char, int> > >::value);

    // cv / ref decay.
    ok = ok && (is_functor<const poly_var<int>& >::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_functor_v<poly_var<int> > ==
                is_functor<poly_var<int> >::value);
    ok = ok && (is_functor_v<poly_unit<int> > == true);
#endif

    return ok;
}


/*
tests_is_functor_negative
  Confirms is_functor is false for non-functors, and resolves cleanly (no hard
  error) on the undefined primary.
  Tests the following:
  - an unrelated user struct
  - scalars
*/
bool
tests_is_functor_negative()
{
    bool ok = true;

    ok = ok && (!is_functor<not_a_functor>::value);
    ok = ok && (!is_functor<int>::value);
    ok = ok && (!is_functor<double>::value);

    return ok;
}


/*
tests_functor_value_type
  Confirms functor_value_type_t reports each carrier's inner X.
  Tests the following:
  - the inner value type of every poly_* carrier
*/
bool
tests_functor_value_type()
{
    static_assert(std::is_same<functor_value_type_t<poly_var<int> >,
                               int>::value, "var");
    static_assert(std::is_same<functor_value_type_t<poly_unit<int> >,
                               int>::value, "unit");
    static_assert(std::is_same<functor_value_type_t<poly_const<char, int> >,
                               int>::value, "const");
    static_assert(
        std::is_same<functor_value_type_t<
                         poly_sum<poly_var<int>, poly_unit<int> > >,
                     int>::value, "sum");
    static_assert(
        std::is_same<functor_value_type_t<
                         poly_product<poly_var<int>,
                                      poly_const<char, int> > >,
                     int>::value, "product");

    return true;
}


/*
tests_functor_rebind
  Confirms functor_traits<F>::rebind<U> re-parameterises each carrier over U,
  recursing structurally through the composites.
  Tests the following:
  - leaf rebinds (const keeps its constant type; unit/var swap X)
  - composite rebinds map U through both arms / components
*/
bool
tests_functor_rebind()
{
    static_assert(
        std::is_same<functor_traits<poly_var<int> >::rebind<double>,
                     poly_var<double> >::value, "var rebind");
    static_assert(
        std::is_same<functor_traits<poly_unit<int> >::rebind<double>,
                     poly_unit<double> >::value, "unit rebind");
    static_assert(
        std::is_same<functor_traits<poly_const<char, int> >::rebind<double>,
                     poly_const<char, double> >::value, "const rebind");
    static_assert(
        std::is_same<
            functor_traits<poly_sum<poly_var<int>, poly_unit<int> > >
                ::rebind<double>,
            poly_sum<poly_var<double>, poly_unit<double> > >::value,
        "sum rebind");
    static_assert(
        std::is_same<
            functor_traits<poly_product<poly_var<int>,
                                        poly_const<char, int> > >
                ::rebind<double>,
            poly_product<poly_var<double>, poly_const<char, double> > >::value,
        "product rebind");

    return true;
}


/*
tests_functor_map_poly_var
  Exercises functor_map over the recursive-position carrier.
  Tests the following:
  - a same-type map transforms the stored value
  - a type-changing map yields poly_var over the new type
  - the trait's map matches the free function
*/
bool
tests_functor_map_poly_var()
{
    bool ok = true;

    // same-type map.
    poly_var<int> v(5);
    auto r = functor_map(v, [](int _x){ return _x * 2; });
    ok = ok && (std::is_same<decltype(r), poly_var<int> >::value);
    ok = ok && (r.content == 10);

    // type-changing map.
    auto r2 = functor_map(poly_var<int>(3),
                          [](int _x){ return static_cast<double>(_x) + 0.5; });
    ok = ok && (std::is_same<decltype(r2), poly_var<double> >::value);
    ok = ok && close_enough(r2.content, 3.5);

    // trait map == free function.
    auto r3 = functor_traits<poly_var<int> >::map(
        poly_var<int>(4), [](int _x){ return _x + 1; });
    ok = ok && (r3.content == 5);

    return ok;
}


/*
tests_functor_map_poly_unit
  Exercises functor_map over the unit carrier.
  Tests the following:
  - the output is poly_unit over the function's (decayed) return type
  - the mapping function is never invoked (X is phantom)
*/
bool
tests_functor_map_poly_unit()
{
    bool ok = true;

    bool called = false;
    auto r = functor_map(poly_unit<int>(),
                         [&called](int _x){
                             called = true;
                             return static_cast<double>(_x);
                         });
    ok = ok && (std::is_same<decltype(r), poly_unit<double> >::value);
    ok = ok && (called == false);
    (void)r;

    // same-type map keeps poly_unit<int>.
    auto r2 = functor_map(poly_unit<int>(), [](int _x){ return _x; });
    ok = ok && (std::is_same<decltype(r2), poly_unit<int> >::value);

    return ok;
}


/*
tests_functor_map_poly_const
  Exercises functor_map over the constant carrier.
  Tests the following:
  - the output is poly_const<C, U> (constant kept, X swapped)
  - the constant payload threads through unchanged
  - the mapping function is never invoked (X is phantom)
*/
bool
tests_functor_map_poly_const()
{
    bool ok = true;

    bool called = false;
    auto r = functor_map(poly_const<char, int>('a'),
                         [&called](int _x){
                             called = true;
                             return static_cast<double>(_x);
                         });
    ok = ok && (std::is_same<decltype(r), poly_const<char, double> >::value);
    ok = ok && (r.content == 'a');
    ok = ok && (called == false);

    return ok;
}


/*
tests_functor_map_poly_sum
  Exercises functor_map over the binary sum.
  Tests the following:
  - a left-tagged value maps through the left arm, tag preserved
  - a right-tagged value maps through the right arm, tag preserved
  - the result type rebinds both arms
*/
bool
tests_functor_map_poly_sum()
{
    bool ok = true;

    typedef poly_var<int>        arm_l;
    typedef poly_const<char, int> arm_r;
    typedef poly_sum<arm_l, arm_r> sum_t;
    typedef poly_sum<poly_var<double>, poly_const<char, double> > out_t;

    // left-active.
    sum_t sl = sum_t::inj_left(arm_l(5));
    auto rl = functor_map(sl, [](int _x){ return static_cast<double>(_x); });
    ok = ok && (std::is_same<decltype(rl), out_t>::value);
    ok = ok && (rl.is_left == true);
    ok = ok && close_enough(rl.left.content, 5.0);

    // right-active (constant arm's payload preserved).
    sum_t sr = sum_t::inj_right(arm_r('q'));
    auto rr = functor_map(sr, [](int _x){ return static_cast<double>(_x); });
    ok = ok && (rr.is_left == false);
    ok = ok && (rr.right.content == 'q');

    return ok;
}


/*
tests_functor_map_poly_product
  Exercises functor_map over the binary product.
  Tests the following:
  - both components are mapped
  - the constant component's payload is preserved
  - the result type rebinds both components
*/
bool
tests_functor_map_poly_product()
{
    bool ok = true;

    typedef poly_var<int>        comp_a;
    typedef poly_const<char, int> comp_b;
    typedef poly_product<comp_a, comp_b> prod_t;
    typedef poly_product<poly_var<double>, poly_const<char, double> > out_t;

    prod_t p(comp_a(4), comp_b('m'));
    auto r = functor_map(p, [](int _x){ return static_cast<double>(_x); });
    ok = ok && (std::is_same<decltype(r), out_t>::value);
    ok = ok && close_enough(r.first.content, 4.0);
    ok = ok && (r.second.content == 'm');

    return ok;
}


/*
tests_functor_law_identity
  Confirms the first functor law: map(id) is structurally the identity.
  Tests the following:
  - mapping the identity over each carrier preserves shape and payload
*/
bool
tests_functor_law_identity()
{
    bool ok = true;

    // poly_var.
    auto rv = functor_map(poly_var<int>(7), [](int _x){ return _x; });
    ok = ok && (std::is_same<decltype(rv), poly_var<int> >::value);
    ok = ok && (rv.content == 7);

    // poly_const (payload preserved, X unchanged).
    auto rc = functor_map(poly_const<char, int>('a'), [](int _x){ return _x; });
    ok = ok && (rc.content == 'a');

    // poly_sum.
    typedef poly_sum<poly_var<int>, poly_unit<int> > sum_t;
    sum_t s = sum_t::inj_left(poly_var<int>(3));
    auto rs = functor_map(s, [](int _x){ return _x; });
    ok = ok && (rs.is_left && (rs.left.content == 3));

    // poly_product.
    poly_product<poly_var<int>, poly_var<int> > p(
        poly_var<int>(1), poly_var<int>(2));
    auto rp = functor_map(p, [](int _x){ return _x; });
    ok = ok && ((rp.first.content == 1) && (rp.second.content == 2));

    return ok;
}


/*
tests_functor_law_composition
  Confirms the second functor law: map(g . f) == map(g) . map(f).
  Tests the following:
  - the two routes agree on poly_var and poly_product
*/
bool
tests_functor_law_composition()
{
    bool ok = true;

    auto f = [](int _x){ return _x + 1; };
    auto g = [](int _x){ return _x * 2; };

    // poly_var: (5+1)*2 == 12 both ways.
    poly_var<int> v(5);
    auto lhs = functor_map(v, [f, g](int _x){ return g(f(_x)); });
    auto rhs = functor_map(functor_map(v, f), g);
    ok = ok && (lhs.content == rhs.content);
    ok = ok && (lhs.content == 12);

    // poly_product.
    poly_product<poly_var<int>, poly_var<int> > p(
        poly_var<int>(3), poly_var<int>(4));
    auto plhs = functor_map(p, [f, g](int _x){ return g(f(_x)); });
    auto prhs = functor_map(functor_map(p, f), g);
    ok = ok && (plhs.first.content  == prhs.first.content);
    ok = ok && (plhs.second.content == prhs.second.content);
    ok = ok && (plhs.first.content  == (3 + 1) * 2);

    return ok;
}


/*
tests_functor_cata_integration
  The payoff test: the poly functor instances are what recursion.hpp's cata /
  ana / hylo call to rebuild a layer, so folding a poly-built fixed point
  validates those instances end-to-end.  nat_f = 1 + X is the Peano functor.
  Tests the following:
  - cata[depth] over hand-built Nats (Zero, Succ Zero, Succ^3 Zero)
  - an ana-built Nat folded back by cata (roundtrip)
  - hylo refolding seed -> depth directly, no fixed point materialised
*/
bool
tests_functor_cata_integration()
{
    bool ok = true;

    typedef mu<nat_f> Nat;

    // hand-build 0, 1, 3.
    Nat zero  = Nat::In(nat_f<Nat>::inj_left(poly_unit<Nat>()));
    Nat one   = Nat::In(nat_f<Nat>::inj_right(poly_var<Nat>(zero)));
    Nat three = Nat::In(nat_f<Nat>::inj_right(poly_var<Nat>(
                    Nat::In(nat_f<Nat>::inj_right(poly_var<Nat>(one))))));

    // the depth algebra phi : nat_f<int> -> int.
    auto depth = [](const nat_f<int>& _layer) -> int
    {
        return _layer.is_left ? 0 : (1 + _layer.right.content);
    };

    ok = ok && (cata<int, Nat>(depth, zero)  == 0);
    ok = ok && (cata<int, Nat>(depth, one)   == 1);
    ok = ok && (cata<int, Nat>(depth, three) == 3);

    // the build coalgebra psi : int -> nat_f<int>.
    auto build = [](int _n) -> nat_f<int>
    {
        return (_n == 0)
                   ? nat_f<int>::inj_left(poly_unit<int>())
                   : nat_f<int>::inj_right(poly_var<int>(_n - 1));
    };

    // ana then cata roundtrips the count.
    ok = ok && (cata<int, Nat>(depth, ana<Nat, int>(build, 5)) == 5);
    ok = ok && (cata<int, Nat>(depth, ana<Nat, int>(build, 0)) == 0);

    // hylo fuses the two with no Nat built in between.
    ok = ok && ((hylo<int, int, nat_f>(depth, build, 5)) == 5);
    ok = ok && ((hylo<int, int, nat_f>(depth, build, 0)) == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
