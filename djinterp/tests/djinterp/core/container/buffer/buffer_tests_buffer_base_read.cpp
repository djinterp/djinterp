/*******************************************************************************
* djinterp [test]                              buffer_tests_buffer_base_read.cpp
*
*   Unit tests for buffer_base's read surface, which is only available
* when the cursor policy exposes a read cursor (dual_cursor_policy).
* Each test in this translation unit operates on a buffer instantiated
* with dual_cursor_policy.
*
*   Operations covered:
*     - read()           - destructive copy-out
*     - peek()           - non-destructive pointer at read cursor
*     - read_position()  - running consumed count
*     - readable()       - count of unconsumed bytes
*     - advance()        - destructive cursor-only advance
*     - compact()        - shift unconsumed bytes to the head
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests_buffer_base_read.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "buffer_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_buffer_base_read
  Covers read(void*, std::size_t) on a dual-cursor buffer.
  Tests the following:
  - null destination returns 0 and does not advance the read cursor
  - zero-length call returns 0 and does not advance the read cursor
  - normal read copies bytes into the destination and advances the
    read cursor
  - partial read of a larger payload leaves remaining bytes intact
  - oversized request is clamped to readable() and reports the
    truncated count
  - read of an empty buffer (no writes) returns 0
  - read after full consumption returns 0
  - read content matches the original write byte-for-byte
*/
void
tests_buffer_base_read(
    test_handler& _handler
)
{
    const char src[] = "0123456789ABCDEF";  // 16 bytes

    // --- null destination short-circuit ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 16);

        std::size_t n = b.read(nullptr, 5);

        record_assertion(
            _handler,
            n == 0,
            "read(): null destination returns 0");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "read(): null destination does not advance read cursor");
    }

    // --- zero-length short-circuit ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 16);

        char        out[16];
        std::size_t n = b.read(out, 0);

        record_assertion(
            _handler,
            n == 0,
            "read(): zero-length returns 0");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "read(): zero-length does not advance read cursor");
    }

    // --- normal read consumes the entire payload ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 16);

        char        out[16] = { 0 };
        std::size_t n       = b.read(out, 16);

        record_assertion(
            _handler,
            n == 16,
            "read(): full-payload read returns 16");

        record_assertion(
            _handler,
            std::memcmp(out, src, 16) == 0,
            "read(): copied bytes match the source byte-for-byte");

        record_assertion(
            _handler,
            b.read_position() == 16,
            "read(): read cursor advances to 16");

        record_assertion(
            _handler,
            b.readable() == 0,
            "read(): readable() == 0 after full consumption");
    }

    // --- partial read leaves remainder intact ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 16);

        char        first[6]  = { 0 };
        char        second[10] = { 0 };

        std::size_t n1 = b.read(first, 6);

        record_assertion(
            _handler,
            n1 == 6,
            "read(): first partial read returns 6");

        record_assertion(
            _handler,
            std::memcmp(first, src, 6) == 0,
            "read(): first partial read returns the leading 6 bytes");

        record_assertion(
            _handler,
            b.readable() == 10,
            "read(): readable() == 10 after consuming 6 of 16");

        std::size_t n2 = b.read(second, 10);

        record_assertion(
            _handler,
            n2 == 10,
            "read(): second read consumes the remaining 10");

        record_assertion(
            _handler,
            std::memcmp(second, src + 6, 10) == 0,
            "read(): second read returns the trailing 10 bytes");

        record_assertion(
            _handler,
            b.readable() == 0,
            "read(): buffer fully consumed after second read");
    }

    // --- oversized read clamped to readable() ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 16);

        char        out[32] = { 0 };
        std::size_t n       = b.read(out, 32);

        record_assertion(
            _handler,
            n == 16,
            "read(): oversized request clamped to 16 readable bytes");

        record_assertion(
            _handler,
            std::memcmp(out, src, 16) == 0,
            "read(): clamped read still copies all readable bytes");
    }

    // --- read of empty buffer returns 0 ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        char        out[16];
        std::size_t n = b.read(out, 16);

        record_assertion(
            _handler,
            n == 0,
            "read(): empty buffer returns 0");
    }

    // --- read after full consumption returns 0 ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 4);

        char        out[16];
        b.read(out, 4);

        std::size_t n2 = b.read(out, 4);

        record_assertion(
            _handler,
            n2 == 0,
            "read(): exhausted buffer returns 0 on subsequent read");
    }

    return;
}


