/*******************************************************************************
* djinterp [test]                         buffer_tests_buffer_base_lifecycle.cpp
*
*   Unit tests for buffer_base's lifecycle and capacity surface:
*     - reset()              - zero cursors, retain storage bytes
*     - clear()              - zero cursors AND zero storage region
*     - reserve()            - ensure capacity, dispatching to grow()
*     - move construction    - transfer cursors + storage, reset source
*     - move assignment      - same, with self-assignment guard
*
*   These tests exercise mock_buffer's grow-failure injection and
* grow-call counter to drive both happy-path and failure-path
* coverage of reserve() without relying on real allocator exhaustion.
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests_buffer_base_lifecycle.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "buffer_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_buffer_base_reset
  Covers reset() across both cursor policies.
  Tests the following:
  - reset() zeros write_position() on a write_only buffer
  - reset() zeros write_position() and read_position() on a dual
    buffer
  - reset() does NOT zero the underlying storage bytes (payload is
    retained, only cursors are cleared)
  - reset() on a freshly-constructed (idle) buffer is a no-op
  - reset() is idempotent: calling it twice yields the same state
  - reset() does not invoke the growth policy (grow_call_count is
    unchanged)
*/
void
tests_buffer_base_reset(
    test_handler& _handler
)
{
    const char src[] = "RESETPAYLOAD0123";  // 16 bytes

    // --- write_only: reset zeros write cursor, retains storage ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(64);

        b.write(src, 16);

        // sanity precondition
        record_assertion(
            _handler,
            b.write_position() == 16,
            "reset(): write_only precondition - write_position == 16");

        std::size_t grows_before = b.grow_call_count();

        b.reset();

        record_assertion(
            _handler,
            b.write_position() == 0,
            "reset(): write_only zeros write_position");

        record_assertion(
            _handler,
            b.empty(),
            "reset(): write_only buffer reports empty after reset");

        // storage bytes survive - reset is cursor-only
        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, 16) == 0,
            "reset(): write_only retains underlying storage bytes");

        // no growth invoked
        record_assertion(
            _handler,
            b.grow_call_count() == grows_before,
            "reset(): write_only does not invoke grow()");
    }

    // --- dual: reset zeros both cursors, retains storage ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 16);
        b.advance(8);  // consume half

        // sanity preconditions
        record_assertion(
            _handler,
            b.write_position() == 16,
            "reset(): dual precondition - write_position == 16");

        record_assertion(
            _handler,
            b.read_position() == 8,
            "reset(): dual precondition - read_position == 8");

        std::size_t grows_before = b.grow_call_count();

        b.reset();

        record_assertion(
            _handler,
            b.write_position() == 0,
            "reset(): dual zeros write_position");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "reset(): dual zeros read_position");

        record_assertion(
            _handler,
            b.readable() == 0,
            "reset(): dual reports zero readable bytes");

        record_assertion(
            _handler,
            b.empty(),
            "reset(): dual buffer reports empty after reset");

        // storage bytes survive
        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, 16) == 0,
            "reset(): dual retains underlying storage bytes");

        // no growth invoked
        record_assertion(
            _handler,
            b.grow_call_count() == grows_before,
            "reset(): dual does not invoke grow()");
    }

    // --- idle reset: no-op on a fresh buffer ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(32);

        b.reset();

        record_assertion(
            _handler,
            b.write_position() == 0,
            "reset(): idle buffer keeps write_position at 0");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "reset(): idle buffer keeps read_position at 0");

        record_assertion(
            _handler,
            b.capacity() == 32,
            "reset(): idle buffer preserves capacity");
    }

    // --- idempotence: reset() twice ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 16);
        b.advance(4);
        b.reset();
        b.reset();

        record_assertion(
            _handler,
            b.write_position() == 0,
            "reset(): idempotent - write_position == 0 after two calls");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "reset(): idempotent - read_position == 0 after two calls");
    }

    return;
}


