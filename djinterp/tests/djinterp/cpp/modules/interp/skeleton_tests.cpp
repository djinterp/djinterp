/*******************************************************************************
* djinterp [cpp][interp]                                      skeleton_tests.cpp
*
* The suite of the interpreter's skeleton, declared in skeleton_tests.hpp.
*   Each reading case is a text and the term it reads as, written as an
* s-expression (script::sexpr); each refusal a text and the place of its
* first error. The running cases use a table of small commands: `echo`
* gives its words as text, `upper` its input upper-cased, `count` its
* input's length, `no` comes to status 1, `fail` to trouble, `throws`
* throws, and `seen` notes that it ran, so that a case can say which stages
* a pipeline ran, and which operands a predicate did. `quiet` and `nope`
* come to nothing, status 0 and 1, a status alone; `pair` and `reals` to
* lists, `rec` and `cer` to records, alike by the language's rule.
*
*
* path:      /tests/djinterp/cpp/modules/interp/skeleton_tests.cpp
* link(s):   /docs/djinterp/cpp/modules/interp/DESIGN.html
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.25
*******************************************************************************/
#include "./skeleton_tests.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cctype>     // std::toupper
#include <cstddef>    // std::size_t
#include <stdexcept>  // std::runtime_error
#include <string>     // std::string
#include <vector>     // std::vector
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../../inc/djinterp/cpp/modules/interp/evaluate.hpp"  // the fold
#include "../../../../../inc/djinterp/cpp/modules/interp/language.hpp"  // the reader

using djinterp::interp::commands;
using djinterp::interp::outcome;
using djinterp::interp::script;
using djinterp::interp::value;


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// read_case
//   struct: a text, and the term it reads as.
struct read_case
{
    const char* text;
    const char* term;
};

/*
reads_as
  Whether each case reads, without an error, as its term; each that does not
said with what it read as.
*/
static bool
reads_as(
    const read_case* _cases,
    std::size_t      _count
)
{
    bool held = true;

    for (std::size_t i = 0u; i < _count; ++i)
    {
        const script read = djinterp::interp::parse_line(_cases[i].text);
        const bool   same = ( (read.ok()) &&
                              (read.sexpr() == _cases[i].term) );

        if (!same)
        {
            D_TEST_NOTE("'%s' read as '%s' (%s)", _cases[i].text,
                        read.sexpr().c_str(),
                        (read.errors().empty())
                        ? "no error"
                        : read.errors().front().message.c_str());
        }

        held = (D_TEST_EXPECT(same)) && (held);
    }

    return held;
}

// seen
//   variable: the stages `seen` ran, in order, each by its first word.
static std::string seen;

/*
table
  The running cases' commands.
*/
static commands
table()
{
    commands out;

    out.add("echo", [](const std::vector<std::string>& _argv, const value&) {
        std::string words;

        for (std::size_t i = 1u; i < _argv.size(); ++i)
        {
            words += ( (i > 1u) ? " " : "" ) + _argv[i];
        }

        return outcome{ value::of_text(words), 0, "", 0u, 0u };
    });
    out.add("upper", [](const std::vector<std::string>&, const value& _in) {
        std::string text = _in.as_text();

        for (char& c : text)
        {
            c = (char)std::toupper((unsigned char)c);
        }

        return outcome{ value::of_text(text), 0, "", 0u, 0u };
    });
    out.add("count", [](const std::vector<std::string>&, const value& _in) {
        return outcome{ value::of_integer((re_std::int64_t)
                                          _in.as_text().size()),
                        0, "", 0u, 0u };
    });
    out.add("no", [](const std::vector<std::string>&, const value&) {
        return outcome{ value::of_text("n"), 1, "", 0u, 0u };
    });
    out.add("fail", [](const std::vector<std::string>&, const value&) {
        return outcome{ value(), 2, "fail: failed", 0u, 0u };
    });
    out.add("throws", [](const std::vector<std::string>&,
                         const value&) -> outcome {
        throw std::runtime_error("boom");
    });
    out.add("quiet", [](const std::vector<std::string>&, const value&) {
        return outcome{ value(), 0, "", 0u, 0u };
    });
    out.add("nope", [](const std::vector<std::string>&, const value&) {
        return outcome{ value(), 1, "", 0u, 0u };
    });
    out.add("pair", [](const std::vector<std::string>&, const value&) {
        return outcome{ value::of_list({ value::of_integer(1),
                                         value::of_integer(2) }),
                        0, "", 0u, 0u };
    });
    out.add("reals", [](const std::vector<std::string>&, const value&) {
        return outcome{ value::of_list({ value::of_real(1.0),
                                         value::of_real(2.0) }),
                        0, "", 0u, 0u };
    });
    out.add("rec", [](const std::vector<std::string>&, const value&) {
        return outcome{ value::of_record({ { "a", value::of_integer(1) },
                                           { "b", value::of_text("x") } }),
                        0, "", 0u, 0u };
    });
    out.add("cer", [](const std::vector<std::string>&, const value&) {
        return outcome{ value::of_record({ { "b", value::of_text("x") },
                                           { "a", value::of_real(1.0) } }),
                        0, "", 0u, 0u };
    });
    out.add("seen", [](const std::vector<std::string>& _argv,
                       const value& _in) {
        seen += ( (seen.empty()) ? "" : "," ) +
                ( (_argv.size() > 1u) ? _argv[1] : std::string("?") );

        return outcome{ _in, 0, "", 0u, 0u };
    });

    return out;
}

