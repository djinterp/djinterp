/*******************************************************************************
* djinterp [tests]                             test_race_tests_linearization.cpp
*
*   Section TU: linearization_event and linearization_log - the append-only
* timestamped operation log.
*
*
* path:      /tests/djinterp/test/sync/test_race_tests_linearization.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
#include <thread>
#include <vector>
// djinterp
#include "test_race_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_rlog_event_defaults()
{
    dt::linearization_event ev;

    D_HR_CHECK(ev.thread_id == 0u);
    D_HR_CHECK(ev.op_name.empty());
    D_HR_CHECK(ev.args.empty());
    D_HR_CHECK(ev.result.empty());
    // both stamps default to the clock epoch
    D_HR_CHECK(ev.invoked_at == dt::linearization_event::time_point());
    D_HR_CHECK(ev.completed_at == dt::linearization_event::time_point());

    return true;
}

bool tests_rlog_empty()
{
    dt::linearization_log log;

    D_HR_CHECK(log.size() == 0u);
    D_HR_CHECK(log.snapshot().empty());

    return true;
}

bool tests_rlog_record_event()
{
    dt::linearization_log log;

    log.record_event(2, std::string("push"), std::string("ok"));

    D_HR_CHECK(log.size() == 1u);

    const std::vector<dt::linearization_event> ev = log.snapshot();
    D_HR_CHECK(ev.size() == 1u);
    D_HR_CHECK(ev[0].thread_id == 2u);
    D_HR_CHECK(ev[0].op_name == "push");
    D_HR_CHECK(ev[0].result == "ok");
    // an instantaneous event has coincident stamps
    D_HR_CHECK(ev[0].invoked_at == ev[0].completed_at);

    return true;
}

bool tests_rlog_begin_end_event()
{
    dt::linearization_log log;

    dt::linearization_log::event_handle h =
        log.begin_event(1, std::string("pop"), std::string("q"));

    D_HR_CHECK(log.size() == 1u);       // the event exists once begun
    D_HR_CHECK(h.index == 0u);

    log.end_event(h, std::string("42"));

    const std::vector<dt::linearization_event> ev = log.snapshot();
    D_HR_CHECK(ev.size() == 1u);
    D_HR_CHECK(ev[0].result == "42");
    // a closed interval runs forward in time
    D_HR_CHECK(ev[0].completed_at >= ev[0].invoked_at);

    return true;
}

bool tests_rlog_begin_without_end()
{
    dt::linearization_log log;

    log.begin_event(0, std::string("dangling"));

    const std::vector<dt::linearization_event> ev = log.snapshot();

    D_HR_CHECK(ev.size() == 1u);
    D_HR_CHECK(ev[0].result.empty());
    // never closed: the completion stamp is still the clock epoch, which is
    // exactly what the consistency checker flags as an interval violation
    D_HR_CHECK(ev[0].completed_at == dt::linearization_event::time_point());
    D_HR_CHECK(ev[0].completed_at < ev[0].invoked_at);

    return true;
}

bool tests_rlog_event_fields()
{
    dt::linearization_log log;

    dt::linearization_log::event_handle h =
        log.begin_event(9, std::string("cas"), std::string("old=1,new=2"));
    log.end_event(h, std::string("true"));

    const std::vector<dt::linearization_event> ev = log.snapshot();

    D_HR_CHECK(ev[0].thread_id == 9u);
    D_HR_CHECK(ev[0].op_name == "cas");
    D_HR_CHECK(ev[0].args == "old=1,new=2");
    D_HR_CHECK(ev[0].result == "true");

    return true;
}

bool tests_rlog_snapshot_is_copy()
{
    dt::linearization_log log;
    log.record_event(0, std::string("a"));

    std::vector<dt::linearization_event> first = log.snapshot();
    first[0].op_name = "mutated";

    // mutating the snapshot must not disturb the log
    const std::vector<dt::linearization_event> second = log.snapshot();
    D_HR_CHECK(second[0].op_name == "a");

    // and a later append is not visible in an older snapshot
    log.record_event(0, std::string("b"));
    D_HR_CHECK(first.size() == 1u);
    D_HR_CHECK(log.size() == 2u);

    return true;
}

bool tests_rlog_clear()
{
    dt::linearization_log log;
    hr_fill_log(log, 0, 5);
    D_HR_CHECK(log.size() == 5u);

    log.clear();

    D_HR_CHECK(log.size() == 0u);
    D_HR_CHECK(log.snapshot().empty());

    // the log is reusable after clearing
    log.record_event(1, std::string("again"));
    D_HR_CHECK(log.size() == 1u);

    return true;
}

bool tests_rlog_ordering_preserved()
{
    dt::linearization_log log;

    log.record_event(0, std::string("first"));
    log.record_event(0, std::string("second"));
    log.record_event(0, std::string("third"));

    const std::vector<dt::linearization_event> ev = log.snapshot();

    D_HR_CHECK(ev.size() == 3u);
    D_HR_CHECK(ev[0].op_name == "first");
    D_HR_CHECK(ev[1].op_name == "second");
    D_HR_CHECK(ev[2].op_name == "third");
    // stamps are non-decreasing in append order
    D_HR_CHECK(ev[0].invoked_at <= ev[1].invoked_at);
    D_HR_CHECK(ev[1].invoked_at <= ev[2].invoked_at);

    return true;
}

bool tests_rlog_concurrent_append_mt()
{
    // the log is mutex-guarded, so concurrent appends must lose nothing
    const std::size_t threads = 4;
    const std::size_t per     = 100;

    dt::linearization_log log;

    std::vector<std::thread> ts;
    for (std::size_t t = 0; t < threads; ++t)
    {
        ts.emplace_back(
            [&log, t]()
            {
                for (std::size_t i = 0; i < per; ++i)
                {
                    dt::linearization_log::event_handle h =
                        log.begin_event(t, std::string("op"));
                    log.end_event(h, std::string("ok"));
                }
            });
    }

    for (std::size_t i = 0; i < ts.size(); ++i)
    {
        ts[i].join();
    }

    D_HR_CHECK(log.size() == (threads * per));

    // every event was closed, so every interval is valid
    const std::vector<dt::linearization_event> ev = log.snapshot();
    D_HR_CHECK(ev.size() == (threads * per));

    bool all_closed = true;
    for (std::size_t i = 0; i < ev.size(); ++i)
    {
        if (ev[i].completed_at < ev[i].invoked_at)
        {
            all_closed = false;
            break;
        }
    }

    D_HR_CHECK(all_closed);

    return true;
}


NS_END  // testing
NS_END  // djinterp
