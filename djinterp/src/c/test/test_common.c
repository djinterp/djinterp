#include "../../../inc/c/test/test_common.h"


/******************************************************************************
 * TEST OBJECT FUNCTIONS
 *****************************************************************************/

struct d_test_object*
d_test_object_new
(
    bool  _is_leaf,
    bool  _result,
    void* _context
)
{
    struct d_test_object* obj;

    obj = (struct d_test_object*)malloc(
              sizeof(struct d_test_object));

    if (!obj)
    {
        return NULL;
    }

    obj->is_leaf = _is_leaf;
    obj->result  = _result;
    obj->context = _context;
    obj->args    = NULL;

    return obj;
}

void
d_test_object_free
(
    struct d_test_object* _obj
)
{
    if (!_obj)
    {
        return;
    }

    if (_obj->args)
    {
        d_test_arg_list_free(_obj->args);
    }

    free(_obj);

    return;
}

void
d_test_object_set_args
(
    struct d_test_object*   _obj,
    struct d_test_arg_list* _args
)
{
    if (!_obj)
    {
        return;
    }

    if (_obj->args)
    {
        d_test_arg_list_free(_obj->args);
    }

    _obj->args = _args;

    return;
}


/******************************************************************************
 * TEST COUNTER FUNCTIONS
 *****************************************************************************/

/*
d_test_counter_init
  Initializes a counter from a packed spec (see D_TEST_COUNTER_SPEC).
Extracts test_type and fields from the spec and heap-allocates a
compact amount array with exactly popcount(fields) elements, all
initialized to zero.

Parameter(s):
  _counter: counter to initialize
  _spec:    packed counter spec (D_TEST_COUNTER_SPEC(type, fields))
Return:
  0 on success, -1 on failure (NULL counter or allocation failure).
*/
int
d_test_counter_init
(
    struct d_test_counter* _counter,
    uint16_t               _spec
)
{
    uint16_t type;
    uint16_t fields;
    uint16_t slot_count;

    if (!_counter)
    {
        return -1;
    }

    type       = D_TEST_COUNTER_SPEC_TYPE(_spec);
    fields     = D_TEST_COUNTER_SPEC_FIELDS(_spec);
    slot_count = D_TEST_COUNTER_FIELD_COUNT(fields);

    _counter->test_type = type;
    _counter->fields    = fields;
    _counter->amount    = NULL;

    if (slot_count > 0)
    {
        _counter->amount = (size_t*)calloc(
                               slot_count,
                               sizeof(size_t));

        if (!_counter->amount)
        {
            return -1;
        }
    }

    return 0;
}

/*
d_test_counter_free
  Frees the heap-allocated amount array and zeros the counter.
Safe to call on a counter that was never initialized or already freed.

Parameter(s):
  _counter: counter to free; may be NULL.
Return:
  none.
*/
void
d_test_counter_free
(
    struct d_test_counter* _counter
)
{
    if (!_counter)
    {
        return;
    }

    if (_counter->amount)
    {
        free(_counter->amount);
        _counter->amount = NULL;
    }

    _counter->test_type = 0;
    _counter->fields    = 0;

    return;
}

/*
d_test_counter_reset
  Zeros all active amount slots without reallocating.

Parameter(s):
  _counter: counter to reset; may be NULL.
Return:
  none.
*/
void
d_test_counter_reset
(
    struct d_test_counter* _counter
)
{
    uint16_t slot_count;

    if ( (!_counter) ||
         (!_counter->amount) )
    {
        return;
    }

    slot_count = D_TEST_COUNTER_FIELD_COUNT(
                     _counter->fields);

    memset(_counter->amount, 0,
           slot_count * sizeof(size_t));

    return;
}

/*
d_test_counter_increment
  Increments the amount slot for a single field flag by 1.
No-op if the field is not active in this counter's bitmask.

Parameter(s):
  _counter: counter to update
  _field:   a single DTestCountField flag (must be a power of 2)
Return:
  none.
*/
void
d_test_counter_increment
(
    struct d_test_counter* _counter,
    uint16_t               _field
)
{
    uint16_t idx;

    if ( (!_counter) ||
         (!_counter->amount) )
    {
        return;
    }

    if (!D_TEST_COUNTER_FIELD_ACTIVE(
             _counter->fields, _field))
    {
        return;
    }

    idx = D_TEST_COUNTER_FIELD_INDEX(
              _counter->fields, _field);

    _counter->amount[idx]++;

    return;
}

