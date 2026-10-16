/*******************************************************************************
* djinterp [core]                              sequence_tests_element_access.cpp
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests_element_access.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : sequence_tests_element_access.cpp
//   Element access: operator[], front, back and data, each in its const and
// mutable form. The const overloads are reached through a const reference, so
// the pair is genuinely distinguished rather than both calls landing on the
// mutable one; the mutable overloads are exercised by writing through them.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "sequence_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_index_operator
  Verifies indexed read access.
  Tests the following:
  - each index yields the element stored at that position
*/
bool
tests_index_operator()
{
    int_sequence s{ 10, 20, 30 };

    bool ok = true;

    ok = ok && (s[0] == 10);
    ok = ok && (s[1] == 20);
    ok = ok && (s[2] == 30);

    return ok;
}

/*
tests_index_operator_const
  Verifies indexed read access through a const reference.
  Tests the following:
  - the const overload is selected and yields the stored elements
*/
bool
tests_index_operator_const()
{
    int_sequence        mutable_seq{ 10, 20, 30 };
    const int_sequence& s = mutable_seq;

    static_assert(std::is_same<decltype(s[0]),
                               int_sequence::const_reference>::value,
                  "const access yields a const reference");

    bool ok = true;

    ok = ok && (s[0] == 10);
    ok = ok && (s[2] == 30);

    return ok;
}

/*
tests_index_operator_mutates
  Verifies indexed write access.
  Tests the following:
  - assigning through operator[] changes the stored element
*/
bool
tests_index_operator_mutates()
{
    int_sequence s{ 1, 2, 3 };

    s[1] = 99;

    return matches(s, { 1, 99, 3 });
}

/*
tests_front
  Verifies front returns the first element.
  Tests the following:
  - front is the element at index zero, for one- and many-element sequences
*/
bool
tests_front()
{
    int_sequence many{ 7, 8, 9 };
    int_sequence one{ 42 };

    bool ok = true;

    ok = ok && (many.front() == 7);
    ok = ok && (one.front() == 42);
    ok = ok && (one.front() == one.back());   // a single element is both

    return ok;
}

/*
tests_front_const
  Verifies front through a const reference.
  Tests the following:
  - the const overload is selected and yields the first element
*/
bool
tests_front_const()
{
    int_sequence        mutable_seq{ 7, 8, 9 };
    const int_sequence& s = mutable_seq;

    static_assert(std::is_same<decltype(s.front()),
                               int_sequence::const_reference>::value,
                  "const front yields a const reference");

    return (s.front() == 7);
}

/*
tests_front_mutates
  Verifies writing through front.
  Tests the following:
  - assigning to front changes the first element and nothing else
*/
bool
tests_front_mutates()
{
    int_sequence s{ 1, 2, 3 };

    s.front() = 99;

    return matches(s, { 99, 2, 3 });
}

/*
tests_back
  Verifies back returns the last element.
  Tests the following:
  - back is the element at index size-1
  - it follows the sequence as elements are appended
*/
bool
tests_back()
{
    int_sequence s{ 7, 8, 9 };

    bool ok = (s.back() == 9);

    s.push_back(10);
    ok = ok && (s.back() == 10);

    return ok;
}

/*
tests_back_const
  Verifies back through a const reference.
  Tests the following:
  - the const overload is selected and yields the last element
*/
bool
tests_back_const()
{
    int_sequence        mutable_seq{ 7, 8, 9 };
    const int_sequence& s = mutable_seq;

    static_assert(std::is_same<decltype(s.back()),
                               int_sequence::const_reference>::value,
                  "const back yields a const reference");

    return (s.back() == 9);
}

/*
tests_back_mutates
  Verifies writing through back.
  Tests the following:
  - assigning to back changes the last element and nothing else
*/
bool
tests_back_mutates()
{
    int_sequence s{ 1, 2, 3 };

    s.back() = 99;

    return matches(s, { 1, 2, 99 });
}

/*
tests_data_const
  Verifies the const data pointer.
  Tests the following:
  - the const overload yields a const pointer
  - the elements are readable through it
*/
bool
tests_data_const()
{
    int_sequence        mutable_seq{ 1, 2, 3 };
    const int_sequence& s = mutable_seq;

    static_assert(std::is_same<decltype(s.data()),
                               int_sequence::const_pointer>::value,
                  "const data yields a const pointer");

    const int* p = s.data();

    bool ok = true;

    ok = ok && (p != nullptr);
    ok = ok && (p[0] == 1 && p[1] == 2 && p[2] == 3);

    return ok;
}

/*
tests_data_mutable
  Verifies the mutable data pointer.
  Tests the following:
  - the mutable overload yields a plain pointer
  - writing through it changes the sequence's elements
*/
bool
tests_data_mutable()
{
    int_sequence s{ 1, 2, 3 };

    static_assert(std::is_same<decltype(s.data()),
                               int_sequence::pointer>::value,
                  "mutable data yields a mutable pointer");

    s.data()[1] = 99;

    return matches(s, { 1, 99, 3 });
}

/*
tests_data_contiguous
  Verifies the storage really is contiguous, as the module promises.
  Tests the following:
  - each element sits one stride past the previous one
  - the pointer range agrees with the index operator across the whole sequence
*/
bool
tests_data_contiguous()
{
    int_sequence s{ 10, 20, 30, 40 };

    const int* p = s.data();

    bool ok = true;

    for (int_sequence::size_type i = 0; i < s.size(); ++i)
    {
        ok = ok && (&p[i] == &s[i]);       // same object, contiguously laid out
        ok = ok && (p[i] == s[i]);
    }

    ok = ok && ((p + s.size()) - p == static_cast<int_sequence::difference_type>(s.size()));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
