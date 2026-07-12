// djinterp [test]  profunctor_tests_operations.cpp
//   Section III -- dimap / lmap / rmap and the profunctor laws.

// std
#include <string>
#include <type_traits>
// djinterp
#include "profunctor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_dimap_both_ends
  dimap pre-composes pre onto the input and post-composes post onto the output.
  Tests the following:
  - the header's own example (adapt length by parse / format) yields 30
  - the composition is post . fn . pre, order-sensitive
*/
bool
tests_dimap_both_ends()
{
    bool ok = true;

    auto length = make_profn([](const std::string& _s){ return (int)_s.size(); });
    auto adapted = dimap(length,
        [](int _n){ return std::string(_n, 'x'); },   // pre:  int -> string
        [](int _n){ return _n * 10; });               // post: int -> int
    ok = ok && (adapted(3) == 30);                     // length("xxx") * 10

    auto ord = dimap(make_profn(doubler{}), add_one{}, add_ten{});
    ok = ok && (ord(5) == 22);                         // ((5 + 1) * 2) + 10
    ok = ok && (ord(0) == 12);

    return ok;
}


/*
tests_dimap_type_change
  dimap can change BOTH parameters -- the input from A to A' and the output from
  B to B'.
  Tests the following:
  - an arrow string -> int becomes double -> double
  - the result is itself a profunctor, with the new output type
*/
bool
tests_dimap_type_change()
{
    bool ok = true;

    auto base = make_profn([](const std::string& _s){ return (int)_s.size(); });
    auto changed = dimap(base,
        [](double _d){ return std::string((int)_d, 'q'); },  // pre:  double -> string
        [](int _n){ return _n + 0.5; });                     // post: int -> double

    ok = ok && (changed(3.0) == 3.5);                        // "qqq" -> 3 -> 3.5
    ok = ok && (is_profunctor<decltype(changed)>::value);
    static_assert(std::is_same<decltype(changed(3.0)), double>::value,
                  "output type changed to double");

    return ok;
}


/*
tests_lmap_input_only
  lmap adapts only the input (the contravariant map): pre : A' -> A, output
  untouched.
  Tests the following:
  - lmap(p, pre)(x) == p(pre(x))
  - the input type changes; the output type is preserved
*/
bool
tests_lmap_input_only()
{
    bool ok = true;

    auto len = make_profn([](const std::string& _s){ return (int)_s.size(); });
    auto lp = lmap(len, [](int _n){ return std::to_string(_n); }); // int -> string

    ok = ok && (lp(42) == 2);      // "42" -> 2
    ok = ok && (lp(100) == 3);     // "100" -> 3
    static_assert(std::is_same<decltype(lp(1)), int>::value,
                  "output type preserved");

    return ok;
}


/*
tests_rmap_output_only
  rmap adapts only the output (the covariant map / the profunctor's fmap):
  post : B -> B', input untouched.
  Tests the following:
  - rmap(p, post)(x) == post(p(x))
  - the output type changes; the input type is preserved
*/
bool
tests_rmap_output_only()
{
    bool ok = true;

    auto len = make_profn([](const std::string& _s){ return (int)_s.size(); });

    auto rp = rmap(len, [](int _n){ return _n * 100; });
    ok = ok && (rp(std::string("ab")) == 200);

    auto rp2 = rmap(len, [](int _n){ return _n > 2; });   // int -> bool
    ok = ok && (rp2(std::string("abcd")) == true);
    ok = ok && (rp2(std::string("a")) == false);
    static_assert(std::is_same<decltype(rp2(std::string())), bool>::value,
                  "output type changed to bool");

    return ok;
}


/*
tests_law_identity
  The identity law: adapting with identities changes nothing.
  Tests the following:
  - dimap(p, id, id), rmap(p, id), and lmap(p, id) all agree with p
  - the library's own identity helper works as the identity
*/
bool
tests_law_identity()
{
    bool ok = true;

    auto p   = make_profn([](int _x){ return _x + 100; });
    auto idf = [](int _x){ return _x; };

    auto d = dimap(p, idf, idf);
    ok = ok && (d(5) == p(5)) && (d(-3) == p(-3));

    auto r = rmap(p, idf);
    ok = ok && (r(5) == p(5)) && (r(0) == p(0));

    auto l = lmap(p, idf);
    ok = ok && (l(5) == p(5)) && (l(0) == p(0));

    auto d2 = dimap(p, internal::profunctor_identity_helper{},
                       internal::profunctor_identity_helper{});
    ok = ok && (d2(9) == p(9));

    return ok;
}


