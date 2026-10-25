/*******************************************************************************
* djinterp [parse]                                            arg_type_tests.cpp
*
* The argument parser's C++ type suite's sections.
*   The sections held to CLI11 run what the CASES section defines: options
* bound to variables of every kind of type, parsed, and the variables
* written after; defaults given as values of the types; values read back
* with as<T>(). tools/cppref/cppref.py compiles that section against CLI11
* 2.7.2, through the face's names for CLI11's (cli11_shim.hpp), and the
* transcripts below are what CLI11 wrote; so the CASES section is written in
* the part of the face CLI11 shares -- `->` on every handle, no status taken
* from a call -- and C++98 unless a guard says otherwise. A variable is
* written by the section's own writers, the same in both builds, so that a
* transcript compares what a parse put in it, not how each library writes
* it. The sections that are the module's own are written apart, after it,
* each saying why CLI11 cannot write its transcript.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_type_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.19
*******************************************************************************/
#include "./arg_type_tests.hpp"  // corresponding header
// std
#include <complex>        // std::complex
#include <cstddef>        // std::size_t, std::ptrdiff_t
#include <deque>          // std::deque
#include <istream>        // std::istream
#include <list>           // std::list
#include <map>            // std::map, std::multimap
#include <ostream>        // std::ostream
#include <set>            // std::set, std::multiset
#include <sstream>        // std::ostringstream, std::istringstream
#include <string>         // std::string, std::wstring
#include <utility>        // std::pair, std::make_pair
#include <vector>         // std::vector
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#include <algorithm>      // std::sort
#include <array>          // std::array
#include <chrono>         // std::chrono::seconds, std::chrono::duration
#include <tuple>          // std::tuple, std::get, std::make_tuple
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set
#endif
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
#include <filesystem>     // std::filesystem::path
#include <optional>       // std::optional
#include <string_view>    // std::string_view
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
using cli::testing::steps_hold;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// NUMBERS
//   constant: what the steps of d_tests_arg_cpp_type_numbers write, as CLI11
// writes them.
const char NUMBERS[] =
    "int: type=[INT] expected=1,1 type_size=1,1\n"
    "short: type=[INT] expected=1,1 type_size=1,1\n"
    "long: type=[INT] expected=1,1 type_size=1,1\n"
    "signed_char: type=[INT] expected=1,1 type_size=1,1\n"
    "plain: type=[INT] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--int|--short|--long|--signed-char|--plain]\n"
    "opt int n=1 res=[-2147483648]\n"
    "opt short n=1 res=[32767]\n"
    "opt long n=1 res=[-77]\n"
    "opt signed_char n=1 res=[-128]\n"
    "opt plain n=1 res=[+12]\n"
    "int=-2147483648\n"
    "short=32767\n"
    "long=-77\n"
    "signed_char=-128\n"
    "plain=12\n"
    "unsigned: type=[UINT] expected=1,1 type_size=1,1\n"
    "ushort: type=[UINT] expected=1,1 type_size=1,1\n"
    "ulong: type=[UINT] expected=1,1 type_size=1,1\n"
    "uchar: type=[UINT] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] order=[--unsigned|--ushort|--ulong|--uchar]\n"
    "opt unsigned n=1 res=[4294967295]\n"
    "opt ushort n=1 res=[0x10]\n"
    "opt ulong n=1 res=[0o17]\n"
    "opt uchar n=1 res=[255]\n"
    "unsigned=4294967295\n"
    "ushort=16\n"
    "ulong=15\n"
    "uchar=255\n"
    "float: type=[FLOAT] expected=1,1 type_size=1,1\n"
    "double: type=[FLOAT] expected=1,1 type_size=1,1\n"
    "long_double: type=[FLOAT] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] order=[--float|--double|--long-double]\n"
    "opt float n=1 res=[0.25]\n"
    "opt double n=1 res=[-1.5e3]\n"
    "opt long_double n=1 res=[7e-2]\n"
    "float=0.25\n"
    "double=-1500\n"
    "long_double=0.07\n"
    "truth: type=[BOOLEAN] expected=1,1 type_size=1,1\n"
    "falsity: type=[BOOLEAN] expected=1,1 type_size=1,1\n"
    "char: type=[CHAR] expected=1,1 type_size=1,1\n"
    "digit: type=[CHAR] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] order=[--truth|--falsity|--char|--digit]\n"
    "opt truth n=1 res=[yes]\n"
    "opt falsity n=1 res=[off]\n"
    "opt char n=1 res=[a]\n"
    "opt digit n=1 res=[7]\n"
    "truth=true\n"
    "falsity=false\n"
    "char='a'\n"
    "digit='7'\n"
    "cmd app n=1 miss=[] subs=[] order=[--truth|--falsity|--char]\n"
    "opt truth n=1 res=[0]\n"
    "opt falsity n=1 res=[TRUE]\n"
    "opt char n=1 res=[65]\n"
    "opt digit n=0 res=[]\n"
    "truth=false\n"
    "falsity=true\n"
    "char='A'\n"
    "ERR ConversionError 104: Could not convert: --uchar = 256\n"
    "cmd app n=1 miss=[] subs=[] order=[--uchar]\n"
    "ERR ConversionError 104: Could not convert: --int = twelve\n"
    "cmd app n=1 miss=[] subs=[] order=[--int]\n"
    "ERR ConversionError 104: Could not convert: --truth = maybe\n"
    "cmd app n=1 miss=[] subs=[] order=[--truth]\n"
    "ERR ConversionError 104: Could not convert: --double = 1.5x\n"
    "cmd app n=1 miss=[] subs=[] order=[--double]\n"
    "ERR ConversionError 104: Could not convert: --unsigned = -1\n"
    "cmd app n=1 miss=[] subs=[] order=[--unsigned]\n";

#if D_ENV_HAS_LONG_LONG

// NUMBERS_LONG_LONG
//   constant: what the steps of d_tests_arg_cpp_type_numbers_long_long
// write, as CLI11 writes them.
const char NUMBERS_LONG_LONG[] =
    "long_long: type=[INT] expected=1,1 type_size=1,1\n"
    "ulong_long: type=[UINT] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] order=[--long-long|--ulong-long]\n"
    "opt long_long n=1 res=[-9223372036854775808]\n"
    "opt ulong_long n=1 res=[18446744073709551615]\n"
    "long_long=-9223372036854775808\n"
    "ulong_long=18446744073709551615\n"
    "ERR ConversionError 104: Could not convert: --long-long = "
    "9223372036854775808\n"
    "cmd app n=1 miss=[] subs=[] order=[--long-long]\n"
    "opt long_long n=1 res=[9223372036854775808]\n"
    "opt ulong_long n=0 res=[]\n";

#endif  // D_ENV_HAS_LONG_LONG

// TEXTS
//   constant: what the steps of d_tests_arg_cpp_type_texts write, as CLI11
// writes them.
const char TEXTS[] =
    "text: type=[TEXT] expected=1,1 type_size=1,1\n"
    "empty: type=[TEXT] expected=1,1 type_size=1,1\n"
    "name: type=[TEXT] expected=1,1 type_size=1,1\n"
    "point: type=[TEXT] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] order=[--text|--empty|--name|--point]\n"
    "opt text n=1 res=[a b]\n"
    "opt empty n=1 res=[]\n"
    "opt name n=1 res=[named]\n"
    "opt point n=1 res=[3,4]\n"
    "text=\"a b\"\n"
    "empty=\"\"\n"
    "name=name \"named\"\n"
    "point=3,4\n"
    "ERR ConversionError 104: Could not convert: --point = 3;4\n"
    "cmd app n=1 miss=[] subs=[] order=[--point]\n"
    "opt text n=0 res=[]\n"
    "opt empty n=0 res=[]\n"
    "opt name n=0 res=[]\n"
    "opt point n=1 res=[3;4]\n";

// CONTAINERS
//   constant: what the steps of d_tests_arg_cpp_type_containers write, as
// CLI11 writes them.
const char CONTAINERS[] =
    "ints: type=[INT] expected=1,536870912 type_size=1,1\n"
    "reals: type=[FLOAT] expected=1,536870912 type_size=1,1\n"
    "chars: type=[CHAR] expected=1,536870912 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--ints|--ints|--ints|--reals|--reals|--chars|--chars]\n"
    "opt ints n=3 res=[1|2|3]\n"
    "opt reals n=2 res=[0.5|-2]\n"
    "opt chars n=2 res=[x|y]\n"
    "ints={1,2,3}\n"
    "reals={0.5,-2}\n"
    "chars={'x','y'}\n"
    "set: type=[TEXT] expected=1,536870912 type_size=1,1\n"
    "multi: type=[INT] expected=1,536870912 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--set|--set|--set|--multi|--multi|--multi]\n"
    "opt set n=3 res=[b|a|b]\n"
    "opt multi n=3 res=[3|1|3]\n"
    "set={\"a\",\"b\"}\n"
    "multi={1,3,3}\n"
    "map: type=[[TEXT,INT]] expected=1,536870912 type_size=2,2\n"
    "multi: type=[[INT,FLOAT]] expected=1,536870912 type_size=2,2\n"
    "pairs: type=[[TEXT,INT]] expected=1,536870912 type_size=2,2\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--map|--map|--map|--map|--map|--map|--multi|--multi|--multi|--multi"
    "|--pairs|--pairs|--pairs|--pairs]\n"
    "opt map n=6 res=[a|1|b|2|a|3]\n"
    "opt multi n=4 res=[1|1.5|1|2.5]\n"
    "opt pairs n=4 res=[x|7|y|8]\n"
    "map={(\"a\",1),(\"b\",2)}\n"
    "multi={(1,1.5),(1,2.5)}\n"
    "pairs={(\"x\",7),(\"y\",8)}\n"
    "ERR ArgumentMismatch 114: --pairs: [TEXT,INT] only partially specified: 2 "
    "required for each element\n"
    "cmd app n=1 miss=[] subs=[] order=[--pairs|--pairs|--pairs]\n"
    "opt map n=0 res=[]\n"
    "opt multi n=0 res=[]\n"
    "opt pairs n=3 res=[x|7|y]\n"
    "may: type=[INT] expected=1,536870912 type_size=1,1\n"
    "texts: type=[TEXT] expected=1,536870912 type_size=1,1\n"
    "ints: type=[INT] expected=1,536870912 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] order=[--may|--texts]\n"
    "opt may n=1 res=[{}]\n"
    "opt texts n=1 res=[{}]\n"
    "opt ints n=0 res=[]\n"
    "may={}\n"
    "texts={\"{}\"}\n"
    "ERR ConversionError 104: Could not convert: --ints = {}\n"
    "cmd app n=1 miss=[] subs=[] order=[--ints]\n"
    "opt may n=0 res=[]\n"
    "opt texts n=0 res=[]\n"
    "opt ints n=1 res=[{}]\n"
    "ints: type=[INT] expected=1,536870912 type_size=1,1\n"
    "ERR ConversionError 104: Could not convert: --ints = 1,x,3\n"
    "cmd app n=1 miss=[] subs=[] order=[--ints|--ints|--ints]\n"
    "opt ints n=3 res=[1|x|3]\n";

// COMPOUND
//   constant: what the steps of d_tests_arg_cpp_type_compound write, as
// CLI11 writes them.
const char COMPOUND[] =
    "pair: type=[[INT,TEXT]] expected=1,1 type_size=2,2\n"
    "cmd app n=1 miss=[] subs=[] order=[--pair|--pair]\n"
    "opt pair n=2 res=[3|x]\n"
    "pair=(3,\"x\")\n"
    "ERR ConversionError 104: Could not convert: --pair = nope,y\n"
    "cmd app n=1 miss=[] subs=[] order=[--pair|--pair]\n"
    "opt pair n=2 res=[nope|y]\n"
    "pair=(3,\"x\")\n"
    "complex: type=[COMPLEX] expected=1,1 type_size=1,2\n"
    "cmd app n=1 miss=[] subs=[] order=[--complex]\n"
    "opt complex n=2 res=[1+2i|]\n"
    "complex=(1,2)\n"
    "cmd app n=1 miss=[] subs=[] order=[--complex|--complex]\n"
    "opt complex n=2 res=[1|2j]\n"
    "complex=(1,2)\n"
    "cmd app n=1 miss=[] subs=[] order=[--complex]\n"
    "opt complex n=2 res=[-3i|]\n"
    "complex=(0,-3)\n"
    "cmd app n=1 miss=[] subs=[] order=[--complex]\n"
    "opt complex n=2 res=[(1,2)|]\n"
    "complex=(1,2)\n"
    "cmd app n=1 miss=[] subs=[] order=[--complex]\n"
    "opt complex n=2 res=[4|]\n"
    "complex=(4,0)\n"
    "cmd app n=1 miss=[] subs=[] order=[--complex]\n"
    "opt complex n=2 res=[1e-5|]\n"
    "complex=(1e-05,0)\n"
    "ERR ConversionError 104: Could not convert: --complex = 1+zi\n"
    "cmd app n=1 miss=[] subs=[] order=[--complex]\n"
    "opt complex n=2 res=[1+zi|]\n";

