/*******************************************************************************
* djinterp [parse]                                                arg_option.hpp
*
* The argument parser's C++ face: options.
*   `cli::option` is a handle on a struct d_arg_option -- one pointer, the
* cost of CLI11's Option* -- with CLI11's Option interface as members: its
* settings, which return the handle so that calls chain with `.` and, as
* CLI11's do, with `->`; its readers; and its results, read as text, as a
* value of any type, or into a variable. A handle does not own its option:
* the command that made it does, and the handle is valid while the option
* is. A handle on no option -- the one a definition that failed returns --
* is ignored by every call, as the C library ignores NULL, so a chain on a
* failed definition is harmless, and the failure is the parser's definition
* error, which its next parse reports.
*   Callbacks are any function object: an each() function, a callback over
* the option's values, a function writing the type name or the default.
* Each is copied, the option owning the copy, and called under the guard of
* arg_common.hpp, so what it throws becomes a status rather than crossing
* the C library. A callback may return nothing, a cli::status, or -- where
* CLI11's returns one -- a bool.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/arg_option.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPTIONS
    -------
    1.  The handle
         1.  option
         2.  option_list
    2.  Comparing handles
         1.  operator== / operator!=
    3.  Layout
         1.  The handle is one pointer
2.  INTERNALS
    ---------
    1.  Callbacks
         1.  callback_failure
         2.  each_call / each_function
         3.  results_call / results_function
         4.  text_call / text_function
    2.  Transformations
         1.  is_validator
         2.  transform_call
3.  DEFINITIONS
    -----------
    1.  option
    2.  Comparing handles
    3.  Internals
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_OPTION_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_OPTION_HPP 1

// std
#include <cstddef>    // std::size_t
#include <exception>  // std::exception
#include <string>     // std::string
#include <vector>     // std::vector
// djinterp
#include "../../../djinterp.hpp"          // framework root
#include "../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../env/env.h"             // language levels
#include "./arg_common.hpp"               // status, text, lists, guard,
                                          // callables
#include "./arg_type.hpp"                 // type_traits, to_text
#include "./arg_validator.hpp"            // validator, validator_handle
#include "./c/arg_option.h"               // d_arg_option
#include "./c/arg_parser.h"               // d_arg_option_fail


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  OPTIONS
//==============================================================================


// 1.1    The handle
//------------------------------------------------------------------------------
// 1.1.1
// option
//   class: CLI11's Option*, as a handle. Settings return the handle; readers
// read the C option's fields through the C calls, a NULL handle reading as
// an option with nothing set; results are views valid until the option's
// values change. Its `explicit operator bool`, from C++11, is CLI11's: a
// value was given, or the callback is forced -- `if (*opt)` reads as
// CLI11's does -- beside the named predicate is_given(); valid() is whether
// the handle refers to an option.
class option
{
public:
    // making: a handle on no option, or on a C option, which C may have made
    option() D_NOEXCEPT;
    explicit option(::d_arg_option* _option) D_NOEXCEPT;

    // the handle: CLI11's ports read `opt->x()` and `*opt`
    option*         operator->() D_NOEXCEPT;
    const option*   operator->() const D_NOEXCEPT;
    option&         operator*() D_NOEXCEPT;
    const option&   operator*() const D_NOEXCEPT;
    ::d_arg_option* get() const D_NOEXCEPT;
    bool            valid() const D_NOEXCEPT;

    // text: CLI11's description(), option_text(), group(), type_name() and
    // envname(), each copying its text; a failure is the definition error
    option description(c_string _description);
    option option_text(c_string _text);
    option group(c_string _group);
    option type_name(c_string _type_name);
    option envname(c_string _name);

    /**
     * @brief Makes the type name be written when help asks for it; CLI11's
     *        type_name_fn().
     *
     * @param[in] _function  a function object taking nothing and returning
     *                       the name as a std::string, a C string or a text;
     *                       copied, the option owning the copy.
     * @return the handle.
     */
    template<typename Function>
    option type_name_fn(Function _function);

    // counts: CLI11's expected(), type_size() and allow_extra_args()
    option expected(int _count) D_NOEXCEPT;
    option expected(int _minimum,
                    int _maximum) D_NOEXCEPT;
    option type_size(int _size) D_NOEXCEPT;
    option type_size(int _minimum,
                     int _maximum) D_NOEXCEPT;
    option allow_extra_args(bool _value = true) D_NOEXCEPT;

    // policies: CLI11's multi_option_policy() and its shorthands,
    // delimiter() and callback_priority(), and the integer rule the
    // option's values convert by
    option multi_option_policy(
        multi_option_policy::value _policy =
            multi_option_policy::throw_error) D_NOEXCEPT;
    option take_last() D_NOEXCEPT;
    option take_first() D_NOEXCEPT;
    option take_all() D_NOEXCEPT;
    option join() D_NOEXCEPT;
    option join(char _delimiter) D_NOEXCEPT;
    option delimiter(char _delimiter = '\0') D_NOEXCEPT;
    option callback_priority(
        callback_priority::value _priority =
            callback_priority::normal) D_NOEXCEPT;
    option integer_rule(integer_rule::value _rule) D_NOEXCEPT;

    // switches: CLI11's; ignore_case and ignore_underscore refuse, as the
    // definition error, a setting that makes a name another option's
    option required(bool _value = true) D_NOEXCEPT;
    option mandatory(bool _value = true) D_NOEXCEPT;
    option configurable(bool _value = true) D_NOEXCEPT;
    option disable_flag_override(bool _value = true) D_NOEXCEPT;
    option always_capture_default(bool _value = true) D_NOEXCEPT;
    option trigger_on_parse(bool _value = true) D_NOEXCEPT;
    option force_callback(bool _value = true) D_NOEXCEPT;
    option run_callback_for_default(bool _value = true) D_NOEXCEPT;
    option inject_separator(bool _value = true) D_NOEXCEPT;
    option ignore_case(bool _value = true) D_NOEXCEPT;
    option ignore_underscore(bool _value = true) D_NOEXCEPT;

    // defaults: CLI11's default_str(), default_val() -- the value written as
    // its type writes it, text as it is, then checked as a value -- and
    // capture_default_str(); a failure is the definition error
    option default_str(c_string _text);
    template<typename Type>
    option default_val(const Type& _value);
    option default_val(const char* _value);
    option capture_default_str();

    /**
     * @brief Sets how the default is captured; CLI11's default_function().
     *
     * @param[in] _function  a function object taking nothing and returning
     *                       the default as a std::string, a C string or a
     *                       text; copied, the option owning the copy.
     * @return the handle.
     */
    template<typename Function>
    option default_function(Function _function);

    // relations: CLI11's needs() and excludes(), by handle or by name --
    // from C++11, several at once -- and their removal, which answers
    // whether there was one
    option needs(const option& _other);
    option needs(c_string _name);
    option excludes(const option& _other);
    option excludes(c_string _name);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename First,
             typename Second,
             typename... Rest>
    option needs(const First&   _first,
                 const Second&  _second,
                 const Rest&... _rest);
    template<typename First,
             typename Second,
             typename... Rest>
    option excludes(const First&   _first,
                    const Second&  _second,
                    const Rest&... _rest);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
    bool   remove_needs(const option& _other) D_NOEXCEPT;
    bool   remove_excludes(const option& _other) D_NOEXCEPT;

    // validators: CLI11's check() and transform(), given a validator --
    // copied, or, from C++11, moved -- or a function: check()'s judges a
    // copy of the value, as CLI11's std::string(const std::string&); a
    // transformation's returns the new value, as CLI11's
    // std::string(std::string). A validator that failed, or could not be
    // copied, is the definition error. A validator is not a function here:
    // its description and name are its own, set on it.
    option check(const validator& _validator,
                 c_string         _name = c_string());
    option transform(const validator& _validator,
                     c_string         _name = c_string());
    option transform(const validator& _validator,
                     c_string         _description,
                     c_string         _name);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    option check(validator&& _validator,
                 c_string    _name = c_string());
    option transform(validator&& _validator,
                     c_string    _name = c_string());
    option transform(validator&& _validator,
                     c_string    _description,
                     c_string    _name);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename Function>
    option check(Function _function,
                 c_string _description = c_string(),
                 c_string _name        = c_string());
    template<typename Function>
    option transform(Function _function,
                     c_string _description = c_string(),
                     c_string _name        = c_string());

    /**
     * @brief Runs a function on each value as it is validated; CLI11's
     *        each().
     *
     * @note The function is called with the value as a std::string, and
     *       returns nothing or a cli::status: validation_error rejects the
     *       value; any other status but ok ends the parse with it. An
     *       exception rejects it with what() as the reason, as CLI11's
     *       ValidationError.
     *
     * @param[in] _function  the function object; copied, the option owning
     *                       the copy.
     * @return the handle.
     */
    template<typename Function>
    option each(Function _function);

    // the validators held: CLI11's get_validator(name) and
    // get_validator(index); a NULL handle when there is none
    validator_handle get_validator(c_string _name = c_string()) const
                         D_NOEXCEPT;
    validator_handle get_validator(std::size_t _index) const D_NOEXCEPT;

    /**
     * @brief Sets the function the option's values are given to, as CLI11's
     *        callback_t.
     *
     * @note The function is called with the values as a cli::text_list --
     *       or a std::vector<std::string>, to which the list converts --
     *       after validation and the multi-option policy, and returns
     *       nothing, a cli::status, or a bool, false being CLI11's
     *       "Could not convert". A variable bound to the option is written
     *       before it is called.
     *
     * @param[in] _function  the function object; copied, the option owning
     *                       the copy.
     * @return the handle.
     */
    template<typename Function>
    option callback(Function _function);

    // binding: the variable an option's values convert into, as CLI11's
    // add_option(name, variable) binds one, through type_traits; the
    // variable must outlive the option's use. unbind() drops it.
    template<typename Type>
    option bind(Type& _variable);
    option unbind() D_NOEXCEPT;

    // text, read: never NULL, "" when unset; get_type_name() and get_name()
    // compose, as CLI11's do, and are empty when they cannot be built
    const char* get_description() const D_NOEXCEPT;
    const char* get_option_text() const D_NOEXCEPT;
    const char* get_group() const D_NOEXCEPT;
    const char* get_envname() const D_NOEXCEPT;
    const char* get_default_str() const D_NOEXCEPT;
    const char* get_single_name() const D_NOEXCEPT;
    const char* get_positional_name() const D_NOEXCEPT;
    std::string get_type_name() const;
    std::string get_name(bool _positional                  = false,
                         bool _all_options                 = false,
                         bool _disable_default_flag_values = false) const;
    name_list   get_lnames() const D_NOEXCEPT;
    name_list   get_snames() const D_NOEXCEPT;
    name_list   get_fnames() const D_NOEXCEPT;
    bool        has_description() const D_NOEXCEPT;

    // switches and counts, read
    bool                       get_required() const D_NOEXCEPT;
    bool                       get_ignore_case() const D_NOEXCEPT;
    bool                       get_ignore_underscore() const D_NOEXCEPT;
    bool                       get_configurable() const D_NOEXCEPT;
    bool                       get_disable_flag_override() const D_NOEXCEPT;
    bool                       get_always_capture_default() const D_NOEXCEPT;
    bool                       get_allow_extra_args() const D_NOEXCEPT;
    bool                       get_trigger_on_parse() const D_NOEXCEPT;
    bool                       get_force_callback() const D_NOEXCEPT;
    bool                       get_run_callback_for_default() const
                                   D_NOEXCEPT;
    bool                       get_inject_separator() const D_NOEXCEPT;
    char                       get_delimiter() const D_NOEXCEPT;
    multi_option_policy::value get_multi_option_policy() const D_NOEXCEPT;
    callback_priority::value   get_callback_priority() const D_NOEXCEPT;
    integer_rule::value        get_integer_rule() const D_NOEXCEPT;
    int                        get_type_size() const D_NOEXCEPT;
    int                        get_type_size_min() const D_NOEXCEPT;
    int                        get_type_size_max() const D_NOEXCEPT;
    int                        get_expected() const D_NOEXCEPT;
    int                        get_expected_min() const D_NOEXCEPT;
    int                        get_expected_max() const D_NOEXCEPT;
    int                        get_items_expected() const D_NOEXCEPT;
    int                        get_items_expected_min() const D_NOEXCEPT;
    int                        get_items_expected_max() const D_NOEXCEPT;
    bool                       get_positional() const D_NOEXCEPT;
    bool                       nonpositional() const D_NOEXCEPT;

    // relations, read: the options needed and excluded, as CLI11's
    // get_needs() and get_excludes(), valid until they change
    basic_handle_list<option, ::d_arg_option> get_needs() const D_NOEXCEPT;
    basic_handle_list<option, ::d_arg_option> get_excludes() const
                                                  D_NOEXCEPT;

    // names, matched: CLI11's check_name(), check_sname(), check_lname()
    // and check_fname(), and matching_name(), NULL when the two share none
    bool        check_name(const text& _name) const D_NOEXCEPT;
    bool        check_sname(const text& _name) const D_NOEXCEPT;
    bool        check_lname(const text& _name) const D_NOEXCEPT;
    bool        check_fname(const text& _name) const D_NOEXCEPT;
    const char* matching_name(const option& _other) const D_NOEXCEPT;

    /**
     * @brief Writes the value a flag name given with an input stands for;
     *        CLI11's get_flag_value.
     *
     * @param[in]  _name   the name the flag was given by, without dashes.
     * @param[in]  _input  the input after `=`, or empty.
     * @param[out] _out    receives the value, replacing what it held.
     * @return status_code::ok; argument_mismatch for a disallowed override,
     *         as CLI11 throws; out_of_memory when the value could not be
     *         built.
     */
    cli::status get_flag_value(const text&  _name,
                               const text&  _input,
                               std::string& _out) const;

    // results: CLI11's count(), empty(), results() and reduced_results(),
    // the values as given and as the policy kept them; is_given() is
    // CLI11's operator bool. The lists are valid until the values change.
    std::size_t count() const D_NOEXCEPT;
    bool        empty() const D_NOEXCEPT;
    bool        is_given() const D_NOEXCEPT;
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_EXPLICIT_BOOL operator bool() const D_NOEXCEPT;
#endif
    text_list   results() const D_NOEXCEPT;
    text_list   values() const D_NOEXCEPT;
    text_list   reduced_results() const D_NOEXCEPT;
    bool        get_callback_run() const D_NOEXCEPT;

    /**
     * @brief Converts the values into a variable; CLI11's results(T&).
     *
     * @note Reduces the values first, as their callback would; with no
     *       values, the default is converted, else nothing.
     *
     * @param[out] _output  receives the value; written only on success.
     * @return status_code::ok; conversion_error, "Could not convert: <name>
     *         = <values>"; or a status of reduce().
     */
    template<typename Type>
    cli::status results(Type& _output) const;

    /**
     * @brief Converts the values into a value of a type; CLI11's as<T>().
     *
     * @note CLI11 throws when the values do not convert; here the result is
     *       then Type(), and results(T&) gives the failure.
     *
     * @return the converted value, or Type().
     */
    template<typename Type>
    Type as() const;

    // working the values: CLI11's add_result() -- each value split as a
    // given one is -- run_callback() and clear(); reduce() validates and
    // reduces the values now. Each status is the C call's.
    cli::status add_result(const text& _value) const;
    cli::status add_result(const text& _value,
                           int&        _results_added) const;
    cli::status add_result(const std::vector<std::string>& _values) const;
    cli::status reduce() const;
    cli::status run_callback() const;
    void        clear() const D_NOEXCEPT;

    // failing from a callback: records why the option's callback ends the
    // parse, naming the option, and returns the status, for the callback
    // to return; the message's own text when none is given
    cli::status fail(const cli::status& _status,
                     c_string           _message = c_string()) const;

