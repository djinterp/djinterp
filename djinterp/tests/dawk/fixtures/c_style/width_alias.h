/* width_alias.h: the one lower-case macro, a width alias (Macro Names) */
#ifndef D_WIDTH_ALIAS_FIXTURE
#define D_WIDTH_ALIAS_FIXTURE 1

// negative: object-like aliases naming a twin, at either width
// d_stat_t
#if (D_FILE_WIDTH == 64)
    #define d_stat_t    d_stat64_t
    #define d_file_stat d_file_stat64
#else
    #define d_stat_t    d_stat32_t
    #define d_file_stat d_file_stat32
#endif  // D_FILE_WIDTH == 64

// positive: lower-case, but naming no twin
// d_not_an_alias
#define d_not_an_alias d_something_else

// positive: function-like, however it is spelled
// d_function_like
#define d_function_like(_x) d_function_like64(_x)


#endif  // D_WIDTH_ALIAS_FIXTURE