/*
tests_buffer_base_clear
  Covers clear() across both cursor policies and storage states.
  Tests the following:
  - clear() zeros write_position() on a write_only buffer
  - clear() zeros write_position() and read_position() on a dual
    buffer
  - clear() zero-fills the underlying storage region (the memset
    path is taken when capacity > 0)
  - clear() on a zero-capacity buffer skips the memset and does not
    fault on a null storage pointer
  - clear() does not invoke grow() and does not change capacity
  - clear() is idempotent
*/
void
tests_buffer_base_clear(
    test_handler& _handler
)
{
    const char src[] = "CLEARPAYLOAD0123";  // 16 bytes

    // --- write_only: clear zeros cursors AND storage ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(64);

        b.write(src, 16);

        std::size_t grows_before = b.grow_call_count();
        std::size_t cap_before   = b.capacity();

        b.clear();

        record_assertion(
            _handler,
            b.write_position() == 0,
            "clear(): write_only zeros write_position");

        record_assertion(
            _handler,
            b.empty(),
            "clear(): write_only buffer reports empty after clear");

        // every byte of storage is zero
        bool        all_zero = true;
        std::size_t i;

        for (i = 0; i < b.capacity(); ++i)
        {
            // any non-zero byte fails the check
            if (b.storage()[i] != 0)
            {
                all_zero = false;
                break;
            }
        }

        record_assertion(
            _handler,
            all_zero,
            "clear(): write_only zeros every byte of storage");

        record_assertion(
            _handler,
            b.capacity() == cap_before,
            "clear(): write_only preserves capacity");

        record_assertion(
            _handler,
            b.grow_call_count() == grows_before,
            "clear(): write_only does not invoke grow()");
    }

    // --- dual: clear zeros both cursors AND storage ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 16);
        b.advance(8);

        std::size_t grows_before = b.grow_call_count();

        b.clear();

        record_assertion(
            _handler,
            b.write_position() == 0,
            "clear(): dual zeros write_position");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "clear(): dual zeros read_position");

        record_assertion(
            _handler,
            b.readable() == 0,
            "clear(): dual reports zero readable bytes");

        // every byte of storage is zero
        bool        all_zero = true;
        std::size_t i;

        for (i = 0; i < b.capacity(); ++i)
        {
            // any non-zero byte fails the check
            if (b.storage()[i] != 0)
            {
                all_zero = false;
                break;
            }
        }

        record_assertion(
            _handler,
            all_zero,
            "clear(): dual zeros every byte of storage");

        record_assertion(
            _handler,
            b.grow_call_count() == grows_before,
            "clear(): dual does not invoke grow()");
    }

    // --- zero-capacity: clear() must be safe (no memset on null) ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b;  // no allocation

        record_assertion(
            _handler,
            b.capacity() == 0,
            "clear(): zero-capacity precondition - capacity == 0");

        record_assertion(
            _handler,
            b.storage() == nullptr,
            "clear(): zero-capacity precondition - storage == nullptr");

        // must not crash; the capacity > 0 guard inside clear() skips memset
        b.clear();

        record_assertion(
            _handler,
            b.write_position() == 0,
            "clear(): zero-capacity leaves write_position at 0");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "clear(): zero-capacity leaves read_position at 0");

        record_assertion(
            _handler,
            b.capacity() == 0,
            "clear(): zero-capacity preserves zero capacity");
    }

    // --- idempotence: clear() twice ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(32);

        b.write(src, 16);
        b.clear();
        b.clear();

        record_assertion(
            _handler,
            b.write_position() == 0,
            "clear(): idempotent - write_position == 0 after two calls");

        record_assertion(
            _handler,
            b.storage()[0] == 0,
            "clear(): idempotent - first storage byte remains zero");
    }

    return;
}


