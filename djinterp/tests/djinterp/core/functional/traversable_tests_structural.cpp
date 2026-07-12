// djinterp [test]  traversable_tests_structural.cpp
//   Section 0 -- traversable_value_type, the internal helpers (identity and
//   append), and the C++20 Traversable concept.

// std
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "traversable_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_traversable_value_type
  The inner value type A of a traversable T<A>.
  Tests the following:
  - the element type of a vector, at several element types
  - the inner type of the maybe structure
  - std::decay is applied to the traversable first
*/
bool
tests_traversable_value_type()
{
    bool ok = true;

    static_assert(std::is_same<
        traversable_value_type<std::vector<int> >::type, int>::value,
        "vector<int>");
    static_assert(std::is_same<
        traversable_value_type<std::vector<std::string> >::type,
        std::string>::value, "vector<string>");
    static_assert(std::is_same<
        traversable_value_type<std::vector<tmaybe<int> > >::type,
        tmaybe<int> >::value, "vector of effects");

    static_assert(std::is_same<
        traversable_value_type<tmaybe<int> >::type, int>::value, "tmaybe<int>");

    // decayed.
    static_assert(std::is_same<
        traversable_value_type<const std::vector<int>&>::type, int>::value,
        "decay applied");

    ok = ok && (std::is_same<traversable_value_type<tmaybe<int> >::type,
                             int>::value);

    return ok;
}


/*
tests_traversable_value_type_t_alias
  The _t alias is exactly the trait's type.
  Tests the following:
  - the two agree on both instances
*/
bool
tests_traversable_value_type_t_alias()
{
    bool ok = true;

    static_assert(std::is_same<
        traversable_value_type_t<std::vector<int> >,
        traversable_value_type<std::vector<int> >::type>::value, "vector");
    static_assert(std::is_same<
        traversable_value_type_t<tmaybe<std::string> >,
        traversable_value_type<tmaybe<std::string> >::type>::value, "maybe");

    static_assert(std::is_same<traversable_value_type_t<std::vector<int> >,
                               int>::value, "and it is the element type");

    ok = ok && (std::is_same<traversable_value_type_t<tmaybe<int> >,
                             int>::value);

    return ok;
}


/*
tests_traversable_value_type_helper_sfinae
  The extraction is built on a SFINAE helper whose primary is a COMPLETE but
  MEMBERLESS struct -- a soft failure, yielding no ::type rather than an error.
  That is the machinery behind the trait's SFINAE-friendliness, and the level at
  which the claim can actually be read.
  Tests the following:
  - the helper has a ::type for both traversables
  - it has NONE for a non-traversable -- and is nevertheless complete, so the
    failure is soft
  NOTE: the PUBLIC traversable_value_type re-declares `type` unconditionally, so
  naming it on a non-traversable is a hard error rather than a detectable absence
  (the same shape as functor_value_type and monad_value_type). Only the helper's
  soft failure is testable here.
*/
bool
tests_traversable_value_type_helper_sfinae()
{
    bool ok = true;

    // present for both instances.
    static_assert(has_type<vt_helper<std::vector<int> > >::value, "vector");
    static_assert(has_type<vt_helper<tmaybe<int> > >::value, "maybe");

    // absent for a non-traversable -- softly.
    static_assert(!has_type<vt_helper<not_traversable> >::value,
                  "no ::type for a non-traversable");
    static_assert(!has_type<vt_helper<int> >::value, "nor for a scalar");
    static_assert(!has_type<vt_helper<tbox<int> > >::value,
                  "nor for a mere applicative");

    // the failure IS soft: the primary is complete, just memberless.
    static_assert(is_complete<vt_helper<not_traversable> >::value,
                  "complete, but memberless");

    // helper and public face agree wherever the public face is usable.
    static_assert(std::is_same<vt_helper<std::vector<int> >::type,
                               traversable_value_type_t<std::vector<int> > >::value,
                  "the same answer");

    ok = ok && (!has_type<vt_helper<not_traversable> >::value);

    return ok;
}


/*
tests_traversable_value_type_mirrors_siblings
  The trait mirrors functor_value_type / foldable_value_type: where a type is both
  a traversable and one of those, the inner type they name is the same one.
  Tests the following:
  - for the maybe structure, the traversable and functor inner types agree
  - the vector's traversable inner type is its element type
*/
bool
tests_traversable_value_type_mirrors_siblings()
{
    bool ok = true;

    // the maybe structure is both a functor (via the monad bridge) and a
    // traversable, and both name the same A.
    static_assert(std::is_same<traversable_value_type_t<tmaybe<int> >,
                               functor_value_type_t<tmaybe<int> > >::value,
                  "traversable A == functor A");
    static_assert(std::is_same<
        traversable_value_type_t<tmaybe<std::string> >,
        functor_value_type_t<tmaybe<std::string> > >::value, "and for string");

    // the vector's A is its element type.
    static_assert(std::is_same<traversable_value_type_t<std::vector<int> >,
                               std::vector<int>::value_type>::value,
                  "vector A == element type");

    ok = ok && (std::is_same<traversable_value_type_t<tmaybe<int> >,
                             functor_value_type_t<tmaybe<int> > >::value);

    return ok;
}


