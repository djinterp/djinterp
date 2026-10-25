/*******************************************************************************
* djinterp [parse]                                               arg_command.hpp
*
* The argument parser's C++ face: commands.
*   CLI11's App is one class for the root, its subcommands and its option
* groups. Here the root is the parser itself (arg_parser.hpp), which owns
* its struct d_arg_parser, and a subcommand or an option group is a handle
* -- one pointer, the cost of CLI11's App* -- on a struct d_arg_command its
* parent owns. All three have CLI11's App interface as members, written
* once in command_interface, a base each derives from, which reads its
* command record from what it derives from and returns, from a setting,
* what its derived class chains on: the parser itself, or the handle.
*   As with options, a handle does not own its command, and a handle on no
* command -- what a definition that failed returns -- is ignored by every
* call; the failure is the parser's definition error. Handles do not carry
* constness: a handle read from a const parser reaches the command as any
* other does, as the C records are reached through their parser.
*   Callbacks are any function object, copied, the command or option
* owning the copy, and called under the guard of arg_common.hpp: a command's
* callbacks may return nothing or a cli::status, and what one throws ends
* the parse as a parse error with what() as its message, where CLI11 lets it
* escape the parse. The queries CLI11 answers with a std::vector or a
* std::string answer the same way, and throw std::bad_alloc, as the standard
* containers do, when memory runs out; with C++ exceptions off, they answer
* nothing instead.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/arg_command.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Names
         1.  name_match
    2.  Failure messages
         1.  failure_message::simple / failure_message::help
2.  FOUNDATIONS
    -----------
    1.  Traits
         1.  is_string / unless_string
    2.  Records
         1.  command_handle
         2.  record_of
         3.  command_result
3.  COMMANDS
    --------
    1.  Option defaults
         1.  option_defaults
    2.  The interface
         1.  command_interface
    3.  Handles
         1.  command
         2.  command_list
         3.  option_group
    4.  Comparing handles
         1.  operator== / operator!=
    5.  Layout
         1.  The handles are one pointer
4.  UTILITIES
    ---------
    1.  Triggers
         1.  trigger_on / trigger_off
    2.  Life cycle of options
         1.  deprecate_option
         2.  retire_option
5.  INTERNALS
    ---------
    1.  Command callbacks
         1.  command_failure
         2.  command_call / command_function
         3.  preparse_call / preparse_function
         4.  command_text_function
         5.  failure_call / failure_function
    2.  Option callbacks
         1.  flag_call / flag_function
         2.  flag_count / count_call / count_function
         3.  typed_call / typed_function
         4.  no_default_text
    3.  Collecting
         1.  raise_out_of_memory
         2.  remaining_collect / remaining_visit
         3.  option_collect / option_visit
         4.  group_collect / group_visit
6.  DEFINITIONS
    -----------
    1.  option_defaults
    2.  command_interface
    3.  command
    4.  option_group
    5.  Comparing handles
    6.  Utilities
    7.  Foundations
    8.  Internals
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMAND_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMAND_HPP 1

// std
#include <cstddef>    // std::size_t
#include <exception>  // std::exception
#include <new>        // std::bad_alloc
#include <string>     // std::string
#include <vector>     // std::vector
// djinterp
#include "../../../djinterp.hpp"          // framework root
#include "../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../../env/env.h"             // language levels
#include "./arg_common.hpp"               // status, text, lists, guard,
                                          // callables, sinks
#include "./arg_option.hpp"               // option, text_call,
                                          // callback_failure
#include "./arg_type.hpp"                 // type_traits, enable_if,
                                          // is_const
#include "./c/arg_command.h"              // d_arg_command
#include "./c/arg_config.h"               // d_arg_command_config_to_sink
#include "./c/arg_formatter.h"            // d_arg_help,
                                          // d_arg_command_formatter
#include "./c/arg_parser.h"               // d_arg_parser,
                                          // d_arg_command_fail
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // intmax_t, INTMAX_MAX


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Names
//------------------------------------------------------------------------------
// 1.1.1
// name_match
//   enum: CLI11's NameMatch, the d_arg_name_match values: how a name matched a
// command's -- not at all, exactly, or as a prefix.
struct name_match
{
    enum value
    {
        none   = D_ARG_NAME_MATCH_NONE,
        exact  = D_ARG_NAME_MATCH_EXACT,
        prefix = D_ARG_NAME_MATCH_PREFIX
    };
};

// 1.2    Failure messages
//------------------------------------------------------------------------------
// 1.2.1
// failure_message::simple / failure_message::help
//   constants: CLI11's FailureMessage::simple, the default -- the error's
// message, then "Run with --help for more information." -- and
// FailureMessage::help, the error and the command's help: the C library's
// d_arg_failure_simple and d_arg_failure_help, for
// command_interface::failure_message.
namespace failure_message
{

D_CONSTEXPR_VAR ::d_arg_failure_fn simple = &::d_arg_failure_simple;
D_CONSTEXPR_VAR ::d_arg_failure_fn help   = &::d_arg_failure_help;

}  // namespace failure_message


//==============================================================================
// 2.  FOUNDATIONS
//==============================================================================


// 2.1    Traits
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.1.1
    // is_string / unless_string
    //   traits: whether an argument given by value is text -- a C string, a
    //   std::string, a text or a c_string -- as `value`, which the settings
    //   taking text or a function tell apart by; and enable_if on its
    //   absence, `type` being Result.
    template<typename Type>
    struct is_string
    {
        enum
        {
            value = 0
        };
    };

    template<>
    struct is_string<const char*>
    {
        enum
        {
            value = 1
        };
    };

    template<>
    struct is_string<char*>
    {
        enum
        {
            value = 1
        };
    };

    template<>
    struct is_string<std::string>
    {
        enum
        {
            value = 1
        };
    };

    template<>
    struct is_string<text>
    {
        enum
        {
            value = 1
        };
    };

    template<>
    struct is_string<c_string>
    {
        enum
        {
            value = 1
        };
    };

    template<typename Function,
             typename Result>
    struct unless_string
        : public enable_if<(!is_string<Function>::value), Result>
    {};

NS_END  // internal

// 2.2    Records
//------------------------------------------------------------------------------
//   The handles, declared here for the interface that returns them.
class command;
class option_group;

NS_INTERNAL

    // 2.2.1
    // command_handle
    //   class: what a command handle derives its interface from: the pointer
    //   to the command record, NULL for none.
    class command_handle
    {
    public:
        command_handle() D_NOEXCEPT;
        explicit command_handle(::d_arg_command* _command) D_NOEXCEPT;

        ::d_arg_command* record() const D_NOEXCEPT;

    private:
        ::d_arg_command* m_command;
    };

    // 2.2.2
    // record_of
    //   functions: the command record of what command_interface derives
    //   from: a parser's root, or a handle's command.
    ::d_arg_command* record_of(const ::d_arg_parser& _parser) D_NOEXCEPT;
    ::d_arg_command* record_of(const command_handle& _handle) D_NOEXCEPT;

    // 2.2.3
    // command_result
    //   trait: what a setting of command_interface returns for the class
    //   deriving it, as `type`: a reference to the object -- the parser --
    //   by default, and a handle by value, as CLI11's App* is.
    template<typename Derived>
    struct command_result
    {
        typedef Derived& type;
    };

    template<>
    struct command_result<command>
    {
        typedef command type;
    };

NS_END  // internal


//==============================================================================
// 3.  COMMANDS
//==============================================================================


// 3.1    Option defaults
//------------------------------------------------------------------------------
// 3.1.1
// option_defaults
//   class: CLI11's OptionDefaults*, as a handle on a command's
// d_arg_option_defaults: what the command's new options start from. Settings
// return the handle; a NULL handle reads as the defaults of a command made
// bare -- group "OPTIONS", configurable, the rest off -- and ignores every
// setting. Its group is copied; a copy that could not be made leaves the
// group as it was.
class option_defaults
{
public:
    option_defaults() D_NOEXCEPT;
    explicit option_defaults(::d_arg_option_defaults* _defaults) D_NOEXCEPT;

    // the handle
    option_defaults*         operator->() D_NOEXCEPT;
    const option_defaults*   operator->() const D_NOEXCEPT;
    option_defaults&         operator*() D_NOEXCEPT;
    const option_defaults&   operator*() const D_NOEXCEPT;
    ::d_arg_option_defaults* get() const D_NOEXCEPT;
    bool                     valid() const D_NOEXCEPT;

    // settings: CLI11's OptionDefaults
    option_defaults group(c_string _group);
    option_defaults required(bool _value = true) D_NOEXCEPT;
    option_defaults mandatory(bool _value = true) D_NOEXCEPT;
    option_defaults always_capture_default(bool _value = true) D_NOEXCEPT;
    option_defaults configurable(bool _value = true) D_NOEXCEPT;
    option_defaults disable_flag_override(bool _value = true) D_NOEXCEPT;
    option_defaults ignore_case(bool _value = true) D_NOEXCEPT;
    option_defaults ignore_underscore(bool _value = true) D_NOEXCEPT;
    option_defaults delimiter(char _delimiter = '\0') D_NOEXCEPT;
    option_defaults multi_option_policy(
        multi_option_policy::value _policy =
            multi_option_policy::throw_error) D_NOEXCEPT;
    option_defaults take_last() D_NOEXCEPT;
    option_defaults take_first() D_NOEXCEPT;
    option_defaults take_all() D_NOEXCEPT;
    option_defaults join() D_NOEXCEPT;
    option_defaults join(char _delimiter) D_NOEXCEPT;
    option_defaults callback_priority(
        callback_priority::value _priority =
            callback_priority::normal) D_NOEXCEPT;
    option_defaults integer_rule(integer_rule::value _rule) D_NOEXCEPT;

    // readers
    const char*                get_group() const D_NOEXCEPT;
    bool                       get_required() const D_NOEXCEPT;
    bool                       get_always_capture_default() const D_NOEXCEPT;
    bool                       get_configurable() const D_NOEXCEPT;
    bool                       get_disable_flag_override() const D_NOEXCEPT;
    bool                       get_ignore_case() const D_NOEXCEPT;
    bool                       get_ignore_underscore() const D_NOEXCEPT;
    char                       get_delimiter() const D_NOEXCEPT;
    multi_option_policy::value get_multi_option_policy() const D_NOEXCEPT;
    callback_priority::value   get_callback_priority() const D_NOEXCEPT;
    integer_rule::value        get_integer_rule() const D_NOEXCEPT;

private:
    option_defaults set_flag(re_std::uint32_t _flag,
                             bool             _value) D_NOEXCEPT;
    bool            has_flag(re_std::uint32_t _flag) const D_NOEXCEPT;

    ::d_arg_option_defaults* m_defaults;
};

// 3.2    The interface
//------------------------------------------------------------------------------
// 3.2.1
// command_interface
//   class: CLI11's App interface, over the command record of Base -- a
// d_arg_parser's root, or a command_handle's command -- for the class
// Derived that derives from it. A setting returns `chain_type`: Derived& for
// the parser, a handle by value for a command, so that calls chain with `.`
// and, as CLI11's do, with `->`. Readers read the record through the C
// calls, which read a NULL command as one with nothing set. It adds no
// member to Base. Every type of the face is named here with `cli::`, so that
// a field of Base -- a parser's `command`, `error` and `formatter` -- never
// stands for it.
template<typename Derived,
         typename Base>
class command_interface : public Base
{
public:
    typedef typename internal::command_result<Derived>::type chain_type;

    // making: a Base made bare, or from one given
    command_interface() D_NOEXCEPT;
    explicit command_interface(const Base& _base) D_NOEXCEPT;

    // the record: the d_arg_command* every C call takes
    ::d_arg_command* record() const D_NOEXCEPT;

    // options: CLI11's add_option(name) and add_option(name, description);
    // the option, or a NULL handle when the definition failed
    cli::option add_option(c_string _names,
                           c_string _description = c_string());

    /**
     * @brief Adds an option bound to a variable; CLI11's add_option(name,
     *        variable, description).
     *
     * @note A const second argument is a description, as in CLI11, so that
     *       a string literal or a const std::string describes. The variable
     *       converts through type_traits, takes the option's values, and
     *       gives the option its type name, sizes and default.
     *
     * @param[in] _names        the names, CLI11's way.
     * @param[in] _variable     the variable, which must outlive the option's
     *                          use.
     * @param[in] _description  the help text.
     * @return the option; a NULL handle when the definition failed.
     */
    template<typename Type>
    typename internal::enable_if<(!internal::is_const<Type>::value),
                                 cli::option>::type
        add_option(c_string _names,
                   Type&    _variable,
                   c_string _description = c_string());

    /**
     * @brief Adds an option bound to a variable whose default is never
     *        written; CLI11's add_option_no_stream(name, variable,
     *        description).
     *
     * @note As add_option(name, variable, description), but the option's
     *       default function gives the empty string, as CLI11's does: a
     *       default captured -- by capture_default_str(), or because the
     *       command's options always capture theirs -- is empty.
     *
     * @param[in] _names        the names, CLI11's way.
     * @param[in] _variable     the variable, which must outlive the option's
     *                          use.
     * @param[in] _description  the help text.
     * @return the option; a NULL handle when the definition failed.
     */
    template<typename Type>
    typename internal::enable_if<(!internal::is_const<Type>::value),
                                 cli::option>::type
        add_option_no_stream(c_string _names,
                             Type&    _variable,
                             c_string _description = c_string());

    /**
     * @brief Adds an option whose values, converted to a type, are given to
     *        a function; CLI11's add_option_function<T>(name, function,
     *        description).
     *
     * @note The function takes a `const Type&` and returns nothing or a
     *       cli::status. Values that do not convert end the parse as
     *       CLI11's "Could not convert"; the function is then not called.
     *       The option takes the type's name, sizes and expected count, as
     *       CLI11's does; like CLI11's, it does not run its callback for a
     *       default unless run_callback_for_default() is set.
     *
     * @param[in] _names        the names.
     * @param[in] _function     the function object; copied, the option owning
     *                          the copy.
     * @param[in] _description  the help text.
     * @return the option; a NULL handle when the definition failed.
     */
    template<typename Type,
             typename Function>
    cli::option add_option_function(c_string _names,
                                    Function _function,
                                    c_string _description = c_string());

    // flags: CLI11's add_flag(name) and add_flag(name, description)
    cli::option add_flag(c_string _names,
                         c_string _description = c_string());

    /**
     * @brief Adds a flag bound to a variable; CLI11's add_flag(name,
     *        variable, description).
     *
     * @note A const second argument is a description, as for add_option. A
     *       `bool` takes the flag's truth, an integer the count of its uses
     *       -- its values summed -- and a container one value per use.
     *
     * @param[in] _names        the names, with `{value}` and `!`.
     * @param[in] _variable     the variable, which must outlive the flag's
     *                          use.
     * @param[in] _description  the help text.
     * @return the flag; a NULL handle when the definition failed.
     */
    template<typename Type>
    typename internal::enable_if<(!internal::is_const<Type>::value),
                                 cli::option>::type
        add_flag(c_string _names,
                 Type&    _variable,
                 c_string _description = c_string());

    /**
     * @brief Adds a flag that calls a function when it is true; CLI11's
     *        add_flag_callback.
     *
     * @param[in] _names        the names.
     * @param[in] _function     a function object taking nothing and
     *                          returning nothing or a cli::status; copied,
     *                          the flag owning the copy.
     * @param[in] _description  the help text.
     * @return the flag; a NULL handle when the definition failed.
     */
    template<typename Function>
    cli::option add_flag_callback(c_string _names,
                                  Function _function,
                                  c_string _description = c_string());

    /**
     * @brief Adds a flag that calls a function with the sum of its values;
     *        CLI11's add_flag_function.
     *
     * @note The function takes the count as an `re_std::intmax_t` -- CLI11's
     *       std::int64_t -- where the platform has one, else as a `long`,
     *       and returns nothing or a cli::status.
     *
     * @param[in] _names        the names.
     * @param[in] _function     the function object; copied, the flag owning
     *                          the copy.
     * @param[in] _description  the help text.
     * @return the flag; a NULL handle when the definition failed.
     */
    template<typename Function>
    cli::option add_flag_function(c_string _names,
                                  Function _function,
                                  c_string _description = c_string());

    // special options: CLI11's set_help_flag, set_help_all_flag,
    // set_version_flag and set_config, each replacing the one before; no
    // names removes it, and returns a NULL handle, as a failure does
    cli::option set_help_flag(c_string _names       = c_string(),
                              c_string _description = c_string());
    cli::option set_help_all_flag(c_string _names       = c_string(),
                                  c_string _description = c_string());
    cli::option set_version_flag(c_string _names       = c_string(),
                                 c_string _version     = c_string(),
                                 c_string _description = c_string());

    /**
     * @brief Replaces the version flag with one whose text a function
     *        gives; CLI11's set_version_flag(name, function, description).
     *
     * @param[in] _names        the names; none removes the flag.
     * @param[in] _function     a function object taking nothing and
     *                          returning the version as a std::string, a C
     *                          string or a text; copied, the flag owning the
     *                          copy.
     * @param[in] _description  the help text.
     * @return the flag; a NULL handle when removed or refused.
     */
    template<typename Function>
    typename internal::unless_string<Function, cli::option>::type
        set_version_flag(c_string _names,
                         Function _function,
                         c_string _description = c_string());

    cli::option set_config(c_string _names        = c_string(),
                           c_string _default_file = c_string(),
                           c_string _description  = c_string(),
                           bool     _required     = false);
    bool        remove_option(const cli::option& _option);

    // subcommands and groups: CLI11's add_subcommand, add_option_group,
    // remove_subcommand; a NULL handle when the definition failed
    cli::command      add_subcommand(c_string _name        = c_string(),
                                     c_string _description = c_string());
    cli::option_group add_option_group(c_string _name,
                                       c_string _description = c_string());
    bool              remove_subcommand(const cli::command& _subcommand);

    // finding: CLI11's get_subcommand, get_option_group, get_option and
    // operator[]; a NULL handle where CLI11 throws OptionNotFound, so that
    // the _no_throw forms are the same; and get_subcommand_ptr, which gives
    // the handle get_subcommand gives, where CLI11's gives a std::shared_ptr
    cli::command      get_subcommand(c_string _name) const D_NOEXCEPT;
    cli::command      get_subcommand(int _index = 0) const D_NOEXCEPT;
    cli::command      get_subcommand(const cli::command& _subcommand) const
                          D_NOEXCEPT;
    cli::command      get_subcommand_no_throw(c_string _name) const
                          D_NOEXCEPT;
    cli::command      get_subcommand_ptr(c_string _name) const D_NOEXCEPT;
    cli::command      get_subcommand_ptr(int _index = 0) const D_NOEXCEPT;
    cli::command      get_subcommand_ptr(const cli::command& _subcommand)
                          const D_NOEXCEPT;
    cli::option_group get_option_group(c_string _name) const D_NOEXCEPT;
    cli::option       get_option(c_string _name) const D_NOEXCEPT;
    cli::option       get_option_no_throw(c_string _name) const D_NOEXCEPT;
    cli::option       operator[](c_string _name) const D_NOEXCEPT;
    cli::option       get_help_ptr() const D_NOEXCEPT;
    cli::option       get_help_all_ptr() const D_NOEXCEPT;
    cli::option       get_config_ptr() const D_NOEXCEPT;
    cli::option       get_version_ptr() const D_NOEXCEPT;
    cli::command      get_parent() const D_NOEXCEPT;

    // names and text: CLI11's name, alias, clear_aliases, description,
    // group, usage and footer, each copying its text
    chain_type name(c_string _name);
    chain_type alias(c_string _alias);
    chain_type clear_aliases() D_NOEXCEPT;
    chain_type description(c_string _description);
    chain_type group(c_string _group);
    chain_type usage(c_string _usage);
    chain_type footer(c_string _footer);

    /**
     * @brief Sets a function whose text leads the usage line; CLI11's
     *        usage(function).
     *
     * @param[in] _function  a function object taking nothing and returning
     *                       the text as a std::string, a C string or a text;
     *                       copied, the command owning the copy.
     * @return what the command chains on.
     */
    template<typename Function>
    typename internal::unless_string<Function, chain_type>::type
        usage(Function _function);

    /**
     * @brief Sets a function whose text leads the footer; CLI11's
     *        footer(function).
     *
     * @param[in] _function  as for usage(function).
     * @return what the command chains on.
     */
    template<typename Function>
    typename internal::unless_string<Function, chain_type>::type
        footer(Function _function);

    // modes: CLI11's allow_extras, allow_config_extras and prefix_command,
    // given a switch or a mode
    chain_type allow_extras(bool _allow = true) D_NOEXCEPT;
    chain_type allow_extras(extras_mode::value _mode) D_NOEXCEPT;
    chain_type allow_config_extras(bool _allow = true) D_NOEXCEPT;
    chain_type allow_config_extras(config_extras_mode::value _mode)
                   D_NOEXCEPT;
    chain_type prefix_command(bool _prefix = true) D_NOEXCEPT;
    chain_type prefix_command(prefix_mode::value _mode) D_NOEXCEPT;

    // switches: CLI11's; ignore_case and ignore_underscore refuse, as the
    // definition error, a setting that makes a name another subcommand's
    chain_type required(bool _value = true) D_NOEXCEPT;
    chain_type disabled(bool _value = true) D_NOEXCEPT;
    chain_type silent(bool _value = true) D_NOEXCEPT;
    chain_type allow_windows_style_options(bool _value = true) D_NOEXCEPT;
    chain_type positionals_at_end(bool _value = true) D_NOEXCEPT;
    chain_type configurable(bool _value = true) D_NOEXCEPT;
    chain_type validate_positionals(bool _value = true) D_NOEXCEPT;
    chain_type validate_optional_arguments(bool _value = true) D_NOEXCEPT;
    chain_type allow_non_standard_option_names(bool _value = true)
                   D_NOEXCEPT;
    chain_type allow_subcommand_prefix_matching(bool _value = true)
                   D_NOEXCEPT;
    chain_type fallthrough(bool _value = true) D_NOEXCEPT;
    chain_type subcommand_fallthrough(bool _value = true) D_NOEXCEPT;
    chain_type ignore_case(bool _value = true) D_NOEXCEPT;
    chain_type ignore_underscore(bool _value = true) D_NOEXCEPT;
    chain_type immediate_callback(bool _value = true) D_NOEXCEPT;
    chain_type disabled_by_default(bool _value = true) D_NOEXCEPT;
    chain_type enabled_by_default(bool _value = true) D_NOEXCEPT;

    // requirements: CLI11's require_subcommand and require_option: one or
    // more; `n` exactly, `-n` at most; or a range, a maximum of 0 unlimited
    chain_type require_subcommand() D_NOEXCEPT;
    chain_type require_subcommand(int _value) D_NOEXCEPT;
    chain_type require_subcommand(std::size_t _minimum,
                                  std::size_t _maximum) D_NOEXCEPT;
    chain_type require_option() D_NOEXCEPT;
    chain_type require_option(int _value) D_NOEXCEPT;
    chain_type require_option(std::size_t _minimum,
                              std::size_t _maximum) D_NOEXCEPT;

    /**
     * @brief Sets the function run when the command's parse completes;
     *        CLI11's callback.
     *
     * @note As in CLI11, the function runs after the whole command line,
     *       or, with immediate_callback, as soon as the command is parsed.
     *       callback, final_callback, parse_complete_callback and
     *       pre_callback each take a function object taking nothing and
     *       returning nothing or a cli::status, which a status not ok() ends
     *       the parse with; pre_callback's is what CLI11's virtual
     *       pre_callback() gives a subclass.
     *
     * @param[in] _function  the function object; copied, the command owning
     *                       the copy.
     * @return what the command chains on.
     */
    template<typename Function>
    chain_type callback(Function _function);
    template<typename Function>
    chain_type final_callback(Function _function);
    template<typename Function>
    chain_type parse_complete_callback(Function _function);
    template<typename Function>
    chain_type pre_callback(Function _function);

    /**
     * @brief Sets the function run when the command is first met in a
     *        parse; CLI11's preparse_callback.
     *
     * @param[in] _function  a function object taking the number of
     *                       arguments left as a std::size_t and returning
     *                       nothing or a cli::status; copied, the command
     *                       owning the copy.
     * @return what the command chains on.
     */
    template<typename Function>
    chain_type preparse_callback(Function _function);

    /**
     * @brief Sets what is written for a failed parse; CLI11's
     *        failure_message.
     *
     * @note failure_message::simple and failure_message::help are CLI11's
     *       two; a function object takes the command -- `const
     *       cli::command&` -- and the error -- `const cli::error&` -- and
     *       returns the message as a std::string, a C string or a text.
     *       One that throws is followed by CLI11's simple message, so that
     *       no failed parse goes unreported, where CLI11's exit() lets the
     *       exception out.
     *
     * @param[in] _function  the function object; copied, the command owning
     *                       the copy.
     * @return what the command chains on.
     */
    template<typename Function>
    chain_type failure_message(Function _function);
    chain_type failure_message(::d_arg_failure_fn _write) D_NOEXCEPT;

    // relations: CLI11's excludes and needs, with an option or a command,
    // and their removal, which answers whether there was one
    chain_type excludes(const cli::option& _option);
    chain_type excludes(const cli::command& _other);
    chain_type needs(const cli::option& _option);
    chain_type needs(const cli::command& _other);
    bool       remove_excludes(const cli::option& _option) D_NOEXCEPT;
    bool       remove_excludes(const cli::command& _other) D_NOEXCEPT;
    bool       remove_needs(const cli::option& _option) D_NOEXCEPT;
    bool       remove_needs(const cli::command& _other) D_NOEXCEPT;

    // formatting: CLI11's formatter and config_formatter, borrowed -- each
    // must outlive the command -- and the ones in effect, the command's or
    // its nearest ancestor's, else the parser's
    chain_type formatter(::d_arg_formatter& _formatter) D_NOEXCEPT;
    chain_type config_formatter(::d_arg_config_format& _format) D_NOEXCEPT;
    const ::d_arg_formatter*     get_formatter() const D_NOEXCEPT;
    const ::d_arg_config_format* get_config_formatter() const D_NOEXCEPT;

    // option defaults: CLI11's option_defaults(), a handle on the
    // command's
    cli::option_defaults option_defaults() D_NOEXCEPT;

    // switches and modes, read
    bool        get_required() const D_NOEXCEPT;
    bool        get_disabled() const D_NOEXCEPT;
    bool        get_silent() const D_NOEXCEPT;
    bool        get_allow_windows_style_options() const D_NOEXCEPT;
    bool        get_positionals_at_end() const D_NOEXCEPT;
    bool        get_configurable() const D_NOEXCEPT;
    bool        get_validate_positionals() const D_NOEXCEPT;
    bool        get_validate_optional_arguments() const D_NOEXCEPT;
    bool        get_allow_non_standard_option_names() const D_NOEXCEPT;
    bool        get_allow_subcommand_prefix_matching() const D_NOEXCEPT;
    bool        get_fallthrough() const D_NOEXCEPT;
    bool        get_subcommand_fallthrough() const D_NOEXCEPT;
    bool        get_ignore_case() const D_NOEXCEPT;
    bool        get_ignore_underscore() const D_NOEXCEPT;
    bool        get_immediate_callback() const D_NOEXCEPT;
    bool        get_disabled_by_default() const D_NOEXCEPT;
    bool        get_enabled_by_default() const D_NOEXCEPT;
    bool        get_allow_extras() const D_NOEXCEPT;
    bool        get_prefix_command() const D_NOEXCEPT;
    std::size_t get_require_subcommand_min() const D_NOEXCEPT;
    std::size_t get_require_subcommand_max() const D_NOEXCEPT;
    std::size_t get_require_option_min() const D_NOEXCEPT;
    std::size_t get_require_option_max() const D_NOEXCEPT;
    extras_mode::value        get_allow_extras_mode() const D_NOEXCEPT;
    config_extras_mode::value get_allow_config_extras() const D_NOEXCEPT;
    prefix_mode::value        get_prefix_command_mode() const D_NOEXCEPT;

    // names and text, read: never NULL, "" when unset ("SUBCOMMANDS" for an
    // unset group); the aliases valid until they change
    const char*      get_name() const D_NOEXCEPT;
    const char*      get_description() const D_NOEXCEPT;
    const char*      get_group() const D_NOEXCEPT;
    name_list        get_aliases() const D_NOEXCEPT;
    bool             check_name(const text& _name) const D_NOEXCEPT;
    name_match::value check_name_detail(const text& _name) const D_NOEXCEPT;

    // the parse, read: CLI11's parsed() and count(), the times the command
    // was used; count_all(); count(name), an option's values, 0 when there
    // is no such option; got_subcommand; the subcommands used, in order,
    // and the options given, in order, as views valid until the next parse
    bool                     parsed() const D_NOEXCEPT;
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_EXPLICIT_BOOL operator bool() const D_NOEXCEPT;
#endif
    std::size_t              count() const D_NOEXCEPT;
    std::size_t              count_all() const D_NOEXCEPT;
    std::size_t              count(c_string _option_name) const D_NOEXCEPT;
    bool                     got_subcommand(const cli::command& _subcommand)
                                 const D_NOEXCEPT;
    bool                     got_subcommand(c_string _name) const D_NOEXCEPT;
    basic_handle_list<cli::command, ::d_arg_command>
                             get_subcommands() const D_NOEXCEPT;
    option_list              parse_order() const D_NOEXCEPT;
    std::size_t              remaining_size(bool _recurse = false) const
                                 D_NOEXCEPT;

    // lists made: CLI11's get_options() -- the command's own options, then
    // its option groups' -- get_subcommands(filter), over all the
    // subcommands, get_groups(), remaining() and
    // remaining_for_passthrough(); a filter takes the handle and returns
    // whether to keep it, and what it throws propagates
    std::vector<cli::option>  get_options() const;
    template<typename Filter>
    std::vector<cli::option>  get_options(Filter _filter) const;
    template<typename Filter>
    std::vector<cli::command> get_subcommands(Filter _filter) const;
    std::vector<std::string>  get_groups() const;
    std::vector<std::string>  remaining(bool _recurse = false) const;
    std::vector<std::string>  remaining_for_passthrough(
                                  bool _recurse = false) const;

    // text made: CLI11's get_display_name, get_usage, get_footer, version and
    // help, empty when a function writing part of it fails; help into a
    // sink, with its status, without building a std::string
    std::string get_display_name(bool _with_aliases = false) const;
    std::string get_usage() const;
    std::string get_footer() const;
    std::string version() const;
    std::string help(c_string        _prefix = c_string(),
                     help_mode::value _mode  = help_mode::normal) const;
    cli::status help(::d_pack_sink    _sink,
                     c_string         _prefix = c_string(),
                     help_mode::value _mode   = help_mode::normal) const;
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    // configuration made: CLI11's config_to_str, the defaults also, or in
    // the mode given
    std::string config_to_str(bool _default_also      = false,
                              bool _write_description = false) const;
    std::string config_to_str(config_output_mode::value _mode,
                              bool _write_description = false) const;
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

    // the parse, forgotten early: CLI11's clear()
    void clear() D_NOEXCEPT;

    // failing from a callback: records why the command's callback ends the
    // parse, for the callback to return; the status's own text when no
    // message is given. fail_runtime is CLI11's RuntimeError(exit_code):
    // the parse ends, nothing is written, and the process ends with the code.
    cli::status fail(const cli::status& _status,
                     c_string           _message = c_string()) const;
    cli::status fail_runtime(int _exit_code = 1) const;
};

