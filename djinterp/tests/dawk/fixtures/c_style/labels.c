/* labels.c: Control Transfer */

/*
d_one
  negative: one cleanup label.
*/
int
d_one(
    int _a
)
{
    int result = 0;

    if (_a < 0)
    {
        goto cleanup;
    }

    result = _a;

    cleanup:
    return result;
}

/*
d_two
  positive: a second label.
*/
int
d_two(
    int _a
)
{
    int result = 0;

    if (_a < 0)
    {
        goto fail;
    }

    result = _a;

    goto done;

    fail:
    result = -1;

    done:
    return result;
}
