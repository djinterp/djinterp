/******************************************************************************
* djinterp [parse]                                          parse_bnf_common.h
*
*   Common infrastructure for BNF-style grammar parsers. Contains:
*     - Token and symbol enums
*     - Configuration structures
*     - Lexer implementation
*     - Grammar data structures and helpers
*
* path:      \inc\parse\grammar\parse_bnf_common.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.09
******************************************************************************/

#ifndef DJINTERP_PARSE_GRAMMAR_BNF_COMMON_
#define DJINTERP_PARSE_GRAMMAR_BNF_COMMON_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Token / symbol enums
 * ========================================================================== */

enum DParseTokenType
{
    D_PARSE_TOKEN_TYPE_STOP = 0,
    D_PARSE_TOKEN_TYPE_IDENT,
    D_PARSE_TOKEN_TYPE_STRING,          /* 'literal' */
    D_PARSE_TOKEN_TYPE_COLON_COLON_EQ,  /* ::= */
    D_PARSE_TOKEN_TYPE_PIPE,            /* | */
    D_PARSE_TOKEN_TYPE_SEMI,            /* ; */

    /* EBNF extensions */
    D_PARSE_TOKEN_TYPE_LBRACE,          /* { */
    D_PARSE_TOKEN_TYPE_RBRACE,          /* } */
    D_PARSE_TOKEN_TYPE_LBRACKET,        /* [ */
    D_PARSE_TOKEN_TYPE_RBRACKET,        /* ] */
    D_PARSE_TOKEN_TYPE_LPAREN,          /* ( */
    D_PARSE_TOKEN_TYPE_RPAREN,          /* ) */
    D_PARSE_TOKEN_TYPE_STAR,            /* * (postfix repetition) */
    D_PARSE_TOKEN_TYPE_PLUS,            /* + (postfix one-or-more) */
    D_PARSE_TOKEN_TYPE_QUESTION         /* ? (postfix optional) */
};

enum DParseSymbolKind
{
    D_PARSE_SYMBOL_KIND_UNKNOWN = 0,
    D_PARSE_SYMBOL_KIND_NONTERM,
    D_PARSE_SYMBOL_KIND_TERM,
    D_PARSE_SYMBOL_KIND_SYNTHETIC       /* generated for EBNF desugaring */
};

/* ============================================================================
 * Whitespace and BNF mode enums
 * ========================================================================== */

enum DParseWhitespaceKind
{
    D_PARSE_WHITESPACE_KIND_SPACE = 0,
    D_PARSE_WHITESPACE_KIND_TAB,
    D_PARSE_WHITESPACE_KIND_NEWLINE,
    D_PARSE_WHITESPACE_KIND_CARRIAGE_RETURN,
    D_PARSE_WHITESPACE_KIND_FORM_FEED,
    D_PARSE_WHITESPACE_KIND_VERTICAL_TAB,
    D_PARSE_WHITESPACE_KIND_COUNT
};

#define D_PARSE_WHITESPACE_MASK(kind) \
    ( (unsigned int)1u << (unsigned int)(kind) )

enum DParseBnfStartMode
{
    D_PARSE_BNF_START_MODE_INFER = 0,   /* infer from first rule LHS */
    D_PARSE_BNF_START_MODE_TOKEN        /* explicit directive (e.g. %START) */
};

enum DParseBnfEndMode
{
    D_PARSE_BNF_END_MODE_INFER = 0,     /* end at EOF */
    D_PARSE_BNF_END_MODE_TOKEN          /* explicit directive (e.g. %END) */
};

enum DParseBnfRuleBeginMode
{
    D_PARSE_BNF_RULE_BEGIN_MODE_INFER = 0,  /* IDENT ::= */
    D_PARSE_BNF_RULE_BEGIN_MODE_TOKEN       /* e.g. %RULE IDENT ::= */
};

enum DParseBnfRuleEndMode
{
    D_PARSE_BNF_RULE_END_MODE_INFER = 0,    /* next rule or EOF */
    D_PARSE_BNF_RULE_END_MODE_TOKEN         /* dedicated terminator (e.g. ;) */
};

