/* layout.hpp: indentation, read as C++ */
#ifndef D_LAYOUT_HPP_FIXTURE
#define D_LAYOUT_HPP_FIXTURE 1

NS_DJINTERP
NS_MATH

NS_INTERNAL

    // d_helper
    //   struct: negative -- one level in, under NS_INTERNAL.
    struct d_helper
    {
        int m_value;
    };

  struct d_stray
  {};

NS_END  // internal

// d_shape
//   class: access specifiers stand on the class's own column.
class d_shape
{
public:
    d_shape();

  private:
    int m_sides;
     int m_stray;
};

    struct d_indented
    {};

NS_END  // math
NS_END  // djinterp

#endif  // D_LAYOUT_HPP_FIXTURE
