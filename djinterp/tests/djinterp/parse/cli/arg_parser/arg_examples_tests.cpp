/*******************************************************************************
* djinterp [parse]                                        arg_examples_tests.cpp
*
* The argument parser's C++ examples suite's sections.
*   Each section runs one of CLI11 2.7.2's examples (CLI11's examples/
* directory), ported to the face in the CASES section: its definitions made
* on the run's parser -- the program CLI11 calls `prog` here, its
* description the example's -- its main's parse run once, as CLI11_PARSE
* runs it (run::parse_as_main writes the code main returns, -1 when the
* parse goes on, and what the ending printed), then what the example
* prints, a line for each it writes to std::cout. Each runs with the
* arguments CLI11's tests run it with (examples/CMakeLists.txt), and others
* where they show more. tools/cppref/cppref.py compiles the section
* against CLI11 2.7.2, and the transcripts below are what CLI11 wrote.
*   What was changed to port, the same in both: the timers of groups and
* subcom_partitioned are gone, as their output is the time; std::cout is
* the run's lines; a variable an example's main declares is the function's
* own, each run's, and a formatter or format an example makes is a static
* object, which a command borrows; config_app gives its parser a format
* quoting as it asks, where CLI11's changes the one its App has;
* subcom_partitioned adds its two nameless subcommands with
* add_subcommand("", description), as the face makes no command apart from
* a parser; custom_validator's and date_validator's validators are made
* from functions, as the face's validator is, not derived from CLI11's
* Validator, the range's centre and width held where both the function and
* the options reach them, and read without CLI::detail::lexical_cast;
* enum's value is written as the number CLI11's enums::operator<< writes;
* modhelp's help asked for is written without CLI11's throw; close_match
* swaps rather than std::exchange()s, which is C++14, and keeps every
* subcommand by a filter where CLI11 takes nullptr; help_usage's usage
* names `prog` and leaves out ensure_utf8, which changes nothing where
* arguments are UTF-8; and subcom_help asks parsed() where CLI11 converts
* the App to bool, as a handle's truth is whether it names a command
* (ISSUES 50). What does not port: json, which reads JSON through
* nlohmann::json; and custom_parse, whose conversion is an overload of
* CLI11's lexical_cast, where the face's is a type_traits specialization
* (the type suite's CALLER_TRAITS).
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_examples_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_examples_tests.hpp"  // corresponding header

// the examples are C++11, and so is everything they include: below it there
// is no suite
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <algorithm>  // std::reverse, std::min
#include <array>      // std::array
#include <cstddef>    // std::size_t
#include <ctime>      // std::tm
#include <iomanip>    // std::get_time
#include <map>        // std::map
#include <memory>     // std::shared_ptr, std::make_shared
#include <numeric>    // std::iota
#include <ostream>    // std::ostream
#include <sstream>    // std::ostringstream, std::istringstream
#include <string>     // std::string
#include <utility>    // std::pair, std::swap
#include <vector>     // std::vector
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
using cli::testing::file_entry;
using cli::testing::escape;
using cli::testing::scratch_directory;
using cli::testing::steps_hold;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// SIMPLE
//   constant: what d_tests_arg_cpp_examples_simple writes, as CLI11 writes
// it.
const char SIMPLE[] =
    "args []\n"
    "main -1\n"
    "Working on file: , direct count: 0, opt count: 0\n"
    "Working on count: 0, direct count: 0, opt count: 0\n"
    "Received flag: 0 (0) times\n"
    "Some value: 0\n"
    "args [-f filename.txt -c 12 --flag --flag -d 1.2]\n"
    "main -1\n"
    "Working on file: filename.txt, direct count: 1, opt count: 1\n"
    "Working on count: 12, direct count: 1, opt count: 1\n"
    "Received flag: 2 (2) times\n"
    "Some value: 1.2\n"
    "args [--version]\n"
    "main 0\n"
    "out 2.7.2\\n\n";

// SUBCOMMANDS
//   constant: what d_tests_arg_cpp_examples_subcommands writes, as CLI11
// writes it.
const char SUBCOMMANDS[] =
    "args []\n"
    "main 106\n"
    "err A subcommand is required\\nRun with --help or --help-all for more "
    "information.\\n\n"
    "args [--random start --file name stop --count]\n"
    "main -1\n"
    "Working on --file from start: name\n"
    "Working on --count from stop: 1, direct count: 1\n"
    "Count of --random flag: 1\n"
    "Subcommand: start\n"
    "Subcommand: stop\n"
    "args [--help-all]\n"
    "main 0\n"
    "out K3Pi goofit fitter\\n\\n\\nprog [OPTIONS] "
    "SUBCOMMANDS\\n\\n\\nOPTIONS:\\n  -h,     --help              Print this "
    "help message and exit\\n          --help-all          Expand all help\\n  "
    "        --random            Some random "
    "flag\\n\\nSUBCOMMANDS:\\nstart\\nA great subcommand\\n\\n\\nOPTIONS:\\n  "
    "-f,     --file TEXT         File name\\n\\nstop\\nDo you really want to "
    "stop?\\n\\n\\nOPTIONS:\\n  -c,     --count             Counter\\n\\n\n";

// SUBCOM_PARTITIONED
//   constant: what d_tests_arg_cpp_examples_subcom_partitioned writes, as
// CLI11 writes it.
const char SUBCOM_PARTITIONED[] =
    "args []\n"
    "main 106\n"
    "err --file is required\\nRun with --help for more information.\\n\n"
    "args [--file this --count --count -d 1.2]\n"
    "main -1\n"
    "Working on file: this, direct count: 1, opt count: 1\n"
    "Working on count: 2, direct count: 2, opt count: 2\n"
    "Some value: 1.2\n"
    "args [--help]\n"
    "main 0\n"
    "out K3Pi goofit fitter\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,    "
    " --help              Print this help message and exit\\n[Option Group: "
    "SUBCOMMANDS]\\n  Important\\n  \\n\\n  POSITIONALS:\\n    file TEXT "
    "REQUIRED          File name\\n\\n  OPTIONS:\\n    -h,     --help          "
    "    Print this help message and exit\\n    -f,     --file TEXT REQUIRED "
    "\\n                                File name\\n    -c,     --count [0]  "
    "REQUIRED \\n                                Counter\\n[Option Group: "
    "SUBCOMMANDS]\\n  Other\\n  \\n\\n  OPTIONS:\\n    -h,     --help          "
    "    Print this help message and exit\\n    -d,     --double FLOAT      "
    "Some Value\\n\n";

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// CONFIG_APP
//   constant: what d_tests_arg_cpp_examples_config_app writes, as CLI11
// writes it.
const char CONFIG_APP[] =
    "args [-p]\n"
    "main -1\n"
    "config [file=\"\"\\ncount=0\\nflag=\"\"\\ndouble=0\\n]\n"
    "args [-p -f /]\n"
    "main -1\n"
    "config [file=\"/\"\\ncount=0\\nflag=\"\"\\ndouble=0\\n]\n"
    "args [-f  -p]\n"
    "main -1\n"
    "config [file=\"\"\\ncount=0\\nflag=\"\"\\ndouble=0\\n]\n"
    "args [-f / -p]\n"
    "main -1\n"
    "config [file=\"/\"\\ncount=0\\nflag=\"\"\\ndouble=0\\n]\n"
    "args [-f data --flag -c 3]\n"
    "main -1\n"
    "Working on file: data, direct count: 1, opt count: 1\n"
    "Working on count: 3, direct count: 1, opt count: 1\n"
    "Received flag: 1 (1) times\n"
    "Some value: 0\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// OPTION_GROUPS
//   constant: what d_tests_arg_cpp_examples_option_groups writes, as CLI11
// writes it.
const char OPTION_GROUPS[] =
    "args []\n"
    "main 106\n"
    "err Exactly 1 option from [--csv,--human,--binary] is required\\nRun with "
    "--help or --help-all for more information.\\n\n"
    "args [--csv --binary]\n"
    "main 106\n"
    "err Exactly 1 option from [--csv,--human,--binary] is required but 2 were "
    "given\\nRun with --help or --help-all for more information.\\n\n"
    "args [--csv --address 192.168.1.1 -o test.out]\n"
    "main 106\n"
    "err Requires at most 1 options be given from [-o,--file,--address]\\nRun "
    "with --help or --help-all for more information.\\n\n"
    "args [--human -o out.txt]\n"
    "main -1\n"
    "Selected human readable format\n"
    " sent to file out.txt\n";

// POSITIONAL_ARITY
//   constant: what d_tests_arg_cpp_examples_positional_arity writes, as
// CLI11 writes it.
const char POSITIONAL_ARITY[] =
    "args [one]\n"
    "main -1\n"
    "File 1 = one\n"
    "args [one two]\n"
    "main -1\n"
    "File 1 = one\n"
    "File 2 = two\n"
    "args [1 2 one]\n"
    "main -1\n"
    "Num1 = 1\n"
    "Num2 = 2\n"
    "File 1 = one\n"
    "args [1 one two]\n"
    "main 104\n"
    "err Could not convert: num2 = one\\nRun with --help for more "
    "information.\\n\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// POSITIONAL_VALIDATION
//   constant: what d_tests_arg_cpp_examples_positional_validation writes, as
// CLI11 writes it.
const char POSITIONAL_VALIDATION[] =
    "args [one]\n"
    "main -1\n"
    "File 1 = one\n"
    "args [one 1 2 two]\n"
    "main -1\n"
    "Num1 = 1\n"
    "Num2 = 2\n"
    "File 1 = one\n"
    "File 2 = two\n"
    "args [1 2 one]\n"
    "main -1\n"
    "Num1 = 1\n"
    "Num2 = 2\n"
    "File 1 = one\n"
    "args [1 one two 2]\n"
    "main -1\n"
    "Num1 = 1\n"
    "Num2 = 2\n"
    "File 1 = one\n"
    "File 2 = two\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// DATE_VALIDATOR
//   constant: what d_tests_arg_cpp_examples_date_validator writes, as CLI11
// writes it.
const char DATE_VALIDATOR[] =
    "args [--time 2025-08-14T14:30:00]\n"
    "main -1\n"
    "date given = 2025-08-14T14:30:00\n"
    "args [--time 8/17/2025T14:30:00]\n"
    "main 105\n"
    "err --time: Failed to parse time string\\nRun with --help for more "
    "information.\\n\n";

// CUSTOM_VALIDATOR
//   constant: what d_tests_arg_cpp_examples_custom_validator writes, as
// CLI11 writes it.
const char CUSTOM_VALIDATOR[] =
    "args [--number 12]\n"
    "main 105\n"
    "err --number: value not within range\\nRun with --help for more "
    "information.\\n\n"
    "args [--number 12 --center 10 --range 3]\n"
    "main -1\n"
    "number 12 in range = 10 +/- 3\n";

// SHAPES
//   constant: what d_tests_arg_cpp_examples_shapes writes, as CLI11 writes
// it.
const char SHAPES[] =
    "args [circle 4.4 circle 10.7 rectangle 4 4 circle 2.3 triangle 4.5 ++ "
    "rectangle 2.1 ++ circle 234.675]\n"
    "circle1 with radius 4.4\n"
    "circle2 with radius 10.7\n"
    "rectangle1 with edges [4,4]\n"
    "circle3 with radius 2.3\n"
    "triangle1 with sides [4.5]\n"
    "rectangle2 with edges [2.1,2.1]\n"
    "circle4 with radius 234.675\n"
    "main -1\n"
    "args [--help-all]\n"
    "main 0\n"
    "out load shapes\\n\\n\\nprog [OPTIONS] [SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  "
    "-h,     --help              Print this help message and exit\\n          "
    "--help-all          \\n\\nSUBCOMMANDS:\\ncircle\\ndraw a "
    "circle\\n\\n\\nPOSITIONALS:\\n  radius FLOAT REQUIRED       the radius of "
    "the circle\\n\\nrectangle\\ndraw a rectangle\\n\\n\\nPOSITIONALS:\\n  "
    "edge1 FLOAT REQUIRED        the first edge length of the rectangle\\n  "
    "edge2 FLOAT                 the second edge length of the "
    "rectangle\\n\\ntriangle\\ndraw a rectangle\\n\\n\\nPOSITIONALS:\\n  sides "
    "FLOAT ...             the side lengths of the triangle\\n\\n\n";

// RANGES
//   constant: what d_tests_arg_cpp_examples_ranges writes, as CLI11 writes
// it.
const char RANGES[] =
    "args [--range 1 2 3]\n"
    "main -1\n"
    "range is [2:1:3]\n"
    "args [--min 2 --max 3]\n"
    "main -1\n"
    "range is [2:1:3]\n"
    "args [--min 2 --max 3 --step 1 --range 1 2 3]\n"
    "main 106\n"
    "err Exactly 1 option from [-R,--range,[Option Group: min_max_step]] is "
    "required but 2 were given\\nRun with --help for more information.\\n\n";

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// VALIDATORS
//   constant: what d_tests_arg_cpp_examples_validators writes, as CLI11
// writes it.
const char VALIDATORS[] =
    "args [--help]\n"
    "main 0\n"
    "out Validator checker\\n\\n\\nprog [OPTIONS] "
    "[file]\\n\\n\\nPOSITIONALS:\\n  file TEXT:FILE              File "
    "name\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -f,     --file TEXT:FILE    File name\\n  -v,     "
    "--value INT:INT in [3 - 6] \\n                              Value in "
    "range\\n\n"
    "args [--file nonex.xxx]\n"
    "main 105\n"
    "err --file: File does not exist: nonex.xxx\\nRun with --help for more "
    "information.\\n\n"
    "args [--value 9]\n"
    "main 105\n"
    "err --value: Value 9 not in range [3 - 6]\\nRun with --help for more "
    "information.\\n\n"
    "args [--value 4]\n"
    "main -1\n"
    "Try printing help or failing the validator\n";

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

// GROUPS
//   constant: what d_tests_arg_cpp_examples_groups writes, as CLI11 writes
// it.
const char GROUPS[] =
    "args []\n"
    "main 106\n"
    "err --file is required\\nRun with --help for more information.\\n\n"
    "args [--file this --count --count -d 1.2]\n"
    "main -1\n"
    "Working on file: this, direct count: 1, opt count: 1\n"
    "Working on count: 2, direct count: 2, opt count: 2\n"
    "Some value: 1.2\n"
    "args [--help]\n"
    "main 0\n"
    "out K3Pi goofit fitter\\n\\n\\nprog [OPTIONS] "
    "file\\n\\n\\nPOSITIONALS:\\n  file TEXT REQUIRED          File "
    "name\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n\\nImportant:\\n  -f,     --file TEXT REQUIRED \\n     "
    "                         File name\\n  -c,     --count [0]  REQUIRED \\n  "
    "                            Counter\\n\\nOther:\\n  -d,     --double "
    "FLOAT      Some Value\\n\n";

// INTER_ARGUMENT_ORDER
//   constant: what d_tests_arg_cpp_examples_inter_argument_order writes, as
// CLI11 writes it.
const char INTER_ARGUMENT_ORDER[] =
    "args [--foo 1 2 3 --x --bar 4 5 6 --z --foo 7 8]\n"
    "main -1\n"
    "foo : 1\n"
    "foo : 2\n"
    "foo : 3\n"
    "bar : 4\n"
    "bar : 5\n"
    "bar : 6\n"
    "foo : 7\n"
    "foo : 8\n";

// PREFIX_COMMAND
//   constant: what d_tests_arg_cpp_examples_prefix_command writes, as CLI11
// writes it.
const char PREFIX_COMMAND[] =
    "args [-v 3 2 1 -- other one two 3]\n"
    "main -1\n"
    "Prefix: 3 : 2 : 1 \n"
    "Remaining commands: other one two 3 \n";

// MINIMAL
//   constant: what d_tests_arg_cpp_examples_minimal writes, as CLI11 writes
// it.
const char MINIMAL[] =
    "args []\n"
    "main -1\n"
    "args [--help]\n"
    "main 0\n"
    "out App description\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n\n";

// ARG_CAPTURE
//   constant: what d_tests_arg_cpp_examples_arg_capture writes, as CLI11
// writes it.
const char ARG_CAPTURE[] =
    "args [-v 27 --sub -v 13 --val prefix]\n"
    "main -1\n"
    "value=27\n"
    "after Args:-v 13 --val prefix \n";

// CALLBACK_PASSTHROUGH
//   constant: what d_tests_arg_cpp_examples_callback_passthrough writes, as
// CLI11 writes it.
const char CALLBACK_PASSTHROUGH[] =
    "args [--argname t2 --t2 test]\n"
    "main -1\n"
    "the value is now test\n"
    "args [--arg EEEK --argname arg]\n"
    "main -1\n"
    "the value is now EEEK\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// ENUM_LEVELS
//   constant: what d_tests_arg_cpp_examples_enum writes, as CLI11 writes it.
const char ENUM_LEVELS[] =
    "args [-l 1]\n"
    "main -1\n"
    "Enum received: 1\n"
    "args [-l 4]\n"
    "main 105\n"
    "err --level: Check 4 value in {high->0,low->2,medium->1} OR {0,2,1} "
    "FAILED\\nRun with --help for more information.\\n\n"
    "args [--level MEDIUM]\n"
    "main -1\n"
    "Enum received: 1\n";

// ENUM_OSTREAM
//   constant: what d_tests_arg_cpp_examples_enum_ostream writes, as CLI11
// writes it.
const char ENUM_OSTREAM[] =
    "args [--level medium]\n"
    "main -1\n"
    "Enum received: Medium (ft rom custom ostream)\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// DIGIT_ARGS
//   constant: what d_tests_arg_cpp_examples_digit_args writes, as CLI11
// writes it.
const char DIGIT_ARGS[] =
    "args [-h]\n"
    "main 0\n"
    "out d\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help          "
    "    Print this help message and exit\\n  -1{1}, -2{2}, -3{3}, -4{4}, "
    "-5{5}, -6{6}, -7{7}, -8{8}, -9{9} [0]  \\n                              "
    "compression level\\n\n"
    "args [-3]\n"
    "main -1\n"
    "value = 3\n"
    "args [-2 -7]\n"
    "main -1\n"
    "value = 9\n";

// MODHELP
//   constant: what d_tests_arg_cpp_examples_modhelp writes, as CLI11 writes
// it.
const char MODHELP[] =
    "args [-a test -h]\n"
    "Option -a string in help: test\n"
    "help [Modify the help print so that argument values are "
    "accessible.\\nNote that this will not shortcut `->required` and other "
    "similar options.\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Request help\\n  -a TEXT                     Some "
    "description\\n]\n"
    "args [-a test]\n"
    "Option -a string: test\n"
    "args [-a test -b]\n"
    "Option -a string in help: test\n"
    "exit 109\n"
    "err prog: The following argument was not expected: -b\\n\n";

// ARRAY_OPTION
//   constant: what d_tests_arg_cpp_examples_array_option writes, as CLI11
// writes it.
const char ARRAY_OPTION[] =
    "args [--a 1 2]\n"
    "main -1\n"
    "pass 1 2\n"
    "args [--help]\n"
    "main 0\n"
    "out My app\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help     "
    "         Print this help message and exit\\n          --a [INT,INT] "
    "[[0,1]]  \\n                              an array\\n\n";

// SUBCOM_IN_FILES
//   constant: what d_tests_arg_cpp_examples_subcom_in_files writes, as CLI11
// writes it.
const char SUBCOM_IN_FILES[] =
    "args [subcommand_a -f this.txt --with-foo]\n"
    "Working on file: this.txt\n"
    "Using foo!\n"
    "main -1\n"
    "args []\n"
    "main 106\n"
    "err A subcommand is required\\nRun with --help for more information.\\n\n";

// FORMATTER
//   constant: what d_tests_arg_cpp_examples_formatter writes, as CLI11
// writes it.
const char FORMATTER[] =
    "args [-h]\n"
    "main 0\n"
    "out d\\n\\n\\nprog [OPTIONS] [SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  -h, "
    "--help OPTION \\n               Print this help message and exit\\n     "
    "--help-all OPTION \\n               Show all help\\n     --flag OPTION "
    "\\n               This is a flag\\n\\nSUBCOMMANDS:\\n  one          "
    "Description One\\n  two          Description Two\\n\n"
    "args [--help-all]\n"
    "main 0\n"
    "out d\\n\\n\\nprog [OPTIONS] [SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  -h, "
    "--help OPTION \\n               Print this help message and exit\\n     "
    "--help-all OPTION \\n               Show all help\\n     --flag OPTION "
    "\\n               This is a flag\\n\\nSUBCOMMANDS:\\none\\nDescription "
    "One\\n\\n\\nOPTIONS:\\n     --oneflag OPTION \\n               Some "
    "flag\\n\\ntwo\\nDescription Two\\n\\n\\nOPTIONS:\\n     --twoflag OPTION "
    "\\n               Some other flag\\n\\n\n"
    "args []\n"
    "main -1\n"
    "This app was meant to show off the formatter, run with -h\n";

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// NESTED
//   constant: what d_tests_arg_cpp_examples_nested writes, as CLI11 writes
// it.
const char NESTED[] =
    "args [--help-all]\n"
    "main 0\n"
    "out Vision Application\\n\\n\\nprog [OPTIONS] "
    "[SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  -h,     --help              Print this "
    "help message and exit\\n          --help-all          Expand all help\\n  "
    "        --version           Get "
    "version\\n\\nSUBCOMMANDS:\\ncamera\\nConfigure the app "
    "camera\\n\\n\\nSUBCOMMANDS:\\n  mvcamera                    MatrixVision "
    "Camera Configuration\\n  mock                        Mock Camera "
    "Configuration\\n\\n\n"
    "args [camera mock -p .]\n"
    "main -1\n"
    "args [camera mvcamera]\n"
    "main -1\n"
    "args [camera mvcamera -c missing.json]\n"
    "main 105\n"
    "err --config: File does not exist: missing.json\\nRun with --help or "
    "--help-all for more information.\\n\n"
    "args [camera mock]\n"
    "main 106\n"
    "err --path is required\\nRun with --help or --help-all for more "
    "information.\\n\n";

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

// SUBCOM_HELP
//   constant: what d_tests_arg_cpp_examples_subcom_help writes, as CLI11
// writes it.
const char SUBCOM_HELP[] =
    "args [sub --help]\n"
    "main 0\n"
    "out Some subcommand\\n\\n\\nprog sub [OPTIONS] "
    "sub_arg\\n\\n\\nPOSITIONALS:\\n  sub_arg TEXT REQUIRED       Argument for "
    "subcommand\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n\n"
    "args [--help sub]\n"
    "main 0\n"
    "out Some subcommand\\n\\n\\nprog sub [OPTIONS] "
    "sub_arg\\n\\n\\nPOSITIONALS:\\n  sub_arg TEXT REQUIRED       Argument for "
    "subcommand\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n\n"
    "args [sub value]\n"
    "main -1\n"
    "Got: value\n";

// RETIRED
//   constant: what d_tests_arg_cpp_examples_retired writes, as CLI11 writes
// it.
const char RETIRED[] =
    "args [--retired_option]\n"
    "main -1\n"
    "out WARNING --retired_option is retired and has no effect\\n\n"
    "args [--retired_option 567]\n"
    "main -1\n"
    "out WARNING --retired_option is retired and has no effect\\n\n"
    "args [--deprecate 1 2]\n"
    "main -1\n"
    "out --deprecate is deprecated please use '--not_deprecated' instead\\n\n"
    "args [--not_deprecated 4 5 --retired_option2 6]\n"
    "main -1\n"
    "out WARNING --retired_option2 is retired and has no effect\\n\n"
    "Retired option example: got --not_deprecated values:4 5 \n";

// CLOSE_MATCH
//   constant: what d_tests_arg_cpp_examples_close_match writes, as CLI11
// writes it.
const char CLOSE_MATCH[] =
    "args [instal]\n"
    "main -1\n"
    "install\n"
    "args [upgrde]\n"
    "unmatched command \"upgrde\", closest match is upgrade\n"
    "main -1\n"
    "args [-v 3 tst]\n"
    "unmatched command \"tst\", closest match is test\n"
    "main -1\n"
    "args [elsewhere]\n"
    "main -1\n";

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// HELP_USAGE
//   constant: what d_tests_arg_cpp_examples_help_usage writes, as CLI11
// writes it.
const char HELP_USAGE[] =
    "args [-h]\n"
    "main 0\n"
    "out CLI11 help\\n\\nUsage: prog <command> [options] <input-file> "
    "<output-file>\\n\\n\\nOPTIONS:\\n  -h,     --help              "
    "\\n\\nSUBCOMMANDS:\\ne\\nencode\\n\\n\\nPOSITIONALS:\\n  input            "
    "           input file\\n  output                      output "
    "file\\n\\nOPTIONS:\\n  -l,     --level [1..9]      encoding level\\n  -R, "
    "    --remove            remove input file\\n  -s,     --suboption         "
    "suboption\\n\\nd\\ndecode\\n\\n\\nPOSITIONALS:\\n  input                  "
    "     input file\\n  output                      output file\\n\\n\n"
    "args [e missing.txt out.txt]\n"
    "main 105\n"
    "err input: File does not exist: missing.txt\\nRun with --help for more "
    "information.\\n\n"
    "args [E in.txt out.txt -l 7 -s]\n"
    "main -1\n"
    "files in.txt out.txt level 7 suboption 1\n"
    "args []\n"
    "main 106\n"
    "err A subcommand is required\\nRun with --help for more information.\\n\n";

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

// TEST_EXE
//   constant: what d_tests_arg_cpp_examples_test_exe writes, as CLI11 writes
// it.
const char TEST_EXE[] =
    "args [-v 27 sub -v 13 --val prefix]\n"
    "main -1\n"
    "value =27\n"
    "after Args:-v 13 --val prefix \n";

//==============================================================================
// 1.  CASES
//==============================================================================
// What the sections held to CLI11 define, in the part of the face CLI11
// shares; tools/cppref/cppref.py compiles this section against CLI11.

// 1.1    Running an example
//------------------------------------------------------------------------------

// invocation
//   struct: the arguments an example runs with, after the program's name;
// they end at the first NULL.
struct invocation
{
    const char* args[20];
};

/**
 * @brief Begins an example's run with a line naming its arguments.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _given  the arguments.
 * @return how many arguments there are.
 */
