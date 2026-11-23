/*******************************************************************************
* djinterp [c][tools]                                                  dtokens.c
*
* A file's tokens as the fixer reads them, and the fixer's two safety checks:
* definitions for dtokens.h.
*   The checks replace tools/same_tokens.py and tools/moved_lines.py, which
* lexed with Clang; they lex with parse's lexer instead, as dcheck's rules
* read, and the Clang cross-check (d_tokens_agree, and the test that runs
* it) keeps the second opinion as a test of the lexer (dawk's decision 45a).
*
*
* path:      /src/djinterp/tools/dawk/dtokens.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dtokens.h"  // corresponding header
// std
#include <ctype.h>   // isalpha, isspace, isupper, islower
#include <stdarg.h>  // va_list
#include <stdlib.h>  // malloc, realloc, free, strtoul
#include <string.h>  // memcmp, memcpy, strcmp, strlen, strncmp, strstr
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
// djinterp
#include "../../../../inc/djinterp/c/modules/parse/lang/c/lex_c.h"      // C
#include "../../../../inc/djinterp/c/modules/parse/lang/cpp/lex_cpp.h"  // C++
#include "../../../../inc/djinterp/c/modules/parse/lex/lex_scan.h"     // lexer
#include "../../../../inc/djinterp/tools/dawk/ext/check/dext_cpp.h"     // C++?


//==============================================================================
// 1.  READING
//==============================================================================

/*
d_internal_tokens_slurp
  The whole file, or NULL.
*/
static
char* d_internal_tokens_slurp(
    const char* _path,
    size_t*     _length
)
{
    FILE* const in = fopen(_path, "rb");

    if (!in)
    {
        return NULL;
    }

    size_t used     = 0u;
    size_t capacity = 4096u;
    char*  text     = malloc(capacity);

    while (text)
    {
        if (used == capacity)
        {
            char* const grown = realloc(text, capacity * 2u);

            if (!grown)
            {
                free(text);
                text = NULL;

                break;
            }

            text      = grown;
            capacity *= 2u;
        }

        const size_t got = fread(text + used, 1u, capacity - used, in);

        used += got;

        if (got == 0u)
        {
            break;
        }
    }

    (void)fclose(in);
    *_length = used;

    return text;
}

/*
d_tokens_read
  The fixer's extensions lex with keywords resolved and comments kept, at the
newest level of the language; so does this.
*/
bool
d_tokens_read(
    struct dj_tokens_file* _file,
    const char*            _path,
    FILE*                  _report
)
{
    memset(_file, 0, sizeof(*_file));
    _file->path = _path;
    _file->cpp  = d_ext_cpp_accepts(_path);
    _file->text = d_internal_tokens_slurp(_path, &_file->length);

    if (!_file->text)
    {
        if (_report)
        {
            (void)fprintf(_report, "%s: cannot read\n", _path);
        }

        return false;
    }

    d_source_init(&_file->source, _file->text, _file->length, _path);
    (void)d_source_skip_bom(&_file->source);

    const struct dj_lex_dialect* const dialect =
        (_file->cpp) ? d_lex_cpp_dialect(d_lex_cpp_default_level())
                     : d_lex_c_dialect(d_lex_c_default_level());
    struct dj_lexer* const lexer =
        d_lex_create(&_file->source,
                     dialect,
                     D_LEX_OPT_KEYWORDS | D_LEX_OPT_EMIT_COMMENTS);
    size_t capacity = 0u;
    bool   ok       = (lexer != NULL);

    while (ok)
    {
        if (_file->count == capacity)
        {
            const size_t           more  = (capacity) ? capacity * 2u : 1024u;
            struct dj_token* const grown =
                realloc(_file->tokens, more * sizeof(struct dj_token));

            ok = (grown != NULL);

            if (!ok)
            {
                break;
            }

            _file->tokens = grown;
            capacity      = more;
        }

        if (!d_lex_next(lexer, &_file->tokens[_file->count]))
        {
            break;
        }

        ++_file->count;
    }

    d_lex_destroy(lexer);

    if ( (!ok) && (_report) )
    {
        (void)fprintf(_report, "%s: out of memory\n", _path);
    }

    if (!ok)
    {
        d_tokens_free(_file);
    }

    return ok;
}

/*
d_tokens_free
  Everything d_tokens_read made.
*/
void
d_tokens_free(
    struct dj_tokens_file* _file
)
{
    if (!_file)
    {
        return;
    }

    free(_file->text);
    free(_file->tokens);
    memset(_file, 0, sizeof(*_file));

    return;
}


//==============================================================================
// 2.  NAMES
//==============================================================================

// D_INTERNAL_TOKENS_PUNCTUATORS
//   constant: each punctuator's primary spelling, and Clang's name for it.
static const struct
{
    const char* spelling;
    const char* name;
} D_INTERNAL_TOKENS_PUNCTUATORS[] =
{
    { "{",   "l_brace" },             { "}",   "r_brace" },
    { "[",   "l_square" },            { "]",   "r_square" },
    { "(",   "l_paren" },             { ")",   "r_paren" },
    { ";",   "semi" },                { ":",   "colon" },
    { "...", "ellipsis" },            { "?",   "question" },
    { ".",   "period" },              { "->",  "arrow" },
    { "~",   "tilde" },               { "!",   "exclaim" },
    { "+",   "plus" },                { "-",   "minus" },
    { "*",   "star" },                { "/",   "slash" },
    { "%",   "percent" },             { "^",   "caret" },
    { "&",   "amp" },                 { "|",   "pipe" },
    { "=",   "equal" },               { "+=",  "plusequal" },
    { "-=",  "minusequal" },          { "*=",  "starequal" },
    { "/=",  "slashequal" },          { "%=",  "percentequal" },
    { "^=",  "caretequal" },          { "&=",  "ampequal" },
    { "|=",  "pipeequal" },           { "==",  "equalequal" },
    { "!=",  "exclaimequal" },        { "<",   "less" },
    { ">",   "greater" },             { "<=",  "lessequal" },
    { ">=",  "greaterequal" },        { "&&",  "ampamp" },
    { "||",  "pipepipe" },            { "<<",  "lessless" },
    { ">>",  "greatergreater" },      { "<<=", "lesslessequal" },
    { ">>=", "greatergreaterequal" }, { "++",  "plusplus" },
    { "--",  "minusminus" },          { ",",   "comma" },
    { "#",   "hash" },                { "##",  "hashhash" },
    { "::",  "coloncolon" },          { ".*",  "periodstar" },
    { "->*", "arrowstar" },           { "<=>", "spaceship" }
};

/*
d_internal_tokens_literal
  A character or string literal's name, by its encoding prefix: L wide, u8
UTF-8, u UTF-16, U UTF-32; a raw string's R after the prefix changes
nothing.
*/
static
const char* d_internal_tokens_literal(
    const char* _text,
    bool        _string
)
{
    if (_text[0] == 'L')
    {
        return (_string) ? "wide_string_literal" : "wide_char_constant";
    }

    if ( (_text[0] == 'u') &&
         (_text[1] == '8') )
    {
        return (_string) ? "utf8_string_literal" : "utf8_char_constant";
    }

    if (_text[0] == 'u')
    {
        return (_string) ? "utf16_string_literal" : "utf16_char_constant";
    }

    if (_text[0] == 'U')
    {
        return (_string) ? "utf32_string_literal" : "utf32_char_constant";
    }

    return (_string) ? "string_literal" : "char_constant";
}

/*
d_tokens_clang_name
  Classes by their kind, keywords as identifiers (the raw lexer resolves
none), punctuators by their primary spelling: a digraph is the kind it
spells, as Clang's raw lexer has it. C++'s alternative tokens spelled in
letters (`and`, `not_eq`, ...) are identifiers to the raw lexer, which leaves
them to the preprocessor, and so are named here.
*/
const char*
d_tokens_clang_name(
    const struct dj_tokens_file* _file,
    const struct dj_token*       _token
)
{
    const int         kind = _token->kind;
    const char* const text = _file->source.text + _token->span.offset;

    if ( (kind == D_TOKEN_IDENTIFIER) ||
         (d_token_is_keyword(kind)) )
    {
        return "raw_identifier";
    }

    switch (kind)
    {
        case D_TOKEN_NUMBER:
            return "numeric_constant";
        case D_TOKEN_CHARACTER:
            return d_internal_tokens_literal(text, false);
        case D_TOKEN_STRING:
        case D_TOKEN_RAW_STRING:
            return d_internal_tokens_literal(text, true);
        case D_TOKEN_HEADER_NAME:
            return (text[0] == '"') ? "string_literal" : "header_name";
        case D_TOKEN_COMMENT:
            return "comment";
        default:
            break;
    }

    const char* const spelling = d_token_spelling(kind);

    if ( (d_token_is_punctuator(kind)) &&
         (isalpha((unsigned char)text[0])) )
    {
        return "raw_identifier";
    }

    for (size_t at = 0u;
         (spelling) &&
         (at < (sizeof(D_INTERNAL_TOKENS_PUNCTUATORS) /
                sizeof(D_INTERNAL_TOKENS_PUNCTUATORS[0])));
         ++at)
    {
        if (strcmp(D_INTERNAL_TOKENS_PUNCTUATORS[at].spelling, spelling) == 0)
        {
            return D_INTERNAL_TOKENS_PUNCTUATORS[at].name;
        }
    }

    return "unknown";
}

