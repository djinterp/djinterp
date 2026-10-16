/*******************************************************************************
* djinterp [test]                                            restd_any_tests.hpp
*
* re_std any test suite header:
*   Declarations, helper types, and test-registration macros for the
* re_std::any unit test suite.  Each test cpp registers its test
* cases at static initialization time via the TEST_CASE macro; the
* main runner walks the resulting linked list.
*
*   FRAMEWORK PHILOSOPHY:
*   Deliberately framework-agnostic - uses only standard C++98
* facilities (cstdio for output, exception-based assertions where
* available).  Macros can be retargeted to the djinterp test
* framework by changing EXPECT_* and TEST_CASE definitions.
*
*   PORTABILITY:
*   C++98 minimum.  C++11+ tests gated on feature macros; C++17+
* std::any compatibility tests gated on D_ENV_CPP17_HAS_ANY.
*
*
* path:      /tests/djinterp/re_std/any/restd_any_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RESTD_ANY_TESTS_
#define RESTD_ANY_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
// djinterp
// djinterp / re_std
#include "djinterp/re_std/any/any.hpp"
#include "djinterp/re_std/any/any_cast.hpp"
#include "djinterp/re_std/any/any_swap.hpp"
#include "djinterp/re_std/any/bad_any_cast.hpp"
#include "djinterp/re_std/type_traits/type_traits.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include "djinterp/re_std/any/make_any.hpp"
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER && \
    defined(__has_include)
    #if __has_include(<any>)
        // std
        #include <any>
        #define RESTD_ANY_TESTS_HAS_STD_ANY 1
    #endif
#endif

#ifndef RESTD_ANY_TESTS_HAS_STD_ANY
    #define RESTD_ANY_TESTS_HAS_STD_ANY 0
#endif


namespace restd_any_tests
{


// ===========================================================================
// I.   REGISTRY
// ===========================================================================

// counters: populated by EXPECT_* macros, read by the runner
extern unsigned long g_total;
extern unsigned long g_failed;
extern unsigned long g_skipped;

// test_fn
//   typedef: signature of a test case body.
typedef void (*test_fn)();

// test_case
//   struct: node in the global linked list of registered tests.
struct test_case
{
    const char* m_suite;
    const char* m_name;
    test_fn     m_fn;
    test_case*  m_next;
};

// g_head
//   variable: head pointer for the registered-test linked list.
extern test_case* g_head;

// test_registrar
//   class: pushes a test_case onto the global list at static
// initialization time.
struct test_registrar
{
    explicit test_registrar(test_case* _tc);
};


// ===========================================================================
// II.  RUNNER ENTRY POINTS
// ===========================================================================

// run_all
//   function: executes every registered test.  Returns the
// number of failed expectations.
int run_all();

// run_suite
//   function: executes every registered test whose suite name
// matches _suite.
int run_suite(const char* _suite);

// log_fail
//   function: records a failed expectation to stderr.  Called
// from the EXPECT_* macros.
void log_fail(
    const char* _file,
    int         _line,
    const char* _expr
);


// ===========================================================================
// III. HELPER TYPES
// ===========================================================================

// Counted
//   class: instrumented type that tracks alive instances for
// leak-detection tests.  Each instance carries a unique id; the
// alive counter is shared.
class Counted
{
public:
    Counted();
    Counted(int _payload);
    Counted(const Counted& _other);
    Counted&
    operator=(const Counted& _other);
    ~Counted();

    int
    id() const D_NOEXCEPT;

    int
    payload() const D_NOEXCEPT;

    static int
    alive() D_NOEXCEPT;

    static int
    total_constructed() D_NOEXCEPT;

    static void
    reset_counts() D_NOEXCEPT;

private:
    int m_id;
    int m_payload;
};

// LargeType
//   class: oversized type used to exercise the heap path with
// a non-trivial payload.  256-byte buffer plus a marker.
struct LargeType
{
    char m_buf[256];
    int  m_marker;

    LargeType();
    LargeType(int _marker);
    LargeType(const LargeType& _other);

    bool
    operator==(const LargeType& _other) const;
};

// Polymorphic types: Base / Derived
struct Base
{
    virtual ~Base();
    virtual int
    kind() const;
};

struct Derived : public Base
{
    int m_extra;

    Derived();
    explicit Derived(int _extra);

    virtual int
    kind() const;
};

// ThrowOnCopy
//   class: throws on copy construction.  Used to test exception
// safety of the heap path.
struct ThrowOnCopy
{
    ThrowOnCopy();
    ThrowOnCopy(const ThrowOnCopy& _other);
};

// SmallEnum
//   enum: small enum for category dispatch tests.
enum SmallEnum
{
    se_zero = 0,
    se_one  = 1,
    se_two  = 2
};

#if D_ENV_CPP_FEATURE_LANG_STRONG_ENUMS
enum class ScopedEnum : int
{
    sc_zero = 0,
    sc_one  = 1,
    sc_two  = 2
};
#endif

// Aggregate
//   struct: simple value-semantic aggregate for heap tests.
struct Aggregate
{
    int    m_a;
    double m_b;

    Aggregate();
    Aggregate(int _a, double _b);

    bool
    operator==(const Aggregate& _other) const;
};


// ===========================================================================
// IV.  HELPER FUNCTIONS
// ===========================================================================

// dummy_function
//   function: target for function-pointer tests.
int
dummy_function();

int
dummy_function_with_args(int _a, int _b);


}  // namespace restd_any_tests


// ===========================================================================
// V.   TEST MACROS
// ===========================================================================

// TEST_CASE
//   macro: declares a test function and registers it with the
// global registry at static initialization time.  Usage:
//
//     TEST_CASE(suite_name, test_name)
//     {
//         EXPECT_TRUE(some_condition);
//         EXPECT_EQ(actual, expected);
//     }
//
#define TEST_CASE(suite, name)                                                \
    static void test_fn_##suite##_##name();                                   \
    namespace                                                                 \
    {                                                                          \
        ::restd_any_tests::test_case s_tc_##suite##_##name =                  \
        {                                                                      \
            #suite,                                                            \
            #name,                                                             \
            &test_fn_##suite##_##name,                                        \
            0                                                                  \
        };                                                                     \
                                                                               \
        ::restd_any_tests::test_registrar s_reg_##suite##_##name(             \
            &s_tc_##suite##_##name );                                          \
    }                                                                          \
    static void test_fn_##suite##_##name()


// EXPECT_TRUE
//   macro: passes if _expr evaluates to true; otherwise logs and
// increments the failure counter.
#define EXPECT_TRUE(_expr)                                                    \
    do                                                                         \
    {                                                                          \
        ++::restd_any_tests::g_total;                                          \
        if (!(_expr))                                                          \
        {                                                                      \
            ++::restd_any_tests::g_failed;                                     \
            ::restd_any_tests::log_fail(__FILE__, __LINE__, #_expr);           \
        }                                                                      \
    }                                                                          \
    while (0)


// EXPECT_FALSE
#define EXPECT_FALSE(_expr)                                                   \
    do                                                                         \
    {                                                                          \
        ++::restd_any_tests::g_total;                                          \
        if ((_expr))                                                           \
        {                                                                      \
            ++::restd_any_tests::g_failed;                                     \
            ::restd_any_tests::log_fail(__FILE__, __LINE__,                    \
                                        "expected false: " #_expr);           \
        }                                                                      \
    }                                                                          \
    while (0)


// EXPECT_EQ
#define EXPECT_EQ(_a, _b)                                                     \
    do                                                                         \
    {                                                                          \
        ++::restd_any_tests::g_total;                                          \
        if (!((_a) == (_b)))                                                   \
        {                                                                      \
            ++::restd_any_tests::g_failed;                                     \
            ::restd_any_tests::log_fail(__FILE__, __LINE__,                    \
                                        #_a " == " #_b);                       \
        }                                                                      \
    }                                                                          \
    while (0)


// EXPECT_NE
#define EXPECT_NE(_a, _b)                                                     \
    do                                                                         \
    {                                                                          \
        ++::restd_any_tests::g_total;                                          \
        if (!((_a) != (_b)))                                                   \
        {                                                                      \
            ++::restd_any_tests::g_failed;                                     \
            ::restd_any_tests::log_fail(__FILE__, __LINE__,                    \
                                        #_a " != " #_b);                       \
        }                                                                      \
    }                                                                          \
    while (0)


// EXPECT_THROWS
//   macro: passes if _stmt throws an exception of type _exc.
#if D_ENV_CPP98_HAS_EXCEPTION || D_ENV_CPP98_HAS_TYPEINFO
    #define EXPECT_THROWS(_stmt, _exc)                                        \
        do                                                                     \
        {                                                                      \
            ++::restd_any_tests::g_total;                                      \
            bool _threw = false;                                               \
            try                                                                \
            {                                                                  \
                _stmt;                                                         \
            }                                                                  \
            catch (const _exc&)                                                \
            {                                                                  \
                _threw = true;                                                 \
            }                                                                  \
            catch (...)                                                        \
            {                                                                  \
                ::restd_any_tests::log_fail(                                   \
                    __FILE__, __LINE__,                                        \
                    "wrong exception type from: " #_stmt);                     \
            }                                                                  \
            if (!_threw)                                                       \
            {                                                                  \
                ++::restd_any_tests::g_failed;                                 \
                ::restd_any_tests::log_fail(                                   \
                    __FILE__, __LINE__, "no throw from: " #_stmt);             \
            }                                                                  \
        }                                                                      \
        while (0)
#else
    #define EXPECT_THROWS(_stmt, _exc)                                        \
        do                                                                     \
        {                                                                      \
            ++::restd_any_tests::g_skipped;                                    \
        }                                                                      \
        while (0)
#endif


// EXPECT_NO_THROW
#if D_ENV_CPP98_HAS_EXCEPTION || D_ENV_CPP98_HAS_TYPEINFO
    #define EXPECT_NO_THROW(_stmt)                                            \
        do                                                                     \
        {                                                                      \
            ++::restd_any_tests::g_total;                                      \
            try                                                                \
            {                                                                  \
                _stmt;                                                         \
            }                                                                  \
            catch (...)                                                        \
            {                                                                  \
                ++::restd_any_tests::g_failed;                                 \
                ::restd_any_tests::log_fail(                                   \
                    __FILE__, __LINE__,                                        \
                    "unexpected throw from: " #_stmt);                         \
            }                                                                  \
        }                                                                      \
        while (0)
#else
    #define EXPECT_NO_THROW(_stmt) do { _stmt; } while (0)
#endif


#endif  // RESTD_ANY_TESTS_
