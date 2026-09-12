/******************************************************************************
* djinterp [cli]                                             functional_cli.hpp
*
*   The runtime spine of the functional command line.  Where the parse
* subframework builds parsers and the paradigm subframework matches and
* dispatches, this module wires them into a running interpreter: read a
* line into tokens, parse those tokens into a command term, and fold the
* term into an effect -- threading a value through a `|` pipeline and
* short-circuiting on the first failure.
*
*   THE SPINE, END TO END.
*     line / argv  --reader-->  Σ*            (a vector of arg_tokens)
*     Σ*           --grammar->  command term  (a pipeline of invocations)
*     term         --fold----->  result<Value>
*
* Each leg leans on an existing subframework: the reader is a small
* deterministic lexer; the grammar is a combinator parser over the token
* alphabet (parser/combinators.hpp); the fold is an ordered walk that
* resolves each command's head and threads the value through result /
* maybe (core/functional).
*
*   SURFACE (POSIX + GNU).  The reader classifies each word by shape:
* `--name` and `--name=value` long options, bundled `-xvf` short clusters,
* the `--` end-of-options terminator, the `|` pipeline separator, and bare
* words (the verb and its operands).  Options parse into a plain runtime
* representation -- flags, key/value bindings, operands -- with no
* dependency on the option subframework; that richer algebra (option_set,
* the optionator pipeline) layers on later.
*
*   VALUE PIPE.  `verb-a args | verb-b | verb-c` threads a *value*, not
* text: each command receives the previous command's output and returns
* result<Value, cli_status>; the interpreter stops at the first error.
* The seed value opens the pipeline.
*
*   TWO DELIBERATE CALLS (see the delivery notes):
*     - The command term is realised concretely as command_pipeline (a
*       foldable sequence of invocations), the stand-in for the free-monad
*       term Free Omega A.  The generic functional free-monad carrier is
*       not wired in here; the term and interpreter are structured so the
*       carrier can be re-pointed onto functional::free<Omega, A> later
*       without touching the reader or the grammar.
*     - Command-head dispatch is ordered first-match by name in the
*       registry -- the degenerate case of the paradigm matcher (name
*       equality as the pattern).  The matcher earns its keep in the
*       separate find-style expression module, where dispatch is by
*       pattern rather than by exact name.
*
*
* path:      /inc/djinterp/parse/functional_cli.hpp
* link(s):   ch-command.tex, ch-behavior.tex, ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    STATUS CODES
      ------------

II.   ARGUMENT TOKEN MODEL
      --------------------

III.  READER  (line / argv -> Σ*)
      ---------------------------
      a. read_word / classify_word
      b. read_line / read_argv / from_argv

IV.   PARSED OPTIONS
      --------------

V.    COMMAND TERM
      ------------

VI.   COMMAND GRAMMAR  (Σ* -> command term)
      -------------------------------------
      a. token_satisfy
      b. build_invocation / command grammar
      c.    parse_tokens / parse_line / parse_argv

VII.  COMMAND SPEC & REGISTRY
      -----------------------

VIII. INTERPRETER  (term -> result<Value>)
      ------------------------------------

IX.   STRUCTURAL TRAITS
      -----------------
*/

#ifndef DJINTERP_CLI_FUNCTIONAL_CLI_
#define DJINTERP_CLI_FUNCTIONAL_CLI_ 1

// std
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "../core/functional/maybe.hpp"
#include "../core/functional/result.hpp"
#include "../parse/parse.hpp"
#include "../parse/parser/parser.hpp"
#include "primitives.hpp"
#include "../parse/parser/combinators.hpp"


NS_DJINTERP
NS_CLI


//   functional_cli.hpp introduces no new carrier: tokens are lexed into a
// plain vector, the grammar is composed from parser/combinators.hpp, the
// command term is a std::vector of invocations, and the interpreter folds
// it into result / maybe (core/functional).  Everything here is wiring
// over surfaces defined elsewhere.


///////////////////////////////////////////////////////////////////////////////
///                I.   STATUS CODES                                        ///
///////////////////////////////////////////////////////////////////////////////

