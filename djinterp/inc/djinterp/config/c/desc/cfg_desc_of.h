/*******************************************************************************
* djinterp [c][desc]                                               cfg_desc_of.h
*
* Configuration for desc's describers of other modules' types, the library
* djinterp_c_desc_of (desc's decision 82): a unit and a header a module,
* above the modules they describe. Each knob is a default that a build
* overrides with -D. A knob that leaves describers out leaves their code and
* their text out of the library; no run-time option says whether a
* describer exists, and a run chooses only how a description is written
* (desc.form, desc.format).
*   targets:  c/desc/of/desc_of.h, c/desc/of/mem_desc.h,
*             c/desc/of/filesys_desc.h, c/desc/of/test_desc.h,
*             cpp/desc/of/desc_of.hpp and each module's .hpp ->
*                 D_CFG_DESC_OF, D_CFG_DESC_OF_MEMORY,
*                 D_CFG_DESC_OF_FILESYS, D_CFG_DESC_OF_TEST
*   requires: cfg_common.h, cfg_desc.h
*
* HOW TO USE
* ----------
*   Set a knob with -D before any djinterp header; each header in the
* targets above includes this one.
*
* OPTIONS
* -------
*   None: whether a describer exists is never chosen at run time.
*
* CONFIGURATION
* -------------
*   D_CFG_DESC_OF, D_CFG_DESC_OF_MEMORY, D_CFG_DESC_OF_FILESYS and
* D_CFG_DESC_OF_TEST, below.
*
*
* path:      /inc/djinterp/config/c/desc/cfg_desc_of.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  D_CFG_DESC_OF
    2.  D_CFG_DESC_OF_MEMORY
    3.  D_CFG_DESC_OF_FILESYS
    4.  D_CFG_DESC_OF_TEST
2.  VALIDATION
    ----------
    1.  Ranges
3.  DERIVED
    -------
    1.  D_INTERNAL_DESC_OF_MEMORY
    2.  D_INTERNAL_DESC_OF_FILESYS
    3.  D_INTERNAL_DESC_OF_TEST
*/

#ifndef DJINTERP_CONFIG_C_DESC_CFG_DESC_OF_H
#define DJINTERP_CONFIG_C_DESC_CFG_DESC_OF_H 1

// djinterp
#include "../../cfg_common.h"  // user overrides
#include "./cfg_desc.h"        // D_CFG_DESC_ENABLED


//==============================================================================
// 1.  KNOBS
//==============================================================================

// 1.1
// D_CFG_DESC_OF
//   configuration: 1 builds desc's describers of other modules' types, the
// library djinterp_c_desc_of; 0 builds it empty, every module's describers
// with it. Nothing is built where desc is not (D_CFG_DESC_ENABLED 0). 1 by
// default.
#ifndef D_CFG_DESC_OF
    #define D_CFG_DESC_OF 1
#endif

// 1.2
// D_CFG_DESC_OF_MEMORY
//   configuration: 0 leaves memory's describers out of the library (the
// arena, the pool, a source, a block and the allocation tally), and their
// C++ units with them. Memory's headers are all they need, so they are
// built wherever the library is. 1 by default.
#ifndef D_CFG_DESC_OF_MEMORY
    #define D_CFG_DESC_OF_MEMORY 1
#endif

// 1.3
// D_CFG_DESC_OF_FILESYS
//   configuration: 0 leaves filesys's describers out of the library (a
// file's status, a directory entry and a filesys notice, at each width
// filesys builds), and their C++ units with them. filesys's headers are
// all they need. 1 by default.
#ifndef D_CFG_DESC_OF_FILESYS
    #define D_CFG_DESC_OF_FILESYS 1
#endif

// 1.4
// D_CFG_DESC_OF_TEST
//   configuration: 0 leaves DTest's describers out of the library (a
// run's summary and options, and a module's, a block's and a test's
// spec), and their C++ units with them. DTest's C header is all they
// need. 1 by default.
#ifndef D_CFG_DESC_OF_TEST
    #define D_CFG_DESC_OF_TEST 1
#endif


//==============================================================================
// 2.  VALIDATION
//==============================================================================

// 2.1    Ranges
//------------------------------------------------------------------------------

#if ( (D_CFG_DESC_OF < 0) || (D_CFG_DESC_OF > 1)                 ||         \
      (D_CFG_DESC_OF_MEMORY < 0) || (D_CFG_DESC_OF_MEMORY > 1)   ||         \
      (D_CFG_DESC_OF_FILESYS < 0) || (D_CFG_DESC_OF_FILESYS > 1) ||         \
      (D_CFG_DESC_OF_TEST < 0) || (D_CFG_DESC_OF_TEST > 1) )
    #error "D_CFG_DESC_OF and each D_CFG_DESC_OF_<MODULE> are 0 or 1"
#endif


//==============================================================================
// 3.  DERIVED
//==============================================================================

// 3.1
// D_INTERNAL_DESC_OF_MEMORY
//   constant: 1 when memory's describers are built: desc is, the library
// is, and memory's knob keeps them; else 0.
#if ( D_CFG_DESC_ENABLED && D_CFG_DESC_OF && D_CFG_DESC_OF_MEMORY )
    #define D_INTERNAL_DESC_OF_MEMORY 1
#else
    #define D_INTERNAL_DESC_OF_MEMORY 0
#endif

// 3.2
// D_INTERNAL_DESC_OF_FILESYS
//   constant: 1 when filesys's describers are built; else 0.
#if ( D_CFG_DESC_ENABLED && D_CFG_DESC_OF && D_CFG_DESC_OF_FILESYS )
    #define D_INTERNAL_DESC_OF_FILESYS 1
#else
    #define D_INTERNAL_DESC_OF_FILESYS 0
#endif

// 3.3
// D_INTERNAL_DESC_OF_TEST
//   constant: 1 when DTest's describers are built; else 0.
#if ( D_CFG_DESC_ENABLED && D_CFG_DESC_OF && D_CFG_DESC_OF_TEST )
    #define D_INTERNAL_DESC_OF_TEST 1
#else
    #define D_INTERNAL_DESC_OF_TEST 0
#endif


#endif  // DJINTERP_CONFIG_C_DESC_CFG_DESC_OF_H
