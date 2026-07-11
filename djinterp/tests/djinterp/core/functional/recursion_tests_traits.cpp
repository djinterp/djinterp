// djinterp [test]  recursion_tests_traits.cpp
//   Sections II-IV -- recursive_traits / corecursive_traits, the detection
//   traits and concepts, and the canonical mu<F> instances.

// std
#include <type_traits>
#include <vector>
// djinterp
#include "recursion_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_recursive_traits_mu
  The mu<F> fold-side instance is well-formed.
  Tests the following:
  - is_specialized is true
  - base<A> is F<A> for the fixture functors
*/
bool
tests_recursive_traits_mu()
{
    static_assert(recursive_traits<mu<nat_f> >::is_specialized::value,
                  "mu<nat_f> is recursive");

    static_assert(
        std::is_same<recursive_traits<mu<nat_f> >::base<int>,
                     nat_f<int> >::value, "nat base<int>");
    static_assert(
        std::is_same<recursive_traits<mu<list_f> >::base<int>,
                     list_f<int> >::value, "list base<int>");
    static_assert(
        std::is_same<recursive_traits<mu<tree_f> >::base<double>,
                     tree_f<double> >::value, "tree base<double>");

    return true;
}


/*
tests_recursive_traits_project
  project peels exactly one layer, equal to out().
  Tests the following:
  - project of a Zero reports the Zero arm
  - project of a Succ^3 reports Succ and a depth-2 child
*/
bool
tests_recursive_traits_project()
{
    bool ok = true;
    using rec = recursive_traits<mu<nat_f> >;

    mu<nat_f> z = mu<nat_f>::In(nat_zero<mu<nat_f> >());
    nat_f<mu<nat_f> > lz = rec::project(z);
    ok = ok && (lz.is_zero == true);

    mu<nat_f> n = make_nat(3);
    nat_f<mu<nat_f> > ln = rec::project(n);
    ok = ok && (ln.is_zero == false);
    ok = ok && (nat_to_unsigned(ln.succ) == 2);   // one layer peeled

    return ok;
}


/*
tests_corecursive_traits_mu
  The mu<F> build-side instance is well-formed.
  Tests the following:
  - is_specialized is true
  - base<A> is F<A>
*/
bool
tests_corecursive_traits_mu()
{
    static_assert(corecursive_traits<mu<nat_f> >::is_specialized::value,
                  "mu<nat_f> is corecursive");

    static_assert(
        std::is_same<corecursive_traits<mu<nat_f> >::base<int>,
                     nat_f<int> >::value, "nat base<int>");
    static_assert(
        std::is_same<corecursive_traits<mu<list_f> >::base<char>,
                     list_f<char> >::value, "list base<char>");

    return true;
}


/*
tests_corecursive_traits_embed
  embed builds exactly one layer, invertible by out().
  Tests the following:
  - embed of a Zero layer yields a value whose out() is that Zero
  - embed of a Succ layer yields the expected depth
*/
bool
tests_corecursive_traits_embed()
{
    bool ok = true;
    using corec = corecursive_traits<mu<nat_f> >;

    mu<nat_f> z = corec::embed(nat_zero<mu<nat_f> >());
    ok = ok && (z.empty() == false);
    ok = ok && (z.out().is_zero == true);

    mu<nat_f> two = corec::embed(nat_succ<mu<nat_f> >(make_nat(1)));
    ok = ok && (nat_to_unsigned(two) == 2);

    return ok;
}


/*
tests_traits_roundtrip
  project and embed are mutual inverses on one layer.
  Tests the following:
  - embed(project(m)) reproduces m (same depth)
  - project(embed(layer)) reproduces the layer
*/
bool
tests_traits_roundtrip()
{
    bool ok = true;
    using rec   = recursive_traits<mu<nat_f> >;
    using corec = corecursive_traits<mu<nat_f> >;

    // embed . project == id (one layer).
    mu<nat_f> m = make_nat(5);
    mu<nat_f> back = corec::embed(rec::project(m));
    ok = ok && (nat_to_unsigned(back) == 5);

    // project . embed == id (one layer).
    nat_f<mu<nat_f> > layer = nat_succ<mu<nat_f> >(make_nat(2));
    nat_f<mu<nat_f> > layer2 = rec::project(corec::embed(layer));
    ok = ok && (layer2.is_zero == false);
    ok = ok && (nat_to_unsigned(layer2.succ) == 2);

    return ok;
}