// cli_status
//   typedef: classifies the outcome of a CLI operation.  Mirrors
// parse_status / match_status so the status vocabularies read alike.
typedef std::int32_t cli_status;

// DCliStatus*
//   constants: standard CLI status codes.  Command handlers may return
// additional codes at or above DCliStatusUserBase.
constexpr cli_status DCliStatusOk             =  0;
constexpr cli_status DCliStatusParseError     =  1;
constexpr cli_status DCliStatusUnknownCommand =  2;
constexpr cli_status DCliStatusArityError     =  3;
constexpr cli_status DCliStatusPipelineAbort  =  4;
constexpr cli_status DCliStatusUserBase       = 64;


///////////////////////////////////////////////////////////////////////////////
///                II.  ARGUMENT TOKEN MODEL                                ///
///////////////////////////////////////////////////////////////////////////////

// arg_kind
//   typedef: classifies a lexed argument token by its surface shape.
// One concrete instance of the terminal alphabet Sigma consumed by the
// command grammar.
typedef std::int32_t arg_kind;

// DArgKind*
//   constants: the argument token kinds the reader produces.
constexpr arg_kind DArgKindWord       = 0;   // a verb or an operand
constexpr arg_kind DArgKindLong       = 1;   // --name  or  --name=value
constexpr arg_kind DArgKindShort      = 2;   // -x  or a bundled cluster -xvf
constexpr arg_kind DArgKindTerminator = 3;   // --  (end of options)
constexpr arg_kind DArgKindPipe       = 4;   // |  (pipeline stage separator)

// arg_token
//   struct: one lexed argument -- a shape kind plus the payload the
// reader extracted.  For a long option, name holds the option name and
// value / has_value hold an attached =value; for a short cluster, name
// holds the undivided letters; for a word, name holds the text.
struct arg_token
{
    arg_kind    kind;
    std::string name;
    std::string value;
    bool        has_value;

    arg_token()
        : kind      (DArgKindWord),
          name      (),
          value     (),
          has_value (false)
    {}

    arg_token(
        arg_kind    _kind,
        std::string _name,
        std::string _value,
        bool        _has_value
    )
        : kind      (_kind),
          name      (std::move(_name)),
          value     (std::move(_value)),
          has_value (_has_value)
    {}
};

// word_token
//   factory: a bare word (a verb or an operand).
D_NODISCARD
inline arg_token
word_token
(
    const std::string& _text
)
{
    return arg_token(DArgKindWord, _text, std::string(), false);
}

// long_token
//   factory: a long option, optionally carrying an attached value.
D_NODISCARD
inline arg_token
long_token
(
    const std::string& _name,
    const std::string& _value,
    bool               _has_value
)
{
    return arg_token(DArgKindLong, _name, _value, _has_value);
}

// short_token
//   factory: a short option or bundled cluster (the letters, undivided).
D_NODISCARD
inline arg_token
short_token
(
    const std::string& _cluster
)
{
    return arg_token(DArgKindShort, _cluster, std::string(), false);
}

// terminator_token
//   factory: the `--` end-of-options marker.
D_NODISCARD
inline arg_token
terminator_token()
{
    return arg_token(DArgKindTerminator, std::string(), std::string(), false);
}

// pipe_token
//   factory: the `|` pipeline stage separator.
D_NODISCARD
inline arg_token
pipe_token()
{
    return arg_token(DArgKindPipe, std::string(), std::string(), false);
}


///////////////////////////////////////////////////////////////////////////////
///                III. READER  (line / argv -> Sigma*)                    ///
///////////////////////////////////////////////////////////////////////////////

// =================================================================
//  a. read_word / classify_word
// =================================================================