// DEFAULTS
//   constant: what the steps of d_tests_arg_cpp_type_defaults write, as
// CLI11 writes them.
const char DEFAULTS[] =
    "int default=[42]\n"
    "string default=[some text]\n"
    "pair default=[[1,a]]\n"
    "complex default=[(1.5,-2)]\n"
    "name default=[given]\n"
    "point default=[5,6]\n"
    "vector default=[[1,2]]\n"
    "empty default=[{}]\n"
    "list default=[[0.5,-2]]\n"
    "set default=[[a,b]]\n"
    "map default=[[[a,1],[b,2]]]\n"
    "pairs default=[[[x,7]]]\n"
    "captured default=[3]\n"
    "given default=[5]\n"
    "count=5\n"
    "as: count=7 word=\"12\",12,12 list={1,2,3},{\"1\",\"2\",\"3\"}\n";

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// KINDS_CPP11
//   constant: what the steps of d_tests_arg_cpp_type_kinds_cpp11 write, as
// CLI11 writes them.
const char KINDS_CPP11[] =
    "tuple: type=[[INT,FLOAT,TEXT]] expected=1,1 type_size=3,3\n"
    "array: type=[[INT,INT,INT]] expected=1,1 type_size=3,3\n"
    "triples: type=[[INT,INT,INT]] expected=1,536870912 type_size=3,3\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--tuple|--tuple|--tuple|--array|--array|--array|--triples|--triples"
    "|--triples|--triples|--triples|--triples]\n"
    "opt tuple n=3 res=[3|1.5|z]\n"
    "opt array n=3 res=[1|2|3]\n"
    "opt triples n=6 res=[1|2|3|4|5|6]\n"
    "tuple=(3,1.5,\"z\")\n"
    "array={1,2,3}\n"
    "triples={(1,2,3),(4,5,6)}\n"
    "ERR ConversionError 104: Could not convert: --triples = 1,2,%%,4,5,6\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--triples|--triples|--triples|--triples|--triples|--triples]\n"
    "opt tuple n=0 res=[]\n"
    "opt array n=0 res=[]\n"
    "opt triples n=6 res=[1|2|%%|4|5|6]\n"
    "colour: type=[ENUM] expected=1,1 type_size=1,1\n"
    "level: type=[ENUM] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] order=[--colour|--level]\n"
    "opt colour n=1 res=[2]\n"
    "opt level n=1 res=[-1]\n"
    "colour=colour 2\n"
    "level=-1\n"
    "ERR ConversionError 104: Could not convert: --colour = green\n"
    "cmd app n=1 miss=[] subs=[] order=[--colour]\n"
    "opt colour n=1 res=[green]\n"
    "opt level n=0 res=[]\n"
    "hashed: type=[INT] expected=1,536870912 type_size=1,1\n"
    "hashed_map: type=[[TEXT,INT]] expected=1,536870912 type_size=2,2\n"
    "tag: type=[TEXT] expected=1,1 type_size=1,1\n"
    "seconds: type=[INT] expected=1,1 type_size=1,1\n"
    "span: type=[FLOAT] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--hashed|--hashed|--hashed|--hashed-map|--hashed-map|--hashed-map|-"
    "-hashed-map|--tag|--seconds|--span]\n"
    "opt hashed n=3 res=[3|1|2]\n"
    "opt hashed_map n=4 res=[b|2|a|1]\n"
    "opt tag n=1 res=[t]\n"
    "opt seconds n=1 res=[9]\n"
    "opt span n=1 res=[1.5]\n"
    "hashed={1,2,3}\n"
    "hashed_map={(\"a\",1),(\"b\",2)}\n"
    "tag=tag \"t\"\n"
    "seconds=duration 9\n"
    "span=duration 1.5\n"
    "tuple default=[[1,b,7]]\n"
    "array default=[[4,5]]\n"
    "level default=[-1]\n"
    "colour default=[2]\n"
    "tag default=[]\n";

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// KINDS_CPP17
//   constant: what the steps of d_tests_arg_cpp_type_kinds_cpp17 write, as
// CLI11 writes them.
const char KINDS_CPP17[] =
    "maybe: type=[INT] expected=1,1 type_size=1,1\n"
    "absent: type=[INT] expected=1,1 type_size=1,1\n"
    "values: type=[INT] expected=1,536870912 type_size=1,1\n"
    "view: type=[TEXT] expected=1,1 type_size=1,1\n"
    "path: type=[TEXT] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--maybe|--values|--values|--view|--path]\n"
    "opt maybe n=1 res=[7]\n"
    "opt absent n=0 res=[]\n"
    "opt values n=2 res=[7|8]\n"
    "opt view n=1 res=[viewed]\n"
    "opt path n=1 res=[/x y/z]\n"
    "maybe=optional 7\n"
    "absent=none\n"
    "values=optional {7,8}\n"
    "view=view \"viewed\"\n"
    "path=path \"/x y/z\"\n"
    "view default=[seen]\n"
    "path default=[/a b/c]\n";

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// WIDE_TEXT
//   constant: what the steps of d_tests_arg_cpp_type_wide_text write, as
// CLI11 writes them.
const char WIDE_TEXT[] =
    "wide: type=[TEXT] expected=1,1 type_size=1,1\n"
    "wides: type=[TEXT] expected=1,536870912 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] order=[--wide|--wides|--wides]\n"
    "opt wide n=1 res=[\xd0\xb6 x]\n"
    "opt wides n=2 res=[a|\xc3\xa9]\n"
    "wide=L{1078,32,120}\n"
    "wides={L{97},L{233}}\n";

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

// TYPE_NAMES
//   constant: what the steps of d_tests_arg_cpp_type_names write: the
// module's own, as CLI11's types have no descriptors to read -- its type
// names are a detail of its own, which the sections above compare through
// the options.
const char TYPE_NAMES[] =
    "int: name=[INT] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=1 once=1\n"
    "unsigned short: name=[UINT] container=0 expected=1 size=1,1 reads=1 "
    "writes=1 builtin=1 once=1\n"
    "size_t: name=[UINT] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=1 once=1\n"
    "ptrdiff_t: name=[INT] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=1 once=1\n"
    "double: name=[FLOAT] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=1 once=1\n"
    "bool: name=[BOOLEAN] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=1 once=1\n"
    "char: name=[CHAR] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=1 once=1\n"
    "const char*: name=[TEXT] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=1 once=1\n"
    "text: name=[TEXT] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=1 once=1\n"
    "std::string: name=[TEXT] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=0 once=1\n"
    "vector<int>: name=[INT] container=1 expected=536870912 size=1,1 reads=1 "
    "writes=1 builtin=0 once=1\n"
    "set<string>: name=[TEXT] container=1 expected=536870912 size=1,1 reads=1 "
    "writes=1 builtin=0 once=1\n"
    "pair<int,string>: name=[[INT,TEXT]] container=0 expected=1 size=2,2 "
    "reads=1 writes=1 builtin=0 once=1\n"
    "map<string,int>: name=[[TEXT,INT]] container=1 expected=536870912 "
    "size=2,2 reads=1 writes=1 builtin=0 once=1\n"
    "complex<double>: name=[COMPLEX] container=0 expected=1 size=1,2 reads=1 "
    "writes=1 builtin=0 once=1\n"
    "name_t: name=[TEXT] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=0 once=1\n"
    "point: name=[TEXT] container=0 expected=1 size=1,1 reads=1 writes=1 "
    "builtin=0 once=1\n";

// CONVERTED
//   constant: what the steps of d_tests_arg_cpp_type_convert write: the
// module's own, as CLI11's lexical_cast and to_string are details of its
// own, with neither a status nor an integer rule.
const char CONVERTED[] =
    "[42] -> OK 42\n"
    "[010] -> OK 10\n"
    "[x] -> ConversionError 10\n"
    "[] -> OK 0\n"
    "[200] -> OK 200\n"
    "[300] -> ConversionError 200\n"
    "[yes] -> OK true\n"
    "[1.5e1] -> OK 15\n"
    "[hello] -> OK \"hello\"\n"
    "[3,4] -> OK 3,4\n"
    "[3,4x] -> ConversionError 3,4\n"
    "[7] -> OK {7}\n"
    "[{}] -> OK {}\n"
    "[010] under C's rule -> OK 8\n"
    "int -> OK [~17]\n"
    "double -> OK [~0.1]\n"
    "string -> OK [~abc]\n"
    "point -> OK [~5,6]\n"
    "vector -> OK [~[1,2]]\n"
    "empty vector -> OK [~{}]\n"
    "set -> OK [~[a,b]]\n";

// CALLER_TRAITS
//   constant: what the steps of d_tests_arg_cpp_type_caller_traits write:
// the module's own, as CLI11 converts a type of the caller's by traits of
// its own, which a caller does not specialize.
const char CALLER_TRAITS[] =
    "length: type=[LENGTH] expected=1,1 type_size=1,1\n"
    "bag: type=[INT] expected=1,536870912 type_size=1,1\n"
    "lengths: type=[LENGTH] expected=1,536870912 type_size=1,1\n"
    "length default=[9m]\n"
    "length=9m\n"
    "bag default=[[1,2]]\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--length|--lengths|--lengths|--bag|--bag]\n"
    "opt length n=1 res=[2.5m]\n"
    "opt bag n=2 res=[4|5]\n"
    "opt lengths n=2 res=[1m|3m]\n"
    "length=2.5m\n"
    "lengths={1m,3m}\n"
    "bag={4,5}\n"
    "cmd app n=1 miss=[] subs=[] order=[--bag]\n"
    "opt length n=0 res=[]\n"
    "opt bag n=1 res=[{}]\n"
    "opt lengths n=0 res=[]\n"
    "bag={}\n"
    "ERR ConversionError 104: Could not convert: --length = 2.5\n"
    "cmd app n=1 miss=[] subs=[] order=[--length]\n"
    "opt length n=1 res=[2.5]\n"
    "opt bag n=0 res=[]\n"
    "opt lengths n=0 res=[]\n"
    "length=2.5m\n";

// DEPARTED
//   constant: what the steps of d_tests_arg_cpp_type_departures write: the
// module's own, where the face writes or reads a value otherwise than
// CLI11. CLI11 writes a default given as a real number to every digit
// max_digits10 allows -- 0.1 as `0.10000000000000001`, 0.1f as
// `0.100000001` -- and captures an 8-bit integer as a character, 65 as `A`
// (ISSUES 23); it writes a truth value and a character given as defaults
// with std::to_string, `true` as `1` and 'c' as `99` (ISSUES 60); it reads
// `010` as eight, where a parser here reads it as ten unless the option's
// rule is C's (D_CFG_ARG_PARSER_INTEGER_RULE); it leaves a variable a value
// does not convert into holding what its conversion wrote before it failed
// -- `[1]` for a vector given `1 x 3`, `(1,0)` for a complex number given
// `1+zi`, `{b:2}` for a map given `b 2 c x` (ISSUES 34); and, from C++20,
// it reads an aggregate as made from a number, `1,2` as 12 (ISSUES 53).
const char DEPARTED[] =
    "double default=[0.1]\n"
    "float default=[0.1]\n"
    "bool default=[true]\n"
    "char default=[c]\n"
    "captured unsigned char default=[65]\n"
    "captured bool default=[true]\n"
    "cmd app n=1 miss=[] subs=[] order=[--decimal|--octal]\n"
    "decimal=10\n"
    "octal=8\n"
    "ERR ConversionError 104: Could not convert: --ints = 1,x,3\n"
    "cmd app n=1 miss=[] subs=[] order=[--ints|--ints|--ints]\n"
    "ints={9}\n"
    "ERR ConversionError 104: Could not convert: --complex = 1+zi\n"
    "cmd app n=1 miss=[] subs=[] order=[--complex]\n"
    "complex=(5,6)\n"
    "ERR ConversionError 104: Could not convert: --map = b,2,c,x\n"
    "cmd app n=1 miss=[] subs=[] order=[--map|--map|--map|--map]\n"
    "map={(\"a\",1)}\n"
    "cell: type=[TEXT] expected=1,1 type_size=1,1\n"
    "cmd app n=1 miss=[] subs=[] order=[--cell]\n"
    "opt cell n=1 res=[1,2]\n"
    "cell=1,2\n";

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// DEPARTED_CPP11
//   constant: what the steps of d_tests_arg_cpp_type_departures_cpp11
// write: the module's own, where the face writes a value of a C++11 kind
// otherwise than CLI11. CLI11 writes a duration as a stream writes it:
// below C++20 as nothing, so that a default given as one puts 0 in a bound
// variable, and from C++20 with its unit, `9s`, which its conversion then
// cannot read, so that default_val throws ConversionError (ISSUES 53); and
// it writes a value of a type with no text form as nothing, an empty
// default it then puts, converted, in a bound variable -- `tag_t("")` for
// `tag_t("t")` (ISSUES 60).
const char DEPARTED_CPP11[] =
    "seconds default=[9]\n"
    "seconds=duration 9\n"
    "span default=[2.5]\n"
    "span=duration 2.5\n"
    "tag default=[]\n"
    "tag=tag \"old\"\n"
    "THROWN Unsupported: Unsupported\n";

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// DEPARTED_CPP17
//   constant: what the steps of d_tests_arg_cpp_type_departures_cpp17
// write: the module's own, where the face writes a value of a C++17 kind
// otherwise than CLI11: CLI11 cannot stream a std::optional, so it writes
// none (ISSUES 53).
const char DEPARTED_CPP17[] =
    "given default=[7]\n"
    "maybe=optional 7\n"
    "none default=[]\n"
    "captured default=[3]\n";

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// DEPARTED_WIDE
//   constant: what the steps of d_tests_arg_cpp_type_departures_wide write:
// the module's own, where the face writes or reads wide text otherwise than
// CLI11 (ISSUES 53): CLI11 writes a std::wstring as a container of its
// characters' numbers -- `[100,233]` for L"dé" -- and, from C++20,
// where a wchar_t no longer streams, as `[]`, which a default given so
// then puts in the variable; and its widen throws std::range_error out of
// the parse for a value that is not UTF-8.
const char DEPARTED_WIDE[] =
    "captured default=[d\xc3\xa9]\n"
    "given default=[d\xc3\xa9]\n"
    "wide=L{100,233}\n"
    "ERR ConversionError 104: Could not convert: --wide = \xff\n"
    "cmd app n=1 miss=[] subs=[] order=[--wide]\n"
    "wide=L{100,233}\n";

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// 1.  CASES
//==============================================================================
// What the sections held to CLI11 define, in the part of the face CLI11
// shares; tools/cppref/cppref.py compiles this section against CLI11.

