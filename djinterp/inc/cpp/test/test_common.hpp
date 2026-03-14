/******************************************************************************
* djinterp [test]                                           test_common.hpp
*
* Common types and declarations for the C++ test framework.
*   Provides the shared foundation for all test modules: a type-safe
* node identifier (test_id), lifecycle stage enumeration, outcome
* status enumeration, callable type aliases, and configurable
* pass/fail display symbols.
*
*   This header is deliberately abstract. It has no knowledge of
* specific node kinds (modules, blocks, assertions, etc.); node
* identity is established solely through test_id. Tree structure,
* rank ordering, and node categorization are the concern of
* test_tree.hpp and higher-level modules that build on top of it.
*
* COMPONENTS:
*   djinterp::test_id              - opaque node identifier
*   djinterp::test_id_generator    - monotonic id source
*   djinterp::DTestStage           - lifecycle stage for execution hooks
*   djinterp::DTestStatus          - outcome status (passed/failed/skipped)
*   djinterp::fn_test              - test callable type alias
*   djinterp::fn_stage             - lifecycle hook callable type alias
*   djinterp::test_symbols         - pass/fail and status display symbols
*
* REPLACES:
*   C header test_common.h. The node type discriminator (DTestTypeFlag),
*   d_test_arg / d_test_arg_list structs, d_test_counter, and d_test_fn
*   are removed. Node types and tree rank are handled by test_tree.hpp;
*   counters by test_stats.hpp; configuration by test_options.hpp.
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


//#ifndef __cplusplus
//    #error "test_common.hpp can only be used in C++ compilation mode"
//#endif
//
//#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
//    #error "test_common.hpp requires C++11 or higher"
//#endif


NS_DJINTERP
NS_TEST

// =========================================================================
// I.   FORWARD DECLARATIONS
// =========================================================================
struct test_node;


// =========================================================================
// II.  CONSTANTS
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

// D_TEST_STATUS_COUNT
//   constant: number of values in DTestStatus.
static constexpr std::size_t D_TEST_STATUS_COUNT = 4;

// =========================================================================
// III. ENUMERATIONS
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
// IV.  IDENTIFIER TYPE
// =========================================================================
 
// test_id
//   type: identifier for test nodes. Nodes are distinguished
// solely by their test_id; this header imposes no type taxonomy
// on nodes. Equality, ordering, and hashing are inherited from
// the underlying integer type.
using test_id = std::uint64_t;
 
// D_TEST_ID_INVALID
//   constant: sentinel value representing a null or unassigned
// test_id. A value of 0 is never assigned by test_id_generator.
static constexpr test_id D_TEST_ID_INVALID = 0;
 
// is_valid
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
// V.   IDENTIFIER GENERATOR
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
    {};
 
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
// VI.  CALLABLE TYPE ALIASES
// =========================================================================
 
// fn_test
//   type: callable for a boolean test that returns D_TEST_PASS
// or D_TEST_FAIL. Used by leaf nodes in the test tree.
using fn_test = std::function<bool()>;
 
// fn_stage
//   type: callable for a test lifecycle stage hook. Receives
// a mutable reference to the test node being executed and
// returns success (true) or failure (false).
using fn_stage = std::function<bool(test_node&)>;
 
 
// =========================================================================
// VII. SYMBOLS
// =========================================================================
 
// test_symbols
//   struct: display symbols for test results and status
// indicators. Uses emoji when D_EMOJIS is enabled at compile
// time, falling back to ASCII bracket tags otherwise. Access
// as static members: test_symbols::pass, test_symbols::info,
// etc.
//
//   Node-type-specific symbols are deliberately omitted; they
// are the concern of the tree or printer modules which have
// knowledge of the node taxonomy.
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