/*
tests_buffer_base_reserve
  Covers reserve() across growable and non-growable buffers.
  Tests the following:
  - reserve(_n) returns true and does NOT invoke grow() when the
    requested capacity is already available
  - reserve(0) returns true on an empty buffer (already sufficient)
  - reserve() on a growable buffer dispatches to grow() and returns
    true when growth succeeds
  - reserve() on a fixed buffer (can_grow == false) returns false
    when the requested capacity exceeds the current capacity, and
    does NOT invoke grow()
  - reserve() returns false when the injected grow-failure hook
    fires (grow() returns false)
  - reserve() does not alter cursor state
*/
void
tests_buffer_base_reserve(
    test_handler& _handler
)
{
    // --- already-sufficient: no grow, returns true ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(128);

        std::size_t grows_before = b.grow_call_count();

        bool ok = b.reserve(64);

        record_assertion(
            _handler,
            ok,
            "reserve(): already-sufficient returns true");

        record_assertion(
            _handler,
            b.grow_call_count() == grows_before,
            "reserve(): already-sufficient does not invoke grow()");

        record_assertion(
            _handler,
            b.capacity() == 128,
            "reserve(): already-sufficient preserves capacity");
    }

    // --- exact-match: requesting current capacity also short-circuits ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(128);

        std::size_t grows_before = b.grow_call_count();

        bool ok = b.reserve(128);

        record_assertion(
            _handler,
            ok,
            "reserve(): exact-match returns true");

        record_assertion(
            _handler,
            b.grow_call_count() == grows_before,
            "reserve(): exact-match does not invoke grow()");
    }

    // --- reserve(0) on an empty buffer is trivially sufficient ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b;  // capacity 0

        std::size_t grows_before = b.grow_call_count();

        bool ok = b.reserve(0);

        record_assertion(
            _handler,
            ok,
            "reserve(0): empty buffer returns true");

        record_assertion(
            _handler,
            b.grow_call_count() == grows_before,
            "reserve(0): empty buffer does not invoke grow()");
    }

    // --- growable + needs growth: dispatches to grow(), succeeds ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        std::size_t grows_before = b.grow_call_count();

        bool ok = b.reserve(200);

        record_assertion(
            _handler,
            ok,
            "reserve(): growable + needs growth returns true");

        record_assertion(
            _handler,
            b.grow_call_count() == (grows_before + 1),
            "reserve(): growable + needs growth invokes grow() once");

        record_assertion(
            _handler,
            b.capacity() >= 200,
            "reserve(): post-grow capacity satisfies the request");

        // exponential policy doubles from 64 -> 128 -> 256
        record_assertion(
            _handler,
            b.capacity() == 256,
            "reserve(): exponential policy yields 256 for request of 200");
    }

    // --- growable + cursor state preserved across grow ---
    {
        const char  src[] = "ABCDEFGH";
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 8);
        b.advance(3);

        bool ok = b.reserve(512);

        record_assertion(
            _handler,
            ok,
            "reserve(): growable preserves payload - grow succeeded");

        record_assertion(
            _handler,
            b.write_position() == 8,
            "reserve(): growable preserves write_position");

        record_assertion(
            _handler,
            b.read_position() == 3,
            "reserve(): growable preserves read_position");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, 8) == 0,
            "reserve(): growable preserves payload bytes");
    }

    // --- fixed + insufficient: returns false, grow() NOT called ---
    {
        mock_buffer<fixed_growth_policy,
                    dual_cursor_policy> b(64);

        std::size_t grows_before = b.grow_call_count();

        bool ok = b.reserve(200);

        record_assertion(
            _handler,
            !ok,
            "reserve(): fixed + insufficient returns false");

        record_assertion(
            _handler,
            b.grow_call_count() == grows_before,
            "reserve(): fixed + insufficient does not invoke grow()");

        record_assertion(
            _handler,
            b.capacity() == 64,
            "reserve(): fixed + insufficient preserves capacity");
    }

    // --- fixed + already-sufficient: returns true ---
    {
        mock_buffer<fixed_growth_policy,
                    dual_cursor_policy> b(64);

        bool ok = b.reserve(64);

        record_assertion(
            _handler,
            ok,
            "reserve(): fixed + already-sufficient returns true");
    }

    // --- growable + grow failure: returns false ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.force_next_grow_failure();

        std::size_t grows_before = b.grow_call_count();

        bool ok = b.reserve(200);

        record_assertion(
            _handler,
            !ok,
            "reserve(): grow-failure injection returns false");

        record_assertion(
            _handler,
            b.grow_call_count() == (grows_before + 1),
            "reserve(): grow-failure still invokes grow() once");

        record_assertion(
            _handler,
            b.capacity() == 64,
            "reserve(): grow-failure preserves capacity");
    }

    return;
}