std::size_t
begin_example(
    run&              _run,
    const invocation& _given
)
{
    std::size_t count = 0u;
    std::string line("args [");

    // each argument, up to the NULL that ends them
    while ( (count < 20u) &&
            (_given.args[count] != D_NULLPTR) )
    {
        line += (count != 0u) ? " " : "";
        line += escape(_given.args[count]);
        ++count;
    }

    _run.note(line + "]");

    return count;
}

/**
 * @brief Writes nothing more: the end of the pieces say_into() writes.
 *
 * @param[in,out] _line  the line; unused.
 */
void
say_into(
    std::ostringstream& _line
)
{
    (void)_line;

    return;
}

/**
 * @brief Writes pieces into a line, as std::cout writes them.
 *
 * @param[in,out] _line   the line.
 * @param[in]     _first  the first piece.
 * @param[in]     _rest   the pieces after it.
 */
template<typename First,
         typename... Rest>
void
say_into(
    std::ostringstream& _line,
    const First&        _first,
    const Rest&...      _rest
)
{
    _line << _first;
    say_into(_line,
             _rest...);

    return;
}

/**
 * @brief Writes a line an example prints, its pieces as std::cout writes
 *        them.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _pieces  the pieces.
 */
template<typename... Pieces>
void
say(
    run&             _run,
    const Pieces&... _pieces
)
{
    std::ostringstream line;

    say_into(line,
             _pieces...);
    _run.note(line.str());

    return;
}

// example_fn
//   type: an example, run with the invocation of its own table at an index.
typedef void (*example_fn)(run&        _run,
                           std::size_t _index);

/**
 * @brief A step: an example run with the invocation of its table at an
 *        index, on the step's own run.
 *
 * @param[in,out] _run  the step's run.
 */
template<example_fn  Example,
         std::size_t Index>
void
example_step(
    run& _run
)
{
    Example(_run,
            Index);

    return;
}

// 1.2    simple
//------------------------------------------------------------------------------

// SIMPLE_RUNS
//   constant: simple's arguments: CLI11's simple_basic, simple_all and
// simple_version.
const invocation SIMPLE_RUNS[] =
{
    { { D_NULLPTR } },
    { { "-f", "filename.txt", "-c", "12", "--flag", "--flag", "-d", "1.2",
        D_NULLPTR } },
    { { "--version", D_NULLPTR } }
};

/**
 * @brief simple.cpp: options of each kind and a version flag; the values
 *        and counts written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of SIMPLE_RUNS.
 */
