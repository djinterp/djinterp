/******************************************************************************
* djinterp [parse]                                             parse_runtime.h
*
*   Runtime parser generator using Earley parsing algorithm. Given a grammar
* loaded at runtime (via d_parse_grammar from parse_bnf.h), this module can
* parse arbitrary input text and produce a parse tree.
*
*   The Earley algorithm handles any context-free grammar including:
*     - ambiguous grammars (produces all valid parses)
*     - left-recursive grammars (no transformation needed)
*     - epsilon productions (empty alternatives)
*
*   Complexity:
*     - O(n³) worst case (highly ambiguous grammars)
*     - O(n²) for unambiguous grammars  
*     - O(n) for many practical grammars (LR-class)
*
* path:      \inc\parse\runtime\parse_runtime.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.12
******************************************************************************/

#ifndef DJINTERP_PARSE_RUNTIME_
#define DJINTERP_PARSE_RUNTIME_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "..\djinterp.h"
#include "..\dstring.h"

/* ============================================================================
 * Grammar structures (compatible with parse_bnf.h)
 *
 * If you're using parse_bnf.h, include it BEFORE this header and define
 * D_PARSE_GRAMMAR_DEFINED to skip these definitions.
 * ========================================================================== */

#ifndef D_PARSE_GRAMMAR_DEFINED
#define D_PARSE_GRAMMAR_DEFINED 1


// DParseSymbolKind
//   enum: classification of grammar symbols.
enum DParseSymbolKind
{
    D_PARSE_SYMBOL_KIND_UNKNOWN = 0,
    D_PARSE_SYMBOL_KIND_NONTERM,
    D_PARSE_SYMBOL_KIND_TERM,
    D_PARSE_SYMBOL_KIND_SYNTHETIC
};

// d_parse_symbol
//   struct: represents a symbol (terminal or nonterminal) in the grammar.
struct d_parse_symbol
{
    char*                 name;
    enum DParseSymbolKind kind;
    int                   is_lhs;
};

// d_parse_production
//   struct: represents a production rule (LHS -> RHS).
struct d_parse_production
{
    int    lhs_index;
    int*   rhs_indices;
    size_t rhs_length;
};

// d_parse_grammar
//   struct: represents a complete grammar with symbols and productions.
struct d_parse_grammar
{
    struct d_parse_symbol*     symbols;
    size_t                     symbol_count;
    size_t                     symbol_capacity;

    struct d_parse_production* productions;
    size_t                     production_count;
    size_t                     production_capacity;

    int                        start_symbol_index;
    size_t                     synthetic_counter;
};

#endif /* D_PARSE_GRAMMAR_DEFINED */

/* ============================================================================
 * Runtime lexer token types
 * ========================================================================== */

// DParseRuntimeTokenType
//   enum: token types produced by the runtime lexer when scanning input.
enum DParseRuntimeTokenType
{
    D_PARSE_RT_TOKEN_EOF = 0,
    D_PARSE_RT_TOKEN_IDENT,
    D_PARSE_RT_TOKEN_INTEGER,
    D_PARSE_RT_TOKEN_FLOAT,
    D_PARSE_RT_TOKEN_STRING,
    D_PARSE_RT_TOKEN_SYMBOL,        // single punctuation character
    D_PARSE_RT_TOKEN_KEYWORD,       // matched a terminal literal
    D_PARSE_RT_TOKEN_WHITESPACE,
    D_PARSE_RT_TOKEN_NEWLINE,
    D_PARSE_RT_TOKEN_INDENT,
    D_PARSE_RT_TOKEN_DEDENT,
    D_PARSE_RT_TOKEN_ERROR
};

/* ============================================================================
 * Runtime token structure
 * ========================================================================== */

// d_parse_rt_token
//   struct: represents a single token from the input stream.
struct d_parse_rt_token
{
    enum DParseRuntimeTokenType type;
    const char*                 lexeme;
    size_t                      length;
    int                         line;
    int                         column;
    int                         symbol_index;   // index in grammar if matched
};

/* ============================================================================
 * Runtime lexer configuration
 * ========================================================================== */

// d_parse_rt_lexer_config
//   struct: configuration for how the runtime lexer tokenizes input.
struct d_parse_rt_lexer_config
{
    int   track_indentation;    // generate INDENT/DEDENT tokens
    int   skip_whitespace;      // skip whitespace tokens
    int   skip_comments;        // skip comment tokens
    char  line_comment_char;    // character starting line comments (0=none)
    char  string_quote_char;    // string delimiter (default: '"')
    char  alt_string_quote;     // alternate string delimiter (default: '\'')
};

// d_parse_rt_lexer
//   struct: runtime lexer state for tokenizing input.
struct d_parse_rt_lexer
{
    const char*                        source;
    size_t                             length;
    size_t                             position;
    int                                line;
    int                                column;

    // indentation tracking
    int*                               indent_stack;
    size_t                             indent_stack_size;
    size_t                             indent_stack_capacity;
    int                                pending_dedents;
    int                                at_line_start;

    // grammar reference for terminal matching
    const struct d_parse_grammar*      grammar;
    const struct d_parse_rt_lexer_config* config;

    // current token
    struct d_parse_rt_token            current;
};

/* ============================================================================
 * Earley parser data structures
 * ========================================================================== */

// d_parse_earley_item
//   struct: represents an Earley item [A -> α • β, origin].
// An item tracks progress through a production rule.
struct d_parse_earley_item
{
    int    production_index;    // which production rule
    size_t dot_position;        // position of dot in RHS (0 = beginning)
    size_t origin;              // chart position where this item started
    
    // for parse tree construction
    struct d_parse_earley_item** predecessors;
    size_t                       predecessor_count;
    size_t                       predecessor_capacity;
    
    // completed item that caused this advance (for COMPLETER)
    struct d_parse_earley_item*  completed_by;
};

// d_parse_earley_set
//   struct: a set of Earley items at a particular position in the input.
struct d_parse_earley_set
{
    struct d_parse_earley_item** items;
    size_t                       count;
    size_t                       capacity;
};

// d_parse_earley_chart
//   struct: the complete Earley chart (array of item sets).
struct d_parse_earley_chart
{
    struct d_parse_earley_set* sets;
    size_t                     count;
    size_t                     capacity;
};

/* ============================================================================
 * Parse tree structures
 * ========================================================================== */

// DParseTreeNodeType
//   enum: distinguishes terminal vs nonterminal nodes in parse tree.
enum DParseTreeNodeType
{
    D_PARSE_TREE_NODE_TERMINAL = 0,
    D_PARSE_TREE_NODE_NONTERMINAL
};

// d_parse_tree_node
//   struct: a node in the parse tree.
struct d_parse_tree_node
{
    enum DParseTreeNodeType      type;
    int                          symbol_index;
    
    // for terminals: token info
    const char*                  lexeme;
    size_t                       lexeme_length;
    int                          line;
    int                          column;
    
    // for nonterminals: children
    struct d_parse_tree_node**   children;
    size_t                       child_count;
    size_t                       child_capacity;
    
    // production used (for nonterminals)
    int                          production_index;
};

// d_parse_result
//   struct: result of parsing, including success status and parse tree(s).
struct d_parse_result
{
    int                          success;
    const char*                  error_message;
    int                          error_line;
    int                          error_column;
    
    // parse trees (may have multiple for ambiguous grammars)
    struct d_parse_tree_node**   trees;
    size_t                       tree_count;
    size_t                       tree_capacity;
};

/* ============================================================================
 * Runtime parser state
 * ========================================================================== */

// d_parse_runtime
//   struct: complete runtime parser state.
struct d_parse_runtime
{
    const struct d_parse_grammar*   grammar;
    struct d_parse_rt_lexer         lexer;
    struct d_parse_earley_chart     chart;
    
