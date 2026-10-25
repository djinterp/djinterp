/*******************************************************************************
* djinterp [parse]                                          arg_config_tests.cpp
*
* The argument parser's C++ configuration suite's sections.
*   The sections held to CLI11 run what the CASES section defines: text and
* files read into items by formats of each setting, a command's values
* written by them, a format given to a command for its parse, operations
* replaced by classes deriving from the format, and formats copied.
* tools/cppref/cppref.py compiles that section against CLI11 2.7.2, through
* the face's names for CLI11's (cli11_shim.hpp, whose format is a
* ConfigBase shared with the Apps it is given to, its operations the
* format's virtual functions), and the transcripts below are what CLI11
* wrote; so the CASES section is written in the part of the face CLI11
* shares -- each setting a statement of its own, as CLI11's return a
* pointer rather than the format, each format given to a command a static
* object, which a command here borrows, and given to it before its
* subcommands are made, which CLI11's copy when they are made. The
* sections that are the module's own are written apart, after it, each
* saying why CLI11 cannot write its transcript. Without configuration files
* built in, only the module's own are, with what a format and a
* configuration option come to then.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_config_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_config_tests.hpp"  // corresponding header
// std
#include <cstddef>   // std::size_t
#include <istream>   // std::istream
#include <sstream>   // std::istringstream
#include <string>    // std::string, std::getline
#include <vector>    // std::vector
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
using cli::testing::number;
using cli::testing::scratch_directory;
using cli::testing::steps_hold;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// READ
//   constant: what the steps of d_tests_arg_cpp_config_read write, as CLI11
// writes them.
const char READ[] =
    "made: 18 items\n"
    "made 0: parents=[] name=[name] inputs=[joe] multiline=0 full=[name]\n"
    "made 1: parents=[] name=[count] inputs=[3] multiline=0 full=[count]\n"
    "made 2: parents=[] name=[list] inputs=[1,2,3] multiline=0 full=[list]\n"
    "made 3: parents=[] name=[quoted] inputs=[single quoted] multiline=0 "
    "full=[quoted]\n"
    "made 4: parents=[] name=[flag] inputs=[true] multiline=0 full=[flag]\n"
    "made 5: parents=[] name=[multi] inputs=[first\\nsecond] multiline=0 "
    "full=[multi]\n"
    "made 6: parents=[section] name=[++] inputs=[] multiline=0 "
    "full=[section.++]\n"
    "made 7: parents=[section] name=[inner] inputs=[value] multiline=0 "
    "full=[section.inner]\n"
    "made 8: parents=[section,deeper] name=[++] inputs=[] multiline=0 "
    "full=[section.deeper.++]\n"
    "made 9: parents=[section,deeper] name=[deep] inputs=[x] multiline=0 "
    "full=[section.deeper.deep]\n"
    "made 10: parents=[section,deeper] name=[--] inputs=[] multiline=0 "
    "full=[section.deeper.--]\n"
    "made 11: parents=[section] name=[--] inputs=[] multiline=0 "
    "full=[section.--]\n"
    "made 12: parents=[array] name=[++] inputs=[] multiline=0 full=[array.++]\n"
    "made 13: parents=[array] name=[a] inputs=[1] multiline=0 full=[array.a]\n"
    "made 14: parents=[array] name=[--] inputs=[] multiline=0 full=[array.--]\n"
    "made 15: parents=[array] name=[++] inputs=[] multiline=0 full=[array.++]\n"
    "made 16: parents=[array] name=[a] inputs=[2] multiline=0 full=[array.a]\n"
    "made 17: parents=[array] name=[--] inputs=[] multiline=0 full=[array.--]\n"
    "toml: 18 items\n"
    "toml 0: parents=[] name=[name] inputs=[joe] multiline=0 full=[name]\n"
    "toml 1: parents=[] name=[count] inputs=[3] multiline=0 full=[count]\n"
    "toml 2: parents=[] name=[list] inputs=[1,2,3] multiline=0 full=[list]\n"
    "toml 3: parents=[] name=[quoted] inputs=[single quoted] multiline=0 "
    "full=[quoted]\n"
    "toml 4: parents=[] name=[flag] inputs=[true] multiline=0 full=[flag]\n"
    "toml 5: parents=[] name=[multi] inputs=[first\\nsecond] multiline=0 "
    "full=[multi]\n"
    "toml 6: parents=[section] name=[++] inputs=[] multiline=0 "
    "full=[section.++]\n"
    "toml 7: parents=[section] name=[inner] inputs=[value] multiline=0 "
    "full=[section.inner]\n"
    "toml 8: parents=[section,deeper] name=[++] inputs=[] multiline=0 "
    "full=[section.deeper.++]\n"
    "toml 9: parents=[section,deeper] name=[deep] inputs=[x] multiline=0 "
    "full=[section.deeper.deep]\n"
    "toml 10: parents=[section,deeper] name=[--] inputs=[] multiline=0 "
    "full=[section.deeper.--]\n"
    "toml 11: parents=[section] name=[--] inputs=[] multiline=0 "
    "full=[section.--]\n"
    "toml 12: parents=[array] name=[++] inputs=[] multiline=0 full=[array.++]\n"
    "toml 13: parents=[array] name=[a] inputs=[1] multiline=0 full=[array.a]\n"
    "toml 14: parents=[array] name=[--] inputs=[] multiline=0 full=[array.--]\n"
    "toml 15: parents=[array] name=[++] inputs=[] multiline=0 full=[array.++]\n"
    "toml 16: parents=[array] name=[a] inputs=[2] multiline=0 full=[array.a]\n"
    "toml 17: parents=[array] name=[--] inputs=[] multiline=0 full=[array.--]\n"
    "ini: 6 items\n"
    "ini 0: parents=[] name=[name] inputs=[joe] multiline=0 full=[name]\n"
    "ini 1: parents=[] name=[list] inputs=[1,2,3] multiline=0 full=[list]\n"
    "ini 2: parents=[section] name=[++] inputs=[] multiline=0 "
    "full=[section.++]\n"
    "ini 3: parents=[section] name=[inner] inputs=[quoted value] multiline=0 "
    "full=[section.inner]\n"
    "ini 4: parents=[section] name=[other] inputs=[a,b] multiline=0 "
    "full=[section.other]\n"
    "ini 5: parents=[section] name=[--] inputs=[] multiline=0 "
    "full=[section.--]\n"
    "ini of toml: 18 items\n"
    "ini of toml 0: parents=[] name=[name] inputs=[joe] multiline=0 "
    "full=[name]\n"
    "ini of toml 1: parents=[] name=[count] inputs=[3] multiline=0 "
    "full=[count]\n"
    "ini of toml 2: parents=[] name=[list] inputs=[1,2,3] multiline=0 "
    "full=[list]\n"
    "ini of toml 3: parents=[] name=[quoted] inputs=[single quoted] "
    "multiline=0 full=[quoted]\n"
    "ini of toml 4: parents=[] name=[flag] inputs=[true] multiline=0 "
    "full=[flag]\n"
    "ini of toml 5: parents=[] name=[multi] inputs=[first\\nsecond] "
    "multiline=0 full=[multi]\n"
    "ini of toml 6: parents=[section] name=[++] inputs=[] multiline=0 "
    "full=[section.++]\n"
    "ini of toml 7: parents=[section] name=[inner] "
    "inputs=[value,#,a,trailing,comment] multiline=0 full=[section.inner]\n"
    "ini of toml 8: parents=[section,deeper] name=[++] inputs=[] multiline=0 "
    "full=[section.deeper.++]\n"
    "ini of toml 9: parents=[section,deeper] name=[deep] inputs=[x] "
    "multiline=0 full=[section.deeper.deep]\n"
    "ini of toml 10: parents=[section,deeper] name=[--] inputs=[] multiline=0 "
    "full=[section.deeper.--]\n"
    "ini of toml 11: parents=[section] name=[--] inputs=[] multiline=0 "
    "full=[section.--]\n"
    "ini of toml 12: parents=[array] name=[++] inputs=[] multiline=0 "
    "full=[array.++]\n"
    "ini of toml 13: parents=[array] name=[a] inputs=[1] multiline=0 "
    "full=[array.a]\n"
    "ini of toml 14: parents=[array] name=[--] inputs=[] multiline=0 "
    "full=[array.--]\n"
    "ini of toml 15: parents=[array] name=[++] inputs=[] multiline=0 "
    "full=[array.++]\n"
    "ini of toml 16: parents=[array] name=[a] inputs=[2] multiline=0 "
    "full=[array.a]\n"
    "ini of toml 17: parents=[array] name=[--] inputs=[] multiline=0 "
    "full=[array.--]\n"
    "made: 3 items\n"
    "made 0: parents=[] name=[a] inputs=[1,%,after,a,percent] multiline=0 "
    "full=[a]\n"
    "made 1: parents=[] name=[% a percent line] inputs=[true] multiline=0 "
    "full=[% a percent line]\n"
    "made 2: parents=[] name=[b] inputs=[2] multiline=0 full=[b]\n"
    "percent: 2 items\n"
    "percent 0: parents=[] name=[a] inputs=[1] multiline=0 full=[a]\n"
    "percent 1: parents=[] name=[b] inputs=[2,#,after,a,hash] multiline=0 "
    "full=[b]\n"
    "made: 3 items\n"
    "made 0: parents=[] name=[a] inputs=[<1, 2>] multiline=0 full=[a]\n"
    "made 1: parents=[] name=[b] inputs=[3,4] multiline=0 full=[b]\n"
    "made 2: parents=[] name=[c] inputs=[5; 6] multiline=0 full=[c]\n"
    "angled: 3 items\n"
    "angled 0: parents=[] name=[a] inputs=[1,2] multiline=0 full=[a]\n"
    "angled 1: parents=[] name=[b] inputs=[[3, 4]] multiline=0 full=[b]\n"
    "angled 2: parents=[] name=[c] inputs=[[5; 6]] multiline=0 full=[c]\n"
    "semicolons: 3 items\n"
    "semicolons 0: parents=[] name=[a] inputs=[<1, 2>] multiline=0 full=[a]\n"
    "semicolons 1: parents=[] name=[b] inputs=[3, 4] multiline=0 full=[b]\n"
    "semicolons 2: parents=[] name=[c] inputs=[5,6] multiline=0 full=[c]\n"
    "made: 3 items\n"
    "made 0: parents=[] name=[a: 1] inputs=[true] multiline=0 full=[a: 1]\n"
    "made 1: parents=[] name=[b : two] inputs=[true] multiline=0 full=[b : "
    "two]\n"
    "made 2: parents=[] name=[c] inputs=[3] multiline=0 full=[c]\n"
    "colons: 3 items\n"
    "colons 0: parents=[] name=[a] inputs=[1] multiline=0 full=[a]\n"
    "colons 1: parents=[] name=[b] inputs=[two] multiline=0 full=[b]\n"
    "colons 2: parents=[] name=[c = 3] inputs=[true] multiline=0 full=[c = 3]\n"
    "made: 2 items\n"
    "made 0: parents=[] name=[a] inputs=[x\\\\ty] multiline=0 full=[a]\n"
    "made 1: parents=[] name=[b] inputs=[z\\tw] multiline=0 full=[b]\n"
    "swapped: 2 items\n"
    "swapped 0: parents=[] name=[a] inputs=[x\\ty] multiline=0 full=[a]\n"
    "swapped 1: parents=[] name=[b] inputs=[z\\\\tw] multiline=0 full=[b]\n"
    "made: 11 items\n"
    "made 0: parents=[a] name=[++] inputs=[] multiline=0 full=[a.++]\n"
    "made 1: parents=[a,b] name=[++] inputs=[] multiline=0 full=[a.b.++]\n"
    "made 2: parents=[a,b,c] name=[++] inputs=[] multiline=0 full=[a.b.c.++]\n"
    "made 3: parents=[a,b,c] name=[x] inputs=[1] multiline=0 full=[a.b.c.x]\n"
    "made 4: parents=[a,b,c] name=[--] inputs=[] multiline=0 full=[a.b.c.--]\n"
    "made 5: parents=[a,b] name=[--] inputs=[] multiline=0 full=[a.b.--]\n"
    "made 6: parents=[a] name=[--] inputs=[] multiline=0 full=[a.--]\n"
    "made 7: parents=[a] name=[++] inputs=[] multiline=0 full=[a.++]\n"
    "made 8: parents=[a] name=[y] inputs=[2] multiline=0 full=[a.y]\n"
    "made 9: parents=[a,d,e] name=[f] inputs=[3] multiline=0 full=[a.d.e.f]\n"
    "made 10: parents=[a] name=[--] inputs=[] multiline=0 full=[a.--]\n"
    "one: 9 items\n"
    "one 0: parents=[a] name=[++] inputs=[] multiline=0 full=[a.++]\n"
    "one 1: parents=[a,b] name=[++] inputs=[] multiline=0 full=[a.b.++]\n"
    "one 2: parents=[a,b,c] name=[++] inputs=[] multiline=0 full=[a.b.c.++]\n"
    "one 3: parents=[a,b,c] name=[--] inputs=[] multiline=0 full=[a.b.c.--]\n"
    "one 4: parents=[a,b] name=[--] inputs=[] multiline=0 full=[a.b.--]\n"
    "one 5: parents=[a] name=[--] inputs=[] multiline=0 full=[a.--]\n"
    "one 6: parents=[a] name=[++] inputs=[] multiline=0 full=[a.++]\n"
    "one 7: parents=[a] name=[y] inputs=[2] multiline=0 full=[a.y]\n"
    "one 8: parents=[a] name=[--] inputs=[] multiline=0 full=[a.--]\n"
    "none: 8 items\n"
    "none 0: parents=[a] name=[++] inputs=[] multiline=0 full=[a.++]\n"
    "none 1: parents=[a,b] name=[++] inputs=[] multiline=0 full=[a.b.++]\n"
    "none 2: parents=[a,b,c] name=[++] inputs=[] multiline=0 full=[a.b.c.++]\n"
    "none 3: parents=[a,b,c] name=[--] inputs=[] multiline=0 full=[a.b.c.--]\n"
    "none 4: parents=[a,b] name=[--] inputs=[] multiline=0 full=[a.b.--]\n"
    "none 5: parents=[a] name=[--] inputs=[] multiline=0 full=[a.--]\n"
    "none 6: parents=[a] name=[++] inputs=[] multiline=0 full=[a.++]\n"
    "none 7: parents=[a] name=[--] inputs=[] multiline=0 full=[a.--]\n"
    "made: 5 items\n"
    "made 0: parents=[a/b] name=[++] inputs=[] multiline=0 full=[a/b.++]\n"
    "made 1: parents=[a/b] name=[x] inputs=[1] multiline=0 full=[a/b.x]\n"
    "made 2: parents=[a/b] name=[c/d] inputs=[2] multiline=0 full=[a/b.c/d]\n"
    "made 3: parents=[a/b,e] name=[f] inputs=[3] multiline=0 full=[a/b.e.f]\n"
    "made 4: parents=[a/b] name=[--] inputs=[] multiline=0 full=[a/b.--]\n"
    "slashes: 7 items\n"
    "slashes 0: parents=[a] name=[++] inputs=[] multiline=0 full=[a.++]\n"
    "slashes 1: parents=[a,b] name=[++] inputs=[] multiline=0 full=[a.b.++]\n"
    "slashes 2: parents=[a,b] name=[x] inputs=[1] multiline=0 full=[a.b.x]\n"
    "slashes 3: parents=[a,b,c] name=[d] inputs=[2] multiline=0 "
    "full=[a.b.c.d]\n"
    "slashes 4: parents=[a,b] name=[e.f] inputs=[3] multiline=0 "
    "full=[a.b.e.f]\n"
    "slashes 5: parents=[a,b] name=[--] inputs=[] multiline=0 full=[a.b.--]\n"
    "slashes 6: parents=[a] name=[--] inputs=[] multiline=0 full=[a.--]\n"
    "made: 3 items\n"
    "made 0: parents=[] name=[x] inputs=[1] multiline=0 full=[x]\n"
    "made 1: parents=[] name=[y] inputs=[2] multiline=0 full=[y]\n"
    "made 2: parents=[] name=[x] inputs=[3,4] multiline=0 full=[x]\n"
    "duplicates: 2 items\n"
    "duplicates 0: parents=[] name=[x] inputs=[1,%%,3,%%,4] multiline=1 "
    "full=[x]\n"
    "duplicates 1: parents=[] name=[y] inputs=[2] multiline=0 full=[y]\n"
    "made: section=[] index=-1\n"
    "set: section=[sec] index=-1\n"
    "sec: 8 items\n"
    "sec 0: parents=[sec] name=[++] inputs=[] multiline=0 full=[sec.++]\n"
    "sec 1: parents=[] name=[x] inputs=[2] multiline=0 full=[x]\n"
    "sec 2: parents=[sec,sub] name=[++] inputs=[] multiline=0 "
    "full=[sec.sub.++]\n"
    "sec 3: parents=[sub] name=[y] inputs=[3] multiline=0 full=[sub.y]\n"
    "sec 4: parents=[sec,sub] name=[--] inputs=[] multiline=0 "
    "full=[sec.sub.--]\n"
    "sec 5: parents=[sec] name=[--] inputs=[] multiline=0 full=[sec.--]\n"
    "sec 6: parents=[other] name=[++] inputs=[] multiline=0 full=[other.++]\n"
    "sec 7: parents=[other] name=[--] inputs=[] multiline=0 full=[other.--]\n"
    "every: 10 items\n"
    "every 0: parents=[] name=[top] inputs=[1] multiline=0 full=[top]\n"
    "every 1: parents=[sec] name=[++] inputs=[] multiline=0 full=[sec.++]\n"
    "every 2: parents=[sec] name=[x] inputs=[2] multiline=0 full=[sec.x]\n"
    "every 3: parents=[sec,sub] name=[++] inputs=[] multiline=0 "
    "full=[sec.sub.++]\n"
    "every 4: parents=[sec,sub] name=[y] inputs=[3] multiline=0 "
    "full=[sec.sub.y]\n"
    "every 5: parents=[sec,sub] name=[--] inputs=[] multiline=0 "
    "full=[sec.sub.--]\n"
    "every 6: parents=[sec] name=[--] inputs=[] multiline=0 full=[sec.--]\n"
    "every 7: parents=[other] name=[++] inputs=[] multiline=0 full=[other.++]\n"
    "every 8: parents=[other] name=[z] inputs=[4] multiline=0 full=[other.z]\n"
    "every 9: parents=[other] name=[--] inputs=[] multiline=0 full=[other.--]\n"
    "first: 7 items\n"
    "first 0: parents=[arr] name=[++] inputs=[] multiline=0 full=[arr.++]\n"
    "first 1: parents=[] name=[x] inputs=[1] multiline=0 full=[x]\n"
    "first 2: parents=[arr] name=[--] inputs=[] multiline=0 full=[arr.--]\n"
    "first 3: parents=[arr] name=[++] inputs=[] multiline=0 full=[arr.++]\n"
    "first 4: parents=[arr] name=[--] inputs=[] multiline=0 full=[arr.--]\n"
    "first 5: parents=[arr] name=[++] inputs=[] multiline=0 full=[arr.++]\n"
    "first 6: parents=[arr] name=[--] inputs=[] multiline=0 full=[arr.--]\n"
    "set: section=[arr] index=1\n"
    "second: 7 items\n"
    "second 0: parents=[arr] name=[++] inputs=[] multiline=0 full=[arr.++]\n"
    "second 1: parents=[arr] name=[--] inputs=[] multiline=0 full=[arr.--]\n"
    "second 2: parents=[arr] name=[++] inputs=[] multiline=0 full=[arr.++]\n"
    "second 3: parents=[] name=[x] inputs=[2] multiline=0 full=[x]\n"
    "second 4: parents=[arr] name=[--] inputs=[] multiline=0 full=[arr.--]\n"
    "second 5: parents=[arr] name=[++] inputs=[] multiline=0 full=[arr.++]\n"
    "second 6: parents=[arr] name=[--] inputs=[] multiline=0 full=[arr.--]\n"
    "none: 6 items\n"
    "none 0: parents=[arr] name=[++] inputs=[] multiline=0 full=[arr.++]\n"
    "none 1: parents=[arr] name=[--] inputs=[] multiline=0 full=[arr.--]\n"
    "none 2: parents=[arr] name=[++] inputs=[] multiline=0 full=[arr.++]\n"
    "none 3: parents=[arr] name=[--] inputs=[] multiline=0 full=[arr.--]\n"
    "none 4: parents=[arr] name=[++] inputs=[] multiline=0 full=[arr.++]\n"
    "none 5: parents=[arr] name=[--] inputs=[] multiline=0 full=[arr.--]\n"
    "toml file: 4 items\n"
    "toml file 0: parents=[] name=[a] inputs=[1] multiline=0 full=[a]\n"
    "toml file 1: parents=[s] name=[++] inputs=[] multiline=0 full=[s.++]\n"
    "toml file 2: parents=[s] name=[b] inputs=[2,3] multiline=0 full=[s.b]\n"
    "toml file 3: parents=[s] name=[--] inputs=[] multiline=0 full=[s.--]\n"
    "ini file: 4 items\n"
    "ini file 0: parents=[] name=[a] inputs=[1] multiline=0 full=[a]\n"
    "ini file 1: parents=[s] name=[++] inputs=[] multiline=0 full=[s.++]\n"
    "ini file 2: parents=[s] name=[b] inputs=[2,3] multiline=0 full=[s.b]\n"
    "ini file 3: parents=[s] name=[--] inputs=[] multiline=0 full=[s.--]\n";

// WRITTEN
//   constant: what the steps of d_tests_arg_cpp_config_written write, as
// CLI11 writes them.
const char WRITTEN[] =
    "active [name=\"joe\"\\nnames=['a', 'b']\\nverbose=true\\ntext=\"two "
    "words\"\\nsub.deep=9\\n]\n"
    "defaults [## d\\n# a count\\ncount=7\\n\\n# a name\\nname=\"joe\"\\n\\n# "
    "some names\\nnames=['a', 'b']\\n\\n# say more\\nverbose=true\\n\\n# a "
    "text\\ntext=\"two words\"\\n## a subcommand\\n# a deep "
    "value\\nsub.deep=9\\n\\n# a depth\\nsub.depth=2\\n]\n"
    "subcommand defaults [count=7\\nname=\"joe\"\\nnames=['a', "
    "'b']\\nverbose=true\\ntext=\"two words\"\\nsub.deep=9\\nsub.depth=2\\n]\n"
    "given [name=\"joe\"\\nnames=['a', 'b']\\nverbose=true\\ntext=\"two "
    "words\"\\nsub.deep=9\\n]\n"
    "active [name=\"joe\"\\nnames='a' 'b'\\nverbose=true\\ntext=\"two "
    "words\"\\nsub.deep=9\\n]\n"
    "defaults [;; d\\n; a count\\ncount=7\\n\\n; a name\\nname=\"joe\"\\n\\n; "
    "some names\\nnames='a' 'b'\\n\\n; say more\\nverbose=true\\n\\n; a "
    "text\\ntext=\"two words\"\\n;; a subcommand\\n; a deep "
    "value\\nsub.deep=9\\n\\n; a depth\\nsub.depth=2\\n]\n"
    "subcommand defaults [count=7\\nname=\"joe\"\\nnames='a' "
    "'b'\\nverbose=true\\ntext=\"two words\"\\nsub.deep=9\\nsub.depth=2\\n]\n"
    "described [;; d\\n; a name\\nname=\"joe\"\\n\\n; some names\\nnames=['a', "
    "'b']\\n\\n; say more\\nverbose=true\\n\\n; a text\\ntext=\"two "
    "words\"\\n;; a subcommand\\n; a deep value\\nsub.deep=9\\n]\n"
    "parentheses [name=\"joe\"\\nnames=('a', 'b')\\nverbose=true\\ntext=\"two "
    "words\"\\nsub.deep=9\\n]\n"
    "semicolons [name=\"joe\"\\nnames=('a'; 'b')\\nverbose=true\\ntext=\"two "
    "words\"\\nsub.deep=9\\n]\n"
    "colons [name:\"joe\"\\nnames:['a', 'b']\\nverbose:true\\ntext:\"two "
    "words\"\\nsub.deep:9\\n]\n"
    "swapped [name:'joe'\\nnames:[\"a\", \"b\"]\\nverbose:true\\ntext:'two "
    "words'\\nsub.deep:9\\n]\n"
    "slashes [name=\"joe\"\\nnames=['a', 'b']\\nverbose=true\\ntext=\"two "
    "words\"\\nsub/deep=9\\n]\n"
    "defaults [count=7\\nname=\"joe\"\\nnames=['a', "
    "'b']\\nverbose=true\\ntext=\"two words\"\\nsub/deep=9\\nsub/depth=2\\n]\n"
    "commented [#count=7\\nname=\"joe\"\\nnames=['a', "
    "'b']\\nverbose=true\\ntext=\"two words\"\\nsub/deep=9\\n#sub/depth=2\\n]\n"
    "configurable [name=\"joe\"\\nnames=['a', 'b']\\nverbose=true\\ntext=\"two "
    "words\"\\n[sub]\\ndeep=9\\n]\n";

// PARSED
//   constant: what the steps of d_tests_arg_cpp_config_parsed write, as
// CLI11 writes them.
const char PARSED[] =
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[conf.ini]\n"
    "opt count n=1 res=[5]\n"
    "opt names n=2 res=[a|b]\n"
    "opt verbose n=1 res=[true]\n"
    "opt deep n=1 res=[3]\n"
    "opt loud n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[default.toml]\n"
    "opt count n=1 res=[6]\n"
    "opt names n=2 res=[c|d]\n"
    "opt verbose n=0 res=[]\n"
    "opt deep n=0 res=[]\n"
    "opt loud n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[sections.toml]\n"
    "opt count n=1 res=[7]\n"
    "opt names n=0 res=[]\n"
    "opt verbose n=0 res=[]\n"
    "opt deep n=0 res=[]\n"
    "opt loud n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "opt count n=1 res=[8]\n"
    "opt names n=2 res=[e|f]\n"
    "opt verbose n=1 res=[true]\n"
    "opt deep n=1 res=[2]\n"
    "opt loud n=1 res=[true]\n";

// REPLACED
//   constant: what the steps of d_tests_arg_cpp_config_replaced write, as
// CLI11 writes them through subclasses of its ConfigBase replacing the
// same operations.
const char REPLACED[] =
    "framed [# begin prog []\\nname=\"joe\"\\nnames=['a', "
    "'b']\\nverbose=true\\ntext=\"two words\"\\n# begin sub "
    "[sub.]\\nsub.deep=9\\n# end sub [sub.]\\n# end prog []\\n]\n"
    "defaults [# begin prog []\\ncount=7\\nname=\"joe\"\\nnames=['a', "
    "'b']\\nverbose=true\\ntext=\"two words\"\\n# begin sub "
    "[sub.]\\nsub.deep=9\\nsub.depth=2\\n# end sub [sub.]\\n# end prog []\\n]\n"
    "spaced: 4 items\n"
    "spaced 0: parents=[] name=[count] inputs=[4] multiline=0 full=[count]\n"
    "spaced 1: parents=[] name=[names] inputs=[g,h,i] multiline=0 "
    "full=[names]\n"
    "spaced 2: parents=[] name=[verbose] inputs=[] multiline=0 full=[verbose]\n"
    "spaced 3: parents=[sub] name=[deep] inputs=[9] multiline=0 "
    "full=[sub.deep]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "opt count n=1 res=[4]\n"
    "opt names n=3 res=[g|h|i]\n"
    "opt verbose n=1 res=[true]\n"
    "opt deep n=1 res=[9]\n"
    "opt loud n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "opt count n=0 res=[]\n"
    "opt names n=0 res=[]\n"
    "opt verbose n=1 res=[true]\n"
    "opt deep n=0 res=[]\n"
    "opt loud n=1 res=[false]\n";

// COPIES
//   constant: what the steps of d_tests_arg_cpp_config_copies write, as
// CLI11 writes them through copies of its ConfigBase.
const char COPIES[] =
    "original: 6 items\n"
    "original 0: parents=[s] name=[++] inputs=[] multiline=0 full=[s.++]\n"
    "original 1: parents=[] name=[a] inputs=[1] multiline=0 full=[a]\n"
    "original 2: parents=[s] name=[b = 2] inputs=[true] multiline=0 full=[s.b "
    "= 2]\n"
    "original 3: parents=[s] name=[--] inputs=[] multiline=0 full=[s.--]\n"
    "original 4: parents=[t] name=[++] inputs=[] multiline=0 full=[t.++]\n"
    "original 5: parents=[t] name=[--] inputs=[] multiline=0 full=[t.--]\n"
    "copied: 6 items\n"
    "copied 0: parents=[s] name=[++] inputs=[] multiline=0 full=[s.++]\n"
    "copied 1: parents=[] name=[a: 1] inputs=[true] multiline=0 full=[a: 1]\n"
    "copied 2: parents=[s] name=[b] inputs=[2] multiline=0 full=[s.b]\n"
    "copied 3: parents=[s] name=[--] inputs=[] multiline=0 full=[s.--]\n"
    "copied 4: parents=[t] name=[++] inputs=[] multiline=0 full=[t.++]\n"
    "copied 5: parents=[t] name=[--] inputs=[] multiline=0 full=[t.--]\n"
    "assigned: 5 items\n"
    "assigned 0: parents=[s] name=[++] inputs=[] multiline=0 full=[s.++]\n"
    "assigned 1: parents=[s] name=[--] inputs=[] multiline=0 full=[s.--]\n"
    "assigned 2: parents=[t] name=[++] inputs=[] multiline=0 full=[t.++]\n"
    "assigned 3: parents=[] name=[c] inputs=[3] multiline=0 full=[c]\n"
    "assigned 4: parents=[t] name=[--] inputs=[] multiline=0 full=[t.--]\n"
    "sections: original=[s] copied=[s] assigned=[t]\n"
    "sliced [name=\"joe\"\\nnames=['a', 'b']\\nverbose=true\\ntext=\"two "
    "words\"\\nsub.deep=9\\n]\n"
    "from ini [name=\"joe\"\\nnames='a' 'b'\\nverbose=true\\ntext=\"two "
    "words\"\\nsub.deep=9\\n]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// SETTINGS
//   constant: what the steps of d_tests_arg_cpp_config_settings write: the
// module's own, as CLI11's ConfigBase keeps its characters, layers and
// switches to itself.
const char SETTINGS[] =
    "toml: comment=[#] array=[[]] separator=[,] delimiter=[=] quotes=[\"'] "
    "parent=[.] layers=255 comment_defaults=0 duplicates=0 section=[] "
    "index=-1\n"
    "ini: comment=[;] array=[\\x00\\x00] separator=[ ] delimiter=[=] "
    "quotes=[\"'] parent=[.] layers=255 comment_defaults=0 duplicates=0 "
    "section=[] index=-1\n"
    "changed: comment=[;] array=[()] separator=[|] delimiter=[:] quotes=[`\"] "
    "parent=[/] layers=3 comment_defaults=1 duplicates=1 section=[part] "
    "index=2\n"
    "changed back: comment=[\\x00] array=[\\x00\\x00] separator=[|] "
    "delimiter=[:] quotes=[`\"] parent=[/] layers=3 comment_defaults=0 "
    "duplicates=0 section=[] index=-1\n"
    "ini copied: comment=[;] array=[\\x00\\x00] separator=[ ] delimiter=[=] "
    "quotes=[\"'] parent=[.] layers=255 comment_defaults=0 duplicates=0 "
    "section=[] index=-1\n";

// ITEMS
//   constant: what the steps of d_tests_arg_cpp_config_items write: the
// module's own, as CLI11's ConfigItem is a struct of strings, filled in
// place, where an item here is built through calls that copy its text --
// NUL bytes kept -- and is adopted by the list that keeps it (ISSUES 16).
const char ITEMS[] =
    "none: valid=0 parents=[] name=[] inputs=[] multiline=0 full=[]\n"
    "made: valid=1 parents=[] name=[] inputs=[] multiline=0 full=[]\n"
    "built: OK\n"
    "item: valid=1 parents=[a,b] name=[name] inputs=[1,x\\x00y] multiline=1 "
    "full=[a.b.name]\n"
    "adopted: OK\n"
    "item after: valid=1 parents=[] name=[] inputs=[] multiline=0 full=[]\n"
    "second: OK OK\n"
    "list: size=2 empty=0 view size=2 view empty=0\n"
    "list 0: valid=1 parents=[a,b] name=[name] inputs=[1,x\\x00y] multiline=1 "
    "full=[a.b.name]\n"
    "view 1: valid=1 parents=[] name=[n\\x00m] inputs=[] multiline=0 "
    "full=[n\\x00m]\n"
    "cleared: size=0 empty=1\n";

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FAILED
//   constant: what the steps of d_tests_arg_cpp_config_failed write: the
// module's own, as a read that fails here returns its status and its
// description, where CLI11's throws, and an operation of the caller's
// fails by returning a status.
const char FAILED[] =
    "escape: ParseError [unrecognized escape sequence \\\\q in bad \\\\q]\n"
    "escape: 1 items\n"
    "escape 0: parents=[] name=[a] inputs=[1] multiline=0 full=[a]\n"
    "missing: FileError [missing.toml was not readable (missing?)]\n"
    "ERR ConfigError 110: refused by the format\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[conf.toml]\n"
    "opt count n=0 res=[]\n"
    "opt names n=0 res=[]\n"
    "opt verbose n=0 res=[]\n"
    "opt deep n=0 res=[]\n"
    "opt loud n=0 res=[]\n"
    "ERR ConfigError 110: refused by the format\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[conf.toml]\n"
    "opt count n=0 res=[]\n"
    "opt names n=0 res=[]\n"
    "opt verbose n=0 res=[]\n"
    "opt deep n=0 res=[]\n"
    "opt loud n=0 res=[]\n"
    "read: ConfigError [refused by the format] items=0\n"
    "written []\n";

// DEPARTED
//   constant: what the steps of d_tests_arg_cpp_config_departures write:
// the module's own, where the face departs from CLI11. CLI11's maxLayers
// takes a std::uint8_t and its index a std::int16_t, so that a count of 257
// layers is 1 there -- the item three sections deep left out -- -1 is 255
// -- the item kept -- and an index of 65537 is 1, the second occurrence
// read, where here layers are held at 0 to 255 and an index is kept as
// given, which no occurrence matches (ISSUES 61); and CLI11 copies a
// command's format into a subcommand when the subcommand is made, so that
// one made before the root is given a format keeps the one it was made
// with, and reads `oui` as no flag's value -- `ConversionError 104: Could
// not convert: --loud = oui` -- where here a subcommand without a format
// of its own follows its parent's whenever it is used (ISSUES 18).
const char DEPARTED[] =
    "257 layers: comment=[#] array=[[]] separator=[,] delimiter=[=] "
    "quotes=[\"'] parent=[.] layers=255 comment_defaults=0 duplicates=0 "
    "section=[] index=-1\n"
    "257 layers: 7 items\n"
    "257 layers 0: parents=[a] name=[++] inputs=[] multiline=0 full=[a.++]\n"
    "257 layers 1: parents=[a,b] name=[++] inputs=[] multiline=0 "
    "full=[a.b.++]\n"
    "257 layers 2: parents=[a,b,c] name=[++] inputs=[] multiline=0 "
    "full=[a.b.c.++]\n"
    "257 layers 3: parents=[a,b,c] name=[x] inputs=[1] multiline=0 "
    "full=[a.b.c.x]\n"
    "257 layers 4: parents=[a,b,c] name=[--] inputs=[] multiline=0 "
    "full=[a.b.c.--]\n"
    "257 layers 5: parents=[a,b] name=[--] inputs=[] multiline=0 "
    "full=[a.b.--]\n"
    "257 layers 6: parents=[a] name=[--] inputs=[] multiline=0 full=[a.--]\n"
    "-1 layers: comment=[#] array=[[]] separator=[,] delimiter=[=] "
    "quotes=[\"'] parent=[.] layers=0 comment_defaults=0 duplicates=0 "
    "section=[] index=-1\n"
    "-1 layers: 6 items\n"
    "-1 layers 0: parents=[a] name=[++] inputs=[] multiline=0 full=[a.++]\n"
    "-1 layers 1: parents=[a,b] name=[++] inputs=[] multiline=0 full=[a.b.++]\n"
    "-1 layers 2: parents=[a,b,c] name=[++] inputs=[] multiline=0 "
    "full=[a.b.c.++]\n"
    "-1 layers 3: parents=[a,b,c] name=[--] inputs=[] multiline=0 "
    "full=[a.b.c.--]\n"
    "-1 layers 4: parents=[a,b] name=[--] inputs=[] multiline=0 full=[a.b.--]\n"
    "-1 layers 5: parents=[a] name=[--] inputs=[] multiline=0 full=[a.--]\n"
    "index 65537: comment=[#] array=[[]] separator=[,] delimiter=[=] "
    "quotes=[\"'] parent=[.] layers=255 comment_defaults=0 duplicates=0 "
    "section=[arr] index=65537\n"
    "index 65537: 4 items\n"
    "index 65537 0: parents=[arr] name=[++] inputs=[] multiline=0 "
    "full=[arr.++]\n"
    "index 65537 1: parents=[arr] name=[--] inputs=[] multiline=0 "
    "full=[arr.--]\n"
    "index 65537 2: parents=[arr] name=[++] inputs=[] multiline=0 "
    "full=[arr.++]\n"
    "index 65537 3: parents=[arr] name=[--] inputs=[] multiline=0 "
    "full=[arr.--]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "opt count n=0 res=[]\n"
    "opt names n=0 res=[]\n"
    "opt verbose n=0 res=[]\n"
    "opt deep n=0 res=[]\n"
    "opt loud n=1 res=[true]\n";

#else

// UNBUILT
//   constant: what the steps of d_tests_arg_cpp_config_unbuilt write: the
// module's own, as CLI11 has no build without configuration files.
const char UNBUILT[] =
    "from_config: Unsupported items=0 to_config: Unsupported []\n"
    "none: OK [{}]\n"
    "one: OK [yes]\n"
    "ERR ConfigError 110: there.cfg: configuration files are not built in\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[there.cfg]\n"
    "opt count n=0 res=[]\n"
    "ERR FileError 103: gone.cfg was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[gone.cfg]\n"
    "opt count n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "opt count n=0 res=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 1.  CASES
//==============================================================================
// What the sections held to CLI11 define, in the part of the face CLI11
// shares; tools/cppref/cppref.py compiles this section against CLI11.

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 1.1    Writers
//------------------------------------------------------------------------------

/**
 * @brief A list's texts, escaped and joined by commas.
 *
 * @param[in] _list  the list: a text_list, or anything with size() and
 *                   operator[] giving a text.
 * @return the texts, joined.
 */
