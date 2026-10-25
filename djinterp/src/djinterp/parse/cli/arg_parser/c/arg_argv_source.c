/*******************************************************************************
* djinterp [parse]                                             arg_argv_source.c
*
* Where arguments come from besides `argv`, declared in arg_argv.h (2.2, 2.3
* and 3.1) and arg_internal.h (8.3.2).
*   CLI11's split_program_name, which looks on disk for the program a line
* starts with, so that an unquoted path holding spaces is found whole;
* response files, read whole -- a byte-order mark honoured -- split by the
* parser's rules, and expanded on the parser's stack before the parse, each
* `@file` in place and its words expanded in turn, as deep as the build
* allows; and wide text converted to UTF-8, as wmain's arguments are, and
* back, as CLI11's widen converts a value.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_argv_source.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_argv.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memchr
#include <wchar.h>   // wchar_t, WCHAR_MAX
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "./arg_internal.h"  // spans, the system layer, encodings, the stack
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  PROGRAM NAMES
//==============================================================================

// NPOS
//   constant: CLI11's std::string::npos, the position of what is not there.
static const size_t NPOS = (size_t)-1;

/**
 * @brief Finds the first space at or after a position, as CLI11's
 *        find_first_of(' ', _from).
 *
 * @param[in] _line  the line.
 * @param[in] _from  where to look from.
 * @return the space's position; NPOS when there is none.
 *
 * CLI11's find_first_of(' ', _from): the position of the first space at or
 * after `_from`, or NPOS.
 */
D_STATIC size_t
d_arg_internal_program_space(
    struct d_pack_text _line,
    size_t             _from
)
{
    const char* const found = (_from < _line.length)
                              ? (const char*)memchr(_line.data + _from,
                                                    ' ',
                                                    _line.length - _from)
                              : NULL;

    return (found != NULL)
           ? (size_t)(found - _line.data)
           : NPOS;
}

/**
 * @brief Tells whether the start of a line names a file, as CLI11's check_path
 *        does.
 *
 * @param[in]     _line     the line.
 * @param[in]     _end      how many of its bytes; NPOS for all.
 * @param[in,out] _scratch  the buffer the path is copied into, cleared first.
 * @param[out]    _is_file  receives whether they name something that is not a
 *                          directory.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when the copy was refused.
 *
 * CLI11's check_path(commandline.substr(0, _end).c_str()) == file: whether
 * the line's first `_end` bytes -- all of it for NPOS -- name something that
 * is not a directory, into `*_is_file`; a refused copy is the failure.
 */
D_STATIC int
d_arg_internal_program_is_file(
    struct d_pack_text   _line,
    size_t               _end,
    struct d_arg_buffer* _scratch,
    bool*                _is_file
)
{
    d_arg_buffer_clear(_scratch);

    const int status = d_arg_buffer_append(_scratch,
                                           _line.data,
                                           (_end < _line.length)
                                           ? _end
                                           : _line.length);

    *_is_file = ( (status == D_ARG_OK) &&
                  (d_arg_internal_path_kind(
                       d_arg_buffer_text(_scratch).data) == 1) );

    return status;
}

/**
 * @brief Searches a line for the program, as CLI11 does.
 *
 * @param[in]  _line   the line.
 * @param[out] _end    receives the position of the space after the run of words
 *                     that named a file; NPOS when the search reached the end.
 * @param[out] _found  receives whether a file was named.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY.
 *
 * CLI11's search for the program: the line up to each space in turn, until
 * one names a file; `*_end` receives that space's position, or NPOS, and
 * `*_found` whether a file was named.
 */
D_STATIC int
d_arg_internal_program_find(
    struct d_pack_text _line,
    size_t*            _end,
    bool*              _found
)
{
    struct d_arg_buffer scratch = D_ARG_BUFFER_INIT;
    size_t              end     = d_arg_internal_program_space(_line,
                                                               1u);
    bool                found   = false;
    int                 status  = d_arg_internal_program_is_file(_line,
                                                                 end,
                                                                 &scratch,
                                                                 &found);

    // each longer run of words, until one is a file or the line ends
    while ( (status == D_ARG_OK) &&
            (!found)             &&
            (end != NPOS) )
    {
        end    = d_arg_internal_program_space(_line,
                                              end + 1u);
        status = d_arg_internal_program_is_file(_line,
                                                end,
                                                &scratch,
                                                &found);
    }

    d_arg_buffer_release(&scratch);
    *_end   = end;
    *_found = found;

    return status;
}