    // token buffer for lookahead
    struct d_parse_rt_token*        tokens;
    size_t                          token_count;
    size_t                          token_capacity;
    
    // configuration
    struct d_parse_rt_lexer_config  config;
};

/* ============================================================================
 * Memory management helpers
 * ========================================================================== */

static void*
d_parse_rt_realloc
(
    void*  _ptr,
    size_t _size
)
{
    void* result;

    result = realloc(_ptr, _size);

    if ( (!result) && 
         (_size > 0u) )
    {
        fprintf(stderr, "d_parse_rt_realloc: out of memory\n");
        exit(EXIT_FAILURE);
    }

    return result;
}

static void*
d_parse_rt_calloc
(
    size_t _count,
    size_t _size
)
{
    void* result;

    result = calloc(_count, _size);

    if ( (!result) && 
         (_count > 0u) && 
         (_size > 0u) )
    {
        fprintf(stderr, "d_parse_rt_calloc: out of memory\n");
        exit(EXIT_FAILURE);
    }

    return result;
}

/* ============================================================================
 * Lexer configuration initialization
 * ========================================================================== */

/*
d_parse_rt_lexer_config_init_default
  Initialize lexer config with sensible defaults for typical programming
language parsing.
*/
static void
d_parse_rt_lexer_config_init_default
(
    struct d_parse_rt_lexer_config* _config
)
{
    if (!_config)
    {
        return;
    }

    _config->track_indentation = 0;
    _config->skip_whitespace   = 1;
    _config->skip_comments     = 1;
    _config->line_comment_char = '#';
    _config->string_quote_char = '"';
    _config->alt_string_quote  = '\'';

    return;
}

/*
d_parse_rt_lexer_config_init_indentation
  Initialize lexer config for indentation-sensitive languages (like Python
or the template language in your grammar).
*/
static void
d_parse_rt_lexer_config_init_indentation
(
    struct d_parse_rt_lexer_config* _config
)
{
    d_parse_rt_lexer_config_init_default(_config);

    _config->track_indentation = 1;
    _config->skip_whitespace   = 0;  // need whitespace for indentation

    return;
}

/* ============================================================================
 * Runtime lexer implementation
 * ========================================================================== */

static int
d_parse_rt_lexer_peek
(
    struct d_parse_rt_lexer* _lexer
)
{
    if (_lexer->position >= _lexer->length)
    {
        return EOF;
    }

    return (unsigned char)_lexer->source[_lexer->position];
}

static int
d_parse_rt_lexer_peek_ahead
(
    struct d_parse_rt_lexer* _lexer,
    size_t                   _offset
)
{
    size_t pos;

    pos = _lexer->position + _offset;

    if (pos >= _lexer->length)
    {
        return EOF;
    }

    return (unsigned char)_lexer->source[pos];
}

static int
d_parse_rt_lexer_advance
(
    struct d_parse_rt_lexer* _lexer
)
{
    int ch;

    if (_lexer->position >= _lexer->length)
    {
        return EOF;
    }

    ch = (unsigned char)_lexer->source[_lexer->position];
    _lexer->position += 1u;

    if (ch == '\n')
    {
        _lexer->line         += 1;
        _lexer->column        = 1;
        _lexer->at_line_start = 1;
    }
    else
    {
        _lexer->column       += 1;
        _lexer->at_line_start = 0;
    }

    return ch;
}

/*
d_parse_rt_lexer_push_indent
  Push a new indentation level onto the stack.
*/
static void
d_parse_rt_lexer_push_indent
(
    struct d_parse_rt_lexer* _lexer,
    int                      _level
)
{
    size_t new_capacity;

    if (_lexer->indent_stack_size == _lexer->indent_stack_capacity)
    {
        new_capacity = (_lexer->indent_stack_capacity != 0u)
            ? (_lexer->indent_stack_capacity * 2u)
            : 8u;

        _lexer->indent_stack = d_parse_rt_realloc(
            _lexer->indent_stack,
            new_capacity * sizeof(int));

        _lexer->indent_stack_capacity = new_capacity;
    }

    _lexer->indent_stack[_lexer->indent_stack_size] = _level;
    _lexer->indent_stack_size += 1u;

    return;
}

/*
d_parse_rt_lexer_current_indent
  Get the current indentation level from the stack.
*/
static int
d_parse_rt_lexer_current_indent
(
    struct d_parse_rt_lexer* _lexer
)
{
    if (_lexer->indent_stack_size == 0u)
    {
        return 0;
    }

    return _lexer->indent_stack[_lexer->indent_stack_size - 1u];
}

/*
d_parse_rt_lexer_match_terminal
  Check if the current position matches a terminal symbol from the grammar.
Returns the symbol index if matched, -1 otherwise.
*/
static int
d_parse_rt_lexer_match_terminal
(
    struct d_parse_rt_lexer* _lexer,
    size_t*                  _match_length
)
{
    const struct d_parse_grammar* grammar;
    size_t                        i;
    size_t                        remaining;

    grammar   = _lexer->grammar;
    remaining = _lexer->length - _lexer->position;

    if ( (!grammar) || 
         (!grammar->symbols) )
    {
        return -1;
    }

    // check each terminal symbol
    for (i = 0u; i < grammar->symbol_count; ++i)
    {
        const struct d_parse_symbol* symbol;
        size_t                       name_len;

        symbol = &grammar->symbols[i];

        // only check terminals
        if (symbol->kind != D_PARSE_SYMBOL_KIND_TERM)
        {
            continue;
        }

        name_len = strlen(symbol->name);

        // check if we have enough characters
        if (name_len > remaining)
        {
            continue;
        }

        // compare
        if (strncmp(_lexer->source + _lexer->position,
                    symbol->name,
                    name_len) == 0)
        {
            // for identifier-like terminals, ensure word boundary
            if (isalnum((unsigned char)symbol->name[0]))
            {
                int next_ch;

                next_ch = d_parse_rt_lexer_peek_ahead(_lexer, name_len);

                if ( (next_ch != EOF) && 
                     (isalnum(next_ch) || (next_ch == '_')) )
                {
                    continue;  // not a word boundary
                }
            }

            *_match_length = name_len;

            return (int)i;
        }
    }

    return -1;
}

/*
d_parse_rt_lexer_skip_line_comment
  Skip a line comment starting at the current position.
*/
static void
d_parse_rt_lexer_skip_line_comment
(
    struct d_parse_rt_lexer* _lexer
)
{
    int ch;

    ch = d_parse_rt_lexer_peek(_lexer);

    while ( (ch != EOF) && 
            (ch != '\n') )
    {
        d_parse_rt_lexer_advance(_lexer);
        ch = d_parse_rt_lexer_peek(_lexer);
    }

    return;
}

/*
d_parse_rt_lexer_make_token
  Create a token structure.
*/
static struct d_parse_rt_token
d_parse_rt_lexer_make_token
(
    enum DParseRuntimeTokenType _type,
    const char*                 _lexeme,
    size_t                      _length,
    int                         _line,
    int                         _column,
    int                         _symbol_index
)
{
    struct d_parse_rt_token token;

    token.type         = _type;
    token.lexeme       = _lexeme;
    token.length       = _length;
    token.line         = _line;
    token.column       = _column;
    token.symbol_index = _symbol_index;

    return token;
}

