/* conditional_includes.c: two empty lines after the includes' own #endif */
#include "./conditional_includes.h"  // corresponding header
// std
#include <stddef.h>  // size_t
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #include <io.h>  // _findfirst
#else
    #include <dirent.h>  // opendir
#endif


// d_first
//   variable: negative above: the #endif closes the includes.
int d_first = 0;

#if D_FIXTURE_MORE
// d_second
//   variable: under a conditional that holds no include.
int d_second = 0;
#endif


// d_third
//   variable: positive above: this #endif closes something else.
int d_third = 0;
