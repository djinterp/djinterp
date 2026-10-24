/*******************************************************************************
* bitwisedom [bitwisedom]                                            version.hpp
*
* The version every part of bitwisedom is built at.
*   One string, so that `bitwisedom --version` and the window's module agree:
* the command line loads a module only of its own program and version
* (GRAMMAR.md, section 7). The build passes the project's version as
* BITWISEDOM_VERSION; a build that does not gets the one below.
*
* path:      /inc/djinterp/tools/bitwisedom/version.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE VERSION
    -----------
    1.  BITWISEDOM_VERSION
    2.  BITWISEDOM_PROGRAM
*/

#ifndef BITWISEDOM_VERSION_HPP
#define BITWISEDOM_VERSION_HPP 1


//==============================================================================
// 1.  THE VERSION
//==============================================================================


// 1.1.1
// BITWISEDOM_VERSION
//   constant: the version, as `--version` prints it after the program's
// name; the build's project version where the build defines it.
#ifndef BITWISEDOM_VERSION
    #define BITWISEDOM_VERSION "0.1.0"
#endif  // BITWISEDOM_VERSION

// 1.1.2
// BITWISEDOM_PROGRAM
//   constant: the program's own name, as diagnostics begin and a module of
// the program names the program it belongs to.
#define BITWISEDOM_PROGRAM "bitwisedom"


#endif  // BITWISEDOM_VERSION_HPP