template<typename List>
std::string
joined(
    const List& _list
)
{
    std::string written;

    for (std::size_t index = 0u; index < _list.size(); ++index)
    {
        written += (index != 0u) ? "," : "";
        written += escape(std::string(_list[index]));
    }

    return written;
}

/**
 * @brief Writes a list of items: how many, then a line for each -- its
 *        sections, its name, its values, whether it ran over lines, and
 *        its full name.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the lines name the list by.
 * @param[in]     _items  the items.
 */
void
write_items(
    run&                         _run,
    const char*                  _label,
    const cli::config_item_list& _items
)
{
    _run.note(std::string(_label) + ": " +
              number(static_cast<unsigned long>(_items.size())) + " items");

    for (std::size_t index = 0u; index < _items.size(); ++index)
    {
        const cli::config_entry entry = _items[index];

        _run.note(std::string(_label) + " " +
                  number(static_cast<unsigned long>(index)) +
                  ": parents=[" + joined(entry.parents()) +
                  "] name=[" + escape(std::string(entry.name())) +
                  "] inputs=[" + joined(entry.inputs()) +
                  "] multiline=" + (entry.multiline() ? "1" : "0") +
                  " full=[" + escape(entry.fullname()) + "]");
    }

    return;
}

/**
 * @brief Reads a text through a format, and writes the items read.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the lines name the items by.
 * @param[in]     _format  the format.
 * @param[in]     _text    the text.
 */