// read_word
//   function: consumes one word from _line starting at _pos, advancing
// _pos past it.  A word runs to the next unquoted whitespace or `|`;
// double- and single-quoted spans are included with their quotes
// stripped, so a quoted operand may contain spaces or a literal `|`.
// _pos is assumed to sit on a non-whitespace character.
D_NODISCARD
inline std::string
read_word
(
    const std::string& _line,
    std::size_t&       _pos
)
{
    std::string       word;
    const std::size_t n = _line.size();

    while (_pos < n)
    {
        const char c = _line[_pos];

        // an unquoted separator ends the word
        if ((c == ' ') || (c == '\t') || (c == '|'))
        {
            break;
        }

        // a quoted span contributes its contents, minus the quotes
        if ((c == '"') || (c == '\''))
        {
            const char quote = c;
            ++_pos;

            while ((_pos < n) && (_line[_pos] != quote))
            {
                word.push_back(_line[_pos]);
                ++_pos;
            }

            if (_pos < n)
            {
                ++_pos;   // consume the closing quote
            }

            continue;
        }

        word.push_back(c);
        ++_pos;
    }

    return word;
}

// classify_word
//   function: assigns an arg_token kind to one already-extracted word.
// Once _operands_only is set (by a preceding `--`) every word is a plain
// operand.  A lone `-` is treated as a word, not a short option.
D_NODISCARD
inline arg_token
classify_word
(
    const std::string& _raw,
    bool               _operands_only
)
{
    // after a `--` terminator, everything remaining is an operand
    if (_operands_only)
    {
        return word_token(_raw);
    }

    // the bare `--` end-of-options terminator
    if (_raw == "--")
    {
        return terminator_token();
    }

    // a long option: --name  or  --name=value
    if ((_raw.size() >= 2) && (_raw[0] == '-') && (_raw[1] == '-'))
    {
        const std::string           body = _raw.substr(2);
        const std::string::size_type eq  = body.find('=');

        if (eq == std::string::npos)
        {
            return long_token(body, std::string(), false);
        }

        return long_token(body.substr(0, eq), body.substr(eq + 1), true);
    }

    // a short option or bundled cluster: -x , -xvf   (but not a lone `-`)
    if ((_raw.size() >= 2) && (_raw[0] == '-'))
    {
        return short_token(_raw.substr(1));
    }

    // otherwise a plain word: the verb or an operand
    return word_token(_raw);
}


// =================================================================
//  b. read_line / read_argv / from_argv
// =================================================================

// read_line
//   function: lexes a full command line into a token stream (Sigma*).
// Words are split on unquoted whitespace; `|` is a stage separator that
// resets option parsing for the next stage.
D_NODISCARD
inline std::vector<arg_token>
read_line
(
    const std::string& _line
)
{
    std::vector<arg_token> out;
    std::size_t            pos           = 0;
    const std::size_t      n             = _line.size();
    bool                   operands_only = false;

    while (pos < n)
    {
        // skip inter-word whitespace
        while ((pos < n) && ((_line[pos] == ' ') || (_line[pos] == '\t')))
        {
            ++pos;
        }

        if (pos >= n)
        {
            break;
        }

        // a pipeline separator opens a fresh stage
        if (_line[pos] == '|')
        {
            out.push_back(pipe_token());
            operands_only = false;
            ++pos;
            continue;
        }

        const std::string raw = read_word(_line, pos);
        const arg_token    tok = classify_word(raw, operands_only);

        if (tok.kind == DArgKindTerminator)
        {
            operands_only = true;
        }

        out.push_back(tok);
    }

    return out;
}

// read_argv
//   function: lexes an already-split argument vector into a token stream.
// Each entry is one word (the shell has done the splitting); a literal
// `|` entry is honoured as a stage separator.
D_NODISCARD
inline std::vector<arg_token>
read_argv
(
    const std::vector<std::string>& _args
)
{
    std::vector<arg_token> out;
    bool                   operands_only = false;

    for (std::size_t i = 0; i < _args.size(); ++i)
    {
        const std::string& raw = _args[i];

        if ((!operands_only) && (raw == "|"))
        {
            out.push_back(pipe_token());
            operands_only = false;
            continue;
        }

        const arg_token tok = classify_word(raw, operands_only);

        if (tok.kind == DArgKindTerminator)
        {
            operands_only = true;
        }

        out.push_back(tok);
    }

    return out;
}

