/* spacing.c: blank lines, initializers, documentation, includes */
#include "./spacing.h"  // corresponding header
#include <stddef.h>     // size_t


/*
d_spacing_return
  positive: a return right after a statement, and one after a comment that
has no line above it; negative: a return opening its block, and one after a
blank line.
*/
int
d_spacing_return(
    int _a
)
{
    int b = _a;
    return b;
}

/*
d_spacing_comment
  see d_spacing_return.
*/
int
d_spacing_comment(
    int _a
)
{
    if (_a)
    {
        return 1;
    }

    int b = _a;
    // the comment has no line above it
    return b;
}

/*
d_spacing_if
  positive: a statement right after an if; negative: an if that closes
its block.
*/
int
d_spacing_if(
    int _a
)
{
    int b = 0;

    if (_a)
    {
        b = 1;
    }
    b += 2;

    while (b > 0)
    {
        if (b > 1)
        {
            --b;
        }
    }

    return b;
}



/*
d_spacing_three
  positive: three empty lines above; negative: none of its lines.
*/
int
d_spacing_three(
    void
)
{
    int uninitialized;


    uninitialized = 3;

    return uninitialized;
}

int
d_spacing_undocumented(
    void
)
{
    return 0;
}

// negative: a static definition waits on #84
static int
d_spacing_static(
    void
)
{
    return 0;
}
