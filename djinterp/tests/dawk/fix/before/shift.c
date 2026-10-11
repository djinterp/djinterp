/* shift.c: an item out of its column moves, and every line of it with it,
   and the comments beside it; or nothing of its block does */


/*
d_shift_switch
  Case labels on the switch's own column and what is under them one level
in: each moves a level, a statement of two lines keeps its second under its
first, and the loop's body moves with the loop.
*/
int
d_shift_switch(
    int _kind,
    int _value
)
{
    int result = 0;

    switch (_kind)
    {
    case 1:
        result = d_shift_other(_value,
                               _kind);

        break;

    default:
        for (int at = 0; at < _value; ++at)
        {
            result += at;
        }

        break;
    }

    return result;
}

/*
d_shift_comments
  A comment in the column of what follows it moves with it, with or without
an empty line between them, a block comment by all its lines; and so does
one that closes a case, in the column of what stands before it. A comment
in another column stays where it is.
*/
int
d_shift_comments(
    int _kind
)
{
    int result = 0;

    switch (_kind)
    {
    // the first case
    case 1:
        // what the case does,
        // on two lines
        result = 1;

        /* a block comment
           of two lines */
        result += 1;

        // with an empty line under it

        break;

// in the first column: put there on purpose
    case 2:
        result = 2;

        break;
        // closes the case

    default:
        break;
        // closes the switch
    }

    return result;
}

/*
d_shift_held
  One statement under a label has no room to move: the 80th column is in
its way. Its label is left with it, and every other line of the switch,
which stood in the columns these stand in.
*/
int
d_shift_held(
    int _kind
)
{
    const char* text = "";

    switch (_kind)
    {
    case 1:
        text = "short";

        break;

    case 2:
        text = "a line that four columns more would carry beyond the eightieth";

        break;

    default:
        break;
    }

    return (text[0] != 0);
}
