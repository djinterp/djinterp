/*******************************************************************************
* djinterp [cpp][text]                              text_core_tests_template.cpp
*
*   The groups for text_align.hpp and text_template.hpp: DTextAlignment's
* values; the placeholder parser's segments; the template's dialect, its
* rendering against inline bindings and against any lookup, its copies and
* its wide form; and a {key} literal carried through DTextTemplate and back
* through make_curly_pattern.
*
*
* path:      /tests/djinterp/cpp/text/text_core_tests_template.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "text_core_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <cstddef>  // NULL, std::size_t
#include <map>      // std::map
#include <string>   // std::string, std::wstring
#include <utility>  // std::move
#include <vector>   // std::vector
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::is_same, std::underlying_type
#endif
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>  // std::string_view
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/text_align.hpp"  // DTextAlignment
#include "../../../../inc/djinterp/cpp/text/text_pattern.hpp"  // DTextPattern
#include "../../../../inc/djinterp/cpp/text/text_template.hpp"  // DTextTemplate
// re_std
#include "../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_text_core_align
  Tests the following:
  - DTextAlignment is one byte, and its underlying type is re_std::uint8_t
  - its values are left, center, right and justify: 0 to 3, in that order
  - the four are distinct, and each survives a round trip through the
    underlying type
*/
void
d_tests_text_core_align(void)
{
    typedef djinterp::DTextAlignment              alignment;
    typedef std::underlying_type<alignment>::type underlying;

    const alignment all[4] =
    {
        alignment::left,
        alignment::center,
        alignment::right,
        alignment::justify
    };
    bool        distinct   = true;
    bool        round_trip = true;
    std::size_t i;
    std::size_t j;

    D_TESTS_TEXT_CHECK(sizeof(alignment) == 1);
    D_TESTS_TEXT_CHECK((std::is_same<underlying, re_std::uint8_t>::value));
    D_TESTS_TEXT_CHECK(static_cast<int>(alignment::left) == 0);
    D_TESTS_TEXT_CHECK(static_cast<int>(alignment::center) == 1);
    D_TESTS_TEXT_CHECK(static_cast<int>(alignment::right) == 2);
    D_TESTS_TEXT_CHECK(static_cast<int>(alignment::justify) == 3);

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            distinct = distinct && ((all[i] == all[j]) == (i == j));
        }

        round_trip = round_trip &&
            (static_cast<alignment>(static_cast<underlying>(all[i])) ==
             all[i]);
    }

    D_TESTS_TEXT_CHECK(distinct);
    D_TESTS_TEXT_CHECK(round_trip);

    return;
}

#endif  // C++11


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_text_core_template_parser
  Tests the following:
  - "Hello {name}!" scans to a literal, a key and a literal, each a view of
    the owned format, with its totals: one key, seven literal bytes
  - keys come in order of appearance, repeats kept
  - an empty format, a NULL one and an unset parser are empty, with nothing
    scanned
*/
void
d_tests_text_core_template_parser(void)
{
    typedef djinterp::internal::DPlaceholderParser<char> parser;

    const parser                        greeting("Hello {name}!");
    const std::vector<parser::segment>& parts = greeting.segments();
    const bool                          three = (parts.size() == 3);
    const parser                        many("{a}-{b}-{a}");
    const std::vector<std::string_view> keys  = many.keys();
    const parser                        blank("");
    const parser                        null_format(
                                            static_cast<const char*>(NULL));
    const parser                        unset;

    D_TESTS_TEXT_CHECK(greeting.segment_count() == 3);
    D_TESTS_TEXT_CHECK(three && !parts[0].m_is_key &&
                       (greeting.text(parts[0]) == "Hello "));
    D_TESTS_TEXT_CHECK(three && parts[1].m_is_key &&
                       (greeting.text(parts[1]) == "name"));
    D_TESTS_TEXT_CHECK(three && !parts[2].m_is_key &&
                       (greeting.text(parts[2]) == "!"));
    D_TESTS_TEXT_CHECK(greeting.key_count() == 1);
    D_TESTS_TEXT_CHECK(greeting.literal_size() == 7);
    D_TESTS_TEXT_CHECK(greeting.format() == "Hello {name}!");
    D_TESTS_TEXT_CHECK(!greeting.empty());

    // in order, repeats kept
    D_TESTS_TEXT_CHECK((keys.size() == 3) && (keys[0] == "a") &&
                       (keys[1] == "b") && (keys[2] == "a"));
    D_TESTS_TEXT_CHECK((many.segment_count() == 5) &&
                       (many.literal_size() == 2));

    // nothing to scan
    D_TESTS_TEXT_CHECK(blank.empty() && (blank.segment_count() == 0));
    D_TESTS_TEXT_CHECK((blank.key_count() == 0) &&
                       (blank.literal_size() == 0));
    D_TESTS_TEXT_CHECK(null_format.empty() &&
                       (null_format.segment_count() == 0));
    D_TESTS_TEXT_CHECK(unset.empty() && (unset.segment_count() == 0));

    return;
}


