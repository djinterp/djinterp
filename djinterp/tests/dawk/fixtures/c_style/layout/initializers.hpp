/* initializers.hpp: a brace in a constructor's initializer list is not its
   body */
#ifndef D_INITIALIZERS_HPP_FIXTURE
#define D_INITIALIZERS_HPP_FIXTURE 1

// d_braced
//   class: members initialized with braces, a base with a template argument
// among them; the body is the brace after the last of them.
template<typename Type>
class d_braced : public d_base<Type>
{
public:
    d_braced(
        const Type& _first
    )
        : d_base<Type>{_first}
        , m_values{}
        , m_count(0)
    {
        m_count = 1;
          m_stray = 2;
    }

    d_braced()
        : m_values{}
    {}

private:
    Type m_values[4];
    int  m_count;
    int  m_stray;
};

#endif  // D_INITIALIZERS_HPP_FIXTURE