/*
tests_buffer_base_peek
  Covers peek() on a dual-cursor buffer.
  Tests the following:
  - peek() returns storage() on an empty buffer
  - peek() points at the byte immediately after the read cursor
  - peek() does not advance the read cursor
  - the pointer remains valid as long as no grow occurs between
    peek and inspection
  - the pointed-to bytes are the same as those that read() would
    return
*/
void
tests_buffer_base_peek(
    test_handler& _handler
)
{
    const char src[] = "PEEKDATA";  // 8 bytes

    // --- peek on empty buffer ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        record_assertion(
            _handler,
            b.peek() == b.storage(),
            "peek(): empty buffer returns storage()");
    }

    // --- peek before consumption ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 8);

        const char* p = b.peek();

        record_assertion(
            _handler,
            p == b.storage(),
            "peek(): unread buffer returns storage()");

        record_assertion(
            _handler,
            std::memcmp(p, src, 8) == 0,
            "peek(): peeked bytes match the written bytes");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "peek(): does not advance read cursor");

        record_assertion(
            _handler,
            b.readable() == 8,
            "peek(): readable() unchanged");
    }

    // --- peek after partial consumption ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 8);
        b.advance(3);

        const char* p = b.peek();

        record_assertion(
            _handler,
            p == b.storage() + 3,
            "peek(): points at storage() + read_position");

        record_assertion(
            _handler,
            std::memcmp(p, src + 3, 5) == 0,
            "peek(): returns the unconsumed tail bytes");
    }

    return;
}


/*
tests_buffer_base_readable_and_read_position
  Covers readable() and read_position() on a dual-cursor buffer.
  Tests the following:
  - both return 0 on a fresh buffer
  - readable() reflects (write_pos - read_pos)
  - read_position() reflects the consumed byte count
  - both update correctly after write/read/advance/compact
*/
void
tests_buffer_base_readable_and_read_position(
    test_handler& _handler
)
{
    mock_buffer<exponential_growth_policy,
                dual_cursor_policy> b(64);

    // fresh buffer
    record_assertion(
        _handler,
        b.readable() == 0,
        "readable(): fresh buffer == 0");

    record_assertion(
        _handler,
        b.read_position() == 0,
        "read_position(): fresh buffer == 0");

    // write 10
    b.write_fill('p', 10);

    record_assertion(
        _handler,
        b.readable() == 10,
        "readable(): == 10 after writing 10 bytes");

    record_assertion(
        _handler,
        b.read_position() == 0,
        "read_position(): == 0 after writes only");

    // advance 4
    b.advance(4);

    record_assertion(
        _handler,
        b.readable() == 6,
        "readable(): == 6 after consuming 4 of 10");

    record_assertion(
        _handler,
        b.read_position() == 4,
        "read_position(): == 4 after consuming 4");

    // read another 3 via destructive read
    char out[8];
    b.read(out, 3);

    record_assertion(
        _handler,
        b.readable() == 3,
        "readable(): == 3 after destructive read of 3");

    record_assertion(
        _handler,
        b.read_position() == 7,
        "read_position(): == 7 after consuming 7 total");

    // compact resets read_position to 0 while preserving readable
    b.compact();

    record_assertion(
        _handler,
        b.read_position() == 0,
        "read_position(): == 0 after compact");

    record_assertion(
        _handler,
        b.readable() == 3,
        "readable(): unchanged after compact (still 3)");

    return;
}


/*
tests_buffer_base_advance
  Covers advance() on a dual-cursor buffer.
  Tests the following:
  - advance(0) is a no-op
  - normal advance moves the read cursor and returns _n
  - advance does not copy data anywhere - it only moves the cursor
  - oversized advance is clamped to readable() and reports the
    clamped count
  - advance on an empty buffer returns 0
  - advance does not affect the write cursor
*/
void
tests_buffer_base_advance(
    test_handler& _handler
)
{
    const char src[] = "advancetest!!";  // 13 bytes

    // --- advance(0) is a no-op ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 13);

        std::size_t n = b.advance(0);

        record_assertion(
            _handler,
            n == 0,
            "advance(): advance(0) returns 0");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "advance(): advance(0) leaves cursor at 0");

        record_assertion(
            _handler,
            b.readable() == 13,
            "advance(): advance(0) leaves readable unchanged");
    }

    // --- normal advance ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 13);

        std::size_t n = b.advance(5);

        record_assertion(
            _handler,
            n == 5,
            "advance(): returns the bytes advanced");

        record_assertion(
            _handler,
            b.read_position() == 5,
            "advance(): cursor at 5 after advance(5)");

        record_assertion(
            _handler,
            b.readable() == 8,
            "advance(): readable == 8 after advancing 5 of 13");

        // peek confirms cursor moved
        record_assertion(
            _handler,
            *b.peek() == src[5],
            "advance(): peek() now points at byte 5");
    }

    // --- oversized advance clamped to readable ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 5);

        std::size_t n = b.advance(100);

        record_assertion(
            _handler,
            n == 5,
            "advance(): oversized clamped to readable (5)");

        record_assertion(
            _handler,
            b.readable() == 0,
            "advance(): buffer empty after clamped advance");

        record_assertion(
            _handler,
            b.read_position() == 5,
            "advance(): read_position == 5 (not 100) after clamp");
    }

    // --- advance on empty buffer returns 0 ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        std::size_t n = b.advance(10);

        record_assertion(
            _handler,
            n == 0,
            "advance(): empty buffer returns 0");
    }

    // --- advance does not affect write cursor ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 10);
        const std::size_t wp_before = b.write_position();

        b.advance(4);

        record_assertion(
            _handler,
            b.write_position() == wp_before,
            "advance(): write_position unchanged");
    }

    return;
}


