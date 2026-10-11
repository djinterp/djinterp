/* else_not_under_its_brace: an else given a line a level too far in */

int
d_else_not_under_its_brace(
    int _n
)
{
    if (_n == 0)
    {
        _n = 1;
    }
        else
    {
        _n = 2;
    }

    return _n;
}
