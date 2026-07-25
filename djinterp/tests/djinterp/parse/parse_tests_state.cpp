#include <climits>
#include <cstring>

#include "parse_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_parse_state_element_alias
  Verifies parse_state publishes the element type it was instantiated with,
so generic code can recover the stream's element without naming it.
  Tests the following:
  - element_type is the exact template argument for character streams,
  - for byte streams,
  - for arbitrary integral streams,
  - for aggregate (token) streams.
*/
bool
tests_parse_state_element_alias()
{
    // the alias mirrors the template argument exactly, on each element kind
    D_PA_CHECK((std::is_same<dp::parse_state<char>::element_type,
                             char>::value));
    D_PA_CHECK((std::is_same<dp::parse_state<unsigned char>::element_type,
                             unsigned char>::value));
    D_PA_CHECK((std::is_same<dp::parse_state<std::int32_t>::element_type,
                             std::int32_t>::value));
    D_PA_CHECK((std::is_same<dp::parse_state<pa_token>::element_type,
                             pa_token>::value));

    return true;
}

/*
tests_parse_state_default_construction
  Verifies the default parse_state is a null, empty, exhausted stream, so a
default-constructed parser has nothing to consume rather than a dangling
pointer.
  Tests the following:
  - data is null, length and offset are zero,
  - remaining() is zero and at_end() is true,
  - current() is null,
  - the type is default-constructible.
*/
bool
tests_parse_state_default_construction()
{
    dp::parse_state<char> state;

    // an empty stream over no storage at all
    D_PA_CHECK(state.data   == nullptr);
    D_PA_CHECK(state.length == 0u);
    D_PA_CHECK(state.offset == 0u);

    // and the derived queries agree it is exhausted
    D_PA_CHECK(state.remaining() == 0u);
    D_PA_CHECK(state.at_end());
    D_PA_CHECK(state.current() == nullptr);

    D_PA_CHECK(std::is_default_constructible<dp::parse_state<char> >::value);

    return true;
}

/*
tests_parse_state_explicit_construction
  Verifies the explicit constructor stores its arguments and defaults the
offset, which is the ordinary way a parse begins.
  Tests the following:
  - data and length are stored as given,
  - the offset defaults to zero when omitted,
  - a supplied in-range offset is stored,
  - the derived queries reflect the supplied offset.
*/
bool
tests_parse_state_explicit_construction()
{
    const char*       text   = "hello";
    const std::size_t length = 5u;

    // the two-argument form defaults the offset to the start of the input
    dp::parse_state<char> fresh(text, length);

    D_PA_CHECK(fresh.data   == text);
    D_PA_CHECK(fresh.length == length);
    D_PA_CHECK(fresh.offset == 0u);
    D_PA_CHECK(fresh.remaining() == 5u);
    D_PA_CHECK(!fresh.at_end());
    D_PA_CHECK(fresh.current() == text);

    // the three-argument form seeds the offset, as if resuming mid-stream
    dp::parse_state<char> resumed(text, length, 2u);

    D_PA_CHECK(resumed.data   == text);
    D_PA_CHECK(resumed.length == length);
    D_PA_CHECK(resumed.offset == 2u);
    D_PA_CHECK(resumed.remaining() == 3u);
    D_PA_CHECK(!resumed.at_end());
    D_PA_CHECK(*resumed.current() == 'l');

    // an offset exactly at the end is in range and means exhaustion
    dp::parse_state<char> at_end(text, length, 5u);

    D_PA_CHECK(at_end.offset == 5u);
    D_PA_CHECK(at_end.remaining() == 0u);
    D_PA_CHECK(at_end.at_end());
    D_PA_CHECK(at_end.current() == nullptr);

    return true;
}

