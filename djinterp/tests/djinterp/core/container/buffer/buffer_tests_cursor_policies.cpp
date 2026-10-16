/*******************************************************************************
* djinterp [test]                               buffer_tests_cursor_policies.cpp
*
*   Unit tests for write_only_cursor_policy and dual_cursor_policy
* (section IV of buffer.hpp).  Each policy is exercised directly by
* operating on its static helpers and cursor_state struct, without going
* through buffer_base - so any regression in the policy itself is caught
* before higher-level integration tests run.
*
*   compact() is the only non-constexpr policy method and is covered
* separately for the four functionally distinct shapes: empty, fully
* consumed, partially consumed, and entirely unread.
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests_cursor_policies.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "buffer_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_write_only_cursor_policy
  Covers the write_only_cursor_policy struct.
  Tests the following:
  - model constant equals buffer_cursor_model::write_only
  - has_read_cursor constant is false
  - cursor_state has a single write_pos member
  - init() returns {0}
  - writable() returns capacity - write_pos
  - advance_write() increments write_pos
  - written() returns write_pos
  - reset() zeros write_pos
  - sequential write/advance/reset transitions preserve the
    expected invariants
*/
void
tests_write_only_cursor_policy(
    test_handler& _handler
)
{
    using policy = write_only_cursor_policy;
    using state  = policy::cursor_state;

    // --- constants ---

    static_assert(
        policy::model == buffer_cursor_model::write_only,
        "write_only::model must be ::write_only.");
    static_assert(
        policy::has_read_cursor == false,
        "write_only::has_read_cursor must be false.");

    record_assertion(
        _handler,
        policy::model == buffer_cursor_model::write_only,
        "write_only: model == ::write_only");

    record_assertion(
        _handler,
        policy::has_read_cursor == false,
        "write_only: has_read_cursor == false");

    // --- init() ---

    state s = policy::init();

    record_assertion(
        _handler,
        s.write_pos == 0,
        "write_only: init() produces write_pos == 0");

    record_assertion(
        _handler,
        policy::written(s) == 0,
        "write_only: written(init()) == 0");

    // --- writable() ---

    record_assertion(
        _handler,
        policy::writable(s, 100) == 100,
        "write_only: writable({0}, 100) == 100");

    record_assertion(
        _handler,
        policy::writable(s, 0) == 0,
        "write_only: writable({0}, 0) == 0");

    // --- advance_write() ---

    policy::advance_write(s, 10);

    record_assertion(
        _handler,
        s.write_pos == 10,
        "write_only: advance_write(10) sets write_pos to 10");

    record_assertion(
        _handler,
        policy::written(s) == 10,
        "write_only: written(s) == 10 after advance");

    record_assertion(
        _handler,
        policy::writable(s, 100) == 90,
        "write_only: writable({10}, 100) == 90");

    // advance by zero is a no-op
    policy::advance_write(s, 0);

    record_assertion(
        _handler,
        s.write_pos == 10,
        "write_only: advance_write(0) leaves write_pos unchanged");

    // advance again to confirm accumulation
    policy::advance_write(s, 5);

    record_assertion(
        _handler,
        policy::written(s) == 15,
        "write_only: advance_write(5) accumulates to 15");

    record_assertion(
        _handler,
        policy::writable(s, 100) == 85,
        "write_only: writable after second advance == 85");

    // --- reset() ---

    policy::reset(s);

    record_assertion(
        _handler,
        s.write_pos == 0,
        "write_only: reset() zeros write_pos");

    record_assertion(
        _handler,
        policy::written(s) == 0,
        "write_only: written(s) == 0 after reset");

    record_assertion(
        _handler,
        policy::writable(s, 100) == 100,
        "write_only: writable() restored to full capacity after reset");

    return;
}


