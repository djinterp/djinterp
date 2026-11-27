/*******************************************************************************
* djinterp [cpp][interp]                                            evaluate.cpp
*
* The command language run, declared in evaluate.hpp: the value's stand-in,
* the commands' table, what the operators do, and the fold, a walk of the
* term.
*   The walk keeps a frame for each application it is inside: the input the
* node was given, what its first child and its last came to, and whether
* the rest are to be skipped. Entering a node gives it its input from its
* parent's frame; leaving it folds what its children came to and gives that
* back. A command is run where it is entered, its words read from its
* leaves, which the walk then skips; a literal is its value.
*
*
* path:      /src/djinterp/cpp/modules/interp/evaluate.cpp
* link(s):   /docs/djinterp/cpp/modules/interp/DESIGN.html
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.25
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/interp/evaluate.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdio>     // std::snprintf
#include <cstdlib>    // std::strtod, std::strtoll
#include <exception>  // std::exception
#include <string>     // std::string
#include <utility>    // std::move
#include <vector>     // std::vector
// djinterp
#include "../../../../../inc/djinterp/c/modules/parse/expr/expr.h"  // core v2
#include "../../../../../inc/djinterp/c/util/dregex.h"  // =~'s patterns


NS_DJINTERP
NS_INTERP


//==============================================================================
// 1.  VALUES
//==============================================================================

value::value()
    : m_kind(nothing),
      m_truth(false),
      m_integer(0),
      m_real(0.0)
{
}

value value::of_truth(bool _truth)
{
    value out;

    out.m_kind  = truth;
    out.m_truth = _truth;

    return out;
}

value value::of_integer(re_std::int64_t _integer)
{
    value out;

    out.m_kind    = integer;
    out.m_integer = _integer;

    return out;
}

value value::of_real(double _real)
{
    value out;

    out.m_kind = real;
    out.m_real = _real;

    return out;
}

value value::of_text(const std::string& _text)
{
    value out;

    out.m_kind = text;
    out.m_text = _text;

    return out;
}

value value::of_bytes(const std::string& _bytes)
{
    value out;

    out.m_kind = bytes;
    out.m_text = _bytes;

    return out;
}

value value::of_list(const std::vector<value>& _items)
{
    value out;

    out.m_kind  = list;
    out.m_items = _items;

    return out;
}

value value::of_record(
    const std::vector<std::pair<std::string, value> >& _fields
)
{
    value out;

    out.m_kind   = record;
    out.m_fields = _fields;

    return out;
}

value::kind_t value::kind() const { return m_kind; }
bool value::as_truth() const { return m_truth; }
re_std::int64_t value::as_integer() const { return m_integer; }
double value::as_real() const { return m_real; }
const std::string& value::as_text() const { return m_text; }
const std::vector<value>& value::items() const { return m_items; }

const std::vector<std::pair<std::string, value> >& value::fields() const
{
    return m_fields;
}

bool value::operator==(const value& _other) const
{
    return ( (m_kind == _other.m_kind) &&
             (m_truth == _other.m_truth) &&
             (m_integer == _other.m_integer) &&
             ( (m_real == _other.m_real) ||
               ( (m_real != m_real) && (_other.m_real != _other.m_real) ) ) &&
             (m_text == _other.m_text) &&
             (m_items == _other.m_items) &&
             (m_fields == _other.m_fields) );
}

bool value::operator!=(const value& _other) const
{
    return !(*this == _other);
}

std::string value::show() const
{
    char        number[40];
    std::string out;

    switch (m_kind)
    {
        case truth:
            return (m_truth) ? "true" : "false";
        case integer:
            (void)std::snprintf(number, sizeof(number), "%lld",
                                (long long)m_integer);
            return number;
        case real:
            (void)std::snprintf(number, sizeof(number), "%.17g", m_real);
            return number;
        case text:
            return "\"" + m_text + "\"";
        case bytes:
            (void)std::snprintf(number, sizeof(number), "bytes(%lu)",
                                (unsigned long)m_text.size());
            return number;
        case list:
            for (std::size_t i = 0u; i < m_items.size(); ++i)
            {
                out += ( (i > 0u) ? ", " : "" ) + m_items[i].show();
            }
            return "[" + out + "]";
        case record:
            for (std::size_t i = 0u; i < m_fields.size(); ++i)
            {
                out += ( (i > 0u) ? ", " : "" ) + m_fields[i].first + ": " +
                       m_fields[i].second.show();
            }
            return "{" + out + "}";
        default:
            return "nothing";
    }
}


//==============================================================================
// 2.  COMMANDS
//==============================================================================

void commands::add(
    const std::string& _name,
    command_fn         _run
)
{
    m_table[_name] = std::move(_run);

    return;
}

const command_fn* commands::find(
    const std::string& _name
) const
{
    const std::map<std::string, command_fn>::const_iterator found =
        m_table.find(_name);

    return ( (found != m_table.end()) &&
             (found->second) ) ? &found->second : nullptr;
}


//==============================================================================
// 3.  WHAT THE OPERATORS DO
//==============================================================================

/*
truth_of
  What decides an `and`, an `or` and a `not` (DESIGN.html, section 5): a
truth value as it is; zero, an empty text, bytes, list or record false.
The stand-in has no tag for a command's status alone, so a construct that
comes to nothing is its status: true where it is 0.
*/
static bool
truth_of(
    const outcome& _outcome
)
{
    const value& it = _outcome.result;

    switch (it.kind())
    {
        case value::truth:   return it.as_truth();
        case value::integer: return (it.as_integer() != 0);
        case value::real:    return (it.as_real() != 0.0);
        case value::text:
        case value::bytes:   return (!it.as_text().empty());
        case value::list:    return (!it.items().empty());
        case value::record:  return (!it.fields().empty());
        default:             return (_outcome.status == status_yes);
    }
}

