/*******************************************************************************
* djinterp [parse]                                          arg_common_tests.cpp
*
* The argument parser's C++ common suite's sections.
*   The one section held to CLI11 writes what the CASES section defines:
* CLI11's exit codes, under the face's names for them. tools/cppref/
* cppref.py compiles that section against CLI11 2.7.2, through the face's
* names for CLI11's (cli11_shim.hpp), and the transcript below is what CLI11
* wrote. The other sections are the module's own, written apart, after it:
* CLI11 has no status, text, parameter, report view, list view, buffer or
* sink of the face's kind to write a transcript of.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_common_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_common_tests.hpp"  // corresponding header
// std
#include <cstddef>   // std::size_t
#include <sstream>   // std::ostringstream
#include <string>    // std::string
#include <vector>    // std::vector
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
#include <string_view>  // std::string_view
#endif
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
using cli::testing::number;
using cli::testing::steps_hold;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// EXIT_CODES
//   constant: what the steps of d_tests_arg_cpp_common_exit_codes write, as
// CLI11 writes them.
const char EXIT_CODES[] =
    "success=0 incorrect_construction=100 bad_name_string=101 "
    "option_already_added=102 file_error=103 conversion_error=104 "
    "validation_error=105 required_error=106 requires_error=107 "
    "excludes_error=108 extras_error=109 config_error=110 invalid_error=111 "
    "horrible_error=112 option_not_found=113 argument_mismatch=114 "
    "base_class=127\n";

// VOCABULARY
//   constant: what the steps of d_tests_arg_cpp_common_vocabulary write: the
// module's own, as CLI11's enumerations are enum classes of values of their
// own, where each of the face's is the C library's.
const char VOCABULARY[] =
    "93 enumerators, 0 not their C values\n";

// STATUSES
//   constant: what the steps of d_tests_arg_cpp_common_statuses write: the
// module's own, as CLI11 throws where the face returns a status.
const char STATUSES[] =
    "OK: code=0 exit=0 ok=1 error=0\n"
    "CallForHelp: code=1 exit=0 ok=0 error=0\n"
    "CallForAllHelp: code=2 exit=0 ok=0 error=0\n"
    "CallForVersion: code=3 exit=0 ok=0 error=0\n"
    "CallForCompletion: code=4 exit=0 ok=0 error=0\n"
    "Success: code=5 exit=0 ok=0 error=0\n"
    "RuntimeError: code=6 exit=1 ok=0 error=1\n"
    "IncorrectConstruction: code=7 exit=100 ok=0 error=1\n"
    "BadNameString: code=8 exit=101 ok=0 error=1\n"
    "OptionAlreadyAdded: code=9 exit=102 ok=0 error=1\n"
    "FileError: code=10 exit=103 ok=0 error=1\n"
    "ConversionError: code=11 exit=104 ok=0 error=1\n"
    "ValidationError: code=12 exit=105 ok=0 error=1\n"
    "RequiredError: code=13 exit=106 ok=0 error=1\n"
    "RequiresError: code=14 exit=107 ok=0 error=1\n"
    "ExcludesError: code=15 exit=108 ok=0 error=1\n"
    "ExtrasError: code=16 exit=109 ok=0 error=1\n"
    "ConfigError: code=17 exit=110 ok=0 error=1\n"
    "InvalidError: code=18 exit=111 ok=0 error=1\n"
    "HorribleError: code=19 exit=112 ok=0 error=1\n"
    "OptionNotFound: code=20 exit=113 ok=0 error=1\n"
    "ArgumentMismatch: code=21 exit=114 ok=0 error=1\n"
    "ParseError: code=22 exit=111 ok=0 error=1\n"
    "OutOfMemory: code=23 exit=127 ok=0 error=1\n"
    "BadArgument: code=24 exit=127 ok=0 error=1\n"
    "Unsupported: code=25 exit=127 ok=0 error=1\n"
    "Error: code=-1 exit=127 ok=0 error=1\n"
    "Error: code=26 exit=127 ok=0 error=1\n"
    "made=OK coded==raw=1 coded!=made=1 coded==code=1 code==coded=1 "
    "code!=made=1 made==ok=1\n";

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// STATUSES_CPP11
//   constant: what the steps of d_tests_arg_cpp_common_statuses_cpp11 write:
// the module's own, as above.
const char STATUSES_CPP11[] =
    "ok=1 help=0 failed=0 not_failed=1 condition\n";

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// TEXT
//   constant: what the steps of d_tests_arg_cpp_common_text write: the
// module's own, as CLI11 copies text into std::string, where the face gives
// a view.
const char TEXT[] =
    "empty: size=0 length=0 empty=1 [] [] [] [] null=1\n"
    "C string: size=3 length=3 empty=0 [abc] [abc] [abc] [abc] null=0\n"
    "NULL: size=0 length=0 empty=1 [] [] [] [] null=1\n"
    "span: size=3 length=3 empty=0 [a\\x00b] [a\\x00b] [a\\x00b] [a\\x00b] "
    "null=0\n"
    "std::string: size=3 length=3 empty=0 [x\\x00y] [x\\x00y] [x\\x00y] "
    "[x\\x00y] null=0\n"
    "C span: size=4 length=4 empty=0 [span] [span] [span] [span] null=0\n"
    "[abc] vs [abc]: compare=0 ==1 !=0 <0 <=1 >0 >=1\n"
    "[ab] vs [abc]: compare=-1 ==0 !=1 <1 <=1 >0 >=0\n"
    "[abd] vs [abc]: compare=1 ==0 !=1 <0 <=0 >1 >=1\n"
    "[a\\x00b] vs [a]: compare=1 ==0 !=1 <0 <=0 >1 >=1\n"
    "[] vs [a]: compare=-1 ==0 !=1 <1 <=1 >0 >=0\n"
    "with C strings: 111 with std::strings: 111\n"
    "streamed: [<a\\x00b>] size=5\n";

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// TEXT_CPP17
//   constant: what the steps of d_tests_arg_cpp_common_text_cpp17 write: the
// module's own, as above.
const char TEXT_CPP17[] =
    "from a view: size=3 length=3 empty=0 [v\\x00w] [v\\x00w] [v\\x00w] "
    "[v\\x00w] null=0\n"
    "back: size=3 same=1\n";

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

// C_STRING
//   constant: what the steps of d_tests_arg_cpp_common_c_string write: the
// module's own, as CLI11's parameters are std::string.
const char C_STRING[] =
    "default null=1 given=[given] NULL null=1 kept=[held] same=1\n";

// ERRORS
//   constant: what the steps of d_tests_arg_cpp_common_errors write: the
// module's own, as CLI11's Error is an exception object, not a view of a
// report the parser keeps.
const char ERRORS[] =
    "nothing: status=OK name=OK exit=0 what=[]\n"
    "message: status=ConversionError name=ConversionError exit=104 what=[a "
    "value did not convert]\n"
    "no message: status=RequiredError name=RequiredError exit=106 "
    "what=[Required error]\n"
    "help: status=CallForHelp name=CallForHelp exit=0 what=[This should be "
    "caught in your main function, see examples]\n"
    "same report=1\n";

// LISTS
//   constant: what the steps of d_tests_arg_cpp_common_lists write: the
// module's own, as CLI11's lists are std::vector and std::set copies, not
// views of the C library's arrays.
const char LISTS[] =
    "spans: size=3 empty=0 indexed=[a][bc][] forward=[a][bc][] "
    "backward=[][bc][a] copied=3\n"
    "spans: front=[a] back=[]\n"
    "names: size=2 empty=0 indexed=[x][y] forward=[x][y] backward=[y][x] "
    "copied=2\n"
    "names: front=[x] back=[y]\n"
    "none: size=0 empty=1 indexed= forward= backward= copied=0\n"
    "data same=1\n"
    "begin=p +=2:r -=1:q +2:s -1:p [1]:r post++:q,r post--:r,q end-begin=4 <1 "
    "==1 !=1\n"
    "handles: size=2 [1]=--second forward=[--first][--second] copied=2 "
    "same=1\n";

// BUFFERS
//   constant: what the steps of d_tests_arg_cpp_common_buffers write: the
// module's own, as CLI11 returns help as a std::string, where the face lets
// a step append to the text being built.
const char BUFFERS[] =
    "new: size=0 empty=1 view=[] str=[]\n"
    "appended: size=4 empty=0 view=[abcd] str=[abcd]\n"
    "cut: size=2 empty=0 view=[ab] str=[ab]\n"
    "refused: size=2 empty=0 view=[ab] str=[ab]\n"
    "statuses: OK OK OutOfMemory same buffer=1\n";

// SINKS
//   constant: what the steps of d_tests_arg_cpp_common_sinks write: the
// module's own, as CLI11 writes to a std::ostream it is given, not through a
// C sink.
const char SINKS[] =
    "text=[<abc>] stream=[xy] statuses: OK OK OK FileError\n";

//==============================================================================
// 1.  CASES
//==============================================================================
// What the section held to CLI11 defines, in the part of the face CLI11
// shares; tools/cppref/cppref.py compiles this section against CLI11.

/**
 * @brief EXIT_CODES 1: each exit code, under the face's name of CLI11's.
 *
 * @param[in,out] _run  the step's run.
 */
