// djinterp [test]  functor_tests_map.cpp
//   Section II -- functor_map (fmap), the one operation, and the functor laws.

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "functor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_functor_map_over_a_monad
  The header's own example: map a plain function over a context, without
  disturbing the context.
  Tests the following:
  - the USAGE example (just(21) doubled) yields 42, still in its context
  - the same call serves the second monad
*/
bool
tests_functor_map_over_a_monad()
{
    bool ok = true;

    // the header's example.
    const tmaybe<int> m = just(21);
    const auto        n = functor_map(m, dbl());
    ok = ok && (n.has);
    ok = ok && (n.value == 42);

    // the same operation over a different monad.
    ok = ok && (functor_map(boxed(21), dbl()).value == 42);
    ok = ok && (functor_map(boxed(0), add1()).value == 1);

    return ok;
}


/*
tests_functor_map_preserves_context
  map applies the function to the value(s) INSIDE the context and leaves the
  surrounding shape alone -- it is not a fold, and it cannot create or destroy a
  context.
  Tests the following:
  - an absent value stays absent, and the function is never applied to it
  - a present value stays present
  - the source functor is unchanged (map returns a new context)
*/
bool
tests_functor_map_preserves_context()
{
    bool ok = true;

    // absent stays absent.
    ok = ok && (!functor_map(nothing<int>(), dbl()).has);
    ok = ok && (!functor_map(nothing<int>(), add1()).has);

    // present stays present.
    ok = ok && (functor_map(just(3), dbl()).has);

    // the source is untouched.
    const tmaybe<int> src = just(21);
    const auto        out = functor_map(src, dbl());
    ok = ok && (out.value == 42);
    ok = ok && (src.value == 21);      // unchanged
    ok = ok && (src.has);

    return ok;
}


/*
tests_functor_map_over_a_view
  For a context whose mapped type depends on the mapping function, functor_map is
  simply the lazy transform that context already provides -- named uniformly.
  Tests the following:
  - mapping a view composes the new function after the pending one
  - it stays lazy: the composed view computes only when asked
  - mapping twice composes twice, in order
*/
bool
tests_functor_map_over_a_view()
{
    bool ok = true;

    const auto v = make_view(5, dbl());          // 5 -> 10
    ok = ok && (v.get() == 10);

    const auto v2 = functor_map(v, add1());      // (5 * 2) + 1
    ok = ok && (v2.get() == 11);

    // twice, in order: ((5 * 2) + 1) * 2
    const auto v3 = functor_map(v2, dbl());
    ok = ok && (v3.get() == 22);

    // the source view is untouched.
    ok = ok && (v.get() == 10);

    return ok;
}


/*
tests_functor_map_changes_inner_type
  The mapping function is T -> U, so the context comes back over U.
  Tests the following:
  - int -> bool and int -> string, through the monad bridge
  - int -> bool through the view, where the change shows in its value_type
*/
bool
tests_functor_map_changes_inner_type()
{
    bool ok = true;

    // through the bridge: the context is rebound.
    const auto b = functor_map(just(4), is_even());
    static_assert(std::is_same<decltype(b), const tmaybe<bool> >::value,
                  "tmaybe<int> -> tmaybe<bool>");
    ok = ok && (b.has && b.value == true);

    const auto s = functor_map(boxed(3), to_str());
    static_assert(std::is_same<decltype(s), const tbox<std::string> >::value,
                  "tbox<int> -> tbox<string>");
    ok = ok && (s.value == std::string("xxx"));

    // through the view: the inner type follows the composed function.
    const auto vb = functor_map(make_view(5, dbl()), is_even());
    static_assert(std::is_same<
        functor_value_type_t<decltype(vb)>, bool>::value,
        "the view's inner type is now bool");
    ok = ok && (vb.get() == true);      // 10 is even

    return ok;
}


