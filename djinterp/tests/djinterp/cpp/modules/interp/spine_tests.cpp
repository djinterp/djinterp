/*******************************************************************************
* djinterp [tests]                                               spine_tests.cpp
*
* The spine's suite: what interp/spine.hpp does as it moved from parse's
* functional_cli.hpp -- the reader's tokens, the grammar's pipeline and each
* invocation's options, and the fold's value threaded through `|` and
* stopped at the first failure -- so that joining its legs to expression core
* v2 changes only what the design says it changes (docs/djinterp/interp/
* DESIGN.md, sections 3 and 4). The splitting of `-xvf` into three flags,
* which G-D2 takes out, is held here as the spine does it today.
*   A suite is a table of sections, each a function that checks one thing
* and says which check failed, the runner a loop over the table that prints
* each verdict and the totals. It is a plain main until DTest 2.0 has a
* leaf for it; below C++11, where the spine is empty, it has no sections.
*
*
* path:      /tests/djinterp/cpp/modules/interp/spine_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <cstdio>   // std::printf
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include <djinterp/cpp/modules/interp/spine.hpp>  // the spine


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

namespace
{

using namespace ::djinterp;          // result, maybe, ok, err
using namespace ::djinterp::interp;

int g_failed = 0;

// expect
//   function: counts and names a check that failed.
void expect(
    bool        _held,
    const char* _what
)
{
    if (!_held)
    {
        std::printf("    failed: %s\n", _what);
        ++g_failed;
    }

    return;
}

// kinds_of
//   function: the kinds of a token stream, as a text: W L S T P.
std::string kinds_of(
    const std::vector<arg_token>& _tokens
)
{
    static const char LETTERS[] = { 'W', 'L', 'S', 'T', 'P' };
    std::string       out;

    for (std::size_t i = 0; i < _tokens.size(); ++i)
    {
        out.push_back(LETTERS[_tokens[i].kind]);
    }

    return out;
}

/*
reader_line
  A line read into tokens: words, long and short options, the terminator,
and `|`, with quoted spans kept whole and their quotes taken out.
*/
void reader_line()
{
    const std::vector<arg_token> a = read_line("ls -la --color=auto | wc -l");
    const std::vector<arg_token> b = read_line("echo \"a b\" 'c|d'");
    const std::vector<arg_token> c = read_line("cmd -- -x --y");

    expect(kinds_of(a) == "WSLPWS", "options, a pipe and words, by shape");
    expect( (a[2].name == "color") && a[2].has_value &&
            (a[2].value == "auto"),
           "--name=value, its name and value apart");
    expect( (b.size() == 3) && (b[1].name == "a b") && (b[2].name == "c|d"),
           "quoted spans kept whole, a quoted `|` a character");
    expect(kinds_of(c) == "WTWW", "after `--`, every word an operand");
    expect(read_line("  ").empty(), "a blank line, no tokens");

    return;
}

/*
reader_argv
  An argument vector read as words the shell split: a lone `|` is a stage
separator, and a word keeps its spaces.
*/
void reader_argv()
{
    const std::vector<std::string> args = { "a", "-x", "|", "b", "c d" };
    const std::vector<arg_token>   t    = read_argv(args);
    char                           p0[] = "prog";
    char                           p1[] = "x";
    char                           p2[] = "y";
    char*                          argv[] = { p0, p1, p2 };

    expect(kinds_of(t) == "WSPWW", "words, an option and a pipe, by shape");
    expect(t[4].name == "c d", "a word with a space, one token");
    expect(from_argv(3, argv) == std::vector<std::string>({ "x", "y" }),
           "from_argv leaves the program's name out");

    return;
}

