// djinterp [test]  traversable_tests_operations.cpp
//   Section II -- traverse (the one obligation, delegated) and sequence
//   (traverse with the identity function).

// std
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "traversable_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_traverse_delegates_to_the_instance
  traverse names no structure of its own: it delegates to traversable_traits<T>,
  so it is the instance -- whichever one -- that decides what happens.
  Tests the following:
  - traverse(ta, f) equals the instance's traverse, in value and in TYPE
  - it dispatches to a DIFFERENT instance for a different structure, proving it
    is not wired to std::vector
*/
bool
tests_traverse_delegates_to_the_instance()
{
    bool ok = true;

    // the vector instance.
    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);
    ok = ok && (traverse(xs, to_box()).value ==
                traversable_traits<std::vector<int> >::traverse(
                    xs, to_box()).value);
    static_assert(std::is_same<
        decltype(traverse(std::declval<const std::vector<int>&>(), to_box())),
        decltype(traversable_traits<std::vector<int> >::traverse(
            std::declval<const std::vector<int>&>(), to_box()))>::value,
        "same type, vector");

    // the maybe instance -- a different structure, a different traverse.
    ok = ok && (traverse(just(4), to_box()).value.get() ==
                traversable_traits<tmaybe<int> >::traverse(
                    just(4), to_box()).value.get());
    static_assert(std::is_same<
        decltype(traverse(std::declval<const tmaybe<int>&>(), to_box())),
        decltype(traversable_traits<tmaybe<int> >::traverse(
            std::declval<const tmaybe<int>&>(), to_box()))>::value,
        "same type, maybe");

    // the two produce genuinely different shapes.
    static_assert(!std::is_same<
        decltype(traverse(std::declval<const std::vector<int>&>(), to_box())),
        decltype(traverse(std::declval<const tmaybe<int>&>(), to_box()))
        >::value, "vector materialises; maybe preserves its shape");

    return ok;
}


/*
tests_traverse_over_the_maybe_structure
  For a shape-preserving structure the result is F<T<B>>: the effect is hoisted
  OUT and the structure is preserved INSIDE it -- the header's own summary of what
  traversal is for.
  Tests the following:
  - traversing just(4) with an effectful function yields the effect around a just
  - the shape is preserved, not materialised into a vector
*/
bool
tests_traverse_over_the_maybe_structure()
{
    bool ok = true;

    // tmaybe<int> -> tbox<tmaybe<int>>: the effect is now outermost.
    const tbox<tmaybe<int> > r = traverse(just(4), to_box());
    ok = ok && (r.value.has());
    ok = ok && (r.value.get() == 40);

    static_assert(std::is_same<decltype(traverse(just(4), to_box())),
                               tbox<tmaybe<int> > >::value,
                  "F<T<B>>, the shape preserved");

    // the inner type may change too.
    const tbox<tmaybe<std::string> > s = traverse(just(2), to_box_str());
    ok = ok && (s.value.has());
    ok = ok && (s.value.get() == std::string("xx"));

    return ok;
}


/*
tests_traverse_empty_recovers_the_effect_from_the_type
  The subtlest claim in the header, and the reason traverse works at all on an
  empty structure: F cannot be deduced from a VALUE, because an empty structure
  never calls f. It is recovered from the TYPE of f's result, and the empty shape
  is injected with pure.
  Tests the following:
  - traversing an empty maybe calls f ZERO times
  - yet the result type is still the full F<T<B>>, with F taken from f's signature
  - the value is pure(nothing) -- a SUCCESSFUL effect over an empty shape
  - the same holds for the empty vector
*/
bool
tests_traverse_empty_recovers_the_effect_from_the_type()
{
    bool ok = true;

    // the empty maybe: f is never invoked...
    std::vector<int> log;
    const logging_fn lf{ &log };
    const tbox<tmaybe<int> > r = traverse(nothing<int>(), lf);
    ok = ok && (log.empty());                    // f NEVER called

    // ...yet F is still known, from the TYPE of f's result.
    static_assert(std::is_same<
        decltype(traverse(std::declval<const tmaybe<int>&>(), logging_fn())),
        tbox<tmaybe<int> > >::value, "F recovered from f's declared result");

    // and the value is pure(nothing): a success carrying an empty shape.
    ok = ok && (!r.value.has());                 // the shape is empty
    // (the effect itself succeeded -- tbox always does; with a failing-capable
    //  effect the same holds, and emptiness is still not failure:)
    const tmaybe<tmaybe<int> > m = traverse(nothing<int>(), pos_or_nothing());
    ok = ok && (m.has());                        // the EFFECT succeeded
    ok = ok && (!m.get().has());                 // over an empty SHAPE

    // the empty vector behaves the same way.
    std::vector<int>       log2;
    const logging_fn       lf2{ &log2 };
    const std::vector<int> empty;
    const tbox<std::vector<int> > v = traverse(empty, lf2);
    ok = ok && (log2.empty());
    ok = ok && (v.value.empty());

    return ok;
}