void
exit_codes_each(
    run& _run
)
{
    std::ostringstream line;

    line << "success=" << cli::exit_code::success
         << " incorrect_construction="
         << cli::exit_code::incorrect_construction
         << " bad_name_string=" << cli::exit_code::bad_name_string
         << " option_already_added=" << cli::exit_code::option_already_added
         << " file_error=" << cli::exit_code::file_error
         << " conversion_error=" << cli::exit_code::conversion_error
         << " validation_error=" << cli::exit_code::validation_error
         << " required_error=" << cli::exit_code::required_error
         << " requires_error=" << cli::exit_code::requires_error
         << " excludes_error=" << cli::exit_code::excludes_error
         << " extras_error=" << cli::exit_code::extras_error
         << " config_error=" << cli::exit_code::config_error
         << " invalid_error=" << cli::exit_code::invalid_error
         << " horrible_error=" << cli::exit_code::horrible_error
         << " option_not_found=" << cli::exit_code::option_not_found
         << " argument_mismatch=" << cli::exit_code::argument_mismatch
         << " base_class=" << cli::exit_code::base_class;
    _run.note(line.str());

    return;
}

// EXIT_CODES_STEPS
//   constant: the steps of d_tests_arg_cpp_common_exit_codes.
const step_fn EXIT_CODES_STEPS[] =
{
    &exit_codes_each
};

// EXIT_CODES_COUNT
//   constant: how many steps EXIT_CODES_STEPS holds.
const std::size_t EXIT_CODES_COUNT =
    sizeof(EXIT_CODES_STEPS) / sizeof(EXIT_CODES_STEPS[0]);

}  // namespace

//==============================================================================
// 2.  THE MODULE'S OWN
//==============================================================================
// What CLI11 cannot write: the face's vocabulary as the C library's values,
// and its values -- statuses, text, parameters, report and list views,
// buffers and sinks. Each is a step written here, through the whole face.

