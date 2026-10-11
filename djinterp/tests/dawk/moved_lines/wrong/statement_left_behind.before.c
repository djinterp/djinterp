/* statement_left_behind: every line of a switch moved but one, which had
   no room */

const char*
d_statement_left_behind(
    int _kind
)
{
    switch (_kind)
    {
    case 1:
        return "one";
    case 2:
        return "two, which is a much longer text than the others, and ends here";
    default:
        return "none";
    }
}
