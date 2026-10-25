/*******************************************************************************
* djinterp [parse]                                                arg_common.hpp
*
* The argument parser's C++ face: the vocabulary its headers share.
*   What arg_common.h is to the C headers, this is to the C++ ones: the
* namespace, djinterp::parse::cli; CLI11's enumerations as struct-scoped
* enums over the C values, so that a setting reads `cli::extras_mode::
* capture` and cannot be given where another kind is wanted; the status
* every fallible call and every parse returns, as a value rather than an
* exception; a view of a run of text; the string parameter every call
* takes; a view of an error report; and sinks that land help and messages in
* a std::string or a std::ostream.
*   Nothing here holds more than the C value it stands for: a status is the
* d_arg_status, a text is the d_pack_text it derives from, an error a
* pointer to the C report. A C++ call therefore costs what its C call costs.
*   The C core calls back into C++ -- to write to a std::string, to run a
* C++ callback -- through functions that run their work under
* internal::guard, which, in a build with exceptions, catches whatever the
* work throws and returns a status in its place: an exception crossing a C
* frame is undefined.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/arg_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Statuses
         1.  status_code
         2.  exit_code
    2.  Options
         1.  multi_option_policy
         2.  callback_priority
         3.  integer_rule
    3.  Commands
         1.  extras_mode
         2.  config_extras_mode
         3.  prefix_mode
    4.  Output
         1.  help_mode
         2.  config_output_mode
    5.  Input
         1.  classifier
         2.  quoting
2.  VALUES
    ------
    1.  Statuses
         1.  status
         2.  Comparing statuses
    2.  Text
         1.  text
         2.  Comparing and writing text
    3.  Parameters
         1.  c_string
    4.  Errors
         1.  error
    5.  Lists
         1.  basic_text_list
         2.  text_list / name_list
         3.  basic_handle_list
    6.  Buffers
         1.  buffer
3.  SINKS
    -----
    1.  Making sinks
         1.  string_sink / stream_sink
4.  INTERNALS
    ---------
    1.  The C boundary
         1.  guard
         2.  fallback
         3.  exception_words
    2.  Writing
         1.  string_append / stream_insert
         2.  string_sink_write / stream_sink_write
    3.  Callables
         1.  returns_tag
         2.  result_kind / result_probe
         3.  call_kind
         4.  call_status
    4.  Lists
         1.  list_text
    5.  Buffers
         1.  scratch
    6.  Checks
         1.  static_check
         2.  D_INTERNAL_ARG_ASSERT_DEPENDENT
5.  DEFINITIONS
    -----------
    1.  status
    2.  text
    3.  c_string
    4.  error
    5.  Lists
    6.  Buffers
    7.  Sinks
    8.  Internals
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMON_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMON_HPP 1

// std
#include <cstddef>    // std::size_t, std::ptrdiff_t
#include <cstring>    // std::memcmp, std::strlen
#include <exception>  // std::exception
#include <iterator>   // std::random_access_iterator_tag
#include <new>        // std::bad_alloc
#include <ostream>    // std::ostream
#include <string>     // std::string
#include <vector>     // std::vector
// djinterp
#include "../../../djinterp.hpp"          // framework root
#include "../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../../env/env.h"             // language levels
#include "./c/arg_common.h"               // statuses, enumerations,
                                          // d_arg_error
// std, from C++17
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>  // std::string_view
#endif


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  VOCABULARY
//==============================================================================
// CLI11's enumerations, struct-scoped as C++98 allows: each is a struct
// holding an enum named `value`, its enumerators the C values under CLI11's
// names in lower case -- `cli::multi_option_policy::take_last` is
// D_ARG_MULTI_TAKE_LAST, CLI11's MultiOptionPolicy::TakeLast. A setting takes
// the `value` type, so a value of another kind, or a bare integer, is
// refused at compile time.


// 1.1    Statuses
//------------------------------------------------------------------------------
// 1.1.1
// status_code
//   enum: the d_arg_status values, named for CLI11's error classes --
// call_for_help is CallForHelp, bad_name_string BadNameString -- with ok
// for D_ARG_OK and the last three the C library's own. See status.
struct status_code
{
    enum value
    {
        ok                     = D_ARG_OK,
        call_for_help          = D_ARG_CALL_FOR_HELP,
        call_for_all_help      = D_ARG_CALL_FOR_ALL_HELP,
        call_for_version       = D_ARG_CALL_FOR_VERSION,
        call_for_completion    = D_ARG_CALL_FOR_COMPLETION,
        success                = D_ARG_SUCCESS,
        runtime_error          = D_ARG_RUNTIME_ERROR,
        incorrect_construction = D_ARG_ERROR_INCORRECT_CONSTRUCTION,
        bad_name_string        = D_ARG_ERROR_BAD_NAME,
        option_already_added   = D_ARG_ERROR_OPTION_ALREADY_ADDED,
        file_error             = D_ARG_ERROR_FILE,
        conversion_error       = D_ARG_ERROR_CONVERSION,
        validation_error       = D_ARG_ERROR_VALIDATION,
        required_error         = D_ARG_ERROR_REQUIRED,
        requires_error         = D_ARG_ERROR_REQUIRES,
        excludes_error         = D_ARG_ERROR_EXCLUDES,
        extras_error           = D_ARG_ERROR_EXTRAS,
        config_error           = D_ARG_ERROR_CONFIG,
        invalid_error          = D_ARG_ERROR_INVALID,
        horrible_error         = D_ARG_ERROR_HORRIBLE,
        option_not_found       = D_ARG_ERROR_OPTION_NOT_FOUND,
        argument_mismatch      = D_ARG_ERROR_ARGUMENT_MISMATCH,
        parse_error            = D_ARG_ERROR_PARSE,
        out_of_memory          = D_ARG_ERROR_OUT_OF_MEMORY,
        bad_argument           = D_ARG_ERROR_BAD_ARGUMENT,
        unsupported            = D_ARG_ERROR_UNSUPPORTED
    };
};

// 1.1.2
// exit_code
//   enum: CLI11's ExitCodes, the d_arg_exit_code values a program ends with.
struct exit_code
{
    enum value
    {
        success                = D_ARG_EXIT_SUCCESS,
        incorrect_construction = D_ARG_EXIT_INCORRECT_CONSTRUCTION,
        bad_name_string        = D_ARG_EXIT_BAD_NAME_STRING,
        option_already_added   = D_ARG_EXIT_OPTION_ALREADY_ADDED,
        file_error             = D_ARG_EXIT_FILE_ERROR,
        conversion_error       = D_ARG_EXIT_CONVERSION_ERROR,
        validation_error       = D_ARG_EXIT_VALIDATION_ERROR,
        required_error         = D_ARG_EXIT_REQUIRED_ERROR,
        requires_error         = D_ARG_EXIT_REQUIRES_ERROR,
        excludes_error         = D_ARG_EXIT_EXCLUDES_ERROR,
        extras_error           = D_ARG_EXIT_EXTRAS_ERROR,
        config_error           = D_ARG_EXIT_CONFIG_ERROR,
        invalid_error          = D_ARG_EXIT_INVALID_ERROR,
        horrible_error         = D_ARG_EXIT_HORRIBLE_ERROR,
        option_not_found       = D_ARG_EXIT_OPTION_NOT_FOUND,
        argument_mismatch      = D_ARG_EXIT_ARGUMENT_MISMATCH,
        base_class             = D_ARG_EXIT_BASE_CLASS
    };
};

// 1.2    Options
//------------------------------------------------------------------------------
// 1.2.1
// multi_option_policy
//   enum: CLI11's MultiOptionPolicy, the d_arg_multi_option_policy values:
// what an option given more values than it expects keeps. CLI11's Throw is
// throw_error, `throw` being a keyword.
struct multi_option_policy
{
    enum value
    {
        throw_error = D_ARG_MULTI_THROW,
        take_last   = D_ARG_MULTI_TAKE_LAST,
        take_first  = D_ARG_MULTI_TAKE_FIRST,
        join        = D_ARG_MULTI_JOIN,
        take_all    = D_ARG_MULTI_TAKE_ALL,
        sum         = D_ARG_MULTI_SUM,
        reverse     = D_ARG_MULTI_REVERSE
    };
};

// 1.2.2
// callback_priority
//   enum: CLI11's CallbackPriority, the d_arg_callback_priority values: when
// an option's callback runs in the processing that follows parsing.
struct callback_priority
{
    enum value
    {
        first_pre_help            = D_ARG_PRIORITY_FIRST_PRE_HELP,
        first                     = D_ARG_PRIORITY_FIRST,
        pre_requirements_pre_help = D_ARG_PRIORITY_PRE_REQUIREMENTS_PRE_HELP,
        pre_requirements          = D_ARG_PRIORITY_PRE_REQUIREMENTS,
        normal_pre_help           = D_ARG_PRIORITY_NORMAL_PRE_HELP,
        normal                    = D_ARG_PRIORITY_NORMAL,
        last_pre_help             = D_ARG_PRIORITY_LAST_PRE_HELP,
        last                      = D_ARG_PRIORITY_LAST
    };
};

// 1.2.3
// integer_rule
//   enum: the d_arg_integer_rule values: whether an integer with a leading
// zero reads as decimal, or as octal, as C and CLI11 read it.
struct integer_rule
{
    enum value
    {
        decimal = D_ARG_INTEGER_DECIMAL,
        c       = D_ARG_INTEGER_C
    };
};

// 1.3    Commands
//------------------------------------------------------------------------------
// 1.3.1
// extras_mode
//   enum: CLI11's ExtrasMode, the d_arg_extras_mode values: what a command
// does with an argument it does not recognize.
struct extras_mode
{
    enum value
    {
        error             = D_ARG_EXTRAS_ERROR,
        error_immediately = D_ARG_EXTRAS_ERROR_IMMEDIATELY,
        ignore            = D_ARG_EXTRAS_IGNORE,
        assume_single     = D_ARG_EXTRAS_ASSUME_SINGLE,
        assume_multiple   = D_ARG_EXTRAS_ASSUME_MULTIPLE,
        capture           = D_ARG_EXTRAS_CAPTURE
    };
};

// 1.3.2
// config_extras_mode
//   enum: CLI11's ConfigExtrasMode, the d_arg_config_extras_mode values: what a
// command does with a configuration entry no option claims.
struct config_extras_mode
{
    enum value
    {
        error      = D_ARG_CONFIG_EXTRAS_ERROR,
        ignore     = D_ARG_CONFIG_EXTRAS_IGNORE,
        ignore_all = D_ARG_CONFIG_EXTRAS_IGNORE_ALL,
        capture    = D_ARG_CONFIG_EXTRAS_CAPTURE
    };
};

// 1.3.3
// prefix_mode
//   enum: CLI11's PrefixCommandMode, the d_arg_prefix_mode values: where a
// command stops parsing and keeps the rest of the arguments.
struct prefix_mode
{
    enum value
    {
        off             = D_ARG_PREFIX_OFF,
        separator_only  = D_ARG_PREFIX_SEPARATOR_ONLY,
        on              = D_ARG_PREFIX_ON,
        positional_only = D_ARG_PREFIX_POSITIONAL_ONLY
    };
};

// 1.4    Output
//------------------------------------------------------------------------------
// 1.4.1
// help_mode
//   enum: CLI11's AppFormatMode, the d_arg_help_mode values: a command's own
// help, help with every subcommand expanded, or a subcommand's section.
struct help_mode
{
    enum value
    {
        normal = D_ARG_HELP_NORMAL,
        all    = D_ARG_HELP_ALL,
        sub    = D_ARG_HELP_SUB
    };
};

// 1.4.2
// config_output_mode
//   enum: CLI11's ConfigOutputMode, the d_arg_config_output_mode values: which
// values a written configuration holds.
struct config_output_mode
{
    enum value
    {
        active                     = D_ARG_CONFIG_OUTPUT_ACTIVE,
        all_defaults               = D_ARG_CONFIG_OUTPUT_ALL_DEFAULTS,
        active_subcommand_defaults =
            D_ARG_CONFIG_OUTPUT_ACTIVE_SUBCOMMAND_DEFAULTS
    };
};

// 1.5    Input
//------------------------------------------------------------------------------
// 1.5.1
// classifier
//   enum: CLI11's detail::Classifier, the d_arg_classifier values: what an
// argument a command did not use was taken to be. CLI11's SHORT and LONG
// are short_name and long_name, `short` and `long` being keywords.
struct classifier
{
    enum value
    {
        none                  = D_ARG_CLASS_NONE,
        positional_mark       = D_ARG_CLASS_POSITIONAL_MARK,
        short_name            = D_ARG_CLASS_SHORT,
        long_name             = D_ARG_CLASS_LONG,
        windows_style         = D_ARG_CLASS_WINDOWS_STYLE,
        subcommand            = D_ARG_CLASS_SUBCOMMAND,
        subcommand_terminator = D_ARG_CLASS_SUBCOMMAND_TERMINATOR
    };
};

// 1.5.2
// quoting
//   enum: the d_arg_quoting values: the rules a line of text is split into
// arguments by; native and native_response are the platform's choices.
struct quoting
{
    enum value
    {
        posix           = D_ARG_QUOTING_POSIX,
        windows         = D_ARG_QUOTING_WINDOWS,
        cli11           = D_ARG_QUOTING_CLI11,
        gnu             = D_ARG_QUOTING_GNU,
        native          = D_ARG_QUOTING_NATIVE,
        native_response = D_ARG_QUOTING_NATIVE_RESPONSE
    };
};


//==============================================================================
// 2.  VALUES
//==============================================================================


// 2.1    Statuses
//------------------------------------------------------------------------------
// 2.1.1
// status
//   class: what a call or a parse came to -- a d_arg_status held as a value,
// which is how the C++ face reports where CLI11 throws. ok() is true for
// status_code::ok alone; a completed request -- help, a version, a
// completion script, CLI11's Success -- is not ok() and not an error, and,
// like an error, goes to arg_parser::exit, which writes what it calls for
// and gives the code to end with. `explicit operator bool`, from C++11, is
// ok().
class status
{
public:
    // making: success; a status_code, implicitly, so that a callback may
    // `return cli::status_code::validation_error;`; any int, explicitly,
    // as a C call returns it
    status() D_NOEXCEPT;
    status(status_code::value _code) D_NOEXCEPT;
    explicit status(int _code) D_NOEXCEPT;

    // reading: none allocates or fails
    bool        ok() const D_NOEXCEPT;
    bool        is_error() const D_NOEXCEPT;
    int         code() const D_NOEXCEPT;
    const char* get_name() const D_NOEXCEPT;
    int         get_exit_code() const D_NOEXCEPT;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_EXPLICIT_BOOL operator bool() const D_NOEXCEPT;
#endif

private:
    int m_code;
};

// 2.1.2
// Comparing statuses
//   functions: two statuses are equal when their codes are; a status_code
// compares as the status it makes.
bool operator==(const status& _first,
                const status& _second) D_NOEXCEPT;
bool operator!=(const status& _first,
                const status& _second) D_NOEXCEPT;

// 2.2    Text
//------------------------------------------------------------------------------
// 2.2.1
// text
//   class: a run of bytes viewed, not owned -- a d_pack_text, which it derives
// from and adds nothing to, so it passes wherever the C API takes a span.
// Values, results and names come back as text; it compares by content,
// converts to a std::string when one is wanted, and, from C++17, to and
// from a std::string_view. It may hold NUL bytes, and is not NUL-terminated
// unless what it views is; it is valid while what it views is.
//   Its data() and length() hide the members of the same names, which
// remain readable as `d_pack_text::data` and `d_pack_text::length`.
class text : public ::d_pack_text
{
public:
    typedef const char* const_iterator;
    typedef std::size_t size_type;

    // making: empty; a C string, NULL reading as empty; a span; a
    // std::string's whole content, NUL included past the end; a C span
    text() D_NOEXCEPT;
    text(const char* _text) D_NOEXCEPT;
    text(const char* _data,
         size_type   _length) D_NOEXCEPT;
    text(const std::string& _text) D_NOEXCEPT;
    text(const ::d_pack_text& _text) D_NOEXCEPT;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    text(std::string_view _text) D_NOEXCEPT;
#endif

    // reading: none allocates or fails; operator[] reads within the length
    const char*    data() const D_NOEXCEPT;
    size_type      size() const D_NOEXCEPT;
    size_type      length() const D_NOEXCEPT;
    bool           empty() const D_NOEXCEPT;
    const_iterator begin() const D_NOEXCEPT;
    const_iterator end() const D_NOEXCEPT;
    char           operator[](size_type _index) const D_NOEXCEPT;
    int            compare(const text& _other) const D_NOEXCEPT;

    /**
     * @brief Copies the text into a std::string.
     *
     * @return the copy; std::bad_alloc, as std::string throws it, when it
     *         cannot be made.
     */
    std::string    str() const;
    /**
     * @brief Copies the text into a std::string, implicitly, so that
     *        `std::string value = option_value;` reads as it does in CLI11.
     *
     * @return the copy, as str() makes it.
     */
    operator std::string() const;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    operator std::string_view() const D_NOEXCEPT;
