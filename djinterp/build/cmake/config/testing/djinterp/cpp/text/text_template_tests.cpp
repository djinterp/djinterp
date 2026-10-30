/******************************************************************************
* djinterp                                                            main.cpp
*
*   A small driver exercising the modules together:
*     - core/meta/template.hpp     : the template-source-sink algebra,
*                                    F = ev . (F^ x id)
*     - core/meta/interpolate.hpp  : the type-agnostic find-and-replace engine
*                                    (scanner -> resolver -> sink), used here for
*                                    string interpolation
*     - core/container/buffer/text_buffer.hpp : a growable text accumulator that
*                                    stands in as an interpolation sink
*     - core/binary/binary_template.hpp : binary serialization (template =
*                                    layout, source = field values, sink = bytes)
*
*   The text section drives interpolate.hpp two ways: once WITHOUT a text_buffer
*   (str() allocates and returns a std::string) and once WITH a text_buffer (a
*   custom sink appends into a caller-owned buffer; the engine allocates nothing).
*
*   The #include lines below mirror each header's own `path:` line; adjust them
*   to your include roots as needed.
******************************************************************************/

#include <djinterp/cpp/paradigm/template/template.hpp>
#include <djinterp/cpp/functional/interpolate.hpp>
#include <djinterp/cpp/container/buffer/text_buffer.hpp>
#include <djinterp/core/binary/binary_template.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <cassert>
#include <iostream>

using namespace djinterp;

using sv = std::string_view;


// text_buffer_sink
//   class: an interpolation sink that appends each emitted span into a
// caller-owned text_buffer.  The engine hands a sink exactly two operations --
// literal(run) and value(resolved) -- so adapting text_buffer is this small.
// Mirrors string_sink, but targets text_buffer instead of std::basic_string,
// which is the entire point of the sink axis: the engine neither knows nor
// cares what it is writing into.
template<typename _Type = char>
class text_buffer_sink
{
public:
    using char_type = _Type;
    using view_type = std::basic_string_view<_Type>;

    explicit
    text_buffer_sink(
        text_buffer<>& _out
    )
        : m_out(_out)
    {}

    // literal -- emit a span verbatim
    void
    literal(
        view_type _run
    )
    {
        m_out.append(_run.data(), _run.size());

        return;
    }

    // value -- emit a resolved value (a string_view binding or an owned string)
    template<typename _Value>
    void
    value(
        const _Value& _value
    )
    {
        const view_type _view(_value);
        m_out.append(_view.data(), _view.size());

        return;
    }

private:
    text_buffer<>& m_out;
};


// banner_demo
//   demo: render the file-banner template through interpolate.hpp directly into
// a text_buffer (the larger "with a text_buffer" example).  The placeholder
// delimiter is { }: the engine has no format-spec concept, so the whole brace
// body is taken as the key name.  The file body is itself one bound value, so
// its braces are emitted verbatim (never re-scanned) and need no escaping.
void banner_demo()
{
    std::cout << "\n=== file banner (rendered into a text_buffer) ===\n";

    static const char* const k_banner =
        R"BANNER(/******************************************************************************
* {framework_name} [{subframework_name}]                            {filename}
*
* {description}
*
*
* path:      {filepath}/{filename}
* link(s):   {links}
* author(s): {authors}                               created: {date}
******************************************************************************/

/*
TABLE OF CONTENTS
=================
{table_of_contents}
*/

#ifndef {include_guard_name}
#define {include_guard_name} 1

{includes}

{namespaces_open}


{code}


{namespaces_close}


#endif  // {include_guard_name})BANNER";

    static const char* const k_code =
        R"CODE(template<typename _Type     = char,
         typename _Resolver = empty_resolver<char>,
         typename _Scanner  = brace_scanner<char>>
class interpolation
{
    // ... full class body ...
};)CODE";

    // one allocation-free pass straight into the buffer
    text_buffer<>          buffer;
    text_buffer_sink<char> sink(buffer);

    interpolate(sv(k_banner))
        .interpolate({
            {"framework_name",    "djinterp"},
            {"subframework_name", "meta"},
            {"filename",          "interpolate.hpp"},
            {"description",       "  The type-agnostic interpolation engine."},
            {"filepath",          "/inc/djinterp/cpp/meta"},
            {"links",             "TBA"},
            {"authors",           "Samuel 'teer' Neal-Blim"},
            {"date",              "2026.06.15"},
            {"table_of_contents", "I.    SCAN-EVENT VOCABULARY\n"
                                  "II.   SCANNERS\n"
                                  "III.  RESOLUTION & RESOLVERS\n"
                                  "IV.   SINKS\n"
                                  "V.    THE ENGINE\n"
                                  "VI.   THE LAZY FUNCTOR"},
            {"include_guard_name","DJINTERP_META_INTERPOLATE_"},
            {"includes",          "#include <string>\n#include <string_view>"},
            {"namespaces_open",   "namespace djinterp {"},
            {"code",              k_code},
            {"namespaces_close",  "} // namespace djinterp"},
        })
        .into(sink);

    std::cout << buffer << "\n";

    return;
}