void
read_text(
    run&                      _run,
    const char*               _label,
    const cli::config_format& _format,
    const char*               _text
)
{
    cli::config_item_list items;
    std::string           error;

    (void)_format.from_text(_text,
                            items,
                            error);
    write_items(_run,
                _label,
                items);

    return;
}

// 1.2    Reading
//------------------------------------------------------------------------------

// TOML_TEXT
//   constant: TOML of every kind a line can be: a comment, values plain,
// quoted, listed and running over lines, a name alone, sections nested and
// repeated.
const char TOML_TEXT[] =
    "# a comment line\n"
    "name = \"joe\"\n"
    "count = 3\n"
    "list = [1, 2, 3]\n"
    "quoted = 'single quoted'\n"
    "flag\n"
    "multi = \"\"\"first\n"
    "second\"\"\"\n"
    "[section]\n"
    "inner = value  # a trailing comment\n"
    "[section.deeper]\n"
    "deep = \"x\"\n"
    "[[array]]\n"
    "a = 1\n"
    "[[array]]\n"
    "a = 2\n";

// INI_TEXT
//   constant: INI: a comment, the default section, values listed by spaces,
// and a section.
const char INI_TEXT[] =
    "; a comment line\n"
    "[default]\n"
    "name=joe\n"
    "list=1 2 3\n"
    "[section]\n"
    "inner = \"quoted value\"\n"
    "other=a b\n";