#endif
};

// 2.2.2
// Comparing and writing text
//   functions: comparisons byte by byte, a shorter text that is a prefix of a
// longer one first -- a C string or a std::string compares as the text it
// makes -- and writing a text to a stream, NUL bytes and all. None
// allocates; the comparisons do not fail.
bool          operator==(const text& _first,
                         const text& _second) D_NOEXCEPT;
bool          operator!=(const text& _first,
                         const text& _second) D_NOEXCEPT;
bool          operator<(const text& _first,
                        const text& _second) D_NOEXCEPT;
bool          operator<=(const text& _first,
                         const text& _second) D_NOEXCEPT;
bool          operator>(const text& _first,
                        const text& _second) D_NOEXCEPT;
bool          operator>=(const text& _first,
                         const text& _second) D_NOEXCEPT;
std::ostream& operator<<(std::ostream& _stream,
                         const text&   _text);

// 2.3    Parameters
//------------------------------------------------------------------------------
// 2.3.1
// c_string
//   class: the type of the string parameters that become a C string -- names,
// descriptions, groups -- made, implicitly, from a C string, NULL
// included, or from a std::string, whose c_str() it keeps: one pointer,
// valid for the call it is passed to, which is all the C call needs. A
// std::string_view is not taken, holding no terminator, as CLI11's
// std::string parameters do not take one.
class c_string
{
public:
    c_string() D_NOEXCEPT;
    c_string(const char* _text) D_NOEXCEPT;
    c_string(const std::string& _text) D_NOEXCEPT;