/*
d_parse_rt_lexer_scan_string
  Scan a string literal.
*/
static struct d_parse_rt_token
d_parse_rt_lexer_scan_string
(
    struct d_parse_rt_lexer* _lexer,
    char                     _quote
)
{
    const char* start;
    int         start_line;
    int         start_column;
    int         ch;

    start        = _lexer->source + _lexer->position;
    start_line   = _lexer->line;
    start_column = _lexer->column;

    // consume opening quote
    d_parse_rt_lexer_advance(_lexer);

    ch = d_parse_rt_lexer_peek(_lexer);

    while ( (ch != EOF) && 
            (ch != _quote) )
    {
        // handle escape sequences
        if (ch == '\\')
        {
            d_parse_rt_lexer_advance(_lexer);
            ch = d_parse_rt_lexer_peek(_lexer);

            if (ch != EOF)
            {
                d_parse_rt_lexer_advance(_lexer);
            }
        }
        else
        {
            d_parse_rt_lexer_advance(_lexer);
        }

        ch = d_parse_rt_lexer_peek(_lexer);
    }

    // consume closing quote
    if (ch == _quote)
    {
        d_parse_rt_lexer_advance(_lexer);
    }

    return d_parse_rt_lexer_make_token(
        D_PARSE_RT_TOKEN_STRING,
        start,
        (size_t)((_lexer->source + _lexer->position) - start),
        start_line,
        start_column,
        -1);
}

/*
d_parse_rt_lexer_scan_number
  Scan an integer or float literal.
*/
static struct d_parse_rt_token
d_parse_rt_lexer_scan_number
(
    struct d_parse_rt_lexer* _lexer
)
{
    const char*                 start;
    int                         start_line;
    int                         start_column;
    int                         ch;
    int                         is_float;
    enum DParseRuntimeTokenType type;

    start        = _lexer->source + _lexer->position;
    start_line   = _lexer->line;
    start_column = _lexer->column;
    is_float     = 0;

    // handle optional leading minus
    ch = d_parse_rt_lexer_peek(_lexer);

    if (ch == '-')
    {
        d_parse_rt_lexer_advance(_lexer);
        ch = d_parse_rt_lexer_peek(_lexer);
    }

    // scan digits
    while ( (ch != EOF) && 
            isdigit(ch) )
    {
        d_parse_rt_lexer_advance(_lexer);
        ch = d_parse_rt_lexer_peek(_lexer);
    }

    // check for decimal point
    if (ch == '.')
    {
        int next;

        next = d_parse_rt_lexer_peek_ahead(_lexer, 1u);

        if ( (next != EOF) && 
             isdigit(next) )
        {
            is_float = 1;
            d_parse_rt_lexer_advance(_lexer);  // consume '.'
            ch = d_parse_rt_lexer_peek(_lexer);

            while ( (ch != EOF) && 
                    isdigit(ch) )
            {
                d_parse_rt_lexer_advance(_lexer);
                ch = d_parse_rt_lexer_peek(_lexer);
            }
        }
    }

    type = is_float ? D_PARSE_RT_TOKEN_FLOAT : D_PARSE_RT_TOKEN_INTEGER;

    return d_parse_rt_lexer_make_token(
        type,
        start,
        (size_t)((_lexer->source + _lexer->position) - start),
        start_line,
        start_column,
        -1);
}

/*
d_parse_rt_lexer_scan_identifier
  Scan an identifier token.
*/
static struct d_parse_rt_token
d_parse_rt_lexer_scan_identifier
(
    struct d_parse_rt_lexer* _lexer
)
{
    const char* start;
    int         start_line;
    int         start_column;
    int         ch;

    start        = _lexer->source + _lexer->position;
    start_line   = _lexer->line;
    start_column = _lexer->column;

    ch = d_parse_rt_lexer_peek(_lexer);

    while ( (ch != EOF) && 
            (isalnum(ch) || (ch == '_')) )
    {
        d_parse_rt_lexer_advance(_lexer);
        ch = d_parse_rt_lexer_peek(_lexer);
    }

    return d_parse_rt_lexer_make_token(
        D_PARSE_RT_TOKEN_IDENT,
        start,
        (size_t)((_lexer->source + _lexer->position) - start),
        start_line,
        start_column,
        -1);
}

/*
d_parse_rt_lexer_handle_indentation
  Process indentation at the start of a line, generating INDENT/DEDENT tokens.
Returns 1 if an INDENT token should be emitted, -N if N DEDENT tokens pending.
*/
static int
d_parse_rt_lexer_handle_indentation
(
    struct d_parse_rt_lexer* _lexer
)
{
    int current_indent;
    int new_indent;
    int ch;

    if ( (!_lexer->config) || 
         (!_lexer->config->track_indentation) )
    {
        return 0;
    }

    if (!_lexer->at_line_start)
    {
        return 0;
    }

    // count leading whitespace
    new_indent = 0;
    ch         = d_parse_rt_lexer_peek(_lexer);

    while ( (ch == ' ') || 
            (ch == '\t') )
    {
        if (ch == '\t')
        {
            new_indent = ((new_indent / 8) + 1) * 8;  // tab stops at 8
        }
        else
        {
            new_indent += 1;
        }

        d_parse_rt_lexer_advance(_lexer);
        ch = d_parse_rt_lexer_peek(_lexer);
    }

    // skip blank lines
    if ( (ch == '\n') || 
         (ch == '\r') || 
         (ch == EOF) )
    {
        return 0;
    }

    current_indent = d_parse_rt_lexer_current_indent(_lexer);

    if (new_indent > current_indent)
    {
        d_parse_rt_lexer_push_indent(_lexer, new_indent);

        return 1;  // emit INDENT
    }
    else if (new_indent < current_indent)
    {
        int dedent_count;

        dedent_count = 0;

        // pop indent levels until we match
        while ( (_lexer->indent_stack_size > 0u) && 
                (d_parse_rt_lexer_current_indent(_lexer) > new_indent) )
        {
            _lexer->indent_stack_size -= 1u;
            dedent_count              += 1;
        }

        _lexer->pending_dedents = dedent_count - 1;

        return -1;  // emit first DEDENT, rest are pending
    }

    _lexer->at_line_start = 0;

    return 0;
}