/*
runs_as
  Whether a line comes to a value and a status, `seen` cleared first and
then holding `_seen`.
*/
static bool
runs_as(
    const char*  _text,
    const value& _input,
    const value& _value,
    int          _status,
    const char*  _seen
)
{
    const commands table_of = table();
    outcome        out;

    seen = "";
    out  = djinterp::interp::evaluate(djinterp::interp::parse_line(_text),
                                      table_of, _input);

    const bool held = ( (out.result == _value) &&
                        (out.status == _status) &&
                        (seen == _seen) );

    if (!held)
    {
        D_TEST_NOTE("'%s' came to %s, status %d, seen '%s' (%s)", _text,
                    out.result.show().c_str(), out.status, seen.c_str(),
                    out.message.c_str());
    }

    return D_TEST_EXPECT(held);
}


//==============================================================================
// 1.  READING A LINE
//==============================================================================

/*
d_tests_interp_read_terms
  Tests the following:
  - a command takes the words after its verb as leaves
  - `|` and `;` flatten into one node each, `;` the looser
  - a newline is `;`; blank lines, and a newline after `|`, `;` or `(`, or
    before `)`, are passed over
  - a `;` may end a sequence, inside brackets too
  - brackets group, and leave no node
*/
bool d_tests_interp_read_terms()
{
    static const read_case CASES[] =
    {
        { "ls -l | grep x ; echo done",
          "seq(pipe(cmd(ls, -l), cmd(grep, x)), cmd(echo, done))" },
        { "a | b | c",          "pipe(cmd(a), cmd(b), cmd(c))" },
        { "a ; b ; c",          "seq(cmd(a), cmd(b), cmd(c))" },
        { "(a ; b) | c",        "pipe(seq(cmd(a), cmd(b)), cmd(c))" },
        { "a | (b ; c) | d",    "pipe(cmd(a), seq(cmd(b), cmd(c)), cmd(d))" },
        { "((a))",              "cmd(a)" },
        { "a\nb",               "seq(cmd(a), cmd(b))" },
        { "\n\na x\n\n\nb\n",   "seq(cmd(a, x), cmd(b))" },
        { "a |\n  b",           "pipe(cmd(a), cmd(b))" },
        { "a ;\nb",             "seq(cmd(a), cmd(b))" },
        { "a ;",                "cmd(a)" },
        { "(a ;) | b",          "pipe(cmd(a), cmd(b))" },
        { "(\n a\n)",           "cmd(a)" }
    };

    return reads_as(CASES, sizeof(CASES) / sizeof(CASES[0]));
}