    const char* get() const D_NOEXCEPT;

private:
    const char* m_text;
};

// 2.4    Errors
//------------------------------------------------------------------------------
// 2.4.1
// error
//   class: a view of an error report -- the last parse's, the parser's
// definition error, or the one a failure message is asked to describe --
// read as CLI11's Error is read: get_name(), get_exit_code(), what(). The
// report is the C library's, so the view holds a pointer to it and is valid
// while the report is: until the next parse, clear or release.
class error
{
public:
    explicit error(const ::d_arg_error& _error) D_NOEXCEPT;

    // reading: what() is never NULL -- the message, or the status's own
    // text -- and none allocates or fails
    status               get_status() const D_NOEXCEPT;
    int                  get_exit_code() const D_NOEXCEPT;
    const char*          get_name() const D_NOEXCEPT;
    const char*          what() const D_NOEXCEPT;
    const ::d_arg_error& get() const D_NOEXCEPT;

private:
    const ::d_arg_error* m_error;
};

// 2.5    Lists
//------------------------------------------------------------------------------
// 2.5.1
// basic_text_list
//   class: a view of an array the C library keeps -- an option's values, a
// command's names -- read as CLI11's std::vector<std::string> is read: its
// size, its elements by index and by iterator, each element a text, and,
// implicitly, a std::vector<std::string> copy. Item is the array's element
// type, a d_pack_text or a C string. It is valid while the array is: the
// option's values until it changes, the names until they do.
template<typename Item>
class basic_text_list
{
public:
    typedef std::size_t size_type;
    typedef text        value_type;

    // const_iterator
    //   class: a random-access iterator whose elements are texts, made as
    //   they are read.
    class const_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef text                            value_type;
        typedef std::ptrdiff_t                  difference_type;
        typedef const text*                     pointer;
        typedef text                            reference;

        const_iterator() D_NOEXCEPT;
        explicit const_iterator(const Item* _at) D_NOEXCEPT;

        text            operator*() const D_NOEXCEPT;
        text            operator[](difference_type _offset) const D_NOEXCEPT;
        const_iterator& operator++() D_NOEXCEPT;
        const_iterator  operator++(int) D_NOEXCEPT;
        const_iterator& operator--() D_NOEXCEPT;
        const_iterator  operator--(int) D_NOEXCEPT;
        const_iterator& operator+=(difference_type _offset) D_NOEXCEPT;
        const_iterator& operator-=(difference_type _offset) D_NOEXCEPT;
        const_iterator  operator+(difference_type _offset) const D_NOEXCEPT;
        const_iterator  operator-(difference_type _offset) const D_NOEXCEPT;
        difference_type operator-(const const_iterator& _other) const
                            D_NOEXCEPT;
        bool            operator==(const const_iterator& _other) const
                            D_NOEXCEPT;
        bool            operator!=(const const_iterator& _other) const
                            D_NOEXCEPT;
        bool            operator<(const const_iterator& _other) const
                            D_NOEXCEPT;

    private:
        const Item* m_at;
    };

    typedef const_iterator iterator;

    // making: empty, or over an array the C library holds
    basic_text_list() D_NOEXCEPT;
    basic_text_list(const Item* _items,
                    size_type   _count) D_NOEXCEPT;

    // reading: none allocates or fails; operator[] reads within the size
    size_type      size() const D_NOEXCEPT;
    bool           empty() const D_NOEXCEPT;
    text           operator[](size_type _index) const D_NOEXCEPT;
    text           front() const D_NOEXCEPT;
    text           back() const D_NOEXCEPT;
    const_iterator begin() const D_NOEXCEPT;
    const_iterator end() const D_NOEXCEPT;
    const Item*    data() const D_NOEXCEPT;

    /**
     * @brief Copies the list into a std::vector<std::string>, implicitly, so
     *        that `std::vector<std::string> values = option->results();`
     *        reads as it does in CLI11.
     *
     * @return the copy; std::bad_alloc, as std::vector and std::string throw
     *         it, when it cannot be made.
     */
    operator std::vector<std::string>() const;

private:
    const Item* m_items;
    size_type   m_count;
};

// 2.5.2
// text_list / name_list
//   types: the views of the C library's two kinds of array: spans -- an
// option's values, results and kept text -- and C strings -- the names of
// an option or a command.
typedef basic_text_list< ::d_pack_text> text_list;
typedef basic_text_list<char*>          name_list;

// 2.5.3
// basic_handle_list
//   class: a view of an array of the C library's records -- the options an
// option needs, a command's subcommands -- whose elements are handles, made
// as they are read: Item is the record type, Handle the handle over it. It
// reads as CLI11's std::vector<App*> and std::set<Option*> are read: its
// size, its elements by index and by iterator -- and so, from C++11, by a
// range-based for -- each element a handle. Valid while the array is.
template<typename Handle,
         typename Item>
class basic_handle_list
{
public:
    typedef std::size_t size_type;
    typedef Handle      value_type;

    // const_iterator
    //   class: a random-access iterator whose elements are handles, made as
    //   they are read.
    class const_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef Handle                          value_type;
        typedef std::ptrdiff_t                  difference_type;
        typedef const Handle*                   pointer;
        typedef Handle                          reference;

        const_iterator() D_NOEXCEPT;
        explicit const_iterator(Item* const* _at) D_NOEXCEPT;

        Handle          operator*() const D_NOEXCEPT;
        Handle          operator[](difference_type _offset) const D_NOEXCEPT;
        const_iterator& operator++() D_NOEXCEPT;
        const_iterator  operator++(int) D_NOEXCEPT;
        const_iterator& operator--() D_NOEXCEPT;
        const_iterator  operator--(int) D_NOEXCEPT;
        const_iterator& operator+=(difference_type _offset) D_NOEXCEPT;
        const_iterator& operator-=(difference_type _offset) D_NOEXCEPT;
        const_iterator  operator+(difference_type _offset) const D_NOEXCEPT;
        const_iterator  operator-(difference_type _offset) const D_NOEXCEPT;
        difference_type operator-(const const_iterator& _other) const
                            D_NOEXCEPT;
        bool            operator==(const const_iterator& _other) const
                            D_NOEXCEPT;
        bool            operator!=(const const_iterator& _other) const
                            D_NOEXCEPT;
        bool            operator<(const const_iterator& _other) const
                            D_NOEXCEPT;

