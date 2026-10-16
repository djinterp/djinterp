/*******************************************************************************
* djinterp [core]                                    sequence_tests_capacity.cpp
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests_capacity.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : sequence_tests_capacity.cpp
//   Capacity: size and empty. Both are thin forwards to the underlying vector,
// so what is worth pinning is that they stay in step with every operation the
// module exposes - including reserve, which must NOT change either.

// std
#include <cstddef>
// djinterp
#include "sequence_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_size
  Verifies the element count.
  Tests the following:
  - a default sequence has size zero
  - size reports the number of elements constructed into it
*/
bool
tests_size()
{
    int_sequence none;
    int_sequence one{ 1 };
    int_sequence three{ 1, 2, 3 };

    bool ok = true;

    ok = ok && (none.size() == 0);
    ok = ok && (one.size() == 1);
    ok = ok && (three.size() == 3);

    return ok;
}

/*
tests_empty
  Verifies the emptiness predicate.
  Tests the following:
  - a default sequence is empty
  - a populated sequence is not
  - emptiness is exactly "size is zero"
*/
bool
tests_empty()
{
    int_sequence none;
    int_sequence some{ 1 };

    bool ok = true;

    ok = ok && ( none.empty());
    ok = ok && (!some.empty());
    ok = ok && (none.empty() == (none.size() == 0));
    ok = ok && (some.empty() == (some.size() == 0));

    return ok;
}

/*
tests_capacity_tracks_modification
  Verifies size and empty follow every modifying operation.
  Tests the following:
  - push_back grows the count by one
  - append grows it by the source's size
  - resize sets it outright, in both directions
  - reserve leaves it alone
  - clear returns it to zero and the sequence to empty
*/
bool
tests_capacity_tracks_modification()
{
    int_sequence s;

    bool ok = (s.empty() && s.size() == 0);

    s.push_back(1);
    ok = ok && (!s.empty() && s.size() == 1);

    s.append(int_sequence{ 2, 3 });
    ok = ok && (s.size() == 3);

    s.resize(5);
    ok = ok && (s.size() == 5);

    s.resize(2);
    ok = ok && (s.size() == 2);

    s.reserve(100);
    ok = ok && (s.size() == 2 && !s.empty());   // reserve changes neither

    s.clear();
    ok = ok && (s.empty() && s.size() == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
