/*******************************************************************************
* djinterp [parse]                                        arg_boundary_tests.cpp
*
* The argument parser's C++ boundary suite's sections.
*   Each step gives the face one function of the caller's and runs what
* calls it: a parse, written as the run writes one -- the ERR line of a
* parse that fails, then the root's report -- or help, written with its
* status, or the writer the function serves. The results' section runs
* each function that may return more than one kind of result with each:
* nothing; a status -- ok, which is no news, one that fails bare, which the
* parse gives its own message, and one its option or command failed with a
* message first, which is kept -- given as a cli::status or a status_code;
* and, for an option's callback, a bool. The sections of exceptions, in a
* build with them, run every function the face guards throwing a
* std::runtime_error whose what() names it, one whose what() is empty,
* std::bad_alloc, then an exception of no standard type, row by row of
* ISSUES 47's table: an option's functions and a type's; a command's;
* text's -- help's steps and a sink; filters; and a configuration format's
* operations. Every section is the module's own.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_boundary_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_boundary_tests.hpp"  // corresponding header
// std
#include <cstddef>    // std::size_t
#include <istream>    // std::istream
#include <map>        // std::map
#include <new>        // std::bad_alloc
#include <ostream>    // std::ostream
#include <sstream>    // std::istringstream
#include <stdexcept>  // std::runtime_error
#include <streambuf>  // std::streambuf
#include <string>     // std::string
// djinterp
#include "../../../../../inc/djinterp/djinterp.hpp"  // framework root
#include "../../../../../inc/djinterp/parse/cli/arg_parser/arg_parser.hpp"  // the face
#include "./arg_tests_support.hpp"  // run, steps, transcripts
#include "../../c/tests_section.h"  // d_tests_section


namespace
{

namespace cli = ::djinterp::parse::cli;

using cli::testing::run;
using cli::testing::step_fn;
using cli::testing::escape;
using cli::testing::steps_hold;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// RESULTS
//   constant: what the steps of d_tests_arg_cpp_boundary_results write: the
// module's own, as CLI11's functions return nothing but an option's
// callback's bool, false being CLI11's "Could not convert".
const char RESULTS[] =
    "option callback called\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ExcludesError 108: Excludes error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR RequiresError 107: needs more\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Could not convert: --opt = 1\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "option function called\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "flag callback called\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "flag function called\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "each called\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: Validation error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: nope\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "callback called\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ExcludesError 108: Excludes error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ValidationError 105: said no\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "final callback called\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "parse complete callback called\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "pre-callback called\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "pre-parse callback called\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n";

#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)

// THROWN_OPTIONS
//   constant: what the steps of d_tests_arg_cpp_boundary_thrown_options
// write: the module's own, as what CLI11's functions throw leaves parse().
const char THROWN_OPTIONS[] =
    "ERR ConversionError 104: option callback\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: option function\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: flag callback\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR ConversionError 104: flag function\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "ERR ValidationError 105: --opt: each\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: Validation error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: Validation error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: validator\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: Validation error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: Validation error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: transformation\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: Validation error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ValidationError 105: --opt: Validation error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: conversion\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Could not convert: --opt = 1\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Could not convert: --opt = 1\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "THROWN ConversionError: Conversion error\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "THROWN ConversionError: Conversion error\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "THROWN OutOfMemory: Out of memory\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "THROWN ConversionError: Conversion error\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=OutOfMemory written=[] string=THROWN std::bad_alloc\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "THROWN ConversionError: Conversion error\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "THROWN ConversionError: Conversion error\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "THROWN OutOfMemory: Out of memory\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "THROWN ConversionError: Conversion error\n";

// THROWN_COMMANDS
//   constant: what the steps of d_tests_arg_cpp_boundary_thrown_commands
// write: the module's own, as what CLI11's functions throw leaves parse().
const char THROWN_COMMANDS[] =
    "ERR ParseError 111: callback\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: final callback\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: parse complete callback\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: pre-callback\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: pre-parse callback\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ParseError 111: Parse error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--opt 1\n"
    "cmd app n=1 miss=[--opt|1] subs=[] order=[]\n"
    "exit 109\n"
    "err prog: The following arguments were not expected: --opt 1\\nRun with "
    "--help for more information.\\n\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--opt 1\n"
    "cmd app n=1 miss=[--opt|1] subs=[] order=[]\n"
    "exit 109\n"
    "err prog: The following arguments were not expected: --opt 1\\nRun with "
    "--help for more information.\\n\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--opt 1\n"
    "cmd app n=1 miss=[--opt|1] subs=[] order=[]\n"
    "exit 109\n"
    "err prog: The following arguments were not expected: --opt 1\\nRun with "
    "--help for more information.\\n\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--opt 1\n"
    "cmd app n=1 miss=[--opt|1] subs=[] order=[]\n"
    "exit 109\n"
    "err prog: The following arguments were not expected: --opt 1\\nRun with "
    "--help for more information.\\n\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=OutOfMemory written=[] string=THROWN std::bad_alloc\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=OutOfMemory written=[] string=THROWN std::bad_alloc\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--version]\n"
    "exit 104\n"
    "err Conversion error\\nRun with --help for more information.\\n\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--version]\n"
    "exit 104\n"
    "err Conversion error\\nRun with --help for more information.\\n\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--version]\n"
    "exit 127\n"
    "err Out of memory\\nRun with --help for more information.\\n\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--version]\n"
    "exit 104\n"
    "err Conversion error\\nRun with --help for more information.\\n\n";

// THROWN_TEXT
//   constant: what the steps of d_tests_arg_cpp_boundary_thrown_text write:
// the module's own, as what CLI11's functions throw leaves the call.
const char THROWN_TEXT[] =
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=OutOfMemory written=[] string=THROWN std::bad_alloc\n"
    "help: sink=ConversionError written=[] string=[]\n"
    "help: sink=FileError\n"
    "help: sink=FileError\n"
    "help: sink=FileError\n"
    "help: sink=FileError\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// THROWN_FILTERS
//   constant: what the steps of d_tests_arg_cpp_boundary_thrown_filters
// write: the module's own, as what CLI11's filters throw leaves parse().
const char THROWN_FILTERS[] =
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// THROWN_CONFIG
//   constant: what the steps of d_tests_arg_cpp_boundary_thrown_config
// write: the module's own, as what CLI11's formats throw leaves the call.
const char THROWN_CONFIG[] =
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "ERR OutOfMemory 127: Out of memory\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "ERR ConversionError 104: Conversion error\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "written []\n"
    "written []\n"
    "written THROWN std::bad_alloc\n"
    "written []\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#endif  // D_INTERNAL_ARG_PARSER_CPP_CATCH

// OPTION_ARGS
//   constant: an option given one value.
const char* const OPTION_ARGS[] =
{
    "--opt", "1"
};

// FLAG_ARGS
//   constant: a flag given.
const char* const FLAG_ARGS[] =
{
    "--flag"
};

//==============================================================================
// 1.  RESULTS
//==============================================================================

// 1.1    Functions that return each kind
//------------------------------------------------------------------------------

// noting
//   struct: a function that returns nothing, and writes that it was called.
struct noting
{
    run*        m_run;
    const char* m_text;

    /**
     * @brief Writes that it was called, given nothing.
     */
    void
    operator()() const
    {
        m_run->note(m_text);

        return;
    }

    /**
     * @brief Writes that it was called, given one argument.
     *
     * @param[in] _first  the argument; unused.
     */
    template<typename First>
    void
    operator()(
        const First& _first
    ) const
    {
        (void)_first;
        m_run->note(m_text);

        return;
    }
};