/*
d_tests_interp_read_words
  Tests the following:
  - a word is a run of bare and quoted parts, quotes taken off: single
    quotes literal, `\"` and `\\` read in double quotes
  - a keyword or an operator quoted is a word
  - `#` begins a comment only where a token could
  - `=`, `-1`, `-ofile`, a dotted option and a path are words as written
*/
bool d_tests_interp_read_words()
{
    static const read_case CASES[] =
    {
        { "echo 'a b' \"c \\\"d\\\" \\\\\" e'f'g",
          "cmd(echo, a b, c \"d\" \\, efg)" },
        { "echo 'and' \"|\" ';'",       "cmd(echo, and, |, ;)" },
        { "echo x # a comment\ny",      "seq(cmd(echo, x), cmd(y))" },
        { "echo C# a#b",                "cmd(echo, C#, a#b)" },
        { "# only a comment\necho",     "cmd(echo)" },
        { "make CC=gcc -j4",            "cmd(make, CC=gcc, -j4)" },
        { "ls -1 -ofile --a.b=2 ./x/y", "cmd(ls, -1, -ofile, --a.b=2, ./x/y)" },
        { "echo a\tb",                  "cmd(echo, a, b)" },
        { "echo a\r\nb",                "seq(cmd(echo, a), cmd(b))" }
    };

    return reads_as(CASES, sizeof(CASES) / sizeof(CASES[0]));
}

/*
d_tests_interp_read_refused
  Tests the following:
  - an operand missing, at either side of `|` or between two `;`
  - an unclosed bracket, and an unopened one
  - a second comparison beside a first; a keyword or a comparison with an
    operand missing; a literal followed by a word; a brace, which this
    step gives no role
  - an unclosed quote, at the quote
  - nothing to read
  - each with its first error's place, and no root
*/
bool d_tests_interp_read_refused()
{
    static const struct
    {
        const char*      text;
        re_std::uint32_t offset;
    } CASES[] =
    {
        { "a |",       3u },
        { "| a",       0u },
        { "a ; ; b",   4u },
        { "(a",        0u },
        { "a )",       2u },
        { "a == b == c", 7u },
        { "a < b <= c", 6u },
        { "not",       3u },
        { "a and",     5u },
        { "or a",      0u },
        { "a ==",      4u },
        { "'x' y",     4u },
        { "a { b }",   2u },
        { "echo 'x",   5u },
        { "echo \"x",  5u },
        { "",          0u },
        { "\n  \n",    4u }
    };
    bool held = true;

    for (const auto& each : CASES)
    {
        const script read = djinterp::interp::parse_line(each.text);
        const bool   refused = ( (!read.ok()) &&
                                 (!read.errors().empty()) &&
                                 (read.term() == nullptr) &&
                                 (read.sexpr().empty()) );

        if ( (!refused) ||
             (read.errors().front().offset != each.offset) )
        {
            D_TEST_NOTE("'%s': %s at %u", each.text,
                        (refused) ? read.errors().front().message.c_str()
                                  : "read without an error",
                        (refused) ? (unsigned)read.errors().front().offset
                                  : 0u);
        }

        held = (D_TEST_EXPECT( (refused) &&
                               (read.errors().front().offset ==
                                each.offset) )) &&
               (held);
    }

    return held;
}


