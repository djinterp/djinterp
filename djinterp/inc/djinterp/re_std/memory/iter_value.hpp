/***********************************************************************
* restd                                                       iter_value.hpp
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
* restd's full iterator_traits will land when <iterator> is implemented;
* at that point this helper will be replaced and code that uses it will
* be updated to depend on the public trait.
*
* the const-pointer specialisations strip top-level const to match
* std::iterator_traits<const T*>::value_type, which is T (not const T).
*
*
* path:      /inc/djinterp/re_std/memory/iter_value.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_INTERNAL_ITER_VALUE_
#define RESTD_MEMORY_INTERNAL_ITER_VALUE_ 1

#include "djinterp.hpp"


namespace restd
{
namespace internal
{

// Primary: assume It is a class iterator with a value_type typedef.
template<typename _It>
struct iter_value
{
    typedef typename _It::value_type type;
};

// Raw pointer specialisations.
template<typename _T>
struct iter_value<_T*>
{
    typedef _T type;
};

template<typename _T>
struct iter_value<const _T*>
{
    typedef _T type;
};


}  // namespace internal
}  // namespace restd

#endif  // RESTD_MEMORY_INTERNAL_ITER_VALUE_
