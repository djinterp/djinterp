/******************************************************************************
* djinterp [test]                                           test_common.hpp
*
* Common types and declarations for the C++ test framework.
*   Provides the shared foundation for all test modules: a node
* identifier type (test_id), a rank type for tree ordering
* (test_rank), a user-extensible status type (test_status),
* lifecycle stage enumeration, callable type aliases, and
* configurable pass/fail display symbols.
*
*   This header is deliberately abstract. It has no knowledge of
* specific node kinds (modules, blocks, assertions, etc.); node
* identity is established solely through test_id. Tree structure,
* rank ordering, and node categorization are the concern of
* test_tree.hpp and higher-level modules that build on top of it.
*
* COMPONENTS:
*   djinterp::test::test_id            - node identifier (uint64_t)
*   djinterp::test::test_id_generator  - monotonic id source
*   djinterp::test::test_rank          - tree ordering rank (int32_t)
*   djinterp::test::test_status        - outcome code (int16_t)
*   djinterp::test::DTestStage         - lifecycle stage for hooks
*   djinterp::test::fn_test            - test callable type alias
*   djinterp::test::fn_stage           - lifecycle hook type alias
*   djinterp::test::test_symbols       - pass/fail display symbols
*
* USER-EXTENSIBLE STATUS:
*   test_status is an int16_t so users can define their own outcome
*   codes via a custom enum backed by int16_t. The framework provides
*   default constants (D_TEST_STATUS_UNKNOWN, _PASSED, _FAILED,
*   _SKIPPED); user values should start at D_TEST_STATUS_USER_START.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/cpp/test/test_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.12
******************************************************************************/

#ifndef DJINTERP_TEST_COMMON_
#define DJINTERP_TEST_COMMON_ 1

#include <cstddef>
#include <cstdint>
#include <functional>
#include "../djinterp.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   FORWARD DECLARATIONS
// =========================================================================

struct test_node_base;


// =========================================================================
// II.  CORE TYPE ALIASES
// =========================================================================

// test_id
//   type: identifier for test nodes. Nodes are distinguished
// solely by their test_id; this header imposes no type taxonomy
// on nodes. Equality, ordering, and hashing are inherited from
// the underlying integer type.
using test_id = std::uint64_t;

// test_rank
//   type: rank value for tree ordering. The tree enforces the
// invariant child.rank <= parent.rank on insertion. Higher
// values represent broader groupings; the user defines the
// numeric taxonomy.
using test_rank = std::int32_t;

// test_status
//   type: outcome status for a completed test or node. Defined
// as int16_t so users can specify their own status codes via a
// custom enum; the framework provides default constants below.
//
// User-defined status values should begin at
// D_TEST_STATUS_USER_START to avoid collisions with future
// framework-reserved values.
//
// Usage:
//   enum MyStatus : djinterp::test::test_status
//   {
//       MY_TIMEOUT = D_TEST_STATUS_USER_START,
//       MY_CRASHED,
//       MY_SKIPPED_PLATFORM
//   };
using test_status = std::int16_t;


// =========================================================================
// III. CONSTANTS
// =========================================================================

// D_TEST_PASS
//   constant: indicates a test, assertion, or evaluation passed.
static constexpr bool D_TEST_PASS = true;

// D_TEST_FAIL
//   constant: indicates a test, assertion, or evaluation failed.
static constexpr bool D_TEST_FAIL = false;

// D_TEST_STAGE_COUNT
//   constant: number of values in DTestStage.
static constexpr std::size_t D_TEST_STAGE_COUNT = 6;

// ---- identifier constants ----

// D_TEST_ID_INVALID
//   constant: sentinel value representing a null or unassigned
// test_id. A value of 0 is never assigned by test_id_generator.
static constexpr test_id D_TEST_ID_INVALID = 0;

// ---- framework-reserved status constants ----

static constexpr test_status D_TEST_STATUS_UNKNOWN = 0;
static constexpr test_status D_TEST_STATUS_PASSED  = 1;
static constexpr test_status D_TEST_STATUS_FAILED  = 2;
static constexpr test_status D_TEST_STATUS_SKIPPED = 3;

// D_TEST_STATUS_USER_START
//   constant: first value available for user-defined status
// codes. All values below this are reserved for the framework.
static constexpr test_status D_TEST_STATUS_USER_START = 0x100;


// =========================================================================
// IV.  ENUMERATIONS
// =========================================================================

// DTestStage
//   enum: lifecycle stages for test execution hooks.
// Each stage corresponds to a point in the test lifecycle where
// user-supplied callbacks may be invoked by the handler.
enum DTestStage : std::int32_t
{
    D_TEST_STAGE_SETUP      = 0,
    D_TEST_STAGE_TEAR_DOWN  = 1,
    D_TEST_STAGE_ON_SUCCESS = 2,
    D_TEST_STAGE_ON_FAILURE = 3,
    D_TEST_STAGE_BEFORE     = 4,
    D_TEST_STAGE_AFTER      = 5
};


// =========================================================================
// V.   STATUS UTILITIES
// =========================================================================

// status_from_bool
//   convenience: converts a boolean result to
// D_TEST_STATUS_PASSED or D_TEST_STATUS_FAILED.
inline test_status
status_from_bool
(
    bool _passed
)
{
    return _passed
           ? D_TEST_STATUS_PASSED
           : D_TEST_STATUS_FAILED;
}

