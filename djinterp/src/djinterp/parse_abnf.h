/******************************************************************************
* djinterp [parse]                                                parse_abnf.h
*
*   Augmented BNF (ABNF) grammar parser per RFC 5234. Supports:
*     - Rule definition:    rulename = elements
*     - Incremental alt:    rulename =/ elements
*     - Alternation:        element / element
*     - Concatenation:      element element (whitespace-separated)
*     - Repetition:         *element, 1*element, *2element, 3*5element, 3element
*     - Optional:           [element]
*     - Grouping:           (element element)
*     - String literals:    "text" (case-insensitive), %s"text" (case-sensitive)
*     - Numeric values:     %d65, %x41, %b1000001
*     - Value ranges:       %x30-39
*     - Value concatenation:%x48.65.6C.6C.6F
*     - Comments:           ; to end of line
*     - Prose values:       <descriptive text>
*
* path:      \inc\parse\grammar\parse_abnf.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.09
******************************************************************************/

#ifndef DJINTERP_PARSE_GRAMMAR_ABNF_
#define DJINTERP_PARSE_GRAMMAR_ABNF_ 1

#include "parse_bnf_common.h"
#include <ctype.h>

/* ============================================================================
 * ABNF-specific token types
 * ========================================================================== */

enum DTokenType_ABNF
{
    D_ABNF_TOKEN_TYPE_STOP = 0,
    D_ABNF_TOKEN_TYPE_RULENAME,         /* case-insensitive identifier */
    D_ABNF_TOKEN_TYPE_EQUALS,           /* = */
    D_ABNF_TOKEN_TYPE_EQUALS_SLASH,     /* =/ (incremental alternatives) */
    D_ABNF_TOKEN_TYPE_SLASH,            /* / (alternation) */
    D_ABNF_TOKEN_TYPE_STAR,             /* * (repetition) */
    D_ABNF_TOKEN_TYPE_NUMBER,           /* decimal number for repetition */
    D_ABNF_TOKEN_TYPE_LPAREN,           /* ( */
    D_ABNF_TOKEN_TYPE_RPAREN,           /* ) */
    D_ABNF_TOKEN_TYPE_LBRACKET,         /* [ */
    D_ABNF_TOKEN_TYPE_RBRACKET,         /* ] */
    D_ABNF_TOKEN_TYPE_DQUOTE_STRING,    /* "..." case-insensitive string */
    D_ABNF_TOKEN_TYPE_SENSITIVE_STRING, /* %s"..." case-sensitive string */
    D_ABNF_TOKEN_TYPE_NUMERIC,          /* %d, %x, %b value (possibly range/concat) */
    D_ABNF_TOKEN_TYPE_PROSE,            /* <prose description> */
    D_ABNF_TOKEN_TYPE_NEWLINE           /* significant line break */
};

/* ============================================================================
 * ABNF token structure
 * ========================================================================== */

struct d_token_ABNF
{
    enum DTokenType_ABNF type;
    const char*          lexeme;
    size_t               length;
    int                  line;
    int                  column;

    /* For NUMBER tokens */
    int                  number_value;

    /* For NUMERIC tokens: parsed values */
    int*                 numeric_values;
    size_t               numeric_count;
    int                  is_range;       /* 1 if this is a range (e.g., %x30-39) */
    int                  range_end;      /* end value if is_range */
};

/* ============================================================================
 * ABNF lexer structure
 * ========================================================================== */

struct d_abnf_lexer
{
    const char*         source;
    size_t              length;
    size_t              position;
    int                 line;
    int                 column;
    struct d_token_ABNF current;
};

/* ============================================================================
 * ABNF parser structure
 * ========================================================================== */

struct d_parser_abnf
{
    struct d_abnf_lexer*    lexer;
    struct d_parse_grammar* grammar;
    struct d_token_ABNF     current;
    struct d_token_ABNF     lookahead;
    int                     has_lookahead;
};

/* ============================================================================
 * Forward declarations
 * ========================================================================== */

static int
d_parser_abnf_parse_element
(
    struct d_parser_abnf* _parser
);

static void
d_parser_abnf_parse_alternation
(
    struct d_parser_abnf* _parser,
    int                   _lhs_index
);

static int
d_abnf_create_repetition_n_m
(
    struct d_parse_grammar* _grammar,
    int                     _symbol_index,
    int                     _min,
    int                     _max
);

static int
d_abnf_create_group
(
    struct d_parser_abnf* _parser
);

/* ============================================================================
 * ABNF Lexer helpers
 * ========================================================================== */

static int
d_abnf_lexer_peek
(
    struct d_abnf_lexer* _lexer
)
{
    if (_lexer->position >= _lexer->length)
    {
        return EOF;
    }

    return (unsigned char)_lexer->source[_lexer->position];
}

static int
d_abnf_lexer_peek_ahead
(
    struct d_abnf_lexer* _lexer,
    size_t               _offset
)
{
    if (_lexer->position + _offset >= _lexer->length)
    {
        return EOF;
    }

    return (unsigned char)_lexer->source[_lexer->position + _offset];
}

