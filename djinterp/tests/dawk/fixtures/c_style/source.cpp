/* source.cpp: the source-file rules, read as C++ */
#include "./source.hpp"
// std
#include <cstddef>  // std::size_t

NS_DJINTERP

/*
d_source::value
  negative: a member's comment may name it with its class.
*/
int
d_source::value() const
{
    return 0;
}

/*
d_source::operator+
  negative: and an operator with its symbol.
*/
int
d_source::operator+(
    int _a
) const
{
    return _a;
}

int
d_source::bare() const
{
    return 1;
}


#define D_SOURCE_BARE 1

// SOURCE_UNPREFIXED
//   macro: positive -- no D_ prefix.
#define SOURCE_UNPREFIXED 2

// NS_SOURCE
//   macro: negative -- a namespace macro is spelled NS_.
#define NS_SOURCE D_NAMESPACE(source)

// D_SOURCE_CALL
//   macro: positive, as advice -- function-like.
#define D_SOURCE_CALL(x) (x)

NS_END  // djinterp
