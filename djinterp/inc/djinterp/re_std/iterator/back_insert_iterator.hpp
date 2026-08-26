/***********************************************************************
* re_std                                             back_insert_iterator.hpp
*
* output-iterator adaptor that turns assignment-through-dereference
* into push_back on a container:
*
*     auto it = re_std::back_inserter(my_vector);
*     *it = 42;     // calls my_vector.push_back(42)
*     ++it;         // no-op
*
* the canonical use is plumbing into algorithms that write results:
*
*     re_std::copy(src.begin(), src.end(), re_std::back_inserter(dst));
*
* surface:
*   - operator*  -> *this    (the assignment-target hack)
*   - operator=(const T&)    -> container.push_back(value)
*   - operator=(T&&)         -> container.push_back(move(value))
*   - operator++(), operator++(int)  -> *this  (no-op)
*
* The five member typedefs are all `void` except iterator_category =
* output_iterator_tag, per the standard. That makes back_insert_iterator
* an OUTPUT iterator only — algorithms that read the dereferenced value
* won't compile against it (they shouldn't — there's nothing to read).
*
*
* path:      /inc/djinterp/re_std/iterator/back_insert_iterator.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_BACK_INSERT_ITERATOR_
#define DJINTERP_RE_STD_ITERATOR_BACK_INSERT_ITERATOR_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>

    #include "re_std/iterator/output_iterator_tag.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

template<typename _Container>
class back_insert_iterator
{
public:
    // The standard's six required member typedefs.
    typedef output_iterator_tag         iterator_category;
    typedef void                        value_type;
    typedef void                        difference_type;
    typedef void                        pointer;
    typedef void                        reference;
    typedef _Container                  container_type;

protected:
    _Container* container;

public:
    explicit back_insert_iterator(_Container& _c) D_NOEXCEPT
        : container(&_c) {}

    back_insert_iterator&
    operator=(const typename _Container::value_type& _value)
    {
        container->push_back(_value);
        return *this;
    }

    back_insert_iterator&
    operator=(typename _Container::value_type&& _value)
    {
        container->push_back(re_std::move(_value));
        return *this;
    }

    // The output-iterator interface hack.
    back_insert_iterator& operator*()       { return *this; }
    back_insert_iterator& operator++()      { return *this; }
    back_insert_iterator  operator++(int)   { return *this; }
};


// ---- back_inserter factory ----

template<typename _Container>
back_insert_iterator<_Container> back_inserter(_Container& _c)
{
    return back_insert_iterator<_Container>(_c);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_BACK_INSERT_ITERATOR_
