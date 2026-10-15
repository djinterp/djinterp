/*******************************************************************************
* bitwisedom [bitwisedom]                                           examples.hpp
*
* Built-in example sheets, for the File menu and the tests.
*   Eighteen words of one to eight bytes, dense and sparse, signed and not:
* registers and hardware, formats and encodings, and the flag words and
* codes of POSIX, Windows, macOS, Linux and FreeBSD -- the last four with
* the values of the headers they come from, which the tests compare against
* those headers wherever the build has them. A PCI function's configuration
* header, 512 bits, wider than any integer. And two large ones, kept as
* JSON in examples/large and built in: every HRESULT winerror.h defines,
* 1,376 codes, and ten of CPUID's feature registers, 320 bits.
*
* path:      /inc/djinterp/tools/bitwisedom/examples.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

#ifndef BITWISEDOM_EXAMPLES_HPP
#define BITWISEDOM_EXAMPLES_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./sheet.hpp"  // sheet


namespace bitwisedom
{

// example_info
//   struct: one built-in example: its name, the title the File menu shows,
// the shelf it sits on there, and its width in bits.
struct example_info
{
    std::string name;
    std::string title;
    std::string shelf;
    int         width = 0;
};

// examples -- every one, in order; the names alone; and a sheet by name
// (an unknown name gives the first)
D_NODISCARD const std::vector<example_info>&  example_list();
D_NODISCARD std::vector<std::string>          example_names();
D_NODISCARD sheet                             make_example(
                                                  const std::string& _name);

}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_EXAMPLES_HPP
