/*******************************************************************************
* djinterp [re_std]                                               iter_value.hpp
*
* minimal iterator-value-type helper for use inside <memory>:
*   internal::iter_value<It>::type yields the value_type associated with
*   iterator type It.
*
* this is NOT iterator_traits. It handles only what the uninitialized_*
* algorithm family needs:
*   - class iterators that expose a value_type member typedef
*   - raw pointers (T*, const T*, T* const, const T* const)
*
* re_std's full iterator_traits will land when <iterator> is implemented;
* at that point this helper will be replaced and code that uses it will
* be updated to depend on the public trait.
*
* the const-pointer specialisations strip top-level const to match
* std::iterator_traits<const T*>::value_type, which is T (not const T).
*
*
* path:      /inc/re_std/memory/iter_value.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_ITER_VALUE_HPP
#define RE_STD_MEMORY_ITER_VALUE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{
namespace internal
{

// Primary: assume It is a class iterator with a value_type typedef.
template<typename It>
struct iter_value
{
    typedef typename It::value_type type;
};

// Raw pointer specialisations.
template<typename T>
struct iter_value<T*>
{
    typedef T type;
};

template<typename T>
struct iter_value<const T*>
{
    typedef T type;
};


}  // internal
}  // re_std
#endif  // RE_STD_MEMORY_ITER_VALUE_HPP
