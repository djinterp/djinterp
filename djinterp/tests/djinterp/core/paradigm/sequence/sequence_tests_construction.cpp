/*******************************************************************************
* djinterp [core]                                sequence_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : sequence_tests_construction.cpp
//   Construction. The interesting part of this section is not any individual
// overload but the RESOLUTION BETWEEN them: sequence offers a variadic ctor, an
// iterator-pair ctor, a pointer+count ctor and a count+fill ctor, several of
// which are two-parameter templates and therefore compete for the same calls.
//
//   The two-argument cases below are the ones that matter. With the iterator-
// pair overload constrained to genuine iterators (see the suite header's build
// note), a two-argument call of matching element type is variadic construction:
//     sequence<int>(1, 2)             -> {1, 2}      (two elements)
//     sequence<int>(size_type(3), 5)  -> {5, 5, 5}   (count + fill)
// Count+fill for an arithmetic element type therefore needs its count spelled
// as size_type; that is the unavoidable cost of offering a variadic ctor at all,
// and it is what the module's own documented usage - sequence<int>(1,2,3,4,5) -
// asks for.

// std
#include <cstddef>
#include <list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "sequence_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_ctor_default
  Verifies the default constructor.
  Tests the following:
  - the sequence is empty with size zero
*/
bool
tests_ctor_default()
{
    int_sequence s;

    bool ok = true;

    ok = ok && (s.empty());
    ok = ok && (s.size() == 0);

    return ok;
}

/*
tests_ctor_variadic_single
  Verifies variadic construction from one element.
  Tests the following:
  - a one-argument call yields a one-element sequence holding that value
*/
bool
tests_ctor_variadic_single()
{
    int_sequence s(7);

    bool ok = true;

    ok = ok && (s.size() == 1);
    ok = ok && (matches(s, { 7 }));

    return ok;
}

/*
tests_ctor_variadic_two
  Verifies variadic construction from exactly two elements - the case in which
  the unconstrained iterator-pair overload used to win overload resolution and
  forward the pair to std::vector, silently producing vector(count, value).
  Tests the following:
  - (1, 2) yields the two elements 1 and 2, not a one-element {2}
  - (3, 5) yields the two elements 3 and 5, not three copies of 5
*/
bool
tests_ctor_variadic_two()
{
    int_sequence a(1, 2);
    int_sequence b(3, 5);

    bool ok = true;

    ok = ok && (a.size() == 2);
    ok = ok && (matches(a, { 1, 2 }));      // NOT {2}

    ok = ok && (b.size() == 2);
    ok = ok && (matches(b, { 3, 5 }));      // NOT {5, 5, 5}

    return ok;
}

/*
tests_ctor_variadic_many
  Verifies the module's documented multi-element variadic form.
  Tests the following:
  - sequence<int>(1, 2, 3, 4, 5) holds those five elements in order
*/
bool
tests_ctor_variadic_many()
{
    int_sequence s(1, 2, 3, 4, 5);

    bool ok = true;

    ok = ok && (s.size() == 5);
    ok = ok && (matches(s, { 1, 2, 3, 4, 5 }));

    return ok;
}

/*
tests_ctor_variadic_class_type
  Verifies variadic construction for a class element type.
  Tests the following:
  - three string elements are stored in order
*/
bool
tests_ctor_variadic_class_type()
{
    string_sequence s(std::string("a"),
                      std::string("b"),
                      std::string("c"));

    bool ok = true;

    ok = ok && (s.size() == 3);
    ok = ok && (matches(s, { std::string("a"), std::string("b"), std::string("c") }));

    return ok;
}

/*
tests_ctor_variadic_two_class_type
  Verifies two-argument variadic construction for a class element type. This is
  the call that did not compile at all before the iterator-pair overload was
  constrained: std::string is not an iterator, so forwarding the pair to
  std::vector left no viable vector constructor.
  Tests the following:
  - two string elements are stored in order
*/
bool
tests_ctor_variadic_two_class_type()
{
    string_sequence s(std::string("first"), std::string("second"));

    bool ok = true;

    ok = ok && (s.size() == 2);
    ok = ok && (matches(s, { std::string("first"), std::string("second") }));

    return ok;
}

