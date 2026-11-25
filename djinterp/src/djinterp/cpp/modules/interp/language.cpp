/*******************************************************************************
* djinterp [cpp][interp]                                            language.cpp
*
* The command language read, declared in language.hpp: the two readers,
* the separators made tidy, and the parse on expression core v2.
*   The signature is built once, at the first parse, and sealed: core v2's
* table for djinterp.peg's section III, as far as this step goes. A
* command is a custom role on the word's class, which takes the words,
* numbers and strings after its verb; a number and a string are atoms where
* a command could stand; core v2 does the rest.
*
*
* path:      /src/djinterp/cpp/modules/interp/language.cpp
* link(s):   /docs/djinterp/interp/DESIGN.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.25
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/interp/language.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstring>  // std::memset, std::strlen
#include <string>   // std::string
#include <utility>  // std::move
#include <vector>   // std::vector
// djinterp
#include "../../../../../inc/djinterp/c/modules/parse/diagnostic.h"  // the sink
#include "../../../../../inc/djinterp/c/modules/parse/expr/expr.h"  // core v2


NS_DJINTERP
NS_INTERP


//==============================================================================
// 1.  THE SCRIPT
//==============================================================================

// script::held
//   struct: what a script holds: its tokens, its text, its errors, and the
// term with its root.
struct script::held
{
    std::vector<token>         tokens;
    std::string                text;
    std::vector<diagnostic>    errors;
    struct dj_parse_expr_term  term;
    re_std::uint32_t           root;

    held()
        : root(D_PARSE_EXPR_NO_NODE)
    {
        d_parse_expr_term_init_heap(&term);
    }

    ~held()
    {
        d_parse_expr_term_release(&term);
    }

    held(const held&)            = delete;
    held& operator=(const held&) = delete;
};

script::script()
    : m_held(std::make_shared<held>())
{
}

bool script::ok() const
{
    return ( (m_held->errors.empty()) &&
             (m_held->root != D_PARSE_EXPR_NO_NODE) );
}

const std::vector<diagnostic>& script::errors() const
{
    return m_held->errors;
}

const std::vector<token>& script::tokens() const
{
    return m_held->tokens;
}

const std::string& script::text() const
{
    return m_held->text;
}

const struct dj_parse_expr_term* script::term() const
{
    return (ok()) ? &m_held->term : nullptr;
}

re_std::uint32_t script::root() const
{
    return (ok()) ? m_held->root : D_PARSE_EXPR_NO_NODE;
}

const std::string& script::word(
    re_std::uint32_t _node
) const
{
    static const std::string none;
    const struct dj_parse_expr_node* const node =
        d_parse_expr_term_node(&m_held->term, _node);

    return ( (node != nullptr) &&
             (node->kind == D_PARSE_EXPR_NODE_LEAF) &&
             (node->first < m_held->tokens.size()) )
           ? m_held->tokens[node->first].text
           : none;
}

/*
sexpr_of
  A node and what is under it: a leaf its word, an application its
operator's name and its children in brackets. The term's depth is guarded,
so the recursion is too.
*/
static void
sexpr_of(
    const script&    _script,
    re_std::uint32_t _node,
    bool             _in_command,
    std::string&     _out
)
{
    static const char* const NAMES[] =
        { "?", "seq", "pipe", "cmd", "group", "or", "and", "not", "==", "!=",
          "<", "<=", ">", ">=", "=~" };
    const struct dj_parse_expr_node* const node =
        d_parse_expr_term_node(_script.term(), _node);

    if (node == nullptr)
    {
        return;
    }

    // a STRING literal in quotes; a command's words as written
    if (node->kind != D_PARSE_EXPR_NODE_APPLY)
    {
        const bool quoted = ( (!_in_command) &&
                              (node->op == (re_std::uint32_t)token_string) );

        _out += (node->kind != D_PARSE_EXPR_NODE_LEAF) ? std::string("_")
              : (quoted) ? ("\"" + _script.word(_node) + "\"")
                         : _script.word(_node);

        return;
    }

    _out += (node->op <= (re_std::uint32_t)op_match) ? NAMES[node->op] : "?";
    _out += '(';

    for (re_std::uint32_t i = 0u; i < node->count; ++i)
    {
        _out += (i > 0u) ? ", " : "";
        sexpr_of(_script, d_parse_expr_term_child(_script.term(), _node, i),
                 (node->op == (re_std::uint32_t)op_cmd), _out);
    }

    _out += ')';

    return;
}