// stating
//   struct: a function that returns a cli::status.
struct stating
{
    cli::status_code::value m_code;

    /**
     * @brief The status, given nothing.
     *
     * @return the status.
     */
    cli::status
    operator()() const
    {
        return cli::status(m_code);
    }

    /**
     * @brief The status, given one argument.
     *
     * @param[in] _first  the argument; unused.
     * @return the status.
     */
    template<typename First>
    cli::status
    operator()(
        const First& _first
    ) const
    {
        (void)_first;

        return cli::status(m_code);
    }
};

// coding
//   struct: a function that returns a status_code, as a callback that
// `return cli::status_code::...;` does with its function's type spelled.
struct coding
{
    cli::status_code::value m_code;

    /**
     * @brief The code, given nothing.
     *
     * @return the code.
     */
    cli::status_code::value
    operator()() const
    {
        return m_code;
    }

    /**
     * @brief The code, given one argument.
     *
     * @param[in] _first  the argument; unused.
     * @return the code.
     */
    template<typename First>
    cli::status_code::value
    operator()(
        const First& _first
    ) const
    {
        (void)_first;

        return m_code;
    }
};

// telling
//   struct: an option's callback that returns a truth value, as CLI11's
// callback_t.
struct telling
{
    bool m_value;

    /**
     * @brief The truth value, given the option's values.
     *
     * @param[in] _values  the values; unused.
     * @return the truth value.
     */
    template<typename Values>
    bool
    operator()(
        const Values& _values
    ) const
    {
        (void)_values;

        return m_value;
    }
};

// refusing_option
//   struct: an option's function that fails through the option's fail(),
// with a message, and returns the status fail() gives.
struct refusing_option
{
    cli::option             m_option;
    cli::status_code::value m_code;
    const char*             m_message;

    /**
     * @brief Fails, given one argument.
     *
     * @param[in] _first  the argument; unused.
     * @return the status fail() gives.
     */
    template<typename First>
    cli::status
    operator()(
        const First& _first
    ) const
    {
        (void)_first;

        return m_option.fail(m_code,
                             m_message);
    }
};

// refusing_command
//   struct: a command's callback that fails through the root's fail(), with
// a message, and returns the status fail() gives.
struct refusing_command
{
    run*                    m_run;
    cli::status_code::value m_code;
    const char*             m_message;

    /**
     * @brief Fails.
     *
     * @return the status fail() gives.
     */
    cli::status
    operator()() const
    {
        return m_run->app().fail(m_code,
                                 m_message);
    }
};

// 1.2    What calls each function
//------------------------------------------------------------------------------

/**
 * @brief An option's callback, called by a parse that gives the option.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the callback.
 */
template<typename Function>
void
option_callback_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().add_option("--opt",
                          "an option")
              ->callback(_function);
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

/**
 * @brief add_option_function's function, called by a parse that gives the
 *        option.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the function, given an int.
 */
template<typename Function>
void
option_function_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().add_option_function<int>("--opt",
                                        _function,
                                        "an option");
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

/**
 * @brief add_flag_callback's function, called by a parse that gives the
 *        flag.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the function.
 */
template<typename Function>
void
flag_callback_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().add_flag_callback("--flag",
                                 _function,
                                 "a flag");
    _run.parse(FLAG_ARGS,
               1u);

    return;
}

/**
 * @brief add_flag_function's function, called by a parse that gives the
 *        flag.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the function, given the count.
 */
template<typename Function>
void
flag_function_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().add_flag_function("--flag",
                                 _function,
                                 "a flag");
    _run.parse(FLAG_ARGS,
               1u);

    return;
}

/**
 * @brief An each() function, called by a parse that gives the option.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the function, given the value.
 */
template<typename Function>
void
each_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().add_option("--opt",
                          "an option")
              ->each(_function);
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

/**
 * @brief The root's callback, called by a parse.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the callback.
 */
template<typename Function>
void
callback_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().callback(_function);
    _run.parse(OPTION_ARGS,
               0u);

    return;
}

/**
 * @brief The root's final callback, called by a parse.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the callback.
 */
template<typename Function>
void
final_callback_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().final_callback(_function);
    _run.parse(OPTION_ARGS,
               0u);

    return;
}

/**
 * @brief The root's parse-complete callback, called by a parse.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the callback.
 */
template<typename Function>
void
parse_complete_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().parse_complete_callback(_function);
    _run.parse(OPTION_ARGS,
               0u);

    return;
}

/**
 * @brief The root's pre-callback, called by a parse.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the callback.
 */
template<typename Function>
void
pre_callback_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().pre_callback(_function);
    _run.parse(OPTION_ARGS,
               0u);

    return;
}

/**
 * @brief The root's pre-parse callback, called by a parse with the count
 *        of the arguments.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _function  the callback, given the count.
 */
template<typename Function>
void
preparse_case(
    run&            _run,
    const Function& _function
)
{
    _run.app().preparse_callback(_function);
    _run.parse(OPTION_ARGS,
               0u);

    return;
}

// 1.3    An option's functions
//------------------------------------------------------------------------------

