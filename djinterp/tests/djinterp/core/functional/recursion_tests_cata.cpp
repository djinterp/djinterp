// djinterp [test]  recursion_tests_cata.cpp
//   Section V -- cata[phi], the universal fold.

// std
#include <string>
#include <vector>
#include <type_traits>
// djinterp
#include "recursion_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_cata_nat_depth
  Folds a Peano value to its depth.
  Tests the following:
  - the base case (Zero) folds to 0
  - Succ^n folds to n, across several n
*/
bool
tests_cata_nat_depth()
{
    bool ok = true;

    auto depth = [](const nat_f<int>& _l) -> int
    {
        return _l.is_zero ? 0 : (1 + _l.succ);
    };

    ok = ok && (cata<int, mu<nat_f> >(depth, make_nat(0)) == 0);
    ok = ok && (cata<int, mu<nat_f> >(depth, make_nat(1)) == 1);
    ok = ok && (cata<int, mu<nat_f> >(depth, make_nat(3)) == 3);
    ok = ok && (cata<int, mu<nat_f> >(depth, make_nat(7)) == 7);

    return ok;
}


/*
tests_cata_list_sum
  Folds a list to the sum of its elements.
  Tests the following:
  - the empty list sums to 0
  - a populated list sums its payloads
*/
bool
tests_cata_list_sum()
{
    bool ok = true;

    auto sum = [](const list_f<int>& _l) -> int
    {
        return _l.is_nil ? 0 : (_l.head + _l.tail);
    };

    ok = ok && (cata<int, mu<list_f> >(sum, make_list(std::vector<int>()))
                == 0);
    ok = ok && (cata<int, mu<list_f> >(sum, make_list({3, 1, 2, 4})) == 10);

    return ok;
}


/*
tests_cata_list_length
  Folds a list to its length -- the algebra ignores the head payload and counts
  only structure, exercising the payload-vs-hole distinction.
  Tests the following:
  - length of the empty list is 0
  - length counts elements regardless of their values
*/
bool
tests_cata_list_length()
{
    bool ok = true;

    auto len = [](const list_f<int>& _l) -> int
    {
        return _l.is_nil ? 0 : (1 + _l.tail);
    };

    ok = ok && (cata<int, mu<list_f> >(len, make_list(std::vector<int>()))
                == 0);
    ok = ok && (cata<int, mu<list_f> >(len, make_list({9, 9, 9})) == 3);
    ok = ok && (cata<int, mu<list_f> >(len, make_list({5})) == 1);

    return ok;
}


/*
tests_cata_list_to_string
  Folds a list to a std::string, so the result carrier differs from both the
  fixed point and the element type -- exercising the explicit _Result pin.
  Tests the following:
  - the empty list folds to the empty string
  - elements are concatenated left to right
*/
bool
tests_cata_list_to_string()
{
    bool ok = true;

    auto render = [](const list_f<std::string>& _l) -> std::string
    {
        return _l.is_nil ? std::string()
                         : (std::to_string(_l.head) + _l.tail);
    };

    ok = ok && (cata<std::string, mu<list_f> >(
                    render, make_list(std::vector<int>())) == std::string());
    ok = ok && (cata<std::string, mu<list_f> >(
                    render, make_list({1, 2, 3})) == std::string("123"));

    return ok;
}


/*
tests_cata_tree_leaf_sum
  Folds a binary tree to the sum of its leaves -- a Branch algebra combines the
  results of BOTH recursive holes.
  Tests the following:
  - a single leaf folds to its value
  - a branching tree sums every leaf
*/
bool
tests_cata_tree_leaf_sum()
{
    bool ok = true;

    auto lsum = [](const tree_f<int>& _l) -> int
    {
        return _l.is_leaf ? _l.value : (_l.left + _l.right);
    };

    ok = ok && (cata<int, mu<tree_f> >(lsum, make_leaf(42)) == 42);

    // ((1 + 2) + 3) tree.
    mu<tree_f> t = make_branch(make_branch(make_leaf(1), make_leaf(2)),
                               make_leaf(3));
    ok = ok && (cata<int, mu<tree_f> >(lsum, t) == 6);

    return ok;
}