std::string script::sexpr() const
{
    std::string out;

    if (ok())
    {
        sexpr_of(*this, m_held->root, false, out);
    }

    return out;
}


//==============================================================================
// 2.  READING A LINE
//==============================================================================

bool is_number(
    const std::string& _word
)
{
    std::size_t at     = (!_word.empty() && (_word[0] == '-')) ? 1u : 0u;
    std::size_t digits = 0u;

    while ( (at < _word.size()) && (_word[at] >= '0') && (_word[at] <= '9') )
    {
        ++at;
        ++digits;
    }

    if ( (digits > 0u) &&
         (at + 1u < _word.size()) &&
         (_word[at] == '.') )
    {
        digits = 0u;
        ++at;

        while ( (at < _word.size()) && (_word[at] >= '0') &&
                (_word[at] <= '9') )
        {
            ++at;
            ++digits;
        }
    }

    return ( (digits > 0u) &&
             (at == _word.size()) );
}

/*
operator_of
  The class a bare word standing alone has: an operator's, a keyword's, a
comparison's, a number's; a word's otherwise.
*/
static token_class
operator_of(
    const std::string& _word
)
{
    static const char* const COMPARE[] =
        { "==", "!=", "<", "<=", ">", ">=", "=~" };

    if (_word == ";") { return token_semi; }
    if (_word == "|") { return token_bar; }
    if (_word == "(") { return token_lparen; }
    if (_word == ")") { return token_rparen; }
    if (_word == "{") { return token_lbrace; }
    if (_word == "}") { return token_rbrace; }
    if (_word == "and") { return token_and; }
    if (_word == "or") { return token_or; }
    if (_word == "not") { return token_not; }

    for (std::size_t i = 0u; i < sizeof(COMPARE) / sizeof(COMPARE[0]); ++i)
    {
        if (_word == COMPARE[i])
        {
            return (token_class)((int)token_eq + (int)i);
        }
    }

    return (is_number(_word)) ? token_number : token_word;
}

/*
is_meta
  Whether a byte ends a bare word: a blank, a newline, or one of the
metacharacters that are tokens of their own, as the grammar's section I has
them.
*/
static bool
is_meta(
    char _c
)
{
    return ( (_c == ' ') || (_c == '\t') || (_c == '\r') || (_c == '\n') ||
             (_c == ';') || (_c == '|') || (_c == '(') || (_c == ')') ||
             (_c == '{') || (_c == '}') );
}

/*
lex_quoted
  A quoted part of a word, from its opening quote: single quotes literal;
double quotes with `\"` and `\\` read. Returns the index after the closing
quote, or the text's length where there is none, an error said.
*/
static std::size_t
lex_quoted(
    const std::string&       _text,
    std::size_t              _at,
    std::string&             _word,
    std::vector<diagnostic>& _errors
)
{
    const char  quote = _text[_at];
    std::size_t at    = _at + 1u;

    while ( (at < _text.size()) &&
            (_text[at] != quote) )
    {
        if ( (quote == '"') &&
             (_text[at] == '\\') &&
             (at + 1u < _text.size()) &&
             ( (_text[at + 1u] == '"') || (_text[at + 1u] == '\\') ) )
        {
            ++at;
        }

        _word += _text[at++];
    }

    if (at >= _text.size())
    {
        _errors.push_back(diagnostic{ std::string("unclosed ") +
                                      ((quote == '"') ? "double" : "single") +
                                      " quote",
                                      (re_std::uint32_t)_at, 1u });

        return _text.size();
    }

    return at + 1u;
}