/**
 * @brief RESULTS 1: an option's callback returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_option_callback_nothing(
    run& _run
)
{
    const noting called = { &_run, "option callback called" };

    option_callback_case(_run,
                         called);

    return;
}

/**
 * @brief RESULTS 2: an option's callback returning ok, which is no news.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_option_callback_ok(
    run& _run
)
{
    const stating passing = { cli::status_code::ok };

    option_callback_case(_run,
                         passing);

    return;
}

/**
 * @brief RESULTS 3: an option's callback returning a status that fails,
 *        bare.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_option_callback_status(
    run& _run
)
{
    const stating failing = { cli::status_code::requires_error };

    option_callback_case(_run,
                         failing);

    return;
}

/**
 * @brief RESULTS 4: an option's callback returning a status_code that
 *        fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_option_callback_code(
    run& _run
)
{
    const coding failing = { cli::status_code::excludes_error };

    option_callback_case(_run,
                         failing);

    return;
}

/**
 * @brief RESULTS 5: an option's callback that fails through the option's
 *        fail(), with a message, which is kept.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_option_callback_failed(
    run& _run
)
{
    cli::option           opt     = _run.app().add_option("--opt",
                                                          "an option");
    const refusing_option refuser = { opt,
                                      cli::status_code::requires_error,
                                      "needs more" };

    opt->callback(refuser);
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

/**
 * @brief RESULTS 6: an option's callback returning true.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_option_callback_true(
    run& _run
)
{
    const telling accepted = { true };

    option_callback_case(_run,
                         accepted);

    return;
}

/**
 * @brief RESULTS 7: an option's callback returning false, CLI11's
 *        "Could not convert".
 *
 * @param[in,out] _run  the step's run.
 */
void
results_option_callback_false(
    run& _run
)
{
    const telling refused = { false };

    option_callback_case(_run,
                         refused);

    return;
}

/**
 * @brief RESULTS 8: add_option_function's function returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_option_function_nothing(
    run& _run
)
{
    const noting called = { &_run, "option function called" };

    option_function_case(_run,
                         called);

    return;
}

/**
 * @brief RESULTS 9: add_option_function's function returning a status that
 *        fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_option_function_status(
    run& _run
)
{
    const stating failing = { cli::status_code::requires_error };

    option_function_case(_run,
                         failing);

    return;
}

/**
 * @brief RESULTS 10: add_flag_callback's function returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_flag_callback_nothing(
    run& _run
)
{
    const noting called = { &_run, "flag callback called" };

    flag_callback_case(_run,
                       called);

    return;
}

/**
 * @brief RESULTS 11: add_flag_callback's function returning a status that
 *        fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_flag_callback_status(
    run& _run
)
{
    const stating failing = { cli::status_code::requires_error };

    flag_callback_case(_run,
                       failing);

    return;
}

/**
 * @brief RESULTS 12: add_flag_function's function returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_flag_function_nothing(
    run& _run
)
{
    const noting called = { &_run, "flag function called" };

    flag_function_case(_run,
                       called);

    return;
}

/**
 * @brief RESULTS 13: add_flag_function's function returning a status that
 *        fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_flag_function_status(
    run& _run
)
{
    const stating failing = { cli::status_code::requires_error };

    flag_function_case(_run,
                       failing);

    return;
}

/**
 * @brief RESULTS 14: an each() function returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_each_nothing(
    run& _run
)
{
    const noting called = { &_run, "each called" };

    each_case(_run,
              called);

    return;
}

/**
 * @brief RESULTS 15: an each() function returning validation_error, bare,
 *        which rejects the value.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_each_rejected(
    run& _run
)
{
    const stating rejecting = { cli::status_code::validation_error };

    each_case(_run,
              rejecting);

    return;
}

/**
 * @brief RESULTS 16: an each() function that fails through the option's
 *        fail() with validation_error and a message, which rejects the
 *        value with it -- CLI11's ValidationError("nope") thrown from one.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_each_failed(
    run& _run
)
{
    cli::option           opt     = _run.app().add_option("--opt",
                                                          "an option");
    const refusing_option refuser = { opt,
                                      cli::status_code::validation_error,
                                      "nope" };

    opt->each(refuser);
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

/**
 * @brief RESULTS 17: an each() function returning a status_code other than
 *        validation_error, which ends the parse with it.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_each_status(
    run& _run
)
{
    const coding failing = { cli::status_code::requires_error };

    each_case(_run,
              failing);

    return;
}

// 1.4    A command's functions
//------------------------------------------------------------------------------

/**
 * @brief RESULTS 18: the root's callback returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_callback_nothing(
    run& _run
)
{
    const noting called = { &_run, "callback called" };

    callback_case(_run,
                  called);

    return;
}

/**
 * @brief RESULTS 19: the root's callback returning ok, which is no news.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_callback_ok(
    run& _run
)
{
    const coding passing = { cli::status_code::ok };

    callback_case(_run,
                  passing);

    return;
}

/**
 * @brief RESULTS 20: the root's callback returning a status that fails,
 *        bare.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_callback_status(
    run& _run
)
{
    const stating failing = { cli::status_code::requires_error };

    callback_case(_run,
                  failing);

    return;
}

/**
 * @brief RESULTS 21: the root's callback returning a status_code that
 *        fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_callback_code(
    run& _run
)
{
    const coding failing = { cli::status_code::excludes_error };

    callback_case(_run,
                  failing);

    return;
}

/**
 * @brief RESULTS 22: the root's callback that fails through the root's
 *        fail(), with a message, which is kept.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_callback_failed(
    run& _run
)
{
    const refusing_command refuser = { &_run,
                                       cli::status_code::validation_error,
                                       "said no" };

    callback_case(_run,
                  refuser);

    return;
}

/**
 * @brief RESULTS 23: the root's final callback returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_final_callback_nothing(
    run& _run
)
{
    const noting called = { &_run, "final callback called" };

    final_callback_case(_run,
                        called);

    return;
}

/**
 * @brief RESULTS 24: the root's final callback returning a status that
 *        fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_final_callback_status(
    run& _run
)
{
    const stating failing = { cli::status_code::requires_error };

    final_callback_case(_run,
                        failing);

    return;
}

/**
 * @brief RESULTS 25: the root's parse-complete callback returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_parse_complete_nothing(
    run& _run
)
{
    const noting called = { &_run, "parse complete callback called" };

    parse_complete_case(_run,
                        called);

    return;
}

/**
 * @brief RESULTS 26: the root's parse-complete callback returning a status
 *        that fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_parse_complete_status(
    run& _run
)
{
    const stating failing = { cli::status_code::requires_error };

    parse_complete_case(_run,
                        failing);

    return;
}

/**
 * @brief RESULTS 27: the root's pre-callback returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_pre_callback_nothing(
    run& _run
)
{
    const noting called = { &_run, "pre-callback called" };

    pre_callback_case(_run,
                      called);

    return;
}

/**
 * @brief RESULTS 28: the root's pre-callback returning a status that fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_pre_callback_status(
    run& _run
)
{
    const stating failing = { cli::status_code::requires_error };

    pre_callback_case(_run,
                      failing);

    return;
}

/**
 * @brief RESULTS 29: the root's pre-parse callback returning nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_preparse_nothing(
    run& _run
)
{
    const noting called = { &_run, "pre-parse callback called" };

    preparse_case(_run,
                  called);

    return;
}

/**
 * @brief RESULTS 30: the root's pre-parse callback returning a status that
 *        fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_preparse_status(
    run& _run
)
{
    const stating failing = { cli::status_code::requires_error };

    preparse_case(_run,
                  failing);

    return;
}

// RESULTS_STEPS
//   constant: the steps of d_tests_arg_cpp_boundary_results.
const step_fn RESULTS_STEPS[] =
{
    &results_option_callback_nothing,
    &results_option_callback_ok,
    &results_option_callback_status,
    &results_option_callback_code,
    &results_option_callback_failed,
    &results_option_callback_true,
    &results_option_callback_false,
    &results_option_function_nothing,
    &results_option_function_status,
    &results_flag_callback_nothing,
    &results_flag_callback_status,
    &results_flag_function_nothing,
    &results_flag_function_status,
    &results_each_nothing,
    &results_each_rejected,
    &results_each_failed,
    &results_each_status,
    &results_callback_nothing,
    &results_callback_ok,
    &results_callback_status,
    &results_callback_code,
    &results_callback_failed,
    &results_final_callback_nothing,
    &results_final_callback_status,
    &results_parse_complete_nothing,
    &results_parse_complete_status,
    &results_pre_callback_nothing,
    &results_pre_callback_status,
    &results_preparse_nothing,
    &results_preparse_status
};

// RESULTS_COUNT
//   constant: how many steps RESULTS_STEPS holds.
const std::size_t RESULTS_COUNT =
    sizeof(RESULTS_STEPS) / sizeof(RESULTS_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)

//==============================================================================
// 2.  EXCEPTIONS
//==============================================================================

// 2.1    Throwing
//------------------------------------------------------------------------------

// thrown_kind
//   enum: what a function throws: a std::runtime_error with words, one whose
// what() is empty, std::bad_alloc, or an exception of no standard type.
enum thrown_kind
{
    runtime_thrown,
    empty_thrown,
    memory_thrown,
    foreign_thrown
};

// foreign
//   struct: an exception of no standard type, which the face's guard
// catches with `...`.
struct foreign
{};

/**
 * @brief Throws what a kind names.
 *
 * @param[in] _kind  the kind.
 * @param[in] _what  the std::runtime_error's what().
 */