/* ============================================================================
 * BNF parser configuration
 * ========================================================================== */

struct d_parse_bnf_config
{
    /* Whitespace + comments */
    unsigned int                  whitespace_mask;
    char                          line_comment_char;

    /* Start symbol handling */
    enum DParseBnfStartMode       start_mode;
    const char*                   start_token_text;

    /* End-of-grammar handling */
    enum DParseBnfEndMode         end_mode;
    const char*                   end_token_text;

    /* Rule-begin handling */
    enum DParseBnfRuleBeginMode   rule_begin_mode;
    const char*                   rule_begin_token_text;

    /* Rule-end handling */
    enum DParseBnfRuleEndMode     rule_end_mode;
    const char*                   rule_end_token_text;

    /* EBNF mode: enable { } [ ] ( ) and postfix operators */
    int                           ebnf_extensions;
};

/*
d_parse_bnf_config_init_default
  Initialize config with "inferred" behavior:

    - start:      inferred from first rule LHS
    - end:        inferred at EOF
    - rule begin: inferred via IDENT ::=
    - rule end:   inferred via next rule / EOF
    - whitespace: standard ASCII whitespace
    - comments:   '#' starts a line comment
    - ebnf:       disabled
*/
static void
d_parse_bnf_config_init_default
(
    struct d_parse_bnf_config* _config
)
{
    if (!_config)
    {
        return;
    }

    _config->whitespace_mask =
          D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_SPACE)
        | D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_TAB)
        | D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_NEWLINE)
        | D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_CARRIAGE_RETURN)
        | D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_FORM_FEED)
        | D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_VERTICAL_TAB);

    _config->line_comment_char = '#';

    _config->start_mode       = D_PARSE_BNF_START_MODE_INFER;
    _config->start_token_text = "%START";

    _config->end_mode         = D_PARSE_BNF_END_MODE_INFER;
    _config->end_token_text   = "%END";

    _config->rule_begin_mode       = D_PARSE_BNF_RULE_BEGIN_MODE_INFER;
    _config->rule_begin_token_text = "%RULE";

    _config->rule_end_mode       = D_PARSE_BNF_RULE_END_MODE_INFER;
    _config->rule_end_token_text = ";";

    _config->ebnf_extensions = 0;
}

/*
d_parse_bnf_config_init_ebnf
  Initialize config with EBNF extensions enabled.
*/
static void
d_parse_bnf_config_init_ebnf
(
    struct d_parse_bnf_config* _config
)
{
    d_parse_bnf_config_init_default(_config);

    _config->ebnf_extensions = 1;
}

/* ============================================================================
 * Core data structures
 * ========================================================================== */

struct d_parse_token
{
    enum DParseTokenType type;
    const char*          lexeme;
    size_t               length;
    int                  line;
    int                  column;
};

struct d_parse_lexer
{
    const char*                      source;
    size_t                           length;
    size_t                           position;
    int                              line;
    int                              column;
    const struct d_parse_bnf_config* config;
    struct d_parse_token             current;
};

struct d_parse_symbol
{
    char*                 name;
    enum DParseSymbolKind kind;
    int                   is_lhs;
};

struct d_parse_production
{
    int    lhs_index;
    int*   rhs_indices;
    size_t rhs_length;
};

struct d_parse_grammar
{
    struct d_parse_symbol*     symbols;
    size_t                     symbol_count;
    size_t                     symbol_capacity;

    struct d_parse_production* productions;
    size_t                     production_count;
    size_t                     production_capacity;

    int                        start_symbol_index;

    /* Counter for generating synthetic symbol names */
    size_t                     synthetic_counter;
};

struct d_parse_parser
{
    struct d_parse_lexer*   lexer;
    struct d_parse_grammar* grammar;

    struct d_parse_token    current;
    struct d_parse_token    lookahead;
    int                     has_lookahead;
};