/*
lex_word
  A word from where it begins: bare and quoted parts, until a blank or a
metacharacter outside quotes. Returns the index after it; `*_quoted` says
whether any part was quoted, so that `and` quoted is no keyword, and
`*_string` whether the word is wholly one quoted part, a STRING.
*/
static std::size_t
lex_word(
    const std::string&       _text,
    std::size_t              _at,
    std::string&             _word,
    bool*                    _quoted,
    bool*                    _string,
    std::vector<diagnostic>& _errors
)
{
    std::size_t at    = _at;
    std::size_t parts = 0u;
    bool        bare  = false;

    *_quoted = false;

    while ( (at < _text.size()) &&
            (!is_meta(_text[at])) )
    {
        if ( (_text[at] == '\'') ||
             (_text[at] == '"') )
        {
            *_quoted = true;
            at       = lex_quoted(_text, at, _word, _errors);
            ++parts;

            continue;
        }

        bare   = true;
        _word += _text[at++];
    }

    *_string = ( (*_quoted) && (!bare) && (parts == 1u) );

    return at;
}

/*
lex_line
  Every token of a text: each blank passed over, a comment from a `#` where
a token could begin to the line's end, a newline its own token, each
metacharacter its own, and words.
*/
static std::vector<token>
lex_line(
    const std::string&       _text,
    std::vector<diagnostic>& _errors
)
{
    std::vector<token> out;
    std::size_t        at    = 0u;
    bool               space = false;
    bool               line  = false;

    while (at < _text.size())
    {
        const char c = _text[at];

        if ( (c == ' ') || (c == '\t') || (c == '\r') )
        {
            space = true;
            ++at;

            continue;
        }

        if (c == '#')
        {
            while ( (at < _text.size()) && (_text[at] != '\n') )
            {
                ++at;
            }

            continue;
        }

        token one{ token_word, std::string(), (re_std::uint32_t)at, 0u, space,
                   line };
        bool  quoted = false;
        bool  string = false;

        if (c == '\n')
        {
            one.cls  = token_newline;
            one.text = "\n";
            ++at;
        }
        else if (is_meta(c))
        {
            one.text = std::string(1u, c);
            one.cls  = operator_of(one.text);
            ++at;
        }
        else
        {
            at       = lex_word(_text, at, one.text, &quoted, &string,
                                _errors);
            one.cls  = (string) ? token_string
                     : (quoted) ? token_word
                                : operator_of(one.text);
        }

        one.length = (re_std::uint32_t)(at - one.offset);
        out.push_back(one);
        space = false;
        line  = (one.cls == token_newline);
    }

    return out;
}


//==============================================================================
// 3.  THE SEPARATORS
//==============================================================================

/*
is_separator
  Whether a token ends a pipeline: `;` or a newline.
*/
static bool
is_separator(
    token_class _cls
)
{
    return ( (_cls == token_semi) ||
             (_cls == token_newline) );
}

/*
tidy
  The separators as the grammar reads them: a newline at the start, at the
end, after `(`, `|`, a keyword, a comparison or another separator, or before
`)`, is passed over - blank lines, and a pipeline or a predicate continued
on the next line after its operator; a `;` at the end, or before `)`, ends
its sequence, and is passed over too. Two `;` in a row stay, for the parse
to refuse.
*/
static std::vector<token>
tidy(
    std::vector<token>&& _tokens
)
{
    std::vector<token> out;

    for (std::size_t i = 0u; i < _tokens.size(); ++i)
    {
        const token_class cls  = _tokens[i].cls;
        const token_class last = (out.empty()) ? token_end : out.back().cls;
        const token_class next = (i + 1u < _tokens.size())
                                 ? _tokens[i + 1u].cls
                                 : token_end;
        const bool        lead = ( (last == token_end) ||
                                   (last == token_lparen) ||
                                   (last == token_bar) ||
                                   (last == token_and) ||
                                   (last == token_or) ||
                                   (last == token_not) ||
                                   ( (last >= token_eq) &&
                                     (last <= token_match) ) ||
                                   (is_separator(last)) );
        const bool        tail = ( (next == token_end) ||
                                   (next == token_rparen) ||
                                   (next == token_newline) );

        if ( ( (cls == token_newline) &&
               ( (lead) || (tail) ) ) ||
             ( (cls == token_semi) &&
               ( (next == token_end) || (next == token_rparen) ) ) )
        {
            continue;
        }

        out.push_back(std::move(_tokens[i]));
    }

    return out;
}