void
raise(
    thrown_kind _kind,
    const char* _what
)
{
    // the allocation refused
    if (_kind == memory_thrown)
    {
        throw std::bad_alloc();
    }

    // no standard type
    if (_kind == foreign_thrown)
    {
        throw foreign();
    }

    throw std::runtime_error((_kind == empty_thrown) ? "" : _what);
}

// throwing_void
//   struct: a function returning nothing that throws, given nothing or one
// argument.
struct throwing_void
{
    thrown_kind m_kind;
    const char* m_what;

    /**
     * @brief Throws, given nothing.
     */
    void
    operator()() const
    {
        raise(m_kind,
              m_what);

        return;
    }

    /**
     * @brief Throws, given one argument.
     *
     * @param[in] _first  the argument; unused.
     */
    template<typename First>
    void
    operator()(
        const First& _first
    ) const
    {
        (void)_first;
        raise(m_kind,
              m_what);

        return;
    }
};

// throwing_text
//   struct: a function returning text that throws, given nothing, one
// argument, or two.
struct throwing_text
{
    thrown_kind m_kind;
    const char* m_what;

    /**
     * @brief Throws, given nothing.
     *
     * @return nothing: it throws.
     */
    std::string
    operator()() const
    {
        raise(m_kind,
              m_what);

        return std::string();
    }

    /**
     * @brief Throws, given one argument.
     *
     * @param[in] _first  the argument; unused.
     * @return nothing: it throws.
     */
    template<typename First>
    std::string
    operator()(
        const First& _first
    ) const
    {
        (void)_first;
        raise(m_kind,
              m_what);

        return std::string();
    }

    /**
     * @brief Throws, given two arguments.
     *
     * @param[in] _first   the first; unused.
     * @param[in] _second  the second; unused.
     * @return nothing: it throws.
     */
    template<typename First,
             typename Second>
    std::string
    operator()(
        const First&  _first,
        const Second& _second
    ) const
    {
        (void)_first;
        (void)_second;
        raise(m_kind,
              m_what);

        return std::string();
    }
};

/**
 * @brief Help as a std::string, escaped and bracketed, or that it threw
 *        std::bad_alloc, as the string form does for memory run out.
 *
 * @param[in,out] _run  the run.
 * @return the text.
 */
std::string
help_string(
    run& _run
)
{
    try
    {
        return "[" + escape(_run.app().help()) + "]";
    }
    catch (const std::bad_alloc&)
    {
        return "THROWN std::bad_alloc";
    }
}

/**
 * @brief Writes help's text and status: into a sink, then as a string.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line names the help by.
 */
void
write_help(
    run&        _run,
    const char* _label
)
{
    std::string       sunk;
    const cli::status written = _run.app().help(cli::string_sink(sunk));

    _run.note(std::string(_label) + ": sink=" + written.get_name() +
              " written=[" + escape(sunk) + "] string=" +
              help_string(_run));

    return;
}

// 2.2    An option's functions and a type's
//------------------------------------------------------------------------------

// brittle
//   struct: a type read with operator>> and written with operator<<, each of
// which throws what the type names -- the type's, as a value is read into
// one the face makes.
template<thrown_kind Kind>
struct brittle
{
    int m_value;
};

/**
 * @brief Reads a brittle value: throws.
 *
 * @param[in,out] _stream  the stream; unused.
 * @param[out]    _value   the value; unused.
 * @return the stream: it throws.
 */
template<thrown_kind Kind>
std::istream&
operator>>(
    std::istream&  _stream,
    brittle<Kind>& _value
)
{
    (void)_value;
    raise(Kind,
          "conversion");

    return _stream;
}

/**
 * @brief Writes a brittle value: throws.
 *
 * @param[in,out] _stream  the stream; unused.
 * @param[in]     _value   the value; unused.
 * @return the stream: it throws.
 */
template<thrown_kind Kind>
std::ostream&
operator<<(
    std::ostream&        _stream,
    const brittle<Kind>& _value
)
{
    (void)_value;
    raise(Kind,
          "writer");

    return _stream;
}

/**
 * @brief THROWN_OPTIONS 1: an option's callback that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_option_callback(
    run& _run
)
{
    const throwing_void thrower = { Kind, "option callback" };

    option_callback_case(_run,
                         thrower);

    return;
}

/**
 * @brief THROWN_OPTIONS 2: add_option_function's function that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_option_function(
    run& _run
)
{
    const throwing_void thrower = { Kind, "option function" };

    option_function_case(_run,
                         thrower);

    return;
}

/**
 * @brief THROWN_OPTIONS 3: add_flag_callback's function that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_flag_callback(
    run& _run
)
{
    const throwing_void thrower = { Kind, "flag callback" };

    flag_callback_case(_run,
                       thrower);

    return;
}

/**
 * @brief THROWN_OPTIONS 4: add_flag_function's function that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_flag_function(
    run& _run
)
{
    const throwing_void thrower = { Kind, "flag function" };

    flag_function_case(_run,
                       thrower);

    return;
}

/**
 * @brief THROWN_OPTIONS 5: an each() function that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_each(
    run& _run
)
{
    const throwing_void thrower = { Kind, "each" };

    each_case(_run,
              thrower);

    return;
}

/**
 * @brief THROWN_OPTIONS 6: a validator's function that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_validator(
    run& _run
)
{
    const throwing_text thrower = { Kind, "validator" };

    _run.app().add_option("--opt",
                          "an option")
              ->check(thrower,
                      "THROWS");
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

/**
 * @brief THROWN_OPTIONS 7: a transformation's function that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_transform(
    run& _run
)
{
    const throwing_text thrower = { Kind, "transformation" };

    _run.app().add_option("--opt",
                          "an option")
              ->transform(thrower,
                          "THROWS");
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

/**
 * @brief THROWN_OPTIONS 8: a type's conversion that throws, the option
 *        given a value.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_conversion(
    run& _run
)
{
    static brittle<Kind> bound = { 0 };

    _run.app().add_option("--opt",
                          bound,
                          "an option");
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

/**
 * @brief THROWN_OPTIONS 9: a type's writer that throws, the default
 *        captured where the option is defined.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_writer(
    run& _run
)
{
    static brittle<Kind> bound = { 0 };

    _run.app().add_option("--opt",
                          bound,
                          "an option")
              ->capture_default_str();
    _run.parse(OPTION_ARGS,
               0u);

    return;
}

/**
 * @brief THROWN_OPTIONS 10: a type name's function that throws, help
 *        asking for it.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_type_name(
    run& _run
)
{
    const throwing_text thrower = { Kind, "type name" };

    _run.app().add_option("--opt",
                          "an option")
              ->type_name_fn(thrower);
    write_help(_run,
               "help");

    return;
}

/**
 * @brief THROWN_OPTIONS 11: a default's function that throws, the default
 *        captured where the option is defined.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_default_function(
    run& _run
)
{
    const throwing_text thrower = { Kind, "default" };

    _run.app().add_option("--opt",
                          "an option")
              ->default_function(thrower)
              ->capture_default_str();
    _run.parse(OPTION_ARGS,
               0u);

    return;
}

// THROWN_OPTIONS_STEPS
//   constant: the steps of d_tests_arg_cpp_boundary_thrown_options: each
// function throwing a std::runtime_error with words, one without,
// std::bad_alloc, then an exception of no standard type.
const step_fn THROWN_OPTIONS_STEPS[] =
{
    &thrown_option_callback<runtime_thrown>,
    &thrown_option_callback<empty_thrown>,
    &thrown_option_callback<memory_thrown>,
    &thrown_option_callback<foreign_thrown>,
    &thrown_option_function<runtime_thrown>,
    &thrown_option_function<empty_thrown>,
    &thrown_option_function<memory_thrown>,
    &thrown_option_function<foreign_thrown>,
    &thrown_flag_callback<runtime_thrown>,
    &thrown_flag_callback<empty_thrown>,
    &thrown_flag_callback<memory_thrown>,
    &thrown_flag_callback<foreign_thrown>,
    &thrown_flag_function<runtime_thrown>,
    &thrown_flag_function<empty_thrown>,
    &thrown_flag_function<memory_thrown>,
    &thrown_flag_function<foreign_thrown>,
    &thrown_each<runtime_thrown>,
    &thrown_each<empty_thrown>,
    &thrown_each<memory_thrown>,
    &thrown_each<foreign_thrown>,
    &thrown_validator<runtime_thrown>,
    &thrown_validator<empty_thrown>,
    &thrown_validator<memory_thrown>,
    &thrown_validator<foreign_thrown>,
    &thrown_transform<runtime_thrown>,
    &thrown_transform<empty_thrown>,
    &thrown_transform<memory_thrown>,
    &thrown_transform<foreign_thrown>,
    &thrown_conversion<runtime_thrown>,
    &thrown_conversion<empty_thrown>,
    &thrown_conversion<memory_thrown>,
    &thrown_conversion<foreign_thrown>,
    &thrown_writer<runtime_thrown>,
    &thrown_writer<empty_thrown>,
    &thrown_writer<memory_thrown>,
    &thrown_writer<foreign_thrown>,
    &thrown_type_name<runtime_thrown>,
    &thrown_type_name<empty_thrown>,
    &thrown_type_name<memory_thrown>,
    &thrown_type_name<foreign_thrown>,
    &thrown_default_function<runtime_thrown>,
    &thrown_default_function<empty_thrown>,
    &thrown_default_function<memory_thrown>,
    &thrown_default_function<foreign_thrown>
};

// THROWN_OPTIONS_COUNT
//   constant: how many steps THROWN_OPTIONS_STEPS holds.
const std::size_t THROWN_OPTIONS_COUNT =
    sizeof(THROWN_OPTIONS_STEPS) / sizeof(THROWN_OPTIONS_STEPS[0]);

// 2.3    A command's functions
//------------------------------------------------------------------------------

/**
 * @brief THROWN_COMMANDS 1: the root's callback that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_callback(
    run& _run
)
{
    const throwing_void thrower = { Kind, "callback" };

    callback_case(_run,
                  thrower);

    return;
}

/**
 * @brief THROWN_COMMANDS 2: the root's final callback that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_final_callback(
    run& _run
)
{
    const throwing_void thrower = { Kind, "final callback" };

    final_callback_case(_run,
                        thrower);

    return;
}

/**
 * @brief THROWN_COMMANDS 3: the root's parse-complete callback that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_parse_complete(
    run& _run
)
{
    const throwing_void thrower = { Kind, "parse complete callback" };

    parse_complete_case(_run,
                        thrower);

    return;
}

/**
 * @brief THROWN_COMMANDS 4: the root's pre-callback that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_pre_callback(
    run& _run
)
{
    const throwing_void thrower = { Kind, "pre-callback" };

    pre_callback_case(_run,
                      thrower);

    return;
}

/**
 * @brief THROWN_COMMANDS 5: the root's pre-parse callback that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_preparse(
    run& _run
)
{
    const throwing_void thrower = { Kind, "pre-parse callback" };

    preparse_case(_run,
                  thrower);

    return;
}

/**
 * @brief THROWN_COMMANDS 6: a failure message's function that throws, a
 *        parse that fails ended.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_failure_message(
    run& _run
)
{
    const throwing_text thrower = { Kind, "failure message" };

    _run.app().failure_message(thrower);
    _run.parse(OPTION_ARGS,
               2u);
    _run.exit();

    return;
}

/**
 * @brief THROWN_COMMANDS 7: a usage function that throws, help asking for
 *        it.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_usage(
    run& _run
)
{
    const throwing_text thrower = { Kind, "usage" };

    _run.app().usage(thrower);
    write_help(_run,
               "help");

    return;
}

/**
 * @brief THROWN_COMMANDS 8: a footer function that throws, help asking for
 *        it.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_footer(
    run& _run
)
{
    const throwing_text thrower = { Kind, "footer" };

    _run.app().footer(thrower);
    write_help(_run,
               "help");

    return;
}

/**
 * @brief THROWN_COMMANDS 9: a version function that throws, the version
 *        asked for and the parse ended.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_version(
    run& _run
)
{
    static const char* const ARGS[] =
    {
        "--version"
    };
    const throwing_text      thrower = { Kind, "version" };

    _run.app().set_version_flag("--version",
                                thrower);
    _run.parse(ARGS,
               1u);
    _run.exit();

    return;
}

// THROWN_COMMANDS_STEPS
//   constant: the steps of d_tests_arg_cpp_boundary_thrown_commands: each
// function throwing a std::runtime_error with words, one without,
// std::bad_alloc, then an exception of no standard type.
const step_fn THROWN_COMMANDS_STEPS[] =
{
    &thrown_callback<runtime_thrown>,
    &thrown_callback<empty_thrown>,
    &thrown_callback<memory_thrown>,
    &thrown_callback<foreign_thrown>,
    &thrown_final_callback<runtime_thrown>,
    &thrown_final_callback<empty_thrown>,
    &thrown_final_callback<memory_thrown>,
    &thrown_final_callback<foreign_thrown>,
    &thrown_parse_complete<runtime_thrown>,
    &thrown_parse_complete<empty_thrown>,
    &thrown_parse_complete<memory_thrown>,
    &thrown_parse_complete<foreign_thrown>,
    &thrown_pre_callback<runtime_thrown>,
    &thrown_pre_callback<empty_thrown>,
    &thrown_pre_callback<memory_thrown>,
    &thrown_pre_callback<foreign_thrown>,
    &thrown_preparse<runtime_thrown>,
    &thrown_preparse<empty_thrown>,
    &thrown_preparse<memory_thrown>,
    &thrown_preparse<foreign_thrown>,
    &thrown_failure_message<runtime_thrown>,
    &thrown_failure_message<empty_thrown>,
    &thrown_failure_message<memory_thrown>,
    &thrown_failure_message<foreign_thrown>,
    &thrown_usage<runtime_thrown>,
    &thrown_usage<empty_thrown>,
    &thrown_usage<memory_thrown>,
    &thrown_usage<foreign_thrown>,
    &thrown_footer<runtime_thrown>,
    &thrown_footer<empty_thrown>,
    &thrown_footer<memory_thrown>,
    &thrown_footer<foreign_thrown>,
    &thrown_version<runtime_thrown>,
    &thrown_version<empty_thrown>,
    &thrown_version<memory_thrown>,
    &thrown_version<foreign_thrown>
};

// THROWN_COMMANDS_COUNT
//   constant: how many steps THROWN_COMMANDS_STEPS holds.
const std::size_t THROWN_COMMANDS_COUNT =
    sizeof(THROWN_COMMANDS_STEPS) / sizeof(THROWN_COMMANDS_STEPS[0]);

// 2.4    Help's steps and sinks
//------------------------------------------------------------------------------

// throwing_formatter
//   class: a formatter whose description step throws.
class throwing_formatter : public cli::formatter
{
public:
    /**
     * @brief Makes the formatter.
     *
     * @param[in] _kind  what its step throws.
     */
    explicit throwing_formatter(
        thrown_kind _kind
    )
        : cli::formatter(),
          m_kind(_kind)
    {}

    /**
     * @brief Throws.
     *
     * @param[in]     _command  the command; unused.
     * @param[in,out] _out      the help being built; unused.
     * @return nothing: it throws.
     */
    virtual cli::status
    make_description(
        const cli::command& _command,
        cli::buffer&        _out
    ) const D_OVERRIDE
    {
        (void)_command;
        (void)_out;
        raise(m_kind,
              "help step");

        return cli::status();
    }

private:
    thrown_kind m_kind;
};

// throwing_buffer
//   class: a stream buffer whose writing throws.
class throwing_buffer : public std::streambuf
{
public:
    /**
     * @brief Makes the buffer.
     *
     * @param[in] _kind  what its writing throws.
     */
    explicit throwing_buffer(
        thrown_kind _kind
    )
        : std::streambuf(),
          m_kind(_kind)
    {}

protected:
    /**
     * @brief Throws, given a character.
     *
     * @param[in] _character  the character; unused.
     * @return nothing: it throws.
     */
    virtual int_type
    overflow(
        int_type _character
    ) D_OVERRIDE
    {
        (void)_character;
        raise(m_kind,
              "sink");

        return traits_type::eof();
    }

    /**
     * @brief Throws, given characters.
     *
     * @param[in] _data   the characters; unused.
     * @param[in] _count  how many; unused.
     * @return nothing: it throws.
     */
    virtual std::streamsize
    xsputn(
        const char*     _data,
        std::streamsize _count
    ) D_OVERRIDE
    {
        (void)_data;
        (void)_count;
        raise(m_kind,
              "sink");

        return 0;
    }

private:
    thrown_kind m_kind;
};

/**
 * @brief THROWN_TEXT 1: a help step that throws.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_help_step(
    run& _run
)
{
    static throwing_formatter thrower(Kind);

    _run.app().formatter(thrower);
    write_help(_run,
               "help");

    return;
}

/**
 * @brief THROWN_TEXT 2: a stream that throws what its buffer throws, help
 *        written to it.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_sink(
    run& _run
)
{
    throwing_buffer buffer(Kind);
    std::ostream    stream(&buffer);
    cli::status     written;

    stream.exceptions(std::ios::badbit);
    written = _run.app().help(cli::stream_sink(stream));
    _run.note(std::string("help: sink=") + written.get_name());

    return;
}

// THROWN_TEXT_STEPS
//   constant: the steps of d_tests_arg_cpp_boundary_thrown_text: each
// function throwing a std::runtime_error with words, one without,
// std::bad_alloc, then an exception of no standard type.
const step_fn THROWN_TEXT_STEPS[] =
{
    &thrown_help_step<runtime_thrown>,
    &thrown_help_step<empty_thrown>,
    &thrown_help_step<memory_thrown>,
    &thrown_help_step<foreign_thrown>,
    &thrown_sink<runtime_thrown>,
    &thrown_sink<empty_thrown>,
    &thrown_sink<memory_thrown>,
    &thrown_sink<foreign_thrown>
};

// THROWN_TEXT_COUNT
//   constant: how many steps THROWN_TEXT_STEPS holds.
const std::size_t THROWN_TEXT_COUNT =
    sizeof(THROWN_TEXT_STEPS) / sizeof(THROWN_TEXT_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 2.5    Filters
//------------------------------------------------------------------------------

// MEMBERS
//   constant: what a membership validator's value must be one of.
const char* const MEMBERS[] =
{
    "a", "b"
};

/**
 * @brief THROWN_FILTERS 1: a filter that throws, a membership validator
 *        filtering the value.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_member_filter(
    run& _run
)
{
    const throwing_text thrower = { Kind, "filter" };

    _run.app().add_option("--opt",
                          "an option")
              ->check(cli::is_member(MEMBERS,
                                     thrower));
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

/**
 * @brief THROWN_FILTERS 2: a filter that throws, a mapping transformer
 *        filtering the value.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_map_filter(
    run& _run
)
{
    const throwing_text                thrower = { Kind, "filter" };
    std::map<std::string, std::string> pairs;

    pairs["one"] = "1";
    pairs["two"] = "2";
    _run.app().add_option("--opt",
                          "an option")
              ->transform(cli::transformer(pairs,
                                           thrower));
    _run.parse(OPTION_ARGS,
               2u);

    return;
}

// THROWN_FILTERS_STEPS
//   constant: the steps of d_tests_arg_cpp_boundary_thrown_filters: each
// filter throwing a std::runtime_error with words, one without,
// std::bad_alloc, then an exception of no standard type.
const step_fn THROWN_FILTERS_STEPS[] =
{
    &thrown_member_filter<runtime_thrown>,
    &thrown_member_filter<empty_thrown>,
    &thrown_member_filter<memory_thrown>,
    &thrown_member_filter<foreign_thrown>,
    &thrown_map_filter<runtime_thrown>,
    &thrown_map_filter<empty_thrown>,
    &thrown_map_filter<memory_thrown>,
    &thrown_map_filter<foreign_thrown>
};

// THROWN_FILTERS_COUNT
//   constant: how many steps THROWN_FILTERS_STEPS holds.
const std::size_t THROWN_FILTERS_COUNT =
    sizeof(THROWN_FILTERS_STEPS) / sizeof(THROWN_FILTERS_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 2.6    A configuration format's operations
//------------------------------------------------------------------------------

// throwing_format
//   class: a configuration format one of whose operations throws.
class throwing_format : public cli::config_format
{
public:
    // which
    //   enum: the operation that throws.
    enum which
    {
        reading,
        writing,
        flagging
    };

    /**
     * @brief Makes the format.
     *
     * @param[in] _kind       what the operation throws.
     * @param[in] _operation  which operation throws.
     */
    throwing_format(
        thrown_kind _kind,
        which       _operation
    )
        : cli::config_format(),
          m_kind(_kind),
          m_operation(_operation)
    {}

    /**
     * @brief Reads, or throws.
     *
     * @param[in]     _input  the text.
     * @param[in,out] _out    the items read.
     * @param[in,out] _error  a failure's description.
     * @return the format's own status.
     */
    virtual cli::status
    from_config(
        const cli::text&   _input,
        cli::config_items& _out,
        cli::buffer&       _error
    ) const D_OVERRIDE
    {
        // the operation that throws
        if (m_operation == reading)
        {
            raise(m_kind,
                  "from_config");
        }

        return cli::config_format::from_config(_input,
                                               _out,
                                               _error);
    }

    /**
     * @brief Writes, or throws.
     *
     * @param[in]     _command            the command.
     * @param[in]     _mode               which values are written.
     * @param[in]     _write_description  whether descriptions are written.
     * @param[in]     _prefix             what precedes each name.
     * @param[in,out] _out                the text being built.
     * @return the format's own status.
     */
    virtual cli::status
    to_config(
        const cli::command&            _command,
        cli::config_output_mode::value _mode,
        bool                           _write_description,
        const cli::text&               _prefix,
        cli::buffer&                   _out
    ) const D_OVERRIDE
    {
        // the operation that throws
        if (m_operation == writing)
        {
            raise(m_kind,
                  "to_config");
        }

        return cli::config_format::to_config(_command,
                                             _mode,
                                             _write_description,
                                             _prefix,
                                             _out);
    }

    /**
     * @brief Reads a flag's text, or throws.
     *
     * @param[in]     _item  the item.
     * @param[in,out] _out   the text.
     * @return the format's own status.
     */
    virtual cli::status
    to_flag(
        const cli::config_entry& _item,
        cli::buffer&             _out
    ) const D_OVERRIDE
    {
        // the operation that throws
        if (m_operation == flagging)
        {
            raise(m_kind,
                  "to_flag");
        }

        return cli::config_format::to_flag(_item,
                                           _out);
    }