/*
d_internal_tokens_text
  A token's spelling with its line splices taken out, into `_buffer`: what
the compiler reads. Cut short at the buffer's end, which only a spelling
longer than any line could reach.
*/
static
const char* d_internal_tokens_text(
    const struct dj_tokens_file* _file,
    const struct dj_token*       _token,
    char*                        _buffer,
    size_t                       _size
)
{
    const char* const text   = _file->source.text + _token->span.offset;
    const size_t      length = _token->span.length;
    size_t            used   = 0u;

    for (size_t at = 0u; (at < length) && (used + 1u < _size); ++at)
    {
        if (text[at] == '\\')
        {
            size_t next = at + 1u;

            if ( (next < length) && (text[next] == '\r') )
            {
                ++next;
            }

            if ( (next < length) && (text[next] == '\n') )
            {
                at = next;

                continue;
            }
        }

        _buffer[used++] = text[at];
    }

    _buffer[used] = '\0';

    return _buffer;
}

/*
d_internal_tokens_text_is
  Whether a token's spelling, its line splices taken out, is `_text`, of
`_length` bytes: d_internal_tokens_text without a buffer to fill, for a
spelling of any length.
*/
static
bool d_internal_tokens_text_is(
    const struct dj_tokens_file* _file,
    const struct dj_token*       _token,
    const char*                  _text,
    size_t                       _length
)
{
    const char* const text   = _file->source.text + _token->span.offset;
    const size_t      length = _token->span.length;
    size_t            used   = 0u;

    for (size_t at = 0u; at < length; ++at)
    {
        if (text[at] == '\\')
        {
            size_t next = at + 1u;

            if ( (next < length) && (text[next] == '\r') )
            {
                ++next;
            }

            if ( (next < length) && (text[next] == '\n') )
            {
                at = next;

                continue;
            }
        }

        if ( (used == _length) || (_text[used] != text[at]) )
        {
            return false;
        }

        ++used;
    }

    return (used == _length);
}

/*
d_internal_tokens_same_text
  Whether two tokens are spelled the same once their line splices are taken
out, at any length.
*/
static
bool d_internal_tokens_same_text(
    const struct dj_tokens_file* _a_file,
    const struct dj_token*       _a,
    const struct dj_tokens_file* _b_file,
    const struct dj_token*       _b
)
{
    char* const clean = malloc((size_t)_b->span.length + 1u);

    if (!clean)
    {
        return false;
    }

    (void)d_internal_tokens_text(_b_file, _b, clean,
                                 (size_t)_b->span.length + 1u);

    const bool same = d_internal_tokens_text_is(_a_file, _a, clean,
                                                strlen(clean));

    free(clean);

    return same;
}

/*
d_internal_tokens_byte_column
  A token's column counted in bytes, as Clang counts it; the lexer counts
code points, as the style guide measures a line. On the first line, a
byte-order mark the source skipped counts too.
*/
static
uint32_t d_internal_tokens_byte_column(
    const struct dj_tokens_file* _file,
    const struct dj_token*       _token
)
{
    size_t start = _token->span.offset;

    while ( (start > 0u) &&
            (_file->source.text[start - 1u] != '\n') )
    {
        --start;
    }

    const size_t mark = (start == 0u)
                            ? (size_t)(_file->source.text - _file->text)
                            : 0u;

    return (uint32_t)(_token->span.offset - start + mark) + 1u;
}


// dj_internal_columns
//   struct: where the line of the token last asked about begins, so that the
// byte columns of tokens asked about in order cost one pass over the file,
// however long its lines.
struct dj_internal_columns
{
    size_t scanned;     // bytes looked at so far
    size_t line_start;  // where the line holding `scanned` begins
};

/*
d_internal_columns_of
  A token's byte column, as d_internal_tokens_byte_column gives it, for
tokens asked about in the order they stand.
*/
static
uint32_t d_internal_columns_of(
    struct dj_internal_columns*  _columns,
    const struct dj_tokens_file* _file,
    const struct dj_token*       _token
)
{
    const size_t offset = _token->span.offset;

    if (offset < _columns->scanned)
    {
        return d_internal_tokens_byte_column(_file, _token);
    }

    for (; _columns->scanned < offset; ++_columns->scanned)
    {
        if (_file->source.text[_columns->scanned] == '\n')
        {
            _columns->line_start = _columns->scanned + 1u;
        }
    }

    const size_t mark = (_columns->line_start == 0u)
                            ? (size_t)(_file->source.text - _file->text)
                            : 0u;

    return (uint32_t)(offset - _columns->line_start + mark) + 1u;
}


//==============================================================================
// 3.  THE DUMP, AND CLANG'S
//==============================================================================

/*
d_tokens_dump
  One line a token: its name, its spelling and where it stands, line and
column (in bytes, as Clang's dump has it).
*/
int
d_tokens_dump(
    const char* _path,
    FILE*       _out,
    FILE*       _report
)
{
    struct dj_tokens_file file;

    if (!d_tokens_read(&file, _path, _report))
    {
        return 2;
    }

    struct dj_internal_columns columns = { 0u, 0u };

    for (size_t at = 0u; at < file.count; ++at)
    {
        const struct dj_token* const token = &file.tokens[at];

        (void)fprintf(_out, "%s '%.*s'\t%u:%u\n",
                      d_tokens_clang_name(&file, token),
                      (int)token->span.length,
                      file.source.text + token->span.offset,
                      token->span.line,
                      d_internal_columns_of(&columns, &file, token));
    }

    d_tokens_free(&file);

    return 0;
}

// dj_internal_clang_token
//   struct: one record of Clang's dump: its name and spelling, pointing into
// the dump, and where the token stands.
struct dj_internal_clang_token
{
    const char* name;
    size_t      name_length;
    const char* spelling;
    size_t      spelling_length;
    uint32_t    line;
    uint32_t    column;
};

