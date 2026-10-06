/* braces.c: the Brackets section -- every case below is one line's worth */
#include <stddef.h>  // size_t


// positive: braceless then-body and else-body; negative: the else-if chain
int
d_braces_if(
    int _a
)
{
    if (_a)
        return 1;
    else if (_a > 2)
    {
        return 2;
    }
    else
        _a = 3;

    return _a;
}

// positive: a function body's brace on the name's line
int
d_braces_same_line(
    int _a
) {
    return _a;
}

// positive: control and aggregate braces on their opener's line
struct d_braces_point {
    int x;
    int y;
};

int
d_braces_loops(
    int _a
)
{
    while (_a > 10) {
        --_a;
    }

    switch (_a) {
        case 0:
            return 0;
        default:
            break;
    }

    return _a;
}

// negative: initializer braces and compound literals are not bodies
int
d_braces_initializers(
    void
)
{
    int                   table[3] = { 1, 2, 3 };
    struct d_braces_point point    = { .x = 1, .y = 2 };

    point = (struct d_braces_point){ 0, 0 };

    return table[0] + point.x;
}

// negative: an #if splitting the header is the pp-split rule's, not this one
int
d_braces_split(
    int _a
)
{
#ifdef D_BRACES_WIDE
    if (_a > 1)
#else
    if (_a > 2)
#endif  // D_BRACES_WIDE
    {
        return 4;
    }

    return _a;
}
