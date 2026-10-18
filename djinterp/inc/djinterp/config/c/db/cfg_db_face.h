/*******************************************************************************
* djinterp [config]                                                cfg_db_face.h
*
* Build-time configuration for the database layers' C++ faces, c/db/db_face.hpp
* and each layer's <vendor>_face.hpp.
*   From C++20 a face reads a literal format before the layer does: each
* conversion is matched against the argument it consumes, and a mismatch, a
* missing argument or one too many stops the compile. A format built at run
* time then goes through face::runtime_format, which the check leaves alone.
* The knob turns the check off, for a build that passes char pointers built
* at run time as they stand; the faces' members then take any C string as a
* format, as they do below C++20. A std::string goes through runtime_format
* either way, at every level.
*
*   targets:  c/db/db_face.hpp -> D_INTERNAL_DB_FACE_FORMAT_CHECK
*   requires: nothing; the knob is read only by C++, from C++20
*
*
* path:      /inc/djinterp/config/c/db/cfg_db_face.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Formats
         1.  D_CFG_DB_FACE_FORMAT_CHECK
2.  VALIDATION
    ----------
    1.  Knob validation
*/

#ifndef DJINTERP_CONFIG_C_DB_CFG_DB_FACE_H
#define DJINTERP_CONFIG_C_DB_CFG_DB_FACE_H 1


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Formats
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_DB_FACE_FORMAT_CHECK
//   knob: 1 to have the faces check a literal format against its arguments at
// compile time, where the level and the compiler can (C++20, consteval); 0 to
// take every C string as a format, unchecked. Defaults to 1.
#ifndef D_CFG_DB_FACE_FORMAT_CHECK
    #define D_CFG_DB_FACE_FORMAT_CHECK 1
#endif  // D_CFG_DB_FACE_FORMAT_CHECK


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
// validation: the knob is a switch
#if ( ((D_CFG_DB_FACE_FORMAT_CHECK + 0) != 0) &&                              \
      ((D_CFG_DB_FACE_FORMAT_CHECK + 0) != 1) )
    #error "D_CFG_DB_FACE_FORMAT_CHECK must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_C_DB_CFG_DB_FACE_H
