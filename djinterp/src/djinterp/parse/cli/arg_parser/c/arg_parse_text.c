/*******************************************************************************
* djinterp [parse]                                              arg_parse_text.c
*
* Parsing arguments that arrive as text, declared in arg_parser.h (4.1.3, 4.2
* and 4.3).
*   CLI11's parse(string): a command line split by CLI11's own rules --
* reading `:` as `=` for a root that takes Windows-style options -- its
* program's name, when it holds one, naming the root; wmain's arguments and
* wide lines, converted to UTF-8 first; and CLI11's ensure_utf8, which
* recovers main's arguments from the process's wide command line, split by
* the C runtime's rules. The words are the parser's pool's until the next
* parse, each stacked whole -- a NUL byte an escape gives kept among its
* bytes, as CLI11's std::string keeps it -- and every parse here then runs as
* d_arg_parser_parse runs.
*   Then CLI11's parse_from_stream: configuration text read with the root's
* format and parsed as a configuration file's items are, adding to the
* parse before it.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_parse_text.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <wchar.h>   // wchar_t, wcslen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_argv.h"  // splitting, wide text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command_has_flag
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_config.h"  // reading configuration text
#include "./arg_internal.h"  // the parse's entry steps, lists, failures


//==============================================================================
// 1.  COMMAND LINES
//==============================================================================

/**
 * @brief Splits the text after the program's name into the parser's pool.
 *
 * @param[in,out] _parser  the parser, whose pool keeps the words.
 * @param[in]     _text    the text.
 * @param[in,out] _words   the list that gains the words' spans.
 * @return D_ARG_OK; D_ARG_ERROR_PARSE for a malformed escape, or the split's
 *         other refusal, either recorded.
 *
 * CLI11's split in parse(string) of what follows the program's name: into
 * the parser's pool by CLI11's rules, `:` read as `=` where the root takes
 * Windows-style options, each word's span after `_words`'s, to the length it
 * was made with; a malformed escape -- the one text CLI11's rules refuse --
 * recorded as CLI11's ParseError, in its words.
 */
D_STATIC int
d_arg_internal_parse_text_split(
    struct d_arg_parser*    _parser,
    struct d_pack_text      _text,
    struct d_arg_text_list* _words
)
{
    const struct d_arg_internal_words words  = { &_parser->pool,
                                                 _words };
    struct d_arg_buffer               error  = D_ARG_BUFFER_INIT;
    const uint32_t                    flags  =
        (d_arg_command_has_flag(&_parser->command,
                                D_ARG_COMMAND_WINDOWS_STYLE))
        ? D_ARG_SPLIT_WINDOWS_STYLE
        : 0u;
    int                               status = d_arg_internal_split_words(
                                                   _text,
                                                   D_ARG_QUOTING_CLI11,
                                                   flags,
                                                   &words,
                                                   &error);

    // the escape CLI11 refuses, in its words; or a refused copy
    if ( (status == D_ARG_ERROR_INVALID) &&
         (error.length > 0u) )
    {
        status = d_arg_internal_fail(&_parser->command,
                                     NULL,
                                     D_ARG_ERROR_PARSE,
                                     "%s",
                                     error.data);
    }
    else
    {
        status = d_arg_internal_parse_stored(&_parser->command,
                                             status);
    }

    d_arg_buffer_release(&error);

    return status;
}

/**
 * @brief Parses a line once the parse has begun.
 *
 * @param[in,out] _parser             the parser.
 * @param[in]     _line               the line.
 * @param[in]     _with_program_name  whether the line begins with the program's
 *                                    name.
 * @return D_ARG_OK, or a failure, recorded.
 *
 * CLI11's parse(string) once the parse has begun: the program's name, when
 * the line holds one, found as CLI11 finds it and naming the root; the rest
 * split into the pool and stacked, each word whole; the line trimmed
 * otherwise.
 */
D_STATIC int
d_arg_internal_parse_text_line(
    struct d_arg_parser* _parser,
    struct d_pack_text   _line,
    bool                 _with_program_name
)
{
    struct d_arg_buffer    name   = D_ARG_BUFFER_INIT;
    struct d_arg_text_list words  = D_ARG_LIST_INIT;
    struct d_pack_text     rest   = d_arg_internal_trim(_line);
    int                    status = D_ARG_OK;

    // the program's name, which names the root
    if (_with_program_name)
    {
        status = d_arg_internal_parse_stored(
                     &_parser->command,
                     d_arg_split_program_name(_line,
                                              &name,
                                              &rest));
        status = (status == D_ARG_OK)
                 ? d_arg_internal_parse_name(
                       _parser,
                       d_arg_buffer_text(&name).data)
                 : status;
    }

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_text_split(_parser,
                                               rest,
                                               &words)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_words(_parser,
                                          &words,
                                          0u)
             : status;
    d_arg_text_list_release(&words);
    d_arg_buffer_release(&name);

    return status;
}