void
example_simple(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = SIMPLE_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    std::string       file;
    int               counted = 0;
    int               v       = 0;
    double            value   = 0.0;

    _run.app().description("K3Pi goofit fitter");
    _run.app().set_version_flag("--version",
                                "2.7.2");

    cli::option opt  = _run.app().add_option("-f,--file,file",
                                             file,
                                             "File name");
    cli::option copt = _run.app().add_option("-c,--count",
                                             counted,
                                             "Counter");
    cli::option flag = _run.app().add_flag(
                           "--flag",
                           v,
                           "Some flag that can be passed multiple times");

    _run.app().add_option("-d,--double",
                          value,
                          "Some Value");

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "Working on file: ",
        file,
        ", direct count: ",
        _run.app().count("--file"),
        ", opt count: ",
        opt->count());
    say(_run,
        "Working on count: ",
        counted,
        ", direct count: ",
        _run.app().count("--count"),
        ", opt count: ",
        copt->count());
    say(_run,
        "Received flag: ",
        v,
        " (",
        flag->count(),
        ") times");
    say(_run,
        "Some value: ",
        value);

    return;
}

// SIMPLE_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_simple.
const step_fn SIMPLE_STEPS[] =
{
    &example_step<&example_simple, 0u>,
    &example_step<&example_simple, 1u>,
    &example_step<&example_simple, 2u>
};

// SIMPLE_COUNT
//   constant: how many steps SIMPLE_STEPS holds.
const std::size_t SIMPLE_COUNT =
    sizeof(SIMPLE_STEPS) / sizeof(SIMPLE_STEPS[0]);

// 1.3    subcommands
//------------------------------------------------------------------------------

// SUBCOMMANDS_RUNS
//   constant: subcommands's arguments: CLI11's subcommands_none and
// subcommands_all, and the help with every subcommand expanded.
const invocation SUBCOMMANDS_RUNS[] =
{
    { { D_NULLPTR } },
    { { "--random", "start", "--file", "name", "stop", "--count",
        D_NULLPTR } },
    { { "--help-all", D_NULLPTR } }
};

/**
 * @brief subcommands.cpp: two subcommands, one at least required; the
 *        values, counts and subcommands written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of SUBCOMMANDS_RUNS.
 */
void
example_subcommands(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = SUBCOMMANDS_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);

    _run.app().description("K3Pi goofit fitter");
    _run.app().set_help_all_flag("--help-all",
                                 "Expand all help");
    _run.app().add_flag("--random",
                        "Some random flag");

    cli::command start = _run.app().add_subcommand("start",
                                                   "A great subcommand");
    cli::command stop  = _run.app().add_subcommand(
                             "stop",
                             "Do you really want to stop?");
    std::string  file;

    _run.app().require_subcommand();
    start->add_option("-f,--file",
                      file,
                      "File name");

    cli::option s = stop->add_flag("-c,--count",
                                   "Counter");

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "Working on --file from start: ",
        file);
    say(_run,
        "Working on --count from stop: ",
        s->count(),
        ", direct count: ",
        stop->count("--count"));
    say(_run,
        "Count of --random flag: ",
        _run.app().count("--random"));

    for (auto subcom : _run.app().get_subcommands())
    {
        say(_run,
            "Subcommand: ",
            subcom->get_name());
    }

    return;
}

// SUBCOMMANDS_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_subcommands.
const step_fn SUBCOMMANDS_STEPS[] =
{
    &example_step<&example_subcommands, 0u>,
    &example_step<&example_subcommands, 1u>,
    &example_step<&example_subcommands, 2u>
};

// SUBCOMMANDS_COUNT
//   constant: how many steps SUBCOMMANDS_STEPS holds.
const std::size_t SUBCOMMANDS_COUNT =
    sizeof(SUBCOMMANDS_STEPS) / sizeof(SUBCOMMANDS_STEPS[0]);

// 1.4    subcom_partitioned
//------------------------------------------------------------------------------

// SUBCOM_PARTITIONED_RUNS
//   constant: subcom_partitioned's arguments: CLI11's
// subcom_partitioned_none, _all and _help.
const invocation SUBCOM_PARTITIONED_RUNS[] =
{
    { { D_NULLPTR } },
    { { "--file", "this", "--count", "--count", "-d", "1.2", D_NULLPTR } },
    { { "--help", D_NULLPTR } }
};

/**
 * @brief subcom_partitioned.cpp: the options in two nameless subcommands,
 *        added with add_subcommand("", description); the values and
 *        counts written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of SUBCOM_PARTITIONED_RUNS.
 */
void
example_subcom_partitioned(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = SUBCOM_PARTITIONED_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    std::string       file;
    int               counted = 0;
    double            value   = 0.0;

    _run.app().description("K3Pi goofit fitter");

    cli::command imp_opt   = _run.app().add_subcommand("",
                                                       "Important");
    cli::option  opt       = imp_opt->add_option("-f,--file,file",
                                                 file,
                                                 "File name")
                                    ->required();
    cli::option  copt      = imp_opt->add_flag("-c,--count",
                                               counted,
                                               "Counter")
                                    ->required();
    cli::command other_opt = _run.app().add_subcommand("",
                                                       "Other");

    other_opt->add_option("-d,--double",
                          value,
                          "Some Value");

    // the parse, its ParseError ended by exit()
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "Working on file: ",
        file,
        ", direct count: ",
        imp_opt->count("--file"),
        ", opt count: ",
        opt->count());
    say(_run,
        "Working on count: ",
        counted,
        ", direct count: ",
        imp_opt->count("--count"),
        ", opt count: ",
        copt->count());
    say(_run,
        "Some value: ",
        value);

    return;
}

// SUBCOM_PARTITIONED_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_subcom_partitioned.
const step_fn SUBCOM_PARTITIONED_STEPS[] =
{
    &example_step<&example_subcom_partitioned, 0u>,
    &example_step<&example_subcom_partitioned, 1u>,
    &example_step<&example_subcom_partitioned, 2u>
};

