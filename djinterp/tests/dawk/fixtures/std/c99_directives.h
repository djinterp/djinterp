/* c99_directives.h: a header of directives alone has its guard excluded too */
#ifndef D_C99_DIRECTIVES_FIXTURE
#define D_C99_DIRECTIVES_FIXTURE 1

#include <uchar.h>

#if D_ENV_LANG_IS_C11_OR_HIGHER
#   include <stdalign.h>
#endif

#endif  // D_C99_DIRECTIVES_FIXTURE