/*
tests_parse_state_construction_does_not_clamp
  PINNED BEHAVIOUR.  Verifies that the constructor stores an out-of-range
initial offset verbatim -- it does NOT clamp to length -- while the derived
queries stay defensive and report exhaustion rather than reading past the
buffer.  The stored-offset assertion is expected to invert if the
constructor ever adds a clamp.
  Tests the following:
  - an initial offset past length is stored unchanged,
  - remaining() saturates at zero rather than underflowing,
  - at_end() reports exhaustion,
  - current() returns null rather than a past-the-end pointer.
*/
bool
tests_parse_state_construction_does_not_clamp()
{
    const char* text = "abc";

    // the constructor performs no clamp: the raw offset is what was passed
    dp::parse_state<char> beyond(text, 3u, 10u);

    D_PA_CHECK(beyond.offset == 10u);
    D_PA_CHECK(beyond.length == 3u);
    D_PA_CHECK(beyond.offset > beyond.length);

    // yet the derived queries do not compute length - offset (which would
    // wrap around SIZE_MAX); remaining() guards the subtraction
    D_PA_CHECK(beyond.remaining() == 0u);
    D_PA_CHECK(beyond.at_end());
    D_PA_CHECK(beyond.current() == nullptr);

    // the same holds for an offset only one past the end
    dp::parse_state<char> just_past(text, 3u, 4u);

    D_PA_CHECK(just_past.offset == 4u);
    D_PA_CHECK(just_past.remaining() == 0u);
    D_PA_CHECK(just_past.at_end());
    D_PA_CHECK(just_past.current() == nullptr);

    return true;
}

/*
tests_parse_state_remaining
  Verifies remaining() counts the elements still available and saturates at
zero, so a length computation never underflows on an over-run offset.
  Tests the following:
  - remaining() is length at offset zero,
  - it decreases by one per element consumed,
  - it is zero exactly at the end,
  - it stays zero for any offset at or beyond the end.
*/
bool
tests_parse_state_remaining()
{
    const char* text = "abcde";

    dp::parse_state<char> state(text, 5u);

    // the full extent is available before anything is consumed
    D_PA_CHECK(state.remaining() == 5u);

    // walking forward one element at a time counts the extent down
    state.offset = 1u;
    D_PA_CHECK(state.remaining() == 4u);

    state.offset = 4u;
    D_PA_CHECK(state.remaining() == 1u);

    // at the end there is nothing left
    state.offset = 5u;
    D_PA_CHECK(state.remaining() == 0u);

    // and past the end it saturates rather than wrapping through SIZE_MAX
    state.offset = 6u;
    D_PA_CHECK(state.remaining() == 0u);

    state.offset = static_cast<std::size_t>(-1);
    D_PA_CHECK(state.remaining() == 0u);

    return true;
}

/*
tests_parse_state_at_end
  Verifies at_end() marks the boundary exactly, so a parse loop stops on the
last element rather than one past it or one short.
  Tests the following:
  - at_end() is false strictly before the length,
  - it is true exactly at the length,
  - it is true beyond the length,
  - it is true for a zero-length stream from the outset.
*/
bool
tests_parse_state_at_end()
{
    const char* text = "xy";

    dp::parse_state<char> state(text, 2u);

    // strictly before the end there is input to consume
    D_PA_CHECK(!state.at_end());

    state.offset = 1u;
    D_PA_CHECK(!state.at_end());

    // exactly at the end the stream is exhausted
    state.offset = 2u;
    D_PA_CHECK(state.at_end());

    // and beyond it likewise
    state.offset = 3u;
    D_PA_CHECK(state.at_end());

    // a zero-length stream is exhausted from the very start
    dp::parse_state<char> empty(text, 0u);
    D_PA_CHECK(empty.at_end());

    return true;
}

/*
tests_parse_state_current
  Verifies current() points at the live element while input remains and is
null at exhaustion, so a parser can dereference it only when it is valid.
  Tests the following:
  - current() is data + offset within the stream,
  - dereferencing it yields the expected element,
  - it tracks the offset as the stream advances,
  - it is null at and beyond the end.
*/
bool
tests_parse_state_current()
{
    const char* text = "abc";

    dp::parse_state<char> state(text, 3u);

    // the current pointer addresses the element at the offset
    D_PA_CHECK(state.current() == (text + 0));
    D_PA_CHECK(*state.current() == 'a');

    // and it moves with the offset
    state.offset = 1u;
    D_PA_CHECK(state.current() == (text + 1));
    D_PA_CHECK(*state.current() == 'b');

    state.offset = 2u;
    D_PA_CHECK(*state.current() == 'c');

    // at the end there is no element to point at
    state.offset = 3u;
    D_PA_CHECK(state.current() == nullptr);

    // and beyond it, still null rather than a wild pointer
    state.offset = 100u;
    D_PA_CHECK(state.current() == nullptr);

    return true;
}

