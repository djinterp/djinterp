/*******************************************************************************
* djinterp [binary/test]                                        encode_tests.hpp
*
*   Declarations, fixtures, and the per-suite assertion macro for the encode.hpp
* unit tests.  encode.hpp owns the binary medium (byte / byte_string), the leaf
* element encoder enc_tau (encode_into) for the built-in leaf families and the
* member customization surface, the convenience total map (encode), and the
* leaf-encodability trait (is_leaf_encodable).  Every tests_* predicate here is
* authored flat in djinterp::testing, returns true iff all its checks passed,
* and is referenced by address from the runner spec.
*
*   The section .cpp files that DEFINE these predicates mirror encode.hpp's own
* section structure:
*     encode_tests_medium.cpp        - I.        the medium (byte / byte_string)
*     encode_tests_detection.cpp     - II, III.  sink probe + member probe
*     encode_tests_leaves.cpp        - IV, V.    fixed-width writers + enc_tau
*     encode_tests_convenience.cpp   - VI.       encode(value) -> B*
*     encode_tests_encodability.cpp  - VII.      is_leaf_encodable (+ _v)
*
*
* path:      /tests/djinterp/core/binary/encode_tests.hpp
* link(s):   TBA
* author(s): unit-test generator                             created: 2026.07.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_BINARY_ENCODE_TESTS_
#define DJINTERP_BINARY_ENCODE_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
// djinterp
#ifdef DTEST_SPEC_MODE
#include <djinterp/test/test_defaults.hpp>
#endif  // DTEST_SPEC_MODE
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/binary/encode.hpp"   // the module under test (bare include; -I inc/djinterp/core/binary)
#endif  // !DTEST_SPEC_MODE


// D_EN_CHECK
//   macro: fail the current test - return false from the enclosing predicate -
// unless the condition holds.  Variadic so a condition may contain top-level
// commas (template argument lists, brace-init).  Unique letters (EN) so
// co-compiled suites never collide on the macro name.
#define D_EN_CHECK(...)                                                       \
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

// recording_sink
//   type: a minimal byte sink - it exposes exactly the one surface encode.hpp
// requires (push_back(byte)) and nothing else, so it proves enc_tau writes
// through any push_back sink, not merely the default byte_string.
struct recording_sink
{
    byte_string data;

    void push_back(byte _b)
    {
        data.push_back(_b);

        return;
    }
};

// not_a_sink
//   type: a type WITHOUT a push_back(byte) surface, the negative fixture for
// has_push_back_byte.
struct not_a_sink
{
    int unrelated;
};

// enc_custom_leaf
//   type: a non-built-in leaf that supplies its own enc_tau through a member
// encode_into(sink) - the customization surface.  It writes a fixed three-byte
// signature so its output is unambiguous.  The member is const-qualified, as
// has_member_encode_into probes a const object.
struct enc_custom_leaf
{
    template<typename _Sink>
    void encode_into(_Sink& _sink) const
    {
        _sink.push_back(static_cast<byte>(0xAB));
        _sink.push_back(static_cast<byte>(0xCD));
        _sink.push_back(static_cast<byte>(0xEF));

        return;
    }
};

// enc_plain
//   type: an ordinary aggregate with NO encode_into and matching no built-in
// leaf family - the negative fixture for is_leaf_encodable / member detection.
struct enc_plain
{
    int    a;
    double b;
};

// enc_enum_u16 / enc_enum_i32 / enc_enum_i8 / enc_enum_unscoped
//   type: enumerations with FIXED underlying types, so their width and
// signedness (and therefore their exact encoding) are portable and
// deterministic across platforms.
enum class enc_enum_u16 : std::uint16_t
{
    value = 0x1234
};

enum class enc_enum_i32 : std::int32_t
{
    value = -123456
};

enum class enc_enum_i8 : std::int8_t
{
    neg = -2,
    pos = 5
};

enum enc_enum_unscoped : std::uint8_t
{
    EU_A = 0x2A
};

#endif  // !DTEST_SPEC_MODE


// ===========================================================================
// Declarations  (grouped by the section of encode.hpp they exercise)
// ===========================================================================

// I.   the medium (byte / byte_string / encode_length_type)
bool tests_encode_medium_types();

// II.  byte-sink detection (has_push_back_byte + _v)
// III. member-encoder detection (has_member_encode_into)
bool tests_encode_has_push_back_byte();
bool tests_encode_has_member_encode_into();

// IV.  fixed-width writers  /  V. enc_tau leaf families
bool tests_encode_bool();
bool tests_encode_unsigned_integrals();
bool tests_encode_signed_integrals();
bool tests_encode_char_types();
bool tests_encode_enums();
bool tests_encode_floating();
bool tests_encode_member_surface();
bool tests_encode_writer_accumulation();

// VI.  convenience: encode(value) -> byte_string
bool tests_encode_convenience();

// VII. leaf-encodability trait (is_leaf_encodable + _v)
bool tests_encode_is_leaf_encodable();
bool tests_encode_is_leaf_encodable_v();

// ===========================================================================
// SPEC PROVIDER   (spec face - the runner hands this to run_module)
// ===========================================================================
#ifdef DTEST_SPEC_MODE
inline ::djinterp::test::module_spec
encode_spec()
{
    namespace dt = ::djinterp::test;
    namespace tk = ::djinterp::testing;
    return dt::module_spec{
    "encode",
    "Unit tests for encode.hpp - the fixed big-endian leaf encoder enc_tau: its "
    "public byte vocabulary and the two sink/member traits it dispatches on, the "
    "five mutually-exclusive encode_into leaf families (member, bool, integral, "
    "enum, floating) with their two's-complement and IEEE-754 byte patterns, the "
    "encode() convenience that materialises a fresh byte_string, and the "
    "is_leaf_encodable surface.",
    {
        // -- I. medium (public vocabulary) --------------------------------------
        dt::block_spec{
            "medium",
            "The serialization medium: the byte / byte_string / encode_length_type "
            "vocabulary the encoder reads and writes through.",
            {
                { "tests_encode_medium_types",
                  "byte is an 8-bit unsigned, byte_string is a contiguous byte sequence, and "
                  "encode_length_type is the documented 64-bit width",
                  &tk::tests_encode_medium_types },
            }
        },

        // -- II. trait detection ------------------------------------------------
        dt::block_spec{
            "detection",
            "The two structural traits enc_tau dispatches on: a push_back-able byte "
            "sink, and a type carrying its own member encode_into.",
            {
                { "tests_encode_has_push_back_byte",
                  "has_push_back_byte is true for a byte sink exposing push_back(byte) and false "
                  "for a type without it",
                  &tk::tests_encode_has_push_back_byte },
                { "tests_encode_has_member_encode_into",
                  "has_member_encode_into detects a type that serialises itself through a member "
                  "encode_into, and rejects one that does not",
                  &tk::tests_encode_has_member_encode_into },
            }
        },

        // -- III. the leaf families ---------------------------------------------
        dt::block_spec{
            "leaves",
            "The five mutually-exclusive encode_into leaf families and the byte "
            "patterns each writes: member, bool, integral (signed / unsigned / "
            "char), enum, and IEEE-754 floating, plus multi-value accumulation.",
            {
                { "tests_encode_bool",
                  "a bool encodes to a single 0x01 / 0x00 byte",
                  &tk::tests_encode_bool },
                { "tests_encode_unsigned_integrals",
                  "unsigned integrals encode big-endian at their fixed width across 8/16/32/64 "
                  "bits, including boundary values",
                  &tk::tests_encode_unsigned_integrals },
                { "tests_encode_signed_integrals",
                  "signed integrals encode as fixed-width big-endian two's-complement, including "
                  "the negative extremes",
                  &tk::tests_encode_signed_integrals },
                { "tests_encode_char_types",
                  "the character types encode as their underlying one-byte integer value",
                  &tk::tests_encode_char_types },
                { "tests_encode_enums",
                  "an enum encodes as its fixed underlying integer, big-endian",
                  &tk::tests_encode_enums },
                { "tests_encode_floating",
                  "float and double encode as their 4- and 8-byte big-endian IEEE-754 bit patterns",
                  &tk::tests_encode_floating },
                { "tests_encode_member_surface",
                  "a type with a member encode_into is serialised through that member, which owns "
                  "its byte layout",
                  &tk::tests_encode_member_surface },
                { "tests_encode_writer_accumulation",
                  "successive encode_into calls append to the same sink, concatenating their byte "
                  "fields in order",
                  &tk::tests_encode_writer_accumulation },
            }
        },

        // -- IV. the convenience wrapper ----------------------------------------
        dt::block_spec{
            "convenience",
            "encode(value): the wrapper that runs enc_tau into a fresh "
            "byte_string and returns it.",
            {
                { "tests_encode_convenience",
                  "encode(value) returns exactly the bytes encode_into would append, in a freshly "
                  "allocated buffer each call",
                  &tk::tests_encode_convenience },
            }
        },

        // -- V. leaf-encodability trait -----------------------------------------
        dt::block_spec{
            "encodability",
            "is_leaf_encodable: the trait (and its _v companion) recognising the "
            "types enc_tau can encode.",
            {
                { "tests_encode_is_leaf_encodable",
                  "is_leaf_encodable is true for every leaf family and false for an unencodable "
                  "type",
                  &tk::tests_encode_is_leaf_encodable },
                { "tests_encode_is_leaf_encodable_v",
                  "the is_leaf_encodable_v variable template mirrors the trait's value",
                  &tk::tests_encode_is_leaf_encodable_v },
            }
        },
    }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_BINARY_ENCODE_TESTS_
