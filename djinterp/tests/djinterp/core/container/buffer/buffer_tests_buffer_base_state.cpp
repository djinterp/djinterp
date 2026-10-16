/*******************************************************************************
* djinterp [test]                             buffer_tests_buffer_base_state.cpp
*
*   Unit tests for buffer_base's type aliases, exposed constants, and
* read-only state predicates: empty(), full(), size(), writable(), and
* write_position().  Higher-level write/read/lifecycle behaviour is
* covered in the sibling buffer_tests_buffer_base_*.cpp translation
* units.
*
*   Coverage strategy:
*   - aliases are checked with std::is_same_v on every public typedef
*   - constants are checked for value AND for matching the underlying
*     policy
*   - predicates are exercised in both write_only and dual cursor modes
*   - full() is exercised in both growable and fixed forms so the
*     short-circuit on can_grow is observed
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests_buffer_base_state.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "buffer_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_buffer_base_aliases
  Verifies buffer_base's public type aliases.
  Tests the following:
  - growth_policy resolves to the supplied policy type
  - cursor_policy resolves to the supplied policy type
  - cursor_state resolves to _CursorPolicy::cursor_state
  - the aliases work for all four policy permutations:
      fixed     + write_only
      fixed     + dual
      exponential + write_only
      exponential + dual
*/
void
tests_buffer_base_aliases(
    test_handler& _handler
)
{
    // --- fixed + write_only ---
    {
        using buf = mock_buffer<fixed_growth_policy,
                                write_only_cursor_policy>;

        static_assert(
            std::is_same<typename buf::growth_policy,
                         fixed_growth_policy>::value,
            "growth_policy alias mismatch (fixed + write_only).");
        static_assert(
            std::is_same<typename buf::cursor_policy,
                         write_only_cursor_policy>::value,
            "cursor_policy alias mismatch (fixed + write_only).");
        static_assert(
            std::is_same<
                typename buf::cursor_state,
                write_only_cursor_policy::cursor_state>::value,
            "cursor_state alias mismatch (fixed + write_only).");

        record_assertion(
            _handler,
            ( std::is_same<typename buf::growth_policy,
                           fixed_growth_policy>::value &&
              std::is_same<typename buf::cursor_policy,
                           write_only_cursor_policy>::value ),
            "aliases: fixed + write_only");
    }

    // --- fixed + dual ---
    {
        using buf = mock_buffer<fixed_growth_policy,
                                dual_cursor_policy>;

        static_assert(
            std::is_same<typename buf::growth_policy,
                         fixed_growth_policy>::value,
            "growth_policy alias mismatch (fixed + dual).");
        static_assert(
            std::is_same<typename buf::cursor_policy,
                         dual_cursor_policy>::value,
            "cursor_policy alias mismatch (fixed + dual).");
        static_assert(
            std::is_same<typename buf::cursor_state,
                         dual_cursor_policy::cursor_state>::value,
            "cursor_state alias mismatch (fixed + dual).");

        record_assertion(
            _handler,
            ( std::is_same<typename buf::growth_policy,
                           fixed_growth_policy>::value &&
              std::is_same<typename buf::cursor_policy,
                           dual_cursor_policy>::value ),
            "aliases: fixed + dual");
    }

    // --- exponential + write_only ---
    {
        using buf = mock_buffer<exponential_growth_policy,
                                write_only_cursor_policy>;

        static_assert(
            std::is_same<typename buf::growth_policy,
                         exponential_growth_policy>::value,
            "growth_policy alias mismatch (exp + write_only).");
        static_assert(
            std::is_same<typename buf::cursor_policy,
                         write_only_cursor_policy>::value,
            "cursor_policy alias mismatch (exp + write_only).");

        record_assertion(
            _handler,
            std::is_same<typename buf::growth_policy,
                         exponential_growth_policy>::value,
            "aliases: exponential growth policy");
    }

    // --- exponential + dual ---
    {
        using buf = mock_buffer<exponential_growth_policy,
                                dual_cursor_policy>;

        static_assert(
            std::is_same<typename buf::cursor_policy,
                         dual_cursor_policy>::value,
            "cursor_policy alias mismatch (exp + dual).");
        static_assert(
            std::is_same<typename buf::cursor_state,
                         dual_cursor_policy::cursor_state>::value,
            "cursor_state alias mismatch (exp + dual).");

        record_assertion(
            _handler,
            std::is_same<typename buf::cursor_policy,
                         dual_cursor_policy>::value,
            "aliases: dual cursor policy");
    }

    // --- linear and page policies wire through unchanged ---
    {
        using buf_l = mock_buffer<linear_growth_policy<>,
                                  dual_cursor_policy>;
        using buf_p = mock_buffer<page_growth_policy<>,
                                  write_only_cursor_policy>;

        static_assert(
            std::is_same<typename buf_l::growth_policy,
                         linear_growth_policy<>>::value,
            "growth_policy alias mismatch (linear).");
        static_assert(
            std::is_same<typename buf_p::growth_policy,
                         page_growth_policy<>>::value,
            "growth_policy alias mismatch (page).");

        record_assertion(
            _handler,
            std::is_same<typename buf_l::growth_policy,
                         linear_growth_policy<>>::value,
            "aliases: linear growth policy");

        record_assertion(
            _handler,
            std::is_same<typename buf_p::growth_policy,
                         page_growth_policy<>>::value,
            "aliases: page growth policy");
    }

    return;
}


