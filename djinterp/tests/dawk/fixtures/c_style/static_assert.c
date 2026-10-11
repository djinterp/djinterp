/* static_assert.c: Assertions */
#include <assert.h>  // static_assert

static_assert(sizeof(int) >= 4, "int holds 32 bits");  // positive: spelling
static_assert(sizeof(long) >= 4);                         // positive: both
_Static_assert(sizeof(short) >= 2, "short holds 16");    // positive: spelling
D_STATIC_ASSERT(sizeof(int) >= 4, "int holds 32 bits");  // negative
D_STATIC_ASSERT(sizeof(long) >= 4);                       // positive: message

// D_MY_ASSERT
//   macro: negative -- a directive may spell the standard's own.
#define D_MY_ASSERT _Static_assert
