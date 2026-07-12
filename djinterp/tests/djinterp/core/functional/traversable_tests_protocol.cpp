// djinterp [test]  traversable_tests_protocol.cpp
//   Section I -- traversable_traits (primary, undefined) and is_traversable.

// std
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "traversable_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_traversable_positive
  Both instances are recognised: the std::vector the header ships, and the maybe
  structure this suite adds.
  Tests the following:
  - std::vector, at several element types, is a traversable
  - the maybe structure is one too
*/
bool
tests_is_traversable_positive()
{
    bool ok = true;

    ok = ok && (is_traversable<std::vector<int> >::value);
    ok = ok && (is_traversable<std::vector<std::string> >::value);
    ok = ok && (is_traversable<std::vector<tmaybe<int> > >::value);

    ok = ok && (is_traversable<tmaybe<int> >::value);
    ok = ok && (is_traversable<tmaybe<std::string> >::value);

    return ok;
}


/*
tests_is_traversable_negative
  Everything else is refused -- including the effect-producing function, which is
  a mapping function and not a structure to walk.
  Tests the following:
  - an unrelated struct, scalars, and a function object are not traversables
*/
bool
tests_is_traversable_negative()
{
    bool ok = true;

    ok = ok && (!is_traversable<not_traversable>::value);
    ok = ok && (!is_traversable<int>::value);
    ok = ok && (!is_traversable<std::string>::value);

    // an effect-producing function is not a traversable.
    ok = ok && (!is_traversable<to_box>::value);
    auto lam = [](int _x){ return boxed(_x); };
    ok = ok && (!is_traversable<decltype(lam)>::value);

    return ok;
}


/*
tests_is_traversable_is_a_separate_obligation
  The sharpest negative in the suite. Traversal needs a structure that is both a
  Functor and a Foldable, and an Applicative to thread -- but being those things
  does NOT make a type traversable. tbox is a Functor AND an Applicative (both by
  the monad bridge) and is still not a Traversable, because nothing supplied the
  one obligation: a traversable_traits with a traverse.
  Tests the following:
  - tbox is a functor, an applicative, and a monad
  - tbox is NOT a traversable
  - tmaybe, which HAS the specialization, is all four
*/
bool
tests_is_traversable_is_a_separate_obligation()
{
    bool ok = true;

    // tbox has everything except the one thing that matters here.
    ok = ok && (is_monad<tbox<int> >::value);
    ok = ok && (is_functor<tbox<int> >::value);
    ok = ok && (is_applicative<tbox<int> >::value);
    ok = ok && (!is_traversable<tbox<int> >::value);

    // tmaybe carries the specialization, so it is all of them.
    ok = ok && (is_functor<tmaybe<int> >::value);
    ok = ok && (is_applicative<tmaybe<int> >::value);
    ok = ok && (is_traversable<tmaybe<int> >::value);

    return ok;
}


/*
tests_is_traversable_decay
  Detection applies std::decay, so cv-qualifiers and references are stripped
  first.
  Tests the following:
  - const / reference / rvalue / volatile spellings all resolve to the traversable
  - a negative stays negative through the same decay
*/
bool
tests_is_traversable_decay()
{
    bool ok = true;

    ok = ok && (is_traversable<const std::vector<int> >::value);
    ok = ok && (is_traversable<std::vector<int>& >::value);
    ok = ok && (is_traversable<const std::vector<int>& >::value);
    ok = ok && (is_traversable<std::vector<int>&& >::value);
    ok = ok && (is_traversable<volatile std::vector<int> >::value);

    ok = ok && (is_traversable<const tmaybe<int>& >::value);

    ok = ok && (!is_traversable<const not_traversable&>::value);

    return ok;
}