// from_argv
//   function: collects a C-style (argc, argv) pair into a word vector,
// conventionally skipping argv[0] (the program name).
D_NODISCARD
inline std::vector<std::string>
from_argv
(
    int    _argc,
    char** _argv
)
{
    std::vector<std::string> args;

    for (int i = 1; i < _argc; ++i)
    {
        args.push_back(std::string(_argv[i]));
    }

    return args;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  PARSED OPTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

// parsed_options
//   class: the plain runtime representation of one command's arguments --
// the flags it was given, its --key=value bindings, and its positional
// operands.  Deliberately not the option subframework's option_set: no
// schema, polarity, or algebra, just what the reader observed.  The
// richer model layers on later.
class parsed_options
{
public:
    parsed_options()
        : m_flags    (),
          m_values   (),
          m_operands ()
    {}

    // -----------------------------------------------------------------
    //  builders (used by the grammar when assembling an invocation)
    // -----------------------------------------------------------------

    // add_flag
    //   method: records a flag (-x or --name with no value).
    void
    add_flag
    (
        const std::string& _name
    )
    {
        m_flags.push_back(_name);

        return;
    }

    // add_value
    //   method: records a --key=value binding.
    void
    add_value
    (
        const std::string& _key,
        const std::string& _value
    )
    {
        m_values.push_back(std::make_pair(_key, _value));

        return;
    }

    // add_operand
    //   method: records a positional operand.
    void
    add_operand
    (
        const std::string& _operand
    )
    {
        m_operands.push_back(_operand);

        return;
    }

    // -----------------------------------------------------------------
    //  queries
    // -----------------------------------------------------------------

    // has_flag
    //   method: true iff the named flag was supplied.
    D_NODISCARD
    bool
    has_flag
    (
        const std::string& _name
    ) const
    {
        for (std::size_t i = 0; i < m_flags.size(); ++i)
        {
            if (m_flags[i] == _name)
            {
                return true;
            }
        }

        return false;
    }

    // value_of
    //   method: the value bound to a --key=value option, or nothing.
    // Returns the first binding when a key repeats.
    D_NODISCARD
    maybe<std::string>
    value_of
    (
        const std::string& _key
    ) const
    {
        for (std::size_t i = 0; i < m_values.size(); ++i)
        {
            if (m_values[i].first == _key)
            {
                return just(m_values[i].second);
            }
        }

        return nothing<std::string>();
    }

    // accessors
    D_NODISCARD
    const std::vector<std::string>& flags() const { return m_flags; }

    D_NODISCARD
    const std::vector<std::pair<std::string, std::string>>& values() const
    {
        return m_values;
    }

    D_NODISCARD
    const std::vector<std::string>& operands() const { return m_operands; }

private:
    std::vector<std::string>                         m_flags;
    std::vector<std::pair<std::string, std::string>> m_values;
    std::vector<std::string>                         m_operands;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   COMMAND TERM                                       ///
///////////////////////////////////////////////////////////////////////////////

// command_invocation
//   struct: one node of a parsed command term -- a verb naming the
// command plus the options and operands bound to it.  The concrete
// Omega-application: "apply the command named `verb` with these
// arguments".
struct command_invocation
{
    std::string    verb;
    parsed_options options;

    command_invocation()
        : verb    (),
          options ()
    {}
};

// command_pipeline
//   type: a parsed command term -- a sequence of invocations threaded by
// the value pipe `|`.  This is the concrete stand-in for the free-monad
// term Free Omega A: a foldable program the interpreter walks.  When the
// functional free-monad carrier is wired into the CLI it can be re-
// pointed onto functional::free<Omega, A> without disturbing the reader,
// the grammar, or the interpreter -- exactly as the parser strata in
// parser/free.hpp will be re-pointed at their functional companions.
typedef std::vector<command_invocation> command_pipeline;


///////////////////////////////////////////////////////////////////////////////
///                VI.  COMMAND GRAMMAR  (Sigma* -> command term)          ///
///////////////////////////////////////////////////////////////////////////////

// =================================================================
//  a. token_satisfy
// =================================================================

// token_satisfy
//   function: a single-token parser accepting one arg_token that passes
// _predicate.  satisfy() defaults its element to char, so the element is
// pinned to arg_token explicitly here; the result is erased to a handle
// for uniform composition.
template<typename _Predicate>
D_NODISCARD
parse::parser<arg_token, arg_token>
token_satisfy
(
    _Predicate         _predicate,
    const std::string& _label
)
{
    return parse::satisfy<_Predicate, arg_token>(_predicate, _label);
}


// =================================================================
//  b. build_invocation / command grammar
// =================================================================

// build_invocation
//   function: folds a verb token and its item tokens into a
// command_invocation.  Long options become flags or key/value bindings;
// a short cluster expands letter by letter into flags; words become
// operands.
D_NODISCARD
inline command_invocation
build_invocation
(
    const arg_token&              _verb,
    const std::vector<arg_token>& _items
)
{
    command_invocation inv;
    inv.verb = _verb.name;

    for (std::size_t i = 0; i < _items.size(); ++i)
    {
        const arg_token& it = _items[i];

        if (it.kind == DArgKindLong)
        {
            if (it.has_value)
            {
                inv.options.add_value(it.name, it.value);
            }
            else
            {
                inv.options.add_flag(it.name);
            }
        }
        else if (it.kind == DArgKindShort)
        {
            // a bundled cluster -xvf expands to flags x, v, f
            for (std::size_t k = 0; k < it.name.size(); ++k)
            {
                inv.options.add_flag(std::string(1, it.name[k]));
            }
        }
        else   // DArgKindWord
        {
            inv.options.add_operand(it.name);
        }
    }

    return inv;
}

// stage_parser
//   function: the parser for one pipeline stage -- a verb followed by any
// number of options and operands -- mapped into a command_invocation.
// The verb is the leading word; every subsequent item is an option or an
// operand.
D_NODISCARD
inline parse::parser<command_invocation, arg_token>
stage_parser()
{
    parse::parser<arg_token, arg_token> verb_p =
        token_satisfy(
            [](const arg_token& _t) { return _t.kind == DArgKindWord; },
            "verb");

    parse::parser<arg_token, arg_token> option_p =
        token_satisfy(
            [](const arg_token& _t)
            {
                return (_t.kind == DArgKindLong) ||
                       (_t.kind == DArgKindShort);
            },
            "option");

    parse::parser<arg_token, arg_token> operand_p =
        token_satisfy(
            [](const arg_token& _t) { return _t.kind == DArgKindWord; },
            "operand");

    parse::parser<arg_token, arg_token> item_p =
        parse::or_(option_p, operand_p);

    // read the verb, then map its trailing items into the invocation
    return parse::bind(
        verb_p,
        [item_p](const arg_token& _verb)
        {
            return parse::map(
                parse::many(item_p),
                [_verb](const std::vector<arg_token>& _items)
                {
                    return build_invocation(_verb, _items);
                });
        });
}

// build_command_parser
//   function: the whole-line grammar -- one or more stages separated by
// `|`, consuming all input.  Yields the command term (the pipeline).
D_NODISCARD
inline parse::parser<command_pipeline, arg_token>
build_command_parser()
{
    parse::parser<command_invocation, arg_token> stage_p = stage_parser();

    parse::parser<arg_token, arg_token> pipe_p =
        token_satisfy(
            [](const arg_token& _t) { return _t.kind == DArgKindPipe; },
            "pipe");

    return parse::seq_l(
        parse::sep_by1(stage_p, pipe_p),
        parse::eof<arg_token>());
}


// =================================================================
//  c. parse_tokens / parse_line / parse_argv
// =================================================================

// parse_tokens
//   function: runs the command grammar over a token stream, yielding the
// command term or a parse error lifted into cli_status.
D_NODISCARD
inline result<command_pipeline, cli_status>
parse_tokens
(
    const std::vector<arg_token>& _tokens
)
{
    parse::parser<command_pipeline, arg_token> grammar =
        build_command_parser();

    parse::parse_state<arg_token>  state(_tokens.data(), _tokens.size());
    parse::parse_result<command_pipeline> outcome = grammar.parse(state);

    if (outcome.ok())
    {
        return ok<command_pipeline, cli_status>(outcome.value());
    }

    return err<command_pipeline, cli_status>(DCliStatusParseError);
}

// parse_line
//   function: reads a command line and parses it into a command term.
D_NODISCARD
inline result<command_pipeline, cli_status>
parse_line
(
    const std::string& _line
)
{
    return parse_tokens(read_line(_line));
}

// parse_argv
//   function: reads an argument vector and parses it into a command term.
D_NODISCARD
inline result<command_pipeline, cli_status>
parse_argv
(
    const std::vector<std::string>& _args
)
{
    return parse_tokens(read_argv(_args));
}


///////////////////////////////////////////////////////////////////////////////
///                VII. COMMAND SPEC & REGISTRY                            ///
///////////////////////////////////////////////////////////////////////////////

// command_spec
//   class: a registered command -- a name and the effect it performs.
// The effect (the handler) receives the command's parsed options and the
// value threaded in from the pipeline, and returns the value to thread
// on, or an error to abort the pipeline.  _Value is the type carried by
// the `|` pipe.
template<typename _Value>
class command_spec
{
public:
    using value_type   = _Value;
    using handler_type =
        std::function<result<_Value, cli_status>(
            const parsed_options&,
            const _Value&)>;

    command_spec(
        std::string  _name,
        handler_type _handler
    )
        : m_name    (std::move(_name)),
          m_handler (std::move(_handler))
    {}

    // name
    D_NODISCARD
    const std::string& name() const { return m_name; }

    // invoke
    //   method: applies the command's handler to its parsed options and
    // the threaded input value.
    D_NODISCARD
    result<_Value, cli_status>
    invoke
    (
        const parsed_options& _options,
        const _Value&         _input
    ) const
    {
        return m_handler(_options, _input);
    }

private:
    std::string  m_name;
    handler_type m_handler;
};

// command_registry
//   class: the set of commands the interpreter can dispatch -- the
// concrete Omega.  resolve() performs command-head dispatch: ordered
// first-match by name, the same ordered-choice device as the paradigm
// matcher and the parser's or_.
template<typename _Value>
class command_registry
{
public:
    using value_type   = _Value;
    using spec_type    = command_spec<_Value>;
    using handler_type = typename spec_type::handler_type;

    command_registry()
        : m_specs()
    {}

    // define
    //   method: registers a command by name and handler.  Registration
    // order is dispatch precedence.  Returns *this for chaining.
    command_registry&
    define
    (
        std::string  _name,
        handler_type _handler
    )
    {
        m_specs.push_back(spec_type(std::move(_name), std::move(_handler)));

        return *this;
    }

    // resolve
    //   method: the command bound to _name, or nothing.
    D_NODISCARD
    maybe<spec_type>
    resolve
    (
        const std::string& _name
    ) const
    {
        for (std::size_t i = 0; i < m_specs.size(); ++i)
        {
            if (m_specs[i].name() == _name)
            {
                return just(m_specs[i]);
            }
        }

        return nothing<spec_type>();
    }

    // introspection
    D_NODISCARD
    bool contains(const std::string& _name) const
    {
        return resolve(_name).has_value();
    }

    D_NODISCARD
    std::size_t size() const { return m_specs.size(); }

    D_NODISCARD
    bool empty() const { return m_specs.empty(); }

private:
    std::vector<spec_type> m_specs;
};


///////////////////////////////////////////////////////////////////////////////
///                VIII. INTERPRETER  (term -> result<Value>)              ///
///////////////////////////////////////////////////////////////////////////////

// run
//   function: interprets a command term -- the fold leg.  Walks the
// pipeline left to right, resolving each invocation's head in the
// registry and threading the value through its effect, and short-circuits
// on the first error (an unresolved command, or a handler failure).  The
// seed opens the pipeline; the final value (or error) is the result.
//
//   This is the concrete realisation of a fold over Free Omega A that
// dispatches each node on its head -- with command_pipeline standing in
// for the free term and the registry standing in for the interpretation
// algebra.
template<typename _Value>
D_NODISCARD
result<_Value, cli_status>
run
(
    const command_pipeline&         _pipeline,
    const command_registry<_Value>& _registry,
    const _Value&                   _seed
)
{
    result<_Value, cli_status> acc = ok<_Value, cli_status>(_seed);

    for (std::size_t i = 0; i < _pipeline.size(); ++i)
    {
        // a prior stage failed -- stop threading
        if (acc.is_err())
        {
            break;
        }

        const command_invocation&   inv  = _pipeline[i];
        maybe<command_spec<_Value>>  spec = _registry.resolve(inv.verb);

        if (spec.is_nothing())
        {
            return err<_Value, cli_status>(DCliStatusUnknownCommand);
        }

        acc = spec.value().invoke(inv.options, acc.value());
    }

    return acc;
}

// evaluate
//   function: the whole spine for a command line -- read, parse, and run.
// A parse failure is reported before any command executes.
template<typename _Value>
D_NODISCARD
result<_Value, cli_status>
evaluate
(
    const std::string&              _line,
    const command_registry<_Value>& _registry,
    const _Value&                   _seed
)
{
    result<command_pipeline, cli_status> parsed = parse_line(_line);

    if (parsed.is_err())
    {
        return err<_Value, cli_status>(parsed.error());
    }

    return run(parsed.value(), _registry, _seed);
}

// evaluate_argv
//   function: the whole spine for an argument vector -- read, parse, run.
template<typename _Value>
D_NODISCARD
result<_Value, cli_status>
evaluate_argv
(
    const std::vector<std::string>& _args,
    const command_registry<_Value>& _registry,
    const _Value&                   _seed
)
{
    result<command_pipeline, cli_status> parsed = parse_argv(_args);

    if (parsed.is_err())
    {
        return err<_Value, cli_status>(parsed.error());
    }

    return run(parsed.value(), _registry, _seed);
}


///////////////////////////////////////////////////////////////////////////////
///                IX.  STRUCTURAL TRAITS                                  ///
///////////////////////////////////////////////////////////////////////////////

// is_command_spec
//   trait: detects a command-spec-shaped type -- a nested value_type plus
// a name() face.
template<typename _Type,
         typename = void>
struct is_command_spec : std::false_type
{};

template<typename _Type>
struct is_command_spec<_Type, void_t<
    typename _Type::value_type,
    decltype(std::declval<const _Type&>().name())
>> : std::true_type
{};


// is_command_registry
//   trait: detects a registry-shaped type -- a nested spec_type plus a
// resolve(name) face.
template<typename _Type,
         typename = void>
struct is_command_registry : std::false_type
{};

template<typename _Type>
struct is_command_registry<_Type, void_t<
    typename _Type::spec_type,
    decltype(
        std::declval<const _Type&>().resolve(
            std::declval<const std::string&>()))
>> : std::true_type
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_command_spec_v
//   constant: shorthand for is_command_spec<_Type>::value.
template<typename _Type>
static D_CONSTEXPR bool is_command_spec_v = is_command_spec<_Type>::value;

// is_command_registry_v
//   constant: shorthand for is_command_registry<_Type>::value.
template<typename _Type>
static D_CONSTEXPR bool is_command_registry_v =
    is_command_registry<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if ( defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                             \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )

// command_spec_c
//   concept: satisfied by types exposing the command-spec surface.
template<typename _Type>
concept command_spec_c = is_command_spec<_Type>::value;

// command_registry_c
//   concept: satisfied by types exposing the registry surface.
template<typename _Type>
concept command_registry_c = is_command_registry<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // cli
NS_END  // djinterp


#endif  // DJINTERP_CLI_FUNCTIONAL_CLI_