/*
tests_parse_state_advance_default_and_zero
  Verifies the two smallest advances: the default single step, and the
zero-length no-op a combinator might issue unconditionally.
  Tests the following:
  - advance() with no argument moves the offset by one,
  - advance(0) leaves the offset unchanged,
  - a multi-element advance moves by exactly that count,
  - the current element after each advance is correct.
*/
bool
tests_parse_state_advance_default_and_zero()
{
    const char* text = "abcdef";

    dp::parse_state<char> state(text, 6u);

    // the default advance consumes exactly one element
    state.advance();
    D_PA_CHECK(state.offset == 1u);
    D_PA_CHECK(*state.current() == 'b');

    // a zero advance is a genuine no-op, not a one-step advance
    state.advance(0u);
    D_PA_CHECK(state.offset == 1u);
    D_PA_CHECK(*state.current() == 'b');

    // a multi-element advance moves by exactly the count
    state.advance(3u);
    D_PA_CHECK(state.offset == 4u);
    D_PA_CHECK(*state.current() == 'e');

    // and one more single step
    state.advance();
    D_PA_CHECK(state.offset == 5u);
    D_PA_CHECK(*state.current() == 'f');

    return true;
}

/*
tests_parse_state_advance_clamps
  Verifies that advancing past the end clamps the offset to exactly the
length, so an over-long consume lands the stream at the end rather than
beyond it.
  Tests the following:
  - advancing past the end from inside the stream clamps to length,
  - advancing from exactly the end stays at length,
  - advancing to exactly the end is an exact fill, not a clamp,
  - the queries agree the stream is exhausted after a clamp.
*/
bool
tests_parse_state_advance_clamps()
{
    const char* text = "abcd";

    // an over-long advance from inside the stream clamps to the length
    dp::parse_state<char> over(text, 4u, 1u);
    over.advance(100u);

    D_PA_CHECK(over.offset == 4u);
    D_PA_CHECK(over.at_end());
    D_PA_CHECK(over.remaining() == 0u);
    D_PA_CHECK(over.current() == nullptr);

    // advancing when already at the end keeps the offset at the length
    dp::parse_state<char> at_end(text, 4u, 4u);
    at_end.advance(10u);

    D_PA_CHECK(at_end.offset == 4u);
    D_PA_CHECK(at_end.at_end());

    // advancing to precisely the end is an exact consume, still equal to
    // the length, and reported as exhausted
    dp::parse_state<char> exact(text, 4u, 0u);
    exact.advance(4u);

    D_PA_CHECK(exact.offset == 4u);
    D_PA_CHECK(exact.at_end());

    // a single step from the last element also lands exactly at the end
    dp::parse_state<char> last(text, 4u, 3u);
    last.advance();

    D_PA_CHECK(last.offset == 4u);
    D_PA_CHECK(last.at_end());

    return true;
}

/*
tests_parse_state_advance_overflow_wraps
  PINNED BEHAVIOUR.  Verifies that advance adds its count to the offset
before it clamps, so a count near SIZE_MAX wraps the addition and the offset
moves BACKWARDS instead of clamping to the end.  A clamp expressed as
`if (_count > length - offset) offset = length;` would instead leave the
stream at the end.  This assertion is expected to invert when advance is
made overflow-safe.
  Tests the following:
  - advance(SIZE_MAX) from a mid-stream offset wraps to offset - 1,
  - the resulting offset is below the length, so at_end() is false,
  - the stream therefore reports input remaining that a clamp would deny,
  - a count that overflows to exactly length is handled by the same path.
*/
bool
tests_parse_state_advance_overflow_wraps()
{
    const std::size_t max_count = static_cast<std::size_t>(-1);

    // from offset 5 over a length-10 input, offset + (2^64 - 1) wraps to
    // 5 - 1 == 4 in modular arithmetic; the post-add value (4) is below the
    // length, so the clamp branch is not taken and the offset stays at 4
    dp::parse_state<char> state("abcdefghij", 10u, 5u);
    state.advance(max_count);

    D_PA_CHECK(state.offset == 4u);
    D_PA_CHECK(state.offset < state.length);
    D_PA_CHECK(!state.at_end());

    // the wrap has made the stream claim MORE input than before the
    // advance -- a real over-run would have exhausted it
    D_PA_CHECK(state.remaining() == 6u);
    D_PA_CHECK(state.current() != nullptr);
    D_PA_CHECK(*state.current() == 'e');

    // the offset genuinely moved backwards from where it started
    D_PA_CHECK(state.offset < 5u);

    return true;
}

