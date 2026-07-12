// djinterp [test]  traversable_tests_vector.cpp
//   The std::vector instance -- the one concrete traversable the header ships:
//   walk left-to-right, run f : A -> F<B> at each element, combine with lift_a2
//   into F<std::vector<B>>.

// std
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "traversable_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_vector_traverse_all_succeed
  Every element yields a successful effect, so the whole traversal succeeds and
  the results are materialised into the vector inside the effect.
  Tests the following:
  - a vector<int> traversed with a maybe-producing function yields just(vector)
  - the mapped values are all present, in order
*/
bool
tests_vector_traverse_all_succeed()
{
    bool ok = true;

    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    const tmaybe<std::vector<int> > r = traverse(xs, pos_or_nothing());

    ok = ok && (r.has());
    ok = ok && (r.get().size() == 3u);
    ok = ok && (r.get()[0] == 2);
    ok = ok && (r.get()[1] == 4);
    ok = ok && (r.get()[2] == 6);

    return ok;
}


/*
tests_vector_traverse_one_failure_sinks_it
  ALL-OR-NOTHING. The effects are combined with lift_a2, so a single failing
  element sinks the entire traversal -- there is no partial result.
  Tests the following:
  - a failure in the middle, at the head, and at the tail each yields nothing
  - the same vector with no failures succeeds (the contrast)
*/
bool
tests_vector_traverse_one_failure_sinks_it()
{
    bool ok = true;

    // failure in the middle.
    std::vector<int> mid;
    mid.push_back(1);
    mid.push_back(-2);
    mid.push_back(3);
    ok = ok && (!traverse(mid, pos_or_nothing()).has());

    // failure at the head.
    std::vector<int> head;
    head.push_back(-1);
    head.push_back(2);
    ok = ok && (!traverse(head, pos_or_nothing()).has());

    // failure at the tail.
    std::vector<int> tail;
    tail.push_back(1);
    tail.push_back(-2);
    ok = ok && (!traverse(tail, pos_or_nothing()).has());

    // no failures: it succeeds.
    std::vector<int> good;
    good.push_back(1);
    good.push_back(2);
    ok = ok && (traverse(good, pos_or_nothing()).has());

    return ok;
}


/*
tests_vector_traverse_empty_uses_pure
  The empty structure never calls f, so the effect cannot come from a value; the
  instance injects the empty shape with pure instead.
  Tests the following:
  - an empty vector yields a SUCCESSFUL effect over an empty vector
  - f is never invoked
  - it holds for a failing-capable effect too: emptiness is not failure
*/
bool
tests_vector_traverse_empty_uses_pure()
{
    bool ok = true;

    const std::vector<int> empty;

    // pure(empty vector) -- a success, not a failure.
    const tmaybe<std::vector<int> > r = traverse(empty, pos_or_nothing());
    ok = ok && (r.has());
    ok = ok && (r.get().empty());

    // and f is never called.
    std::vector<int> log;
    const logging_fn lf{ &log };
    const tbox<std::vector<int> > b = traverse(empty, lf);
    ok = ok && (log.empty());          // f never invoked
    ok = ok && (b.value.empty());

    return ok;
}


/*
tests_vector_traverse_is_left_to_right
  The walk is left-to-right, which a recording function makes observable: the
  calls arrive in source order, and the results land in that same order.
  Tests the following:
  - f is called once per element, in source order
  - the materialised vector is in that order too
  - the call count equals the element count
*/
bool
tests_vector_traverse_is_left_to_right()
{
    bool ok = true;

    std::vector<int> xs;
    xs.push_back(10);
    xs.push_back(20);
    xs.push_back(30);

    std::vector<int> log;
    const logging_fn lf{ &log };

    const tbox<std::vector<int> > r = traverse(xs, lf);

    // one call per element, in source order.
    ok = ok && (log.size() == 3u);
    ok = ok && (log[0] == 10 && log[1] == 20 && log[2] == 30);

    // and the results are in that order.
    ok = ok && (r.value.size() == 3u);
    ok = ok && (r.value[0] == 10 && r.value[1] == 20 && r.value[2] == 30);

    return ok;
}


/*
tests_vector_traverse_single_element
  The one-element case: exactly one effect, combined once with the pure seed.
  Tests the following:
  - a singleton succeeds and yields a singleton
  - a singleton that fails yields nothing
*/
bool
tests_vector_traverse_single_element()
{
    bool ok = true;

    std::vector<int> one;
    one.push_back(5);

    const tmaybe<std::vector<int> > r = traverse(one, pos_or_nothing());
    ok = ok && (r.has());
    ok = ok && (r.get().size() == 1u);
    ok = ok && (r.get()[0] == 10);

    std::vector<int> bad;
    bad.push_back(-5);
    ok = ok && (!traverse(bad, pos_or_nothing()).has());

    return ok;
}


