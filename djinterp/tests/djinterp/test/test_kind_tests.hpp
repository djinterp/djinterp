/******************************************************************************
* djinterp [test]                                          test_kind_tests.hpp
*
*   Declarations for the unit-test suite covering test_kind.hpp.  Each free
* function exercises one facet of the header and returns true iff every check
* inside it passed.  Tests are grouped into translation units by the semantic
* section of test_kind.hpp they cover (Section III, the kind-set wrapper, is
* split across three files along its internal banners):
*
*   - test_kind_tests_record.cpp           -> I.   TEST KIND RECORD
*   - test_kind_tests_factory.cpp          -> II.  FACTORY FUNCTION
*   - test_kind_tests_set_construction.cpp -> III. set: ctors / underlying /
*                                                   aliases
*   - test_kind_tests_set_surface.cpp      -> III. set: size/empty/clear/
*                                                   insert/erase/find/iterate
*   - test_kind_tests_set_contains.cpp     -> III. set: contains + dispatch
*   - test_kind_tests_queries.cpp          -> IV.  RESOLVED QUERIES
*   - test_kind_tests_detection.cpp        -> V.   STRUCTURAL DETECTION
*
*   Helpers (all flat in djinterp::testing):
*   - test_kind_check : reports a failing check, forwards the boolean.
*   - flat_set<Value, Key, HasContains> : a tiny vector-backed set satisfying
*       the structural set-like surface test_kind_set forwards to.  The
*       HasContains parameter toggles a native contains() member so BOTH of
*       test_kind_set::contains()'s dispatch branches (native vs. find()
*       fallback) can be exercised deterministically.  Three aliases are used:
*           id_set     : flat set of test_type_id, WITH    contains()
*           id_set_nc  : flat set of test_type_id, WITHOUT contains()
*           kind_set   : flat set of test_kind keyed by id, WITH contains()
*
*   NOTE: the entities under test live in djinterp::test; the tests themselves
* live, flat, in djinterp::testing.
*
*
* path:      /inc/djinterp/test/test_kind_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_TEST_KIND_TESTS_
#define DJINTERP_TEST_KIND_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <utility>
#include <vector>
// djinterp
#include "test_kind.hpp"


NS_DJINTERP
NS_TESTING


// test_kind_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
test_kind_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}


// flat_key_of
//   helper: extracts the set key from a stored value.  An id stores itself;
// a test_kind record is keyed by its id.  Declared before flat_set_core so
// the latter's dependent call binds to these overloads.
inline ::djinterp::test::test_type_id
flat_key_of(const ::djinterp::test::test_type_id& _value)
{
    return _value;
}

inline ::djinterp::test::test_type_id
flat_key_of(const ::djinterp::test::test_kind& _value)
{
    return _value.id;
}


// flat_set_core
//   helper: the common surface of the minimal flat set -- the nested type
// aliases plus size/empty/clear, key-unique insert (copy and move), key-based
// erase/find, and iteration.  A linear scan keyed through flat_key_of.
template<typename _Value, typename _Key>
struct flat_set_core
{
    using key_type       = _Key;
    using value_type     = _Value;
    using size_type      = std::size_t;
    using iterator       = typename std::vector<_Value>::iterator;
    using const_iterator = typename std::vector<_Value>::const_iterator;

    std::vector<_Value> data;

    iterator       begin()        { return data.begin(); }
    const_iterator begin() const  { return data.begin(); }
    iterator       end()          { return data.end();   }
    const_iterator end()   const  { return data.end();   }

    size_type size()  const { return data.size();  }
    bool      empty() const { return data.empty(); }
    void      clear()       { data.clear();         }

    std::pair<iterator, bool>
    insert(const _Value& _value)
    {
        for (iterator it = data.begin(); it != data.end(); ++it)
        {
            if (flat_key_of(*it) == flat_key_of(_value))
            {
                return std::pair<iterator, bool>(it, false);
            }
        }
        data.push_back(_value);
        return std::pair<iterator, bool>(data.end() - 1, true);
    }

    std::pair<iterator, bool>
    insert(_Value&& _value)
    {
        for (iterator it = data.begin(); it != data.end(); ++it)
        {
            if (flat_key_of(*it) == flat_key_of(_value))
            {
                return std::pair<iterator, bool>(it, false);
            }
        }
        data.push_back(static_cast<_Value&&>(_value));
        return std::pair<iterator, bool>(data.end() - 1, true);
    }

    size_type
    erase(const _Key& _key)
    {
        for (iterator it = data.begin(); it != data.end(); ++it)
        {
            if (flat_key_of(*it) == _key)
            {
                data.erase(it);
                return 1;
            }
        }
        return 0;
    }

    iterator
    find(const _Key& _key)
    {
        for (iterator it = data.begin(); it != data.end(); ++it)
        {
            if (flat_key_of(*it) == _key)
            {
                return it;
            }
        }
        return data.end();
    }

    const_iterator
    find(const _Key& _key) const
    {
        for (const_iterator it = data.begin(); it != data.end(); ++it)
        {
            if (flat_key_of(*it) == _key)
            {
                return it;
            }
        }
        return data.end();
    }
};


// flat_set
//   helper: flat_set_core plus an optional native contains().  The primary
// template is unused; the two specializations select on HasContains.
template<typename _Value, typename _Key, bool _HasContains>
struct flat_set;

template<typename _Value, typename _Key>
struct flat_set<_Value, _Key, false> : flat_set_core<_Value, _Key>
{};

template<typename _Value, typename _Key>
struct flat_set<_Value, _Key, true> : flat_set_core<_Value, _Key>
{
    bool
    contains(const _Key& _key) const
    {
        return (this->find(_key) != this->end());
    }
};


using id_set    = flat_set< ::djinterp::test::test_type_id,
                            ::djinterp::test::test_type_id, true >;
using id_set_nc = flat_set< ::djinterp::test::test_type_id,
                            ::djinterp::test::test_type_id, false >;
using kind_set  = flat_set< ::djinterp::test::test_kind,
                            ::djinterp::test::test_type_id, true >;


// I.   TEST KIND RECORD
bool tests_test_kind_aggregate();
bool tests_test_kind_members();
bool tests_test_kind_traits();
bool tests_test_kind_values();

// II.  FACTORY FUNCTION
bool tests_make_test_kind_basic();
bool tests_make_test_kind_default_arg();
bool tests_make_test_kind_constexpr();
bool tests_make_test_kind_noexcept();

// III. TEST KIND SET -- construction / underlying / aliases
bool tests_set_aliases();
bool tests_set_default_ctor();
bool tests_set_copy_ctor();
bool tests_set_move_ctor();
bool tests_set_underlying();
bool tests_set_ctor_noexcept();

// III. TEST KIND SET -- forwarded surface + iteration
bool tests_set_size_empty();
bool tests_set_clear();
bool tests_set_insert();
bool tests_set_erase();
bool tests_set_find();
bool tests_set_iteration();

// III. TEST KIND SET -- contains + dispatch
bool tests_set_contains_native();
bool tests_set_contains_fallback();
bool tests_set_contains_detection();

// IV.  RESOLVED QUERIES
bool tests_find_kind();
bool tests_rank_of();
bool tests_is_leaf();
bool tests_is_interior();
bool tests_name_of();
bool tests_default_options();
bool tests_can_be_child_of();
bool tests_queries_compose();

// V.   STRUCTURAL DETECTION
bool tests_is_test_kind_set();
bool tests_is_test_kind_set_cvref();
bool tests_is_test_kind_set_variable();


NS_END  // testing
NS_END  // djinterp


// D_TK_CHECK
//   macro: evaluates its argument exactly once and routes it through
// test_kind_check, capturing the expression text and source location.
// Variadic so a condition containing a top-level comma passes through whole.
#define D_TK_CHECK(...)                                                       \
    ::djinterp::testing::test_kind_check(                                     \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_KIND_TESTS_