/*
decided
  A predicate's outcome: its value, and the status its truth gives, 0 yes
and 1 no.
*/
static outcome
decided(
    const value& _value,
    bool         _truth
)
{
    return outcome{ _value, (_truth) ? status_yes : status_no,
                    std::string(), 0u, 0u };
}

// number
//   struct: a value read as a number: an integer, or a real.
struct number
{
    bool            whole;
    re_std::int64_t integer;
    double          real;
};

/*
number_of
  A value as a number: an integer or a real as it is; with `_text`, a text
that reads wholly as one - an optional '-', digits, a fraction - too.
False for anything else.
*/
static bool
number_of(
    const value& _value,
    bool         _text,
    number&      _out
)
{
    if (_value.kind() == value::integer)
    {
        _out = number{ true, _value.as_integer(), 0.0 };

        return true;
    }

    if (_value.kind() == value::real)
    {
        _out = number{ false, 0, _value.as_real() };

        return true;
    }

    if ( (!_text) ||
         (_value.kind() != value::text) )
    {
        return false;
    }

    const std::string& word = _value.as_text();

    if (!is_number(word))
    {
        return false;
    }

    _out.whole   = (word.find('.') == std::string::npos);
    _out.integer = (_out.whole) ? (re_std::int64_t)std::strtoll(
                                      word.c_str(), nullptr, 10)
                                : 0;
    _out.real    = (_out.whole) ? 0.0 : std::strtod(word.c_str(), nullptr);

    return true;
}

/*
order_of
  How two numbers order: -1, 0 or 1; integers exactly, else as reals.
*/
static int
order_of(
    const number& _a,
    const number& _b
)
{
    if ( (_a.whole) && (_b.whole) )
    {
        return (_a.integer < _b.integer) ? -1 : (_a.integer > _b.integer);
    }

    const double a = (_a.whole) ? (double)_a.integer : _a.real;
    const double b = (_b.whole) ? (double)_b.integer : _b.real;

    return (a < b) ? -1 : (a > b);
}

/*
same_as
  `==` by the language's rule, for two values of any kind: two numbers, or
a number and a text that reads as one, numerically; two lists item by
item; two records by their keys, whatever their order, and each key's
value; anything else by structure.
*/
static bool
same_as(
    const value& _a,
    const value& _b
)
{
    number a;
    number b;

    if ( (number_of(_a, (_b.kind() == value::integer) ||
                        (_b.kind() == value::real), a)) &&
         (number_of(_b, (_a.kind() == value::integer) ||
                        (_a.kind() == value::real), b)) )
    {
        return (order_of(a, b) == 0);
    }

    if ( (_a.kind() == value::list) &&
         (_b.kind() == value::list) )
    {
        bool same = (_a.items().size() == _b.items().size());

        for (std::size_t i = 0u; (same) && (i < _a.items().size()); ++i)
        {
            same = same_as(_a.items()[i], _b.items()[i]);
        }

        return same;
    }

    if ( (_a.kind() == value::record) &&
         (_b.kind() == value::record) )
    {
        bool same = (_a.fields().size() == _b.fields().size());

        for (const auto& field : _a.fields())
        {
            bool found = false;

            for (const auto& other : _b.fields())
            {
                found = ( (found) ||
                          ( (other.first == field.first) &&
                            (same_as(field.second, other.second)) ) );
            }

            same = (same) && (found);
        }

        return same;
    }

    return (_a == _b);
}

