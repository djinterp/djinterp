/* leave.c: what stands right already, or was put where it is on purpose */


/*
d_leave_block
  A block further down a case is a statement among the others, one level in
from the label; only the label's own body stands on the label's column.
*/
int
d_leave_block(
    int _kind
)
{
    int result = 0;

    switch (_kind)
    {
        case 1:
        {
            result = 1;

            break;
        }

        case 2:
            result = 2;

            {
                int twice = result * 2;

                result = twice;
            }

            break;

        default:
            break;
    }

    return result;
}

/*
d_leave_table
  A switch written as a table: each `break` lines up with the statement
above it, 21 columns from where a level would put it. Not a whole number of
levels: left.
*/
int
d_leave_table(
    int _kind
)
{
    int result = 0;

    switch (_kind)
    {
        case 1:  result = 10;
                 break;
        case 2:  result = 20;
                 break;
        default: result = 0;
                 break;
    }

    return result;
}

/*
d_leave_split
  An if whose head is chosen by #if has two texts and one body, which stands
where one if puts it.
*/
int
d_leave_split(
    int _bits
)
{
#ifdef D_LEAVE_WIDE
    if (_bits & 3)
#else
    if (_bits & 1)
#endif  // D_LEAVE_WIDE
    {
        return 1;
    }

    return 0;
}