/*
tests_traverse_hoists_the_effect
  When f fails somewhere inside the structure, the failure is hoisted out to the
  effect -- the whole traversal fails, and no partial shape survives.
  Tests the following:
  - traversing just(-1) with a failing function yields a failed effect
  - traversing just(1) with the same function succeeds
  - a failure anywhere in a vector sinks the whole traversal
*/
bool
tests_traverse_hoists_the_effect()
{
    bool ok = true;

    // the maybe structure, with a failing effect.
    const tmaybe<tmaybe<int> > bad = traverse(just(-1), pos_or_nothing());
    ok = ok && (!bad.has());                     // the EFFECT failed

    const tmaybe<tmaybe<int> > good = traverse(just(1), pos_or_nothing());
    ok = ok && (good.has());
    ok = ok && (good.get().has());
    ok = ok && (good.get().get() == 2);

    // the vector, likewise.
    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(-1);
    ok = ok && (!traverse(xs, pos_or_nothing()).has());

    return ok;
}


/*
tests_traverse_result_type_is_deduced
  The result type is whatever the instance produces, so the protocol never names
  it: for a shape-preserving structure it is F<T<B>>; for a sequence it is
  F<std::vector<B>>.
  Tests the following:
  - the maybe structure preserves its shape
  - the vector materialises into a std::vector
  - both carry the same effect F when given the same f
*/
bool
tests_traverse_result_type_is_deduced()
{
    bool ok = true;

    // shape preserved.
    static_assert(std::is_same<
        decltype(traverse(std::declval<const tmaybe<int>&>(), to_box())),
        tbox<tmaybe<int> > >::value, "F<T<B>>");

    // elements materialised.
    static_assert(std::is_same<
        decltype(traverse(std::declval<const std::vector<int>&>(), to_box())),
        tbox<std::vector<int> > >::value, "F<vector<B>>");

    // the same effect, either way.
    static_assert(std::is_same<
        applicative_value_type_t<decltype(to_box()(0))>, int>::value,
        "f : int -> tbox<int>");

    ok = ok && (traverse(just(4), to_box()).value.get() == 40);

    return ok;
}


/*
tests_traverse_forwarding
  traverse takes the traversable by forwarding reference and passes it on.
  Tests the following:
  - an rvalue, an lvalue, a const lvalue, and an xvalue traversable all work
  - the function is taken by value and may be an lvalue or an rvalue
*/
bool
tests_traverse_forwarding()
{
    bool ok = true;

    // rvalue.
    ok = ok && (traverse(just(4), to_box()).value.get() == 40);

    // lvalue.
    tmaybe<int> m = just(4);
    ok = ok && (traverse(m, to_box()).value.get() == 40);

    // const lvalue.
    const tmaybe<int> cm = just(4);
    ok = ok && (traverse(cm, to_box()).value.get() == 40);

    // xvalue.
    ok = ok && (traverse(std::move(m), to_box()).value.get() == 40);

    // a vector, by lvalue and by rvalue.
    std::vector<int> xs;
    xs.push_back(1);
    ok = ok && (traverse(xs, to_box()).value.size() == 1u);

    // the function, by lvalue.
    to_box f;
    ok = ok && (traverse(cm, f).value.get() == 40);

    return ok;
}