/*
d_parse_rt_lexer_next
  Get the next token from the input.
*/
static struct d_parse_rt_token
d_parse_rt_lexer_next
(
    struct d_parse_rt_lexer* _lexer
)
{
    int                               ch;
    int                               start_line;
    int                               start_column;
    const char*                       start;
    const struct d_parse_rt_lexer_config* config;

    config = _lexer->config;

    // handle pending DEDENT tokens
    if (_lexer->pending_dedents > 0)
    {
        _lexer->pending_dedents -= 1;

        return d_parse_rt_lexer_make_token(
            D_PARSE_RT_TOKEN_DEDENT,
            _lexer->source + _lexer->position,
            0u,
            _lexer->line,
            _lexer->column,
            -1);
    }

    // handle indentation at line start
    if ( (config) && 
         (config->track_indentation) && 
         (_lexer->at_line_start) )
    {
        int indent_result;

        indent_result = d_parse_rt_lexer_handle_indentation(_lexer);

        if (indent_result > 0)
        {
            return d_parse_rt_lexer_make_token(
                D_PARSE_RT_TOKEN_INDENT,
                _lexer->source + _lexer->position,
                0u,
                _lexer->line,
                _lexer->column,
                -1);
        }
        else if (indent_result < 0)
        {
            return d_parse_rt_lexer_make_token(
                D_PARSE_RT_TOKEN_DEDENT,
                _lexer->source + _lexer->position,
                0u,
                _lexer->line,
                _lexer->column,
                -1);
        }
    }

skip_whitespace_and_comments:

    ch = d_parse_rt_lexer_peek(_lexer);

    // handle whitespace
    while ( (ch == ' ')  || 
            (ch == '\t') || 
            (ch == '\r') )
    {
        if ( (config) && 
             (!config->skip_whitespace) )
        {
            break;
        }

        d_parse_rt_lexer_advance(_lexer);
        ch = d_parse_rt_lexer_peek(_lexer);
    }

    // handle line comments
    if ( (config) && 
         (config->line_comment_char != 0) && 
         (ch == config->line_comment_char) )
    {
        d_parse_rt_lexer_skip_line_comment(_lexer);

        if ( (config) && 
             (config->skip_comments) )
        {
            goto skip_whitespace_and_comments;
        }
    }

    // check for EOF
    if (ch == EOF)
    {
        // emit any remaining DEDENTs
        if ( (config) && 
             (config->track_indentation) && 
             (_lexer->indent_stack_size > 0u) )
        {
            _lexer->indent_stack_size -= 1u;
            _lexer->pending_dedents    = (int)_lexer->indent_stack_size;

            return d_parse_rt_lexer_make_token(
                D_PARSE_RT_TOKEN_DEDENT,
                _lexer->source + _lexer->position,
                0u,
                _lexer->line,
                _lexer->column,
                -1);
        }

        return d_parse_rt_lexer_make_token(
            D_PARSE_RT_TOKEN_EOF,
            _lexer->source + _lexer->position,
            0u,
            _lexer->line,
            _lexer->column,
            -1);
    }

    start_line   = _lexer->line;
    start_column = _lexer->column;
    start        = _lexer->source + _lexer->position;

    // handle newlines
    if (ch == '\n')
    {
        d_parse_rt_lexer_advance(_lexer);

        if ( (config) && 
             (config->skip_whitespace) && 
             (!config->track_indentation) )
        {
            goto skip_whitespace_and_comments;
        }

        return d_parse_rt_lexer_make_token(
            D_PARSE_RT_TOKEN_NEWLINE,
            start,
            1u,
            start_line,
            start_column,
            -1);
    }

    // try to match a terminal from the grammar (longest match)
    {
        size_t match_len;
        int    symbol_idx;

        symbol_idx = d_parse_rt_lexer_match_terminal(_lexer, &match_len);

        if (symbol_idx >= 0)
        {
            size_t i;

            for (i = 0u; i < match_len; ++i)
            {
                d_parse_rt_lexer_advance(_lexer);
            }

            return d_parse_rt_lexer_make_token(
                D_PARSE_RT_TOKEN_KEYWORD,
                start,
                match_len,
                start_line,
                start_column,
                symbol_idx);
        }
    }

    // string literals
    if ( (config) && 
         ( (ch == config->string_quote_char) || 
           (ch == config->alt_string_quote) ) )
    {
        return d_parse_rt_lexer_scan_string(_lexer, (char)ch);
    }

    // numbers
    if ( isdigit(ch) || 
         ( (ch == '-') && 
           isdigit(d_parse_rt_lexer_peek_ahead(_lexer, 1u)) ) )
    {
        return d_parse_rt_lexer_scan_number(_lexer);
    }

    // identifiers
    if ( isalpha(ch) || 
         (ch == '_') )
    {
        return d_parse_rt_lexer_scan_identifier(_lexer);
    }

    // single character symbol
    d_parse_rt_lexer_advance(_lexer);

    return d_parse_rt_lexer_make_token(
        D_PARSE_RT_TOKEN_SYMBOL,
        start,
        1u,
        start_line,
        start_column,
        -1);
}

/*
d_parse_rt_lexer_init
  Initialize the runtime lexer.
*/
static void
d_parse_rt_lexer_init
(
    struct d_parse_rt_lexer*              _lexer,
    const char*                           _source,
    const struct d_parse_grammar*         _grammar,
    const struct d_parse_rt_lexer_config* _config
)
{
    if ( (!_lexer) || 
         (!_source) )
    {
        return;
    }

    memset(_lexer, 0, sizeof(*_lexer));

    _lexer->source        = _source;
    _lexer->length        = strlen(_source);
    _lexer->position      = 0u;
    _lexer->line          = 1;
    _lexer->column        = 1;
    _lexer->grammar       = _grammar;
    _lexer->config        = _config;
    _lexer->at_line_start = 1;

    // initialize indent stack with level 0
    if ( (_config) && 
         (_config->track_indentation) )
    {
        d_parse_rt_lexer_push_indent(_lexer, 0);
    }

    return;
}

/*
d_parse_rt_lexer_destroy
  Clean up lexer resources.
*/
static void
d_parse_rt_lexer_destroy
(
    struct d_parse_rt_lexer* _lexer
)
{
    if (!_lexer)
    {
        return;
    }

    if (_lexer->indent_stack)
    {
        free(_lexer->indent_stack);
    }

    memset(_lexer, 0, sizeof(*_lexer));

    return;
}

/* ============================================================================
 * Earley item management
 * ========================================================================== */

/*
d_parse_earley_item_create
  Create a new Earley item.
*/
static struct d_parse_earley_item*
d_parse_earley_item_create
(
    int    _production_index,
    size_t _dot_position,
    size_t _origin
)
{
    struct d_parse_earley_item* item;

    item = d_parse_rt_calloc(1u, sizeof(struct d_parse_earley_item));

    item->production_index    = _production_index;
    item->dot_position        = _dot_position;
    item->origin              = _origin;
    item->predecessors        = NULL;
    item->predecessor_count   = 0u;
    item->predecessor_capacity = 0u;
    item->completed_by        = NULL;

    return item;
}

/*
d_parse_earley_item_add_predecessor
  Add a predecessor to an Earley item (for parse tree construction).
*/
static void
d_parse_earley_item_add_predecessor
(
    struct d_parse_earley_item* _item,
    struct d_parse_earley_item* _predecessor
)
{
    size_t new_capacity;

    if ( (!_item) || 
         (!_predecessor) )
    {
        return;
    }

    if (_item->predecessor_count == _item->predecessor_capacity)
    {
        new_capacity = (_item->predecessor_capacity != 0u)
            ? (_item->predecessor_capacity * 2u)
            : 4u;

        _item->predecessors = d_parse_rt_realloc(
            _item->predecessors,
            new_capacity * sizeof(struct d_parse_earley_item*));

        _item->predecessor_capacity = new_capacity;
    }

    _item->predecessors[_item->predecessor_count] = _predecessor;
    _item->predecessor_count += 1u;

    return;
}

/*
d_parse_earley_item_destroy
  Free an Earley item.
*/
static void
d_parse_earley_item_destroy
(
    struct d_parse_earley_item* _item
)
{
    if (!_item)
    {
        return;
    }

    if (_item->predecessors)
    {
        free(_item->predecessors);
    }

    free(_item);

    return;
}

/* ============================================================================
 * Earley set management
 * ========================================================================== */

/*
d_parse_earley_set_init
  Initialize an Earley set.
*/
static void
d_parse_earley_set_init
(
    struct d_parse_earley_set* _set
)
{
    if (!_set)
    {
        return;
    }

    _set->items    = NULL;
    _set->count    = 0u;
    _set->capacity = 0u;

    return;
}

/*
d_parse_earley_set_contains
  Check if the set already contains an equivalent item.
Returns the existing item if found, NULL otherwise.
*/
static struct d_parse_earley_item*
d_parse_earley_set_contains
(
    struct d_parse_earley_set* _set,
    int                        _production_index,
    size_t                     _dot_position,
    size_t                     _origin
)
{
    size_t i;

    if (!_set)
    {
        return NULL;
    }

    for (i = 0u; i < _set->count; ++i)
    {
        struct d_parse_earley_item* item;

        item = _set->items[i];

        if ( (item->production_index == _production_index) && 
             (item->dot_position == _dot_position)         && 
             (item->origin == _origin) )
        {
            return item;
        }
    }

    return NULL;
}