private:
    ::d_arg_option* m_option;
};

// 1.1.2
// option_list
//   type: a view of options a record refers to, whose elements are handles.
typedef basic_handle_list<option, ::d_arg_option> option_list;

// 1.2    Comparing handles
//------------------------------------------------------------------------------
// 1.2.1
// operator== / operator!=
//   functions: two handles are equal when they refer to the same option, as
// two of CLI11's Option* are.
bool operator==(const option& _first,
                const option& _second) D_NOEXCEPT;
bool operator!=(const option& _first,
                const option& _second) D_NOEXCEPT;

// 1.3    Layout
//------------------------------------------------------------------------------
// 1.3.1
// The handle is one pointer
//   assertion: an option handle costs what CLI11's Option* costs.
D_STATIC_ASSERT( (sizeof(option) == sizeof(::d_arg_option*)),
                 "cli::option is one pointer" );


//==============================================================================
// 2.  INTERNALS
//==============================================================================


// 2.1    Callbacks
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.1.1
    // callback_failure
    //   class: the handler of guard for an option's callback: memory that ran
    //   out is status_code::out_of_memory; any other exception ends the parse
    //   with the status given -- a validation error for each(), a conversion
    //   error for the callback over the values -- what() its message, or,
    //   with no words, the status's own.
    class callback_failure
    {
    public:
        typedef int result_type;

        callback_failure(const ::d_arg_option* _option,
                         int                   _status) D_NOEXCEPT;

        int operator()(const std::exception* _exception,
                       bool                  _out_of_memory) const;

    private:
        const ::d_arg_option* m_option;
        int                   m_status;
    };

    // 2.1.2
    // each_call / each_function
    //   class, function: an each() function's work, as d_arg_each_fn: the
    //   value as a std::string, given to the function.
    template<typename Function>
    class each_call
    {
    public:
        typedef int result_type;

        each_call(::d_pack_text _value,
                  void*         _context) D_NOEXCEPT;

        int operator()() const;

    private:
        ::d_pack_text m_value;
        Function*     m_function;
    };
    template<typename Function>
    int each_function(const ::d_arg_option* _option,
                      ::d_pack_text         _value,
                      void*                 _context);

    // 2.1.3
    // results_call / results_function
    //   class, function: a callback over the values, as d_arg_results_fn:
    //   the values as a text_list, given to the function.
    template<typename Function>
    class results_call
    {
    public:
        typedef int result_type;

        results_call(const ::d_pack_text* _results,
                     std::size_t          _count,
                     void*                _context) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_pack_text* m_results;
        std::size_t          m_count;
        Function*            m_function;
    };
    template<typename Function>
    int results_function(::d_arg_option*      _option,
                         const ::d_pack_text* _results,
                         std::size_t          _count,
                         void*                _context);

    // 2.1.4
    // text_call / text_function
    //   class, function: a function writing an option's text -- its type
    //   name, its default -- as d_arg_option_text_fn: what it returns,
    //   written to the sink as a text.
    template<typename Function>
    class text_call
    {
    public:
        typedef int result_type;

        text_call(::d_pack_sink _sink,
                  void*         _context) D_NOEXCEPT;

        int operator()() const;

        template<typename Result>
        int write(const Result& _result) const;

    private:
        ::d_pack_sink m_sink;
        Function*     m_function;
    };
    template<typename Function>
    int text_function(const ::d_arg_option* _option,
                      ::d_pack_sink         _sink,
                      void*                 _context);

