/* else.c: an else behind a closing brace gets a line of its own when its
   brace does */


/*
d_else_chain
  A chain written with each brace at its line's end: every brace moves, and
each else with it. A body on the else's line is laid out as any other is.
*/
int
d_else_chain(
    int _n
)
{
    int result = 0;

    if (_n == 0)
    {
        result = 1;
    }
    else if (_n == 1)
    {
        result = 2;
    }
    else
    {
        result = 3;
    }

    if (_n > 9)
    {
        result = 9;
    }
    else
    {
        result = 0;
    }

    return result;
}
