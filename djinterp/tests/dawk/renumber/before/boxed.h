/* boxed.h: sections between rules, and a table of contents in Roman */

/*
TABLE OF CONTENTS
=================
I.    TYPES                     (what the module keeps)
      -------------------------------------------------

II.   OPERATIONS
      ----------
      i.   creation
      ii.  access
*/

#ifndef D_RENUMBER_BOXED_FIXTURE
#define D_RENUMBER_BOXED_FIXTURE 1

// =============================================================================
// I.   TYPES
// =============================================================================

struct d_boxed
{
    int member;
};

// =============================================================================
// II.  OPERATIONS
// =============================================================================

// i.    creation

struct d_boxed* d_boxed_new(void);

// ii.   access
int d_boxed_get(const struct d_boxed* _boxed);

#endif  // D_RENUMBER_BOXED_FIXTURE
