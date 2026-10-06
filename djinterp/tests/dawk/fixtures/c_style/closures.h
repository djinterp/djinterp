/* closures.h: Conditional-Block Closures, in a header */
#ifndef D_CLOSURES_GUARD
#define D_CLOSURES_GUARD 1

// negative: named
#ifdef D_CLOSURES_A
    #define D_CLOSURES_A_ON 1
#endif  // D_CLOSURES_A

// positive: no comment at all
#ifndef D_CLOSURES_B
    #define D_CLOSURES_B 1
#endif

// positive: a comment naming the wrong symbol
#ifdef D_CLOSURES_C
    #define D_CLOSURES_C_ON 1
#endif  // D_CLOSURES_CC

// negative: an expression #if needs no comment
#if defined(D_CLOSURES_D) && (D_CLOSURES_D > 1)
    #define D_CLOSURES_D_ON 1
#endif

// negative: nesting pairs each #endif with its own opener
#ifdef D_CLOSURES_E
    #ifndef D_CLOSURES_F
        #define D_CLOSURES_F 1
    #endif  // D_CLOSURES_F
#endif  // D_CLOSURES_E


#endif  // D_CLOSURES_GUARD
