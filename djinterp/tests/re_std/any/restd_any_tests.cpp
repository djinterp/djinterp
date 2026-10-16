/*******************************************************************************
* djinterp [test]                                            restd_any_tests.cpp
*
* re_std any test suite implementation:
*   Out-of-line definitions for the registry, helper types, and
* runner functions declared in restd_any_tests.hpp.
*
*
* path:      /tests/djinterp/re_std/any/restd_any_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstdio>
#include <cstring>
// djinterp
#include "restd_any_tests.hpp"


namespace restd_any_tests
{


// ===========================================================================
// I.   REGISTRY STORAGE
// ===========================================================================

unsigned long g_total   = 0;
unsigned long g_failed  = 0;
unsigned long g_skipped = 0;

test_case* g_head = 0;


test_registrar::test_registrar(
    test_case* _tc
)
{
    _tc->m_next = g_head;
    g_head      = _tc;
}


// ===========================================================================
// II.  RUNNER
// ===========================================================================

void
log_fail(
    const char* _file,
    int         _line,
    const char* _expr
)
{
    std::fprintf(stderr, "  FAIL: %s:%d  %s\n", _file, _line, _expr);
}


static int
run_filtered(
    const char* _suite_filter
)
{
    // record baseline counters so we can compute deltas per test
    unsigned long pre_total  = g_total;
    unsigned long pre_failed = g_failed;
    (void)pre_total;
    (void)pre_failed;

    unsigned long suite_count = 0;
    unsigned long test_count  = 0;

    // because tests are pushed to head, the registered order is
    // reversed.  We walk once to count, then walk again to run
    // (no sort - deterministic enough for development).
    for (test_case* tc = g_head; tc; tc = tc->m_next)
    {
        if ( _suite_filter &&
             (std::strcmp(tc->m_suite, _suite_filter) != 0) )
        {
            continue;
        }

        ++test_count;
    }

    std::printf("\n=== running %lu test(s)", test_count);
    if (_suite_filter)
    {
        std::printf(" in suite '%s'", _suite_filter);
    }
    std::printf(" ===\n");

    for (test_case* tc = g_head; tc; tc = tc->m_next)
    {
        if ( _suite_filter &&
             (std::strcmp(tc->m_suite, _suite_filter) != 0) )
        {
            continue;
        }

        unsigned long before_fail = g_failed;

        std::printf("  [%s] %s ... ", tc->m_suite, tc->m_name);
        std::fflush(stdout);

#if D_ENV_CPP98_HAS_EXCEPTION || D_ENV_CPP98_HAS_TYPEINFO
        try
        {
            tc->m_fn();
        }
        catch (const std::exception& _e)
        {
            ++g_failed;
            std::printf("EXCEPTION (%s)\n", _e.what());
            continue;
        }
        catch (...)
        {
            ++g_failed;
            std::printf("UNKNOWN EXCEPTION\n");
            continue;
        }
#else
        tc->m_fn();
#endif

        if (g_failed > before_fail)
        {
            std::printf("FAIL\n");
        }
        else
        {
            std::printf("ok\n");
            ++suite_count;
        }
    }

    std::printf(
        "\n=== %lu/%lu tests passed; %lu expectations total, %lu failed,"
        " %lu skipped ===\n",
        suite_count,
        test_count,
        g_total,
        g_failed,
        g_skipped
    );

    return static_cast<int>(g_failed);
}


int
run_all()
{
    return run_filtered(0);
}


int
run_suite(
    const char* _suite
)
{
    return run_filtered(_suite);
}


// ===========================================================================
// III. COUNTED IMPLEMENTATION
// ===========================================================================

static int s_counted_alive = 0;
static int s_counted_total = 0;
static int s_counted_next  = 1;


Counted::Counted()
    : m_id(s_counted_next++),
      m_payload(0)
{
    ++s_counted_alive;
    ++s_counted_total;
}


Counted::Counted(
    int _payload
)
    : m_id(s_counted_next++),
      m_payload(_payload)
{
    ++s_counted_alive;
    ++s_counted_total;
}


Counted::Counted(
    const Counted& _other
)
    : m_id(s_counted_next++),
      m_payload(_other.m_payload)
{
    ++s_counted_alive;
    ++s_counted_total;
}


Counted&
Counted::operator=(
    const Counted& _other
)
{
    if (this != &_other)
    {
        m_payload = _other.m_payload;
    }
    return *this;
}


Counted::~Counted()
{
    --s_counted_alive;
}


int
Counted::id() const D_NOEXCEPT
{
    return m_id;
}


int
Counted::payload() const D_NOEXCEPT
{
    return m_payload;
}


int
Counted::alive() D_NOEXCEPT
{
    return s_counted_alive;
}


int
Counted::total_constructed() D_NOEXCEPT
{
    return s_counted_total;
}


void
Counted::reset_counts() D_NOEXCEPT
{
    s_counted_alive = 0;
    s_counted_total = 0;
    s_counted_next  = 1;
}


// ===========================================================================
// IV.  LARGETYPE IMPLEMENTATION
// ===========================================================================

LargeType::LargeType()
    : m_marker(0)
{
    for (int i = 0; i < 256; ++i)
    {
        m_buf[i] = static_cast<char>(i & 0xff);
    }
}


LargeType::LargeType(
    int _marker
)
    : m_marker(_marker)
{
    for (int i = 0; i < 256; ++i)
    {
        m_buf[i] = static_cast<char>((i + _marker) & 0xff);
    }
}


LargeType::LargeType(
    const LargeType& _other
)
    : m_marker(_other.m_marker)
{
    for (int i = 0; i < 256; ++i)
    {
        m_buf[i] = _other.m_buf[i];
    }
}


bool
LargeType::operator==(
    const LargeType& _other
) const
{
    if (m_marker != _other.m_marker)
    {
        return false;
    }
    for (int i = 0; i < 256; ++i)
    {
        if (m_buf[i] != _other.m_buf[i])
        {
            return false;
        }
    }
    return true;
}


// ===========================================================================
// V.   POLYMORPHIC TYPES
// ===========================================================================

Base::~Base()
{}


int
Base::kind() const
{
    return 1;
}


Derived::Derived()
    : m_extra(0)
{}


Derived::Derived(
    int _extra
)
    : m_extra(_extra)
{}


int
Derived::kind() const
{
    return 2;
}


// ===========================================================================
// VI.  THROW-ON-COPY
// ===========================================================================

ThrowOnCopy::ThrowOnCopy()
{}


ThrowOnCopy::ThrowOnCopy(
    const ThrowOnCopy&
)
{
#if D_ENV_CPP98_HAS_EXCEPTION
    throw std::exception();
#endif
}


// ===========================================================================
// VII. AGGREGATE
// ===========================================================================

Aggregate::Aggregate()
    : m_a(0),
      m_b(0.0)
{}


Aggregate::Aggregate(
    int    _a,
    double _b
)
    : m_a(_a),
      m_b(_b)
{}


bool
Aggregate::operator==(
    const Aggregate& _other
) const
{
    return ( (m_a == _other.m_a) &&
             (m_b == _other.m_b) );
}


// ===========================================================================
// VIII. FUNCTION-POINTER TARGETS
// ===========================================================================

int
dummy_function()
{
    return 42;
}


int
dummy_function_with_args(
    int _a,
    int _b
)
{
    return (_a + _b);
}


}  // namespace restd_any_tests