    private:
        Item* const* m_at;
    };

    typedef const_iterator iterator;

    basic_handle_list() D_NOEXCEPT;
    basic_handle_list(Item* const* _items,
                      size_type    _count) D_NOEXCEPT;

    // reading: none allocates or fails; operator[] reads within the size;
    // data() is the array, for a C call taking the records
    size_type      size() const D_NOEXCEPT;
    bool           empty() const D_NOEXCEPT;
    Handle         operator[](size_type _index) const D_NOEXCEPT;
    const_iterator begin() const D_NOEXCEPT;
    const_iterator end() const D_NOEXCEPT;
    Item* const*   data() const D_NOEXCEPT;

    /**
     * @brief Copies the handles into a std::vector, as CLI11's lists of
     *        pointers are copied.
     *
     * @return the handles; std::bad_alloc, as std::vector throws it, when
     *         they cannot be held.
     */
    std::vector<Handle> to_vector() const;

private:
    Item* const* m_items;
    size_type    m_count;
};

// 2.6    Buffers
//------------------------------------------------------------------------------
// 2.6.1
// buffer
//   class: a view of a d_arg_buffer the C library builds text in -- help, a
// configuration -- that a step of the caller's appends to: what CLI11's
// make_* and to_config return as a std::string, here written into the text
// being built, without a copy. Each append returns status_code::ok, or
// out_of_memory once the buffer could not grow, after which every append
// does nothing; what was appended stays. Valid while the buffer is.
class buffer
{
public:
    explicit buffer(::d_arg_buffer& _buffer) D_NOEXCEPT;

    // appending
    status append(const text& _text) D_NOEXCEPT;
    status append(char _character) D_NOEXCEPT;

    // reading: the text built so far, as a view valid until the next append,
    // or copied
    std::size_t size() const D_NOEXCEPT;
    bool        empty() const D_NOEXCEPT;
    text        view() const D_NOEXCEPT;
    std::string str() const;

    // taking back: the text cut to its first `_length` bytes
    void truncate(std::size_t _length) D_NOEXCEPT;

    // the C buffer, for the C calls
    ::d_arg_buffer& get() const D_NOEXCEPT;

private:
    ::d_arg_buffer* m_buffer;
};


//==============================================================================
// 3.  SINKS
//==============================================================================


// 3.1    Making sinks
//------------------------------------------------------------------------------
// 3.1.1
// string_sink / stream_sink
//   functions: sinks over a std::string, which they append to, and over a
// std::ostream, which they write with write(); either must outlive the
// sink's use. A write the string cannot take -- it could not grow -- or the
// stream refuses -- it is, or becomes, not good() -- is refused whole, and
// the C call writing it reports D_ARG_ERROR_FILE, as for any sink; an
// exception either throws is caught. Neither allocates or fails.
::d_pack_sink string_sink(std::string& _text) D_NOEXCEPT;
::d_pack_sink stream_sink(std::ostream& _stream) D_NOEXCEPT;


//==============================================================================
// 4.  INTERNALS
//==============================================================================


// 4.1    The C boundary
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.1.1
    // guard
    /**
     * @brief Runs work the C core asked of C++, so that nothing is thrown
     *        through a C frame.
     *
     * @note Every function the C core calls into C++ through runs its work
     *       here. With C++ exceptions -- D_INTERNAL_ARG_PARSER_CPP_CATCH --
     *       an exception the work lets out is caught and given to
     *       `_handler`, with whether it is std::bad_alloc: a std::exception
     *       as itself, anything else as NULL. Without them, the work's result
     *       is returned as it is.
     *
     * @param[in] _work     a function object taking nothing, with a
     *                      `result_type`.
     * @param[in] _handler  a function object taking a `const
     *                      std::exception*` and a `bool` -- the exception,
     *                      and whether memory ran out -- and returning the
     *                      work's result type: what to return instead.
     * @return the work's result, or the handler's for an exception.
     */
    template<typename Work,
             typename Handler>
    typename Work::result_type guard(const Work&    _work,
                                     const Handler& _handler);

    // 4.1.2
    // fallback
    //   class: the handler of guard that answers every exception with one
    //   value.
    template<typename Result>
    class fallback
    {
    public:
        typedef Result result_type;

        explicit fallback(Result _value);

        Result operator()(const std::exception* _exception,
                          bool                  _out_of_memory) const;

    private:
        Result m_value;
    };

    // 4.1.3
    // exception_words
    //   function: what an exception guard caught has to say, for a handler
    //   that makes it a message or a reason: its what(), when that holds
    //   anything; NULL for an exception of no standard type or an empty
    //   what(), so that the status it becomes speaks for itself -- and a
    //   value a validator throws over is not passed for want of a reason.
    const char* exception_words(const std::exception* _exception) D_NOEXCEPT;

NS_END  // internal

// 4.2    Writing
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.2.1
    // string_append / stream_insert
    //   class: the work string_sink_write and stream_sink_write guard: append
    //   the bytes to the std::string, or write them to the std::ostream,
    //   returning how many were taken.
    class string_append
    {
    public:
        typedef std::size_t result_type;

        string_append(void*       _text,
                      const void* _data,
                      std::size_t _size) D_NOEXCEPT;

        std::size_t operator()() const;

    private:
        std::string* m_text;
        const char*  m_data;
        std::size_t  m_size;
    };
    class stream_insert
    {
    public:
        typedef std::size_t result_type;

        stream_insert(void*       _stream,
                      const void* _data,
                      std::size_t _size) D_NOEXCEPT;

        std::size_t operator()() const;

    private:
        std::ostream* m_stream;
        const char*   m_data;
        std::size_t   m_size;
    };

    // 4.2.2
    // string_sink_write / stream_sink_write
    //   functions: the write functions of string_sink and stream_sink, as the
    //   sink calls them: every byte taken, or none. Neither throws.
    std::size_t string_sink_write(void*       _context,
                                  const void* _data,
                                  std::size_t _size);
    std::size_t stream_sink_write(void*       _context,
                                  const void* _data,
                                  std::size_t _size);

NS_END  // internal