// SUBCOM_PARTITIONED_COUNT
//   constant: how many steps SUBCOM_PARTITIONED_STEPS holds.
const std::size_t SUBCOM_PARTITIONED_COUNT =
    sizeof(SUBCOM_PARTITIONED_STEPS) / sizeof(SUBCOM_PARTITIONED_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 1.5    config_app
//------------------------------------------------------------------------------

// CONFIG_APP_RUNS
//   constant: config_app's arguments: CLI11's config_app1 to config_app4,
// and the values written.
const invocation CONFIG_APP_RUNS[] =
{
    { { "-p", D_NULLPTR } },
    { { "-p", "-f", "/", D_NULLPTR } },
    { { "-f", "", "-p", D_NULLPTR } },
    { { "-f", "/", "-p", D_NULLPTR } },
    { { "-f", "data", "--flag", "-c", "3", D_NULLPTR } }
};

/**
 * @brief config_app.cpp: defaults captured, and the configuration written
 *        when asked for, through a format quoting with `"` alone; else the
 *        values and counts written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of CONFIG_APP_RUNS.
 */
void
example_config_app(
    run&        _run,
    std::size_t _index
)
{
    static cli::config_format quoted;
    const invocation&         given = CONFIG_APP_RUNS[_index];
    const std::size_t         count = begin_example(_run,
                                                    given);
    std::string               file;
    int                       counted = 0;
    int                       v       = 0;
    double                    value   = 0.0;

    _run.app().description("configuration print example");
    _run.app().add_flag("-p,--print",
                        "Print configuration and exit")
              ->configurable(false);

    cli::option opt  = _run.app().add_option("-f,--file,file",
                                             file,
                                             "File name")
                                 ->capture_default_str()
                                 ->run_callback_for_default();
    cli::option copt = _run.app().add_option("-c,--count",
                                             counted,
                                             "Counter")
                                 ->capture_default_str();
    cli::option flag = _run.app().add_flag(
                           "--flag",
                           v,
                           "Some flag that can be passed multiple times")
                                 ->capture_default_str();

    _run.app().add_option("-d,--double",
                          value,
                          "Some Value")
              ->capture_default_str();
    quoted.quote_character('"',
                           '"');
    _run.app().config_formatter(quoted);

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    // the configuration printed, and the program ended
    if (_run.app().get_option("--print")->as<bool>())
    {
        say(_run,
            "config [",
            escape(_run.app().config_to_str(true,
                                            false)),
            "]");

        return;
    }

    say(_run,
        "Working on file: ",
        file,
        ", direct count: ",
        _run.app().count("--file"),
        ", opt count: ",
        opt->count());
    say(_run,
        "Working on count: ",
        counted,
        ", direct count: ",
        _run.app().count("--count"),
        ", opt count: ",
        copt->count());
    say(_run,
        "Received flag: ",
        v,
        " (",
        flag->count(),
        ") times");
    say(_run,
        "Some value: ",
        value);

    return;
}

// CONFIG_APP_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_config_app.
const step_fn CONFIG_APP_STEPS[] =
{
    &example_step<&example_config_app, 0u>,
    &example_step<&example_config_app, 1u>,
    &example_step<&example_config_app, 2u>,
    &example_step<&example_config_app, 3u>,
    &example_step<&example_config_app, 4u>
};

// CONFIG_APP_COUNT
//   constant: how many steps CONFIG_APP_STEPS holds.
const std::size_t CONFIG_APP_COUNT =
    sizeof(CONFIG_APP_STEPS) / sizeof(CONFIG_APP_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// 1.6    option_groups
//------------------------------------------------------------------------------

// OPTION_GROUPS_RUNS
//   constant: option_groups's arguments: CLI11's option_groups_missing,
// _extra and _extra2, and one that passes.
const invocation OPTION_GROUPS_RUNS[] =
{
    { { D_NULLPTR } },
    { { "--csv", "--binary", D_NULLPTR } },
    { { "--csv", "--address", "192.168.1.1", "-o", "test.out",
        D_NULLPTR } },
    { { "--human", "-o", "out.txt", D_NULLPTR } }
};

/**
 * @brief option_groups.cpp: two option groups, one requiring exactly one
 *        option and the other at most one; the choice written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of OPTION_GROUPS_RUNS.
 */
void
example_option_groups(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given  = OPTION_GROUPS_RUNS[_index];
    const std::size_t count  = begin_example(_run,
                                             given);
    bool              csv    = false;
    bool              human  = false;
    bool              binary = false;
    std::string       file_loc;
    std::string       network_address;

    _run.app().description("data output specification");
    _run.app().set_help_all_flag("--help-all",
                                 "Expand all help");

    cli::option_group format = _run.app().add_option_group(
                                   "output_format",
                                   "formatting type for output");
    cli::option_group target = _run.app().add_option_group(
                                   "output target",
                                   "target location for the output");

    format->add_flag("--csv",
                     csv,
                     "specify the output in csv format");
    format->add_flag("--human",
                     human,
                     "specify the output in human readable text format");
    format->add_flag("--binary",
                     binary,
                     "specify the output in binary format");
    format->require_option(1);
    target->add_option("-o,--file",
                       file_loc,
                       "specify the file location of the output");
    target->add_option("--address",
                       network_address,
                       "specify a network address to send the file");
    target->require_option(0u,
                           1u);

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    const std::string format_type = (csv)
                                    ? std::string("CSV")
                                    : ((human) ? "human readable"
                                               : "binary");

    say(_run,
        "Selected ",
        format_type,
        " format");

    // where the output goes
    if (!file_loc.empty())
    {
        say(_run,
            " sent to file ",
            file_loc);
    }
    else if (!network_address.empty())
    {
        say(_run,
            " sent over network to ",
            network_address);
    }
    else
    {
        say(_run,
            " sent to std::cout");
    }

    return;
}

// OPTION_GROUPS_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_option_groups.
const step_fn OPTION_GROUPS_STEPS[] =
{
    &example_step<&example_option_groups, 0u>,
    &example_step<&example_option_groups, 1u>,
    &example_step<&example_option_groups, 2u>,
    &example_step<&example_option_groups, 3u>
};

// OPTION_GROUPS_COUNT
//   constant: how many steps OPTION_GROUPS_STEPS holds.
const std::size_t OPTION_GROUPS_COUNT =
    sizeof(OPTION_GROUPS_STEPS) / sizeof(OPTION_GROUPS_STEPS[0]);

// 1.7    positional_arity
//------------------------------------------------------------------------------

// POSITIONAL_ARITY_RUNS
//   constant: positional_arity's arguments: CLI11's positional_arity1 to 3
// and positional_arity_fail.
const invocation POSITIONAL_ARITY_RUNS[] =
{
    { { "one", D_NULLPTR } },
    { { "one", "two", D_NULLPTR } },
    { { "1", "2", "one", D_NULLPTR } },
    { { "1", "one", "two", D_NULLPTR } }
};

/**
 * @brief positional_arity.cpp: two groups of positionals, the numbers'
 *        disabled by a pre-parse callback when two arguments or fewer are
 *        given; the values written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of POSITIONAL_ARITY_RUNS.
 */
void
example_positional_arity(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = POSITIONAL_ARITY_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    int               num1  = -1;
    int               num2  = -1;
    std::string       file1;
    std::string       file2;

    _run.app().description("test for positional arity");

    cli::option_group numbers = _run.app().add_option_group(
                                    "numbers",
                                    "specify key numbers");
    cli::option_group files   = _run.app().add_option_group(
                                    "files",
                                    "specify files");

    numbers->add_option("num1",
                        num1,
                        "first number");
    numbers->add_option("num2",
                        num2,
                        "second number");
    files->add_option("file1",
                      file1,
                      "first file")
         ->required();
    files->add_option("file2",
                      file2,
                      "second file");
    _run.app().preparse_callback([numbers](std::size_t _arity) mutable {
        // the numbers taken only when there are enough arguments
        if (_arity <= 2u)
        {
            numbers->disabled();
        }
        else
        {
            numbers->disabled(false);
        }
    });

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    // the numbers given
    if (num1 != -1)
    {
        say(_run,
            "Num1 = ",
            num1);
    }

    if (num2 != -1)
    {
        say(_run,
            "Num2 = ",
            num2);
    }

    say(_run,
        "File 1 = ",
        file1);

    // the second file given
    if (!file2.empty())
    {
        say(_run,
            "File 2 = ",
            file2);
    }

    return;
}

// POSITIONAL_ARITY_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_positional_arity.
const step_fn POSITIONAL_ARITY_STEPS[] =
{
    &example_step<&example_positional_arity, 0u>,
    &example_step<&example_positional_arity, 1u>,
    &example_step<&example_positional_arity, 2u>,
    &example_step<&example_positional_arity, 3u>
};

// POSITIONAL_ARITY_COUNT
//   constant: how many steps POSITIONAL_ARITY_STEPS holds.
const std::size_t POSITIONAL_ARITY_COUNT =
    sizeof(POSITIONAL_ARITY_STEPS) / sizeof(POSITIONAL_ARITY_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 1.8    positional_validation
//------------------------------------------------------------------------------

// POSITIONAL_VALIDATION_RUNS
//   constant: positional_validation's arguments: CLI11's
// positional_validation1 to 4.
const invocation POSITIONAL_VALIDATION_RUNS[] =
{
    { { "one", D_NULLPTR } },
    { { "one", "1", "2", "two", D_NULLPTR } },
    { { "1", "2", "one", D_NULLPTR } },
    { { "1", "one", "two", "2", D_NULLPTR } }
};

/**
 * @brief positional_validation.cpp: positionals that take a value only
 *        when it passes their validator; the values written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of POSITIONAL_VALIDATION_RUNS.
 */
void
example_positional_validation(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = POSITIONAL_VALIDATION_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    int               num1  = -1;
    int               num2  = -1;
    std::string       file1;
    std::string       file2;

    _run.app().description("test for positional validation");
    _run.app().add_option("num1",
                          num1,
                          "first number")
              ->check(cli::number());
    _run.app().add_option("num2",
                          num2,
                          "second number")
              ->check(cli::number());
    _run.app().add_option("file1",
                          file1,
                          "first file")
              ->required();
    _run.app().add_option("file2",
                          file2,
                          "second file");
    _run.app().validate_positionals();

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    // the numbers given
    if (num1 != -1)
    {
        say(_run,
            "Num1 = ",
            num1);
    }

    if (num2 != -1)
    {
        say(_run,
            "Num2 = ",
            num2);
    }

    say(_run,
        "File 1 = ",
        file1);

    // the second file given
    if (!file2.empty())
    {
        say(_run,
            "File 2 = ",
            file2);
    }

    return;
}

// POSITIONAL_VALIDATION_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_positional_validation.
const step_fn POSITIONAL_VALIDATION_STEPS[] =
{
    &example_step<&example_positional_validation, 0u>,
    &example_step<&example_positional_validation, 1u>,
    &example_step<&example_positional_validation, 2u>,
    &example_step<&example_positional_validation, 3u>
};

// POSITIONAL_VALIDATION_COUNT
//   constant: how many steps POSITIONAL_VALIDATION_STEPS holds.
const std::size_t POSITIONAL_VALIDATION_COUNT =
    sizeof(POSITIONAL_VALIDATION_STEPS) /
    sizeof(POSITIONAL_VALIDATION_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// 1.9    date_validator
//------------------------------------------------------------------------------

// DATE_VALIDATOR_RUNS
//   constant: date_validator's arguments: CLI11's date_validator1 and 2.
const invocation DATE_VALIDATOR_RUNS[] =
{
    { { "--time", "2025-08-14T14:30:00", D_NULLPTR } },
    { { "--time", "8/17/2025T14:30:00", D_NULLPTR } }
};

/**
 * @brief date_validator.cpp: an option checked by a validator made from a
 *        function, which reads the value as an ISO 8601 time; the value
 *        written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of DATE_VALIDATOR_RUNS.
 */
void
example_date_validator(
    run&        _run,
    std::size_t _index
)
{
    const invocation&    given   = DATE_VALIDATOR_RUNS[_index];
    const std::size_t    count   = begin_example(_run,
                                                 given);
    const cli::validator iso8601(
        [](std::string& _input) {
            std::tm            time = std::tm();
            std::istringstream read(_input);

            read >> std::get_time(&time,
                                  "%Y-%m-%dT%H:%M:%S");

            return (read.fail())
                   ? std::string("Failed to parse time string")
                   : std::string();
        },
        "datetime[%Y-%m-%dT%H:%M:%S]");
    std::string          value;

    _run.app().description("custom validator testing");
    _run.app().add_option("--time",
                          value,
                          "enter a date in iso8601 format")
              ->check(iso8601)
              ->required();

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "date given = ",
        value);

    return;
}

// DATE_VALIDATOR_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_date_validator.
const step_fn DATE_VALIDATOR_STEPS[] =
{
    &example_step<&example_date_validator, 0u>,
    &example_step<&example_date_validator, 1u>
};

// DATE_VALIDATOR_COUNT
//   constant: how many steps DATE_VALIDATOR_STEPS holds.
const std::size_t DATE_VALIDATOR_COUNT =
    sizeof(DATE_VALIDATOR_STEPS) / sizeof(DATE_VALIDATOR_STEPS[0]);

// 1.10   custom_validator
//------------------------------------------------------------------------------

// delta_range
//   struct: custom_validator's range: a centre and the width either side of
// it, which the options change as they are parsed.
struct delta_range
{
    int center_point;
    int delta;
};

// CUSTOM_VALIDATOR_RUNS
//   constant: custom_validator's arguments: CLI11's custom_validator1 and 2.
const invocation CUSTOM_VALIDATOR_RUNS[] =
{
    { { "--number", "12", D_NULLPTR } },
    { { "--number", "12", "--center", "10", "--range", "3", D_NULLPTR } }
};

/**
 * @brief custom_validator.cpp: an option checked against a range that two
 *        other options move as they are parsed; the value and the range
 *        written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of CUSTOM_VALIDATOR_RUNS.
 */
void
example_custom_validator(
    run&        _run,
    std::size_t _index
)
{
    const invocation&                  given = CUSTOM_VALIDATOR_RUNS[_index];
    const std::size_t                  count = begin_example(_run,
                                                             given);
    const std::shared_ptr<delta_range> range =
        std::make_shared<delta_range>();
    const cli::validator               within(
        [range](std::string& _value) {
            std::istringstream read(_value);
            int                value = 0;
            const bool         whole = ( (read >> value) &&
                                         (read.peek() ==
                                          std::char_traits<char>::eof()) );

            return ( (whole)                                         &&
                     (value >= (range->center_point - range->delta)) &&
                     (value <= (range->center_point + range->delta)) )
                   ? std::string()
                   : std::string("value not within range");
        },
        "RANGE");
    std::string                        value;

    range->center_point = 7;
    range->delta        = 3;
    _run.app().description("custom range validator");
    _run.app().add_option("--number",
                          value,
                          "enter value in the related range")
              ->check(within)
              ->required();
    _run.app().add_option_function<int>(
                  "--center",
                  [range](const int& _new_center) {
                      range->center_point = _new_center;
                  })
              ->trigger_on_parse();
    _run.app().add_option_function<int>(
                  "--range",
                  [range](const int& _new_range) {
                      range->delta = _new_range;
                  })
              ->trigger_on_parse();

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "number ",
        value,
        " in range = ",
        range->center_point,
        " +/- ",
        range->delta);

    return;
}

// CUSTOM_VALIDATOR_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_custom_validator.
const step_fn CUSTOM_VALIDATOR_STEPS[] =
{
    &example_step<&example_custom_validator, 0u>,
    &example_step<&example_custom_validator, 1u>
};

// CUSTOM_VALIDATOR_COUNT
//   constant: how many steps CUSTOM_VALIDATOR_STEPS holds.
const std::size_t CUSTOM_VALIDATOR_COUNT =
    sizeof(CUSTOM_VALIDATOR_STEPS) / sizeof(CUSTOM_VALIDATOR_STEPS[0]);

// 1.11   shapes
//------------------------------------------------------------------------------

// SHAPES_RUNS
//   constant: shapes's arguments: CLI11's shapes_all, and the help.
const invocation SHAPES_RUNS[] =
{
    { { "circle", "4.4", "circle", "10.7", "rectangle", "4", "4", "circle",
        "2.3", "triangle", "4.5", "++", "rectangle", "2.1", "++", "circle",
        "234.675", D_NULLPTR } },
    { { "--help-all", D_NULLPTR } }
};

/**
 * @brief shapes.cpp: subcommands whose callbacks run as each is parsed,
 *        each drawing its shape.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of SHAPES_RUNS.
 */
void
example_shapes(
    run&        _run,
    std::size_t _index
)
{
    const invocation&   given          = SHAPES_RUNS[_index];
    const std::size_t   count          = begin_example(_run,
                                                       given);
    double              radius         = 0.0;
    int                 circle_counter = 0;
    double              edge1          = 0.0;
    double              edge2          = 0.0;
    int                 rect_counter   = 0;
    std::vector<double> sides;
    int                 tri_counter    = 0;

    _run.app().description("load shapes");
    _run.app().set_help_all_flag("--help-all");

    cli::command circle = _run.app().add_subcommand("circle",
                                                    "draw a circle")
                                    ->immediate_callback();

    circle->callback([&_run, &radius, &circle_counter] {
        ++circle_counter;
        say(_run,
            "circle",
            circle_counter,
            " with radius ",
            radius);
    });
    circle->add_option("radius",
                       radius,
                       "the radius of the circle")
          ->required();

    cli::command rect = _run.app().add_subcommand("rectangle",
                                                  "draw a rectangle")
                                  ->immediate_callback();

    rect->callback([&_run, &edge1, &edge2, &rect_counter] {
        ++rect_counter;

        // a square's second edge its first
        if (edge2 == 0)
        {
            edge2 = edge1;
        }

        say(_run,
            "rectangle",
            rect_counter,
            " with edges [",
            edge1,
            ',',
            edge2,
            "]");
        edge2 = 0;
    });
    rect->add_option("edge1",
                     edge1,
                     "the first edge length of the rectangle")
        ->required();
    rect->add_option("edge2",
                     edge2,
                     "the second edge length of the rectangle");

    cli::command tri = _run.app().add_subcommand("triangle",
                                                 "draw a rectangle")
                                 ->immediate_callback();

    tri->callback([&_run, &sides, &tri_counter] {
        std::ostringstream joined;

        ++tri_counter;

        // CLI::detail::join's text: the sides, with commas between
        for (std::size_t index = 0u; index < sides.size(); ++index)
        {
            joined << ((index != 0u) ? "," : "") << sides[index];
        }

        say(_run,
            "triangle",
            tri_counter,
            " with sides [",
            joined.str(),
            "]");
    });
    tri->add_option("sides",
                    sides,
                    "the side lengths of the triangle");

    // CLI11_PARSE
    (void)_run.parse_as_main(given.args,
                             count);

    return;
}

// SHAPES_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_shapes.
const step_fn SHAPES_STEPS[] =
{
    &example_step<&example_shapes, 0u>,
    &example_step<&example_shapes, 1u>
};

// SHAPES_COUNT
//   constant: how many steps SHAPES_STEPS holds.
const std::size_t SHAPES_COUNT =
    sizeof(SHAPES_STEPS) / sizeof(SHAPES_STEPS[0]);

// 1.12   ranges
//------------------------------------------------------------------------------

// RANGES_RUNS
//   constant: ranges's arguments: CLI11's ranges_range, _minmax and _error.
const invocation RANGES_RUNS[] =
{
    { { "--range", "1", "2", "3", D_NULLPTR } },
    { { "--min", "2", "--max", "3", D_NULLPTR } },
    { { "--min", "2", "--max", "3", "--step", "1", "--range", "1", "2",
        "3", D_NULLPTR } }
};

/**
 * @brief ranges.cpp: a range given as two or three values, or by an option
 *        group of its parts, exactly one of the two required; the range
 *        written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of RANGES_RUNS.
 */
void
example_ranges(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given   = RANGES_RUNS[_index];
    const std::size_t count   = begin_example(_run,
                                              given);
    std::vector<int>  range;
    int               minimum = 0;
    int               maximum = 0;
    int               step    = 1;

    _run.app().description("App to demonstrate exclusionary option groups.");
    _run.app().add_option("--range,-R",
                          range,
                          "A range")
              ->expected(-2);

    cli::option_group ogroup = _run.app().add_option_group(
                                   "min_max_step",
                                   "set the min max and step");

    ogroup->add_option("--min,-m",
                       minimum,
                       "The minimum")
          ->required();
    ogroup->add_option("--max,-M",
                       maximum,
                       "The maximum")
          ->required();
    ogroup->add_option("--step,-s",
                       step,
                       "The step")
          ->capture_default_str();
    _run.app().require_option(1);

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    // the range's parts, given whole
    if (!range.empty())
    {
        if (range.size() == 2u)
        {
            minimum = range[0];
            maximum = range[1];
        }

        if (range.size() >= 3u)
        {
            step    = range[0];
            minimum = range[1];
            maximum = range[2];
        }
    }

    say(_run,
        "range is [",
        minimum,
        ':',
        step,
        ':',
        maximum,
        "]");

    return;
}

// RANGES_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_ranges.
const step_fn RANGES_STEPS[] =
{
    &example_step<&example_ranges, 0u>,
    &example_step<&example_ranges, 1u>,
    &example_step<&example_ranges, 2u>
};

// RANGES_COUNT
//   constant: how many steps RANGES_STEPS holds.
const std::size_t RANGES_COUNT =
    sizeof(RANGES_STEPS) / sizeof(RANGES_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// 1.13   validators
//------------------------------------------------------------------------------

// VALIDATORS_RUNS
//   constant: validators's arguments: CLI11's validators_help, _file and
// _plain, and one that passes.
const invocation VALIDATORS_RUNS[] =
{
    { { "--help", D_NULLPTR } },
    { { "--file", "nonex.xxx", D_NULLPTR } },
    { { "--value", "9", D_NULLPTR } },
    { { "--value", "4", D_NULLPTR } }
};

/**
 * @brief validators.cpp: an existing file and a value in a range.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of VALIDATORS_RUNS.
 */
void
example_validators(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given   = VALIDATORS_RUNS[_index];
    const std::size_t count   = begin_example(_run,
                                              given);
    std::string       file;
    int               counted = 0;

    _run.app().description("Validator checker");
    _run.app().add_option("-f,--file,file",
                          file,
                          "File name")
              ->check(cli::existing_file());
    _run.app().add_option("-v,--value",
                          counted,
                          "Value in range")
              ->check(cli::range(3,
                                 6));

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "Try printing help or failing the validator");

    return;
}

// VALIDATORS_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_validators.
const step_fn VALIDATORS_STEPS[] =
{
    &example_step<&example_validators, 0u>,
    &example_step<&example_validators, 1u>,
    &example_step<&example_validators, 2u>,
    &example_step<&example_validators, 3u>
};

// VALIDATORS_COUNT
//   constant: how many steps VALIDATORS_STEPS holds.
const std::size_t VALIDATORS_COUNT =
    sizeof(VALIDATORS_STEPS) / sizeof(VALIDATORS_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

// 1.14   groups
//------------------------------------------------------------------------------

// GROUPS_RUNS
//   constant: groups's arguments: CLI11's groups_none and groups_all, and
// the help.
const invocation GROUPS_RUNS[] =
{
    { { D_NULLPTR } },
    { { "--file", "this", "--count", "--count", "-d", "1.2", D_NULLPTR } },
    { { "--help", D_NULLPTR } }
};

/**
 * @brief groups.cpp: options in groups of their own, two required; the
 *        values and counts written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of GROUPS_RUNS.
 */
void
example_groups(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given   = GROUPS_RUNS[_index];
    const std::size_t count   = begin_example(_run,
                                              given);
    std::string       file;
    int               counted = 0;
    double            value   = 0.0;

    _run.app().description("K3Pi goofit fitter");

    cli::option opt  = _run.app().add_option("-f,--file,file",
                                             file,
                                             "File name")
                                 ->required()
                                 ->group("Important");
    cli::option copt = _run.app().add_flag("-c,--count",
                                           counted,
                                           "Counter")
                                 ->required()
                                 ->group("Important");

    _run.app().add_option("-d,--double",
                          value,
                          "Some Value")
              ->group("Other");

    // the parse, its ParseError ended by exit()
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "Working on file: ",
        file,
        ", direct count: ",
        _run.app().count("--file"),
        ", opt count: ",
        opt->count());
    say(_run,
        "Working on count: ",
        counted,
        ", direct count: ",
        _run.app().count("--count"),
        ", opt count: ",
        copt->count());
    say(_run,
        "Some value: ",
        value);

    return;
}

// GROUPS_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_groups.
const step_fn GROUPS_STEPS[] =
{
    &example_step<&example_groups, 0u>,
    &example_step<&example_groups, 1u>,
    &example_step<&example_groups, 2u>
};

// GROUPS_COUNT
//   constant: how many steps GROUPS_STEPS holds.
const std::size_t GROUPS_COUNT =
    sizeof(GROUPS_STEPS) / sizeof(GROUPS_STEPS[0]);

// 1.15   inter_argument_order
//------------------------------------------------------------------------------

// INTER_ARGUMENT_ORDER_RUNS
//   constant: inter_argument_order's arguments: CLI11's
// inter_argument_order.
const invocation INTER_ARGUMENT_ORDER_RUNS[] =
{
    { { "--foo", "1", "2", "3", "--x", "--bar", "4", "5", "6", "--z",
        "--foo", "7", "8", D_NULLPTR } }
};

/**
 * @brief inter_argument_order.cpp: two options taking any number of
 *        values, their values put back in the order given by the parse's
 *        order; the pairs written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of INTER_ARGUMENT_ORDER_RUNS.
 */
void
example_inter_argument_order(
    run&        _run,
    std::size_t _index
)
{
    const invocation&                        given = INTER_ARGUMENT_ORDER_RUNS[
                                                         _index];
    const std::size_t                        count = begin_example(_run,
                                                                   given);
    std::vector<int>                         foos;
    std::vector<int>                         bars;
    std::vector<std::pair<std::string, int>> keyval;

    _run.app().description("An app to practice mixing unlimited arguments, "
                           "but still recover the original order.");

    cli::option foo = _run.app().add_option("--foo,-f",
                                            foos,
                                            "Some unlimited argument");
    cli::option bar = _run.app().add_option("--bar",
                                            bars,
                                            "Some unlimited argument");

    _run.app().add_flag("--z,--x",
                        "Random other flags");

    // the parse, its ParseError ended by exit()
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    std::reverse(foos.begin(),
                 foos.end());
    std::reverse(bars.begin(),
                 bars.end());

    // each value, in the order its option came
    for (auto option : _run.app().parse_order())
    {
        if (option == foo)
        {
            keyval.push_back(std::make_pair(std::string("foo"),
                                            foos.back()));
            foos.pop_back();
        }

        if (option == bar)
        {
            keyval.push_back(std::make_pair(std::string("bar"),
                                            bars.back()));
            bars.pop_back();
        }
    }

    for (const auto& pair : keyval)
    {
        say(_run,
            pair.first,
            " : ",
            pair.second);
    }

    return;
}

// INTER_ARGUMENT_ORDER_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_inter_argument_order.
const step_fn INTER_ARGUMENT_ORDER_STEPS[] =
{
    &example_step<&example_inter_argument_order, 0u>
};

// INTER_ARGUMENT_ORDER_COUNT
//   constant: how many steps INTER_ARGUMENT_ORDER_STEPS holds.
const std::size_t INTER_ARGUMENT_ORDER_COUNT =
    sizeof(INTER_ARGUMENT_ORDER_STEPS) /
    sizeof(INTER_ARGUMENT_ORDER_STEPS[0]);

// 1.16   prefix_command
//------------------------------------------------------------------------------

// PREFIX_COMMAND_RUNS
//   constant: prefix_command's arguments: CLI11's prefix_command.
const invocation PREFIX_COMMAND_RUNS[] =
{
    { { "-v", "3", "2", "1", "--", "other", "one", "two", "3", D_NULLPTR } }
};

/**
 * @brief prefix_command.cpp: values taken until a separator, the rest left
 *        for another command; both written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of PREFIX_COMMAND_RUNS.
 */
void
example_prefix_command(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = PREFIX_COMMAND_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    std::vector<int>  vals;
    std::string       prefix("Prefix");
    std::string       remaining("Remaining commands: ");

    _run.app().description("Prefix command app");
    _run.app().prefix_command(cli::prefix_mode::on);
    _run.app().add_option("--vals,-v",
                          vals)
              ->expected(-1);

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    const std::vector<std::string> more_comms = _run.app().remaining();

    for (int v : vals)
    {
        std::ostringstream piece;

        piece << ": " << v << " ";
        prefix += piece.str();
    }

    for (const auto& com : more_comms)
    {
        remaining += com + " ";
    }

    say(_run,
        prefix);
    say(_run,
        remaining);

    return;
}

// PREFIX_COMMAND_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_prefix_command.
const step_fn PREFIX_COMMAND_STEPS[] =
{
    &example_step<&example_prefix_command, 0u>
};

// PREFIX_COMMAND_COUNT
//   constant: how many steps PREFIX_COMMAND_STEPS holds.
const std::size_t PREFIX_COMMAND_COUNT =
    sizeof(PREFIX_COMMAND_STEPS) / sizeof(PREFIX_COMMAND_STEPS[0]);

// 1.17   minimal
//------------------------------------------------------------------------------

// MINIMAL_RUNS
//   constant: minimal's arguments: CLI11's minimal, and the help.
const invocation MINIMAL_RUNS[] =
{
    { { D_NULLPTR } },
    { { "--help", D_NULLPTR } }
};

/**
 * @brief minimal.cpp: a parser with a description and nothing else.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of MINIMAL_RUNS.
 */
void
example_minimal(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = MINIMAL_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);

    _run.app().description("App description");

    // CLI11_PARSE
    (void)_run.parse_as_main(given.args,
                             count);

    return;
}

// MINIMAL_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_minimal.
const step_fn MINIMAL_STEPS[] =
{
    &example_step<&example_minimal, 0u>,
    &example_step<&example_minimal, 1u>
};

// MINIMAL_COUNT
//   constant: how many steps MINIMAL_STEPS holds.
const std::size_t MINIMAL_COUNT =
    sizeof(MINIMAL_STEPS) / sizeof(MINIMAL_STEPS[0]);

// 1.18   arg_capture
//------------------------------------------------------------------------------

// ARG_CAPTURE_RUNS
//   constant: arg_capture's arguments: CLI11's arg_capture and
// arg_capture2.
const invocation ARG_CAPTURE_RUNS[] =
{
    { { "-v", "27", "--sub", "-v", "13", "--val", "prefix", D_NULLPTR } }
};

/**
 * @brief arg_capture.cpp: a subcommand, with an alias that reads as an
 *        option, that takes every argument after it; the value and those
 *        arguments written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of ARG_CAPTURE_RUNS.
 */
void
example_arg_capture(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = ARG_CAPTURE_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    int               value = 0;
    std::string       after("after Args:");

    _run.app().description("Test App");
    _run.app().add_option("-v",
                          value,
                          "value");

    cli::command subcom = _run.app().add_subcommand("sub",
                                                    "")
                                    ->prefix_command();

    subcom->alias("--sub");

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "value=",
        value);

    for (const auto& aarg : subcom->remaining())
    {
        after += aarg + " ";
    }

    say(_run,
        after);

    return;
}

