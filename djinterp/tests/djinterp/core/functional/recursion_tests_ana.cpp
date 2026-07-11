// djinterp [test]  recursion_tests_ana.cpp
//   Section VI -- ana[psi], the universal unfold.

// std
#include <vector>
#include <type_traits>
// djinterp
#include "recursion_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_ana_nat
  Unfolds an int into a Peano value.
  Tests the following:
  - seed 0 builds Zero (depth 0)
  - seed n builds Succ^n (depth n), across several n
*/
bool
tests_ana_nat()
{
    bool ok = true;

    auto grow = [](int _n) -> nat_f<int>
    {
        return (_n <= 0) ? nat_zero<int>() : nat_succ<int>(_n - 1);
    };

    ok = ok && (nat_to_unsigned(ana<mu<nat_f>, int>(grow, 0)) == 0u);
    ok = ok && (nat_to_unsigned(ana<mu<nat_f>, int>(grow, 1)) == 1u);
    ok = ok && (nat_to_unsigned(ana<mu<nat_f>, int>(grow, 4)) == 4u);
    ok = ok && (nat_to_unsigned(ana<mu<nat_f>, int>(grow, 9)) == 9u);

    return ok;
}


/*
tests_ana_list_range
  Unfolds a seed into a descending list.
  Tests the following:
  - seed 0 builds the empty list
  - seed n builds [n, n-1, ..., 1]
*/
bool
tests_ana_list_range()
{
    bool ok = true;

    auto down = [](int _n) -> list_f<int>
    {
        return (_n <= 0) ? list_nil<int>() : list_cons<int>(_n, _n - 1);
    };

    ok = ok && (list_to_vector(ana<mu<list_f>, int>(down, 0)).empty());
    ok = ok && (list_to_vector(ana<mu<list_f>, int>(down, 4))
                == std::vector<int>({4, 3, 2, 1}));

    return ok;
}


/*
tests_ana_tree_balanced
  Unfolds a depth seed into a full binary tree -- a coalgebra with TWO holes
  (Branch seeds both children).
  Tests the following:
  - seed d builds a tree of height d
  - that tree has 2^d leaves (all value 1)
*/
bool
tests_ana_tree_balanced()
{
    bool ok = true;

    auto split = [](int _d) -> tree_f<int>
    {
        return (_d <= 0) ? tree_leaf<int>(1)
                         : tree_branch<int>(_d - 1, _d - 1);
    };

    mu<tree_f> t3 = ana<mu<tree_f>, int>(split, 3);
    ok = ok && (tree_height_manual(t3) == 3);
    ok = ok && (tree_leaf_sum_manual(t3) == 8);    // 2^3 leaves of 1

    mu<tree_f> t0 = ana<mu<tree_f>, int>(split, 0);
    ok = ok && (tree_height_manual(t0) == 0);
    ok = ok && (tree_leaf_sum_manual(t0) == 1);

    return ok;
}


/*
tests_ana_native_vector
  Unfolds a seed directly into a std::vector<int>, via its corecursive_traits
  registration -- ana builds a native carrier, not a mu.
  Tests the following:
  - seed 0 builds the empty vector
  - seed n builds [n, n-1, ..., 1] as a real vector
*/
bool
tests_ana_native_vector()
{
    bool ok = true;

    auto down = [](int _n) -> list_f<int>
    {
        return (_n <= 0) ? list_nil<int>() : list_cons<int>(_n, _n - 1);
    };

    ok = ok && (ana<std::vector<int>, int>(down, 0).empty());

    std::vector<int> v = ana<std::vector<int>, int>(down, 5);
    ok = ok && (v == std::vector<int>({5, 4, 3, 2, 1}));

    return ok;
}


/*
tests_ana_base_case
  When the coalgebra yields a hole-less layer immediately, ana builds a single
  base layer.
  Tests the following:
  - a Zero-producing coalgebra builds a non-empty Zero value
  - a Nil-producing coalgebra builds the empty list
*/
bool
tests_ana_base_case()
{
    bool ok = true;

    auto grow = [](int _n) -> nat_f<int>
    {
        return (_n <= 0) ? nat_zero<int>() : nat_succ<int>(_n - 1);
    };
    mu<nat_f> z = ana<mu<nat_f>, int>(grow, 0);
    ok = ok && (z.empty() == false);
    ok = ok && (z.out().is_zero == true);

    auto down = [](int _n) -> list_f<int>
    {
        return (_n <= 0) ? list_nil<int>() : list_cons<int>(_n, _n - 1);
    };
    mu<list_f> nil = ana<mu<list_f>, int>(down, 0);
    ok = ok && (nil.out().is_nil == true);

    return ok;
}


/*
tests_ana_produces_structure
  ana lays down the expected layers, checked by walking out() directly (not by
  a fold), so the unfold is validated independently of cata.
  Tests the following:
  - ana nat(3) is a Succ chain of length 3 ending in Zero
  - ana list [3,2,1] has exactly those heads then Nil
*/
bool
tests_ana_produces_structure()
{
    bool ok = true;

    auto grow = [](int _n) -> nat_f<int>
    {
        return (_n <= 0) ? nat_zero<int>() : nat_succ<int>(_n - 1);
    };
    mu<nat_f> n = ana<mu<nat_f>, int>(grow, 3);
    ok = ok && (n.out().is_zero == false);
    ok = ok && (n.out().succ.out().is_zero == false);
    ok = ok && (n.out().succ.out().succ.out().is_zero == false);
    ok = ok && (n.out().succ.out().succ.out().succ.out().is_zero == true);

    auto down = [](int _n) -> list_f<int>
    {
        return (_n <= 0) ? list_nil<int>() : list_cons<int>(_n, _n - 1);
    };
    mu<list_f> l = ana<mu<list_f>, int>(down, 3);
    ok = ok && (l.out().head == 3);
    ok = ok && (l.out().tail.out().head == 2);
    ok = ok && (l.out().tail.out().tail.out().head == 1);
    ok = ok && (l.out().tail.out().tail.out().tail.out().is_nil == true);

    return ok;
}


/*
tests_ana_coalgebra_forms
  The coalgebra may be any callable.
  Tests the following:
  - a lambda, a free function, and a function object all unfold identically
*/
bool
tests_ana_coalgebra_forms()
{
    bool ok = true;

    // lambda
    mu<nat_f> a = ana<mu<nat_f>, int>(
        [](int _n){ return (_n <= 0) ? nat_zero<int>()
                                     : nat_succ<int>(_n - 1); }, 5);
    // free function
    mu<nat_f> b = ana<mu<nat_f>, int>(&nat_succ_fn, 5);
    // function object
    mu<nat_f> c = ana<mu<nat_f>, int>(nat_succ_co(), 5);

    ok = ok && (nat_to_unsigned(a) == 5u);
    ok = ok && (nat_to_unsigned(b) == 5u);
    ok = ok && (nat_to_unsigned(c) == 5u);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
