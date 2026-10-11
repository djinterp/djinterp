/* exceptions.hpp: a header does not throw */

int d_checked(int _a)
{
    if (_a < 0)
    {
        throw _a;
    }

    return _a;
}

int d_quiet(int _a) throw();