// 1.1    Values
//------------------------------------------------------------------------------

// name_t
//   struct: a type made from text, and turned back into it, implicitly.
struct name_t
{
    std::string m_held;

    /**
     * @brief Makes a name holding no text.
     */
    name_t()
        : m_held()
    {}

    /**
     * @brief Makes a name from text, implicitly, as CLI11 assigns one.
     *
     * @param[in] _value  the text.
     */
    name_t(
        const std::string& _value
    )
        : m_held(_value)
    {}

    /**
     * @brief The text the name holds, implicitly, as CLI11 writes one.
     *
     * @return the text.
     */
    operator std::string() const
    {
        return m_held;
    }
};

// point
//   struct: a type read and written with stream operators, `x,y`. It has
// constructors, so it is no aggregate: from C++20, CLI11 makes an aggregate
// from a number (ISSUES 53), as the module's own sections show.
struct point
{
    int m_x;
    int m_y;

    /**
     * @brief Makes the point at the origin.
     */
    point()
        : m_x(0),
          m_y(0)
    {}

    /**
     * @brief Makes a point.
     *
     * @param[in] _x  the first coordinate.
     * @param[in] _y  the second.
     */
    point(
        int _x,
        int _y
    )
        : m_x(_x),
          m_y(_y)
    {}
};

/**
 * @brief Reads a point, `x,y`.
 *
 * @param[in,out] _in     the stream.
 * @param[out]    _point  receives the point.
 * @return the stream, failed for anything else.
 */
std::istream&
operator>>(
    std::istream& _in,
    point&        _point
)
{
    char comma = '\0';

    _in >> _point.m_x >> comma >> _point.m_y;

    // anything but a comma between the numbers
    if (comma != ',')
    {
        _in.setstate(std::ios_base::failbit);
    }

    return _in;
}

/**
 * @brief Writes a point, `x,y`.
 *
 * @param[in,out] _out    the stream.
 * @param[in]     _point  the point.
 * @return the stream.
 */
std::ostream&
operator<<(
    std::ostream& _out,
    const point&  _point
)
{
    return _out << _point.m_x << ',' << _point.m_y;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// colour
//   enum: an enumeration of its own scope, over `char`.
enum class colour : char
{
    red   = 1,
    green = 2
};

// level
//   enum: an enumeration with a negative value.
enum level
{
    low  = -1,
    high = 7
};

// tag_t
//   struct: a type made from text by an explicit constructor alone, and
// with no text form.
struct tag_t
{
    std::string m_held;

    /**
     * @brief Makes a tag holding no text.
     */
    tag_t()
        : m_held()
    {}

    /**
     * @brief Makes a tag from text, explicitly, as CLI11 constructs one.
     *
     * @param[in] _value  the text.
     */
    explicit tag_t(
        const std::string& _value
    )
        : m_held(_value)
    {}
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// The writers a transcript writes a variable with: a value as a stream
// writes it, by default, and those a stream writes otherwise or not at all
// each by an overload of its own -- a truth value as a word, a character
// and text between quotes, escaped, and a composed value of its parts. Each
// is declared before the composed writers that call it, as a call in a
// template finds only what is declared before the template but through the
// argument's own namespace.

/**
 * @brief A value as a stream writes it, in the classic locale.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Value>
std::string
streamed(
    const Value& _value
)
{
    std::ostringstream out;

    out << _value;

    return out.str();
}

/**
 * @brief A value of a type that has no overload of its own -- a number, a
 *        complex number, a streamed type -- as a stream writes it.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Value>
std::string
shown(
    const Value& _value
)
{
    return streamed(_value);
}

/**
 * @brief A truth value, as a word.
 *
 * @param[in] _value  the value.
 * @return "true" or "false".
 */
std::string
shown(
    bool _value
)
{
    return (_value) ? "true" : "false";
}

/**
 * @brief A character, between single quotes, escaped.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    char _value
)
{
    return "'" + escape(std::string(1u,
                                    _value)) + "'";
}

/**
 * @brief A signed character, as its number.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    signed char _value
)
{
    return streamed(static_cast<int>(_value));
}

/**
 * @brief An unsigned character, as its number.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    unsigned char _value
)
{
    return streamed(static_cast<unsigned int>(_value));
}

/**
 * @brief Text, between double quotes, escaped.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    const std::string& _value
)
{
    return "\"" + escape(_value) + "\"";
}

/**
 * @brief A name, as the text it holds.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    const name_t& _value
)
{
    return "name " + shown(_value.m_held);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief An enumeration of its own scope, which no stream writes, as its
 *        number.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    colour _value
)
{
    return "colour " + streamed(static_cast<int>(_value));
}

/**
 * @brief A tag, as the text it holds.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    const tag_t& _value
)
{
    return "tag " + shown(_value.m_held);
}

/**
 * @brief A duration, which a stream writes only from C++20, as its count.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Count,
         typename Period>
std::string
shown(
    const std::chrono::duration<Count, Period>& _value
)
{
    return "duration " + streamed(_value.count());
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/**
 * @brief A string view, as the text it views.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    std::string_view _value
)
{
    return "view " + shown(std::string(_value));
}

/**
 * @brief A path, as its generic text, the same on every system.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    const std::filesystem::path& _value
)
{
    return "path " + shown(_value.generic_string());
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/**
 * @brief Wide text, as its characters' numbers, as a transcript is
 *        narrow.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    const std::wstring& _value
)
{
    std::string out = "L{";

    // each character's number, after a comma when one comes before it
    for (std::size_t index = 0u; index < _value.size(); ++index)
    {
        out += (index > 0u) ? "," : "";
        out += streamed(static_cast<unsigned long>(_value[index]));
    }

    return out + "}";
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

/**
 * @brief A pair, `(first,second)`.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename First,
         typename Second>
std::string
shown(
    const std::pair<First, Second>& _value
)
{
    return "(" + shown(_value.first) + "," + shown(_value.second) + ")";
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief A tuple of three, `(first,second,third)`.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename First,
         typename Second,
         typename Third>
std::string
shown(
    const std::tuple<First, Second, Third>& _value
)
{
    return "(" + shown(std::get<0>(_value)) +
           "," + shown(std::get<1>(_value)) +
           "," + shown(std::get<2>(_value)) + ")";
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief The elements from one iterator to another, `{a,b}`.
 *
 * @param[in] _first  the first.
 * @param[in] _last   the end.
 * @return the text.
 */
template<typename Iterator>
std::string
shown_range(
    Iterator _first,
    Iterator _last
)
{
    std::string out = "{";

    // each element, after a comma when one comes before it
    for (Iterator at = _first; at != _last; ++at)
    {
        out += (at != _first) ? "," : "";
        out += shown(*at);
    }

    return out + "}";
}

/**
 * @brief A vector, its elements in order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Item>
std::string
shown(
    const std::vector<Item>& _value
)
{
    return shown_range(_value.begin(),
                       _value.end());
}

/**
 * @brief A list, its elements in order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Item>
std::string
shown(
    const std::list<Item>& _value
)
{
    return shown_range(_value.begin(),
                       _value.end());
}

/**
 * @brief A double-ended queue, its elements in order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Item>
std::string
shown(
    const std::deque<Item>& _value
)
{
    return shown_range(_value.begin(),
                       _value.end());
}

/**
 * @brief A set, its elements in order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Item>
std::string
shown(
    const std::set<Item>& _value
)
{
    return shown_range(_value.begin(),
                       _value.end());
}

/**
 * @brief A set of repeated elements, its elements in order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Item>
std::string
shown(
    const std::multiset<Item>& _value
)
{
    return shown_range(_value.begin(),
                       _value.end());
}

/**
 * @brief A map, its pairs in order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Key,
         typename Value>
std::string
shown(
    const std::map<Key, Value>& _value
)
{
    return shown_range(_value.begin(),
                       _value.end());
}

/**
 * @brief A map of repeated keys, its pairs in order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Key,
         typename Value>
std::string
shown(
    const std::multimap<Key, Value>& _value
)
{
    return shown_range(_value.begin(),
                       _value.end());
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief An array, its elements in order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Item,
         std::size_t Size>
std::string
shown(
    const std::array<Item, Size>& _value
)
{
    return shown_range(_value.begin(),
                       _value.end());
}

/**
 * @brief An unordered set, its elements sorted, as it has no order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Item>
std::string
shown(
    const std::unordered_set<Item>& _value
)
{
    std::vector<Item> sorted(_value.begin(),
                             _value.end());

    std::sort(sorted.begin(),
              sorted.end());

    return shown(sorted);
}

/**
 * @brief An unordered map, its pairs sorted, as it has no order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Key,
         typename Value>
std::string
shown(
    const std::unordered_map<Key, Value>& _value
)
{
    std::vector<std::pair<Key, Value> > sorted(_value.begin(),
                                               _value.end());

    std::sort(sorted.begin(),
              sorted.end());

    return shown(sorted);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/**
 * @brief An optional value: the value it holds, or "none".
 *
 * @param[in] _value  the value.
 * @return the text.
 */
template<typename Type>
std::string
shown(
    const std::optional<Type>& _value
)
{
    return (_value.has_value())
           ? ("optional " + shown(*_value))
           : std::string("none");
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

/**
 * @brief Writes how a bound option takes its values -- its type's name,
 *        the values it expects and the size of one -- on one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line names the option by.
 * @param[in]     _option  the option.
 */
void
write_bound(
    run&               _run,
    const char*        _label,
    const cli::option& _option
)
{
    std::ostringstream line;

    line << _label
         << ": type=[" << _option->get_type_name()
         << "] expected=" << _option->get_expected_min()
         << "," << _option->get_expected_max()
         << " type_size=" << _option->get_type_size_min()
         << "," << _option->get_type_size_max();
    _run.note(line.str());

    return;
}

/**
 * @brief Binds an option to a variable, watches it, and writes how it
 *        takes its values.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _label     what the option is reported under.
 * @param[in]     _names     its names.
 * @param[in,out] _variable  the variable.
 * @return the option.
 */
template<typename Type>
cli::option
bind(
    run&        _run,
    const char* _label,
    const char* _names,
    Type&       _variable
)
{
    const cli::option option = _run.watch(_label,
                                          _run.app().add_option(_names,
                                                                _variable));

    write_bound(_run,
                _label,
                option);

    return option;
}

/**
 * @brief Writes a variable on one line, `<label>=<value>`.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _label     what the line names the variable by.
 * @param[in]     _variable  the variable.
 */
template<typename Type>
void
write_value(
    run&        _run,
    const char* _label,
    const Type& _variable
)
{
    _run.note(std::string(_label) + "=" + shown(_variable));

    return;
}

/**
 * @brief Writes an option's default text on one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line names the default by.
 * @param[in]     _option  the option.
 */
void
write_default(
    run&               _run,
    const char*        _label,
    const cli::option& _option
)
{
    _run.note(std::string(_label) +
              " default=[" + escape(_option->get_default_str()) + "]");

    return;
}

// 1.2    Built-in types
//------------------------------------------------------------------------------

/**
 * @brief NUMBERS 1: signed integers of each width, given values at their
 *        limits and with a sign.
 *
 * @param[in,out] _run  the step's run.
 */
void
numbers_signed(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--int", "-2147483648",
        "--short", "32767",
        "--long", "-77",
        "--signed-char", "-128",
        "--plain", "+12"
    };
    int                      whole = 0;
    short                    small = 0;
    long                     wide  = 0;
    signed char              tiny  = 0;
    int                      plain = 0;

    (void)bind(_run,
               "int",
               "--int",
               whole);
    (void)bind(_run,
               "short",
               "--short",
               small);
    (void)bind(_run,
               "long",
               "--long",
               wide);
    (void)bind(_run,
               "signed_char",
               "--signed-char",
               tiny);
    (void)bind(_run,
               "plain",
               "--plain",
               plain);
    _run.parse(GIVEN,
               10u);
    write_value(_run,
                "int",
                whole);
    write_value(_run,
                "short",
                small);
    write_value(_run,
                "long",
                wide);
    write_value(_run,
                "signed_char",
                tiny);
    write_value(_run,
                "plain",
                plain);

    return;
}

/**
 * @brief NUMBERS 2: unsigned integers of each width, given values at their
 *        limits and in hexadecimal and octal.
 *
 * @param[in,out] _run  the step's run.
 */
void
numbers_unsigned(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--unsigned", "4294967295",
        "--ushort", "0x10",
        "--ulong", "0o17",
        "--uchar", "255"
    };
    unsigned int             whole = 0u;
    unsigned short           small = 0u;
    unsigned long            wide  = 0u;
    unsigned char            tiny  = 0u;

    (void)bind(_run,
               "unsigned",
               "--unsigned",
               whole);
    (void)bind(_run,
               "ushort",
               "--ushort",
               small);
    (void)bind(_run,
               "ulong",
               "--ulong",
               wide);
    (void)bind(_run,
               "uchar",
               "--uchar",
               tiny);
    _run.parse(GIVEN,
               8u);
    write_value(_run,
                "unsigned",
                whole);
    write_value(_run,
                "ushort",
                small);
    write_value(_run,
                "ulong",
                wide);
    write_value(_run,
                "uchar",
                tiny);

    return;
}

/**
 * @brief NUMBERS 3: real numbers of each width, in fixed and exponent
 *        forms.
 *
 * @param[in,out] _run  the step's run.
 */
void
numbers_real(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--float", "0.25",
        "--double", "-1.5e3",
        "--long-double", "7e-2"
    };
    float                    single  = 0.0f;
    double                   twice   = 0.0;
    long double              longest = 0.0L;

    (void)bind(_run,
               "float",
               "--float",
               single);
    (void)bind(_run,
               "double",
               "--double",
               twice);
    (void)bind(_run,
               "long_double",
               "--long-double",
               longest);
    _run.parse(GIVEN,
               6u);
    write_value(_run,
                "float",
                single);
    write_value(_run,
                "double",
                twice);
    write_value(_run,
                "long_double",
                longest);

    return;
}

