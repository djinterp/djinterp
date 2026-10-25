/* macros.h: brief comments, iterative blocks, fallbacks, function-like */
#ifndef D_MACROS_FIXTURE
#define D_MACROS_FIXTURE 1


// 1.1    A subsection
//------------------------------------------------------------------------------

// D_MACROS_UNDOCUMENTED_NEIGHBOUR
//   positive above: the delimiter is followed by an empty line.
#define D_MACROS_UNDOCUMENTED_NEIGHBOUR 1

#define D_MACROS_BARE 2

// D_MACROS_BLOCK<1-3>
//   macro: one comment covers an iterative block.
#define D_MACROS_BLOCK1 1
#define D_MACROS_BLOCK2 2
#define D_MACROS_BLOCK3 3

// D_MACROS_FALLBACK
//   constant: documented above its #ifndef, as the config skeleton does.
#ifndef D_MACROS_FALLBACK
    #define D_MACROS_FALLBACK 0
#endif  // D_MACROS_FALLBACK

// D_MACROS_TWICE
//   macro: function-like, so listed for review.
#define D_MACROS_TWICE(x) ((x) * 2)

// D_MACROS_PAREN
//   constant: a space before the parenthesis makes it object-like.
#define D_MACROS_PAREN (3)

int  d_macros_a(void);

int  d_macros_b(void);
int  d_macros_c(void);


#endif  // D_MACROS_FIXTURE
// D_MACROS_CHOSEN
//   constant: chosen by #if, documented once above it.
#if defined(D_MACROS_WIDE)
    #define D_MACROS_CHOSEN 2
#else
    #define D_MACROS_CHOSEN 1
#endif
