/*******************************************************************************
* djinterp [c]                                                   option_common.c
*
*   Implementation of the option cell. See option_common.h for the rationale;
* this file carries only what the code needs to be read correctly.
*
*
* path:      /src/djinterp/c/option/option_common.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#include "../../../../inc/djinterp/c/option/option_common.h"


// ===========================================================================
// I.   STATUS
// ===========================================================================

struct d_option_result
d_option_ok(
    uint32_t _value
)
{
    struct d_option_result result;

    result.is_ok         = true;
    result.payload.value = _value;

    return result;
}

struct d_option_result
d_option_fail(
    int32_t _status
)
{
    struct d_option_result result;

    result.is_ok         = false;
    result.payload.error = _status;

    return result;
}

int32_t
d_option_result_status(
    const struct d_option_result* _result
)
{
    if (_result == NULL)
    {
        return (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
    }

    return ( _result->is_ok
                 ? (int32_t)D_OPTION_STATUS_OK
                 : _result->payload.error );
}

const char*
d_option_status_name(
    enum d_option_status _status
)
{
    switch (_status)
    {
    case D_OPTION_STATUS_OK:                  return "OK";
    case D_OPTION_STATUS_KEY_ABSENT:          return "KEY_ABSENT";
    case D_OPTION_STATUS_KEY_DUPLICATE:       return "KEY_DUPLICATE";
    case D_OPTION_STATUS_VALUE_TYPE_MISMATCH: return "VALUE_TYPE_MISMATCH";
    case D_OPTION_STATUS_KEY_TYPE_MISMATCH:   return "KEY_TYPE_MISMATCH";
    case D_OPTION_STATUS_NO_VALUE:            return "NO_VALUE";
    case D_OPTION_STATUS_POLICY_REJECTED:     return "POLICY_REJECTED";
    case D_OPTION_STATUS_CONFLICT:            return "CONFLICT";
    case D_OPTION_STATUS_INVALID_ARGUMENT:    return "INVALID_ARGUMENT";
    case D_OPTION_STATUS_BUFFER_TOO_SMALL:    return "BUFFER_TOO_SMALL";
    case D_OPTION_STATUS_CAPACITY:            return "CAPACITY";
    case D_OPTION_STATUS_VALUE_CAPACITY:      return "VALUE_CAPACITY";
    case D_OPTION_STATUS_NO_MEMORY:           return "NO_MEMORY";
    }

    return "UNKNOWN";
}

const char*
d_option_status_message(
    enum d_option_status _status
)
{
    switch (_status)
    {
    case D_OPTION_STATUS_OK:
        return "the operation completed";
    case D_OPTION_STATUS_KEY_ABSENT:
        return "no option in the set carries that key";
    case D_OPTION_STATUS_KEY_DUPLICATE:
        return "the key is already present; a set's keys are unique";
    case D_OPTION_STATUS_VALUE_TYPE_MISMATCH:
        return "the value's width does not match the slot's";
    case D_OPTION_STATUS_KEY_TYPE_MISMATCH:
        return "the key's declared type does not match the set's";
    case D_OPTION_STATUS_NO_VALUE:
        return "the option is unary and carries no value";
    case D_OPTION_STATUS_POLICY_REJECTED:
        return "the merge policy refused this position";
    case D_OPTION_STATUS_CONFLICT:
        return "the sets conflict; their plain union is not defined";
    case D_OPTION_STATUS_INVALID_ARGUMENT:
        return "an argument was null, aliased, or out of range";
    case D_OPTION_STATUS_BUFFER_TOO_SMALL:
        return "the destination is too small; the required size is reported";
    case D_OPTION_STATUS_CAPACITY:
        return "the schema array is full";
    case D_OPTION_STATUS_VALUE_CAPACITY:
        return "the value block is full";
    case D_OPTION_STATUS_NO_MEMORY:
        return "an allocation failed";
    }

    return "unknown status";
}


// ===========================================================================
// II.  THE CELL
// ===========================================================================

struct d_option
d_option_make(
    uint64_t      _key,
    d_type_info16 _key_type,
    d_type_info16 _value_type,
    uint32_t      _value_offset,
    uint32_t      _value_size,
    uint32_t      _flags
)
{
    struct d_option option;

    option.key          = _key;
    option.value_offset = _value_offset;
    option.value_size   = _value_size;
    option.key_type     = _key_type;
    option.value_type   = _value_type;
    option.flags        = (_flags & D_OPTION_FLAG_MASK);

    return option;
}

struct d_option
d_option_make_unary(
    uint64_t      _key,
    d_type_info16 _key_type
)
{
    return d_option_make(_key, _key_type, (d_type_info16)0,
                         0u, 0u, D_OPTION_FLAG_UNARY);
}

bool
d_option_is_valid(
    const struct d_option* _option
)
{
    if (_option == NULL)
    {
        return false;
    }

    //   reserved bits must be zero, so an old cell is distinguishable from a
    // corrupt one.
    if ((_option->flags & ~D_OPTION_FLAG_MASK) != 0u)
    {
        return false;
    }

    //   a unary option occupies no bytes; a valued one occupies some.
    if ((_option->flags & D_OPTION_FLAG_UNARY) != 0u)
    {
        return (_option->value_size == 0u);
    }

    return (_option->value_size > 0u);
}

bool
d_option_is_unary(
    const struct d_option* _option
)
{
    return ( (_option != NULL) &&
             ((_option->flags & D_OPTION_FLAG_UNARY) != 0u) );
}

bool
d_option_is_assigned(
    const struct d_option* _option
)
{
    return ( (_option != NULL) &&
             ((_option->flags & D_OPTION_FLAG_ASSIGNED) != 0u) );
}

bool
d_option_has_interned_key(
    const struct d_option* _option
)
{
    return ( (_option != NULL) &&
             ((_option->flags & D_OPTION_FLAG_INTERNED_KEY) != 0u) );
}

uint32_t
d_option_value_size(
    const struct d_option* _option
)
{
    return ( (_option != NULL) ? _option->value_size : 0u );
}


// ===========================================================================
// III. ORDERING -- KEY ONLY
// ===========================================================================

int
d_option_key_less(
    const void* _lhs,
    const void* _rhs,
    void*       _context
)
{
    const struct d_option* lhs;
    const struct d_option* rhs;

    (void)_context;

    lhs = (const struct d_option*)_lhs;
    rhs = (const struct d_option*)_rhs;

    if ((lhs == NULL) || (rhs == NULL))
    {
        return 0;
    }

    //   subtraction would overflow on 64-bit keys, so compare rather than
    // arithmetic.
    if (lhs->key < rhs->key)
    {
        return -1;
    }

    return ( (lhs->key > rhs->key) ? 1 : 0 );
}

bool
d_option_key_eq(
    const struct d_option* _lhs,
    const struct d_option* _rhs
)
{
    if ((_lhs == NULL) || (_rhs == NULL))
    {
        return false;
    }

    return (_lhs->key == _rhs->key);
}


// ===========================================================================
// IV.  THE CARRIER AND THE PROJECTION
// ===========================================================================

struct d_option_carrier
d_option_project_value(
    const struct d_option* _option,
    const unsigned char*   _values,
    void*                  _context
)
{
    struct d_option_carrier carrier;

    (void)_context;

    carrier.data      = NULL;
    carrier.size      = 0u;
    carrier.has_value = 0u;

    //   a unary option projects to the absent carrier; so does a valued one
    // whose block is missing, since there is no value of the requested kind
    // to be had either way.
    if ( (_option == NULL) ||
         (_values == NULL) ||
         ((_option->flags & D_OPTION_FLAG_UNARY) != 0u) ||
         (_option->value_size == 0u) )
    {
        return carrier;
    }

    carrier.data      = (const void*)(_values + _option->value_offset);
    carrier.size      = _option->value_size;
    carrier.has_value = 1u;

    return carrier;
}

bool
d_option_carrier_eq(
    const struct d_option_carrier* _lhs,
    const struct d_option_carrier* _rhs,
    fn_binary_predicate            _compare,
    void*                          _context
)
{
    if ((_lhs == NULL) || (_rhs == NULL))
    {
        return false;
    }

    //   the .tex's structural rule, in three lines: absent equals absent,
    // absent differs from present, present compares.
    if ((_lhs->has_value == 0u) || (_rhs->has_value == 0u))
    {
        return (_lhs->has_value == _rhs->has_value);
    }

    //   a width disagreement is a value disagreement. Comparing the common
    // prefix would report two differently-typed slots as equal.
    if (_lhs->size != _rhs->size)
    {
        return false;
    }

    if (_compare != NULL)
    {
        return _compare(_lhs->data, _rhs->data, _context);
    }

    return (memcmp(_lhs->data, _rhs->data, (size_t)_lhs->size) == 0);
}


// ===========================================================================
// V.   IDENTITY -- KEY AND VALUE
// ===========================================================================

bool
d_option_value_eq(
    const struct d_option* _lhs,
    const unsigned char*   _lhs_values,
    const struct d_option* _rhs,
    const unsigned char*   _rhs_values,
    fn_option_project      _project,
    fn_binary_predicate    _compare,
    void*                  _context
)
{
    fn_option_project       project;
    struct d_option_carrier lhs_carrier;
    struct d_option_carrier rhs_carrier;

    //   NULL selects pi = id, which is a documented default rather than a
    // silent one -- see option_common.h section V.
    project = ( (_project != NULL) ? _project : &d_option_project_value );

    lhs_carrier = project(_lhs, _lhs_values, _context);
    rhs_carrier = project(_rhs, _rhs_values, _context);

    return d_option_carrier_eq(&lhs_carrier, &rhs_carrier, _compare, _context);
}

bool
d_option_eq(
    const struct d_option* _lhs,
    const unsigned char*   _lhs_values,
    const struct d_option* _rhs,
    const unsigned char*   _rhs_values,
    fn_option_project      _project,
    fn_binary_predicate    _compare,
    void*                  _context
)
{
    if (!d_option_key_eq(_lhs, _rhs))
    {
        return false;
    }

    return d_option_value_eq(_lhs, _lhs_values, _rhs, _rhs_values,
                             _project, _compare, _context);
}


// ===========================================================================
// VI.  SLOT ACCESS
// ===========================================================================

void*
d_option_slot(
    const struct d_option* _option,
    unsigned char*         _values
)
{
    if ( (_option == NULL) ||
         (_values == NULL) ||
         (_option->value_size == 0u) )
    {
        return NULL;
    }

    return (void*)(_values + _option->value_offset);
}

const void*
d_option_slot_const(
    const struct d_option* _option,
    const unsigned char*   _values
)
{
    if ( (_option == NULL) ||
         (_values == NULL) ||
         (_option->value_size == 0u) )
    {
        return NULL;
    }

    return (const void*)(_values + _option->value_offset);
}

struct d_option_result
d_option_read(
    const struct d_option* _option,
    const unsigned char*   _values,
    void*                  _out,
    size_t                 _out_size
)
{
    const void* slot;

    if ((_option == NULL) || (_out == NULL))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if ((_option->flags & D_OPTION_FLAG_UNARY) != 0u)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_NO_VALUE);
    }

    //   the width is checked rather than trusted: a short read would return
    // a truncated value under a success status.
    if ((size_t)_option->value_size != _out_size)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_VALUE_TYPE_MISMATCH);
    }

    slot = d_option_slot_const(_option, _values);

    if (slot == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_NO_VALUE);
    }

    memcpy(_out, slot, _out_size);

    return d_option_ok((uint32_t)_out_size);
}

struct d_option_result
d_option_write(
    struct d_option* _option,
    unsigned char*   _values,
    const void*      _value,
    size_t           _value_size
)
{
    void* slot;

    if ((_option == NULL) || (_value == NULL))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if ((_option->flags & D_OPTION_FLAG_UNARY) != 0u)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_NO_VALUE);
    }

    if ((size_t)_option->value_size != _value_size)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_VALUE_TYPE_MISMATCH);
    }

    slot = d_option_slot(_option, _values);

    if (slot == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_NO_VALUE);
    }

    memcpy(slot, _value, _value_size);

    _option->flags |= D_OPTION_FLAG_ASSIGNED;

    return d_option_ok((uint32_t)_value_size);
}