/*
tests_traverse_constexpr
  DUAL DOMAIN. traverse and sequence are D_CONSTEXPR and fold wherever the
  instance and the effect do. The maybe structure over these literal-type effects
  folds at compile time; the vector instance materialises a std::vector and so
  runs at runtime (as the header's tier note allows).
  Tests the following:
  - traverse and sequence over the maybe structure fold inside static_assert,
    including the empty case
  - the same calls run at runtime over values the compiler does not know
*/
bool
tests_traverse_constexpr()
{
    // compile time: the shape-preserving instance.
    static_assert(traverse(just(4), to_box()).value.get() == 40, "traverse");
    static_assert(traverse(just(4), to_box()).value.has(), "shape preserved");
    static_assert(!traverse(nothing<int>(), to_box()).value.has(),
                  "the empty case folds too");
    static_assert(!traverse(just(-1), pos_or_nothing()).has(),
                  "a failed effect folds");

    // sequence, likewise.
    static_assert(sequence(just(boxed(7))).value.get() == 7, "sequence");

    // runtime: the other half of the dual domain.
    bool ok = true;
    int  x  = 4;
    ok = ok && (traverse(just(x), to_box()).value.get() == 40);
    ok = ok && (sequence(just(boxed(x))).value.get() == 4);

    return ok;
}


/*
tests_sequence_is_traverse_with_identity
  sequence is derived once, generically: it is traverse with the library's
  identity helper, and nothing more.
  Tests the following:
  - sequence(ta) equals traverse(ta, identity), in value and in TYPE
  - it holds for both structures
*/
bool
tests_sequence_is_traverse_with_identity()
{
    bool ok = true;

    const internal::traversable_identity_helper id;

    // the maybe structure.
    ok = ok && (sequence(just(boxed(7))).value.get() ==
                traverse(just(boxed(7)), id).value.get());
    static_assert(std::is_same<
        decltype(sequence(std::declval<const tmaybe<tbox<int> >&>())),
        decltype(traverse(std::declval<const tmaybe<tbox<int> >&>(),
                          internal::traversable_identity_helper()))>::value,
        "same type, maybe");

    // the vector.
    std::vector<tbox<int> > vs;
    vs.push_back(boxed(1));
    vs.push_back(boxed(2));
    ok = ok && (sequence(vs).value.size() == traverse(vs, id).value.size());
    static_assert(std::is_same<
        decltype(sequence(std::declval<const std::vector<tbox<int> >&>())),
        decltype(traverse(std::declval<const std::vector<tbox<int> >&>(),
                          internal::traversable_identity_helper()))>::value,
        "same type, vector");

    return ok;
}


/*
tests_sequence_inverts_the_nesting
  The whole point: sequence turns a structure OF effects inside out into an effect
  OF a structure -- T<F<A>> becomes F<T<A>>.
  Tests the following:
  - tmaybe<tbox<int>> becomes tbox<tmaybe<int>>
  - vector<tbox<int>> becomes tbox<vector<int>>, with the elements in order
  - the nesting really is inverted (the types are not the same)
*/
bool
tests_sequence_inverts_the_nesting()
{
    bool ok = true;

    // T<F<A>> -> F<T<A>>, shape preserved.
    const tbox<tmaybe<int> > m = sequence(just(boxed(7)));
    ok = ok && (m.value.has());
    ok = ok && (m.value.get() == 7);
    static_assert(std::is_same<decltype(sequence(just(boxed(7)))),
                               tbox<tmaybe<int> > >::value, "inverted");
    static_assert(!std::is_same<decltype(sequence(just(boxed(7)))),
                                tmaybe<tbox<int> > >::value,
                  "and it really did change");

    // T<F<A>> -> F<T<A>>, materialised.
    std::vector<tbox<int> > vs;
    vs.push_back(boxed(1));
    vs.push_back(boxed(2));
    vs.push_back(boxed(3));
    const tbox<std::vector<int> > v = sequence(vs);
    ok = ok && (v.value.size() == 3u);
    ok = ok && (v.value[0] == 1 && v.value[1] == 2 && v.value[2] == 3);

    return ok;
}


