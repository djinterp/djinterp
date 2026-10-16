/*******************************************************************************
* djinterp [binary/test]                                encode_options_tests.hpp
*
*   Declarations, fixtures, and the per-suite assertion macro for the
* encode_options.hpp unit tests.  encode_options.hpp owns the serial parameter
* enums (serial_endian / serial_length / serial_fidelity) and their defaults, the
* parameterised byte writers (put_uint<E> / put_varint / put_length<L,E>), the
* parameterised leaf encoder enc_tau<E> (encode_leaf_into), and its convenience
* total map (encode_leaf<E>).  The trait and sink machinery is reused verbatim
* from encode.hpp and is exercised by the encode suite; here the focus is the two
* open degrees of freedom (byte order and length encoding).  Every tests_*
* predicate is authored flat in djinterp::testing and referenced by address from
* the runner spec.
*
*   The section .cpp files that DEFINE these predicates mirror encode_options.hpp:
*     encode_options_tests_enums.cpp        - I.   serial parameter enums + defaults
*     encode_options_tests_writers.cpp      - II.  put_uint<E> / put_varint / put_length
*     encode_options_tests_leaves.cpp       - III. enc_tau<E> leaf families, both orders
*     encode_options_tests_convenience.cpp  - IV.  encode_leaf<E> total map
*
*
* path:      /tests/djinterp/core/binary/encode_options_tests.hpp
* link(s):   TBA
* author(s): unit-test generator                             created: 2026.07.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_BINARY_ENCODE_OPTIONS_TESTS_
#define DJINTERP_BINARY_ENCODE_OPTIONS_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
// djinterp
#ifdef DTEST_SPEC_MODE
#include <djinterp/test/test_defaults.hpp>
#endif  // DTEST_SPEC_MODE
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/binary/encode_options.hpp"   // the module under test (also pulls encode.hpp)
#endif  // !DTEST_SPEC_MODE


// D_EO_CHECK
//   macro: fail the current test - return false from the enclosing predicate -
// unless the condition holds.  Variadic so a condition may contain top-level
// commas (template argument lists, brace-init).  Unique letters (EO) so
// co-compiled suites never collide on the macro name.
#define D_EO_CHECK(...)                                                       \
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

// eo_sink
//   type: a minimal byte sink (push_back(byte) only), proving the parameterised
// writers and enc_tau<E> write through any push_back sink, not only byte_string.
struct eo_sink
{
    byte_string data;

    void push_back(byte _b)
    {
        data.push_back(_b);

        return;
    }
};

// eo_custom_leaf
//   type: a non-built-in leaf carrying its own encode_into - its byte layout is
// its own, so enc_tau<E> must write it identically under either byte order (the
// option governs only the built-in scalars).
struct eo_custom_leaf
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

// eo_enum_u16 / eo_enum_i32
//   type: enumerations with fixed underlying types, so their width and byte
// order under each serial_endian are deterministic across platforms.
enum class eo_enum_u16 : std::uint16_t
{
    value = 0x1234
};

enum class eo_enum_i32 : std::int32_t
{
    value = -123456
};

#endif  // !DTEST_SPEC_MODE


// ===========================================================================
// Declarations  (grouped by the section of encode_options.hpp they exercise)
// ===========================================================================

// I.   serial parameter enums + default_* constants
bool tests_encode_options_enums();
bool tests_encode_options_defaults();

// II.  parameterised byte writers (direct internal coverage)
bool tests_encode_options_put_uint();
bool tests_encode_options_put_varint();
bool tests_encode_options_put_length();

// III. enc_tau<E> leaf families, both byte orders
bool tests_encode_options_leaf_bool();
bool tests_encode_options_leaf_integrals();
bool tests_encode_options_leaf_enums();
bool tests_encode_options_leaf_floating();
bool tests_encode_options_leaf_member();
bool tests_encode_options_big_matches_encode();

// IV.  convenience: encode_leaf<E> total map
bool tests_encode_options_convenience();

// ===========================================================================
// SPEC PROVIDER   (spec face - the runner hands this to run_module)
// ===========================================================================
#ifdef DTEST_SPEC_MODE
inline ::djinterp::test::module_spec
encode_options_spec()
{
    namespace dt = ::djinterp::test;
    namespace tk = ::djinterp::testing;
    return dt::module_spec{
    "encode_options",
    "Unit tests for encode_options.hpp - the parameterised encoder enc_tau<E>: "
    "the serial parameter enums and their defaults, the internal byte writers "
    "(put_uint<E>, the LEB128 put_varint, and put_length<L,E>) exercised directly "
    "since the leaf path does not reach them, the five leaf families under both "
    "byte orders, the proof that the big-endian instance reproduces encode.hpp, "
    "and the encode_leaf<E> convenience.",
    {
        // -- I. the serial parameter enums --------------------------------------
        dt::block_spec{
            "enums",
            "The serial parameter enums (serial_endian / serial_length / "
            "serial_fidelity) and the documented default_* constants.",
            {
                { "tests_encode_options_enums",
                  "the three serial parameter enums expose their documented enumerators as "
                  "distinct values",
                  &tk::tests_encode_options_enums },
                { "tests_encode_options_defaults",
                  "default_endian / default_length / default_fidelity carry the documented "
                  "big-endian / u64 / native defaults",
                  &tk::tests_encode_options_defaults },
            }
        },

        // -- II. the parameterised byte writers ---------------------------------
        dt::block_spec{
            "writers",
            "The internal parameterised writers, reached by direct call because the "
            "leaf path does not use them: put_uint<E>, the LEB128 put_varint, and "
            "put_length<L,E>, including width overflow.",
            {
                { "tests_encode_options_put_uint",
                  "put_uint<E> writes a fixed-width value most- or least-significant byte first, "
                  "and truncates to the requested width on overflow",
                  &tk::tests_encode_options_put_uint },
                { "tests_encode_options_put_varint",
                  "put_varint emits unsigned LEB128 - seven bits per byte, low group first, "
                  "continuation bit set until the last",
                  &tk::tests_encode_options_put_varint },
                { "tests_encode_options_put_length",
                  "put_length<L,E> writes a count per the length policy: a fixed 2/4/8-byte field "
                  "in byte order E, or a self-delimiting varint",
                  &tk::tests_encode_options_put_length },
            }
        },

        // -- III. the leaf families under both orders ---------------------------
        dt::block_spec{
            "leaves",
            "The five enc_tau<E> leaf families under both byte orders: bool, "
            "integral, enum, and floating reverse between big and little; a member "
            "leaf owns its layout; and the big-endian instance equals encode.hpp.",
            {
                { "tests_encode_options_leaf_bool",
                  "the bool leaf is a single order-independent 0x01 / 0x00 byte",
                  &tk::tests_encode_options_leaf_bool },
                { "tests_encode_options_leaf_integrals",
                  "integrals write at their natural width, the byte field reversing between big "
                  "and little endian",
                  &tk::tests_encode_options_leaf_integrals },
                { "tests_encode_options_leaf_enums",
                  "an enum writes as its underlying integer, reversing between the two byte orders",
                  &tk::tests_encode_options_leaf_enums },
                { "tests_encode_options_leaf_floating",
                  "float and double write their IEEE-754 bit patterns, reversing between the two "
                  "byte orders",
                  &tk::tests_encode_options_leaf_floating },
                { "tests_encode_options_leaf_member",
                  "a custom member leaf writes its own byte layout identically under either order "
                  "(the option does not reach it)",
                  &tk::tests_encode_options_leaf_member },
                { "tests_encode_options_big_matches_encode",
                  "encode_leaf<big> reproduces encode.hpp's fixed enc_tau byte for byte across the "
                  "leaf families",
                  &tk::tests_encode_options_big_matches_encode },
            }
        },

        // -- IV. the convenience wrapper ----------------------------------------
        dt::block_spec{
            "convenience",
            "encode_leaf<E>(value): the wrapper that runs enc_tau<E> into a fresh "
            "byte_string carrying the chosen byte order.",
            {
                { "tests_encode_options_convenience",
                  "encode_leaf<E> returns exactly what encode_leaf_into<E> would append, fresh "
                  "each call, and the two orders differ in the produced bytes",
                  &tk::tests_encode_options_convenience },
            }
        },
    }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_BINARY_ENCODE_OPTIONS_TESTS_