/*
d_test_counter_get
  Returns the current value of a single field flag's amount slot.
Returns 0 if the field is not active or the counter is NULL.

Parameter(s):
  _counter: counter to read
  _field:   a single DTestCountField flag (must be a power of 2)
Return:
  The amount for that field, or 0 if inactive/NULL.
*/
size_t
d_test_counter_get
(
    const struct d_test_counter* _counter,
    uint16_t                     _field
)
{
    uint16_t idx;

    if ( (!_counter) ||
         (!_counter->amount) )
    {
        return 0;
    }

    if (!D_TEST_COUNTER_FIELD_ACTIVE(
             _counter->fields, _field))
    {
        return 0;
    }

    idx = D_TEST_COUNTER_FIELD_INDEX(
              _counter->fields, _field);

    return _counter->amount[idx];
}

/*
d_test_counter_add
  Adds the source counter's values into the destination for all
fields that are active in both. The two counters must have the same
test_type; mismatched types are a no-op.

Parameter(s):
  _dest:   destination counter (modified)
  _source: source counter (read-only)
Return:
  none.
*/
void
d_test_counter_add
(
    struct d_test_counter*       _dest,
    const struct d_test_counter* _source
)
{
    uint16_t common;
    uint16_t flag;
    uint16_t d_idx;
    uint16_t s_idx;
    uint16_t i;

    if ( (!_dest)           ||
         (!_source)         ||
         (!_dest->amount)   ||
         (!_source->amount) )
    {
        return;
    }

    if (_dest->test_type != _source->test_type)
    {
        return;
    }

    common = _dest->fields & _source->fields;

    for (i = 0; i < D_TEST_COUNT_FIELD_MAX; i++)
    {
        flag = (uint16_t)(1u << i);

        if (!(common & flag))
        {
            continue;
        }

        d_idx = D_TEST_COUNTER_FIELD_INDEX(
                    _dest->fields, flag);
        s_idx = D_TEST_COUNTER_FIELD_INDEX(
                    _source->fields, flag);

        _dest->amount[d_idx] +=
            _source->amount[s_idx];
    }

    return;
}


/******************************************************************************
 * ARG LIST UTILITY FUNCTIONS
 *****************************************************************************/

struct d_test_arg_list*
d_test_arg_list_new
(
    size_t _capacity
)
{
    struct d_test_arg_list* list;

    list = (struct d_test_arg_list*)malloc(
               sizeof(struct d_test_arg_list));

    if (!list)
    {
        return NULL;
    }

    list->count    = 0;
    list->capacity = _capacity;

    if (_capacity > 0)
    {
        list->args = (struct d_test_arg*)calloc(
                         _capacity,
                         sizeof(struct d_test_arg));

        if (!list->args)
        {
            free(list);

            return NULL;
        }
    }
    else
    {
        list->args = NULL;
    }

    return list;
}

void
d_test_arg_list_free
(
    struct d_test_arg_list* _list
)
{
    if (!_list)
    {
        return;
    }

    if (_list->args)
    {
        free(_list->args);
    }

    free(_list);

    return;
}

/*
d_test_arg_list_set
  Sets or updates an argument in the list by key. If the key already
exists, its value is updated. Otherwise, the argument is appended.
Grows the backing array if needed.
*/
int
d_test_arg_list_set
(
    struct d_test_arg_list* _list,
    int16_t                 _key,
    void*                   _value
)
{
    size_t            i;
    size_t            new_capacity;
    struct d_test_arg* new_args;

    if (!_list)
    {
        return -1;
    }

    // search for existing key
    for (i = 0; i < _list->count; i++)
    {
        if (_list->args[i].key == _key)
        {
            _list->args[i].value = _value;

            return 0;
        }
    }

    // grow if needed
    if (_list->count >= _list->capacity)
    {
        new_capacity = (_list->capacity > 0)
                           ? _list->capacity * 2
                           : 8;

        new_args = (struct d_test_arg*)realloc(
                       _list->args,
                       new_capacity *
                           sizeof(struct d_test_arg));

        if (!new_args)
        {
            return -1;
        }

        _list->args     = new_args;
        _list->capacity = new_capacity;
    }

    // append new entry
    _list->args[_list->count].key   = _key;
    _list->args[_list->count].value = _value;
    _list->count++;

    return 0;
}

