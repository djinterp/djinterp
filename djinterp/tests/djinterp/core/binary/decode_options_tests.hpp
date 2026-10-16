/*******************************************************************************
* djinterp [binary/test]                                decode_options_tests.hpp
*
*   Declarations, fixtures, and the per-suite assertion macro for the
* decode_options.hpp unit tests.  decode_options.hpp owns the parameterised byte
* readers (get_uint<E> / get_varint / get_length<L,E>), the parameterised leaf
* decoder dec_tau<E> (decode_leaf), and its convenience over a byte_string.  The
* reader / result / member-surface machinery is reused from decode.hpp and the
* serial enums from encode_options.hpp; the focus here is byte order and length
* decoding, and the round trip against the parameterised encoder.  Every tests_*
* predicate is authored flat in djinterp::testing and referenced by address from
* the runner spec.
*
*   The section .cpp files that DEFINE these predicates mirror decode_options.hpp:
*     decode_options_tests_readers.cpp      - I.   get_uint<E> / get_varint / get_length
*     decode_options_tests_leaves.cpp       - II.  dec_tau<E> leaf families, both orders
*     decode_options_tests_roundtrip.cpp    - the encode_leaf<E> <-> decode_leaf<E> identity
*     decode_options_tests_convenience.cpp  - III. decode_leaf<E>(byte_string)
*
*
* path:      /tests/djinterp/core/binary/decode_options_tests.hpp
* link(s):   TBA
* author(s): unit-test generator                             created: 2026.07.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_BINARY_DECODE_OPTIONS_TESTS_
#define DJINTERP_BINARY_DECODE_OPTIONS_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
// djinterp
#ifdef DTEST_SPEC_MODE
#include <djinterp/test/test_defaults.hpp>
#endif  // DTEST_SPEC_MODE
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/binary/decode_options.hpp"   // module under test (pulls decode.hpp + encode_options.hpp)
#endif  // !DTEST_SPEC_MODE


// D_DO_CHECK
//   macro: fail the current test - return false from the enclosing predicate -
// unless the condition holds.  Variadic so a condition may contain top-level
// commas (template argument lists, brace-init).  Unique letters (DO) so
// co-compiled suites never collide on the macro name.
#define D_DO_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


NS_DJINTERP
NS_TESTING

#ifndef DTEST_SPEC_MODE   // fixtures: normal face only
// ===========================================================================
// Fixtures
// ===========================================================================

// do_leaf
//   type: a custom leaf carrying BOTH a member encode_into and a static decode,
// each moving a 4-byte big-endian payload by hand.  A member surface owns its
// own byte layout, so it is written and read identically under either byte order
// - the fixture proves the option does not reach it, and drives the round trip.
struct do_leaf
{
    std::uint32_t payload;

    do_leaf()
        : payload(0)
    {}

    explicit do_leaf(std::uint32_t _p)
        : payload(_p)
    {}

    template<typename _Sink>
    void encode_into(_Sink& _sink) const
    {
        _sink.push_back(static_cast<byte>((payload >> 24) & 0xFFu));
        _sink.push_back(static_cast<byte>((payload >> 16) & 0xFFu));
        _sink.push_back(static_cast<byte>((payload >>  8) & 0xFFu));
        _sink.push_back(static_cast<byte>((payload >>  0) & 0xFFu));

        return;
    }

    static decode_result<do_leaf> decode(byte_reader& _reader)
    {
        byte _b[4] = {0, 0, 0, 0};

        if (!_reader.take(_b, 4))
        {
            return decode_failure<do_leaf>();
        }

        const std::uint32_t _v =
            ( static_cast<std::uint32_t>(_b[0]) << 24 )
          | ( static_cast<std::uint32_t>(_b[1]) << 16 )
          | ( static_cast<std::uint32_t>(_b[2]) <<  8 )
          | ( static_cast<std::uint32_t>(_b[3]) <<  0 );

        return decode_success(do_leaf(_v));
    }
};

// do_enum_u16 / do_enum_i32
//   type: enumerations with fixed underlying types, so their byte order under
// each serial_endian is deterministic across platforms.
enum class do_enum_u16 : std::uint16_t
{
    value = 0x1234
};

enum class do_enum_i32 : std::int32_t
{
    value = -123456
};

#endif  // !DTEST_SPEC_MODE


// ===========================================================================
// Declarations  (grouped by the section of decode_options.hpp they exercise)
// ===========================================================================

// I.   parameterised byte readers (direct internal coverage)
bool tests_decode_options_get_uint();
bool tests_decode_options_get_varint();
bool tests_decode_options_get_length();

// II.  dec_tau<E> leaf families, both byte orders
bool tests_decode_options_leaf_bool();
bool tests_decode_options_leaf_integrals();
bool tests_decode_options_leaf_enums();
bool tests_decode_options_leaf_floating();
bool tests_decode_options_leaf_member();
bool tests_decode_options_leaf_failures();
bool tests_decode_options_big_matches_decode();

// round trip: encode_leaf<E> <-> decode_leaf<E>, and put_length <-> get_length
bool tests_decode_options_roundtrip_scalars();
bool tests_decode_options_roundtrip_both_orders();
bool tests_decode_options_roundtrip_length();

// III. convenience: decode_leaf<E>(byte_string)
bool tests_decode_options_convenience();

// ===========================================================================
// SPEC PROVIDER   (spec face - the runner hands this to run_module)
// ===========================================================================
#ifdef DTEST_SPEC_MODE
inline ::djinterp::test::module_spec
decode_options_spec()
{
    namespace dt = ::djinterp::test;
    namespace tk = ::djinterp::testing;
    return dt::module_spec{
    "decode_options",
    "Unit tests for decode_options.hpp - the parameterised decoder dec_tau<E>: "
    "the internal parameterised readers (get_uint<E>, the LEB128 get_varint with "
    "its overlong / truncated rejection, and get_length<L,E>) exercised directly, "
    "the five leaf families under both byte orders with their failure paths, the "
    "proof that the big-endian instance reproduces decode.hpp, the encode_leaf<E> "
    "/ decode_leaf<E> and put_length / get_length round trips, and the "
    "decode_leaf<E> convenience.",
    {
        // -- I. the parameterised byte readers ----------------------------------
        dt::block_spec{
            "readers",
            "The internal parameterised readers, reached by direct call: "
            "get_uint<E> with its width guard, the LEB128 get_varint with overlong "
            "and truncated rejection, and get_length<L,E>.",
            {
                { "tests_decode_options_get_uint",
                  "get_uint<E> reassembles a fixed-width value in the chosen byte order, refuses "
                  "a width beyond eight without consuming, and fails cleanly on underflow",
                  &tk::tests_decode_options_get_uint },
                { "tests_decode_options_get_varint",
                  "get_varint reads unsigned LEB128 - single and multi-group values and the full "
                  "64-bit maximum - and rejects a truncated or overlong encoding",
                  &tk::tests_decode_options_get_varint },
                { "tests_decode_options_get_length",
                  "get_length<L,E> reads a count per the length policy: a fixed 2/4/8-byte field "
                  "in byte order E, or a self-delimiting varint",
                  &tk::tests_decode_options_get_length },
            }
        },

        // -- II. the leaf families under both orders ----------------------------
        dt::block_spec{
            "leaves",
            "The five dec_tau<E> leaf families under both byte orders, their "
            "underflow failure paths, a member leaf that owns its layout, and the "
            "big-endian instance matching decode.hpp.",
            {
                { "tests_decode_options_leaf_bool",
                  "the bool leaf reads one order-independent byte",
                  &tk::tests_decode_options_leaf_bool },
                { "tests_decode_options_leaf_integrals",
                  "integrals read at their natural width from either byte order to the same value",
                  &tk::tests_decode_options_leaf_integrals },
                { "tests_decode_options_leaf_enums",
                  "an enum reads through its underlying integer from either byte order",
                  &tk::tests_decode_options_leaf_enums },
                { "tests_decode_options_leaf_floating",
                  "float and double read from their IEEE-754 fields under either byte order",
                  &tk::tests_decode_options_leaf_floating },
                { "tests_decode_options_leaf_member",
                  "a custom member leaf reads its own layout identically under either order (the "
                  "option does not reach it)",
                  &tk::tests_decode_options_leaf_member },
                { "tests_decode_options_leaf_failures",
                  "each family fails with ok = false and a bad reader when the input underflows, "
                  "under either order",
                  &tk::tests_decode_options_leaf_failures },
                { "tests_decode_options_big_matches_decode",
                  "decode_leaf<big> reproduces decode.hpp's fixed dec_tau result across the leaf "
                  "families",
                  &tk::tests_decode_options_big_matches_decode },
            }
        },

        // -- III. the round trip ------------------------------------------------
        dt::block_spec{
            "roundtrip",
            "The parameterised round trip: encode_leaf<E> then decode_leaf<E> "
            "recovers a value under each order, and put_length then get_length "
            "recovers a count under every length policy.",
            {
                { "tests_decode_options_roundtrip_scalars",
                  "under big-endian, every leaf family (bool, integral, enum, floating, member) "
                  "recovers its value",
                  &tk::tests_decode_options_roundtrip_scalars },
                { "tests_decode_options_roundtrip_both_orders",
                  "the round trip holds under BOTH byte orders for each scalar family, including "
                  "boundary integral values",
                  &tk::tests_decode_options_roundtrip_both_orders },
                { "tests_decode_options_roundtrip_length",
                  "a leading count survives put_length -> get_length under every policy (u16 / u32 "
                  "/ u64 both orders, and varint) across magnitudes",
                  &tk::tests_decode_options_roundtrip_length },
            }
        },

        // -- IV. the convenience wrapper ----------------------------------------
        dt::block_spec{
            "convenience",
            "decode_leaf<E>(byte_string): the wrapper that reads one leaf from the "
            "front of a whole buffer in the chosen byte order.",
            {
                { "tests_decode_options_convenience",
                  "a whole-buffer decode returns the leading value in the chosen order, consuming "
                  "only the leading leaf and failing on a short buffer",
                  &tk::tests_decode_options_convenience },
            }
        },
    }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_BINARY_DECODE_OPTIONS_TESTS_