// is_passing
//   returns true if the given status represents a passing
// outcome (currently only D_TEST_STATUS_PASSED).
inline bool
is_passing
(
    test_status _status
)
{
    return (_status == D_TEST_STATUS_PASSED);
}

// is_failing
//   returns true if the given status represents a definitive
// failure (currently only D_TEST_STATUS_FAILED).
inline bool
is_failing
(
    test_status _status
)
{
    return (_status == D_TEST_STATUS_FAILED);
}

// is_valid (test_id)
//   returns true if the test_id has been assigned (is not the
// invalid sentinel).
inline bool
is_valid
(
    test_id _id
)
{
    return (_id != D_TEST_ID_INVALID);
}


// =========================================================================
// VI.  IDENTIFIER GENERATOR
// =========================================================================

// test_id_generator
//   class: monotonically increasing id source. Each call to
// next() returns a unique test_id. Values start at 1; the
// value 0 is reserved for D_TEST_ID_INVALID.
class test_id_generator
{
public:
    test_id_generator()
        : m_next(1)
    {
    };

    // next
    //   returns the next unique test_id.
    test_id next()
    {
        return m_next++;
    };

    // peek
    //   returns the value that the next call to next() will
    // produce, without advancing the counter.
    test_id peek() const
    {
        return m_next;
    };

    // count
    //   returns the number of ids generated so far.
    std::uint64_t count() const
    {
        return (m_next - 1);
    };

    // reset
    //   resets the generator to its initial state.
    void reset()
    {
        m_next = 1;
    };

private:
    std::uint64_t m_next;
};


// =========================================================================
// VII. CALLABLE TYPE ALIASES
// =========================================================================

// fn_test
//   type: callable for a boolean test that returns D_TEST_PASS
// or D_TEST_FAIL. Used by leaf nodes in the test tree.
using fn_test = std::function<bool()>;

// fn_stage
//   type: callable for a test lifecycle stage hook. Receives
// a mutable reference to the base node being executed and
// returns success (true) or failure (false).
using fn_stage = std::function<bool(test_node_base&)>;


// =========================================================================
// VIII. SYMBOLS
// =========================================================================

// test_symbols
//   struct: display symbols for test results and status
// indicators. Uses emoji when D_EMOJIS is enabled at compile
// time, falling back to ASCII bracket tags otherwise.
struct test_symbols
{
#if ( defined(D_EMOJIS) &&  \
      (D_EMOJIS == D_ENABLED) )

    // ---- result symbols ----
    static constexpr const char* pass    = "\xE2\x9C\x94";
    static constexpr const char* fail    = "\xE2\x9D\x8C";
    static constexpr const char* success = "\xF0\x9F\x8E\x89";
    static constexpr const char* skipped = "\xE2\x9E\x96";

    // ---- status symbols ----
    static constexpr const char* info    = "\xF0\x9F\x93\x8B";
    static constexpr const char* warning = "\xE2\x9A\xA0";
    static constexpr const char* unknown = "\xE2\x9D\x93";

#else

    // ---- result symbols ----
    static constexpr const char* pass    = "[PASS]";
    static constexpr const char* fail    = "[FAIL]";
    static constexpr const char* success = "[SUCCESS]";
    static constexpr const char* skipped = "[SKIP]";

    // ---- status symbols ----
    static constexpr const char* info    = "[INFO]";
    static constexpr const char* warning = "[WARNING]";
    static constexpr const char* unknown = "[UNKNOWN]";

#endif  // D_EMOJIS
};


// =========================================================================
// IX.  UTILITY FUNCTIONS
// =========================================================================

// result_symbol
//   returns the display symbol for the given test_status.
inline const char*
result_symbol
(
    test_status _status
)
{
    switch (_status)
    {
        case D_TEST_STATUS_PASSED:
        {
            return test_symbols::pass;
        }

        case D_TEST_STATUS_FAILED:
        {
            return test_symbols::fail;
        }

        case D_TEST_STATUS_SKIPPED:
        {
            return test_symbols::skipped;
        }

        default:
        {
            return test_symbols::unknown;
        }
    }
}

// status_to_string
//   returns a human-readable string for the framework-reserved
// status values. User-defined statuses return "user".
inline const char*
status_to_string
(
    test_status _status
)
{
    switch (_status)
    {
        case D_TEST_STATUS_UNKNOWN:
        {
            return "unknown";
        }

        case D_TEST_STATUS_PASSED:
        {
            return "passed";
        }

        case D_TEST_STATUS_FAILED:
        {
            return "failed";
        }

        case D_TEST_STATUS_SKIPPED:
        {
            return "skipped";
        }

        default:
        {
            return "user";
        }
    }
}

// stage_to_string
//   returns a human-readable string for the given lifecycle
// stage.
inline const char*
stage_to_string
(
    DTestStage _stage
)
{
    switch (_stage)
    {
        case D_TEST_STAGE_SETUP:
        {
            return "setup";
        }

        case D_TEST_STAGE_TEAR_DOWN:
        {
            return "tear_down";
        }

        case D_TEST_STAGE_ON_SUCCESS:
        {
            return "on_success";
        }

        case D_TEST_STAGE_ON_FAILURE:
        {
            return "on_failure";
        }

        case D_TEST_STAGE_BEFORE:
        {
            return "before";
        }

        case D_TEST_STAGE_AFTER:
        {
            return "after";
        }

        default:
        {
            return "unknown";
        }
    }
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_COMMON_
