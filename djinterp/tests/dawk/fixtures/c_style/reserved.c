/* reserved.c: Reserved Identifiers */
#define _POSIX_C_SOURCE 200809L   // negative: the implementation reads it
#define _D_RESERVED_MACRO 1       // positive
#define D_RESERVED__MACRO 1       // positive: a double underscore anywhere

struct d_reserved
{
    int __count;                  // positive
    int d_ok__not;                // positive
    int _Upper;                   // positive
    int d_ok;                     // negative
};

int
d_reserved_param(
    int _fine                     // negative: underscore, then lower case
)
{
    return _fine;
}
