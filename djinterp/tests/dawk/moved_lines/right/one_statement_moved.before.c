/* one_statement_moved: a statement a level too far left, in the column the
   labels of its switch stand in, moved alone: a label is not a statement */

int
d_one_statement_moved(
    int _kind
)
{
    int result = 0;

    switch (_kind)
    {
        case 1:
            result = 1;

            break;

        default:
        // its own comment
        result = 2;

            break;
    }

      result += 1;

    return result;
}