/**
 * @brief READ 1: TOML read by the format as made, and by CLI11's
 *        ConfigTOML, which it is.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_toml(
    run& _run
)
{
    const cli::config_format made;
    const cli::config_toml   toml;

    read_text(_run,
              "made",
              made,
              TOML_TEXT);
    read_text(_run,
              "toml",
              toml,
              TOML_TEXT);

    return;
}

/**
 * @brief READ 2: INI read by the INI format, and the TOML text by it.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_ini(
    run& _run
)
{
    const cli::config_ini ini;

    read_text(_run,
              "ini",
              ini,
              INI_TEXT);
    read_text(_run,
              "ini of toml",
              ini,
              TOML_TEXT);

    return;
}

/**
 * @brief READ 3: comments marked by `%`, against the format as made.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_comment(
    run& _run
)
{
    static const char  TEXT[] =
        "a = 1 % after a percent\n"
        "% a percent line\n"
        "# a hash line\n"
        "b = 2 # after a hash\n";
    cli::config_format percent;

    percent.comment('%');
    read_text(_run,
              "made",
              cli::config_format(),
              TEXT);
    read_text(_run,
              "percent",
              percent,
              TEXT);

    return;
}

/**
 * @brief READ 4: arrays between `<` and `>`, and separated by `;`, against
 *        the format as made.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_arrays(
    run& _run
)
{
    static const char  TEXT[] =
        "a = <1, 2>\n"
        "b = [3, 4]\n"
        "c = [5; 6]\n";
    cli::config_format angled;
    cli::config_format semicolons;

    angled.array_bounds('<',
                        '>');
    semicolons.array_separator(';');
    read_text(_run,
              "made",
              cli::config_format(),
              TEXT);
    read_text(_run,
              "angled",
              angled,
              TEXT);
    read_text(_run,
              "semicolons",
              semicolons,
              TEXT);

    return;
}

/**
 * @brief READ 5: names and values separated by `:`, against the format as
 *        made.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_delimiter(
    run& _run
)
{
    static const char  TEXT[] =
        "a: 1\n"
        "b : two\n"
        "c = 3\n";
    cli::config_format colons;

    colons.value_separator(':');
    read_text(_run,
              "made",
              cli::config_format(),
              TEXT);
    read_text(_run,
              "colons",
              colons,
              TEXT);

    return;
}

/**
 * @brief READ 6: the two quotes swapped, against the format as made: the
 *        one escapes are read in, and the one they are not.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_quotes(
    run& _run
)
{
    static const char  TEXT[] =
        "a = 'x\\ty'\n"
        "b = \"z\\tw\"\n";
    cli::config_format swapped;

    swapped.quote_character('\'',
                            '"');
    read_text(_run,
              "made",
              cli::config_format(),
              TEXT);
    read_text(_run,
              "swapped",
              swapped,
              TEXT);

    return;
}

/**
 * @brief READ 7: sections nested past one layer, and past none, against
 *        the format as made.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_layers(
    run& _run
)
{
    static const char  TEXT[] =
        "[a.b.c]\n"
        "x = 1\n"
        "[a]\n"
        "y = 2\n"
        "d.e.f = 3\n";
    cli::config_format one;
    cli::config_format none;

    one.max_layers(1);
    none.max_layers(0);
    read_text(_run,
              "made",
              cli::config_format(),
              TEXT);
    read_text(_run,
              "one",
              one,
              TEXT);
    read_text(_run,
              "none",
              none,
              TEXT);

    return;
}

/**
 * @brief READ 8: sections separated by `/`, against the format as made.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_parents(
    run& _run
)
{
    static const char  TEXT[] =
        "[a/b]\n"
        "x = 1\n"
        "c/d = 2\n"
        "e.f = 3\n";
    cli::config_format slashes;

    slashes.parent_separator('/');
    read_text(_run,
              "made",
              cli::config_format(),
              TEXT);
    read_text(_run,
              "slashes",
              slashes,
              TEXT);

    return;
}

/**
 * @brief READ 9: a name given again after another, merged only when
 *        duplicate fields are allowed.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_duplicates(
    run& _run
)
{
    static const char  TEXT[] =
        "x = 1\n"
        "y = 2\n"
        "x = 3\n"
        "x = 4\n";
    cli::config_format duplicates;

    duplicates.allow_duplicate_fields();
    read_text(_run,
              "made",
              cli::config_format(),
              TEXT);
    read_text(_run,
              "duplicates",
              duplicates,
              TEXT);

    return;
}

/**
 * @brief READ 10: one section read as the top level, and the section and
 *        index read back, as made and as set.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_section(
    run& _run
)
{
    static const char  TEXT[] =
        "top = 1\n"
        "[sec]\n"
        "x = 2\n"
        "[sec.sub]\n"
        "y = 3\n"
        "[other]\n"
        "z = 4\n";
    cli::config_format chosen;

    _run.note("made: section=[" + std::string(chosen.get_section()) +
              "] index=" + number(static_cast<long>(chosen.get_index())));
    chosen.section("sec");
    _run.note("set: section=[" + std::string(chosen.get_section()) +
              "] index=" + number(static_cast<long>(chosen.get_index())));
    read_text(_run,
              "sec",
              chosen,
              TEXT);
    chosen.section("");
    read_text(_run,
              "every",
              chosen,
              TEXT);

    return;
}

/**
 * @brief READ 11: one occurrence of a repeated section read, the first,
 *        the second, and one that is not there.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_index(
    run& _run
)
{
    static const char  TEXT[] =
        "[[arr]]\n"
        "x = 1\n"
        "[[arr]]\n"
        "x = 2\n"
        "[[arr]]\n"
        "x = 3\n";
    cli::config_format chosen;

    chosen.section("arr");
    chosen.index(0);
    read_text(_run,
              "first",
              chosen,
              TEXT);
    chosen.index(1);
    _run.note("set: section=[" + std::string(chosen.get_section()) +
              "] index=" + number(static_cast<long>(chosen.get_index())));
    read_text(_run,
              "second",
              chosen,
              TEXT);
    chosen.index(5);
    read_text(_run,
              "none",
              chosen,
              TEXT);

    return;
}

/**
 * @brief READ 12: files read, TOML and INI.
 *
 * @param[in,out] _run  the step's run.
 */