// 3.3    Handles
//------------------------------------------------------------------------------
// 3.3.1
// command
//   class: CLI11's App*, for a subcommand -- or any command, the root
// included, since a parser converts to the handle on its root -- with
// CLI11's App interface (command_interface). Its `explicit operator bool`,
// from C++11, is CLI11's App's: the command was parsed -- `if (*sub)` reads
// as CLI11's does -- beside parsed(); valid() is whether the handle refers
// to a command.
class command
    : public command_interface<command, internal::command_handle>
{
public:
    command() D_NOEXCEPT;
    explicit command(::d_arg_command* _command) D_NOEXCEPT;
    command(::d_arg_parser& _parser) D_NOEXCEPT;

    // the handle: CLI11's ports read `sub->x()` and `*sub`
    command*         operator->() D_NOEXCEPT;
    const command*   operator->() const D_NOEXCEPT;
    command&         operator*() D_NOEXCEPT;
    const command&   operator*() const D_NOEXCEPT;
    ::d_arg_command* get() const D_NOEXCEPT;
    bool             valid() const D_NOEXCEPT;
};

// 3.3.2
// command_list
//   type: a view of commands a record refers to -- the subcommands a parse
// used -- whose elements are handles.
typedef basic_handle_list<command, ::d_arg_command> command_list;