/*
tests_parse_state_empty_and_null_input
  Verifies the two degenerate streams: a zero-length view over real storage,
and a wholly null stream, both of which a parser may legitimately be handed.
  Tests the following:
  - a zero-length stream over a real pointer is exhausted immediately,
  - its current() is null despite the pointer being non-null,
  - a fully null stream is exhausted,
  - advancing either degenerate stream is a safe no-op.
*/
bool
tests_parse_state_empty_and_null_input()
{
    const char* backing = "unused";

    // a zero-length window over real storage: the pointer is non-null but
    // there is nothing to read
    dp::parse_state<char> empty(backing, 0u);

    D_PA_CHECK(empty.data == backing);
    D_PA_CHECK(empty.at_end());
    D_PA_CHECK(empty.remaining() == 0u);
    D_PA_CHECK(empty.current() == nullptr);

    // advancing an empty stream cannot move the offset past zero length
    empty.advance(5u);
    D_PA_CHECK(empty.offset == 0u);
    D_PA_CHECK(empty.at_end());

    // a fully null stream is the default-constructed shape
    dp::parse_state<char> null_stream(nullptr, 0u);

    D_PA_CHECK(null_stream.data == nullptr);
    D_PA_CHECK(null_stream.at_end());
    D_PA_CHECK(null_stream.current() == nullptr);

    // and advancing it is likewise a safe no-op
    null_stream.advance();
    D_PA_CHECK(null_stream.offset == 0u);

    return true;
}

/*
tests_parse_state_save_and_restore
  Verifies the save-and-restore idiom the alt combinator relies on: capture
the offset, consume input, then rewind exactly to the captured point.
  Tests the following:
  - a saved offset can be restored after consuming input,
  - the restored stream sees precisely the input it saw before,
  - a restore to an earlier point re-exposes consumed elements,
  - repeated save/consume/restore cycles are exact.
*/
bool
tests_parse_state_save_and_restore()
{
    const char* text = "abcdef";

    dp::parse_state<char> state(text, 6u);

    // capture the residual position, then consume several elements
    std::size_t saved = state.offset;
    state.advance(3u);

    D_PA_CHECK(state.offset == 3u);
    D_PA_CHECK(*state.current() == 'd');

    // rewinding restores the exact residual the branch started from
    state.offset = saved;

    D_PA_CHECK(state.offset == 0u);
    D_PA_CHECK(*state.current() == 'a');
    D_PA_CHECK(state.remaining() == 6u);

    // a rewind to a mid-stream mark re-exposes the elements after it
    state.advance(5u);
    std::size_t mark = state.offset;
    state.advance(1u);
    D_PA_CHECK(state.at_end());

    state.offset = mark;
    D_PA_CHECK(state.offset == 5u);
    D_PA_CHECK(*state.current() == 'f');

    return true;
}

