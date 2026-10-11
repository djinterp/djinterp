/* fix.h: an #endif with no name, and three a repair leaves alone */
#ifndef D_FIX_FIXTURE
#define D_FIX_FIXTURE 1

#ifdef D_FIX_EXTRA
int d_fix_extra(int _a);
#endif  // D_FIX_EXTRA

#ifdef D_FIX_NAMED
int d_fix_named(int _a);
#endif  // D_FIX_NAMED

#ifdef D_FIX_WORDS
int d_fix_words(int _a);
#endif  /* the end */

#ifdef D_FIX_A_SYMBOL_SO_LONG_THAT_ITS_NAME_AFTER_ITS_ENDIF_WOULD_PASS_THE_LIMIT
int d_fix_long(int _a);
#endif

#endif  // D_FIX_FIXTURE