/*
tests_dual_cursor_policy
  Covers the dual_cursor_policy struct.
  Tests the following:
  - model constant equals buffer_cursor_model::dual
  - has_read_cursor constant is true
  - cursor_state has both write_pos and read_pos members
  - init() returns {0, 0}
  - writable() / readable() compute the right deltas
  - advance_write() / advance_read() move the cursors
  - written() / consumed() expose the raw positions
  - reset() zeros both cursors
  - compact() with four distinct shapes:
      - empty buffer (no data and no consumption)
      - fully consumed (write_pos == read_pos > 0)
      - partially consumed (memmove path)
      - no consumption (read_pos == 0, early-exit path)
*/
void
tests_dual_cursor_policy(
    test_handler& _handler
)
{
    using policy = dual_cursor_policy;
    using state  = policy::cursor_state;

    // --- constants ---

    static_assert(
        policy::model == buffer_cursor_model::dual,
        "dual::model must be ::dual.");
    static_assert(
        policy::has_read_cursor == true,
        "dual::has_read_cursor must be true.");

    record_assertion(
        _handler,
        policy::model == buffer_cursor_model::dual,
        "dual: model == ::dual");

    record_assertion(
        _handler,
        policy::has_read_cursor == true,
        "dual: has_read_cursor == true");

    // --- init() ---

    state s = policy::init();

    record_assertion(
        _handler,
        s.write_pos == 0,
        "dual: init() produces write_pos == 0");

    record_assertion(
        _handler,
        s.read_pos == 0,
        "dual: init() produces read_pos == 0");

    record_assertion(
        _handler,
        policy::written(s) == 0,
        "dual: written(init()) == 0");

    record_assertion(
        _handler,
        policy::consumed(s) == 0,
        "dual: consumed(init()) == 0");

    record_assertion(
        _handler,
        policy::readable(s) == 0,
        "dual: readable(init()) == 0");

    // --- writable() / readable() with initial state ---

    record_assertion(
        _handler,
        policy::writable(s, 100) == 100,
        "dual: writable({0,0}, 100) == 100");

    record_assertion(
        _handler,
        policy::writable(s, 0) == 0,
        "dual: writable({0,0}, 0) == 0");

    // --- advance_write() ---

    policy::advance_write(s, 20);

    record_assertion(
        _handler,
        s.write_pos == 20,
        "dual: advance_write(20) sets write_pos to 20");

    record_assertion(
        _handler,
        policy::written(s) == 20,
        "dual: written(s) == 20");

    record_assertion(
        _handler,
        policy::readable(s) == 20,
        "dual: readable(s) == 20  (write_pos - read_pos)");

    record_assertion(
        _handler,
        policy::writable(s, 100) == 80,
        "dual: writable(s, 100) == 80");

    // --- advance_read() ---

    policy::advance_read(s, 5);

    record_assertion(
        _handler,
        s.read_pos == 5,
        "dual: advance_read(5) sets read_pos to 5");

    record_assertion(
        _handler,
        policy::consumed(s) == 5,
        "dual: consumed(s) == 5");

    record_assertion(
        _handler,
        policy::readable(s) == 15,
        "dual: readable(s) == 15  (20 - 5)");

    record_assertion(
        _handler,
        policy::writable(s, 100) == 80,
        "dual: writable unchanged by advance_read");

    // advance_read by zero is a no-op
    policy::advance_read(s, 0);

    record_assertion(
        _handler,
        (s.read_pos == 5) && (s.write_pos == 20),
        "dual: advance_read(0) is a no-op");

    // --- reset() ---

    policy::reset(s);

    record_assertion(
        _handler,
        (s.write_pos == 0) && (s.read_pos == 0),
        "dual: reset() zeros both cursors");

    record_assertion(
        _handler,
        policy::readable(s) == 0,
        "dual: readable(s) == 0 after reset");

    // -----------------------------------------------------------
    //  compact() coverage
    // -----------------------------------------------------------

    // --- compact() shape 1: empty buffer (no data, no consumption) ---
    {
        char  data[16];
        state e = policy::init();

        std::memset(data, 0xAB, sizeof(data));

        std::size_t pending = policy::compact(e, data);

        record_assertion(
            _handler,
            pending == 0,
            "dual: compact(empty) returns 0");

        record_assertion(
            _handler,
            (e.read_pos == 0) && (e.write_pos == 0),
            "dual: compact(empty) leaves cursors zeroed");

        record_assertion(
            _handler,
            // payload must be untouched: no memmove triggered
            (static_cast<unsigned char>(data[0]) == 0xAB),
            "dual: compact(empty) does not touch the payload");
    }

    // --- compact() shape 2: fully consumed (read_pos == write_pos > 0) ---
    {
        char  data[16];
        state f = { 8, 8 };

        std::memcpy(data, "ABCDEFGH12345678", 16);

        std::size_t pending = policy::compact(f, data);

        record_assertion(
            _handler,
            pending == 0,
            "dual: compact(all-consumed) returns 0");

        record_assertion(
            _handler,
            (f.read_pos == 0) && (f.write_pos == 0),
            "dual: compact(all-consumed) zeros both cursors");

        // no memmove because pending == 0; payload unchanged
        record_assertion(
            _handler,
            std::memcmp(data, "ABCDEFGH12345678", 16) == 0,
            "dual: compact(all-consumed) does not touch payload "
            "(memmove skipped)");
    }

    // --- compact() shape 3: partial consumption (memmove path) ---
    {
        char  data[16];
        state p = { 8, 3 };  // wrote 8 bytes; consumed first 3

        std::memcpy(data, "ABCDEFGH12345678", 16);

        std::size_t pending = policy::compact(p, data);

        record_assertion(
            _handler,
            pending == 5,
            "dual: compact(partial) returns 5 pending bytes");

        record_assertion(
            _handler,
            (p.read_pos == 0) && (p.write_pos == 5),
            "dual: compact(partial) resets read_pos, sets "
            "write_pos to pending");

        // Bytes 3..7 ("DEFGH") should have been shifted to the
        // front; the tail content at positions 5..15 is permitted
        // by spec to be anything, so we only check the head.
        record_assertion(
            _handler,
            std::memcmp(data, "DEFGH", 5) == 0,
            "dual: compact(partial) shifts unconsumed bytes to "
            "the front (memmove)");
    }

    // --- compact() shape 4: no consumption (read_pos == 0, but data present) ---
    {
        char  data[16];
        state u = { 6, 0 };  // wrote 6 bytes; nothing consumed

        std::memcpy(data, "QWERTY1234567890", 16);

        std::size_t pending = policy::compact(u, data);

        record_assertion(
            _handler,
            pending == 6,
            "dual: compact(no-consumption) returns 6 pending bytes");

        record_assertion(
            _handler,
            (u.read_pos == 0) && (u.write_pos == 6),
            "dual: compact(no-consumption) preserves cursor "
            "positions (early-exit path)");

        // memmove is skipped because read_pos is already zero;
        // payload must be byte-for-byte unchanged.
        record_assertion(
            _handler,
            std::memcmp(data, "QWERTY1234567890", 16) == 0,
            "dual: compact(no-consumption) does not touch payload "
            "(memmove skipped)");
    }

    return;
}


NS_END  // test
NS_END  // djinterp