/*
d_parse_earley_set_add
  Add an item to the set (if not already present).
Returns the item (existing or new).
*/
static struct d_parse_earley_item*
d_parse_earley_set_add
(
    struct d_parse_earley_set* _set,
    int                        _production_index,
    size_t                     _dot_position,
    size_t                     _origin
)
{
    struct d_parse_earley_item* existing;
    struct d_parse_earley_item* item;
    size_t                      new_capacity;

    if (!_set)
    {
        return NULL;
    }

    // check if already exists
    existing = d_parse_earley_set_contains(_set,
                                           _production_index,
                                           _dot_position,
                                           _origin);
    if (existing)
    {
        return existing;
    }

    // create new item
    item = d_parse_earley_item_create(_production_index,
                                      _dot_position,
                                      _origin);

    // add to set
    if (_set->count == _set->capacity)
    {
        new_capacity = (_set->capacity != 0u) ? (_set->capacity * 2u) : 16u;

        _set->items = d_parse_rt_realloc(
            _set->items,
            new_capacity * sizeof(struct d_parse_earley_item*));

        _set->capacity = new_capacity;
    }

    _set->items[_set->count] = item;
    _set->count += 1u;

    return item;
}

/*
d_parse_earley_set_destroy
  Free all items in a set.
*/
static void
d_parse_earley_set_destroy
(
    struct d_parse_earley_set* _set
)
{
    size_t i;

    if (!_set)
    {
        return;
    }

    for (i = 0u; i < _set->count; ++i)
    {
        d_parse_earley_item_destroy(_set->items[i]);
    }

    if (_set->items)
    {
        free(_set->items);
    }

    _set->items    = NULL;
    _set->count    = 0u;
    _set->capacity = 0u;

    return;
}

/* ============================================================================
 * Earley chart management
 * ========================================================================== */

/*
d_parse_earley_chart_init
  Initialize an Earley chart.
*/
static void
d_parse_earley_chart_init
(
    struct d_parse_earley_chart* _chart
)
{
    if (!_chart)
    {
        return;
    }

    _chart->sets     = NULL;
    _chart->count    = 0u;
    _chart->capacity = 0u;

    return;
}

/*
d_parse_earley_chart_get_set
  Get or create the set at position k.
*/
static struct d_parse_earley_set*
d_parse_earley_chart_get_set
(
    struct d_parse_earley_chart* _chart,
    size_t                       _position
)
{
    size_t new_capacity;
    size_t i;

    if (!_chart)
    {
        return NULL;
    }

    // expand chart if needed
    while (_position >= _chart->capacity)
    {
        new_capacity = (_chart->capacity != 0u)
            ? (_chart->capacity * 2u)
            : 16u;

        _chart->sets = d_parse_rt_realloc(
            _chart->sets,
            new_capacity * sizeof(struct d_parse_earley_set));

        // initialize new sets
        for (i = _chart->capacity; i < new_capacity; ++i)
        {
            d_parse_earley_set_init(&_chart->sets[i]);
        }

        _chart->capacity = new_capacity;
    }

    // update count
    if (_position >= _chart->count)
    {
        _chart->count = _position + 1u;
    }

    return &_chart->sets[_position];
}

/*
d_parse_earley_chart_destroy
  Free all resources in a chart.
*/
static void
d_parse_earley_chart_destroy
(
    struct d_parse_earley_chart* _chart
)
{
    size_t i;

    if (!_chart)
    {
        return;
    }

    for (i = 0u; i < _chart->count; ++i)
    {
        d_parse_earley_set_destroy(&_chart->sets[i]);
    }

    if (_chart->sets)
    {
        free(_chart->sets);
    }

    _chart->sets     = NULL;
    _chart->count    = 0u;
    _chart->capacity = 0u;

    return;
}

/* ============================================================================
 * Parse tree management
 * ========================================================================== */

/*
d_parse_tree_node_create
  Create a new parse tree node.
*/
static struct d_parse_tree_node*
d_parse_tree_node_create
(
    enum DParseTreeNodeType _type,
    int                     _symbol_index
)
{
    struct d_parse_tree_node* node;

    node = d_parse_rt_calloc(1u, sizeof(struct d_parse_tree_node));

    node->type             = _type;
    node->symbol_index     = _symbol_index;
    node->production_index = -1;

    return node;
}

/*
d_parse_tree_node_add_child
  Add a child to a parse tree node.
*/
static void
d_parse_tree_node_add_child
(
    struct d_parse_tree_node* _parent,
    struct d_parse_tree_node* _child
)
{
    size_t new_capacity;

    if ( (!_parent) || 
         (!_child) )
    {
        return;
    }

    if (_parent->child_count == _parent->child_capacity)
    {
        new_capacity = (_parent->child_capacity != 0u)
            ? (_parent->child_capacity * 2u)
            : 4u;

        _parent->children = d_parse_rt_realloc(
            _parent->children,
            new_capacity * sizeof(struct d_parse_tree_node*));

        _parent->child_capacity = new_capacity;
    }

    _parent->children[_parent->child_count] = _child;
    _parent->child_count += 1u;

    return;
}

/*
d_parse_tree_node_destroy
  Recursively free a parse tree.
*/
static void
d_parse_tree_node_destroy
(
    struct d_parse_tree_node* _node
)
{
    size_t i;

    if (!_node)
    {
        return;
    }

    for (i = 0u; i < _node->child_count; ++i)
    {
        d_parse_tree_node_destroy(_node->children[i]);
    }

    if (_node->children)
    {
        free(_node->children);
    }

    free(_node);

    return;
}

/* ============================================================================
 * Parse result management
 * ========================================================================== */

/*
d_parse_result_init
  Initialize a parse result.
*/
static void
d_parse_result_init
(
    struct d_parse_result* _result
)
{
    if (!_result)
    {
        return;
    }

    _result->success       = 0;
    _result->error_message = NULL;
    _result->error_line    = 0;
    _result->error_column  = 0;
    _result->trees         = NULL;
    _result->tree_count    = 0u;
    _result->tree_capacity = 0u;

    return;
}

/*
d_parse_result_add_tree
  Add a parse tree to the result.
*/
static void
d_parse_result_add_tree
(
    struct d_parse_result*    _result,
    struct d_parse_tree_node* _tree
)
{
    size_t new_capacity;

    if ( (!_result) || 
         (!_tree) )
    {
        return;
    }

    if (_result->tree_count == _result->tree_capacity)
    {
        new_capacity = (_result->tree_capacity != 0u)
            ? (_result->tree_capacity * 2u)
            : 4u;

        _result->trees = d_parse_rt_realloc(
            _result->trees,
            new_capacity * sizeof(struct d_parse_tree_node*));

        _result->tree_capacity = new_capacity;
    }

    _result->trees[_result->tree_count] = _tree;
    _result->tree_count += 1u;

    return;
}

/*
d_parse_result_destroy
  Free all resources in a parse result.
*/
static void
d_parse_result_destroy
(
    struct d_parse_result* _result
)
{
    size_t i;

    if (!_result)
    {
        return;
    }

    for (i = 0u; i < _result->tree_count; ++i)
    {
        d_parse_tree_node_destroy(_result->trees[i]);
    }

    if (_result->trees)
    {
        free(_result->trees);
    }

    _result->trees         = NULL;
    _result->tree_count    = 0u;
    _result->tree_capacity = 0u;

    return;
}

/* ============================================================================
 * Earley parser core algorithm
 * ========================================================================== */