/*
d_tests_interp_read_logic
  Tests the following:
  - `or` looser than `and`, `and` than `not`, `not` than a comparison;
    `or` and `and` flattened; `|` looser than them all (decision 3)
  - each comparison a node of two, its operands commands or literals
  - a NUMBER and a STRING are literals where a command could stand, and a
    command's arguments after its verb; a STRING in quotes in the term
  - a predicate continued on the next line after its operator
  - a command line reads as its line
*/
bool d_tests_interp_read_logic()
{
    static const read_case CASES[] =
    {
        { "a and b",                "and(cmd(a), cmd(b))" },
        { "a and b and c",          "and(cmd(a), cmd(b), cmd(c))" },
        { "a or b and c",           "or(cmd(a), and(cmd(b), cmd(c)))" },
        { "a and b or c",           "or(and(cmd(a), cmd(b)), cmd(c))" },
        { "not a and b",            "and(not(cmd(a)), cmd(b))" },
        { "not not a",              "not(not(cmd(a)))" },
        { "not a == b",             "not(==(cmd(a), cmd(b)))" },
        { "test -f x and test -r x | report",
          "pipe(and(cmd(test, -f, x), cmd(test, -r, x)), cmd(report))" },
        { "size > 100",             ">(cmd(size), 100)" },
        { "a == 'x'",               "==(cmd(a), \"x\")" },
        { "a != b ; c <= -1.5",
          "seq(!=(cmd(a), cmd(b)), <=(cmd(c), -1.5))" },
        { "n >= 2 or name =~ 'a.*'",
          "or(>=(cmd(n), 2), =~(cmd(name), \"a.*\"))" },
        { "(a == b) == c",          "==(==(cmd(a), cmd(b)), cmd(c))" },
        { "a < (b)",                "<(cmd(a), cmd(b))" },
        { "echo 'x' 3 -1.5 \"y\"z", "cmd(echo, x, 3, -1.5, yz)" },
        { "-1 < 2",                 "<(-1, 2)" },
        { "'and'",                  "\"and\"" },
        { "a and\n  b",             "and(cmd(a), cmd(b))" },
        { "a ==\n b",               "==(cmd(a), cmd(b))" },
        { "not\n a",                "not(cmd(a))" }
    };
    const script words = djinterp::interp::parse_argv(
        { "size", ">", "100", "and", "not", "x", "==", "-2" });
    const script line  = djinterp::interp::parse_line(
        "size > 100 and not x == -2");
    bool         held  = reads_as(CASES, sizeof(CASES) / sizeof(CASES[0]));

    held = (D_TEST_EXPECT( (words.ok()) &&
                           (words.sexpr() == line.sexpr()) &&
                           (words.sexpr() ==
                            "and(>(cmd(size), 100), not(==(cmd(x), -2)))") &&
                           (words.tokens()[2].cls ==
                            djinterp::interp::token_number) )) && (held);

    return held;
}


//==============================================================================
// 2.  READING A COMMAND LINE
//==============================================================================

/*
d_tests_interp_read_argv
  Tests the following:
  - a word the shell split is one token, whatever its characters; only a
    word that is wholly an operator or a keyword is one
  - the text is the words joined by a space, and each token's span is its
    word's in it
  - a line and its words read as the same term
*/
bool d_tests_interp_read_argv()
{
    const script words = djinterp::interp::parse_argv(
        { "ls", "-l", "|", "grep", "x", ";", "echo", "done" });
    const script line  = djinterp::interp::parse_line(
        "ls -l | grep x ; echo done");
    const script whole = djinterp::interp::parse_argv(
        { "echo", "a;b", "|x", "a b", "(" });
    const script split = djinterp::interp::parse_argv(
        { "echo", "a;b", "|x", "a b" });
    bool         held  = true;

    held = (D_TEST_EXPECT( (words.ok()) &&
                           (words.sexpr() == line.sexpr()) &&
                           (words.text() == "ls -l | grep x ; echo done") &&
                           (words.tokens().size() == 8u) &&
                           (words.tokens()[3].offset == 8u) &&
                           (words.tokens()[3].length == 4u) )) && (held);
    held = (D_TEST_EXPECT( (split.ok()) &&
                           (split.sexpr() == "cmd(echo, a;b, |x, a b)") )) &&
           (held);
    held = (D_TEST_EXPECT( (!whole.ok()) &&
                           (!whole.errors().empty()) )) && (held);

    return held;
}


//==============================================================================
// 3.  VALUES
//==============================================================================