/*
tests_sequence_is_all_or_nothing
  The header's second USAGE example: sequencing a vector of maybes yields a value
  only if EVERY element was one.
  Tests the following:
  - all present: just(the whole vector), in order
  - one absent (head, middle, or tail): nothing
  - sequencing a maybe of a failed effect propagates the failure
*/
bool
tests_sequence_is_all_or_nothing()
{
    bool ok = true;

    // every element present.
    std::vector<tmaybe<int> > all;
    all.push_back(just(1));
    all.push_back(just(2));
    all.push_back(just(3));

    const tmaybe<std::vector<int> > s = sequence(all);
    ok = ok && (s.has());
    ok = ok && (s.get().size() == 3u);
    ok = ok && (s.get()[0] == 1 && s.get()[1] == 2 && s.get()[2] == 3);

    // one absent, in the middle.
    std::vector<tmaybe<int> > mid;
    mid.push_back(just(1));
    mid.push_back(nothing<int>());
    mid.push_back(just(3));
    ok = ok && (!sequence(mid).has());

    // one absent, at the head.
    std::vector<tmaybe<int> > head;
    head.push_back(nothing<int>());
    head.push_back(just(2));
    ok = ok && (!sequence(head).has());

    // one absent, at the tail.
    std::vector<tmaybe<int> > tail;
    tail.push_back(just(1));
    tail.push_back(nothing<int>());
    ok = ok && (!sequence(tail).has());

    // the shape-preserving structure: a maybe of a failed effect.
    ok = ok && (!sequence(just(nothing<int>())).has());
    ok = ok && (sequence(just(just(5))).has());

    return ok;
}


/*
tests_sequence_empty
  The empty cases, where the identity function is never called at all.
  Tests the following:
  - an empty vector of effects sequences to a SUCCESS over an empty vector
  - an empty maybe of effects sequences to a success over an empty maybe
  - emptiness is not failure
*/
bool
tests_sequence_empty()
{
    bool ok = true;

    // an empty vector of maybes: just({}) -- success, not failure.
    const std::vector<tmaybe<int> > none;
    const tmaybe<std::vector<int> > s = sequence(none);
    ok = ok && (s.has());
    ok = ok && (s.get().empty());

    // an empty maybe of effects: the effect succeeds over an empty shape.
    const tmaybe<tmaybe<int> > m = sequence(nothing<tmaybe<int> >());
    ok = ok && (m.has());              // the effect SUCCEEDED
    ok = ok && (!m.get().has());       // over an empty shape

    // and under the never-failing effect.
    const tbox<tmaybe<int> > b = sequence(nothing<tbox<int> >());
    ok = ok && (!b.value.has());

    return ok;
}


/*
tests_law_traverse_is_sequence_after_map
  The defining relationship between the two operations, in the other direction:
  traversing with f is the same as mapping f over the structure and then
  sequencing. (functor_map gives T<F<B>>; sequence turns it into F<T<B>>.)
  Tests the following:
  - traverse(ta, f) == sequence(functor_map(ta, f)), in value and in TYPE
  - it holds on a present and on an EMPTY structure
  - it holds for a failing effect
*/
bool
tests_law_traverse_is_sequence_after_map()
{
    bool ok = true;

    // present.
    const tbox<tmaybe<int> > lhs = traverse(just(4), to_box());
    const tbox<tmaybe<int> > rhs = sequence(functor_map(just(4), to_box()));
    ok = ok && (lhs.value.has() == rhs.value.has());
    ok = ok && (lhs.value.get() == rhs.value.get());
    static_assert(std::is_same<decltype(traverse(just(4), to_box())),
                               decltype(sequence(functor_map(just(4),
                                                             to_box())))>::value,
                  "the same type");

    // empty: both sides inject the empty shape with pure.
    const tbox<tmaybe<int> > le = traverse(nothing<int>(), to_box());
    const tbox<tmaybe<int> > re = sequence(functor_map(nothing<int>(),
                                                       to_box()));
    ok = ok && (le.value.has() == re.value.has());
    ok = ok && (!le.value.has());

    // a failing effect: both sides fail.
    ok = ok && (traverse(just(-1), pos_or_nothing()).has() ==
                sequence(functor_map(just(-1), pos_or_nothing())).has());
    ok = ok && (!traverse(just(-1), pos_or_nothing()).has());

    // and it folds at compile time.
    static_assert(traverse(just(4), to_box()).value.get() ==
                  sequence(functor_map(just(4), to_box())).value.get(),
                  "the law, constexpr");

    return ok;
}


NS_END  // testing
NS_END  // djinterp
