// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "test_object_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- test_metadata                                ///
///////////////////////////////////////////////////////////////////////////////

static_assert(std::is_same<test_metadata::value_type,
                           ::djinterp::kv_pair<std::string, std::string> >::value,
              "test_metadata::value_type must be kv_pair<string, string>");
static_assert(std::is_default_constructible<test_metadata>::value,
              "test_metadata must be default-constructible");
static_assert(!std::is_trivially_copyable<test_metadata>::value,
              "test_metadata owns a std::vector, so it is not trivially copyable");
static_assert(noexcept(std::declval<const test_metadata&>().size()),
              "test_metadata::size() must be noexcept");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- test_metadata                                          ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_test_metadata_types
  Verifies the row type exposed by the metadata store.
  Tests the following:
  - value_type is kv_pair<string, string>, with string key_type / value_type
  - a row constructed through the value constructor exposes m_key / m_value
    (rows are built this way deliberately, never via kv_pair's relational ops)
*/
bool
tests_test_metadata_types()
{
    bool ok = true;

    ok = D_TO_CHECK(std::is_same<test_metadata::value_type,
                    ::djinterp::kv_pair<std::string, std::string> >::value) && ok;
    ok = D_TO_CHECK(std::is_same<test_metadata::value_type::key_type,
                                 std::string>::value) && ok;
    ok = D_TO_CHECK(std::is_same<test_metadata::value_type::value_type,
                                 std::string>::value) && ok;

    // a row built through the value constructor (the only path test_metadata
    // uses) exposes the expected members
    {
        test_metadata::value_type row("k", "v");
        ok = D_TO_CHECK(row.m_key == "k")   && ok;  // std::string compare
        ok = D_TO_CHECK(row.m_value == "v") && ok;
    }

    return ok;
}


/*
tests_test_metadata_set
  Verifies insertion and overwrite semantics.
  Tests the following:
  - a fresh store is empty
  - setting a new key appends an entry (the insert branch) and is retrievable
  - setting further new keys grows the store
  - setting an existing key overwrites in place without growing (the overwrite
    branch)
*/
bool
tests_test_metadata_set()
{
    bool ok = true;

    test_metadata m;
    ok = D_TO_CHECK(m.size() == 0u) && ok;

    // insert branch
    m.set("name", "alpha");
    ok = D_TO_CHECK(m.size() == 1u)            && ok;
    ok = D_TO_CHECK(m.get("name") == "alpha")  && ok;
    ok = D_TO_CHECK(m.contains("name"))        && ok;

    // a second distinct key grows the store (insert branch again)
    m.set("message", "hello");
    ok = D_TO_CHECK(m.size() == 2u)              && ok;
    ok = D_TO_CHECK(m.get("message") == "hello") && ok;

    // overwrite branch: same key updates in place, size unchanged
    m.set("name", "beta");
    ok = D_TO_CHECK(m.size() == 2u)           && ok;
    ok = D_TO_CHECK(m.get("name") == "beta")  && ok;
    // the untouched entry is preserved
    ok = D_TO_CHECK(m.get("message") == "hello") && ok;

    return ok;
}


/*
tests_test_metadata_get
  Verifies value retrieval.
  Tests the following:
  - get on an empty store returns an empty string (the loop never runs)
  - get on a present key returns its stored value (the found branch)
  - get on an absent key returns an empty string (the not-found branch)
  - a key explicitly set to "" reads back "" yet is reported present, so the
    empty-string return is distinguishable from absence only via contains()
*/
bool
tests_test_metadata_get()
{
    bool ok = true;

    // empty store
    {
        test_metadata m;
        ok = D_TO_CHECK(m.get("anything") == "") && ok;
    }

    // populated store: found vs. missing
    {
        test_metadata m;
        m.set("a", "1");
        m.set("b", "2");

        ok = D_TO_CHECK(m.get("a") == "1") && ok;   // found
        ok = D_TO_CHECK(m.get("b") == "2") && ok;   // found
        ok = D_TO_CHECK(m.get("z") == "") && ok;    // missing -> empty
    }

    // present-but-empty vs. absent
    {
        test_metadata m;
        m.set("empty", "");

        ok = D_TO_CHECK(m.get("empty") == "")    && ok;
        ok = D_TO_CHECK(m.contains("empty"))     && ok;  // present
        ok = D_TO_CHECK(m.get("absent") == "")   && ok;
        ok = D_TO_CHECK(!m.contains("absent"))   && ok;  // absent
    }

    return ok;
}


/*
tests_test_metadata_contains
  Verifies key presence checks.
  Tests the following:
  - contains on an empty store is false (the loop never runs)
  - contains on a present key is true (the found branch)
  - contains on an absent key is false (the not-found branch)
  - presence tracks insertions
*/
bool
tests_test_metadata_contains()
{
    bool ok = true;

    test_metadata m;
    ok = D_TO_CHECK(!m.contains("x")) && ok;   // empty -> false

    m.set("x", "1");
    ok = D_TO_CHECK(m.contains("x"))  && ok;   // present -> true
    ok = D_TO_CHECK(!m.contains("y")) && ok;   // absent  -> false

    m.set("y", "2");
    ok = D_TO_CHECK(m.contains("y"))  && ok;   // now present

    return ok;
}


NS_END  // testing
NS_END  // djinterp