//==============================================================================
// 4.  THE PARSE
//==============================================================================

/*
take_command
  The command's role: the verb's leaf, then a leaf for each argument after
it while the next token is a word, a number or a string; one `cmd` node
over them, spanning them. A leaf's value is its token's place, so the
script can give its word.
*/
static int
take_command(
    void*                             _ctx,
    struct dj_parse_expr_parser*      _parser,
    const struct dj_parse_expr_token* _token,
    re_std::uint32_t                  _left,
    re_std::uint32_t*                 _out
)
{
    struct dj_parse_expr_term* const term = d_parse_expr_parser_term(_parser);
    std::vector<re_std::uint32_t>    parts;
    re_std::uint32_t                 end  = _token->offset + _token->length;
    struct dj_parse_expr_token       next;

    (void)_ctx;
    (void)_left;

    parts.push_back(d_parse_expr_term_leaf(term, _token->cls, _token->value,
                                           _token->offset, _token->length));

    while ( (d_parse_expr_peek(_parser, D_PARSE_EXPR_OPERATOR, 0u,
                               &next) > 0) &&
            ( (next.cls == (re_std::uint32_t)token_word) ||
              (next.cls == (re_std::uint32_t)token_number) ||
              (next.cls == (re_std::uint32_t)token_string) ) )
    {
        (void)d_parse_expr_take(_parser, D_PARSE_EXPR_OPERATOR, 0u, &next);
        parts.push_back(d_parse_expr_term_leaf(term, next.cls, next.value,
                                               next.offset, next.length));
        end = next.offset + next.length;
    }

    for (re_std::uint32_t part : parts)
    {
        if (part == D_PARSE_EXPR_NO_NODE)
        {
            return -1;
        }
    }

    *_out = d_parse_expr_term_apply(term, (re_std::uint32_t)op_cmd,
                                    parts.data(),
                                    (re_std::uint32_t)parts.size(),
                                    _token->offset, end - _token->offset);

    return (*_out != D_PARSE_EXPR_NO_NODE) ? 1 : -1;
}

/*
define_operators
  The operators of this step: `;` and the newline one flattened `seq` at
10; `|` a flattened `pipe` at 20; the command, which only its role builds,
at 90; and the brackets, which leave no node.
*/
static bool
define_operators(
    struct dj_parse_expr_signature* _s
)
{
    struct dj_parse_expr_mixfix group;

    std::memset(&group, 0, sizeof(group));
    group.parts            = 2u;
    group.part[0]          = (re_std::uint32_t)token_lparen;
    group.part[1]          = (re_std::uint32_t)token_rparen;
    group.part_spelling[0] = "(";
    group.part_spelling[1] = ")";

    return ( (d_parse_expr_define_op(_s, op_seq, D_PARSE_EXPR_FORM_INFIX, 10,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_FLATTEN, ";")) &&
             (d_parse_expr_define_op(_s, op_pipe, D_PARSE_EXPR_FORM_INFIX, 20,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_FLATTEN, "|")) &&
             (d_parse_expr_define_op(_s, op_cmd, D_PARSE_EXPR_FORM_INFIX, 90,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_FLATTEN, "")) &&
             (d_parse_expr_define_mixfix(_s, op_group, 0, 0u,
                                         D_PARSE_EXPR_OP_TRANSPARENT,
                                         &group)) &&
             (d_parse_expr_define_token(_s, token_semi, D_PARSE_EXPR_OPERATOR,
                                        op_seq, 0u)) &&
             (d_parse_expr_define_token(_s, token_newline,
                                        D_PARSE_EXPR_OPERATOR, op_seq, 0u)) &&
             (d_parse_expr_define_token(_s, token_bar, D_PARSE_EXPR_OPERATOR,
                                        op_pipe, 0u)) &&
             (d_parse_expr_define_token(_s, token_lparen,
                                        D_PARSE_EXPR_OPERAND, op_group,
                                        0u)) );
}

