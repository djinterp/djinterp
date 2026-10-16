/*******************************************************************************
* djinterp [test]                               option_generator_tests_parse.cpp
*
*   Section II - the compile-time stream parser.  parse_stream folds a flat
* interleaved NTTP stream into a std::tuple of specs, using four productions
* made mutually exclusive by requires-clauses: exhausted (empty), a trailing
* lone key (unary), a key followed by a non-key value (valued), and a key
* followed by another key (the first is unary).  These tests pin down each
* production and, in particular, STRUCTURAL UNARY DETECTION: a key is unary
* exactly when the next token is another key or the stream ends - including
* mid-stream, between valued pairs.  C++20-gated.
*
*
* path:      /tests/djinterp/core/option/option_generator_tests_parse.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./option_generator_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// parsed
//   helper: run the parser over a stream, seeded with setting as the key type
// and an empty accumulator - exactly as option_generator drives it.
template<auto... _Args>
using parsed =
    typename ::djinterp::internal::parse_stream<setting, std::tuple<>, _Args...>::type;
#endif


::djinterp::test::block_spec
option_generator_parse_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "II.  stream parse";
    block.descriptor = "parse_stream productions + structural unary detection";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    namespace osi = ::djinterp::internal;

    // ---- base productions ------------------------------------------------

    block.tests.push_back({
        "parse_empty_stream",
        "an empty stream parses to no specs",
        []() -> bool
        {
            constexpr bool ok = same_v< parsed<>, std::tuple<> >;
            static_assert(ok, "parse<> == tuple<>");
            return ok;
        } });

    block.tests.push_back({
        "parse_single_key_is_unary",
        "a lone trailing key becomes a unary spec",
        []() -> bool
        {
            constexpr bool ok = same_v<
                parsed<setting::visible>,
                std::tuple< osi::k_spec<setting::visible> > >;
            static_assert(ok, "parse<visible> == <k_spec<visible>>");
            return ok;
        } });

    block.tests.push_back({
        "parse_key_value_is_valued",
        "a key followed by a non-key value becomes a valued spec",
        []() -> bool
        {
            constexpr bool ok = same_v<
                parsed<setting::width, 1024>,
                std::tuple< osi::kv_spec<setting::width, 1024> > >;
            static_assert(ok, "parse<width,1024> == <kv_spec<width,1024>>");
            return ok;
        } });

    // ---- sequences of valued pairs ---------------------------------------

    block.tests.push_back({
        "parse_two_valued_pairs",
        "consecutive key/value pairs parse in order",
        []() -> bool
        {
            constexpr bool ok = same_v<
                parsed<setting::width, 1024, setting::height, 768>,
                std::tuple< osi::kv_spec<setting::width, 1024>,
                            osi::kv_spec<setting::height, 768> > >;
            static_assert(ok, "parse<width,1024,height,768>");
            return ok;
        } });

    block.tests.push_back({
        "parse_all_valued",
        "a fully valued stream yields all kv_specs",
        []() -> bool
        {
            constexpr bool ok = same_v<
                parsed<setting::width, 1, setting::height, 2, setting::depth, 3>,
                std::tuple< osi::kv_spec<setting::width, 1>,
                            osi::kv_spec<setting::height, 2>,
                            osi::kv_spec<setting::depth, 3> > >;
            static_assert(ok, "parse<...all valued...>");
            return ok;
        } });

    block.tests.push_back({
        "parse_non_key_value_types",
        "bool and char values are non-key, so they read as values",
        []() -> bool
        {
            constexpr bool ok = same_v<
                parsed<setting::width, true, setting::height, 'z'>,
                std::tuple< osi::kv_spec<setting::width, true>,
                            osi::kv_spec<setting::height, 'z'> > >;
            static_assert(ok, "parse<width,true,height,'z'>");
            return ok;
        } });

    // ---- structural unary detection --------------------------------------

    block.tests.push_back({
        "parse_key_then_key_first_is_unary",
        "a key immediately followed by another key is unary",
        []() -> bool
        {
            // visible is unary (next is a key); mode is a trailing lone key
            constexpr bool ok = same_v<
                parsed<setting::visible, setting::mode>,
                std::tuple< osi::k_spec<setting::visible>,
                            osi::k_spec<setting::mode> > >;
            static_assert(ok, "parse<visible,mode> -> two unary");
            return ok;
        } });

    block.tests.push_back({
        "parse_run_of_unary_keys",
        "a run of keys with no values are all unary",
        []() -> bool
        {
            constexpr bool ok = same_v<
                parsed<setting::visible, setting::mode, setting::name>,
                std::tuple< osi::k_spec<setting::visible>,
                            osi::k_spec<setting::mode>,
                            osi::k_spec<setting::name> > >;
            static_assert(ok, "parse<visible,mode,name> -> three unary");
            return ok;
        } });

    block.tests.push_back({
        "parse_unary_then_valued",
        "a leading unary key is followed by a normal valued pair",
        []() -> bool
        {
            constexpr bool ok = same_v<
                parsed<setting::visible, setting::width, 1024>,
                std::tuple< osi::k_spec<setting::visible>,
                            osi::kv_spec<setting::width, 1024> > >;
            static_assert(ok, "parse<visible,width,1024>");
            return ok;
        } });

    block.tests.push_back({
        "parse_valued_then_trailing_unary",
        "a trailing lone key after a pair is unary",
        []() -> bool
        {
            constexpr bool ok = same_v<
                parsed<setting::width, 1024, setting::visible>,
                std::tuple< osi::kv_spec<setting::width, 1024>,
                            osi::k_spec<setting::visible> > >;
            static_assert(ok, "parse<width,1024,visible>");
            return ok;
        } });

    block.tests.push_back({
        "parse_unary_between_valued_pairs",
        "a key mid-stream is unary iff the NEXT token is a key",
        []() -> bool
        {
            // visible sits between two pairs; the token after it (height) is a
            // key, so visible is unary
            constexpr bool ok = same_v<
                parsed<setting::width, 1, setting::visible, setting::height, 2>,
                std::tuple< osi::kv_spec<setting::width, 1>,
                            osi::k_spec<setting::visible>,
                            osi::kv_spec<setting::height, 2> > >;
            static_assert(ok, "parse<width,1,visible,height,2>");
            return ok;
        } });

    block.tests.push_back({
        "parse_trailing_key_after_pairs",
        "a trailing key after several pairs closes the stream as unary",
        []() -> bool
        {
            constexpr bool ok = same_v<
                parsed<setting::width, 1, setting::height, 2, setting::visible>,
                std::tuple< osi::kv_spec<setting::width, 1>,
                            osi::kv_spec<setting::height, 2>,
                            osi::k_spec<setting::visible> > >;
            static_assert(ok, "parse<width,1,height,2,visible>");
            return ok;
        } });

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
