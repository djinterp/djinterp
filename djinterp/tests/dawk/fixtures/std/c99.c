/* c99.c: the C floor -- nothing newer than C99 unless a conditional guards it */
#include <stdalign.h>
#include <stdbool.h>

#if D_ENV_LANG_IS_C11_OR_HIGHER
#include <stdnoreturn.h>
#endif

_Alignas(16) int d_aligned;
static inline int d_inline(void);
bool d_flag = true;

#if D_ENV_LANG_IS_C11_OR_HIGHER
_Noreturn void d_fatal(void);
#else
void d_fatal(void);
#endif

// D_MY_ALIGNAS
//   macro: negative -- a directive may spell the newer keyword.
#define D_MY_ALIGNAS _Alignas