/*
tests_functor_map_law_identity
  The first functor law: mapping the identity function changes nothing.
  Tests the following:
  - map(fa, id) == fa, on both roads and on an absent context
*/
bool
tests_functor_map_law_identity()
{
    bool ok = true;

    // bridged, present.
    const tmaybe<int> m = just(21);
    const auto        mi = functor_map(m, ident_fn());
    ok = ok && (mi.has == m.has);
    ok = ok && (mi.value == m.value);
    static_assert(std::is_same<decltype(mi), const tmaybe<int> >::value,
                  "same context");

    // bridged, absent.
    const auto ni = functor_map(nothing<int>(), ident_fn());
    ok = ok && (!ni.has);

    // the other monad.
    ok = ok && (functor_map(boxed(7), ident_fn()).value == 7);

    // the view: composing the identity leaves the observable value alone.
    const auto v = make_view(5, dbl());
    ok = ok && (functor_map(v, ident_fn()).get() == v.get());

    return ok;
}


/*
tests_functor_map_law_composition
  The second functor law: mapping f then g is mapping (g . f) once.
  Tests the following:
  - map(map(fa, f), g) == map(fa, g . f), on both roads
  - the composite is applied in that order, which a non-commutative pair pins
  - the law also holds on an absent context (vacuously, but checkably)
*/
bool
tests_functor_map_law_composition()
{
    bool ok = true;

    // bridged: double, then add one.  (21 * 2) + 1 = 43.
    const tmaybe<int> m = just(21);
    const auto        twice = functor_map(functor_map(m, dbl()), add1());
    const auto        once  = functor_map(m, dbl_then_add1());
    ok = ok && (twice.value == once.value);
    ok = ok && (twice.value == 43);
    static_assert(std::is_same<decltype(twice), decltype(once)>::value,
                  "and the same type");

    // the order matters: add one, then double, is 44.
    ok = ok && (functor_map(functor_map(m, add1()), dbl()).value == 44);

    // the other monad.
    ok = ok && (functor_map(functor_map(boxed(21), dbl()), add1()).value ==
                functor_map(boxed(21), dbl_then_add1()).value);

    // absent: the law holds vacuously, and the context is still absent.
    ok = ok && (!functor_map(functor_map(nothing<int>(), dbl()), add1()).has);

    // the view.
    const auto v = make_view(5, dbl());
    ok = ok && (functor_map(functor_map(v, dbl()), add1()).get() ==
                functor_map(v, dbl_then_add1()).get());

    return ok;
}


/*
tests_functor_map_result_type
  The result type is whatever the INSTANCE's map produces -- F<U> for a concrete
  context, the instance's transformed view otherwise -- so it is deduced, never
  named by the protocol.
  Tests the following:
  - a monad yields the rebound context F<U>
  - a view yields a composed view over a new function type, NOT an F<U>
  - functor_map's type matches the traits' map exactly, on both roads
*/
bool
tests_functor_map_result_type()
{
    bool ok = true;

    // a concrete context: F<T> -> F<U>.
    static_assert(std::is_same<decltype(functor_map(just(1), is_even())),
                               tmaybe<bool> >::value, "tmaybe<bool>");
    static_assert(std::is_same<decltype(functor_map(boxed(1), to_str())),
                               tbox<std::string> >::value, "tbox<string>");

    // a view: a composed view, whose type depends on the function supplied.
    static_assert(std::is_same<
        decltype(functor_map(make_view(5, dbl()), add1())),
        lazy_view<int, composed<dbl, add1> > >::value, "a composed view");

    // functor_map == the instance's map, in type.
    static_assert(std::is_same<
        decltype(functor_map(just(1), dbl())),
        decltype(functor_traits<tmaybe<int> >::map(just(1), dbl()))>::value,
        "delegates to the traits");
    static_assert(std::is_same<
        decltype(functor_map(make_view(5, dbl()), add1())),
        decltype(functor_traits<lazy_view<int, dbl> >::map(
            make_view(5, dbl()), add1()))>::value, "and for the view");

    ok = ok && (functor_map(just(1), is_even()).value == false);

    return ok;
}