// 3.3.3
// option_group
//   class: CLI11's Option_group*: a command handle on an option group, with
// CLI11's Option_group members beside App's: options and subcommands moved
// into the group from its parent. App's settings return a command handle,
// as CLI11's return App*.
class option_group : public command
{
public:
    option_group() D_NOEXCEPT;
    explicit option_group(::d_arg_command* _group) D_NOEXCEPT;

    // the handle
    option_group*       operator->() D_NOEXCEPT;
    const option_group* operator->() const D_NOEXCEPT;
    option_group&       operator*() D_NOEXCEPT;
    const option_group& operator*() const D_NOEXCEPT;

    // moving into the group: CLI11's Option_group::add_option(Option*),
    // add_options and add_subcommand(App*); the option or subcommand, or a
    // NULL handle when the move was refused, which is the definition error
    using command::add_option;
    cli::option  add_option(const cli::option& _option);
    void         add_options(const cli::option& _option);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Options>
    void         add_options(const cli::option&    _first,
                             const cli::option&    _second,
                             const Options&...     _rest);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
    using command::add_subcommand;
    cli::command add_subcommand(const cli::command& _subcommand);
};

// 3.4    Comparing handles
//------------------------------------------------------------------------------
// 3.4.1
// operator== / operator!=
//   functions: two command handles are equal when they refer to the same
// command, as two of CLI11's App* are.
bool operator==(const command& _first,
                const command& _second) D_NOEXCEPT;
bool operator!=(const command& _first,
                const command& _second) D_NOEXCEPT;

// 3.5    Layout
//------------------------------------------------------------------------------
// 3.5.1
// The handles are one pointer
//   assertions: a command handle and an option group's cost what CLI11's App*
// costs.
D_STATIC_ASSERT( (sizeof(command) == sizeof(::d_arg_command*)),
                 "cli::command is one pointer" );
D_STATIC_ASSERT( (sizeof(option_group) == sizeof(::d_arg_command*)),
                 "cli::option_group is one pointer" );


//==============================================================================
// 4.  UTILITIES
//==============================================================================


// 4.1    Triggers
//------------------------------------------------------------------------------
// 4.1.1
// trigger_on / trigger_off
//   functions: CLI11's TriggerOn and TriggerOff: a command that, when met,
// enables -- or disables -- one command or several, which start each parse
// the other way. A failure is the definition error; the status is
// d_arg_command_trigger_on's.
cli::status trigger_on(const command& _trigger,
                       const command& _target);
cli::status trigger_on(const command&             _trigger,
                       const std::vector<command>& _targets);
cli::status trigger_off(const command& _trigger,
                        const command& _target);
cli::status trigger_off(const command&             _trigger,
                        const std::vector<command>& _targets);

// 4.2    Life cycle of options
//------------------------------------------------------------------------------
// 4.2.1
// deprecate_option
//   functions: CLI11's deprecate_option, for an option or for one a command
// finds by name; using the option warns, naming the replacement when there
// is one. A failure is the definition error.
cli::status deprecate_option(const option& _option,
                             c_string      _replacement = c_string());
cli::status deprecate_option(const command& _command,
                             c_string       _name,
                             c_string       _replacement = c_string());

// 4.2.2
// retire_option
//   functions: CLI11's retire_option: the option, or the one a command finds
// by name -- or a new one taking at most one value when there is none --
// replaced with one that takes the same values, warns, and does nothing. The
// retired option, or a NULL handle when the definition failed.
option retire_option(const command& _command,
                     const option&  _option);
option retire_option(const command& _command,
                     c_string       _name);


//==============================================================================
// 5.  INTERNALS
//==============================================================================


// 5.1    Command callbacks
//------------------------------------------------------------------------------
NS_INTERNAL

    // 5.1.1
    // command_failure
    //   class: the handler of guard for a command's callback: memory that
    //   ran out is status_code::out_of_memory; any other exception ends the
    //   parse with the status given, what() its message.
    class command_failure
    {
    public:
        typedef int result_type;

        command_failure(const ::d_arg_command* _command,
                        int                    _status) D_NOEXCEPT;

        int operator()(const std::exception* _exception,
                       bool                  _out_of_memory) const;

    private:
        const ::d_arg_command* m_command;
        int                    m_status;
    };

    // 5.1.2
    // command_call / command_function
    //   class, function: a command callback's work, as d_arg_command_fn: the
    //   function called with nothing.
    template<typename Function>
    class command_call
    {
    public:
        typedef int result_type;

        explicit command_call(void* _context) D_NOEXCEPT;

        int operator()() const;

    private:
        Function* m_function;
    };
    template<typename Function>
    int command_function(::d_arg_command* _command,
                         void*            _context);

    // 5.1.3
    // preparse_call / preparse_function
    //   class, function: a pre-parse callback's work, as d_arg_preparse_fn:
    //   the function called with the arguments left.
    template<typename Function>
    class preparse_call
    {
    public:
        typedef int result_type;

        preparse_call(std::size_t _remaining,
                      void*       _context) D_NOEXCEPT;

        int operator()() const;

    private:
        std::size_t m_remaining;
        Function*   m_function;
    };
    template<typename Function>
    int preparse_function(::d_arg_command* _command,
                          std::size_t      _remaining,
                          void*            _context);

    // 5.1.4
    // command_text_function
    //   function: a function writing a command's text -- its usage, footer or
    //   version -- as d_arg_command_text_fn: text_call's work.
    template<typename Function>
    int command_text_function(const ::d_arg_command* _command,
                              ::d_pack_sink          _sink,
                              void*                  _context);

    // 5.1.5
    // failure_call / failure_function
    //   class, function: a failure message's work, as d_arg_failure_fn: the
    //   function called with the command and the error, its text written.
    template<typename Function>
    class failure_call
    {
    public:
        typedef int result_type;

        failure_call(const ::d_arg_command* _command,
                     const ::d_arg_error*   _error,
                     ::d_pack_sink          _sink,
                     void*                  _context) D_NOEXCEPT;

        int operator()() const;

        template<typename Result>
        int write(const Result& _result) const;

    private:
        const ::d_arg_command* m_command;
        const ::d_arg_error*   m_error;
        ::d_pack_sink          m_sink;
        Function*              m_function;
    };
    template<typename Function>
    int failure_function(const ::d_arg_command* _command,
                         const ::d_arg_error*   _error,
                         ::d_pack_sink          _sink,
                         void*                  _context);

