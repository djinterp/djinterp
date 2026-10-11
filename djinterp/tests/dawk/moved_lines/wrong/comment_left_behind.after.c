/* comment_left_behind: the statement moved and the comment above it did
   not */

int
d_comment_left_behind(
    int _kind
)
{
    int result = 0;

    switch (_kind)
    {
        case 1:
        // what the case does
            result = 1;

            break;

        default:
            break;
    }

    return result;
}
