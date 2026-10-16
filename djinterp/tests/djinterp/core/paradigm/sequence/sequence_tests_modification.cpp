/*******************************************************************************
* djinterp [core]                                sequence_tests_modification.cpp
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests_modification.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : sequence_tests_modification.cpp
//   Modification: push_back in both overloads, append, clear, reserve and
// resize.
//
//   Two cases carry the weight here. The push_back(_Type&&) overload is proved
// to MOVE rather than copy by inspecting the source object (the shared probe
// fixture records having been moved from), not the stored element - the
// underlying vector is free to shuffle stored elements around during
// reallocation, so only the source is a reliable witness. And append() is
// exercised against ITSELF: inserting from a range that aliases the container's
// own storage is undefined, and in practice the reallocating insert
// move-constructs the existing elements into the new buffer before copying the
// source range, so a move-enabled element type reads back empty in the appended
// half unless the operation guards for self-append (see the suite header).

// std
#include <cstddef>
#include <string>
#include <utility>
// djinterp
#include "sequence_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_push_back_copy
  Verifies appending a copy.
  Tests the following:
  - the element is appended at the end
  - the source is left intact - it was copied, not moved
*/
bool
tests_push_back_copy()
{
    sequence<probe> s;

    probe source(7);

    s.push_back(source);                   // lvalue -> the copying overload

    bool ok = true;

    ok = ok && (s.size() == 1);
    ok = ok && (s[0].value == 7);
    ok = ok && (!source.moved_from);       // the source was not moved from
    ok = ok && (source.value == 7);        // and still holds its value

    s.push_back(probe(8));
    ok = ok && (s.size() == 2 && s[1].value == 8);

    return ok;
}

/*
tests_push_back_move
  Verifies appending by move.
  Tests the following:
  - the element is appended at the end with its value intact
  - the source really was moved from, so the rvalue overload was selected
*/
bool
tests_push_back_move()
{
    sequence<probe> s;

    probe source(9);

    s.push_back(std::move(source));        // rvalue -> the moving overload

    bool ok = true;

    ok = ok && (s.size() == 1);
    ok = ok && (s[0].value == 9);
    ok = ok && (source.moved_from);        // the moving overload really ran

    // and the same for a class type with observable move semantics
    string_sequence strings;
    std::string     long_source = "a-long-string-defeating-small-string-optimisation";

    strings.push_back(std::move(long_source));

    ok = ok && (strings.size() == 1);
    ok = ok && (strings[0] == "a-long-string-defeating-small-string-optimisation");
    ok = ok && (long_source.empty());      // moved out of, not copied

    return ok;
}

/*
tests_append
  Verifies appending another sequence.
  Tests the following:
  - the source's elements land at the end, in order
  - the source is left untouched
*/
bool
tests_append()
{
    int_sequence       target{ 1, 2 };
    const int_sequence source{ 3, 4, 5 };

    target.append(source);

    bool ok = true;

    ok = ok && (matches(target, { 1, 2, 3, 4, 5 }));
    ok = ok && (matches(source, { 3, 4, 5 }));   // the source is unchanged

    return ok;
}

/*
tests_append_empty_source
  Verifies appending an empty sequence.
  Tests the following:
  - the target is unchanged
*/
bool
tests_append_empty_source()
{
    int_sequence target{ 1, 2, 3 };
    int_sequence empty;

    target.append(empty);

    return matches(target, { 1, 2, 3 });
}

/*
tests_append_to_empty
  Verifies appending onto an empty sequence.
  Tests the following:
  - the target ends up equal to the source
*/
bool
tests_append_to_empty()
{
    int_sequence target;
    int_sequence source{ 1, 2, 3 };

    target.append(source);

    bool ok = true;

    ok = ok && (matches(target, { 1, 2, 3 }));
    ok = ok && (target == source);

    return ok;
}

/*
tests_append_self
  Verifies appending a sequence to itself. Without a self-append guard this
  inserts from a range aliasing the container's own storage: the reallocating
  insert move-constructs the existing elements into the new buffer first, so the
  copied half is read from moved-from elements and a class element type comes
  back empty.
  Tests the following:
  - an arithmetic element type doubles correctly
  - a class element type doubles correctly too, with no empty (moved-from)
    elements in the appended half
*/
bool
tests_append_self()
{
    int_sequence numbers{ 1, 2, 3 };

    numbers.append(numbers);

    bool ok = (matches(numbers, { 1, 2, 3, 1, 2, 3 }));

    // the case that actually exposes the aliasing: a move-enabled element type,
    // with enough elements to force the insert to reallocate.
    string_sequence strings{ std::string("alpha"),
                             std::string("beta"),
                             std::string("gamma") };

    strings.append(strings);

    ok = ok && (strings.size() == 6);
    ok = ok && (matches(strings, { std::string("alpha"), std::string("beta"),
                                   std::string("gamma"), std::string("alpha"),
                                   std::string("beta"),  std::string("gamma") }));

    // no element was left empty by a move that should never have happened
    for (const std::string& e : strings)
    {
        ok = ok && (!e.empty());
    }

    return ok;
}

/*
tests_clear
  Verifies removing every element.
  Tests the following:
  - the sequence becomes empty
  - it is reusable afterwards
  - clearing an already-empty sequence is harmless
*/
bool
tests_clear()
{
    int_sequence s{ 1, 2, 3 };

    s.clear();

    bool ok = (s.empty() && s.size() == 0);

    s.clear();                             // idempotent
    ok = ok && (s.empty());

    s.push_back(4);                        // and still usable
    ok = ok && (matches(s, { 4 }));

    return ok;
}

/*
tests_reserve
  Verifies pre-allocation.
  Tests the following:
  - reserve changes neither the size nor the contents
  - the sequence still behaves normally afterwards
  - reserving less than the current size is harmless
*/
bool
tests_reserve()
{
    int_sequence s{ 1, 2 };

    s.reserve(100);

    bool ok = true;

    ok = ok && (s.size() == 2);
    ok = ok && (matches(s, { 1, 2 }));

    s.push_back(3);
    ok = ok && (matches(s, { 1, 2, 3 }));

    s.reserve(1);                          // a shrinking request is ignored
    ok = ok && (matches(s, { 1, 2, 3 }));

    return ok;
}

/*
tests_resize_grow
  Verifies growing the sequence.
  Tests the following:
  - the existing elements are preserved
  - the new elements are value-initialized
*/
bool
tests_resize_grow()
{
    int_sequence s{ 1, 2, 3 };

    s.resize(5);

    bool ok = true;

    ok = ok && (s.size() == 5);
    ok = ok && (matches(s, { 1, 2, 3, 0, 0 }));   // new elements value-initialized

    return ok;
}

/*
tests_resize_shrink
  Verifies shrinking the sequence.
  Tests the following:
  - the trailing elements are dropped and the leading ones kept
*/
bool
tests_resize_shrink()
{
    int_sequence s{ 1, 2, 3, 4, 5 };

    s.resize(2);

    bool ok = true;

    ok = ok && (s.size() == 2);
    ok = ok && (matches(s, { 1, 2 }));

    return ok;
}

/*
tests_resize_zero
  Verifies resizing to nothing.
  Tests the following:
  - the sequence becomes empty
  - resizing to the current size is a no-op
*/
bool
tests_resize_zero()
{
    int_sequence s{ 1, 2, 3 };

    s.resize(3);                           // a no-op
    bool ok = (matches(s, { 1, 2, 3 }));

    s.resize(0);
    ok = ok && (s.empty() && s.size() == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
