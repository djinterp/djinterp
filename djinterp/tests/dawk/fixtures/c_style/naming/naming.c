/* naming.c: Naming Conventions, Typedefs, void functions */

// d_good
//   struct: negative.
struct d_good
{
    int member;
};

// bad_tag
//   struct: positive -- no d_ prefix.
struct bad_tag
{
    int member;
};

// bad_union
//   union: positive -- no d_ prefix.
union bad_union
{
    int   whole;
    float part;
};

// bad_enum
//   enum: positive -- no d_ prefix.
enum bad_enum
{
    BAD_ENUM_ONE
};

// D_GOOD_MACRO
//   macro: negative.
#define D_GOOD_MACRO 1

// BAD_MACRO
//   macro: positive -- no D_ prefix.
#define BAD_MACRO 2

// d_hidden
//   type: positive -- hides a struct.
typedef struct d_good d_hidden;

// d_callback
//   type: negative -- a function pointer.
typedef int (*d_callback)(int _value);

/*
d_named
  positive: `value` has no leading underscore.
*/
int
d_named(
    int value
)
{
    return value;
}

/*
d_silent
  positive: a void function with no closing return.
*/
void
d_silent(
    int _a
)
{
    (void)_a;
}
