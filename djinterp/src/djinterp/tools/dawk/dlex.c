/******************************************************************************
* djinterp [dawk]                                                       dlex.c
*
*   Definitions for the non-inline declarations in dlex.h.
*     The scanner keeps one bit of history: whether the previous token can end
* an expression. That bit decides whether a slash opens a regular expression or
* divides, and it is the whole of awk's notorious lexical ambiguity. It also
* decides nothing else, so the parser needs no feedback channel.
*
*
* path:      /src/djinterp/tools/dawk/dlex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dlex.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free, strtod
#include <string.h>  // memcpy, strlen, strncmp


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Scanner state
//------------------------------------------------------------------------------
// 1.1.1
// d_awk_lexer
//   struct: cursor, the scratch buffer holding the current token's text, and
//   the one bit of history the slash rule needs.
struct d_awk_lexer
{
    const char* source;
    size_t      length;
    size_t      at;
    size_t      line;
    size_t      column;
    const char* origin;
    char*       scratch;
    size_t      scratch_capacity;
    bool        operand_ends;   // the previous token can end an expression
    bool        suppress_nl;    // a newline here cannot terminate a statement
    size_t      error_line;
    size_t      error_column;
    char        error[192];
};

// 1.1.2
// d_internal_keyword
//   struct: one reserved word and the kind it produces.
struct d_internal_keyword
{
    const char*           text;
    enum d_awk_token_kind kind;
};

// POSIX reserves `function` and not `func`.  one-true-awk and gawk accept
// `func` as a synonym, but reserving it breaks the perfectly legal program
// `BEGIN { func = 5 }`, which mawk runs.  Fidelity wins; if the synonym is
// ever wanted it belongs behind a non-POSIX flag rather than in this table.
static const struct d_internal_keyword g_keywords[] =
{
    { "BEGIN",    D_AWK_TOK_BEGIN    },
    { "END",      D_AWK_TOK_END      },
    { "function", D_AWK_TOK_FUNCTION },
    { "if",       D_AWK_TOK_IF       },
    { "else",     D_AWK_TOK_ELSE     },
    { "while",    D_AWK_TOK_WHILE    },
    { "for",      D_AWK_TOK_FOR      },
    { "do",       D_AWK_TOK_DO       },
    { "break",    D_AWK_TOK_BREAK    },
    { "continue", D_AWK_TOK_CONTINUE },
    { "next",     D_AWK_TOK_NEXT     },
    { "nextfile", D_AWK_TOK_NEXTFILE },
    { "exit",     D_AWK_TOK_EXIT     },
    { "return",   D_AWK_TOK_RETURN   },
    { "delete",   D_AWK_TOK_DELETE   },
    { "in",       D_AWK_TOK_IN       },
    { "print",    D_AWK_TOK_PRINT    },
    { "printf",   D_AWK_TOK_PRINTF   },
    { "getline",  D_AWK_TOK_GETLINE  }
};

static const char* const g_builtins[] =
{
    "length", "substr", "index", "split", "sub", "gsub", "match", "sprintf",
    "sin", "cos", "atan2", "exp", "log", "sqrt", "int", "rand", "srand",
    "tolower", "toupper", "system", "close"
};


//==============================================================================
// 2.  SCRATCH AND CURSOR
//==============================================================================


/*
d_internal_reserve
  Ensures the scratch buffer can hold a token of a given length.

Parameter(s):
  _lexer:  the scanner.
  _needed: the number of bytes required, excluding the terminator.
Return:
  A boolean value corresponding to either:
  - true, if the buffer is large enough, or
  - false, otherwise.
*/
static bool
d_internal_reserve(
    struct d_awk_lexer* _lexer,
    size_t              _needed
)
{
    // grow the buffer when the token and its terminator do not fit
    if (_lexer->scratch_capacity < (_needed + 1u))
    {
        size_t capacity = (_lexer->scratch_capacity == 0)
                        ? 64u
                        : _lexer->scratch_capacity;

        while (capacity < (_needed + 1u))
        {
            capacity *= 2u;
        }

        char* grown = realloc(_lexer->scratch, capacity);

        // report the failure rather than truncating the token
        if (!grown)
        {
            return false;
        }

        _lexer->scratch          = grown;
        _lexer->scratch_capacity = capacity;
    }

    return true;
}


/*
d_internal_fail
  Records a diagnostic and stops the scan.

Parameter(s):
  _lexer:   the scanner.
  _message: the text of the diagnostic.
Return:
  none.
*/
static void
d_internal_fail(
    struct d_awk_lexer* _lexer,
    const char*         _message
)
{
    // The message is stored raw.  Position and origin are added by whoever
    // reports it, so a scanner diagnostic and a parser diagnostic come out in
    // one format instead of the parser wrapping an already-formatted string
    // with a position it has not reached yet.
    if (_lexer->error[0] == '\0')
    {
        snprintf(_lexer->error, sizeof(_lexer->error), "%s", _message);
        _lexer->error_line   = _lexer->line;
        _lexer->error_column = _lexer->column;
    }

    return;
}


/*
d_internal_peek
  Returns the byte at an offset from the cursor without consuming it.

Parameter(s):
  _lexer:  the scanner.
  _ahead:  how far past the cursor to look.
Return:
  The byte, or -1 past the end of the source.
*/
static int
d_internal_peek(
    const struct d_awk_lexer* _lexer,
    size_t                    _ahead
)
{
    const size_t at = _lexer->at + _ahead;

    return (at < _lexer->length)
         ? (int)(unsigned char)_lexer->source[at]
         : -1;
}


/*
d_internal_advance
  Consumes one byte, maintaining the line and column counters.

Parameter(s):
  _lexer: the scanner.
Return:
  The byte consumed, or -1 past the end of the source.
*/
static int
d_internal_advance(
    struct d_awk_lexer* _lexer
)
{
    const int byte = d_internal_peek(_lexer, 0);

    // there is nothing to consume past the end
    if (byte < 0)
    {
        return -1;
    }

    _lexer->at++;

    // a newline resets the column and advances the line
    if (byte == '\n')
    {
        _lexer->line++;
        _lexer->column = 1;
    }
    else
    {
        _lexer->column++;
    }

    return byte;
}


//==============================================================================
// 3.  TOKEN SCANNERS
//==============================================================================


/*
d_internal_scan_string
  Consumes a string literal and unescapes it into the scratch buffer.

Parameter(s):
  _lexer:      the scanner, positioned at the opening quote.
  _out_token:  receives the token.
Return:
  A boolean value corresponding to either:
  - true, if a literal was consumed, or
  - false, otherwise.
*/
static bool
d_internal_scan_string(
    struct d_awk_lexer* _lexer,
    struct d_awk_token* _out_token
)
{
    (void)d_internal_advance(_lexer);  // consume the opening quote

    size_t used = 0;

    // consume until the closing quote, unescaping as we go
    while (true)
    {
        const int byte = d_internal_peek(_lexer, 0);

        // an unterminated literal is an error
        if ((byte < 0) || (byte == '\n'))
        {
            d_internal_fail(_lexer, "unterminated string literal");
            return false;
        }

        (void)d_internal_advance(_lexer);

        // the closing quote ends the literal
        if (byte == '"')
        {
            break;
        }

        int value = byte;

        // a backslash introduces an escape
        if (byte == '\\')
        {
            const int next = d_internal_advance(_lexer);

            // a trailing backslash is an error
            if (next < 0)
            {
                d_internal_fail(_lexer, "unterminated escape");
                return false;
            }

            switch (next)
            {
                case 'n':  value = '\n'; break;
                case 't':  value = '\t'; break;
                case 'r':  value = '\r'; break;
                case '\\': value = '\\'; break;
                case '"':  value = '"';  break;
                case '/':  value = '/';  break;
                case 'a':  value = '\a'; break;
                case 'b':  value = '\b'; break;
                case 'f':  value = '\f'; break;
                case 'v':  value = '\v'; break;
                default:   value = next; break;
            }

            // an octal escape takes up to three digits
            if ((next >= '0') && (next <= '7'))
            {
                value = next - '0';

                for (int taken = 0; taken < 2; ++taken)
                {
                    const int digit = d_internal_peek(_lexer, 0);

                    // stop at the first byte that is not an octal digit
                    if ((digit < '0') || (digit > '7'))
                    {
                        break;
                    }

                    value = (value * 8) + (digit - '0');
                    (void)d_internal_advance(_lexer);
                }
            }
        }

        // report the failure rather than truncating the literal
        if (!d_internal_reserve(_lexer, used + 1u))
        {
            d_internal_fail(_lexer, "out of memory");
            return false;
        }

        _lexer->scratch[used++] = (char)value;
    }

    // report the failure rather than returning an unterminated buffer
    if (!d_internal_reserve(_lexer, used))
    {
        d_internal_fail(_lexer, "out of memory");
        return false;
    }

    _lexer->scratch[used] = '\0';
    _out_token->kind      = D_AWK_TOK_STRING;
    _out_token->text      = _lexer->scratch;
    _out_token->length    = used;

    return true;
}


