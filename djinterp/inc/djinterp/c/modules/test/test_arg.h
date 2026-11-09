/*******************************************************************************
* djinterp [test]                                                     test_arg.h
*
* A key-value argument, and a list of them, for the C test framework's older
* forms.
*   The classic tree's *_new_args constructors (default/test.h and its kin) and
* test_printer take one (the retired standalone object carried one too). They
* used the two structs and none declared them: the rewrite of 2026.08.08
* dropped them from test_common.h, which keeps only the status and identifier
* vocabulary, so they are declared here, once, in the shape they had.
*
*
* path:      /inc/djinterp/c/modules/test/test_arg.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.07
*******************************************************************************/

#ifndef DJINTERP_C_TEST_ARG_H
#define DJINTERP_C_TEST_ARG_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"  // framework root


D_EXTERN_C_BEGIN


// dj_test_arg
//   struct: one argument: a key, and a value the key's reader interprets.
struct dj_test_arg
{
    const char* key;    // the argument's name
    void*       value;  // its value; not owned by the argument
};

// dj_test_arg_list
//   struct: the arguments a node was made with.
struct dj_test_arg_list
{
    size_t              count;  // arguments in args
    struct dj_test_arg* args;   // the arguments; owned by the list's holder
};


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_ARG_H