/*
grammar_pipeline
  Tokens parsed into a pipeline of invocations, each with its verb, flags,
values and operands; and a line that is not a pipeline refused.
*/
void grammar_pipeline()
{
    result<command_pipeline, cli_status> p =
        parse_line("tar -xvf --out=a.tar f1 f2 | gzip --best");

    expect(p.is_ok() && (p.value().size() == 2), "two stages");

    if (p.is_ok() && (p.value().size() == 2))
    {
        const command_invocation& first = p.value()[0];

        expect(first.verb == "tar", "the verb");
        expect( first.options.has_flag("x") && first.options.has_flag("v") &&
                first.options.has_flag("f"),
               "-xvf, as the spine splits it today: three flags");
        expect( first.options.value_of("out").has_value() &&
                (first.options.value_of("out").value() == "a.tar"),
               "--out=a.tar, a value");
        expect(first.options.operands().size() == 2, "two operands");
        expect(p.value()[1].options.has_flag("best"), "--best, a flag");
    }

    expect(parse_line("| a").is_err(), "a pipe before any command, refused");
    expect(parse_line("a |").is_err(), "a pipe after the last, refused");
    expect(parse_argv({ "a", "|", "b" }).is_ok(), "an argv pipeline");

    return;
}

// registry
//   function: three commands over an int: `add --n=N` adds N, `twice`
// doubles, `fail` fails with a status of its own.
command_registry<int> registry()
{
    command_registry<int> r;

    r.define("add", [](const parsed_options& _o, const int& _in)
             {
                 const maybe<std::string> n = _o.value_of("n");

                 return ok<int, cli_status>(
                     _in + (n.has_value() ? std::stoi(n.value()) : 1));
             })
     .define("twice", [](const parsed_options&, const int& _in)
             {
                 return ok<int, cli_status>(_in * 2);
             })
     .define("fail", [](const parsed_options&, const int&)
             {
                 return err<int, cli_status>(DCliStatusUserBase + 1);
             });

    return r;
}

/*
fold_threads
  The fold threads a value through `|` from the seed, resolving each verb
in the registry, and stops at the first failure: an unknown command, a
command that fails, or a line that does not parse.
*/
void fold_threads()
{
    const command_registry<int> r = registry();
    result<int, cli_status>     a = evaluate("add --n=2 | twice | add", r, 1);
    result<int, cli_status>     b = evaluate("add | nonesuch | add", r, 0);
    result<int, cli_status>     c = evaluate("add | fail | add", r, 0);
    result<int, cli_status>     d = evaluate("| add", r, 0);
    result<int, cli_status>     e =
        evaluate_argv({ "twice", "|", "twice" }, r, 3);

    expect(a.is_ok() && (a.value() == 7), "((1 + 2) * 2) + 1 is 7");
    expect( b.is_err() && (b.error() == DCliStatusUnknownCommand),
           "an unknown command, its status");
    expect( c.is_err() && (c.error() == DCliStatusUserBase + 1),
           "a failing command, its own status, the rest not run");
    expect( d.is_err() && (d.error() == DCliStatusParseError),
           "a line that does not parse, the parse's status");
    expect(e.is_ok() && (e.value() == 12), "an argv pipeline, 3 * 2 * 2");
    expect( r.contains("twice") && (!r.contains("thrice")) &&
            (r.size() == 3),
           "the registry, by name");

    return;
}

/*
traits_hold
  The structural traits, and their variable templates where the language
has them.
*/
void traits_hold()
{
    expect(is_command_spec<command_spec<int>>::value, "a spec is a spec");
    expect(!is_command_spec<int>::value, "an int is not");
    expect(is_command_registry<command_registry<int>>::value,
           "a registry is a registry");
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    expect( is_command_spec_v<command_spec<int>> &&
            (!is_command_registry_v<int>),
           "the variable templates agree");
#endif

    return;
}

struct section
{
    const char* name;
    void      (*run)();
};

const section SECTIONS[] = {
    { "reader: a line",          reader_line },
    { "reader: an argv",         reader_argv },
    { "grammar: a pipeline",     grammar_pipeline },
    { "fold: a value threaded",  fold_threads },
    { "traits",                  traits_hold }
};

}  // namespace


int main()
{
    int passed = 0;
    int failed = 0;

    for (const section& each : SECTIONS)
    {
        const int before = g_failed;

        each.run();

        if (g_failed == before)
        {
            ++passed;
            std::printf("[pass] %s\n", each.name);
        }
        else
        {
            ++failed;
            std::printf("[FAIL] %s\n", each.name);
        }
    }

    std::printf("passed: %d   failed: %d\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}

#else

int main()
{
    std::printf("passed: 0   failed: 0\n");

    return 0;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