/*
d_arg_parser_parse_line
  CLI11's parse(string, program_name_included): begun, split, stacked and
parsed.
*/
int
d_arg_parser_parse_line(
    struct d_arg_parser* _parser,
    struct d_pack_text   _line,
    bool                 _with_program_name
)
{
    // validate the parameters
    if ( (!_parser)                 ||
         ( (!_line.data) &&
           (_line.length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = d_arg_internal_parse_begin(_parser);

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_text_line(
                   _parser,
                   (_line.data != NULL)
                   ? _line
                   : d_arg_internal_span("",
                                         0u),
                   _with_program_name)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_run(_parser)
           : status;
}

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

//==============================================================================
// 2.  WIDE ARGUMENTS
//==============================================================================

/**
 * @brief Converts wide arguments to UTF-8 into the parser's pool.
 *
 * @param[in,out] _parser  the parser, whose pool keeps the words.
 * @param[in]     _argc    how many arguments there are.
 * @param[in]     _argv    the arguments.
 * @param[in,out] _words   the list that gains their spans.
 * @return D_ARG_OK, or the conversion's or the storage's refusal, recorded.
 *
 * Wide arguments converted to UTF-8 into the parser's pool, in order, each
 * one's span after `_words`'s; a refusal recorded.
 */
D_STATIC int
d_arg_internal_parse_text_wide(
    struct d_arg_parser*    _parser,
    int                     _argc,
    wchar_t* const*         _argv,
    struct d_arg_text_list* _words
)
{
    const struct d_arg_internal_words words  = { &_parser->pool,
                                                 _words };
    struct d_arg_buffer               word   = D_ARG_BUFFER_INIT;
    int                               status = D_ARG_OK;

    // each argument in turn
    for (int index = 0;
         (status == D_ARG_OK) && (index < _argc);
         ++index)
    {
        d_arg_buffer_clear(&word);
        status = d_arg_utf8_from_wide(_argv[index],
                                      wcslen(_argv[index]),
                                      &word);
        status = (status == D_ARG_OK)
                 ? d_arg_internal_words_add(&words,
                                            d_arg_buffer_text(&word))
                 : status;
    }

    d_arg_buffer_release(&word);

    return d_arg_internal_parse_stored(&_parser->command,
                                       status);
}

/*
d_arg_parser_parse_wide
  CLI11's parse(argc, const wchar_t* const* argv): the arguments converted
into the pool, the first naming the root, the rest stacked and parsed.
*/
int
d_arg_parser_parse_wide(
    struct d_arg_parser* _parser,
    int                  _argc,
    wchar_t* const*      _argv
)
{
    // validate the parameters
    if ( (!_parser) ||
         ( (_argc > 0) &&
           (!_argv) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // every argument must be there
    for (int index = 0; index < _argc; ++index)
    {
        if (!_argv[index])
        {
            return D_ARG_ERROR_BAD_ARGUMENT;
        }
    }

    struct d_arg_text_list words  = D_ARG_LIST_INIT;
    int                    status = d_arg_internal_parse_begin(_parser);

    // the arguments converted, the first the name, the rest stacked
    if ( (status == D_ARG_OK) &&
         (_argc > 0) )
    {
        status = d_arg_internal_parse_text_wide(_parser,
                                                _argc,
                                                _argv,
                                                &words);
        status = (status == D_ARG_OK)
                 ? d_arg_internal_parse_name(_parser,
                                             words.items[0].data)
                 : status;
        status = (status == D_ARG_OK)
                 ? d_arg_internal_parse_words(_parser,
                                              &words,
                                              1u)
                 : status;
    }

    d_arg_text_list_release(&words);

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_run(_parser)
           : status;
}

/*
d_arg_parser_parse_line_wide
  CLI11's parse(wstring, program_name_included): the line converted to
UTF-8, which then parses as d_arg_parser_parse_line parses it.
*/
int
d_arg_parser_parse_line_wide(
    struct d_arg_parser* _parser,
    const wchar_t*       _line,
    size_t               _length,
    bool                 _with_program_name
)
{
    // validate the parameters
    if ( (!_parser)                 ||
         ( (!_line) &&
           (_length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer line   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_parse_begin(_parser);

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_stored(
                   &_parser->command,
                   d_arg_utf8_from_wide(_line,
                                        _length,
                                        &line))
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_text_line(_parser,
                                              d_arg_buffer_text(&line),
                                              _with_program_name)
             : status;
    d_arg_buffer_release(&line);

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_run(_parser)
           : status;
}

//==============================================================================
// 3.  UTF-8 ARGUMENTS
//==============================================================================

/*
d_arg_parser_ensure_utf8
  CLI11's ensure_utf8: the process's wide command line split by the C
runtime's rules, as the runtime made `argv` from it, kept by the parser and
returned, a NULL after its last as `argv` has, when it holds as many
arguments as `argv` -- which it does not when the runtime expanded
wildcards; `argv` where there is no such line.
*/
char**
d_arg_parser_ensure_utf8(
    struct d_arg_parser* _parser,
    char**               _argv
)
{
    struct d_arg_buffer      line   = D_ARG_BUFFER_INIT;
    struct d_arg_string_list words  = D_ARG_LIST_INIT;
    size_t                   count  = 0u;
    int                      status = (_parser != NULL)
                                      ? d_arg_internal_command_line(&line)
                                      : D_ARG_ERROR_BAD_ARGUMENT;

    // `argv` as it is, counted
    while ( (_argv != NULL) &&
            (_argv[count] != NULL) )
    {
        count++;
    }

    status = (status == D_ARG_OK)
             ? d_arg_split_line(d_arg_buffer_text(&line),
                                D_ARG_QUOTING_WINDOWS,
                                D_ARG_SPLIT_PROGRAM_NAME,
                                &words,
                                NULL)
             : status;
    // room for the NULL after the last
    status = ( (status == D_ARG_OK) &&
               (words.count == count) )
             ? d_arg_internal_string_reserve(&words,
                                             count + 1u)
             : status;
    d_arg_buffer_release(&line);

    // the recovered arguments, ending in NULL, replace those kept before
    if ( (status == D_ARG_OK) &&
         (words.count == count) )
    {
        words.items[count] = NULL;
        d_arg_string_list_release(&_parser->utf8_argv);
        _parser->utf8_argv = words;

        return _parser->utf8_argv.items;
    }

    // a copy refused is the parser's definition error
    if (status == D_ARG_ERROR_OUT_OF_MEMORY)
    {
        (void)d_arg_internal_fail_definition(&_parser->command,
                                             NULL,
                                             status,
                                             NULL);
    }

    d_arg_string_list_release(&words);

    return _argv;
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

//==============================================================================
// 4.  CONFIGURATION TEXT
//==============================================================================

/**
 * @brief Parses text read as a configuration file, as CLI11's _parse_stream.
 *
 * @param[in,out] _root  the root command.
 * @param[in]     _text  the text.
 * @return D_ARG_OK, or a failure, recorded.
 *
 * CLI11's _parse_stream: the text read with the root's format -- a malformed
 * quote or escape recorded in the reader's words -- its items parsed, the
 * root's use counted, its pre-parse step taken with the count of items, and
 * the processing that ends a parse.
 */
D_STATIC int
d_arg_internal_parse_text_stream(
    struct d_arg_command* _root,
    struct d_pack_text    _text
)
{
    struct d_arg_config_item_list items  = D_ARG_LIST_INIT;
    struct d_arg_buffer           error  = D_ARG_BUFFER_INIT;
    int                           status = d_arg_config_from_text(
                                               d_arg_command_config_format(
                                                   _root),
                                               _text,
                                               &items,
                                               &error);

    // the reading's failure, in its words
    if (status != D_ARG_OK)
    {
        status = d_arg_internal_fail(_root,
                                     NULL,
                                     status,
                                     (error.length > 0u) ? "%s" : NULL,
                                     error.data);
    }

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_config(_root,
                                           &items)
             : status;

    // counted, then pre-parsed with the count of items
    if (status == D_ARG_OK)
    {
        d_arg_internal_increment_parsed(_root);
        status = d_arg_internal_trigger_pre_parse(_root,
                                                  items.count);
    }

    status = (status == D_ARG_OK)
             ? d_arg_internal_process(_root)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_extras(_root)
             : status;
    d_arg_config_item_list_release(&items);
    d_arg_buffer_release(&error);

    return status;
}

/*
d_arg_parser_parse_config
  CLI11's parse_from_stream: the call started without clearing the root,
whose definitions are checked and configured unless it has parsed before;
then the text parsed as _parse_stream parses it, and the callbacks run.
*/
int
d_arg_parser_parse_config(
    struct d_arg_parser* _parser,
    struct d_pack_text   _text
)
{
    // validate the parameters
    if ( (!_parser)                 ||
         ( (!_text.data) &&
           (_text.length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_command* const root   = &_parser->command;
    int                         status = d_arg_internal_parse_resume(_parser);

    // a root that has not parsed is checked and configured first
    if ( (status == D_ARG_OK) &&
         (root->parsed == 0u) )
    {
        status = d_arg_internal_validate(root);

        if (status == D_ARG_OK)
        {
            d_arg_internal_configure(root);
        }
    }

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_text_stream(
                   root,
                   (_text.data != NULL)
                   ? _text
                   : d_arg_internal_span("",
                                         0u))
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_run_callback(root,
                                         false,
                                         false)
           : status;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