/*
tests_law_composition
  The dimap composition law: dimap(dimap(p, pre1, post1), pre2, post2) equals
  dimap(p, pre1 . pre2, post2 . post1) -- pre composes contravariantly, post
  covariantly.
  Tests the following:
  - the two arrows agree at several inputs
*/
bool
tests_law_composition()
{
    bool ok = true;

    auto p     = make_profn([](int _x){ return _x; });
    auto pre1  = [](int _x){ return _x + 1; };
    auto post1 = [](int _x){ return _x * 2; };
    auto pre2  = [](int _x){ return _x * 3; };
    auto post2 = [](int _x){ return _x + 5; };

    auto lhs = dimap(dimap(p, pre1, post1), pre2, post2);
    auto rhs = dimap(p,
        [pre1, pre2](int _x){ return pre1(pre2(_x)); },      // pre1 . pre2
        [post1, post2](int _x){ return post2(post1(_x)); }); // post2 . post1

    ok = ok && (lhs(0) == rhs(0));
    ok = ok && (lhs(1) == rhs(1));
    ok = ok && (lhs(4) == rhs(4));
    ok = ok && (lhs(-2) == rhs(-2));

    return ok;
}


/*
tests_rmap_covariant_composition
  rmap satisfies the functor composition law: rmap(rmap(p, post1), post2) ==
  rmap(p, post2 . post1).
  Tests the following:
  - the two arrows agree, with a concrete spot-check
*/
bool
tests_rmap_covariant_composition()
{
    bool ok = true;

    auto p     = make_profn([](int _x){ return _x; });
    auto post1 = [](int _x){ return _x + 1; };
    auto post2 = [](int _x){ return _x * 2; };

    auto lhs = rmap(rmap(p, post1), post2);
    auto rhs = rmap(p, [post1, post2](int _x){ return post2(post1(_x)); });

    ok = ok && (lhs(3) == rhs(3)) && (lhs(0) == rhs(0));
    ok = ok && (lhs(3) == 8);       // (3 + 1) * 2

    return ok;
}


/*
tests_lmap_contravariant_composition
  lmap composes contravariantly: lmap(lmap(p, pre1), pre2) == lmap(p, pre1 .
  pre2) -- the outer map's pre runs first.
  Tests the following:
  - the two arrows agree, with a concrete spot-check
*/
bool
tests_lmap_contravariant_composition()
{
    bool ok = true;

    auto p    = make_profn([](int _x){ return _x; });
    auto pre1 = [](int _x){ return _x + 1; };
    auto pre2 = [](int _x){ return _x * 2; };

    auto lhs = lmap(lmap(p, pre1), pre2);
    auto rhs = lmap(p, [pre1, pre2](int _x){ return pre1(pre2(_x)); });

    ok = ok && (lhs(3) == rhs(3)) && (lhs(0) == rhs(0));
    ok = ok && (lhs(3) == 7);       // pre1(pre2(3)) = pre1(6) = 7

    return ok;
}


/*
tests_dimap_equals_lmap_rmap
  dimap factors through lmap and rmap either way round.
  Tests the following:
  - dimap(p, pre, post) == rmap(lmap(p, pre), post) == lmap(rmap(p, post), pre)
*/
bool
tests_dimap_equals_lmap_rmap()
{
    bool ok = true;

    auto p    = make_profn([](int _x){ return _x + 1; });
    auto pre  = [](int _x){ return _x * 2; };
    auto post = [](int _x){ return _x + 100; };

    auto d  = dimap(p, pre, post);
    auto rl = rmap(lmap(p, pre), post);
    auto lr = lmap(rmap(p, post), pre);

    ok = ok && (d(3) == rl(3)) && (d(3) == lr(3));
    ok = ok && (d(0) == rl(0)) && (d(0) == lr(0));
    ok = ok && (d(3) == 107);       // post(p(pre(3))) = post(7) = 107

    return ok;
}


/*
tests_operations_constexpr
  dimap / lmap / rmap fold inside a constant expression, including when they
  change the input or output type.
  Tests the following:
  - each operation combines within static_assert
  - a constexpr input change (bool -> int) and output change (int -> bool)
*/
bool
tests_operations_constexpr()
{
    static_assert(dimap(make_profn(doubler{}), add_one{}, add_ten{})(5) == 22,
                  "dimap constexpr");
    static_assert(lmap(make_profn(doubler{}), add_one{})(5) == 12,
                  "lmap constexpr");     // (5 + 1) * 2
    static_assert(rmap(make_profn(doubler{}), add_ten{})(5) == 20,
                  "rmap constexpr");     // (5 * 2) + 10

    // type changes at compile time.
    static_assert(lmap(make_profn(doubler{}), b2i{})(true) == 20,
                  "lmap input type change");    // doubler(b2i(true)) = 20
    static_assert(rmap(make_profn(doubler{}), even_p{})(3) == true,
                  "rmap output type change");   // doubler(3) = 6, even
    static_assert(dimap(make_profn(doubler{}), b2i{}, even_p{})(true) == true,
                  "dimap both type changes");   // even(doubler(b2i(true))) = even(20)

    return true;
}


NS_END  // testing
NS_END  // djinterp
