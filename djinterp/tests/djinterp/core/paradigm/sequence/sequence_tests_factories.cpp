/*******************************************************************************
* djinterp [core]                                   sequence_tests_factories.cpp
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests_factories.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : sequence_tests_factories.cpp
//   The make_sequence factories: the variadic form, the pointer+count form, and
// the C-array form (whose extent is deduced).
//
//   The two-argument variadic case is repeated here deliberately. The factory
// forwards its arguments straight into a sequence constructor, so the
// constructor's overload-resolution bug propagated through it unchanged -
// make_sequence<int>(1, 2) returned a ONE-element sequence holding 2. A factory
// is exactly the kind of thin wrapper whose own tests are assumed to be
// redundant, which is why it gets its own case.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "sequence_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_make_sequence_variadic
  Verifies the variadic factory.
  Tests the following:
  - the element type is the one named explicitly, not deduced from the arguments
  - the elements are stored in order
*/
bool
tests_make_sequence_variadic()
{
    auto s = make_sequence<int>(1, 2, 3, 4, 5);

    static_assert(std::is_same<decltype(s), int_sequence>::value,
                  "the element type is the explicit template argument");

    bool ok = true;

    ok = ok && (s.size() == 5);
    ok = ok && (matches(s, { 1, 2, 3, 4, 5 }));

    return ok;
}

/*
tests_make_sequence_variadic_two
  Verifies the variadic factory with exactly two arguments - the call that
  inherited the constructor's overload-resolution bug and silently produced a
  one-element sequence.
  Tests the following:
  - (1, 2) yields the two elements 1 and 2, not a one-element {2}
  - (3, 5) yields the two elements 3 and 5, not three copies of 5
*/
bool
tests_make_sequence_variadic_two()
{
    auto a = make_sequence<int>(1, 2);
    auto b = make_sequence<int>(3, 5);

    bool ok = true;

    ok = ok && (a.size() == 2);
    ok = ok && (matches(a, { 1, 2 }));      // NOT {2}

    ok = ok && (b.size() == 2);
    ok = ok && (matches(b, { 3, 5 }));      // NOT {5, 5, 5}

    return ok;
}

/*
tests_make_sequence_variadic_single
  Verifies the variadic factory with one argument.
  Tests the following:
  - a single element is stored
*/
bool
tests_make_sequence_variadic_single()
{
    auto s = make_sequence<int>(7);

    bool ok = true;

    ok = ok && (s.size() == 1);
    ok = ok && (matches(s, { 7 }));

    return ok;
}

/*
tests_make_sequence_variadic_class_type
  Verifies the variadic factory for a class element type, including the
  two-argument form that previously failed to compile.
  Tests the following:
  - two string elements are stored in order
  - three string elements are stored in order
*/
bool
tests_make_sequence_variadic_class_type()
{
    auto two = make_sequence<std::string>(std::string("a"), std::string("b"));
    auto three = make_sequence<std::string>(std::string("x"),
                                            std::string("y"),
                                            std::string("z"));

    bool ok = true;

    ok = ok && (two.size() == 2);
    ok = ok && (matches(two, { std::string("a"), std::string("b") }));

    ok = ok && (three.size() == 3);
    ok = ok && (matches(three, { std::string("x"), std::string("y"), std::string("z") }));

    return ok;
}

/*
tests_make_sequence_pointer_count
  Verifies the pointer+count factory.
  Tests the following:
  - exactly count elements are copied from the pointer
  - a partial prefix of the source array is honoured
  - a count of zero yields an empty sequence
*/
bool
tests_make_sequence_pointer_count()
{
    const int src[] = { 1, 2, 3, 4, 5 };

    auto all    = make_sequence<int>(src, std::size_t(5));
    auto prefix = make_sequence<int>(src, std::size_t(3));
    auto none   = make_sequence<int>(src, std::size_t(0));

    bool ok = true;

    ok = ok && (matches(all, { 1, 2, 3, 4, 5 }));
    ok = ok && (matches(prefix, { 1, 2, 3 }));
    ok = ok && (none.empty());

    return ok;
}

/*
tests_make_sequence_c_array
  Verifies the C-array factory, whose extent is deduced from the array type.
  Tests the following:
  - every element of the array is copied, in order, with no count supplied
  - a single-element array works too
*/
bool
tests_make_sequence_c_array()
{
    const int  src[] = { 1, 2, 3, 4 };
    const int  one[] = { 9 };

    auto s      = make_sequence<int>(src);
    auto single = make_sequence<int>(one);

    bool ok = true;

    ok = ok && (s.size() == 4);             // the extent was deduced
    ok = ok && (matches(s, { 1, 2, 3, 4 }));

    ok = ok && (single.size() == 1);
    ok = ok && (matches(single, { 9 }));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
