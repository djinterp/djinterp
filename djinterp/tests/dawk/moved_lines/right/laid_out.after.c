/* laid_out: what each repair of layout does when it is right */

int d_laid_out(
    int _a,
    int _b
)
{
    if (_a == 0)
    {
        _a = 1;
    }

    if (_a > _b)
    {
        _a = _b;
    }
    else if (_a < 0)
    {
        _a = 0;
    }
    else
    {
        _a += 1;
    }

    switch (_a)
    {
        case 1:
        {
            return 2;
        }
        default: break;
    }

    return _a;
}

enum d_laid_out_kind
{
    d_laid_out_a = 1,
    d_laid_out_b = 2
};