/*
matches
  `a =~ b`: b's text a POSIX ERE (c/util/dregex.h), true where it matches
anywhere in a's text; a number's text is its own. Trouble, with why, for
a pattern that does not compile or an operand that is no text.
*/
static outcome
matches(
    const value& _a,
    const value& _b
)
{
    const bool texts = ( ( (_a.kind() == value::text) ||
                           (_a.kind() == value::integer) ||
                           (_a.kind() == value::real) ) &&
                         (_b.kind() == value::text) );
    const std::string subject = (_a.kind() == value::text) ? _a.as_text()
                                                          : _a.show();

    if (!texts)
    {
        return outcome{ value(), status_trouble,
                        "=~ matches a text against a pattern's text, not " +
                        _a.show() + " against " + _b.show(), 0u, 0u };
    }

    enum dj_regex_status   status  = D_REGEX_OK;
    struct dj_regex* const pattern = d_regex_compile(_b.as_text().c_str(),
                                                     &status);

    if (pattern == nullptr)
    {
        return outcome{ value(), status_trouble,
                        "'" + _b.as_text() + "' is not a pattern: " +
                        d_regex_status_text(status), 0u, 0u };
    }

    const bool found = d_regex_test(pattern, subject.data(),
                                    subject.size());

    d_regex_free(pattern);

    return decided(value::of_truth(found), found);
}

/*
compared
  A comparison's outcome (DESIGN.html, section 5): two numbers, or a number
and a text that reads wholly as one, numerically; two texts by code point;
`==` and `!=` on anything by same_as; the orderings on anything else
trouble.
*/
static outcome
compared(
    re_std::uint32_t _op,
    const value&     _a,
    const value&     _b
)
{
    static const char* const SPELLED[] = { "<", "<=", ">", ">=" };
    const bool                numeric = ( (_a.kind() == value::integer) ||
                                          (_a.kind() == value::real) ||
                                          (_b.kind() == value::integer) ||
                                          (_b.kind() == value::real) );
    number                    a;
    number                    b;
    int                       order   = 0;

    if (_op == (re_std::uint32_t)op_match)
    {
        return matches(_a, _b);
    }

    if ( (_op == (re_std::uint32_t)op_eq) ||
         (_op == (re_std::uint32_t)op_ne) )
    {
        const bool same = same_as(_a, _b);
        const bool yes  = (_op == (re_std::uint32_t)op_eq) ? same : !same;

        return decided(value::of_truth(yes), yes);
    }

    if ( (numeric) &&
         (number_of(_a, true, a)) &&
         (number_of(_b, true, b)) )
    {
        order = order_of(a, b);
    }
    else if ( (_a.kind() == value::text) &&
              (_b.kind() == value::text) )
    {
        // UTF-8's bytes order as its code points do
        order = _a.as_text().compare(_b.as_text());
        order = (order < 0) ? -1 : (order > 0);
    }
    else
    {
        return outcome{ value(), status_trouble,
                        std::string("'") + SPELLED[_op - op_lt] +
                        "' does not order " + _a.show() + " and " +
                        _b.show(), 0u, 0u };
    }

    const bool yes = (_op == (re_std::uint32_t)op_lt) ? (order < 0)
                   : (_op == (re_std::uint32_t)op_le) ? (order <= 0)
                   : (_op == (re_std::uint32_t)op_gt) ? (order > 0)
                                                      : (order >= 0);

    return decided(value::of_truth(yes), yes);
}

/*
literal_of
  A literal leaf's value: a NUMBER an integer, or a real where it has a
fraction; a STRING its text, quotes taken off.
*/
static value
literal_of(
    const script&                    _script,
    re_std::uint32_t                 _node,
    const struct dj_parse_expr_node* _leaf
)
{
    const std::string& word = _script.word(_node);
    number             read;

    if ( (_leaf->op == (re_std::uint32_t)token_number) &&
         (number_of(value::of_text(word), true, read)) )
    {
        return (read.whole) ? value::of_integer(read.integer)
                            : value::of_real(read.real);
    }

    return value::of_text(word);
}


//==============================================================================
// 4.  THE FOLD
//==============================================================================