/**
 * @brief Appends a quoted name with its escaped quotes unescaped.
 *
 * @param[in]     _name  the name: what the quotes enclose.
 * @param[in]     _key   the quote.
 * @param[in,out] _out   the buffer that gains the name.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY.
 *
 * CLI11's find_and_replace(name, "\\\\" + key, key): each quote after a
 * backslash written without it. The name is what a quote encloses, so its last
 * byte is no backslash -- the quote after it would not have closed it -- and
 * the bound only keeps the look at the next byte within the name.
 */
D_STATIC int
d_arg_internal_program_unescape(
    struct d_pack_text   _name,
    char                 _key,
    struct d_arg_buffer* _out
)
{
    size_t at = 0u;

    // a byte at a time, an escaped quote as the quote
    while (at < _name.length)
    {
        const bool escaped = ( (_name.data[at] == '\\')       &&
                               ((at + 1u) < _name.length)     &&
                               (_name.data[at + 1u] == _key) );

        at += (escaped) ? 1u : 0u;
        (void)d_arg_buffer_append_char(_out,
                                       _name.data[at]);
        at++;
    }

    return (_out->failed)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

/**
 * @brief Reads the name a line opens with in quotes, as CLI11's fallback does.
 *
 * @param[in]     _line  the line, its first byte the opening quote.
 * @param[in,out] _name  the buffer that gains the name.
 * @param[out]    _end   receives the position after the closing quote; NPOS
 *                       when it never closes.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY.
 *
 * CLI11's fallback for a line that opens with a quote: the next such quote
 * that no backslash precedes closes it, and what it encloses is the name, its
 * escaped quotes unescaped -- CLI11 unescapes only when the search passed one,
 * but a name holding none comes out of the unescaping as it went in. `*_end`
 * receives the position after the closing quote, or NPOS when it never
 * closes. The line holds the opening quote, so each search, of what follows a
 * quote, stays within it.
 */
D_STATIC int
d_arg_internal_program_quoted(
    struct d_pack_text   _line,
    struct d_arg_buffer* _name,
    size_t*              _end
)
{
    const char        key   = _line.data[0];
    const char* const first = (const char*)memchr(_line.data + 1u,
                                                  key,
                                                  _line.length - 1u);
    size_t            close = (first != NULL)
                              ? (size_t)(first - _line.data)
                              : NPOS;

    // a quote after a backslash does not close
    while ( (close != NPOS) &&
            (_line.data[close - 1u] == '\\') )
    {
        const char* const next = (const char*)memchr(
                                     _line.data + close + 1u,
                                     key,
                                     _line.length - close - 1u);

        close = (next != NULL) ? (size_t)(next - _line.data) : NPOS;
    }

    *_end = (close != NPOS) ? (close + 1u) : NPOS;

    // never closed: no name from the quote
    if (close == NPOS)
    {
        return D_ARG_OK;
    }

    return d_arg_internal_program_unescape(
               d_arg_internal_span(_line.data + 1u,
                                   close - 1u),
               key,
               _name);
}

/**
 * @brief Chooses the program's name once the search for a file is over, as
 *        CLI11 does.
 *
 * @param[in]     _line   the line.
 * @param[in]     _found  whether a run of words named a file.
 * @param[in,out] _name   the buffer that gains the name.
 * @param[in,out] _end    the end of the run that named a file, or NPOS;
 *                        receives the position CLI11 continues from.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY.
 *
 * CLI11's choice of the name once the search for a file is over: the run of
 * words that named one; or else a quoted name; or else the first word -- the
 * run, or the first word, less its trailing whitespace. `*_end` receives the
 * position CLI11 continues from.
 */
D_STATIC int
d_arg_internal_program_name(
    struct d_pack_text   _line,
    bool                 _found,
    struct d_arg_buffer* _name,
    size_t*              _end
)
{
    const size_t before = _name->length;
    const bool   quote  = ( (!_found)                    &&
                            (_line.length > 0u)          &&
                            ( (_line.data[0] == '"')  ||
                              (_line.data[0] == '\'') ||
                              (_line.data[0] == '`') ) );
    int          status = (quote)
                          ? d_arg_internal_program_quoted(_line,
                                                          _name,
                                                          _end)
                          : D_ARG_OK;

    // without a file or a closed quote, the first word
    if ( (!_found) &&
         (*_end == NPOS) )
    {
        *_end = d_arg_internal_program_space(_line,
                                             1u);
    }

    // a name not taken from a quote, or an empty one, is the run itself
    if ( (status == D_ARG_OK) &&
         (_name->length == before) )
    {
        status = d_arg_buffer_append_text(
                     _name,
                     d_arg_internal_trim(
                         d_arg_internal_span(_line.data,
                                             (*_end < _line.length)
                                             ? *_end
                                             : _line.length)));
    }

    return status;
}

/*
d_arg_split_program_name
  CLI11's split_program_name over the trimmed line: the name the search and
its fallbacks give; then the rest, one byte after the name dropped, less
its leading whitespace. On failure the name buffer is as it was.
*/
int
d_arg_split_program_name(
    struct d_pack_text   _line,
    struct d_arg_buffer* _name,
    struct d_pack_text*  _rest
)
{
    // validate the parameters
    if ( (!_name)                   ||
         (!_rest)                   ||
         ( (!_line.data) &&
           (_line.length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const struct d_pack_text line   = d_arg_internal_trim(
                                          (_line.data != NULL)
                                          ? _line
                                          : d_arg_internal_span("",
                                                                0u));
    const size_t             before = _name->length;
    size_t                   end    = NPOS;
    bool                     found  = false;
    int                      status = d_arg_internal_program_find(line,
                                                                  &end,
                                                                  &found);

    status = (status == D_ARG_OK)
             ? d_arg_internal_program_name(line,
                                           found,
                                           _name,
                                           &end)
             : status;

    const size_t start = ( (end != NPOS) &&
                           ((end + 1u) < line.length) )
                         ? (end + 1u)
                         : line.length;

    *_rest = d_arg_internal_trim(d_arg_internal_span(line.data + start,
                                                     line.length - start));

    // a failure leaves the name as it was
    if (status != D_ARG_OK)
    {
        d_arg_buffer_truncate(_name,
                              before);
        *_rest = d_arg_internal_span(line.data,
                                     0u);
    }

    return status;
}

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

//==============================================================================
// 2.  RESPONSE FILES
//==============================================================================

/**
 * @brief Converts UTF-16 bytes to UTF-8.
 *
 * @param[in]     _bytes       the bytes after the byte-order mark.
 * @param[in]     _big_endian  whether the mark gave big-endian order.
 * @param[in,out] _out         the buffer that gains the UTF-8.
 * @return D_ARG_OK, or the buffer's status.
 *
 * A file's bytes after a UTF-16 byte-order mark, as UTF-8: each two bytes a
 * unit, in the order the mark gave, an unpaired surrogate U+FFFD; and an odd
 * byte at the end U+FFFD too.
 */
D_STATIC int
d_arg_internal_response_utf16(
    struct d_pack_text   _bytes,
    bool                 _big_endian,
    struct d_arg_buffer* _out
)
{
    struct d_arg_internal_utf16 state  = { 0u };
    int                         status = D_ARG_OK;

    // each unit in turn
    for (size_t at = 0u;
         (status == D_ARG_OK) && ((at + 1u) < _bytes.length);
         at += 2u)
    {
        const uint32_t first  = (unsigned char)_bytes.data[at];
        const uint32_t second = (unsigned char)_bytes.data[at + 1u];

        status = d_arg_internal_utf16_feed(&state,
                                           (_big_endian)
                                           ? ((first << 8u) | second)
                                           : ((second << 8u) | first),
                                           _out);
    }

    status = (status == D_ARG_OK)
             ? d_arg_internal_utf16_finish(&state,
                                           _out)
             : status;

    return ( (status == D_ARG_OK) &&
             ((_bytes.length % 2u) != 0u) )
           ? d_arg_internal_utf8_append(_out,
                                        0xFFFDu)
           : status;
}

/**
 * @brief Finds a response file's text as UTF-8.
 *
 * @param[in]     _bytes      the file's bytes.
 * @param[in,out] _converted  the buffer that gains a conversion from UTF-16.
 * @param[out]    _text       receives a span of the bytes or of the conversion.
 * @return D_ARG_OK, or the buffer's status.
 *
 * A response file's text as UTF-8: after UTF-8's byte-order mark, which is
 * skipped; converted after UTF-16's, in either order, into `_converted`; or
 * the bytes as they are. `*_text` receives a span of the bytes or of the
 * conversion.
 */
D_STATIC int
d_arg_internal_response_text(
    struct d_pack_text   _bytes,
    struct d_arg_buffer* _converted,
    struct d_pack_text*  _text
)
{
    const unsigned char* const mark   = (const unsigned char*)_bytes.data;
    const bool                 little = ( (_bytes.length >= 2u) &&
                                          (mark[0] == 0xFFu)    &&
                                          (mark[1] == 0xFEu) );
    const bool                 big    = ( (_bytes.length >= 2u) &&
                                          (mark[0] == 0xFEu)    &&
                                          (mark[1] == 0xFFu) );

    // UTF-8's mark, skipped
    if ( (_bytes.length >= 3u) &&
         (mark[0] == 0xEFu)    &&
         (mark[1] == 0xBBu)    &&
         (mark[2] == 0xBFu) )
    {
        *_text = d_arg_internal_span(_bytes.data + 3u,
                                     _bytes.length - 3u);

        return D_ARG_OK;
    }

    // no UTF-16 mark: the bytes as they are
    if ( (!little) &&
         (!big) )
    {
        *_text = _bytes;

        return D_ARG_OK;
    }

    const int status = d_arg_internal_response_utf16(
                           d_arg_internal_span(_bytes.data + 2u,
                                               _bytes.length - 2u),
                           big,
                           _converted);

    *_text = d_arg_buffer_text(_converted);

    return status;
}

/**
 * @brief Reads a response file's words.
 *
 * @param[in]     _path     the file's path.
 * @param[in]     _quoting  the d_arg_quoting value the file is split by.
 * @param[in]     _words    where the words go.
 * @param[in,out] _error    receives a failure's description, appended; may be
 *                          NULL.
 * @return D_ARG_OK; D_ARG_ERROR_BAD_ARGUMENT for no path, no list, or a quoting
 *         not known; D_ARG_ERROR_FILE for a file that cannot be read; otherwise
 *         a status as d_arg_split_line's.
 *
 * A file read whole, its text found, and split by the rules asked for, line
 * breaks separating words, the words stored where they go; a file that cannot
 * be read described as CLI11 describes a missing one.
 */
D_STATIC int
d_arg_internal_response_words(
    const char*                        _path,
    int                                _quoting,
    const struct d_arg_internal_words* _words,
    struct d_arg_buffer*               _error
)
{
    // validate the parameters
    if ( (!_path)                                  ||
         (!_words->strings)                        ||
         (!d_arg_internal_quoting_known(_quoting)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer bytes     = D_ARG_BUFFER_INIT;
    struct d_arg_buffer converted = D_ARG_BUFFER_INIT;
    struct d_pack_text  text      = d_arg_internal_span("",
                                                        0u);
    int                 status    = d_arg_internal_read_file(_path,
                                                             false,
                                                             &bytes);

    // a file that cannot be read
    if ( (status == D_ARG_ERROR_FILE) &&
         (_error != NULL) )
    {
        (void)d_arg_buffer_printf(_error,
                                  "%s was not readable (missing?)",
                                  _path);
    }

    status = (status == D_ARG_OK)
             ? d_arg_internal_response_text(d_arg_buffer_text(&bytes),
                                            &converted,
                                            &text)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_split_words(text,
                                          _quoting,
                                          D_ARG_SPLIT_LINE_BREAKS,
                                          _words,
                                          _error)
             : status;
    d_arg_buffer_release(&converted);
    d_arg_buffer_release(&bytes);

    return status;
}

/*
d_arg_read_response_file
  d_arg_internal_response_words into a string list alone.
*/
int
d_arg_read_response_file(
    const char*               _path,
    int                       _quoting,
    struct d_arg_string_list* _out,
    struct d_arg_buffer*      _error
)
{
    const struct d_arg_internal_words words = { _out,
                                                NULL };

    return d_arg_internal_response_words(_path,
                                         _quoting,
                                         &words,
                                         _error);
}

// d_arg_internal_response_walk
//   struct: an expansion under way: the parser, whose pool keeps the files'
// words; their spans, in the order read, each to the length it was made
// with; the arguments as they come out, the first first; and whether a `--`
// has been met, after which nothing is expanded.
struct d_arg_internal_response_walk
{
    struct d_arg_parser*   parser;
    struct d_arg_text_list words;
    struct d_arg_text_list out;
    bool                   ended;
};

/**
 * @brief Expands one argument met in order.
 *
 * @param[in,out] _walk   the walk, whose arguments gain the argument, or the
 *                        words of the file it names.
 * @param[in]     _word   the argument.
 * @param[in]     _depth  how many files deep it was met.
 * @return D_ARG_OK, or a failure, recorded.
 *
 * Declared ahead of its definition, which follows the step it takes for a
 * file, whose words come back to it one level deeper.
 */
D_STATIC int
     d_arg_internal_response_word(struct d_arg_internal_response_walk* _walk,
                                  struct d_pack_text                   _word,
                                  int                                  _depth);

/**
 * @brief Records a response file's failure with its path.
 *
 * @param[in] _parser  the parser.
 * @param[in] _path    the file's path.
 * @param[in] _status  the failure.
 * @param[in] _error   its description; empty for none.
 * @return `_status`, recorded.
 *
 * A response file's words that break its rules, recorded with the path they
 * came from; a refused copy recorded bare.
 */
D_STATIC int
d_arg_internal_response_refuse(
    const struct d_arg_parser* _parser,
    const char*                _path,
    int                        _status,
    const struct d_arg_buffer* _error
)
{
    // nothing to say but the status
    if (_error->length == 0u)
    {
        return d_arg_internal_parse_stored(&_parser->command,
                                           _status);
    }

    return d_arg_internal_fail(&_parser->command,
                               NULL,
                               _status,
                               "%s: %s",
                               _path,
                               _error->data);
}

/**
 * @brief Reads the words of the file an `\@file` names.
 *
 * @param[in,out] _walk  the walk, whose words gain the file's, or whose
 *                       arguments gain `_word` when the file cannot be read.
 * @param[in]     _word  the argument: `@` and the path.
 * @param[out]    _read  receives whether the file's words were read.
 * @return D_ARG_OK, or a failure, recorded.
 *
 * The file an `\@file` names, its words read into the parser's pool and
 * their spans after the walk's, `*_read` set when they are there: a file that
 * cannot be read kept as the argument, as GCC keeps one; words that break the
 * file's rules a failure.
 */
D_STATIC int
d_arg_internal_response_read(
    struct d_arg_internal_response_walk* _walk,
    struct d_pack_text                   _word,
    bool*                                _read
)
{
    struct d_arg_parser* const        parser = _walk->parser;
    const char* const                 path   = _word.data + 1;
    const struct d_arg_internal_words words  = { &parser->pool,
                                                 &_walk->words };
    struct d_arg_buffer               error  = D_ARG_BUFFER_INIT;
    int                               status = d_arg_internal_response_words(
                                                   path,
                                                   parser->response_quoting,
                                                   &words,
                                                   &error);

    *_read = (status == D_ARG_OK);

    // unreadable: the argument as it is; broken: the failure
    if (status == D_ARG_ERROR_FILE)
    {
        status = d_arg_internal_parse_stored(
                     &parser->command,
                     d_arg_internal_text_push(&_walk->out,
                                              _word));
    }
    else if (status != D_ARG_OK)
    {
        status = d_arg_internal_response_refuse(parser,
                                                path,
                                                status,
                                                &error);
    }

    d_arg_buffer_release(&error);

    return status;
}

/**
 * @brief Expands an `\@file` within the nesting allowed.
 *
 * @param[in,out] _walk   the walk.
 * @param[in]     _word   the argument: `@` and the path.
 * @param[in]     _depth  how many files deep it was met.
 * @return D_ARG_OK; D_ARG_ERROR_FILE, recorded, for a file nested too deep; or
 *         another failure, recorded.
 *
 * An `\@file` within the nesting allowed: its file's words read and each
 * expanded one level deeper, in place; a file too deep a failure.
 */
D_STATIC int
d_arg_internal_response_file(
    struct d_arg_internal_response_walk* _walk,
    struct d_pack_text                   _word,
    int                                  _depth
)
{
    struct d_arg_parser* const parser = _walk->parser;

    // nested too deeply
    if (_depth >= (int)D_INTERNAL_ARG_PARSER_RESPONSE_DEPTH)
    {
        return d_arg_internal_fail(&parser->command,
                                   NULL,
                                   D_ARG_ERROR_FILE,
                                   "%s: response files nest more than %d "
                                   "deep",
                                   _word.data + 1,
                                   (int)D_INTERNAL_ARG_PARSER_RESPONSE_DEPTH);
    }

    const size_t first  = _walk->words.count;
    bool         read   = false;
    int          status = d_arg_internal_response_read(_walk,
                                                       _word,
                                                       &read);
    const size_t last   = (read) ? _walk->words.count : first;

    // each of the file's own words, expanded in turn; the words of files
    // they name land past `last`, and are this loop's no more
    for (size_t index = first;
         (status == D_ARG_OK) && (index < last);
         ++index)
    {
        status = d_arg_internal_response_word(_walk,
                                              _walk->words.items[index],
                                              _depth + 1);
    }

    return status;
}

/*
d_arg_internal_response_word
  One argument met in order: one starting with `@` before any `--` stands
for its file's words -- unless it holds a NUL byte, which no path can;
anything else, the `--` included, is kept.
*/
D_STATIC int
d_arg_internal_response_word(
    struct d_arg_internal_response_walk* _walk,
    struct d_pack_text                   _word,
    int                                  _depth
)
{
    const bool file = ( (!_walk->ended)                     &&
                        (_word.length > 1u)                 &&
                        (_word.data[0] == '@')              &&
                        (memchr(_word.data,
                                '\0',
                                _word.length) == NULL) );

    _walk->ended = ( (_walk->ended) ||
                     (d_arg_internal_equal_cstr(_word,
                                                "--")) );

    return (file)
           ? d_arg_internal_response_file(_walk,
                                          _word,
                                          _depth)
           : d_arg_internal_parse_stored(
                 &_walk->parser->command,
                 d_arg_internal_text_push(&_walk->out,
                                          _word));
}

/*
d_arg_internal_expand_responses
  The stack walked from its top -- the next argument -- down, each argument
expanded into a list in argument order, which then replaces the stack, the
first argument on top again.
*/
int
d_arg_internal_expand_responses(
    struct d_arg_parser* _parser
)
{
    struct d_arg_text_list* const       pending = &_parser->pending;
    struct d_arg_internal_response_walk walk    = { _parser,
                                                    D_ARG_LIST_INIT,
                                                    D_ARG_LIST_INIT,
                                                    false };
    int                                 status  = D_ARG_OK;

    // each argument, the next one first
    for (size_t index = pending->count;
         (status == D_ARG_OK) && (index > 0u);
         --index)
    {
        status = d_arg_internal_response_word(&walk,
                                              pending->items[index - 1u],
                                              0);
    }

    // the expansion back onto the stack, the first argument last
    if (status == D_ARG_OK)
    {
        pending->count = 0u;
        status         = d_arg_internal_text_reserve(pending,
                                                     walk.out.count);

        for (size_t index = walk.out.count;
             (status == D_ARG_OK) && (index > 0u);
             --index)
        {
            status = d_arg_internal_text_push(pending,
                                              walk.out.items[index - 1u]);
        }

        status = d_arg_internal_parse_stored(&_parser->command,
                                             status);
    }

    d_arg_text_list_release(&walk.out);
    d_arg_text_list_release(&walk.words);

    return status;
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

//==============================================================================
// 3.  WIDE TEXT
//==============================================================================

#if (WCHAR_MAX <= 0xFFFF)

/*
d_arg_utf8_from_wide
  Wide text where `wchar_t` has 16 bits, as on Windows: UTF-16, each
unpaired surrogate U+FFFD, as Windows converts one.
*/
int
d_arg_utf8_from_wide(
    const wchar_t*       _text,
    size_t               _length,
    struct d_arg_buffer* _out
)
{
    // validate the parameters
    if ( (!_out) ||
         ( (!_text) &&
           (_length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_internal_utf16 state  = { 0u };
    int                         status = D_ARG_OK;

    // each unit in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _length);
         ++index)
    {
        status = d_arg_internal_utf16_feed(&state,
                                           (uint32_t)_text[index],
                                           _out);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_utf16_finish(&state,
                                         _out)
           : status;
}

#else

/*
d_arg_utf8_from_wide
  Wide text where `wchar_t` has more than 16 bits: UTF-32, a surrogate or a
value past U+10FFFF -- a negative one too -- U+FFFD.
*/
int
d_arg_utf8_from_wide(
    const wchar_t*       _text,
    size_t               _length,
    struct d_arg_buffer* _out
)
{
    // validate the parameters
    if ( (!_out) ||
         ( (!_text) &&
           (_length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = D_ARG_OK;

    // each code point in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _length);
         ++index)
    {
        status = d_arg_internal_utf8_append(_out,
                                            (uint32_t)_text[index]);
    }

    return status;
}

#endif  // WCHAR_MAX

/**
 * @brief Counts the `wchar_t` units UTF-8 text takes.
 *
 * @param[in] _text  the text.
 * @return the count; NPOS when the text is not well-formed UTF-8.
 *
 * One unit for each code point, but two for one past U+FFFF where `wchar_t`
 * has 16 bits; the first sequence that does not decode ends the count.
 */
D_STATIC size_t
d_arg_internal_wide_units(
    struct d_pack_text _text
)
{
    const bool sixteen    = (WCHAR_MAX <= 0xFFFF);
    size_t     at         = 0u;
    size_t     units      = 0u;
    uint32_t   code_point = 0u;

    // each sequence in turn, refused at the first that is not one
    while (at < _text.length)
    {
        if (!d_arg_internal_utf8_next(_text,
                                      &at,
                                      &code_point))
        {
            return NPOS;
        }

        units += ( (sixteen) &&
                   (code_point > 0xFFFFu) )
                 ? 2u
                 : 1u;
    }

    return units;
}

/**
 * @brief Writes well-formed UTF-8 text as `wchar_t` units.
 *
 * @param[in]  _text  the text, which d_arg_internal_wide_units counted.
 * @param[out] _out   receives the units; room for as many as were counted.
 *
 * A code point past U+FFFF becomes a surrogate pair where `wchar_t` has 16
 * bits, and stays whole where it has more.
 */
D_STATIC void
d_arg_internal_wide_write(
    struct d_pack_text _text,
    wchar_t*           _out
)
{
    const bool sixteen    = (WCHAR_MAX <= 0xFFFF);
    size_t     at         = 0u;
    size_t     out        = 0u;
    uint32_t   code_point = 0u;

    // each code point, a pair past U+FFFF where units have 16 bits
    while ( (at < _text.length) &&
            (d_arg_internal_utf8_next(_text,
                                      &at,
                                      &code_point)) )
    {
        if ( (sixteen) &&
             (code_point > 0xFFFFu) )
        {
            _out[out++] = (wchar_t)(0xD800u +
                                    ((code_point - 0x10000u) >> 10u));
            code_point  = 0xDC00u + ((code_point - 0x10000u) & 0x3FFu);
        }

        _out[out++] = (wchar_t)code_point;
    }

    return;
}

/*
d_arg_wide_from_utf8
  Checked and counted first, so that a refusal writes nothing; then, when
the units fit, written.
*/
int
d_arg_wide_from_utf8(
    const char* _text,
    size_t      _length,
    wchar_t*    _out,
    size_t      _capacity,
    size_t*     _needed
)
{
    // validate the parameters
    if ( (!_needed)              ||
         ( (!_text) &&
           (_length > 0u) )      ||
         ( (!_out) &&
           (_capacity > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const struct d_pack_text text  = { _text, _length };
    const size_t             units = d_arg_internal_wide_units(text);

    // text that is not UTF-8 converts to nothing
    if (units == NPOS)
    {
        return D_ARG_ERROR_CONVERSION;
    }

    // the units, when there is room for them
    if (units <= _capacity)
    {
        d_arg_internal_wide_write(text,
                                  _out);
    }

    *_needed = units;

    return D_ARG_OK;
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV
