/*******************************************************************************
* djinterp [tests]                            test_concurrent_tests_patterns.cpp
*
*   Section TU: the convenience execution patterns - run_simultaneous,
* run_simultaneous_with_timeout, and run_reader_writer.
*
*
* path:      /tests/djinterp/test/sync/test_concurrent_tests_patterns.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>
// djinterp
#include "test_concurrent_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_cpatterns_run_simultaneous()
{
    std::atomic<int> hits(0);

    const dt::concurrent_run_report rep =
        dt::run_simultaneous(
            4,
            [&hits](std::size_t /*id*/)
            {
                hits.fetch_add(1, std::memory_order_acq_rel);
            });

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(rep.thread_count == 4u);
    D_HC_CHECK(hits.load(std::memory_order_acquire) == 4);

    return true;
}

bool tests_cpatterns_run_simultaneous_reports()
{
    std::atomic<int> hits(0);

    const dt::concurrent_run_report rep =
        dt::run_simultaneous(
            3,
            [&hits](std::size_t /*id*/)
            {
                hits.fetch_add(1, std::memory_order_acq_rel);
            });

    D_HC_CHECK(rep.threads.size() == 3u);
    D_HC_CHECK(rep.completed_count == 3u);
    D_HC_CHECK(rep.failed_count == 0u);

    for (std::size_t i = 0; i < rep.threads.size(); ++i)
    {
        D_HC_CHECK(rep.threads[i].id == i);
        D_HC_CHECK(rep.threads[i].final_state ==
                   dt::test_thread_state::completed);
    }

    return true;
}

bool tests_cpatterns_run_simultaneous_failure()
{
    const dt::concurrent_run_report rep =
        dt::run_simultaneous(
            2,
            [](std::size_t /*id*/)
            {
                throw std::runtime_error("pattern throw");
            });

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::thread_threw);
    D_HC_CHECK(rep.failed_count == 2u);
    D_HC_CHECK(rep.first_exception != nullptr);

    return true;
}

bool tests_cpatterns_with_timeout_success()
{
    std::atomic<int> hits(0);

    const dt::concurrent_run_report rep =
        dt::run_simultaneous_with_timeout(
            3,
            std::chrono::seconds(5),          // generous window
            [&hits](std::size_t /*id*/)
            {
                hits.fetch_add(1, std::memory_order_acq_rel);
            });

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(hits.load(std::memory_order_acquire) == 3);

    return true;
}

bool tests_cpatterns_with_timeout_expires()
{
    const unsigned sleep_ms = hc_long_ms();

    const dt::concurrent_run_report rep =
        dt::run_simultaneous_with_timeout(
            2,
            hc_short(),                       // window far shorter than the work
            [sleep_ms](std::size_t /*id*/)
            {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(sleep_ms));
            });

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::timed_out);
    D_HC_CHECK(!rep.success());

    return true;
}

bool tests_cpatterns_reader_writer_counts()
{
    std::atomic<int> reads(0);
    std::atomic<int> writes(0);

    const dt::concurrent_run_report rep =
        dt::run_reader_writer(
            3,   // readers
            2,   // writers
            [&reads](std::size_t /*id*/)
            {
                reads.fetch_add(1, std::memory_order_acq_rel);
            },
            [&writes](std::size_t /*id*/)
            {
                writes.fetch_add(1, std::memory_order_acq_rel);
            });

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(rep.thread_count == 5u);
    D_HC_CHECK(reads.load(std::memory_order_acquire) == 3);
    D_HC_CHECK(writes.load(std::memory_order_acquire) == 2);

    return true;
}

bool tests_cpatterns_reader_writer_id_split()
{
    // readers take ids [0 .. readers-1]; writers take [readers .. total-1]
    std::mutex            mx;
    std::set<std::size_t> reader_ids;
    std::set<std::size_t> writer_ids;

    const dt::concurrent_run_report rep =
        dt::run_reader_writer(
            2,   // readers -> ids 0,1
            3,   // writers -> ids 2,3,4
            [&mx, &reader_ids](std::size_t _id)
            {
                std::lock_guard<std::mutex> lk(mx);
                reader_ids.insert(_id);
            },
            [&mx, &writer_ids](std::size_t _id)
            {
                std::lock_guard<std::mutex> lk(mx);
                writer_ids.insert(_id);
            });

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);

    D_HC_CHECK(reader_ids.size() == 2u);
    D_HC_CHECK(reader_ids.count(0) == 1u);
    D_HC_CHECK(reader_ids.count(1) == 1u);

    D_HC_CHECK(writer_ids.size() == 3u);
    D_HC_CHECK(writer_ids.count(2) == 1u);
    D_HC_CHECK(writer_ids.count(3) == 1u);
    D_HC_CHECK(writer_ids.count(4) == 1u);

    return true;
}


NS_END  // testing
NS_END  // djinterp