/**
 * @brief NUMBERS 4: truth values in each kind of their words, and
 *        characters given as one and as a number.
 *
 * @param[in,out] _run  the step's run.
 */
void
numbers_logical(
    run& _run
)
{
    static const char* const FIRST[]  =
    {
        "--truth", "yes",
        "--falsity", "off",
        "--char", "a",
        "--digit", "7"
    };
    static const char* const SECOND[] =
    {
        "--truth", "0",
        "--falsity", "TRUE",
        "--char", "65"
    };
    bool                     truth    = false;
    bool                     falsity  = true;
    char                     letter   = '\0';
    char                     digit    = '\0';

    (void)bind(_run,
               "truth",
               "--truth",
               truth);
    (void)bind(_run,
               "falsity",
               "--falsity",
               falsity);
    (void)bind(_run,
               "char",
               "--char",
               letter);
    (void)bind(_run,
               "digit",
               "--digit",
               digit);
    _run.parse(FIRST,
               8u);
    write_value(_run,
                "truth",
                truth);
    write_value(_run,
                "falsity",
                falsity);
    write_value(_run,
                "char",
                letter);
    write_value(_run,
                "digit",
                digit);
    _run.parse(SECOND,
               6u);
    write_value(_run,
                "truth",
                truth);
    write_value(_run,
                "falsity",
                falsity);
    write_value(_run,
                "char",
                letter);

    return;
}

/**
 * @brief NUMBERS 5: values a number's type refuses: past its limit, not a
 *        number, a truth value no word gives, text after a real number, a
 *        sign before an unsigned one.
 *
 * @param[in,out] _run  the step's run.
 */
void
numbers_refused(
    run& _run
)
{
    static const char* const BIG[]   = { "--uchar", "256" };
    static const char* const WORD[]  = { "--int", "twelve" };
    static const char* const TRUTH[] = { "--truth", "maybe" };
    static const char* const REAL[]  = { "--double", "1.5x" };
    static const char* const SIGN[]  = { "--unsigned", "-1" };
    unsigned char            tiny    = 9u;
    int                      whole   = 9;
    bool                     truth   = true;
    double                   twice   = 9.0;
    unsigned int             count   = 9u;

    (void)_run.app().add_option("--uchar",
                                tiny);
    (void)_run.app().add_option("--int",
                                whole);
    (void)_run.app().add_option("--truth",
                                truth);
    (void)_run.app().add_option("--double",
                                twice);
    (void)_run.app().add_option("--unsigned",
                                count);
    _run.parse(BIG,
               2u);
    _run.parse(WORD,
               2u);
    _run.parse(TRUTH,
               2u);
    _run.parse(REAL,
               2u);
    _run.parse(SIGN,
               2u);

    return;
}

// NUMBERS_STEPS
//   constant: the steps of d_tests_arg_cpp_type_numbers.
const step_fn NUMBERS_STEPS[] =
{
    &numbers_signed,
    &numbers_unsigned,
    &numbers_real,
    &numbers_logical,
    &numbers_refused
};

// NUMBERS_COUNT
//   constant: how many steps NUMBERS_STEPS holds.
const std::size_t NUMBERS_COUNT =
    sizeof(NUMBERS_STEPS) / sizeof(NUMBERS_STEPS[0]);

#if D_ENV_HAS_LONG_LONG
D_LONG_LONG_DIAG_PUSH

/**
 * @brief NUMBERS_LONG_LONG 1: the widest integers, given values at their
 *        limits, and one past.
 *
 * @param[in,out] _run  the step's run.
 */
void
numbers_long_long(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--long-long", "-9223372036854775808",
        "--ulong-long", "18446744073709551615"
    };
    static const char* const PAST[]  =
    {
        "--long-long", "9223372036854775808"
    };
    long long                wide    = 0;
    unsigned long long       widest  = 0u;

    (void)bind(_run,
               "long_long",
               "--long-long",
               wide);
    (void)bind(_run,
               "ulong_long",
               "--ulong-long",
               widest);
    _run.parse(GIVEN,
               4u);
    write_value(_run,
                "long_long",
                wide);
    write_value(_run,
                "ulong_long",
                widest);
    _run.parse(PAST,
               2u);

    return;
}

D_LONG_LONG_DIAG_POP

// NUMBERS_LONG_LONG_STEPS
//   constant: the steps of d_tests_arg_cpp_type_numbers_long_long.
const step_fn NUMBERS_LONG_LONG_STEPS[] =
{
    &numbers_long_long
};

// NUMBERS_LONG_LONG_COUNT
//   constant: how many steps NUMBERS_LONG_LONG_STEPS holds.
const std::size_t NUMBERS_LONG_LONG_COUNT =
    sizeof(NUMBERS_LONG_LONG_STEPS) / sizeof(NUMBERS_LONG_LONG_STEPS[0]);

#endif  // D_ENV_HAS_LONG_LONG

/**
 * @brief TEXTS 1: text, with a space and empty; a type made from text; a
 *        type read with streams, and a value it refuses.
 *
 * @param[in,out] _run  the step's run.
 */
void
texts_bound(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--text", "a b",
        "--empty", "",
        "--name", "named",
        "--point", "3,4"
    };
    static const char* const BAD[]   = { "--point", "3;4" };
    std::string              text("kept");
    std::string              empty("kept");
    name_t                   name;
    point                    where;

    (void)bind(_run,
               "text",
               "--text",
               text);
    (void)bind(_run,
               "empty",
               "--empty",
               empty);
    (void)bind(_run,
               "name",
               "--name",
               name);
    (void)bind(_run,
               "point",
               "--point",
               where);
    _run.parse(GIVEN,
               8u);
    write_value(_run,
                "text",
                text);
    write_value(_run,
                "empty",
                empty);
    write_value(_run,
                "name",
                name);
    write_value(_run,
                "point",
                where);
    _run.parse(BAD,
               2u);

    return;
}

// TEXTS_STEPS
//   constant: the steps of d_tests_arg_cpp_type_texts.
const step_fn TEXTS_STEPS[] =
{
    &texts_bound
};

// TEXTS_COUNT
//   constant: how many steps TEXTS_STEPS holds.
const std::size_t TEXTS_COUNT = sizeof(TEXTS_STEPS) / sizeof(TEXTS_STEPS[0]);

// 1.3    Composed types
//------------------------------------------------------------------------------

/**
 * @brief CONTAINERS 1: sequences -- a vector, a list, a double-ended queue
 *        -- each given several values, one negative.
 *
 * @param[in,out] _run  the step's run.
 */
void
containers_sequences(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--ints", "1", "2", "3",
        "--reals", "0.5", "-2",
        "--chars", "x", "y"
    };
    std::vector<int>         ints;
    std::list<double>        reals;
    std::deque<char>         chars;

    (void)bind(_run,
               "ints",
               "--ints",
               ints);
    (void)bind(_run,
               "reals",
               "--reals",
               reals);
    (void)bind(_run,
               "chars",
               "--chars",
               chars);
    _run.parse(GIVEN,
               10u);
    write_value(_run,
                "ints",
                ints);
    write_value(_run,
                "reals",
                reals);
    write_value(_run,
                "chars",
                chars);

    return;
}

/**
 * @brief CONTAINERS 2: sets, of unique values and of repeated ones, given
 *        values out of order and twice.
 *
 * @param[in,out] _run  the step's run.
 */
void
containers_sets(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--set", "b", "a", "b",
        "--multi", "3", "1", "3"
    };
    std::set<std::string>    unique;
    std::multiset<int>       many;

    (void)bind(_run,
               "set",
               "--set",
               unique);
    (void)bind(_run,
               "multi",
               "--multi",
               many);
    _run.parse(GIVEN,
               8u);
    write_value(_run,
                "set",
                unique);
    write_value(_run,
                "multi",
                many);

    return;
}

/**
 * @brief CONTAINERS 3: maps and pairs in a vector, given keys and values,
 *        a key twice, and a pair cut short.
 *
 * @param[in,out] _run  the step's run.
 */
void
containers_maps(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--map", "a", "1", "b", "2", "a", "3",
        "--multi", "1", "1.5", "1", "2.5",
        "--pairs", "x", "7", "y", "8"
    };
    static const char* const ODD[]   = { "--pairs", "x", "7", "y" };
    std::map<std::string, int>                map;
    std::multimap<int, double>                multi;
    std::vector<std::pair<std::string, int> > pairs;

    (void)bind(_run,
               "map",
               "--map",
               map);
    (void)bind(_run,
               "multi",
               "--multi",
               multi);
    (void)bind(_run,
               "pairs",
               "--pairs",
               pairs);
    _run.parse(GIVEN,
               17u);
    write_value(_run,
                "map",
                map);
    write_value(_run,
                "multi",
                multi);
    write_value(_run,
                "pairs",
                pairs);
    _run.parse(ODD,
               4u);

    return;
}

/**
 * @brief CONTAINERS 4: "{}", which empties a container an option may give
 *        no values to, and is a value like any other given one that needs
 *        values: an element of a vector of text, refused by a vector of
 *        numbers.
 *
 * @param[in,out] _run  the step's run.
 */
