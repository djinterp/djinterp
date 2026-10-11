/* fix.c: what `dcheck --fix` puts right, and what it leaves */
#include "./fix.h"  // corresponding header
#include "../djinterp.h"


/*
d_fix_brace
  Three braces at the end of a line, one of them under a condition of two
lines; and a comment between a return and what is above it.
*/
int
d_fix_brace(
    int _a
)
{
    int result = 0;

    if (_a > 0)
    {
        result = _a;
    }

    while (result > 10)
    {
        result -= 10;
    }

    if ( (_a < 0) &&
         (result == 0) )
    {
        result = -1;
    }

    // the answer
    return result;
}

/*
d_fix_return
  A return straight under a statement.
*/
int
d_fix_return(
    int _a
)
{
    int result = _a;

    return result;
}

// d_fix_pair
//   struct: a brace after a tag.
struct d_fix_pair
{
    int first;
    int second;
};

// d_fix_inline
//   function: a body on its brace's line is laid out under it, and stays
// under this comment.
int d_fix_inline(int _a)
{
    int twice = _a + _a;

    return twice;
}

// d_fix_inline_noted
//   function: left as it is, since its body holds a comment; and a return
// in the middle of a line is no reason to part this comment from it.
int d_fix_inline_noted(int _a) { int twice = _a + _a; return twice; /* 2a */ }

/*
d_fix_joined
  Two parameters on the name's line.
*/
static int d_fix_joined(
    int         _a,
    const char* _name
)
{
    return _a + (int)_name[0];
}

/*
d_fix_under
  One to a line under the first, a function pointer among them: its name is
not its last word, so no name is put in a column.
*/
int
d_fix_under(
    int _a,
    const char* _name,
    int (*_fn)(int _x)
)
{
    return _fn(_a) + (int)_name[0];
}

/*
d_fix_noted
  A comment in the list: left, since moving the lines would move what the
comment is beside.
*/
int
d_fix_noted(int _a,   // the count
            int _b)
{
    return _a + _b;
}
