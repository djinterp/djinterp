/* closing_brace_left_behind: a loop moved by one line too few */

int
d_closing_brace_left_behind(
    int _n
)
{
    int result = 0;

        for (int at = 0; at < _n; ++at)
        {
            result += at;
        }

    return result;
}
