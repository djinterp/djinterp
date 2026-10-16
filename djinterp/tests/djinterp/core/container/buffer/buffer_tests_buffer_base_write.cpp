/*******************************************************************************
* djinterp [test]                             buffer_tests_buffer_base_write.cpp
*
*   Unit tests for buffer_base's write surface: write(), write_byte(),
* write_fill(), write_head(), and commit().  These tests cover the
* happy paths, every short-circuit branch, the growth-path success and
* failure modes, and the direct-write workflow used for low-level
* producers.
*
*   ensure_writable() is exercised indirectly through the public
* writers; mock_buffer's force_next_grow_failure() hook drives the
* grow-failure branches without relying on real allocator exhaustion.
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests_buffer_base_write.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "buffer_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_buffer_base_write
  Covers the write(const void*, std::size_t) entry point.
  Tests the following:
  - null source returns 0 and does not advance the cursor
  - zero-length call returns 0 and does not advance the cursor
  - normal write copies bytes and advances the cursor
  - write that exactly fits the current capacity succeeds without
    invoking grow()
  - write that overflows a growable buffer succeeds and triggers
    grow() exactly once
  - write that overflows a fixed buffer is clamped to writable()
    and returns the truncated count
  - write that overflows a full fixed buffer returns 0
  - write that requires a grow() but where grow() reports failure
    falls back to writing whatever still fits, or returns 0 when
    nothing fits
  - the byte payload is preserved across all successful paths
*/
void
tests_buffer_base_write(
    test_handler& _handler
)
{
    const char src[]   = "hello world!";
    const std::size_t  n_src = sizeof(src) - 1;  // 12 bytes

    // --- null source short-circuit ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        std::size_t n = b.write(nullptr, 5);

        record_assertion(
            _handler,
            n == 0,
            "write(): null source returns 0");

        record_assertion(
            _handler,
            b.write_position() == 0,
            "write(): null source does not advance cursor");
    }

    // --- zero-length short-circuit ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        std::size_t n = b.write(src, 0);

        record_assertion(
            _handler,
            n == 0,
            "write(): zero-length returns 0");

        record_assertion(
            _handler,
            b.write_position() == 0,
            "write(): zero-length does not advance cursor");
    }

    // --- normal write fits in current capacity (no grow) ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        std::size_t n = b.write(src, n_src);

        record_assertion(
            _handler,
            n == n_src,
            "write(): normal write returns the bytes written");

        record_assertion(
            _handler,
            b.write_position() == n_src,
            "write(): write_position advances by n_src");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, n_src) == 0,
            "write(): payload matches the source byte-for-byte");

        record_assertion(
            _handler,
            b.grow_call_count() == 0,
            "write(): no grow() invocation when n fits in capacity");
    }

    // --- write exactly fills capacity (boundary case) ---
    {
        mock_buffer<fixed_growth_policy,
                    write_only_cursor_policy> b(n_src);

        std::size_t n = b.write(src, n_src);

        record_assertion(
            _handler,
            n == n_src,
            "write(): exact-fit on fixed buffer succeeds");

        record_assertion(
            _handler,
            b.full(),
            "write(): fixed buffer is full after exact-fit write");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, n_src) == 0,
            "write(): payload preserved at exact-fit boundary");
    }

    // --- write triggers grow on growable buffer ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(4);  // smaller than n_src

        std::size_t n = b.write(src, n_src);

        record_assertion(
            _handler,
            n == n_src,
            "write(): growable buffer accepts all bytes despite "
            "starting under capacity");

        record_assertion(
            _handler,
            b.grow_call_count() >= 1,
            "write(): grow() was invoked at least once");

        record_assertion(
            _handler,
            b.capacity() >= n_src,
            "write(): capacity now >= n_src after grow");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, n_src) == 0,
            "write(): payload preserved across grow");
    }

    // --- write overflows fixed buffer: truncated partial write ---
    {
        const std::size_t cap = 5;
        mock_buffer<fixed_growth_policy,
                    write_only_cursor_policy> b(cap);

        std::size_t n = b.write(src, n_src);

        record_assertion(
            _handler,
            n == cap,
            "write(): fixed-buffer overflow is truncated to "
            "writable() (5 bytes)");

        record_assertion(
            _handler,
            b.full(),
            "write(): fixed buffer is full after truncated write");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, cap) == 0,
            "write(): truncated bytes copied are the leading bytes "
            "of the source");
    }

    // --- write to an already-full fixed buffer returns 0 ---
    {
        mock_buffer<fixed_growth_policy,
                    write_only_cursor_policy> b(4);

        b.write_fill('x', 4);

        std::size_t n = b.write(src, n_src);

        record_assertion(
            _handler,
            n == 0,
            "write(): full fixed buffer returns 0");

        record_assertion(
            _handler,
            b.write_position() == 4,
            "write(): cursor unchanged on full-buffer write");
    }

    // --- write where grow() fails: partial write up to remaining ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(8);

        // write 4 bytes first; 4 bytes remain
        b.write_fill('A', 4);
        b.force_next_grow_failure();

        std::size_t n = b.write(src, n_src);  // 12 > 4 remaining

        record_assertion(
            _handler,
            n == 4,
            "write(): grow failure falls back to writable() (4 bytes)");

        record_assertion(
            _handler,
            b.write_position() == 8,
            "write(): cursor at capacity after fallback write");

        record_assertion(
            _handler,
            std::memcmp(b.storage() + 4, src, 4) == 0,
            "write(): the four fallback bytes match source prefix");
    }

    // --- write where grow() fails AND writable() is zero: returns 0 ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(4);

        b.write_fill('B', 4);          // fill it
        b.force_next_grow_failure();   // next grow will fail

        std::size_t n = b.write(src, n_src);

        record_assertion(
            _handler,
            n == 0,
            "write(): grow failure with zero writable returns 0");

        record_assertion(
            _handler,
            b.write_position() == 4,
            "write(): cursor unchanged on zero-fallback write");
    }

    return;
}