// 4.3    Callables
//------------------------------------------------------------------------------
//   A callback of the caller's may return nothing, a cli::status -- or a
// status_code -- or, where CLI11's returns one, a bool. What it returns is
// told at compile time, at every language level, without decltype: in
// `(call, returns_tag())`, a call returning void leaves the built-in comma,
// whose result is the tag, while any other finds the comma operator below,
// whose result is the call's own; the probe's overloads then tell which,
// by the size of the array each returns. Nothing here is ever called but
// the dispatch of 4.3.4.
NS_INTERNAL

    // 4.3.1
    // returns_tag
    //   struct: the right operand of the comma that tells a call returning
    //   void from any other; the comma operator over any result and the tag,
    //   declared and never defined.
    struct returns_tag
    {};

    template<typename Result>
    const Result& operator,(const Result& _result,
                            returns_tag   _tag);

    // 4.3.2
    // result_kind / result_probe
    //   enum, functions: the kinds of result -- nothing, a status, a truth
    //   value, anything else -- and the overloads whose array sizes are
    //   those kinds; declared and never defined.
    struct result_kind
    {
        enum value
        {
            none   = 1,
            status = 2,
            truth  = 3,
            other  = 4
        };
    };

    char (&result_probe(returns_tag _result))[result_kind::none];
    char (&result_probe(const cli::status& _result))[result_kind::status];
    char (&result_probe(status_code::value _result))[result_kind::status];
    char (&result_probe(bool _result))[result_kind::truth];
    template<typename Other>
    char (&result_probe(const Other& _result))[result_kind::other];

    // make
    //   function: std::declval, at every language level: an lvalue of the
    //   type, in an unevaluated operand only; declared and never defined.
    template<typename Type>
    Type& make();

    // 4.3.3
    // call_kind
    //   traits: the result_kind of calling a Function with no argument, or
    //   with lvalues of the types given, as `value`.
    template<typename Function>
    struct call_kind0
    {
        enum
        {
            value = sizeof(result_probe((make<Function>()(),
                                         returns_tag())))
        };
    };

    template<typename Function,
             typename First>
    struct call_kind1
    {
        enum
        {
            value = sizeof(result_probe((make<Function>()(make<First>()),
                                         returns_tag())))
        };
    };

    template<typename Function,
             typename First,
             typename Second>
    struct call_kind2
    {
        enum
        {
            value = sizeof(result_probe((make<Function>()(make<First>(),
                                                          make<Second>()),
                                         returns_tag())))
        };
    };

    template<typename Function,
             typename First,
             typename Second,
             typename Third>
    struct call_kind3
    {
        enum
        {
            value = sizeof(result_probe((make<Function>()(make<First>(),
                                                          make<Second>(),
                                                          make<Third>()),
                                         returns_tag())))
        };
    };

    // 4.3.4
    // call_status
    //   class: a call of the kind given, made, and its result given as a
    //   d_arg_status: nothing is D_ARG_OK; a status is its code; a truth
    //   value is D_ARG_OK, or, false, D_ARG_ERROR_CONVERSION, as CLI11's
    //   callback_t returning false is a ConversionError. A call of any other
    //   kind has no specialization, so it does not compile.
    template<int Kind>
    struct call_status;

    template<>
    struct call_status<result_kind::none>
    {
        template<typename Function>
        static int call(Function& _function);
        template<typename Function,
                 typename First>
        static int call(Function& _function,
                        First&    _first);
        template<typename Function,
                 typename First,
                 typename Second>
        static int call(Function& _function,
                        First&    _first,
                        Second&   _second);
        template<typename Function,
                 typename First,
                 typename Second,
                 typename Third>
        static int call(Function& _function,
                        First&    _first,
                        Second&   _second,
                        Third&    _third);
    };

    template<>
    struct call_status<result_kind::status>
    {
        template<typename Function>
        static int call(Function& _function);
        template<typename Function,
                 typename First>
        static int call(Function& _function,
                        First&    _first);
        template<typename Function,
                 typename First,
                 typename Second>
        static int call(Function& _function,
                        First&    _first,
                        Second&   _second);
        template<typename Function,
                 typename First,
                 typename Second,
                 typename Third>
        static int call(Function& _function,
                        First&    _first,
                        Second&   _second,
                        Third&    _third);
    };

    template<>
    struct call_status<result_kind::truth>
    {
        template<typename Function>
        static int call(Function& _function);
        template<typename Function,
                 typename First>
        static int call(Function& _function,
                        First&    _first);
        template<typename Function,
                 typename First,
                 typename Second>
        static int call(Function& _function,
                        First&    _first,
                        Second&   _second);
        template<typename Function,
                 typename First,
                 typename Second,
                 typename Third>
        static int call(Function& _function,
                        First&    _first,
                        Second&   _second,
                        Third&    _third);
    };

NS_END  // internal

// 4.4    Lists
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.4.1
    // list_text
    //   functions: an element of each kind of C array as a text: a span as
    //   it is, a C string by its length. Neither allocates nor fails.
    text list_text(const ::d_pack_text& _item) D_NOEXCEPT;
    text list_text(const char* _item) D_NOEXCEPT;

NS_END  // internal

// 4.5    Buffers
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.5.1
    // scratch
    //   class: a d_arg_buffer released when it goes, for the C calls that
    //   write into one -- names, help, descriptions -- whose content is then
    //   copied into a std::string.
    class scratch
    {
    public:
        scratch() D_NOEXCEPT;
        ~scratch();

        ::d_arg_buffer* get() D_NOEXCEPT;
        std::string     str() const;

    private:
        D_DELETED_FN(scratch(const scratch& _other))
        D_DELETED_FN(scratch& operator=(const scratch& _other))

        ::d_arg_buffer m_buffer;
    };

NS_END  // internal

// 4.6    Checks
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.6.1
    // static_check
    //   trait: a condition checked at compile time below C++11: complete,
    //   with `value` 1, where it holds, and incomplete where it does not, so
    //   that sizeof refuses it; declared and never defined for false.
    template<bool Holds>
    struct static_check;

    template<>
    struct static_check<true>
    {
        enum
        {
            value = 1
        };
    };

NS_END  // internal

// 4.6.2
// D_INTERNAL_ARG_ASSERT_DEPENDENT
//   macro: D_STATIC_ASSERT for a condition a function template's parameters
// decide, as a statement in its body: static_assert, through D_STATIC_ASSERT,
// from C++11; below, sizeof of internal::static_check, which a failed
// condition leaves incomplete -- since a fallback that sizes an array by the
// condition is given a dependent size there, which compilers judge only once
// the template is made, and may warn of as a sign conversion.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #define D_INTERNAL_ARG_ASSERT_DEPENDENT(c, m) D_STATIC_ASSERT(c, m)
#else
    #define D_INTERNAL_ARG_ASSERT_DEPENDENT(c, m)                              \
        static_cast<void>(                                                     \
            sizeof(::djinterp::parse::cli::internal::static_check<(c)>))
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


//==============================================================================
// 5.  DEFINITIONS
//==============================================================================


// 5.1    status
//------------------------------------------------------------------------------
// status::status
//   D_ARG_OK, the given code, or the int a C call returned.
inline
status::status() D_NOEXCEPT
    : m_code(D_ARG_OK)
{}

inline
status::status(
    status_code::value _code
) D_NOEXCEPT
    : m_code(static_cast<int>(_code))
{}

inline
status::status(
    int _code
) D_NOEXCEPT
    : m_code(_code)
{}

// status::ok
//   D_ARG_OK alone goes on.
inline bool
status::ok() const D_NOEXCEPT
{
    return (m_code == static_cast<int>(D_ARG_OK));
}

// status::is_error
//   d_arg_status_is_error: the runtime error and every D_ARG_ERROR_* status.
inline bool
status::is_error() const D_NOEXCEPT
{
    return d_arg_status_is_error(m_code);
}

// status::code
//   The d_arg_status value.
inline int
status::code() const D_NOEXCEPT
{
    return m_code;
}

// status::get_name
//   CLI11's error class name for the status, as d_arg_status_name gives it.
inline const char*
status::get_name() const D_NOEXCEPT
{
    return d_arg_status_name(m_code);
}

