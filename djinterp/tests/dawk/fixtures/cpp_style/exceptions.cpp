/* exceptions.cpp: negative -- a source may hold the two cases the guide keeps */

int d_checked(int _a)
{
    if (_a < 0)
    {
        throw _a;
    }

    return _a;
}