private:
    thrown_kind m_kind;
    which       m_operation;
};

/**
 * @brief The configuration a command writes, as a std::string, escaped and
 *        bracketed, or that it threw std::bad_alloc, as the string form
 *        does for memory run out.
 *
 * @param[in,out] _run  the run.
 * @return the text.
 */
std::string
config_string(
    run& _run
)
{
    try
    {
        return "[" + escape(_run.app().config_to_str()) + "]";
    }
    catch (const std::bad_alloc&)
    {
        return "THROWN std::bad_alloc";
    }
}

/**
 * @brief THROWN_CONFIG 1: reading text that throws, a parse reading it.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_config_read(
    run& _run
)
{
    static throwing_format thrower(Kind,
                                   throwing_format::reading);
    std::istringstream     input("verbose = true\n");

    (void)_run.app().add_flag("--verbose",
                              "a flag");
    _run.app().config_formatter(thrower);
    _run.parse_stream(input);

    return;
}

/**
 * @brief THROWN_CONFIG 2: reading a flag's text that throws, a parse
 *        reading the flag.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_config_flag(
    run& _run
)
{
    static throwing_format thrower(Kind,
                                   throwing_format::flagging);
    std::istringstream     input("verbose = true\n");

    (void)_run.app().add_flag("--verbose",
                              "a flag");
    _run.app().config_formatter(thrower);
    _run.parse_stream(input);

    return;
}

/**
 * @brief THROWN_CONFIG 3: writing that throws, the configuration written.
 *
 * @param[in,out] _run  the step's run.
 */
template<thrown_kind Kind>
void
thrown_config_write(
    run& _run
)
{
    static throwing_format thrower(Kind,
                                   throwing_format::writing);

    (void)_run.app().add_flag("--verbose",
                              "a flag");
    _run.app().config_formatter(thrower);
    _run.note("written " + config_string(_run));

    return;
}

