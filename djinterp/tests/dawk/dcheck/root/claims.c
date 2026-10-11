/* claims.c: one node, several rules */

void
d_both(
    int _a,
    int _b
)
{
    (void)_a;
    (void)_b;
}

int
d_three(
    int _a,
    int _b,
    int _c
)
{
    return _a + _b + _c;
}

void
d_exempt(
    int _a,
    int _b,
    int _c
)
{
    (void)_a;
    (void)_b;
    (void)_c;
}

int
d_column(
    int _a
)
{
  return _a;
}
