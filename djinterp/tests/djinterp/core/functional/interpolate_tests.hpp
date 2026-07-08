/******************************************************************************
* djinterp [test]                                            interpolate_tests.hpp
*
*   DTest declarations and shared fixtures for the interpolate.hpp unit-test
* suite.  interpolate.hpp is the type-agnostic interpolation engine (scanner /
* resolver / sink policies + the fold that binds them, plus the lazy
* `interpolation` builder and its pre-parsed `prepared_interpolation`
* counterpart).  Each like-group semantic section of the module maps to one
* .cpp translation unit, which defines its `bool()` test predicates and packages
* them into a block via the block-provider declared here.  The runner
* (interpolate_tests_runner.cpp) assembles the blocks into a module_spec and
* hands it to run_module.
*
*     interpolate_tests_piece.cpp       -> piece_block         (I)
*     interpolate_tests_brace.cpp       -> brace_block         (II.i)
*     interpolate_tests_sigil.cpp       -> sigil_block         (II.ii)
*     interpolate_tests_replay.cpp      -> replay_block        (II.iii)
*     interpolate_tests_resolution.cpp  -> resolution_block    (III.i)
*     interpolate_tests_resolvers.cpp   -> resolvers_block     (III.ii-iv,vii)
*     interpolate_tests_composition.cpp -> composition_block   (III.v-vii)
*     interpolate_tests_sink.cpp        -> sink_block          (IV)
*     interpolate_tests_engine.cpp      -> engine_block        (V)
*     interpolate_tests_recursive.cpp   -> recursive_block     (VI)
*     interpolate_tests_builder.cpp     -> builder_block       (VII)
*     interpolate_tests_prepared.cpp    -> prepared_block      (VIII)
*     interpolate_tests_concepts.cpp    -> concepts_block      (IX, C++20)
*
*   Every test is a parameterless predicate returning true on success and false
* on the first failed check (via D_INTERP_CHECK).  Tests and fixtures live flat
* in djinterp::testing.
*
*   PORTABILITY:
*   interpolate.hpp requires C++17 (std::string_view backs the piece views) and
* self-suppresses below it; its concepts are C++20-only.  This suite mirrors
* those gates: below C++17 every block is emitted empty (the fixtures and
* predicates are compiled out), and the concepts block is additionally empty
* below C++20 -- so the suite links and reports a clean run at any standard
* level rather than failing to build.
*
*   FIXTURES use named functor / struct types rather than lambdas so the same
* types can serve both runtime invocation and (where relevant) constexpr /
* unevaluated contexts across every supported standard.
*
* path:      /tests/djinterp/core/functional/interpolate_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_INTERPOLATE_
#define DJINTERP_TESTING_FUNCTIONAL_INTERPOLATE_ 1

// std
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
// djinterp
#include <djinterp/core/functional/interpolate.hpp>
// DTest framework (spec model: module_spec / block_spec / test_spec + run_module)
#include <djinterp/test/test_common.hpp>
#include <djinterp/test/test_handler.hpp>
#include <djinterp/test/test_defaults.hpp>
#include <djinterp/test/test_runner.hpp>


NS_DJINTERP
NS_TESTING


// dt -- the framework namespace the block/test spec vocabulary lives in.
namespace dt = ::djinterp::test;


// D_INTERP_CHECK
//   macro: returns false from the enclosing predicate the moment a condition
// fails.  Wrapped in a do/while so it is a single statement usable without
// surrounding braces.  Defined unconditionally (harmless where no test uses it,
// i.e. below C++17).
#define D_INTERP_CHECK(_cond)                                                 \
    do                                                                        \
    {                                                                         \
        if (!(_cond))                                                         \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


// ---------------------------------------------------------------------------
//   SHARED FIXTURES (C++17+; the whole module is suppressed below C++17)
// ---------------------------------------------------------------------------
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// sv -- the ubiquitous key/value view type of a char interpolation.
using sv = std::basic_string_view<char>;


// scan_all
//   helper: drive a scanner to exhaustion, collecting every emitted piece in
// order.  Takes the scanner by value (scanners are cheap cursors) so callers
// can pass a temporary.  The returned pieces hold views into the scanner's
// source, which must outlive the vector.
template<typename _Scanner>
inline std::vector<::djinterp::piece<char>>
scan_all(
    _Scanner _scanner
)
{
    std::vector<::djinterp::piece<char>> _out;
    ::djinterp::piece<char>              _p;

    while (_scanner.next(_p))
    {
        _out.push_back(_p);
    }

    return _out;
}


// wrap_fn
//   helper: a (view) -> std::string callable that always produces a value,
// wrapping the key in brackets.  The always-hit shape drives lookup_resolver /
// lookup (a missing key still resolves, to "[key]").
struct wrap_fn
{
    std::string
    operator()(
        sv _key
    ) const
    {
        return std::string("[") + std::string(_key) + std::string("]");
    }
};


// pred_true
//   helper: a key predicate that admits every key (an always-open gate).
struct pred_true
{
    D_CONSTEXPR bool
    operator()(
        sv
    ) const
    {
        return true;
    }
};


// pred_is_a
//   helper: a key predicate true only for keys beginning with 'a' (a
// selective gate, for when / interpolate_if).
struct pred_is_a
{
    D_CONSTEXPR bool
    operator()(
        sv _key
    ) const
    {
        return (!_key.empty()) && (_key[0] == 'a');
    }
};


// counting_resolver
//   helper: a resolver that tallies how many times it is invoked (through a
// caller-owned counter) and answers with a fixed found/miss.  Used to observe
// the laziness of chain composition -- a chained fall-through frame must be
// consulted ONLY when the earlier frame misses.  value type is the view, so it
// composes with map_resolver / empty_resolver.
struct counting_resolver
{
    int* m_calls   = nullptr;
    bool m_hit     = false;
    sv   m_value   {};

    ::djinterp::resolution<sv>
    operator()(
        sv
    ) const
    {
        if (m_calls != nullptr)
        {
            ++(*m_calls);
        }

        if (m_hit)
        {
            return ::djinterp::resolved(m_value);
        }

        return ::djinterp::unresolved<sv>();
    }
};


// recording_sink
//   helper: a sink that records literal runs and resolved values into two
// separate caller-owned strings (each emission suffixed with '|'), so the
// engine's dispatch -- literal() for literal runs and misses, value() for hits
// -- can be asserted precisely rather than only through the merged output.
struct recording_sink
{
    std::string* m_literals = nullptr;
    std::string* m_values   = nullptr;

    void
    literal(
        sv _run
    )
    {
        if (m_literals != nullptr)
        {
            m_literals->append(_run.data(), _run.size());
            m_literals->push_back('|');
        }

        return;
    }

    template<typename _Value>
    void
    value(
        const _Value& _value
    )
    {
        if (m_values != nullptr)
        {
            const sv _view(_value);
            m_values->append(_view.data(), _view.size());
            m_values->push_back('|');
        }

        return;
    }
};

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


// ---------------------------------------------------------------------------
//   BLOCK PROVIDERS (one per section .cpp; always declared, emitted empty
//   below the tier a section needs)
// ---------------------------------------------------------------------------

dt::block_spec piece_block();        // I.    scan-event vocabulary
dt::block_spec brace_block();        // II.i  brace_scanner
dt::block_spec sigil_block();        // II.ii sigil_scanner
dt::block_spec replay_block();       // II.iii replay_scanner
dt::block_spec resolution_block();   // III.i resolution + or_else + factories
dt::block_spec resolvers_block();    // III.  empty / map / lookup resolvers
dt::block_spec composition_block();  // III.  chain / when composition
dt::block_spec sink_block();         // IV.   interp_string_sink
dt::block_spec engine_block();       // V.    interpolate_into
dt::block_spec recursive_block();    // VI.   recursive expansion
dt::block_spec builder_block();      // VII.  the lazy interpolation functor
dt::block_spec prepared_block();     // VIII. prepared_interpolation
dt::block_spec concepts_block();     // IX.   concepts (C++20)


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_INTERPOLATE_