/*
tests_buffer_base_constants
  Verifies buffer_base's static constant members.
  Tests the following:
  - growth_strategy mirrors _GrowthPolicy::strategy
  - cursor_model mirrors _CursorPolicy::model
  - can_grow mirrors _GrowthPolicy::can_grow
  - has_read_cursor mirrors _CursorPolicy::has_read_cursor
  - both can_grow values (true / false) and both has_read_cursor
    values (true / false) are observed
*/
void
tests_buffer_base_constants(
    test_handler& _handler
)
{
    // --- fixed + write_only: can_grow = false, has_read_cursor = false ---
    {
        using buf = mock_buffer<fixed_growth_policy,
                                write_only_cursor_policy>;

        static_assert(
            buf::growth_strategy == buffer_growth_strategy::none,
            "fixed: growth_strategy must be ::none.");
        static_assert(
            buf::cursor_model == buffer_cursor_model::write_only,
            "write_only: cursor_model must be ::write_only.");
        static_assert(
            buf::can_grow == false,
            "fixed: can_grow must be false.");
        static_assert(
            buf::has_read_cursor == false,
            "write_only: has_read_cursor must be false.");

        record_assertion(
            _handler,
            buf::growth_strategy == buffer_growth_strategy::none,
            "constants: fixed::growth_strategy == none");

        record_assertion(
            _handler,
            buf::cursor_model == buffer_cursor_model::write_only,
            "constants: write_only::cursor_model == write_only");

        record_assertion(
            _handler,
            buf::can_grow == false,
            "constants: fixed::can_grow == false");

        record_assertion(
            _handler,
            buf::has_read_cursor == false,
            "constants: write_only::has_read_cursor == false");
    }

    // --- exponential + dual: can_grow = true, has_read_cursor = true ---
    {
        using buf = mock_buffer<exponential_growth_policy,
                                dual_cursor_policy>;

        static_assert(
            buf::growth_strategy == buffer_growth_strategy::exponential,
            "exponential: growth_strategy must be ::exponential.");
        static_assert(
            buf::cursor_model == buffer_cursor_model::dual,
            "dual: cursor_model must be ::dual.");
        static_assert(
            buf::can_grow == true,
            "exponential: can_grow must be true.");
        static_assert(
            buf::has_read_cursor == true,
            "dual: has_read_cursor must be true.");

        record_assertion(
            _handler,
            buf::growth_strategy == buffer_growth_strategy::exponential,
            "constants: exp::growth_strategy == exponential");

        record_assertion(
            _handler,
            buf::cursor_model == buffer_cursor_model::dual,
            "constants: dual::cursor_model == dual");

        record_assertion(
            _handler,
            buf::can_grow == true,
            "constants: exp::can_grow == true");

        record_assertion(
            _handler,
            buf::has_read_cursor == true,
            "constants: dual::has_read_cursor == true");
    }

    // --- linear and page growth strategies surface correctly ---
    {
        using buf_l = mock_buffer<linear_growth_policy<>,
                                  write_only_cursor_policy>;
        using buf_p = mock_buffer<page_growth_policy<>,
                                  dual_cursor_policy>;

        record_assertion(
            _handler,
            buf_l::growth_strategy == buffer_growth_strategy::linear,
            "constants: linear::growth_strategy == linear");

        record_assertion(
            _handler,
            buf_p::growth_strategy == buffer_growth_strategy::page_aligned,
            "constants: page::growth_strategy == page_aligned");
    }

    return;
}