/*
tests_cata_tree_height
  Folds a binary tree to its height -- a Branch algebra takes the max over both
  holes, another two-child fold.
  Tests the following:
  - a leaf has height 0
  - height is 1 + the deeper subtree, verified against the manual walk
*/
bool
tests_cata_tree_height()
{
    bool ok = true;

    auto height = [](const tree_f<int>& _l) -> int
    {
        return _l.is_leaf ? 0
                          : (1 + ((_l.left > _l.right) ? _l.left : _l.right));
    };

    ok = ok && (cata<int, mu<tree_f> >(height, make_leaf(0)) == 0);

    // unbalanced: left subtree deeper.
    mu<tree_f> t = make_branch(
        make_branch(make_leaf(1), make_branch(make_leaf(2), make_leaf(3))),
        make_leaf(4));
    ok = ok && (cata<int, mu<tree_f> >(height, t) == tree_height_manual(t));
    ok = ok && (cata<int, mu<tree_f> >(height, t) == 3);

    // a balanced tree of depth 4 has height 4.
    ok = ok && (cata<int, mu<tree_f> >(height, make_balanced_tree(4)) == 4);

    return ok;
}


/*
tests_cata_native_vector
  Folds a plain std::vector<int> as a list, via its recursive_traits
  registration -- cata runs over a native carrier, not a mu.
  Tests the following:
  - the empty vector folds to 0
  - a populated vector sums and counts correctly
*/
bool
tests_cata_native_vector()
{
    bool ok = true;

    auto sum = [](const list_f<int>& _l) -> int
    {
        return _l.is_nil ? 0 : (_l.head + _l.tail);
    };
    auto len = [](const list_f<int>& _l) -> int
    {
        return _l.is_nil ? 0 : (1 + _l.tail);
    };

    std::vector<int> v;
    ok = ok && (cata<int, std::vector<int> >(sum, v) == 0);

    v.push_back(4);
    v.push_back(8);
    v.push_back(15);
    ok = ok && (cata<int, std::vector<int> >(sum, v) == 27);
    ok = ok && (cata<int, std::vector<int> >(len, v) == 3);

    return ok;
}


/*
tests_cata_base_case
  A one-layer value with no recursive holes folds by applying the algebra to
  that single base layer.
  Tests the following:
  - Zero / Nil / Leaf each fold through their base arm
*/
bool
tests_cata_base_case()
{
    bool ok = true;

    ok = ok && (cata<int, mu<nat_f> >(
                    [](const nat_f<int>& _l){ return _l.is_zero ? 111 : 0; },
                    make_nat(0)) == 111);

    ok = ok && (cata<int, mu<list_f> >(
                    [](const list_f<int>& _l){ return _l.is_nil ? 222 : 0; },
                    make_list(std::vector<int>())) == 222);

    ok = ok && (cata<int, mu<tree_f> >(
                    [](const tree_f<int>& _l){ return _l.is_leaf
                                                        ? _l.value : -1; },
                    make_leaf(333)) == 333);

    return ok;
}


/*
tests_cata_reflection_law
  cata with the embed algebra is the identity (cata[In] == id): folding a value
  by re-boxing each layer reproduces it.
  Tests the following:
  - a reflected Peano value has the same depth
  - a reflected list has the same elements
*/
bool
tests_cata_reflection_law()
{
    bool ok = true;

    auto nat_in = [](const nat_f<mu<nat_f> >& _l) -> mu<nat_f>
    {
        return mu<nat_f>::In(_l);
    };
    mu<nat_f> rebuilt_n = cata<mu<nat_f>, mu<nat_f> >(nat_in, make_nat(6));
    ok = ok && (nat_to_unsigned(rebuilt_n) == 6);

    auto list_in = [](const list_f<mu<list_f> >& _l) -> mu<list_f>
    {
        return mu<list_f>::In(_l);
    };
    mu<list_f> rebuilt_l = cata<mu<list_f>, mu<list_f> >(
        list_in, make_list({7, 8, 9}));
    ok = ok && (list_to_vector(rebuilt_l) == std::vector<int>({7, 8, 9}));

    return ok;
}


/*
tests_cata_algebra_forms
  The algebra may be any callable.
  Tests the following:
  - a lambda, a free function, and a function object all fold identically
*/
bool
tests_cata_algebra_forms()
{
    bool ok = true;

    mu<nat_f> five = make_nat(5);

    // lambda
    const int a = cata<int, mu<nat_f> >(
        [](const nat_f<int>& _l){ return _l.is_zero ? 0 : 1 + _l.succ; }, five);
    // free function
    const int b = cata<int, mu<nat_f> >(&nat_depth_fn, five);
    // function object
    const int c = cata<int, mu<nat_f> >(nat_depth_alg(), five);

    ok = ok && (a == 5);
    ok = ok && (a == b);
    ok = ok && (b == c);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
