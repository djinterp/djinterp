/* static_assert.c: Assertions */
#include <assert.h>  // static_assert

static_assert(sizeof(int) >= 4, "int holds 32 bits");  // negative
static_assert(sizeof(long) >= 4);                         // positive
_Static_assert(sizeof(short) >= 2, "short holds 16");    // positive: spelling