// frame
//   struct: an application the walk is inside: its node and operator; the
// input it was given; what its first child and its last came to; whether a
// child has come to anything; and whether the rest are to be skipped - a
// pipeline's after a stage in trouble, an `and`'s after a false operand,
// an `or`'s after a true one, any operator's after one in trouble.
struct frame
{
    re_std::uint32_t node;
    re_std::uint32_t op;
    value            input;
    outcome          first;
    outcome          last;
    bool             any;
    bool             broken;
};

// walk
//   struct: the fold's state: the script, the commands, the frames from the
// root in, and what the root came to.
struct walk
{
    const script*      code;
    const commands*    table;
    value              seed;
    std::vector<frame> frames;
    outcome            done;
    bool               finished;
};

/*
trouble
  An outcome of trouble: no value, status 2, its words and span.
*/
static outcome
trouble(
    const std::string&               _message,
    const struct dj_parse_expr_node* _node
)
{
    return outcome{ value(), status_trouble, _message,
                    (_node != nullptr) ? _node->offset : 0u,
                    (_node != nullptr) ? _node->length : 0u };
}

/*
run_command
  A command node: its words from its leaves, the verb first; the command the
verb names, run with them and the input; trouble for a verb no command has,
and for a command that throws.
*/
static outcome
run_command(
    const walk&      _walk,
    re_std::uint32_t _node,
    const value&     _input
)
{
    const struct dj_parse_expr_term* const term = _walk.code->term();
    const struct dj_parse_expr_node* const node =
        d_parse_expr_term_node(term, _node);
    std::vector<std::string>               argv;

    for (re_std::uint32_t i = 0u; i < node->count; ++i)
    {
        argv.push_back(_walk.code->word(d_parse_expr_term_child(term, _node,
                                                                i)));
    }

    const command_fn* const run = _walk.table->find(argv[0]);

    if (run == nullptr)
    {
        return trouble("no command is named '" + argv[0] + "'", node);
    }

    try
    {
        outcome out = (*run)(argv, _input);

        out.offset = (out.status == status_trouble) ? node->offset : 0u;
        out.length = (out.status == status_trouble) ? node->length : 0u;

        return out;
    }
    catch (const std::exception& _caught)
    {
        return trouble(argv[0] + ": " + _caught.what(), node);
    }
    catch (...)
    {
        return trouble(argv[0] + ": failed", node);
    }
}

/*
input_of
  The input a node is given by its parent's frame: a sequence's own input
to each of its children; a pipeline's to its first stage, and each stage
before's value to the next; the seed to the root.
*/
static value
input_of(
    const walk& _walk
)
{
    if (_walk.frames.empty())
    {
        return _walk.seed;
    }

    const frame& parent = _walk.frames.back();

    return ( (parent.op == (re_std::uint32_t)op_pipe) &&
             (parent.any) ) ? parent.last.result : parent.input;
}

/*
give
  An outcome given to the parent's frame, or kept as the whole's at the
root. A pipeline marks itself broken at a stage in trouble.
*/
static void
give(
    walk&          _walk,
    const outcome& _outcome
)
{
    if (_walk.frames.empty())
    {
        _walk.done     = _outcome;
        _walk.finished = true;

        return;
    }

    frame&     parent  = _walk.frames.back();
    const bool trouble = (_outcome.status == status_trouble);
    const bool logic   = ( (parent.op != (re_std::uint32_t)op_seq) &&
                           (parent.op != (re_std::uint32_t)op_pipe) );

    parent.first  = (parent.any) ? parent.first : _outcome;
    parent.last   = _outcome;
    parent.any    = true;
    parent.broken = ( (parent.broken) ||
                      ( (trouble) &&
                        ( (parent.op == (re_std::uint32_t)op_pipe) ||
                          (logic) ) ) ||
                      ( (parent.op == (re_std::uint32_t)op_and) &&
                        (!truth_of(_outcome)) ) ||
                      ( (parent.op == (re_std::uint32_t)op_or) &&
                        (truth_of(_outcome)) ) );

    return;
}