/*
tests_is_traversable_requires_marker
  Detection keys on the is_specialized marker, not on the mere presence of a
  traversable_traits specialization or of a traverse.
  Tests the following:
  - a specialization carrying a traverse but NO marker is not detected
  - it really does have both a traits and a working traverse (only the marker is
    missing)
*/
bool
tests_is_traversable_requires_marker()
{
    bool ok = true;

    ok = ok && (!is_traversable<no_marker>::value);

    // the traits exists, and its traverse runs.
    ok = ok && (is_complete<traversable_traits<no_marker> >::value);
    ok = ok && (traversable_traits<no_marker>::traverse(no_marker(), to_box())
                == 0);

    // the contrast.
    ok = ok && (is_traversable<std::vector<int> >::value);
    ok = ok && (is_traversable<tmaybe<int> >::value);

    return ok;
}


/*
tests_traversable_traits_primary_is_undefined
  The primary template is declared but left UNDEFINED, so a use on a
  non-traversable is a clean resolution error rather than a silently wrong answer.
  That makes it an INCOMPLETE type, which a completeness probe reads SFINAE-safely.
  Tests the following:
  - traversable_traits<non-traversable> is incomplete, for a struct and a scalar
  - traversable_traits<traversable> is complete, for both instances
*/
bool
tests_traversable_traits_primary_is_undefined()
{
    bool ok = true;

    static_assert(is_complete<int>::value, "the probe is sound");

    static_assert(!is_complete<traversable_traits<not_traversable> >::value,
                  "no specialization -> incomplete");
    static_assert(!is_complete<traversable_traits<int> >::value,
                  "a scalar -> incomplete");
    static_assert(!is_complete<traversable_traits<tbox<int> > >::value,
                  "an applicative that is not a traversable -> incomplete");

    static_assert(is_complete<traversable_traits<std::vector<int> > >::value,
                  "the vector instance");
    static_assert(is_complete<traversable_traits<tmaybe<int> > >::value,
                  "the maybe instance");

    ok = ok && (!is_complete<traversable_traits<not_traversable> >::value);

    return ok;
}


/*
tests_traversable_traits_surface
  A specialization exposes exactly the protocol surface: the marker, the inner
  value type, and traverse -- traverse being the whole obligation.
  Tests the following:
  - is_specialized is std::true_type on both instances
  - value_type names the inner type A of T<A>
  - traverse is callable and produces F<T'<B>>
*/
bool
tests_traversable_traits_surface()
{
    bool ok = true;

    // the vector instance.
    static_assert(std::is_same<
        traversable_traits<std::vector<int> >::is_specialized,
        std::true_type>::value, "vector marker");
    static_assert(std::is_same<
        traversable_traits<std::vector<int> >::value_type,
        int>::value, "vector value_type");

    // the maybe instance.
    static_assert(std::is_same<
        traversable_traits<tmaybe<int> >::is_specialized,
        std::true_type>::value, "maybe marker");
    static_assert(std::is_same<
        traversable_traits<tmaybe<std::string> >::value_type,
        std::string>::value, "maybe value_type");

    // traverse, on both.
    const std::vector<int> xs{ 1, 2, 3 };
    const auto             rv = traversable_traits<std::vector<int> >::traverse(
        xs, to_box());
    ok = ok && (rv.value.size() == 3u);

    const auto rm = traversable_traits<tmaybe<int> >::traverse(just(4),
                                                               to_box());
    ok = ok && (rm.value.has() && rm.value.get() == 40);

    return ok;
}


/*
tests_is_traversable_v_agrees
  The _v shorthand is exactly the trait's value.
  Tests the following:
  - the two agree, positively and negatively and through decay
*/
bool
tests_is_traversable_v_agrees()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_traversable_v<std::vector<int> > ==
                is_traversable<std::vector<int> >::value);
    ok = ok && (is_traversable_v<not_traversable> ==
                is_traversable<not_traversable>::value);

    static_assert(is_traversable_v<std::vector<int> >, "the vector instance");
    static_assert(is_traversable_v<tmaybe<int> >, "the maybe instance");
    static_assert(!is_traversable_v<tbox<int> >, "an applicative is not one");
    static_assert(!is_traversable_v<no_marker>, "no marker");
    static_assert(is_traversable_v<const std::vector<int>&>, "decayed");
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
