/***********************************************************************
* restd                                             front_insert_iterator.hpp
*
* mirror of back_insert_iterator, but calls push_front instead of
* push_back. Useful for any container that supports push_front
* (deque, list, forward_list).
*
* IMPORTANT: copying [first, last) into a front_insert_iterator
* REVERSES the order of elements in the destination. The first
* element copied becomes the last one pushed (and so ends up at
* the very front).
*
*     std::list<int> src = { 1, 2, 3 };
*     std::list<int> dst;
*     restd::copy(src.begin(), src.end(), restd::front_inserter(dst));
*     // dst is now { 3, 2, 1 }
*
*
* path:      /inc/restd/iterator/front_insert_iterator.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_FRONT_INSERT_ITERATOR_
#define RESTD_ITERATOR_FRONT_INSERT_ITERATOR_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>

    #include "restd/iterator/output_iterator_tag.hpp"
    #include "restd/utility/move.hpp"


namespace restd
{

template<typename _Container>
class front_insert_iterator
{
public:
    typedef output_iterator_tag         iterator_category;
    typedef void                        value_type;
    typedef void                        difference_type;
    typedef void                        pointer;
    typedef void                        reference;
    typedef _Container                  container_type;

protected:
    _Container* container;

public:
    explicit front_insert_iterator(_Container& _c) D_NOEXCEPT
        : container(&_c) {}

    front_insert_iterator&
    operator=(const typename _Container::value_type& _value)
    {
        container->push_front(_value);
        return *this;
    }

    front_insert_iterator&
    operator=(typename _Container::value_type&& _value)
    {
        container->push_front(restd::move(_value));
        return *this;
    }

    front_insert_iterator& operator*()       { return *this; }
    front_insert_iterator& operator++()      { return *this; }
    front_insert_iterator  operator++(int)   { return *this; }
};


// ---- front_inserter factory ----

template<typename _Container>
front_insert_iterator<_Container> front_inserter(_Container& _c)
{
    return front_insert_iterator<_Container>(_c);
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_FRONT_INSERT_ITERATOR_