/*
d_tests_text_core_template_dialect
  Tests the following:
  - blanks inside the braces are trimmed: "{ name }" and "{\tname\n}" both
    name "name"
  - "{{" and "}}" are escapes, one brace each, and make no key
  - an unmatched "{" and a lone "}" are literal
  - the first "}" closes a key: no nesting, and what follows is literal
  - an empty key, or one of blanks, is still a key, named ""
  - escapes beside a key: "{{{a}}}" renders "{1}" for a = 1
*/
void
d_tests_text_core_template_dialect(void)
{
    typedef djinterp::DTextTemplate<char> text_template;

    const text_template           trimmed("{ name }|{\tname\n}");
    const text_template           escaped("{{x}}");
    const text_template           unmatched("a{b");
    const text_template           lone("a}b");
    const text_template           nested("{a{b}c}");
    const text_template           empty_key("<{}>");
    const text_template           blank_key("<{ }>");
    const text_template           braced("{{{a}}}");
    std::vector<std::string_view> keys = trimmed.keys();

    D_TESTS_TEXT_CHECK((keys.size() == 2) && (keys[0] == "name") &&
                       (keys[1] == "name"));
    D_TESTS_TEXT_CHECK(trimmed.render({{"name", "x"}}) == "x|x");

    D_TESTS_TEXT_CHECK(escaped.key_count() == 0);
    D_TESTS_TEXT_CHECK(escaped.render({}) == "{x}");

    D_TESTS_TEXT_CHECK((unmatched.key_count() == 0) &&
                       (unmatched.segment_count() == 1));
    D_TESTS_TEXT_CHECK(unmatched.render({}) == "a{b");
    D_TESTS_TEXT_CHECK((lone.key_count() == 0) &&
                       (lone.render({}) == "a}b"));

    keys = nested.keys();
    D_TESTS_TEXT_CHECK((keys.size() == 1) && (keys[0] == "a{b"));
    D_TESTS_TEXT_CHECK(nested.render({{"a{b", "X"}}) == "Xc}");

    D_TESTS_TEXT_CHECK((empty_key.key_count() == 1) &&
                       empty_key.keys()[0].empty());
    D_TESTS_TEXT_CHECK(empty_key.render({{"", "e"}}) == "<e>");
    D_TESTS_TEXT_CHECK((blank_key.key_count() == 1) &&
                       (blank_key.render({{"", "e"}}) == "<e>"));

    D_TESTS_TEXT_CHECK(braced.render({{"a", "1"}}) == "{1}");

    return;
}


/*
d_tests_text_core_template_render
  Tests the following:
  - inline bindings fill each key, in any order, and extra bindings are
    ignored
  - a key with no binding renders empty
  - the functor face renders as render does
  - render_to appends to what its string holds
  - the format, the keys in order and the segment count are kept, and
    rendering twice gives the same text
  - a template with no keys renders its text; an empty one, nothing
*/
void
d_tests_text_core_template_render(void)
{
    typedef djinterp::DTextTemplate<char> text_template;

    const text_template                 letter("Dear {who}, {what} at {when}.");
    const std::vector<std::string_view> keys = letter.keys();
    const text_template                 plain("no keys");
    const text_template                 none;
    std::string                         out("> ");

    D_TESTS_TEXT_CHECK(
        letter.render({{"who", "Ada"}, {"what", "tea"}, {"when", "four"}}) ==
        "Dear Ada, tea at four.");
    D_TESTS_TEXT_CHECK(
        letter.render({{"when", "four"}, {"x", "y"}, {"what", "tea"},
                       {"who", "Ada"}}) ==
        "Dear Ada, tea at four.");
    D_TESTS_TEXT_CHECK(letter.render({{"who", "Ada"}}) ==
                       "Dear Ada,  at .");
    D_TESTS_TEXT_CHECK(
        letter({{"who", "A"}, {"what", "B"}, {"when", "C"}}) ==
        "Dear A, B at C.");

    letter.render_to(out, {{"who", "Ada"}, {"what", "tea"}, {"when", "four"}});
    D_TESTS_TEXT_CHECK(out == "> Dear Ada, tea at four.");

    D_TESTS_TEXT_CHECK(letter.format() == "Dear {who}, {what} at {when}.");
    D_TESTS_TEXT_CHECK((keys.size() == 3) && (keys[0] == "who") &&
                       (keys[1] == "what") && (keys[2] == "when"));
    D_TESTS_TEXT_CHECK((letter.key_count() == 3) &&
                       (letter.segment_count() == 7));
    D_TESTS_TEXT_CHECK(letter.render({{"who", "x"}}) ==
                       letter.render({{"who", "x"}}));

    D_TESTS_TEXT_CHECK(plain.render({}) == "no keys");
    D_TESTS_TEXT_CHECK(none.empty() && none.render({}).empty());

    return;
}