NS_END  // internal

// 2.2    Transformations
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.2.1
    // is_validator
    //   traits: whether a type is a validator, or a handle on one, which the
    //   function forms of check() and transform() refuse, as `value`.
    template<typename Type>
    struct is_validator
    {
        enum
        {
            value = 0
        };
    };

    template<>
    struct is_validator<validator>
    {
        enum
        {
            value = 1
        };
    };

    template<>
    struct is_validator<validator_handle>
    {
        enum
        {
            value = 1
        };
    };

    // 2.2.2
    // transform_call
    //   class: a transformation function as a validator's function: the
    //   value replaced with what it returns, as CLI11's transform(func)
    //   wraps it.
    template<typename Function>
    class transform_call
    {
    public:
        explicit transform_call(const Function& _function);

        std::string operator()(std::string& _value);

    private:
        Function m_function;
    };

NS_END  // internal


//==============================================================================
// 3.  DEFINITIONS
//==============================================================================


// 3.1    option
//------------------------------------------------------------------------------
// option::option / operator-> / operator* / get / valid
//   The pointer; the handle itself, for CLI11's arrow and star.
inline
option::option() D_NOEXCEPT
    : m_option(D_NULLPTR)
{}

inline
option::option(
    ::d_arg_option* _option
) D_NOEXCEPT
    : m_option(_option)
{}

