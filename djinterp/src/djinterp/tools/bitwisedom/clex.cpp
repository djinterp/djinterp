/*******************************************************************************
* bitwisedom [bitwisedom]                                               clex.cpp
*
* C and C++ text as tokens: djinterp's scanners and literal decoder, behind
* values that own their spelling.
*   djinterp's lexer headers are C and declare their functions without the
* extern "C" guards its diagnostic header has, so they are included inside
* one here. Their diagnostics reach a sink with no storage and a hook, which
* turns each error into text as it is raised.
*
* path:      /src/djinterp/tools/bitwisedom/clex.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.10.10
*******************************************************************************/
#include <djinterp/tools/bitwisedom/clex.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cctype>  // std::isdigit, std::isxdigit
#include <vector>  // std::vector
// djinterp -- C headers, without guards of their own
extern "C"
{
#include "djinterp/parse/lang/c/lex_c.h"      // d_lex_c_create
#include "djinterp/parse/lang/cpp/lex_cpp.h"  // d_lex_cpp_create
#include "djinterp/parse/lex/lex_literal.h"   // d_lex_literal_decode
}


namespace bitwisedom
{

namespace
{

// heard
//   struct: what a diagnostic hook was told: every error as
// "line:column: message", and the first error and warning alone.
struct heard
{
    std::vector<std::string> errors;
    std::string              first_error;
    std::string              first_warning;
};

}  // anonymous namespace

}  // namespace bitwisedom


// the hook djinterp calls, with C's linkage and this unit's alone
extern "C"
{

static void
bitwisedom_clex_hook(
    void*                            _context,
    const struct d_parse_diag_sink*  _sink,
    const struct d_parse_diagnostic* _diagnostic
)
{
    (void)_sink;

    bitwisedom::heard& into  = *static_cast<bitwisedom::heard*>(_context);
    const std::string  text  = d_lex_error_text(_diagnostic->code);

    if (_diagnostic->severity >= D_PARSE_SEVERITY_ERROR)
    {
        into.errors.push_back(std::to_string(_diagnostic->span.line) + ":" +
                              std::to_string(_diagnostic->span.column) +
                              ": " + text);

        if (into.first_error.empty())
        {
            into.first_error = text;
        }
    }
    else if ( (_diagnostic->severity == D_PARSE_SEVERITY_WARNING) &&
              (into.first_warning.empty()) )
    {
        into.first_warning = text;
    }

    return;
}

}  // extern "C"


