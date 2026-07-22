/******************************************************************************
* djinterp [test]                                              test_common.hpp
*
*   DTest framework common definitions header.  Provides the foundational
* types, enumerations, and event infrastructure shared by all DTest modules:
*   - test type identification (test_type_id)
*   - the default node metadata container (basic_metadata / test_metadata)
*   - test status classification (passed, failed, skipped, pending, error)
*   - test event identifier type and event data structure
*   - event handler function pointer type for runtime event dispatch
*   - portable constexpr support macros for compile-time evaluation
*
*   TEST TYPE IDENTIFICATION:
*   Every test_object carries a test_type_id - a signed 32-bit integer
* that identifies the kind of test it represents.  In isolation (no
* test_type registry), the id acts as a numeric rank: a child's id
* must be <= its parent's.  When a test_tree holds a test_type
* registry, matching ids resolve to their test_kind definition, which
* supplies rank, leaf/interior classification, and default options.
*
*   CONSTEXPR EVALUATION:
*   Test objects support dual-mode evaluation: constexpr (compile-time)
* and runtime.  The D_TEST_CONSTEXPR macro resolves to D_CONSTEXPR when
* the language standard permits relaxed constexpr (C++14+), enabling
* test logic to be verified at compile time.  Event dispatch is
* runtime-only.
*
*   PORTABILITY:
*   This header uses env.h for C++ version detection and djinterp.hpp
* for namespace macros and constexpr support.  Minimum requirement is
* C++11.
* 
*
* path:      /inc/djinterp/test/test_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.14
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TEST TYPE IDENTIFICATION
II.   STATUS CLASSIFICATION
III.  EVENT SYSTEM
IV.   CONSTEXPR SUPPORT MACROS
*/

#ifndef DJINTERP_TEST_COMMON_
#define DJINTERP_TEST_COMMON_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/meta/kv_pair.hpp"   // kv_pair (the metadata row type)


#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST TYPE IDENTIFICATION                             ///
///////////////////////////////////////////////////////////////////////////////

// test_type_id
//   type: signed 32-bit identifier for the KIND of test a
// test_object represents.
//
//   In isolation (no test_kind set) the id doubles as a
// numeric rank: a child's id must be <= its parent's.  When a
// test_kind set is attached to the tree, a matching id resolves
// to its test_kind definition, which supplies rank,
// leaf/interior classification, and default options.  See
// test_kind.hpp for the resolved-query free functions.
//
//   This is the node's structural type identity; it is NOT a
// per-instance unique id (a node carries neither a unique id
// nor a depth - both are facts of position the owning tree
// confers during the walk; see test_object.hpp).
typedef std::int32_t test_type_id;


// test_callable_id
//   type: opaque non-owning handle into a test_callable_table.
//
//   Used by test_object to reference a deferred-evaluation
// callable (a closure or function) without storing the
// callable inline.  This keeps test_object trivially
// copyable and constexpr-friendly while still allowing
// individual leaves to carry runtime work that the handler
// invokes during the tree walk.
//
//   The value 0 is reserved as the "no callable" sentinel.
// Any node carrying id 0 is treated as fully-evaluated
// (the value already in the node's m_result / m_status is
// authoritative); any node carrying a non-zero id is
// treated as deferred (the handler invokes the callable
// from the bound table immediately before firing
// per-test events).
//
//   The choice of std::uint32_t allows up to ~4 billion
// distinct callables per table without wrap.  Test suites
// of any realistic size fit easily within this range.
typedef std::uint32_t test_callable_id;


// k_no_callable
//   constant: the reserved zero-id meaning "no deferred
// callable is bound to this node".  Equivalent to writing
// 0 directly, but more self-documenting at call sites.
D_STATIC D_CONSTEXPR test_callable_id k_no_callable =
    static_cast<test_callable_id>(0);


// iii. default node metadata
//////////////////////////////////////////

// basic_metadata
//   forward declaration: the default per-node key/value metadata container,
// completed in test_object.hpp.  It is declared HERE - the shared floor both
// test_object.hpp and test_event.hpp include - so that both siblings can name
// the default instantiation without either including the other.
//
//   The default template arguments live on THIS declaration and nowhere else:
// a class template's default arguments may appear on only one declaration, and
// the definition in test_object.hpp deliberately omits them.  Placing them on
// the floor header is what lets test_event.hpp spell `test_metadata` (below)
// for its `basic_test` alias while sitting beneath test_object.hpp in the
// include chain.
//
// Template parameters:
//   _Key       - the metadata key type.        Default: std::string.
//   _Value     - the metadata mapped value.     Default: std::string.
//   _Container - the backing storage, holding kv_pair<_Key, _Value> rows.
//                Default: std::vector<kv_pair<_Key, _Value>>.
template<typename _Key       = std::string,
         typename _Value     = std::string,
         typename _Container = std::vector< ::djinterp::kv_pair<_Key, _Value> > >
class basic_metadata;

// test_metadata
//   type: the default metadata container - basic_metadata's all-defaulted
// instantiation (std::string keys / std::string values over a std::vector of
// rows).  A non-template alias, so every use site spells `test_metadata`
// unchanged; it is the default _MetadataContainer for test_object (and thus
// for basic_test).
using test_metadata = basic_metadata<>;


///////////////////////////////////////////////////////////////////////////////
///                II.  STATUS CLASSIFICATION                               ///
///////////////////////////////////////////////////////////////////////////////

// test_status
//   enum: classification of a test object's evaluation state.
// A test object transitions from `pending` to `passed` or
// `failed` upon evaluation.  `skipped` indicates intentional
// non-evaluation.  `error` indicates that evaluation itself
// could not complete.
enum class test_status
{
    passed  = 0,
    failed  = 1,
    skipped = 2,
    pending = 3,
    error   = 4
};

///////////////////////////////////////////////////////////////////////////////
///                III. EVENT SYSTEM                                         ///
///////////////////////////////////////////////////////////////////////////////

// i.   event identifier
//////////////////////////////////////////

// test_event_id
//   type: lightweight numeric identifier for test lifecycle
// events.
typedef std::size_t test_event_id;


// ii.  event data
//////////////////////////////////////////

// test_event
//   struct: encapsulates the data associated with a single
// test event.  Carries the event identifier, the status at
// the time of the event, and an optional human-readable
// message.
struct test_event
{
    test_event_id event;
    test_status   status;
    const char*   message;

    D_CONSTEXPR test_event(
        test_event_id _event,
        test_status   _status,
        const char*   _message = nullptr
    ) D_NOEXCEPT
        : event(_event),
          status(_status),
          message(_message)
    {}
};

///////////////////////////////////////////////////////////////////////////////
///                IV.  CONSTEXPR SUPPORT MACROS                            ///
///////////////////////////////////////////////////////////////////////////////

// D_TEST_CONSTEXPR
//   macro: resolves to D_CONSTEXPR on C++14 or later, where
// relaxed constexpr permits local variables, loops, and
// assignments within constexpr functions.  On C++11, resolves
// to nothing because single-return constexpr is too restrictive
// for most test evaluation bodies.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_TEST_CONSTEXPR            D_CONSTEXPR
#else
    #define D_TEST_CONSTEXPR
#endif

// D_TEST_STATIC_CONSTEXPR
//   macro: compound qualifier composing D_STATIC and
// D_TEST_CONSTEXPR.
#define D_TEST_STATIC_CONSTEXPR         D_STATIC D_TEST_CONSTEXPR


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_COMMON_