/*
tests_functor_map_forwarding
  functor_map takes the functor and the function by forwarding reference and
  forwards both on to the instance's map.
  Tests the following:
  - an lvalue, a const lvalue, and an rvalue functor all map
  - an lvalue and an rvalue function both map
  - a const functor maps (the traits' map takes it by const reference)
*/
bool
tests_functor_map_forwarding()
{
    bool ok = true;

    // rvalue functor.
    ok = ok && (functor_map(just(21), dbl()).value == 42);

    // lvalue functor.
    tmaybe<int> m = just(21);
    ok = ok && (functor_map(m, dbl()).value == 42);

    // const lvalue functor.
    const tmaybe<int> cm = just(21);
    ok = ok && (functor_map(cm, dbl()).value == 42);

    // xvalue functor.
    ok = ok && (functor_map(std::move(m), dbl()).value == 42);

    // lvalue and const-lvalue function.
    dbl       f;
    const dbl cf{};
    ok = ok && (functor_map(cm, f).value == 42);
    ok = ok && (functor_map(cm, cf).value == 42);

    // a lambda, by rvalue.
    ok = ok && (functor_map(cm, [](int _x){ return _x + 1; }).value == 22);

    return ok;
}


/*
tests_functor_map_constexpr
  DUAL DOMAIN. functor_map is D_CONSTEXPR and folds exactly where the instance's
  map does -- so over these literal-type fixtures it folds at compile time, and
  the same call runs at runtime over values not known to the compiler.
  Tests the following:
  - maps over both roads fold inside static_assert, including a type change
  - the composition law holds inside a constant expression
  - the same operations run at runtime
*/
bool
tests_functor_map_constexpr()
{
    // compile time, through the bridge.
    static_assert(functor_map(just(21), dbl()).value == 42, "bridged");
    static_assert(!functor_map(nothing<int>(), dbl()).has, "absent, bridged");
    static_assert(functor_map(boxed(21), dbl()).value == 42, "the other monad");
    static_assert(functor_map(just(4), is_even()).value == true, "type change");

    // compile time, through the view.
    static_assert(functor_map(make_view(5, dbl()), add1()).get() == 11, "view");

    // the composition law, as one constant expression.
    static_assert(
        functor_map(functor_map(just(21), dbl()), add1()).value ==
        functor_map(just(21), dbl_then_add1()).value, "composition, constexpr");

    // runtime: the other half of the dual domain.
    bool ok = true;
    int  x  = 21;
    ok = ok && (functor_map(just(x), dbl()).value == 42);
    ok = ok && (functor_map(make_view(x, dbl()), add1()).get() == 43);

    return ok;
}


/*
tests_functor_map_generic_over_any_functor
  The point of the protocol: one generic function, written once against
  functor_map, works over EVERY functor -- a monad reached through the bridge, a
  second unrelated monad, and a view with its own specialization.
  Tests the following:
  - the header's `bump` example maps all three contexts
  - it preserves each context's own shape and result type
*/
bool
tests_functor_map_generic_over_any_functor()
{
    bool ok = true;

    // one call, three different functors.
    ok = ok && (bump(just(21), dbl()).value == 42);
    ok = ok && (bump(boxed(21), dbl()).value == 42);
    ok = ok && (bump(make_view(21, ident_fn()), dbl()).get() == 42);

    // each keeps its own shape.
    ok = ok && (!bump(nothing<int>(), dbl()).has);
    static_assert(std::is_same<decltype(bump(just(1), is_even())),
                               tmaybe<bool> >::value, "rebound context");
    static_assert(std::is_same<
        decltype(bump(make_view(5, dbl()), add1())),
        lazy_view<int, composed<dbl, add1> > >::value, "composed view");

    // and it folds at compile time, like the call it wraps.
    static_assert(bump(just(21), dbl()).value == 42, "constexpr");

    return ok;
}


NS_END  // testing
NS_END  // djinterp
