/* namespaces.hpp: a namespace's contents stand where its first item does */
#ifndef D_NAMESPACES_HPP_FIXTURE
#define D_NAMESPACES_HPP_FIXTURE 1

namespace d_flat
{

// d_flat_first
//   struct: negative -- at the namespace's own level, and so the rest is.
struct d_flat_first
{};

    struct d_flat_stray
    {};

}

namespace d_deep
{

    // d_deep_first
    //   struct: negative -- one level in, and so the rest is.
    struct d_deep_first
    {
        int m_value;
    };

struct d_deep_stray
{};

}

D_NAMESPACE(d_macro)

    // d_macro_first
    //   struct: negative -- a header's own namespace, indented as it has it.
    struct d_macro_first
    {};

  struct d_macro_stray
  {};

NS_END  // d_macro

D_NAMESPACE(d_level)

// d_level_first
//   struct: negative -- and one that does not indent.
struct d_level_first
{};

NS_END  // d_level

// d_after
//   struct: negative -- the level is back once the last namespace is shut.
struct d_after
{};

#endif  // D_NAMESPACES_HPP_FIXTURE