void*
d_test_arg_list_get
(
    const struct d_test_arg_list* _list,
    int16_t                       _key
)
{
    size_t i;

    if ( (!_list) ||
         (!_list->args) )
    {
        return NULL;
    }

    for (i = 0; i < _list->count; i++)
    {
        if (_list->args[i].key == _key)
        {
            return _list->args[i].value;
        }
    }

    return NULL;
}


/******************************************************************************
 * ARG LIST TYPED CONVENIENCE FUNCTIONS
 *****************************************************************************/

bool
d_test_arg_list_get_bool
(
    const struct d_test_arg_list* _list,
    int16_t                       _key,
    bool                          _default
)
{
    void* val;

    val = d_test_arg_list_get(_list, _key);

    if (!val && !_default)
    {
        // distinguish "not found" from "stored false"
        // by checking if key exists at all
        size_t i;

        if ( (!_list) ||
             (!_list->args) )
        {
            return _default;
        }

        for (i = 0; i < _list->count; i++)
        {
            if (_list->args[i].key == _key)
            {
                return D_TEST_ARG_AS_BOOL(
                           _list->args[i].value);
            }
        }

        return _default;
    }

    return val ? D_TEST_ARG_AS_BOOL(val) : _default;
}

size_t
d_test_arg_list_get_size
(
    const struct d_test_arg_list* _list,
    int16_t                       _key,
    size_t                        _default
)
{
    void* val;

    val = d_test_arg_list_get(_list, _key);

    return val ? D_TEST_ARG_AS_SIZE(val) : _default;
}

intptr_t
d_test_arg_list_get_intptr
(
    const struct d_test_arg_list* _list,
    int16_t                       _key,
    intptr_t                      _default
)
{
    void* val;

    val = d_test_arg_list_get(_list, _key);

    return val ? D_TEST_ARG_AS_INTPTR(val) : _default;
}

const char*
d_test_arg_list_get_str
(
    const struct d_test_arg_list* _list,
    int16_t                       _key,
    const char*                   _default
)
{
    void* val;

    val = d_test_arg_list_get(_list, _key);

    return val ? (const char*)val : _default;
}


/******************************************************************************
 * METADATA UTILITY FUNCTIONS
 *****************************************************************************/

const char*
d_test_metadata_get
(
    const struct d_test_metadata* _meta,
    uint16_t                      _key
)
{
    size_t lo;
    size_t hi;
    size_t mid;

    if ( (!_meta)          ||
         (!_meta->entries) ||
         (_meta->count == 0) )
    {
        return NULL;
    }

    lo = 0;
    hi = _meta->count;

    while (lo < hi)
    {
        mid = lo + (hi - lo) / 2;

        if (_meta->entries[mid].key == _key)
        {
            return _meta->entries[mid].value;
        }
        else if (_meta->entries[mid].key < _key)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }

    return NULL;
}

/*
d_test_arg_list_get_meta
  Convenience: retrieves a metadata string from the D_TEST_ARG_METADATA
entry on an arg_list. Returns _default if the arg_list, metadata, or
key is not found.
*/
const char*
d_test_arg_list_get_meta
(
    const struct d_test_arg_list* _list,
    uint16_t                      _meta_key,
    const char*                   _default
)
{
    struct d_test_metadata* meta;
    const char*             result;

    if (!_list)
    {
        return _default;
    }

    meta = (struct d_test_metadata*)
               d_test_arg_list_get(
                   _list, D_TEST_ARG_METADATA);

    if (!meta)
    {
        return _default;
    }

    result = d_test_metadata_get(meta, _meta_key);

    return result ? result : _default;
}
