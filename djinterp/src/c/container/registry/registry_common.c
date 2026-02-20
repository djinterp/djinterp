#include "..\..\..\inc\container\registry\registry_common.h"



//==============================================================================
// d_registry_strcmp
//==============================================================================

// d_internal_ascii_tolower
//   function: ASCII-only lowercase conversion (locale-independent).
static unsigned char
d_internal_ascii_tolower
(
    unsigned char _c
)
{
    if ( (_c >= (unsigned char)'A') && (_c <= (unsigned char)'Z') )
    {
        return (unsigned char)(_c + (unsigned char)('a' - 'A'));
    }

    return _c;
}

// d_registry_strcmp
//   function: strcmp with optional ASCII case-insensitivity.
//   notes:
//     - NULL handling:
//         * both NULL => 0
//         * NULL < non-NULL
int
d_registry_strcmp
(
    const char* _a,
    const char* _b,
    bool        _case_sensitive
)
{
    if ( _a == _b )
    {
        return 0;
    }

    if ( !_a )
    {
        return -1;
    }

    if ( !_b )
    {
        return 1;
    }

    if ( _case_sensitive )
    {
        return strcmp(_a, _b);
    }

    {
        const unsigned char* a = (const unsigned char*)_a;
        const unsigned char* b = (const unsigned char*)_b;

        while ( (*a != 0u) && (*b != 0u) )
        {
            const unsigned char ca = d_internal_ascii_tolower(*a);
            const unsigned char cb = d_internal_ascii_tolower(*b);

            if ( ca != cb )
            {
                return (int)ca - (int)cb;
            }

            ++a;
            ++b;
        }

        // One (or both) ended: compare terminators too (like strcmp).
        return (int)d_internal_ascii_tolower(*a) - (int)d_internal_ascii_tolower(*b);
    }
}


//==============================================================================
// d_registry_schema_max_enum_key
//==============================================================================

// d_registry_schema_max_enum_key
//   function: finds maximum enum_key in schema rows.
//   return:
//     max enum_key; returns 0 if schema is NULL or schema_count is 0.
uint16_t
d_registry_schema_max_enum_key
(
    const struct d_registry_schema_row* _schema,
    size_t                              _schema_count
)
{
    size_t   i;
    uint16_t max_key;

    if ( (!_schema) || (_schema_count == 0u) )
    {
        return 0u;
    }

    max_key = _schema[0].enum_key;

    for ( i = 1u; i < _schema_count; ++i )
    {
        const uint16_t k = _schema[i].enum_key;

        if ( k > max_key )
        {
            max_key = k;
        }
    }

    return max_key;
}