/*
d_tests_interp_values
  Tests the following:
  - each tag made, read back, written as the suites write it
  - equal by tag and payload: an integer is not a real nor a text of it
*/
bool d_tests_interp_values()
{
    const value list   = value::of_list({ value::of_integer(1),
                                          value::of_text("a") });
    const value fields = value::of_record({ { "k", value::of_truth(true) },
                                            { "n", value() } });
    bool        held   = true;

    held = (D_TEST_EXPECT( (value().kind() == value::nothing) &&
                           (value().show() == "nothing") &&
                           (value::of_truth(false).show() == "false") &&
                           (value::of_integer(-3).show() == "-3") &&
                           (value::of_real(2.5).show() == "2.5") &&
                           (value::of_text("t").show() == "\"t\"") &&
                           (value::of_bytes("abc").show() == "bytes(3)") &&
                           (list.show() == "[1, \"a\"]") &&
                           (fields.show() == "{k: true, n: nothing}") )) &&
           (held);
    held = (D_TEST_EXPECT( (value::of_integer(1) == value::of_integer(1)) &&
                           (value::of_integer(1) != value::of_real(1.0)) &&
                           (value::of_integer(1) != value::of_text("1")) &&
                           (value::of_text("x") != value::of_bytes("x")) &&
                           (list == value::of_list(list.items())) &&
                           (list.items().size() == 2u) &&
                           (fields.fields()[0].first == "k") )) && (held);

    return held;
}


//==============================================================================
// 4.  RUNNING
//==============================================================================

/*
d_tests_interp_run_sequences
  Tests the following:
  - `a ; b` runs both, and comes to b's value and status, whatever a's
  - each pipeline of a sequence is given the sequence's input
  - a newline runs as `;` does
  - a group's value goes on down a pipeline
*/
bool d_tests_interp_run_sequences()
{
    bool held = true;

    held = (runs_as("echo a ; echo b", value(), value::of_text("b"), 0, ""))
           && (held);
    held = (runs_as("fail ; seen x", value(), value(), 0, "x")) && (held);
    held = (runs_as("seen x ; no", value(), value::of_text("n"), 1, "x")) &&
           (held);
    held = (runs_as("upper ; seen a ; upper", value::of_text("in"),
                    value::of_text("IN"), 0, "a")) && (held);
    held = (runs_as("seen a\nseen b", value(), value(), 0, "a,b")) && (held);
    held = (runs_as("(echo a ; echo b) | upper", value(),
                    value::of_text("B"), 0, "")) && (held);

    return held;
}

/*
d_tests_interp_run_pipelines
  Tests the following:
  - each stage takes the value of the one before; the first the input
  - a stage that comes to no (1) does not end the pipeline
  - a pipeline in a group, and a group in a pipeline
*/
bool d_tests_interp_run_pipelines()
{
    bool held = true;

    held = (runs_as("echo hi | upper", value(), value::of_text("HI"), 0, ""))
           && (held);
    held = (runs_as("upper | count", value::of_text("four"),
                    value::of_integer(4), 0, "")) && (held);
    held = (runs_as("no | upper", value(), value::of_text("N"), 0, "")) &&
           (held);
    held = (runs_as("echo x | seen a | seen b | upper", value(),
                    value::of_text("X"), 0, "a,b")) && (held);
    held = (runs_as("echo ab | (seen a | count)", value(),
                    value::of_integer(2), 0, "a")) && (held);

    return held;
}

/*
d_tests_interp_run_trouble
  Tests the following:
  - a stage in trouble ends its pipeline: the stages after it not run, its
    outcome the pipeline's, with its span
  - a sequence goes on after a pipeline in trouble
  - a verb no command has, and a command that throws, are trouble
  - a script read with errors is trouble, its first error the message
*/
bool d_tests_interp_run_trouble()
{
    const commands table_of = table();
    outcome        out;
    bool           held     = true;

    held = (runs_as("echo x | fail | seen z", value(), value(), 2, "")) &&
           (held);
    held = (runs_as("fail | seen x ; seen y", value(), value(), 0, "y")) &&
           (held);
    held = (runs_as("seen a | (fail ; seen b) | seen c", value(), value(), 0,
                    "a,b,c")) && (held);

    out  = djinterp::interp::evaluate(
               djinterp::interp::parse_line("echo x | fail | seen z"),
               table_of);
    held = (D_TEST_EXPECT( (out.status == 2) &&
                           (out.message == "fail: failed") &&
                           (out.offset == 9u) &&
                           (out.length == 4u) )) && (held);
    out  = djinterp::interp::evaluate(
               djinterp::interp::parse_line("echo | nosuch a"), table_of);
    held = (D_TEST_EXPECT( (out.status == 2) &&
                           (out.message == "no command is named 'nosuch'") &&
                           (out.offset == 7u) &&
                           (out.length == 8u) )) && (held);
    out  = djinterp::interp::evaluate(
               djinterp::interp::parse_line("throws now"), table_of);
    held = (D_TEST_EXPECT( (out.status == 2) &&
                           (out.message == "throws: boom") )) && (held);
    out  = djinterp::interp::evaluate(djinterp::interp::parse_line("a |"),
                                      table_of);
    held = (D_TEST_EXPECT( (out.status == 2) &&
                           (!out.message.empty()) &&
                           (out.offset == 3u) )) && (held);

    return held;
}