/*
d_parse_earley_production_symbol_after_dot
  Get the symbol index after the dot in a production, or -1 if dot is at end.
*/
static int
d_parse_earley_production_symbol_after_dot
(
    const struct d_parse_grammar*    _grammar,
    const struct d_parse_earley_item* _item
)
{
    const struct d_parse_production* production;

    if ( (!_grammar) || 
         (!_item) )
    {
        return -1;
    }

    production = &_grammar->productions[_item->production_index];

    if (_item->dot_position >= production->rhs_length)
    {
        return -1;  // dot at end (completed item)
    }

    return production->rhs_indices[_item->dot_position];
}

/*
d_parse_earley_is_complete
  Check if an Earley item is complete (dot at end of production).
*/
static int
d_parse_earley_is_complete
(
    const struct d_parse_grammar*    _grammar,
    const struct d_parse_earley_item* _item
)
{
    const struct d_parse_production* production;

    if ( (!_grammar) || 
         (!_item) )
    {
        return 0;
    }

    production = &_grammar->productions[_item->production_index];

    return (_item->dot_position >= production->rhs_length);
}

/*
d_parse_earley_predictor
  PREDICTOR: For items [A -> α • B β, j], add [B -> • γ, k] for all B -> γ.
*/
static void
d_parse_earley_predictor
(
    const struct d_parse_grammar* _grammar,
    struct d_parse_earley_set*    _set,
    size_t                        _position,
    int                           _symbol_index
)
{
    size_t i;

    if ( (!_grammar) || 
         (!_set) )
    {
        return;
    }

    // find all productions for this symbol
    for (i = 0u; i < _grammar->production_count; ++i)
    {
        const struct d_parse_production* production;

        production = &_grammar->productions[i];

        if (production->lhs_index == _symbol_index)
        {
            d_parse_earley_set_add(_set,
                                   (int)i,
                                   0u,
                                   _position);
        }
    }

    return;
}

/*
d_parse_earley_token_matches_terminal
  Check if a token matches a grammar terminal symbol.
Handles both literal matches and token-type-to-terminal mappings.
*/
static int
d_parse_earley_token_matches_terminal
(
    const struct d_parse_rt_token*  _token,
    const struct d_parse_symbol*    _symbol
)
{
    const char* sym_name;
    size_t      sym_len;

    if ( (!_token) || 
         (!_symbol) )
    {
        return 0;
    }

    sym_name = _symbol->name;
    sym_len  = strlen(sym_name);

    // 1. Check for token-type-to-terminal mappings
    // Map INTEGER/FLOAT tokens to NUMBER terminal
    if ( ( (strcmp(sym_name, "NUMBER") == 0) ||
           (strcmp(sym_name, "number") == 0) ) &&
         ( (_token->type == D_PARSE_RT_TOKEN_INTEGER) ||
           (_token->type == D_PARSE_RT_TOKEN_FLOAT) ) )
    {
        return 1;
    }

    // Map INTEGER token to INTEGER terminal
    if ( ( (strcmp(sym_name, "INTEGER") == 0) ||
           (strcmp(sym_name, "integer") == 0) ||
           (strcmp(sym_name, "INT") == 0) ||
           (strcmp(sym_name, "int") == 0) ) &&
         (_token->type == D_PARSE_RT_TOKEN_INTEGER) )
    {
        return 1;
    }

    // Map FLOAT token to FLOAT terminal
    if ( ( (strcmp(sym_name, "FLOAT") == 0) ||
           (strcmp(sym_name, "float") == 0) ) &&
         (_token->type == D_PARSE_RT_TOKEN_FLOAT) )
    {
        return 1;
    }

    // Map STRING token to STRING terminal
    if ( ( (strcmp(sym_name, "STRING") == 0) ||
           (strcmp(sym_name, "string") == 0) ) &&
         (_token->type == D_PARSE_RT_TOKEN_STRING) )
    {
        return 1;
    }

    // Map IDENT token to IDENT/ID/identifier terminal
    if ( ( (strcmp(sym_name, "IDENT") == 0) ||
           (strcmp(sym_name, "ident") == 0) ||
           (strcmp(sym_name, "ID") == 0) ||
           (strcmp(sym_name, "id") == 0) ||
           (strcmp(sym_name, "identifier") == 0) ) &&
         (_token->type == D_PARSE_RT_TOKEN_IDENT) )
    {
        return 1;
    }

    // 2. Exact lexeme match for literal terminals
    if ( (_token->length == sym_len) && 
         (strncmp(_token->lexeme, sym_name, sym_len) == 0) )
    {
        return 1;
    }

    // 3. Check for keyword match (lexer already identified it)
    if ( (_token->type == D_PARSE_RT_TOKEN_KEYWORD) &&
         (_token->length == sym_len) &&
         (strncmp(_token->lexeme, sym_name, sym_len) == 0) )
    {
        return 1;
    }

    return 0;
}

/*
d_parse_earley_scanner
  SCANNER: For items [A -> α • a β, j] where a matches token, add
[A -> α a • β, j] to S[k+1].
*/
static void
d_parse_earley_scanner
(
    const struct d_parse_grammar*   _grammar,
    struct d_parse_earley_chart*    _chart,
    struct d_parse_earley_item*     _item,
    size_t                          _position,
    const struct d_parse_rt_token*  _token
)
{
    int                          symbol_index;
    const struct d_parse_symbol* symbol;
    struct d_parse_earley_set*   next_set;
    struct d_parse_earley_item*  new_item;
    int                          matches;

    if ( (!_grammar) || 
         (!_chart)   || 
         (!_item)    || 
         (!_token) )
    {
        return;
    }

    symbol_index = d_parse_earley_production_symbol_after_dot(_grammar, _item);

    if (symbol_index < 0)
    {
        return;
    }

    symbol = &_grammar->symbols[symbol_index];

    // only terminals can be scanned
    if (symbol->kind != D_PARSE_SYMBOL_KIND_TERM)
    {
        return;
    }

    // check if token matches terminal
    matches = d_parse_earley_token_matches_terminal(_token, symbol);

    if (!matches)
    {
        return;
    }

    // add advanced item to next set
    next_set = d_parse_earley_chart_get_set(_chart, _position + 1u);
    new_item = d_parse_earley_set_add(next_set,
                                      _item->production_index,
                                      _item->dot_position + 1u,
                                      _item->origin);

    // record predecessor for tree construction
    d_parse_earley_item_add_predecessor(new_item, _item);

    return;
}

/*
d_parse_earley_completer
  COMPLETER: For complete items [A -> γ •, j], find items [B -> α • A β, i]
in S[j] and add [B -> α A • β, i] to current set.
*/
static void
d_parse_earley_completer
(
    const struct d_parse_grammar* _grammar,
    struct d_parse_earley_chart*  _chart,
    struct d_parse_earley_item*   _completed_item,
    size_t                        _position
)
{
    const struct d_parse_production* completed_prod;
    int                              completed_lhs;
    struct d_parse_earley_set*       origin_set;
    struct d_parse_earley_set*       current_set;
    size_t                           i;

    if ( (!_grammar)       || 
         (!_chart)         || 
         (!_completed_item) )
    {
        return;
    }

    completed_prod = &_grammar->productions[_completed_item->production_index];
    completed_lhs  = completed_prod->lhs_index;
    origin_set     = d_parse_earley_chart_get_set(_chart,
                                                  _completed_item->origin);
    current_set    = d_parse_earley_chart_get_set(_chart, _position);

    // look through items in the origin set
    for (i = 0u; i < origin_set->count; ++i)
    {
        struct d_parse_earley_item* waiting_item;
        int                         symbol_after_dot;
        struct d_parse_earley_item* new_item;

        waiting_item = origin_set->items[i];

        symbol_after_dot = d_parse_earley_production_symbol_after_dot(
            _grammar,
            waiting_item);

        // check if waiting for the completed nonterminal
        if (symbol_after_dot != completed_lhs)
        {
            continue;
        }

        // add advanced item
        new_item = d_parse_earley_set_add(current_set,
                                          waiting_item->production_index,
                                          waiting_item->dot_position + 1u,
                                          waiting_item->origin);

        // record both predecessors for tree construction
        d_parse_earley_item_add_predecessor(new_item, waiting_item);
        new_item->completed_by = _completed_item;
    }

    return;
}

