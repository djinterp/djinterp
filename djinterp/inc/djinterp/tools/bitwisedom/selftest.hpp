/*******************************************************************************
* bitwisedom [bitwisedom]                                           selftest.hpp
*
* A header's self-test: a C or C++ file that proves the header right.
*   It includes the header, and compiles only if every macro has the value
* the sheet gives it, and every range's GET and SET templates agree with
* its named values. A sheet wider than an integer keeps its bits in arrays,
* which no compile-time check reads: its self-test checks them as it runs,
* and returns 0 only if each holds -- every bit set, tested and cleared
* through the array macros, every field written and read back. The build's
* tests compile one at every language level, and run a wide sheet's, which
* is how the generator is proved rather than merely run.
*   `bitwisedom gen --self-test` and `bitwisedom-gen --self-test` write it.
*
* path:      /inc/djinterp/tools/bitwisedom/selftest.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SELF-TESTS
    ----------
    1.  self_test
*/

#ifndef BITWISEDOM_SELFTEST_HPP
#define BITWISEDOM_SELFTEST_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./resolve.hpp"  // resolution
#include "./sheet.hpp"    // sheet


namespace bitwisedom
{


//==============================================================================
// 1.  SELF-TESTS
//==============================================================================


// 1.1.1
// self_test
/**
 * @brief Writes a header's self-test.
 *
 * @note A sheet wider than an integer gets the self-test that runs, and
 *       any other the one that only compiles. The file is C for a C header
 *       and C++ for a C++ one, at the sheet's language level, and names the
 *       header by `_header`, as an #include does.
 *
 * @param[in] _sheet       the sheet, its generator settings those the
 *                         header was written with.
 * @param[in] _resolution  its resolution.
 * @param[in] _header      the header's file name, as the self-test
 *                         includes it.
 * @return the self-test's text.
 */
D_NODISCARD std::string  self_test(const sheet&       _sheet,
                                   const resolution&  _resolution,
                                   const std::string& _header);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_SELFTEST_HPP
