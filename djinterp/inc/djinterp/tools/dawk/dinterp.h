/******************************************************************************
* djinterp [dawk]                                                    dinterp.h
*
*   Tree-walking interpreter for the syntax tree in dparse.h.
*     Fields are split lazily: a record is not divided until a field or NF is
* read, and $0 is not rebuilt until a field is written. A program that only
* prints whole records therefore never splits one.
*     Values are owned by their container and copied on assignment, which is
* what awk's scalar semantics already are. Arrays pass by reference, and only
* through function parameters.
*
*
* path:      /inc/djinterp/tools/dawk/dinterp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  d_awk_interp
2.  OPERATIONS
    ----------
    1.  Lifecycle
    2.  Configuration
    3.  Execution
    4.  Diagnostics
*/

#ifndef DJINTERP_TOOLS_DAWK_DINTERP_H
#define DJINTERP_TOOLS_DAWK_DINTERP_H 1

// std
#include <stdbool.h>   // bool
#include <stddef.h>    // size_t
// djinterp
#include "./dparse.h"  // d_awk_program


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Interpreter
//------------------------------------------------------------------------------
// 1.1.1
// d_awk_interp
//   struct: one interpreter over one parsed program.  Layout is private.
struct d_awk_interp;
struct d_awk_source;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifecycle
//------------------------------------------------------------------------------
struct d_awk_interp* d_awk_interp_new(struct d_awk_program* _program);
void                 d_awk_interp_free(struct d_awk_interp* _interp);

// 2.2    Configuration
//------------------------------------------------------------------------------
// An assignment given with -v, or as a file operand of the form name=value,
// is input-derived text and therefore obeys the numeric string rule.
bool                 d_awk_interp_assign(struct d_awk_interp* _interp,
                                         const char*          _text);
bool                 d_awk_interp_add_input(struct d_awk_interp* _interp,
                                            const char*          _path);
bool                 d_awk_interp_set_environ(struct d_awk_interp* _interp,
                                              char* const*         _environ);

bool                 d_awk_interp_set_source(struct d_awk_interp* _interp,
                                             struct d_awk_source* _source);

// 2.3    Execution
//------------------------------------------------------------------------------
int                  d_awk_interp_run(struct d_awk_interp* _interp);

// 2.4    Diagnostics
//------------------------------------------------------------------------------
const char*          d_awk_interp_error(const struct d_awk_interp* _interp);


#endif  // DJINTERP_TOOLS_DAWK_DINTERP_H