/*
tests_buffer_base_empty
  Covers empty() across both cursor models.
  Tests the following:
  - default-constructed buffer is empty (write_only and dual)
  - empty() is false after a write (write_only)
  - empty() is false after a write but no read (dual)
  - empty() becomes true again after full consumption (dual)
  - empty() stays false in write_only mode even when all data has
    been logically taken by the caller (no read cursor exists, so
    written count is the predicate)
*/
void
tests_buffer_base_empty(
    test_handler& _handler
)
{
    const char src[] = "hello";

    // --- write_only mode ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(64);

        record_assertion(
            _handler,
            b.empty(),
            "empty(): write_only newly-constructed is empty");

        b.write(src, 5);

        record_assertion(
            _handler,
            !b.empty(),
            "empty(): write_only is not empty after write");

        // write_only has no read cursor, so the only way back to
        // empty is reset/clear (tested in lifecycle).
    }

    // --- dual mode ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        record_assertion(
            _handler,
            b.empty(),
            "empty(): dual newly-constructed is empty");

        b.write(src, 5);

        record_assertion(
            _handler,
            !b.empty(),
            "empty(): dual is not empty after write");

        // partial consumption keeps it non-empty
        b.advance(2);

        record_assertion(
            _handler,
            !b.empty(),
            "empty(): dual is not empty after partial consumption");

        // full consumption returns to empty even though write_pos > 0
        b.advance(3);

        record_assertion(
            _handler,
            b.empty(),
            "empty(): dual is empty after full consumption "
            "(readable() == 0)");
    }

    return;
}


/*
tests_buffer_base_full
  Covers full() across growable and fixed buffers.
  Tests the following:
  - full() is false on a buffer with available writable space
  - full() is false on a growable buffer even when writable == 0
    (the can_grow short-circuit keeps it expandable)
  - full() is true on a fixed buffer once write_pos reaches capacity
  - the predicate is independent of read cursor state
*/
void
tests_buffer_base_full(
    test_handler& _handler
)
{
    // --- growable buffer is never reported as full ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(8);

        record_assertion(
            _handler,
            !b.full(),
            "full(): growable empty buffer is not full");

        // fill it to capacity; still not "full" because can_grow
        b.write_fill('x', 8);

        record_assertion(
            _handler,
            b.writable() == 0,
            "full(): writable == 0 after filling growable buffer");

        record_assertion(
            _handler,
            !b.full(),
            "full(): growable buffer with writable==0 is still "
            "NOT full (can_grow short-circuit)");
    }

    // --- fixed buffer becomes full when writable == 0 ---
    {
        mock_buffer<fixed_growth_policy,
                    write_only_cursor_policy> b(8);

        record_assertion(
            _handler,
            !b.full(),
            "full(): fixed empty buffer is not full");

        // partial fill keeps it not full
        std::size_t n1 = b.write_fill('x', 4);

        record_assertion(
            _handler,
            n1 == 4,
            "full(): partial fill writes 4 bytes");

        record_assertion(
            _handler,
            !b.full(),
            "full(): fixed buffer with writable > 0 is not full");

        // fill to capacity
        std::size_t n2 = b.write_fill('y', 4);

        record_assertion(
            _handler,
            n2 == 4,
            "full(): second fill writes remaining 4 bytes");

        record_assertion(
            _handler,
            b.writable() == 0,
            "full(): writable == 0 after fixed buffer is filled");

        record_assertion(
            _handler,
            b.full(),
            "full(): fixed buffer with writable==0 is full");
    }

    // --- fixed dual buffer: read cursor doesn't affect full() ---
    {
        mock_buffer<fixed_growth_policy,
                    dual_cursor_policy> b(8);

        b.write_fill('z', 8);

        record_assertion(
            _handler,
            b.full(),
            "full(): fixed dual buffer is full when write_pos==cap");

        // consume half - writable() looks at write_pos, not
        // readable, so full() stays true.
        b.advance(4);

        record_assertion(
            _handler,
            b.full(),
            "full(): fixed dual buffer remains full even after "
            "partial consumption (full() is write-cursor based)");
    }

    return;
}


