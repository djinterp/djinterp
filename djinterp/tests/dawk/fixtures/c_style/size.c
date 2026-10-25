/* size.c: the length tripwire of Function Size and Complexity, 75 lines
   from the return type to the closing brace (decision 57) */
#include <stddef.h>  // size_t


/*
d_size_at_limit
  negative: 75 lines, the limit itself, is not over it.
*/
int
d_size_at_limit(
    int _x
)
{
    int x = _x;

    x = x + 1;
    x = x + 2;
    x = x + 3;
    x = x + 4;
    x = x + 5;
    x = x + 6;
    x = x + 7;
    x = x + 8;
    x = x + 9;
    x = x + 10;
    x = x + 11;
    x = x + 12;
    x = x + 13;
    x = x + 14;
    x = x + 15;
    x = x + 16;
    x = x + 17;
    x = x + 18;
    x = x + 19;
    x = x + 20;
    x = x + 21;
    x = x + 22;
    x = x + 23;
    x = x + 24;
    x = x + 25;
    x = x + 26;
    x = x + 27;
    x = x + 28;
    x = x + 29;
    x = x + 30;
    x = x + 31;
    x = x + 32;
    x = x + 33;
    x = x + 34;
    x = x + 35;
    x = x + 36;
    x = x + 37;
    x = x + 38;
    x = x + 39;
    x = x + 40;
    x = x + 41;
    x = x + 42;
    x = x + 43;
    x = x + 44;
    x = x + 45;
    x = x + 46;
    x = x + 47;
    x = x + 48;
    x = x + 49;
    x = x + 50;
    x = x + 51;
    x = x + 52;
    x = x + 53;
    x = x + 54;
    x = x + 55;
    x = x + 56;
    x = x + 57;
    x = x + 58;
    x = x + 59;
    x = x + 60;
    x = x + 61;
    x = x + 62;
    x = x + 63;
    x = x + 64;
    x = x + 65;

    return x;
}

/*
d_size_over_limit
  positive: 76 lines is one over the limit.
*/
int
d_size_over_limit(
    int _x
)
{
    int x = _x;

    x = x + 1;
    x = x + 2;
    x = x + 3;
    x = x + 4;
    x = x + 5;
    x = x + 6;
    x = x + 7;
    x = x + 8;
    x = x + 9;
    x = x + 10;
    x = x + 11;
    x = x + 12;
    x = x + 13;
    x = x + 14;
    x = x + 15;
    x = x + 16;
    x = x + 17;
    x = x + 18;
    x = x + 19;
    x = x + 20;
    x = x + 21;
    x = x + 22;
    x = x + 23;
    x = x + 24;
    x = x + 25;
    x = x + 26;
    x = x + 27;
    x = x + 28;
    x = x + 29;
    x = x + 30;
    x = x + 31;
    x = x + 32;
    x = x + 33;
    x = x + 34;
    x = x + 35;
    x = x + 36;
    x = x + 37;
    x = x + 38;
    x = x + 39;
    x = x + 40;
    x = x + 41;
    x = x + 42;
    x = x + 43;
    x = x + 44;
    x = x + 45;
    x = x + 46;
    x = x + 47;
    x = x + 48;
    x = x + 49;
    x = x + 50;
    x = x + 51;
    x = x + 52;
    x = x + 53;
    x = x + 54;
    x = x + 55;
    x = x + 56;
    x = x + 57;
    x = x + 58;
    x = x + 59;
    x = x + 60;
    x = x + 61;
    x = x + 62;
    x = x + 63;
    x = x + 64;
    x = x + 65;
    x = x + 66;

    return x;
}
