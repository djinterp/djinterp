/*******************************************************************************
* djinterp [test]                                            test_kind_tests.hpp
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
* path:      /tests/djinterp/test/test_kind_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                              created: 2026.06.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_KIND_TESTS_
#define DJINTERP_TEST_KIND_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <utility>
#include <vector>
// djinterp
// djinterp  -- framework header first, then mode-gated.
#include <djinterp/djinterp.hpp>            // NS_*, D_* qualifiers, language gates
#ifndef DTEST_SPEC_MODE
#include "../../../inc/djinterp/test/test_kind.hpp"   // the header under test (normal mode)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"       // spec mode: module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test); the tests live here in
// djinterp::testing.  (Spec mode also uses it for dt::module_spec / run_module.)
namespace dt = ::djinterp::test;


#ifndef DTEST_SPEC_MODE  // fixtures + check helper: normal (section-file) mode only


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

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- test declarations (visible in both modes; defined in the section TUs) ----

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


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    test_kind.hpp's suite, authored ONCE as declarative metadata: a nested
//  module_spec - module > block > test - every node carrying a name AND a
//  verbose descriptor.  run_module lowers it into the six-kind tree for the
//  structural console dump and projects the SAME spec onto the report_builder
//  that feeds the live console and the PDF.  References only the test_fn_ptr
//  declarations above; no fixtures required.
// =========================================================================

inline dt::module_spec
test_kind_spec()
{
    return dt::module_spec{
        "test_kind",
        "Unit tests for test_kind.hpp - the ranked test-kind record at the center of "
        "the six-kind test taxonomy (test_suite > test_module > test_block > unit_test "
        "> test_fn > assert), its aggregate factory make_test_kind, the flat kind-set "
        "wrapper that stores kinds with compile-time contains-dispatch, the resolved "
        "queries that answer rank / leaf / name questions over a range of records, and "
        "the structural trait that recognises a kind-set type.",
        {
            dt::block_spec{
                "record",
                "The test_kind record itself: aggregate initialisation, the "
                "id / name / rank / leaf members, its type-trait surface, and the "
                "canonical kind constants.",
                {
                    { "tests_test_kind_aggregate",
                      "a test_kind is an aggregate - brace-initialisation fills its members "
                      "in declared order with no user-provided constructor",
                      &tests_test_kind_aggregate },
                    { "tests_test_kind_members",
                      "each member (id, name, rank, leaf flag) reads back exactly the value "
                      "the record was constructed with",
                      &tests_test_kind_members },
                    { "tests_test_kind_traits",
                      "test_kind satisfies the expected type traits - trivially copyable and "
                      "standard-layout, so it is cheap to pass and store",
                      &tests_test_kind_traits },
                    { "tests_test_kind_values",
                      "the canonical kind constants carry the ranks and leaf flags the six-kind "
                      "hierarchy requires (suite outermost, assert / test_fn as leaves)",
                      &tests_test_kind_values },
                }
            },
            dt::block_spec{
                "factory",
                "make_test_kind: the aggregate factory that stamps a record - its "
                "defaulted trailing argument, its usability in a constant expression, "
                "and its noexcept guarantee.",
                {
                    { "tests_make_test_kind_basic",
                      "make_test_kind returns a record carrying exactly the id, name, and rank "
                      "arguments it was handed",
                      &tests_make_test_kind_basic },
                    { "tests_make_test_kind_default_arg",
                      "the trailing argument defaults, so a call that omits it yields the "
                      "documented default",
                      &tests_make_test_kind_default_arg },
                    { "tests_make_test_kind_constexpr",
                      "make_test_kind is constexpr-callable - a record can be built at compile "
                      "time and used in a constant expression",
                      &tests_make_test_kind_constexpr },
                    { "tests_make_test_kind_noexcept",
                      "make_test_kind is declared noexcept and is statically verified never to throw",
                      &tests_make_test_kind_noexcept },
                }
            },
            dt::block_spec{
                "set_construction",
                "test_kind_set construction: the nested type aliases, the default / copy "
                "/ move constructors, access to the underlying container, and the "
                "noexcept guarantees of construction.",
                {
                    { "tests_set_aliases",
                      "the set exposes the nested type aliases (value_type, size_type, iterator, "
                      "...) its container surface promises",
                      &tests_set_aliases },
                    { "tests_set_default_ctor",
                      "a default-constructed set is empty and ready to use",
                      &tests_set_default_ctor },
                    { "tests_set_copy_ctor",
                      "copy construction yields an independent set equal in size and contents to "
                      "its source",
                      &tests_set_copy_ctor },
                    { "tests_set_move_ctor",
                      "move construction transfers the elements, leaving a valid source and "
                      "preserving the contents in the target",
                      &tests_set_move_ctor },
                    { "tests_set_underlying",
                      "underlying() exposes the wrapped container for callers that need the "
                      "concrete storage",
                      &tests_set_underlying },
                    { "tests_set_ctor_noexcept",
                      "the constructors that can be are declared noexcept, matching the "
                      "underlying container's guarantees",
                      &tests_set_ctor_noexcept },
                }
            },
            dt::block_spec{
                "set_surface",
                "The container surface test_kind_set forwards to its storage: size / "
                "empty, clear, key-unique insert, key-based erase and find, and "
                "iteration.",
                {
                    { "tests_set_size_empty",
                      "size() and empty() agree and track the element count as the set changes",
                      &tests_set_size_empty },
                    { "tests_set_clear",
                      "clear() removes every element, leaving the set empty",
                      &tests_set_clear },
                    { "tests_set_insert",
                      "insert() adds an element and is key-unique - inserting an equivalent key "
                      "does not duplicate",
                      &tests_set_insert },
                    { "tests_set_erase",
                      "erase() removes the element matching a key and reports whether anything "
                      "was removed",
                      &tests_set_erase },
                    { "tests_set_find",
                      "find() locates an element by key and yields the end iterator when the key "
                      "is absent",
                      &tests_set_find },
                    { "tests_set_iteration",
                      "iterating the set visits each stored element exactly once",
                      &tests_set_iteration },
                }
            },
            dt::block_spec{
                "set_contains",
                "test_kind_set::contains and the compile-time dispatch that prefers the "
                "container's native contains() and otherwise falls back to a keyed "
                "linear scan.",
                {
                    { "tests_set_contains_native",
                      "contains() calls the underlying container's native contains() when the "
                      "container exposes one",
                      &tests_set_contains_native },
                    { "tests_set_contains_fallback",
                      "contains() falls back to a keyed linear scan when the container has no "
                      "native contains()",
                      &tests_set_contains_fallback },
                    { "tests_set_contains_detection",
                      "the detection trait selects the native-vs-fallback contains() path at "
                      "compile time, with no runtime branch",
                      &tests_set_contains_detection },
                }
            },
            dt::block_spec{
                "queries",
                "The resolved queries over a range of test_kind records: find-by-id, "
                "rank, leaf / interior classification, name lookup, default options, "
                "child-of legality, and their composition.",
                {
                    { "tests_find_kind",
                      "find_kind locates the record for a given id within a range, or reports "
                      "its absence",
                      &tests_find_kind },
                    { "tests_rank_of",
                      "rank_of returns the hierarchy rank recorded for a kind",
                      &tests_rank_of },
                    { "tests_is_leaf",
                      "is_leaf is true for the leaf kinds (assert, test_fn) and false otherwise",
                      &tests_is_leaf },
                    { "tests_is_interior",
                      "is_interior is the complement of is_leaf - true for the grouping kinds",
                      &tests_is_interior },
                    { "tests_name_of",
                      "name_of resolves a kind id to its human-readable name",
                      &tests_name_of },
                    { "tests_default_options",
                      "default_options yields the documented option set for a kind",
                      &tests_default_options },
                    { "tests_can_be_child_of",
                      "can_be_child_of enforces the legal nesting - a kind may nest only directly "
                      "beneath the next rank up",
                      &tests_can_be_child_of },
                    { "tests_queries_compose",
                      "the queries compose - answering a structural question by combining rank_of, "
                      "is_leaf, and can_be_child_of agrees with the direct query",
                      &tests_queries_compose },
                }
            },
            dt::block_spec{
                "detection",
                "The is_test_kind_set structural trait: it recognises a kind-set type "
                "through cv / reference qualifiers and is usable as a variable template.",
                {
                    { "tests_is_test_kind_set",
                      "is_test_kind_set is true for a test_kind_set and false for unrelated types",
                      &tests_is_test_kind_set },
                    { "tests_is_test_kind_set_cvref",
                      "is_test_kind_set strips cv-qualifiers and references before inspecting, so "
                      "const / ref forms are still recognised",
                      &tests_is_test_kind_set_cvref },
                    { "tests_is_test_kind_set_variable",
                      "the is_test_kind_set_v variable template mirrors the trait's value for terse "
                      "use in constraints",
                      &tests_is_test_kind_set_variable },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


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
