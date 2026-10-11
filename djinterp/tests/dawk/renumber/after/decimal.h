/* decimal.h: sections already decimal and no table of contents; one is
   written from the headings, to the fourth level */

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Limits
         1.  D_DECIMAL_MAX
         2.  Kinds
              1.  D_DECIMAL_KIND_ONE
2.  OPERATIONS
    ----------
    1.  Reading
*/

#ifndef D_RENUMBER_DECIMAL_FIXTURE
#define D_RENUMBER_DECIMAL_FIXTURE 1

//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Limits
//------------------------------------------------------------------------------
// 1.1.1
// D_DECIMAL_MAX
//   constant: the most there may be.
#define D_DECIMAL_MAX 8

// 1.1.2
// Kinds
// 1.1.2.1
// D_DECIMAL_KIND_ONE
//   constant: the first kind.
#define D_DECIMAL_KIND_ONE 1

//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Reading
//------------------------------------------------------------------------------
int d_decimal_read(void);

#endif  // D_RENUMBER_DECIMAL_FIXTURE