/*
folded
  What a frame's node comes to, its children done: a sequence's and a
pipeline's last child's outcome, a broken pipeline's the stage in trouble's;
an `and`'s and an `or`'s the operand that decided it, its status by its
truth; a `not`'s the opposite truth; a comparison's, its two operands'.
An operand in trouble is the whole's trouble.
*/
static outcome
folded(
    const frame& _frame
)
{
    const re_std::uint32_t op = _frame.op;

    if ( (op == (re_std::uint32_t)op_seq) ||
         (op == (re_std::uint32_t)op_pipe) ||
         (_frame.last.status == status_trouble) )
    {
        return _frame.last;
    }

    if ( (op == (re_std::uint32_t)op_and) ||
         (op == (re_std::uint32_t)op_or) )
    {
        return decided(_frame.last.result, truth_of(_frame.last));
    }

    if (op == (re_std::uint32_t)op_not)
    {
        const bool yes = !truth_of(_frame.last);

        return decided(value::of_truth(yes), yes);
    }

    return compared(op, _frame.first.result, _frame.last.result);
}

/*
enter
  A node entered: skipped where its pipeline is broken; a command run, its
leaves skipped; a sequence or a pipeline given a frame.
*/
static int
enter(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    re_std::uint32_t                 _node,
    re_std::uint32_t                 _parent
)
{
    walk&                                  state = *static_cast<walk*>(_ctx);
    const struct dj_parse_expr_node* const node  =
        d_parse_expr_term_node(_term, _node);

    (void)_parent;

    if ( (!state.frames.empty()) &&
         (state.frames.back().broken) )
    {
        return D_PARSE_EXPR_WALK_SKIP;
    }

    // a literal: its value, which needs no input
    if (node->kind == D_PARSE_EXPR_NODE_LEAF)
    {
        give(state, outcome{ literal_of(*state.code, _node, node), status_yes,
                             std::string(), 0u, 0u });

        return D_PARSE_EXPR_WALK_SKIP;
    }

    if ( (node->kind == D_PARSE_EXPR_NODE_APPLY) &&
         (node->op == (re_std::uint32_t)op_cmd) )
    {
        give(state, run_command(state, _node, input_of(state)));

        return D_PARSE_EXPR_WALK_SKIP;
    }

    if ( (node->kind == D_PARSE_EXPR_NODE_APPLY) &&
         ( (node->op == (re_std::uint32_t)op_seq) ||
           ( (node->op >= (re_std::uint32_t)op_pipe) &&
             (node->op <= (re_std::uint32_t)op_match) &&
             (node->op != (re_std::uint32_t)op_group) ) ) )
    {
        const outcome none{ value(), status_yes, std::string(), 0u, 0u };

        state.frames.push_back(frame{ _node, node->op, input_of(state), none,
                                      none, false, false });

        return D_PARSE_EXPR_WALK_CONTINUE;
    }

    // what this step cannot run: the whole is trouble, and the walk ends
    state.done     = trouble("this construct comes in a later step", node);
    state.finished = true;

    return D_PARSE_EXPR_WALK_STOP;
}

/*
leave
  A node left: a sequence's or a pipeline's frame taken off, what its last
child came to given to its parent - a broken pipeline's, the stage in
trouble's.
*/
static int
leave(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    re_std::uint32_t                 _node,
    re_std::uint32_t                 _parent
)
{
    walk& state = *static_cast<walk*>(_ctx);

    (void)_parent;

    if ( (state.frames.empty()) ||
         (state.frames.back().node != _node) )
    {
        return D_PARSE_EXPR_WALK_CONTINUE;
    }

    outcome done = folded(state.frames.back());

    // trouble the construct found itself is said at its span
    if ( (done.status == status_trouble) &&
         (done.length == 0u) )
    {
        const struct dj_parse_expr_node* const node =
            d_parse_expr_term_node(_term, _node);

        done.offset = (node != nullptr) ? node->offset : 0u;
        done.length = (node != nullptr) ? node->length : 0u;
    }

    state.frames.pop_back();
    give(state, done);

    return D_PARSE_EXPR_WALK_CONTINUE;
}

outcome evaluate(
    const script&   _script,
    const commands& _commands,
    const value&    _input
)
{
    if (!_script.ok())
    {
        const diagnostic first = (_script.errors().empty())
                                 ? diagnostic{ "nothing to run", 0u, 0u }
                                 : _script.errors().front();

        return outcome{ value(), status_trouble, first.message, first.offset,
                        first.length };
    }

    walk state{ &_script, &_commands, _input, std::vector<frame>(),
                outcome{ value(), status_yes, std::string(), 0u, 0u },
                false };

    if ( (!d_parse_expr_walk(_script.term(), _script.root(), enter, leave,
                             &state)) ||
         (!state.finished) )
    {
        return (state.finished)
               ? state.done
               : outcome{ value(), status_trouble,
                          "the script could not be walked", 0u, 0u };
    }

    return state.done;
}


NS_END  // interp
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
