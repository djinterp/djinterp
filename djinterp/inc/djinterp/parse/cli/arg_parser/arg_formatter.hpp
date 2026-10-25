/*******************************************************************************
* djinterp [parse]                                             arg_formatter.hpp
*
* The argument parser's C++ face: help's formatter.
*   `cli::formatter` is CLI11's Formatter: a struct d_arg_formatter -- the
* widths, switches and labels of arg_formatter.h -- whose steps are CLI11's
* virtual make_* functions. It derives from the C struct, so that it is
* given to a command (command_interface::formatter) as the C formatter it
* is, and its table of steps calls the virtual functions, so that a class
* deriving from it replaces a step as CLI11's subclasses do, wherever help
* calls the step. Each default step is the C library's, which calls the
* others through the table, as CLI11's call through the virtual functions.
*   A step appends its piece of help to a cli::buffer and returns a
* cli::status, where CLI11's returns a std::string: the text is built in
* place, a function that fails -- a usage function, a type name -- ends
* help with its status, and nothing is thrown. What a step throws ends help
* as a conversion error, or as out_of_memory for std::bad_alloc.
*   formatter_lambda is CLI11's FormatterLambda: a formatter whose whole
* help is a function's.
*   A command borrows its formatter, which must outlive it.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/arg_formatter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FORMATTERS
    ----------
    1.  The formatter
         1.  formatter
    2.  A function as help
         1.  formatter_lambda
2.  INTERNALS
    ---------
    1.  Steps
         1.  step_call2 / step_call3 / step_call4
         2.  formatter_table
         3.  The step functions
    2.  Lambdas
         1.  lambda_append
3.  DEFINITIONS
    -----------
    1.  formatter
    2.  formatter_lambda
    3.  Internals
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_FORMATTER_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_FORMATTER_HPP 1

// std
#include <cstddef>    // std::size_t
#include <string>     // std::string
// djinterp
#include "../../../djinterp.hpp"          // framework root
#include "../../../c/util/sink_common.h"  // d_pack_text
#include "../../../env/env.h"             // language levels
#include "./arg_command.hpp"              // command, raise_out_of_memory
#include "./arg_common.hpp"               // status, text, buffer, guard
#include "./arg_option.hpp"               // option, option_list
#include "./arg_type.hpp"                 // conversion_failure
#include "./c/arg_formatter.h"            // d_arg_formatter
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // uint32_t


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  FORMATTERS
//==============================================================================


// 1.1    The formatter
//------------------------------------------------------------------------------
// 1.1.1
// formatter
//   class: CLI11's Formatter, over a d_arg_formatter made by its constructor
// and released by its destructor, whose table calls the virtual steps with
// the object as its context. The settings are CLI11's FormatterBase's, and
// return the formatter; line_width and terminal_width are this module's
// (arg_formatter.h). A copy has the settings and labels of the original and
// its own table; copying throws std::bad_alloc, as CLI11's std::map of
// labels does, when a label cannot be copied. A member function hides the
// field of its name.
class formatter : public ::d_arg_formatter
{
public:
    // making, copying and releasing
    formatter() D_NOEXCEPT;
    formatter(const formatter& _other);
    formatter& operator=(const formatter& _other);
    virtual ~formatter();

    // labels: CLI11's label() and get_label(), which translate help's fixed
    // words and type names; a label that cannot be copied is not set, and
    // std::bad_alloc is thrown, as by CLI11's std::map
    formatter&  label(c_string _key,
                      c_string _value);
    std::string get_label(c_string _key) const;

    // settings: CLI11's widths, ratio and switches
    formatter& column_width(std::size_t _width) D_NOEXCEPT;
    formatter& right_column_width(std::size_t _width) D_NOEXCEPT;
    formatter& description_paragraph_width(std::size_t _width) D_NOEXCEPT;
    formatter& footer_paragraph_width(std::size_t _width) D_NOEXCEPT;
    formatter& long_option_alignment_ratio(float _ratio) D_NOEXCEPT;
    formatter& enable_description_formatting(bool _value = true) D_NOEXCEPT;
    formatter& enable_footer_formatting(bool _value = true) D_NOEXCEPT;
    formatter& enable_option_defaults(bool _value = true) D_NOEXCEPT;
    formatter& enable_option_type_names(bool _value = true) D_NOEXCEPT;
    formatter& enable_default_flag_values(bool _value = true) D_NOEXCEPT;

    // settings beyond CLI11: the longest line help writes, 0 for no limit,
    // and whether the terminal's width narrows it (arg_formatter.h)
    formatter& line_width(std::size_t _width) D_NOEXCEPT;
    formatter& terminal_width(bool _value = true) D_NOEXCEPT;

    // settings, read
    std::size_t get_column_width() const D_NOEXCEPT;
    std::size_t get_right_column_width() const D_NOEXCEPT;
    std::size_t get_description_paragraph_width() const D_NOEXCEPT;
    std::size_t get_footer_paragraph_width() const D_NOEXCEPT;
    std::size_t get_line_width() const D_NOEXCEPT;
    float       get_long_option_alignment_ratio() const D_NOEXCEPT;
    bool        is_description_paragraph_formatting_enabled() const
                    D_NOEXCEPT;
    bool        is_footer_paragraph_formatting_enabled() const D_NOEXCEPT;
    bool        is_option_defaults_enabled() const D_NOEXCEPT;
    bool        is_option_type_names_enabled() const D_NOEXCEPT;
    bool        is_default_flag_values_enabled() const D_NOEXCEPT;
    bool        is_terminal_width_enabled() const D_NOEXCEPT;

    // the steps: CLI11's make_* functions, each appending its piece of help
    // to `_out` and returning status_code::ok or the status that stopped it,
    // what it appended before then left in place. `_name` is the command's
    // name as help shows it, and `_options` a group's options. The default
    // of each is the C library's step (arg_formatter.h), which calls the
    // others through this formatter; make_groups, as in CLI11, is not
    // replaceable.
    virtual cli::status make_help(const cli::command& _command,
                                  const text&         _name,
                                  help_mode::value    _mode,
                                  buffer&             _out) const;
    virtual cli::status make_group(const text&        _group,
                                   bool               _positional,
                                   const option_list& _options,
                                   buffer&            _out) const;
    virtual cli::status make_positionals(const cli::command& _command,
                                         buffer&             _out) const;
    cli::status         make_groups(const cli::command& _command,
                                    help_mode::value    _mode,
                                    buffer&             _out) const;
    virtual cli::status make_subcommands(const cli::command& _command,
                                         help_mode::value    _mode,
                                         buffer&             _out) const;
    virtual cli::status make_subcommand(const cli::command& _command,
                                        buffer&             _out) const;
    virtual cli::status make_expanded(const cli::command& _command,
                                      help_mode::value    _mode,
                                      buffer&             _out) const;
    virtual cli::status make_footer(const cli::command& _command,
                                    buffer&             _out) const;
    virtual cli::status make_description(const cli::command& _command,
                                         buffer&             _out) const;
    virtual cli::status make_usage(const cli::command& _command,
                                   const text&         _name,
                                   buffer&             _out) const;
    virtual cli::status make_option(const cli::option& _option,
                                    bool               _positional,
                                    buffer&            _out) const;
    virtual cli::status make_option_name(const cli::option& _option,
                                         bool               _positional,
                                         buffer&            _out) const;
    virtual cli::status make_option_opts(const cli::option& _option,
                                         buffer&            _out) const;
    virtual cli::status make_option_desc(const cli::option& _option,
                                         buffer&            _out) const;
    virtual cli::status make_option_usage(const cli::option& _option,
                                          buffer&            _out) const;

private:
    formatter& set_flag(re_std::uint32_t _flag,
                        bool             _value) D_NOEXCEPT;
    bool       has_flag(re_std::uint32_t _flag) const D_NOEXCEPT;
    void       own_table() D_NOEXCEPT;
};

// 1.2    A function as help
//------------------------------------------------------------------------------
// 1.2.1
// formatter_lambda
//   class: CLI11's FormatterLambda: a formatter whose help is what a function
// returns, given the command -- `const cli::command&` -- the name help
// shows, as a `const cli::text&`, and the mode, a help_mode::value, and
// returning the help as a std::string, a C string or a text. The function
// is copied, and called as the caller's own copy would be, const or not;
// the other steps stay the formatter's.
template<typename Function>
class formatter_lambda : public formatter
{
public:
    explicit formatter_lambda(const Function& _function);

    virtual cli::status make_help(const cli::command& _command,
                                  const text&         _name,
                                  help_mode::value    _mode,
                                  buffer&             _out) const D_OVERRIDE;

private:
    mutable Function m_function;
};


//==============================================================================
// 2.  INTERNALS
//==============================================================================


// 2.1    Steps
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.1.1
    // step_call2 / step_call3 / step_call4
    //   classes: the work guard runs for a step: the virtual function, by its
    //   member pointer, called on the formatter with the arguments given, its
    //   status as a d_arg_status.
    template<typename First>
    class step_call2
    {
    public:
        typedef int result_type;
        typedef cli::status (formatter::*method_type)(First,
                                                      buffer&) const;

        step_call2(const formatter& _self,
                   method_type      _method,
                   First            _first,
                   buffer&          _out) D_NOEXCEPT;

        int operator()() const;

    private:
        const formatter& m_self;
        method_type      m_method;
        First            m_first;
        buffer&          m_out;
    };

    template<typename First,
             typename Second>
    class step_call3
    {
    public:
        typedef int result_type;
        typedef cli::status (formatter::*method_type)(First,
                                                      Second,
                                                      buffer&) const;

        step_call3(const formatter& _self,
                   method_type      _method,
                   First            _first,
                   Second           _second,
                   buffer&          _out) D_NOEXCEPT;

        int operator()() const;

    private:
        const formatter& m_self;
        method_type      m_method;
        First            m_first;
        Second           m_second;
        buffer&          m_out;
    };

    template<typename First,
             typename Second,
             typename Third>
    class step_call4
    {
    public:
        typedef int result_type;
        typedef cli::status (formatter::*method_type)(First,
                                                      Second,
                                                      Third,
                                                      buffer&) const;

        step_call4(const formatter& _self,
                   method_type      _method,
                   First            _first,
                   Second           _second,
                   Third            _third,
                   buffer&          _out) D_NOEXCEPT;

        int operator()() const;

    private:
        const formatter& m_self;
        method_type      m_method;
        First            m_first;
        Second           m_second;
        Third            m_third;
        buffer&          m_out;
    };

    // 2.1.2
    // formatter_table
    //   function: the table of steps every cli::formatter holds, one for the
    //   program, each entry the step function of 2.1.3.
    const ::d_arg_formatter_vtable& formatter_table() D_NOEXCEPT;

    // 2.1.3
    // The step functions
    //   functions: the table's entries, with the C steps' parameters: each
    //   reads the formatter object from the C formatter's context, makes the
    //   handles, views and buffer the virtual step takes, and calls it under
    //   guard, an exception answered as conversion_failure answers it.
    int step_help(const ::d_arg_formatter* _formatter,
                  const ::d_arg_command*   _command,
                  ::d_pack_text            _name,
                  int                      _mode,
                  ::d_arg_buffer*          _out);
    int step_group(const ::d_arg_formatter*   _formatter,
                   ::d_pack_text              _group,
                   bool                       _positional,
                   const ::d_arg_option_list* _options,
                   ::d_arg_buffer*            _out);
    int step_positionals(const ::d_arg_formatter* _formatter,
                         const ::d_arg_command*   _command,
                         ::d_arg_buffer*          _out);
    int step_subcommands(const ::d_arg_formatter* _formatter,
                         const ::d_arg_command*   _command,
                         int                      _mode,
                         ::d_arg_buffer*          _out);
    int step_subcommand(const ::d_arg_formatter* _formatter,
                        const ::d_arg_command*   _command,
                        ::d_arg_buffer*          _out);
    int step_expanded(const ::d_arg_formatter* _formatter,
                      const ::d_arg_command*   _command,
                      int                      _mode,
                      ::d_arg_buffer*          _out);
    int step_footer(const ::d_arg_formatter* _formatter,
                    const ::d_arg_command*   _command,
                    ::d_arg_buffer*          _out);
    int step_description(const ::d_arg_formatter* _formatter,
                         const ::d_arg_command*   _command,
                         ::d_arg_buffer*          _out);
    int step_usage(const ::d_arg_formatter* _formatter,
                   const ::d_arg_command*   _command,
                   ::d_pack_text            _name,
                   ::d_arg_buffer*          _out);
    int step_option(const ::d_arg_formatter* _formatter,
                    const ::d_arg_option*    _option,
                    bool                     _positional,
                    ::d_arg_buffer*          _out);
    int step_option_name(const ::d_arg_formatter* _formatter,
                         const ::d_arg_option*    _option,
                         bool                     _positional,
                         ::d_arg_buffer*          _out);
    int step_option_opts(const ::d_arg_formatter* _formatter,
                         const ::d_arg_option*    _option,
                         ::d_arg_buffer*          _out);
    int step_option_desc(const ::d_arg_formatter* _formatter,
                         const ::d_arg_option*    _option,
                         ::d_arg_buffer*          _out);
    int step_option_usage(const ::d_arg_formatter* _formatter,
                          const ::d_arg_option*    _option,
                          ::d_arg_buffer*          _out);

NS_END  // internal

// 2.2    Lambdas
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.2.1
    // lambda_append
    //   function: formatter_lambda's help: the function's result, whatever
    //   text type it is, appended whole.
    template<typename Result>
    cli::status lambda_append(const Result& _result,
                              buffer&       _out);

NS_END  // internal


//==============================================================================
// 3.  DEFINITIONS
//==============================================================================


// 3.1    formatter
//------------------------------------------------------------------------------
// formatter::formatter / operator= / ~formatter / own_table
//   d_arg_formatter_init over the base, which is plain data and so already
// there to be made, then the table and this object as its context;
// d_arg_formatter_copy, whose failure for a label is std::bad_alloc, the
// copy then holding the settings and no labels; d_arg_formatter_release.
inline
formatter::formatter() D_NOEXCEPT
{
    d_arg_formatter_init(this);
    own_table();
}

inline
formatter::formatter(
    const formatter& _other
)
    : ::d_arg_formatter()
{
    d_arg_formatter_init(this);

    const int copied = d_arg_formatter_copy(this,
                                            &_other);

    own_table();
    internal::raise_out_of_memory(copied);
}

inline formatter&
formatter::operator=(
    const formatter& _other
)
{
    // a formatter given itself is left as it is
    if (this != &_other)
    {
        const int copied = d_arg_formatter_copy(this,
                                                &_other);

        own_table();
        internal::raise_out_of_memory(copied);
    }

    return *this;
}

inline
formatter::~formatter()
{
    d_arg_formatter_release(this);
}

inline void
formatter::own_table() D_NOEXCEPT
{
    this->d_arg_formatter::vtable  = &internal::formatter_table();
    this->d_arg_formatter::context = this;

    return;
}

// formatter::label / get_label
//   The C label calls; a label not set for want of memory is std::bad_alloc,
// and the label read is copied before the key it may be dies.
inline formatter&
formatter::label(
    c_string _key,
    c_string _value
)
{
    internal::raise_out_of_memory(d_arg_formatter_set_label(this,
                                                            _key.get(),
                                                            _value.get()));

    return *this;
}

inline std::string
formatter::get_label(
    c_string _key
) const
{
    return text(d_arg_formatter_label(this,
                                      text(_key.get()))).str();
}

// formatter::column_width and kin
//   The C settings; the switches through set_flag.
inline formatter&
formatter::column_width(
    std::size_t _width
) D_NOEXCEPT
{
    d_arg_formatter_set_column_width(this,
                                     _width);

    return *this;
}

inline formatter&
formatter::right_column_width(
    std::size_t _width
) D_NOEXCEPT
{
    d_arg_formatter_set_right_column_width(this,
                                           _width);

    return *this;
}

inline formatter&
formatter::description_paragraph_width(
    std::size_t _width
) D_NOEXCEPT
{
    d_arg_formatter_set_description_paragraph_width(this,
                                                    _width);

    return *this;
}

inline formatter&
formatter::footer_paragraph_width(
    std::size_t _width
) D_NOEXCEPT
{
    d_arg_formatter_set_footer_paragraph_width(this,
                                               _width);

    return *this;
}

inline formatter&
formatter::long_option_alignment_ratio(
    float _ratio
) D_NOEXCEPT
{
    d_arg_formatter_set_long_option_alignment_ratio(this,
                                                    _ratio);

    return *this;
}

inline formatter&
formatter::enable_description_formatting(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_FORMAT_DESCRIPTION_PARAGRAPHS,
                    _value);
}

inline formatter&
formatter::enable_footer_formatting(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_FORMAT_FOOTER_PARAGRAPHS,
                    _value);
}

inline formatter&
formatter::enable_option_defaults(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_FORMAT_OPTION_DEFAULTS,
                    _value);
}

inline formatter&
formatter::enable_option_type_names(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_FORMAT_OPTION_TYPE_NAMES,
                    _value);
}

inline formatter&
formatter::enable_default_flag_values(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_FORMAT_DEFAULT_FLAG_VALUES,
                    _value);
}

inline formatter&
formatter::line_width(
    std::size_t _width
) D_NOEXCEPT
{
    d_arg_formatter_set_line_width(this,
                                   _width);

    return *this;
}

inline formatter&
formatter::terminal_width(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_FORMAT_TERMINAL_WIDTH,
                    _value);
}

// formatter::get_column_width and kin
//   The C formatter's fields, read.
inline std::size_t
formatter::get_column_width() const D_NOEXCEPT
{
    return this->d_arg_formatter::column_width;
}

inline std::size_t
formatter::get_right_column_width() const D_NOEXCEPT
{
    return this->d_arg_formatter::right_column_width;
}

inline std::size_t
formatter::get_description_paragraph_width() const D_NOEXCEPT
{
    return this->d_arg_formatter::description_paragraph_width;
}

inline std::size_t
formatter::get_footer_paragraph_width() const D_NOEXCEPT
{
    return this->d_arg_formatter::footer_paragraph_width;
}

inline std::size_t
formatter::get_line_width() const D_NOEXCEPT
{
    return this->d_arg_formatter::line_width;
}

inline float
formatter::get_long_option_alignment_ratio() const D_NOEXCEPT
{
    return this->d_arg_formatter::long_option_alignment_ratio;
}

inline bool
formatter::is_description_paragraph_formatting_enabled() const D_NOEXCEPT
{
    return has_flag(D_ARG_FORMAT_DESCRIPTION_PARAGRAPHS);
}

inline bool
formatter::is_footer_paragraph_formatting_enabled() const D_NOEXCEPT
{
    return has_flag(D_ARG_FORMAT_FOOTER_PARAGRAPHS);
}

inline bool
formatter::is_option_defaults_enabled() const D_NOEXCEPT
{
    return has_flag(D_ARG_FORMAT_OPTION_DEFAULTS);
}

inline bool
formatter::is_option_type_names_enabled() const D_NOEXCEPT
{
    return has_flag(D_ARG_FORMAT_OPTION_TYPE_NAMES);
}

inline bool
formatter::is_default_flag_values_enabled() const D_NOEXCEPT
{
    return has_flag(D_ARG_FORMAT_DEFAULT_FLAG_VALUES);
}

inline bool
formatter::is_terminal_width_enabled() const D_NOEXCEPT
{
    return has_flag(D_ARG_FORMAT_TERMINAL_WIDTH);
}

// formatter::make_help and kin
//   The C library's default steps, with this formatter, whose table brings
// the steps they call back here.
inline cli::status
formatter::make_help(
    const cli::command& _command,
    const text&         _name,
    help_mode::value    _mode,
    buffer&             _out
) const
{
    return cli::status(d_arg_format_help_default(this,
                                                 _command.get(),
                                                 _name,
                                                 static_cast<int>(_mode),
                                                 &_out.get()));
}

inline cli::status
formatter::make_group(
    const text&        _group,
    bool               _positional,
    const option_list& _options,
    buffer&            _out
) const
{
    const ::d_arg_option_list options =
    {
        const_cast< ::d_arg_option**>(_options.data()),
        _options.size(),
        _options.size()
    };

    return cli::status(d_arg_format_group_default(this,
                                                  _group,
                                                  _positional,
                                                  &options,
                                                  &_out.get()));
}

inline cli::status
formatter::make_positionals(
    const cli::command& _command,
    buffer&             _out
) const
{
    return cli::status(d_arg_format_positionals_default(this,
                                                        _command.get(),
                                                        &_out.get()));
}

inline cli::status
formatter::make_groups(
    const cli::command& _command,
    help_mode::value    _mode,
    buffer&             _out
) const
{
    return cli::status(d_arg_format_groups(this,
                                           _command.get(),
                                           static_cast<int>(_mode),
                                           &_out.get()));
}

inline cli::status
formatter::make_subcommands(
    const cli::command& _command,
    help_mode::value    _mode,
    buffer&             _out
) const
{
    return cli::status(d_arg_format_subcommands_default(
                           this,
                           _command.get(),
                           static_cast<int>(_mode),
                           &_out.get()));
}

inline cli::status
formatter::make_subcommand(
    const cli::command& _command,
    buffer&             _out
) const
{
    return cli::status(d_arg_format_subcommand_default(this,
                                                       _command.get(),
                                                       &_out.get()));
}

inline cli::status
formatter::make_expanded(
    const cli::command& _command,
    help_mode::value    _mode,
    buffer&             _out
) const
{
    return cli::status(d_arg_format_expanded_default(this,
                                                     _command.get(),
                                                     static_cast<int>(_mode),
                                                     &_out.get()));
}

inline cli::status
formatter::make_footer(
    const cli::command& _command,
    buffer&             _out
) const
{
    return cli::status(d_arg_format_footer_default(this,
                                                   _command.get(),
                                                   &_out.get()));
}

inline cli::status
formatter::make_description(
    const cli::command& _command,
    buffer&             _out
) const
{
    return cli::status(d_arg_format_description_default(this,
                                                        _command.get(),
                                                        &_out.get()));
}

inline cli::status
formatter::make_usage(
    const cli::command& _command,
    const text&         _name,
    buffer&             _out
) const
{
    return cli::status(d_arg_format_usage_default(this,
                                                  _command.get(),
                                                  _name,
                                                  &_out.get()));
}

inline cli::status
formatter::make_option(
    const cli::option& _option,
    bool               _positional,
    buffer&            _out
) const
{
    return cli::status(d_arg_format_option_default(this,
                                                   _option.get(),
                                                   _positional,
                                                   &_out.get()));
}

inline cli::status
formatter::make_option_name(
    const cli::option& _option,
    bool               _positional,
    buffer&            _out
) const
{
    return cli::status(d_arg_format_option_name_default(this,
                                                        _option.get(),
                                                        _positional,
                                                        &_out.get()));
}

inline cli::status
formatter::make_option_opts(
    const cli::option& _option,
    buffer&            _out
) const
{
    return cli::status(d_arg_format_option_opts_default(this,
                                                        _option.get(),
                                                        &_out.get()));
}

inline cli::status
formatter::make_option_desc(
    const cli::option& _option,
    buffer&            _out
) const
{
    return cli::status(d_arg_format_option_desc_default(this,
                                                        _option.get(),
                                                        &_out.get()));
}

inline cli::status
formatter::make_option_usage(
    const cli::option& _option,
    buffer&            _out
) const
{
    return cli::status(d_arg_format_option_usage_default(this,
                                                         _option.get(),
                                                         &_out.get()));
}

// formatter::set_flag / has_flag
//   One D_ARG_FORMAT_* switch, set through the C call or read from the
// field.
inline formatter&
formatter::set_flag(
    re_std::uint32_t _flag,
    bool             _value
) D_NOEXCEPT
{
    d_arg_formatter_set_flag(this,
                             _flag,
                             _value);

    return *this;
}

inline bool
formatter::has_flag(
    re_std::uint32_t _flag
) const D_NOEXCEPT
{
    return ((this->d_arg_formatter::flags & _flag) != 0u);
}

// 3.2    formatter_lambda
//------------------------------------------------------------------------------
// formatter_lambda::formatter_lambda / make_help
//   The function kept by value; help its result.
template<typename Function>
inline
formatter_lambda<Function>::formatter_lambda(
    const Function& _function
)
    : formatter(),
      m_function(_function)
{}

template<typename Function>
inline cli::status
formatter_lambda<Function>::make_help(
    const cli::command& _command,
    const text&         _name,
    help_mode::value    _mode,
    buffer&             _out
) const
{
    return internal::lambda_append(m_function(_command,
                                              _name,
                                              _mode),
                                   _out);
}

// 3.3    Internals
//------------------------------------------------------------------------------
// internal::step_call2 / step_call3 / step_call4
//   The arguments kept as given -- references to the step function's locals
// -- and the virtual function called through its member pointer.
template<typename First>
inline
internal::step_call2<First>::step_call2(
    const formatter& _self,
    method_type      _method,
    First            _first,
    buffer&          _out
) D_NOEXCEPT
    : m_self(_self),
      m_method(_method),
      m_first(_first),
      m_out(_out)
{}

template<typename First>
inline int
internal::step_call2<First>::operator()() const
{
    return (m_self.*m_method)(m_first,
                              m_out).code();
}

template<typename First,
         typename Second>
inline
internal::step_call3<First, Second>::step_call3(
    const formatter& _self,
    method_type      _method,
    First            _first,
    Second           _second,
    buffer&          _out
) D_NOEXCEPT
    : m_self(_self),
      m_method(_method),
      m_first(_first),
      m_second(_second),
      m_out(_out)
{}

template<typename First,
         typename Second>
inline int
internal::step_call3<First, Second>::operator()() const
{
    return (m_self.*m_method)(m_first,
                              m_second,
                              m_out).code();
}

template<typename First,
         typename Second,
         typename Third>
inline
internal::step_call4<First, Second, Third>::step_call4(
    const formatter& _self,
    method_type      _method,
    First            _first,
    Second           _second,
    Third            _third,
    buffer&          _out
) D_NOEXCEPT
    : m_self(_self),
      m_method(_method),
      m_first(_first),
      m_second(_second),
      m_third(_third),
      m_out(_out)
{}

template<typename First,
         typename Second,
         typename Third>
inline int
internal::step_call4<First, Second, Third>::operator()() const
{
    return (m_self.*m_method)(m_first,
                              m_second,
                              m_third,
                              m_out).code();
}

// internal::formatter_table
//   A table of constant addresses, made before any code runs.
inline const ::d_arg_formatter_vtable&
internal::formatter_table() D_NOEXCEPT
{
    static const ::d_arg_formatter_vtable table =
    {
        &internal::step_help,
        &internal::step_group,
        &internal::step_positionals,
        &internal::step_subcommands,
        &internal::step_subcommand,
        &internal::step_expanded,
        &internal::step_footer,
        &internal::step_description,
        &internal::step_usage,
        &internal::step_option,
        &internal::step_option_name,
        &internal::step_option_opts,
        &internal::step_option_desc,
        &internal::step_option_usage
    };

    return table;
}

// internal::step_help and kin
//   The formatter object from the context; the C records as handles, the
// spans as views, the C buffer as a buffer; the virtual step under guard.
inline int
internal::step_help(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_command*   _command,
    ::d_pack_text            _name,
    int                      _mode,
    ::d_arg_buffer*          _out
)
{
    const formatter&   self = *static_cast<const formatter*>(
                                  _formatter->context);
    const cli::command subject(const_cast< ::d_arg_command*>(_command));
    const text         name(_name);
    buffer             out(*_out);

    return internal::guard(
               internal::step_call4<const cli::command&,
                                    const text&,
                                    help_mode::value>(
                   self,
                   &formatter::make_help,
                   subject,
                   name,
                   static_cast<help_mode::value>(_mode),
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_group(
    const ::d_arg_formatter*   _formatter,
    ::d_pack_text              _group,
    bool                       _positional,
    const ::d_arg_option_list* _options,
    ::d_arg_buffer*            _out
)
{
    const formatter&  self = *static_cast<const formatter*>(
                                 _formatter->context);
    const text        group(_group);
    const option_list options(_options->items,
                              _options->count);
    buffer            out(*_out);

    return internal::guard(
               internal::step_call4<const text&,
                                    bool,
                                    const option_list&>(
                   self,
                   &formatter::make_group,
                   group,
                   _positional,
                   options,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_positionals(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_command*   _command,
    ::d_arg_buffer*          _out
)
{
    const formatter&   self = *static_cast<const formatter*>(
                                  _formatter->context);
    const cli::command subject(const_cast< ::d_arg_command*>(_command));
    buffer             out(*_out);

    return internal::guard(
               internal::step_call2<const cli::command&>(
                   self,
                   &formatter::make_positionals,
                   subject,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_subcommands(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_command*   _command,
    int                      _mode,
    ::d_arg_buffer*          _out
)
{
    const formatter&   self = *static_cast<const formatter*>(
                                  _formatter->context);
    const cli::command subject(const_cast< ::d_arg_command*>(_command));
    buffer             out(*_out);

    return internal::guard(
               internal::step_call3<const cli::command&,
                                    help_mode::value>(
                   self,
                   &formatter::make_subcommands,
                   subject,
                   static_cast<help_mode::value>(_mode),
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_subcommand(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_command*   _command,
    ::d_arg_buffer*          _out
)
{
    const formatter&   self = *static_cast<const formatter*>(
                                  _formatter->context);
    const cli::command subject(const_cast< ::d_arg_command*>(_command));
    buffer             out(*_out);

    return internal::guard(
               internal::step_call2<const cli::command&>(
                   self,
                   &formatter::make_subcommand,
                   subject,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_expanded(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_command*   _command,
    int                      _mode,
    ::d_arg_buffer*          _out
)
{
    const formatter&   self = *static_cast<const formatter*>(
                                  _formatter->context);
    const cli::command subject(const_cast< ::d_arg_command*>(_command));
    buffer             out(*_out);

    return internal::guard(
               internal::step_call3<const cli::command&,
                                    help_mode::value>(
                   self,
                   &formatter::make_expanded,
                   subject,
                   static_cast<help_mode::value>(_mode),
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_footer(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_command*   _command,
    ::d_arg_buffer*          _out
)
{
    const formatter&   self = *static_cast<const formatter*>(
                                  _formatter->context);
    const cli::command subject(const_cast< ::d_arg_command*>(_command));
    buffer             out(*_out);

    return internal::guard(
               internal::step_call2<const cli::command&>(
                   self,
                   &formatter::make_footer,
                   subject,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_description(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_command*   _command,
    ::d_arg_buffer*          _out
)
{
    const formatter&   self = *static_cast<const formatter*>(
                                  _formatter->context);
    const cli::command subject(const_cast< ::d_arg_command*>(_command));
    buffer             out(*_out);

    return internal::guard(
               internal::step_call2<const cli::command&>(
                   self,
                   &formatter::make_description,
                   subject,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_usage(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_command*   _command,
    ::d_pack_text            _name,
    ::d_arg_buffer*          _out
)
{
    const formatter&   self = *static_cast<const formatter*>(
                                  _formatter->context);
    const cli::command subject(const_cast< ::d_arg_command*>(_command));
    const text         name(_name);
    buffer             out(*_out);

    return internal::guard(
               internal::step_call3<const cli::command&,
                                    const text&>(
                   self,
                   &formatter::make_usage,
                   subject,
                   name,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_option(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_option*    _option,
    bool                     _positional,
    ::d_arg_buffer*          _out
)
{
    const formatter&  self = *static_cast<const formatter*>(
                                 _formatter->context);
    const cli::option subject(const_cast< ::d_arg_option*>(_option));
    buffer            out(*_out);

    return internal::guard(
               internal::step_call3<const cli::option&,
                                    bool>(
                   self,
                   &formatter::make_option,
                   subject,
                   _positional,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_option_name(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_option*    _option,
    bool                     _positional,
    ::d_arg_buffer*          _out
)
{
    const formatter&  self = *static_cast<const formatter*>(
                                 _formatter->context);
    const cli::option subject(const_cast< ::d_arg_option*>(_option));
    buffer            out(*_out);

    return internal::guard(
               internal::step_call3<const cli::option&,
                                    bool>(
                   self,
                   &formatter::make_option_name,
                   subject,
                   _positional,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_option_opts(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_option*    _option,
    ::d_arg_buffer*          _out
)
{
    const formatter&  self = *static_cast<const formatter*>(
                                 _formatter->context);
    const cli::option subject(const_cast< ::d_arg_option*>(_option));
    buffer            out(*_out);

    return internal::guard(
               internal::step_call2<const cli::option&>(
                   self,
                   &formatter::make_option_opts,
                   subject,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_option_desc(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_option*    _option,
    ::d_arg_buffer*          _out
)
{
    const formatter&  self = *static_cast<const formatter*>(
                                 _formatter->context);
    const cli::option subject(const_cast< ::d_arg_option*>(_option));
    buffer            out(*_out);

    return internal::guard(
               internal::step_call2<const cli::option&>(
                   self,
                   &formatter::make_option_desc,
                   subject,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

inline int
internal::step_option_usage(
    const ::d_arg_formatter* _formatter,
    const ::d_arg_option*    _option,
    ::d_arg_buffer*          _out
)
{
    const formatter&  self = *static_cast<const formatter*>(
                                 _formatter->context);
    const cli::option subject(const_cast< ::d_arg_option*>(_option));
    buffer            out(*_out);

    return internal::guard(
               internal::step_call2<const cli::option&>(
                   self,
                   &formatter::make_option_usage,
                   subject,
                   out),
               internal::conversion_failure(D_NULLPTR));
}

// internal::lambda_append
//   The result as a text, appended.
template<typename Result>
inline cli::status
internal::lambda_append(
    const Result& _result,
    buffer&       _out
)
{
    return _out.append(text(_result));
}

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_FORMATTER_HPP
