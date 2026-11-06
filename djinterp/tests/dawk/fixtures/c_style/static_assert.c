/* static_assert.c: Assertions -- D_STATIC_ASSERT, in both languages */
#include <assert.h>  // static_assert

static_assert(sizeof(int) >= 4, "int holds 32 bits");    // positive: spelling
static_assert(sizeof(long) >= 4);                         // positive: twice
_Static_assert(sizeof(short) >= 2, "short holds 16");    // positive: spelling
D_STATIC_ASSERT(sizeof(char) == 1, "a char is a byte");  // negative
D_STATIC_ASSERT(sizeof(char) == 1);                       // positive: message