NS_END  // internal

// 5.2    Option callbacks
//------------------------------------------------------------------------------
NS_INTERNAL

    // 5.2.1
    // flag_call / flag_function
    //   class, function: add_flag_callback's work, as d_arg_flag_fn: the
    //   function called with nothing when the flag is true.
    template<typename Function>
    class flag_call
    {
    public:
        typedef int result_type;

        explicit flag_call(void* _context) D_NOEXCEPT;

        int operator()() const;

    private:
        Function* m_function;
    };
    template<typename Function>
    int flag_function(::d_arg_option* _option,
                      long            _count,
                      void*           _context);

    // 5.2.2
    // flag_count / count_call / count_function
    //   type, class, function: add_flag_function's count -- intmax_t where
    //   the platform has it, as CLI11's std::int64_t, else long -- and its
    //   work, as d_arg_flag_intmax_fn or d_arg_flag_fn: the function called
    //   with the count.
#if defined(INTMAX_MAX)
    typedef re_std::intmax_t flag_count;
#else
    typedef long             flag_count;
#endif  // INTMAX_MAX

    template<typename Function>
    class count_call
    {
    public:
        typedef int result_type;

        count_call(flag_count _count,
                   void*      _context) D_NOEXCEPT;

        int operator()() const;

    private:
        flag_count m_count;
        Function*  m_function;
    };
    template<typename Function>
    int count_function(::d_arg_option* _option,
                       flag_count      _count,
                       void*           _context);

    // 5.2.3
    // typed_call / typed_function
    //   class, function: add_option_function's work, as d_arg_results_fn:
    //   the values converted to Type, then the function called with the
    //   value; values that do not convert end the parse as CLI11's "Could
    //   not convert".
    template<typename Type,
             typename Function>
    class typed_call
    {
    public:
        typedef int result_type;

        typed_call(const ::d_arg_option* _option,
                   const ::d_pack_text*  _results,
                   std::size_t           _count,
                   void*                 _context) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_arg_option* m_option;
        const ::d_pack_text*  m_results;
        std::size_t           m_count;
        Function*             m_function;
    };
    template<typename Type,
             typename Function>
    int typed_function(::d_arg_option*      _option,
                       const ::d_pack_text* _results,
                       std::size_t          _count,
                       void*                _context);

    // 5.2.4
    // no_default_text
    //   class: the default function of an option added without a stream,
    //   as CLI11's add_option_no_stream gives it: the empty string.
    struct no_default_text
    {
        const char* operator()() const D_NOEXCEPT;
    };

NS_END  // internal

// 5.3    Collecting
//------------------------------------------------------------------------------
NS_INTERNAL

    // 5.3.1
    // raise_out_of_memory
    //   function: answers the status of building a list or a std::string
    //   through the C library -- out of memory, or a std::string sink that
    //   refused bytes, which only memory makes it do -- with std::bad_alloc,
    //   as the standard containers do; with C++ exceptions off, nothing, the
    //   standard containers having then ended the program already.
    void raise_out_of_memory(int _status);

    // 5.3.2
    // remaining_collect / remaining_visit
    //   class, function: an unused argument, appended to a vector of
    //   strings, as d_arg_extra_visit_fn; memory that runs out stops the
    //   visit with status_code::out_of_memory.
    class remaining_collect
    {
    public:
        typedef int result_type;

        remaining_collect(const ::d_arg_extra* _extra,
                          void*                _context) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_arg_extra*      m_extra;
        std::vector<std::string>* m_list;
    };
    int remaining_visit(const ::d_arg_extra* _extra,
                        void*                _context);

    // 5.3.3
    // option_collect / option_visit
    //   class, function: an option, appended to a vector of handles, as
    //   d_arg_option_visit_fn.
    class option_collect
    {
    public:
        typedef int result_type;

        option_collect(::d_arg_option* _option,
                       void*           _context) D_NOEXCEPT;

        int operator()() const;

    private:
        ::d_arg_option*           m_option;
        std::vector<cli::option>* m_list;
    };
    int option_visit(::d_arg_option* _option,
                     void*           _context);

    // 5.3.4
    // group_collect / group_visit
    //   class, function: a group name, appended to a vector of strings, as
    //   d_arg_group_visit_fn.
    class group_collect
    {
    public:
        typedef int result_type;

        group_collect(::d_pack_text _group,
                      void*         _context) D_NOEXCEPT;

        int operator()() const;

    private:
        ::d_pack_text             m_group;
        std::vector<std::string>* m_list;
    };
    int group_visit(::d_pack_text _group,
                    void*         _context);

NS_END  // internal


//==============================================================================
// 6.  DEFINITIONS
//==============================================================================


// 6.1    option_defaults
//------------------------------------------------------------------------------
// option_defaults::option_defaults / operator-> / operator* / get / valid
//   The pointer; the handle itself, for CLI11's arrow and star.
inline
option_defaults::option_defaults() D_NOEXCEPT
    : m_defaults(D_NULLPTR)
{}

inline
option_defaults::option_defaults(
    ::d_arg_option_defaults* _defaults
) D_NOEXCEPT
    : m_defaults(_defaults)
{}

inline option_defaults*
option_defaults::operator->() D_NOEXCEPT
{
    return this;
}

inline const option_defaults*
option_defaults::operator->() const D_NOEXCEPT
{
    return this;
}

inline option_defaults&
option_defaults::operator*() D_NOEXCEPT
{
    return *this;
}

inline const option_defaults&
option_defaults::operator*() const D_NOEXCEPT
{
    return *this;
}

inline ::d_arg_option_defaults*
option_defaults::get() const D_NOEXCEPT
{
    return m_defaults;
}

inline bool
option_defaults::valid() const D_NOEXCEPT
{
    return (m_defaults != D_NULLPTR);
}

// option_defaults::group and kin
//   The C settings, which ignore NULL defaults; the switches through
// set_flag.
inline option_defaults
option_defaults::group(
    c_string _group
)
{
    (void)d_arg_option_defaults_set_group(m_defaults,
                                          _group.get());

    return *this;
}

inline option_defaults
option_defaults::required(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_OPTION_REQUIRED,
                    _value);
}

inline option_defaults
option_defaults::mandatory(
    bool _value
) D_NOEXCEPT
{
    return required(_value);
}

inline option_defaults
option_defaults::always_capture_default(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT,
                    _value);
}

inline option_defaults
option_defaults::configurable(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_OPTION_CONFIGURABLE,
                    _value);
}

inline option_defaults
option_defaults::disable_flag_override(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_OPTION_DISABLE_FLAG_OVERRIDE,
                    _value);
}

inline option_defaults
option_defaults::ignore_case(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_OPTION_IGNORE_CASE,
                    _value);
}

inline option_defaults
option_defaults::ignore_underscore(
    bool _value
) D_NOEXCEPT
{
    return set_flag(D_ARG_OPTION_IGNORE_UNDERSCORE,
                    _value);
}

inline option_defaults
option_defaults::delimiter(
    char _delimiter
) D_NOEXCEPT
{
    d_arg_option_defaults_set_delimiter(m_defaults,
                                        _delimiter);

    return *this;
}

inline option_defaults
option_defaults::multi_option_policy(
    multi_option_policy::value _policy
) D_NOEXCEPT
{
    d_arg_option_defaults_set_multi_option_policy(m_defaults,
                                                  static_cast<int>(_policy));

    return *this;
}

inline option_defaults
option_defaults::take_last() D_NOEXCEPT
{
    return multi_option_policy(multi_option_policy::take_last);
}

inline option_defaults
option_defaults::take_first() D_NOEXCEPT
{
    return multi_option_policy(multi_option_policy::take_first);
}

inline option_defaults
option_defaults::take_all() D_NOEXCEPT
{
    return multi_option_policy(multi_option_policy::take_all);
}

inline option_defaults
option_defaults::join() D_NOEXCEPT
{
    return multi_option_policy(multi_option_policy::join);
}

inline option_defaults
option_defaults::join(
    char _delimiter
) D_NOEXCEPT
{
    d_arg_option_defaults_set_delimiter(m_defaults,
                                        _delimiter);

    return multi_option_policy(multi_option_policy::join);
}

inline option_defaults
option_defaults::callback_priority(
    callback_priority::value _priority
) D_NOEXCEPT
{
    d_arg_option_defaults_set_callback_priority(m_defaults,
                                                static_cast<int>(_priority));

    return *this;
}

inline option_defaults
option_defaults::integer_rule(
    integer_rule::value _rule
) D_NOEXCEPT
{
    d_arg_option_defaults_set_integer_rule(m_defaults,
                                           static_cast<int>(_rule));

    return *this;
}

// option_defaults::get_group and kin
//   The record's fields, a NULL handle reading as the defaults a command
// starts with.
inline const char*
option_defaults::get_group() const D_NOEXCEPT
{
    return ( (m_defaults != D_NULLPTR) &&
             (m_defaults->group != D_NULLPTR) )
           ? m_defaults->group
           : "OPTIONS";
}

inline bool
option_defaults::get_required() const D_NOEXCEPT
{
    return has_flag(D_ARG_OPTION_REQUIRED);
}

inline bool
option_defaults::get_always_capture_default() const D_NOEXCEPT
{
    return has_flag(D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT);
}

inline bool
option_defaults::get_configurable() const D_NOEXCEPT
{
    return (m_defaults != D_NULLPTR)
           ? has_flag(D_ARG_OPTION_CONFIGURABLE)
           : true;
}

inline bool
option_defaults::get_disable_flag_override() const D_NOEXCEPT
{
    return has_flag(D_ARG_OPTION_DISABLE_FLAG_OVERRIDE);
}

inline bool
option_defaults::get_ignore_case() const D_NOEXCEPT
{
    return has_flag(D_ARG_OPTION_IGNORE_CASE);
}

inline bool
option_defaults::get_ignore_underscore() const D_NOEXCEPT
{
    return has_flag(D_ARG_OPTION_IGNORE_UNDERSCORE);
}

inline char
option_defaults::get_delimiter() const D_NOEXCEPT
{
    return (m_defaults != D_NULLPTR) ? m_defaults->delimiter : '\0';
}

inline multi_option_policy::value
option_defaults::get_multi_option_policy() const D_NOEXCEPT
{
    return (m_defaults != D_NULLPTR)
           ? static_cast<multi_option_policy::value>(
                 m_defaults->multi_option_policy)
           : multi_option_policy::throw_error;
}

inline callback_priority::value
option_defaults::get_callback_priority() const D_NOEXCEPT
{
    return (m_defaults != D_NULLPTR)
           ? static_cast<callback_priority::value>(
                 m_defaults->callback_priority)
           : callback_priority::normal;
}

inline integer_rule::value
option_defaults::get_integer_rule() const D_NOEXCEPT
{
    return (m_defaults != D_NULLPTR)
           ? static_cast<integer_rule::value>(m_defaults->integer_rule)
           : static_cast<integer_rule::value>(
                 D_INTERNAL_ARG_PARSER_INTEGER_RULE);
}

// option_defaults::set_flag / has_flag
//   One D_ARG_OPTION_* switch, set through the C call or read from the
// record.
inline option_defaults
option_defaults::set_flag(
    re_std::uint32_t _flag,
    bool             _value
) D_NOEXCEPT
{
    d_arg_option_defaults_set_flag(m_defaults,
                                   _flag,
                                   _value);

    return *this;
}

inline bool
option_defaults::has_flag(
    re_std::uint32_t _flag
) const D_NOEXCEPT
{
    return ( (m_defaults != D_NULLPTR) &&
             ((m_defaults->flags & _flag) != 0u) );
}