/*
d_tests_interp_run_logic
  Tests the following:
  - `and` runs its next operand only while each before is true, `or` only
    while each is false; each comes to the operand that decided it, its
    status 0 where that is true and 1 where false
  - truth: a text true unless empty, a number unless zero; a command that
    comes to nothing, its status alone, true where it is 0
  - `not` the opposite truth value
  - an operand in trouble is the whole's trouble, the rest not run
  - each operand given the construct's input
*/
bool d_tests_interp_run_logic()
{
    bool held = true;

    held = (runs_as("echo x and echo y", value(), value::of_text("y"), 0,
                    "")) && (held);
    held = (runs_as("echo and seen z", value(), value::of_text(""), 1,
                    "")) && (held);
    held = (runs_as("echo or seen z", value(), value(), 0, "z")) && (held);
    held = (runs_as("echo x or seen z", value(), value::of_text("x"), 0,
                    "")) && (held);
    held = (runs_as("nope or quiet", value(), value(), 0, "")) && (held);
    held = (runs_as("quiet and nope", value(), value(), 1, "")) && (held);
    held = (runs_as("0 or 2", value(), value::of_integer(2), 0, "")) &&
           (held);
    held = (runs_as("1 and 0.0", value(), value::of_real(0.0), 1, "")) &&
           (held);
    held = (runs_as("not echo", value(), value::of_truth(true), 0, "")) &&
           (held);
    held = (runs_as("not echo x", value(), value::of_truth(false), 1,
                    "")) && (held);
    held = (runs_as("fail and seen z", value(), value(), 2, "")) && (held);
    held = (runs_as("fail or seen z", value(), value(), 2, "")) && (held);
    held = (runs_as("not fail", value(), value(), 2, "")) && (held);
    held = (runs_as("seen a and seen b or seen c", value::of_text("in"),
                    value::of_text("in"), 0, "a,b")) && (held);
    held = (runs_as("upper | count == 2 and seen z", value::of_text("ab"),
                    value::of_text("AB"), 0, "z")) && (held);

    return held;
}