void
containers_empty(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--may", "{}",
        "--texts", "{}"
    };
    static const char* const BAD[]   = { "--ints", "{}" };
    std::vector<int>         may(1u,
                                 9);
    std::vector<std::string> texts(1u,
                                   std::string("kept"));
    std::vector<int>         ints(1u,
                                  9);

    bind(_run,
         "may",
         "--may",
         may)->expected(0,
                        2);
    (void)bind(_run,
               "texts",
               "--texts",
               texts);
    (void)bind(_run,
               "ints",
               "--ints",
               ints);
    _run.parse(GIVEN,
               4u);
    write_value(_run,
                "may",
                may);
    write_value(_run,
                "texts",
                texts);
    _run.parse(BAD,
               2u);

    return;
}

/**
 * @brief CONTAINERS 5: an element a container's type refuses.
 *
 * @param[in,out] _run  the step's run.
 */
void
containers_refused(
    run& _run
)
{
    static const char* const BAD[] = { "--ints", "1", "x", "3" };
    std::vector<int>         ints;

    (void)bind(_run,
               "ints",
               "--ints",
               ints);
    _run.parse(BAD,
               4u);

    return;
}

// CONTAINERS_STEPS
//   constant: the steps of d_tests_arg_cpp_type_containers.
const step_fn CONTAINERS_STEPS[] =
{
    &containers_sequences,
    &containers_sets,
    &containers_maps,
    &containers_empty,
    &containers_refused
};

// CONTAINERS_COUNT
//   constant: how many steps CONTAINERS_STEPS holds.
const std::size_t CONTAINERS_COUNT =
    sizeof(CONTAINERS_STEPS) / sizeof(CONTAINERS_STEPS[0]);

/**
 * @brief COMPOUND 1: a pair given its two values, and a first value it
 *        refuses.
 *
 * @param[in,out] _run  the step's run.
 */
void
compound_pair(
    run& _run
)
{
    static const char* const GIVEN[] = { "--pair", "3", "x" };
    static const char* const BAD[]   = { "--pair", "nope", "y" };
    std::pair<int, std::string> pair(0,
                                     std::string());

    (void)bind(_run,
               "pair",
               "--pair",
               pair);
    _run.parse(GIVEN,
               3u);
    write_value(_run,
                "pair",
                pair);
    _run.parse(BAD,
               3u);
    write_value(_run,
                "pair",
                pair);

    return;
}

/**
 * @brief COMPOUND 2: complex numbers in each form CLI11 reads: a sum with
 *        `i`, two values with `j`, an imaginary part alone, a pair in
 *        parentheses, a real part alone, an exponent; and one refused.
 *
 * @param[in,out] _run  the step's run.
 */
void
compound_complex(
    run& _run
)
{
    static const char* const SUM[]       = { "--complex", "1+2i" };
    static const char* const TWO[]       = { "--complex", "1", "2j" };
    static const char* const IMAGINARY[] = { "--complex", "-3i" };
    static const char* const PAIR[]      = { "--complex", "(1,2)" };
    static const char* const REAL[]      = { "--complex", "4" };
    static const char* const EXPONENT[]  = { "--complex", "1e-5" };
    static const char* const BAD[]       = { "--complex", "1+zi" };
    std::complex<double>     number;

    (void)bind(_run,
               "complex",
               "--complex",
               number);
    _run.parse(SUM,
               2u);
    write_value(_run,
                "complex",
                number);
    _run.parse(TWO,
               3u);
    write_value(_run,
                "complex",
                number);
    _run.parse(IMAGINARY,
               2u);
    write_value(_run,
                "complex",
                number);
    _run.parse(PAIR,
               2u);
    write_value(_run,
                "complex",
                number);
    _run.parse(REAL,
               2u);
    write_value(_run,
                "complex",
                number);
    _run.parse(EXPONENT,
               2u);
    write_value(_run,
                "complex",
                number);
    _run.parse(BAD,
               2u);

    return;
}

// COMPOUND_STEPS
//   constant: the steps of d_tests_arg_cpp_type_compound.
const step_fn COMPOUND_STEPS[] =
{
    &compound_pair,
    &compound_complex
};

// COMPOUND_COUNT
//   constant: how many steps COMPOUND_STEPS holds.
const std::size_t COMPOUND_COUNT =
    sizeof(COMPOUND_STEPS) / sizeof(COMPOUND_STEPS[0]);

/**
 * @brief DEFAULTS 1: defaults given as values of the types: an integer,
 *        text, a pair, a complex number, a type made from text and a
 *        streamed type.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_values(
    run& _run
)
{
    cli::option defaulted = _run.app().add_option("--defaulted");

    defaulted->default_val(42);
    write_default(_run,
                  "int",
                  defaulted);
    defaulted->default_val(std::string("some text"));
    write_default(_run,
                  "string",
                  defaulted);
    defaulted->default_val(std::make_pair(1,
                                          std::string("a")));
    write_default(_run,
                  "pair",
                  defaulted);
    defaulted->default_val(std::complex<float>(1.5f,
                                               -2.0f));
    write_default(_run,
                  "complex",
                  defaulted);
    defaulted->default_val(name_t("given"));
    write_default(_run,
                  "name",
                  defaulted);
    defaulted->default_val(point(5,
                                 6));
    write_default(_run,
                  "point",
                  defaulted);

    return;
}

/**
 * @brief DEFAULTS 2: defaults given as containers: a vector, empty and
 *        not, a list, a set, a map and pairs in a vector.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_containers(
    run& _run
)
{
    std::vector<int>                          values;
    std::list<double>                         reals;
    std::set<std::string>                     unique;
    std::map<std::string, int>                map;
    std::vector<std::pair<std::string, int> > pairs;
    cli::option                               defaulted =
        _run.app().add_option("--defaulted");

    values.push_back(1);
    values.push_back(2);
    reals.push_back(0.5);
    reals.push_back(-2.0);
    unique.insert("b");
    unique.insert("a");
    map["a"] = 1;
    map["b"] = 2;
    pairs.push_back(std::make_pair(std::string("x"),
                                   7));
    defaulted->default_val(values);
    write_default(_run,
                  "vector",
                  defaulted);
    defaulted->default_val(std::vector<int>());
    write_default(_run,
                  "empty",
                  defaulted);
    defaulted->default_val(reals);
    write_default(_run,
                  "list",
                  defaulted);
    defaulted->default_val(unique);
    write_default(_run,
                  "set",
                  defaulted);
    defaulted->default_val(map);
    write_default(_run,
                  "map",
                  defaulted);
    defaulted->default_val(pairs);
    write_default(_run,
                  "pairs",
                  defaulted);

    return;
}

/**
 * @brief DEFAULTS 3: a bound variable's default captured, then given, and
 *        the values a parse gives read back with as<T>() as several types.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_read(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--count", "7",
        "--word", "12",
        "--list", "1", "2", "3"
    };
    int                      count   = 3;
    cli::option              counted = _run.app().add_option("--count",
                                                             count);
    const cli::option        word    = _run.app().add_option("--word");
    const cli::option        list    = _run.app().add_option("--list")
                                                ->expected(1,
                                                           3);

    counted->capture_default_str();
    write_default(_run,
                  "captured",
                  counted);
    counted->default_val(5);
    write_default(_run,
                  "given",
                  counted);
    write_value(_run,
                "count",
                count);
    (void)_run.parse_quiet(GIVEN,
                           8u);
    _run.note("as: count=" + shown(counted->as<int>()) +
              " word=" + shown(word->as<std::string>()) +
              "," + shown(word->as<double>()) +
              "," + shown(word->as<unsigned char>()) +
              " list=" + shown(list->as<std::vector<int> >()) +
              "," + shown(list->as<std::vector<std::string> >()));

    return;
}

// DEFAULTS_STEPS
//   constant: the steps of d_tests_arg_cpp_type_defaults.
const step_fn DEFAULTS_STEPS[] =
{
    &defaults_values,
    &defaults_containers,
    &defaults_read
};

// DEFAULTS_COUNT
//   constant: how many steps DEFAULTS_STEPS holds.
const std::size_t DEFAULTS_COUNT =
    sizeof(DEFAULTS_STEPS) / sizeof(DEFAULTS_STEPS[0]);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// 1.4    The kinds C++11 brings
//------------------------------------------------------------------------------

/**
 * @brief KINDS_CPP11 1: a tuple, an array and a vector of tuples given
 *        their values, and a separator given inside a tuple.
 *
 * @param[in,out] _run  the step's run.
 */
void
kinds_tuples(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--tuple", "3", "1.5", "z",
        "--array", "1", "2", "3",
        "--triples", "1", "2", "3", "4", "5", "6"
    };
    static const char* const CUT[]   =
    {
        "--triples", "1", "2", "%%", "4", "5", "6"
    };
    std::tuple<int, double, std::string>    tuple;
    std::array<int, 3>                      array = {{ 0, 0, 0 }};
    std::vector<std::tuple<int, int, int> > triples;

    (void)bind(_run,
               "tuple",
               "--tuple",
               tuple);
    (void)bind(_run,
               "array",
               "--array",
               array);
    (void)bind(_run,
               "triples",
               "--triples",
               triples);
    _run.parse(GIVEN,
               15u);
    write_value(_run,
                "tuple",
                tuple);
    write_value(_run,
                "array",
                array);
    write_value(_run,
                "triples",
                triples);
    _run.parse(CUT,
               7u);

    return;
}

/**
 * @brief KINDS_CPP11 2: enumerations, of their own scope and with a
 *        negative value, given numbers, and a name refused.
 *
 * @param[in,out] _run  the step's run.
 */
void
kinds_enumerations(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--colour", "2",
        "--level", "-1"
    };
    static const char* const BAD[]   = { "--colour", "green" };
    colour                   hue     = colour::red;
    level                    height  = high;

    (void)bind(_run,
               "colour",
               "--colour",
               hue);
    (void)bind(_run,
               "level",
               "--level",
               height);
    _run.parse(GIVEN,
               4u);
    write_value(_run,
                "colour",
                hue);
    write_value(_run,
                "level",
                height);
    _run.parse(BAD,
               2u);

    return;
}

/**
 * @brief KINDS_CPP11 3: unordered containers, a type made from text by an
 *        explicit constructor, and durations counted in whole seconds and
 *        in a real number of them.
 *
 * @param[in,out] _run  the step's run.
 */
void
kinds_others(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--hashed", "3", "1", "2",
        "--hashed-map", "b", "2", "a", "1",
        "--tag", "t",
        "--seconds", "9",
        "--span", "1.5"
    };
    std::unordered_set<int>              hashed;
    std::unordered_map<std::string, int> hashed_map;
    tag_t                                tag;
    std::chrono::seconds                 seconds(0);
    std::chrono::duration<double>        span(0.0);

    (void)bind(_run,
               "hashed",
               "--hashed",
               hashed);
    (void)bind(_run,
               "hashed_map",
               "--hashed-map",
               hashed_map);
    (void)bind(_run,
               "tag",
               "--tag",
               tag);
    (void)bind(_run,
               "seconds",
               "--seconds",
               seconds);
    (void)bind(_run,
               "span",
               "--span",
               span);
    _run.parse(GIVEN,
               15u);
    write_value(_run,
                "hashed",
                hashed);
    write_value(_run,
                "hashed_map",
                hashed_map);
    write_value(_run,
                "tag",
                tag);
    write_value(_run,
                "seconds",
                seconds);
    write_value(_run,
                "span",
                span);

    return;
}

/**
 * @brief KINDS_CPP11 4: defaults given as values of these types -- a
 *        tuple, an array, enumerations -- and one captured from a type
 *        with no text form.
 *
 * @param[in,out] _run  the step's run.
 */
void
kinds_defaults(
    run& _run
)
{
    const std::array<int, 2> two       = {{ 4, 5 }};
    tag_t                    tag(std::string("t"));
    cli::option              defaulted = _run.app().add_option("--defaulted");
    cli::option              tagged    = _run.app().add_option("--tag",
                                                               tag);

    defaulted->default_val(std::make_tuple(1,
                                           std::string("b"),
                                           7u));
    write_default(_run,
                  "tuple",
                  defaulted);
    defaulted->default_val(two);
    write_default(_run,
                  "array",
                  defaulted);
    defaulted->default_val(low);
    write_default(_run,
                  "level",
                  defaulted);
    defaulted->default_val(colour::green);
    write_default(_run,
                  "colour",
                  defaulted);
    tagged->capture_default_str();
    write_default(_run,
                  "tag",
                  tagged);

    return;
}

// KINDS_CPP11_STEPS
//   constant: the steps of d_tests_arg_cpp_type_kinds_cpp11.
const step_fn KINDS_CPP11_STEPS[] =
{
    &kinds_tuples,
    &kinds_enumerations,
    &kinds_others,
    &kinds_defaults
};