/*
d_internal_scan_ere
  Consumes a regular-expression literal into the scratch buffer.
NOTE:
  A backslash and the byte after it are both kept, since the pattern text is
  handed to the regex compiler rather than interpreted here.

Parameter(s):
  _lexer:     the scanner, positioned at the opening slash.
  _out_token: receives the token.
Return:
  A boolean value corresponding to either:
  - true, if a literal was consumed, or
  - false, otherwise.
*/
static bool
d_internal_scan_ere(
    struct d_awk_lexer* _lexer,
    struct d_awk_token* _out_token
)
{
    (void)d_internal_advance(_lexer);  // consume the opening slash

    size_t used      = 0;
    bool   in_bracket = false;

    // consume until the closing slash, tracking bracket expressions
    while (true)
    {
        const int byte = d_internal_peek(_lexer, 0);

        // an unterminated literal is an error
        if ((byte < 0) || (byte == '\n'))
        {
            d_internal_fail(_lexer, "unterminated regular expression");
            return false;
        }

        (void)d_internal_advance(_lexer);

        // a slash outside a bracket expression ends the literal
        if ((byte == '/') && (!in_bracket))
        {
            break;
        }

        // report the failure rather than truncating the literal
        if (!d_internal_reserve(_lexer, used + 2u))
        {
            d_internal_fail(_lexer, "out of memory");
            return false;
        }

        // a bracket expression suspends the closing-slash test
        if (byte == '[')
        {
            in_bracket = true;
        }
        else if (byte == ']')
        {
            in_bracket = false;
        }

        _lexer->scratch[used++] = (char)byte;

        // a backslash carries the byte after it through untouched
        if (byte == '\\')
        {
            const int next = d_internal_advance(_lexer);

            // a trailing backslash is an error
            if (next < 0)
            {
                d_internal_fail(_lexer, "unterminated escape");
                return false;
            }

            _lexer->scratch[used++] = (char)next;
        }
    }

    _lexer->scratch[used] = '\0';
    _out_token->kind      = D_AWK_TOK_ERE;
    _out_token->text      = _lexer->scratch;
    _out_token->length    = used;

    return true;
}


/*
d_internal_scan_word
  Consumes a name, resolving it to a keyword, builtin or plain name.

Parameter(s):
  _lexer:     the scanner, positioned at the first name byte.
  _out_token: receives the token.
Return:
  A boolean value corresponding to either:
  - true, if a word was consumed, or
  - false, otherwise.
*/
static bool
d_internal_scan_word(
    struct d_awk_lexer* _lexer,
    struct d_awk_token* _out_token
)
{
    const size_t start = _lexer->at;

    // a name is one or more letters, digits and underscores
    while (true)
    {
        const int byte = d_internal_peek(_lexer, 0);

        if ( (byte == '_')                          ||
             ((byte >= 'a') && (byte <= 'z'))       ||
             ((byte >= 'A') && (byte <= 'Z'))       ||
             ((byte >= '0') && (byte <= '9')) )
        {
            (void)d_internal_advance(_lexer);
            continue;
        }

        break;
    }

    const size_t span = _lexer->at - start;

    // report the failure rather than truncating the name
    if (!d_internal_reserve(_lexer, span))
    {
        d_internal_fail(_lexer, "out of memory");
        return false;
    }

    memcpy(_lexer->scratch, &_lexer->source[start], span);
    _lexer->scratch[span] = '\0';

    _out_token->text   = _lexer->scratch;
    _out_token->length = span;

    // a reserved word wins over a name
    for (size_t at = 0; at < (sizeof(g_keywords) / sizeof(g_keywords[0])); ++at)
    {
        if ( (strlen(g_keywords[at].text) == span) &&
             (strncmp(g_keywords[at].text, _lexer->scratch, span) == 0) )
        {
            _out_token->kind = g_keywords[at].kind;
            return true;
        }
    }

    // a builtin name is distinguished so the parser can check its arity
    for (size_t at = 0; at < (sizeof(g_builtins) / sizeof(g_builtins[0])); ++at)
    {
        if ( (strlen(g_builtins[at]) == span) &&
             (strncmp(g_builtins[at], _lexer->scratch, span) == 0) )
        {
            _out_token->kind = D_AWK_TOK_BUILTIN;
            return true;
        }
    }

    // a name followed immediately by '(' is a call, with no space permitted
    _out_token->kind = (d_internal_peek(_lexer, 0) == '(')
                     ? D_AWK_TOK_FUNC_NAME
                     : D_AWK_TOK_NAME;

    return true;
}