/*
tests_is_recursive
  is_recursive recognises fold-side carriers and rejects the rest.
  Tests the following:
  - true for mu<F>
  - false for an unregistered struct and a scalar
  - cv / ref are stripped before the test
  - is_recursive_v agrees (where available); the concept agrees (C++20)
*/
bool
tests_is_recursive()
{
    bool ok = true;

    ok = ok && (is_recursive<mu<nat_f> >::value);
    ok = ok && (is_recursive<mu<tree_f> >::value);
    ok = ok && (!is_recursive<not_recursive>::value);
    ok = ok && (!is_recursive<int>::value);

    // cv / ref decay.
    ok = ok && (is_recursive<const mu<nat_f>& >::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_recursive_v<mu<nat_f> > == is_recursive<mu<nat_f> >::value);
    ok = ok && (is_recursive_v<int> == false);
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Recursive<mu<nat_f> >, "mu is Recursive");
    static_assert(!Recursive<int>, "int is not Recursive");
#endif

    return ok;
}


/*
tests_is_corecursive
  is_corecursive recognises build-side carriers and rejects the rest.
  Tests the following:
  - true for mu<F>
  - false for an unregistered struct and a scalar
  - cv / ref are stripped before the test
  - is_corecursive_v agrees (where available); the concept agrees (C++20)
*/
bool
tests_is_corecursive()
{
    bool ok = true;

    ok = ok && (is_corecursive<mu<nat_f> >::value);
    ok = ok && (is_corecursive<mu<list_f> >::value);
    ok = ok && (!is_corecursive<not_recursive>::value);
    ok = ok && (!is_corecursive<int>::value);

    // cv / ref decay.
    ok = ok && (is_corecursive<mu<nat_f>& >::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_corecursive_v<mu<nat_f> > ==
                is_corecursive<mu<nat_f> >::value);
    ok = ok && (is_corecursive_v<not_recursive> == false);
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Corecursive<mu<nat_f> >, "mu is Corecursive");
    static_assert(!Corecursive<int>, "int is not Corecursive");
#endif

    return ok;
}


/*
tests_custom_carrier_registration
  A native carrier (std::vector<int>) is made foldable and buildable purely by
  specialising the traits -- no rewrite as a fixed point.
  Tests the following:
  - is_recursive / is_corecursive hold for the registered vector
  - base<A> is the list functor
  - project exposes head / tail; embed reassembles them
*/
bool
tests_custom_carrier_registration()
{
    bool ok = true;

    ok = ok && (is_recursive<std::vector<int> >::value);
    ok = ok && (is_corecursive<std::vector<int> >::value);

    static_assert(
        std::is_same<recursive_traits<std::vector<int> >::base<int>,
                     list_f<int> >::value, "vector base is list_f");

    // project peels the front element as head, the rest as the tail hole.
    std::vector<int> v;
    v.push_back(10);
    v.push_back(20);
    list_f<std::vector<int> > layer =
        recursive_traits<std::vector<int> >::project(v);
    ok = ok && (layer.is_nil == false);
    ok = ok && (layer.head == 10);
    ok = ok && (layer.tail.size() == 1);
    ok = ok && (layer.tail[0] == 20);

    // project of the empty vector is Nil.
    ok = ok && (recursive_traits<std::vector<int> >::project(
                    std::vector<int>()).is_nil == true);

    // embed reassembles a layer back into a vector.
    std::vector<int> rebuilt =
        corecursive_traits<std::vector<int> >::embed(layer);
    ok = ok && (rebuilt.size() == 2);
    ok = ok && (rebuilt[0] == 10);
    ok = ok && (rebuilt[1] == 20);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