/*
d_parse_earley_process_set
  Process all items in a set (predictor and completer).
*/
static void
d_parse_earley_process_set
(
    const struct d_parse_grammar* _grammar,
    struct d_parse_earley_chart*  _chart,
    size_t                        _position
)
{
    struct d_parse_earley_set* set;
    size_t                     i;

    if ( (!_grammar) || 
         (!_chart) )
    {
        return;
    }

    set = d_parse_earley_chart_get_set(_chart, _position);

    // process items (set may grow during iteration)
    for (i = 0u; i < set->count; ++i)
    {
        struct d_parse_earley_item* item;
        int                         symbol_after_dot;

        item = set->items[i];

        if (d_parse_earley_is_complete(_grammar, item))
        {
            // COMPLETER
            d_parse_earley_completer(_grammar, _chart, item, _position);
        }
        else
        {
            // check symbol after dot
            symbol_after_dot = d_parse_earley_production_symbol_after_dot(
                _grammar,
                item);

            if (symbol_after_dot >= 0)
            {
                const struct d_parse_symbol* symbol;

                symbol = &_grammar->symbols[symbol_after_dot];

                if (symbol->kind == D_PARSE_SYMBOL_KIND_NONTERM ||
                    symbol->kind == D_PARSE_SYMBOL_KIND_SYNTHETIC)
                {
                    // PREDICTOR
                    d_parse_earley_predictor(_grammar,
                                             set,
                                             _position,
                                             symbol_after_dot);
                }
            }
        }
    }

    return;
}

/* ============================================================================
 * Parse tree construction from Earley chart
 * ========================================================================== */

/*
d_parse_build_tree_from_item
  Recursively build a parse tree from an Earley item.
*/
static struct d_parse_tree_node*
d_parse_build_tree_from_item
(
    const struct d_parse_grammar*     _grammar,
    const struct d_parse_earley_item* _item,
    const struct d_parse_rt_token*    _tokens,
    size_t                            _token_count,
    size_t                            _end_position
);

static struct d_parse_tree_node*
d_parse_build_tree_recursive
(
    const struct d_parse_grammar*     _grammar,
    const struct d_parse_earley_item* _item,
    const struct d_parse_rt_token*    _tokens,
    size_t                            _token_count,
    size_t*                           _token_position
)
{
    const struct d_parse_production* production;
    struct d_parse_tree_node*        node;
    size_t                           rhs_idx;

    if ( (!_grammar) || 
         (!_item) )
    {
        return NULL;
    }

    production = &_grammar->productions[_item->production_index];

    node = d_parse_tree_node_create(D_PARSE_TREE_NODE_NONTERMINAL,
                                    production->lhs_index);

    node->production_index = _item->production_index;

    // build children from RHS symbols
    for (rhs_idx = 0u; rhs_idx < production->rhs_length; ++rhs_idx)
    {
        int                          symbol_idx;
        const struct d_parse_symbol* symbol;
        struct d_parse_tree_node*    child;

        symbol_idx = production->rhs_indices[rhs_idx];
        symbol     = &_grammar->symbols[symbol_idx];

        if ( (symbol->kind == D_PARSE_SYMBOL_KIND_NONTERM) ||
             (symbol->kind == D_PARSE_SYMBOL_KIND_SYNTHETIC) )
        {
            // nonterminal - need to find matching completed item
            // simplified: create placeholder
            child = d_parse_tree_node_create(D_PARSE_TREE_NODE_NONTERMINAL,
                                             symbol_idx);
        }
        else
        {
            // terminal - use token
            if (*_token_position < _token_count)
            {
                const struct d_parse_rt_token* token;

                token = &_tokens[*_token_position];

                child = d_parse_tree_node_create(D_PARSE_TREE_NODE_TERMINAL,
                                                 symbol_idx);

                child->lexeme        = token->lexeme;
                child->lexeme_length = token->length;
                child->line          = token->line;
                child->column        = token->column;

                *_token_position += 1u;
            }
            else
            {
                child = d_parse_tree_node_create(D_PARSE_TREE_NODE_TERMINAL,
                                                 symbol_idx);
            }
        }

        d_parse_tree_node_add_child(node, child);
    }

    return node;
}

static struct d_parse_tree_node*
d_parse_build_tree_from_item
(
    const struct d_parse_grammar*     _grammar,
    const struct d_parse_earley_item* _item,
    const struct d_parse_rt_token*    _tokens,
    size_t                            _token_count,
    size_t                            _end_position
)
{
    size_t token_pos;

    (void)_end_position;

    token_pos = _item->origin;

    return d_parse_build_tree_recursive(_grammar,
                                        _item,
                                        _tokens,
                                        _token_count,
                                        &token_pos);
}

/* ============================================================================
 * Main parsing interface
 * ========================================================================== */

/*
d_parse_runtime_init
  Initialize the runtime parser.

Parameter(s):
  _runtime: the runtime parser state to initialize.
  _grammar: the grammar to parse against (must remain valid during parsing).
  _config:  optional lexer configuration (NULL for defaults).
*/
static void
d_parse_runtime_init
(
    struct d_parse_runtime*               _runtime,
    const struct d_parse_grammar*         _grammar,
    const struct d_parse_rt_lexer_config* _config
)
{
    if ( (!_runtime) || 
         (!_grammar) )
    {
        return;
    }

    memset(_runtime, 0, sizeof(*_runtime));

    _runtime->grammar = _grammar;

    if (_config)
    {
        _runtime->config = *_config;
    }
    else
    {
        d_parse_rt_lexer_config_init_default(&_runtime->config);
    }

    d_parse_earley_chart_init(&_runtime->chart);

    return;
}

/*
d_parse_runtime_destroy
  Clean up runtime parser resources.
*/
static void
d_parse_runtime_destroy
(
    struct d_parse_runtime* _runtime
)
{
    if (!_runtime)
    {
        return;
    }

    d_parse_rt_lexer_destroy(&_runtime->lexer);
    d_parse_earley_chart_destroy(&_runtime->chart);

    if (_runtime->tokens)
    {
        free(_runtime->tokens);
    }

    memset(_runtime, 0, sizeof(*_runtime));

    return;
}

/*
d_parse_runtime_tokenize
  Tokenize the input and store tokens.
*/
static void
d_parse_runtime_tokenize
(
    struct d_parse_runtime* _runtime,
    const char*             _input
)
{
    struct d_parse_rt_token token;
    size_t                  new_capacity;

    if ( (!_runtime) || 
         (!_input) )
    {
        return;
    }

    d_parse_rt_lexer_init(&_runtime->lexer,
                          _input,
                          _runtime->grammar,
                          &_runtime->config);

    // clear existing tokens
    _runtime->token_count = 0u;

    for (;;)
    {
        token = d_parse_rt_lexer_next(&_runtime->lexer);

        // skip whitespace/indent tokens for basic mode
        if ( (!_runtime->config.track_indentation) && 
             ( (token.type == D_PARSE_RT_TOKEN_WHITESPACE) ||
               (token.type == D_PARSE_RT_TOKEN_NEWLINE) ) )
        {
            continue;
        }

        // add token to buffer
        if (_runtime->token_count == _runtime->token_capacity)
        {
            new_capacity = (_runtime->token_capacity != 0u)
                ? (_runtime->token_capacity * 2u)
                : 64u;

            _runtime->tokens = d_parse_rt_realloc(
                _runtime->tokens,
                new_capacity * sizeof(struct d_parse_rt_token));

            _runtime->token_capacity = new_capacity;
        }

        _runtime->tokens[_runtime->token_count] = token;
        _runtime->token_count += 1u;

        if (token.type == D_PARSE_RT_TOKEN_EOF)
        {
            break;
        }
    }

    return;
}

