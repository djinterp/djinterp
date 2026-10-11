/* classes.hpp: a parenthesis in a class's head is no parameter list, and
   the class no function */
#ifndef D_CLASSES_HPP_FIXTURE
#define D_CLASSES_HPP_FIXTURE 1

// d_mixed
//   class: two bases, each given an argument in parentheses. The head has
// no parameters to lay out, and the member is the definition it looks like.
template<unsigned Feat>
struct d_mixed
    : d_underline<d_has(Feat, 1)>
    , d_overline<d_has(Feat, 2)>
{
    // sum
    //   function: two parameters on the name's line.
    int
    sum(int _a, int _b)
    {
        return _a + _b;
    }
};

// d_traits
//   class: a specialization whose argument spells a function type.
template<typename Return,
         typename... Args>
struct d_traits<Return(Args...)>
{
    // apply
    //   function: two parameters on the name's line.
    Return
    apply(Return _a, int _b)
    {
        return _a;
    }
};

// d_kind
//   enum: an underlying type after the name.
enum class d_kind : unsigned char
{
    D_KIND_NONE = 0,
    D_KIND_SOME
};

// d_make
//   function: returns a tag's type by its template arguments, and is still
// the function it looks like, with parameters of its own.
struct d_node<int>*
d_make(int _a, int _b)
{
    return 0;
}

#endif  // D_CLASSES_HPP_FIXTURE
