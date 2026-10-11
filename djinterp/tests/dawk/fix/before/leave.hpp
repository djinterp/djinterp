/* leave.hpp: what stands right already in C++, or waits for a ruling */

namespace d_leave_deep
{

    // d_leave_braced
    //   class: members initialized with braces; the constructor's body is
    // the brace after the last of them, and stands where it is.
    class d_leave_braced
    {
    public:
        d_leave_braced()
            : m_values{}
            , m_count(0)
        {
            m_count = 1;
        }

    private:
        int m_values[4];
        int m_count;
    };

}

namespace d_leave_flat
{

// d_leave_plain
//   struct: a namespace that does not indent what it holds.
struct d_leave_plain
{
    int m_value;
};

}

// d_leave_rows
//   function: a table of ifs, each on one line. An empty line under each
// row would make it neither the table it is nor the guide's form, so the
// rows stand as they are until their shape is ruled on.
inline int
d_leave_rows(
    int _kind
)
{
    if (_kind == 1) { return 10; }
    if (_kind == 2) { return 20; }
    if (_kind == 3) { return 30; }

    return 0;
}

// d_leave_base
//   struct: a base given an argument in parentheses, which is no parameter
// list to lay out.
template<unsigned Feat>
struct d_leave_base
    : d_leave_mixin<d_leave_has(Feat, 1)>
{
    int m_value;
};
