// djinterp [test] : pattern_tests_capture.cpp
//   Status codes (section I) and the capture map (section II): the pattern_capture
// binding and pattern_capture_map's capacity, lookup, mutation (set / erase /
// merge), iteration, and storage access.

// std
#include <cstddef>
#include <string>
#include <utility>
// djinterp
#include "pattern_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    using cmap = pattern_capture_map<std::string, int>;
}


/*
tests_status_codes
  Verifies the standard status constants.
  Tests the following:
  - Ok/NoMatch/Ambiguous/KeyNotFound/Malformed take their documented values
  - the user base sits above the reserved range
*/
bool
tests_status_codes()
{
    bool ok = true;

    ok = ok && (DPatternStatusOk == 0);
    ok = ok && (DPatternStatusNoMatch == 1);
    ok = ok && (DPatternStatusAmbiguous == 2);
    ok = ok && (DPatternStatusKeyNotFound == 3);
    ok = ok && (DPatternStatusMalformed == 4);
    ok = ok && (DPatternStatusUserBase == 64);
    ok = ok && (DPatternStatusUserBase > DPatternStatusMalformed);

    return ok;
}

/*
tests_capture_construction
  Verifies pattern_capture construction.
  Tests the following:
  - the default capture value-initializes key and value
  - the copy constructor stores the supplied key and value
  - the move constructor transfers key and value
*/
bool
tests_capture_construction()
{
    pattern_capture<std::string, int> def;
    pattern_capture<std::string, int> cpy("k", 5);

    std::string mk = "m";
    int         mv = 9;
    pattern_capture<std::string, int> mov(std::move(mk), std::move(mv));

    bool ok = true;

    ok = ok && (def.key.empty() && def.value == 0);
    ok = ok && (cpy.key == "k" && cpy.value == 5);
    ok = ok && (mov.key == "m" && mov.value == 9);

    return ok;
}

/*
tests_map_capacity
  Verifies the map capacity surface.
  Tests the following:
  - a fresh map is empty with size zero
  - size grows with entries; clear empties it
*/
bool
tests_map_capacity()
{
    cmap m;

    bool ok = (m.empty() && m.size() == 0);

    m.set("a", 1);
    m.set("b", 2);
    ok = ok && (!m.empty() && m.size() == 2);

    m.clear();
    ok = ok && (m.empty() && m.size() == 0);

    return ok;
}

/*
tests_map_find
  Verifies const find.
  Tests the following:
  - find returns a pointer to the bound value
  - find returns nullptr for an absent key
*/
bool
tests_map_find()
{
    cmap m;
    m.set("a", 7);

    const cmap& cm = m;

    bool ok = true;

    ok = ok && (cm.find("a") != nullptr && *cm.find("a") == 7);
    ok = ok && (cm.find("absent") == nullptr);

    return ok;
}

/*
tests_map_find_mutable
  Verifies the mutable find overload.
  Tests the following:
  - the returned pointer allows in-place mutation of the bound value
*/
bool
tests_map_find_mutable()
{
    cmap m;
    m.set("a", 1);

    int* p = m.find("a");
    if (p == nullptr) { return false; }
    *p = 42;

    return (*m.find("a") == 42);
}

/*
tests_map_has
  Verifies the has query.
  Tests the following:
  - has is true for a bound key and false otherwise
*/
bool
tests_map_has()
{
    cmap m;
    m.set("a", 1);

    return (m.has("a") && !m.has("b"));
}

/*
tests_map_set_replace
  Verifies set inserts, replaces, and chains.
  Tests the following:
  - set on a new key inserts
  - set on an existing key replaces the value without growing the map
  - set returns *this for chaining
*/
bool
tests_map_set_replace()
{
    cmap m;

    m.set("a", 1).set("b", 2).set("a", 10);   // chained; last replaces "a"

    bool ok = true;

    ok = ok && (m.size() == 2);
    ok = ok && (*m.find("a") == 10);
    ok = ok && (*m.find("b") == 2);

    return ok;
}

/*
tests_map_set_move
  Verifies the rvalue set overload.
  Tests the following:
  - inserting from moved key/value binds correctly
  - replacing an existing key from an rvalue value updates it
*/
bool
tests_map_set_move()
{
    cmap m;

    std::string k1 = "x";
    int         v1 = 1;
    m.set(std::move(k1), std::move(v1));       // insert path

    std::string k2 = "x";
    int         v2 = 99;
    m.set(std::move(k2), std::move(v2));       // replace path

    bool ok = true;

    ok = ok && (m.size() == 1);
    ok = ok && (*m.find("x") == 99);

    return ok;
}

/*
tests_map_erase
  Verifies erase.
  Tests the following:
  - erasing a bound key removes it and returns true
  - erasing an absent key returns false and changes nothing
*/
bool
tests_map_erase()
{
    cmap m;
    m.set("a", 1).set("b", 2);

    bool removed = m.erase("a");
    bool absent  = m.erase("zzz");

    bool ok = true;

    ok = ok && (removed);
    ok = ok && (!absent);
    ok = ok && (m.size() == 1);
    ok = ok && (!m.has("a") && m.has("b"));

    return ok;
}

/*
tests_map_merge
  Verifies merge with and without overwrite.
  Tests the following:
  - merge(other, false) adds absent keys but preserves existing ones
  - merge(other, true) overwrites existing keys
*/
bool
tests_map_merge()
{
    cmap other;
    other.set("a", 100).set("d", 4);

    cmap keep;
    keep.set("a", 1).set("b", 2);
    keep.merge(other, false);                  // no overwrite

    bool ok = true;

    ok = ok && (*keep.find("a") == 1);         // preserved
    ok = ok && (*keep.find("d") == 4);         // added
    ok = ok && (*keep.find("b") == 2);

    cmap over;
    over.set("a", 1);
    over.merge(other, true);                    // overwrite (default)
    ok = ok && (*over.find("a") == 100);

    return ok;
}

/*
tests_map_iteration
  Verifies begin/end iteration in insertion order.
  Tests the following:
  - iteration visits every entry
  - entries appear in insertion order
*/
bool
tests_map_iteration()
{
    cmap m;
    m.set("first", 1).set("second", 2).set("third", 3);

    int         count = 0;
    std::string order;
    for (const auto& e : m)
    {
        ++count;
        order += e.key;
        order += ";";
    }

    bool ok = true;

    ok = ok && (count == 3);
    ok = ok && (order == "first;second;third;");

    return ok;
}

/*
tests_map_entries
  Verifies storage access via entries().
  Tests the following:
  - entries() exposes the underlying vector with the right size
  - the exposed entries carry the stored bindings
*/
bool
tests_map_entries()
{
    cmap m;
    m.set("a", 1).set("b", 2);

    const auto& v = m.entries();

    bool ok = true;

    ok = ok && (v.size() == 2);
    ok = ok && (v[0].key == "a" && v[0].value == 1);
    ok = ok && (v[1].key == "b" && v[1].value == 2);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