/* ============================================================================
 * Utility helpers
 * ========================================================================== */

static void*
d_parse_realloc
(
    void*  _ptr,
    size_t _size
)
{
    void* result;

    result = realloc(_ptr, _size);

    if (!result)
    {
        fprintf(stderr, "d_parse_realloc: out of memory\n");
        exit(EXIT_FAILURE);
    }

    return result;
}

static struct d_parse_token
d_parse_make_token
(
    enum DParseTokenType _type,
    const char*          _start,
    size_t               _length,
    int                  _line,
    int                  _column
)
{
    struct d_parse_token token;

    token.type   = _type;
    token.lexeme = _start;
    token.length = _length;
    token.line   = _line;
    token.column = _column;

    return token;
}

/* ============================================================================
 * Lexer
 * ========================================================================== */

static int
d_parse_lexer_peek
(
    struct d_parse_lexer* _lexer
)
{
    if (_lexer->position >= _lexer->length)
    {
        return EOF;
    }

    return (unsigned char)_lexer->source[_lexer->position];
}

static int
d_parse_lexer_advance
(
    struct d_parse_lexer* _lexer
)
{
    int result;

    if (_lexer->position >= _lexer->length)
    {
        return EOF;
    }

    result = (unsigned char)_lexer->source[_lexer->position];
    _lexer->position += 1u;

    if (result == '\n')
    {
        _lexer->line  += 1;
        _lexer->column = 1;
    }
    else
    {
        _lexer->column += 1;
    }

    return result;
}

static unsigned int
d_parse_whitespace_bit_for_char
(
    int _ch
)
{
    switch (_ch)
    {
        case ' ':
            return D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_SPACE);

        case '\t':
            return D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_TAB);

        case '\n':
            return D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_NEWLINE);

        case '\r':
            return D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_CARRIAGE_RETURN);

        case '\f':
            return D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_FORM_FEED);

        case '\v':
            return D_PARSE_WHITESPACE_MASK(D_PARSE_WHITESPACE_KIND_VERTICAL_TAB);

        default:
            return 0u;
    }
}

static int
d_parse_is_config_whitespace
(
    const struct d_parse_lexer* _lexer,
    int                         _ch
)
{
    unsigned int bit;

    if (!_lexer || !_lexer->config)
    {
        /* Fallback: classic whitespace */
        return ( (_ch == ' ')
                 || (_ch == '\t')
                 || (_ch == '\n')
                 || (_ch == '\r')
                 || (_ch == '\f')
                 || (_ch == '\v') );
    }

    bit = d_parse_whitespace_bit_for_char(_ch);

    if (bit == 0u)
    {
        return 0;
    }

    return ((_lexer->config->whitespace_mask & bit) != 0u);
}

static void
d_parse_lexer_skip_whitespace_and_comments
(
    struct d_parse_lexer* _lexer
)
{
    const struct d_parse_bnf_config* config;
    int                              ch;

    config = _lexer->config;

    for (;;)
    {
        ch = d_parse_lexer_peek(_lexer);

        if (ch == EOF)
        {
            break;
        }

        /* Line comments if configured */
        if (config && (config->line_comment_char != 0)
            && (ch == (unsigned char)config->line_comment_char))
        {
            while ((ch != EOF) && (ch != '\n'))
            {
                d_parse_lexer_advance(_lexer);
                ch = d_parse_lexer_peek(_lexer);
            }

            continue;
        }

        if (d_parse_is_config_whitespace(_lexer, ch))
        {
            d_parse_lexer_advance(_lexer);
            continue;
        }

        break;
    }

    return;
}

static int
d_parse_lexer_is_ebnf_char
(
    const struct d_parse_lexer* _lexer,
    int                         _ch
)
{
    if (!_lexer || !_lexer->config || !_lexer->config->ebnf_extensions)
    {
        return 0;
    }

    return ( (_ch == '{') || (_ch == '}')
             || (_ch == '[') || (_ch == ']')
             || (_ch == '(') || (_ch == ')')
             || (_ch == '*') || (_ch == '+') || (_ch == '?') );
}

