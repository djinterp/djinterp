/*******************************************************************************
* djinterp [config]                                         cfg_mongodb_common.h
*
* Build-time configuration shared by every module of djinterp's MongoDB layer
* (c/db/mongodb/mongodb_*.h).
*   One knob decides what the layer is built over: MongoDB's C driver,
* libmongoc with libbson -- either of its lines, 1.x from 1.21 or 2.x, whose
* headers are both reached as <mongoc/mongoc.h> through the directory each
* line's own flags name (pkg-config's libmongoc-1.0 or mongoc2) -- or stubs
* that report D_DB_UNSUPPORTED, so that code using the layer compiles and
* links where no driver is installed and decides at run time, as the other
* layers do. It defaults to whether the driver's header can be found.
*   Built over the driver, the layer's sources reach mongoc.h through
* env_mongodb.h, which includes it only when D_CFG_ENV_USING_MONGODB is 1.
* This file sets that knob for the layer's own sources -- each defines
* D_INTERNAL_MONGODB_SOURCE before its first include -- unless the build
* already has, and only then includes env's configuration, so that env's
* cascade honours it (the config README, 6.2). Nothing else that includes the
* layer meets the driver's names: not the public headers, which name only
* the layer's own types, and not core/db.
*   The driver's headers name struct addrinfo, which glibc and musl declare
* in a strict ISO mode only with POSIX asked for: a build at -std=c99 defines
* _XOPEN_SOURCE=700, as the style guide has every build do.
*
*   targets:  c/db/mongodb/mongodb_*.h and their sources ->
*             D_INTERNAL_MONGODB, D_INTERNAL_MONGODB_DOCUMENT_FORMAT,
*             D_INTERNAL_MONGODB_LOG_LEVEL, D_MONGODB_FLOOR_*
*   requires: cfg_common.h, cfg_env_mongodb.h
*
*
* path:      /inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BINDING
    -------
    1.  Environment-defaulted enable
         1.  D_CFG_MONGODB
2.  KNOBS
    -----
    1.  Documents
         1.  D_CFG_MONGODB_DOCUMENT_FORMAT
    2.  The driver's log
         1.  D_CFG_MONGODB_LOG_LEVEL
3.  DEPENDENCIES
    ------------
    1.  Driver detection
         1.  D_CFG_ENV_USING_MONGODB
4.  VALIDATION
    ----------
    1.  Knob validation
5.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_MONGODB
         2.  D_INTERNAL_MONGODB_DOCUMENT_FORMAT
         3.  D_INTERNAL_MONGODB_LOG_LEVEL
6.  QUERIES
    -------
    1.  Floors
         1.  D_MONGODB_FLOOR_DRIVER_VERSION
         2.  D_MONGODB_FLOOR_SERVER_VERSION
*/

#ifndef DJINTERP_CONFIG_C_DB_MONGODB_CFG_MONGODB_COMMON_H
#define DJINTERP_CONFIG_C_DB_MONGODB_CFG_MONGODB_COMMON_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_IS_ON, D_CFG_IS_BOOL, user overrides


//==============================================================================
// 1.  BINDING
//==============================================================================
// Whether the layer runs over the driver. Without it every function reports
// D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail.


// 1.1    Environment-defaulted enable
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_MONGODB
//   knob: build the layer over MongoDB's C driver (1) or as stubs (0).
// Defaults to 1 where the driver's header can be found where env looks -- at
// D_CFG_ENV_MONGODB_C_PATH when the build names one, else as
// <mongoc/mongoc.h>, the path both of the driver's lines install -- and to 0
// where it cannot; to 1 where the preprocessor has no __has_include to look
// with. A path the build names but cannot be found leaves the stubs. Define
// it as 1 to require the driver, which stops a build that lacks it with an
// #error, or as 0 to build the stubs whatever is installed.
#ifndef D_CFG_MONGODB
    #if defined(__has_include)
        #if defined(D_CFG_ENV_MONGODB_C_PATH)
            #if __has_include(D_CFG_ENV_MONGODB_C_PATH)
                #define D_CFG_MONGODB 1
            #else
                #define D_CFG_MONGODB 0
            #endif
        #elif __has_include(<mongoc/mongoc.h>)
            #define D_CFG_MONGODB 1
        #else
            #define D_CFG_MONGODB 0
        #endif
    #else
        #define D_CFG_MONGODB 1
    #endif
#endif  // D_CFG_MONGODB


//==============================================================================
// 2.  KNOBS
//==============================================================================


// 2.1    Documents
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_MONGODB_DOCUMENT_FORMAT
//   knob: how a handle the layer makes writes the documents it returns, until
// d_mongodb_set_document_format says otherwise (mongodb_settings.h): 0,
// canonical extended JSON, every value's BSON type written out -- {"n":
// {"$numberLong": "5"}} -- so that a document read and written back keeps
// every type; 1, relaxed extended JSON, numbers as plain JSON -- {"n": 5} --
// readable, but read back an int64 that fits 32 bits becomes an int32 and a
// double without a fraction an integer; 2, the document's BSON itself, as
// bytes. Defaults to 0. The layer reads every extended JSON, canonical,
// relaxed or legacy, whatever this says.
#ifndef D_CFG_MONGODB_DOCUMENT_FORMAT
    #define D_CFG_MONGODB_DOCUMENT_FORMAT 0
