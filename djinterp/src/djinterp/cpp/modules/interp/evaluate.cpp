/*******************************************************************************
* djinterp [cpp][interp]                                            evaluate.cpp
*
* The command language run, declared in evaluate.hpp: the value's stand-in,
* the commands' table, and the fold, a walk of the term.
*   The walk keeps a frame for each application it is inside: the input the
* node was given, what its last child came to, and, for a pipeline, whether
* a stage has been in trouble. Entering a node gives it its input from its
* parent's frame; leaving it gives its outcome back. A command is run where
* it is entered, its words read from its leaves, which the walk then skips.
*
*
* path:      /src/djinterp/cpp/modules/interp/evaluate.cpp
* link(s):   /docs/djinterp/interp/DESIGN.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.22
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/interp/evaluate.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdio>     // std::snprintf
#include <exception>  // std::exception
#include <string>     // std::string
#include <utility>    // std::move
#include <vector>     // std::vector
// djinterp
#include "../../../../../inc/djinterp/c/modules/parse/expr/expr.h"  // core v2


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
// 3.  THE FOLD
//==============================================================================

// frame
//   struct: an application the walk is inside: its node and operator; the
// input it was given; what its last child came to; whether a child has
// come to anything; and, for a pipeline, whether a stage was in trouble.
struct frame
{
    re_std::uint32_t node;
    re_std::uint32_t op;
    value            input;
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

    frame& parent = _walk.frames.back();

    parent.last   = _outcome;
    parent.any    = true;
    parent.broken = ( (parent.broken) ||
                      ( (parent.op == (re_std::uint32_t)op_pipe) &&
                        (_outcome.status == status_trouble) ) );

    return;
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

    if ( (node->kind == D_PARSE_EXPR_NODE_APPLY) &&
         (node->op == (re_std::uint32_t)op_cmd) )
    {
        give(state, run_command(state, _node, input_of(state)));

        return D_PARSE_EXPR_WALK_SKIP;
    }

    if ( (node->kind == D_PARSE_EXPR_NODE_APPLY) &&
         ( (node->op == (re_std::uint32_t)op_seq) ||
           (node->op == (re_std::uint32_t)op_pipe) ) )
    {
        state.frames.push_back(frame{ _node, node->op, input_of(state),
                                      outcome{ value(), status_yes,
                                               std::string(), 0u, 0u },
                                      false, false });

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

    (void)_term;
    (void)_parent;

    if ( (state.frames.empty()) ||
         (state.frames.back().node != _node) )
    {
        return D_PARSE_EXPR_WALK_CONTINUE;
    }

    const outcome last = state.frames.back().last;

    state.frames.pop_back();
    give(state, last);

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