static struct d_parse_token
d_parse_lexer_next
(
    struct d_parse_lexer* _lexer
)
{
    int         ch;
    int         start_line;
    int         start_column;
    const char* start;
    size_t      length;
    int         ebnf_enabled;

    d_parse_lexer_skip_whitespace_and_comments(_lexer);

    ch = d_parse_lexer_peek(_lexer);

    if (ch == EOF)
    {
        return d_parse_make_token(D_PARSE_TOKEN_TYPE_STOP,
                                  _lexer->source + _lexer->length,
                                  0u,
                                  _lexer->line,
                                  _lexer->column);
    }

    start_line   = _lexer->line;
    start_column = _lexer->column;
    start        = _lexer->source + _lexer->position;

    ebnf_enabled = (_lexer->config && _lexer->config->ebnf_extensions);

    /* 1) string literal: '...' */
    if (ch == '\'')
    {
        const char* string_start;

        d_parse_lexer_advance(_lexer); /* consume opening quote */

        string_start = _lexer->source + _lexer->position;
        ch           = d_parse_lexer_peek(_lexer);

        while ((ch != EOF) && (ch != '\''))
        {
            if (ch == '\n')
            {
                fprintf(stderr,
                        "lexer error at %d:%d: newline in string literal\n",
                        _lexer->line,
                        _lexer->column);

                exit(EXIT_FAILURE);
            }

            d_parse_lexer_advance(_lexer);
            ch = d_parse_lexer_peek(_lexer);
        }

        if (ch != '\'')
        {
            fprintf(stderr,
                    "lexer error at %d:%d: unterminated string literal\n",
                    _lexer->line,
                    _lexer->column);

            exit(EXIT_FAILURE);
        }

        length =
            (size_t)((_lexer->source + _lexer->position) - string_start);

        d_parse_lexer_advance(_lexer); /* consume closing quote */

        return d_parse_make_token(D_PARSE_TOKEN_TYPE_STRING,
                                  string_start,
                                  length,
                                  start_line,
                                  start_column);
    }

    /* 2) '::=' at token start */
    if ( (ch == ':')
         && (_lexer->position + 2u < _lexer->length)
         && (_lexer->source[_lexer->position + 1u] == ':')
         && (_lexer->source[_lexer->position + 2u] == '=') )
    {
        d_parse_lexer_advance(_lexer);
        d_parse_lexer_advance(_lexer);
        d_parse_lexer_advance(_lexer);

        return d_parse_make_token(D_PARSE_TOKEN_TYPE_COLON_COLON_EQ,
                                  start,
                                  3u,
                                  start_line,
                                  start_column);
    }

    /* 3) '|' and ';' as dedicated tokens */
    if (ch == '|')
    {
        d_parse_lexer_advance(_lexer);

        return d_parse_make_token(D_PARSE_TOKEN_TYPE_PIPE,
                                  start,
                                  1u,
                                  start_line,
                                  start_column);
    }

    if (ch == ';')
    {
        d_parse_lexer_advance(_lexer);

        return d_parse_make_token(D_PARSE_TOKEN_TYPE_SEMI,
                                  start,
                                  1u,
                                  start_line,
                                  start_column);
    }

    /* 4) EBNF-specific single-character tokens */
    if (ebnf_enabled)
    {
        enum DParseTokenType ebnf_type;

        ebnf_type = D_PARSE_TOKEN_TYPE_STOP;

        switch (ch)
        {
            case '{': ebnf_type = D_PARSE_TOKEN_TYPE_LBRACE;   break;
            case '}': ebnf_type = D_PARSE_TOKEN_TYPE_RBRACE;   break;
            case '[': ebnf_type = D_PARSE_TOKEN_TYPE_LBRACKET; break;
            case ']': ebnf_type = D_PARSE_TOKEN_TYPE_RBRACKET; break;
            case '(': ebnf_type = D_PARSE_TOKEN_TYPE_LPAREN;   break;
            case ')': ebnf_type = D_PARSE_TOKEN_TYPE_RPAREN;   break;
            case '*': ebnf_type = D_PARSE_TOKEN_TYPE_STAR;     break;
            case '+': ebnf_type = D_PARSE_TOKEN_TYPE_PLUS;     break;
            case '?': ebnf_type = D_PARSE_TOKEN_TYPE_QUESTION; break;
            default:  break;
        }

        if (ebnf_type != D_PARSE_TOKEN_TYPE_STOP)
        {
            d_parse_lexer_advance(_lexer);

            return d_parse_make_token(ebnf_type,
                                      start,
                                      1u,
                                      start_line,
                                      start_column);
        }
    }

    /* 5) generic identifier-like chunk */
    start = _lexer->source + _lexer->position;

    for (;;)
    {
        ch = d_parse_lexer_peek(_lexer);

        if (ch == EOF)
        {
            break;
        }

        if (d_parse_is_config_whitespace(_lexer, ch))
        {
            break;
        }

        if ( (ch == '|') || 
             (ch == ';') || 
             (ch == '\'') )
        {
            break;
        }

        /* Stop at EBNF chars if enabled */
        if (d_parse_lexer_is_ebnf_char(_lexer, ch))
        {
            break;
        }

        if ( (ch == ':')                                    &&
             (_lexer->position + 2u < _lexer->length)       &&
             (_lexer->source[_lexer->position + 1u] == ':') &&
             (_lexer->source[_lexer->position + 2u] == '=') )
        {
            break;
        }

        d_parse_lexer_advance(_lexer);
    }

    length = (size_t)((_lexer->source + _lexer->position) - start);

    if (length == 0u)
    {
        return d_parse_make_token(D_PARSE_TOKEN_TYPE_STOP,
                                  _lexer->source + _lexer->length,
                                  0u,
                                  _lexer->line,
                                  _lexer->column);
    }

    return d_parse_make_token(D_PARSE_TOKEN_TYPE_IDENT,
                              start,
                              length,
                              start_line,
                              start_column);
}