inline option*
option::operator->() D_NOEXCEPT
{
    return this;
}

inline const option*
option::operator->() const D_NOEXCEPT
{
    return this;
}

inline option&
option::operator*() D_NOEXCEPT
{
    return *this;
}

inline const option&
option::operator*() const D_NOEXCEPT
{
    return *this;
}

inline ::d_arg_option*
option::get() const D_NOEXCEPT
{
    return m_option;
}

inline bool
option::valid() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR);
}

// option::description and kin
//   The C text settings, whose failures the C library records.
inline option
option::description(
    c_string _description
)
{
    (void)d_arg_option_set_description(m_option,
                                       _description.get());

    return *this;
}

inline option
option::option_text(
    c_string _text
)
{
    (void)d_arg_option_set_option_text(m_option,
                                       _text.get());

    return *this;
}

inline option
option::group(
    c_string _group
)
{
    (void)d_arg_option_set_group(m_option,
                                 _group.get());

    return *this;
}

inline option
option::type_name(
    c_string _type_name
)
{
    (void)d_arg_option_set_type_name(m_option,
                                     _type_name.get());

    return *this;
}

inline option
option::envname(
    c_string _name
)
{
    (void)d_arg_option_set_env_name(m_option,
                                    _name.get());

    return *this;
}

// option::type_name_fn
//   A copy of the function, owned by the option through its release
// function, written by text_function.
template<typename Function>
inline option
option::type_name_fn(
    Function _function
)
{
    d_arg_option_set_type_name_fn(m_option,
                                  &internal::text_function<Function>,
                                  new Function(_function),
                                  &internal::release_context<Function>);

    return *this;
}

// option::expected and kin
//   The C count settings.
inline option
option::expected(
    int _count
) D_NOEXCEPT
{
    d_arg_option_set_expected(m_option,
                              _count);

    return *this;
}

inline option
option::expected(
    int _minimum,
    int _maximum
) D_NOEXCEPT
{
    d_arg_option_set_expected_range(m_option,
                                    _minimum,
                                    _maximum);

    return *this;
}

inline option
option::type_size(
    int _size
) D_NOEXCEPT
{
    d_arg_option_set_type_size(m_option,
                               _size);

    return *this;
}

inline option
option::type_size(
    int _minimum,
    int _maximum
) D_NOEXCEPT
{
    d_arg_option_set_type_size_range(m_option,
                                     _minimum,
                                     _maximum);

    return *this;
}

inline option
option::allow_extra_args(
    bool _value
) D_NOEXCEPT
{
    d_arg_option_set_allow_extra_args(m_option,
                                      _value);

    return *this;
}

// option::multi_option_policy and kin
//   The C policy settings; join(char) sets the delimiter, then the policy,
// as CLI11's does.
inline option
option::multi_option_policy(
    multi_option_policy::value _policy
) D_NOEXCEPT
{
    d_arg_option_set_multi_option_policy(m_option,
                                         static_cast<int>(_policy));

    return *this;
}

inline option
option::take_last() D_NOEXCEPT
{
    return multi_option_policy(multi_option_policy::take_last);
}

inline option
option::take_first() D_NOEXCEPT
{
    return multi_option_policy(multi_option_policy::take_first);
}

inline option
option::take_all() D_NOEXCEPT
{
    return multi_option_policy(multi_option_policy::take_all);
}

inline option
option::join() D_NOEXCEPT
{
    return multi_option_policy(multi_option_policy::join);
}

inline option
option::join(
    char _delimiter
) D_NOEXCEPT
{
    d_arg_option_set_delimiter(m_option,
                               _delimiter);

    return multi_option_policy(multi_option_policy::join);
}