/*
tests_buffer_base_compact
  Covers compact() on a dual-cursor buffer.
  Tests the following:
  - compact on an empty buffer returns 0 and leaves cursors at 0
  - compact after partial consumption shifts unconsumed bytes to
    the head of storage and updates cursors accordingly
  - compact after full consumption returns 0 and zeros both cursors
  - compact with no consumption (read_pos == 0) returns the same
    readable count and leaves the payload untouched
  - after a compact, the freed tail can be written into without
    growth
  - subsequent reads return the shifted data correctly
*/
void
tests_buffer_base_compact(
    test_handler& _handler
)
{
    const char src[] = "compactme!"; // 10 bytes

    // --- empty buffer ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        std::size_t pending = b.compact();

        record_assertion(
            _handler,
            pending == 0,
            "compact(): empty buffer returns 0");

        record_assertion(
            _handler,
            (b.read_position() == 0) && (b.write_position() == 0),
            "compact(): empty buffer leaves cursors at 0");
    }

    // --- partial consumption: shift to head ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 10);
        b.advance(3);

        const std::size_t pre_readable = b.readable();

        std::size_t pending = b.compact();

        record_assertion(
            _handler,
            pending == pre_readable,
            "compact(): partial consumption returns readable count");

        record_assertion(
            _handler,
            b.read_position() == 0,
            "compact(): read_position reset to 0");

        record_assertion(
            _handler,
            b.write_position() == pending,
            "compact(): write_position == pending");

        record_assertion(
            _handler,
            b.readable() == pending,
            "compact(): readable count preserved");

        // verify the unconsumed bytes are now at the head
        record_assertion(
            _handler,
            std::memcmp(b.storage(), src + 3, pending) == 0,
            "compact(): unconsumed bytes shifted to the head");
    }

    // --- full consumption: cursors reset, no shift needed ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 10);
        b.advance(10);  // consume everything

        std::size_t pending = b.compact();

        record_assertion(
            _handler,
            pending == 0,
            "compact(): full-consumption returns 0");

        record_assertion(
            _handler,
            (b.read_position() == 0) && (b.write_position() == 0),
            "compact(): full-consumption zeros both cursors");
    }

    // --- no consumption: data must not move ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        b.write(src, 10);

        const std::size_t  pre_w = b.write_position();
        std::size_t        pending = b.compact();

        record_assertion(
            _handler,
            pending == 10,
            "compact(): no-consumption returns full readable count");

        record_assertion(
            _handler,
            b.write_position() == pre_w,
            "compact(): no-consumption keeps write_position unchanged");

        record_assertion(
            _handler,
            std::memcmp(b.storage(), src, 10) == 0,
            "compact(): no-consumption leaves payload untouched");
    }

    // --- compact frees tail space for writes (round-trip) ---
    {
        mock_buffer<fixed_growth_policy,
                    dual_cursor_policy> b(16);

        b.write_fill('A', 16);   // fully fill (writable=0)
        b.advance(10);            // consume 10, leaving 6

        record_assertion(
            _handler,
            b.writable() == 0,
            "compact(): writable == 0 before compact (fixed buffer)");

        std::size_t pending = b.compact();

        record_assertion(
            _handler,
            pending == 6,
            "compact(): preserves the 6 unconsumed bytes");

        record_assertion(
            _handler,
            b.writable() == 10,
            "compact(): writable == 10 after compact "
            "(tail freed by shifting)");

        // now we can write 10 bytes without overflowing
        std::size_t w = b.write_fill('Z', 10);

        record_assertion(
            _handler,
            w == 10,
            "compact(): freed tail accepts 10 new bytes");

        record_assertion(
            _handler,
            b.full(),
            "compact(): fixed buffer is full again after refill");

        // read back: first 6 = 'A's (the shifted tail of the
        // original fill), next 10 = 'Z's.
        char out[16];
        std::size_t r = b.read(out, 16);

        record_assertion(
            _handler,
            r == 16,
            "compact(): full read after compact + refill yields 16");

        bool head_ok = true;

        for (std::size_t i = 0; i < 6; ++i)
        {
            if (out[i] != 'A')
            {
                head_ok = false;
                break;
            }
        }

        bool tail_ok = true;

        for (std::size_t i = 6; i < 16; ++i)
        {
            if (out[i] != 'Z')
            {
                tail_ok = false;
                break;
            }
        }

        record_assertion(
            _handler,
            head_ok,
            "compact(): first 6 bytes == 'A' (shifted survivors)");

        record_assertion(
            _handler,
            tail_ok,
            "compact(): next 10 bytes == 'Z' (refill)");
    }

    return;
}


NS_END  // test
NS_END  // djinterp