static void
d_parse_lexer_init
(
    struct d_parse_lexer*            _lexer,
    const char*                      _source,
    const struct d_parse_bnf_config* _config
)
{
    size_t length;

    if ( (!_lexer) || 
         (!_source) )
    {
        fprintf(stderr, "d_parse_lexer_init: invalid argument\n");
        exit(EXIT_FAILURE);
    }

    length           = strlen(_source);
    _lexer->source   = _source;
    _lexer->length   = length;
    _lexer->position = 0u;
    _lexer->line     = 1;
    _lexer->column   = 1;
    _lexer->config   = _config;

    _lexer->current = d_parse_make_token(D_PARSE_TOKEN_TYPE_STOP,
                                         _lexer->source + _lexer->length,
                                         0u,
                                         _lexer->line,
                                         _lexer->column);
}

/* ============================================================================
 * Grammar helpers
 * ========================================================================== */

static void
d_parse_grammar_init
(
    struct d_parse_grammar* _grammar
)
{
    if (!_grammar)
    {
        return;
    }

    _grammar->symbols             = NULL;
    _grammar->symbol_count        = 0u;
    _grammar->symbol_capacity     = 0u;
    _grammar->productions         = NULL;
    _grammar->production_count    = 0u;
    _grammar->production_capacity = 0u;
    _grammar->start_symbol_index  = -1;
    _grammar->synthetic_counter   = 0u;
}