// ARG_CAPTURE_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_arg_capture.
const step_fn ARG_CAPTURE_STEPS[] =
{
    &example_step<&example_arg_capture, 0u>
};

// ARG_CAPTURE_COUNT
//   constant: how many steps ARG_CAPTURE_STEPS holds.
const std::size_t ARG_CAPTURE_COUNT =
    sizeof(ARG_CAPTURE_STEPS) / sizeof(ARG_CAPTURE_STEPS[0]);

// 1.19   callback_passthrough
//------------------------------------------------------------------------------

// CALLBACK_PASSTHROUGH_RUNS
//   constant: callback_passthrough's arguments: CLI11's
// callback_passthrough1 and 2.
const invocation CALLBACK_PASSTHROUGH_RUNS[] =
{
    { { "--argname", "t2", "--t2", "test", D_NULLPTR } },
    { { "--arg", "EEEK", "--argname", "arg", D_NULLPTR } }
};

/**
 * @brief callback_passthrough.cpp: the arguments left over given, by the
 *        parser's callback, to a parser made there, with an option named by
 *        a value; the value it took written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of CALLBACK_PASSTHROUGH_RUNS.
 */
void
example_callback_passthrough(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = CALLBACK_PASSTHROUGH_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    std::string       arg_name;
    std::string       val;

    _run.app().description("callback_passthrough");
    _run.app().allow_extras();
    _run.app().add_option("--argname",
                          arg_name,
                          "the name of the custom command line argument");
    _run.app().callback([&_run, &val, &arg_name]() {
        // the option named, parsed from what is left
        if (!arg_name.empty())
        {
            cli::arg_parser sub_app;

            sub_app.add_option("--" + arg_name,
                               val,
                               "custom argument option");
            (void)sub_app.parse(_run.app().remaining_for_passthrough());
        }
    });

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "the value is now ",
        val);

    return;
}

// CALLBACK_PASSTHROUGH_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_callback_passthrough.
const step_fn CALLBACK_PASSTHROUGH_STEPS[] =
{
    &example_step<&example_callback_passthrough, 0u>,
    &example_step<&example_callback_passthrough, 1u>
};

// CALLBACK_PASSTHROUGH_COUNT
//   constant: how many steps CALLBACK_PASSTHROUGH_STEPS holds.
const std::size_t CALLBACK_PASSTHROUGH_COUNT =
    sizeof(CALLBACK_PASSTHROUGH_STEPS) /
    sizeof(CALLBACK_PASSTHROUGH_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 1.20   enum and enum_ostream
//------------------------------------------------------------------------------

// level_value
//   enum: enum's levels.
enum class level_value : int
{
    high,
    medium,
    low
};

// shown_level
//   enum: enum_ostream's levels, which write themselves.
enum class shown_level : int
{
    high,
    medium,
    low
};

/**
 * @brief enum_ostream.cpp's operator<<: a level's name, and a note.
 *
 * @param[in,out] _stream  the stream.
 * @param[in]     _level   the level.
 * @return the stream.
 */
std::ostream&
operator<<(
    std::ostream&      _stream,
    const shown_level& _level
)
{
    switch (_level)
    {
        case shown_level::high:
            _stream << "High";
            break;

        case shown_level::medium:
            _stream << "Medium";
            break;

        case shown_level::low:
            _stream << "Low";
            break;
    }

    _stream << " (ft rom custom ostream)";

    return _stream;
}

// ENUM_LEVELS_RUNS
//   constant: enum's arguments: CLI11's enum_pass and enum_fail, and a
// name in another case.
const invocation ENUM_LEVELS_RUNS[] =
{
    { { "-l", "1", D_NULLPTR } },
    { { "-l", "4", D_NULLPTR } },
    { { "--level", "MEDIUM", D_NULLPTR } }
};

/**
 * @brief enum.cpp: an enumeration read by a checked transformation over a
 *        map of names, in any case; the number it holds written, as
 *        CLI11's enums::operator<< writes it.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of ENUM_LEVELS_RUNS.
 */
void
example_enum(
    run&        _run,
    std::size_t _index
)
{
    const invocation&                        given  = ENUM_LEVELS_RUNS[_index];
    const std::size_t                        count  = begin_example(_run,
                                                                    given);
    level_value                              chosen = level_value::low;
    const std::map<std::string, level_value> map    =
    {
        { "high", level_value::high },
        { "medium", level_value::medium },
        { "low", level_value::low }
    };

    _run.app().add_option("-l,--level",
                          chosen,
                          "Level settings")
              ->required()
              ->transform(cli::checked_transformer(map,
                                                   cli::ignore_case));

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "Enum received: ",
        static_cast<int>(chosen));

    return;
}

// ENUM_LEVELS_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_enum.
const step_fn ENUM_LEVELS_STEPS[] =
{
    &example_step<&example_enum, 0u>,
    &example_step<&example_enum, 1u>,
    &example_step<&example_enum, 2u>
};

// ENUM_LEVELS_COUNT
//   constant: how many steps ENUM_LEVELS_STEPS holds.
const std::size_t ENUM_LEVELS_COUNT =
    sizeof(ENUM_LEVELS_STEPS) / sizeof(ENUM_LEVELS_STEPS[0]);

// ENUM_OSTREAM_RUNS
//   constant: enum_ostream's arguments: CLI11's enum_ostream_pass.
const invocation ENUM_OSTREAM_RUNS[] =
{
    { { "--level", "medium", D_NULLPTR } }
};

/**
 * @brief enum_ostream.cpp: as enum.cpp, the level written by its own
 *        operator<<.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of ENUM_OSTREAM_RUNS.
 */
void
example_enum_ostream(
    run&        _run,
    std::size_t _index
)
{
    const invocation&                        given  = ENUM_OSTREAM_RUNS[_index];
    const std::size_t                        count  = begin_example(_run,
                                                                    given);
    shown_level                              chosen = shown_level::low;
    const std::map<std::string, shown_level> map    =
    {
        { "high", shown_level::high },
        { "medium", shown_level::medium },
        { "low", shown_level::low }
    };

    _run.app().add_option("-l,--level",
                          chosen,
                          "Level settings")
              ->required()
              ->transform(cli::checked_transformer(map,
                                                   cli::ignore_case));

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "Enum received: ",
        chosen);

    return;
}

// ENUM_OSTREAM_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_enum_ostream.
const step_fn ENUM_OSTREAM_STEPS[] =
{
    &example_step<&example_enum_ostream, 0u>
};

