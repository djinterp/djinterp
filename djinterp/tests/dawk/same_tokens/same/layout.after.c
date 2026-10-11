/* a banner, which is a comment */
#ifndef D_SAME_FIXTURE
#define D_SAME_FIXTURE 1
#include "./other.h"  // a summary
#define D_SAME_TWICE(x) \
    ((x) * 2)

// d_same
//   function: laid out again, and nothing else.
static int d_same(
    int         _a,
    const char* _name
)
{
    if (_a == 0)
    {
        _a = 1;
    }

    switch (_a)
    {
        case 1:
            return D_SAME_TWICE(_a);
    }

    return _a + (int)_name[0];
}

struct d_same_empty
{};

#endif  // D_SAME_FIXTURE