#endif  // D_CFG_MONGODB_DOCUMENT_FORMAT

// 2.2    The driver's log
//------------------------------------------------------------------------------
// 2.2.1
// D_CFG_MONGODB_LOG_LEVEL
//   knob: what the driver's log may print. The driver logs through one
// handler for the whole process, whose default prints errors, criticals and
// warnings on stderr, and messages, information and debugging on stdout --
// a failed sign-in among them -- where they mix with a program's own output.
// From 0 to 6, the driver's levels (error, critical, warning, message, info,
// debug, trace): the layer installs a handler of its own the first time it
// makes or wraps a handle, which passes what is at that level or more urgent
// to the driver's default handler and drops the rest. -1 installs nothing,
// leaving the handler to the driver and to the program. Defaults to 2,
// warnings and worse, on stderr. d_mongodb_set_log_level changes it while the
// program runs (mongodb_common.h); a program that sets its own handler with
// mongoc_log_set_handler after that replaces the layer's.
#ifndef D_CFG_MONGODB_LOG_LEVEL
    #define D_CFG_MONGODB_LOG_LEVEL 2
#endif  // D_CFG_MONGODB_LOG_LEVEL


//==============================================================================
// 3.  DEPENDENCIES
//==============================================================================


// 3.1    Driver detection
//------------------------------------------------------------------------------
// 3.1.1
// D_CFG_ENV_USING_MONGODB
//   knob (env's): turned on here for the layer's own sources, when the layer
// is built over the driver, before env's own default can turn it off, so
// that they find mongoc.h. A source says it is one by defining
// D_INTERNAL_MONGODB_SOURCE before its first include; everything else keeps
// env's default. A value the build gave first wins.
#if ( (D_CFG_IS_ON(D_CFG_MONGODB)) &&                                          \
      (defined(D_INTERNAL_MONGODB_SOURCE)) )
    #ifndef D_CFG_ENV_USING_MONGODB
        #define D_CFG_ENV_USING_MONGODB 1
    #endif  // D_CFG_ENV_USING_MONGODB
#endif

// djinterp
#include "../../../env/db/mongodb/cfg_env_mongodb.h"  // env's knobs


//==============================================================================
// 4.  VALIDATION
//==============================================================================


// 4.1    Knob validation
//------------------------------------------------------------------------------
// validation: #if reads the knob as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_MONGODB)
    #error "D_CFG_MONGODB must be 0 or 1"
#endif

// validation: a format the layer has
#if ( ((D_CFG_MONGODB_DOCUMENT_FORMAT + 0) < 0) ||                             \
      ((D_CFG_MONGODB_DOCUMENT_FORMAT + 0) > 2) )
    #error "D_CFG_MONGODB_DOCUMENT_FORMAT must be 0, 1 or 2"
#endif

// validation: one of the driver's levels, or -1
#if ( ((D_CFG_MONGODB_LOG_LEVEL + 0) < -1) ||                                  \
      ((D_CFG_MONGODB_LOG_LEVEL + 0) > 6) )
    #error "D_CFG_MONGODB_LOG_LEVEL must be -1, or from 0 to 6"
#endif


//==============================================================================
// 5.  RESOLVED VALUES
//==============================================================================


// 5.1    Effective values
//------------------------------------------------------------------------------
// 5.1.1
// D_INTERNAL_MONGODB
//   brief: 1 when the layer is built over the driver, 0 when it is built as
// stubs; the sources choose their implementation by it.
#if D_CFG_IS_ON(D_CFG_MONGODB)
    #define D_INTERNAL_MONGODB 1
#else
    #define D_INTERNAL_MONGODB 0
#endif

// 5.1.2
// D_INTERNAL_MONGODB_DOCUMENT_FORMAT
//   brief: D_CFG_MONGODB_DOCUMENT_FORMAT, which the sources read through this
// name: a new handle's enum dj_mongodb_document_format.
#define D_INTERNAL_MONGODB_DOCUMENT_FORMAT                                     \
    (D_CFG_MONGODB_DOCUMENT_FORMAT + 0)

// 5.1.3
// D_INTERNAL_MONGODB_LOG_LEVEL
//   brief: D_CFG_MONGODB_LOG_LEVEL, which the sources read through this name.
#define D_INTERNAL_MONGODB_LOG_LEVEL (D_CFG_MONGODB_LOG_LEVEL + 0)


//==============================================================================
// 6.  QUERIES
//==============================================================================


// 6.1    Floors
//------------------------------------------------------------------------------
// Versions are MAJOR*10000 + MINOR*100 + PATCH, as env_mongodb.h encodes
// them. Safe in #if.

// 6.1.1
// D_MONGODB_FLOOR_DRIVER_VERSION
//   query: the oldest driver the layer builds against, 1.21.0, the one Ubuntu
// 22.04 ships; the sources stop with an #error below it.
#define D_MONGODB_FLOOR_DRIVER_VERSION 12100

// 6.1.2
// D_MONGODB_FLOOR_SERVER_VERSION
//   query: the oldest server the layer is written and tested for, 4.2, the
// oldest the driver's 2.x line connects to.
#define D_MONGODB_FLOOR_SERVER_VERSION 40200


#endif  // DJINTERP_CONFIG_C_DB_MONGODB_CFG_MONGODB_COMMON_H