// 6.2    command_interface
//------------------------------------------------------------------------------
// command_interface::command_interface / record
//   Base default-made -- a parser's plain data left for its own
// constructor to make, rather than zeroed first -- or copied; the record
// through record_of.
template<typename Derived,
         typename Base>
inline
command_interface<Derived, Base>::command_interface() D_NOEXCEPT
{}

template<typename Derived,
         typename Base>
inline
command_interface<Derived, Base>::command_interface(
    const Base& _base
) D_NOEXCEPT
    : Base(_base)
{}

template<typename Derived,
         typename Base>
inline ::d_arg_command*
command_interface<Derived, Base>::record() const D_NOEXCEPT
{
    return internal::record_of(static_cast<const Base&>(*this));
}

// command_interface::add_option and kin
//   The C definitions; a variable through its type's traits; a function's
// copy owned by the option through its release function, which frees it
// too when the definition fails.
template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::add_option(
    c_string _names,
    c_string _description
)
{
    return cli::option(d_arg_command_add_option(record(),
                                                _names.get(),
                                                _description.get()));
}

template<typename Derived,
         typename Base>
template<typename Type>
inline typename internal::enable_if<(!internal::is_const<Type>::value),
                                    cli::option>::type
command_interface<Derived, Base>::add_option(
    c_string _names,
    Type&    _variable,
    c_string _description
)
{
    return cli::option(d_arg_command_add_option_bound(
                           record(),
                           _names.get(),
                           type_traits<Type>::target(_variable),
                           type_traits<Type>::descriptor(),
                           _description.get()));
}

// command_interface::add_option_no_stream
//   add_option's, its default function then one giving nothing, and a
// default the command's options always capture captured again from it, as
// CLI11's add_option captures it once the function is set.
template<typename Derived,
         typename Base>
template<typename Type>
inline typename internal::enable_if<(!internal::is_const<Type>::value),
                                    cli::option>::type
command_interface<Derived, Base>::add_option_no_stream(
    c_string _names,
    Type&    _variable,
    c_string _description
)
{
    cli::option added = add_option(_names,
                                   _variable,
                                   _description);

    added.default_function(internal::no_default_text());

    // a default always captured is captured again, as nothing
    if (added.get_always_capture_default())
    {
        added.capture_default_str();
    }

    return added;
}

template<typename Derived,
         typename Base>
template<typename Type,
         typename Function>