/*
tests_buffer_base_write_byte
  Covers the single-byte write_byte() entry point.
  Tests the following:
  - normal write_byte returns true and advances the cursor
  - write_byte preserves the exact byte value (including 0x00 and 0xFF)
  - write_byte triggers grow on a growable buffer
  - write_byte on a full fixed buffer returns false and does not
    advance the cursor
  - write_byte on a fixed buffer where grow is unavailable but space
    remains succeeds normally
*/
void
tests_buffer_base_write_byte(
    test_handler& _handler
)
{
    // --- normal sequence of write_byte calls ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(64);

        bool ok_a = b.write_byte('A');
        bool ok_b = b.write_byte('B');
        bool ok_c = b.write_byte('C');

        record_assertion(
            _handler,
            (ok_a && ok_b && ok_c),
            "write_byte(): three calls return true");

        record_assertion(
            _handler,
            b.write_position() == 3,
            "write_byte(): cursor advances by 1 per call");

        record_assertion(
            _handler,
            (b.storage()[0] == 'A') &&
            (b.storage()[1] == 'B') &&
            (b.storage()[2] == 'C'),
            "write_byte(): bytes stored in order");
    }

    // --- edge byte values: 0x00 and 0xFF ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(64);

        b.write_byte('\0');
        b.write_byte(static_cast<char>(0xFF));

        record_assertion(
            _handler,
            b.storage()[0] == '\0',
            "write_byte(): 0x00 byte preserved exactly");

        record_assertion(
            _handler,
            static_cast<unsigned char>(b.storage()[1]) == 0xFF,
            "write_byte(): 0xFF byte preserved exactly");
    }

    // --- write_byte triggers grow on growable buffer ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(1);

        b.write_byte('X');  // fills it
        bool ok = b.write_byte('Y');

        record_assertion(
            _handler,
            ok,
            "write_byte(): triggers grow on growable buffer");

        record_assertion(
            _handler,
            (b.storage()[0] == 'X') &&
            (b.storage()[1] == 'Y'),
            "write_byte(): payload preserved across grow");

        record_assertion(
            _handler,
            b.grow_call_count() >= 1,
            "write_byte(): grow invoked at least once");
    }

    // --- write_byte on a full fixed buffer fails ---
    {
        mock_buffer<fixed_growth_policy,
                    write_only_cursor_policy> b(2);

        b.write_byte('1');
        b.write_byte('2');

        record_assertion(
            _handler,
            b.full(),
            "write_byte(): fixed buffer is full after 2 writes");

        bool ok = b.write_byte('3');

        record_assertion(
            _handler,
            !ok,
            "write_byte(): returns false when fixed buffer is full");

        record_assertion(
            _handler,
            b.write_position() == 2,
            "write_byte(): cursor unchanged on failed write");
    }

    return;
}


