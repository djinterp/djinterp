#include "..\..\inc\functional\functional.h"


D_INLINE bool
d_functional_is_sorted
(
    const void*            _input,
    size_t                 _count,
    size_t                 _element_size,
    fn_function_comparator _function_comparator
)
{
    const unsigned char* src;
    size_t               i;

    if ( (!_input)                 || 
         (!_function_comparator)   ||
         (_count <= 1) )
    {
        return true;
    }

    src = (const unsigned char*)_input;

    for (i = 0; i + 1 < _count; i++)
    {
        if (_function_comparator(src + (i * _element_size),
                                 src + ((i + 1) * _element_size),
                                 NULL) > 0)
        {
            return false;
        }
    }

    return true;
}

D_INLINE size_t
d_functional_index_of
(
    const void*  _input,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test
)
{
    void*  found;
    size_t offset;

    if ( (!_input)     ||
         (!_test)      ||
         (_count == 0) ||
         (_element_size == 0) )
    {
        return (size_t)-1;
    }

    found = d_functional_find_if(_input,
                                 _count,
                                 _element_size,
                                 _test,
                                 NULL);

    if (!found)
    {
        return (size_t)-1;
    }

    offset = (size_t)((const unsigned char*)found
             - (const unsigned char*)_input);

    return offset / _element_size;
}

D_INLINE void*
d_functional_find_last
(
    const void* _input,
    size_t      _count,
    size_t      _element_size,
    fn_predicate _test
)
{
    const unsigned char* src;
    size_t               i;

    if ( (!_input)     ||
         (!_test)      ||
         (_count == 0) ||
         (_element_size == 0) )
    {
        return NULL;
    }

    src = (const unsigned char*)_input;

    for (i = _count; i > 0; i--)
    {
        if (_test(src + ((i - 1) * _element_size), NULL))
        {
            return (void*)(src + ((i - 1) * _element_size));
        }
    }

    return NULL;
}

D_INLINE size_t
d_functional_last_index_of
(
    const void* _input,
    size_t      _count,
    size_t      _element_size,
    fn_predicate _test
)
{
    const unsigned char* src;
    size_t               i;

    if ( (!_input)     ||
         (!_test)      ||
         (_count == 0) ||
         (_element_size == 0) )
    {
        return (size_t)-1;
    }

    src = (const unsigned char*)_input;

    for (i = _count; i > 0; i--)
    {
        if (_test(src + ((i - 1) * _element_size), NULL))
        {
            return i - 1;
        }
    }

    return (size_t)-1;
}