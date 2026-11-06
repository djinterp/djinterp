/*******************************************************************************
* djinterp [c]                                                        dversion.h
*
* The framework's version.
*   One place states it: the release this tree is working toward, as three
* numbers and a pre-release label, which semantic versioning orders before
* the release it names: 0.8.0-dev comes before 0.8.0-rc.1, which comes
* before 0.8.0. The build reads it from here too -- the CMake project takes
* its VERSION from these lines -- so a release changes this file and nothing
* else names a number.
*   D_VERSION_NUMBER puts the three numbers in one integer that #if can
* compare, and D_VERSION_AT_LEAST compares it with a version of the caller's:
*         #if D_VERSION_AT_LEAST(0, 9, 0)
*   targets:  any unit that needs to know which djinterp it is built
*             against, which includes this header itself (the framework root
*             does not: a file includes what it uses)
*   requires: nothing (it defines only macros)
*
*
* path:      /inc/djinterp/c/dversion.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE VERSION
    -----------
    1.  Its parts
         1.  D_VERSION_MAJOR
         2.  D_VERSION_MINOR
         3.  D_VERSION_PATCH
         4.  D_VERSION_PRERELEASE
         5.  D_VERSION_IS_RELEASE
    2.  Its forms
         1.  D_VERSION_STRING
         2.  D_VERSION_NUMBER
         3.  D_VERSION_AT_LEAST
*/

#ifndef DJINTERP_C_DVERSION_H
#define DJINTERP_C_DVERSION_H 1


//==============================================================================
// 1.  THE VERSION
//==============================================================================
//   The release process changes the five parts and the string together;
// scripts/ci/check_version.py fails a build where they disagree.


// 1.1    Its parts
//------------------------------------------------------------------------------
// 1.1.1
// D_VERSION_MAJOR
//   constant: the major version: 0 until 1.0.0, then a change that breaks
// the stable API.
#define D_VERSION_MAJOR        0

// 1.1.2
// D_VERSION_MINOR
//   constant: the minor version: a release that adds to the API.
#define D_VERSION_MINOR        8

// 1.1.3
// D_VERSION_PATCH
//   constant: the patch version: a release that only fixes.
#define D_VERSION_PATCH        0

// 1.1.4
// D_VERSION_PRERELEASE
//   constant: the pre-release label, as a string: "dev" while the release is
// being built, "rc.1" and so on for its candidates, and "" for the release.
#define D_VERSION_PRERELEASE   "dev"

// 1.1.5
// D_VERSION_IS_RELEASE
//   constant: 1 for a release, 0 while the label is not empty, so that #if
// can tell the two apart.
#define D_VERSION_IS_RELEASE   0


// 1.2    Its forms
//------------------------------------------------------------------------------
// 1.2.1
// D_VERSION_STRING
//   constant: the version as semantic versioning spells it: the three
// numbers, and the label after a hyphen where there is one.
#define D_VERSION_STRING       "0.8.0-dev"

// 1.2.2
// D_VERSION_NUMBER
//   constant: the three numbers in one integer, major * 1000000 + minor *
// 1000 + patch (8000 for 0.8.0), long so that it fits where int has 16 bits.
// It leaves the label out: 0.8.0-dev and 0.8.0 compare equal.
#define D_VERSION_NUMBER                                                       \
    ( ((D_VERSION_MAJOR) * 1000000L) +                                         \
      ((D_VERSION_MINOR) * 1000L)    +                                         \
      (D_VERSION_PATCH) )

// 1.2.3
// D_VERSION_AT_LEAST
//   macro: 1 when this version's numbers are at least the ones given, 0
// otherwise; usable in #if.
#define D_VERSION_AT_LEAST(major, minor, patch)                                \
    ( (D_VERSION_NUMBER) >= ( ((major) * 1000000L) +                           \
                              ((minor) * 1000L)    +                           \
                              (patch) ) )


#endif  // DJINTERP_C_DVERSION_H
