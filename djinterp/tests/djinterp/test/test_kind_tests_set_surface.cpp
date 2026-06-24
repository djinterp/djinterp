// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "test_kind_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- III. forwarded set surface + iteration                 ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_set_size_empty
  Verifies the forwarded size() and empty().
  Tests the following:
  - a fresh wrapper is empty with size zero
  - both track insertions and erasures of the underlying container
*/
bool
tests_set_size_empty()
{
    bool ok = true;

    test_kind_set<id_set> ks;
    ok = D_TK_CHECK(ks.size() == 0u) && ok;
    ok = D_TK_CHECK(ks.empty())      && ok;

    ks.insert(1);
    ks.insert(2);
    ok = D_TK_CHECK(ks.size() == 2u) && ok;
    ok = D_TK_CHECK(!ks.empty())     && ok;

    ks.erase(1);
    ks.erase(2);
    ok = D_TK_CHECK(ks.size() == 0u) && ok;
    ok = D_TK_CHECK(ks.empty())      && ok;

    return ok;
}


/*
tests_set_clear
  Verifies the forwarded clear().
  Tests the following:
  - clearing a populated wrapper empties it
  - clearing an already-empty wrapper is a no-op
*/
bool
tests_set_clear()
{
    bool ok = true;

    test_kind_set<id_set> ks;
    ks.insert(1);
    ks.insert(2);
    ks.insert(3);
    ok = D_TK_CHECK(ks.size() == 3u) && ok;

    ks.clear();
    ok = D_TK_CHECK(ks.empty())      && ok;
    ok = D_TK_CHECK(ks.size() == 0u) && ok;

    ks.clear();                       // idempotent
    ok = D_TK_CHECK(ks.empty())      && ok;

    return ok;
}


/*
tests_set_insert
  Verifies both forwarded insert() overloads.
  Tests the following:
  - the lvalue (copy) overload inserts a new key and reports success
  - the rvalue (move) overload likewise inserts and reports success
  - inserting a duplicate key reports failure and does not grow the set
*/
bool
tests_set_insert()
{
    bool ok = true;

    test_kind_set<id_set> ks;

    // lvalue -> copy overload
    test_type_id five = 5;
    std::pair<id_set::iterator, bool> r1 = ks.insert(five);
    ok = D_TK_CHECK(r1.second == true) && ok;
    ok = D_TK_CHECK(ks.size() == 1u)   && ok;

    // rvalue -> move overload
    std::pair<id_set::iterator, bool> r2 = ks.insert(7);
    ok = D_TK_CHECK(r2.second == true) && ok;
    ok = D_TK_CHECK(ks.size() == 2u)   && ok;

    // duplicate key -> failure, no growth
    std::pair<id_set::iterator, bool> r3 = ks.insert(five);
    ok = D_TK_CHECK(r3.second == false) && ok;
    ok = D_TK_CHECK(ks.size() == 2u)    && ok;

    std::pair<id_set::iterator, bool> r4 = ks.insert(7);  // rvalue dup
    ok = D_TK_CHECK(r4.second == false) && ok;

    return ok;
}


/*
tests_set_erase
  Verifies the forwarded erase().
  Tests the following:
  - erasing a present key removes it and reports a count of one
  - erasing an absent key reports zero and leaves the set unchanged
*/
bool
tests_set_erase()
{
    bool ok = true;

    test_kind_set<id_set> ks;
    ks.insert(10);
    ks.insert(20);

    ok = D_TK_CHECK(ks.erase(10) == 1u) && ok;
    ok = D_TK_CHECK(ks.size() == 1u)    && ok;
    ok = D_TK_CHECK(!ks.contains(10))   && ok;

    ok = D_TK_CHECK(ks.erase(10) == 0u) && ok;   // already gone
    ok = D_TK_CHECK(ks.erase(99) == 0u) && ok;   // never present
    ok = D_TK_CHECK(ks.size() == 1u)    && ok;

    return ok;
}


/*
tests_set_find
  Verifies both forwarded find() overloads.
  Tests the following:
  - the mutable overload locates a present key and reports its value
  - a missing key yields the end iterator
  - the const overload behaves identically on a const wrapper
*/
bool
tests_set_find()
{
    bool ok = true;

    test_kind_set<id_set> ks;
    ks.insert(11);
    ks.insert(22);

    id_set::iterator it = ks.find(11);
    ok = D_TK_CHECK(it != ks.end()) && ok;
    ok = D_TK_CHECK(*it == 11)      && ok;
    ok = D_TK_CHECK(ks.find(99) == ks.end()) && ok;

    const test_kind_set<id_set>& cks = ks;
    id_set::const_iterator cit = cks.find(22);
    ok = D_TK_CHECK(cit != cks.end()) && ok;
    ok = D_TK_CHECK(*cit == 22)       && ok;
    ok = D_TK_CHECK(cks.find(0) == cks.end()) && ok;

    return ok;
}


/*
tests_set_iteration
  Verifies the forwarded iteration surface.
  Tests the following:
  - mutable begin()/end() visit every element
  - cbegin()/cend() yield const iterators over the same elements
  - const begin()/end() on a const wrapper behave identically
*/
bool
tests_set_iteration()
{
    bool ok = true;

    test_kind_set<id_set> ks;
    ks.insert(1);
    ks.insert(2);
    ks.insert(3);

    // mutable begin/end
    std::size_t count = 0;
    long        sum   = 0;
    for (id_set::iterator it = ks.begin(); it != ks.end(); ++it)
    {
        ++count;
        sum += *it;
    }
    ok = D_TK_CHECK(count == 3u) && ok;
    ok = D_TK_CHECK(sum == 6)    && ok;

    // cbegin/cend
    std::size_t ccount = 0;
    for (id_set::const_iterator it = ks.cbegin(); it != ks.cend(); ++it)
    {
        ++ccount;
    }
    ok = D_TK_CHECK(ccount == 3u) && ok;

    // const begin/end
    const test_kind_set<id_set>& cks = ks;
    std::size_t kcount = 0;
    for (id_set::const_iterator it = cks.begin(); it != cks.end(); ++it)
    {
        ++kcount;
    }
    ok = D_TK_CHECK(kcount == 3u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