namespace
{

// 2.1    Vocabulary
//------------------------------------------------------------------------------

// vocabulary_row
//   struct: one enumerator of the face: its name, its value, and the C
// library's value it stands for.
struct vocabulary_row
{
    const char* m_name;
    int         m_face;
    int         m_c;
};

// VOCABULARY_ROWS
//   constant: every enumerator of the face's enumerations beside the C
// value it is defined as.
const vocabulary_row VOCABULARY_ROWS[] =
{
    { "status_code::ok", cli::status_code::ok, D_ARG_OK },
    { "status_code::call_for_help",
      cli::status_code::call_for_help, D_ARG_CALL_FOR_HELP },
    { "status_code::call_for_all_help",
      cli::status_code::call_for_all_help, D_ARG_CALL_FOR_ALL_HELP },
    { "status_code::call_for_version",
      cli::status_code::call_for_version, D_ARG_CALL_FOR_VERSION },
    { "status_code::call_for_completion",
      cli::status_code::call_for_completion, D_ARG_CALL_FOR_COMPLETION },
    { "status_code::success", cli::status_code::success, D_ARG_SUCCESS },
    { "status_code::runtime_error",
      cli::status_code::runtime_error, D_ARG_RUNTIME_ERROR },
    { "status_code::incorrect_construction",
      cli::status_code::incorrect_construction,
      D_ARG_ERROR_INCORRECT_CONSTRUCTION },
    { "status_code::bad_name_string",
      cli::status_code::bad_name_string, D_ARG_ERROR_BAD_NAME },
    { "status_code::option_already_added",
      cli::status_code::option_already_added,
      D_ARG_ERROR_OPTION_ALREADY_ADDED },
    { "status_code::file_error",
      cli::status_code::file_error, D_ARG_ERROR_FILE },
    { "status_code::conversion_error",
      cli::status_code::conversion_error, D_ARG_ERROR_CONVERSION },
    { "status_code::validation_error",
      cli::status_code::validation_error, D_ARG_ERROR_VALIDATION },
    { "status_code::required_error",
      cli::status_code::required_error, D_ARG_ERROR_REQUIRED },
    { "status_code::requires_error",
      cli::status_code::requires_error, D_ARG_ERROR_REQUIRES },
    { "status_code::excludes_error",
      cli::status_code::excludes_error, D_ARG_ERROR_EXCLUDES },
    { "status_code::extras_error",
      cli::status_code::extras_error, D_ARG_ERROR_EXTRAS },
    { "status_code::config_error",
      cli::status_code::config_error, D_ARG_ERROR_CONFIG },
    { "status_code::invalid_error",
      cli::status_code::invalid_error, D_ARG_ERROR_INVALID },
    { "status_code::horrible_error",
      cli::status_code::horrible_error, D_ARG_ERROR_HORRIBLE },
    { "status_code::option_not_found",
      cli::status_code::option_not_found, D_ARG_ERROR_OPTION_NOT_FOUND },
    { "status_code::argument_mismatch",
      cli::status_code::argument_mismatch, D_ARG_ERROR_ARGUMENT_MISMATCH },
    { "status_code::parse_error",
      cli::status_code::parse_error, D_ARG_ERROR_PARSE },
    { "status_code::out_of_memory",
      cli::status_code::out_of_memory, D_ARG_ERROR_OUT_OF_MEMORY },
    { "status_code::bad_argument",
      cli::status_code::bad_argument, D_ARG_ERROR_BAD_ARGUMENT },
    { "status_code::unsupported",
      cli::status_code::unsupported, D_ARG_ERROR_UNSUPPORTED },
    { "exit_code::success", cli::exit_code::success, D_ARG_EXIT_SUCCESS },
    { "exit_code::incorrect_construction",
      cli::exit_code::incorrect_construction,
      D_ARG_EXIT_INCORRECT_CONSTRUCTION },
    { "exit_code::bad_name_string",
      cli::exit_code::bad_name_string, D_ARG_EXIT_BAD_NAME_STRING },
    { "exit_code::option_already_added",
      cli::exit_code::option_already_added,
      D_ARG_EXIT_OPTION_ALREADY_ADDED },
    { "exit_code::file_error",
      cli::exit_code::file_error, D_ARG_EXIT_FILE_ERROR },
    { "exit_code::conversion_error",
      cli::exit_code::conversion_error, D_ARG_EXIT_CONVERSION_ERROR },
    { "exit_code::validation_error",
      cli::exit_code::validation_error, D_ARG_EXIT_VALIDATION_ERROR },
    { "exit_code::required_error",
      cli::exit_code::required_error, D_ARG_EXIT_REQUIRED_ERROR },
    { "exit_code::requires_error",
      cli::exit_code::requires_error, D_ARG_EXIT_REQUIRES_ERROR },
    { "exit_code::excludes_error",
      cli::exit_code::excludes_error, D_ARG_EXIT_EXCLUDES_ERROR },
    { "exit_code::extras_error",
      cli::exit_code::extras_error, D_ARG_EXIT_EXTRAS_ERROR },
    { "exit_code::config_error",
      cli::exit_code::config_error, D_ARG_EXIT_CONFIG_ERROR },
    { "exit_code::invalid_error",
      cli::exit_code::invalid_error, D_ARG_EXIT_INVALID_ERROR },
    { "exit_code::horrible_error",
      cli::exit_code::horrible_error, D_ARG_EXIT_HORRIBLE_ERROR },
    { "exit_code::option_not_found",
      cli::exit_code::option_not_found, D_ARG_EXIT_OPTION_NOT_FOUND },
    { "exit_code::argument_mismatch",
      cli::exit_code::argument_mismatch, D_ARG_EXIT_ARGUMENT_MISMATCH },
    { "exit_code::base_class",
      cli::exit_code::base_class, D_ARG_EXIT_BASE_CLASS },
    { "multi_option_policy::throw_error",
      cli::multi_option_policy::throw_error, D_ARG_MULTI_THROW },
    { "multi_option_policy::take_last",
      cli::multi_option_policy::take_last, D_ARG_MULTI_TAKE_LAST },
    { "multi_option_policy::take_first",
      cli::multi_option_policy::take_first, D_ARG_MULTI_TAKE_FIRST },
    { "multi_option_policy::join",
      cli::multi_option_policy::join, D_ARG_MULTI_JOIN },
    { "multi_option_policy::take_all",
      cli::multi_option_policy::take_all, D_ARG_MULTI_TAKE_ALL },
    { "multi_option_policy::sum",
      cli::multi_option_policy::sum, D_ARG_MULTI_SUM },
    { "multi_option_policy::reverse",
      cli::multi_option_policy::reverse, D_ARG_MULTI_REVERSE },
    { "callback_priority::first_pre_help",
      cli::callback_priority::first_pre_help,
      D_ARG_PRIORITY_FIRST_PRE_HELP },
    { "callback_priority::first",
      cli::callback_priority::first, D_ARG_PRIORITY_FIRST },
    { "callback_priority::pre_requirements_pre_help",
      cli::callback_priority::pre_requirements_pre_help,
      D_ARG_PRIORITY_PRE_REQUIREMENTS_PRE_HELP },
    { "callback_priority::pre_requirements",
      cli::callback_priority::pre_requirements,
      D_ARG_PRIORITY_PRE_REQUIREMENTS },
    { "callback_priority::normal_pre_help",
      cli::callback_priority::normal_pre_help,
      D_ARG_PRIORITY_NORMAL_PRE_HELP },
    { "callback_priority::normal",
      cli::callback_priority::normal, D_ARG_PRIORITY_NORMAL },
    { "callback_priority::last_pre_help",
      cli::callback_priority::last_pre_help, D_ARG_PRIORITY_LAST_PRE_HELP },
    { "callback_priority::last",
      cli::callback_priority::last, D_ARG_PRIORITY_LAST },
    { "integer_rule::decimal",
      cli::integer_rule::decimal, D_ARG_INTEGER_DECIMAL },
    { "integer_rule::c", cli::integer_rule::c, D_ARG_INTEGER_C },
    { "extras_mode::error", cli::extras_mode::error, D_ARG_EXTRAS_ERROR },
    { "extras_mode::error_immediately",
      cli::extras_mode::error_immediately, D_ARG_EXTRAS_ERROR_IMMEDIATELY },
    { "extras_mode::ignore", cli::extras_mode::ignore, D_ARG_EXTRAS_IGNORE },
    { "extras_mode::assume_single",
      cli::extras_mode::assume_single, D_ARG_EXTRAS_ASSUME_SINGLE },
    { "extras_mode::assume_multiple",
      cli::extras_mode::assume_multiple, D_ARG_EXTRAS_ASSUME_MULTIPLE },
    { "extras_mode::capture",
      cli::extras_mode::capture, D_ARG_EXTRAS_CAPTURE },
    { "config_extras_mode::error",
      cli::config_extras_mode::error, D_ARG_CONFIG_EXTRAS_ERROR },
    { "config_extras_mode::ignore",
      cli::config_extras_mode::ignore, D_ARG_CONFIG_EXTRAS_IGNORE },
    { "config_extras_mode::ignore_all",
      cli::config_extras_mode::ignore_all, D_ARG_CONFIG_EXTRAS_IGNORE_ALL },
    { "config_extras_mode::capture",
      cli::config_extras_mode::capture, D_ARG_CONFIG_EXTRAS_CAPTURE },
    { "prefix_mode::off", cli::prefix_mode::off, D_ARG_PREFIX_OFF },
    { "prefix_mode::separator_only",
      cli::prefix_mode::separator_only, D_ARG_PREFIX_SEPARATOR_ONLY },
    { "prefix_mode::on", cli::prefix_mode::on, D_ARG_PREFIX_ON },
    { "prefix_mode::positional_only",
      cli::prefix_mode::positional_only, D_ARG_PREFIX_POSITIONAL_ONLY },
    { "help_mode::normal", cli::help_mode::normal, D_ARG_HELP_NORMAL },
    { "help_mode::all", cli::help_mode::all, D_ARG_HELP_ALL },
    { "help_mode::sub", cli::help_mode::sub, D_ARG_HELP_SUB },
    { "config_output_mode::active",
      cli::config_output_mode::active, D_ARG_CONFIG_OUTPUT_ACTIVE },
    { "config_output_mode::all_defaults",
      cli::config_output_mode::all_defaults,
      D_ARG_CONFIG_OUTPUT_ALL_DEFAULTS },
    { "config_output_mode::active_subcommand_defaults",
      cli::config_output_mode::active_subcommand_defaults,
      D_ARG_CONFIG_OUTPUT_ACTIVE_SUBCOMMAND_DEFAULTS },
    { "classifier::none", cli::classifier::none, D_ARG_CLASS_NONE },
    { "classifier::positional_mark",
      cli::classifier::positional_mark, D_ARG_CLASS_POSITIONAL_MARK },
    { "classifier::short_name",
      cli::classifier::short_name, D_ARG_CLASS_SHORT },
    { "classifier::long_name", cli::classifier::long_name, D_ARG_CLASS_LONG },
    { "classifier::windows_style",
      cli::classifier::windows_style, D_ARG_CLASS_WINDOWS_STYLE },
    { "classifier::subcommand",
      cli::classifier::subcommand, D_ARG_CLASS_SUBCOMMAND },
    { "classifier::subcommand_terminator",
      cli::classifier::subcommand_terminator,
      D_ARG_CLASS_SUBCOMMAND_TERMINATOR },
    { "quoting::posix", cli::quoting::posix, D_ARG_QUOTING_POSIX },
    { "quoting::windows", cli::quoting::windows, D_ARG_QUOTING_WINDOWS },
    { "quoting::cli11", cli::quoting::cli11, D_ARG_QUOTING_CLI11 },
    { "quoting::gnu", cli::quoting::gnu, D_ARG_QUOTING_GNU },
    { "quoting::native", cli::quoting::native, D_ARG_QUOTING_NATIVE },
    { "quoting::native_response",
      cli::quoting::native_response, D_ARG_QUOTING_NATIVE_RESPONSE }
};

// VOCABULARY_ROW_COUNT
//   constant: how many rows VOCABULARY_ROWS holds.
const std::size_t VOCABULARY_ROW_COUNT =
    sizeof(VOCABULARY_ROWS) / sizeof(VOCABULARY_ROWS[0]);

/**
 * @brief VOCABULARY 1: each enumerator of the face against the C value it
 *        stands for: a line for each that differs, then the count.
 *
 * @param[in,out] _run  the step's run.
 */
void
vocabulary_values(
    run& _run
)
{
    std::size_t differing = 0u;

    // each enumerator against its C value
    for (std::size_t index = 0u; index < VOCABULARY_ROW_COUNT; ++index)
    {
        const vocabulary_row& row = VOCABULARY_ROWS[index];

        // one that is not its C value
        if (row.m_face != row.m_c)
        {
            _run.note(std::string(row.m_name) + "=" +
                      number(static_cast<long>(row.m_face)) + " c=" +
                      number(static_cast<long>(row.m_c)));
            ++differing;
        }
    }

    _run.note(number(static_cast<unsigned long>(VOCABULARY_ROW_COUNT)) +
              " enumerators, " +
              number(static_cast<unsigned long>(differing)) +
              " not their C values");

    return;
}

// VOCABULARY_STEPS
//   constant: the steps of d_tests_arg_cpp_common_vocabulary.
const step_fn VOCABULARY_STEPS[] =
{
    &vocabulary_values
};

// VOCABULARY_COUNT
//   constant: how many steps VOCABULARY_STEPS holds.
const std::size_t VOCABULARY_COUNT =
    sizeof(VOCABULARY_STEPS) / sizeof(VOCABULARY_STEPS[0]);

// 2.2    Statuses
//------------------------------------------------------------------------------

/**
 * @brief Writes what a status reads on one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _status  the status.
 */
void
write_status(
    run&               _run,
    const cli::status& _status
)
{
    std::ostringstream line;

    line << _status.get_name()
         << ": code=" << _status.code()
         << " exit=" << _status.get_exit_code()
         << " ok=" << _status.ok()
         << " error=" << _status.is_error();
    _run.note(line.str());

    return;
}

/**
 * @brief STATUSES 1: every status, made from its code: its name, its exit
 *        code, and whether it is ok and an error.
 *
 * @param[in,out] _run  the step's run.
 */
void
statuses_each(
    run& _run
)
{
    // each status, from its C value
    for (int code = 0; code < D_ARG_STATUS_COUNT; ++code)
    {
        write_status(_run,
                     cli::status(code));
    }

    write_status(_run,
                 cli::status(-1));
    write_status(_run,
                 cli::status(D_ARG_STATUS_COUNT));

    return;
}

/**
 * @brief STATUSES 2: statuses made by default, from a status_code and from
 *        an int, and compared, a status_code on either side.
 *
 * @param[in,out] _run  the step's run.
 */
void
statuses_compared(
    run& _run
)
{
    const cli::status  made;
    const cli::status  coded = cli::status_code::validation_error;
    const cli::status  raw(D_ARG_ERROR_VALIDATION);
    std::ostringstream line;

    line << "made=" << made.get_name()
         << " coded==raw=" << (coded == raw)
         << " coded!=made=" << (coded != made)
         << " coded==code=" << (coded == cli::status_code::validation_error)
         << " code==coded=" << (cli::status_code::validation_error == coded)
         << " code!=made=" << (cli::status_code::conversion_error != made)
         << " made==ok=" << (made == cli::status_code::ok);
    _run.note(line.str());

    return;
}

// STATUSES_STEPS
//   constant: the steps of d_tests_arg_cpp_common_statuses.
const step_fn STATUSES_STEPS[] =
{
    &statuses_each,
    &statuses_compared
};

// STATUSES_COUNT
//   constant: how many steps STATUSES_STEPS holds.
const std::size_t STATUSES_COUNT =
    sizeof(STATUSES_STEPS) / sizeof(STATUSES_STEPS[0]);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief STATUSES_CPP11 1: a status's explicit truth, its ok(): true for
 *        ok alone, false for a completed request and for an error.
 *
 * @param[in,out] _run  the step's run.
 */
void
statuses_truth(
    run& _run
)
{
    const cli::status  ok;
    const cli::status  help = cli::status_code::call_for_help;
    const cli::status  failed = cli::status_code::conversion_error;
    std::ostringstream line;

    line << "ok=" << static_cast<bool>(ok)
         << " help=" << static_cast<bool>(help)
         << " failed=" << static_cast<bool>(failed)
         << " not_failed=" << !failed;

    // a status in a condition reads as its ok()
    if (ok)
    {
        line << " condition";
    }

    _run.note(line.str());

    return;
}

// STATUSES_CPP11_STEPS
//   constant: the steps of d_tests_arg_cpp_common_statuses_cpp11.
const step_fn STATUSES_CPP11_STEPS[] =
{
    &statuses_truth
};

// STATUSES_CPP11_COUNT
//   constant: how many steps STATUSES_CPP11_STEPS holds.
const std::size_t STATUSES_CPP11_COUNT =
    sizeof(STATUSES_CPP11_STEPS) / sizeof(STATUSES_CPP11_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 2.3    Text and parameters
//------------------------------------------------------------------------------

/**
 * @brief The sign of a comparison, as -1, 0 or 1.
 *
 * @param[in] _compared  what a comparison returned.
 * @return the sign.
 */
int
sign_of(
    int _compared
)
{
    return (_compared < 0)
           ? -1
           : ( (_compared > 0)
               ? 1
               : 0 );
}

/**
 * @brief Writes what a text reads on one line: its size, whether it is
 *        empty, its bytes escaped, read three ways -- by index, by iterator
 *        and as a std::string -- and whether its data is NULL.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line names the text by.
 * @param[in]     _text   the text.
 */
void
write_text(
    run&             _run,
    const char*      _label,
    const cli::text& _text
)
{
    std::string by_index;
    std::string by_iterator;

    // each byte by its index
    for (std::size_t index = 0u; index < _text.size(); ++index)
    {
        by_index += _text[index];
    }

    // each byte by an iterator
    for (cli::text::const_iterator at = _text.begin();
         at != _text.end();
         ++at)
    {
        by_iterator += *at;
    }

    const std::string copied = _text;

    _run.note(std::string(_label) +
              ": size=" + number(static_cast<unsigned long>(_text.size())) +
              " length=" +
              number(static_cast<unsigned long>(_text.length())) +
              " empty=" + (_text.empty() ? "1" : "0") +
              " [" + escape(by_index) +
              "] [" + escape(by_iterator) +
              "] [" + escape(copied) +
              "] [" + escape(_text.str()) +
              "] null=" + ((_text.data() == D_NULLPTR) ? "1" : "0"));

    return;
}

/**
 * @brief TEXT 1: texts made each way: empty, from a C string and from NULL,
 *        from a span holding a NUL, from a std::string holding one, and
 *        over a C span.
 *
 * @param[in,out] _run  the step's run.
 */
void
text_made(
    run& _run
)
{
    const std::string        held("x\0y",
                                  3u);
    const ::d_pack_text      span   = { "span", 4u };
    const char* const        absent = D_NULLPTR;

    write_text(_run,
               "empty",
               cli::text());
    write_text(_run,
               "C string",
               cli::text("abc"));
    write_text(_run,
               "NULL",
               cli::text(absent));
    write_text(_run,
               "span",
               cli::text("a\0b",
                         3u));
    write_text(_run,
               "std::string",
               cli::text(held));
    write_text(_run,
               "C span",
               cli::text(span));

    return;
}

/**
 * @brief Writes how two texts compare, by compare() and by each operator.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _first   the first text.
 * @param[in]     _second  the second.
 */
void
write_order(
    run&             _run,
    const cli::text& _first,
    const cli::text& _second
)
{
    std::ostringstream line;

    line << "[" << escape(_first) << "] vs [" << escape(_second) << "]:"
         << " compare=" << sign_of(_first.compare(_second))
         << " ==" << (_first == _second)
         << " !=" << (_first != _second)
         << " <" << (_first < _second)
         << " <=" << (_first <= _second)
         << " >" << (_first > _second)
         << " >=" << (_first >= _second);
    _run.note(line.str());

    return;
}

/**
 * @brief TEXT 2: texts compared byte by byte -- equal, a prefix first, a
 *        NUL byte counted, empty first -- and against C strings and
 *        std::strings on either side.
 *
 * @param[in,out] _run  the step's run.
 */
void
text_compared(
    run& _run
)
{
    const std::string  word("abc");
    std::ostringstream line;

    write_order(_run,
                cli::text("abc"),
                cli::text("abc"));
    write_order(_run,
                cli::text("ab"),
                cli::text("abc"));
    write_order(_run,
                cli::text("abd"),
                cli::text("abc"));
    write_order(_run,
                cli::text("a\0b",
                          3u),
                cli::text("a"));
    write_order(_run,
                cli::text(),
                cli::text("a"));
    line << "with C strings: " << (cli::text("abc") == "abc")
         << (cli::text("abc") < "abd")
         << ("abd" > cli::text("abc"))
         << " with std::strings: " << (cli::text("abc") == word)
         << (word == cli::text("abc"))
         << (cli::text("abb") < word);
    _run.note(line.str());

    return;
}

/**
 * @brief TEXT 3: a text written to a stream, its NUL byte and all.
 *
 * @param[in,out] _run  the step's run.
 */
void
text_streamed(
    run& _run
)
{
    std::ostringstream out;

    out << "<" << cli::text("a\0b",
                            3u) << ">";
    _run.note("streamed: [" + escape(out.str()) + "] size=" +
              number(static_cast<unsigned long>(out.str().size())));

    return;
}

// TEXT_STEPS
//   constant: the steps of d_tests_arg_cpp_common_text.
const step_fn TEXT_STEPS[] =
{
    &text_made,
    &text_compared,
    &text_streamed
};

// TEXT_COUNT
//   constant: how many steps TEXT_STEPS holds.
const std::size_t TEXT_COUNT = sizeof(TEXT_STEPS) / sizeof(TEXT_STEPS[0]);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/**
 * @brief TEXT_CPP17 1: a text made from a std::string_view and made into
 *        one, NUL byte and all.
 *
 * @param[in,out] _run  the step's run.
 */
void
text_viewed(
    run& _run
)
{
    const std::string_view given("v\0w",
                                 3u);
    const cli::text        made(given);
    const std::string_view back = made;

    write_text(_run,
               "from a view",
               made);
    _run.note("back: size=" +
              number(static_cast<unsigned long>(back.size())) +
              " same=" + ((back == given) ? "1" : "0"));

    return;
}

// TEXT_CPP17_STEPS
//   constant: the steps of d_tests_arg_cpp_common_text_cpp17.
const step_fn TEXT_CPP17_STEPS[] =
{
    &text_viewed
};

// TEXT_CPP17_COUNT
//   constant: how many steps TEXT_CPP17_STEPS holds.
const std::size_t TEXT_CPP17_COUNT =
    sizeof(TEXT_CPP17_STEPS) / sizeof(TEXT_CPP17_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

/**
 * @brief C_STRING 1: the string parameter made each way: by default, from a
 *        C string, from NULL, and from a std::string, whose c_str() it
 *        keeps.
 *
 * @param[in,out] _run  the step's run.
 */
void
c_string_made(
    run& _run
)
{
    const std::string   held("held");
    const char* const   absent    = D_NULLPTR;
    const cli::c_string none;
    const cli::c_string given("given");
    const cli::c_string nothing(absent);
    const cli::c_string kept(held);

    _run.note(std::string("default null=") +
              ((none.get() == D_NULLPTR) ? "1" : "0") +
              " given=[" + given.get() +
              "] NULL null=" + ((nothing.get() == D_NULLPTR) ? "1" : "0") +
              " kept=[" + kept.get() +
              "] same=" + ((kept.get() == held.c_str()) ? "1" : "0"));

    return;
}

// C_STRING_STEPS
//   constant: the steps of d_tests_arg_cpp_common_c_string.
const step_fn C_STRING_STEPS[] =
{
    &c_string_made
};

// C_STRING_COUNT
//   constant: how many steps C_STRING_STEPS holds.
const std::size_t C_STRING_COUNT =
    sizeof(C_STRING_STEPS) / sizeof(C_STRING_STEPS[0]);

// 2.4    Reports, lists and buffers
//------------------------------------------------------------------------------

/**
 * @brief Writes what an error view reads on one line.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line names the view by.
 * @param[in]     _error  the view.
 */
void
write_error(
    run&              _run,
    const char*       _label,
    const cli::error& _error
)
{
    std::ostringstream line;

    line << _label
         << ": status=" << _error.get_status().get_name()
         << " name=" << _error.get_name()
         << " exit=" << _error.get_exit_code()
         << " what=[" << _error.what() << "]";
    _run.note(line.str());

    return;
}

/**
 * @brief ERRORS 1: a view of an error report, reporting nothing, a failure
 *        with a message and one without, and a completed request.
 *
 * @param[in,out] _run  the step's run.
 */
void
errors_viewed(
    run& _run
)
{
    ::d_arg_error    report = ::d_arg_error();
    const cli::error view(report);

    d_arg_error_init(&report);
    write_error(_run,
                "nothing",
                view);
    (void)d_arg_error_set(&report,
                          D_ARG_ERROR_CONVERSION,
                          D_NULLPTR,
                          D_NULLPTR,
                          "a value did not convert");
    write_error(_run,
                "message",
                view);
    (void)d_arg_error_set(&report,
                          D_ARG_ERROR_REQUIRED,
                          D_NULLPTR,
                          D_NULLPTR,
                          D_NULLPTR);
    write_error(_run,
                "no message",
                view);
    (void)d_arg_error_set(&report,
                          D_ARG_CALL_FOR_HELP,
                          D_NULLPTR,
                          D_NULLPTR,
                          D_NULLPTR);
    write_error(_run,
                "help",
                view);
    _run.note(std::string("same report=") +
              ((&view.get() == &report) ? "1" : "0"));
    d_arg_error_release(&report);

    return;
}

// ERRORS_STEPS
//   constant: the steps of d_tests_arg_cpp_common_errors.
const step_fn ERRORS_STEPS[] =
{
    &errors_viewed
};

// ERRORS_COUNT
//   constant: how many steps ERRORS_STEPS holds.
const std::size_t ERRORS_COUNT =
    sizeof(ERRORS_STEPS) / sizeof(ERRORS_STEPS[0]);

/**
 * @brief Writes a list of texts read every way: its size and ends, its
 *        elements by index, forward and backward by iterator, and copied.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the lines name the list by.
 * @param[in]     _list   the list.
 */
template<typename List>
void
write_list(
    run&        _run,
    const char* _label,
    const List& _list
)
{
    std::string forward;
    std::string backward;
    std::string indexed;

    // each element by its index
    for (std::size_t index = 0u; index < _list.size(); ++index)
    {
        indexed += "[" + escape(_list[index]) + "]";
    }

    // each element forward
    for (typename List::const_iterator at = _list.begin();
         at != _list.end();
         ++at)
    {
        forward += "[" + escape(*at) + "]";
    }

    // each element backward
    for (typename List::const_iterator at = _list.end();
         at != _list.begin();
         )
    {
        --at;
        backward += "[" + escape(*at) + "]";
    }

    const std::vector<std::string> copied = _list;

    _run.note(std::string(_label) +
              ": size=" + number(static_cast<unsigned long>(_list.size())) +
              " empty=" + (_list.empty() ? "1" : "0") +
              " indexed=" + indexed +
              " forward=" + forward +
              " backward=" + backward +
              " copied=" +
              number(static_cast<unsigned long>(copied.size())));

    // the ends, where there are any
    if (!_list.empty())
    {
        _run.note(std::string(_label) +
                  ": front=[" + escape(_list.front()) +
                  "] back=[" + escape(_list.back()) + "]");
    }

    return;
}

/**
 * @brief LISTS 1: views of an array of spans and of one of C strings, and
 *        an empty view, read every way.
 *
 * @param[in,out] _run  the step's run.
 */
void
lists_read(
    run& _run
)
{
    static const ::d_pack_text SPANS[] =
    {
        { "a", 1u },
        { "bc", 2u },
        { "", 0u }
    };
    static char                NAME_X[] = "x";
    static char                NAME_Y[] = "y";
    static char* const         NAMES[]  = { NAME_X, NAME_Y };
    const cli::text_list       spans(SPANS,
                                     3u);
    const cli::name_list       names(NAMES,
                                     2u);
    const cli::text_list       none;

    write_list(_run,
               "spans",
               spans);
    write_list(_run,
               "names",
               names);
    write_list(_run,
               "none",
               none);
    _run.note(std::string("data same=") +
              ((spans.data() == SPANS) ? "1" : "0"));

    return;
}

/**
 * @brief LISTS 2: a list's iterator moved every way: by steps and offsets,
 *        compared, subtracted and indexed.
 *
 * @param[in,out] _run  the step's run.
 */
void
lists_iterated(
    run& _run
)
{
    static const ::d_pack_text SPANS[] =
    {
        { "p", 1u },
        { "q", 1u },
        { "r", 1u },
        { "s", 1u }
    };
    const cli::text_list       list(SPANS,
                                    4u);
    cli::text_list::const_iterator at = list.begin();
    std::ostringstream             line;

    line << "begin=" << *at;
    at += 2;
    line << " +=2:" << *at;
    at -= 1;
    line << " -=1:" << *at;
    line << " +2:" << *(at + 2);
    line << " -1:" << *(at - 1);
    line << " [1]:" << at[1];
    line << " post++:" << *(at++) << "," << *at;
    line << " post--:" << *(at--) << "," << *at;
    line << " end-begin=" << (list.end() - list.begin());
    line << " <" << (list.begin() < list.end())
         << " ==" << (at == list.begin() + 1)
         << " !=" << (at != list.begin());
    _run.note(line.str());

    return;
}

/**
 * @brief LISTS 3: a view of an array of the C library's options, whose
 *        elements are handles, read by index and by iterator, and copied.
 *
 * @param[in,out] _run  the step's run.
 */
void
lists_handles(
    run& _run
)
{
    cli::option          first   = _run.app().add_option("--first");
    cli::option          second  = _run.app().add_option("--second");
    ::d_arg_option*      items[] = { first.get(), second.get() };
    const cli::option_list
                         handles(items,
                                 2u);
    std::string          forward;

    // each handle by an iterator
    for (cli::option_list::const_iterator at = handles.begin();
         at != handles.end();
         ++at)
    {
        forward += "[" + std::string((*at)->get_name()) + "]";
    }

    const std::vector<cli::option> copied = handles.to_vector();

    _run.note("handles: size=" +
              number(static_cast<unsigned long>(handles.size())) +
              " [1]=" + std::string(handles[1]->get_name()) +
              " forward=" + forward +
              " copied=" + number(static_cast<unsigned long>(copied.size())) +
              " same=" + ((copied[0] == first) ? "1" : "0"));

    return;
}

// LISTS_STEPS
//   constant: the steps of d_tests_arg_cpp_common_lists.
const step_fn LISTS_STEPS[] =
{
    &lists_read,
    &lists_iterated,
    &lists_handles
};

// LISTS_COUNT
//   constant: how many steps LISTS_STEPS holds.
const std::size_t LISTS_COUNT = sizeof(LISTS_STEPS) / sizeof(LISTS_STEPS[0]);

/**
 * @brief Writes what a buffer view reads on one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line names the buffer by.
 * @param[in]     _buffer  the view.
 */
void
write_buffer(
    run&               _run,
    const char*        _label,
    const cli::buffer& _buffer
)
{
    _run.note(std::string(_label) +
              ": size=" + number(static_cast<unsigned long>(_buffer.size())) +
              " empty=" + (_buffer.empty() ? "1" : "0") +
              " view=[" + escape(_buffer.view()) +
              "] str=[" + escape(_buffer.str()) + "]");

    return;
}

/**
 * @brief BUFFERS 1: a buffer appended to, by text and by character, cut,
 *        and refusing every append once it could not grow, what it held
 *        kept.
 *
 * @param[in,out] _run  the step's run.
 */
void
buffers_appended(
    run& _run
)
{
    ::d_arg_buffer    raw  = ::d_arg_buffer();
    cli::buffer       text(raw);

    write_buffer(_run,
                 "new",
                 text);

    const cli::status first  = text.append(cli::text("abc"));
    const cli::status second = text.append('d');

    write_buffer(_run,
                 "appended",
                 text);
    text.truncate(2u);
    write_buffer(_run,
                 "cut",
                 text);
    raw.failed = 1;

    const cli::status refused = text.append(cli::text("x"));

    write_buffer(_run,
                 "refused",
                 text);
    _run.note(std::string("statuses: ") + first.get_name() + " " +
              second.get_name() + " " + refused.get_name() +
              " same buffer=" + ((&text.get() == &raw) ? "1" : "0"));
    d_arg_buffer_release(&raw);

    return;
}

// BUFFERS_STEPS
//   constant: the steps of d_tests_arg_cpp_common_buffers.
const step_fn BUFFERS_STEPS[] =
{
    &buffers_appended
};

// BUFFERS_COUNT
//   constant: how many steps BUFFERS_STEPS holds.
const std::size_t BUFFERS_COUNT =
    sizeof(BUFFERS_STEPS) / sizeof(BUFFERS_STEPS[0]);

/**
 * @brief SINKS 1: a sink over a std::string, appending; one over a stream,
 *        writing until the stream fails, then refusing every write whole.
 *
 * @param[in,out] _run  the step's run.
 */
void
sinks_written(
    run& _run
)
{
    std::string        text("<");
    std::ostringstream stream;
    const ::d_pack_sink into_text   = cli::string_sink(text);
    const ::d_pack_sink into_stream = cli::stream_sink(stream);
    const int           first       = d_arg_sink_write(into_text,
                                                       "ab",
                                                       2u);
    const int           second      = d_arg_sink_cstr(into_text,
                                                      "c>");
    const int           streamed    = d_arg_sink_write(into_stream,
                                                       "xy",
                                                       2u);

    stream.setstate(std::ios_base::badbit);

    const int refused = d_arg_sink_write(into_stream,
                                         "z",
                                         1u);

    _run.note("text=[" + escape(text) + "] stream=[" +
              escape(stream.str()) + "] statuses: " +
              cli::status(first).get_name() + " " +
              cli::status(second).get_name() + " " +
              cli::status(streamed).get_name() + " " +
              cli::status(refused).get_name());

    return;
}

// SINKS_STEPS
//   constant: the steps of d_tests_arg_cpp_common_sinks.
const step_fn SINKS_STEPS[] =
{
    &sinks_written
};

// SINKS_COUNT
//   constant: how many steps SINKS_STEPS holds.
const std::size_t SINKS_COUNT = sizeof(SINKS_STEPS) / sizeof(SINKS_STEPS[0]);

}  // namespace

//==============================================================================
// 3.  CLI11'S VOCABULARY
//==============================================================================

/*
d_tests_arg_cpp_common_exit_codes
  CLI11's exit codes, under the face's names for them.
  Tests the following:
  - each of CLI11's ExitCodes, by the face's name of it
*/
bool
d_tests_arg_cpp_common_exit_codes()
{
    return steps_hold("EXIT_CODES",
                      EXIT_CODES_STEPS,
                      EXIT_CODES_COUNT,
                      EXIT_CODES);
}

/*
d_tests_arg_cpp_common_vocabulary
  The face's enumerations as the C library's values.
  Tests the following:
  - every enumerator of every enumeration is the C value it stands for
*/
bool
d_tests_arg_cpp_common_vocabulary()
{
    return steps_hold("VOCABULARY",
                      VOCABULARY_STEPS,
                      VOCABULARY_COUNT,
                      VOCABULARY);
}

//==============================================================================
// 4.  STATUSES
//==============================================================================

/*
d_tests_arg_cpp_common_statuses
  A status: the value every fallible call and every parse returns.
  Tests the following:
  - every status's name, exit code, and whether it is ok and an error; a
    value past either end of the enumeration
  - made by default, from a status_code and from an int; compared, a
    status_code on either side
*/
bool
d_tests_arg_cpp_common_statuses()
{
    return steps_hold("STATUSES",
                      STATUSES_STEPS,
                      STATUSES_COUNT,
                      STATUSES);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_common_statuses_cpp11
  A status's explicit truth, from C++11.
  Tests the following:
  - true for ok alone; false for a completed request and for an error; read
    in a condition
*/
bool
d_tests_arg_cpp_common_statuses_cpp11()
{
    return steps_hold("STATUSES_CPP11",
                      STATUSES_CPP11_STEPS,
                      STATUSES_CPP11_COUNT,
                      STATUSES_CPP11);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// 5.  TEXT AND PARAMETERS
//==============================================================================

/*
d_tests_arg_cpp_common_text
  A text: a run of bytes viewed, not owned.
  Tests the following:
  - made empty, from a C string and from NULL, from a span and a std::string
    holding a NUL, over a C span; read by index, by iterator, as a
    std::string and by str()
  - compared byte by byte, a prefix first, against C strings and
    std::strings on either side
  - written to a stream, its NUL byte and all
*/
bool
d_tests_arg_cpp_common_text()
{
    return steps_hold("TEXT",
                      TEXT_STEPS,
                      TEXT_COUNT,
                      TEXT);
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_arg_cpp_common_text_cpp17
  A text and std::string_view, from C++17.
  Tests the following:
  - made from a view and made into one, a NUL byte kept
*/
bool
d_tests_arg_cpp_common_text_cpp17()
{
    return steps_hold("TEXT_CPP17",
                      TEXT_CPP17_STEPS,
                      TEXT_CPP17_COUNT,
                      TEXT_CPP17);
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_arg_cpp_common_c_string
  The string parameter every call takes.
  Tests the following:
  - by default NULL; from a C string; from NULL; from a std::string, its
    c_str() kept
*/
bool
d_tests_arg_cpp_common_c_string()
{
    return steps_hold("C_STRING",
                      C_STRING_STEPS,
                      C_STRING_COUNT,
                      C_STRING);
}

//==============================================================================
// 6.  REPORTS, LISTS AND BUFFERS
//==============================================================================

/*
d_tests_arg_cpp_common_errors
  A view of an error report, read as CLI11's Error is read.
  Tests the following:
  - reporting nothing; a failure with a message and without, what() the
    status's own text then; a completed request; the report viewed
*/
bool
d_tests_arg_cpp_common_errors()
{
    return steps_hold("ERRORS",
                      ERRORS_STEPS,
                      ERRORS_COUNT,
                      ERRORS);
}

/*
d_tests_arg_cpp_common_lists
  Views of the C library's arrays, read as CLI11's lists are read.
  Tests the following:
  - spans, C strings and none: size, ends, by index, forward and backward,
    copied into a std::vector<std::string>
  - an iterator moved by steps and offsets, compared, subtracted, indexed
  - records viewed as handles: by index, by iterator, copied
*/
bool
d_tests_arg_cpp_common_lists()
{
    return steps_hold("LISTS",
                      LISTS_STEPS,
                      LISTS_COUNT,
                      LISTS);
}

/*
d_tests_arg_cpp_common_buffers
  A view of the buffer help and configurations are built in.
  Tests the following:
  - appended to by text and by character; cut; refusing every append once
    it could not grow, what it held kept
*/
bool
d_tests_arg_cpp_common_buffers()
{
    return steps_hold("BUFFERS",
                      BUFFERS_STEPS,
                      BUFFERS_COUNT,
                      BUFFERS);
}

/*
d_tests_arg_cpp_common_sinks
  The sinks over a std::string and a std::ostream.
  Tests the following:
  - a string appended to; a stream written until it fails, then every
    write refused whole as a file error
*/
bool
d_tests_arg_cpp_common_sinks()
{
    return steps_hold("SINKS",
                      SINKS_STEPS,
                      SINKS_COUNT,
                      SINKS);
}

//==============================================================================
// 7.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_common
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_common[] =
{
    { "cpp_common_exit_codes",
      d_tests_arg_cpp_common_exit_codes },
    { "cpp_common_vocabulary",
      d_tests_arg_cpp_common_vocabulary },
    { "cpp_common_statuses",
      d_tests_arg_cpp_common_statuses },
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_common_statuses_cpp11",
      d_tests_arg_cpp_common_statuses_cpp11 },
#endif
    { "cpp_common_text",
      d_tests_arg_cpp_common_text },
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    { "cpp_common_text_cpp17",
      d_tests_arg_cpp_common_text_cpp17 },
#endif
    { "cpp_common_c_string",
      d_tests_arg_cpp_common_c_string },
    { "cpp_common_errors",
      d_tests_arg_cpp_common_errors },
    { "cpp_common_lists",
      d_tests_arg_cpp_common_lists },
    { "cpp_common_buffers",
      d_tests_arg_cpp_common_buffers },
    { "cpp_common_sinks",
      d_tests_arg_cpp_common_sinks }
};

// d_tests_arg_cpp_common_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_common_count =
    sizeof(d_tests_arg_cpp_common) / sizeof(d_tests_arg_cpp_common[0]);