/*
tests_buffer_base_move_construction
  Covers the implicit base_type move constructor via mock_buffer's
  move constructor (which forwards through base_type(base_type&&)).
  Tests the following:
  - moved-to buffer receives the source's write_position
  - moved-to buffer receives the source's read_position (dual)
  - moved-to buffer receives the source's storage pointer + capacity
  - moved-to buffer can be read and yields the original payload
  - moved-from buffer has zeroed cursors
  - moved-from buffer has zero capacity (mock_buffer transfers ownership)
*/
void
tests_buffer_base_move_construction(
    test_handler& _handler
)
{
    const char src[] = "MOVECTORPAYLOAD!";  // 16 bytes

    // --- dual cursor: full state transfer ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> a(64);

        a.write(src, 16);
        a.advance(5);

        // sanity preconditions on source
        record_assertion(
            _handler,
            a.write_position() == 16,
            "move-ctor: dual source precondition - write_position == 16");

        record_assertion(
            _handler,
            a.read_position() == 5,
            "move-ctor: dual source precondition - read_position == 5");

        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(static_cast<
                        mock_buffer<exponential_growth_policy,
                                    dual_cursor_policy>&&>(a));

        // destination inherits cursor state
        record_assertion(
            _handler,
            b.write_position() == 16,
            "move-ctor: dual destination has write_position == 16");

        record_assertion(
            _handler,
            b.read_position() == 5,
            "move-ctor: dual destination has read_position == 5");

        record_assertion(
            _handler,
            b.readable() == 11,
            "move-ctor: dual destination has readable == 11");

        record_assertion(
            _handler,
            b.capacity() == 64,
            "move-ctor: dual destination has capacity == 64");

        // payload survives - read back the unconsumed tail
        char        out[16];
        std::size_t n = b.read(out, 16);

        record_assertion(
            _handler,
            n == 11,
            "move-ctor: dual destination read returns 11 unconsumed bytes");

        record_assertion(
            _handler,
            std::memcmp(out, src + 5, 11) == 0,
            "move-ctor: dual destination payload matches source tail");

        // source is left in a resourceless state
        record_assertion(
            _handler,
            a.write_position() == 0,
            "move-ctor: dual source has write_position reset to 0");

        record_assertion(
            _handler,
            a.read_position() == 0,
            "move-ctor: dual source has read_position reset to 0");

        record_assertion(
            _handler,
            a.capacity() == 0,
            "move-ctor: dual source has capacity reset to 0");

        record_assertion(
            _handler,
            a.storage() == nullptr,
            "move-ctor: dual source has storage reset to nullptr");
    }

    // --- write_only cursor: single-cursor transfer ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> a(32);

        a.write(src, 12);

        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(static_cast<
                        mock_buffer<exponential_growth_policy,
                                    write_only_cursor_policy>&&>(a));

        record_assertion(
            _handler,
            b.write_position() == 12,
            "move-ctor: write_only destination has write_position == 12");

        record_assertion(
            _handler,
            b.capacity() == 32,
            "move-ctor: write_only destination has capacity == 32");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, 12) == 0,
            "move-ctor: write_only destination payload matches source");

        record_assertion(
            _handler,
            a.write_position() == 0,
            "move-ctor: write_only source has write_position reset");

        record_assertion(
            _handler,
            a.capacity() == 0,
            "move-ctor: write_only source has capacity reset");
    }

    // --- moving an idle buffer: both ends end up empty ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> a;  // empty, zero capacity

        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(static_cast<
                        mock_buffer<exponential_growth_policy,
                                    dual_cursor_policy>&&>(a));

        record_assertion(
            _handler,
            b.write_position() == 0,
            "move-ctor: idle source - destination write_position == 0");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "move-ctor: idle source - destination read_position == 0");

        record_assertion(
            _handler,
            b.capacity() == 0,
            "move-ctor: idle source - destination capacity == 0");

        record_assertion(
            _handler,
            a.write_position() == 0,
            "move-ctor: idle source - source write_position == 0");
    }

    return;
}