void
read_files(
    run& _run
)
{
    static const file_entry FILES[] =
    {
        { "read.toml", "a = 1\n[s]\nb = [2, 3]\n" },
        { "read.ini",  "[default]\na=1\n[s]\nb=2 3\n" },
        { D_NULLPTR,   D_NULLPTR }
    };
    const cli::config_toml  toml;
    const cli::config_ini   ini;
    cli::config_item_list   from_toml;
    cli::config_item_list   from_ini;
    std::string             error;
    scratch_directory       files(FILES);

    // the files the step reads
    if (!files.ok())
    {
        _run.note("the scratch directory was not made");
    }

    (void)toml.from_file("read.toml",
                         from_toml,
                         error);
    (void)ini.from_file("read.ini",
                        from_ini,
                        error);
    write_items(_run,
                "toml file",
                from_toml);
    write_items(_run,
                "ini file",
                from_ini);

    return;
}

// READ_STEPS
//   constant: the steps of d_tests_arg_cpp_config_read.
const step_fn READ_STEPS[] =
{
    &read_toml,
    &read_ini,
    &read_comment,
    &read_arrays,
    &read_delimiter,
    &read_quotes,
    &read_layers,
    &read_parents,
    &read_duplicates,
    &read_section,
    &read_index,
    &read_files
};

// READ_COUNT
//   constant: how many steps READ_STEPS holds.
const std::size_t READ_COUNT = sizeof(READ_STEPS) / sizeof(READ_STEPS[0]);

// 1.3    Writing
//------------------------------------------------------------------------------

// WRITTEN_ARGS
//   constant: the arguments the writing steps parse before they write.
const char* const WRITTEN_ARGS[] =
{
    "--name", "joe", "--names", "a", "b", "-v", "--text", "two words",
    "sub", "--deep", "9"
};

// WRITTEN_ARG_COUNT
//   constant: how many arguments WRITTEN_ARGS holds.
const std::size_t WRITTEN_ARG_COUNT =
    sizeof(WRITTEN_ARGS) / sizeof(WRITTEN_ARGS[0]);

/**
 * @brief Defines the program the writing steps write, and parses
 *        WRITTEN_ARGS: a count with a default, not given, a name, names
 *        bound to a vector, a flag, a text bound to a string, and a
 *        subcommand with an option given and one with a default; each
 *        described.
 *
 * @param[in,out] _run  the run.
 * @return the subcommand.
 */
cli::command
define_written(
    run& _run
)
{
    static std::vector<std::string> names;
    static std::string              text;

    _run.app().add_option("--count",
                          "a count")
              ->default_str("7");
    (void)_run.app().add_option("--name",
                                "a name");
    (void)_run.app().add_option("--names",
                                names,
                                "some names");
    (void)_run.app().add_flag("-v,--verbose",
                              "say more");
    (void)_run.app().add_option("--text",
                                text,
                                "a text");

    cli::command sub = _run.app().add_subcommand("sub",
                                                 "a subcommand");

    (void)sub->add_option("--deep",
                          "a deep value");
    sub->add_option("--depth",
                    "a depth")
       ->default_str("2");

    // the values written
    if (!_run.parse_quiet(WRITTEN_ARGS,
                          WRITTEN_ARG_COUNT))
    {
        _run.note("the parse failed");
    }

    return sub;
}

/**
 * @brief Writes the program's configuration: the values given; every
 *        default, with descriptions; and the subcommands' defaults.
 *
 * @param[in,out] _run  the run.
 */
void
write_modes(
    run& _run
)
{
    _run.note("active [" + escape(_run.app().config_to_str()) + "]");
    _run.note("defaults [" +
              escape(_run.app().config_to_str(true,
                                              true)) + "]");
    _run.note("subcommand defaults [" +
              escape(_run.app().config_to_str(
                  cli::config_output_mode::active_subcommand_defaults)) +
              "]");

    return;
}

/**
 * @brief WRITTEN 1: the program written as TOML, by the command's own
 *        format and by one given to it.
 *
 * @param[in,out] _run  the step's run.
 */
void
written_toml(
    run& _run
)
{
    static cli::config_toml toml;

    (void)define_written(_run);
    write_modes(_run);
    _run.app().config_formatter(toml);
    _run.note("given [" + escape(_run.app().config_to_str()) + "]");

    return;
}

/**
 * @brief WRITTEN 2: the program written as INI.
 *
 * @param[in,out] _run  the step's run.
 */
void
written_ini(
    run& _run
)
{
    static cli::config_ini ini;

    _run.app().config_formatter(ini);
    (void)define_written(_run);
    write_modes(_run);

    return;
}

/**
 * @brief WRITTEN 3: the program written with `;` comments, its
 *        descriptions among them.
 *
 * @param[in,out] _run  the step's run.
 */
void
written_comment(
    run& _run
)
{
    static cli::config_format semicolon;

    semicolon.comment(';');
    _run.app().config_formatter(semicolon);
    (void)define_written(_run);
    _run.note("described [" +
              escape(_run.app().config_to_str(false,
                                              true)) + "]");

    return;
}

/**
 * @brief WRITTEN 4: the program written with arrays between `(` and `)`,
 *        then separated by `;`.
 *
 * @param[in,out] _run  the step's run.
 */
void
written_arrays(
    run& _run
)
{
    static cli::config_format arrays;

    arrays.array_bounds('(',
                        ')');
    _run.app().config_formatter(arrays);
    (void)define_written(_run);
    _run.note("parentheses [" + escape(_run.app().config_to_str()) + "]");
    arrays.array_separator(';');
    _run.note("semicolons [" + escape(_run.app().config_to_str()) + "]");

    return;
}

/**
 * @brief WRITTEN 5: the program written with `:` between names and
 *        values, then with the quotes swapped.
 *
 * @param[in,out] _run  the step's run.
 */
void
written_delimiters(
    run& _run
)
{
    static cli::config_format delimited;

    delimited.value_separator(':');
    _run.app().config_formatter(delimited);
    (void)define_written(_run);
    _run.note("colons [" + escape(_run.app().config_to_str()) + "]");
    delimited.quote_character('\'',
                              '"');
    _run.note("swapped [" + escape(_run.app().config_to_str()) + "]");

    return;
}

/**
 * @brief WRITTEN 6: the program written with `/` between sections, then
 *        every default, then every default commented.
 *
 * @param[in,out] _run  the step's run.
 */
void
written_parents(
    run& _run
)
{
    static cli::config_format slashes;

    slashes.parent_separator('/');
    _run.app().config_formatter(slashes);
    (void)define_written(_run);
    _run.note("slashes [" + escape(_run.app().config_to_str()) + "]");
    _run.note("defaults [" +
              escape(_run.app().config_to_str(true,
                                              false)) + "]");
    slashes.comment_defaults();
    _run.note("commented [" +
              escape(_run.app().config_to_str(true,
                                              false)) + "]");

    return;
}

/**
 * @brief WRITTEN 7: the program written with its subcommand configurable,
 *        a section of its own.
 *
 * @param[in,out] _run  the step's run.
 */
void
written_configurable(
    run& _run
)
{
    cli::command sub = define_written(_run);

    sub->configurable();
    _run.note("configurable [" + escape(_run.app().config_to_str()) + "]");

    return;
}

// WRITTEN_STEPS
//   constant: the steps of d_tests_arg_cpp_config_written.
const step_fn WRITTEN_STEPS[] =
{
    &written_toml,
    &written_ini,
    &written_comment,
    &written_arrays,
    &written_delimiters,
    &written_parents,
    &written_configurable
};

// WRITTEN_COUNT
//   constant: how many steps WRITTEN_STEPS holds.
const std::size_t WRITTEN_COUNT =
    sizeof(WRITTEN_STEPS) / sizeof(WRITTEN_STEPS[0]);

// 1.4    A format given to a command
//------------------------------------------------------------------------------

/**
 * @brief Defines the program the parsing steps parse, each part watched: a
 *        configuration option, a count, names that keep every value, a
 *        flag, and a subcommand with an option and a flag of its own.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _default  the configuration option's default file; ""
 *                          for none.
 */
void
define_parsed(
    run&        _run,
    const char* _default
)
{
    cli::command sub = _run.make_subcommand(_run.root(),
                                            "sub");

    (void)_run.watch("config",
                     _run.app().set_config("--config",
                                           _default));
    (void)_run.make_option("count",
                           _run.root(),
                           "--count");
    (void)_run.make_all("names",
                        _run.root(),
                        "--names");
    (void)_run.make_flag("verbose",
                         _run.root(),
                         "--verbose");
    (void)_run.make_option("deep",
                           sub,
                           "--deep");
    (void)_run.make_flag("loud",
                         sub,
                         "--loud");

    return;
}