// THROWN_CONFIG_STEPS
//   constant: the steps of d_tests_arg_cpp_boundary_thrown_config: each
// operation throwing a std::runtime_error with words, one without,
// std::bad_alloc, then an exception of no standard type.
const step_fn THROWN_CONFIG_STEPS[] =
{
    &thrown_config_read<runtime_thrown>,
    &thrown_config_read<empty_thrown>,
    &thrown_config_read<memory_thrown>,
    &thrown_config_read<foreign_thrown>,
    &thrown_config_flag<runtime_thrown>,
    &thrown_config_flag<empty_thrown>,
    &thrown_config_flag<memory_thrown>,
    &thrown_config_flag<foreign_thrown>,
    &thrown_config_write<runtime_thrown>,
    &thrown_config_write<empty_thrown>,
    &thrown_config_write<memory_thrown>,
    &thrown_config_write<foreign_thrown>
};

// THROWN_CONFIG_COUNT
//   constant: how many steps THROWN_CONFIG_STEPS holds.
const std::size_t THROWN_CONFIG_COUNT =
    sizeof(THROWN_CONFIG_STEPS) / sizeof(THROWN_CONFIG_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#endif  // D_INTERNAL_ARG_PARSER_CPP_CATCH

}  // namespace

//==============================================================================
// 3.  RESULTS
//==============================================================================

/*
d_tests_arg_cpp_boundary_results
  Each function that may return more than one kind of result, with each.
  Tests the following:
  - an option's callback returning nothing; ok; a status and a status_code
    that fail bare, the parse's message theirs; a status after the option's
    fail(), its message kept; true; false, CLI11's "Could not convert"
  - add_option_function's, add_flag_callback's and add_flag_function's
    functions, nothing and a status
  - each(): nothing; validation_error bare, rejecting the value; with the
    option's fail() and a message, rejecting it with that; another status,
    ending the parse
  - the root's callback: nothing, ok, a status and a status_code, and the
    root's fail() with a message; its final, parse-complete, pre- and
    pre-parse callbacks, nothing and a status
*/
bool
d_tests_arg_cpp_boundary_results()
{
    return steps_hold("RESULTS",
                      RESULTS_STEPS,
                      RESULTS_COUNT,
                      RESULTS);
}

#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)

//==============================================================================
// 4.  EXCEPTIONS
//==============================================================================

/*
d_tests_arg_cpp_boundary_thrown_options
  An option's functions and a type's, throwing (ISSUES 47).
  Tests the following:
  - an option's callback, add_option_function's, add_flag_callback's and
    add_flag_function's functions: conversion_error, what() the message
  - each(): validation_error, what() the message
  - a validator's and a transformation's functions: the validation stopped,
    what() the reason
  - a type's conversion: conversion_error; its writer, a type name's and a
    default's functions: conversion_error without a message
  - std::bad_alloc from each: out_of_memory; an exception of no standard
    type: the row's status without a message
*/
bool
d_tests_arg_cpp_boundary_thrown_options()
{
    return steps_hold("THROWN_OPTIONS",
                      THROWN_OPTIONS_STEPS,
                      THROWN_OPTIONS_COUNT,
                      THROWN_OPTIONS);
}

/*
d_tests_arg_cpp_boundary_thrown_commands
  A command's functions, throwing (ISSUES 47).
  Tests the following:
  - the root's callback, final, parse-complete, pre- and pre-parse
    callbacks: parse_error, what() the message
  - a failure message's, a usage, a footer and a version function:
    conversion_error without a message
  - std::bad_alloc from each: out_of_memory; an exception of no standard
    type: the row's status without a message
*/
bool
d_tests_arg_cpp_boundary_thrown_commands()
{
    return steps_hold("THROWN_COMMANDS",
                      THROWN_COMMANDS_STEPS,
                      THROWN_COMMANDS_COUNT,
                      THROWN_COMMANDS);
}

/*
d_tests_arg_cpp_boundary_thrown_text
  Help's steps and sinks, throwing (ISSUES 47).
  Tests the following:
  - a help step: conversion_error without a message, nothing written, the
    string empty
  - a sink: it takes nothing, which its writer reports
  - std::bad_alloc from each: out_of_memory, the string form throwing
    std::bad_alloc; an exception of no standard type: as std::runtime_error
*/
bool
d_tests_arg_cpp_boundary_thrown_text()
{
    return steps_hold("THROWN_TEXT",
                      THROWN_TEXT_STEPS,
                      THROWN_TEXT_COUNT,
                      THROWN_TEXT);
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/*
d_tests_arg_cpp_boundary_thrown_filters
  Filters, throwing (ISSUES 47).
  Tests the following:
  - a membership validator's and a mapping transformer's filter:
    conversion_error without a message
  - std::bad_alloc from each: out_of_memory; an exception of no standard
    type: as std::runtime_error
*/
bool
d_tests_arg_cpp_boundary_thrown_filters()
{
    return steps_hold("THROWN_FILTERS",
                      THROWN_FILTERS_STEPS,
                      THROWN_FILTERS_COUNT,
                      THROWN_FILTERS);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/*
d_tests_arg_cpp_boundary_thrown_config
  A configuration format's operations, throwing (ISSUES 47).
  Tests the following:
  - reading text, reading a flag's text and writing: conversion_error
    without a message
  - std::bad_alloc from each: out_of_memory, the string form throwing
    std::bad_alloc; an exception of no standard type: as
    std::runtime_error
*/
bool
d_tests_arg_cpp_boundary_thrown_config()
{
    return steps_hold("THROWN_CONFIG",
                      THROWN_CONFIG_STEPS,
                      THROWN_CONFIG_COUNT,
                      THROWN_CONFIG);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#endif  // D_INTERNAL_ARG_PARSER_CPP_CATCH

//==============================================================================
// 5.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_boundary
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_boundary[] =
{
    { "cpp_boundary_results",
      d_tests_arg_cpp_boundary_results },
#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)
    { "cpp_boundary_thrown_options",
      d_tests_arg_cpp_boundary_thrown_options },
    { "cpp_boundary_thrown_commands",
      d_tests_arg_cpp_boundary_thrown_commands },
    { "cpp_boundary_thrown_text",
      d_tests_arg_cpp_boundary_thrown_text },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_boundary_thrown_filters",
      d_tests_arg_cpp_boundary_thrown_filters },
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "cpp_boundary_thrown_config",
      d_tests_arg_cpp_boundary_thrown_config },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#endif  // D_INTERNAL_ARG_PARSER_CPP_CATCH
};

// d_tests_arg_cpp_boundary_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_boundary_count =
    sizeof(d_tests_arg_cpp_boundary) / sizeof(d_tests_arg_cpp_boundary[0]);