/*
d_tests_interp_run_compare
  Tests the following:
  - two numbers numerically, an integer and a real alike; a number and a
    text that reads wholly as one numerically; two texts by code point,
    UTF-8's bytes in its code points' order
  - `==` and `!=` on lists item by item, on records whatever their keys'
    order, the scalars inside by the same rule; on anything else by
    structure
  - an ordering of two values it cannot order is trouble, at the
    comparison
  - `=~` a POSIX ERE matched anywhere in the text; a pattern that does not
    compile is trouble
  - a comparison comes to a truth value, status 0 for true and 1 for false
*/
bool d_tests_interp_run_compare()
{
    const value yes  = value::of_truth(true);
    const value no   = value::of_truth(false);
    bool        held = true;

    held = (runs_as("3 < 10", value(), yes, 0, "")) && (held);
    held = (runs_as("'3' < '10'", value(), no, 1, "")) && (held);
    held = (runs_as("echo 3 < 10", value(), yes, 0, "")) && (held);
    held = (runs_as("1 == 1.0 and 2 != 2.5", value(), yes, 0, "")) &&
           (held);
    held = (runs_as("'1' == '1.0'", value(), no, 1, "")) && (held);
    held = (runs_as("'\xc3\xa9' > 'z' and 'b' >= 'b'", value(), yes, 0,
                    "")) && (held);
    held = (runs_as("count >= 2", value::of_text("ab"), yes, 0, "")) &&
           (held);
    held = (runs_as("pair == reals and rec == cer", value(), yes, 0, "")) &&
           (held);
    held = (runs_as("pair != rec", value(), yes, 0, "")) && (held);
    held = (runs_as("quiet == quiet", value(), yes, 0, "")) && (held);
    held = (runs_as("echo abc =~ 'b+c$'", value(), yes, 0, "")) && (held);
    held = (runs_as("echo abc =~ '^b'", value(), no, 1, "")) && (held);
    held = (runs_as("not 1 == 2", value(), yes, 0, "")) && (held);

    // trouble: at the comparison, with why
    const commands table_of = table();
    const outcome  order    = djinterp::interp::evaluate(
        djinterp::interp::parse_line("echo x ; 'x' < 3"), table_of);
    const outcome  lists    = djinterp::interp::evaluate(
        djinterp::interp::parse_line("pair < reals"), table_of);
    const outcome  pattern  = djinterp::interp::evaluate(
        djinterp::interp::parse_line("echo x =~ '('"), table_of);

    held = (D_TEST_EXPECT( (order.status == 2) &&
                           (order.message ==
                            "'<' does not order \"x\" and 3") &&
                           (order.offset == 9u) &&
                           (order.length == 7u) )) && (held);
    held = (D_TEST_EXPECT( (lists.status == 2) &&
                           (lists.message ==
                            "'<' does not order [1, 2] and [1, 2]") )) &&
           (held);
    held = (D_TEST_EXPECT( (pattern.status == 2) &&
                           (pattern.message.find("'(' is not a pattern") ==
                            0u) )) && (held);

    return held;
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_interp_skeleton
//   constant: the suite's sections, in the order they run.
static const struct dj_test_case_spec d_tests_interp_skeleton[] =
{
    { "interp_read_terms",
      "lines read as their terms: commands, ;, |, newlines, groups",
      d_tests_interp_read_terms },
    { "interp_read_words",
      "words: quotes, escapes, comments, words as written",
      d_tests_interp_read_words },
    { "interp_read_refused",
      "what a line cannot be, each at its first error",
      d_tests_interp_read_refused },
    { "interp_read_logic",
      "and, or, not, the comparisons and literals, by precedence",
      d_tests_interp_read_logic },
    { "interp_read_argv",
      "a command line a token a word, as its line reads",
      d_tests_interp_read_argv },
    { "interp_values",
      "the value's stand-in: each tag, shown and compared",
      d_tests_interp_values },
    { "interp_run_sequences",
      "a ; b: both run, b's outcome; a newline as ;",
      d_tests_interp_run_sequences },
    { "interp_run_pipelines",
      "a | b: each stage the value of the one before",
      d_tests_interp_run_pipelines },
    { "interp_run_trouble",
      "trouble ends a pipeline; unknown verbs, throws, bad scripts",
      d_tests_interp_run_trouble },
    { "interp_run_logic",
      "and and or by truth, short-circuiting; not; trouble in an operand",
      d_tests_interp_run_logic },
    { "interp_run_compare",
      "comparisons by number, by text, by structure; =~ by ERE",
      d_tests_interp_run_compare }
};

// d_tests_interp_skeleton_blocks
//   constant: the suite's one block.
static const struct dj_test_block_spec d_tests_interp_skeleton_blocks[] =
{
    { "skeleton",
      "the interpreter's second step: sequences, pipelines and predicates",
      d_tests_interp_skeleton,
      sizeof(d_tests_interp_skeleton) / sizeof(d_tests_interp_skeleton[0]) }
};

const struct dj_test_module_spec d_tests_interp_skeleton_module =
{
    "interp",
    "the interpreter (language.hpp, evaluate.hpp)",
    d_tests_interp_skeleton_blocks,
    1u
};

#else  // below C++11

// below the skeleton's floor the module has no blocks
const struct dj_test_module_spec d_tests_interp_skeleton_module =
{
    "interp",
    "the interpreter: none below C++11",
    NULL,
    0u
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
