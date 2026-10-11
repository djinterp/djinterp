/* closing_comment_left_behind: the comment that closes a case stood under
   its last statement, which moved */

int
d_closing_comment_left_behind(
    int _kind
)
{
    int result = 0;

    switch (_kind)
    {
    case 1:
        result = 1;

        break;
        // nothing falls through

    default:
        break;
    }

    return result;
}