/*
define_logic
  The operators of the second step: `or` a flattened `or` at 30, `and` a
flattened `and` at 40, each spelled as a word; `not` a prefix at 50; the
comparisons at 60, none beside another; and a number and a string atoms,
literals where a command could stand.
*/
static bool
define_logic(
    struct dj_parse_expr_signature* _s
)
{
    static const char* const COMPARE[] =
        { "==", "!=", "<", "<=", ">", ">=", "=~" };
    bool made =
        ( (d_parse_expr_define_op(_s, op_or, D_PARSE_EXPR_FORM_INFIX, 30,
                                  D_PARSE_EXPR_ASSOC_LEFT,
                                  D_PARSE_EXPR_OP_FLATTEN |
                                  D_PARSE_EXPR_OP_WORD, "or")) &&
          (d_parse_expr_define_op(_s, op_and, D_PARSE_EXPR_FORM_INFIX, 40,
                                  D_PARSE_EXPR_ASSOC_LEFT,
                                  D_PARSE_EXPR_OP_FLATTEN |
                                  D_PARSE_EXPR_OP_WORD, "and")) &&
          (d_parse_expr_define_op(_s, op_not, D_PARSE_EXPR_FORM_PREFIX, 50,
                                  D_PARSE_EXPR_ASSOC_RIGHT,
                                  D_PARSE_EXPR_OP_WORD, "not")) &&
          (d_parse_expr_define_token(_s, token_or, D_PARSE_EXPR_OPERATOR,
                                     op_or, 0u)) &&
          (d_parse_expr_define_token(_s, token_and, D_PARSE_EXPR_OPERATOR,
                                     op_and, 0u)) &&
          (d_parse_expr_define_token(_s, token_not, D_PARSE_EXPR_OPERAND,
                                     op_not, 0u)) &&
          (d_parse_expr_define_atom(_s, token_number, 0u)) &&
          (d_parse_expr_define_atom(_s, token_string, 0u)) );

    // each comparison its own operator and token, in op_id's order
    for (re_std::uint32_t i = 0u;
         (made) && (i < sizeof(COMPARE) / sizeof(COMPARE[0]));
         ++i)
    {
        made = ( (d_parse_expr_define_op(_s, (re_std::uint32_t)op_eq + i,
                                         D_PARSE_EXPR_FORM_INFIX, 60,
                                         D_PARSE_EXPR_ASSOC_NONE, 0u,
                                         COMPARE[i])) &&
                 (d_parse_expr_define_token(_s,
                                            (re_std::uint32_t)token_eq + i,
                                            D_PARSE_EXPR_OPERATOR,
                                            (re_std::uint32_t)op_eq + i,
                                            0u)) );
    }

    return made;
}

/*
signature
  The language's signature, built and sealed at the first call; NULL where
it could not be.
*/
static const struct dj_parse_expr_signature*
signature()
{
    struct made
    {
        struct dj_parse_expr_signature table;
        bool                           ok;

        made()
            : ok(false)
        {
            struct dj_parse_expr_role command;

            std::memset(&command, 0, sizeof(command));
            command.cls      = (re_std::uint32_t)token_word;
            command.position = D_PARSE_EXPR_OPERAND;
            command.kind     = D_PARSE_EXPR_ROLE_CUSTOM;
            command.custom   = take_command;

            d_parse_expr_signature_init_heap(&table);
            ok = ( (define_operators(&table)) &&
                   (define_logic(&table)) &&
                   (d_parse_expr_define_role(&table, &command)) &&
                   (d_parse_expr_signature_seal(&table, nullptr)) );
        }

        ~made()
        {
            d_parse_expr_signature_release(&table);
        }
    };

    static made once;

    return (once.ok) ? &once.table : nullptr;
}