// ENUM_OSTREAM_COUNT
//   constant: how many steps ENUM_OSTREAM_STEPS holds.
const std::size_t ENUM_OSTREAM_COUNT =
    sizeof(ENUM_OSTREAM_STEPS) / sizeof(ENUM_OSTREAM_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// 1.21   digit_args
//------------------------------------------------------------------------------

// DIGIT_ARGS_RUNS
//   constant: digit_args's arguments: CLI11's digit_args, and flags given.
const invocation DIGIT_ARGS_RUNS[] =
{
    { { "-h", D_NULLPTR } },
    { { "-3", D_NULLPTR } },
    { { "-2", "-7", D_NULLPTR } }
};

/**
 * @brief digit_args.cpp: flags named by digits, each with a value of its
 *        own; the value written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of DIGIT_ARGS_RUNS.
 */
void
example_digit_args(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = DIGIT_ARGS_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    int               val   = 0;

    _run.app().add_flag("-1{1},-2{2},-3{3},-4{4},-5{5},-6{6}, -7{7}, -8{8}, "
                        "-9{9}",
                        val,
                        "compression level");

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "value = ",
        val);

    return;
}

// DIGIT_ARGS_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_digit_args.
const step_fn DIGIT_ARGS_STEPS[] =
{
    &example_step<&example_digit_args, 0u>,
    &example_step<&example_digit_args, 1u>,
    &example_step<&example_digit_args, 2u>
};

// DIGIT_ARGS_COUNT
//   constant: how many steps DIGIT_ARGS_STEPS holds.
const std::size_t DIGIT_ARGS_COUNT =
    sizeof(DIGIT_ARGS_STEPS) / sizeof(DIGIT_ARGS_STEPS[0]);

// 1.22   modhelp
//------------------------------------------------------------------------------

// MODHELP_RUNS
//   constant: modhelp's arguments: CLI11's modhelp, the value alone, and an
// option it does not have.
const invocation MODHELP_RUNS[] =
{
    { { "-a", "test", "-h", D_NULLPTR } },
    { { "-a", "test", D_NULLPTR } },
    { { "-a", "test", "-b", D_NULLPTR } }
};

/**
 * @brief modhelp.cpp: the help flag replaced by an ordinary flag, so that
 *        the values are read before help is written; the value written,
 *        then help or the failure.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of MODHELP_RUNS.
 */
void
example_modhelp(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = MODHELP_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    std::string       some_option;

    _run.app().description("Modify the help print so that argument values "
                           "are accessible.\nNote that this will not "
                           "shortcut `->required` and other similar "
                           "options.");
    _run.app().set_help_flag();

    cli::option help = _run.app().add_flag("-h,--help",
                                           "Request help");

    _run.app().add_option("-a",
                          some_option,
                          "Some description");

    const bool parsed = _run.parse_quiet(given.args,
                                         count);

    // help asked for, or a failure: the value, then the ending
    if ( (!parsed) ||
         (help->count() != 0u) )
    {
        say(_run,
            "Option -a string in help: ",
            some_option);

        if (parsed)
        {
            say(_run,
                "help [",
                escape(_run.app().help()),
                "]");
        }
        else
        {
            _run.exit();
        }

        return;
    }

    say(_run,
        "Option -a string: ",
        some_option);

    return;
}

// MODHELP_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_modhelp.
const step_fn MODHELP_STEPS[] =
{
    &example_step<&example_modhelp, 0u>,
    &example_step<&example_modhelp, 1u>,
    &example_step<&example_modhelp, 2u>
};

// MODHELP_COUNT
//   constant: how many steps MODHELP_STEPS holds.
const std::size_t MODHELP_COUNT =
    sizeof(MODHELP_STEPS) / sizeof(MODHELP_STEPS[0]);

// 1.23   array_option
//------------------------------------------------------------------------------

// ARRAY_OPTION_RUNS
//   constant: array_option's arguments: CLI11's array_option, and the help.
const invocation ARRAY_OPTION_RUNS[] =
{
    { { "--a", "1", "2", D_NULLPTR } },
    { { "--help", D_NULLPTR } }
};

/**
 * @brief array_option.cpp: an option bound to a std::array, its default
 *        captured; `pass` written, and the values.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of ARRAY_OPTION_RUNS.
 */
void
example_array_option(
    run&        _run,
    std::size_t _index
)
{
    const invocation&  given = ARRAY_OPTION_RUNS[_index];
    const std::size_t  count = begin_example(_run,
                                             given);
    std::array<int, 2> a     = {{ 0, 1 }};

    _run.app().description("My app");
    _run.app().add_option("--a",
                          a,
                          "an array")
              ->capture_default_str();

    // the parse
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "pass ",
        a[0],
        " ",
        a[1]);

    return;
}

// ARRAY_OPTION_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_array_option.
const step_fn ARRAY_OPTION_STEPS[] =
{
    &example_step<&example_array_option, 0u>,
    &example_step<&example_array_option, 1u>
};

// ARRAY_OPTION_COUNT
//   constant: how many steps ARRAY_OPTION_STEPS holds.
const std::size_t ARRAY_OPTION_COUNT =
    sizeof(ARRAY_OPTION_STEPS) / sizeof(ARRAY_OPTION_STEPS[0]);

// 1.24   subcom_in_files
//------------------------------------------------------------------------------

// subcommand_a_options
//   struct: subcom_in_files's SubcommandAOptions: what subcommand_a's
// options are bound to.
struct subcommand_a_options
{
    std::string file;
    bool        with_foo;
};

/**
 * @brief subcom_in_files's run_subcommand_a: what subcommand_a does.
 *
 * @param[in,out] _run  the run.
 * @param[in]     _opt  the options.
 */
void
run_subcommand_a(
    run&                        _run,
    const subcommand_a_options& _opt
)
{
    say(_run,
        "Working on file: ",
        _opt.file);

    // the flag given
    if (_opt.with_foo)
    {
        say(_run,
            "Using foo!");
    }

    return;
}

/**
 * @brief subcom_in_files's setup_subcommand_a: subcommand_a, its options
 *        bound to a structure its callback keeps alive.
 *
 * @param[in,out] _run  the run, whose parser takes the subcommand.
 */
void
setup_subcommand_a(
    run& _run
)
{
    const std::shared_ptr<subcommand_a_options> opt =
        std::make_shared<subcommand_a_options>();
    cli::command                                sub =
        _run.app().add_subcommand("subcommand_a",
                                  "performs subcommand a");

    opt->with_foo = false;
    sub->add_option("-f,--file",
                    opt->file,
                    "File name")
       ->required();
    sub->add_flag("--with-foo",
                  opt->with_foo,
                  "Counter");
    sub->callback([opt, &_run]() {
        run_subcommand_a(_run,
                         *opt);
    });

    return;
}

// SUBCOM_IN_FILES_RUNS
//   constant: subcom_in_files's arguments: CLI11's subcom_in_files, and no
// subcommand.
const invocation SUBCOM_IN_FILES_RUNS[] =
{
    { { "subcommand_a", "-f", "this.txt", "--with-foo", D_NULLPTR } },
    { { D_NULLPTR } }
};

/**
 * @brief subcom_in_files's subcommand_main.cpp: a subcommand set up apart,
 *        one required.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of SUBCOM_IN_FILES_RUNS.
 */
void
example_subcom_in_files(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = SUBCOM_IN_FILES_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);

    _run.app().description("...");
    setup_subcommand_a(_run);
    _run.app().require_subcommand();

    // CLI11_PARSE
    (void)_run.parse_as_main(given.args,
                             count);

    return;
}

// SUBCOM_IN_FILES_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_subcom_in_files.
const step_fn SUBCOM_IN_FILES_STEPS[] =
{
    &example_step<&example_subcom_in_files, 0u>,
    &example_step<&example_subcom_in_files, 1u>
};

// SUBCOM_IN_FILES_COUNT
//   constant: how many steps SUBCOM_IN_FILES_STEPS holds.
const std::size_t SUBCOM_IN_FILES_COUNT =
    sizeof(SUBCOM_IN_FILES_STEPS) / sizeof(SUBCOM_IN_FILES_STEPS[0]);

// 1.25   formatter
//------------------------------------------------------------------------------

// my_formatter
//   class: formatter.cpp's MyFormatter: each option's part after its names
// written as ` OPTION`.
class my_formatter : public cli::formatter
{
public:
    /**
     * @brief Writes an option's part after its names.
     *
     * @param[in]     _option  the option; unused.
     * @param[in,out] _out     the help being built.
     * @return the append's status.
     */
    virtual cli::status
    make_option_opts(
        const cli::option& _option,
        cli::buffer&       _out
    ) const D_OVERRIDE
    {
        (void)_option;

        return _out.append(cli::text(" OPTION"));
    }
};

// FORMATTER_RUNS
//   constant: formatter's arguments: the help it was made to show, all of
// it, and nothing.
const invocation FORMATTER_RUNS[] =
{
    { { "-h", D_NULLPTR } },
    { { "--help-all", D_NULLPTR } },
    { { D_NULLPTR } }
};

/**
 * @brief formatter.cpp: help through a formatter of the program's own.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of FORMATTER_RUNS.
 */
void
example_formatter(
    run&        _run,
    std::size_t _index
)
{
    static my_formatter fmt;
    const invocation&   given = FORMATTER_RUNS[_index];
    const std::size_t   count = begin_example(_run,
                                              given);

    _run.app().set_help_all_flag("--help-all",
                                 "Show all help");
    fmt.column_width(15u);
    _run.app().formatter(fmt);
    _run.app().add_flag("--flag",
                        "This is a flag");

    cli::command sub1 = _run.app().add_subcommand("one",
                                                  "Description One");

    sub1->add_flag("--oneflag",
                   "Some flag");

    cli::command sub2 = _run.app().add_subcommand("two",
                                                  "Description Two");

    sub2->add_flag("--twoflag",
                   "Some other flag");

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "This app was meant to show off the formatter, run with -h");

    return;
}

// FORMATTER_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_formatter.
const step_fn FORMATTER_STEPS[] =
{
    &example_step<&example_formatter, 0u>,
    &example_step<&example_formatter, 1u>,
    &example_step<&example_formatter, 2u>
};