/*
tests_identity_helper
  traversable_identity_helper is the function sequence is derived with: it returns
  its argument unchanged. A named functor, so it works in a trailing return type
  on every floor.
  Tests the following:
  - it returns its argument unchanged, across several types
  - it folds in a constant expression
  - applied to an effect, it hands back that very effect (which is exactly how
    sequence turns T<F<A>> into F<T<A>>)
*/
bool
tests_identity_helper()
{
    bool ok = true;

    internal::traversable_identity_helper id;

    ok = ok && (id(42) == 42);
    ok = ok && (id(std::string("hi")) == std::string("hi"));
    ok = ok && (id(true) == true);

    static_assert(internal::traversable_identity_helper{}(7) == 7,
                  "constexpr");

    // applied to an effect, it is the effect -- the key to sequence.
    const tbox<int> b = id(boxed(9));
    ok = ok && (b.value == 9);
    static_assert(std::is_same<
        decltype(internal::traversable_identity_helper{}(boxed(1))),
        tbox<int> >::value, "F<A> in, F<A> out");

    const tmaybe<int> m = id(nothing<int>());
    ok = ok && (!m.has());

    return ok;
}


/*
tests_append_helper
  traversable_append_helper is the reducer the sequence traversables hand to
  lift_a2 to grow the materialised vector inside the effect. Its signature is
  (std::vector<B>, const B&) -> std::vector<B>.
  Tests the following:
  - it appends the element at the END, preserving order
  - appending to an empty vector yields a singleton
  - repeated application builds the vector left-to-right
*/
bool
tests_append_helper()
{
    bool ok = true;

    const internal::traversable_append_helper<int> append;

    // appends at the end.
    const std::vector<int> one = append(std::vector<int>(), 1);
    ok = ok && (one.size() == 1u && one[0] == 1);

    const std::vector<int> two = append(one, 2);
    ok = ok && (two.size() == 2u && two[0] == 1 && two[1] == 2);

    // repeated application builds left-to-right.
    std::vector<int> acc;
    acc = append(acc, 10);
    acc = append(acc, 20);
    acc = append(acc, 30);
    ok = ok && (acc.size() == 3u);
    ok = ok && (acc[0] == 10 && acc[1] == 20 && acc[2] == 30);

    // it works over other element types.
    const internal::traversable_append_helper<std::string> append_s;
    const std::vector<std::string> ss = append_s(std::vector<std::string>(),
                                                 std::string("a"));
    ok = ok && (ss.size() == 1u && ss[0] == std::string("a"));

    return ok;
}


/*
tests_append_helper_threads_by_value
  The accumulator is threaded BY VALUE -- lift_a2 lifts a PURE binary function
  over the effect, so the reducer must not mutate its argument. The helper takes
  the vector by value and returns a new one.
  Tests the following:
  - the source vector is unchanged after an append
  - the returned vector is a distinct object carrying the new element
*/
bool
tests_append_helper_threads_by_value()
{
    bool ok = true;

    const internal::traversable_append_helper<int> append;

    std::vector<int> source;
    source.push_back(1);
    source.push_back(2);

    const std::vector<int> grown = append(source, 3);

    // the new vector has the element...
    ok = ok && (grown.size() == 3u);
    ok = ok && (grown[2] == 3);

    // ...and the source is untouched.
    ok = ok && (source.size() == 2u);
    ok = ok && (source[0] == 1 && source[1] == 2);

    // the parameter really is by value, not by reference.
    static_assert(std::is_same<
        decltype(append(std::declval<std::vector<int> >(), 0)),
        std::vector<int> >::value, "returns a vector by value");

    return ok;
}


/*
tests_traversable_concept
  Traversable is the PascalCase typeclass face of is_traversable, and it really
  GATES overload resolution.
  Tests the following:
  - it mirrors the trait, on both instances and on the negatives
  - a constrained overload wins for a traversable and is excluded otherwise --
    including for the mere applicative and for the marker-less near-miss
*/
bool
tests_traversable_concept()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Traversable<std::vector<int> > ==
                  is_traversable<std::vector<int> >::value, "mirrors");
    static_assert(Traversable<not_traversable> ==
                  is_traversable<not_traversable>::value, "mirrors, negative");

    static_assert(Traversable<std::vector<int> >, "the vector instance");
    static_assert(Traversable<tmaybe<int> >, "the maybe instance");
    static_assert(!Traversable<tbox<int> >,
                  "a Functor + Applicative is still not a Traversable");
    static_assert(!Traversable<no_marker>, "no marker");
    static_assert(!Traversable<int>, "a scalar");

    // and it gates.
    ok = ok && (which_traversable(std::vector<int>()) == 1);
    ok = ok && (which_traversable(just(1)) == 1);
    ok = ok && (which_traversable(boxed(1)) == 0);       // applicative only
    ok = ok && (which_traversable(no_marker()) == 0);
    ok = ok && (which_traversable(42) == 0);
#endif

    return ok;
}


/*
tests_traversable_concept_gating
  The concept face is gated to C++20, leaving is_traversable as the always-present
  floor beneath it.
  Tests the following:
  - the gate agrees with the language level
  - the trait answers identically whether or not the gate is open
*/
bool
tests_traversable_concept_gating()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = ok && (Traversable<std::vector<int> > ==
                is_traversable<std::vector<int> >::value);
    ok = ok && (Traversable<tmaybe<int> > ==
                is_traversable<tmaybe<int> >::value);
    #if !D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // the face is present on a standard that lacks concepts
    #endif
#else
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // the face is missing on a standard that has concepts
    #endif
#endif

    // the trait is the floor.
    ok = ok && (is_traversable<std::vector<int> >::value);
    ok = ok && (!is_traversable<tbox<int> >::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