// PARSED_FILES
//   constant: the files the parsing steps read.
const file_entry PARSED_FILES[] =
{
    { "conf.ini",
      "[default]\ncount=5\nnames=a b\nverbose=true\n[sub]\ndeep=3\n" },
    { "default.toml",
      "count = 6\nnames = [\"c\", \"d\"]\n" },
    { "sections.toml",
      "count = 1\n[app]\ncount = 7\nverbose = true\n[other]\ncount = 8\n" },
    { D_NULLPTR,
      D_NULLPTR }
};

/**
 * @brief PARSED 1: an INI file named on the command line, read by the INI
 *        format given to the command.
 *
 * @param[in,out] _run  the step's run.
 */
void
parsed_ini(
    run& _run
)
{
    static const char* const ARGS[] =
    {
        "--config", "conf.ini"
    };
    static cli::config_ini   ini;
    scratch_directory        files(PARSED_FILES);

    // the files the step reads
    if (!files.ok())
    {
        _run.note("the scratch directory was not made");
    }

    _run.app().config_formatter(ini);
    define_parsed(_run,
                  "");
    _run.parse(ARGS,
               2u);

    return;
}

/**
 * @brief PARSED 2: the configuration option's default file, read by the
 *        command's own format when the option is not given.
 *
 * @param[in,out] _run  the step's run.
 */
void
parsed_default(
    run& _run
)
{
    scratch_directory files(PARSED_FILES);

    // the files the step reads
    if (!files.ok())
    {
        _run.note("the scratch directory was not made");
    }

    define_parsed(_run,
                  "default.toml");
    _run.parse(D_NULLPTR,
               0u);

    return;
}

/**
 * @brief PARSED 3: one section of a file read as the top level, by a
 *        format given that section.
 *
 * @param[in,out] _run  the step's run.
 */
void
parsed_section(
    run& _run
)
{
    static const char* const ARGS[] =
    {
        "--config", "sections.toml"
    };
    static cli::config_format chosen;
    scratch_directory         files(PARSED_FILES);

    // the files the step reads
    if (!files.ok())
    {
        _run.note("the scratch directory was not made");
    }

    chosen.section("app");
    _run.app().config_formatter(chosen);
    define_parsed(_run,
                  "");
    _run.parse(ARGS,
               2u);

    return;
}

/**
 * @brief PARSED 4: text parsed from a stream by a format with `:` between
 *        names and values and `;` comments.
 *
 * @param[in,out] _run  the step's run.
 */
void
parsed_stream(
    run& _run
)
{
    static cli::config_format colons;
    std::istringstream        input("count: 8\n"
                                    "; a comment\n"
                                    "names: [\"e\", \"f\"]\n"
                                    "verbose: true\n"
                                    "[sub]\n"
                                    "deep: 2\n"
                                    "loud: true\n");

    colons.value_separator(':');
    colons.comment(';');
    _run.app().config_formatter(colons);
    define_parsed(_run,
                  "");
    _run.parse_stream(input);

    return;
}

// PARSED_STEPS
//   constant: the steps of d_tests_arg_cpp_config_parsed.
const step_fn PARSED_STEPS[] =
{
    &parsed_ini,
    &parsed_default,
    &parsed_section,
    &parsed_stream
};

// PARSED_COUNT
//   constant: how many steps PARSED_STEPS holds.
const std::size_t PARSED_COUNT =
    sizeof(PARSED_STEPS) / sizeof(PARSED_STEPS[0]);

// 1.5    Operations replaced
//------------------------------------------------------------------------------

// framed
//   class: a format whose writing frames each command it writes -- the root,
// and each subcommand, which the writing writes through the same format --
// between comment lines naming the command and the prefix it is written
// under.
class framed : public cli::config_format
{
public:
    /**
     * @brief Writes the format's own configuration of a command between
     *        a line before and a line after.
     *
     * @param[in]     _command            the command.
     * @param[in]     _mode               which values are written.
     * @param[in]     _write_description  whether descriptions are written.
     * @param[in]     _prefix             what precedes each name.
     * @param[in,out] _out                the text being built.
     * @return the status of the first part that failed, or ok.
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
        const std::string named = std::string(_command->get_name()) +
                                  " [" + std::string(_prefix) + "]\n";
        cli::status       written = _out.append(cli::text("# begin " +
                                                          named));

        written = (written.ok())
                  ? cli::config_format::to_config(_command,
                                                  _mode,
                                                  _write_description,
                                                  _prefix,
                                                  _out)
                  : written;

        return (written.ok())
               ? _out.append(cli::text("# end " + named))
               : written;
    }
};

// spaced
//   class: a format that reads a syntax of its own: on each line a name,
// its sections before it joined by `.`, then its values, separated by
// spaces; a line starting with `#` and an empty line are skipped.
class spaced : public cli::config_format
{
public:
    /**
     * @brief Reads the format's syntax into items.
     *
     * @param[in]     _input  the text.
     * @param[in,out] _out    the items read, appended.
     * @param[in,out] _error  a failure's description; unused.
     * @return the status of the first item that could not be built or
     *         kept, or ok.
     */
    virtual cli::status
    from_config(
        const cli::text&   _input,
        cli::config_items& _out,
        cli::buffer&       _error
    ) const D_OVERRIDE
    {
        const std::string  input(_input);
        std::istringstream lines(input);
        std::string        line;
        cli::status        read;

        (void)_error;

        // each line, while each is read
        while ( (read.ok()) &&
                (std::getline(lines,
                              line)) )
        {
            std::istringstream words(line);
            std::string        word;

            // an empty line and a comment hold no item
            if ( (!(words >> word)) ||
                 (word[0] == '#') )
            {
                continue;
            }

            read = build(word,
                         words,
                         _out);
        }

        return read;
    }

private:
    /**
     * @brief Builds an item from its name and the words after it, and
     *        adds it to the items.
     *
     * @param[in]     _name   the name, its sections before it.
     * @param[in,out] _words  the words after it, its values.
     * @param[in,out] _out    the items.
     * @return the status of the first part that failed, or ok.
     */
    static cli::status
    build(
        const std::string& _name,
        std::istream&      _words,
        cli::config_items& _out
    )
    {
        cli::config_item       item;
        cli::status            built;
        std::string::size_type start = 0u;
        std::string::size_type dot   = _name.find('.');
        std::string            word;

        // each section before the name
        while ( (built.ok()) &&
                (dot != std::string::npos) )
        {
            built = item.add_parent(_name.substr(start,
                                                 dot - start));
            start = dot + 1u;
            dot   = _name.find('.',
                               start);
        }

        built = (built.ok())
                ? item.set_name(_name.substr(start))
                : built;

        // each value
        while ( (built.ok()) &&
                (_words >> word) )
        {
            built = item.add_input(word);
        }

        return (built.ok())
               ? _out.adopt(item)
               : built;
    }
};

// worded
//   class: a format that reads `oui` and `non` as a flag's true and false,
// and any other value as the format's own does.
class worded : public cli::config_format
{
public:
    /**
     * @brief Writes the text a flag takes from an item.
     *
     * @param[in]     _item  the item.
     * @param[in,out] _out   the text, appended.
     * @return the append's status, or the format's own.
     */
    virtual cli::status
    to_flag(
        const cli::config_entry& _item,
        cli::buffer&             _out
    ) const D_OVERRIDE
    {
        const cli::text_list inputs = _item.inputs();

        // a word of the format's own
        if (inputs.size() == 1u)
        {
            const std::string value(inputs[0]);

            if (value == "oui")
            {
                return _out.append(cli::text("true"));
            }

            if (value == "non")
            {
                return _out.append(cli::text("false"));
            }
        }

        return cli::config_format::to_flag(_item,
                                           _out);
    }
};

/**
 * @brief REPLACED 1: the program written through a format that frames
 *        each command it writes, the values given and every default.
 *
 * @param[in,out] _run  the step's run.
 */
void
replaced_framed(
    run& _run
)
{
    static framed frames;

    _run.app().config_formatter(frames);
    (void)define_written(_run);
    _run.note("framed [" + escape(_run.app().config_to_str()) + "]");
    _run.note("defaults [" +
              escape(_run.app().config_to_str(true,
                                              false)) + "]");

    return;
}

/**
 * @brief REPLACED 2: text in a syntax of the format's own, read into items
 *        and parsed from a stream.
 *
 * @param[in,out] _run  the step's run.
 */
void
replaced_spaced(
    run& _run
)
{
    static const char         TEXT[] =
        "count 4\n"
        "# a comment\n"
        "\n"
        "names g h i\n"
        "verbose\n"
        "sub.deep 9\n";
    static spaced             words;
    cli::config_item_list     items;
    std::string               error;
    std::istringstream        input(TEXT);

    (void)words.from_text(TEXT,
                          items,
                          error);
    write_items(_run,
                "spaced",
                items);
    _run.app().config_formatter(words);
    define_parsed(_run,
                  "");
    _run.parse_stream(input);

    return;
}

/**
 * @brief REPLACED 3: flags read from words of the format's own, the root's
 *        and a subcommand's.
 *
 * @param[in,out] _run  the step's run.
 */
void
replaced_worded(
    run& _run
)
{
    static worded      words;
    std::istringstream input("verbose = oui\n"
                             "[sub]\n"
                             "loud = non\n");

    _run.app().config_formatter(words);
    define_parsed(_run,
                  "");
    _run.parse_stream(input);

    return;
}

// REPLACED_STEPS
//   constant: the steps of d_tests_arg_cpp_config_replaced.
const step_fn REPLACED_STEPS[] =
{
    &replaced_framed,
    &replaced_spaced,
    &replaced_worded
};

// REPLACED_COUNT
//   constant: how many steps REPLACED_STEPS holds.
const std::size_t REPLACED_COUNT =
    sizeof(REPLACED_STEPS) / sizeof(REPLACED_STEPS[0]);

// 1.6    Copies
//------------------------------------------------------------------------------

/**
 * @brief COPIES 1: formats copied by construction and assignment, with
 *        their settings, each changed apart from the other, read through;
 *        an INI format copied as a format, and a format whose writing is
 *        replaced copied as a format, written through.
 *
 * @param[in,out] _run  the step's run.
 */
void
copies_made(
    run& _run
)
{
    static const char         TEXT[] =
        "top: 0\n"
        "[s]\n"
        "a: 1 ; a comment\n"
        "b = 2\n"
        "[t]\n"
        "c: 3\n";
    static cli::config_format original;
    static cli::config_format copied;
    static cli::config_format assigned;
    static framed             frames;
    static cli::config_format sliced;
    static cli::config_format from_ini = cli::config_ini();

    original.value_separator(':');
    original.comment(';');
    original.section("s");
    copied = cli::config_format(original);
    assigned = original;
    copied.value_separator('=');
    assigned.section("t");
    read_text(_run,
              "original",
              original,
              TEXT);
    read_text(_run,
              "copied",
              copied,
              TEXT);
    read_text(_run,
              "assigned",
              assigned,
              TEXT);
    _run.note("sections: original=[" + std::string(original.get_section()) +
              "] copied=[" + std::string(copied.get_section()) +
              "] assigned=[" + std::string(assigned.get_section()) + "]");
    sliced = frames;
    _run.app().config_formatter(sliced);
    (void)define_written(_run);
    _run.note("sliced [" + escape(_run.app().config_to_str()) + "]");
    _run.app().config_formatter(from_ini);
    _run.note("from ini [" + escape(_run.app().config_to_str()) + "]");

    return;
}