static int
d_parse_grammar_add_symbol
(
    struct d_parse_grammar* _grammar,
    const char*             _name,
    size_t                  _length
)
{
    size_t                 i;
    struct d_parse_symbol* symbols;
    struct d_parse_symbol* symbol;
    char*                  buffer;
    size_t                 new_capacity;

    if ( (!_grammar) || 
         (!_name) )
    {
        fprintf(stderr, "d_parse_grammar_add_symbol: invalid argument\n");
        exit(EXIT_FAILURE);
    }

    symbols = _grammar->symbols;

    for (i = 0u; i < _grammar->symbol_count; ++i)
    {
        symbol = &symbols[i];

        if (strlen(symbol->name) == _length
            && strncmp(symbol->name, _name, _length) == 0)
        {
            return (int)i;
        }
    }

    if (_grammar->symbol_count == _grammar->symbol_capacity)
    {
        new_capacity = (_grammar->symbol_capacity != 0u)
                ? (_grammar->symbol_capacity * 2u)
                : 16u;

        _grammar->symbols = d_parse_realloc(_grammar->symbols,
                            (new_capacity * sizeof(struct d_parse_symbol) ) );

        _grammar->symbol_capacity = new_capacity;
        symbols                   = _grammar->symbols;
    }

    symbol = &symbols[_grammar->symbol_count];

    buffer = (char*)malloc(_length + 1u);

    if (!buffer)
    {
        fprintf(stderr, "`d_parse_grammar_add_symbol`: out of memory\n");

        exit(EXIT_FAILURE);
    }

    memcpy(buffer, _name, _length);
    buffer[_length] = '\0';

    symbol->name   = buffer;
    symbol->kind   = D_PARSE_SYMBOL_KIND_UNKNOWN;
    symbol->is_lhs = 0;

    _grammar->symbol_count += 1u;

    return (int)(_grammar->symbol_count - 1u);
}

static int
d_parse_grammar_add_symbol_cstr
(
    struct d_parse_grammar* _grammar,
    const char*             _name
)
{
    return d_parse_grammar_add_symbol(_grammar, _name, strlen(_name));
}

/*
d_parse_grammar_create_synthetic_symbol
  Generate a unique synthetic nonterminal for EBNF desugaring.
  The name format is "__synth_<counter>__".
*/
static int
d_parse_grammar_create_synthetic_symbol
(
    struct d_parse_grammar* _grammar,
    const char*             _suffix
)
{
    char                   name_buffer[64];
    int                    index;
    struct d_parse_symbol* symbol;

    snprintf(name_buffer,
             sizeof(name_buffer),
             "__synth_%zu_%s__",
             _grammar->synthetic_counter,
             _suffix ? _suffix : "");

    _grammar->synthetic_counter += 1u;

    index  = d_parse_grammar_add_symbol_cstr(_grammar, name_buffer);
    symbol = &_grammar->symbols[index];

    symbol->kind   = D_PARSE_SYMBOL_KIND_SYNTHETIC;
    symbol->is_lhs = 1;

    return index;
}

static struct d_parse_production*
d_parse_grammar_new_production
(
    struct d_parse_grammar* _grammar
)
{
    size_t                     new_capacity;
    struct d_parse_production* production;

    if (!_grammar)
    {
        fprintf(stderr,
                "d_parse_grammar_new_production: invalid argument\n");
        exit(EXIT_FAILURE);
    }

    if (_grammar->production_count == _grammar->production_capacity)
    {
        new_capacity = (_grammar->production_capacity != 0u)
            ? (_grammar->production_capacity * 2u)
            : 16u;

        _grammar->productions = d_parse_realloc(_grammar->productions,
                            (new_capacity * sizeof(struct d_parse_production)) );

        _grammar->production_capacity = new_capacity;
    }

    production = &(_grammar->productions[_grammar->production_count]);

    _grammar->production_count += 1u;

    production->lhs_index   = -1;
    production->rhs_indices = NULL;
    production->rhs_length  = 0u;

    return production;
}

/*
d_parse_grammar_add_production
  Convenience function to add a production with given LHS and RHS.
*/
static void
d_parse_grammar_add_production
(
    struct d_parse_grammar* _grammar,
    int                     _lhs_index,
    const int*              _rhs_indices,
    size_t                  _rhs_length
)
{
    struct d_parse_production* production;

    production = d_parse_grammar_new_production(_grammar);

    production->lhs_index  = _lhs_index;
    production->rhs_length = _rhs_length;

    if (_rhs_length > 0u)
    {
        production->rhs_indices = (int*)malloc(_rhs_length * sizeof(int));

        if (!production->rhs_indices)
        {
            fprintf(stderr,
                    "d_parse_grammar_add_production: out of memory\n");

            exit(EXIT_FAILURE);
        }

        memcpy(production->rhs_indices,
               _rhs_indices,
               _rhs_length * sizeof(int));
    }
    else
    {
        production->rhs_indices = NULL;
    }
}