/*
d_parse_runtime_parse
  Parse input text against the loaded grammar.

Parameter(s):
  _runtime: initialized runtime parser.
  _input:   the input text to parse.
  _result:  output parse result (caller must call d_parse_result_destroy).

Return:
  1 if parsing succeeded, 0 if failed.
*/
static int
d_parse_runtime_parse
(
    struct d_parse_runtime* _runtime,
    const char*             _input,
    struct d_parse_result*  _result
)
{
    const struct d_parse_grammar* grammar;
    struct d_parse_earley_set*    initial_set;
    size_t                        i;
    size_t                        k;

    if ( (!_runtime) || 
         (!_input)   || 
         (!_result) )
    {
        return 0;
    }

    d_parse_result_init(_result);

    grammar = _runtime->grammar;

    if ( (!grammar) || 
         (grammar->start_symbol_index < 0) )
    {
        _result->error_message = "Invalid grammar or no start symbol";

        return 0;
    }

    // tokenize input
    d_parse_runtime_tokenize(_runtime, _input);

    // reset chart
    d_parse_earley_chart_destroy(&_runtime->chart);
    d_parse_earley_chart_init(&_runtime->chart);

    // initialize S[0] with start symbol productions
    initial_set = d_parse_earley_chart_get_set(&_runtime->chart, 0u);

    for (i = 0u; i < grammar->production_count; ++i)
    {
        if (grammar->productions[i].lhs_index == grammar->start_symbol_index)
        {
            d_parse_earley_set_add(initial_set, (int)i, 0u, 0u);
        }
    }

    // main Earley parsing loop
    for (k = 0u; k < _runtime->token_count; ++k)
    {
        struct d_parse_earley_set* current_set;
        struct d_parse_rt_token*   token;

        // process current set (predictor, completer)
        d_parse_earley_process_set(grammar, &_runtime->chart, k);

        current_set = d_parse_earley_chart_get_set(&_runtime->chart, k);
        token       = &_runtime->tokens[k];

        // skip EOF for scanning
        if (token->type == D_PARSE_RT_TOKEN_EOF)
        {
            break;
        }

        // scanner: process terminals
        for (i = 0u; i < current_set->count; ++i)
        {
            d_parse_earley_scanner(grammar,
                                   &_runtime->chart,
                                   current_set->items[i],
                                   k,
                                   token);
        }
    }

    // final processing of last set
    d_parse_earley_process_set(grammar,
                               &_runtime->chart,
                               _runtime->token_count - 1u);

    // check for successful parse: look for completed start symbol items
    {
        struct d_parse_earley_set* final_set;
        size_t                     final_pos;
        int                        found_complete;

        final_pos      = _runtime->token_count - 1u;
        final_set      = d_parse_earley_chart_get_set(&_runtime->chart,
                                                      final_pos);
        found_complete = 0;

        for (i = 0u; i < final_set->count; ++i)
        {
            struct d_parse_earley_item* item;
            const struct d_parse_production* prod;

            item = final_set->items[i];
            prod = &grammar->productions[item->production_index];

            // check if this is a completed start symbol production from origin 0
            if ( (prod->lhs_index == grammar->start_symbol_index) && 
                 (item->origin == 0u)                             && 
                 d_parse_earley_is_complete(grammar, item) )
            {
                struct d_parse_tree_node* tree;

                found_complete = 1;

                // build parse tree
                tree = d_parse_build_tree_from_item(grammar,
                                                    item,
                                                    _runtime->tokens,
                                                    _runtime->token_count,
                                                    final_pos);

                if (tree)
                {
                    d_parse_result_add_tree(_result, tree);
                }
            }
        }

        if (!found_complete)
        {
            // find the furthest position reached for error reporting
            size_t furthest;

            furthest = 0u;

            for (k = 0u; k < _runtime->chart.count; ++k)
            {
                if (_runtime->chart.sets[k].count > 0u)
                {
                    furthest = k;
                }
            }

            if (furthest < _runtime->token_count)
            {
                _result->error_line   = _runtime->tokens[furthest].line;
                _result->error_column = _runtime->tokens[furthest].column;
            }

            _result->error_message = "Parse failed: unexpected token";

            return 0;
        }
    }

    _result->success = 1;

    return 1;
}

/* ============================================================================
 * Debug/utility functions
 * ========================================================================== */

/*
d_parse_tree_print
  Print a parse tree for debugging.
*/
static void
d_parse_tree_print_internal
(
    const struct d_parse_grammar*   _grammar,
    const struct d_parse_tree_node* _node,
    int                             _depth
)
{
    int    i;
    size_t c;

    if ( (!_grammar) || 
         (!_node) )
    {
        return;
    }

    // indent
    for (i = 0; i < _depth; ++i)
    {
        printf("  ");
    }

    if (_node->type == D_PARSE_TREE_NODE_TERMINAL)
    {
        printf("TERMINAL: %s = \"",
               _grammar->symbols[_node->symbol_index].name);

        // print lexeme safely
        for (c = 0u; c < _node->lexeme_length; ++c)
        {
            char ch;

            ch = _node->lexeme[c];

            if ( (ch >= 32) && 
                 (ch < 127) )
            {
                printf("%c", ch);
            }
            else
            {
                printf("\\x%02x", (unsigned char)ch);
            }
        }

        printf("\" [%d:%d]\n", _node->line, _node->column);
    }
    else
    {
        printf("NONTERMINAL: %s (production %d)\n",
               _grammar->symbols[_node->symbol_index].name,
               _node->production_index);

        for (c = 0u; c < _node->child_count; ++c)
        {
            d_parse_tree_print_internal(_grammar,
                                        _node->children[c],
                                        _depth + 1);
        }
    }

    return;
}

static void
d_parse_tree_print
(
    const struct d_parse_grammar*   _grammar,
    const struct d_parse_tree_node* _node
)
{
    d_parse_tree_print_internal(_grammar, _node, 0);

    return;
}

/*
d_parse_earley_chart_print
  Print the Earley chart for debugging.
*/
static void
d_parse_earley_chart_print
(
    const struct d_parse_grammar*     _grammar,
    const struct d_parse_earley_chart* _chart
)
{
    size_t k;
    size_t i;

    if ( (!_grammar) || 
         (!_chart) )
    {
        return;
    }

    printf("=== Earley Chart ===\n");

    for (k = 0u; k < _chart->count; ++k)
    {
        const struct d_parse_earley_set* set;

        set = &_chart->sets[k];

        printf("\n--- S[%zu] (%zu items) ---\n", k, set->count);

        for (i = 0u; i < set->count; ++i)
        {
            const struct d_parse_earley_item* item;
            const struct d_parse_production*  prod;
            size_t                            j;

            item = set->items[i];
            prod = &_grammar->productions[item->production_index];

            printf("  [%s ->", _grammar->symbols[prod->lhs_index].name);

            for (j = 0u; j < prod->rhs_length; ++j)
            {
                if (j == item->dot_position)
                {
                    printf(" •");
                }

                printf(" %s", _grammar->symbols[prod->rhs_indices[j]].name);
            }

            if (item->dot_position == prod->rhs_length)
            {
                printf(" •");
            }

            printf(", %zu]\n", item->origin);
        }
    }

    return;
}

#endif  /* DJINTERP_PARSE_RUNTIME_ */
