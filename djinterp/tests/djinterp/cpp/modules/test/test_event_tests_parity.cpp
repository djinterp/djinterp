/*******************************************************************************
* djinterp [cpp][test]                               test_event_tests_parity.cpp
*
*   Section V of the DTestEvent suite: the two faces agree on every name.
*
*   DTest's twelve built-in events are named once, in test_event_names.h.  C's
* alphabet (test_event.c) returns those names from d_test_event_kind_name, and
* C++'s tags give them to D_EVENT.  A name is what an event prints, what a
* listener binds by, and what d_event_key_of_name hashes into the event's key,
* so a C name that drifted from its C++ tag would split one event into two.
* These tests hold the two faces to the one table: each kind against its tag,
* each key against its tag's key, and the table against the enumerators.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_event_tests_parity.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

// std
#include <cstddef>
#include <cstring>
// djinterp
#include "test_event_tests.hpp"
#include "djinterp/c/event/event_common.h"       // d_event_key_of_name
#include "djinterp/c/modules/test/test_event.h"  // C's alphabet and its names


NS_DJINTERP
NS_TESTING


namespace
{

// test_event_parity_same_
//   function: true when C's name for _kind is _tag's name(), and both hash to
// the same key.
template<typename _Tag>
bool test_event_parity_same_(
    enum dj_test_event_kind _kind
)
{
    const char* c_name   = d_test_event_kind_name(_kind);
    const char* cpp_name = _Tag::name();

    return ( (c_name != nullptr) &&
             (cpp_name != nullptr) &&
             (std::strcmp(c_name, cpp_name) == 0) &&
             (d_event_key_of_name(c_name) == d_event_key_of_name(cpp_name)) );
}

// test_event_parity_is_test_name_
//   function: true when _name is "test." followed by lower-case words: letters,
// digits and underscores, with no empty word.
bool test_event_parity_is_test_name_(
    const char* _name
)
{
    std::size_t i = 0;

    if ( (_name == nullptr) ||
         (std::strncmp(_name, "test.", 5) != 0) ||
         (_name[5] == '\0') )
    {
        return false;
    }

    for (i = 5; _name[i] != '\0'; ++i)
    {
        const char c = _name[i];

        if (!( ((c >= 'a') && (c <= 'z')) ||
               ((c >= '0') && (c <= '9')) ||
               (c == '_') ))
        {
            return false;
        }
    }

    return true;
}

}  // namespace


// tests_event_parity_each_kind_names_its_tag
//   every built-in kind's C name is its C++ tag's name(), and the two hash to
// one key.
bool tests_event_parity_each_kind_names_its_tag()
{
    D_TE_CHECK(test_event_parity_same_<dt::DOnSessionStart>(
                   D_TEST_EVENT_SESSION_START));
    D_TE_CHECK(test_event_parity_same_<dt::DOnSessionEnd>(
                   D_TEST_EVENT_SESSION_END));
    D_TE_CHECK(test_event_parity_same_<dt::DOnModuleStart>(
                   D_TEST_EVENT_MODULE_START));
    D_TE_CHECK(test_event_parity_same_<dt::DOnModuleEnd>(
                   D_TEST_EVENT_MODULE_END));
    D_TE_CHECK(test_event_parity_same_<dt::DOnTestStart>(
                   D_TEST_EVENT_TEST_START));
    D_TE_CHECK(test_event_parity_same_<dt::DOnTestEnd>(
                   D_TEST_EVENT_TEST_END));
    D_TE_CHECK(test_event_parity_same_<dt::DOnTestPassed>(
                   D_TEST_EVENT_TEST_PASSED));
    D_TE_CHECK(test_event_parity_same_<dt::DOnTestFailed>(
                   D_TEST_EVENT_TEST_FAILED));
    D_TE_CHECK(test_event_parity_same_<dt::DOnTestSkipped>(
                   D_TEST_EVENT_TEST_SKIPPED));
    D_TE_CHECK(test_event_parity_same_<dt::DOnTestError>(
                   D_TEST_EVENT_TEST_ERROR));
    D_TE_CHECK(test_event_parity_same_<dt::DOnStatusChange>(
                   D_TEST_EVENT_STATUS_CHANGE));
    D_TE_CHECK(test_event_parity_same_<dt::DOnListenerThrew>(
                   D_TEST_EVENT_LISTENER_THREW));

    return true;
}


// tests_event_parity_table_follows_the_kinds
//   the table lists every built-in once, in kind order: row i is kind i, and
// C's alphabet returns row i's name for it.
bool tests_event_parity_table_follows_the_kinds()
{
    int rows = 0;

#define D_TE_PARITY_ROW(_kind, _name)                                          \
    D_TE_CHECK((int)D_TEST_EVENT_##_kind == rows);                             \
    D_TE_CHECK(std::strcmp(d_test_event_kind_name(D_TEST_EVENT_##_kind),       \
                           _name) == 0);                                       \
    ++rows;

    D_TEST_EVENT_NAMES(D_TE_PARITY_ROW)

#undef D_TE_PARITY_ROW

    D_TE_CHECK(rows == D_TEST_EVENT_KIND_COUNT);

    return true;
}


// tests_event_parity_names_are_test_names
//   every built-in name is "test." and lower-case words, the form message
// codes take (module.name).
bool tests_event_parity_names_are_test_names()
{
    int kind = 0;

    for (kind = 0; kind < D_TEST_EVENT_KIND_COUNT; ++kind)
    {
        D_TE_CHECK(test_event_parity_is_test_name_(
                       d_test_event_kind_name((enum dj_test_event_kind)kind)));
    }

    D_TE_CHECK(std::strcmp(dt::DOnTestFailed::name(), "test.test_failed") == 0);

    return true;
}


// tests_event_parity_keys_are_distinct
//   the twelve names hash to twelve distinct keys, none of them the reserved
// D_EVENT_KEY_NONE, so no two built-ins share a key.
bool tests_event_parity_keys_are_distinct()
{
    dj_event_key keys[D_TEST_EVENT_KIND_COUNT];
    int          i = 0;
    int          j = 0;

    for (i = 0; i < D_TEST_EVENT_KIND_COUNT; ++i)
    {
        keys[i] = d_event_key_of_name(
                      d_test_event_kind_name((enum dj_test_event_kind)i));

        D_TE_CHECK(keys[i] != D_EVENT_KEY_NONE);
    }

    for (i = 0; i < D_TEST_EVENT_KIND_COUNT; ++i)
    {
        for (j = i + 1; j < D_TEST_EVENT_KIND_COUNT; ++j)
        {
            D_TE_CHECK(keys[i] != keys[j]);
        }
    }

    return true;
}


// tests_event_parity_custom_kind_is_not_named
//   a kind past the built-ins has no name in the table: C's alphabet returns
// "custom", which no built-in name equals.
bool tests_event_parity_custom_kind_is_not_named()
{
    const char* name = d_test_event_kind_name(
                           (enum dj_test_event_kind)D_TEST_EVENT_KIND_COUNT);

    D_TE_CHECK(name != nullptr);
    D_TE_CHECK(std::strcmp(name, "custom") == 0);
    D_TE_CHECK(!test_event_parity_is_test_name_(name));

    return true;
}


NS_END  // testing
NS_END  // djinterp