/*
d_internal_scan_number
  Consumes a numeric literal.

Parameter(s):
  _lexer:     the scanner, positioned at the first digit or point.
  _out_token: receives the token.
Return:
  A boolean value corresponding to either:
  - true, if a literal was consumed, or
  - false, otherwise.
*/
static bool
d_internal_scan_number(
    struct d_awk_lexer* _lexer,
    struct d_awk_token* _out_token
)
{
    const size_t start = _lexer->at;

    // the integer part
    while ((d_internal_peek(_lexer, 0) >= '0') &&
           (d_internal_peek(_lexer, 0) <= '9'))
    {
        (void)d_internal_advance(_lexer);
    }

    // an optional fraction
    if (d_internal_peek(_lexer, 0) == '.')
    {
        (void)d_internal_advance(_lexer);

        while ((d_internal_peek(_lexer, 0) >= '0') &&
               (d_internal_peek(_lexer, 0) <= '9'))
        {
            (void)d_internal_advance(_lexer);
        }
    }

    // an exponent counts only when it carries at least one digit
    if ((d_internal_peek(_lexer, 0) == 'e') ||
        (d_internal_peek(_lexer, 0) == 'E'))
    {
        size_t ahead = 1;

        // the exponent may itself be signed
        if ((d_internal_peek(_lexer, ahead) == '+') ||
            (d_internal_peek(_lexer, ahead) == '-'))
        {
            ahead++;
        }

        // only digits after the sign make it an exponent
        if ((d_internal_peek(_lexer, ahead) >= '0') &&
            (d_internal_peek(_lexer, ahead) <= '9'))
        {
            while (ahead-- > 0)
            {
                (void)d_internal_advance(_lexer);
            }

            while ((d_internal_peek(_lexer, 0) >= '0') &&
                   (d_internal_peek(_lexer, 0) <= '9'))
            {
                (void)d_internal_advance(_lexer);
            }
        }
    }

    const size_t span = _lexer->at - start;

    // report the failure rather than truncating the literal
    if (!d_internal_reserve(_lexer, span))
    {
        d_internal_fail(_lexer, "out of memory");
        return false;
    }

    memcpy(_lexer->scratch, &_lexer->source[start], span);
    _lexer->scratch[span] = '\0';

    _out_token->kind   = D_AWK_TOK_NUMBER;
    _out_token->number = strtod(_lexer->scratch, NULL);
    _out_token->text   = _lexer->scratch;
    _out_token->length = span;

    return true;
}


//==============================================================================
// 4.  PUBLIC INTERFACE
//==============================================================================


/*
d_awk_lexer_new
  Creates a scanner over a program's source text.

Parameter(s):
  _source: the program text.
  _length: its length in bytes.
  _origin: the name used in diagnostics; may be NULL.
Return:
  The scanner, or NULL on failure.
*/
struct d_awk_lexer*
d_awk_lexer_new(
    const char* _source,
    size_t      _length,
    const char* _origin
)
{
    // parameter validation first
    if ((!_source) && (_length > 0))
    {
        return NULL;
    }

    struct d_awk_lexer* lexer = calloc(1, sizeof(*lexer));

    // abandon the allocation when the handle could not be held
    if (!lexer)
    {
        return NULL;
    }

    lexer->source = _source;
    lexer->length = _length;
    lexer->line   = 1;
    lexer->column = 1;
    lexer->origin = _origin;

    return lexer;
}


