/***********************************************************************
* re_std                                                  insert_iterator.hpp
*
* output-iterator adaptor that inserts at a tracked position via
* container.insert(iter, value). Unlike back/front_inserter, this
* preserves the source ORDER of elements when a range is inserted:
*
*     // dst = { 10, 20, 30 }, with hint = dst.begin() + 1 (between 10 and 20)
*     copy(src.begin(), src.end(), inserter(dst, dst.begin() + 1));
*     // After copying { 1, 2, 3 }: dst = { 10, 1, 2, 3, 20, 30 }
*
* the trick is that container.insert(it, value) returns an iterator
* to the newly-inserted element. We advance our internal iterator to
* the position AFTER it, so the next insert lands one further along —
* which keeps the source order intact.
*
* this is the only one of the three insert iterator adaptors whose
* internal state is non-trivial.
*
*
* path:      /inc/djinterp/re_std/iterator/insert_iterator.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_INSERT_ITERATOR_
#define DJINTERP_RE_STD_ITERATOR_INSERT_ITERATOR_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>

    #include "re_std/iterator/output_iterator_tag.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

template<typename _Container>
class insert_iterator
{
public:
    typedef output_iterator_tag                 iterator_category;
    typedef void                                value_type;
    typedef void                                difference_type;
    typedef void                                pointer;
    typedef void                                reference;
    typedef _Container                          container_type;

protected:
    _Container*                                 container;
    typename _Container::iterator               iter;

public:
    insert_iterator(_Container& _c,
                    typename _Container::iterator _i)
        : container(&_c), iter(_i) {}

    insert_iterator&
    operator=(const typename _Container::value_type& _value)
    {
        // The standard says: iter = container->insert(iter, value);
        // followed by ++iter. The simpler form is to assign to
        // post-increment of insert's return.
        iter = container->insert(iter, _value);
        ++iter;
        return *this;
    }

    insert_iterator&
    operator=(typename _Container::value_type&& _value)
    {
        iter = container->insert(iter, re_std::move(_value));
        ++iter;
        return *this;
    }

    insert_iterator& operator*()       { return *this; }
    insert_iterator& operator++()      { return *this; }
    insert_iterator& operator++(int)   { return *this; }
};


// ---- inserter factory ----

template<typename _Container>
insert_iterator<_Container>
inserter(_Container& _c, typename _Container::iterator _i)
{
    return insert_iterator<_Container>(_c, _i);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_INSERT_ITERATOR_
