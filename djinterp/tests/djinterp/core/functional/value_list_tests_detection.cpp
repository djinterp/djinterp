// djinterp [test]  value_list_tests_detection.cpp
//   Section II -- is_value_list (+ _v) and the ValueList concept face.

// std
#include <tuple>
#include <type_traits>
// djinterp
#include "value_list_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_value_list_positive
  Any value_list specialization is recognised.
  Tests the following:
  - populated lists, a singleton, and the EMPTY list (a list all the same)
  - a heterogeneous list
*/
bool
tests_is_value_list_positive()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    ok = ok && (is_value_list<value_list<1, 2, 3> >::value);
    ok = ok && (is_value_list<value_list<1> >::value);
    ok = ok && (is_value_list<value_list<> >::value);
    ok = ok && (is_value_list<value_list<10, 'x', true, color::red> >::value);
#endif

    return ok;
}


/*
tests_is_value_list_negative
  Everything else is refused -- notably std::tuple, which is the TYPE-domain
  sequence this module is the value-domain counterpart of.
  Tests the following:
  - scalars, an unrelated struct, and a value CARRIER (val_t is not a list)
  - std::tuple, the type sequence
*/
bool
tests_is_value_list_negative()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    ok = ok && (!is_value_list<int>::value);
    ok = ok && (!is_value_list<not_a_list>::value);

    // the type-domain sequence is not the value-domain one.
    ok = ok && (!is_value_list<std::tuple<int, char> >::value);
    ok = ok && (!is_value_list<std::tuple<> >::value);

    // a single carrier is not a sequence of them.
    ok = ok && (!is_value_list<val_t<1> >::value);
#endif

    return ok;
}


/*
tests_is_value_list_exact
  Detection is specialization-based, so it is EXACT: mimicking the interface does
  not qualify, and neither does deriving from a list.
  Tests the following:
  - a look-alike exposing the same static size() is refused
  - a class DERIVED from a value_list is refused (it is not a specialization)
  - the base it derives from is of course accepted
*/
bool
tests_is_value_list_exact()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // it has the interface, but it is not a value_list.
    ok = ok && (fake_list::size() == 3u);
    ok = ok && (!is_value_list<fake_list>::value);

    // derived-from is not is-a, for a specialization match.
    ok = ok && (std::is_base_of<value_list<1, 2>, derived_list>::value);
    ok = ok && (!is_value_list<derived_list>::value);
    ok = ok && (is_value_list<value_list<1, 2> >::value);
#endif

    return ok;
}


/*
tests_is_value_list_cvref
  clean_t is applied first, so cv-qualifiers and references are stripped before
  the match.
  Tests the following:
  - const / reference / rvalue / volatile spellings all resolve to the list
  - a negative stays negative through the same decay
*/
bool
tests_is_value_list_cvref()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    ok = ok && (is_value_list<const value_list<1> >::value);
    ok = ok && (is_value_list<value_list<1>& >::value);
    ok = ok && (is_value_list<const value_list<1>& >::value);
    ok = ok && (is_value_list<value_list<1>&& >::value);
    ok = ok && (is_value_list<volatile value_list<1> >::value);

    ok = ok && (!is_value_list<const int&>::value);
#endif

    return ok;
}


/*
tests_is_value_list_v_agrees
  The _v companion (emitted by D_TYPE_TRAIT_VALUE_BOOL) is exactly the trait's
  value.
  Tests the following:
  - the two agree, positively and negatively and through cv-ref
  - the shorthand is a constant expression of type bool
*/
bool
tests_is_value_list_v_agrees()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    ok = ok && (is_value_list_v<value_list<1, 2> > ==
                is_value_list<value_list<1, 2> >::value);
    ok = ok && (is_value_list_v<int> == is_value_list<int>::value);

    static_assert(is_value_list_v<value_list<> >, "empty list");
    static_assert(!is_value_list_v<fake_list>, "look-alike");
    static_assert(is_value_list_v<const value_list<1>&>, "clean_t applied");
    static_assert(std::is_same<decltype(is_value_list_v<value_list<1> >),
                               const bool>::value, "typed bool");
#endif

    return ok;
}


/*
tests_concept_value_list
  ValueList is the concept face of is_value_list, and it really GATES resolution
  -- it is a constraint, not merely a bool that happens to be true.
  Tests the following:
  - the face mirrors the trait, positively and negatively
  - a ValueList-constrained function accepts a list and computes over it
  - a constrained overload wins for a list and is excluded for anything else
*/
bool
tests_concept_value_list()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
#  if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(ValueList<value_list<1, 2> > ==
                  is_value_list<value_list<1, 2> >::value, "mirrors");
    static_assert(ValueList<int> == is_value_list<int>::value,
                  "mirrors, negative");

    static_assert(ValueList<value_list<> >, "the empty list");
    static_assert(!ValueList<fake_list>, "a look-alike is not one");
    static_assert(!ValueList<derived_list>, "derived is not a specialization");
    static_assert(!ValueList<std::tuple<int> >, "the type sequence is not one");

    // a constrained function, applied.
    static_assert(list_size_of(value_list<1, 2, 3>{}) == 3u, "constrained call");
    ok = ok && (list_size_of(value_list<1, 2, 3>{}) == 3u);
    ok = ok && (list_size_of(value_list<>{}) == 0u);

    // and it gates overload resolution.
    ok = ok && (which_list(value_list<1>{}) == 1);
    ok = ok && (which_list(value_list<>{}) == 1);
    ok = ok && (which_list(fake_list{}) == 0);
    ok = ok && (which_list(derived_list{}) == 0);
    ok = ok && (which_list(42) == 0);
#  endif
#endif

    return ok;
}


/*
tests_concept_value_list_gating
  The concept face is gated to C++20, one tier above the module's own C++17 floor,
  leaving is_value_list as the always-present face in between.
  Tests the following:
  - the gate agrees with the language level
  - the trait answers identically whether or not the gate is open
*/
bool
tests_concept_value_list_gating()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
#  if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = ok && (ValueList<value_list<1> > == is_value_list<value_list<1> >::value);
#    if !D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // the face is present on a standard that lacks concepts
#    endif
#  else
#    if D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // the face is missing on a standard that has concepts
#    endif
#  endif

    // the trait is the floor from C++17 up, gate or no gate.
    ok = ok && (is_value_list<value_list<1> >::value);
    ok = ok && (!is_value_list<int>::value);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