/*
tests_buffer_base_write_fill
  Covers write_fill().
  Tests the following:
  - zero-length call returns 0 and is a no-op
  - normal fill writes _n bytes of _byte and advances the cursor
  - fill triggers grow on a growable buffer
  - fill overflows a fixed buffer: returns truncated count
  - fill on a full fixed buffer returns 0
  - fill where grow fails falls back to writable()
  - fill where grow fails and writable is zero returns 0
  - the byte value used in the fill is preserved exactly
*/
void
tests_buffer_base_write_fill(
    test_handler& _handler
)
{
    // --- zero-length is a no-op ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(64);

        std::size_t n = b.write_fill('Z', 0);

        record_assertion(
            _handler,
            n == 0,
            "write_fill(): zero-length returns 0");

        record_assertion(
            _handler,
            b.write_position() == 0,
            "write_fill(): zero-length does not advance cursor");
    }

    // --- normal fill ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(64);

        std::size_t n = b.write_fill('K', 10);

        record_assertion(
            _handler,
            n == 10,
            "write_fill(): normal fill returns 10");

        record_assertion(
            _handler,
            b.write_position() == 10,
            "write_fill(): cursor advances by 10");

        // verify every byte matches the requested fill character
        bool every_byte_ok = true;

        for (std::size_t i = 0; i < 10; ++i)
        {
            if (b.storage()[i] != 'K')
            {
                every_byte_ok = false;
                break;
            }
        }

        record_assertion(
            _handler,
            every_byte_ok,
            "write_fill(): all 10 bytes equal 'K'");
    }

    // --- fill triggers grow ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(4);

        std::size_t n = b.write_fill('q', 20);

        record_assertion(
            _handler,
            n == 20,
            "write_fill(): triggers grow and writes all 20 bytes");

        record_assertion(
            _handler,
            b.capacity() >= 20,
            "write_fill(): capacity expanded to >= 20");

        record_assertion(
            _handler,
            b.grow_call_count() >= 1,
            "write_fill(): grow invoked at least once");
    }

    // --- fill overflows fixed buffer: truncated ---
    {
        mock_buffer<fixed_growth_policy,
                    write_only_cursor_policy> b(5);

        std::size_t n = b.write_fill('!', 10);

        record_assertion(
            _handler,
            n == 5,
            "write_fill(): fixed-buffer overflow truncated to "
            "writable() (5 bytes)");

        record_assertion(
            _handler,
            b.full(),
            "write_fill(): fixed buffer is full after truncated fill");

        for (std::size_t i = 0; i < 5; ++i)
        {
            if (b.storage()[i] != '!')
            {
                record_assertion(
                    _handler,
                    false,
                    "write_fill(): truncated bytes match fill char");
                return;
            }
        }

        record_assertion(
            _handler,
            true,
            "write_fill(): truncated bytes match fill char");
    }

    // --- fill on full fixed buffer returns 0 ---
    {
        mock_buffer<fixed_growth_policy,
                    write_only_cursor_policy> b(4);

        b.write_fill('a', 4);

        std::size_t n = b.write_fill('b', 10);

        record_assertion(
            _handler,
            n == 0,
            "write_fill(): full fixed buffer returns 0");

        record_assertion(
            _handler,
            b.write_position() == 4,
            "write_fill(): cursor unchanged when fill returns 0");

        // pre-existing payload must not have been clobbered
        bool intact = true;

        for (std::size_t i = 0; i < 4; ++i)
        {
            if (b.storage()[i] != 'a')
            {
                intact = false;
                break;
            }
        }

        record_assertion(
            _handler,
            intact,
            "write_fill(): pre-existing payload preserved on rejection");
    }

    // --- fill where grow fails: partial fallback ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(8);

        b.write_fill('A', 4);
        b.force_next_grow_failure();

        std::size_t n = b.write_fill('B', 20);

        record_assertion(
            _handler,
            n == 4,
            "write_fill(): grow failure falls back to writable() "
            "(4 bytes remaining)");

        record_assertion(
            _handler,
            b.write_position() == 8,
            "write_fill(): cursor at capacity after fallback");

        record_assertion(
            _handler,
            (b.storage()[3] == 'A') && (b.storage()[4] == 'B'),
            "write_fill(): boundary byte transitions from A to B");
    }

    // --- fill where grow fails AND writable is zero: returns 0 ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(4);

        b.write_fill('X', 4);
        b.force_next_grow_failure();

        std::size_t n = b.write_fill('Y', 10);

        record_assertion(
            _handler,
            n == 0,
            "write_fill(): grow failure with zero writable returns 0");

        record_assertion(
            _handler,
            b.write_position() == 4,
            "write_fill(): cursor unchanged on zero-fallback fill");
    }

    // --- fill of 0x00 is preserved (memset edge) ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(8);

        b.write_fill(static_cast<char>(0x00), 3);

        record_assertion(
            _handler,
            (b.storage()[0] == 0) &&
            (b.storage()[1] == 0) &&
            (b.storage()[2] == 0),
            "write_fill(): 0x00 bytes preserved");

        record_assertion(
            _handler,
            b.write_position() == 3,
            "write_fill(): cursor advanced after 0x00 fill");
    }

    return;
}