static int
d_abnf_lexer_advance
(
    struct d_abnf_lexer* _lexer
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

static void
d_abnf_lexer_skip_whitespace
(
    struct d_abnf_lexer* _lexer,
    int                  _skip_newlines
)
{
    int ch;

    for (;;)
    {
        ch = d_abnf_lexer_peek(_lexer);

        if (ch == EOF)
        {
            break;
        }

        /* Skip spaces and tabs always */
        if ((ch == ' ') || (ch == '\t'))
        {
            d_abnf_lexer_advance(_lexer);
            continue;
        }

        /* Skip carriage return */
        if (ch == '\r')
        {
            d_abnf_lexer_advance(_lexer);
            continue;
        }

        /* Handle newlines */
        if (ch == '\n')
        {
            if (_skip_newlines)
            {
                d_abnf_lexer_advance(_lexer);
                continue;
            }
            else
            {
                break;
            }
        }

        /* ABNF comments: ; to end of line */
        if (ch == ';')
        {
            while ((ch != EOF) && (ch != '\n'))
            {
                d_abnf_lexer_advance(_lexer);
                ch = d_abnf_lexer_peek(_lexer);
            }

            continue;
        }

        break;
    }
}

static struct d_token_ABNF
d_abnf_make_token
(
    enum DTokenType_ABNF _type,
    const char*         _start,
    size_t              _length,
    int                 _line,
    int                 _column
)
{
    struct d_token_ABNF token;

    memset(&token, 0, sizeof(token));

    token.type   = _type;
    token.lexeme = _start;
    token.length = _length;
    token.line   = _line;
    token.column = _column;

    return token;
}

static int
d_abnf_is_rulename_start
(
    int _ch
)
{
    return isalpha(_ch);
}

static int
d_abnf_is_rulename_char
(
    int _ch
)
{
    return (isalnum(_ch) || (_ch == '-'));
}

static int
d_abnf_parse_decimal
(
    struct d_abnf_lexer* _lexer
)
{
    int value;
    int ch;

    value = 0;
    ch    = d_abnf_lexer_peek(_lexer);

    while (isdigit(ch))
    {
        value = (value * 10) + (ch - '0');
        d_abnf_lexer_advance(_lexer);
        ch = d_abnf_lexer_peek(_lexer);
    }

    return value;
}

static int
d_abnf_parse_hex
(
    struct d_abnf_lexer* _lexer
)
{
    int value;
    int ch;

    value = 0;
    ch    = d_abnf_lexer_peek(_lexer);

    while (isxdigit(ch))
    {
        value *= 16;

        if (isdigit(ch))
        {
            value += (ch - '0');
        }
        else if ((ch >= 'a') && (ch <= 'f'))
        {
            value += (ch - 'a' + 10);
        }
        else
        {
            value += (ch - 'A' + 10);
        }

        d_abnf_lexer_advance(_lexer);
        ch = d_abnf_lexer_peek(_lexer);
    }

    return value;
}

static int
d_abnf_parse_binary
(
    struct d_abnf_lexer* _lexer
)
{
    int value;
    int ch;

    value = 0;
    ch    = d_abnf_lexer_peek(_lexer);

    while ((ch == '0') || (ch == '1'))
    {
        value = (value * 2) + (ch - '0');
        d_abnf_lexer_advance(_lexer);
        ch = d_abnf_lexer_peek(_lexer);
    }

    return value;
}

static struct d_token_ABNF
d_abnf_lexer_scan_numeric
(
    struct d_abnf_lexer* _lexer,
    int                  _start_line,
    int                  _start_column
)
{
    struct d_token_ABNF token;
    int                 ch;
    int                 base;
    int*                values;
    size_t              count;
    size_t              capacity;
    int                 first_value;

    memset(&token, 0, sizeof(token));

    token.type   = D_ABNF_TOKEN_TYPE_NUMERIC;
    token.lexeme = _lexer->source + _lexer->position - 1; /* include % */
    token.line   = _start_line;
    token.column = _start_column;

    ch = d_abnf_lexer_peek(_lexer);

    /* Determine base */
    if ((ch == 'd') || (ch == 'D'))
    {
        base = 10;
        d_abnf_lexer_advance(_lexer);
    }
    else if ((ch == 'x') || (ch == 'X'))
    {
        base = 16;
        d_abnf_lexer_advance(_lexer);
    }
    else if ((ch == 'b') || (ch == 'B'))
    {
        base = 2;
        d_abnf_lexer_advance(_lexer);
    }
    else
    {
        fprintf(stderr,
                "lexer error at %d:%d: expected 'd', 'x', or 'b' after %%\n",
                _lexer->line,
                _lexer->column);

        exit(EXIT_FAILURE);
    }

    /* Parse first value */
    if (base == 10)
    {
        first_value = d_abnf_parse_decimal(_lexer);
    }
    else if (base == 16)
    {
        first_value = d_abnf_parse_hex(_lexer);
    }
    else
    {
        first_value = d_abnf_parse_binary(_lexer);
    }

    ch = d_abnf_lexer_peek(_lexer);

    /* Check for range (-) or concatenation (.) */
    if (ch == '-')
    {
        int end_value;

        d_abnf_lexer_advance(_lexer);

        if (base == 10)
        {
            end_value = d_abnf_parse_decimal(_lexer);
        }
        else if (base == 16)
        {
            end_value = d_abnf_parse_hex(_lexer);
        }
        else
        {
            end_value = d_abnf_parse_binary(_lexer);
        }

        token.is_range        = 1;
        token.numeric_values  = (int*)malloc(sizeof(int));
        token.numeric_values[0] = first_value;
        token.numeric_count   = 1u;
        token.range_end       = end_value;
    }
    else if (ch == '.')
    {
        /* Concatenation of values */
        values   = NULL;
        count    = 0u;
        capacity = 0u;

        /* Add first value */
        values    = (int*)malloc(4u * sizeof(int));
        capacity  = 4u;
        values[0] = first_value;
        count     = 1u;

        while (ch == '.')
        {
            int next_value;

            d_abnf_lexer_advance(_lexer);

            if (base == 10)
            {
                next_value = d_abnf_parse_decimal(_lexer);
            }
            else if (base == 16)
            {
                next_value = d_abnf_parse_hex(_lexer);
            }
            else
            {
                next_value = d_abnf_parse_binary(_lexer);
            }

            if (count == capacity)
            {
                capacity *= 2u;
                values = (int*)realloc(values, capacity * sizeof(int));
            }

            values[count] = next_value;
            count        += 1u;

            ch = d_abnf_lexer_peek(_lexer);
        }

        token.numeric_values = values;
        token.numeric_count  = count;
        token.is_range       = 0;
    }
    else
    {
        /* Single value */
        token.numeric_values    = (int*)malloc(sizeof(int));
        token.numeric_values[0] = first_value;
        token.numeric_count     = 1u;
        token.is_range          = 0;
    }

    token.length = (size_t)((_lexer->source + _lexer->position) - token.lexeme);

    return token;
}

static struct d_token_ABNF
d_abnf_lexer_next
(
    struct d_abnf_lexer* _lexer
)
{
    int         ch;
    int         start_line;
    int         start_column;
    const char* start;

    /* Skip whitespace but NOT newlines (they're significant in ABNF) */
    d_abnf_lexer_skip_whitespace(_lexer, 0);

    ch = d_abnf_lexer_peek(_lexer);

    if (ch == EOF)
    {
        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_STOP,
                                 _lexer->source + _lexer->length,
                                 0u,
                                 _lexer->line,
                                 _lexer->column);
    }

    start_line   = _lexer->line;
    start_column = _lexer->column;
    start        = _lexer->source + _lexer->position;

    /* Newline token */
    if (ch == '\n')
    {
        d_abnf_lexer_advance(_lexer);

        /* Skip any continuation whitespace (WSP after CRLF continues rule) */
        ch = d_abnf_lexer_peek(_lexer);

        if ((ch == ' ') || (ch == '\t'))
        {
            /* This is a continuation line, skip and get next token */
            d_abnf_lexer_skip_whitespace(_lexer, 0);

            return d_abnf_lexer_next(_lexer);
        }

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_NEWLINE,
                                 start,
                                 1u,
                                 start_line,
                                 start_column);
    }

    /* =/ or = */
    if (ch == '=')
    {
        d_abnf_lexer_advance(_lexer);
        ch = d_abnf_lexer_peek(_lexer);

        if (ch == '/')
        {
            d_abnf_lexer_advance(_lexer);

            return d_abnf_make_token(D_ABNF_TOKEN_TYPE_EQUALS_SLASH,
                                     start,
                                     2u,
                                     start_line,
                                     start_column);
        }

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_EQUALS,
                                 start,
                                 1u,
                                 start_line,
                                 start_column);
    }

    /* / (alternation) */
    if (ch == '/')
    {
        d_abnf_lexer_advance(_lexer);

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_SLASH,
                                 start,
                                 1u,
                                 start_line,
                                 start_column);
    }

    /* * (repetition) */
    if (ch == '*')
    {
        d_abnf_lexer_advance(_lexer);

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_STAR,
                                 start,
                                 1u,
                                 start_line,
                                 start_column);
    }

    /* Grouping */
    if (ch == '(')
    {
        d_abnf_lexer_advance(_lexer);

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_LPAREN,
                                 start,
                                 1u,
                                 start_line,
                                 start_column);
    }

    if (ch == ')')
    {
        d_abnf_lexer_advance(_lexer);

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_RPAREN,
                                 start,
                                 1u,
                                 start_line,
                                 start_column);
    }

    /* Optional */
    if (ch == '[')
    {
        d_abnf_lexer_advance(_lexer);

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_LBRACKET,
                                 start,
                                 1u,
                                 start_line,
                                 start_column);
    }

    if (ch == ']')
    {
        d_abnf_lexer_advance(_lexer);

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_RBRACKET,
                                 start,
                                 1u,
                                 start_line,
                                 start_column);
    }

    /* Prose value: <...> */
    if (ch == '<')
    {
        const char* prose_start;
        size_t      prose_length;

        d_abnf_lexer_advance(_lexer);
        prose_start = _lexer->source + _lexer->position;
        ch          = d_abnf_lexer_peek(_lexer);

        while ((ch != EOF) && (ch != '>'))
        {
            d_abnf_lexer_advance(_lexer);
            ch = d_abnf_lexer_peek(_lexer);
        }

        prose_length = (size_t)((_lexer->source + _lexer->position) - prose_start);

        if (ch == '>')
        {
            d_abnf_lexer_advance(_lexer);
        }

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_PROSE,
                                 prose_start,
                                 prose_length,
                                 start_line,
                                 start_column);
    }

    /* Double-quoted string (case-insensitive) */
    if (ch == '"')
    {
        const char* string_start;
        size_t      string_length;

        d_abnf_lexer_advance(_lexer);
        string_start = _lexer->source + _lexer->position;
        ch           = d_abnf_lexer_peek(_lexer);

        while ((ch != EOF) && (ch != '"') && (ch != '\n'))
        {
            d_abnf_lexer_advance(_lexer);
            ch = d_abnf_lexer_peek(_lexer);
        }

        string_length = (size_t)((_lexer->source + _lexer->position) - string_start);

        if (ch == '"')
        {
            d_abnf_lexer_advance(_lexer);
        }

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_DQUOTE_STRING,
                                 string_start,
                                 string_length,
                                 start_line,
                                 start_column);
    }

    /* % - numeric value or %s/%i string */
    if (ch == '%')
    {
        int next_ch;

        d_abnf_lexer_advance(_lexer);
        next_ch = d_abnf_lexer_peek(_lexer);

        /* %s"..." case-sensitive string */
        if ((next_ch == 's') || (next_ch == 'S'))
        {
            const char* string_start;
            size_t      string_length;

            d_abnf_lexer_advance(_lexer);
            ch = d_abnf_lexer_peek(_lexer);

            if (ch != '"')
            {
                fprintf(stderr,
                        "lexer error at %d:%d: expected '\"' after %%s\n",
                        _lexer->line,
                        _lexer->column);

                exit(EXIT_FAILURE);
            }

            d_abnf_lexer_advance(_lexer);
            string_start = _lexer->source + _lexer->position;
            ch           = d_abnf_lexer_peek(_lexer);

            while ((ch != EOF) && (ch != '"') && (ch != '\n'))
            {
                d_abnf_lexer_advance(_lexer);
                ch = d_abnf_lexer_peek(_lexer);
            }

            string_length = (size_t)((_lexer->source + _lexer->position) - string_start);

            if (ch == '"')
            {
                d_abnf_lexer_advance(_lexer);
            }

            return d_abnf_make_token(D_ABNF_TOKEN_TYPE_SENSITIVE_STRING,
                                     string_start,
                                     string_length,
                                     start_line,
                                     start_column);
        }

        /* %i"..." case-insensitive (same as regular "") */
        if ((next_ch == 'i') || (next_ch == 'I'))
        {
            const char* string_start;
            size_t      string_length;

            d_abnf_lexer_advance(_lexer);
            ch = d_abnf_lexer_peek(_lexer);

            if (ch != '"')
            {
                fprintf(stderr,
                        "lexer error at %d:%d: expected '\"' after %%i\n",
                        _lexer->line,
                        _lexer->column);

                exit(EXIT_FAILURE);
            }

            d_abnf_lexer_advance(_lexer);
            string_start = _lexer->source + _lexer->position;
            ch           = d_abnf_lexer_peek(_lexer);

            while ((ch != EOF) && (ch != '"') && (ch != '\n'))
            {
                d_abnf_lexer_advance(_lexer);
                ch = d_abnf_lexer_peek(_lexer);
            }

            string_length = (size_t)((_lexer->source + _lexer->position) - string_start);

            if (ch == '"')
            {
                d_abnf_lexer_advance(_lexer);
            }

            return d_abnf_make_token(D_ABNF_TOKEN_TYPE_DQUOTE_STRING,
                                     string_start,
                                     string_length,
                                     start_line,
                                     start_column);
        }

        /* Numeric value: %d, %x, %b */
        return d_abnf_lexer_scan_numeric(_lexer, start_line, start_column);
    }

    /* Decimal number (for repetition) */
    if (isdigit(ch))
    {
        struct d_token_ABNF token;
        int                 value;

        value = d_abnf_parse_decimal(_lexer);

        token = d_abnf_make_token(D_ABNF_TOKEN_TYPE_NUMBER,
                                  start,
                                  (size_t)((_lexer->source + _lexer->position) - start),
                                  start_line,
                                  start_column);

        token.number_value = value;

        return token;
    }

    /* Rule name (identifier) */
    if (d_abnf_is_rulename_start(ch))
    {
        while (d_abnf_is_rulename_char(d_abnf_lexer_peek(_lexer)))
        {
            d_abnf_lexer_advance(_lexer);
        }

        return d_abnf_make_token(D_ABNF_TOKEN_TYPE_RULENAME,
                                 start,
                                 (size_t)((_lexer->source + _lexer->position) - start),
                                 start_line,
                                 start_column);
    }

    /* Unknown character - skip and try again */
    fprintf(stderr,
            "lexer warning at %d:%d: unexpected character '%c'\n",
            _lexer->line,
            _lexer->column,
            ch);

    d_abnf_lexer_advance(_lexer);

    return d_abnf_lexer_next(_lexer);
}