/*
d_awk_lexer_free
  Releases a scanner.

Parameter(s):
  _lexer: the scanner to release; may be NULL.
Return:
  none.
*/
void
d_awk_lexer_free(
    struct d_awk_lexer* _lexer
)
{
    if (_lexer)
    {
        free(_lexer->scratch);
        free(_lexer);
    }

    return;
}


/*
d_awk_lexer_next
  Produces the next token.
NOTE:
  A newline is suppressed after any token that cannot end a statement -- an
  opening brace, a comma, `&&`, `||`, `do`, `else`, a semicolon and the
  statement-introducing keywords -- so the parser never sees a terminator
  where the grammar does not allow one.

Parameter(s):
  _lexer:     the scanner.
  _out_token: receives the token.
Return:
  A boolean value corresponding to either:
  - true, if a token was produced, or
  - false, at end of input or on error.
*/
bool
d_awk_lexer_next(
    struct d_awk_lexer* _lexer,
    struct d_awk_token* _out_token
)
{
    // parameter validation first
    if ((!_lexer) || (!_out_token) || (_lexer->error[0] != '\0'))
    {
        return false;
    }

    // skip blanks, comments, escaped newlines, and suppressed newlines
    while (true)
    {
        const int byte = d_internal_peek(_lexer, 0);

        // blanks never carry meaning between tokens
        if ((byte == ' ') || (byte == '\t') || (byte == '\r'))
        {
            (void)d_internal_advance(_lexer);
            continue;
        }

        // a comment runs to the end of the line but not past it
        if (byte == '#')
        {
            while ( (d_internal_peek(_lexer, 0) >= 0) &&
                    (d_internal_peek(_lexer, 0) != '\n') )
            {
                (void)d_internal_advance(_lexer);
            }

            continue;
        }

        // a backslash before a newline continues the line
        if ((byte == '\\') && (d_internal_peek(_lexer, 1) == '\n'))
        {
            (void)d_internal_advance(_lexer);
            (void)d_internal_advance(_lexer);
            continue;
        }

        // a newline the grammar cannot accept is swallowed here
        if ((byte == '\n') && (_lexer->suppress_nl))
        {
            (void)d_internal_advance(_lexer);
            continue;
        }

        break;
    }

    _out_token->kind   = D_AWK_TOK_EOF;
    _out_token->text   = NULL;
    _out_token->length = 0;
    _out_token->number = 0.0;
    _out_token->line   = _lexer->line;
    _out_token->column = _lexer->column;

    const int byte = d_internal_peek(_lexer, 0);

    // end of input produces the terminal token
    if (byte < 0)
    {
        return false;
    }

    bool ok = true;

    // dispatch on the first byte of the token
    if (byte == '\n')
    {
        (void)d_internal_advance(_lexer);
        _out_token->kind = D_AWK_TOK_NEWLINE;
    }
    else if (byte == '"')
    {
        ok = d_internal_scan_string(_lexer, _out_token);
    }
    else if ( (byte == '_')                    ||
              ((byte >= 'a') && (byte <= 'z')) ||
              ((byte >= 'A') && (byte <= 'Z')) )
    {
        ok = d_internal_scan_word(_lexer, _out_token);
    }
    else if ( ((byte >= '0') && (byte <= '9')) ||
              ((byte == '.') && (d_internal_peek(_lexer, 1) >= '0') &&
               (d_internal_peek(_lexer, 1) <= '9')) )
    {
        ok = d_internal_scan_number(_lexer, _out_token);
    }
    else if ((byte == '/') && (!_lexer->operand_ends))
    {
        // the slash rule: an operand cannot follow an operand, so a slash
        // here opens a regular expression rather than dividing
        ok = d_internal_scan_ere(_lexer, _out_token);
    }
    else
    {
        (void)d_internal_advance(_lexer);

        const int next = d_internal_peek(_lexer, 0);

        // two-character operators are matched before their prefixes
        switch (byte)
        {
            case '{': _out_token->kind = D_AWK_TOK_LBRACE;   break;
            case '}': _out_token->kind = D_AWK_TOK_RBRACE;   break;
            case '(': _out_token->kind = D_AWK_TOK_LPAREN;   break;
            case ')': _out_token->kind = D_AWK_TOK_RPAREN;   break;
            case '[': _out_token->kind = D_AWK_TOK_LBRACKET; break;
            case ']': _out_token->kind = D_AWK_TOK_RBRACKET; break;
            case ';': _out_token->kind = D_AWK_TOK_SEMI;     break;
            case ',': _out_token->kind = D_AWK_TOK_COMMA;    break;
            case '?': _out_token->kind = D_AWK_TOK_QUESTION; break;
            case ':': _out_token->kind = D_AWK_TOK_COLON;    break;
            case '$': _out_token->kind = D_AWK_TOK_DOLLAR;   break;
            case '~': _out_token->kind = D_AWK_TOK_MATCH;    break;

            case '+':
                _out_token->kind = (next == '+') ? D_AWK_TOK_INCR
                                 : ((next == '=') ? D_AWK_TOK_ADD_ASSIGN
                                                  : D_AWK_TOK_PLUS);
                break;

            case '-':
                _out_token->kind = (next == '-') ? D_AWK_TOK_DECR
                                 : ((next == '=') ? D_AWK_TOK_SUB_ASSIGN
                                                  : D_AWK_TOK_MINUS);
                break;

            case '*':
                _out_token->kind = (next == '=') ? D_AWK_TOK_MUL_ASSIGN
                                                 : D_AWK_TOK_STAR;
                break;

            case '/':
                _out_token->kind = (next == '=') ? D_AWK_TOK_DIV_ASSIGN
                                                 : D_AWK_TOK_SLASH;
                break;

            case '%':
                _out_token->kind = (next == '=') ? D_AWK_TOK_MOD_ASSIGN
                                                 : D_AWK_TOK_PERCENT;
                break;

            case '^':
                _out_token->kind = (next == '=') ? D_AWK_TOK_POW_ASSIGN
                                                 : D_AWK_TOK_CARET;
                break;

            case '=':
                _out_token->kind = (next == '=') ? D_AWK_TOK_EQ
                                                 : D_AWK_TOK_ASSIGN;
                break;

            case '!':
                _out_token->kind = (next == '=') ? D_AWK_TOK_NE
                                 : ((next == '~') ? D_AWK_TOK_NOMATCH
                                                  : D_AWK_TOK_NOT);
                break;

            case '<':
                _out_token->kind = (next == '=') ? D_AWK_TOK_LE
                                                 : D_AWK_TOK_LT;
                break;

            case '>':
                _out_token->kind = (next == '=') ? D_AWK_TOK_GE
                                 : ((next == '>') ? D_AWK_TOK_APPEND
                                                  : D_AWK_TOK_GT);
                break;

            case '&':
                _out_token->kind = (next == '&') ? D_AWK_TOK_AND
                                                 : D_AWK_TOK_EOF;
                break;

            case '|':
                _out_token->kind = (next == '|') ? D_AWK_TOK_OR
                                                 : D_AWK_TOK_PIPE;
                break;

            default:
                _out_token->kind = D_AWK_TOK_EOF;
                break;
        }

        // a lone ampersand is not an operator in awk
        if ((byte == '&') && (next != '&'))
        {
            d_internal_fail(_lexer, "unexpected '&'");
            return false;
        }

        // an unrecognised byte ends the scan
        if (_out_token->kind == D_AWK_TOK_EOF)
        {
            char message[64];

            snprintf(message, sizeof(message), "unexpected byte '%c'", byte);
            d_internal_fail(_lexer, message);

            return false;
        }

        // consume the second byte of a two-character operator
        switch (_out_token->kind)
        {
            case D_AWK_TOK_INCR:
            case D_AWK_TOK_DECR:
            case D_AWK_TOK_ADD_ASSIGN:
            case D_AWK_TOK_SUB_ASSIGN:
            case D_AWK_TOK_MUL_ASSIGN:
            case D_AWK_TOK_DIV_ASSIGN:
            case D_AWK_TOK_MOD_ASSIGN:
            case D_AWK_TOK_POW_ASSIGN:
            case D_AWK_TOK_EQ:
            case D_AWK_TOK_NE:
            case D_AWK_TOK_LE:
            case D_AWK_TOK_GE:
            case D_AWK_TOK_AND:
            case D_AWK_TOK_OR:
            case D_AWK_TOK_NOMATCH:
            case D_AWK_TOK_APPEND:
                (void)d_internal_advance(_lexer);
                break;

            default:
                break;
        }
    }

    // abandon the scan when a sub-scanner reported a diagnostic
    if (!ok)
    {
        return false;
    }

    // record whether this token can end an expression, for the slash rule
    switch (_out_token->kind)
    {
        case D_AWK_TOK_NUMBER:
        case D_AWK_TOK_STRING:
        case D_AWK_TOK_ERE:
        case D_AWK_TOK_NAME:
        case D_AWK_TOK_RPAREN:
        case D_AWK_TOK_RBRACKET:
        case D_AWK_TOK_INCR:
        case D_AWK_TOK_DECR:
        case D_AWK_TOK_BUILTIN:
            _lexer->operand_ends = true;
            break;

        default:
            _lexer->operand_ends = false;
            break;
    }

    // record whether a newline may follow, for the suppression rule above
    switch (_out_token->kind)
    {
        case D_AWK_TOK_LBRACE:
        case D_AWK_TOK_COMMA:
        case D_AWK_TOK_AND:
        case D_AWK_TOK_OR:
        case D_AWK_TOK_DO:
        case D_AWK_TOK_ELSE:
        case D_AWK_TOK_SEMI:
        case D_AWK_TOK_NEWLINE:
            _lexer->suppress_nl = true;
            break;

        default:
            _lexer->suppress_nl = false;
            break;
    }

    return true;
}


