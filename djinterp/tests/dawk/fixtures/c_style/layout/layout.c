/* layout.c: indentation and the size tripwires */

/*
d_joined
  positive: a definition's parameters aligned after its (.
*/
int
d_joined(int _a,
         int _b)
{
    return _a + _b;
}

/*
d_wide
  positive: more than five parameters.
*/
int
d_wide(
    int _a,
    int _b,
    int _c,
    int _d,
    int _e,
    int _f
)
{
    return _a + _b + _c + _d + _e + _f;
}

/*
d_deep
  positive: nested past four levels, with one statement out of its column
at each of levels 1, 2, 4, 5 and 6: columns 4, 10, 18, 22 and 26.
*/
int
d_deep(
    int _a
)
{
    int total = 0;

   total = _a;

    if (_a > 0)
    {
         total = 1;

        if (_a > 1)
        {
            if (_a > 2)
            {
                 total = 3;

                if (_a > 3)
                {
                     total = 4;

                    if (_a > 4)
                    {
                         total = 5;
                    }
                }
            }
        }
    }

    return total;
}

/*
d_long
  positive: more than sixty lines.
*/
int
d_long(
    int _a
)
{
    int total = _a;

    total = total + 0;
    total = total + 1;
    total = total + 2;
    total = total + 3;
    total = total + 4;
    total = total + 5;
    total = total + 6;
    total = total + 7;
    total = total + 8;
    total = total + 9;
    total = total + 10;
    total = total + 11;
    total = total + 12;
    total = total + 13;
    total = total + 14;
    total = total + 15;
    total = total + 16;
    total = total + 17;
    total = total + 18;
    total = total + 19;
    total = total + 20;
    total = total + 21;
    total = total + 22;
    total = total + 23;
    total = total + 24;
    total = total + 25;
    total = total + 26;
    total = total + 27;
    total = total + 28;
    total = total + 29;
    total = total + 30;
    total = total + 31;
    total = total + 32;
    total = total + 33;
    total = total + 34;
    total = total + 35;
    total = total + 36;
    total = total + 37;
    total = total + 38;
    total = total + 39;
    total = total + 40;
    total = total + 41;
    total = total + 42;
    total = total + 43;
    total = total + 44;
    total = total + 45;
    total = total + 46;
    total = total + 47;
    total = total + 48;
    total = total + 49;
    total = total + 50;
    total = total + 51;
    total = total + 52;
    total = total + 53;
    total = total + 54;
    total = total + 55;
    total = total + 56;
    total = total + 57;
    total = total + 58;
    total = total + 59;

    return total;
}
