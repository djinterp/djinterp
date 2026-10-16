/*******************************************************************************
* djinterp [test]                    threadsafe_container_tests_atomic_state.cpp
*
*   Layer 3 - atomic_state, the bundled atomic size + version metadata.
* Defines the "atomic_state" block.
*
*
* path:      /tests/djinterp/core/container/threadsafe_container_tests_atomic_state.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "threadsafe_container_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_atomic_state_size
//   Verifies the size field's load / store / fetch_add / fetch_sub, including
//   the fetch-* return value (the value BEFORE the update).
//   Tests the following:
//   - starts at zero; store then load round-trips
//   - fetch_add returns the prior value and advances the field
//   - fetch_sub returns the prior value and reduces the field
bool
tests_atomic_state_size()
{
    atomic_state st;
    D_TC_CHECK(st.load_size() == 0u);

    st.store_size(5);
    D_TC_CHECK(st.load_size() == 5u);

    std::size_t before_add = st.fetch_add_size(3);
    D_TC_CHECK(before_add == 5u);
    D_TC_CHECK(st.load_size() == 8u);

    std::size_t before_sub = st.fetch_sub_size(2);
    D_TC_CHECK(before_sub == 8u);
    D_TC_CHECK(st.load_size() == 6u);
    return true;
}

// tests_atomic_state_version
//   Verifies the version field's load and increment.
//   Tests the following:
//   - starts at zero
//   - increment returns the prior value and advances by one, twice
bool
tests_atomic_state_version()
{
    atomic_state st;
    D_TC_CHECK(st.load_version() == 0u);

    std::uint64_t v0 = st.increment_version();
    D_TC_CHECK(v0 == 0u);
    D_TC_CHECK(st.load_version() == 1u);

    std::uint64_t v1 = st.increment_version();
    D_TC_CHECK(v1 == 1u);
    D_TC_CHECK(st.load_version() == 2u);
    return true;
}

// tests_atomic_state_snapshot
//   Verifies the snapshot capture and validation.  validate_snapshot keys on
//   the VERSION only - a size change with no version bump stays valid.
//   Tests the following:
//   - take_snapshot captures the current size and version
//   - validate is true while the version is unchanged
//   - a version bump invalidates; a bare size change does not
bool
tests_atomic_state_snapshot()
{
    atomic_state st;
    st.store_size(4);
    st.increment_version();                 // version -> 1

    atomic_state::snapshot snap = st.take_snapshot();
    D_TC_CHECK(snap.size == 4u);
    D_TC_CHECK(snap.version == 1u);
    D_TC_CHECK(st.validate_snapshot(snap));

    st.increment_version();                 // version -> 2
    D_TC_CHECK(!st.validate_snapshot(snap));

    atomic_state::snapshot snap2 = st.take_snapshot();
    st.store_size(100);                     // no version bump
    D_TC_CHECK(st.validate_snapshot(snap2));
    return true;
}

NS_END  // testing
NS_END  // djinterp
