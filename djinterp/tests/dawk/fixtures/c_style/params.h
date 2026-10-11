/* params.h: Function Declarations -- parameter names */

int  d_named(int _a);
int  d_unnamed(int,
               const char*);
int  d_none(void);
int  d_variadic(const char* _format,
                ...);
void d_callback(int (*_fn)(int));

// d_vtable
//   struct: negative -- function-pointer members, their parameters named.
struct d_vtable
{
    int (*open)(void* _context);
    int (*close)(void* _context);
};

D_DECLARE_ALL(D_INTERNAL_DECLARE)