inline option
option::delimiter(
    char _delimiter
) D_NOEXCEPT
{
    d_arg_option_set_delimiter(m_option,
                               _delimiter);

    return *this;
}

inline option
option::callback_priority(
    callback_priority::value _priority
) D_NOEXCEPT
{
    d_arg_option_set_callback_priority(m_option,
                                       static_cast<int>(_priority));

    return *this;
}

inline option
option::integer_rule(
    integer_rule::value _rule
) D_NOEXCEPT
{
    d_arg_option_set_integer_rule(m_option,
                                  static_cast<int>(_rule));

    return *this;
}

// option::required and kin
//   The C switches; mandatory is required, as in CLI11.
inline option
option::required(
    bool _value
) D_NOEXCEPT
{
    d_arg_option_set_required(m_option,
                              _value);

    return *this;
}

inline option
option::mandatory(
    bool _value
) D_NOEXCEPT
{
    return required(_value);
}

inline option
option::configurable(
    bool _value
) D_NOEXCEPT
{
    d_arg_option_set_configurable(m_option,
                                  _value);

    return *this;
}

inline option
option::disable_flag_override(
    bool _value
) D_NOEXCEPT
{
    d_arg_option_set_disable_flag_override(m_option,
                                           _value);

    return *this;
}

inline option
option::always_capture_default(
    bool _value
) D_NOEXCEPT
{
    d_arg_option_set_always_capture_default(m_option,
                                            _value);

    return *this;
}

inline option
option::trigger_on_parse(
    bool _value
) D_NOEXCEPT
{
    d_arg_option_set_trigger_on_parse(m_option,
                                      _value);

    return *this;
}

inline option
option::force_callback(
    bool _value
) D_NOEXCEPT
{
    d_arg_option_set_force_callback(m_option,
                                    _value);

    return *this;
}

inline option
option::run_callback_for_default(
    bool _value
) D_NOEXCEPT
{
    d_arg_option_set_run_callback_for_default(m_option,
                                              _value);

    return *this;
}

inline option
option::inject_separator(
    bool _value
) D_NOEXCEPT
{
    d_arg_option_set_inject_separator(m_option,
                                      _value);

    return *this;
}

inline option
option::ignore_case(
    bool _value
) D_NOEXCEPT
{
    (void)d_arg_option_set_ignore_case(m_option,
                                       _value);

    return *this;
}

inline option
option::ignore_underscore(
    bool _value
) D_NOEXCEPT
{
    (void)d_arg_option_set_ignore_underscore(m_option,
                                             _value);

    return *this;
}

// option::default_str / default_val / capture_default_str /
// default_function
//   The C default settings; default_val given a value through the type's
// traits, so that the C library writes it, and text as it is.
inline option
option::default_str(
    c_string _text
)
{
    (void)d_arg_option_set_default_text(m_option,
                                        _text.get());

    return *this;
}

template<typename Type>
inline option
option::default_val(
    const Type& _value
)
{
    (void)d_arg_option_set_default_typed(m_option,
                                         type_traits<Type>::descriptor(),
                                         type_traits<Type>::source(_value));

    return *this;
}

inline option
option::default_val(
    const char* _value
)
{
    (void)d_arg_option_set_default_value(m_option,
                                         _value);

    return *this;
}

inline option
option::capture_default_str()
{
    (void)d_arg_option_capture_default(m_option);

    return *this;
}

template<typename Function>
inline option
option::default_function(
    Function _function
)
{
    d_arg_option_set_default_fn(m_option,
                                &internal::text_function<Function>,
                                new Function(_function),
                                &internal::release_context<Function>);

    return *this;
}

// option::needs / excludes / remove_needs / remove_excludes
//   The C relations, by record or by name; several, one at a time, in
// order, as CLI11's.
inline option
option::needs(
    const option& _other
)
{
    (void)d_arg_option_needs(m_option,
                             _other.get());

    return *this;
}

inline option
option::needs(
    c_string _name
)
{
    (void)d_arg_option_needs_name(m_option,
                                  _name.get());

    return *this;
}

inline option
option::excludes(
    const option& _other
)
{
    (void)d_arg_option_excludes(m_option,
                                _other.get());

    return *this;
}

