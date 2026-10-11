/* directives.c: the empty line a return and an if ask for, beside a
   directive and beside a comment on a brace's line */

/*
d_directive_branch
  negative: a return directly under #if, under #elif, and under a comment
that opens an #else: each opens its branch.
*/
int
d_directive_branch(
    int _a
)
{
    int b = _a;

#if D_FIXTURE_FIRST
    return b;
#elif D_FIXTURE_SECOND
    return b + 1;
#else
    // the comment opens the branch, and its return with it
    return b + 2;
#endif
}

/*
d_directive_after
  positive: a return directly under #endif.
*/
int
d_directive_after(
    int _a
)
{
    int b = _a;

#if D_FIXTURE_FIRST
    b += 1;
#endif
    return b;
}

/*
d_directive_if
  negative: an if directly above #else and one directly above #endif; an
if whose last brace has a comment on its line and an empty line under it.
positive: a statement directly under such a comment's line; an #if directly
under an if.
*/
int
d_directive_if(
    int _a
)
{
    int b = 0;

#if D_FIXTURE_FIRST
    if (_a)
    {
        b = 1;
    }
#else
    if (_a)
    {
        b = 2;
    }
#endif

    if (_a)
    {
        b = 3;
    }   // on the brace's line, an empty line under it

    if (_a)
    {
        b = 4;
    }   // on the brace's line, a statement under it
    b += 5;

    if (_a)
    {
        b = 6;
    }
#if D_FIXTURE_SECOND
    b += 7;
#endif

    return b;
}