static void
d_parse_grammar_classify_symbols
(
    struct d_parse_grammar* _grammar
)
{
    size_t                 i;
    struct d_parse_symbol* symbol;

    if (!_grammar)
    {
        return;
    }

    for (i = 0u; i < _grammar->symbol_count; ++i)
    {
        symbol = &_grammar->symbols[i];

        if (symbol->kind == D_PARSE_SYMBOL_KIND_UNKNOWN)
        {
            if (symbol->is_lhs)
            {
                symbol->kind = D_PARSE_SYMBOL_KIND_NONTERM;
            }
            else
            {
                symbol->kind = D_PARSE_SYMBOL_KIND_TERM;
            }
        }
    }
}

static void
d_parse_grammar_destroy
(
    struct d_parse_grammar* _grammar
)
{
    size_t i;

    if (!_grammar)
    {
        return;
    }

    if (_grammar->symbols)
    {
        for (i = 0u; i < _grammar->symbol_count; ++i)
        {
            free(_grammar->symbols[i].name);
        }

        free(_grammar->symbols);
    }

    if (_grammar->productions)
    {
        for (i = 0u; i < _grammar->production_count; ++i)
        {
            free(_grammar->productions[i].rhs_indices);
        }

        free(_grammar->productions);
    }

    _grammar->symbols             = NULL;
    _grammar->symbol_count        = 0u;
    _grammar->symbol_capacity     = 0u;
    _grammar->productions         = NULL;
    _grammar->production_count    = 0u;
    _grammar->production_capacity = 0u;
    _grammar->start_symbol_index  = -1;
    _grammar->synthetic_counter   = 0u;
}

/* ============================================================================
 * Parser helpers
 * ========================================================================== */

static struct d_parse_token*
d_parse_parser_current
(
    struct d_parse_parser* _parser
)
{
    return &_parser->current;
}

static struct d_parse_token*
d_parse_parser_peek
(
    struct d_parse_parser* _parser
)
{
    if (_parser->has_lookahead == 0)
    {
        _parser->lookahead =
            d_parse_lexer_next(_parser->lexer);

        _parser->has_lookahead = 1;
    }

    return &_parser->lookahead;
}

static void
d_parse_parser_advance
(
    struct d_parse_parser* _parser
)
{
    if (_parser->has_lookahead != 0)
    {
        _parser->current       = _parser->lookahead;
        _parser->has_lookahead = 0;
    }
    else
    {
        _parser->current =
            d_parse_lexer_next(_parser->lexer);
    }

    return;
}

static void
d_parse_parser_expect
(
    struct d_parse_parser* _parser,
    enum DParseTokenType   _expected_type,
    const char*            _message
)
{
    struct d_parse_token* token;

    token = d_parse_parser_current(_parser);

    if (token->type != _expected_type)
    {
        fprintf(stderr,
                "parse error at %d:%d: expected %s\n",
                token->line,
                token->column,
                _message);

        exit(EXIT_FAILURE);
    }

    return;
}

static int
d_parse_token_matches_cstring
(
    const struct d_parse_token* _token,
    const char*                 _text
)
{
    size_t length;

    if (!_token || !_text || (_text[0] == '\0'))
    {
        return 0;
    }

    length = strlen(_text);

    if (_token->length != length)
    {
        return 0;
    }

    if (strncmp(_token->lexeme, _text, length) != 0)
    {
        return 0;
    }

    return 1;
}

#endif  /* DJINTERP_PARSE_GRAMMAR_BNF_COMMON_ */