// KINDS_CPP11_COUNT
//   constant: how many steps KINDS_CPP11_STEPS holds.
const std::size_t KINDS_CPP11_COUNT =
    sizeof(KINDS_CPP11_STEPS) / sizeof(KINDS_CPP11_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// 1.5    The kinds C++17 brings
//------------------------------------------------------------------------------

/**
 * @brief KINDS_CPP17 1: optional values, given and not, an optional
 *        vector, a string view and a path; and a view's and a path's
 *        defaults.
 *
 * @param[in,out] _run  the step's run.
 */
void
kinds_cpp17(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--maybe", "7",
        "--values", "7", "8",
        "--view", "viewed",
        "--path", "/x y/z"
    };
    std::optional<int>              maybe;
    std::optional<int>              absent;
    std::optional<std::vector<int>> values;
    std::string_view                view;
    std::filesystem::path           path;

    (void)bind(_run,
               "maybe",
               "--maybe",
               maybe);
    (void)bind(_run,
               "absent",
               "--absent",
               absent);
    (void)bind(_run,
               "values",
               "--values",
               values);

    cli::option viewed = bind(_run,
                              "view",
                              "--view",
                              view);
    cli::option pathed = bind(_run,
                              "path",
                              "--path",
                              path);

    _run.parse(GIVEN,
               9u);
    write_value(_run,
                "maybe",
                maybe);
    write_value(_run,
                "absent",
                absent);
    write_value(_run,
                "values",
                values);
    write_value(_run,
                "view",
                view);
    write_value(_run,
                "path",
                path);
    viewed->default_val(std::string_view("seen"));
    write_default(_run,
                  "view",
                  viewed);
    pathed->default_val(std::filesystem::path("/a b/c"));
    write_default(_run,
                  "path",
                  pathed);

    return;
}

// KINDS_CPP17_STEPS
//   constant: the steps of d_tests_arg_cpp_type_kinds_cpp17.
const step_fn KINDS_CPP17_STEPS[] =
{
    &kinds_cpp17
};

// KINDS_CPP17_COUNT
//   constant: how many steps KINDS_CPP17_STEPS holds.
const std::size_t KINDS_CPP17_COUNT =
    sizeof(KINDS_CPP17_STEPS) / sizeof(KINDS_CPP17_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// 1.6    Wide text
//------------------------------------------------------------------------------

/**
 * @brief WIDE_TEXT 1: wide text given UTF-8 beyond ASCII and with a space,
 *        and a vector of it.
 *
 * @param[in,out] _run  the step's run.
 */
void
wide_bound(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--wide", "\xd0\xb6 x",
        "--wides", "a", "\xc3\xa9"
    };
    std::wstring              wide;
    std::vector<std::wstring> wides;

    (void)bind(_run,
               "wide",
               "--wide",
               wide);
    (void)bind(_run,
               "wides",
               "--wides",
               wides);
    _run.parse(GIVEN,
               5u);
    write_value(_run,
                "wide",
                wide);
    write_value(_run,
                "wides",
                wides);

    return;
}

// WIDE_TEXT_STEPS
//   constant: the steps of d_tests_arg_cpp_type_wide_text.
const step_fn WIDE_TEXT_STEPS[] =
{
    &wide_bound
};

// WIDE_TEXT_COUNT
//   constant: how many steps WIDE_TEXT_STEPS holds.
const std::size_t WIDE_TEXT_COUNT =
    sizeof(WIDE_TEXT_STEPS) / sizeof(WIDE_TEXT_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

}  // namespace

//==============================================================================
// 2.  THE MODULE'S OWN
//==============================================================================
// What CLI11 cannot write: the face's descriptors and its conversions called
// directly, types given traits of the caller's own, and the departures. Each
// is a step written here, through the whole face.