inline option
option::excludes(
    c_string _name
)
{
    (void)d_arg_option_excludes_name(m_option,
                                     _name.get());

    return *this;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename First,
         typename Second,
         typename... Rest>
inline option
option::needs(
    const First&   _first,
    const Second&  _second,
    const Rest&... _rest
)
{
    (void)needs(_first);

    return needs(_second,
                 _rest...);
}

template<typename First,
         typename Second,
         typename... Rest>
inline option
option::excludes(
    const First&   _first,
    const Second&  _second,
    const Rest&... _rest
)
{
    (void)excludes(_first);

    return excludes(_second,
                    _rest...);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

inline bool
option::remove_needs(
    const option& _other
) D_NOEXCEPT
{
    return d_arg_option_remove_needs(m_option,
                                     _other.get());
}

inline bool
option::remove_excludes(
    const option& _other
) D_NOEXCEPT
{
    return d_arg_option_remove_excludes(m_option,
                                        _other.get());
}

// option::check / transform
//   A copy of the validator, or the one moved, handed to the C call, which
// consumes it; a function made a validator first, a transformation's
// wrapped to replace the value.
inline option
option::check(
    const validator& _validator,
    c_string         _name
)
{
    validator held(_validator);

    (void)d_arg_option_check(m_option,
                             held.detach(),
                             _name.get());

    return *this;
}

inline option
option::transform(
    const validator& _validator,
    c_string         _name
)
{
    validator held(_validator);

    (void)d_arg_option_transform(m_option,
                                 held.detach(),
                                 _name.get(),
                                 D_NULLPTR);

    return *this;
}

inline option
option::transform(
    const validator& _validator,
    c_string         _description,
    c_string         _name
)
{
    validator held(_validator);

    (void)d_arg_option_transform(m_option,
                                 held.detach(),
                                 _name.get(),
                                 _description.get());

    return *this;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

inline option
option::check(
    validator&& _validator,
    c_string    _name
)
{
    (void)d_arg_option_check(m_option,
                             _validator.detach(),
                             _name.get());

    return *this;
}

inline option
option::transform(
    validator&& _validator,
    c_string    _name
)
{
    (void)d_arg_option_transform(m_option,
                                 _validator.detach(),
                                 _name.get(),
                                 D_NULLPTR);

    return *this;
}

inline option
option::transform(
    validator&& _validator,
    c_string    _description,
    c_string    _name
)
{
    (void)d_arg_option_transform(m_option,
                                 _validator.detach(),
                                 _name.get(),
                                 _description.get());

    return *this;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename Function>
inline option
option::check(
    Function _function,
    c_string _description,
    c_string _name
)
{
    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        (!internal::is_validator<Function>::value),
        "a validator is checked by check(validator, name)" );

    validator made(_function,
                   _description,
                   _name);

    (void)d_arg_option_check(m_option,
                             made.detach(),
                             D_NULLPTR);

    return *this;
}

template<typename Function>
inline option
option::transform(
    Function _function,
    c_string _description,
    c_string _name
)
{
    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        (!internal::is_validator<Function>::value),
        "a validator transforms by transform(validator, name)" );

    validator made(internal::transform_call<Function>(_function),
                   _description,
                   _name);

    (void)d_arg_option_transform(m_option,
                                 made.detach(),
                                 D_NULLPTR,
                                 D_NULLPTR);

    return *this;
}

// option::each
//   A copy of the function, owned by the option, called by each_function.
template<typename Function>
inline option
option::each(
    Function _function
)
{
    (void)d_arg_option_each(m_option,
                            &internal::each_function<Function>,
                            new Function(_function),
                            &internal::release_context<Function>);

    return *this;
}

// option::get_validator
//   The C lookups, as handles.
inline validator_handle
option::get_validator(
    c_string _name
) const D_NOEXCEPT
{
    return validator_handle(d_arg_option_validator(m_option,
                                                   _name.get()));
}

inline validator_handle
option::get_validator(
    std::size_t _index
) const D_NOEXCEPT
{
    return validator_handle(d_arg_option_validator_at(m_option,
                                                      _index));
}

// option::callback
//   A copy of the function, owned by the option, called by
// results_function.
template<typename Function>
inline option
option::callback(
    Function _function
)
{
    d_arg_option_set_callback(m_option,
                              &internal::results_function<Function>,
                              new Function(_function),
                              &internal::release_context<Function>);

    return *this;
}

// option::bind / unbind
//   d_arg_option_bind through the type's traits; with no variable.
template<typename Type>
inline option
option::bind(
    Type& _variable
)
{
    (void)d_arg_option_bind(m_option,
                            type_traits<Type>::target(_variable),
                            type_traits<Type>::descriptor());

    return *this;
}

inline option
option::unbind() D_NOEXCEPT
{
    (void)d_arg_option_bind(m_option,
                            D_NULLPTR,
                            D_NULLPTR);

    return *this;
}

// option::get_description and kin
//   The C lookups, which read NULL as an option with nothing set; the
// single and positional names; names composed into a scratch buffer.
inline const char*
option::get_description() const D_NOEXCEPT
{
    return d_arg_option_description(m_option);
}

inline const char*
option::get_option_text() const D_NOEXCEPT
{
    return d_arg_option_option_text(m_option);
}

inline const char*
option::get_group() const D_NOEXCEPT
{
    return d_arg_option_group(m_option);
}

inline const char*
option::get_envname() const D_NOEXCEPT
{
    return d_arg_option_env_name(m_option);
}

inline const char*
option::get_default_str() const D_NOEXCEPT
{
    return d_arg_option_default_text(m_option);
}

inline const char*
option::get_single_name() const D_NOEXCEPT
{
    return d_arg_option_single_name(m_option);
}

inline const char*
option::get_positional_name() const D_NOEXCEPT
{
    return d_arg_option_positional_name(m_option);
}

inline std::string
option::get_type_name() const
{
    internal::scratch written;

    return (d_arg_option_type_name(m_option,
                                   written.get()) == D_ARG_OK)
           ? written.str()
           : std::string();
}

inline std::string
option::get_name(
    bool _positional,
    bool _all_options,
    bool _disable_default_flag_values
) const
{
    const re_std::uint32_t form =
        (_positional ? D_ARG_NAME_POSITIONAL : 0u) |
        (_all_options ? D_ARG_NAME_ALL : 0u) |
        (_disable_default_flag_values ? D_ARG_NAME_NO_FLAG_VALUES : 0u);

    internal::scratch written;

    return (d_arg_option_name(m_option,
                              form,
                              written.get()) == D_ARG_OK)
           ? written.str()
           : std::string();
}

// option::get_lnames / get_snames / get_fnames / has_description
//   The C option's name lists, read in place.
inline name_list
option::get_lnames() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR)
           ? name_list(m_option->long_names.items,
                       m_option->long_names.count)
           : name_list();
}

inline name_list
option::get_snames() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR)
           ? name_list(m_option->short_names.items,
                       m_option->short_names.count)
           : name_list();
}

inline name_list
option::get_fnames() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR)
           ? name_list(m_option->flag_names.items,
                       m_option->flag_names.count)
           : name_list();
}

inline bool
option::has_description() const D_NOEXCEPT
{
    return (*d_arg_option_description(m_option) != '\0');
}

// option::get_required and kin
//   The C option's switches and counts, read; a NULL handle reads as an
// option with nothing set, the defaults of an option made bare.
inline bool
option::get_required() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_REQUIRED);
}

inline bool
option::get_ignore_case() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_IGNORE_CASE);
}

inline bool
option::get_ignore_underscore() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_IGNORE_UNDERSCORE);
}

inline bool
option::get_configurable() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_CONFIGURABLE);
}

inline bool
option::get_disable_flag_override() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_DISABLE_FLAG_OVERRIDE);
}

inline bool
option::get_always_capture_default() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT);
}

inline bool
option::get_allow_extra_args() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_ALLOW_EXTRA_ARGS);
}

