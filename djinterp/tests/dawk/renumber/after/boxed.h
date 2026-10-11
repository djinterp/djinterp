/* boxed.h: sections between rules, and a table of contents in Roman */

/*
TABLE OF CONTENTS
=================
1.  TYPES                     (what the module keeps)
    -------------------------------------------------
2.  OPERATIONS
    ----------
    1.  creation
    2.  access
*/

#ifndef D_RENUMBER_BOXED_FIXTURE
#define D_RENUMBER_BOXED_FIXTURE 1

//==============================================================================
// 1.  TYPES
//==============================================================================

struct d_boxed
{
    int member;
};

//==============================================================================
// 2.  OPERATIONS
//==============================================================================

// 2.1    creation
//------------------------------------------------------------------------------
struct d_boxed* d_boxed_new(void);

// 2.2    access
//------------------------------------------------------------------------------
int d_boxed_get(const struct d_boxed* _boxed);

#endif  // D_RENUMBER_BOXED_FIXTURE