/*
d_tests_text_core_template_lookups
  Tests the following:
  - a lookup that returns a temporary std::string: the value lives through
    the append
  - a lookup that returns const char*, and one over a std::map
  - the functor face and render_to take a lookup too
  - the lookup is called once per key, in order
*/
void
d_tests_text_core_template_lookups(void)
{
    typedef djinterp::DTextTemplate<char> text_template;

    const text_template                      pair_of("[{a}][{b}]");
    const std::map<std::string, std::string> values = {{"a", "x"},
                                                       {"b", "y"}};
    std::string                              calls;
    std::string                              out("=");

    const auto doubled = [](std::string_view _key)
    {
        return std::string(_key) + std::string(_key);
    };
    const auto digits = [](std::string_view _key) -> const char*
    {
        return (_key == "a") ? "1" : "2";
    };
    const auto from_map = [&values](std::string_view _key) -> std::string_view
    {
        const auto found = values.find(std::string(_key));

        return (found == values.end()) ? std::string_view()
                                       : std::string_view(found->second);
    };
    const auto counting = [&calls](std::string_view _key)
    {
        calls.append(_key.data(), _key.size());

        return std::string_view("-");
    };

    D_TESTS_TEXT_CHECK(pair_of.render(doubled) == "[aa][bb]");
    D_TESTS_TEXT_CHECK(pair_of.render(digits) == "[1][2]");
    D_TESTS_TEXT_CHECK(pair_of.render(from_map) == "[x][y]");
    D_TESTS_TEXT_CHECK(pair_of(from_map) == "[x][y]");

    pair_of.render_to(out, from_map);
    D_TESTS_TEXT_CHECK(out == "=[x][y]");

    D_TESTS_TEXT_CHECK(pair_of.render(counting) == "[-][-]");
    D_TESTS_TEXT_CHECK(calls == "ab");

    return;
}


/*
d_tests_text_core_template_copies
  Tests the following:
  - a copy renders after its original is gone, with a format past any
    small-string buffer, so the copy's segments are offsets into its own
    text
  - a moved template, and an assigned copy, render as the original did
  - DTextTemplate<wchar_t>: its keys, and its rendering against wide
    bindings
*/
void
d_tests_text_core_template_copies(void)
{
    typedef djinterp::DTextTemplate<char>    text_template;
    typedef djinterp::DTextTemplate<wchar_t> wide_template;

    const std::string   format = std::string(64, '.') + "{key}" +
                                 std::string(64, ',');
    const std::string   expect = std::string(64, '.') + "v" +
                                 std::string(64, ',');
    text_template       copy;
    text_template       moved_from(format);
    const text_template moved(std::move(moved_from));
    text_template       assigned;
    const wide_template wide(L"{a}-{b}");

    {
        const text_template original(format);

        copy = original;
    }

    D_TESTS_TEXT_CHECK(copy.render({{"key", "v"}}) == expect);
    D_TESTS_TEXT_CHECK(copy.format() == format);
    D_TESTS_TEXT_CHECK(moved.render({{"key", "v"}}) == expect);

    assigned = copy;
    D_TESTS_TEXT_CHECK(assigned.render({{"key", "v"}}) == expect);

    D_TESTS_TEXT_CHECK(wide.key_count() == 2);
    D_TESTS_TEXT_CHECK(wide.render({{L"a", L"1"}, {L"b", L"2"}}) ==
                       L"1-2");

    return;
}


/*
d_tests_text_core_template_pattern
  Tests the following:
  - DTextTemplate renders a {key} literal, and make_curly_pattern over the
    same literal reads the bindings back from the text
  - the pattern renders the same text from what it read
  - the two agree on the keys, in order
*/
void
d_tests_text_core_template_pattern(void)
{
    typedef djinterp::DTextPattern::match_result_type result;

    const char* const                   literal = "Hello {name}, you are "
                                                  "{age}.";
    const djinterp::DTextTemplate<char> generator(literal);
    const djinterp::DTextPattern        reader =
                                            djinterp::make_curly_pattern(
                                                literal);
    const std::string                   text = generator.render(
                                            {{"name", "Ada"}, {"age", "36"}});
    const result                        read = reader.extract(text);
    const std::string*                  name = read.captures.find("name");
    const std::string*                  age  = read.captures.find("age");
    const std::vector<std::string_view> made = generator.keys();
    const std::vector<std::string>      seen = reader.capture_keys();

    D_TESTS_TEXT_CHECK(text == "Hello Ada, you are 36.");
    D_TESTS_TEXT_CHECK(read.matched);
    D_TESTS_TEXT_CHECK((name != NULL) && (*name == "Ada"));
    D_TESTS_TEXT_CHECK((age != NULL) && (*age == "36"));
    D_TESTS_TEXT_CHECK(reader.render(read.captures) == text);
    D_TESTS_TEXT_CHECK((made.size() == 2) && (seen.size() == 2) &&
                       (made[0] == seen[0]) && (made[1] == seen[1]));

    return;
}

#endif  // C++17
