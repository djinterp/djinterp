/*******************************************************************************
* djinterp [binary/test]                                        decode_tests.hpp
*
*   Declarations, fixtures, and the per-suite assertion macro for the decode.hpp
* unit tests.  decode.hpp owns the reader over a bit string (byte_reader), the
* partial-result type a decode yields (decode_result, with decode_success /
* decode_failure), the member-decode customization surface, the leaf element
* decoder dec_tau (decode) for the built-in families, and the leaf-decodability
* trait (is_leaf_decodable).  Every tests_* predicate here is authored flat in
* djinterp::testing, returns true iff all its checks passed, and is referenced by
* address from the runner spec.
*
*   The section .cpp files that DEFINE these predicates mirror decode.hpp's own
* section structure, with a round-trip section that pairs decode against encode:
*     decode_tests_reader.cpp        - II.       byte_reader + get_uint_be
*     decode_tests_result.cpp        - III, IV.  decode_result + member probe
*     decode_tests_leaves.cpp        - V, VI.    leaf readers + dec_tau families
*     decode_tests_roundtrip.cpp     - the encode <-> decode identity (adds encode.hpp)
*     decode_tests_decodability.cpp  - VII.      is_leaf_decodable (+ _v)
*
*
* path:      /tests/djinterp/core/binary/decode_tests.hpp
* link(s):   TBA
* author(s): unit-test generator                             created: 2026.07.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_BINARY_DECODE_TESTS_
#define DJINTERP_BINARY_DECODE_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
// djinterp
#ifdef DTEST_SPEC_MODE
#include <djinterp/test/test_defaults.hpp>
#endif  // DTEST_SPEC_MODE
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/binary/decode.hpp"   // the module under test (bare include; -I inc/djinterp/core/binary)
#endif  // !DTEST_SPEC_MODE


// D_DE_CHECK
//   macro: fail the current test - return false from the enclosing predicate -
// unless the condition holds.  Variadic so a condition may contain top-level
// commas (template argument lists, brace-init).  Unique letters (DE) so
// co-compiled suites never collide on the macro name.
#define D_DE_CHECK(...)                                                       \
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

// rt_leaf
//   type: a custom leaf carrying BOTH surfaces - a member encode_into and a
// static decode - so it exercises the member decode path and the encode<->decode
// round trip.  Both surfaces move a 4-byte big-endian payload by hand (no
// dependency on encode.hpp), so the fixture is self-contained in every section.
struct rt_leaf
{
    std::uint32_t payload;

    rt_leaf()
        : payload(0)
    {}

    explicit rt_leaf(std::uint32_t _p)
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

    static decode_result<rt_leaf> decode(byte_reader& _reader)
    {
        byte _b[4] = {0, 0, 0, 0};

        if (!_reader.take(_b, 4))
        {
            return decode_failure<rt_leaf>();
        }

        const std::uint32_t _v =
            ( static_cast<std::uint32_t>(_b[0]) << 24 )
          | ( static_cast<std::uint32_t>(_b[1]) << 16 )
          | ( static_cast<std::uint32_t>(_b[2]) <<  8 )
          | ( static_cast<std::uint32_t>(_b[3]) <<  0 );

        return decode_success(rt_leaf(_v));
    }
};

// dec_no_decode
//   type: a plain aggregate with no static decode and matching no built-in leaf
// family - the negative fixture for has_member_decode / is_leaf_decodable.
struct dec_no_decode
{
    int unrelated;
};

// dec_wrong_decode
//   type: a type with a static decode of the WRONG return type.  The member
// probe finds the name but the return type is not decode_result<dec_wrong_decode>,
// so has_member_decode must report false - the negative that exercises the
// return-type check, not merely the name check.
struct dec_wrong_decode
{
    static bool decode(byte_reader&)
    {
        return true;
    }
};

// dec_enum_i32 / dec_enum_u16
//   type: enumerations with fixed underlying types, so a decoded byte field maps
// back to a deterministic enumerator across platforms.
enum class dec_enum_i32 : std::int32_t
{
    value = -123456
};

enum class dec_enum_u16 : std::uint16_t
{
    value = 0x1234
};

#endif  // !DTEST_SPEC_MODE


// ===========================================================================
// Declarations  (grouped by the section of decode.hpp they exercise)
// ===========================================================================

// II.  byte_reader (construction, observation, consumption) + get_uint_be
bool tests_decode_reader_construction();
bool tests_decode_reader_take();
bool tests_decode_reader_sticky_bad();
bool tests_decode_get_uint_be();

// III. decode_result + decode_success / decode_failure
// IV.  member-decode detection (has_member_decode)
bool tests_decode_result_type();
bool tests_decode_success_failure();
bool tests_decode_has_member_decode();

// V.   fixed-width readers  /  VI. dec_tau leaf families
bool tests_decode_bool();
bool tests_decode_unsigned_integrals();
bool tests_decode_signed_integrals();
bool tests_decode_enums();
bool tests_decode_floating();
bool tests_decode_member_surface();
bool tests_decode_failure_paths();
bool tests_decode_byte_string_convenience();

// round trip: encode.hpp <-> decode.hpp (identity through the medium)
bool tests_decode_roundtrip_integrals();
bool tests_decode_roundtrip_floating();
bool tests_decode_roundtrip_bool_enum();
bool tests_decode_roundtrip_member();

// VII. leaf-decodability trait (is_leaf_decodable + _v)
bool tests_decode_is_leaf_decodable();
bool tests_decode_is_leaf_decodable_v();

// ===========================================================================
// SPEC PROVIDER   (spec face - the runner hands this to run_module)
// ===========================================================================
#ifdef DTEST_SPEC_MODE
inline ::djinterp::test::module_spec
decode_spec()
{
    namespace dt = ::djinterp::test;
    namespace tk = ::djinterp::testing;
    return dt::module_spec{
    "decode",
    "Unit tests for decode.hpp - the fixed big-endian leaf decoder dec_tau: the "
    "byte_reader cursor with its sticky-bad underflow discipline, decode_result "
    "and its success / failure factories, the has_member_decode trait with its "
    "exact-return-type requirement, the five leaf families, the byte_string "
    "convenience, the encode/decode round trip, and the is_leaf_decodable surface.",
    {
        // -- I. the reader ------------------------------------------------------
        dt::block_spec{
            "reader",
            "byte_reader: construction from the three sources, take() with its "
            "sticky-bad underflow rule and always-succeeding zero-length take, and "
            "the internal get_uint_be width guard.",
            {
                { "tests_decode_reader_construction",
                  "a reader is constructible default, from a (pointer, size) pair, and from a "
                  "byte_string, reporting the right remaining count",
                  &tk::tests_decode_reader_construction },
                { "tests_decode_reader_take",
                  "take() copies out the requested bytes, advances the cursor, and a zero-length "
                  "take always succeeds while the reader is good",
                  &tk::tests_decode_reader_take },
                { "tests_decode_reader_sticky_bad",
                  "an underflowing take fails and leaves the reader permanently bad - subsequent "
                  "takes fail even when they would fit",
                  &tk::tests_decode_reader_sticky_bad },
                { "tests_decode_get_uint_be",
                  "the internal get_uint_be reassembles a big-endian fixed-width value, and a "
                  "width beyond eight is refused without consuming input",
                  &tk::tests_decode_get_uint_be },
            }
        },

        // -- II. the result type ------------------------------------------------
        dt::block_spec{
            "result",
            "decode_result<T> and its factories, and the has_member_decode trait "
            "with its exact decode_result return-type requirement.",
            {
                { "tests_decode_result_type",
                  "decode_result carries an ok flag beside a value, default-constructing to the "
                  "not-ok state",
                  &tk::tests_decode_result_type },
                { "tests_decode_success_failure",
                  "decode_success wraps a value with ok = true and decode_failure yields ok = "
                  "false",
                  &tk::tests_decode_success_failure },
                { "tests_decode_has_member_decode",
                  "has_member_decode is true only for a static T::decode returning exactly "
                  "decode_result<T> - a wrong return type reads as false",
                  &tk::tests_decode_has_member_decode },
            }
        },

        // -- III. the leaf families ---------------------------------------------
        dt::block_spec{
            "leaves",
            "The five leaf families dec_tau reads (member, bool, integral, enum, "
            "floating), their failure paths on underflow, and the whole-buffer "
            "byte_string convenience.",
            {
                { "tests_decode_bool",
                  "a bool reads from one byte, any non-zero decoding to true",
                  &tk::tests_decode_bool },
                { "tests_decode_unsigned_integrals",
                  "unsigned integrals read big-endian at their fixed width across 8/16/32/64 bits",
                  &tk::tests_decode_unsigned_integrals },
                { "tests_decode_signed_integrals",
                  "signed integrals read as fixed-width big-endian two's-complement, including the "
                  "negative extremes",
                  &tk::tests_decode_signed_integrals },
                { "tests_decode_enums",
                  "an enum reads through its fixed underlying integer",
                  &tk::tests_decode_enums },
                { "tests_decode_floating",
                  "float and double read from their 4- and 8-byte big-endian IEEE-754 fields",
                  &tk::tests_decode_floating },
                { "tests_decode_member_surface",
                  "a type with a static decode is read through that member, which owns its byte "
                  "layout",
                  &tk::tests_decode_member_surface },
                { "tests_decode_failure_paths",
                  "each family fails with ok = false and a bad reader when the input underflows",
                  &tk::tests_decode_failure_paths },
                { "tests_decode_byte_string_convenience",
                  "decode over a whole byte_string reads the leading leaf and reports success or "
                  "failure",
                  &tk::tests_decode_byte_string_convenience },
            }
        },

        // -- IV. the round trip -------------------------------------------------
        dt::block_spec{
            "roundtrip",
            "The encode/decode identity: a value encoded by enc_tau and read back "
            "by dec_tau recovers itself, across every leaf family.",
            {
                { "tests_decode_roundtrip_integrals",
                  "integral values (signed / unsigned, several widths, boundaries) survive an "
                  "encode-then-decode round trip",
                  &tk::tests_decode_roundtrip_integrals },
                { "tests_decode_roundtrip_floating",
                  "float and double values survive the round trip bit-for-bit",
                  &tk::tests_decode_roundtrip_floating },
                { "tests_decode_roundtrip_bool_enum",
                  "bool and enum values survive the round trip",
                  &tk::tests_decode_roundtrip_bool_enum },
                { "tests_decode_roundtrip_member",
                  "a custom type with member encode_into and static decode survives the round "
                  "trip through its own layout",
                  &tk::tests_decode_roundtrip_member },
            }
        },

        // -- V. leaf-decodability trait -----------------------------------------
        dt::block_spec{
            "decodability",
            "is_leaf_decodable: the trait (and its _v companion) recognising the "
            "types dec_tau can decode.",
            {
                { "tests_decode_is_leaf_decodable",
                  "is_leaf_decodable is true for every leaf family and false for an undecodable "
                  "type",
                  &tk::tests_decode_is_leaf_decodable },
                { "tests_decode_is_leaf_decodable_v",
                  "the is_leaf_decodable_v variable template mirrors the trait's value",
                  &tk::tests_decode_is_leaf_decodable_v },
            }
        },
    }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_BINARY_DECODE_TESTS_