/*
collect
  The parse's errors, from its sink, each with its words and span; the
result's own where the sink kept none.
*/
static void
collect(
    const struct dj_parse_diag_sink*   _sink,
    const struct dj_parse_expr_result& _result,
    std::vector<diagnostic>&           _errors
)
{
    const std::size_t before = _errors.size();

    for (re_std::uint32_t i = 0u; i < _sink->count; ++i)
    {
        const struct dj_parse_diagnostic* const each =
            d_parse_diag_at(_sink, i);

        if ( (each != nullptr) &&
             ((int)each->severity >= (int)D_PARSE_SEVERITY_ERROR) )
        {
            _errors.push_back(diagnostic{ d_parse_diag_message(_sink, each),
                                          each->span.offset,
                                          each->span.length });
        }
    }

    if ( (_errors.size() == before) &&
         (_result.status != D_PARSE_EXPR_OK) )
    {
        _errors.push_back(diagnostic{ "the text could not be read",
                                      _result.span.offset,
                                      _result.span.length });
    }

    return;
}

script parse_tokens(
    std::vector<token>&&      _tokens,
    std::string&&             _text,
    std::vector<diagnostic>&& _errors
)
{
    script                               out;
    script::held&                        held = *out.m_held;
    const struct dj_parse_expr_signature* s   = signature();

    held.tokens = tidy(std::move(_tokens));
    held.text   = std::move(_text);
    held.errors = std::move(_errors);

    if (s == nullptr)
    {
        held.errors.push_back(diagnostic{ "the language could not be made",
                                          0u, 0u });

        return out;
    }

    if (!held.errors.empty())
    {
        return out;
    }

    std::vector<struct dj_parse_expr_token> raw;

    for (std::size_t i = 0u; i < held.tokens.size(); ++i)
    {
        const token& each = held.tokens[i];

        raw.push_back(dj_parse_expr_token{
            (re_std::uint32_t)each.cls,
            ( (each.space_before) ? D_PARSE_EXPR_TOKEN_SPACE_BEFORE : 0u ) |
            ( (each.line_before) ? D_PARSE_EXPR_TOKEN_LINE_BEFORE : 0u ),
            each.offset, each.length, (re_std::uint32_t)i });
    }

    struct dj_parse_expr_source       source;
    struct dj_parse_expr_array_source state;
    struct dj_parse_expr_options      options;
    struct dj_parse_expr_result       result;
    struct dj_parse_diagnostic        items[32];
    char                              text[4096];
    struct dj_parse_diag_sink         sink;

    d_parse_expr_source_array(&source, &state, raw.data(),
                              (re_std::uint32_t)raw.size(),
                              (re_std::uint32_t)held.text.size());
    std::memset(&options, 0, sizeof(options));
    options.text        = held.text.c_str();
    options.text_length = held.text.size();
    d_parse_diag_sink_init(&sink, items, 32u, text, (re_std::uint32_t)
                           sizeof(text));
    (void)d_parse_expr_parse(s, &source, &options, &held.term, &sink, &result);
    collect(&sink, result, held.errors);
    held.root = (held.errors.empty()) ? result.root : D_PARSE_EXPR_NO_NODE;
    d_parse_diag_sink_release(&sink);

    return out;
}

script parse_line(
    const std::string& _text
)
{
    std::vector<diagnostic> errors;
    std::vector<token>      tokens = lex_line(_text, errors);

    return parse_tokens(std::move(tokens), std::string(_text),
                        std::move(errors));
}

script parse_argv(
    const std::vector<std::string>& _words
)
{
    std::vector<token> tokens;
    std::string        text;

    for (const std::string& each : _words)
    {
        text += (text.empty()) ? "" : " ";

        tokens.push_back(token{ operator_of(each), each,
                                (re_std::uint32_t)text.size(),
                                (re_std::uint32_t)each.size(),
                                !tokens.empty(), false });
        text += each;
    }

    return parse_tokens(std::move(tokens), std::move(text),
                        std::vector<diagnostic>());
}


NS_END  // interp
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