/*
tests_vector_traverse_changes_inner_type
  f is A -> F<B>, so the element type may change on the way through: a vector<A>
  comes back as F<vector<B>>.
  Tests the following:
  - int -> tbox<string> gives tbox<vector<string>>
  - int -> tmaybe<string> gives tmaybe<vector<string>>
  - the mapped values are correct
*/
bool
tests_vector_traverse_changes_inner_type()
{
    bool ok = true;

    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);

    // int -> string, inside the box.
    const tbox<std::vector<std::string> > b = traverse(xs, to_box_str());
    ok = ok && (b.value.size() == 2u);
    ok = ok && (b.value[0] == std::string("x"));
    ok = ok && (b.value[1] == std::string("xx"));

    // int -> string, inside the maybe.
    const tmaybe<std::vector<std::string> > m = traverse(xs, to_maybe_str());
    ok = ok && (m.has());
    ok = ok && (m.get().size() == 2u);
    ok = ok && (m.get()[1] == std::string("yy"));

    return ok;
}


/*
tests_vector_traverse_result_type
  The result type is F<std::vector<B>>, assembled through monad_rebind from the
  effect F named by f's result -- the elements are MATERIALISED, which is the
  documented behaviour for a sequence-shaped traversable.
  Tests the following:
  - the declared result type for each effect and each B
  - it is exactly what the instance's own traverse produces
*/
bool
tests_vector_traverse_result_type()
{
    bool ok = true;

    static_assert(std::is_same<
        decltype(traverse(std::declval<const std::vector<int>&>(),
                          pos_or_nothing())),
        tmaybe<std::vector<int> > >::value, "F = tmaybe, B = int");

    static_assert(std::is_same<
        decltype(traverse(std::declval<const std::vector<int>&>(),
                          to_box_str())),
        tbox<std::vector<std::string> > >::value, "F = tbox, B = string");

    // and it matches the instance's own traverse.
    static_assert(std::is_same<
        decltype(traverse(std::declval<const std::vector<int>&>(), to_box())),
        decltype(traversable_traits<std::vector<int> >::traverse(
            std::declval<const std::vector<int>&>(), to_box()))>::value,
        "delegates to the instance");

    std::vector<int> xs;
    xs.push_back(1);
    ok = ok && (traverse(xs, to_box()).value.size() == 1u);

    return ok;
}


/*
tests_vector_traverse_second_effect
  The instance is generic in the effect F: it names nothing about which
  applicative it is threading, so a second, unrelated effect works identically.
  Tests the following:
  - the same vector traverses under the identity effect (which cannot fail)
  - and under the maybe effect (which can)
  - the two agree on the mapped values where both succeed
*/
bool
tests_vector_traverse_second_effect()
{
    bool ok = true;

    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    // the never-failing effect.
    const tbox<std::vector<int> > b = traverse(xs, to_box());
    ok = ok && (b.value.size() == 3u);
    ok = ok && (b.value[0] == 10 && b.value[1] == 20 && b.value[2] == 30);

    // the failing-capable effect, on the same vector.
    const tmaybe<std::vector<int> > m = traverse(xs, pos_or_nothing());
    ok = ok && (m.has());
    ok = ok && (m.get().size() == 3u);

    // different effects, different result types.
    static_assert(!std::is_same<decltype(b), decltype(m)>::value,
                  "the effect is part of the result type");

    return ok;
}


/*
tests_vector_traverse_source_untouched
  traverse builds a new structure inside the effect; the source vector is walked,
  not consumed. (The append reducer threads its accumulator by value, so nothing
  in the pipeline mutates the input.)
  Tests the following:
  - the source vector is unchanged after a successful traversal
  - and after a failed one
*/
bool
tests_vector_traverse_source_untouched()
{
    bool ok = true;

    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    const tmaybe<std::vector<int> > r = traverse(xs, pos_or_nothing());
    ok = ok && (r.has());

    ok = ok && (xs.size() == 3u);
    ok = ok && (xs[0] == 1 && xs[1] == 2 && xs[2] == 3);

    // after a failure, too.
    std::vector<int> bad;
    bad.push_back(1);
    bad.push_back(-2);
    ok = ok && (!traverse(bad, pos_or_nothing()).has());
    ok = ok && (bad.size() == 2u);
    ok = ok && (bad[0] == 1 && bad[1] == -2);

    return ok;
}


/*
tests_vector_is_the_foldable_companion
  The header calls this instance "the companion to the std::vector foldable
  instance": traversal needs a Foldable to walk the elements, and a vector is
  both.
  Tests the following:
  - std::vector is a foldable AND a traversable
  - the two name the same element type
  - the maybe structure is likewise both a functor and a traversable
*/
bool
tests_vector_is_the_foldable_companion()
{
    bool ok = true;

    ok = ok && (is_foldable<std::vector<int> >::value);
    ok = ok && (is_traversable<std::vector<int> >::value);

    static_assert(std::is_same<
        traversable_value_type_t<std::vector<int> >,
        foldable_value_type_t<std::vector<int> > >::value,
        "the same element type");

    // the maybe structure: a functor (to rebuild the shape) and a traversable.
    ok = ok && (is_functor<tmaybe<int> >::value);
    ok = ok && (is_traversable<tmaybe<int> >::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