/*
tests_ctor_variadic_is_explicit
  Verifies the variadic constructor is explicit.
  Tests the following:
  - an element does not implicitly convert to a sequence
  - it does explicitly construct one
*/
bool
tests_ctor_variadic_is_explicit()
{
    static_assert(!std::is_convertible<int, int_sequence>::value,
                  "the variadic ctor is explicit");
    static_assert(std::is_constructible<int_sequence, int>::value,
                  "but it still constructs explicitly");

    return ( !std::is_convertible<int, int_sequence>::value &&
              std::is_constructible<int_sequence, int>::value );
}

/*
tests_ctor_variadic_sfinae_mixed_types
  Verifies the variadic constructor's SFINAE guard: every argument must be the
  element type exactly.
  Tests the following:
  - a mixed-type argument list does not construct a sequence
  - a uniform argument list does
*/
bool
tests_ctor_variadic_sfinae_mixed_types()
{
    static_assert(!std::is_constructible<string_sequence,
                                         std::string, int>::value,
                  "mixed argument types are rejected");
    static_assert(std::is_constructible<string_sequence,
                                        std::string, std::string>::value,
                  "a uniform list is accepted");

    return ( !std::is_constructible<string_sequence, std::string, int>::value &&
              std::is_constructible<string_sequence, std::string, std::string>::value );
}

/*
tests_ctor_initializer_list
  Verifies brace-enclosed construction.
  Tests the following:
  - the elements are stored in order
  - the two-element brace form is unambiguous (unlike the parenthesised form,
    braces always select the initializer_list overload)
*/
bool
tests_ctor_initializer_list()
{
    int_sequence s{ 1, 2, 3 };
    int_sequence two{ 1, 2 };

    bool ok = true;

    ok = ok && (matches(s, { 1, 2, 3 }));
    ok = ok && (matches(two, { 1, 2 }));

    return ok;
}

/*
tests_ctor_initializer_list_empty
  Verifies construction from an empty brace list.
  Tests the following:
  - the sequence is empty
*/
bool
tests_ctor_initializer_list_empty()
{
    int_sequence s = {};

    bool ok = true;

    ok = ok && (s.empty());
    ok = ok && (s.size() == 0);

    return ok;
}

/*
tests_ctor_iterator_pair_pointer
  Verifies construction from a raw pointer range.
  Tests the following:
  - the half-open range [begin, end) is copied in order
*/
bool
tests_ctor_iterator_pair_pointer()
{
    const int    src[] = { 4, 5, 6, 7 };
    int_sequence s(src, src + 4);

    bool ok = true;

    ok = ok && (s.size() == 4);
    ok = ok && (matches(s, { 4, 5, 6, 7 }));

    return ok;
}

/*
tests_ctor_iterator_pair_vector
  Verifies construction from a vector's iterators.
  Tests the following:
  - the range is copied in order
  - a sub-range of the source is honoured
*/
bool
tests_ctor_iterator_pair_vector()
{
    std::vector<int> src{ 1, 2, 3, 4 };

    int_sequence all(src.begin(), src.end());
    int_sequence part(src.begin() + 1, src.begin() + 3);

    bool ok = true;

    ok = ok && (matches(all, { 1, 2, 3, 4 }));
    ok = ok && (matches(part, { 2, 3 }));

    return ok;
}

/*
tests_ctor_iterator_pair_non_contiguous
  Verifies construction from a non-contiguous forward range, proving the
  overload accepts any iterator rather than only pointers.
  Tests the following:
  - a std::list's iterators produce the same elements in order
*/
bool
tests_ctor_iterator_pair_non_contiguous()
{
    std::list<int> src{ 8, 9, 10 };

    int_sequence s(src.begin(), src.end());

    bool ok = true;

    ok = ok && (s.size() == 3);
    ok = ok && (matches(s, { 8, 9, 10 }));

    return ok;
}

/*
tests_ctor_iterator_pair_empty_range
  Verifies construction from an empty range.
  Tests the following:
  - begin == end yields an empty sequence
*/
bool
tests_ctor_iterator_pair_empty_range()
{
    std::vector<int> src;

    int_sequence s(src.begin(), src.end());

    return (s.empty() && s.size() == 0);
}

