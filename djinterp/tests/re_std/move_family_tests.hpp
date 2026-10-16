/*******************************************************************************
* djinterp [test]                                          move_family_tests.hpp
*
*   Unit-test suite header for the re_std "move family": the utility cast
* (re_std::move) and move_if_noexcept, the move / move_backward algorithms, the
* uninitialized_move / _n memory algorithms, the move_iterator adaptor, the
* iter_move CPO, and the six move-related type traits.
*
*   Two faces, selected by DTEST_SPEC_MODE:
*
*     - default face: the flat declarations of every tests_* predicate plus the
*       shared fixtures the section TUs build against.  Every
*       move_family_tests_*.cpp includes the header this way and DEFINES the
*       predicates it owns.
*
*     - spec-provider face (DTEST_SPEC_MODE defined): the above PLUS
*       move_family_spec(), which the runner hands to run_module.  Only the
*       runner defines DTEST_SPEC_MODE, so the spec is compiled exactly once.
*
*   LANGUAGE FLOOR: this is a move library — every header is gated on rvalue
* references and does nothing below C++11.  The suite therefore targets C++11
* and up; the handful of dialect-conditional checks (variable-template _v forms
* at C++14, move_iterator::iterator_concept at C++20) are guarded internally.
*
*   NOTE ON DEPENDENCIES: the headers under test pull in a wide swath of re_std
* (remove_reference, is_assignable, iterator_traits, addressof, destroy_at,
* pair, ...).  In the harness those are thin, std-backed stand-ins so the family
* can be exercised in isolation; the suite asserts only the behaviour of the
* move-family headers themselves, cross-checking the traits against <type_traits>.
*
*
* path:      /tests/djinterp/re_std/move_family_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.10
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_MOVE_FAMILY_TESTS_
#define DJINTERP_TEST_MOVE_FAMILY_TESTS_ 1

// std
#include <cstddef>
#include <iterator>
// djinterp
// djinterp core (macros / gates)
#include "djinterp/djinterp.hpp"
// the headers under test
#include "djinterp/re_std/utility/move.hpp"
#include "djinterp/re_std/utility/move_if_noexcept.hpp"
#include "djinterp/re_std/algorithm/move.hpp"
#include "djinterp/re_std/algorithm/move_backward.hpp"
#include "djinterp/re_std/memory/uninitialized_move.hpp"
#include "djinterp/re_std/memory/uninitialized_move_n.hpp"
#include "djinterp/re_std/iterator/move_iterator.hpp"
#include "djinterp/re_std/iterator/iter_move.hpp"
#include "djinterp/re_std/type_traits/is_move_assignable.hpp"
#include "djinterp/re_std/type_traits/is_move_constructible.hpp"
#include "djinterp/re_std/type_traits/is_nothrow_move_assignable.hpp"
#include "djinterp/re_std/type_traits/is_nothrow_move_constructible.hpp"
#include "djinterp/re_std/type_traits/is_trivially_move_assignable.hpp"
#include "djinterp/re_std/type_traits/is_trivially_move_constructible.hpp"
// test framework
#include "djinterp/test/test_defaults.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                SHARED FIXTURES                                          ///
///////////////////////////////////////////////////////////////////////////////
//   All fixtures keep their move/copy tallies in a caller-owned `counters`
// rather than in static members, so the header carries no out-of-line
// definitions and is safe to include from every section TU.

// counters
//   struct: per-test move/copy tally, addressed by pointer from the probes.
struct counters
{
    int moves;
    int copies;

    counters()
        : moves(0),
          copies(0)
    {}
};

// probe
//   type: a movable + copyable element that records, into its bound counters,
// whether each construction/assignment took the move path or the copy path.
// Its move operations are noexcept (so it is nothrow-move-constructible).
struct probe
{
    counters* c;
    int       id;

    explicit probe(counters* _c = nullptr,
                   int       _id = 0) noexcept
        : c(_c),
          id(_id)
    {}

    probe(const probe& _o)
        : c(_o.c),
          id(_o.id)
    {
        if (c != nullptr) { ++c->copies; }
    }

    probe(probe&& _o) noexcept
        : c(_o.c),
          id(_o.id)
    {
        if (c != nullptr) { ++c->moves; }
    }

    probe&
    operator=(const probe& _o)
    {
        c  = _o.c;
        id = _o.id;
        if (c != nullptr) { ++c->copies; }

        return *this;
    }

    probe&
    operator=(probe&& _o) noexcept
    {
        c  = _o.c;
        id = _o.id;
        if (c != nullptr) { ++c->moves; }

        return *this;
    }
};

// move_only
//   type: a move-only element (copy deleted). Used to prove the algorithms and
// adaptors never fall back to copying.
struct move_only
{
    counters* c;
    int       id;

    explicit move_only(counters* _c = nullptr,
                       int       _id = 0) noexcept
        : c(_c),
          id(_id)
    {}

    move_only(const move_only&)            = delete;
    move_only& operator=(const move_only&) = delete;

    move_only(move_only&& _o) noexcept
        : c(_o.c),
          id(_o.id)
    {
        if (c != nullptr) { ++c->moves; }
    }

    move_only&
    operator=(move_only&& _o) noexcept
    {
        c  = _o.c;
        id = _o.id;
        if (c != nullptr) { ++c->moves; }

        return *this;
    }
};

// proxy_ref / proxy_iter
//   types: a proxy iterator whose dereference yields a BY-VALUE reference
// (proxy_ref), not a language reference. move_iterator and iter_move must pass
// such a reference type through unchanged rather than casting a temporary to
// an rvalue reference (which would dangle).
struct proxy_ref
{
    int v;
};

struct proxy_iter
{
    typedef std::forward_iterator_tag iterator_category;
    typedef int                       value_type;
    typedef std::ptrdiff_t            difference_type;
    typedef void                      pointer;
    typedef proxy_ref                 reference;

    int* p;

    explicit proxy_iter(int* _p = nullptr) : p(_p) {}

    proxy_ref operator*()  const { return proxy_ref{ *p }; }
    proxy_iter& operator++()     { ++p; return *this; }

    bool operator==(const proxy_iter& _o) const { return p == _o.p; }
    bool operator!=(const proxy_iter& _o) const { return p != _o.p; }
};


///////////////////////////////////////////////////////////////////////////////
///                TEST DECLARATIONS                                         ///
///////////////////////////////////////////////////////////////////////////////

// -- section: utility move (move_family_tests_utility_move.cpp)
bool tests_move_type();
bool tests_move_noexcept();
bool tests_move_semantics();
bool tests_move_constexpr();

// -- section: move_if_noexcept (move_family_tests_move_if_noexcept.cpp)
bool tests_move_if_noexcept_nothrow_type();
bool tests_move_if_noexcept_throwing_copyable();
bool tests_move_if_noexcept_move_only();
bool tests_move_if_noexcept_semantics();

// -- section: move algorithm (move_family_tests_algorithm_move.cpp)
bool tests_algorithm_move_basic();
bool tests_algorithm_move_semantics();
bool tests_algorithm_move_empty();
bool tests_algorithm_move_overload_coexist();

// -- section: move_backward (move_family_tests_move_backward.cpp)
bool tests_move_backward_basic();
bool tests_move_backward_overlap();
bool tests_move_backward_semantics();
bool tests_move_backward_empty();

// -- section: uninitialized_move (move_family_tests_uninitialized_move.cpp)
bool tests_uninitialized_move_basic();
bool tests_uninitialized_move_semantics();
bool tests_uninitialized_move_exception_safety();

// -- section: uninitialized_move_n (move_family_tests_uninitialized_move_n.cpp)
bool tests_uninitialized_move_n_basic();
bool tests_uninitialized_move_n_semantics();
bool tests_uninitialized_move_n_exception_safety();

// -- section: move_iterator (move_family_tests_move_iterator.cpp)
bool tests_move_iterator_reference_ref();
bool tests_move_iterator_reference_proxy();
bool tests_move_iterator_deref_moves();
bool tests_move_iterator_base();
bool tests_move_iterator_arithmetic();
bool tests_move_iterator_relational();
bool tests_move_iterator_converting_ctor();
bool tests_move_iterator_index();
bool tests_move_iterator_arrow();
bool tests_move_iterator_make();
bool tests_move_iterator_category();

// -- section: iter_move (move_family_tests_iter_move.cpp)
bool tests_iter_move_default_type();
bool tests_iter_move_rvalue_reference_alias();
bool tests_iter_move_proxy_passthrough();
bool tests_iter_move_adl_customization();
bool tests_iter_move_moves();

// -- section: move traits (move_family_tests_traits.cpp)
bool tests_is_move_constructible();
bool tests_is_move_assignable();
bool tests_is_nothrow_move_constructible();
bool tests_is_nothrow_move_assignable();
bool tests_is_trivially_move_constructible();
bool tests_is_trivially_move_assignable();
bool tests_traits_v_forms();


///////////////////////////////////////////////////////////////////////////////
///                SPEC-PROVIDER FACE  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// move_family_spec
//   function: assembles the module_spec for the move-family suite — one block
// per header/family-member.  Compiled only in the runner TU.
inline ::djinterp::test::module_spec
move_family_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec m;

    m.name       = "re_std move family";
    m.descriptor =
        "The re_std move family: the move cast and move_if_noexcept, the move / "
        "move_backward algorithms, uninitialized_move / _n, the move_iterator "
        "adaptor, the iter_move CPO, and the six move type traits.";

    m.blocks = std::vector<dt::block_spec>
    {
        dt::block_spec
        {
            "utility: move (cast)",
            "re_std::move — the cast to rvalue reference.",
            std::vector<dt::test_spec>
            {
                { "return type",  "move(T&) yields remove_reference&&.", &tests_move_type      },
                { "noexcept",     "the cast is noexcept.",               &tests_move_noexcept  },
                { "semantics",    "move actually enables moving.",       &tests_move_semantics },
                { "constexpr",    "usable in a constant expression.",    &tests_move_constexpr }
            }
        },
        dt::block_spec
        {
            "utility: move_if_noexcept",
            "move-or-copy cast keyed on nothrow-move / copyability.",
            std::vector<dt::test_spec>
            {
                { "nothrow-move type",   "returns T&& when move is nothrow.",   &tests_move_if_noexcept_nothrow_type      },
                { "throwing + copyable", "returns const T& to force a copy.",   &tests_move_if_noexcept_throwing_copyable },
                { "move-only",           "returns T&& when there is no copy.",  &tests_move_if_noexcept_move_only         },
                { "semantics",           "moves vs copies as the rule dictates.", &tests_move_if_noexcept_semantics       }
            }
        },
        dt::block_spec
        {
            "algorithm: move",
            "range move into a destination iterator.",
            std::vector<dt::test_spec>
            {
                { "basic",            "moves the range; returns dest end.", &tests_algorithm_move_basic           },
                { "semantics",        "moves, never copies.",               &tests_algorithm_move_semantics       },
                { "empty range",      "no-op on an empty input.",           &tests_algorithm_move_empty           },
                { "overload coexist", "1-arg cast vs 3-arg algorithm.",     &tests_algorithm_move_overload_coexist }
            }
        },
        dt::block_spec
        {
            "algorithm: move_backward",
            "reverse-order range move for right-shifting overlap.",
            std::vector<dt::test_spec>
            {
                { "basic",     "moves the range; returns dest begin.", &tests_move_backward_basic     },
                { "overlap",   "correct right-shift on overlap.",      &tests_move_backward_overlap   },
                { "semantics", "moves, never copies.",                 &tests_move_backward_semantics },
                { "empty range","no-op on an empty input.",            &tests_move_backward_empty     }
            }
        },
        dt::block_spec
        {
            "memory: uninitialized_move",
            "move-construct a range into raw storage.",
            std::vector<dt::test_spec>
            {
                { "basic",             "move-constructs; returns dest end.", &tests_uninitialized_move_basic           },
                { "semantics",         "moves, never copies.",               &tests_uninitialized_move_semantics       },
                { "exception safety",  "a throw destroys built elements.",   &tests_uninitialized_move_exception_safety }
            }
        },
        dt::block_spec
        {
            "memory: uninitialized_move_n",
            "sized move-construct; returns the iterator pair.",
            std::vector<dt::test_spec>
            {
                { "basic",            "returns {src+n, dest+n}.",       &tests_uninitialized_move_n_basic           },
                { "semantics",        "moves, never copies.",           &tests_uninitialized_move_n_semantics       },
                { "exception safety", "a throw destroys built elements.", &tests_uninitialized_move_n_exception_safety }
            }
        },
        dt::block_spec
        {
            "iterator: move_iterator",
            "adaptor turning dereference into an xvalue.",
            std::vector<dt::test_spec>
            {
                { "reference (ref iter)",  "reference is T&& for T& bases.",     &tests_move_iterator_reference_ref     },
                { "reference (proxy)",     "value references pass through.",     &tests_move_iterator_reference_proxy   },
                { "deref moves",           "operator* yields a movable xvalue.", &tests_move_iterator_deref_moves       },
                { "base",                  "base() returns the wrapped iter.",   &tests_move_iterator_base              },
                { "arithmetic",            "++ -- += -= + - and difference.",    &tests_move_iterator_arithmetic        },
                { "relational",           "== != < > <= >= (non-flipped).",     &tests_move_iterator_relational        },
                { "converting ctor",       "constructs from move_iterator<U>.",  &tests_move_iterator_converting_ctor   },
                { "operator[]",            "indexed access yields an xvalue.",   &tests_move_iterator_index             },
                { "operator->",            "arrow returns the base iterator.",   &tests_move_iterator_arrow             },
                { "make_move_iterator",    "factory wraps an iterator.",         &tests_move_iterator_make              },
                { "category / concept",    "category preserved; concept input.", &tests_move_iterator_category          }
            }
        },
        dt::block_spec
        {
            "iterator: iter_move",
            "the iter_move CPO and iter_rvalue_reference_t.",
            std::vector<dt::test_spec>
            {
                { "default type",       "default cast yields the rvalue ref.",  &tests_iter_move_default_type            },
                { "rvalue-ref alias",   "iter_rvalue_reference_t matches.",     &tests_iter_move_rvalue_reference_alias  },
                { "proxy pass-through", "value references pass through.",       &tests_iter_move_proxy_passthrough       },
                { "ADL customization",  "a user iter_move is preferred.",       &tests_iter_move_adl_customization       },
                { "moves",              "default path actually moves.",         &tests_iter_move_moves                   }
            }
        },
        dt::block_spec
        {
            "type traits: move family",
            "the six move traits, cross-checked against <type_traits>.",
            std::vector<dt::test_spec>
            {
                { "is_move_constructible",           "constructible from T&&.",        &tests_is_move_constructible            },
                { "is_move_assignable",              "assignable from T&&.",           &tests_is_move_assignable               },
                { "is_nothrow_move_constructible",   "nothrow-constructible from T&&.", &tests_is_nothrow_move_constructible    },
                { "is_nothrow_move_assignable",      "nothrow-assignable from T&&.",   &tests_is_nothrow_move_assignable       },
                { "is_trivially_move_constructible", "trivially-constructible from T&&.", &tests_is_trivially_move_constructible },
                { "is_trivially_move_assignable",    "trivially-assignable from T&&.", &tests_is_trivially_move_assignable     },
                { "_v forms",                        "variable templates mirror ::value.", &tests_traits_v_forms               }
            }
        }
    };

    return m;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_MOVE_FAMILY_TESTS_