/*
tests_buffer_base_move_assignment
  Covers move assignment on buffer_base via mock_buffer's
  forwarding move-assignment operator.
  Tests the following:
  - destination receives source's cursor state (both cursors)
  - destination receives source's storage and capacity
  - destination's prior storage is released (no leak; we can keep
    using the buffer afterwards)
  - source is left with zeroed cursors and zero capacity
  - self-move assignment is a no-op: state is preserved
  - reading from the destination yields the original payload
*/
void
tests_buffer_base_move_assignment(
    test_handler& _handler
)
{
    const char src[] = "MOVEASSIGNPAYLD!";  // 16 bytes

    // --- normal move-assign: a -> b, where b has prior storage ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> a(64);

        a.write(src, 16);
        a.advance(4);

        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(32);  // distinct prior storage

        // write into b so its cursors are nonzero before assignment
        char filler[8] = {'X','X','X','X','X','X','X','X'};
        b.write(filler, 8);

        record_assertion(
            _handler,
            b.write_position() == 8,
            "move-assign: destination precondition - write_position == 8");

        b = static_cast<
                mock_buffer<exponential_growth_policy,
                            dual_cursor_policy>&&>(a);

        // destination now mirrors source's prior state
        record_assertion(
            _handler,
            b.write_position() == 16,
            "move-assign: destination receives write_position == 16");

        record_assertion(
            _handler,
            b.read_position() == 4,
            "move-assign: destination receives read_position == 4");

        record_assertion(
            _handler,
            b.readable() == 12,
            "move-assign: destination receives readable == 12");

        record_assertion(
            _handler,
            b.capacity() == 64,
            "move-assign: destination receives capacity == 64");

        // payload survives
        char        out[16];
        std::size_t n = b.read(out, 16);

        record_assertion(
            _handler,
            n == 12,
            "move-assign: destination yields 12 unconsumed bytes");

        record_assertion(
            _handler,
            std::memcmp(out, src + 4, 12) == 0,
            "move-assign: destination payload matches source tail");

        // source is resourceless
        record_assertion(
            _handler,
            a.write_position() == 0,
            "move-assign: source write_position reset to 0");

        record_assertion(
            _handler,
            a.read_position() == 0,
            "move-assign: source read_position reset to 0");

        record_assertion(
            _handler,
            a.capacity() == 0,
            "move-assign: source capacity reset to 0");

        record_assertion(
            _handler,
            a.storage() == nullptr,
            "move-assign: source storage reset to nullptr");
    }

    // --- self-move-assignment guard: no-op, state preserved ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 16);
        b.advance(3);

        // capture pointer identity for the post-condition
        const char* storage_before = b.storage();

        // Hide the self-move from the compiler via a reference so the
        // self-assignment guard inside operator= actually fires at
        // runtime. Writing `b = std::move(b)` triggers a warning.
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy>& alias = b;

        b = static_cast<
                mock_buffer<exponential_growth_policy,
                            dual_cursor_policy>&&>(alias);

        record_assertion(
            _handler,
            b.write_position() == 16,
            "move-assign: self-move preserves write_position");

        record_assertion(
            _handler,
            b.read_position() == 3,
            "move-assign: self-move preserves read_position");

        record_assertion(
            _handler,
            b.capacity() == 64,
            "move-assign: self-move preserves capacity");

        record_assertion(
            _handler,
            b.storage() == storage_before,
            "move-assign: self-move preserves storage pointer identity");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, 16) == 0,
            "move-assign: self-move preserves payload bytes");
    }

    // --- write_only flavor: single-cursor transfer ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> a(32);

        a.write(src, 10);

        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b;  // empty

        b = static_cast<
                mock_buffer<exponential_growth_policy,
                            write_only_cursor_policy>&&>(a);

        record_assertion(
            _handler,
            b.write_position() == 10,
            "move-assign: write_only destination has write_position == 10");

        record_assertion(
            _handler,
            b.capacity() == 32,
            "move-assign: write_only destination has capacity == 32");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, 10) == 0,
            "move-assign: write_only destination payload matches source");

        record_assertion(
            _handler,
            a.write_position() == 0,
            "move-assign: write_only source write_position reset");

        record_assertion(
            _handler,
            a.capacity() == 0,
            "move-assign: write_only source capacity reset");
    }

    return;
}


NS_END  // test
NS_END  // djinterp
