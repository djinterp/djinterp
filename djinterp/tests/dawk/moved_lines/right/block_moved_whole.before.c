/* block_moved_whole: a switch's body a level too far left, moved with its
   comments, its continuation lines and a call whose ( ends its line */

int
d_block_moved_whole(
    int _kind,
    int _value
)
{
    int result = 0;

    switch (_kind)
    {
    // the first case
    case 1:
        // two lines of statement
        result = d_other(_value,
                         _kind);

        if (d_another(
                _value,
                _kind) == 0)
        {
            result = 1;
        }

        break;
        // closes the case

// put in the first column on purpose
    default:
        for (int at = 0; at < _value; ++at)
        {
            result += at;
        }

        break;
    }

    return result;
}