inline cli::option
command_interface<Derived, Base>::add_option_function(
    c_string _names,
    Function _function,
    c_string _description
)
{
    const ::d_arg_type* const type  = type_traits<Type>::descriptor();
    ::d_arg_option* const     added = d_arg_command_add_option_callback(
                                          record(),
                                          _names.get(),
                                          &internal::typed_function<Type,
                                                                    Function>,
                                          new Function(_function),
                                          &internal::release_context<Function>,
                                          _description.get());

    // the type's shape, as CLI11 gives the option its type's
    if (added != D_NULLPTR)
    {
        (void)d_arg_option_set_type_name(added,
                                         type->name);
        d_arg_option_set_type_size_range(added,
                                         type->type_size_min,
                                         type->type_size_max);
        d_arg_option_set_expected(added,
                                  type->expected);
    }

    return cli::option(added);
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::add_flag(
    c_string _names,
    c_string _description
)
{
    return cli::option(d_arg_command_add_flag(record(),
                                              _names.get(),
                                              _description.get()));
}

template<typename Derived,
         typename Base>
template<typename Type>
inline typename internal::enable_if<(!internal::is_const<Type>::value),
                                    cli::option>::type
command_interface<Derived, Base>::add_flag(
    c_string _names,
    Type&    _variable,
    c_string _description
)
{
    return cli::option(d_arg_command_add_flag_bound(
                           record(),
                           _names.get(),
                           type_traits<Type>::target(_variable),
                           type_traits<Type>::descriptor(),
                           _description.get()));
}

template<typename Derived,
         typename Base>
template<typename Function>
inline cli::option
command_interface<Derived, Base>::add_flag_callback(
    c_string _names,
    Function _function,
    c_string _description
)
{
    return cli::option(d_arg_command_add_flag_callback(
                           record(),
                           _names.get(),
                           &internal::flag_function<Function>,
                           new Function(_function),
                           &internal::release_context<Function>,
                           _description.get()));
}

#if defined(INTMAX_MAX)

template<typename Derived,
         typename Base>
template<typename Function>
inline cli::option
command_interface<Derived, Base>::add_flag_function(
    c_string _names,
    Function _function,
    c_string _description
)
{
    return cli::option(d_arg_command_add_flag_function_intmax(
                           record(),
                           _names.get(),
                           &internal::count_function<Function>,
                           new Function(_function),
                           &internal::release_context<Function>,
                           _description.get()));
}

#else

template<typename Derived,
         typename Base>
template<typename Function>
inline cli::option
command_interface<Derived, Base>::add_flag_function(
    c_string _names,
    Function _function,
    c_string _description
)
{
    return cli::option(d_arg_command_add_flag_function(
                           record(),
                           _names.get(),
                           &internal::count_function<Function>,
                           new Function(_function),
                           &internal::release_context<Function>,
                           _description.get()));
}

#endif  // INTMAX_MAX

// command_interface::set_help_flag and kin / remove_option
//   The C replacements and the removal.
template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::set_help_flag(
    c_string _names,
    c_string _description
)
{
    return cli::option(d_arg_command_set_help_flag(record(),
                                                   _names.get(),
                                                   _description.get()));
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::set_help_all_flag(
    c_string _names,
    c_string _description
)
{
    return cli::option(d_arg_command_set_help_all_flag(record(),
                                                       _names.get(),
                                                       _description.get()));
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::set_version_flag(
    c_string _names,
    c_string _version,
    c_string _description
)
{
    return cli::option(d_arg_command_set_version_flag(record(),
                                                      _names.get(),
                                                      _version.get(),
                                                      _description.get()));
}

template<typename Derived,
         typename Base>
template<typename Function>
inline typename internal::unless_string<Function, cli::option>::type
command_interface<Derived, Base>::set_version_flag(
    c_string _names,
    Function _function,
    c_string _description
)
{
    return cli::option(d_arg_command_set_version_fn(
                           record(),
                           _names.get(),
                           &internal::command_text_function<Function>,
                           new Function(_function),
                           &internal::release_context<Function>,
                           _description.get()));
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::set_config(
    c_string _names,
    c_string _default_file,
    c_string _description,
    bool     _required
)
{
    return cli::option(d_arg_command_set_config(record(),
                                                _names.get(),
                                                _default_file.get(),
                                                _description.get(),
                                                _required));
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::remove_option(
    const cli::option& _option
)
{
    return d_arg_command_remove_option(record(),
                                       _option.get());
}

// command_interface::add_subcommand / add_option_group / remove_subcommand
//   The C definitions and the removal.
template<typename Derived,
         typename Base>
inline cli::command
command_interface<Derived, Base>::add_subcommand(
    c_string _name,
    c_string _description
)
{
    return cli::command(d_arg_command_add_subcommand(record(),
                                                     _name.get(),
                                                     _description.get()));
}

template<typename Derived,
         typename Base>
inline cli::option_group
command_interface<Derived, Base>::add_option_group(
    c_string _name,
    c_string _description
)
{
    return cli::option_group(d_arg_command_add_option_group(
                                 record(),
                                 _name.get(),
                                 _description.get()));
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::remove_subcommand(
    const cli::command& _subcommand
)
{
    return d_arg_command_remove_subcommand(record(),
                                           _subcommand.get());
}

// command_interface::get_subcommand and kin
//   The C lookups, as handles; a subcommand given is found among the
// command's own, as CLI11's get_subcommand(App*) finds it; a negative index
// finds none.
template<typename Derived,
         typename Base>
inline cli::command
command_interface<Derived, Base>::get_subcommand(
    c_string _name
) const D_NOEXCEPT
{
    return cli::command(d_arg_command_subcommand(record(),
                                                 _name.get()));
}

template<typename Derived,
         typename Base>
inline cli::command
command_interface<Derived, Base>::get_subcommand(
    int _index
) const D_NOEXCEPT
{
    return (_index >= 0)
           ? cli::command(d_arg_command_subcommand_at(
                              record(),
                              static_cast<std::size_t>(_index)))
           : cli::command();
}

template<typename Derived,
         typename Base>
inline cli::command
command_interface<Derived, Base>::get_subcommand(
    const cli::command& _subcommand
) const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    // the command's own subcommands, searched for the one given
    for (std::size_t index = 0u;
         ( (self != D_NULLPTR) &&
           (index < self->subcommands.count) );
         ++index)
    {
        // the one given, when it is the command's
        if ( (_subcommand.get() != D_NULLPTR) &&
             (self->subcommands.items[index] == _subcommand.get()) )
        {
            return _subcommand;
        }
    }

    return cli::command();
}

template<typename Derived,
         typename Base>
inline cli::command
command_interface<Derived, Base>::get_subcommand_no_throw(
    c_string _name
) const D_NOEXCEPT
{
    return get_subcommand(_name);
}

// command_interface::get_subcommand_ptr
//   get_subcommand's.
template<typename Derived,
         typename Base>
inline cli::command
command_interface<Derived, Base>::get_subcommand_ptr(
    c_string _name
) const D_NOEXCEPT
{
    return get_subcommand(_name);
}

template<typename Derived,
         typename Base>
inline cli::command
command_interface<Derived, Base>::get_subcommand_ptr(
    int _index
) const D_NOEXCEPT
{
    return get_subcommand(_index);
}

template<typename Derived,
         typename Base>
inline cli::command
command_interface<Derived, Base>::get_subcommand_ptr(
    const cli::command& _subcommand
) const D_NOEXCEPT
{
    return get_subcommand(_subcommand);
}

template<typename Derived,
         typename Base>
inline cli::option_group
command_interface<Derived, Base>::get_option_group(
    c_string _name
) const D_NOEXCEPT
{
    return cli::option_group(d_arg_command_option_group(record(),
                                                        _name.get()));
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::get_option(
    c_string _name
) const D_NOEXCEPT
{
    return cli::option(d_arg_command_option(record(),
                                            _name.get()));
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::get_option_no_throw(
    c_string _name
) const D_NOEXCEPT
{
    return get_option(_name);
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::operator[](
    c_string _name
) const D_NOEXCEPT
{
    return get_option(_name);
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::get_help_ptr() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return cli::option((self != D_NULLPTR) ? self->help : D_NULLPTR);
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::get_help_all_ptr() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return cli::option((self != D_NULLPTR) ? self->help_all : D_NULLPTR);
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::get_config_ptr() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return cli::option((self != D_NULLPTR) ? self->config : D_NULLPTR);
}

template<typename Derived,
         typename Base>
inline cli::option
command_interface<Derived, Base>::get_version_ptr() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return cli::option((self != D_NULLPTR) ? self->version : D_NULLPTR);
}

template<typename Derived,
         typename Base>
inline cli::command
command_interface<Derived, Base>::get_parent() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return cli::command((self != D_NULLPTR) ? self->parent : D_NULLPTR);
}

// command_interface::name and kin
//   The C text settings, whose failures the C library records.
template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::name(
    c_string _name
)
{
    (void)d_arg_command_set_name(record(),
                                 _name.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::alias(
    c_string _alias
)
{
    (void)d_arg_command_add_alias(record(),
                                  _alias.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::clear_aliases() D_NOEXCEPT
{
    d_arg_command_clear_aliases(record());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::description(
    c_string _description
)
{
    (void)d_arg_command_set_description(record(),
                                        _description.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::group(
    c_string _group
)
{
    (void)d_arg_command_set_group(record(),
                                  _group.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::usage(
    c_string _usage
)
{
    (void)d_arg_command_set_usage(record(),
                                  _usage.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::footer(
    c_string _footer
)
{
    (void)d_arg_command_set_footer(record(),
                                   _footer.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
template<typename Function>
inline typename internal::unless_string<
    Function,
    typename command_interface<Derived, Base>::chain_type>::type
command_interface<Derived, Base>::usage(
    Function _function
)
{
    d_arg_command_set_usage_fn(record(),
                               &internal::command_text_function<Function>,
                               new Function(_function),
                               &internal::release_context<Function>);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
template<typename Function>
inline typename internal::unless_string<
    Function,
    typename command_interface<Derived, Base>::chain_type>::type
command_interface<Derived, Base>::footer(
    Function _function
)
{
    d_arg_command_set_footer_fn(record(),
                                &internal::command_text_function<Function>,
                                new Function(_function),
                                &internal::release_context<Function>);

    return static_cast<Derived&>(*this);
}

// command_interface::allow_extras and kin
//   The C modes; a switch as CLI11 reads it: extras captured or errors,
// configuration extras captured with the command line's or errors, a
// prefix command on or off.
template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::allow_extras(
    bool _allow
) D_NOEXCEPT
{
    return allow_extras(_allow
                        ? extras_mode::capture
                        : extras_mode::error);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::allow_extras(
    extras_mode::value _mode
) D_NOEXCEPT
{
    d_arg_command_set_allow_extras(record(),
                                   static_cast<int>(_mode));

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::allow_config_extras(
    bool _allow
) D_NOEXCEPT
{
    d_arg_command_set_allow_config_extras_bool(record(),
                                               _allow);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::allow_config_extras(
    config_extras_mode::value _mode
) D_NOEXCEPT
{
    d_arg_command_set_allow_config_extras(record(),
                                          static_cast<int>(_mode));

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::prefix_command(
    bool _prefix
) D_NOEXCEPT
{
    return prefix_command(_prefix
                          ? prefix_mode::on
                          : prefix_mode::off);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::prefix_command(
    prefix_mode::value _mode
) D_NOEXCEPT
{
    d_arg_command_set_prefix_command(record(),
                                     static_cast<int>(_mode));

    return static_cast<Derived&>(*this);
}

// command_interface::required and kin
//   The C switches.
template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::required(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_required(record(),
                               _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::disabled(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_disabled(record(),
                               _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::silent(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_silent(record(),
                             _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::allow_windows_style_options(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_windows_style(record(),
                                    _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::positionals_at_end(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_positionals_at_end(record(),
                                         _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::configurable(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_configurable(record(),
                                   _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::validate_positionals(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_validate_positionals(record(),
                                           _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::validate_optional_arguments(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_validate_optional_arguments(record(),
                                                  _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::allow_non_standard_option_names(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_non_standard_names(record(),
                                         _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::allow_subcommand_prefix_matching(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_prefix_matching(record(),
                                      _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::fallthrough(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_fallthrough(record(),
                                  _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::subcommand_fallthrough(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_subcommand_fallthrough(record(),
                                             _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::ignore_case(
    bool _value
) D_NOEXCEPT
{
    (void)d_arg_command_set_ignore_case(record(),
                                        _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::ignore_underscore(
    bool _value
) D_NOEXCEPT
{
    (void)d_arg_command_set_ignore_underscore(record(),
                                              _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::immediate_callback(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_immediate_callback(record(),
                                         _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::disabled_by_default(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_disabled_by_default(record(),
                                          _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::enabled_by_default(
    bool _value
) D_NOEXCEPT
{
    d_arg_command_set_enabled_by_default(record(),
                                         _value);

    return static_cast<Derived&>(*this);
}

// command_interface::require_subcommand / require_option
//   The C requirements; none given is CLI11's one or more.
template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::require_subcommand() D_NOEXCEPT
{
    return require_subcommand(static_cast<std::size_t>(1u),
                              static_cast<std::size_t>(0u));
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::require_subcommand(
    int _value
) D_NOEXCEPT
{
    d_arg_command_require_subcommand_count(record(),
                                           _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::require_subcommand(
    std::size_t _minimum,
    std::size_t _maximum
) D_NOEXCEPT
{
    d_arg_command_require_subcommand(record(),
                                     _minimum,
                                     _maximum);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::require_option() D_NOEXCEPT
{
    return require_option(static_cast<std::size_t>(1u),
                          static_cast<std::size_t>(0u));
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::require_option(
    int _value
) D_NOEXCEPT
{
    d_arg_command_require_option_count(record(),
                                       _value);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::require_option(
    std::size_t _minimum,
    std::size_t _maximum
) D_NOEXCEPT
{
    d_arg_command_require_option(record(),
                                 _minimum,
                                 _maximum);

    return static_cast<Derived&>(*this);
}

// command_interface::callback and kin
//   A copy of the function, owned by the command through its release
// function, called by the matching internal function.
template<typename Derived,
         typename Base>
template<typename Function>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::callback(
    Function _function
)
{
    d_arg_command_set_callback(record(),
                               &internal::command_function<Function>,
                               new Function(_function),
                               &internal::release_context<Function>);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
template<typename Function>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::final_callback(
    Function _function
)
{
    d_arg_command_set_final_callback(record(),
                                     &internal::command_function<Function>,
                                     new Function(_function),
                                     &internal::release_context<Function>);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
template<typename Function>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::parse_complete_callback(
    Function _function
)
{
    d_arg_command_set_parse_complete_callback(
        record(),
        &internal::command_function<Function>,
        new Function(_function),
        &internal::release_context<Function>);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
template<typename Function>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::pre_callback(
    Function _function
)
{
    d_arg_command_set_pre_callback(record(),
                                   &internal::command_function<Function>,
                                   new Function(_function),
                                   &internal::release_context<Function>);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
template<typename Function>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::preparse_callback(
    Function _function
)
{
    d_arg_command_set_preparse_callback(record(),
                                        &internal::preparse_function<Function>,
                                        new Function(_function),
                                        &internal::release_context<Function>);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
template<typename Function>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::failure_message(
    Function _function
)
{
    d_arg_command_set_failure_message(record(),
                                      &internal::failure_function<Function>,
                                      new Function(_function),
                                      &internal::release_context<Function>);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::failure_message(
    ::d_arg_failure_fn _write
) D_NOEXCEPT
{
    d_arg_command_set_failure_message(record(),
                                      _write,
                                      D_NULLPTR,
                                      D_NULLPTR);

    return static_cast<Derived&>(*this);
}

// command_interface::excludes and kin
//   The C relations, by record.
template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::excludes(
    const cli::option& _option
)
{
    (void)d_arg_command_excludes_option(record(),
                                        _option.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::excludes(
    const cli::command& _other
)
{
    (void)d_arg_command_excludes_command(record(),
                                         _other.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::needs(
    const cli::option& _option
)
{
    (void)d_arg_command_needs_option(record(),
                                     _option.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::needs(
    const cli::command& _other
)
{
    (void)d_arg_command_needs_command(record(),
                                      _other.get());

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::remove_excludes(
    const cli::option& _option
) D_NOEXCEPT
{
    return d_arg_command_remove_excludes_option(record(),
                                                _option.get());
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::remove_excludes(
    const cli::command& _other
) D_NOEXCEPT
{
    return d_arg_command_remove_excludes_command(record(),
                                                 _other.get());
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::remove_needs(
    const cli::option& _option
) D_NOEXCEPT
{
    return d_arg_command_remove_needs_option(record(),
                                             _option.get());
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::remove_needs(
    const cli::command& _other
) D_NOEXCEPT
{
    return d_arg_command_remove_needs_command(record(),
                                              _other.get());
}

// command_interface::formatter and kin / option_defaults
//   The C settings and resolutions; the defaults' handle.
template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::formatter(
    ::d_arg_formatter& _formatter
) D_NOEXCEPT
{
    d_arg_command_set_formatter(record(),
                                &_formatter);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline typename command_interface<Derived, Base>::chain_type
command_interface<Derived, Base>::config_formatter(
    ::d_arg_config_format& _format
) D_NOEXCEPT
{
    d_arg_command_set_config_format(record(),
                                    &_format);

    return static_cast<Derived&>(*this);
}

template<typename Derived,
         typename Base>
inline const ::d_arg_formatter*
command_interface<Derived, Base>::get_formatter() const D_NOEXCEPT
{
    return d_arg_command_formatter(record());
}

template<typename Derived,
         typename Base>
inline const ::d_arg_config_format*
command_interface<Derived, Base>::get_config_formatter() const D_NOEXCEPT
{
    return d_arg_command_config_format(record());
}

template<typename Derived,
         typename Base>
inline cli::option_defaults
command_interface<Derived, Base>::option_defaults() D_NOEXCEPT
{
    return cli::option_defaults(d_arg_command_option_defaults(record()));
}

// command_interface::get_required and kin
//   The C command's switches, modes and requirements, read; a NULL handle
// reads as a command made bare.
template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_required() const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_REQUIRED);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_disabled() const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_DISABLED);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_silent() const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_SILENT);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_allow_windows_style_options() const
    D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_WINDOWS_STYLE);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_positionals_at_end() const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_POSITIONALS_AT_END);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_configurable() const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_CONFIGURABLE);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_validate_positionals() const
    D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_VALIDATE_POSITIONALS);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_validate_optional_arguments() const
    D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_VALIDATE_OPTIONAL_ARGUMENTS);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_allow_non_standard_option_names()
    const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_NON_STANDARD_NAMES);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_allow_subcommand_prefix_matching()
    const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_PREFIX_MATCHING);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_fallthrough() const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_FALLTHROUGH);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_subcommand_fallthrough() const
    D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_ignore_case() const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_IGNORE_CASE);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_ignore_underscore() const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_IGNORE_UNDERSCORE);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_immediate_callback() const D_NOEXCEPT
{
    return d_arg_command_has_flag(record(),
                                  D_ARG_COMMAND_IMMEDIATE_CALLBACK);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_disabled_by_default() const
    D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return ( (self != D_NULLPTR) &&
             (self->startup == D_ARG_STARTUP_DISABLED) );
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_enabled_by_default() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return ( (self != D_NULLPTR) &&
             (self->startup == D_ARG_STARTUP_ENABLED) );
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_allow_extras() const D_NOEXCEPT
{
    return (get_allow_extras_mode() > extras_mode::ignore);
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::get_prefix_command() const D_NOEXCEPT
{
    return (get_prefix_command_mode() != prefix_mode::off);
}

template<typename Derived,
         typename Base>
inline std::size_t
command_interface<Derived, Base>::get_require_subcommand_min() const
    D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR) ? self->require_subcommand_min : 0u;
}

template<typename Derived,
         typename Base>
inline std::size_t
command_interface<Derived, Base>::get_require_subcommand_max() const
    D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR) ? self->require_subcommand_max : 0u;
}

template<typename Derived,
         typename Base>
inline std::size_t
command_interface<Derived, Base>::get_require_option_min() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR) ? self->require_option_min : 0u;
}

template<typename Derived,
         typename Base>
inline std::size_t
command_interface<Derived, Base>::get_require_option_max() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR) ? self->require_option_max : 0u;
}

template<typename Derived,
         typename Base>
inline extras_mode::value
command_interface<Derived, Base>::get_allow_extras_mode() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR)
           ? static_cast<extras_mode::value>(self->allow_extras)
           : extras_mode::error;
}

template<typename Derived,
         typename Base>
inline config_extras_mode::value
command_interface<Derived, Base>::get_allow_config_extras() const
    D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR)
           ? static_cast<config_extras_mode::value>(
                 self->allow_config_extras)
           : config_extras_mode::ignore;
}

template<typename Derived,
         typename Base>
inline prefix_mode::value
command_interface<Derived, Base>::get_prefix_command_mode() const
    D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR)
           ? static_cast<prefix_mode::value>(self->prefix_command)
           : prefix_mode::off;
}

// command_interface::get_name and kin
//   The C lookups and matching; the aliases read in place.
template<typename Derived,
         typename Base>
inline const char*
command_interface<Derived, Base>::get_name() const D_NOEXCEPT
{
    return d_arg_command_name(record());
}

template<typename Derived,
         typename Base>
inline const char*
command_interface<Derived, Base>::get_description() const D_NOEXCEPT
{
    return d_arg_command_description(record());
}

template<typename Derived,
         typename Base>
inline const char*
command_interface<Derived, Base>::get_group() const D_NOEXCEPT
{
    return d_arg_command_group(record());
}

template<typename Derived,
         typename Base>
inline name_list
command_interface<Derived, Base>::get_aliases() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR)
           ? name_list(self->aliases.items,
                       self->aliases.count)
           : name_list();
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::check_name(
    const text& _name
) const D_NOEXCEPT
{
    return d_arg_command_check_name(record(),
                                    _name);
}

template<typename Derived,
         typename Base>
inline name_match::value
command_interface<Derived, Base>::check_name_detail(
    const text& _name
) const D_NOEXCEPT
{
    return static_cast<name_match::value>(
               d_arg_command_check_name_detail(record(),
                                               _name));
}

// command_interface::parsed and kin
//   The C parse state, read; the lists over the record's own arrays.
template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::parsed() const D_NOEXCEPT
{
    return (d_arg_command_count(record()) > 0u);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename Derived,
         typename Base>
inline
command_interface<Derived, Base>::operator bool() const D_NOEXCEPT
{
    return parsed();
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename Derived,
         typename Base>
inline std::size_t
command_interface<Derived, Base>::count() const D_NOEXCEPT
{
    return d_arg_command_count(record());
}

template<typename Derived,
         typename Base>
inline std::size_t
command_interface<Derived, Base>::count_all() const D_NOEXCEPT
{
    return d_arg_command_count_all(record());
}

template<typename Derived,
         typename Base>
inline std::size_t
command_interface<Derived, Base>::count(
    c_string _option_name
) const D_NOEXCEPT
{
    return d_arg_option_count(d_arg_command_option(record(),
                                                   _option_name.get()));
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::got_subcommand(
    const cli::command& _subcommand
) const D_NOEXCEPT
{
    return d_arg_command_got_subcommand(record(),
                                        _subcommand.get());
}

template<typename Derived,
         typename Base>
inline bool
command_interface<Derived, Base>::got_subcommand(
    c_string _name
) const D_NOEXCEPT
{
    return d_arg_command_got_subcommand_name(record(),
                                             _name.get());
}

template<typename Derived,
         typename Base>
inline basic_handle_list<cli::command, ::d_arg_command>
command_interface<Derived, Base>::get_subcommands() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR)
           ? cli::command_list(self->parsed_subcommands.items,
                               self->parsed_subcommands.count)
           : cli::command_list();
}

template<typename Derived,
         typename Base>
inline option_list
command_interface<Derived, Base>::parse_order() const D_NOEXCEPT
{
    const ::d_arg_command* const self = record();

    return (self != D_NULLPTR)
           ? option_list(self->parse_order.items,
                         self->parse_order.count)
           : option_list();
}

template<typename Derived,
         typename Base>
inline std::size_t
command_interface<Derived, Base>::remaining_size(
    bool _recurse
) const D_NOEXCEPT
{
    return d_arg_command_remaining_size(record(),
                                        _recurse);
}

// command_interface::get_options and kin
//   The C traversals, each visit appending to a vector, whose failure for
// memory is std::bad_alloc once the traversal is back in C++; a filter run
// over the vector here, so that what it throws never crosses a C frame;
// the subcommands read from the record's own list.
template<typename Derived,
         typename Base>
inline std::vector<cli::option>
command_interface<Derived, Base>::get_options() const
{
    std::vector<cli::option> options;

    internal::raise_out_of_memory(d_arg_command_each_option(
                                      record(),
                                      &internal::option_visit,
                                      &options));

    return options;
}

template<typename Derived,
         typename Base>
template<typename Filter>
inline std::vector<cli::option>
command_interface<Derived, Base>::get_options(
    Filter _filter
) const
{
    const std::vector<cli::option> options = get_options();
    std::vector<cli::option>       kept;

    // each option the filter keeps
    for (std::size_t index = 0u;
         index < options.size();
         ++index)
    {
        // kept, when the filter says so
        if (_filter(options[index]))
        {
            kept.push_back(options[index]);
        }
    }

    return kept;
}

template<typename Derived,
         typename Base>
template<typename Filter>
inline std::vector<cli::command>
command_interface<Derived, Base>::get_subcommands(
    Filter _filter
) const
{
    const ::d_arg_command* const self = record();
    std::vector<cli::command>    kept;

    // each subcommand the filter keeps
    for (std::size_t index = 0u;
         ( (self != D_NULLPTR) &&
           (index < self->subcommands.count) );
         ++index)
    {
        const cli::command subcommand(self->subcommands.items[index]);

        // kept, when the filter says so
        if (_filter(subcommand))
        {
            kept.push_back(subcommand);
        }
    }

    return kept;
}

template<typename Derived,
         typename Base>
inline std::vector<std::string>
command_interface<Derived, Base>::get_groups() const
{
    std::vector<std::string> groups;

    internal::raise_out_of_memory(d_arg_command_each_group(
                                      record(),
                                      &internal::group_visit,
                                      &groups));

    return groups;
}

template<typename Derived,
         typename Base>
inline std::vector<std::string>
command_interface<Derived, Base>::remaining(
    bool _recurse
) const
{
    std::vector<std::string> arguments;

    internal::raise_out_of_memory(d_arg_command_each_remaining(
                                      record(),
                                      _recurse,
                                      &internal::remaining_visit,
                                      &arguments));

    return arguments;
}

template<typename Derived,
         typename Base>
inline std::vector<std::string>
command_interface<Derived, Base>::remaining_for_passthrough(
    bool _recurse
) const
{
    std::vector<std::string> arguments = remaining(_recurse);
    std::vector<std::string> reversed(arguments.rbegin(),
                                      arguments.rend());

    return reversed;
}

// command_interface::get_display_name and kin
//   The C writers, into a scratch buffer or a std::string; a failure for
// memory std::bad_alloc, any other an empty string.
template<typename Derived,
         typename Base>
inline std::string
command_interface<Derived, Base>::get_display_name(
    bool _with_aliases
) const
{
    internal::scratch written;
    const int         result = d_arg_command_display_name(record(),
                                                          _with_aliases,
                                                          written.get());

    internal::raise_out_of_memory(result);

    return (result == D_ARG_OK) ? written.str() : std::string();
}

template<typename Derived,
         typename Base>
inline std::string
command_interface<Derived, Base>::get_usage() const
{
    std::string written;
    const int   result = d_arg_command_usage(record(),
                                             string_sink(written));

    internal::raise_out_of_memory(result);

    return (result == D_ARG_OK) ? written : std::string();
}

template<typename Derived,
         typename Base>
inline std::string
command_interface<Derived, Base>::get_footer() const
{
    std::string written;
    const int   result = d_arg_command_footer(record(),
                                              string_sink(written));

    internal::raise_out_of_memory(result);

    return (result == D_ARG_OK) ? written : std::string();
}

template<typename Derived,
         typename Base>
inline std::string
command_interface<Derived, Base>::version() const
{
    std::string written;
    const int   result = d_arg_command_version(record(),
                                               string_sink(written));

    internal::raise_out_of_memory(result);

    return (result == D_ARG_OK) ? written : std::string();
}

template<typename Derived,
         typename Base>
inline std::string
command_interface<Derived, Base>::help(
    c_string         _prefix,
    help_mode::value _mode
) const
{
    std::string       written;
    const cli::status result = help(string_sink(written),
                                    _prefix,
                                    _mode);

    internal::raise_out_of_memory(result.code());

    return result.ok() ? written : std::string();
}

template<typename Derived,
         typename Base>
inline cli::status
command_interface<Derived, Base>::help(
    ::d_pack_sink    _sink,
    c_string         _prefix,
    help_mode::value _mode
) const
{
    return cli::status(d_arg_help(record(),
                                  text(_prefix.get()),
                                  static_cast<int>(_mode),
                                  _sink));
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// command_interface::config_to_str
//   The C writer, into a std::string; the defaults also is CLI11's all
// defaults.
template<typename Derived,
         typename Base>
inline std::string
command_interface<Derived, Base>::config_to_str(
    bool _default_also,
    bool _write_description
) const
{
    return config_to_str(_default_also
                         ? config_output_mode::all_defaults
                         : config_output_mode::active,
                         _write_description);
}

template<typename Derived,
         typename Base>
inline std::string
command_interface<Derived, Base>::config_to_str(
    config_output_mode::value _mode,
    bool                      _write_description
) const
{
    std::string written;
    const int   result = d_arg_command_config_to_sink(record(),
                                                      static_cast<int>(_mode),
                                                      _write_description,
                                                      string_sink(written));

    internal::raise_out_of_memory(result);

    return (result == D_ARG_OK) ? written : std::string();
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// command_interface::clear / fail / fail_runtime
//   The C calls; fail with the message as its format's one argument, or no
// format for the status's own text.
template<typename Derived,
         typename Base>
inline void
command_interface<Derived, Base>::clear() D_NOEXCEPT
{
    d_arg_command_clear(record());

    return;
}

template<typename Derived,
         typename Base>
inline cli::status
command_interface<Derived, Base>::fail(
    const cli::status& _status,
    c_string           _message
) const
{
    return cli::status((_message.get() != D_NULLPTR)
                       ? d_arg_command_fail(record(),
                                            _status.code(),
                                            "%s",
                                            _message.get())
                       : d_arg_command_fail(record(),
                                            _status.code(),
                                            D_NULLPTR));
}

template<typename Derived,
         typename Base>
inline cli::status
command_interface<Derived, Base>::fail_runtime(
    int _exit_code
) const
{
    return cli::status(d_arg_command_fail_runtime(record(),
                                                  _exit_code));
}

// 6.3    command
//------------------------------------------------------------------------------
// command::command / operator-> / operator* / get / valid
//   The handle; a parser's root, given the parser.
inline
command::command() D_NOEXCEPT
    : command_interface<command, internal::command_handle>()
{}

inline
command::command(
    ::d_arg_command* _command
) D_NOEXCEPT
    : command_interface<command, internal::command_handle>(
          internal::command_handle(_command))
{}

inline
command::command(
    ::d_arg_parser& _parser
) D_NOEXCEPT
    : command_interface<command, internal::command_handle>(
          internal::command_handle(&_parser.command))
{}

inline command*
command::operator->() D_NOEXCEPT
{
    return this;
}

inline const command*
command::operator->() const D_NOEXCEPT
{
    return this;
}

inline command&
command::operator*() D_NOEXCEPT
{
    return *this;
}

inline const command&
command::operator*() const D_NOEXCEPT
{
    return *this;
}

inline ::d_arg_command*
command::get() const D_NOEXCEPT
{
    return record();
}

inline bool
command::valid() const D_NOEXCEPT
{
    return (record() != D_NULLPTR);
}

// 6.4    option_group
//------------------------------------------------------------------------------
// option_group::option_group / operator-> / operator*
//   The handle, its own type for CLI11's arrow and star.
inline
option_group::option_group() D_NOEXCEPT
    : command()
{}

inline
option_group::option_group(
    ::d_arg_command* _group
) D_NOEXCEPT
    : command(_group)
{}

inline option_group*
option_group::operator->() D_NOEXCEPT
{
    return this;
}

inline const option_group*
option_group::operator->() const D_NOEXCEPT
{
    return this;
}

inline option_group&
option_group::operator*() D_NOEXCEPT
{
    return *this;
}

inline const option_group&
option_group::operator*() const D_NOEXCEPT
{
    return *this;
}

// option_group::add_option / add_options / add_subcommand
//   The C moves; several options moved one at a time, in order, as CLI11's
// add_options moves them.
inline cli::option
option_group::add_option(
    const cli::option& _option
)
{
    return (d_arg_command_group_add_option(get(),
                                           _option.get()) == D_ARG_OK)
           ? _option
           : cli::option();
}

inline void
option_group::add_options(
    const cli::option& _option
)
{
    (void)add_option(_option);

    return;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename... Options>
inline void
option_group::add_options(
    const cli::option& _first,
    const cli::option& _second,
    const Options&...  _rest
)
{
    (void)add_option(_first);
    add_options(_second,
                _rest...);

    return;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

inline cli::command
option_group::add_subcommand(
    const cli::command& _subcommand
)
{
    return (d_arg_command_group_add_subcommand(get(),
                                               _subcommand.get())
            == D_ARG_OK)
           ? _subcommand
           : cli::command();
}

// 6.5    Comparing handles
//------------------------------------------------------------------------------
// operator== / operator!= (command)
//   By record.
inline bool
operator==(
    const command& _first,
    const command& _second
) D_NOEXCEPT
{
    return (_first.get() == _second.get());
}

inline bool
operator!=(
    const command& _first,
    const command& _second
) D_NOEXCEPT
{
    return (_first.get() != _second.get());
}

// 6.6    Utilities
//------------------------------------------------------------------------------
// trigger_on / trigger_off
//   The C calls, given one target's record or a vector's records, in order.
inline cli::status
trigger_on(
    const command& _trigger,
    const command& _target
)
{
    ::d_arg_command* const target = _target.get();

    return cli::status(d_arg_command_trigger_on(_trigger.get(),
                                                &target,
                                                1u));
}

inline cli::status
trigger_on(
    const command&              _trigger,
    const std::vector<command>& _targets
)
{
    std::vector< ::d_arg_command*> targets;

    targets.reserve(_targets.size());

    // each target's record
    for (std::size_t index = 0u;
         index < _targets.size();
         ++index)
    {
        targets.push_back(_targets[index].get());
    }

    return cli::status(d_arg_command_trigger_on(
                           _trigger.get(),
                           targets.empty() ? D_NULLPTR : &targets[0],
                           targets.size()));
}

inline cli::status
trigger_off(
    const command& _trigger,
    const command& _target
)
{
    ::d_arg_command* const target = _target.get();

    return cli::status(d_arg_command_trigger_off(_trigger.get(),
                                                 &target,
                                                 1u));
}

inline cli::status
trigger_off(
    const command&              _trigger,
    const std::vector<command>& _targets
)
{
    std::vector< ::d_arg_command*> targets;

    targets.reserve(_targets.size());

    // each target's record
    for (std::size_t index = 0u;
         index < _targets.size();
         ++index)
    {
        targets.push_back(_targets[index].get());
    }

    return cli::status(d_arg_command_trigger_off(
                           _trigger.get(),
                           targets.empty() ? D_NULLPTR : &targets[0],
                           targets.size()));
}

// deprecate_option / retire_option
//   The C calls, each by option and by name; a name no option has is
// CLI11's OptionNotFound for deprecate_option, which C records, and a new
// retired option for retire_option.
inline cli::status
deprecate_option(
    const option& _option,
    c_string      _replacement
)
{
    return cli::status(d_arg_command_deprecate_option(_option.get(),
                                                      _replacement.get()));
}

inline cli::status
deprecate_option(
    const command& _command,
    c_string       _name,
    c_string       _replacement
)
{
    return cli::status(d_arg_command_deprecate_option_name(_command.get(),
                                                           _name.get(),
                                                           _replacement.get()));
}

inline option
retire_option(
    const command& _command,
    const option&  _option
)
{
    return option(d_arg_command_retire_option(_command.get(),
                                              _option.get()));
}

inline option
retire_option(
    const command& _command,
    c_string       _name
)
{
    return option(d_arg_command_retire_option_name(_command.get(),
                                                   _name.get()));
}

// 6.7    Foundations
//------------------------------------------------------------------------------
// internal::command_handle / record_of
//   The pointer; the root a parser holds, which a const parser's handles
// reach as any other's do.
inline
internal::command_handle::command_handle() D_NOEXCEPT
    : m_command(D_NULLPTR)
{}

inline
internal::command_handle::command_handle(
    ::d_arg_command* _command
) D_NOEXCEPT
    : m_command(_command)
{}

inline ::d_arg_command*
internal::command_handle::record() const D_NOEXCEPT
{
    return m_command;
}

inline ::d_arg_command*
internal::record_of(
    const ::d_arg_parser& _parser
) D_NOEXCEPT
{
    return const_cast< ::d_arg_command*>(&_parser.command);
}

inline ::d_arg_command*
internal::record_of(
    const command_handle& _handle
) D_NOEXCEPT
{
    return _handle.record();
}

// 6.8    Internals
//------------------------------------------------------------------------------
// internal::command_failure
//   Out of memory, or the status given with the exception's words, or with
// none for the status's own.
inline
internal::command_failure::command_failure(
    const ::d_arg_command* _command,
    int                    _status
) D_NOEXCEPT
    : m_command(_command),
      m_status(_status)
{}

inline int
internal::command_failure::operator()(
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
           ? d_arg_command_fail(m_command,
                                m_status,
                                "%s",
                                words)
           : d_arg_command_fail(m_command,
                                m_status,
                                D_NULLPTR);
}

// internal::command_call / command_function
//   The function called with nothing; its result -- nothing or a status --
// the callback's status.
template<typename Function>
inline
internal::command_call<Function>::command_call(
    void* _context
) D_NOEXCEPT
    : m_function(static_cast<Function*>(_context))
{}

template<typename Function>
inline int
internal::command_call<Function>::operator()() const
{
    const int kind = internal::call_kind0<Function>::value;

    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        ( (kind == internal::result_kind::none)   ||
          (kind == internal::result_kind::status) ),
        "a command callback returns nothing or a status" );

    return internal::call_status<kind>::call(*m_function);
}

template<typename Function>
inline int
internal::command_function(
    ::d_arg_command* _command,
    void*            _context
)
{
    return internal::guard(internal::command_call<Function>(_context),
                           internal::command_failure(_command,
                                                     D_ARG_ERROR_PARSE));
}

// internal::preparse_call / preparse_function
//   The function called with the arguments left.
template<typename Function>
inline
internal::preparse_call<Function>::preparse_call(
    std::size_t _remaining,
    void*       _context
) D_NOEXCEPT
    : m_remaining(_remaining),
      m_function(static_cast<Function*>(_context))
{}

template<typename Function>
inline int
internal::preparse_call<Function>::operator()() const
{
    const int kind = internal::call_kind1<Function, std::size_t>::value;

    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        ( (kind == internal::result_kind::none)   ||
          (kind == internal::result_kind::status) ),
        "a pre-parse callback returns nothing or a status" );

    std::size_t remaining = m_remaining;

    return internal::call_status<kind>::call(*m_function,
                                             remaining);
}

template<typename Function>
inline int
internal::preparse_function(
    ::d_arg_command* _command,
    std::size_t      _remaining,
    void*            _context
)
{
    return internal::guard(internal::preparse_call<Function>(_remaining,
                                                             _context),
                           internal::command_failure(_command,
                                                     D_ARG_ERROR_PARSE));
}

// internal::command_text_function
//   text_call's work, the command unused.
template<typename Function>
inline int
internal::command_text_function(
    const ::d_arg_command* _command,
    ::d_pack_sink          _sink,
    void*                  _context
)
{
    (void)_command;

    return internal::guard(internal::text_call<Function>(_sink,
                                                         _context),
                           internal::conversion_failure(D_NULLPTR));
}

// internal::failure_call / failure_function
//   The function called with the command's handle and the error's view; its
// text, whatever text type it is, written whole.
template<typename Function>
inline
internal::failure_call<Function>::failure_call(
    const ::d_arg_command* _command,
    const ::d_arg_error*   _error,
    ::d_pack_sink          _sink,
    void*                  _context
) D_NOEXCEPT
    : m_command(_command),
      m_error(_error),
      m_sink(_sink),
      m_function(static_cast<Function*>(_context))
{}

template<typename Function>
inline int
internal::failure_call<Function>::operator()() const
{
    const cli::command failed(const_cast< ::d_arg_command*>(m_command));
    const cli::error   report(*m_error);

    return write((*m_function)(failed,
                               report));
}

template<typename Function>
template<typename Result>
inline int
internal::failure_call<Function>::write(
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
internal::failure_function(
    const ::d_arg_command* _command,
    const ::d_arg_error*   _error,
    ::d_pack_sink          _sink,
    void*                  _context
)
{
    return internal::guard(internal::failure_call<Function>(_command,
                                                            _error,
                                                            _sink,
                                                            _context),
                           internal::conversion_failure(D_NULLPTR));
}

// internal::flag_call / flag_function
//   The function called with nothing; the count, always 1, unused.
template<typename Function>
inline
internal::flag_call<Function>::flag_call(
    void* _context
) D_NOEXCEPT
    : m_function(static_cast<Function*>(_context))
{}

template<typename Function>
inline int
internal::flag_call<Function>::operator()() const
{
    const int kind = internal::call_kind0<Function>::value;

    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        ( (kind == internal::result_kind::none)   ||
          (kind == internal::result_kind::status) ),
        "a flag callback returns nothing or a status" );

    return internal::call_status<kind>::call(*m_function);
}

template<typename Function>
inline int
internal::flag_function(
    ::d_arg_option* _option,
    long            _count,
    void*           _context
)
{
    (void)_count;

    return internal::guard(internal::flag_call<Function>(_context),
                           internal::callback_failure(
                               _option,
                               D_ARG_ERROR_CONVERSION));
}

// internal::count_call / count_function
//   The function called with the count.
template<typename Function>
inline
internal::count_call<Function>::count_call(
    flag_count _count,
    void*      _context
) D_NOEXCEPT
    : m_count(_count),
      m_function(static_cast<Function*>(_context))
{}

template<typename Function>
inline int
internal::count_call<Function>::operator()() const
{
    const int kind = internal::call_kind1<Function, flag_count>::value;

    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        ( (kind == internal::result_kind::none)   ||
          (kind == internal::result_kind::status) ),
        "a flag function returns nothing or a status" );

    flag_count count = m_count;

    return internal::call_status<kind>::call(*m_function,
                                             count);
}

template<typename Function>
inline int
internal::count_function(
    ::d_arg_option* _option,
    flag_count      _count,
    void*           _context
)
{
    return internal::guard(internal::count_call<Function>(_count,
                                                          _context),
                           internal::callback_failure(
                               _option,
                               D_ARG_ERROR_CONVERSION));
}

// internal::typed_call / typed_function
//   The values converted by the type's descriptor under the option's
// integer rule, without a reason -- the parser then writes CLI11's "Could
// not convert" -- then the function called with the value.
template<typename Type,
         typename Function>
inline
internal::typed_call<Type, Function>::typed_call(
    const ::d_arg_option* _option,
    const ::d_pack_text*  _results,
    std::size_t           _count,
    void*                 _context
) D_NOEXCEPT
    : m_option(_option),
      m_results(_results),
      m_count(_count),
      m_function(static_cast<Function*>(_context))
{}

template<typename Type,
         typename Function>
inline int
internal::typed_call<Type, Function>::operator()() const
{
    const int kind = internal::call_kind1<Function, const Type>::value;

    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        ( (kind == internal::result_kind::none)   ||
          (kind == internal::result_kind::status) ),
        "an option function returns nothing or a status" );

    Type                          value   = Type();
    void* const                   target  = type_traits<Type>::target(value);
    const ::d_arg_convert_context context = { m_option->integer_rule,
                                              m_option,
                                              D_NULLPTR };
    const int                     result  = d_arg_convert(
                                                type_traits<Type>::descriptor(),
                                                m_results,
                                                m_count,
                                                target,
                                                &context);

    // values that do not convert are not given to the function
    if (result != D_ARG_OK)
    {
        return result;
    }

    const Type& converted = value;

    return internal::call_status<kind>::call(*m_function,
                                             converted);
}

template<typename Type,
         typename Function>
inline int
internal::typed_function(
    ::d_arg_option*      _option,
    const ::d_pack_text* _results,
    std::size_t          _count,
    void*                _context
)
{
    return internal::guard(internal::typed_call<Type, Function>(_option,
                                                                _results,
                                                                _count,
                                                                _context),
                           internal::callback_failure(
                               _option,
                               D_ARG_ERROR_CONVERSION));
}

// internal::no_default_text
//   Nothing.
inline const char*
internal::no_default_text::operator()() const D_NOEXCEPT
{
    return "";
}

// internal::raise_out_of_memory
//   With exceptions: std::bad_alloc for the statuses that mean memory ran
// out. Without them: nothing.
#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)

inline void
internal::raise_out_of_memory(
    int _status
)
{
    // memory ran out, or a std::string refused bytes, which is the same
    if ( (_status == D_ARG_ERROR_OUT_OF_MEMORY) ||
         (_status == D_ARG_ERROR_FILE) )
    {
        throw std::bad_alloc();
    }

    return;
}

#else

inline void
internal::raise_out_of_memory(
    int _status
)
{
    (void)_status;

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_CPP_CATCH

// internal::remaining_collect / remaining_visit
//   The argument's text, appended as a std::string.
inline
internal::remaining_collect::remaining_collect(
    const ::d_arg_extra* _extra,
    void*                _context
) D_NOEXCEPT
    : m_extra(_extra),
      m_list(static_cast<std::vector<std::string>*>(_context))
{}

inline int
internal::remaining_collect::operator()() const
{
    m_list->push_back(text(m_extra->text).str());

    return D_ARG_OK;
}

inline int
internal::remaining_visit(
    const ::d_arg_extra* _extra,
    void*                _context
)
{
    return internal::guard(internal::remaining_collect(_extra,
                                                       _context),
                           internal::fallback<int>(
                               D_ARG_ERROR_OUT_OF_MEMORY));
}

// internal::option_collect / option_visit
//   The option's handle, appended.
inline
internal::option_collect::option_collect(
    ::d_arg_option* _option,
    void*           _context
) D_NOEXCEPT
    : m_option(_option),
      m_list(static_cast<std::vector<cli::option>*>(_context))
{}

inline int
internal::option_collect::operator()() const
{
    m_list->push_back(cli::option(m_option));

    return D_ARG_OK;
}

inline int
internal::option_visit(
    ::d_arg_option* _option,
    void*           _context
)
{
    return internal::guard(internal::option_collect(_option,
                                                    _context),
                           internal::fallback<int>(
                               D_ARG_ERROR_OUT_OF_MEMORY));
}

// internal::group_collect / group_visit
//   The group's name, appended as a std::string.
inline
internal::group_collect::group_collect(
    ::d_pack_text _group,
    void*         _context
) D_NOEXCEPT
    : m_group(_group),
      m_list(static_cast<std::vector<std::string>*>(_context))
{}

inline int
internal::group_collect::operator()() const
{
    m_list->push_back(text(m_group).str());

    return D_ARG_OK;
}

inline int
internal::group_visit(
    ::d_pack_text _group,
    void*         _context
)
{
    return internal::guard(internal::group_collect(_group,
                                                   _context),
                           internal::fallback<int>(
                               D_ARG_ERROR_OUT_OF_MEMORY));
}

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMAND_HPP
