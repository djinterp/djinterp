/* endif_includes.c: one empty line after the includes' own #endif */
#include "./endif_includes.h"  // corresponding header
// std
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #include <io.h>  // _findfirst
#endif

// d_value
//   variable: positive above: the #endif closes the includes, and one
// empty line follows it.
int d_value = 0;
