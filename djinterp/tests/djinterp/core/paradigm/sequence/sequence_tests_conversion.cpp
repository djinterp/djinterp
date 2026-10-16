/*******************************************************************************
* djinterp [core]                                  sequence_tests_conversion.cpp
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests_conversion.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : sequence_tests_conversion.cpp
//   Conversion: vec, a const view onto the underlying vector, and release,
// which hands that vector out by move.
//
//   release is where the two differ in kind: vec merely observes, while release
// transfers ownership. That the source is left empty afterwards is asserted
// here because the module documents it - it follows from vector's move
// construction stealing the buffer.

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "sequence_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_vec
  Verifies the const view onto the underlying storage.
  Tests the following:
  - vec yields a const reference to a vector of the element type
  - it holds the same elements, in the same order
  - an empty sequence yields an empty vector
*/
bool
tests_vec()
{
    int_sequence s{ 1, 2, 3 };

    static_assert(std::is_same<decltype(s.vec()),
                               const std::vector<int>&>::value,
                  "vec yields a const vector reference");

    const std::vector<int>& v = s.vec();

    bool ok = true;

    ok = ok && (v.size() == 3);
    ok = ok && (v[0] == 1 && v[1] == 2 && v[2] == 3);

    int_sequence none;
    ok = ok && (none.vec().empty());

    return ok;
}

/*
tests_vec_reflects_contents
  Verifies the view tracks the sequence rather than snapshotting it.
  Tests the following:
  - a reference obtained from vec sees elements appended afterwards
  - it sees the sequence cleared
*/
bool
tests_vec_reflects_contents()
{
    int_sequence s{ 1, 2 };

    const std::vector<int>& v = s.vec();

    bool ok = (v.size() == 2);

    s.push_back(3);
    ok = ok && (v.size() == 3 && v.back() == 3);

    s.clear();
    ok = ok && (v.empty());

    return ok;
}

/*
tests_release
  Verifies the storage is handed out.
  Tests the following:
  - release yields a vector by value, holding the sequence's elements in order
  - the elements survive the transfer intact
*/
bool
tests_release()
{
    int_sequence s{ 1, 2, 3 };

    static_assert(std::is_same<decltype(s.release()),
                               std::vector<int> >::value,
                  "release yields a vector by value");

    std::vector<int> out = s.release();

    bool ok = true;

    ok = ok && (out.size() == 3);
    ok = ok && (out[0] == 1 && out[1] == 2 && out[2] == 3);

    return ok;
}

/*
tests_release_empties_source
  Verifies release really moves rather than copies, leaving the sequence empty
  as the module documents.
  Tests the following:
  - the sequence is empty afterwards
  - it is still usable - it can be repopulated
  - the released elements are intact, so the transfer was a move of the storage
    and not a move of the elements out of a retained buffer
*/
bool
tests_release_empties_source()
{
    string_sequence s(std::string("a-long-string-defeating-small-string-optimisation"),
                      std::string("another-long-string-defeating-the-same"));

    std::vector<std::string> out = s.release();

    bool ok = true;

    ok = ok && (out.size() == 2);
    ok = ok && (out[0] == "a-long-string-defeating-small-string-optimisation");
    ok = ok && (!out[1].empty());          // the elements themselves were not gutted

    ok = ok && (s.empty());                // the source was emptied
    ok = ok && (s.size() == 0);

    // and it remains a usable sequence
    s.push_back(std::string("fresh"));
    ok = ok && (s.size() == 1 && s[0] == "fresh");

    return ok;
}


NS_END  // testing
NS_END  // djinterp