/*
tests_parse_state_copy_independence
  Verifies a copied parse_state is an independent snapshot, since the alt
combinator effectively takes one when it saves the offset -- advancing one
copy must not disturb another.
  Tests the following:
  - a copy shares the same backing data pointer and length,
  - advancing the copy does not move the original's offset,
  - advancing the original does not move the copy's offset,
  - parse_state is trivially copyable, so a copy is a plain snapshot.
*/
bool
tests_parse_state_copy_independence()
{
    const char* text = "abcdef";

    dp::parse_state<char> original(text, 6u, 1u);
    dp::parse_state<char> snapshot(original);

    // the snapshot views the same buffer from the same position
    D_PA_CHECK(snapshot.data   == original.data);
    D_PA_CHECK(snapshot.length == original.length);
    D_PA_CHECK(snapshot.offset == original.offset);

    // advancing the snapshot leaves the original where it was
    snapshot.advance(3u);
    D_PA_CHECK(snapshot.offset == 4u);
    D_PA_CHECK(original.offset == 1u);

    // and advancing the original leaves the snapshot where it was
    original.advance(1u);
    D_PA_CHECK(original.offset == 2u);
    D_PA_CHECK(snapshot.offset == 4u);

    // the value semantics are trivial: a copy is a byte-for-byte snapshot
    D_PA_CHECK(
        std::is_trivially_copy_constructible<dp::parse_state<char> >::value);
    D_PA_CHECK(
        std::is_trivially_copyable<dp::parse_state<char> >::value);

    return true;
}

/*
tests_parse_state_non_character_elements
  Verifies parse_state is agnostic to its element type, so byte buffers and
token sequences instantiate it exactly as a character stream does.
  Tests the following:
  - a byte stream advances and reads back its bytes,
  - an integral stream does the same,
  - a token (aggregate) stream does the same,
  - the element pointer arithmetic respects the element size.
*/
bool
tests_parse_state_non_character_elements()
{
    // a raw byte buffer
    const unsigned char bytes[4] = { 0x10u, 0x20u, 0x30u, 0x40u };

    dp::parse_state<unsigned char> byte_stream(bytes, 4u);

    D_PA_CHECK(*byte_stream.current() == 0x10u);
    byte_stream.advance(2u);
    D_PA_CHECK(byte_stream.offset == 2u);
    D_PA_CHECK(*byte_stream.current() == 0x30u);

    // a stream of wide integers
    const std::int32_t numbers[3] = { -1, 0, 77 };

    dp::parse_state<std::int32_t> number_stream(numbers, 3u);

    D_PA_CHECK(*number_stream.current() == -1);
    number_stream.advance();
    D_PA_CHECK(*number_stream.current() == 0);
    number_stream.advance();
    D_PA_CHECK(*number_stream.current() == 77);

    // a stream of aggregates (the token case the docstring calls out)
    const pa_token tokens[2] =
    {
        { 1, 100 },
        { 2, 200 }
    };

    dp::parse_state<pa_token> token_stream(tokens, 2u);

    D_PA_CHECK(token_stream.current()->kind  == 1);
    D_PA_CHECK(token_stream.current()->value == 100);

    token_stream.advance();

    D_PA_CHECK(token_stream.current()->kind  == 2);
    D_PA_CHECK(token_stream.current()->value == 200);

    // the current pointer moved by one element, not by one byte
    D_PA_CHECK(token_stream.current() == (tokens + 1));

    return true;
}

/*
tests_parse_state_full_traversal
  Verifies a complete walk of a stream visits every element exactly once and
lands cleanly at the end, which is the shape of every consuming parser.
  Tests the following:
  - iterating with current()/advance() reads every element in order,
  - the visit count equals the length,
  - the loop terminates exactly at the end,
  - the reconstructed sequence equals the original input.
*/
bool
tests_parse_state_full_traversal()
{
    const char*       text   = "traverse";
    const std::size_t length = 8u;

    dp::parse_state<char> state(text, length);

    char        rebuilt[16];
    std::size_t visited;

    visited = 0;

    // consume the whole stream one element at a time
    while (!state.at_end())
    {
        rebuilt[visited] = *state.current();
        ++visited;
        state.advance();
    }

    rebuilt[visited] = '\0';

    // every element was visited exactly once
    D_PA_CHECK(visited == length);

    // the loop ended precisely at the end, not beyond it
    D_PA_CHECK(state.at_end());
    D_PA_CHECK(state.offset == length);
    D_PA_CHECK(state.remaining() == 0u);

    // and the copy reconstructs the input faithfully
    D_PA_CHECK(std::strcmp(rebuilt, "traverse") == 0);

    return true;
}

NS_END  // testing
NS_END  // djinterp
