/*******************************************************************************
* djinterp [test]                      threadsafe_container_tests_locked_ref.cpp
*
*   Layer 2 - the RAII locked accessors (locked_ref, const_locked_ref) and the
* locked_apply helpers.  Defines the "locked_ref" block.
*
*
* path:      /tests/djinterp/core/container/threadsafe_container_tests_locked_ref.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <type_traits>
#include <utility>
// djinterp
#include "threadsafe_container_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_locked_ref_write
//   Verifies a write-locked handle grants mutable access through each spelling
//   and that the writes land in the container.
//   Tests the following:
//   - operator-> , operator* , and get() all reach the mutable container
//   - the accumulated writes are observed after the lock is released
bool
tests_locked_ref_write()
{
    tc_vec<tc_excl> c;
    {
        locked_ref<tc_vec<tc_excl>, tc_excl> r(c, c.mutex());
        r->push_(1);
        (*r).push_(2);
        r.get().push_(3);
    }

    D_TC_CHECK(c.size() == 3u);
    D_TC_CHECK(c[0] == 1);
    D_TC_CHECK(c[1] == 2);
    D_TC_CHECK(c[2] == 3);
    return true;
}

// tests_const_locked_ref_read
//   Verifies a read-locked handle grants const access and reads the contents.
//   The write handle is released before the read handle is taken (the same
//   thread cannot hold both on a non-recursive mutex).
//   Tests the following:
//   - operator-> , operator* , and get() all reach the const container
bool
tests_const_locked_ref_read()
{
    tc_vec<tc_excl> c;
    {
        locked_ref<tc_vec<tc_excl>, tc_excl> w(c, c.mutex());
        w->push_(7);
        w->push_(8);
    }

    {
        const_locked_ref<tc_vec<tc_excl>, tc_excl> cr(c, c.mutex());
        D_TC_CHECK(cr->size() == 2u);
        D_TC_CHECK((*cr)[0] == 7);
        D_TC_CHECK(cr.get()[1] == 8);
    }
    return true;
}

// tests_locked_ref_move_noncopy
//   Verifies the ownership contract of the handles and a real move transfer.
//   Tests the following:
//   - both handles are move-constructible and non-copyable
//   - a moved-to write handle still mutates the container, then releases
bool
tests_locked_ref_move_noncopy()
{
    using w_ref = locked_ref<tc_vec<tc_excl>, tc_excl>;
    using r_ref = const_locked_ref<tc_vec<tc_excl>, tc_excl>;

    D_TC_CHECK(std::is_move_constructible<w_ref>::value);
    D_TC_CHECK(!std::is_copy_constructible<w_ref>::value);
    D_TC_CHECK(std::is_move_constructible<r_ref>::value);
    D_TC_CHECK(!std::is_copy_constructible<r_ref>::value);

    tc_vec<tc_excl> c;
    {
        w_ref r1(c, c.mutex());
        w_ref r2(std::move(r1));        // lock ownership transfers to r2
        r2->push_(5);
    }
    D_TC_CHECK(c.size() == 1u);
    D_TC_CHECK(c[0] == 5);
    return true;
}

// tests_locked_apply
//   Verifies the apply helpers run the callable under a lock and return its
//   result.
//   Tests the following:
//   - locked_apply invokes with a mutable reference and returns the result
//   - locked_apply_read invokes with a const reference and returns the result
bool
tests_locked_apply()
{
    tc_vec<tc_excl> c;

    std::size_t n =
        locked_apply<tc_vec<tc_excl>, tc_excl>(
            c, c.mutex(),
            [](tc_vec<tc_excl>& _v)
            {
                _v.push_(1);
                _v.push_(2);
                return _v.size();
            });
    D_TC_CHECK(n == 2u);
    D_TC_CHECK(c.size() == 2u);

    std::size_t m =
        locked_apply_read<tc_vec<tc_excl>, tc_excl>(
            c, c.mutex(),
            [](const tc_vec<tc_excl>& _v)
            {
                return _v.size();
            });
    D_TC_CHECK(m == 2u);
    return true;
}

NS_END  // testing
NS_END  // djinterp
