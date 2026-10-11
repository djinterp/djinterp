/* expand.c: a body on its brace's line is laid out under it */


/*
d_expand_guard
  One statement in its braces, a block after each case label, and a body
with a comment in it, which is left.
*/
int
d_expand_guard(
    int _n
)
{
    if (_n == 0) { _n = 1; }

    switch (_n)
    {
        case 1:   { return 2; }
        default:  { _n += 1; break; }
    }

    if (_n > 9) { _n = 9; /* the most */ }

    return _n;
}

// d_expand_kind
//   enum: a type on one line.
enum d_expand_kind { d_expand_kind_a = 1, d_expand_kind_b = 2 };

/*
d_expand_rows
  Two ifs on one line each, one under the other: each is laid out, and then
they are set apart as any two ifs are.
*/
int
d_expand_rows(
    int _n
)
{
    if (_n == 1) { _n = 2; }
    if (_n == 3) { _n = 4; }

    return _n;
}