inline bool
option::get_trigger_on_parse() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_TRIGGER_ON_PARSE);
}

inline bool
option::get_force_callback() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_FORCE_CALLBACK);
}

inline bool
option::get_run_callback_for_default() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT);
}

inline bool
option::get_inject_separator() const D_NOEXCEPT
{
    return d_arg_option_has_flag(m_option,
                                 D_ARG_OPTION_INJECT_SEPARATOR);
}

inline char
option::get_delimiter() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR) ? m_option->delimiter : '\0';
}

inline multi_option_policy::value
option::get_multi_option_policy() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR)
           ? static_cast<multi_option_policy::value>(
                 m_option->multi_option_policy)
           : multi_option_policy::throw_error;
}

inline callback_priority::value
option::get_callback_priority() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR)
           ? static_cast<callback_priority::value>(
                 m_option->callback_priority)
           : callback_priority::normal;
}

inline integer_rule::value
option::get_integer_rule() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR)
           ? static_cast<integer_rule::value>(m_option->integer_rule)
           : static_cast<integer_rule::value>(
                 D_INTERNAL_ARG_PARSER_INTEGER_RULE);
}

inline int
option::get_type_size() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR) ? m_option->type_size_min : 0;
}

inline int
option::get_type_size_min() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR) ? m_option->type_size_min : 0;
}

inline int
option::get_type_size_max() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR) ? m_option->type_size_max : 0;
}

inline int
option::get_expected() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR) ? m_option->expected_min : 0;
}

inline int
option::get_expected_min() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR) ? m_option->expected_min : 0;
}

inline int
option::get_expected_max() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR) ? m_option->expected_max : 0;
}

inline int
option::get_items_expected() const D_NOEXCEPT
{
    return d_arg_option_items_expected_min(m_option);
}

inline int
option::get_items_expected_min() const D_NOEXCEPT
{
    return d_arg_option_items_expected_min(m_option);
}

inline int
option::get_items_expected_max() const D_NOEXCEPT
{
    return d_arg_option_items_expected_max(m_option);
}

inline bool
option::get_positional() const D_NOEXCEPT
{
    return d_arg_option_is_positional(m_option);
}

inline bool
option::nonpositional() const D_NOEXCEPT
{
    return d_arg_option_is_nonpositional(m_option);
}

// option::get_needs / get_excludes
//   The C option's lists, read in place.
inline option_list
option::get_needs() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR)
           ? option_list(m_option->needs.items,
                         m_option->needs.count)
           : option_list();
}

inline option_list
option::get_excludes() const D_NOEXCEPT
{
    return (m_option != D_NULLPTR)
           ? option_list(m_option->excludes.items,
                         m_option->excludes.count)
           : option_list();
}

// option::check_name and kin / matching_name
//   The C matching.
inline bool
option::check_name(
    const text& _name
) const D_NOEXCEPT
{
    return d_arg_option_check_name(m_option,
                                   _name);
}

inline bool
option::check_sname(
    const text& _name
) const D_NOEXCEPT
{
    return d_arg_option_check_short_name(m_option,
                                         _name);
}

inline bool
option::check_lname(
    const text& _name
) const D_NOEXCEPT
{
    return d_arg_option_check_long_name(m_option,
                                        _name);
}

inline bool
option::check_fname(
    const text& _name
) const D_NOEXCEPT
{
    return d_arg_option_check_flag_name(m_option,
                                        _name);
}

inline const char*
option::matching_name(
    const option& _other
) const D_NOEXCEPT
{
    return d_arg_option_matching_name(m_option,
                                      _other.get());
}

// option::get_flag_value
//   The value written into a scratch buffer, then copied out.
inline cli::status
option::get_flag_value(
    const text&  _name,
    const text&  _input,
    std::string& _out
) const
{
    internal::scratch written;

    const cli::status result(d_arg_option_flag_value(m_option,
                                                     _name,
                                                     _input,
                                                     written.get()));

    // the value replaces the output only when it was made
    if (result.ok())
    {
        _out = written.str();
    }

    return result;
}

// option::count and kin
//   The C results, as lists over the option's own arrays.
inline std::size_t
option::count() const D_NOEXCEPT
{
    return d_arg_option_count(m_option);
}

inline bool
option::empty() const D_NOEXCEPT
{
    return (d_arg_option_count(m_option) == 0u);
}