/*
d_internal_clang_blank
  Whether a record is white space, which Clang's dump keeps as `unknown` and
parse's lexer skips: blanks, line ends and the line splices among them.
*/
static
bool d_internal_clang_blank(
    const struct dj_internal_clang_token* _token
)
{
    if ( (_token->name_length != 7u) ||
         (memcmp(_token->name, "unknown", 7u) != 0) )
    {
        return false;
    }

    for (size_t at = 0u; at < _token->spelling_length; ++at)
    {
        const char c = _token->spelling[at];

        if ( (!isspace((unsigned char)c)) && (c != '\\') )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_clang_next
  The record from `*_at`, a token Clang read: `name 'spelling'`, a tab, flags,
then `Loc=<path:line:column>` and the end of the line. A spelling may hold line
ends, quotes and a location's text; it ends at the first quote before a tab,
and the location is the one after it. It is the spelling with its line splices
taken out; the flags then hold the one with them (`[UnClean='...']`). False at
the dump's end.
*/
static
bool d_internal_clang_next(
    const char**                    _at,
    const char*                     _end,
    struct dj_internal_clang_token* _token
)
{
    const char* const start = *_at;
    const char* const space = (start < _end) ? strchr(start, ' ') : NULL;
    const char* const close = (space) ? strstr(space + 2, "'\t") : NULL;
    const char* const loc   = (close) ? strstr(close, "Loc=<") : NULL;

    // the location after the spelling: a spelling may hold `Loc=<` too
    if ( (!loc) || (loc >= _end) )
    {
        return false;
    }

    const char* const finish = strchr(loc, '>');
    const char*       number = finish;

    if (!finish)
    {
        return false;
    }

    // the column after the last colon, the line after the one before it
    while ( (number > loc) && (*(number - 1) != ':') )
    {
        --number;
    }

    _token->column = (uint32_t)strtoul(number, NULL, 10);
    --number;

    while ( (number > loc) && (*(number - 1) != ':') )
    {
        --number;
    }

    _token->line            = (uint32_t)strtoul(number, NULL, 10);
    _token->name            = start;
    _token->name_length     = (size_t)(space - start);
    _token->spelling        = space + 2;
    _token->spelling_length = (size_t)(close - (space + 2));

    const char* const newline = strchr(finish, '\n');

    *_at = (newline) ? newline + 1 : _end;

    return true;
}

/*
d_internal_clang_say
  A disagreement, once: where, what parse's lexer read, what Clang did.
*/
static
void d_internal_clang_say(
    FILE*                                 _report,
    const struct dj_tokens_file*          _file,
    const struct dj_token*                _token,
    const struct dj_internal_clang_token* _clang
)
{
    if (_token)
    {
        (void)fprintf(_report, "%s:%u:%u: parse's lexer reads %s '%.*s'",
                      _file->path,
                      _token->span.line,
                      d_internal_tokens_byte_column(_file, _token),
                      d_tokens_clang_name(_file, _token),
                      (int)_token->span.length,
                      _file->source.text + _token->span.offset);
    }
    else
    {
        (void)fprintf(_report, "%s: parse's lexer reads no more", _file->path);
    }

    if (_clang)
    {
        (void)fprintf(_report, ", and Clang %.*s '%.*s' at %u:%u\n",
                      (int)_clang->name_length, _clang->name,
                      (int)_clang->spelling_length, _clang->spelling,
                      _clang->line, _clang->column);
    }
    else
    {
        (void)fprintf(_report, ", and Clang no more\n");
    }

    return;
}

/*
d_internal_clang_matches
  Whether a token and a record agree: name, spelling, line and column (the
token's, in bytes, `_column`). Clang spells a token without its line
splices, as the compiler reads it, but a comment as it is written.
*/
static
bool d_internal_clang_matches(
    const struct dj_tokens_file*          _file,
    const struct dj_token*                _token,
    uint32_t                              _column,
    const struct dj_internal_clang_token* _clang
)
{
    const char* const name = d_tokens_clang_name(_file, _token);

    return ( (strlen(name) == _clang->name_length)                      &&
             (memcmp(name, _clang->name, _clang->name_length) == 0)     &&
             ( (_token->kind == D_TOKEN_COMMENT)
                 ? ( (_token->span.length == _clang->spelling_length) &&
                     (memcmp(_file->source.text + _token->span.offset,
                             _clang->spelling,
                             _clang->spelling_length) == 0) )
                 : d_internal_tokens_text_is(_file, _token,
                                             _clang->spelling,
                                             _clang->spelling_length) )   &&
             (_token->span.line == _clang->line)                        &&
             (_column == _clang->column) );
}

/*
d_internal_clang_compare
  Walks the two readings together, the blanks of Clang's left out. A header
name in angle brackets is one token to parse's lexer and the tokens it holds
to Clang's raw lexer: those are the records that begin inside it. Stops at the
first disagreement.
*/
static
bool d_internal_clang_compare(
    const struct dj_tokens_file* _file,
    const char*                  _dump,
    size_t                       _size,
    FILE*                        _report
)
{
    const char* const              end     = _dump + _size;
    const char*                    at      = _dump;
    struct dj_internal_clang_token next    = { NULL, 0u, NULL, 0u, 0u, 0u };
    struct dj_internal_columns     columns = { 0u, 0u };
    bool                           more    = false;
    size_t                         i       = 0u;

    #define D_INTERNAL_CLANG_ADVANCE()                                        \
        do                                                                    \
        {                                                                     \
            more = d_internal_clang_next(&at, end, &next);                    \
        } while ( (more) && (d_internal_clang_blank(&next)) )

    D_INTERNAL_CLANG_ADVANCE();

    for (; i < _file->count; ++i)
    {
        const struct dj_token* const token  = &_file->tokens[i];
        const uint32_t               column = d_internal_columns_of(&columns,
                                                                    _file,
                                                                    token);

        if ( (more) &&
             (token->kind == D_TOKEN_HEADER_NAME) &&
             (_file->source.text[token->span.offset] == '<') &&
             (next.line == token->span.line) &&
             (next.column == column) )
        {
            const uint32_t last = next.column + token->span.length;

            while ( (more) &&
                    (next.line == token->span.line) &&
                    (next.column < last) )
            {
                D_INTERNAL_CLANG_ADVANCE();
            }

            continue;
        }

        if ( (!more) ||
             (!d_internal_clang_matches(_file, token, column, &next)) )
        {
            d_internal_clang_say(_report, _file, token, (more) ? &next : NULL);

            return false;
        }

        D_INTERNAL_CLANG_ADVANCE();
    }

    #undef D_INTERNAL_CLANG_ADVANCE

    if (more)
    {
        d_internal_clang_say(_report, _file, NULL, &next);

        return false;
    }

    return true;
}

/*
d_tokens_agree
  Clang's dump is read whole; Clang names its file as it was given, which
needn't be how it is given here, so only the line and column of a location
are read.
*/
int
d_tokens_agree(
    const char* _path,
    const char* _clang_dump,
    FILE*       _report
)
{
    struct dj_tokens_file file;
    size_t                size = 0u;
    char* const           dump = d_internal_tokens_slurp(_clang_dump, &size);

    if (!dump)
    {
        (void)fprintf(_report, "%s: cannot read\n", _clang_dump);

        return 2;
    }

    char* const terminated = realloc(dump, size + 1u);

    if (!terminated)
    {
        free(dump);
        (void)fprintf(_report, "%s: out of memory\n", _clang_dump);

        return 2;
    }

    terminated[size] = '\0';

    if (!d_tokens_read(&file, _path, _report))
    {
        free(terminated);

        return 2;
    }

    const bool agree = d_internal_clang_compare(&file, terminated, size,
                                                _report);

    d_tokens_free(&file);
    free(terminated);

    return (agree) ? 0 : 1;
}


//==============================================================================
// 4.  SAME TOKENS
//==============================================================================

// dj_internal_sig
//   struct: one thing the compiler reads, in order: a token (its index), or
// the end of a directive (`token` the directive's last). `space` is -1 where
// white space before the token means nothing, else whether there is any.
struct dj_internal_sig
{
    size_t token;
    bool   directive_end;
    int    space;
};

// dj_internal_sigs
//   struct: a file's sequence of dj_internal_sig.
struct dj_internal_sigs
{
    struct dj_internal_sig* items;
    size_t                  count;
    size_t                  capacity;
};

/*
d_internal_sigs_push
  Appends one; false when memory runs out.
*/
static
bool d_internal_sigs_push(
    struct dj_internal_sigs* _sigs,
    size_t                   _token,
    bool                     _directive_end,
    int                      _space
)
{
    if (_sigs->count == _sigs->capacity)
    {
        const size_t more = (_sigs->capacity) ? _sigs->capacity * 2u : 256u;
        struct dj_internal_sig* const grown =
            realloc(_sigs->items, more * sizeof(struct dj_internal_sig));

        if (!grown)
        {
            return false;
        }

        _sigs->items    = grown;
        _sigs->capacity = more;
    }

    _sigs->items[_sigs->count].token         = _token;
    _sigs->items[_sigs->count].directive_end = _directive_end;
    _sigs->items[_sigs->count].space         = _space;
    ++_sigs->count;

    return true;
}

/*
d_internal_tokens_is
  Whether a token is the punctuator spelled `_spelling`.
*/
static
bool d_internal_tokens_is(
    const struct dj_token* _token,
    const char*            _spelling
)
{
    const char* const spelling = d_token_spelling(_token->kind);

    return ( (d_token_is_punctuator(_token->kind)) &&
             (spelling) &&
             (strcmp(spelling, _spelling) == 0) );
}

/*
d_internal_tokens_word
  Whether a token is the identifier or keyword spelled `_word`.
*/
static
bool d_internal_tokens_word(
    const struct dj_tokens_file* _file,
    const struct dj_token*       _token,
    const char*                  _word
)
{
    const size_t length = strlen(_word);

    return ( ( (_token->kind == D_TOKEN_IDENTIFIER) ||
               (d_token_is_keyword(_token->kind)) )                   &&
             (_token->span.length == length)                          &&
             (memcmp(_file->source.text + _token->span.offset,
                     _word,
                     length) == 0) );
}

/*
d_internal_tokens_capitals
  Whether an identifier is spelled in capitals, digits and underscores, with
a capital among them: a macro, by the style guide's naming.
*/
static
bool d_internal_tokens_capitals(
    const struct dj_tokens_file* _file,
    const struct dj_token*       _token
)
{
    const char* const text    = _file->source.text + _token->span.offset;
    bool              capital = false;

    if (_token->kind != D_TOKEN_IDENTIFIER)
    {
        return false;
    }

    for (size_t at = 0u; at < _token->span.length; ++at)
    {
        const unsigned char c = (unsigned char)text[at];

        if (islower(c))
        {
            return false;
        }

        capital = (capital) || (isupper(c));
    }

    return capital;
}

/*
d_internal_tokens_code
  The next token from `_at` that isn't a comment, or `_file->count`.
*/
static
size_t d_internal_tokens_code(
    const struct dj_tokens_file* _file,
    size_t                       _at
)
{
    while ( (_at < _file->count) &&
            (_file->tokens[_at].kind == D_TOKEN_COMMENT) )
    {
        ++_at;
    }

    return _at;
}

/*
d_internal_tokens_stringifies
  Whether the `#define` whose name is token `_name` turns a parameter into a
string: a `#` (not `##`) in its replacement list, before an identifier.
*/
static
bool d_internal_tokens_stringifies(
    const struct dj_tokens_file* _file,
    size_t                       _name
)
{
    const uint32_t line = _file->tokens[_name].span.line;

    for (size_t at = _name + 1u; at + 1u < _file->count; ++at)
    {
        const struct dj_token* const token = &_file->tokens[at];

        if ( (!(token->flags & D_TOKEN_FLAG_IN_DIRECTIVE)) ||
             ( (token->flags & D_TOKEN_FLAG_LINE_START) &&
               (token->span.line != line) &&
               (d_internal_tokens_is(token, "#")) ) )
        {
            return false;
        }

        if (d_internal_tokens_is(token, "#"))
        {
            const size_t next = d_internal_tokens_code(_file, at + 1u);

            if ( (next < _file->count) &&
                 (_file->tokens[next].kind == D_TOKEN_IDENTIFIER) )
            {
                return true;
            }
        }
    }

    return false;
}

/*
d_internal_tokens_stringifier
  Whether a call of the identifier at `_callee` may turn its arguments into
strings: `assert`, a macro in capitals, or one this file defines that does.
*/
static
bool d_internal_tokens_stringifier(
    const struct dj_tokens_file* _file,
    size_t                       _callee
)
{
    const struct dj_token* const callee = &_file->tokens[_callee];

    if ( (d_internal_tokens_word(_file, callee, "assert")) ||
         (d_internal_tokens_capitals(_file, callee)) )
    {
        return true;
    }

    for (size_t at = 0u; at < _file->count; ++at)
    {
        const struct dj_token* const token = &_file->tokens[at];

        if ( (token->flags & D_TOKEN_FLAG_MACRO_NAME) &&
             (token->span.length == callee->span.length) &&
             (memcmp(_file->source.text + token->span.offset,
                     _file->source.text + callee->span.offset,
                     callee->span.length) == 0) &&
             (d_internal_tokens_stringifies(_file, at)) )
        {
            return true;
        }
    }

    return false;
}

// dj_internal_space_state
//   struct: what d_internal_tokens_space needs to remember between tokens:
// the directive it is in and where in a `#define` it stands, and the depth
// of the arguments of a call that may stringify.
struct dj_internal_space_state
{
    int    define_step;   // 0 none, 1 name next, 2 after the name, 3 body
    int    define_depth;  // parameters' parentheses still open
    bool   body_first;    // the replacement list's first token is next
    int    call_depth;    // stringifying call's parentheses still open
    size_t previous;      // the previous code token
};

/*
d_internal_tokens_space_define
  Inside a `#define`: the `(` right after the name, whose white space makes
the macro object-like, and the replacement list, between whose tokens white
space is part of the definition.
*/
static
int d_internal_tokens_space_define(
    const struct dj_tokens_file*    _file,
    size_t                          _at,
    struct dj_internal_space_state* _state
)
{
    const struct dj_token* const token = &_file->tokens[_at];
    const bool spaced = ((token->flags & D_TOKEN_FLAG_LEADING_SPACE) != 0u);

    if (_state->define_step == 1)
    {
        _state->define_step = 2;

        return -1;
    }

    if (_state->define_step == 2)
    {
        _state->define_step = 3;
        _state->body_first  = true;

        if ( (!spaced) && (d_internal_tokens_is(token, "(")) )
        {
            _state->define_depth = 1;
            _state->body_first   = false;

            return 0;
        }

        _state->body_first = false;

        return (spaced) ? 1 : 0;
    }

    if (_state->define_depth > 0)
    {
        _state->define_depth -= (d_internal_tokens_is(token, ")")) ? 1 : 0;
        _state->body_first    = (_state->define_depth == 0);

        return -1;
    }

    if (_state->body_first)
    {
        _state->body_first = false;

        return -1;
    }

    return (spaced) ? 1 : 0;
}

/*
d_internal_tokens_space_call
  Inside the arguments of a call that may stringify: white space between two
tokens of one argument becomes a space in the string; before an argument's
first token, around a comma between arguments and before the closing `)`,
it is dropped.
*/
static
int d_internal_tokens_space_call(
    const struct dj_tokens_file*    _file,
    size_t                          _at,
    struct dj_internal_space_state* _state
)
{
    const struct dj_token* const token    = &_file->tokens[_at];
    const struct dj_token* const previous = &_file->tokens[_state->previous];
    const bool spaced = ((token->flags & D_TOKEN_FLAG_LEADING_SPACE) != 0u);
    const bool outer  = (_state->call_depth == 1);
    int        space  = (spaced) ? 1 : 0;

    if ( (outer) &&
         ( (d_internal_tokens_is(previous, "(")) ||
           (d_internal_tokens_is(previous, ",")) ||
           (d_internal_tokens_is(token, ",")) ||
           (d_internal_tokens_is(token, ")")) ) )
    {
        space = -1;
    }

    if (d_internal_tokens_is(token, "("))
    {
        ++_state->call_depth;
    }
    else if (d_internal_tokens_is(token, ")"))
    {
        --_state->call_depth;
    }

    return space;
}

/*
d_internal_tokens_space
  Whether white space before a code token means anything, and if so whether
there is any: -1, 0 or 1.
*/
static
int d_internal_tokens_space(
    const struct dj_tokens_file*    _file,
    size_t                          _at,
    struct dj_internal_space_state* _state
)
{
    const struct dj_token* const token = &_file->tokens[_at];
    int                          space = -1;

    if (token->flags & D_TOKEN_FLAG_IN_DIRECTIVE)
    {
        if (_state->define_step > 0)
        {
            space = d_internal_tokens_space_define(_file, _at, _state);
        }
        else if ( (token->flags & D_TOKEN_FLAG_DIRECTIVE_NAME) &&
                  (d_internal_tokens_word(_file, token, "define")) )
        {
            _state->define_step = 1;
        }
    }
    else if (_state->call_depth > 0)
    {
        space = d_internal_tokens_space_call(_file, _at, _state);
    }
    else if ( (d_internal_tokens_is(token, "(")) &&
              (_at > 0u) &&
              (_file->tokens[_state->previous].kind == D_TOKEN_IDENTIFIER) &&
              (d_internal_tokens_stringifier(_file, _state->previous)) )
    {
        _state->call_depth = 1;
    }

    _state->previous = _at;

    return space;
}

/*
d_internal_tokens_sigs
  A file as the compiler reads it: its code tokens in order, the end of each
directive after its last, and white space where it means something.
*/
static
bool d_internal_tokens_sigs(
    const struct dj_tokens_file* _file,
    struct dj_internal_sigs*     _sigs
)
{
    struct dj_internal_space_state state;
    bool                           inside = false;
    size_t                         last   = 0u;

    memset(&state, 0, sizeof(state));

    for (size_t at = 0u; at < _file->count; ++at)
    {
        const struct dj_token* const token = &_file->tokens[at];
        const bool directive =
            ((token->flags & D_TOKEN_FLAG_IN_DIRECTIVE) != 0u);
        const bool opens     = ( (directive) &&
                                 (token->flags & D_TOKEN_FLAG_LINE_START) &&
                                 (d_internal_tokens_is(token, "#")) );

        if ( (token->kind == D_TOKEN_COMMENT) ||
             (token->kind == D_TOKEN_END) )
        {
            continue;
        }

        if ( (inside) && ( (!directive) || (opens) ) &&
             (!d_internal_sigs_push(_sigs, last, true, -1)) )
        {
            return false;
        }

        if (opens)
        {
            state.define_step  = 0;
            state.define_depth = 0;
        }

        inside = directive;
        last   = at;

        if (!d_internal_sigs_push(_sigs, at, false,
                                  d_internal_tokens_space(_file, at, &state)))
        {
            return false;
        }
    }

    return ( (!inside) ||
             (d_internal_sigs_push(_sigs, last, true, -1)) );
}

/*
d_internal_tokens_say_sig
  One side of a difference: where, and what stands there.
*/
static
void d_internal_tokens_say_sig(
    FILE*                         _report,
    const struct dj_tokens_file*  _file,
    const struct dj_internal_sig* _sig
)
{
    char buffer[256];

    if (!_sig)
    {
        (void)fprintf(_report, "the end of %s", _file->path);

        return;
    }

    const struct dj_token* const token = &_file->tokens[_sig->token];

    (void)fprintf(_report, "%s:%u: %s%s '%s'%s",
                  _file->path,
                  token->span.line,
                  (_sig->directive_end) ? "the directive's end, after " : "",
                  d_tokens_clang_name(_file, token),
                  d_internal_tokens_text(_file, token, buffer,
                                         sizeof(buffer)),
                  (_sig->space == 1) ? ", with white space before it"
                  : (_sig->space == 0) ? ", with none before it" : "");

    return;
}

/*
d_internal_tokens_sig_equal
  Whether two entries say the same to the compiler.
*/
static
bool d_internal_tokens_sig_equal(
    const struct dj_tokens_file*  _before,
    const struct dj_internal_sig* _a,
    const struct dj_tokens_file*  _after,
    const struct dj_internal_sig* _b
)
{
    if ( (_a->directive_end != _b->directive_end) ||
         (_a->space != _b->space) )
    {
        return false;
    }

    if (_a->directive_end)
    {
        return true;
    }

    const struct dj_token* const x = &_before->tokens[_a->token];
    const struct dj_token* const y = &_after->tokens[_b->token];

    return ( (strcmp(d_tokens_clang_name(_before, x),
                     d_tokens_clang_name(_after, y)) == 0) &&
             (d_internal_tokens_same_text(_before, x, _after, y)) );
}

/*
d_internal_tokens_same_files
  Compares two lexed files; says the first difference.
*/
static
bool d_internal_tokens_same_files(
    const struct dj_tokens_file* _before,
    const struct dj_tokens_file* _after,
    FILE*                        _report
)
{
    struct dj_internal_sigs a = { NULL, 0u, 0u };
    struct dj_internal_sigs b = { NULL, 0u, 0u };
    bool                    same = ( (d_internal_tokens_sigs(_before, &a)) &&
                                     (d_internal_tokens_sigs(_after, &b)) );
    size_t                  at   = 0u;

    if (!same)
    {
        (void)fprintf(_report, "%s: out of memory\n", _after->path);
    }

    for (; (same) && (at < a.count) && (at < b.count); ++at)
    {
        same = d_internal_tokens_sig_equal(_before, &a.items[at],
                                           _after, &b.items[at]);
    }

    if ( (same) && (a.count != b.count) )
    {
        same = false;
        ++at;
    }

    if (!same && (a.items) && (b.items))
    {
        --at;
        d_internal_tokens_say_sig(_report, _after,
                                  (at < b.count) ? &b.items[at] : NULL);
        (void)fprintf(_report, " differs from ");
        d_internal_tokens_say_sig(_report, _before,
                                  (at < a.count) ? &a.items[at] : NULL);
        (void)fprintf(_report, "\n");
    }

    free(a.items);
    free(b.items);

    return same;
}

/*
d_tokens_same
  Reads both files and compares them.
*/
int
d_tokens_same(
    const char* _before,
    const char* _after,
    FILE*       _report
)
{
    struct dj_tokens_file before;
    struct dj_tokens_file after;

    if (!d_tokens_read(&before, _before, _report))
    {
        return 2;
    }

    if (!d_tokens_read(&after, _after, _report))
    {
        d_tokens_free(&before);

        return 2;
    }

    const bool same = d_internal_tokens_same_files(&before, &after, _report);

    d_tokens_free(&before);
    d_tokens_free(&after);

    return (same) ? 0 : 1;
}


//==============================================================================
// 5.  MOVED LINES
//==============================================================================

// D_INTERNAL_MOVED_NONE
//   constant: no token.
#define D_INTERNAL_MOVED_NONE ((size_t)-1)

// dj_internal_side
//   struct: one file's code tokens (comments left out): each one's index
// among all the file's tokens, and per token, whether it begins its line and
// which token does.
struct dj_internal_side
{
    const struct dj_tokens_file* file;
    size_t*                      code;   // code index -> token index
    size_t*                      of;     // token index -> code index
    size_t*                      line;   // token index -> its line's first
    bool*                        first;  // token index -> begins its line
};

// dj_internal_moved
//   struct: the two sides, and the structure of their code, which is one
// structure, since the two hold the same code tokens: per code index, the
// matching opener of a closer, the innermost open bracket, the statement
// (labels ending one) and the head (labels not ending one) it belongs to,
// and whether it begins a label or ends one.
struct dj_internal_moved
{
    struct dj_internal_side before;
    struct dj_internal_side after;
    size_t                  count;
    size_t*                 opener;
    size_t*                 frame;
    size_t*                 stmt;
    size_t*                 head;
    bool*                   label;
    bool*                   label_end;
    FILE*                   report;
    bool                    clean;
};

/*
d_internal_side_build
  Indexes a file's code tokens, and where its lines begin: a token begins
its line when no earlier token reaches that line.
*/
static
bool d_internal_side_build(
    struct dj_internal_side*     _side,
    const struct dj_tokens_file* _file
)
{
    const size_t n = (_file->count) ? _file->count : 1u;

    _side->file  = _file;
    _side->code  = calloc(n, sizeof(size_t));
    _side->of    = calloc(n, sizeof(size_t));
    _side->line  = calloc(n, sizeof(size_t));
    _side->first = calloc(n, sizeof(bool));

    if ( (!_side->code) || (!_side->of) || (!_side->line) || (!_side->first) )
    {
        return false;
    }

    size_t   count   = 0u;
    size_t   current = 0u;
    uint32_t reached = 0u;

    for (size_t at = 0u; at < _file->count; ++at)
    {
        const struct dj_token* const token = &_file->tokens[at];

        _side->first[at] = (token->span.line > reached);
        current          = (_side->first[at]) ? at : current;
        _side->line[at]  = current;
        _side->of[at]    = D_INTERNAL_MOVED_NONE;
        reached          = (token->end_line > reached) ? token->end_line
                                                       : reached;

        if (token->kind != D_TOKEN_COMMENT)
        {
            _side->of[at]         = count;
            _side->code[count++] = at;
        }
    }

    return true;
}

/*
d_internal_side_free
  What d_internal_side_build made.
*/
static
void d_internal_side_free(
    struct dj_internal_side* _side
)
{
    free(_side->code);
    free(_side->of);
    free(_side->line);
    free(_side->first);

    return;
}

/*
d_internal_moved_token
  Code token `_c` of one side.
*/
static
const struct dj_token* d_internal_moved_token(
    const struct dj_internal_side* _side,
    size_t                         _c
)
{
    return &_side->file->tokens[_side->code[_c]];
}

/*
d_internal_moved_directive
  Whether code token `_c` stands in a directive, which the checks leave
alone.
*/
static
bool d_internal_moved_directive(
    const struct dj_internal_moved* _moved,
    size_t                          _c
)
{
    return ((d_internal_moved_token(&_moved->before, _c)->flags &
             D_TOKEN_FLAG_IN_DIRECTIVE) != 0u);
}

// dj_internal_frame
//   struct: one open bracket while the structure is read: its opener (none
// for the file), its kind, and the statement and head being read in it.
struct dj_internal_frame
{
    size_t opener;
    char   kind;
    size_t stmt;
    size_t head;
    bool   label;
};

/*
d_internal_moved_closes
  The bracket a closing punctuator closes, or 0.
*/
static
char d_internal_moved_closes(
    const struct dj_token* _token
)
{
    return (d_internal_tokens_is(_token, ")")) ? '('
         : (d_internal_tokens_is(_token, "]")) ? '['
         : (d_internal_tokens_is(_token, "}")) ? '{'
                                                : '\0';
}

/*
d_internal_moved_opens
  The bracket an opening punctuator opens, or 0.
*/
static
char d_internal_moved_opens(
    const struct dj_token* _token
)
{
    return (d_internal_tokens_is(_token, "(")) ? '('
         : (d_internal_tokens_is(_token, "[")) ? '['
         : (d_internal_tokens_is(_token, "{")) ? '{'
                                                : '\0';
}

/*
d_internal_moved_label_starts
  Whether a token beginning a statement begins a label: `case`, `default`
before its colon, or a name right before a lone colon (a `goto` label, or
`public:` and its kind).
*/
static
bool d_internal_moved_label_starts(
    const struct dj_internal_moved* _moved,
    size_t                          _c
)
{
    const struct dj_tokens_file* const file  = _moved->before.file;
    const struct dj_token* const       token =
        d_internal_moved_token(&_moved->before, _c);
    const struct dj_token* const       next  =
        (_c + 1u < _moved->count)
            ? d_internal_moved_token(&_moved->before, _c + 1u)
            : NULL;

    if (d_internal_tokens_word(file, token, "case"))
    {
        return true;
    }

    return ( (next) &&
             (d_internal_tokens_is(next, ":")) &&
             ( (token->kind == D_TOKEN_IDENTIFIER) ||
               (d_token_is_keyword(token->kind)) ) );
}

/*
d_internal_moved_close
  A closing bracket: it closes the innermost frame if that is its kind, and
a `}` ends the statement it belonged to in the frame outside.
*/
static
void d_internal_moved_close(
    struct dj_internal_moved* _moved,
    struct dj_internal_frame* _stack,
    size_t*                   _depth,
    size_t                    _c,
    char                      _kind
)
{
    if ( (*_depth > 1u) && (_stack[*_depth - 1u].kind == _kind) )
    {
        _moved->opener[_c] = _stack[*_depth - 1u].opener;
        --*_depth;
    }

    struct dj_internal_frame* const top = &_stack[*_depth - 1u];

    _moved->frame[_c] = top->opener;

    if ( (top->kind == '{') || (top->kind == 'f') )
    {
        _moved->stmt[_c] = (_kind == '{') ? D_INTERNAL_MOVED_NONE : top->stmt;
        _moved->head[_c] = (_kind == '{') ? D_INTERNAL_MOVED_NONE : top->head;

        if (_kind == '{')
        {
            top->stmt  = D_INTERNAL_MOVED_NONE;
            top->head  = D_INTERNAL_MOVED_NONE;
            top->label = false;
        }
    }

    return;
}

/*
d_internal_moved_step
  One code token outside a directive, read into the structure: the frame it
stands in, and in a block or the file, the statement and head it belongs
to; `;` ends both, a label's colon ends the statement.
*/
static
bool d_internal_moved_step(
    struct dj_internal_moved* _moved,
    struct dj_internal_frame* _stack,
    size_t*                   _depth,
    size_t                    _c
)
{
    const struct dj_token* const    token = d_internal_moved_token(
                                                &_moved->before, _c);
    struct dj_internal_frame* const top   = &_stack[*_depth - 1u];
    const char                      opens = d_internal_moved_opens(token);

    _moved->frame[_c] = top->opener;

    if ( (top->kind == '{') || (top->kind == 'f') )
    {
        if (top->stmt == D_INTERNAL_MOVED_NONE)
        {
            top->stmt          = _c;
            top->label         = d_internal_moved_label_starts(_moved, _c);
            _moved->label[_c]  = top->label;
        }

        top->head        = (top->head == D_INTERNAL_MOVED_NONE) ? _c
                                                                 : top->head;
        _moved->stmt[_c] = top->stmt;
        _moved->head[_c] = top->head;

        if (d_internal_tokens_is(token, ";"))
        {
            top->stmt  = D_INTERNAL_MOVED_NONE;
            top->head  = D_INTERNAL_MOVED_NONE;
            top->label = false;
        }
        else if ( (top->label) && (d_internal_tokens_is(token, ":")) )
        {
            _moved->label_end[_c] = true;
            top->stmt             = D_INTERNAL_MOVED_NONE;
            top->label            = false;
        }
    }

    if (opens)
    {
        if (*_depth == _moved->count + 1u)
        {
            return false;
        }

        _stack[*_depth].opener = _c;
        _stack[*_depth].kind   = opens;
        _stack[*_depth].stmt   = D_INTERNAL_MOVED_NONE;
        _stack[*_depth].head   = D_INTERNAL_MOVED_NONE;
        _stack[*_depth].label  = false;
        ++*_depth;
    }

    return true;
}

/*
d_internal_moved_structure
  Reads the code outside directives, bracket by bracket.
*/
static
bool d_internal_moved_structure(
    struct dj_internal_moved* _moved
)
{
    const size_t n = _moved->count + 1u;

    _moved->opener    = malloc(n * sizeof(size_t));
    _moved->frame     = malloc(n * sizeof(size_t));
    _moved->stmt      = malloc(n * sizeof(size_t));
    _moved->head      = malloc(n * sizeof(size_t));
    _moved->label     = calloc(n, sizeof(bool));
    _moved->label_end = calloc(n, sizeof(bool));

    struct dj_internal_frame* const stack =
        malloc((n + 1u) * sizeof(struct dj_internal_frame));
    size_t depth = 1u;
    bool   ok    = ( (_moved->opener) && (_moved->frame) && (_moved->stmt) &&
                     (_moved->head) && (_moved->label) &&
                     (_moved->label_end) && (stack) );

    for (size_t c = 0u; (ok) && (c < n); ++c)
    {
        _moved->opener[c] = D_INTERNAL_MOVED_NONE;
        _moved->frame[c]  = D_INTERNAL_MOVED_NONE;
        _moved->stmt[c]   = D_INTERNAL_MOVED_NONE;
        _moved->head[c]   = D_INTERNAL_MOVED_NONE;
    }

    if (ok)
    {
        stack[0].opener = D_INTERNAL_MOVED_NONE;
        stack[0].kind   = 'f';
        stack[0].stmt   = D_INTERNAL_MOVED_NONE;
        stack[0].head   = D_INTERNAL_MOVED_NONE;
        stack[0].label  = false;
    }

    for (size_t c = 0u; (ok) && (c < _moved->count); ++c)
    {
        const struct dj_token* const token =
            d_internal_moved_token(&_moved->before, c);
        const char closes = d_internal_moved_closes(token);

        if (d_internal_moved_directive(_moved, c))
        {
            continue;
        }

        if (closes)
        {
            d_internal_moved_close(_moved, stack, &depth, c, closes);
        }
        else
        {
            ok = d_internal_moved_step(_moved, stack, &depth, c);
        }
    }

    free(stack);

    return ok;
}

/*
d_internal_moved_col
  Where code token `_c` stands on one side: its column, in code points.
*/
static
uint32_t d_internal_moved_col(
    const struct dj_internal_side* _side,
    size_t                         _c
)
{
    return d_internal_moved_token(_side, _c)->span.column;
}

/*
d_internal_moved_first
  Whether code token `_c` begins its line on one side.
*/
static
bool d_internal_moved_first(
    const struct dj_internal_side* _side,
    size_t                         _c
)
{
    return _side->first[_side->code[_c]];
}

/*
d_internal_moved_line_col
  The column the first token of code token `_c`'s line stands in, after.
*/
static
uint32_t d_internal_moved_line_col(
    const struct dj_internal_moved* _moved,
    size_t                          _c
)
{
    const struct dj_internal_side* const after = &_moved->after;

    return after->file->tokens[after->line[after->code[_c]]].span.column;
}

/*
d_internal_moved_line_delta
  How far the line code token `_c` was on moved: its first token's move,
when that token is a code token beginning a line on both sides; false
otherwise, when the line has no move of its own to compare with.
*/
static
bool d_internal_moved_line_delta(
    const struct dj_internal_moved* _moved,
    size_t                          _c,
    int*                            _delta,
    uint32_t*                       _line
)
{
    const struct dj_internal_side* const before = &_moved->before;
    const size_t first = before->line[before->code[_c]];
    const size_t h     = before->of[first];

    if ( (h == D_INTERNAL_MOVED_NONE) ||
         (!d_internal_moved_first(&_moved->after, h)) )
    {
        return false;
    }

    *_delta = (int)d_internal_moved_col(&_moved->after, h) -
              (int)d_internal_moved_col(before, h);
    *_line  = d_internal_moved_token(&_moved->after, h)->span.line;

    return true;
}

/*
d_internal_moved_say
  One finding, as `after:line: what`.
*/
static
void d_internal_moved_say(
    struct dj_internal_moved* _moved,
    uint32_t                  _line,
    const char*               _format,
    ...
)
{
    va_list args;

    va_start(args, _format);
    (void)fprintf(_moved->report, "%s:%u: ", _moved->after.file->path, _line);
    (void)vfprintf(_moved->report, _format, args);
    (void)fprintf(_moved->report, "\n");
    va_end(args);
    _moved->clean = false;

    return;
}

// dj_internal_expect
//   struct: where the layout puts a token given a line of its own, and the
// words that say what it stands under.
struct dj_internal_expect
{
    uint32_t    column;
    const char* under;
};

/*
d_internal_moved_previous
  The code token before `_c` outside directives, or none.
*/
static
size_t d_internal_moved_previous(
    const struct dj_internal_moved* _moved,
    size_t                          _c
)
{
    while (_c > 0u)
    {
        --_c;

        if (!d_internal_moved_directive(_moved, _c))
        {
            return _c;
        }
    }

    return D_INTERNAL_MOVED_NONE;
}

/*
d_internal_moved_expect_bracket
  A token given a line inside a `(` or `[`: a level in from the line the
bracket opens on, when its first token was given a line too; otherwise under
that first token, beside the bracket.
*/
static
void d_internal_moved_expect_bracket(
    const struct dj_internal_moved* _moved,
    size_t                          _opener,
    struct dj_internal_expect*      _expect
)
{
    const size_t first = _opener + 1u;
    const bool   round = d_internal_tokens_is(
                             d_internal_moved_token(&_moved->before, _opener),
                             "(");

    if (d_internal_moved_first(&_moved->after, first))
    {
        _expect->column = d_internal_moved_line_col(_moved, _opener) + 4u;
        _expect->under  = "";

        return;
    }

    _expect->column = d_internal_moved_col(&_moved->after, first);
    _expect->under  = (round) ? " under the first token after its ("
                              : " under the first token after its [";

    return;
}

/*
d_internal_moved_expect
  Where the layout puts code token `_c` on a line of its own, after: false
where nothing says. A closing bracket under the line its opener is on; a `{`
under the line its head begins on; `else`, `while` or `catch` under the `}`
before it; inside a `(` or `[`, as d_internal_moved_expect_bracket says; a
statement a level in from its block's line, or from its label's.
*/
static
bool d_internal_moved_expect(
    const struct dj_internal_moved* _moved,
    size_t                          _c,
    struct dj_internal_expect*      _expect
)
{
    const struct dj_tokens_file* const file  = _moved->after.file;
    const struct dj_token* const       token =
        d_internal_moved_token(&_moved->after, _c);
    const size_t                       frame = _moved->frame[_c];
    const size_t                       prev  =
        d_internal_moved_previous(_moved, _c);
    const struct dj_token* const       before =
        (prev != D_INTERNAL_MOVED_NONE)
            ? d_internal_moved_token(&_moved->after, prev)
            : NULL;
    const char                         kind  =
        (frame == D_INTERNAL_MOVED_NONE)
            ? 'f'
            : d_internal_moved_opens(d_internal_moved_token(&_moved->after,
                                                            frame));

    if ( (d_internal_moved_closes(token)) &&
         (_moved->opener[_c] != D_INTERNAL_MOVED_NONE) )
    {
        _expect->column = d_internal_moved_line_col(_moved,
                                                    _moved->opener[_c]);
        _expect->under  = " under the line its opener is on";

        return true;
    }

    if ( (d_internal_tokens_is(token, "{")) && (kind != '(') &&
         (kind != '[') && (_moved->head[_c] != _c) &&
         (_moved->head[_c] != D_INTERNAL_MOVED_NONE) )
    {
        _expect->column = d_internal_moved_line_col(_moved, _moved->head[_c]);
        _expect->under  = " under the line its head begins on";

        return true;
    }

    if ( (before) && (d_internal_tokens_is(before, "}")) &&
         ( (d_internal_tokens_word(file, token, "else")) ||
           (d_internal_tokens_word(file, token, "while")) ||
           (d_internal_tokens_word(file, token, "catch")) ) )
    {
        _expect->column = d_internal_moved_col(&_moved->after, prev);
        _expect->under  = " under the } before it";

        return true;
    }

    if ( (kind == '(') || (kind == '[') )
    {
        d_internal_moved_expect_bracket(_moved, frame, _expect);

        return true;
    }

    if (_moved->stmt[_c] != _c)
    {
        return false;
    }

    _expect->under  = "";
    _expect->column =
        ( (prev != D_INTERNAL_MOVED_NONE) && (_moved->label_end[prev]) )
            ? d_internal_moved_line_col(_moved, _moved->stmt[prev]) + 4u
        : (kind == 'f') ? 1u
                        : d_internal_moved_line_col(_moved, frame) + 4u;

    return true;
}

/*
d_internal_moved_given
  A token given a line of its own: it stands where the layout puts it.
*/
static
void d_internal_moved_given(
    struct dj_internal_moved* _moved,
    size_t                    _c
)
{
    const struct dj_token* const token =
        d_internal_moved_token(&_moved->after, _c);
    struct dj_internal_expect    expect = { 0u, "" };

    if ( (d_internal_moved_expect(_moved, _c, &expect)) &&
         (token->span.column != expect.column) )
    {
        d_internal_moved_say(_moved, token->span.line,
                             "%s '%.*s' was given a line and stands in "
                             "column %u, not in %u%s",
                             d_tokens_clang_name(_moved->after.file, token),
                             (int)token->span.length,
                             _moved->after.file->source.text +
                                 token->span.offset,
                             token->span.column,
                             expect.column,
                             expect.under);
    }

    return;
}

/*
d_internal_moved_follow
  A token that kept its line and must move as another line did: `_anchor`'s,
named by `_what`. Where the layout puts it is as good: a repair may bring a
token to its place.
*/
static
void d_internal_moved_follow(
    struct dj_internal_moved* _moved,
    size_t                    _c,
    size_t                    _anchor,
    const char*               _what
)
{
    const struct dj_token* const token =
        d_internal_moved_token(&_moved->after, _c);
    const int moved = (int)token->span.column -
                      (int)d_internal_moved_col(&_moved->before, _c);
    struct dj_internal_expect expect = { 0u, "" };
    int                       delta  = 0;
    uint32_t                  line   = 0u;

    if ( (!d_internal_moved_line_delta(_moved, _anchor, &delta, &line)) ||
         (moved == delta) ||
         ( (d_internal_moved_expect(_moved, _c, &expect)) &&
           (token->span.column == expect.column) ) )
    {
        return;
    }

    if (strcmp(_what, "statement") == 0)
    {
        d_internal_moved_say(_moved, token->span.line,
                             "%s '%.*s' moved %+d, and the statement it "
                             "continues, on line %u, moved %+d",
                             d_tokens_clang_name(_moved->after.file, token),
                             (int)token->span.length,
                             _moved->after.file->source.text +
                                 token->span.offset,
                             moved, line, delta);

        return;
    }

    d_internal_moved_say(_moved, token->span.line,
                         "%s '%.*s' moved %+d, and %s moved %+d",
                         d_tokens_clang_name(_moved->after.file, token),
                         (int)token->span.length,
                         _moved->after.file->source.text + token->span.offset,
                         moved, _what, delta);

    return;
}

/*
d_internal_moved_earlier
  Whether code token `_a` stood on an earlier line than `_b`, before.
*/
static
bool d_internal_moved_earlier(
    const struct dj_internal_moved* _moved,
    size_t                          _a,
    size_t                          _b
)
{
    return ( (_a != D_INTERNAL_MOVED_NONE) &&
             (d_internal_moved_token(&_moved->before, _a)->span.line <
              d_internal_moved_token(&_moved->before, _b)->span.line) );
}

/*
d_internal_moved_kept
  A token that began its line before and after. A closing bracket moves as
the line its opener is on; a token in a `(` or `[` as the line the bracket
opens on; a `{` as the line its head begins on; a line continuing a
statement as the statement's first line. True when none of these holds it,
and it begins a statement, for d_internal_moved_siblings.
*/
static
bool d_internal_moved_kept(
    struct dj_internal_moved* _moved,
    size_t                    _c
)
{
    const struct dj_token* const token =
        d_internal_moved_token(&_moved->before, _c);
    const size_t frame  = _moved->frame[_c];
    const size_t opener = _moved->opener[_c];
    const char   kind   =
        (frame == D_INTERNAL_MOVED_NONE)
            ? 'f'
            : d_internal_moved_opens(d_internal_moved_token(&_moved->before,
                                                            frame));

    if (d_internal_moved_closes(token))
    {
        if (d_internal_moved_earlier(_moved, opener, _c))
        {
            d_internal_moved_follow(_moved, _c, opener,
                                    "the line its opener is on");
        }

        return false;
    }

    if ( (kind == '(') || (kind == '[') )
    {
        if (d_internal_moved_earlier(_moved, frame, _c))
        {
            d_internal_moved_follow(_moved, _c, frame,
                                    (kind == '(') ? "the line its ( is on"
                                                  : "the line its [ is on");
        }

        return false;
    }

    if ( (d_internal_tokens_is(token, "{")) && (_moved->head[_c] != _c) )
    {
        if (d_internal_moved_earlier(_moved, _moved->head[_c], _c))
        {
            d_internal_moved_follow(_moved, _c, _moved->head[_c],
                                    "the line its head begins on");
        }

        return false;
    }

    if (_moved->stmt[_c] != _c)
    {
        if (d_internal_moved_earlier(_moved, _moved->stmt[_c], _c))
        {
            d_internal_moved_follow(_moved, _c, _moved->stmt[_c],
                                    "statement");
        }

        return false;
    }

    return true;
}

/*
d_internal_moved_siblings
  Statements that stood in one column of one block stand in one column
still: each against the first of them, labels apart from statements. `_heads`
holds the statements' code indices, in order.
*/
static
bool d_internal_moved_siblings(
    struct dj_internal_moved* _moved,
    const size_t*             _heads,
    size_t                    _count
)
{
    size_t* const anchors = malloc(((_count) ? _count : 1u) * sizeof(size_t));
    size_t        groups  = 0u;

    if (!anchors)
    {
        return false;
    }

    for (size_t at = 0u; at < _count; ++at)
    {
        const size_t   c      = _heads[at];
        const uint32_t column = d_internal_moved_col(&_moved->before, c);
        size_t         g      = 0u;

        while ( (g < groups) &&
                ( (_moved->frame[anchors[g]] != _moved->frame[c]) ||
                  (_moved->label[anchors[g]] != _moved->label[c]) ||
                  (d_internal_moved_col(&_moved->before, anchors[g]) !=
                   column) ) )
        {
            ++g;
        }

        if (g == groups)
        {
            anchors[groups++] = c;

            continue;
        }

        const struct dj_token* const token =
            d_internal_moved_token(&_moved->after, c);
        const struct dj_token* const first =
            d_internal_moved_token(&_moved->after, anchors[g]);

        if (token->span.column != first->span.column)
        {
            d_internal_moved_say(_moved, token->span.line,
                                 "%s '%.*s' stood in column %u with line %u "
                                 "and no longer does: %u and %u",
                                 d_tokens_clang_name(_moved->after.file,
                                                     token),
                                 (int)token->span.length,
                                 _moved->after.file->source.text +
                                     token->span.offset,
                                 column, first->span.line,
                                 token->span.column, first->span.column);
        }
    }

    free(anchors);

    return true;
}

/*
d_internal_moved_ref
  How far a line moved whose first token, before, is token `_r`: a code
token by its index, a comment by `_map`; false when it begins no line after.
*/
static
bool d_internal_moved_ref(
    const struct dj_internal_moved* _moved,
    const size_t*                   _map,
    size_t                          _r,
    int*                            _delta,
    uint32_t*                       _line
)
{
    const struct dj_tokens_file* const before = _moved->before.file;
    const struct dj_tokens_file* const after  = _moved->after.file;
    const size_t c = _moved->before.of[_r];
    const size_t a = (c != D_INTERNAL_MOVED_NONE) ? _moved->after.code[c]
                                                  : _map[_r];

    if ( (a == D_INTERNAL_MOVED_NONE) ||
         (!_moved->after.first[a]) )
    {
        return false;
    }

    *_delta = (int)after->tokens[a].span.column -
              (int)before->tokens[_r].span.column;
    *_line  = after->tokens[a].span.line;

    return true;
}

/*
d_internal_moved_comment
  A comment on a line of its own, before and after: if it stood in the
column of the line after it, it moves as that line did; if not, and it stood
in the column of the line before it, as that one did.
*/
static
void d_internal_moved_comment(
    struct dj_internal_moved* _moved,
    const size_t*             _map,
    size_t                    _k
)
{
    const struct dj_tokens_file* const before = _moved->before.file;
    const struct dj_token* const       k      = &before->tokens[_k];
    const struct dj_token* const       to     =
        &_moved->after.file->tokens[_map[_k]];
    const int moved = (int)to->span.column - (int)k->span.column;
    size_t    next  = _k + 1u;
    int       delta = 0;
    uint32_t  line  = 0u;

    while ( (next < before->count) &&
            (before->tokens[next].span.line <= k->end_line) )
    {
        ++next;
    }

    if ( (next < before->count) &&
         (before->tokens[next].span.column == k->span.column) &&
         (d_internal_moved_ref(_moved, _map, next, &delta, &line)) )
    {
        if (moved != delta)
        {
            d_internal_moved_say(_moved, to->span.line,
                                 "a comment stood in the column of the line "
                                 "after it, line %u, which moved %+d; it "
                                 "moved %+d", line, delta, moved);
        }

        return;
    }

    const size_t prev = (_k > 0u) ? _moved->before.line[_k - 1u]
                                  : D_INTERNAL_MOVED_NONE;

    if ( (prev != D_INTERNAL_MOVED_NONE) &&
         (before->tokens[prev].span.column == k->span.column) &&
         (d_internal_moved_ref(_moved, _map, prev, &delta, &line)) &&
         (moved != delta) )
    {
        d_internal_moved_say(_moved, to->span.line,
                             "a comment stood in the column of the line "
                             "before it, line %u, which moved %+d; it moved "
                             "%+d", line, delta, moved);
    }

    return;
}

/*
d_internal_moved_comments
  Pairs the comments of the two files: between the same two code tokens, as
many on each side, in order. A repair that rewrote the comments between two
tokens into more or fewer leaves those unpaired, and unchecked. Then checks
each paired comment that has a line of its own on both sides.
*/
static
bool d_internal_moved_comments(
    struct dj_internal_moved* _moved
)
{
    const struct dj_tokens_file* const before = _moved->before.file;
    const struct dj_tokens_file* const after  = _moved->after.file;
    size_t* const map = malloc(((before->count) ? before->count : 1u) *
                               sizeof(size_t));
    size_t        b   = 0u;
    size_t        a   = 0u;

    if (!map)
    {
        return false;
    }

    for (size_t at = 0u; at < before->count; ++at)
    {
        map[at] = D_INTERNAL_MOVED_NONE;
    }

    while ( (b <= before->count) && (a <= after->count) )
    {
        size_t eb = b;
        size_t ea = a;

        while ( (eb < before->count) &&
                (before->tokens[eb].kind == D_TOKEN_COMMENT) )
        {
            ++eb;
        }

        while ( (ea < after->count) &&
                (after->tokens[ea].kind == D_TOKEN_COMMENT) )
        {
            ++ea;
        }

        for (size_t at = 0u; (eb - b == ea - a) && (at < eb - b); ++at)
        {
            map[b + at] = a + at;
        }

        b = eb + 1u;
        a = ea + 1u;
    }

    for (size_t at = 0u; at < before->count; ++at)
    {
        if ( (map[at] != D_INTERNAL_MOVED_NONE) &&
             (_moved->before.first[at]) &&
             (_moved->after.first[map[at]]) )
        {
            d_internal_moved_comment(_moved, map, at);
        }
    }

    free(map);

    return true;
}

/*
d_internal_moved_same_code
  Whether the two files hold the same code tokens, which pairing them by
their place in order needs.
*/
static
bool d_internal_moved_same_code(
    const struct dj_internal_moved* _moved
)
{
    for (size_t c = 0u; c < _moved->count; ++c)
    {
        const struct dj_token* const x =
            d_internal_moved_token(&_moved->before, c);
        const struct dj_token* const y =
            d_internal_moved_token(&_moved->after, c);

        if ( (x->kind != y->kind) ||
             (!d_internal_tokens_same_text(_moved->before.file, x,
                                           _moved->after.file, y)) )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_moved_lines
  Every line's first token, after, held to what it belongs with; then the
statements against their siblings, and the comments.
*/
static
bool d_internal_moved_lines(
    struct dj_internal_moved* _moved
)
{
    size_t* const heads = malloc(((_moved->count) ? _moved->count : 1u) *
                                 sizeof(size_t));
    size_t        count = 0u;

    if (!heads)
    {
        return false;
    }

    for (size_t c = 0u; c < _moved->count; ++c)
    {
        if ( (d_internal_moved_directive(_moved, c)) ||
             (!d_internal_moved_first(&_moved->after, c)) )
        {
            continue;
        }

        if (!d_internal_moved_first(&_moved->before, c))
        {
            d_internal_moved_given(_moved, c);
        }
        else if (d_internal_moved_kept(_moved, c))
        {
            heads[count++] = c;
        }
    }

    const bool ok = ( (d_internal_moved_siblings(_moved, heads, count)) &&
                      (d_internal_moved_comments(_moved)) );

    free(heads);

    return ok;
}

/*
d_internal_moved_free
  What the check made.
*/
static
void d_internal_moved_free(
    struct dj_internal_moved* _moved
)
{
    d_internal_side_free(&_moved->before);
    d_internal_side_free(&_moved->after);
    free(_moved->opener);
    free(_moved->frame);
    free(_moved->stmt);
    free(_moved->head);
    free(_moved->label);
    free(_moved->label_end);

    return;
}

/*
d_internal_moved_files
  The check over two lexed files.
*/
static
int d_internal_moved_files(
    const struct dj_tokens_file* _before,
    const struct dj_tokens_file* _after,
    FILE*                        _report
)
{
    struct dj_internal_moved moved;
    int                      result = 2;

    memset(&moved, 0, sizeof(moved));
    moved.report = _report;
    moved.clean  = true;

    if ( (d_internal_side_build(&moved.before, _before)) &&
         (d_internal_side_build(&moved.after, _after)) )
    {
        size_t in_after = 0u;

        for (size_t at = 0u; at < _after->count; ++at)
        {
            in_after += (_after->tokens[at].kind != D_TOKEN_COMMENT) ? 1u : 0u;
        }

        for (size_t at = 0u; at < _before->count; ++at)
        {
            moved.count += (_before->tokens[at].kind != D_TOKEN_COMMENT) ? 1u
                                                                         : 0u;
        }

        if ( (moved.count != in_after) ||
             (!d_internal_moved_same_code(&moved)) )
        {
            (void)fprintf(_report, "%s: not the same tokens as %s, and only "
                          "files that are can be compared line by line\n",
                          _after->path, _before->path);
            result = 1;
        }
        else if ( (d_internal_moved_structure(&moved)) &&
                  (d_internal_moved_lines(&moved)) )
        {
            result = (moved.clean) ? 0 : 1;
        }
    }

    if (result == 2)
    {
        (void)fprintf(_report, "%s: out of memory\n", _after->path);
    }

    d_internal_moved_free(&moved);

    return result;
}

/*
d_tokens_moved
  Reads both files and checks the second against the first.
*/
int
d_tokens_moved(
    const char* _before,
    const char* _after,
    FILE*       _report
)
{
    struct dj_tokens_file before;
    struct dj_tokens_file after;

    if (!d_tokens_read(&before, _before, _report))
    {
        return 2;
    }

    if (!d_tokens_read(&after, _after, _report))
    {
        d_tokens_free(&before);

        return 2;
    }

    const int result = d_internal_moved_files(&before, &after, _report);

    d_tokens_free(&before);
    d_tokens_free(&after);

    return result;
}
