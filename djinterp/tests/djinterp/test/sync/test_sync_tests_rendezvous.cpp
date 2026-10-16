/*******************************************************************************
* djinterp [tests]                                test_sync_tests_rendezvous.cpp
*
*   Section TU: test_rendezvous<T> - two-thread meeting point with payload
* exchange.
*
*   NOTE: send() blocks until a receiver consumes the value, so every send in
* this section is paired with a receiver on another thread.  Only the
* empty-state try_receive probes are safe single-threaded.
*
*
* path:      /tests/djinterp/test/sync/test_sync_tests_rendezvous.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
// djinterp
#include "test_sync_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_rendezvous_try_receive_empty()
{
    dt::test_rendezvous<int> rv;

    int out = -7;
    const bool got = rv.try_receive(&out);

    D_HY_CHECK(!got);
    D_HY_CHECK(out == -7);   // out slot left untouched

    return true;
}

bool tests_rendezvous_try_receive_null_empty()
{
    dt::test_rendezvous<int> rv;

    // an empty rendezvous returns false before touching the out pointer
    D_HY_CHECK(!rv.try_receive(nullptr));

    return true;
}

bool tests_rendezvous_send_receive_mt()
{
    dt::test_rendezvous<int> rv;

    std::thread sender(
        [&rv]()
        {
            rv.send(42);
        });

    const int value = rv.receive();
    sender.join();

    D_HY_CHECK(value == 42);

    // the slot is empty again once consumed
    int out = 0;
    D_HY_CHECK(!rv.try_receive(&out));

    return true;
}

bool tests_rendezvous_receive_first_mt()
{
    dt::test_rendezvous<int> rv;
    std::atomic<bool> got(false);
    std::atomic<int>  value(0);

    std::thread receiver(
        [&rv, &got, &value]()
        {
            value.store(rv.receive(), std::memory_order_release);
            got.store(true, std::memory_order_release);
        });

    // the receiver arrived first, so it must still be blocked
    std::this_thread::sleep_for(hy_short());
    D_HY_CHECK(!got.load(std::memory_order_acquire));

    rv.send(99);
    receiver.join();

    D_HY_CHECK(got.load(std::memory_order_acquire));
    D_HY_CHECK(value.load(std::memory_order_acquire) == 99);

    return true;
}

bool tests_rendezvous_try_receive_after_send_mt()
{
    dt::test_rendezvous<int> rv;

    std::thread sender(
        [&rv]()
        {
            rv.send(7);   // blocks until consumed
        });

    // spin until the sender has published its value
    int  out    = 0;
    bool got    = false;
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + hy_settle();

    while (!got && (std::chrono::steady_clock::now() < deadline))
    {
        got = rv.try_receive(&out);
        if (!got)
        {
            std::this_thread::yield();
        }
    }

    sender.join();

    D_HY_CHECK(got);
    D_HY_CHECK(out == 7);

    return true;
}

bool tests_rendezvous_payload_string_mt()
{
    dt::test_rendezvous<std::string> rv;

    std::thread sender(
        [&rv]()
        {
            rv.send(std::string("payload"));
        });

    const std::string value = rv.receive();
    sender.join();

    D_HY_CHECK(value == "payload");

    return true;
}

bool tests_rendezvous_multiple_exchanges_mt()
{
    dt::test_rendezvous<int> rv;

    std::thread sender(
        [&rv]()
        {
            for (int i = 1; i <= 3; ++i)
            {
                rv.send(i);
            }
        });

    const int a = rv.receive();
    const int b = rv.receive();
    const int c = rv.receive();

    sender.join();

    // successive sends are consumed in order
    D_HY_CHECK(a == 1);
    D_HY_CHECK(b == 2);
    D_HY_CHECK(c == 3);

    return true;
}


NS_END  // testing
NS_END  // djinterp