namespace bitwisedom
{

namespace
{

// listen
//   a sink that stores nothing and tells _heard everything.
void
listen(
    struct d_parse_diag_sink& _sink,
    heard&                    _heard
)
{
    d_parse_diag_sink_init(&_sink, nullptr, 0u, nullptr, 0u);
    d_parse_diag_sink_hook(&_sink, &bitwisedom_clex_hook, &_heard);

    return;
}

// class_of
//   a djinterp kind as the expander's class.
c_class
class_of(
    int _kind
)
{
    switch (_kind)
    {
        case D_TOKEN_IDENTIFIER:  return c_class::identifier;
        case D_TOKEN_NUMBER:      return c_class::number;
        case D_TOKEN_CHARACTER:   return c_class::character;
        case D_TOKEN_STRING:      return c_class::string;
        case D_TOKEN_HEADER_NAME: return c_class::string;
        case D_TOKEN_RAW_STRING:  return c_class::string;
        case D_TOKEN_COMMENT:     return c_class::comment;
        default:                  break;
    }

    if (d_token_is_punctuator(_kind))
    {
        return c_class::punctuator;
    }

    // keywords are never resolved here, but a keyword is a name
    if (d_token_is_keyword(_kind))
    {
        return c_class::identifier;
    }

    return c_class::other;
}

// lexer_for
//   a scanner over _source at a level.
struct d_lexer*
lexer_for(
    const struct d_source* _source,
    standard               _level,
    unsigned               _options
)
{
    const unsigned year = lex_level(_level);

    return cpp_level(_level) ? d_lex_cpp_create(_source, year, _options)
                             : d_lex_c_create(_source, year, _options);
}

// prefix_length
//   how many characters of an integer's spelling are its base's prefix.
std::size_t
prefix_length(
    const std::string& _text,
    unsigned           _base
)
{
    if ( (_text.size() >= 2) &&
         (_text[0] == '0')   &&
         ( (_base == 16) || (_base == 2) ) )
    {
        return 2;
    }

    if ( (_base == 8)        &&
         (_text.size() >= 2) &&
         (_text[0] == '0') )
    {
        // C2y's 0o prefix, or the plain leading zero
        return ( (_text[1] == 'o') || (_text[1] == 'O') ) ? 2u : 1u;
    }

    return 0;
}

}  // anonymous namespace


unsigned
lex_level(
    standard _level
)
{
    switch (_level)
    {
        case standard::c99:   return D_LEX_C99;
        case standard::c11:   return D_LEX_C11;
        case standard::c17:   return D_LEX_C17;
        case standard::c23:   return D_LEX_C23;
        case standard::cpp98: return D_LEX_CPP98;
        case standard::cpp11: return D_LEX_CPP11;
        case standard::cpp14: return D_LEX_CPP14;
        case standard::cpp17: return D_LEX_CPP17;
        case standard::cpp20: return D_LEX_CPP20;
        case standard::cpp23: return D_LEX_CPP23;
    }

    return D_LEX_C99;
}

bool
cpp_level(
    standard _level
)
{
    return (_level >= standard::cpp98);
}

/*
lex_c
  A text's tokens, every one. The scanner borrows the text, so each token's
spelling is copied out as it is read: from the text itself when nothing was
rewritten in it, otherwise as djinterp spells it, splices removed.
*/
c_lexed
lex_c(
    std::string_view _text,
    standard         _level,
    bool             _comments
)
{
    c_lexed out;

    if (_text.empty())
    {
        return out;
    }

    struct d_source source;
    d_source_init(&source, _text.data(), _text.size(), "bitwisedom");

    // offsets are the source's, which starts past a byte-order mark
    const std::size_t skipped =
        static_cast<std::size_t>(source.text - _text.data());
    struct d_lexer*   lexer   =
        lexer_for(&source,
                  _level,
                  _comments ? D_LEX_OPT_EMIT_COMMENTS : D_LEX_OPT_NONE);

    if (lexer == nullptr)
    {
        out.errors.push_back("1:1: the scanner could not start");

        return out;
    }

    heard                    errors;
    struct d_parse_diag_sink sink;
    struct d_token           token;
    std::vector<char>        buffer;

    listen(sink, errors);
    d_lex_set_diagnostics(lexer, &sink);

    // rewritten
    //   the flags that make a token's bytes differ from its spelling
    const unsigned rewritten = D_TOKEN_FLAG_SPLICED  |
                               D_TOKEN_FLAG_TRIGRAPH |
                               D_TOKEN_FLAG_UCN;

    while (d_lex_next(lexer, &token))
    {
        c_token made;
        made.kind       = class_of(token.kind);
        made.flags      = token.flags;
        made.offset     = skipped + token.span.offset;
        made.length     = token.span.length;
        made.line       = static_cast<int>(token.span.line);
        made.column     = static_cast<int>(token.span.column);
        made.end_line   = static_cast<int>(token.end_line);
        made.end_column = static_cast<int>(token.end_column);

        const char* primary = d_token_spelling(token.kind);

        if ( (made.kind == c_class::punctuator) &&
             (primary != nullptr) )
        {
            made.text = primary;
        }
        else if ((token.flags & rewritten) != 0)
        {
            const std::size_t need = d_token_spell(lexer, &token, nullptr, 0);

            buffer.assign(need + 1, '\0');
            (void)d_token_spell(lexer, &token, buffer.data(), buffer.size());
            made.text.assign(buffer.data(), need);
        }
        else
        {
            made.text.assign(source.text + token.span.offset,
                             token.span.length);
        }

        out.tokens.push_back(std::move(made));
    }

    d_lex_set_diagnostics(lexer, nullptr);
    d_lex_destroy(lexer);
    out.errors = std::move(errors.errors);

    return out;
}

std::string
spelled(
    const std::vector<c_token>& _tokens
)
{
    std::string out;

    for (std::size_t i = 0; i < _tokens.size(); ++i)
    {
        const c_token& each = _tokens[i];

        if (each.kind == c_class::placemarker)
        {
            continue;
        }

        if ( (!out.empty()) &&
             (each.has(C_LEADING_SPACE)) )
        {
            out += ' ';
        }

        out += each.text;
    }

    return out;
}

/*
decode_literal
  djinterp's decoder reads a spelling against a token for its kind and its
place; a token made for the purpose serves, since the spelling here may be
one the expander pasted together rather than one read from a source.
*/
c_literal
decode_literal(
    const c_token& _token,
    standard       _level,
    data_model     _model
)
{
    c_literal out;

    if ( (_token.kind != c_class::number) &&
         (_token.kind != c_class::character) )
    {
        out.error = "'" + _token.text + "' is not a literal";

        return out;
    }

    const unsigned year   = lex_level(_level);
    const auto     length = static_cast<re_std::uint32_t>(_token.text.size());

    struct d_token token = {};
    token.kind        = (_token.kind == c_class::number) ? D_TOKEN_NUMBER
                                                          : D_TOKEN_CHARACTER;
    token.span.length = length;
    token.span.line   = 1u;
    token.span.column = 1u;
    token.end_line    = 1u;
    token.end_column  = length + 1u;

    heard                        said;
    struct d_parse_diag_sink     sink;
    struct d_lex_literal         literal = {};
    struct d_lex_literal_context context = {};

    listen(sink, said);
    context.dialect = cpp_level(_level) ? d_lex_cpp_dialect(year)
                                        : d_lex_c_dialect(year);
    context.target  = d_lex_target_preset(
        (_model == data_model::llp64) ? D_LEX_TARGET_WINDOWS
                                      : D_LEX_TARGET_UNIX);
    context.sink    = &sink;

    out.ok          = d_lex_literal_decode(&context,
                                           &token,
                                           _token.text.data(),
                                           _token.text.size(),
                                           &literal);
    out.integer     = (literal.category == D_LEX_LITERAL_INTEGER);
    out.floating    = (literal.category == D_LEX_LITERAL_FLOATING);
    out.character   = (literal.category == D_LEX_LITERAL_CHARACTER);
    out.value       = literal.integer;
    out.base        = literal.base;
    out.is_unsigned = ((literal.suffix & D_LEX_SUFFIX_UNSIGNED) != 0);
    out.longs       = ((literal.suffix & D_LEX_SUFFIX_LONG_LONG) != 0) ? 2
                    : ((literal.suffix & D_LEX_SUFFIX_LONG) != 0)      ? 1
                                                                       : 0;
    out.size        = ((literal.suffix & D_LEX_SUFFIX_SIZE) != 0);
    out.bit_precise = ((literal.suffix & D_LEX_SUFFIX_BIT_PRECISE) != 0);
    out.user_suffix = ((literal.suffix & D_LEX_SUFFIX_USER) != 0);
    out.overflow    = ((literal.flags & D_LEX_LITERAL_FLAG_OVERFLOW) != 0);
    out.multichar   = ((literal.flags & D_LEX_LITERAL_FLAG_MULTICHAR) != 0);
    out.encoding    = literal.encoding;
    out.error       = said.first_error;
    out.warning     = said.first_warning;

    if ( (!out.ok) &&
         (out.error.empty()) )
    {
        out.error = "'" + _token.text + "' is not a literal this level reads";
    }

    if (!out.integer)
    {
        return out;
    }

    // how it is written: its digits, their case and its suffix's
    const std::size_t prefix = prefix_length(_token.text, out.base);
    const std::size_t end    = (literal.suffix_length > 0)
                                   ? literal.suffix_offset
                                   : _token.text.size();

    for (std::size_t i = prefix; i < end; ++i)
    {
        const char each = _token.text[i];

        if (std::isxdigit(static_cast<unsigned char>(each)))
        {
            ++out.digits;
            out.upper_hex = ( (out.upper_hex) ||
                              ( (each >= 'A') && (each <= 'F') ) );
            out.lower_hex = ( (out.lower_hex) ||
                              ( (each >= 'a') && (each <= 'f') ) );
        }
    }

    for (std::size_t i = end; i < _token.text.size(); ++i)
    {
        out.upper_suffix = ( (out.upper_suffix) ||
                             (_token.text[i] == 'U') ||
                             (_token.text[i] == 'L') );
    }

    return out;
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