/*
tests_ctor_pointer_count
  Verifies construction from a contiguous array and a count.
  Tests the following:
  - exactly _count elements are copied from the pointer
  - a partial prefix of the array is honoured
*/
bool
tests_ctor_pointer_count()
{
    const int src[] = { 1, 2, 3, 4, 5 };

    int_sequence all(src, 5);
    int_sequence prefix(src, 3);

    bool ok = true;

    ok = ok && (matches(all, { 1, 2, 3, 4, 5 }));
    ok = ok && (matches(prefix, { 1, 2, 3 }));

    return ok;
}

/*
tests_ctor_pointer_count_zero
  Verifies construction from a zero-length pointer range.
  Tests the following:
  - a count of zero yields an empty sequence
*/
bool
tests_ctor_pointer_count_zero()
{
    const int src[] = { 1, 2, 3 };

    int_sequence s(src, 0);

    return (s.empty() && s.size() == 0);
}

/*
tests_ctor_vector_copy
  Verifies construction by copying a vector.
  Tests the following:
  - the elements are copied in order
  - the source vector is left intact
*/
bool
tests_ctor_vector_copy()
{
    const std::vector<int> src{ 1, 2, 3 };

    int_sequence s(src);

    bool ok = true;

    ok = ok && (matches(s, { 1, 2, 3 }));
    ok = ok && (src.size() == 3);          // the source was only read

    return ok;
}

/*
tests_ctor_vector_move
  Verifies construction by taking ownership of a vector.
  Tests the following:
  - the elements land in the sequence
  - the source vector really was moved from, not copied
*/
bool
tests_ctor_vector_move()
{
    std::vector<std::string> src{
        std::string("a-long-string-defeating-small-string-optimisation"),
        std::string("another-long-string-defeating-the-same") };

    string_sequence s(std::move(src));

    bool ok = true;

    ok = ok && (s.size() == 2);
    ok = ok && (s[0] == "a-long-string-defeating-small-string-optimisation");
    ok = ok && (src.empty());              // ownership was taken, not copied

    return ok;
}

/*
tests_ctor_count_value_fill
  Verifies fill construction. For an arithmetic element type the count must be
  spelled as size_type, since a bare (3, 5) is a two-element variadic call.
  Tests the following:
  - size_type(3) with the value 5 yields three copies of 5
*/
bool
tests_ctor_count_value_fill()
{
    int_sequence s(int_sequence::size_type(3), 5);

    bool ok = true;

    ok = ok && (s.size() == 3);
    ok = ok && (matches(s, { 5, 5, 5 }));

    return ok;
}

/*
tests_ctor_count_value_zero
  Verifies fill construction with a count of zero.
  Tests the following:
  - the sequence is empty
*/
bool
tests_ctor_count_value_zero()
{
    int_sequence s(int_sequence::size_type(0), 5);

    return (s.empty() && s.size() == 0);
}

/*
tests_ctor_copy
  Verifies the implicit copy constructor gives value semantics.
  Tests the following:
  - the copy holds the same elements
  - mutating the copy does not disturb the original
*/
bool
tests_ctor_copy()
{
    int_sequence original{ 1, 2, 3 };
    int_sequence copy(original);

    bool ok = (matches(copy, { 1, 2, 3 }));

    copy[0] = 99;
    copy.push_back(4);

    ok = ok && (matches(original, { 1, 2, 3 }));   // untouched
    ok = ok && (matches(copy, { 99, 2, 3, 4 }));

    return ok;
}

/*
tests_ctor_move
  Verifies the implicit move constructor.
  Tests the following:
  - the elements land in the destination
  - the source really was moved from (its storage was taken, not copied)
*/
bool
tests_ctor_move()
{
    string_sequence source(std::string("a-long-string-defeating-small-string-optimisation"),
                           std::string("another-long-string-defeating-the-same"));

    string_sequence sink(std::move(source));

    bool ok = true;

    ok = ok && (sink.size() == 2);
    ok = ok && (sink[0] == "a-long-string-defeating-small-string-optimisation");
    ok = ok && (source.empty());           // storage was taken

    return ok;
}


NS_END  // testing
NS_END  // djinterp