// COPIES_STEPS
//   constant: the steps of d_tests_arg_cpp_config_copies.
const step_fn COPIES_STEPS[] =
{
    &copies_made
};

// COPIES_COUNT
//   constant: how many steps COPIES_STEPS holds.
const std::size_t COPIES_COUNT =
    sizeof(COPIES_STEPS) / sizeof(COPIES_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

}  // namespace

//==============================================================================
// 2.  THE MODULE'S OWN
//==============================================================================
// What CLI11 cannot write: the settings read back, items built, operations
// that fail by their statuses, and the departures; without configuration
// files, what a format and a configuration option come to. Each is a step
// written here, through the whole face.

namespace
{

// 2.1    Settings
//------------------------------------------------------------------------------

/**
 * @brief A character as a transcript writes it, escaped.
 *
 * @param[in] _character  the character.
 * @return the character, escaped: a NUL as `\\x00`.
 */
std::string
character(
    char _character
)
{
    return escape(cli::text(&_character,
                            1u));
}

/**
 * @brief Writes a format's settings, read back, on one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line names the format by.
 * @param[in]     _format  the format.
 */
void
write_settings(
    run&                      _run,
    const char*               _label,
    const cli::config_format& _format
)
{
    _run.note(std::string(_label) +
              ": comment=[" + character(_format.get_comment()) +
              "] array=[" + character(_format.get_array_start()) +
              character(_format.get_array_end()) +
              "] separator=[" + character(_format.get_array_separator()) +
              "] delimiter=[" + character(_format.get_value_separator()) +
              "] quotes=[" + character(_format.get_string_quote()) +
              character(_format.get_literal_quote()) +
              "] parent=[" + character(_format.get_parent_separator()) +
              "] layers=" +
              number(static_cast<long>(_format.get_max_layers())) +
              " comment_defaults=" +
              (_format.get_comment_defaults() ? "1" : "0") +
              " duplicates=" +
              (_format.get_allow_duplicate_fields() ? "1" : "0") +
              " section=[" + escape(_format.get_section()) +
              "] index=" + number(static_cast<long>(_format.get_index())));

    return;
}

/**
 * @brief SETTINGS 1: the settings of TOML and of INI as made, then each
 *        changed, in a chain, and read back; a copy of each.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_read(
    run& _run
)
{
    const cli::config_toml toml;
    const cli::config_ini  ini;
    cli::config_format     changed;

    write_settings(_run,
                   "toml",
                   toml);
    write_settings(_run,
                   "ini",
                   ini);
    changed.comment(';')
           .array_bounds('(',
                         ')')
           .array_separator('|')
           .value_separator(':')
           .quote_character('`',
                            '"')
           .max_layers(3)
           .parent_separator('/')
           .comment_defaults()
           .allow_duplicate_fields()
           .section("part")
           .index(2);
    write_settings(_run,
                   "changed",
                   changed);
    changed.comment('\0')
           .array_bounds('\0',
                         '\0')
           .comment_defaults(false)
           .allow_duplicate_fields(false)
           .section(cli::c_string())
           .index(-1);
    write_settings(_run,
                   "changed back",
                   changed);
    write_settings(_run,
                   "ini copied",
                   cli::config_format(ini));

    return;
}

// SETTINGS_STEPS
//   constant: the steps of d_tests_arg_cpp_config_settings.
const step_fn SETTINGS_STEPS[] =
{
    &settings_read
};

// SETTINGS_COUNT
//   constant: how many steps SETTINGS_STEPS holds.
const std::size_t SETTINGS_COUNT =
    sizeof(SETTINGS_STEPS) / sizeof(SETTINGS_STEPS[0]);

// 2.2    Items
//------------------------------------------------------------------------------

/**
 * @brief Writes an item's view on one line: whether it views an item, its
 *        sections, name, values, whether it ran over lines, and its full
 *        name.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line names the item by.
 * @param[in]     _entry  the view.
 */
void
write_entry(
    run&                     _run,
    const char*              _label,
    const cli::config_entry& _entry
)
{
    const cli::text_list parents = _entry.parents();
    const cli::text_list inputs  = _entry.inputs();
    std::string          line    = std::string(_label) + ": valid=" +
                                   (_entry.valid() ? "1" : "0") +
                                   " parents=[";

    for (std::size_t index = 0u; index < parents.size(); ++index)
    {
        line += ((index != 0u) ? "," : "") + escape(parents[index]);
    }

    line += "] name=[" + escape(_entry.name()) + "] inputs=[";

    for (std::size_t index = 0u; index < inputs.size(); ++index)
    {
        line += ((index != 0u) ? "," : "") + escape(inputs[index]);
    }

    _run.note(line + "] multiline=" + (_entry.multiline() ? "1" : "0") +
              " full=[" + escape(_entry.fullname()) + "]");

    return;
}

/**
 * @brief ITEMS 1: an item built by its calls -- NUL bytes in its text
 *        kept -- viewed, adopted by a list, which leaves it empty, and the
 *        list cleared; a view of none.
 *
 * @param[in,out] _run  the step's run.
 */
void
items_built(
    run& _run
)
{
    static const char       NUL_NAME[]  = { 'n', '\0', 'm' };
    static const char       NUL_VALUE[] = { 'x', '\0', 'y' };
    cli::config_item_list   list;
    cli::config_items       items = list.items();
    cli::config_item        item;
    cli::config_item        second;
    const cli::config_entry none;
    cli::status             built;

    write_entry(_run,
                "none",
                none);
    write_entry(_run,
                "made",
                item.entry());
    built = item.set_name("name");
    built = (built.ok()) ? item.add_parent("a") : built;
    built = (built.ok()) ? item.add_parent("b") : built;
    built = (built.ok()) ? item.add_input("1") : built;
    built = (built.ok())
            ? item.add_input(cli::text(NUL_VALUE,
                                       sizeof(NUL_VALUE)))
            : built;
    item.set_multiline();
    _run.note(std::string("built: ") + built.get_name());
    write_entry(_run,
                "item",
                item.entry());
    _run.note(std::string("adopted: ") + items.adopt(item).get_name());
    write_entry(_run,
                "item after",
                item.entry());
    built = second.set_name(cli::text(NUL_NAME,
                                      sizeof(NUL_NAME)));
    second.set_multiline(false);
    _run.note(std::string("second: ") + built.get_name() + " " +
              items.adopt(second).get_name());
    _run.note("list: size=" + number(static_cast<unsigned long>(list.size())) +
              " empty=" + (list.empty() ? "1" : "0") +
              " view size=" +
              number(static_cast<unsigned long>(items.size())) +
              " view empty=" + (items.empty() ? "1" : "0"));
    write_entry(_run,
                "list 0",
                list[0]);
    write_entry(_run,
                "view 1",
                items[1]);
    list.clear();
    _run.note("cleared: size=" +
              number(static_cast<unsigned long>(list.size())) +
              " empty=" + (list.empty() ? "1" : "0"));

    return;
}

// ITEMS_STEPS
//   constant: the steps of d_tests_arg_cpp_config_items.
const step_fn ITEMS_STEPS[] =
{
    &items_built
};

// ITEMS_COUNT
//   constant: how many steps ITEMS_STEPS holds.
const std::size_t ITEMS_COUNT = sizeof(ITEMS_STEPS) / sizeof(ITEMS_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 2.3    Failures
//------------------------------------------------------------------------------

// refusing
//   class: a format whose reading refuses every text, describing why, and
// whose writing refuses every command.
class refusing : public cli::config_format
{
public:
    /**
     * @brief Refuses the text.
     *
     * @param[in]     _input  the text; unused.
     * @param[in,out] _out    the items; unused.
     * @param[in,out] _error  the description, appended.
     * @return status_code::config_error, or the append's failure.
     */
    virtual cli::status
    from_config(
        const cli::text&   _input,
        cli::config_items& _out,
        cli::buffer&       _error
    ) const D_OVERRIDE
    {
        const cli::status described =
            _error.append(cli::text("refused by the format"));

        (void)_input;
        (void)_out;

        return (described.ok())
               ? cli::status(cli::status_code::config_error)
               : described;
    }

    /**
     * @brief Refuses the command, after writing part of it.
     *
     * @param[in]     _command            the command; unused.
     * @param[in]     _mode               unused.
     * @param[in]     _write_description  unused.
     * @param[in]     _prefix             unused.
     * @param[in,out] _out                the text being built.
     * @return status_code::config_error.
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
        (void)_command;
        (void)_mode;
        (void)_write_description;
        (void)_prefix;
        (void)_out.append(cli::text("partial"));

        return cli::status_code::config_error;
    }
};

/**
 * @brief FAILED 1: text with a malformed escape after a value, and a file
 *        that is not there: each read's status, its description, and what
 *        was read before.
 *
 * @param[in,out] _run  the step's run.
 */
void
failed_reads(
    run& _run
)
{
    const cli::config_toml toml;
    cli::config_item_list  items;
    std::string            error;
    scratch_directory      files;
    cli::status            read;

    // the scratch directory, which holds no file
    if (!files.ok())
    {
        _run.note("the scratch directory was not made");
    }

    read = toml.from_text("a = 1\nb = \"bad \\q\"\nc = 3\n",
                          items,
                          error);
    _run.note(std::string("escape: ") + read.get_name() + " [" +
              escape(error) + "]");
    write_items(_run,
                "escape",
                items);
    items.clear();
    error.clear();
    read = toml.from_file("missing.toml",
                          items,
                          error);
    _run.note(std::string("missing: ") + read.get_name() + " [" +
              escape(error) + "]");

    return;
}

/**
 * @brief FAILED 2: a format that refuses: a file read through it for a
 *        parse, text parsed from a stream, text read directly, and the
 *        command written through it.
 *
 * @param[in,out] _run  the step's run.
 */
void
failed_format(
    run& _run
)
{
    static const file_entry  FILES[] =
    {
        { "conf.toml", "count = 1\n" },
        { D_NULLPTR,   D_NULLPTR }
    };
    static const char* const ARGS[] =
    {
        "--config", "conf.toml"
    };
    static refusing          refuser;
    cli::config_item_list    items;
    std::string              error;
    std::istringstream       input("count = 2\n");
    scratch_directory        files(FILES);
    cli::status              read;

    // the file the parse reads
    if (!files.ok())
    {
        _run.note("the scratch directory was not made");
    }

    _run.app().config_formatter(refuser);
    define_parsed(_run,
                  "");
    _run.parse(ARGS,
               2u);
    _run.parse_stream(input);
    read = refuser.from_text("count = 3\n",
                             items,
                             error);
    _run.note(std::string("read: ") + read.get_name() + " [" +
              escape(error) + "] items=" +
              number(static_cast<unsigned long>(items.size())));
    _run.note("written [" + escape(_run.app().config_to_str()) + "]");

    return;
}

// FAILED_STEPS
//   constant: the steps of d_tests_arg_cpp_config_failed.
const step_fn FAILED_STEPS[] =
{
    &failed_reads,
    &failed_format
};

// FAILED_COUNT
//   constant: how many steps FAILED_STEPS holds.
const std::size_t FAILED_COUNT =
    sizeof(FAILED_STEPS) / sizeof(FAILED_STEPS[0]);

// 2.4    Departures
//------------------------------------------------------------------------------

/**
 * @brief DEPARTED 1: layers and an index outside the ranges of CLI11's
 *        parameters: held at the nearer end, and kept, here.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_ranges(
    run& _run
)
{
    static const char  LAYERS[] =
        "[a.b.c]\n"
        "x = 1\n";
    static const char  REPEATED[] =
        "[[arr]]\n"
        "x = 1\n"
        "[[arr]]\n"
        "x = 2\n";
    cli::config_format many;
    cli::config_format negative;
    cli::config_format far;

    many.max_layers(257);
    negative.max_layers(-1);
    far.section("arr")
       .index(65537);
    write_settings(_run,
                   "257 layers",
                   many);
    read_text(_run,
              "257 layers",
              many,
              LAYERS);
    write_settings(_run,
                   "-1 layers",
                   negative);
    read_text(_run,
              "-1 layers",
              negative,
              LAYERS);
    write_settings(_run,
                   "index 65537",
                   far);
    read_text(_run,
              "index 65537",
              far,
              REPEATED);

    return;
}

/**
 * @brief DEPARTED 2: a subcommand made before the root is given a format,
 *        whose flags follow the root's format.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_followed(
    run& _run
)
{
    static worded      words;
    std::istringstream input("[sub]\n"
                             "loud = oui\n");

    define_parsed(_run,
                  "");
    _run.app().config_formatter(words);
    _run.parse_stream(input);

    return;
}

// DEPARTED_STEPS
//   constant: the steps of d_tests_arg_cpp_config_departures.
const step_fn DEPARTED_STEPS[] =
{
    &departed_ranges,
    &departed_followed
};

// DEPARTED_COUNT
//   constant: how many steps DEPARTED_STEPS holds.
const std::size_t DEPARTED_COUNT =
    sizeof(DEPARTED_STEPS) / sizeof(DEPARTED_STEPS[0]);

#else

// 2.3    Without configuration files
//------------------------------------------------------------------------------

/**
 * @brief UNBUILT 1: a format's reading and writing, which answer
 *        unsupported; reading a flag, which is built in every build.
 *
 * @param[in,out] _run  the step's run.
 */
void
unbuilt_operations(
    run& _run
)
{
    const cli::config_format format;
    cli::config_item_list    list;
    cli::config_items        items = list.items();
    cli::config_item         flag;
    ::d_arg_buffer           described = D_ARG_BUFFER_INIT;
    ::d_arg_buffer           written   = D_ARG_BUFFER_INIT;
    cli::buffer              error(described);
    cli::buffer              out(written);
    const cli::status        read = format.from_config(cli::text("a = 1"),
                                                       items,
                                                       error);
    const cli::status        wrote = format.to_config(
                                         _run.root(),
                                         cli::config_output_mode::active,
                                         false,
                                         cli::text(),
                                         out);
    cli::status              flagged;

    _run.note(std::string("from_config: ") + read.get_name() +
              " items=" + number(static_cast<unsigned long>(items.size())) +
              " to_config: " + wrote.get_name() +
              " [" + escape(out.view()) + "]");
    flagged = format.to_flag(flag.entry(),
                             out);
    _run.note(std::string("none: ") + flagged.get_name() + " [" +
              escape(out.view()) + "]");
    out.truncate(0u);
    flagged = flag.add_input("yes");
    flagged = (flagged.ok()) ? format.to_flag(flag.entry(),
                                              out) : flagged;
    _run.note(std::string("one: ") + flagged.get_name() + " [" +
              escape(out.view()) + "]");
    d_arg_buffer_release(&described);
    d_arg_buffer_release(&written);

    return;
}

/**
 * @brief UNBUILT 2: a configuration option naming a file that is there,
 *        one that is not, and a default that is not there.
 *
 * @param[in,out] _run  the step's run.
 */
void
unbuilt_option(
    run& _run
)
{
    static const file_entry  FILES[] =
    {
        { "there.cfg", "count = 1\n" },
        { D_NULLPTR,   D_NULLPTR }
    };
    static const char* const THERE[] =
    {
        "--config", "there.cfg"
    };
    static const char* const GONE[] =
    {
        "--config", "gone.cfg"
    };
    scratch_directory        files(FILES);

    // the file the first parse names
    if (!files.ok())
    {
        _run.note("the scratch directory was not made");
    }

    (void)_run.watch("config",
                     _run.app().set_config("--config",
                                           "gone.cfg"));
    (void)_run.make_option("count",
                           _run.root(),
                           "--count");
    _run.parse(THERE,
               2u);
    _run.parse(GONE,
               2u);
    _run.parse(D_NULLPTR,
               0u);

    return;
}

// UNBUILT_STEPS
//   constant: the steps of d_tests_arg_cpp_config_unbuilt.
const step_fn UNBUILT_STEPS[] =
{
    &unbuilt_operations,
    &unbuilt_option
};

// UNBUILT_COUNT
//   constant: how many steps UNBUILT_STEPS holds.
const std::size_t UNBUILT_COUNT =
    sizeof(UNBUILT_STEPS) / sizeof(UNBUILT_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

}  // namespace

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

//==============================================================================
// 3.  CLI11'S CONFIGURATION
//==============================================================================

/*
d_tests_arg_cpp_config_read
  Text and files read into items, as CLI11's ConfigTOML and ConfigINI read
  them.
  Tests the following:
  - TOML and INI as made, and TOML read as INI
  - each setting a reading follows: comments, arrays' bounds and
    separator, the value delimiter, quotes, layers, the parent separator,
    duplicate fields, a section and an index, read back
  - files read, TOML and INI
*/
bool
d_tests_arg_cpp_config_read()
{
    return steps_hold("READ",
                      READ_STEPS,
                      READ_COUNT,
                      READ);
}

/*
d_tests_arg_cpp_config_written
  A command's values written, as CLI11's config_to_str writes them.
  Tests the following:
  - TOML and INI, in each mode, with descriptions
  - each setting a writing follows: comments, arrays' bounds and
    separator, the value delimiter, quotes, the parent separator, defaults
    commented
  - a subcommand configurable, a section of its own
*/
bool
d_tests_arg_cpp_config_written()
{
    return steps_hold("WRITTEN",
                      WRITTEN_STEPS,
                      WRITTEN_COUNT,
                      WRITTEN);
}

/*
d_tests_arg_cpp_config_parsed
  A format given to a command, which its parse reads with, as CLI11's
  config_formatter.
  Tests the following:
  - an INI file named on the command line; a default file
  - one section of a file; text from a stream, delimited otherwise
*/
bool
d_tests_arg_cpp_config_parsed()
{
    return steps_hold("PARSED",
                      PARSED_STEPS,
                      PARSED_COUNT,
                      PARSED);
}

/*
d_tests_arg_cpp_config_replaced
  Operations replaced by classes deriving from cli::config_format, as
  CLI11's subclasses of its ConfigBase replace them.
  Tests the following:
  - writing framed, around the root and each subcommand
  - reading a syntax of the format's own, into items and for a parse
  - a flag's words of the format's own, the root's and a subcommand's
*/
bool
d_tests_arg_cpp_config_replaced()
{
    return steps_hold("REPLACED",
                      REPLACED_STEPS,
                      REPLACED_COUNT,
                      REPLACED);
}

/*
d_tests_arg_cpp_config_copies
  Formats copied, as CLI11's ConfigBase is.
  Tests the following:
  - by construction and assignment, settings copied, each changed apart
  - an INI format, and one whose writing is replaced, copied as formats
*/
bool
d_tests_arg_cpp_config_copies()
{
    return steps_hold("COPIES",
                      COPIES_STEPS,
                      COPIES_COUNT,
                      COPIES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 4.  THE FACE'S OWN
//==============================================================================

/*
d_tests_arg_cpp_config_settings
  A format's settings, read back, which CLI11's keeps to itself.
  Tests the following:
  - TOML's and INI's as made
  - each changed in a chain, and changed back; a copy's
*/
bool
d_tests_arg_cpp_config_settings()
{
    return steps_hold("SETTINGS",
                      SETTINGS_STEPS,
                      SETTINGS_COUNT,
                      SETTINGS);
}

/*
d_tests_arg_cpp_config_items
  Items built and adopted, as a format of the caller's builds them.
  Tests the following:
  - an item made, built, viewed; NUL bytes in its name and values kept
  - adopted by a list, the item left empty; the list viewed and cleared
  - a view of none
*/
bool
d_tests_arg_cpp_config_items()
{
    return steps_hold("ITEMS",
                      ITEMS_STEPS,
                      ITEMS_COUNT,
                      ITEMS);
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/*
d_tests_arg_cpp_config_failed
  Reads that fail, and operations of the caller's that fail, by status.
  Tests the following:
  - a malformed escape: ParseError, its description, the items before it
  - a file not there: FileError
  - a format that refuses: a parse's file and stream, a direct read, and
    the command written through it
*/
bool
d_tests_arg_cpp_config_failed()
{
    return steps_hold("FAILED",
                      FAILED_STEPS,
                      FAILED_COUNT,
                      FAILED);
}

//==============================================================================
// 5.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_cpp_config_departures
  Where the face's formats depart from CLI11's (ISSUES 18, 58).
  Tests the following:
  - layers past 255 and below 0 held at the nearer end, an index past
    int16_t kept
  - a subcommand made before the root's format is given follows it
*/
bool
d_tests_arg_cpp_config_departures()
{
    return steps_hold("DEPARTED",
                      DEPARTED_STEPS,
                      DEPARTED_COUNT,
                      DEPARTED);
}

#else

//==============================================================================
// 5.  WITHOUT CONFIGURATION FILES
//==============================================================================

/*
d_tests_arg_cpp_config_unbuilt
  What a format and a configuration option come to without configuration
  files built in.
  Tests the following:
  - reading and writing unsupported; a flag read
  - a file named that is there refused; one not there, and a default not
    there, as with them
*/
bool
d_tests_arg_cpp_config_unbuilt()
{
    return steps_hold("UNBUILT",
                      UNBUILT_STEPS,
                      UNBUILT_COUNT,
                      UNBUILT);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 6.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_config
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_config[] =
{
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "cpp_config_read",
      d_tests_arg_cpp_config_read },
    { "cpp_config_written",
      d_tests_arg_cpp_config_written },
    { "cpp_config_parsed",
      d_tests_arg_cpp_config_parsed },
    { "cpp_config_replaced",
      d_tests_arg_cpp_config_replaced },
    { "cpp_config_copies",
      d_tests_arg_cpp_config_copies },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
    { "cpp_config_settings",
      d_tests_arg_cpp_config_settings },
    { "cpp_config_items",
      d_tests_arg_cpp_config_items },
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "cpp_config_failed",
      d_tests_arg_cpp_config_failed },
    { "cpp_config_departures",
      d_tests_arg_cpp_config_departures }
#else
    { "cpp_config_unbuilt",
      d_tests_arg_cpp_config_unbuilt }
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
};

// d_tests_arg_cpp_config_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_config_count =
    sizeof(d_tests_arg_cpp_config) / sizeof(d_tests_arg_cpp_config[0]);