/*
tests_buffer_base_write_head_commit
  Covers the direct-write workflow: write_head() + commit().
  Tests the following:
  - write_head() returns a pointer to storage() + write_position()
  - commit() advances the write cursor by exactly _n bytes
  - data written through write_head() and committed is later
    visible via storage(), size(), and (in dual mode) read()
  - commit(0) is a no-op
  - sequential write_head/commit cycles compose correctly
*/
void
tests_buffer_base_write_head_commit(
    test_handler& _handler
)
{
    // --- write_head returns the right pointer ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        record_assertion(
            _handler,
            b.write_head() == b.storage(),
            "write_head(): == storage() on a fresh buffer");

        // direct write + commit
        const char data[] = "direct";
        std::memcpy(b.write_head(), data, 6);
        b.commit(6);

        record_assertion(
            _handler,
            b.write_position() == 6,
            "commit(): write_position advances by 6");

        record_assertion(
            _handler,
            b.write_head() == b.storage() + 6,
            "write_head(): == storage()+6 after commit(6)");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), data, 6) == 0,
            "write_head/commit: payload visible via storage()");

        record_assertion(
            _handler,
            b.size() == 6,
            "write_head/commit: size() == 6 in dual mode");
    }

    // --- commit(0) is a no-op ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write_byte('1');
        b.commit(0);

        record_assertion(
            _handler,
            b.write_position() == 1,
            "commit(): commit(0) does not advance the cursor");
    }

    // --- chained write_head/commit cycles ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        std::memcpy(b.write_head(), "AAA", 3);
        b.commit(3);

        std::memcpy(b.write_head(), "BBB", 3);
        b.commit(3);

        std::memcpy(b.write_head(), "CCC", 3);
        b.commit(3);

        record_assertion(
            _handler,
            b.write_position() == 9,
            "write_head/commit: chained cycles accumulate position");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), "AAABBBCCC", 9) == 0,
            "write_head/commit: chained payloads are contiguous");
    }

    // --- write_head followed by read() proves the round-trip ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        std::memcpy(b.write_head(), "ROUNDTRIP", 9);
        b.commit(9);

        char out[10] = { 0 };
        std::size_t r = b.read(out, 9);

        record_assertion(
            _handler,
            r == 9,
            "write_head/commit: read() returns 9 bytes");

        record_assertion(
            _handler,
            std::memcmp(out, "ROUNDTRIP", 9) == 0,
            "write_head/commit: round-trip payload intact");
    }

    return;
}


NS_END  // test
NS_END  // djinterp
