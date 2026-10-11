/* one_line_if.c: the empty line an if asks for, when the if is on one line */

/*
d_one_line_if
  positive: a statement directly under an if on one line, and one under such
an if's comment; negative: the same with an empty line between.
*/
int
d_one_line_if(
    int _a
)
{
    int b = 0;

    if (_a == 1) { b = 1; }
    b += 2;

    if (_a == 2) { b = 2; }  // the second
    b += 3;

    if (_a == 3) { b = 3; }  // the third

    return b;
}