/*
tests_buffer_base_size
  Covers size() across both cursor models.
  Tests the following:
  - size() returns 0 on a fresh buffer
  - write_only: size() == total bytes written
  - dual:       size() == unconsumed bytes (write - read)
  - size() is unaffected by capacity changes
*/
void
tests_buffer_base_size(
    test_handler& _handler
)
{
    const char src[] = "abcdef";

    // --- write_only: size == written ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(64);

        record_assertion(
            _handler,
            b.size() == 0,
            "size(): write_only fresh buffer has size 0");

        b.write(src, 6);

        record_assertion(
            _handler,
            b.size() == 6,
            "size(): write_only size == 6 after writing 6 bytes");

        b.write(src, 6);

        record_assertion(
            _handler,
            b.size() == 12,
            "size(): write_only size == 12 after writing 6 more");
    }

    // --- dual: size == readable ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(64);

        record_assertion(
            _handler,
            b.size() == 0,
            "size(): dual fresh buffer has size 0");

        b.write(src, 6);

        record_assertion(
            _handler,
            b.size() == 6,
            "size(): dual size == 6 after writing 6 bytes");

        b.advance(2);

        record_assertion(
            _handler,
            b.size() == 4,
            "size(): dual size == 4 after consuming 2 of 6");

        b.advance(4);

        record_assertion(
            _handler,
            b.size() == 0,
            "size(): dual size == 0 after consuming all bytes");
    }

    return;
}


/*
tests_buffer_base_writable_and_position
  Covers writable() and write_position().
  Tests the following:
  - writable() returns capacity - write_pos
  - write_position() returns the running write_pos
  - both update correctly under write(), write_byte(), and
    write_fill()
  - both apply equally in write_only and dual modes
  - after a grow, writable() reflects the new capacity
*/
void
tests_buffer_base_writable_and_position(
    test_handler& _handler
)
{
    // --- write_only ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(64);

        record_assertion(
            _handler,
            b.writable() == 64,
            "writable(): fresh buffer writable == capacity (64)");

        record_assertion(
            _handler,
            b.write_position() == 0,
            "write_position(): fresh buffer position == 0");

        b.write_fill('x', 10);

        record_assertion(
            _handler,
            b.writable() == 54,
            "writable(): == 54 after writing 10 bytes");

        record_assertion(
            _handler,
            b.write_position() == 10,
            "write_position(): == 10 after writing 10 bytes");

        b.write_byte('y');

        record_assertion(
            _handler,
            b.write_position() == 11,
            "write_position(): == 11 after write_byte");
    }

    // --- dual: same predicates apply ---
    {
        mock_buffer<exponential_growth_policy,
                    dual_cursor_policy> b(32);

        b.write_fill('a', 16);

        record_assertion(
            _handler,
            b.writable() == 16,
            "writable(): dual mode reports capacity - write_pos");

        record_assertion(
            _handler,
            b.write_position() == 16,
            "write_position(): dual mode tracks write cursor");

        // advancing read does NOT change writable / write_position
        b.advance(8);

        record_assertion(
            _handler,
            b.writable() == 16,
            "writable(): dual unaffected by advance()");

        record_assertion(
            _handler,
            b.write_position() == 16,
            "write_position(): dual unaffected by advance()");
    }

    // --- grow path: writable expands ---
    {
        mock_buffer<exponential_growth_policy,
                    write_only_cursor_policy> b(8);

        b.write_fill('q', 8);

        record_assertion(
            _handler,
            b.writable() == 0,
            "writable(): == 0 at end of initial capacity");

        // trigger growth via reserve
        bool ok = b.reserve(64);

        record_assertion(
            _handler,
            ok,
            "writable(): reserve(64) succeeds on growable buffer");

        record_assertion(
            _handler,
            b.writable() >= 56,
            "writable(): reflects expanded capacity after grow");

        record_assertion(
            _handler,
            b.write_position() == 8,
            "write_position(): unchanged across grow");
    }

    return;
}


NS_END  // test
NS_END  // djinterp
