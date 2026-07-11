// djinterp [test]  recursion_tests_hylo.cpp
//   Section VII -- hylo, the deforested refold (cata . ana, no mu built).

// std
#include <vector>
#include <type_traits>
// djinterp
#include "recursion_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_hylo_nat_depth
  Refolds an int seed to a depth without building a Peano value.
  Tests the following:
  - seed 0 refolds to 0 (immediate base layer)
  - seed n refolds to n
*/
bool
tests_hylo_nat_depth()
{
    bool ok = true;

    auto grow  = [](int _n) -> nat_f<int>
    {
        return (_n <= 0) ? nat_zero<int>() : nat_succ<int>(_n - 1);
    };
    auto depth = [](const nat_f<int>& _l) -> int
    {
        return _l.is_zero ? 0 : (1 + _l.succ);
    };

    ok = ok && (hylo<int, int, nat_f>(depth, grow, 0) == 0);
    ok = ok && (hylo<int, int, nat_f>(depth, grow, 7) == 7);

    return ok;
}


/*
tests_hylo_list_sum
  Refolds a seed into a sum through the list functor, no list materialised.
  Tests the following:
  - seed 0 refolds to 0
  - seed n refolds to n + (n-1) + ... + 1
*/
bool
tests_hylo_list_sum()
{
    bool ok = true;

    auto down = [](int _n) -> list_f<int>
    {
        return (_n <= 0) ? list_nil<int>() : list_cons<int>(_n, _n - 1);
    };
    auto sum = [](const list_f<int>& _l) -> int
    {
        return _l.is_nil ? 0 : (_l.head + _l.tail);
    };

    ok = ok && (hylo<int, int, list_f>(sum, down, 0) == 0);
    ok = ok && (hylo<int, int, list_f>(sum, down, 5) == 15);

    return ok;
}


/*
tests_hylo_factorial
  The textbook hylomorphism: unfold [n, n-1, ..., 1] and fold by product, fused.
  Tests the following:
  - factorial(0) == 1 (empty product)
  - factorial(5) == 120
*/
bool
tests_hylo_factorial()
{
    bool ok = true;

    auto count_down = [](int _n) -> list_f<int>
    {
        return (_n <= 0) ? list_nil<int>() : list_cons<int>(_n, _n - 1);
    };
    auto product = [](const list_f<int>& _l) -> int
    {
        return _l.is_nil ? 1 : (_l.head * _l.tail);
    };

    ok = ok && (hylo<int, int, list_f>(product, count_down, 0) == 1);
    ok = ok && (hylo<int, int, list_f>(product, count_down, 5) == 120);

    return ok;
}


/*
tests_hylo_tree
  Refolds a depth seed through the two-hole tree functor.
  Tests the following:
  - seed d refolds to 2^d (the leaf count of the tree it would build)
*/
bool
tests_hylo_tree()
{
    bool ok = true;

    auto split = [](int _d) -> tree_f<int>
    {
        return (_d <= 0) ? tree_leaf<int>(1)
                         : tree_branch<int>(_d - 1, _d - 1);
    };
    auto lsum = [](const tree_f<int>& _l) -> int
    {
        return _l.is_leaf ? _l.value : (_l.left + _l.right);
    };

    ok = ok && (hylo<int, int, tree_f>(lsum, split, 0) == 1);
    ok = ok && (hylo<int, int, tree_f>(lsum, split, 3) == 8);
    ok = ok && (hylo<int, int, tree_f>(lsum, split, 4) == 16);

    return ok;
}


/*
tests_hylo_equals_cata_ana
  hylo[phi, psi] agrees with cata[phi] . ana[psi] -- the deforested refold and
  the materialised one compute the same value.
  Tests the following:
  - equality for the Peano depth refold
  - equality for the list sum refold
*/
bool
tests_hylo_equals_cata_ana()
{
    bool ok = true;

    // Peano.
    auto grow  = [](int _n) -> nat_f<int>
    {
        return (_n <= 0) ? nat_zero<int>() : nat_succ<int>(_n - 1);
    };
    auto depth = [](const nat_f<int>& _l) -> int
    {
        return _l.is_zero ? 0 : (1 + _l.succ);
    };
    const int hn = hylo<int, int, nat_f>(depth, grow, 6);
    const int cn = cata<int, mu<nat_f> >(depth, ana<mu<nat_f>, int>(grow, 6));
    ok = ok && (hn == cn);
    ok = ok && (hn == 6);

    // list.
    auto down = [](int _n) -> list_f<int>
    {
        return (_n <= 0) ? list_nil<int>() : list_cons<int>(_n, _n - 1);
    };
    auto sum = [](const list_f<int>& _l) -> int
    {
        return _l.is_nil ? 0 : (_l.head + _l.tail);
    };
    const int hl = hylo<int, int, list_f>(sum, down, 5);
    const int cl = cata<int, mu<list_f> >(sum, ana<mu<list_f>, int>(down, 5));
    ok = ok && (hl == cl);
    ok = ok && (hl == 15);

    return ok;
}


/*
tests_hylo_base_case
  A seed the coalgebra maps straight to a hole-less layer refolds by one algebra
  application.
  Tests the following:
  - a Zero seed refolds to the base value
  - a Nil seed refolds to the empty-product / empty-sum identity
*/
bool
tests_hylo_base_case()
{
    bool ok = true;

    auto grow = [](int _n) -> nat_f<int>
    {
        return (_n <= 0) ? nat_zero<int>() : nat_succ<int>(_n - 1);
    };
    ok = ok && (hylo<int, int, nat_f>(
                    [](const nat_f<int>& _l){ return _l.is_zero ? 77 : 0; },
                    grow, 0) == 77);

    auto down = [](int _n) -> list_f<int>
    {
        return (_n <= 0) ? list_nil<int>() : list_cons<int>(_n, _n - 1);
    };
    ok = ok && (hylo<int, int, list_f>(
                    [](const list_f<int>& _l){ return _l.is_nil ? 1
                                                        : _l.head * _l.tail; },
                    down, 0) == 1);

    return ok;
}


/*
tests_hylo_forms
  Both operands may be any callable.
  Tests the following:
  - free-function and function-object operands refold like the lambda pair
*/
bool
tests_hylo_forms()
{
    bool ok = true;

    // lambdas
    auto grow  = [](int _n) -> nat_f<int>
    {
        return (_n <= 0) ? nat_zero<int>() : nat_succ<int>(_n - 1);
    };
    auto depth = [](const nat_f<int>& _l) -> int
    {
        return _l.is_zero ? 0 : (1 + _l.succ);
    };
    const int a = hylo<int, int, nat_f>(depth, grow, 5);

    // free functions
    const int b = hylo<int, int, nat_f>(&nat_depth_fn, &nat_succ_fn, 5);

    // function objects
    const int c = hylo<int, int, nat_f>(nat_depth_alg(), nat_succ_co(), 5);

    ok = ok && (a == 5);
    ok = ok && (a == b);
    ok = ok && (b == c);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