static void
d_abnf_lexer_init
(
    struct d_abnf_lexer* _lexer,
    const char*          _source
)
{
    if (!_lexer || !_source)
    {
        fprintf(stderr, "d_abnf_lexer_init: invalid argument\n");
        exit(EXIT_FAILURE);
    }

    _lexer->source   = _source;
    _lexer->length   = strlen(_source);
    _lexer->position = 0u;
    _lexer->line     = 1;
    _lexer->column   = 1;

    memset(&_lexer->current, 0, sizeof(_lexer->current));
}

static void
d_token_ABNF_destroy
(
    struct d_token_ABNF* _token
)
{
    if (_token && _token->numeric_values)
    {
        free(_token->numeric_values);
        _token->numeric_values = NULL;
    }
}

/* ============================================================================
 * ABNF Parser helpers
 * ========================================================================== */

static struct d_token_ABNF*
d_parser_abnf_current
(
    struct d_parser_abnf* _parser
)
{
    return &_parser->current;
}

static struct d_token_ABNF*
d_parser_abnf_peek
(
    struct d_parser_abnf* _parser
)
{
    if (_parser->has_lookahead == 0)
    {
        _parser->lookahead     = d_abnf_lexer_next(_parser->lexer);
        _parser->has_lookahead = 1;
    }

    return &_parser->lookahead;
}

