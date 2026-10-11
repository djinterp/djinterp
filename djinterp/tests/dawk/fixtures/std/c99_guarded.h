/* c99_guarded.h: the include guard is no conditional */
#ifndef D_C99_GUARDED_FIXTURE
#define D_C99_GUARDED_FIXTURE 1

#include <threads.h>

_Thread_local int d_counter;

#if D_ENV_LANG_IS_C11_OR_HIGHER
#include <stdatomic.h>
_Atomic int d_shared;
#endif

#endif  // D_C99_GUARDED_FIXTURE