// FORMATTER_COUNT
//   constant: how many steps FORMATTER_STEPS holds.
const std::size_t FORMATTER_COUNT =
    sizeof(FORMATTER_STEPS) / sizeof(FORMATTER_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// 1.26   nested
//------------------------------------------------------------------------------

// NESTED_RUNS
//   constant: nested's arguments: the help with everything expanded, a path
// that exists, the default file, and a file that does not exist.
const invocation NESTED_RUNS[] =
{
    { { "--help-all", D_NULLPTR } },
    { { "camera", "mock", "-p", ".", D_NULLPTR } },
    { { "camera", "mvcamera", D_NULLPTR } },
    { { "camera", "mvcamera", "-c", "missing.json", D_NULLPTR } },
    { { "camera", "mock", D_NULLPTR } }
};

/**
 * @brief nested.cpp: subcommands of a subcommand, at most one required,
 *        their options checked against the file system.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of NESTED_RUNS.
 */
void
example_nested(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given                = NESTED_RUNS[_index];
    const std::size_t count                = begin_example(_run,
                                                           given);
    std::string       mvcamera_config_file = "mvcamera_config.json";
    std::string       mock_camera_path;

    _run.app().description("Vision Application");
    _run.app().set_help_all_flag("--help-all",
                                 "Expand all help");
    _run.app().add_flag("--version",
                        "Get version");

    cli::command camera_app = _run.app().add_subcommand(
                                  "camera",
                                  "Configure the app camera");

    camera_app->require_subcommand(0u,
                                   1u);

    cli::command mvcamera_app = camera_app->add_subcommand(
                                    "mvcamera",
                                    "MatrixVision Camera Configuration");

    mvcamera_app->add_option("-c,--config",
                             mvcamera_config_file,
                             "Config filename")
                ->capture_default_str()
                ->check(cli::existing_file());

    cli::command mockcamera_app = camera_app->add_subcommand(
                                      "mock",
                                      "Mock Camera Configuration");

    mockcamera_app->add_option("-p,--path",
                               mock_camera_path,
                               "Path")
                  ->required()
                  ->check(cli::existing_path());

    // CLI11_PARSE
    (void)_run.parse_as_main(given.args,
                             count);

    return;
}

// NESTED_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_nested.
const step_fn NESTED_STEPS[] =
{
    &example_step<&example_nested, 0u>,
    &example_step<&example_nested, 1u>,
    &example_step<&example_nested, 2u>,
    &example_step<&example_nested, 3u>,
    &example_step<&example_nested, 4u>
};

// NESTED_COUNT
//   constant: how many steps NESTED_STEPS holds.
const std::size_t NESTED_COUNT =
    sizeof(NESTED_STEPS) / sizeof(NESTED_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

// 1.27   subcom_help
//------------------------------------------------------------------------------

// SUBCOM_HELP_RUNS
//   constant: subcom_help's arguments: CLI11's subcom_help_normal and
// _reversed, and the subcommand's argument given.
const invocation SUBCOM_HELP_RUNS[] =
{
    { { "sub", "--help", D_NULLPTR } },
    { { "--help", "sub", D_NULLPTR } },
    { { "sub", "value", D_NULLPTR } }
};

/**
 * @brief subcom_help.cpp: a subcommand with a required positional, its
 *        help asked for before and after it; its argument written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of SUBCOM_HELP_RUNS.
 */
void
example_subcom_help(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = SUBCOM_HELP_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    std::string       sub_arg;

    _run.app().description("Demo app");

    cli::command cli_sub = _run.app().add_subcommand("sub",
                                                     "Some subcommand");

    cli_sub->add_option("sub_arg",
                        sub_arg,
                        "Argument for subcommand")
           ->required();

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    // the subcommand used
    if (cli_sub->parsed())
    {
        say(_run,
            "Got: ",
            sub_arg);
    }

    return;
}

// SUBCOM_HELP_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_subcom_help.
const step_fn SUBCOM_HELP_STEPS[] =
{
    &example_step<&example_subcom_help, 0u>,
    &example_step<&example_subcom_help, 1u>,
    &example_step<&example_subcom_help, 2u>
};

// SUBCOM_HELP_COUNT
//   constant: how many steps SUBCOM_HELP_STEPS holds.
const std::size_t SUBCOM_HELP_COUNT =
    sizeof(SUBCOM_HELP_STEPS) / sizeof(SUBCOM_HELP_STEPS[0]);

// 1.28   retired
//------------------------------------------------------------------------------

// RETIRED_RUNS
//   constant: retired's arguments: CLI11's retired_retired_test and
// _test2, the deprecated option, and its replacement.
const invocation RETIRED_RUNS[] =
{
    { { "--retired_option", D_NULLPTR } },
    { { "--retired_option", "567", D_NULLPTR } },
    { { "--deprecate", "1", "2", D_NULLPTR } },
    { { "--not_deprecated", "4", "5", "--retired_option2", "6", D_NULLPTR } }
};

/**
 * @brief retired.cpp: options retired, by name and by handle, and one
 *        deprecated for another; the values written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of RETIRED_RUNS.
 */
void
example_retired(
    run&        _run,
    std::size_t _index
)
{
    const invocation&   given = RETIRED_RUNS[_index];
    const std::size_t   count = begin_example(_run,
                                              given);
    std::vector<int>    x;
    std::pair<int, int> y;
    std::string         line("Retired option example: got --not_deprecated "
                             "values:");

    _run.app().description("example for retired/deprecated options");

    cli::option opt1 = _run.app().add_option("--retired_option2",
                                             x);
    cli::option opt2 = _run.app().add_option("--deprecate",
                                             y);

    _run.app().add_option("--not_deprecated",
                          x);
    cli::retire_option(_run.root(),
                       "--retired_option");
    cli::retire_option(_run.root(),
                       opt1);
    (void)cli::deprecate_option(opt2,
                                "--not_deprecated");

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    // what was received
    if (!x.empty())
    {
        for (auto& xval : x)
        {
            std::ostringstream piece;

            piece << xval << " ";
            line += piece.str();
        }

        say(_run,
            line);
    }
    else if (_run.app().count_all() == 1u)
    {
        say(_run,
            "Retired option example: no arguments received");
    }

    return;
}

// RETIRED_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_retired.
const step_fn RETIRED_STEPS[] =
{
    &example_step<&example_retired, 0u>,
    &example_step<&example_retired, 1u>,
    &example_step<&example_retired, 2u>,
    &example_step<&example_retired, 3u>
};

// RETIRED_COUNT
//   constant: how many steps RETIRED_STEPS holds.
const std::size_t RETIRED_COUNT =
    sizeof(RETIRED_STEPS) / sizeof(RETIRED_STEPS[0]);

// 1.29   close_match
//------------------------------------------------------------------------------

/**
 * @brief close_match.cpp's levenshteinDistance: the edits from one text to
 *        another.
 *
 * @param[in] _s1  the one.
 * @param[in] _s2  the other.
 * @return the distance.
 */
std::size_t
levenshtein_distance(
    const std::string& _s1,
    const std::string& _s2
)
{
    const std::size_t        len1 = _s1.size();
    const std::size_t        len2 = _s2.size();
    std::vector<std::size_t> prev(len2 + 1u);
    std::vector<std::size_t> curr(len2 + 1u);

    // an empty text
    if ( (len1 == 0u) ||
         (len2 == 0u) )
    {
        return (std::max)(len1,
                          len2);
    }

    std::iota(prev.begin(),
              prev.end(),
              static_cast<std::size_t>(0u));

    for (std::size_t ii = 1u; ii <= len1; ++ii)
    {
        curr[0] = ii;

        for (std::size_t jj = 1u; jj <= len2; ++jj)
        {
            const std::size_t cost = (_s1[ii - 1u] == _s2[jj - 1u]) ? 0u
                                                                    : 1u;

            curr[jj] = (std::min)({ prev[jj] + 1u,
                                    curr[jj - 1u] + 1u,
                                    prev[jj - 1u] + cost });
        }

        std::swap(prev,
                  curr);
    }

    return prev[len2];
}

/**
 * @brief close_match.cpp's findClosestMatch: the candidate nearest a text.
 *
 * @param[in] _input       the text.
 * @param[in] _candidates  the candidates.
 * @return the nearest, and its distance.
 */
std::pair<std::string, std::size_t>
find_closest_match(
    const std::string&              _input,
    const std::vector<std::string>& _candidates
)
{
    std::string closest;
    std::size_t min_distance = std::string::npos;

    for (const auto& candidate : _candidates)
    {
        const std::size_t distance = levenshtein_distance(_input,
                                                          candidate);

        // a nearer candidate
        if (distance < min_distance)
        {
            min_distance = distance;
            closest      = candidate;
        }
    }

    return std::make_pair(closest,
                          min_distance);
}

/**
 * @brief close_match.cpp's addSubcommandCloseMatchDetection: extras
 *        allowed, and a callback, once the parse is complete, that writes
 *        the subcommand nearest each extra word.
 *
 * @param[in,out] _run           the run, whose parser it is.
 * @param[in]     _min_distance  the farthest a match may be.
 */
void
add_subcommand_close_match_detection(
    run&        _run,
    std::size_t _min_distance
)
{
    cli::command             app  = _run.root();
    std::vector<std::string> list;

    app->allow_extras(true);

    // each subcommand's name and aliases
    for (auto sub : app->get_subcommands([](const cli::command&) {
                                             return true;
                                         }))
    {
        const auto aliases = sub->get_aliases();

        if (!std::string(sub->get_name()).empty())
        {
            list.push_back(std::string(sub->get_name()));
        }

        for (std::size_t index = 0u; index < aliases.size(); ++index)
        {
            list.push_back(std::string(aliases[index]));
        }
    }

    app->parse_complete_callback([app, _min_distance, list, &_run]() {
        for (const auto& extra : app->remaining())
        {
            // a word that is not an option's
            if ( (!extra.empty()) &&
                 (extra[0] != '-') )
            {
                const std::pair<std::string, std::size_t> closest =
                    find_closest_match(extra,
                                       list);

                if (closest.second <= _min_distance)
                {
                    say(_run,
                        "unmatched command \"",
                        extra,
                        "\", closest match is ",
                        closest.first);
                }
            }
        }
    });

    return;
}

// CLOSE_MATCH_RUNS
//   constant: close_match's arguments: a prefix of a subcommand, a word near
// one, and a word near none.
const invocation CLOSE_MATCH_RUNS[] =
{
    { { "instal", D_NULLPTR } },
    { { "upgrde", D_NULLPTR } },
    { { "-v", "3", "tst", D_NULLPTR } },
    { { "elsewhere", D_NULLPTR } }
};

/**
 * @brief close_match.cpp: subcommands matched by prefix, and words near
 *        them reported; the subcommands used written.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of CLOSE_MATCH_RUNS.
 */
void
example_close_match(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = CLOSE_MATCH_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    int               value = 0;

    _run.app().description("App for testing prefix matching and close "
                           "string matching");
    _run.app().allow_subcommand_prefix_matching();
    _run.app().add_option("-v",
                          value,
                          "value");
    _run.app().add_subcommand("install",
                              "");
    _run.app().add_subcommand("upgrade",
                              "");
    _run.app().add_subcommand("remove",
                              "");
    _run.app().add_subcommand("test",
                              "");
    add_subcommand_close_match_detection(_run,
                                         5u);

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    for (auto sub : _run.app().get_subcommands())
    {
        say(_run,
            sub->get_name());
    }

    return;
}

// CLOSE_MATCH_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_close_match.
const step_fn CLOSE_MATCH_STEPS[] =
{
    &example_step<&example_close_match, 0u>,
    &example_step<&example_close_match, 1u>,
    &example_step<&example_close_match, 2u>,
    &example_step<&example_close_match, 3u>
};

// CLOSE_MATCH_COUNT
//   constant: how many steps CLOSE_MATCH_STEPS holds.
const std::size_t CLOSE_MATCH_COUNT =
    sizeof(CLOSE_MATCH_STEPS) / sizeof(CLOSE_MATCH_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// 1.30   help_usage
//------------------------------------------------------------------------------

// HELP_USAGE_RUNS
//   constant: help_usage's arguments: the help its comment shows, a file
// that does not exist, files that do, and no subcommand.
const invocation HELP_USAGE_RUNS[] =
{
    { { "-h", D_NULLPTR } },
    { { "e", "missing.txt", "out.txt", D_NULLPTR } },
    { { "E", "in.txt", "out.txt", "-l", "7", "-s", D_NULLPTR } },
    { { D_NULLPTR } }
};

/**
 * @brief help_usage.cpp: subcommands in any case, positionals with text
 *        in place of their type, a usage of the program's own, and the
 *        help flag that shows everything.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of HELP_USAGE_RUNS.
 */
void
example_help_usage(
    run&        _run,
    std::size_t _index
)
{
    static const file_entry FILES[] =
    {
        { "in.txt",  "input" },
        { D_NULLPTR, D_NULLPTR }
    };
    const invocation&       given  = HELP_USAGE_RUNS[_index];
    const std::size_t       count  = begin_example(_run,
                                                   given);
    scratch_directory       files(FILES);
    std::string             input_file_name;
    std::string             output_file_name;
    int                     level  = 5;
    int                     subopt = 0;

    // the file the second run reads
    if (!files.ok())
    {
        _run.note("the scratch directory was not made");
    }

    _run.app().description("CLI11 help");
    _run.app().require_subcommand(1);

    cli::command encode = _run.app().add_subcommand("e",
                                                    "encode")
                                    ->ignore_case();

    encode->add_option("input",
                       input_file_name,
                       "input file")
          ->option_text(" ")
          ->required()
          ->check(cli::existing_file());
    encode->add_option("output",
                       output_file_name,
                       "output file")
          ->option_text(" ")
          ->required();
    encode->add_option("-l, --level",
                       level,
                       "encoding level")
          ->option_text("[1..9]")
          ->check(cli::range(1,
                             9))
          ->default_val(5);
    encode->add_flag("-R, --remove",
                     "remove input file");
    encode->add_flag("-s, --suboption",
                     subopt,
                     "suboption")
          ->option_text(" ");

    cli::command decode = _run.app().add_subcommand("d",
                                                    "decode")
                                    ->ignore_case();

    decode->add_option("input",
                       input_file_name,
                       "input file")
          ->option_text(" ")
          ->required()
          ->check(cli::existing_file());
    decode->add_option("output",
                       output_file_name,
                       "output file")
          ->option_text(" ")
          ->required();

    const std::string usage_msg = "Usage: " + std::string("prog") +
                                  " <command> [options] <input-file> "
                                  "<output-file>";

    _run.app().usage(usage_msg);
    _run.app().set_help_flag("");
    _run.app().set_help_all_flag("-h, --help");

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "files ",
        input_file_name,
        " ",
        output_file_name,
        " level ",
        level,
        " suboption ",
        subopt);

    return;
}

// HELP_USAGE_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_help_usage.
const step_fn HELP_USAGE_STEPS[] =
{
    &example_step<&example_help_usage, 0u>,
    &example_step<&example_help_usage, 1u>,
    &example_step<&example_help_usage, 2u>,
    &example_step<&example_help_usage, 3u>
};

// HELP_USAGE_COUNT
//   constant: how many steps HELP_USAGE_STEPS holds.
const std::size_t HELP_USAGE_COUNT =
    sizeof(HELP_USAGE_STEPS) / sizeof(HELP_USAGE_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

// 1.31   testEXE
//------------------------------------------------------------------------------

// TEST_EXE_RUNS
//   constant: testEXE's arguments: a value, then the subcommand and the
// arguments it takes.
const invocation TEST_EXE_RUNS[] =
{
    { { "-v", "27", "sub", "-v", "13", "--val", "prefix", D_NULLPTR } }
};

/**
 * @brief testEXE.cpp: as arg_capture.cpp, without the alias.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _index  the invocation of TEST_EXE_RUNS.
 */
void
example_test_exe(
    run&        _run,
    std::size_t _index
)
{
    const invocation& given = TEST_EXE_RUNS[_index];
    const std::size_t count = begin_example(_run,
                                            given);
    int               value = 0;
    std::string       after("after Args:");

    _run.app().description("Test App");
    _run.app().add_option("-v",
                          value,
                          "value");

    cli::command subcom = _run.app().add_subcommand("sub",
                                                    "")
                                    ->prefix_command();

    // CLI11_PARSE
    if (_run.parse_as_main(given.args,
                           count) != -1)
    {
        return;
    }

    say(_run,
        "value =",
        value);

    for (const auto& aarg : subcom->remaining())
    {
        after += aarg + " ";
    }

    say(_run,
        after);

    return;
}

// TEST_EXE_STEPS
//   constant: the steps of d_tests_arg_cpp_examples_test_exe.
const step_fn TEST_EXE_STEPS[] =
{
    &example_step<&example_test_exe, 0u>
};

// TEST_EXE_COUNT
//   constant: how many steps TEST_EXE_STEPS holds.
const std::size_t TEST_EXE_COUNT =
    sizeof(TEST_EXE_STEPS) / sizeof(TEST_EXE_STEPS[0]);

}  // namespace

//==============================================================================
// 2.  CLI11'S EXAMPLES
//==============================================================================

/*
d_tests_arg_cpp_examples_simple
  simple.cpp, as CLI11 runs it.
  Tests the following:
  - no arguments; every option; the version
*/
bool
d_tests_arg_cpp_examples_simple()
{
    return steps_hold("SIMPLE",
                      SIMPLE_STEPS,
                      SIMPLE_COUNT,
                      SIMPLE);
}

/*
d_tests_arg_cpp_examples_subcommands
  subcommands.cpp, as CLI11 runs it.
  Tests the following:
  - no subcommand, refused; both, with their options; help expanded
*/
bool
d_tests_arg_cpp_examples_subcommands()
{
    return steps_hold("SUBCOMMANDS",
                      SUBCOMMANDS_STEPS,
                      SUBCOMMANDS_COUNT,
                      SUBCOMMANDS);
}

/*
d_tests_arg_cpp_examples_subcom_partitioned
  subcom_partitioned.cpp, as CLI11 runs it.
  Tests the following:
  - the required options missing; every option; the help, with the
    nameless subcommands' options
*/
bool
d_tests_arg_cpp_examples_subcom_partitioned()
{
    return steps_hold("SUBCOM_PARTITIONED",
                      SUBCOM_PARTITIONED_STEPS,
                      SUBCOM_PARTITIONED_COUNT,
                      SUBCOM_PARTITIONED);
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/*
d_tests_arg_cpp_examples_config_app
  config_app.cpp, as CLI11 runs it.
  Tests the following:
  - the configuration printed, with files given before and after, empty
    and not; the values otherwise
*/
bool
d_tests_arg_cpp_examples_config_app()
{
    return steps_hold("CONFIG_APP",
                      CONFIG_APP_STEPS,
                      CONFIG_APP_COUNT,
                      CONFIG_APP);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

/*
d_tests_arg_cpp_examples_option_groups
  option_groups.cpp, as CLI11 runs it.
  Tests the following:
  - no format, refused; two formats, refused; two targets, refused; one of
    each
*/
bool
d_tests_arg_cpp_examples_option_groups()
{
    return steps_hold("OPTION_GROUPS",
                      OPTION_GROUPS_STEPS,
                      OPTION_GROUPS_COUNT,
                      OPTION_GROUPS);
}

/*
d_tests_arg_cpp_examples_positional_arity
  positional_arity.cpp, as CLI11 runs it.
  Tests the following:
  - one and two files; numbers and a file; a word where a number goes
*/
bool
d_tests_arg_cpp_examples_positional_arity()
{
    return steps_hold("POSITIONAL_ARITY",
                      POSITIONAL_ARITY_STEPS,
                      POSITIONAL_ARITY_COUNT,
                      POSITIONAL_ARITY);
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/*
d_tests_arg_cpp_examples_positional_validation
  positional_validation.cpp, as CLI11 runs it.
  Tests the following:
  - files and numbers in each order, each positional taking what it
    validates
*/
bool
d_tests_arg_cpp_examples_positional_validation()
{
    return steps_hold("POSITIONAL_VALIDATION",
                      POSITIONAL_VALIDATION_STEPS,
                      POSITIONAL_VALIDATION_COUNT,
                      POSITIONAL_VALIDATION);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

/*
d_tests_arg_cpp_examples_date_validator
  date_validator.cpp, as CLI11 runs it.
  Tests the following:
  - a time that reads, and one that does not
*/
bool
d_tests_arg_cpp_examples_date_validator()
{
    return steps_hold("DATE_VALIDATOR",
                      DATE_VALIDATOR_STEPS,
                      DATE_VALIDATOR_COUNT,
                      DATE_VALIDATOR);
}

/*
d_tests_arg_cpp_examples_custom_validator
  custom_validator.cpp, as CLI11 runs it.
  Tests the following:
  - a value outside the range; the range moved by later options to hold it
*/
bool
d_tests_arg_cpp_examples_custom_validator()
{
    return steps_hold("CUSTOM_VALIDATOR",
                      CUSTOM_VALIDATOR_STEPS,
                      CUSTOM_VALIDATOR_COUNT,
                      CUSTOM_VALIDATOR);
}

/*
d_tests_arg_cpp_examples_shapes
  shapes.cpp, as CLI11 runs it.
  Tests the following:
  - shapes drawn as they are parsed, `++` ending a subcommand's values
  - the help, expanded
*/
bool
d_tests_arg_cpp_examples_shapes()
{
    return steps_hold("SHAPES",
                      SHAPES_STEPS,
                      SHAPES_COUNT,
                      SHAPES);
}

/*
d_tests_arg_cpp_examples_ranges
  ranges.cpp, as CLI11 runs it.
  Tests the following:
  - the range whole; its parts; both, refused
*/
bool
d_tests_arg_cpp_examples_ranges()
{
    return steps_hold("RANGES",
                      RANGES_STEPS,
                      RANGES_COUNT,
                      RANGES);
}

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

/*
d_tests_arg_cpp_examples_validators
  validators.cpp, as CLI11 runs it.
  Tests the following:
  - the help; a file that does not exist; a value outside the range, and
    one inside
*/
bool
d_tests_arg_cpp_examples_validators()
{
    return steps_hold("VALIDATORS",
                      VALIDATORS_STEPS,
                      VALIDATORS_COUNT,
                      VALIDATORS);
}

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

/*
d_tests_arg_cpp_examples_groups
  groups.cpp, as CLI11 runs it, without its timer.
  Tests the following:
  - the required options missing; every option; the help, in its groups
*/
bool
d_tests_arg_cpp_examples_groups()
{
    return steps_hold("GROUPS",
                      GROUPS_STEPS,
                      GROUPS_COUNT,
                      GROUPS);
}

/*
d_tests_arg_cpp_examples_inter_argument_order
  inter_argument_order.cpp, as CLI11 runs it.
  Tests the following:
  - values of two options, interleaved, put back in their order
*/
bool
d_tests_arg_cpp_examples_inter_argument_order()
{
    return steps_hold("INTER_ARGUMENT_ORDER",
                      INTER_ARGUMENT_ORDER_STEPS,
                      INTER_ARGUMENT_ORDER_COUNT,
                      INTER_ARGUMENT_ORDER);
}

/*
d_tests_arg_cpp_examples_prefix_command
  prefix_command.cpp, as CLI11 runs it.
  Tests the following:
  - values up to the separator, and the rest left
*/
bool
d_tests_arg_cpp_examples_prefix_command()
{
    return steps_hold("PREFIX_COMMAND",
                      PREFIX_COMMAND_STEPS,
                      PREFIX_COMMAND_COUNT,
                      PREFIX_COMMAND);
}

/*
d_tests_arg_cpp_examples_minimal
  minimal.cpp, as CLI11 runs it.
  Tests the following:
  - nothing; the help
*/
bool
d_tests_arg_cpp_examples_minimal()
{
    return steps_hold("MINIMAL",
                      MINIMAL_STEPS,
                      MINIMAL_COUNT,
                      MINIMAL);
}

/*
d_tests_arg_cpp_examples_arg_capture
  arg_capture.cpp, as CLI11 runs it.
  Tests the following:
  - a value, then the subcommand by its alias taking the rest
*/
bool
d_tests_arg_cpp_examples_arg_capture()
{
    return steps_hold("ARG_CAPTURE",
                      ARG_CAPTURE_STEPS,
                      ARG_CAPTURE_COUNT,
                      ARG_CAPTURE);
}

/*
d_tests_arg_cpp_examples_callback_passthrough
  callback_passthrough.cpp, as CLI11 runs it.
  Tests the following:
  - the option named before its value, and after it
*/
bool
d_tests_arg_cpp_examples_callback_passthrough()
{
    return steps_hold("CALLBACK_PASSTHROUGH",
                      CALLBACK_PASSTHROUGH_STEPS,
                      CALLBACK_PASSTHROUGH_COUNT,
                      CALLBACK_PASSTHROUGH);
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/*
d_tests_arg_cpp_examples_enum
  enum.cpp, as CLI11 runs it.
  Tests the following:
  - a level by number; a number no level has; a name in another case
*/
bool
d_tests_arg_cpp_examples_enum()
{
    return steps_hold("ENUM_LEVELS",
                      ENUM_LEVELS_STEPS,
                      ENUM_LEVELS_COUNT,
                      ENUM_LEVELS);
}

/*
d_tests_arg_cpp_examples_enum_ostream
  enum_ostream.cpp, as CLI11 runs it.
  Tests the following:
  - a level by name, written by its own operator<<
*/
bool
d_tests_arg_cpp_examples_enum_ostream()
{
    return steps_hold("ENUM_OSTREAM",
                      ENUM_OSTREAM_STEPS,
                      ENUM_OSTREAM_COUNT,
                      ENUM_OSTREAM);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

/*
d_tests_arg_cpp_examples_digit_args
  digit_args.cpp, as CLI11 runs it.
  Tests the following:
  - the help, with each flag's value; one flag; two
*/
bool
d_tests_arg_cpp_examples_digit_args()
{
    return steps_hold("DIGIT_ARGS",
                      DIGIT_ARGS_STEPS,
                      DIGIT_ARGS_COUNT,
                      DIGIT_ARGS);
}

/*
d_tests_arg_cpp_examples_modhelp
  modhelp.cpp, as CLI11 runs it.
  Tests the following:
  - help after the value is read; the value alone; a failure after it
*/
bool
d_tests_arg_cpp_examples_modhelp()
{
    return steps_hold("MODHELP",
                      MODHELP_STEPS,
                      MODHELP_COUNT,
                      MODHELP);
}

/*
d_tests_arg_cpp_examples_array_option
  array_option.cpp, as CLI11 runs it.
  Tests the following:
  - two values into a std::array; the help, with its default
*/
bool
d_tests_arg_cpp_examples_array_option()
{
    return steps_hold("ARRAY_OPTION",
                      ARRAY_OPTION_STEPS,
                      ARRAY_OPTION_COUNT,
                      ARRAY_OPTION);
}

/*
d_tests_arg_cpp_examples_subcom_in_files
  subcom_in_files, as CLI11 runs it.
  Tests the following:
  - the subcommand with its options; none, refused
*/
bool
d_tests_arg_cpp_examples_subcom_in_files()
{
    return steps_hold("SUBCOM_IN_FILES",
                      SUBCOM_IN_FILES_STEPS,
                      SUBCOM_IN_FILES_COUNT,
                      SUBCOM_IN_FILES);
}

/*
d_tests_arg_cpp_examples_formatter
  formatter.cpp, as CLI11 runs it.
  Tests the following:
  - the help, and all of it, through the program's formatter; nothing
*/
bool
d_tests_arg_cpp_examples_formatter()
{
    return steps_hold("FORMATTER",
                      FORMATTER_STEPS,
                      FORMATTER_COUNT,
                      FORMATTER);
}

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

/*
d_tests_arg_cpp_examples_nested
  nested.cpp, as CLI11 runs it.
  Tests the following:
  - the help, expanded; a path that exists; a default file not checked; a
    file that does not exist; a required path missing
*/
bool
d_tests_arg_cpp_examples_nested()
{
    return steps_hold("NESTED",
                      NESTED_STEPS,
                      NESTED_COUNT,
                      NESTED);
}

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

/*
d_tests_arg_cpp_examples_subcom_help
  subcom_help.cpp, as CLI11 runs it.
  Tests the following:
  - the subcommand's help asked for after it and before it; its argument
*/
bool
d_tests_arg_cpp_examples_subcom_help()
{
    return steps_hold("SUBCOM_HELP",
                      SUBCOM_HELP_STEPS,
                      SUBCOM_HELP_COUNT,
                      SUBCOM_HELP);
}

/*
d_tests_arg_cpp_examples_retired
  retired.cpp, as CLI11 runs it.
  Tests the following:
  - an option retired by name, alone and with a value; one deprecated; the
    replacement, and one retired by handle
*/
bool
d_tests_arg_cpp_examples_retired()
{
    return steps_hold("RETIRED",
                      RETIRED_STEPS,
                      RETIRED_COUNT,
                      RETIRED);
}

/*
d_tests_arg_cpp_examples_close_match
  close_match.cpp, as CLI11 runs it.
  Tests the following:
  - a subcommand by prefix; words near a subcommand and near none
*/
bool
d_tests_arg_cpp_examples_close_match()
{
    return steps_hold("CLOSE_MATCH",
                      CLOSE_MATCH_STEPS,
                      CLOSE_MATCH_COUNT,
                      CLOSE_MATCH);
}

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

/*
d_tests_arg_cpp_examples_help_usage
  help_usage.cpp, as CLI11 runs it.
  Tests the following:
  - the help; a file that does not exist; the subcommand in another case,
    with its options; no subcommand, refused
*/
bool
d_tests_arg_cpp_examples_help_usage()
{
    return steps_hold("HELP_USAGE",
                      HELP_USAGE_STEPS,
                      HELP_USAGE_COUNT,
                      HELP_USAGE);
}

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

/*
d_tests_arg_cpp_examples_test_exe
  testEXE.cpp, as CLI11 runs it.
  Tests the following:
  - a value, then the subcommand taking the rest
*/
bool
d_tests_arg_cpp_examples_test_exe()
{
    return steps_hold("TEST_EXE",
                      TEST_EXE_STEPS,
                      TEST_EXE_COUNT,
                      TEST_EXE);
}

//==============================================================================
// 3.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_examples
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_examples[] =
{
    { "cpp_examples_simple",
      d_tests_arg_cpp_examples_simple },
    { "cpp_examples_subcommands",
      d_tests_arg_cpp_examples_subcommands },
    { "cpp_examples_subcom_partitioned",
      d_tests_arg_cpp_examples_subcom_partitioned },
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "cpp_examples_config_app",
      d_tests_arg_cpp_examples_config_app },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
    { "cpp_examples_option_groups",
      d_tests_arg_cpp_examples_option_groups },
    { "cpp_examples_positional_arity",
      d_tests_arg_cpp_examples_positional_arity },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_examples_positional_validation",
      d_tests_arg_cpp_examples_positional_validation },
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
    { "cpp_examples_date_validator",
      d_tests_arg_cpp_examples_date_validator },
    { "cpp_examples_custom_validator",
      d_tests_arg_cpp_examples_custom_validator },
    { "cpp_examples_shapes",
      d_tests_arg_cpp_examples_shapes },
    { "cpp_examples_ranges",
      d_tests_arg_cpp_examples_ranges },
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
    { "cpp_examples_validators",
      d_tests_arg_cpp_examples_validators },
#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
    { "cpp_examples_groups",
      d_tests_arg_cpp_examples_groups },
    { "cpp_examples_inter_argument_order",
      d_tests_arg_cpp_examples_inter_argument_order },
    { "cpp_examples_prefix_command",
      d_tests_arg_cpp_examples_prefix_command },
    { "cpp_examples_minimal",
      d_tests_arg_cpp_examples_minimal },
    { "cpp_examples_arg_capture",
      d_tests_arg_cpp_examples_arg_capture },
    { "cpp_examples_callback_passthrough",
      d_tests_arg_cpp_examples_callback_passthrough },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_examples_enum",
      d_tests_arg_cpp_examples_enum },
    { "cpp_examples_enum_ostream",
      d_tests_arg_cpp_examples_enum_ostream },
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
    { "cpp_examples_digit_args",
      d_tests_arg_cpp_examples_digit_args },
    { "cpp_examples_modhelp",
      d_tests_arg_cpp_examples_modhelp },
    { "cpp_examples_array_option",
      d_tests_arg_cpp_examples_array_option },
    { "cpp_examples_subcom_in_files",
      d_tests_arg_cpp_examples_subcom_in_files },
    { "cpp_examples_formatter",
      d_tests_arg_cpp_examples_formatter },
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
    { "cpp_examples_nested",
      d_tests_arg_cpp_examples_nested },
#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
    { "cpp_examples_subcom_help",
      d_tests_arg_cpp_examples_subcom_help },
    { "cpp_examples_retired",
      d_tests_arg_cpp_examples_retired },
    { "cpp_examples_close_match",
      d_tests_arg_cpp_examples_close_match },
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
    { "cpp_examples_help_usage",
      d_tests_arg_cpp_examples_help_usage },
#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
    { "cpp_examples_test_exe",
      d_tests_arg_cpp_examples_test_exe }
};

// d_tests_arg_cpp_examples_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_examples_count =
    sizeof(d_tests_arg_cpp_examples) / sizeof(d_tests_arg_cpp_examples[0]);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