inline bool
option::is_given() const D_NOEXCEPT
{
    return d_arg_option_is_given(m_option);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

inline
option::operator bool() const D_NOEXCEPT
{
    return is_given();
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

inline text_list
option::results() const D_NOEXCEPT
{
    std::size_t                size  = 0u;
    const ::d_pack_text* const items = d_arg_option_results(m_option,
                                                            &size);

    return text_list(items,
                     size);
}

inline text_list
option::values() const D_NOEXCEPT
{
    std::size_t                size  = 0u;
    const ::d_pack_text* const items = d_arg_option_values(m_option,
                                                           &size);

    return text_list(items,
                     size);
}

inline text_list
option::reduced_results() const D_NOEXCEPT
{
    (void)d_arg_option_reduce(m_option);

    return values();
}

inline bool
option::get_callback_run() const D_NOEXCEPT
{
    return d_arg_option_callback_run(m_option);
}

// option::results (into a variable) / as
//   d_arg_option_as through the type's traits; as() into a Type() it
// returns whatever came of it.
template<typename Type>
inline cli::status
option::results(
    Type& _output
) const
{
    return cli::status(d_arg_option_as(m_option,
                                       type_traits<Type>::descriptor(),
                                       type_traits<Type>::target(_output)));
}

template<typename Type>
inline Type
option::as() const
{
    Type converted = Type();

    (void)results(converted);

    return converted;
}

// option::add_result and kin / reduce / run_callback / clear
//   The C calls; a vector's values added one by one, the first failure
// ending it.
inline cli::status
option::add_result(
    const text& _value
) const
{
    return cli::status(d_arg_option_add_result(m_option,
                                               _value,
                                               D_NULLPTR));
}

inline cli::status
option::add_result(
    const text& _value,
    int&        _results_added
) const
{
    return cli::status(d_arg_option_add_result(m_option,
                                               _value,
                                               &_results_added));
}

inline cli::status
option::add_result(
    const std::vector<std::string>& _values
) const
{
    cli::status added;

    // each value as the command line would give it
    for (std::size_t index = 0u;
         ( (added.ok()) &&
           (index < _values.size()) );
         ++index)
    {
        added = add_result(text(_values[index]));
    }

    return added;
}

inline cli::status
option::reduce() const
{
    return cli::status(d_arg_option_reduce(m_option));
}

inline cli::status
option::run_callback() const
{
    return cli::status(d_arg_option_run_callback(m_option));
}

inline void
option::clear() const D_NOEXCEPT
{
    d_arg_option_clear(m_option);

    return;
}

// option::fail
//   d_arg_option_fail with the message as its format's one argument, or no
// format for the status's own text.
inline cli::status
option::fail(
    const cli::status& _status,
    c_string           _message
) const
{
    return cli::status((_message.get() != D_NULLPTR)
                       ? d_arg_option_fail(m_option,
                                           _status.code(),
                                           "%s",
                                           _message.get())
                       : d_arg_option_fail(m_option,
                                           _status.code(),
                                           D_NULLPTR));
}

// 3.2    Comparing handles
//------------------------------------------------------------------------------
// operator== / operator!= (option)
//   By record.
inline bool
operator==(
    const option& _first,
    const option& _second
) D_NOEXCEPT
{
    return (_first.get() == _second.get());
}

inline bool
operator!=(
    const option& _first,
    const option& _second
) D_NOEXCEPT
{
    return (_first.get() != _second.get());
}

// 3.3    Internals
//------------------------------------------------------------------------------
// internal::callback_failure
//   Out of memory, or the status given with the exception's words, or with
// none for the status's own.
inline
internal::callback_failure::callback_failure(
    const ::d_arg_option* _option,
    int                   _status
) D_NOEXCEPT
    : m_option(_option),
      m_status(_status)
{}

inline int
internal::callback_failure::operator()(
    const std::exception* _exception,
    bool                  _out_of_memory
) const
{
    // a refused allocation is the C library's out-of-memory error
    if (_out_of_memory)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const char* const words = internal::exception_words(_exception);

    return (words != D_NULLPTR)
           ? d_arg_option_fail(m_option,
                               m_status,
                               "%s",
                               words)
           : d_arg_option_fail(m_option,
                               m_status,
                               D_NULLPTR);
}

// internal::each_call / each_function
//   The value as a string, given to the function, whose result -- nothing
// or a status -- is the each() status.
template<typename Function>
inline
internal::each_call<Function>::each_call(
    ::d_pack_text _value,
    void*         _context
) D_NOEXCEPT
    : m_value(_value),
      m_function(static_cast<Function*>(_context))
{}

template<typename Function>
inline int
internal::each_call<Function>::operator()() const
{
    const int kind = internal::call_kind1<Function, std::string>::value;

    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        ( (kind == internal::result_kind::none)   ||
          (kind == internal::result_kind::status) ),
        "an each() function returns nothing or a status" );

    std::string value = text(m_value).str();

    return internal::call_status<kind>::call(*m_function,
                                             value);
}

template<typename Function>
inline int
internal::each_function(
    const ::d_arg_option* _option,
    ::d_pack_text         _value,
    void*                 _context
)
{
    return internal::guard(internal::each_call<Function>(_value,
                                                         _context),
                           internal::callback_failure(
                               _option,
                               D_ARG_ERROR_VALIDATION));
}

// internal::results_call / results_function
//   The values as a list, given to the function, whose result -- nothing, a
// status or a bool -- is the callback's status.
template<typename Function>
inline
internal::results_call<Function>::results_call(
    const ::d_pack_text* _results,
    std::size_t          _count,
    void*                _context
) D_NOEXCEPT
    : m_results(_results),
      m_count(_count),
      m_function(static_cast<Function*>(_context))
{}

template<typename Function>
inline int
internal::results_call<Function>::operator()() const
{
    const int kind = internal::call_kind1<Function, text_list>::value;

    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        (kind != internal::result_kind::other),
        "an option callback returns nothing, a status or a "
        "bool" );

    text_list results(m_results,
                      m_count);

    return internal::call_status<kind>::call(*m_function,
                                             results);
}

template<typename Function>
inline int
internal::results_function(
    ::d_arg_option*      _option,
    const ::d_pack_text* _results,
    std::size_t          _count,
    void*                _context
)
{
    return internal::guard(internal::results_call<Function>(_results,
                                                            _count,
                                                            _context),
                           internal::callback_failure(
                               _option,
                               D_ARG_ERROR_CONVERSION));
}

// internal::text_call / text_function
//   The function's result, whatever text type it is, written whole.
template<typename Function>
inline
internal::text_call<Function>::text_call(
    ::d_pack_sink _sink,
    void*         _context
) D_NOEXCEPT
    : m_sink(_sink),
      m_function(static_cast<Function*>(_context))
{}

template<typename Function>
inline int
internal::text_call<Function>::operator()() const
{
    return write((*m_function)());
}

template<typename Function>
template<typename Result>
inline int
internal::text_call<Function>::write(
    const Result& _result
) const
{
    const text written(_result);

    return d_arg_sink_write(m_sink,
                            written.data(),
                            written.size());
}

template<typename Function>
inline int
internal::text_function(
    const ::d_arg_option* _option,
    ::d_pack_sink         _sink,
    void*                 _context
)
{
    (void)_option;

    return internal::guard(internal::text_call<Function>(_sink,
                                                         _context),
                           internal::conversion_failure(D_NULLPTR));
}

// internal::transform_call
//   The function kept by value; the value replaced with its result.
template<typename Function>
inline
internal::transform_call<Function>::transform_call(
    const Function& _function
)
    : m_function(_function)
{}

template<typename Function>
inline std::string
internal::transform_call<Function>::operator()(
    std::string& _value
)
{
    _value = m_function(_value);

    return std::string();
}

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_OPTION_HPP
