#include "..\..\inc\c\djinterp.h"


/*
d_index_convert_fast
  Converts a `d_index` to its positive equivalent.
  Note that if any of the following conditions are not met, then this will 
result in undefined behavior (at the very least), and very possibly a SIGSEGV
error:
  1. The absolute value of parameter `_index` must be greater than, or equal to
     parameter `_count`.
  2. `_count` must be nonzero.
  3. Parameter `_storage` cannot be `NULL`, and must point to a valid `size_t`
     location in memory.
  For the safer version of this function, see: `d_index_convert_safe`.

Parameter(s):
  _index:     the `d_index` being converted to a positive size_t.
  _count: the maximum possible index (exclusive)
Return:
  A `size_t` value equivalent to parameter `_index`, equal to `_index` (if 0 or
positive), or equal to `_count` - `abs(_index)` (if negative, where `abs`
is the absolute value function.
*/
D_INLINE size_t 
d_index_convert_fast
(
    d_index _index, 
    size_t  _count
) 
{
    // if index is positive or zero: return index
    // if index is negative: return _count - abs(_index)
    return (_index >= 0) ?
        (size_t)_index :
        (size_t)(_count - ((ssize_t)(-_index)));
}

/*
d_index_convert_safe
  Converts a `d_index` to its positive equivalent after first ensuring that the
absolute value of parameter `d_index` is less than, or equal to, parameter
`_count`.
  Additionally, parameter `_size` must be greater than zero, and the parameter
`_destination` must be a valid pointer.  If all preconditions are  met, then a
size_t equivalent will be saved in the memory location specified by the 
parameter.
  For the faster (yet unsafe) version of this function, see: 
`d_index_convert_fast`.

Parameter(s):
  _index:       the `d_index` being converted to a positive `size_t` and stored in 
                `_destination`.
  _count:       the maximum possible index (exclusive).
  _destination: a pointer corresponding to a valid, non-NULL location in
                memory.
Precondition(s):
  1. The absolute value of parameter `_index` must be less than `_count`.
  2. Parameter `_destination` cannot be `NULL`, and must point to a valid `size_t`
     location in memory.
Return:
  A boolean value equivalent to either:
  - D_SUCCESS, if all preconditions were satisfied and the valid positive index
    was computed and stored in `storage`, or
  - D_FAILURE, if any one (or more) of the preconditions were not met (in which
    case, the `_destination` remains unmodified by this operation.
*/
D_INLINE bool
d_index_convert_safe
(
    d_index _index, 
    size_t  _count,
    size_t* _destination
) 
{
    size_t positive_index,
           abs_value;

    // `_destination` must be non-NULL
    if (_destination == NULL)
    {
        return D_FAILURE;
    }
    
    // convert negative to positive (requires size)
    if (_index >= 0)
    {
        positive_index = (size_t)_index;
    }
    else
    {
        abs_value = (size_t)(-_index);

        if (abs_value > _count)
        {
            return D_FAILURE;
        }
        
        positive_index = _count - abs_value;
    }
    
    // validate against max (requires max_index)
    if (positive_index >= _count)
    {
        return D_FAILURE;
    }
    
    *(_destination) = positive_index;

    return D_SUCCESS;
}

/*
d_index_is_valid
  Evaluates a `d_index` (which may be negative) against the maximum allowable
index (exclusive) for validity.
  In order to return a boolean value equivalent to `true`, the following 
conditions must be true:
  1. `_count` must be nonzero, and
  2. one of the following conditions must be true:
     - (if parameter `_index` is negative):  then the absolute value of 
       parameter `_index` must be less than, or equal to parameter `_count`, or
     - (if parameter `_index` is non-negative) the value of parameter `_index` 
       must be less than parameter `_count`.

Parameter(s):
  _index: the `d_index` being evaluated for validity
  _count:  the maximum possible index (exclusive)
Return:
  A boolean value equivalent to either:
  - `true`, if the conditions outlined above are met and parameter `_index` is 
     a valid index, or
  - `false, if one or more of the conditions outlined were NOT met, and 
     parameter `_index` is an invalid index.
*/
D_INLINE bool
d_index_is_valid
(
    d_index _index, 
    size_t  _count
)
{
    return ( (_count > 0) &&
             ( ( (_index >= 0) && ((size_t)_index < _count) ) ||
               ( (_index < 0)  && ((size_t)(-_index) <= _count) ) ) );
}

