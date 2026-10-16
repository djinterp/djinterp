/*******************************************************************************
* djinterp [test]                      threadsafe_container_tests_optimistic.cpp
*
*   Layer 4 - optimistic_read, the seqlock read protocol.  Both the fast path
* (validate succeeds first attempt) and the retry-then-lock fall path are
* exercised deterministically by controlling the version counter.  Defines the
* "optimistic" block.
*
*
* path:      /tests/djinterp/core/container/threadsafe_container_tests_optimistic.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "threadsafe_container_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_optimistic_read_fast_path
//   Verifies that with no interleaved writer the read validates on the first
//   attempt and returns the callable's result without a retry or a fallback.
//   Tests the following:
//   - the returned value is the callable's result over the current contents
//   - the version is untouched (no retry / no fall to the read lock)
bool
tests_optimistic_read_fast_path()
{
    tc_vec<tc_excl> c;
    {
        locked_ref<tc_vec<tc_excl>, tc_excl> w(c, c.mutex());
        w->push_(10);
        w->push_(20);
        w->push_(30);
    }

    atomic_state st;
    st.store_size(3);

    int sum =
        optimistic_read<tc_vec<tc_excl>, tc_excl>(
            st, c, c.mutex(),
            [](const tc_vec<tc_excl>& _v)
            {
                int s = 0;
                for (std::size_t i = 0; i < _v.size(); ++i)
                {
                    s += _v[i];
                }
                return s;
            });

    D_TC_CHECK(sum == 60);
    D_TC_CHECK(st.load_version() == 0u);
    return true;
}

// tests_optimistic_read_fallback
//   Verifies the fall path: a callable that bumps the version on every call
//   makes validation fail every time, so the loop exhausts its retries and
//   falls through to a real read lock for one final, guaranteed-safe call.
//   Tests the following:
//   - the returned value is still the callable's result
//   - the callable ran max_retries + 1 times (3 retries + 1 fallback here),
//     evidenced by the version having advanced by exactly four
bool
tests_optimistic_read_fallback()
{
    tc_vec<tc_excl> c;
    {
        locked_ref<tc_vec<tc_excl>, tc_excl> w(c, c.mutex());
        w->push_(1);
        w->push_(2);
        w->push_(3);
        w->push_(4);
    }

    atomic_state st;
    st.store_size(4);

    std::size_t got =
        optimistic_read<tc_vec<tc_excl>, tc_excl>(
            st, c, c.mutex(),
            [&st](const tc_vec<tc_excl>& _v)
            {
                st.increment_version();     // forces validation to fail
                return _v.size();
            },
            3);

    D_TC_CHECK(got == 4u);
    D_TC_CHECK(st.load_version() == 4u);
    return true;
}

NS_END  // testing
NS_END  // djinterp