// status::get_exit_code
//   CLI11's exit code for the status, as d_arg_status_exit_code gives it.
inline int
status::get_exit_code() const D_NOEXCEPT
{
    return d_arg_status_exit_code(m_code);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// status::operator bool
//   ok(), for `if (!result)`.
inline
status::operator bool() const D_NOEXCEPT
{
    return ok();
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// operator== / operator!= (status)
//   By code.
inline bool
operator==(
    const status& _first,
    const status& _second
) D_NOEXCEPT
{
    return (_first.code() == _second.code());
}

inline bool
operator!=(
    const status& _first,
    const status& _second
) D_NOEXCEPT
{
    return (_first.code() != _second.code());
}

// 5.2    text
//------------------------------------------------------------------------------
// text::text
//   The base's two members set directly: empty is a NULL pointer and no
// length, as D_PACK_TEXT_NONE; a std::string is viewed through c_str(), so
// that a terminator follows the view, as the C calls that take an argument
// whole expect.
inline
text::text() D_NOEXCEPT
{
    this->d_pack_text::data   = D_NULLPTR;
    this->d_pack_text::length = 0u;
}

inline
text::text(
    const char* _text
) D_NOEXCEPT
{
    this->d_pack_text::data   = _text;
    this->d_pack_text::length = (_text != D_NULLPTR) ? std::strlen(_text) : 0u;
}

inline
text::text(
    const char* _data,
    size_type   _length
) D_NOEXCEPT
{
    this->d_pack_text::data   = _data;
    this->d_pack_text::length = _length;
}

inline
text::text(
    const std::string& _text
) D_NOEXCEPT
{
    this->d_pack_text::data   = _text.c_str();
    this->d_pack_text::length = _text.size();
}

inline
text::text(
    const ::d_pack_text& _text
) D_NOEXCEPT
{
    this->d_pack_text::data   = _text.data;
    this->d_pack_text::length = _text.length;
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

inline
text::text(
    std::string_view _text
) D_NOEXCEPT
{
    this->d_pack_text::data   = _text.data();
    this->d_pack_text::length = _text.size();
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

// text::data / size / length / empty / begin / end / operator[]
//   The base's members, read.
inline const char*
text::data() const D_NOEXCEPT
{
    return this->d_pack_text::data;
}

inline text::size_type
text::size() const D_NOEXCEPT
{
    return this->d_pack_text::length;
}

inline text::size_type
text::length() const D_NOEXCEPT
{
    return this->d_pack_text::length;
}

inline bool
text::empty() const D_NOEXCEPT
{
    return (this->d_pack_text::length == 0u);
}

inline text::const_iterator
text::begin() const D_NOEXCEPT
{
    return this->d_pack_text::data;
}

inline text::const_iterator
text::end() const D_NOEXCEPT
{
    // an empty text may view nothing at all, which has no end to add to
    return (this->d_pack_text::length == 0u)
           ? this->d_pack_text::data
           : (this->d_pack_text::data + this->d_pack_text::length);
}

inline char
text::operator[](
    size_type _index
) const D_NOEXCEPT
{
    return this->d_pack_text::data[_index];
}

// text::compare
//   memcmp over the shorter length, then the lengths: as std::string's
// compare orders bytes, as unsigned char.
inline int
text::compare(
    const text& _other
) const D_NOEXCEPT
{
    const size_type shorter = (size() < _other.size())
                              ? size()
                              : _other.size();
    const int       order   = (shorter == 0u)
                              ? 0
                              : std::memcmp(data(),
                                            _other.data(),
                                            shorter);

    // the bytes they share decide, then the length
    if (order != 0)
    {
        return order;
    }

    return (size() < _other.size())
           ? -1
           : ((size() > _other.size()) ? 1 : 0);
}

// text::str / operator std::string
//   A copy; an empty text makes an empty string without reading its pointer,
// which may be NULL.
inline std::string
text::str() const
{
    return empty()
           ? std::string()
           : std::string(data(),
                         size());
}

inline
text::operator std::string() const
{
    return str();
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// text::operator std::string_view
//   The same bytes, viewed by the standard's view.
inline
text::operator std::string_view() const D_NOEXCEPT
{
    return empty()
           ? std::string_view()
           : std::string_view(data(),
                              size());
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

// operator== and kin (text)
//   Equality by length, then bytes; order by compare().
inline bool
operator==(
    const text& _first,
    const text& _second
) D_NOEXCEPT
{
    return ( (_first.size() == _second.size()) &&
             (_first.compare(_second) == 0) );
}

inline bool
operator!=(
    const text& _first,
    const text& _second
) D_NOEXCEPT
{
    return !(_first == _second);
}

inline bool
operator<(
    const text& _first,
    const text& _second
) D_NOEXCEPT
{
    return (_first.compare(_second) < 0);
}

inline bool
operator<=(
    const text& _first,
    const text& _second
) D_NOEXCEPT
{
    return (_first.compare(_second) <= 0);
}

inline bool
operator>(
    const text& _first,
    const text& _second
) D_NOEXCEPT
{
    return (_first.compare(_second) > 0);
}

inline bool
operator>=(
    const text& _first,
    const text& _second
) D_NOEXCEPT
{
    return (_first.compare(_second) >= 0);
}

// operator<< (text)
//   write(), so that NUL bytes are written too.
inline std::ostream&
operator<<(
    std::ostream& _stream,
    const text&   _text
)
{
    // an empty text writes nothing, and may hold no pointer to write from
    if (!_text.empty())
    {
        _stream.write(_text.data(),
                      static_cast<std::streamsize>(_text.size()));
    }

    return _stream;
}

// 5.3    c_string
//------------------------------------------------------------------------------
// c_string::c_string
//   NULL, the C string, or the std::string's c_str().
inline
c_string::c_string() D_NOEXCEPT
    : m_text(D_NULLPTR)
{}

inline
c_string::c_string(
    const char* _text
) D_NOEXCEPT
    : m_text(_text)
{}

inline
c_string::c_string(
    const std::string& _text
) D_NOEXCEPT
    : m_text(_text.c_str())
{}

// c_string::get
//   The pointer the C call takes.
inline const char*
c_string::get() const D_NOEXCEPT
{
    return m_text;
}

// 5.4    error
//------------------------------------------------------------------------------
// error::error
//   The report's address.
inline
error::error(
    const ::d_arg_error& _error
) D_NOEXCEPT
    : m_error(&_error)
{}

// error::get_status / get_exit_code / get_name / what / get
//   The report's fields, through the C calls where there are any:
// d_arg_error_exit_code gives a runtime error's chosen code, and
// d_arg_error_message the status's own text when no message was recorded.
inline status
error::get_status() const D_NOEXCEPT
{
    return status(m_error->status);
}

inline int
error::get_exit_code() const D_NOEXCEPT
{
    return d_arg_error_exit_code(m_error);
}

inline const char*
error::get_name() const D_NOEXCEPT
{
    return d_arg_status_name(m_error->status);
}

inline const char*
error::what() const D_NOEXCEPT
{
    return d_arg_error_message(m_error);
}

inline const ::d_arg_error&
error::get() const D_NOEXCEPT
{
    return *m_error;
}

// 5.5    Lists
//------------------------------------------------------------------------------
// basic_text_list::const_iterator
//   A pointer into the array, each element made a text as it is read.
template<typename Item>
inline
basic_text_list<Item>::const_iterator::const_iterator() D_NOEXCEPT
    : m_at(D_NULLPTR)
{}

template<typename Item>
inline
basic_text_list<Item>::const_iterator::const_iterator(
    const Item* _at
) D_NOEXCEPT
    : m_at(_at)
{}

template<typename Item>
inline text
basic_text_list<Item>::const_iterator::operator*() const D_NOEXCEPT
{
    return internal::list_text(*m_at);
}

template<typename Item>
inline text
basic_text_list<Item>::const_iterator::operator[](
    difference_type _offset
) const D_NOEXCEPT
{
    return internal::list_text(m_at[_offset]);
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator&
basic_text_list<Item>::const_iterator::operator++() D_NOEXCEPT
{
    ++m_at;

    return *this;
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator
basic_text_list<Item>::const_iterator::operator++(int) D_NOEXCEPT
{
    const const_iterator before = *this;

    ++m_at;

    return before;
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator&
basic_text_list<Item>::const_iterator::operator--() D_NOEXCEPT
{
    --m_at;

    return *this;
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator
basic_text_list<Item>::const_iterator::operator--(int) D_NOEXCEPT
{
    const const_iterator before = *this;

    --m_at;

    return before;
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator&
basic_text_list<Item>::const_iterator::operator+=(
    difference_type _offset
) D_NOEXCEPT
{
    m_at += _offset;

    return *this;
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator&
basic_text_list<Item>::const_iterator::operator-=(
    difference_type _offset
) D_NOEXCEPT
{
    m_at -= _offset;

    return *this;
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator
basic_text_list<Item>::const_iterator::operator+(
    difference_type _offset
) const D_NOEXCEPT
{
    return const_iterator(m_at + _offset);
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator
basic_text_list<Item>::const_iterator::operator-(
    difference_type _offset
) const D_NOEXCEPT
{
    return const_iterator(m_at - _offset);
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator::difference_type
basic_text_list<Item>::const_iterator::operator-(
    const const_iterator& _other
) const D_NOEXCEPT
{
    return (m_at - _other.m_at);
}

template<typename Item>
inline bool
basic_text_list<Item>::const_iterator::operator==(
    const const_iterator& _other
) const D_NOEXCEPT
{
    return (m_at == _other.m_at);
}

template<typename Item>
inline bool
basic_text_list<Item>::const_iterator::operator!=(
    const const_iterator& _other
) const D_NOEXCEPT
{
    return (m_at != _other.m_at);
}

template<typename Item>
inline bool
basic_text_list<Item>::const_iterator::operator<(
    const const_iterator& _other
) const D_NOEXCEPT
{
    return (m_at < _other.m_at);
}

// basic_text_list
//   The array and its count; an empty list holds no pointer.
template<typename Item>
inline
basic_text_list<Item>::basic_text_list() D_NOEXCEPT
    : m_items(D_NULLPTR),
      m_count(0u)
{}

template<typename Item>
inline
basic_text_list<Item>::basic_text_list(
    const Item* _items,
    size_type   _count
) D_NOEXCEPT
    : m_items((_count == 0u) ? D_NULLPTR : _items),
      m_count((_items == D_NULLPTR) ? 0u : _count)
{}

template<typename Item>
inline typename basic_text_list<Item>::size_type
basic_text_list<Item>::size() const D_NOEXCEPT
{
    return m_count;
}

template<typename Item>
inline bool
basic_text_list<Item>::empty() const D_NOEXCEPT
{
    return (m_count == 0u);
}

template<typename Item>
inline text
basic_text_list<Item>::operator[](
    size_type _index
) const D_NOEXCEPT
{
    return internal::list_text(m_items[_index]);
}

template<typename Item>
inline text
basic_text_list<Item>::front() const D_NOEXCEPT
{
    return internal::list_text(m_items[0]);
}

template<typename Item>
inline text
basic_text_list<Item>::back() const D_NOEXCEPT
{
    return internal::list_text(m_items[m_count - 1u]);
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator
basic_text_list<Item>::begin() const D_NOEXCEPT
{
    return const_iterator(m_items);
}

template<typename Item>
inline typename basic_text_list<Item>::const_iterator
basic_text_list<Item>::end() const D_NOEXCEPT
{
    // an empty list may hold no pointer to add to
    return const_iterator((m_count == 0u) ? m_items : (m_items + m_count));
}

template<typename Item>
inline const Item*
basic_text_list<Item>::data() const D_NOEXCEPT
{
    return m_items;
}

template<typename Item>
inline
basic_text_list<Item>::operator std::vector<std::string>() const
{
    std::vector<std::string> copied;

    copied.reserve(m_count);

    // each element copied in order
    for (size_type index = 0u;
         index < m_count;
         ++index)
    {
        copied.push_back(internal::list_text(m_items[index]).str());
    }

    return copied;
}

// basic_handle_list::const_iterator
//   A pointer into the array, a handle made over each record as it is read.
template<typename Handle,
         typename Item>
inline
basic_handle_list<Handle, Item>::const_iterator::const_iterator() D_NOEXCEPT
    : m_at(D_NULLPTR)
{}

template<typename Handle,
         typename Item>
inline
basic_handle_list<Handle, Item>::const_iterator::const_iterator(
    Item* const* _at
) D_NOEXCEPT
    : m_at(_at)
{}

template<typename Handle,
         typename Item>
inline Handle
basic_handle_list<Handle, Item>::const_iterator::operator*() const D_NOEXCEPT
{
    return Handle(*m_at);
}

template<typename Handle,
         typename Item>
inline Handle
basic_handle_list<Handle, Item>::const_iterator::operator[](
    difference_type _offset
) const D_NOEXCEPT
{
    return Handle(m_at[_offset]);
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator&
basic_handle_list<Handle, Item>::const_iterator::operator++() D_NOEXCEPT
{
    ++m_at;

    return *this;
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator
basic_handle_list<Handle, Item>::const_iterator::operator++(int) D_NOEXCEPT
{
    const const_iterator before = *this;

    ++m_at;

    return before;
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator&
basic_handle_list<Handle, Item>::const_iterator::operator--() D_NOEXCEPT
{
    --m_at;

    return *this;
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator
basic_handle_list<Handle, Item>::const_iterator::operator--(int) D_NOEXCEPT
{
    const const_iterator before = *this;

    --m_at;

    return before;
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator&
basic_handle_list<Handle, Item>::const_iterator::operator+=(
    difference_type _offset
) D_NOEXCEPT
{
    m_at += _offset;

    return *this;
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator&
basic_handle_list<Handle, Item>::const_iterator::operator-=(
    difference_type _offset
) D_NOEXCEPT
{
    m_at -= _offset;

    return *this;
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator
basic_handle_list<Handle, Item>::const_iterator::operator+(
    difference_type _offset
) const D_NOEXCEPT
{
    return const_iterator(m_at + _offset);
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator
basic_handle_list<Handle, Item>::const_iterator::operator-(
    difference_type _offset
) const D_NOEXCEPT
{
    return const_iterator(m_at - _offset);
}

template<typename Handle,
         typename Item>
inline
typename basic_handle_list<Handle, Item>::const_iterator::difference_type
basic_handle_list<Handle, Item>::const_iterator::operator-(
    const const_iterator& _other
) const D_NOEXCEPT
{
    return (m_at - _other.m_at);
}

template<typename Handle,
         typename Item>
inline bool
basic_handle_list<Handle, Item>::const_iterator::operator==(
    const const_iterator& _other
) const D_NOEXCEPT
{
    return (m_at == _other.m_at);
}

template<typename Handle,
         typename Item>
inline bool
basic_handle_list<Handle, Item>::const_iterator::operator!=(
    const const_iterator& _other
) const D_NOEXCEPT
{
    return (m_at != _other.m_at);
}

template<typename Handle,
         typename Item>
inline bool
basic_handle_list<Handle, Item>::const_iterator::operator<(
    const const_iterator& _other
) const D_NOEXCEPT
{
    return (m_at < _other.m_at);
}

// basic_handle_list
//   The array and its count; a handle over each record as it is read.
template<typename Handle,
         typename Item>
inline
basic_handle_list<Handle, Item>::basic_handle_list() D_NOEXCEPT
    : m_items(D_NULLPTR),
      m_count(0u)
{}

template<typename Handle,
         typename Item>
inline
basic_handle_list<Handle, Item>::basic_handle_list(
    Item* const* _items,
    size_type    _count
) D_NOEXCEPT
    : m_items((_count == 0u) ? D_NULLPTR : _items),
      m_count((_items == D_NULLPTR) ? 0u : _count)
{}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::size_type
basic_handle_list<Handle, Item>::size() const D_NOEXCEPT
{
    return m_count;
}

template<typename Handle,
         typename Item>
inline bool
basic_handle_list<Handle, Item>::empty() const D_NOEXCEPT
{
    return (m_count == 0u);
}

template<typename Handle,
         typename Item>
inline Handle
basic_handle_list<Handle, Item>::operator[](
    size_type _index
) const D_NOEXCEPT
{
    return Handle(m_items[_index]);
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator
basic_handle_list<Handle, Item>::begin() const D_NOEXCEPT
{
    return const_iterator(m_items);
}

template<typename Handle,
         typename Item>
inline typename basic_handle_list<Handle, Item>::const_iterator
basic_handle_list<Handle, Item>::end() const D_NOEXCEPT
{
    return const_iterator(m_items + m_count);
}

template<typename Handle,
         typename Item>
inline Item* const*
basic_handle_list<Handle, Item>::data() const D_NOEXCEPT
{
    return m_items;
}

template<typename Handle,
         typename Item>
inline std::vector<Handle>
basic_handle_list<Handle, Item>::to_vector() const
{
    std::vector<Handle> handles;

    handles.reserve(m_count);

    // a handle over each record, in order
    for (size_type index = 0u;
         index < m_count;
         ++index)
    {
        handles.push_back(Handle(m_items[index]));
    }

    return handles;
}

// 5.6    Buffers
//------------------------------------------------------------------------------
// buffer
//   The C buffer's calls; a buffer whose growth failed reports it on every
// append, as the C library's sticky failure keeps it.
inline
buffer::buffer(
    ::d_arg_buffer& _buffer
) D_NOEXCEPT
    : m_buffer(&_buffer)
{}

inline status
buffer::append(
    const text& _text
) D_NOEXCEPT
{
    return status(d_arg_buffer_append(m_buffer,
                                      _text.data(),
                                      _text.size()));
}

inline status
buffer::append(
    char _character
) D_NOEXCEPT
{
    return status(d_arg_buffer_append_char(m_buffer,
                                           _character));
}

inline std::size_t
buffer::size() const D_NOEXCEPT
{
    return m_buffer->length;
}

inline bool
buffer::empty() const D_NOEXCEPT
{
    return (m_buffer->length == 0u);
}

inline text
buffer::view() const D_NOEXCEPT
{
    return text(d_arg_buffer_text(m_buffer));
}

inline std::string
buffer::str() const
{
    return view().str();
}

inline void
buffer::truncate(
    std::size_t _length
) D_NOEXCEPT
{
    d_arg_buffer_truncate(m_buffer,
                          _length);

    return;
}

inline ::d_arg_buffer&
buffer::get() const D_NOEXCEPT
{
    return *m_buffer;
}

// 5.7    Sinks
//------------------------------------------------------------------------------
// string_sink / stream_sink
//   The object is the sink's context and the internal write function its
// writer.
inline ::d_pack_sink
string_sink(
    std::string& _text
) D_NOEXCEPT
{
    const ::d_pack_sink sink = { internal::string_sink_write,
                                 &_text };

    return sink;
}

inline ::d_pack_sink
stream_sink(
    std::ostream& _stream
) D_NOEXCEPT
{
    const ::d_pack_sink sink = { internal::stream_sink_write,
                                 &_stream };

    return sink;
}

// 5.8    Internals
//------------------------------------------------------------------------------
#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)

// internal::guard
//   With exceptions: a std::exception is handed over as itself, so that a
// handler can use its what(), std::bad_alloc first, flagged, so that a
// handler can answer it as the C library answers a refused allocation;
// anything else as NULL.
template<typename Work,
         typename Handler>
inline typename Work::result_type
internal::guard(
    const Work&    _work,
    const Handler& _handler
)
{
    try
    {
        return _work();
    }
    catch (const std::bad_alloc& caught)
    {
        return _handler(&caught,
                        true);
    }
    catch (const std::exception& caught)
    {
        return _handler(&caught,
                        false);
    }
    catch (...)
    {
        return _handler(D_NULLPTR,
                        false);
    }
}

#else

// internal::guard
//   Without exceptions: the work alone, which cannot throw.
template<typename Work,
         typename Handler>
inline typename Work::result_type
internal::guard(
    const Work&    _work,
    const Handler& _handler
)
{
    (void)_handler;

    return _work();
}

#endif  // D_INTERNAL_ARG_PARSER_CPP_CATCH

// internal::fallback
//   The value given, whatever was thrown.
template<typename Result>
inline
internal::fallback<Result>::fallback(
    Result _value
)
    : m_value(_value)
{}

template<typename Result>
inline Result
internal::fallback<Result>::operator()(
    const std::exception* _exception,
    bool                  _out_of_memory
) const
{
    (void)_exception;
    (void)_out_of_memory;

    return m_value;
}

// internal::exception_words
//   NULL unless there is an exception whose what() holds something.
inline const char*
internal::exception_words(
    const std::exception* _exception
) D_NOEXCEPT
{
    // no standard type: nothing to say
    if (_exception == D_NULLPTR)
    {
        return D_NULLPTR;
    }

    const char* const words = _exception->what();

    return ( (words != D_NULLPTR) &&
             (words[0] != '\0') )
           ? words
           : D_NULLPTR;
}

// internal::string_append / stream_insert
//   The context restored to its type; the bytes appended, or written, and
// every byte reported taken only when the stream is still good.
inline
internal::string_append::string_append(
    void*       _text,
    const void* _data,
    std::size_t _size
) D_NOEXCEPT
    : m_text(static_cast<std::string*>(_text)),
      m_data(static_cast<const char*>(_data)),
      m_size(_size)
{}

inline std::size_t
internal::string_append::operator()() const
{
    m_text->append(m_data,
                   m_size);

    return m_size;
}

inline
internal::stream_insert::stream_insert(
    void*       _stream,
    const void* _data,
    std::size_t _size
) D_NOEXCEPT
    : m_stream(static_cast<std::ostream*>(_stream)),
      m_data(static_cast<const char*>(_data)),
      m_size(_size)
{}

inline std::size_t
internal::stream_insert::operator()() const
{
    m_stream->write(m_data,
                    static_cast<std::streamsize>(m_size));

    return m_stream->good() ? m_size : 0u;
}

// internal::string_sink_write / stream_sink_write
//   The work under the guard; an exception -- std::bad_alloc from the
// string, the stream's failure when it throws -- takes no bytes.
inline std::size_t
internal::string_sink_write(
    void*       _context,
    const void* _data,
    std::size_t _size
)
{
    return internal::guard(internal::string_append(_context,
                                                   _data,
                                                   _size),
                           internal::fallback<std::size_t>(0u));
}

inline std::size_t
internal::stream_sink_write(
    void*       _context,
    const void* _data,
    std::size_t _size
)
{
    return internal::guard(internal::stream_insert(_context,
                                                   _data,
                                                   _size),
                           internal::fallback<std::size_t>(0u));
}

// internal::list_text
//   The span itself; the C string by its length.
inline text
internal::list_text(
    const ::d_pack_text& _item
) D_NOEXCEPT
{
    return text(_item);
}

inline text
internal::list_text(
    const char* _item
) D_NOEXCEPT
{
    return text(_item);
}

// internal::scratch
//   An empty buffer, released whatever it came to hold; its content copied.
inline
internal::scratch::scratch() D_NOEXCEPT
{
    d_arg_buffer_init(&m_buffer);
}

inline
internal::scratch::~scratch()
{
    d_arg_buffer_release(&m_buffer);
}

inline ::d_arg_buffer*
internal::scratch::get() D_NOEXCEPT
{
    return &m_buffer;
}

inline std::string
internal::scratch::str() const
{
    return text(d_arg_buffer_text(&m_buffer)).str();
}

// internal::call_status<none>
//   The call, then D_ARG_OK.
template<typename Function>
inline int
internal::call_status<internal::result_kind::none>::call(
    Function& _function
)
{
    _function();

    return D_ARG_OK;
}

template<typename Function,
         typename First>
inline int
internal::call_status<internal::result_kind::none>::call(
    Function& _function,
    First&    _first
)
{
    _function(_first);

    return D_ARG_OK;
}

template<typename Function,
         typename First,
         typename Second>
inline int
internal::call_status<internal::result_kind::none>::call(
    Function& _function,
    First&    _first,
    Second&   _second
)
{
    _function(_first,
              _second);

    return D_ARG_OK;
}

template<typename Function,
         typename First,
         typename Second,
         typename Third>
inline int
internal::call_status<internal::result_kind::none>::call(
    Function& _function,
    First&    _first,
    Second&   _second,
    Third&    _third
)
{
    _function(_first,
              _second,
              _third);

    return D_ARG_OK;
}

// internal::call_status<status>
//   The call's status, a status_code made one.
template<typename Function>
inline int
internal::call_status<internal::result_kind::status>::call(
    Function& _function
)
{
    return cli::status(_function()).code();
}

template<typename Function,
         typename First>
inline int
internal::call_status<internal::result_kind::status>::call(
    Function& _function,
    First&    _first
)
{
    return cli::status(_function(_first)).code();
}

template<typename Function,
         typename First,
         typename Second>
inline int
internal::call_status<internal::result_kind::status>::call(
    Function& _function,
    First&    _first,
    Second&   _second
)
{
    return cli::status(_function(_first,
                                 _second)).code();
}

template<typename Function,
         typename First,
         typename Second,
         typename Third>
inline int
internal::call_status<internal::result_kind::status>::call(
    Function& _function,
    First&    _first,
    Second&   _second,
    Third&    _third
)
{
    return cli::status(_function(_first,
                                 _second,
                                 _third)).code();
}

// internal::call_status<truth>
//   True goes on; false is a conversion error.
template<typename Function>
inline int
internal::call_status<internal::result_kind::truth>::call(
    Function& _function
)
{
    return _function() ? D_ARG_OK : D_ARG_ERROR_CONVERSION;
}

template<typename Function,
         typename First>
inline int
internal::call_status<internal::result_kind::truth>::call(
    Function& _function,
    First&    _first
)
{
    return _function(_first) ? D_ARG_OK : D_ARG_ERROR_CONVERSION;
}

template<typename Function,
         typename First,
         typename Second>
inline int
internal::call_status<internal::result_kind::truth>::call(
    Function& _function,
    First&    _first,
    Second&   _second
)
{
    return _function(_first,
                     _second)
           ? D_ARG_OK
           : D_ARG_ERROR_CONVERSION;
}

template<typename Function,
         typename First,
         typename Second,
         typename Third>
inline int
internal::call_status<internal::result_kind::truth>::call(
    Function& _function,
    First&    _first,
    Second&   _second,
    Third&    _third
)
{
    return _function(_first,
                     _second,
                     _third)
           ? D_ARG_OK
           : D_ARG_ERROR_CONVERSION;
}

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMON_HPP