/*
d_awk_lexer_error
  Returns the first diagnostic recorded, if any.

Parameter(s):
  _lexer: the scanner.
Return:
  The diagnostic, or NULL when none was recorded.
*/
const char*
d_awk_lexer_error(
    const struct d_awk_lexer* _lexer
)
{
    // parameter validation first
    if ((!_lexer) || (_lexer->error[0] == '\0'))
    {
        return NULL;
    }

    return _lexer->error;
}


/*
d_awk_lexer_line
  Returns the current line number.

Parameter(s):
  _lexer: the scanner.
Return:
  The line number, counting from one.
*/
size_t
d_awk_lexer_line(
    const struct d_awk_lexer* _lexer
)
{
    // the position of the diagnostic, when there is one, not the cursor
    if ((_lexer) && (_lexer->error[0] != '\0'))
    {
        return _lexer->error_line;
    }

    return _lexer ? _lexer->line : 0;
}


/*
d_awk_lexer_column
  Returns the current column number.

Parameter(s):
  _lexer: the scanner.
Return:
  The column number, counting from one.
*/
size_t
d_awk_lexer_column(
    const struct d_awk_lexer* _lexer
)
{
    // the position of the diagnostic, when there is one, not the cursor
    if ((_lexer) && (_lexer->error[0] != '\0'))
    {
        return _lexer->error_column;
    }

    return _lexer ? _lexer->column : 0;
}


/*
d_awk_token_name
  Returns a stable name for a token kind, for diagnostics.

Parameter(s):
  _kind: the kind to name.
Return:
  The name, owned by the library.
*/
const char*
d_awk_token_name(
    enum d_awk_token_kind _kind
)
{
    static const char* const names[] =
    {
        "end of input", "newline", "number", "string", "regexp", "name",
        "function name", "builtin", "BEGIN", "END", "function", "if", "else",
        "while", "for", "do", "break", "continue", "next", "nextfile", "exit",
        "return", "delete", "in", "print", "printf", "getline", "{", "}",
        "(", ")", "[", "]", ";", ",", "?", ":", "$", "=", "+=", "-=", "*=",
        "/=", "%=", "^=", "||", "&&", "!", "~", "!~", "==", "!=", "<", "<=",
        ">", ">=", "+", "-", "*", "/", "%", "^", "++", "--", ">>", "|"
    };

    const size_t index = (size_t)_kind;

    return (index < (sizeof(names) / sizeof(names[0])))
         ? names[index]
         : "unknown";
}