static void
d_parser_abnf_advance
(
    struct d_parser_abnf* _parser
)
{
    /* Don't destroy numeric_values as they may be referenced */
    if (_parser->has_lookahead != 0)
    {
        _parser->current       = _parser->lookahead;
        _parser->has_lookahead = 0;
    }
    else
    {
        _parser->current = d_abnf_lexer_next(_parser->lexer);
    }
}

static void
d_parser_abnf_skip_newlines
(
    struct d_parser_abnf* _parser
)
{
    struct d_token_ABNF* token;

    token = d_parser_abnf_current(_parser);

    while (token->type == D_ABNF_TOKEN_TYPE_NEWLINE)
    {
        d_parser_abnf_advance(_parser);
        token = d_parser_abnf_current(_parser);
    }
}

/* ============================================================================
 * ABNF Symbol name normalization (lowercase for case-insensitivity)
 * ========================================================================== */

static int
d_abnf_add_rulename_symbol
(
    struct d_parse_grammar* _grammar,
    const char*             _name,
    size_t                  _length
)
{
    char*  normalized;
    size_t i;
    int    index;

    /* Create lowercase version of rule name */
    normalized = (char*)malloc(_length + 1u);

    if (!normalized)
    {
        fprintf(stderr, "d_abnf_add_rulename_symbol: out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0u; i < _length; ++i)
    {
        normalized[i] = (char)tolower((unsigned char)_name[i]);
    }

    normalized[_length] = '\0';

    index = d_parse_grammar_add_symbol(_grammar, normalized, _length);

    free(normalized);

    return index;
}

/* ============================================================================
 * ABNF Helper: Create terminal symbol for string literal
 * ========================================================================== */

static int
d_abnf_add_string_terminal
(
    struct d_parse_grammar* _grammar,
    const char*             _str,
    size_t                  _length,
    int                     _case_sensitive
)
{
    char*                  buffer;
    size_t                 prefix_len;
    int                    index;
    struct d_parse_symbol* symbol;

    /* Create a terminal name like "str:hello" or "STR:Hello" */
    prefix_len = _case_sensitive ? 4u : 4u;  /* "str:" */
    buffer     = (char*)malloc(prefix_len + _length + 1u);

    if (!buffer)
    {
        fprintf(stderr, "d_abnf_add_string_terminal: out of memory\n");
        exit(EXIT_FAILURE);
    }

    if (_case_sensitive)
    {
        memcpy(buffer, "STR:", 4u);
        memcpy(buffer + 4u, _str, _length);
    }
    else
    {
        size_t i;

        memcpy(buffer, "str:", 4u);

        /* Lowercase for case-insensitive matching */
        for (i = 0u; i < _length; ++i)
        {
            buffer[4u + i] = (char)tolower((unsigned char)_str[i]);
        }
    }

    buffer[prefix_len + _length] = '\0';

    index  = d_parse_grammar_add_symbol(_grammar, buffer, prefix_len + _length);
    symbol = &_grammar->symbols[index];
    symbol->kind = D_PARSE_SYMBOL_KIND_TERM;

    free(buffer);

    return index;
}

/* ============================================================================
 * ABNF Helper: Create terminal symbol for numeric value
 * ========================================================================== */

static int
d_abnf_add_numeric_terminal
(
    struct d_parse_grammar* _grammar,
    int                     _value
)
{
    char                   buffer[32];
    int                    index;
    struct d_parse_symbol* symbol;

    snprintf(buffer, sizeof(buffer), "%%x%02X", _value);

    index  = d_parse_grammar_add_symbol_cstr(_grammar, buffer);
    symbol = &_grammar->symbols[index];
    symbol->kind = D_PARSE_SYMBOL_KIND_TERM;

    return index;
}

/* ============================================================================
 * ABNF Helper: Create range terminal (or choice of terminals)
 *
 *   For a range like %x30-39, creates a synthetic nonterminal with
 *   alternatives for each value in the range.
 * ========================================================================== */

static int
d_abnf_add_numeric_range
(
    struct d_parse_grammar* _grammar,
    int                     _start,
    int                     _end
)
{
    int synth_index;
    int i;

    if (_start == _end)
    {
        return d_abnf_add_numeric_terminal(_grammar, _start);
    }

    synth_index = d_parse_grammar_create_synthetic_symbol(_grammar, "range");

    for (i = _start; i <= _end; ++i)
    {
        int term_index;

        term_index = d_abnf_add_numeric_terminal(_grammar, i);

        d_parse_grammar_add_production(_grammar,
                                       synth_index,
                                       &term_index,
                                       1u);
    }

    return synth_index;
}

/* ============================================================================
 * ABNF Helper: Create concatenation of numeric values
 * ========================================================================== */

static int
d_abnf_add_numeric_concat
(
    struct d_parse_grammar* _grammar,
    const int*              _values,
    size_t                  _count
)
{
    int    synth_index;
    int*   rhs;
    size_t i;

    if (_count == 1u)
    {
        return d_abnf_add_numeric_terminal(_grammar, _values[0]);
    }

    synth_index = d_parse_grammar_create_synthetic_symbol(_grammar, "concat");

    rhs = (int*)malloc(_count * sizeof(int));

    if (!rhs)
    {
        fprintf(stderr, "d_abnf_add_numeric_concat: out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0u; i < _count; ++i)
    {
        rhs[i] = d_abnf_add_numeric_terminal(_grammar, _values[i]);
    }

    d_parse_grammar_add_production(_grammar,
                                   synth_index,
                                   rhs,
                                   _count);

    free(rhs);

    return synth_index;
}

/* ============================================================================
 * ABNF Helper: Create repetition with min/max bounds
 *
 *   n*m X  =>  X X ... X (n times) [X]? [X]? ... (m-n times)
 *   *X     =>  0*inf
 *   n*X    =>  n*inf
 *   *mX    =>  0*m
 *   nX     =>  exactly n times
 * ========================================================================== */

static int
d_abnf_create_repetition_n_m
(
    struct d_parse_grammar* _grammar,
    int                     _symbol_index,
    int                     _min,
    int                     _max
)
{
    int     synth_index;
    int*    rhs;
    size_t  rhs_len;
    size_t  i;

    /* Special case: 0*1 is just optional */
    if ((_min == 0) && (_max == 1))
    {
        return d_parse_ebnf_create_optional(_grammar, _symbol_index);
    }

    /* Special case: 0* is just repetition */
    if ((_min == 0) && (_max < 0))
    {
        return d_parse_ebnf_create_repetition(_grammar, _symbol_index);
    }

    /* Special case: 1* is one-or-more */
    if ((_min == 1) && (_max < 0))
    {
        return d_parse_ebnf_create_one_or_more(_grammar, _symbol_index);
    }

    synth_index = d_parse_grammar_create_synthetic_symbol(_grammar, "rep");

    /* Build: min copies of symbol, then (max-min) optional copies */
    /* If max is unbounded (<0), add a repetition at the end */

    if (_max < 0)
    {
        /* n* case: n copies followed by * */
        int rep_index;

        rep_index = d_parse_ebnf_create_repetition(_grammar, _symbol_index);

        rhs_len = (size_t)_min + 1u;
        rhs     = (int*)malloc(rhs_len * sizeof(int));

        if (!rhs)
        {
            fprintf(stderr, "d_abnf_create_repetition_n_m: out of memory\n");
            exit(EXIT_FAILURE);
        }

        for (i = 0u; i < (size_t)_min; ++i)
        {
            rhs[i] = _symbol_index;
        }

        rhs[_min] = rep_index;

        d_parse_grammar_add_production(_grammar,
                                       synth_index,
                                       rhs,
                                       rhs_len);

        free(rhs);
    }
    else
    {
        /* n*m case: build from max down to min */
        /* Each production has: min required + some optional */

        /* Actually, simpler approach: */
        /* synth -> X X ... X (m times) */
        /* synth -> X X ... X (m-1 times) */
        /* ... */
        /* synth -> X X ... X (n times) */

        int count;

        for (count = _max; count >= _min; --count)
        {
            if (count == 0)
            {
                /* Epsilon production */
                d_parse_grammar_add_production(_grammar,
                                               synth_index,
                                               NULL,
                                               0u);
            }
            else
            {
                rhs = (int*)malloc((size_t)count * sizeof(int));

                if (!rhs)
                {
                    fprintf(stderr,
                            "d_abnf_create_repetition_n_m: out of memory\n");
                    exit(EXIT_FAILURE);
                }

                for (i = 0u; i < (size_t)count; ++i)
                {
                    rhs[i] = _symbol_index;
                }

                d_parse_grammar_add_production(_grammar,
                                               synth_index,
                                               rhs,
                                               (size_t)count);

                free(rhs);
            }
        }
    }

    return synth_index;
}

/* ============================================================================
 * ABNF Helper: Create group (parse parenthesized alternation)
 * ========================================================================== */

static int
d_abnf_create_group
(
    struct d_parser_abnf* _parser
)
{
    int synth_index;

    synth_index = d_parse_grammar_create_synthetic_symbol(_parser->grammar,
                                                          "group");

    /* Parse the grouped alternation */
    d_parser_abnf_parse_alternation(_parser, synth_index);

    return synth_index;
}

/* ============================================================================
 * ABNF Parser: Check if token ends a concatenation
 * ========================================================================== */

static int
d_abnf_is_concatenation_end
(
    struct d_token_ABNF* _token
)
{
    enum DTokenType_ABNF type;

    type = _token->type;

    return ( (type == D_ABNF_TOKEN_TYPE_STOP)
             || (type == D_ABNF_TOKEN_TYPE_NEWLINE)
             || (type == D_ABNF_TOKEN_TYPE_SLASH)
             || (type == D_ABNF_TOKEN_TYPE_RPAREN)
             || (type == D_ABNF_TOKEN_TYPE_RBRACKET) );
}

/* ============================================================================
 * ABNF Parser: Check if token can start an element
 * ========================================================================== */

static int
d_abnf_is_element_start
(
    enum DTokenType_ABNF _type
)
{
    return ( (_type == D_ABNF_TOKEN_TYPE_RULENAME)
             || (_type == D_ABNF_TOKEN_TYPE_DQUOTE_STRING)
             || (_type == D_ABNF_TOKEN_TYPE_SENSITIVE_STRING)
             || (_type == D_ABNF_TOKEN_TYPE_NUMERIC)
             || (_type == D_ABNF_TOKEN_TYPE_PROSE)
             || (_type == D_ABNF_TOKEN_TYPE_LPAREN)
             || (_type == D_ABNF_TOKEN_TYPE_LBRACKET)
             || (_type == D_ABNF_TOKEN_TYPE_STAR)
             || (_type == D_ABNF_TOKEN_TYPE_NUMBER) );
}

/* ============================================================================
 * ABNF Parser: Parse a repetition specification and element
 *
 *   repetition = [repeat] element
 *   repeat     = 1*DIGIT / (*DIGIT "*" *DIGIT)
 * ========================================================================== */

static int
d_parser_abnf_parse_element
(
    struct d_parser_abnf* _parser
)
{
    struct d_token_ABNF* token;
    int                  min_rep;
    int                  max_rep;
    int                  has_star;
    int                  symbol_index;

    token    = d_parser_abnf_current(_parser);
    min_rep  = 0;
    max_rep  = -1;  /* -1 means unbounded */
    has_star = 0;

    /* Parse optional repetition prefix */
    if (token->type == D_ABNF_TOKEN_TYPE_NUMBER)
    {
        min_rep = token->number_value;

        d_parser_abnf_advance(_parser);
        token = d_parser_abnf_current(_parser);

        if (token->type == D_ABNF_TOKEN_TYPE_STAR)
        {
            has_star = 1;

            d_parser_abnf_advance(_parser);
            token = d_parser_abnf_current(_parser);

            if (token->type == D_ABNF_TOKEN_TYPE_NUMBER)
            {
                max_rep = token->number_value;

                d_parser_abnf_advance(_parser);
                token = d_parser_abnf_current(_parser);
            }
            /* else max_rep stays -1 (unbounded) */
        }
        else
        {
            /* Just "n element" means exactly n */
            max_rep = min_rep;
        }
    }
    else if (token->type == D_ABNF_TOKEN_TYPE_STAR)
    {
        has_star = 1;
        min_rep  = 0;

        d_parser_abnf_advance(_parser);
        token = d_parser_abnf_current(_parser);

        if (token->type == D_ABNF_TOKEN_TYPE_NUMBER)
        {
            max_rep = token->number_value;

            d_parser_abnf_advance(_parser);
            token = d_parser_abnf_current(_parser);
        }
        /* else max_rep stays -1 (unbounded) */
    }

    /* Now parse the actual element */

    /* Rule name reference */
    if (token->type == D_ABNF_TOKEN_TYPE_RULENAME)
    {
        symbol_index = d_abnf_add_rulename_symbol(_parser->grammar,
                                                   token->lexeme,
                                                   token->length);

        d_parser_abnf_advance(_parser);
    }
    /* String literal */
    else if (token->type == D_ABNF_TOKEN_TYPE_DQUOTE_STRING)
    {
        symbol_index = d_abnf_add_string_terminal(_parser->grammar,
                                                   token->lexeme,
                                                   token->length,
                                                   0);

        d_parser_abnf_advance(_parser);
    }
    /* Case-sensitive string */
    else if (token->type == D_ABNF_TOKEN_TYPE_SENSITIVE_STRING)
    {
        symbol_index = d_abnf_add_string_terminal(_parser->grammar,
                                                   token->lexeme,
                                                   token->length,
                                                   1);

        d_parser_abnf_advance(_parser);
    }
    /* Numeric value */
    else if (token->type == D_ABNF_TOKEN_TYPE_NUMERIC)
    {
        if (token->is_range)
        {
            symbol_index = d_abnf_add_numeric_range(_parser->grammar,
                                                     token->numeric_values[0],
                                                     token->range_end);
        }
        else if (token->numeric_count > 1u)
        {
            symbol_index = d_abnf_add_numeric_concat(_parser->grammar,
                                                      token->numeric_values,
                                                      token->numeric_count);
        }
        else
        {
            symbol_index = d_abnf_add_numeric_terminal(_parser->grammar,
                                                        token->numeric_values[0]);
        }

        d_parser_abnf_advance(_parser);
    }
    /* Prose value */
    else if (token->type == D_ABNF_TOKEN_TYPE_PROSE)
    {
        char*                  buffer;
        struct d_parse_symbol* symbol;

        buffer = (char*)malloc(token->length + 3u);

        if (!buffer)
        {
            fprintf(stderr, "d_parser_abnf_parse_element: out of memory\n");
            exit(EXIT_FAILURE);
        }

        buffer[0] = '<';
        memcpy(buffer + 1u, token->lexeme, token->length);
        buffer[token->length + 1u] = '>';
        buffer[token->length + 2u] = '\0';

        symbol_index = d_parse_grammar_add_symbol(_parser->grammar,
                                                   buffer,
                                                   token->length + 2u);

        symbol       = &_parser->grammar->symbols[symbol_index];
        symbol->kind = D_PARSE_SYMBOL_KIND_TERM;

        free(buffer);

        d_parser_abnf_advance(_parser);
    }
    /* Grouping: ( ... ) */
    else if (token->type == D_ABNF_TOKEN_TYPE_LPAREN)
    {
        d_parser_abnf_advance(_parser);

        symbol_index = d_abnf_create_group(_parser);

        token = d_parser_abnf_current(_parser);

        if (token->type != D_ABNF_TOKEN_TYPE_RPAREN)
        {
            fprintf(stderr,
                    "parse error at %d:%d: expected ')'\n",
                    token->line,
                    token->column);

            exit(EXIT_FAILURE);
        }

        d_parser_abnf_advance(_parser);
    }
    /* Optional: [ ... ] */
    else if (token->type == D_ABNF_TOKEN_TYPE_LBRACKET)
    {
        int group_index;

        d_parser_abnf_advance(_parser);

        group_index = d_abnf_create_group(_parser);

        token = d_parser_abnf_current(_parser);

        if (token->type != D_ABNF_TOKEN_TYPE_RBRACKET)
        {
            fprintf(stderr,
                    "parse error at %d:%d: expected ']'\n",
                    token->line,
                    token->column);

            exit(EXIT_FAILURE);
        }

        d_parser_abnf_advance(_parser);

        /* [X] is equivalent to *1X */
        symbol_index = d_parse_ebnf_create_optional(_parser->grammar,
                                                    group_index);

        /* Don't apply repetition again since optional was explicitly used */
        return symbol_index;
    }
    else
    {
        /* No valid element */
        return -1;
    }

    /* Apply repetition if specified */
    if (has_star || (min_rep != 1) || (max_rep != 1))
    {
        if (!has_star && (min_rep == max_rep))
        {
            /* Exact count: handled by repetition_n_m */
            symbol_index = d_abnf_create_repetition_n_m(_parser->grammar,
                                                        symbol_index,
                                                        min_rep,
                                                        max_rep);
        }
        else
        {
            symbol_index = d_abnf_create_repetition_n_m(_parser->grammar,
                                                        symbol_index,
                                                        min_rep,
                                                        max_rep);
        }
    }

    return symbol_index;
}

/* ============================================================================
 * ABNF Parser: Parse a concatenation (sequence of elements)
 * ========================================================================== */

static void
d_parser_abnf_parse_concatenation
(
    struct d_parser_abnf* _parser,
    int                   _lhs_index
)
{
    int*                       tmp_rhs;
    size_t                     tmp_count;
    size_t                     tmp_capacity;
    struct d_token_ABNF*       token;
    struct d_parse_production* production;

    tmp_rhs      = NULL;
    tmp_count    = 0u;
    tmp_capacity = 0u;

    token = d_parser_abnf_current(_parser);

    while (!d_abnf_is_concatenation_end(token))
    {
        int    symbol_index;
        size_t new_capacity;

        if (!d_abnf_is_element_start(token->type))
        {
            break;
        }

        symbol_index = d_parser_abnf_parse_element(_parser);

        if (symbol_index < 0)
        {
            break;
        }

        if (tmp_count == tmp_capacity)
        {
            new_capacity = (tmp_capacity != 0u) ? (tmp_capacity * 2u) : 4u;
            tmp_rhs      = (int*)realloc(tmp_rhs, new_capacity * sizeof(int));

            if (!tmp_rhs)
            {
                fprintf(stderr,
                        "d_parser_abnf_parse_concatenation: out of memory\n");
                exit(EXIT_FAILURE);
            }

            tmp_capacity = new_capacity;
        }

        tmp_rhs[tmp_count] = symbol_index;
        tmp_count         += 1u;

        token = d_parser_abnf_current(_parser);
    }

    production = d_parse_grammar_new_production(_parser->grammar);
    production->lhs_index  = _lhs_index;
    production->rhs_length = tmp_count;

    if (tmp_count > 0u)
    {
        production->rhs_indices = (int*)malloc(tmp_count * sizeof(int));

        if (!production->rhs_indices)
        {
            fprintf(stderr,
                    "d_parser_abnf_parse_concatenation: out of memory\n");
            exit(EXIT_FAILURE);
        }

        memcpy(production->rhs_indices, tmp_rhs, tmp_count * sizeof(int));
    }
    else
    {
        production->rhs_indices = NULL;  /* epsilon */
    }

    free(tmp_rhs);
}

/* ============================================================================
 * ABNF Parser: Parse alternation (alternatives separated by /)
 * ========================================================================== */

static void
d_parser_abnf_parse_alternation
(
    struct d_parser_abnf* _parser,
    int                   _lhs_index
)
{
    struct d_token_ABNF* token;

    d_parser_abnf_parse_concatenation(_parser, _lhs_index);

    token = d_parser_abnf_current(_parser);

    while (token->type == D_ABNF_TOKEN_TYPE_SLASH)
    {
        d_parser_abnf_advance(_parser);

        d_parser_abnf_parse_concatenation(_parser, _lhs_index);

        token = d_parser_abnf_current(_parser);
    }
}

/* ============================================================================
 * ABNF Parser: Parse a rule definition
 * ========================================================================== */

static void
d_parser_abnf_parse_rule
(
    struct d_parser_abnf* _parser
)
{
    struct d_token_ABNF*   token;
    int                    lhs_index;
    struct d_parse_symbol* lhs_symbol;
    int                    is_incremental;

    token = d_parser_abnf_current(_parser);

    if (token->type != D_ABNF_TOKEN_TYPE_RULENAME)
    {
        fprintf(stderr,
                "parse error at %d:%d: expected rule name\n",
                token->line,
                token->column);

        exit(EXIT_FAILURE);
    }

    lhs_index = d_abnf_add_rulename_symbol(_parser->grammar,
                                            token->lexeme,
                                            token->length);

    lhs_symbol         = &_parser->grammar->symbols[lhs_index];
    lhs_symbol->kind   = D_PARSE_SYMBOL_KIND_NONTERM;
    lhs_symbol->is_lhs = 1;

    d_parser_abnf_advance(_parser);
    token = d_parser_abnf_current(_parser);

    /* Check for = or =/ */
    is_incremental = 0;

    if (token->type == D_ABNF_TOKEN_TYPE_EQUALS_SLASH)
    {
        is_incremental = 1;
        d_parser_abnf_advance(_parser);
    }
    else if (token->type == D_ABNF_TOKEN_TYPE_EQUALS)
    {
        d_parser_abnf_advance(_parser);
    }
    else
    {
        fprintf(stderr,
                "parse error at %d:%d: expected '=' or '=/'\n",
                token->line,
                token->column);

        exit(EXIT_FAILURE);
    }

    /* Note: is_incremental could be used to validate no duplicate rules */
    (void)is_incremental;

    /* Parse the rule body (alternation) */
    d_parser_abnf_parse_alternation(_parser, lhs_index);

    /* Consume trailing newline if present */
    token = d_parser_abnf_current(_parser);

    if (token->type == D_ABNF_TOKEN_TYPE_NEWLINE)
    {
        d_parser_abnf_advance(_parser);
    }
}

/* ============================================================================
 * Public interface
 * ========================================================================== */

void
d_parse_grammar_from_abnf
(
    struct d_parse_grammar* _grammar,
    const char*             _source
)
{
    struct d_parser_abnf  parser;
    struct d_abnf_lexer   lexer;
    struct d_token_ABNF*  token;
    size_t                i;

    if (!_grammar || !_source)
    {
        fprintf(stderr, "d_parse_grammar_from_abnf: invalid argument\n");
        exit(EXIT_FAILURE);
    }

    d_parse_grammar_init(_grammar);

    memset(&parser, 0, sizeof(parser));
    memset(&lexer,  0, sizeof(lexer));

    parser.grammar       = _grammar;
    parser.lexer         = &lexer;
    parser.has_lookahead = 0;

    d_abnf_lexer_init(parser.lexer, _source);

    parser.current = d_abnf_lexer_next(parser.lexer);

    /* Skip leading newlines */
    d_parser_abnf_skip_newlines(&parser);

    token = d_parser_abnf_current(&parser);

    while (token->type != D_ABNF_TOKEN_TYPE_STOP)
    {
        if (token->type == D_ABNF_TOKEN_TYPE_NEWLINE)
        {
            d_parser_abnf_advance(&parser);
            token = d_parser_abnf_current(&parser);
            continue;
        }

        if (token->type == D_ABNF_TOKEN_TYPE_RULENAME)
        {
            d_parser_abnf_parse_rule(&parser);
        }
        else
        {
            fprintf(stderr,
                    "parse error at %d:%d: expected rule definition\n",
                    token->line,
                    token->column);

            exit(EXIT_FAILURE);
        }

        token = d_parser_abnf_current(&parser);
    }

    /* Infer start symbol from first LHS */
    if (_grammar->start_symbol_index < 0)
    {
        for (i = 0u; i < _grammar->symbol_count; ++i)
        {
            if (_grammar->symbols[i].is_lhs)
            {
                _grammar->start_symbol_index = (int)i;
                break;
            }
        }
    }

    d_parse_grammar_classify_symbols(_grammar);
}

#endif  /* DJINTERP_PARSE_GRAMMAR_ABNF_ */