namespace
{

// 2.1    Descriptors and conversions
//------------------------------------------------------------------------------

/**
 * @brief Whether a descriptor is one of the C library's built-in ones.
 *
 * @param[in] _type  the descriptor.
 * @return whether d_arg_type_builtin gives it for some d_arg_builtin.
 */
bool
is_builtin(
    const ::d_arg_type* _type
)
{
    // each built-in descriptor, until one is it
    for (int builtin = 0; builtin < D_ARG_BUILTIN_COUNT; ++builtin)
    {
        // this one is it
        if (d_arg_type_builtin(builtin) == _type)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Writes a type's name and its descriptor's shape on one line:
 *        whether it is a container, how many values it expects and makes
 *        one item of, whether it reads and writes, whether it is the C
 *        library's own, and whether a second ask gives the same one.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line names the type by.
 */
template<typename Type>
void
write_type(
    run&        _run,
    const char* _label
)
{
    const ::d_arg_type* const type = cli::type_traits<Type>::descriptor();
    std::ostringstream        line;

    line << _label
         << ": name=[" << cli::type_name<Type>()
         << "] container="
         << ((type->flags & D_ARG_TYPE_FLAG_CONTAINER) != 0u)
         << " expected=" << type->expected
         << " size=" << type->type_size_min
         << "," << type->type_size_max
         << " reads=" << (type->convert != D_NULLPTR)
         << " writes=" << (type->format != D_NULLPTR)
         << " builtin=" << is_builtin(type)
         << " once="
         << (type == cli::type_traits<Type>::descriptor());
    _run.note(line.str());

    return;
}

/**
 * @brief TYPE_NAMES 1: the name and shape of each kind of type's
 *        descriptor; a built-in type's, and one of its other names', is
 *        the C library's own; a made one is made once.
 *
 * @param[in,out] _run  the step's run.
 */
void
names_each(
    run& _run
)
{
    write_type<int>(_run,
                    "int");
    write_type<unsigned short>(_run,
                               "unsigned short");
    write_type<std::size_t>(_run,
                            "size_t");
    write_type<std::ptrdiff_t>(_run,
                               "ptrdiff_t");
    write_type<double>(_run,
                       "double");
    write_type<bool>(_run,
                     "bool");
    write_type<char>(_run,
                     "char");
    write_type<const char*>(_run,
                            "const char*");
    write_type<cli::text>(_run,
                          "text");
    write_type<std::string>(_run,
                            "std::string");
    write_type<std::vector<int> >(_run,
                                  "vector<int>");
    write_type<std::set<std::string> >(_run,
                                       "set<string>");
    write_type<std::pair<int, std::string> >(_run,
                                             "pair<int,string>");
    write_type<std::map<std::string, int> >(_run,
                                            "map<string,int>");
    write_type<std::complex<double> >(_run,
                                      "complex<double>");
    write_type<name_t>(_run,
                       "name_t");
    write_type<point>(_run,
                      "point");

    return;
}

// TYPE_NAMES_STEPS
//   constant: the steps of d_tests_arg_cpp_type_names.
const step_fn TYPE_NAMES_STEPS[] =
{
    &names_each
};

// TYPE_NAMES_COUNT
//   constant: how many steps TYPE_NAMES_STEPS holds.
const std::size_t TYPE_NAMES_COUNT =
    sizeof(TYPE_NAMES_STEPS) / sizeof(TYPE_NAMES_STEPS[0]);

/**
 * @brief Converts a value, and writes the status and the target after on
 *        one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _value   the value.
 * @param[in,out] _target  the target; written only when the value
 *                         converts.
 */
template<typename Type>
void
write_converted(
    run&        _run,
    const char* _value,
    Type&       _target
)
{
    const cli::status converted = cli::convert(cli::text(_value),
                                               _target);

    _run.note("[" + escape(_value) + "] -> " + converted.get_name() + " " +
              shown(_target));

    return;
}

/**
 * @brief Writes a value's text, appended to "~", and the status, on one
 *        line.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line names the value by.
 * @param[in]     _value  the value.
 */
template<typename Type>
void
write_text(
    run&        _run,
    const char* _label,
    const Type& _value
)
{
    std::string       out("~");
    const cli::status written = cli::to_text(_value,
                                             out);

    _run.note(std::string(_label) + " -> " + written.get_name() + " [" +
              escape(out) + "]");

    return;
}

/**
 * @brief CONVERTED 1: convert() over values of several types -- given,
 *        empty, refused with the target kept, "{}" alone emptying a
 *        container -- and under C's integer rule.
 *
 * @param[in,out] _run  the step's run.
 */
void
converted_values(
    run& _run
)
{
    int              whole = 5;
    unsigned char    tiny  = 0u;
    bool             truth = false;
    double           real  = 0.0;
    std::string      text("keep");
    point            where;
    std::vector<int> numbers(1u,
                             9);

    write_converted(_run,
                    "42",
                    whole);
    write_converted(_run,
                    "010",
                    whole);
    write_converted(_run,
                    "x",
                    whole);
    write_converted(_run,
                    "",
                    whole);
    write_converted(_run,
                    "200",
                    tiny);
    write_converted(_run,
                    "300",
                    tiny);
    write_converted(_run,
                    "yes",
                    truth);
    write_converted(_run,
                    "1.5e1",
                    real);
    write_converted(_run,
                    "hello",
                    text);
    write_converted(_run,
                    "3,4",
                    where);
    write_converted(_run,
                    "3,4x",
                    where);
    write_converted(_run,
                    "7",
                    numbers);
    write_converted(_run,
                    "{}",
                    numbers);

    const cli::status octal = cli::convert(cli::text("010"),
                                           cli::integer_rule::c,
                                           whole);

    _run.note(std::string("[010] under C's rule -> ") + octal.get_name() +
              " " + shown(whole));

    return;
}

/**
 * @brief CONVERTED 2: to_text() of values of several types, appended to
 *        what the string held.
 *
 * @param[in,out] _run  the step's run.
 */
void
converted_texts(
    run& _run
)
{
    std::vector<int>      numbers;
    std::set<std::string> unique;

    numbers.push_back(1);
    numbers.push_back(2);
    unique.insert("b");
    unique.insert("a");
    write_text(_run,
               "int",
               17);
    write_text(_run,
               "double",
               0.1);
    write_text(_run,
               "string",
               std::string("abc"));
    write_text(_run,
               "point",
               point(5,
                     6));
    write_text(_run,
               "vector",
               numbers);
    write_text(_run,
               "empty vector",
               std::vector<int>());
    write_text(_run,
               "set",
               unique);

    return;
}

// CONVERTED_STEPS
//   constant: the steps of d_tests_arg_cpp_type_convert.
const step_fn CONVERTED_STEPS[] =
{
    &converted_values,
    &converted_texts
};

// CONVERTED_COUNT
//   constant: how many steps CONVERTED_STEPS holds.
const std::size_t CONVERTED_COUNT =
    sizeof(CONVERTED_STEPS) / sizeof(CONVERTED_STEPS[0]);

// 2.2    A caller's traits
//------------------------------------------------------------------------------

// meters
//   struct: a length, which the caller converts with traits of its own: a
// number and the letter `m`.
struct meters
{
    double m_value;
};

// bag
//   struct: a container of the caller's own, with only what
// container_traits needs of one, its numbers kept in a list.
struct bag
{
    typedef int                            value_type;
    typedef std::list<int>::iterator       iterator;
    typedef std::list<int>::const_iterator const_iterator;

    std::list<int> m_items;

    /**
     * @brief The first number.
     *
     * @return an iterator on it.
     */
    iterator
    begin()
    {
        return m_items.begin();
    }

    /**
     * @brief The first number, to read.
     *
     * @return an iterator on it.
     */
    const_iterator
    begin() const
    {
        return m_items.begin();
    }

    /**
     * @brief The end of the numbers.
     *
     * @return an iterator past the last.
     */
    iterator
    end()
    {
        return m_items.end();
    }

    /**
     * @brief The end of the numbers, to read.
     *
     * @return an iterator past the last.
     */
    const_iterator
    end() const
    {
        return m_items.end();
    }

    /**
     * @brief Whether the bag holds no number.
     *
     * @return whether it is empty.
     */
    bool
    empty() const
    {
        return m_items.empty();
    }

    /**
     * @brief Exchanges the numbers of two bags.
     *
     * @param[in,out] _other  the other bag.
     */
    void
    swap(
        bag& _other
    )
    {
        m_items.swap(_other.m_items);

        return;
    }

    /**
     * @brief Puts a number in the bag.
     *
     * @param[in] _at     where.
     * @param[in] _value  the number.
     * @return an iterator on it.
     */
    iterator
    insert(
        iterator   _at,
        const int& _value
    )
    {
        return m_items.insert(_at,
                              _value);
    }
};

/**
 * @brief Reads meters, `<number>m`; a d_arg_convert_fn.
 *
 * @param[in]  _type     the descriptor; unused.
 * @param[in]  _values   the values; the first is read.
 * @param[in]  _count    how many.
 * @param[out] _target   the meters, written when the value reads.
 * @param[in]  _context  the conversion's context; unused.
 * @return D_ARG_OK, or D_ARG_ERROR_CONVERSION.
 */
int
meters_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;
    (void)_context;

    // one value, of a number and `m`
    if ( (_count != 1u)                                      ||
         (_values[0].length < 2u)                            ||
         (_values[0].data[_values[0].length - 1u] != 'm') )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    std::istringstream in(std::string(_values[0].data,
                                      _values[0].length - 1u));
    double             value = 0.0;

    in >> value;

    // the number, and nothing after it
    if ( (in.fail()) ||
         (!in.eof()) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    static_cast<meters*>(_target)->m_value = value;

    return D_ARG_OK;
}

/**
 * @brief Writes meters, `<number>m`; a d_arg_format_fn.
 *
 * @param[in] _type   the descriptor; unused.
 * @param[in] _value  the meters.
 * @param[in] _sink   where the text goes.
 * @return D_ARG_OK, or the status of a sink that refused the text.
 */
int
meters_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    (void)_type;

    const std::string text =
        streamed(static_cast<const meters*>(_value)->m_value) + "m";

    return d_arg_sink_write(_sink,
                            text.data(),
                            text.size());
}

}  // namespace

namespace djinterp
{
namespace parse
{
namespace cli
{

// type_traits<meters>
//   trait: the caller's own traits for meters: a descriptor whose converter
// reads `<number>m` and whose writer writes it back.
template<>
struct type_traits<meters>
{
    // the descriptor, made once; the meters themselves, as the address the
    // converter writes and the one the writer reads
    static const ::d_arg_type* descriptor();
    static void*               target(meters& _value) D_NOEXCEPT;
    static const void*         source(const meters& _value) D_NOEXCEPT;
};

// type_traits<bag>
//   trait: a container of the caller's own, converted as CLI11 converts a
// container, through container_traits.
template<>
struct type_traits<bag> : public container_traits<bag>
{};

}  // namespace cli
}  // namespace parse
}  // namespace djinterp

/*
type_traits<meters>::descriptor
  Made once, named LENGTH, a scalar taking one value.
*/
const ::d_arg_type*
djinterp::parse::cli::type_traits<meters>::descriptor()
{
    static const ::d_arg_type DESCRIPTOR = { "LENGTH",
                                             &meters_convert,
                                             &meters_format,
                                             D_NULLPTR,
                                             sizeof(meters),
                                             1,
                                             1,
                                             1,
                                             0u };

    return &DESCRIPTOR;
}

/*
type_traits<meters>::target / source
  The meters themselves.
*/
void*
djinterp::parse::cli::type_traits<meters>::target(
    meters& _value
) D_NOEXCEPT
{
    return &_value;
}

const void*
djinterp::parse::cli::type_traits<meters>::source(
    const meters& _value
) D_NOEXCEPT
{
    return &_value;
}

namespace
{

/**
 * @brief Meters, as the number and its letter.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    const meters& _value
)
{
    return streamed(_value.m_value) + "m";
}

/**
 * @brief A bag, its numbers in order.
 *
 * @param[in] _value  the value.
 * @return the text.
 */
std::string
shown(
    const bag& _value
)
{
    return shown_range(_value.begin(),
                       _value.end());
}

/**
 * @brief CALLER_TRAITS 1: a type the caller's traits convert, bound alone
 *        and in a vector, given a default, and refused; a container of the
 *        caller's own, which may be given no values, its default captured,
 *        given numbers and emptied.
 *
 * @param[in,out] _run  the step's run.
 */
void
caller_traits_used(
    run& _run
)
{
    static const char* const GIVEN[]   =
    {
        "--length", "2.5m",
        "--lengths", "1m", "3m",
        "--bag", "4", "5"
    };
    static const char* const EMPTIED[] = { "--bag", "{}" };
    static const char* const BAD[]     = { "--length", "2.5" };
    meters                   length    = { 0.0 };
    const meters             fallback  = { 9.0 };
    std::vector<meters>      lengths;
    bag                      held;

    held.m_items.push_back(1);
    held.m_items.push_back(2);

    cli::option measured = bind(_run,
                                "length",
                                "--length",
                                length);
    cli::option bagged   = bind(_run,
                                "bag",
                                "--bag",
                                held)->expected(0,
                                                3);

    (void)bind(_run,
               "lengths",
               "--lengths",
               lengths);
    measured->default_val(fallback);
    write_default(_run,
                  "length",
                  measured);
    write_value(_run,
                "length",
                length);
    bagged->capture_default_str();
    write_default(_run,
                  "bag",
                  bagged);
    _run.parse(GIVEN,
               8u);
    write_value(_run,
                "length",
                length);
    write_value(_run,
                "lengths",
                lengths);
    write_value(_run,
                "bag",
                held);
    _run.parse(EMPTIED,
               2u);
    write_value(_run,
                "bag",
                held);
    _run.parse(BAD,
               2u);
    write_value(_run,
                "length",
                length);

    return;
}

// CALLER_TRAITS_STEPS
//   constant: the steps of d_tests_arg_cpp_type_caller_traits.
const step_fn CALLER_TRAITS_STEPS[] =
{
    &caller_traits_used
};

// CALLER_TRAITS_COUNT
//   constant: how many steps CALLER_TRAITS_STEPS holds.
const std::size_t CALLER_TRAITS_COUNT =
    sizeof(CALLER_TRAITS_STEPS) / sizeof(CALLER_TRAITS_STEPS[0]);

// 2.3    Departures
//------------------------------------------------------------------------------

// cell
//   struct: an aggregate read and written with stream operators,
// `row,column`.
struct cell
{
    int m_row;
    int m_column;
};

/**
 * @brief Reads a cell, `row,column`.
 *
 * @param[in,out] _in    the stream.
 * @param[out]    _cell  receives the cell.
 * @return the stream, failed for anything else.
 */
std::istream&
operator>>(
    std::istream& _in,
    cell&         _cell
)
{
    char comma = '\0';

    _in >> _cell.m_row >> comma >> _cell.m_column;

    // anything but a comma between the numbers
    if (comma != ',')
    {
        _in.setstate(std::ios_base::failbit);
    }

    return _in;
}

/**
 * @brief Writes a cell, `row,column`.
 *
 * @param[in,out] _out   the stream.
 * @param[in]     _cell  the cell.
 * @return the stream.
 */
std::ostream&
operator<<(
    std::ostream& _out,
    const cell&   _cell
)
{
    return _out << _cell.m_row << ',' << _cell.m_column;
}

/**
 * @brief DEPARTED 1: defaults written as their types write values: real
 *        numbers as the shortest text that reads back as each, a truth
 *        value as a word, a character as itself, an 8-bit integer in
 *        decimal.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_written(
    run& _run
)
{
    unsigned char tiny      = 65u;
    bool          truth     = true;
    cli::option   defaulted = _run.app().add_option("--defaulted");
    cli::option   small     = _run.app().add_option("--tiny",
                                                    tiny);
    cli::option   flagged   = _run.app().add_option("--truth",
                                                    truth);

    defaulted->default_val(0.1);
    write_default(_run,
                  "double",
                  defaulted);
    defaulted->default_val(0.1f);
    write_default(_run,
                  "float",
                  defaulted);
    defaulted->default_val(true);
    write_default(_run,
                  "bool",
                  defaulted);
    defaulted->default_val('c');
    write_default(_run,
                  "char",
                  defaulted);
    small->capture_default_str();
    write_default(_run,
                  "captured unsigned char",
                  small);
    flagged->capture_default_str();
    write_default(_run,
                  "captured bool",
                  flagged);

    return;
}

/**
 * @brief DEPARTED 2: an integer with a leading zero, read as decimal under
 *        the parser's rule, and as octal by an option given C's.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_rule(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--decimal", "010",
        "--octal", "010"
    };
    unsigned long            decimal = 0u;
    unsigned long            octal   = 0u;

    (void)_run.app().add_option("--decimal",
                                decimal);
    (void)_run.app().add_option("--octal",
                                octal)
                    ->integer_rule(cli::integer_rule::c);
    _run.parse(GIVEN,
               4u);
    write_value(_run,
                "decimal",
                decimal);
    write_value(_run,
                "octal",
                octal);

    return;
}

/**
 * @brief DEPARTED 3: variables of composed types a value does not convert
 *        into, each holding after the parse what it held before.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_kept(
    run& _run
)
{
    static const char* const   INTS[]   = { "--ints", "1", "x", "3" };
    static const char* const   NUMBER[] = { "--complex", "1+zi" };
    static const char* const   KEYED[]  = { "--map", "b", "2", "c", "x" };
    std::vector<int>           ints(1u,
                                    9);
    std::complex<double>       number(5.0,
                                      6.0);
    std::map<std::string, int> map;

    map["a"] = 1;
    (void)_run.app().add_option("--ints",
                                ints);
    (void)_run.app().add_option("--complex",
                                number);
    (void)_run.app().add_option("--map",
                                map);
    _run.parse(INTS,
               4u);
    write_value(_run,
                "ints",
                ints);
    _run.parse(NUMBER,
               2u);
    write_value(_run,
                "complex",
                number);
    _run.parse(KEYED,
               5u);
    write_value(_run,
                "map",
                map);

    return;
}

/**
 * @brief DEPARTED 4: an aggregate with stream operators, read through them
 *        at every level.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_aggregate(
    run& _run
)
{
    static const char* const GIVEN[] = { "--cell", "1,2" };
    cell                     given   = { 0, 0 };

    (void)bind(_run,
               "cell",
               "--cell",
               given);
    _run.parse(GIVEN,
               2u);
    write_value(_run,
                "cell",
                given);

    return;
}

// DEPARTED_STEPS
//   constant: the steps of d_tests_arg_cpp_type_departures.
const step_fn DEPARTED_STEPS[] =
{
    &departed_written,
    &departed_rule,
    &departed_kept,
    &departed_aggregate
};

// DEPARTED_COUNT
//   constant: how many steps DEPARTED_STEPS holds.
const std::size_t DEPARTED_COUNT =
    sizeof(DEPARTED_STEPS) / sizeof(DEPARTED_STEPS[0]);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief DEPARTED_CPP11 1: defaults given as values of C++11 kinds:
 *        durations, written as their counts and so read back into their
 *        variables; a value of a type with no text form, refused, the
 *        default and the variable kept.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_unwritten(
    run& _run
)
{
    tag_t                         tag(std::string("old"));
    std::chrono::seconds          seconds(1);
    std::chrono::duration<double> span(1.0);
    cli::option                   counted = _run.app().add_option("--seconds",
                                                                  seconds);
    cli::option                   spanned = _run.app().add_option("--span",
                                                                  span);
    cli::option                   tagged  = _run.app().add_option("--tag",
                                                                  tag);

    counted->default_val(std::chrono::seconds(9));
    write_default(_run,
                  "seconds",
                  counted);
    write_value(_run,
                "seconds",
                seconds);
    spanned->default_val(std::chrono::duration<double>(2.5));
    write_default(_run,
                  "span",
                  spanned);
    write_value(_run,
                "span",
                span);
    tagged->default_val(tag_t("t"));
    write_default(_run,
                  "tag",
                  tagged);
    write_value(_run,
                "tag",
                tag);

    return;
}

// DEPARTED_CPP11_STEPS
//   constant: the steps of d_tests_arg_cpp_type_departures_cpp11.
const step_fn DEPARTED_CPP11_STEPS[] =
{
    &departed_unwritten
};

// DEPARTED_CPP11_COUNT
//   constant: how many steps DEPARTED_CPP11_STEPS holds.
const std::size_t DEPARTED_CPP11_COUNT =
    sizeof(DEPARTED_CPP11_STEPS) / sizeof(DEPARTED_CPP11_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/**
 * @brief DEPARTED_CPP17 1: an optional value's default -- given holding a
 *        value and none, and captured -- written as the value it holds.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_optional(
    run& _run
)
{
    std::optional<int> maybe;
    std::optional<int> held(3);
    cli::option        defaulted = _run.app().add_option("--maybe",
                                                         maybe);
    cli::option        captured  = _run.app().add_option("--held",
                                                         held);

    defaulted->default_val(std::optional<int>(7));
    write_default(_run,
                  "given",
                  defaulted);
    write_value(_run,
                "maybe",
                maybe);
    defaulted->default_val(std::optional<int>());
    write_default(_run,
                  "none",
                  defaulted);
    captured->capture_default_str();
    write_default(_run,
                  "captured",
                  captured);

    return;
}

// DEPARTED_CPP17_STEPS
//   constant: the steps of d_tests_arg_cpp_type_departures_cpp17.
const step_fn DEPARTED_CPP17_STEPS[] =
{
    &departed_optional
};

// DEPARTED_CPP17_COUNT
//   constant: how many steps DEPARTED_CPP17_STEPS holds.
const std::size_t DEPARTED_CPP17_COUNT =
    sizeof(DEPARTED_CPP17_STEPS) / sizeof(DEPARTED_CPP17_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/**
 * @brief DEPARTED_WIDE 1: wide text's default, captured and given, written
 *        as its UTF-8; and a value that is not UTF-8, refused as a value
 *        that does not convert.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_wide(
    run& _run
)
{
    static const char* const BAD[] = { "--wide", "\xff" };
    std::wstring             wide;

    wide += L'd';
    wide += static_cast<wchar_t>(0xe9);

    cli::option option = _run.app().add_option("--wide",
                                               wide);

    option->capture_default_str();
    write_default(_run,
                  "captured",
                  option);
    option->default_val(wide);
    write_default(_run,
                  "given",
                  option);
    write_value(_run,
                "wide",
                wide);
    _run.parse(BAD,
               2u);
    write_value(_run,
                "wide",
                wide);

    return;
}

// DEPARTED_WIDE_STEPS
//   constant: the steps of d_tests_arg_cpp_type_departures_wide.
const step_fn DEPARTED_WIDE_STEPS[] =
{
    &departed_wide
};

// DEPARTED_WIDE_COUNT
//   constant: how many steps DEPARTED_WIDE_STEPS holds.
const std::size_t DEPARTED_WIDE_COUNT =
    sizeof(DEPARTED_WIDE_STEPS) / sizeof(DEPARTED_WIDE_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

}  // namespace

//==============================================================================
// 3.  BUILT-IN TYPES
//==============================================================================

/*
d_tests_arg_cpp_type_numbers
  The built-in numbers, truth values and characters as option values, as
CLI11 converts them.
  Tests the following:
  - signed integers of each width at their limits and with a sign; unsigned
    ones at theirs and in hexadecimal and octal; reals of each width
  - truth values in each kind of their words; characters given as one and
    as a number
  - values refused: past a limit, not a number, no truth word, text after a
    real, a sign before an unsigned number
*/
bool
d_tests_arg_cpp_type_numbers()
{
    return steps_hold("NUMBERS",
                      NUMBERS_STEPS,
                      NUMBERS_COUNT,
                      NUMBERS);
}

#if D_ENV_HAS_LONG_LONG

/*
d_tests_arg_cpp_type_numbers_long_long
  The widest integers, where the compiler has them, as CLI11 converts them.
  Tests the following:
  - long long and unsigned long long at their limits, and one past
*/
bool
d_tests_arg_cpp_type_numbers_long_long()
{
    return steps_hold("NUMBERS_LONG_LONG",
                      NUMBERS_LONG_LONG_STEPS,
                      NUMBERS_LONG_LONG_COUNT,
                      NUMBERS_LONG_LONG);
}

#endif  // D_ENV_HAS_LONG_LONG

/*
d_tests_arg_cpp_type_texts
  Text and types made from it as option values, as CLI11 converts them.
  Tests the following:
  - std::string with a space and empty; a type made from text implicitly; a
    type read with streams, and a value it refuses
*/
bool
d_tests_arg_cpp_type_texts()
{
    return steps_hold("TEXTS",
                      TEXTS_STEPS,
                      TEXTS_COUNT,
                      TEXTS);
}

//==============================================================================
// 4.  COMPOSED TYPES
//==============================================================================

/*
d_tests_arg_cpp_type_containers
  The standard containers as option values, as CLI11 converts them.
  Tests the following:
  - a vector, a list and a double-ended queue
  - sets, of unique values and of repeated ones
  - maps, of unique keys and of repeated ones; pairs in a vector, and one
    cut short
  - "{}" emptying a container an option may give no values to, and an
    element given one that needs values: text, or a number refused
  - an element refused
*/
bool
d_tests_arg_cpp_type_containers()
{
    return steps_hold("CONTAINERS",
                      CONTAINERS_STEPS,
                      CONTAINERS_COUNT,
                      CONTAINERS);
}

/*
d_tests_arg_cpp_type_compound
  Pairs and complex numbers as option values, as CLI11 converts them.
  Tests the following:
  - a pair, and a first value refused
  - complex numbers in each form CLI11 reads, and one refused
*/
bool
d_tests_arg_cpp_type_compound()
{
    return steps_hold("COMPOUND",
                      COMPOUND_STEPS,
                      COMPOUND_COUNT,
                      COMPOUND);
}

/*
d_tests_arg_cpp_type_defaults
  Defaults given as values of the types, and values read back as types, as
CLI11 writes and reads them.
  Tests the following:
  - default_val of an integer, text, a pair, a complex number, a type made
    from text and a streamed type
  - default_val of containers: a vector, empty and not, a list, a set, a
    map, pairs in a vector
  - capture_default_str of a bound variable; default_val setting it
  - as<T>() of one value as several types, and of several as vectors
*/
bool
d_tests_arg_cpp_type_defaults()
{
    return steps_hold("DEFAULTS",
                      DEFAULTS_STEPS,
                      DEFAULTS_COUNT,
                      DEFAULTS);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_type_kinds_cpp11
  The kinds of type C++11 brings, as option values, as CLI11 converts them.
  Tests the following:
  - a tuple, an array, a vector of tuples, and a separator inside a tuple
  - enumerations of their own scope and with a negative value, and a name
    refused
  - unordered containers, a type made by an explicit constructor, and
    durations counted in whole seconds and in a real number of them
  - defaults given as a tuple, an array and enumerations, and captured
    from a type with no text form
*/
bool
d_tests_arg_cpp_type_kinds_cpp11()
{
    return steps_hold("KINDS_CPP11",
                      KINDS_CPP11_STEPS,
                      KINDS_CPP11_COUNT,
                      KINDS_CPP11);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_arg_cpp_type_kinds_cpp17
  The kinds of type C++17 brings, as option values, as CLI11 converts them.
  Tests the following:
  - optional values given and not; an optional vector; a string view; a
    path
  - a string view's and a path's defaults
*/
bool
d_tests_arg_cpp_type_kinds_cpp17()
{
    return steps_hold("KINDS_CPP17",
                      KINDS_CPP17_STEPS,
                      KINDS_CPP17_COUNT,
                      KINDS_CPP17);
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/*
d_tests_arg_cpp_type_wide_text
  Wide text as option values, as CLI11 converts it.
  Tests the following:
  - std::wstring given UTF-8 beyond ASCII and with a space; a vector of it
*/
bool
d_tests_arg_cpp_type_wide_text()
{
    return steps_hold("WIDE_TEXT",
                      WIDE_TEXT_STEPS,
                      WIDE_TEXT_COUNT,
                      WIDE_TEXT);
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// 5.  THE TRAITS THEMSELVES
//==============================================================================

/*
d_tests_arg_cpp_type_names
  What type_traits gives each kind of type: a name and a descriptor.
  Tests the following:
  - type_name of the built-in types, text, containers, pairs, maps, complex
    numbers, a type made from text and a streamed type
  - each descriptor's shape: container, expected count, sizes, reading and
    writing
  - a built-in type's descriptor, and size_t's and ptrdiff_t's, is the C
    library's own; a made one is made once
*/
bool
d_tests_arg_cpp_type_names()
{
    return steps_hold("TYPE_NAMES",
                      TYPE_NAMES_STEPS,
                      TYPE_NAMES_COUNT,
                      TYPE_NAMES);
}

/*
d_tests_arg_cpp_type_convert
  convert() and to_text() called directly.
  Tests the following:
  - values converted, empty, refused with the target kept; "{}" alone, an
    empty container; an integer under C's rule for a leading zero
  - values of several types written, appended to what the string held
*/
bool
d_tests_arg_cpp_type_convert()
{
    return steps_hold("CONVERTED",
                      CONVERTED_STEPS,
                      CONVERTED_COUNT,
                      CONVERTED);
}

/*
d_tests_arg_cpp_type_caller_traits
  Traits of the caller's own -- a type_traits specialization, and one
deriving from container_traits -- used wherever the type is.
  Tests the following:
  - an option bound to the type, and to a vector of it, the vector's
    elements converted by the caller's traits; a default written by the
    caller's writer; a value its converter refuses, the variable kept
  - a container of the caller's own, with only what container_traits
    needs: captured, given values, emptied by "{}" as an option that may be
    given none
*/
bool
d_tests_arg_cpp_type_caller_traits()
{
    return steps_hold("CALLER_TRAITS",
                      CALLER_TRAITS_STEPS,
                      CALLER_TRAITS_COUNT,
                      CALLER_TRAITS);
}

//==============================================================================
// 6.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_cpp_type_departures
  Where the face writes or reads a value otherwise than CLI11 (ISSUES 23,
34, 53 and 60, and the integer rule).
  Tests the following:
  - real numbers as defaults, as the shortest text that reads back as each;
    a truth value and a character as defaults, as a word and as itself; an
    8-bit integer captured in decimal
  - `010` as ten under the parser's rule, and eight under C's
  - a vector, a complex number and a map a value does not convert into,
    each keeping what it held
  - an aggregate with stream operators, read through them
*/
bool
d_tests_arg_cpp_type_departures()
{
    return steps_hold("DEPARTED",
                      DEPARTED_STEPS,
                      DEPARTED_COUNT,
                      DEPARTED);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_type_departures_cpp11
  Where the face writes a value of a C++11 kind otherwise than CLI11 (ISSUES
53 and 60).
  Tests the following:
  - durations as defaults, counted in whole seconds and in a real number of
    them, each written as its count and read back into its variable
  - a value of a type with no text form as a default, refused as the
    definition error, the default and the variable kept
*/
bool
d_tests_arg_cpp_type_departures_cpp11()
{
    return steps_hold("DEPARTED_CPP11",
                      DEPARTED_CPP11_STEPS,
                      DEPARTED_CPP11_COUNT,
                      DEPARTED_CPP11);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_arg_cpp_type_departures_cpp17
  Where the face writes a value of a C++17 kind otherwise than CLI11 (ISSUES
53).
  Tests the following:
  - an optional value's default, given and captured, as the value it holds;
    empty when it holds none
*/
bool
d_tests_arg_cpp_type_departures_cpp17()
{
    return steps_hold("DEPARTED_CPP17",
                      DEPARTED_CPP17_STEPS,
                      DEPARTED_CPP17_COUNT,
                      DEPARTED_CPP17);
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/*
d_tests_arg_cpp_type_departures_wide
  Where the face writes or reads wide text otherwise than CLI11 (ISSUES 53).
  Tests the following:
  - wide text's default, captured and given, as its UTF-8, the variable
    kept
  - a value that is not UTF-8, refused as a value that does not convert
*/
bool
d_tests_arg_cpp_type_departures_wide()
{
    return steps_hold("DEPARTED_WIDE",
                      DEPARTED_WIDE_STEPS,
                      DEPARTED_WIDE_COUNT,
                      DEPARTED_WIDE);
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// 7.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_type
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_type[] =
{
    { "cpp_type_numbers",
      d_tests_arg_cpp_type_numbers },
#if D_ENV_HAS_LONG_LONG
    { "cpp_type_numbers_long_long",
      d_tests_arg_cpp_type_numbers_long_long },
#endif
    { "cpp_type_texts",
      d_tests_arg_cpp_type_texts },
    { "cpp_type_containers",
      d_tests_arg_cpp_type_containers },
    { "cpp_type_compound",
      d_tests_arg_cpp_type_compound },
    { "cpp_type_defaults",
      d_tests_arg_cpp_type_defaults },
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_type_kinds_cpp11",
      d_tests_arg_cpp_type_kinds_cpp11 },
#endif
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    { "cpp_type_kinds_cpp17",
      d_tests_arg_cpp_type_kinds_cpp17 },
#endif
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    { "cpp_type_wide_text",
      d_tests_arg_cpp_type_wide_text },
#endif
    { "cpp_type_names",
      d_tests_arg_cpp_type_names },
    { "cpp_type_convert",
      d_tests_arg_cpp_type_convert },
    { "cpp_type_caller_traits",
      d_tests_arg_cpp_type_caller_traits },
    { "cpp_type_departures",
      d_tests_arg_cpp_type_departures },
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_type_departures_cpp11",
      d_tests_arg_cpp_type_departures_cpp11 },
#endif
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    { "cpp_type_departures_cpp17",
      d_tests_arg_cpp_type_departures_cpp17 },
#endif
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    { "cpp_type_departures_wide",
      d_tests_arg_cpp_type_departures_wide },
#endif
};

// d_tests_arg_cpp_type_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_type_count =
    sizeof(d_tests_arg_cpp_type) / sizeof(d_tests_arg_cpp_type[0]);