int main()
{
    // =======================================================================
    // interpolate.hpp : string interpolation (scanner -> resolver -> sink)
    // =======================================================================
    std::cout << "=== interpolate.hpp ===\n";

    // --- method 1: WITHOUT a text_buffer -------------------------------------
    // str() forces the single pass and returns a freshly allocated std::string.
    std::cout << interpolate(sv("Hello, {name}! You have {count} new messages."))
                     .interpolate({{"name", "Ada"}, {"count", "3"}})
                     .str()
              << "\n";

    // --- method 2: WITH a text_buffer ----------------------------------------
    // into() forces the same pass into a caller-owned sink; the engine performs
    // no result allocation -- the buffer is ours to size, keep, and reuse.
    {
        text_buffer<>          out;
        text_buffer_sink<char> sink(out);

        interpolate(sv("Hello, {name}! You have {count} new messages."))
            .interpolate({{"name", "Ada"}, {"count", "3"}})
            .into(sink);

        std::cout << out << "\n";
    }

    // a callable lookup -- values computed on demand.  lookup() ALWAYS hits
    // (a missing key becomes empty, the classic DTextTemplate policy), so it
    // seeds the chain via make_interpolation rather than extending the
    // identity frame (chained frames must share one resolution value type).
    auto host_of = [](sv key) -> std::string
    {
        if (key == "user") { return "ada"; }
        if (key == "host") { return "djinterp.dev"; }
        return std::string();
    };
    std::cout << make_interpolation(sv("{user}@{host}"), lookup(host_of)).str()
              << "\n";

    // escaping: {{ and }} render as literal braces
    std::cout << interpolate(sv("set({{ {key} }})"))
                     .interpolate({{"key", "x"}})
                     .str()
              << "\n";

    // bind a template once, reuse it as a functor over many sources (F_t with
    // the template curried): each .interpolate(...) returns a fresh render and
    // leaves `link` untouched.
    auto link = interpolate(sv("<a href=\"{url}\">{label}</a>"));
    std::cout << link.interpolate({{"url", "/home"}, {"label", "Home"}}).str()
              << "\n";
    std::cout << link.interpolate({{"url", "/docs"}, {"label", "Docs"}}).str()
              << "\n";

    // PARTIAL interpolation -- a miss leaves the placeholder for a later frame,
    // and chained frames collapse into ONE pass (no intermediate string).
    auto greeting = interpolate(sv("{greeting}, {name}! Your role is {role}."));
    std::cout << greeting.interpolate({{"name", "Ada"}}).str() << "\n";
    std::cout << greeting.interpolate({{"name", "Ada"}})
                         .interpolate({{"greeting", "Hello"}, {"role", "admin"}})
                         .str()
              << "\n";

    // a predicate-gated frame: only keys beginning with '_' are resolved here;
    // the rest fall through untouched for the next frame to fill.
    auto is_private = [](sv key)
    {
        return (!key.empty()) && (key.front() == '_');
    };
    std::cout << interpolate(sv("{_secret} / {public}"))
                     .interpolate_if(is_private, bindings({{"_secret", "xyz"}}))
                     .interpolate({{"public", "ok"}})
                     .str()
              << "\n";

    // a different placeholder syntax is just a different scanner -- the engine,
    // the resolvers, and the sink are all unchanged.
    std::cout << interpolate<char, DSigilScanner<char>>(sv("$user@$host"))
                     .interpolate({{"user", "ada"}, {"host", "djinterp.dev"}})
                     .str()
              << "\n";

    // =======================================================================
    // template.hpp : F = ev . (F^ x id)
    // =======================================================================
    std::cout << "\n=== template.hpp ===\n";

    // F : (int, int) -> int ; template t, source alpha
    auto F   = [](int t, int alpha) { return (t * alpha) + 1; };
    auto sys = make_template_system<int>(F);

    const int  direct  = sys.apply(3, 10);      // F(3, 10)
    const auto F3      = sys.instantiate(3);     // F^(3)
    const int  curried = evaluate(F3, 10);       // ev(F_3, 10)
    std::cout << "apply(3,10)           = " << direct  << "\n";
    std::cout << "evaluate(instantiate) = " << curried << "\n";
    assert(direct == curried);                   // instantiate-then-apply == apply

    static_assert(is_transformation_v<decltype(F), int>,      "F");
    static_assert(is_source_transformer_v<decltype(F3), int>, "F_t");

    // stage decomposition: F = inc . sq . f1  (only f1 is binary)
    auto f1     = [](int t, int a) { return t + a; };
    auto sq     = [](int x)        { return x * x; };
    auto inc    = [](int x)        { return x + 1; };
    auto staged = make_template_system<int>(stages(f1, sq, inc));
    std::cout << "staged.apply(2,3)     = " << staged.apply(2, 3)
              << "   ((2+3)^2 + 1)\n";
    assert(staged.apply(2, 3) == 26);

    // a one-digit parser, sequenced with Kleisli composition (Q <> P)
    using outcome = DParseOutcome<int, std::string, std::string>;
    auto digit = [](const std::string& s) -> outcome {
        if (!s.empty() && (s[0] >= '0') && (s[0] <= '9'))
        {
            return parse_success<std::string>(int(s[0] - '0'), s.substr(1));
        }
        return parse_failure<int, std::string>(std::string("not a digit"));
    };
    auto two = kleisli_then(digit, digit);
    auto ok  = two(std::string("42"));
    std::cout << "parse \"42\" (two digits) -> value " << ok.value()
              << ", remaining \"" << ok.remaining() << "\"\n";
    assert(ok.is_ok() && (ok.value() == 2) && ok.remaining().empty());

    // =======================================================================
    // binary_template : layout + values -> bytes
    // =======================================================================
    std::cout << "\n=== binary_template ===\n";

    // a length-prefixed packet: magic, u16 big-endian length, then payload
    binary_template packet;
    packet.literal({0xCA, 0xFE})
          .u16("length", byte_order::big)
          .raw("payload");

    std::string payload = "djinterp";
    auto frame = packet.render({
        {"length",  static_cast<std::uint64_t>(payload.size())},
        {"payload", binary_value::bytes(payload)},
    });

    std::cout << "frame (" << frame.size() << " bytes): ";
    const char* hexd = "0123456789ABCDEF";
    for (std::uint8_t byte : frame)
    {
        std::cout << hexd[byte >> 4] << hexd[byte & 0x0F] << ' ';
    }
    std::cout << "\n";
    assert(frame.size() == 2 + 2 + payload.size());
    assert((frame[0] == 0xCA) && (frame[1] == 0xFE));
    assert((frame[2] == 0x00) && (frame[3] == 0x08));   // length 8, big-endian

    // the larger "with a text_buffer" example
    banner_demo();

    // =======================================================================
    // the connection
    // =======================================================================
    std::cout << "\n=== connection ===\n";
    std::cout <<
        "Text interpolation and binary serialization are one schema specialized\n"
        "two ways: the template is t, the source feeds the fields, the sink is\n"
        "the result (a string / a text_buffer / a byte buffer), and interpolate(t)\n"
        "/ pack(t) is F_t = F^(t).  interpolate.hpp factors the text half into a\n"
        "scanner -> resolver -> sink fold, so the placeholder syntax, the lookup,\n"
        "and the output target each vary independently.\n";

    std::cout << "\nall demos ran.\n";

    return 0;
}