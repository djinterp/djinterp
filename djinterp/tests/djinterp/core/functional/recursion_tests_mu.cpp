// djinterp [test]  recursion_tests_mu.cpp
//   Section I -- mu<F>, the least fixed point (ctor, In, out, empty, sharing).

// std
#include <type_traits>
// djinterp
#include "recursion_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_mu_default_empty
  A default-constructed mu holds no layer.
  Tests the following:
  - empty() is true after default construction
*/
bool
tests_mu_default_empty()
{
    bool ok = true;

    mu<nat_f> m;
    ok = ok && (m.empty() == true);

    return ok;
}


/*
tests_mu_in_nonempty
  In boxes a layer, yielding a non-empty value.
  Tests the following:
  - empty() is false after In
*/
bool
tests_mu_in_nonempty()
{
    bool ok = true;

    mu<nat_f> z = mu<nat_f>::In(nat_zero<mu<nat_f> >());
    ok = ok && (z.empty() == false);

    mu<nat_f> s = mu<nat_f>::In(nat_succ<mu<nat_f> >(z));
    ok = ok && (s.empty() == false);

    return ok;
}


/*
tests_mu_out_returns_layer
  out() returns the boxed F-layer.
  Tests the following:
  - out() of a boxed Zero reports the Zero arm
  - out() of a boxed Succ reports the Succ arm and its child
*/
bool
tests_mu_out_returns_layer()
{
    bool ok = true;

    mu<nat_f> z = mu<nat_f>::In(nat_zero<mu<nat_f> >());
    ok = ok && (z.out().is_zero == true);

    mu<nat_f> s = mu<nat_f>::In(nat_succ<mu<nat_f> >(z));
    ok = ok && (s.out().is_zero == false);
    ok = ok && (s.out().succ.out().is_zero == true);   // child is the Zero

    return ok;
}


/*
tests_mu_layer_type
  The layer_type alias is exactly F applied to the fixed point.
  Tests the following:
  - mu<F>::layer_type == F<mu<F>> for each fixture functor
*/
bool
tests_mu_layer_type()
{
    static_assert(
        std::is_same<mu<nat_f>::layer_type, nat_f<mu<nat_f> > >::value,
        "nat_f layer_type");
    static_assert(
        std::is_same<mu<list_f>::layer_type, list_f<mu<list_f> > >::value,
        "list_f layer_type");
    static_assert(
        std::is_same<mu<tree_f>::layer_type, tree_f<mu<tree_f> > >::value,
        "tree_f layer_type");

    return true;
}


/*
tests_mu_sharing
  The layer sits behind a shared_ptr: copies share it, and In boxes a COPY of
  its argument (later mutation of the source layer does not leak in).
  Tests the following:
  - a copy is non-empty and observes the same layer content
  - assignment behaves likewise
  - mutating the source layer after In leaves the boxed layer unchanged
*/
bool
tests_mu_sharing()
{
    bool ok = true;

    // copy shares the layer content.
    mu<nat_f> a = mu<nat_f>::In(nat_succ<mu<nat_f> >(
        mu<nat_f>::In(nat_zero<mu<nat_f> >())));
    mu<nat_f> b = a;
    ok = ok && (b.empty() == false);
    ok = ok && (b.out().is_zero == a.out().is_zero);

    // assignment likewise.
    mu<nat_f> c;
    c = a;
    ok = ok && (c.empty() == false);
    ok = ok && (c.out().is_zero == false);

    // In copies its argument: mutating the source afterwards is invisible.
    nat_f<mu<nat_f> > layer = nat_zero<mu<nat_f> >();   // Zero
    mu<nat_f> boxed = mu<nat_f>::In(layer);
    layer.is_zero = false;                              // tamper with the source
    ok = ok && (boxed.out().is_zero == true);           // boxed copy intact

    return ok;
}


/*
tests_mu_nested_navigation
  A multi-layer value can be walked one layer at a time through out(), and the
  same works for a functor with two holes.
  Tests the following:
  - Succ(Succ(Succ(Zero))) walks to depth 3 then Zero
  - a Branch exposes both children through out()
*/
bool
tests_mu_nested_navigation()
{
    bool ok = true;

    // walk a Peano chain of length 3.
    mu<nat_f> n = make_nat(3);
    ok = ok && (n.out().is_zero == false);
    ok = ok && (n.out().succ.out().is_zero == false);
    ok = ok && (n.out().succ.out().succ.out().is_zero == false);
    ok = ok && (n.out().succ.out().succ.out().succ.out().is_zero == true);

    // both children of a Branch are reachable.
    mu<tree_f> t = make_branch(make_leaf(4), make_leaf(9));
    ok = ok && (t.out().is_leaf == false);
    ok = ok && (t.out().left.out().is_leaf == true);
    ok = ok && (t.out().left.out().value == 4);
    ok = ok && (t.out().right.out().value == 9);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
