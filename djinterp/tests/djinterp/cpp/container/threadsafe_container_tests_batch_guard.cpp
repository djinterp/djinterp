/*******************************************************************************
* djinterp [test]                     threadsafe_container_tests_batch_guard.cpp
*
*   Layer 7 - DBatchGuard, the write-lock-held multi-operation transaction
* guard.  Defines the "DBatchGuard" block.
*
*
* path:      /tests/djinterp/cpp/container/threadsafe_container_tests_batch_guard.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "threadsafe_container_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_batch_guard_count
//   Verifies the diagnostic operation counter.
//   Tests the following:
//   - a fresh guard reports count zero
//   - each record() advances the count
bool tests_batch_guard_count()
{
    DTcVec<DTcExcl> c;
    DBatchGuard<DTcExcl> bg(c.mutex());

    D_TC_CHECK(bg.count() == 0u);
    bg.record();
    bg.record();
    bg.record();
    D_TC_CHECK(bg.count() == 3u);
    return true;
}

// tests_batch_guard_raii
//   Verifies the guard acquires and releases the write lock over a real mutex:
//   after the batch scope the mutex is free, so a further lock acquires (this
//   would deadlock if the guard had not released).
//   Tests the following:
//   - a write lock and a try_write_lock both acquire after the batch ends
bool tests_batch_guard_raii()
{
    DTcVec<DTcExcl> c;

    {
        DBatchGuard<DTcExcl> bg(c.mutex());
        bg.record();
    }                                       // write lock released here

    { c.write_lock(); }                     // would hang if still held
    D_TC_CHECK(c.try_write_lock().owns_lock());
    return true;
}

NS_END  // testing
NS_END  // djinterp
