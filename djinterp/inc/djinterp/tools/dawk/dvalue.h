/******************************************************************************
* djinterp [dawk]                                                     dvalue.h
*
*   awk's scalar model, its conversion rules, and its associative arrays.
*     The subtlety here is not the representation but the comparison. A value
* that came from input and looks like a number compares numerically; a string
* constant of identical text does not. An uninitialised value is the empty
* string and zero at once. Those two rules, crossed against every tag, are the
* whole of section 4, and they are where reimplementations of awk fail.
*     One tag is added to the POSIX five. An extern value is an opaque pointer
* plus a callback yielding its text, so a host layer's tree node is a first
* class awk value and every builtin works on it unchanged.
*
*
* path:      /inc/djinterp/tools/dawk/dvalue.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  D_AWK_CONVFMT_DEFAULT
         2.  D_AWK_OFMT_DEFAULT
         3.  D_AWK_SUBSEP_DEFAULT
    2.  Types
         1.  d_awk_value_tag
         2.  d_awk_value
         3.  d_awk_array
         4.  d_awk_fn_extern_text
2.  SCALARS
    -------
    1.  Lifecycle
    2.  Assignment
    3.  Inspection
    4.  Conversion
    5.  Comparison
3.  ASSOCIATIVE ARRAYS
    ------------------
    1.  Lifecycle
    2.  Element access
    3.  Iteration
    4.  Subscripts
*/

#ifndef DJINTERP_TOOLS_DAWK_DVALUE_H
#define DJINTERP_TOOLS_DAWK_DVALUE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================
// The scalar tags, the opaque scalar and array handles, and the default
// formats a caller supplies when none has been set by the program.


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_AWK_CONVFMT_DEFAULT
//   constant: initial value of CONVFMT, the format used when a number is
// converted to a string other than for output.
#define D_AWK_CONVFMT_DEFAULT "%.6g"

// 1.1.2
// D_AWK_OFMT_DEFAULT
//   constant: initial value of OFMT, the format used when print converts a
// number to a string.
#define D_AWK_OFMT_DEFAULT "%.6g"

// 1.1.3
// D_AWK_SUBSEP_DEFAULT
//   constant: initial value of SUBSEP, the byte joining the parts of a
// multi-dimensional subscript.  POSIX specifies "\034".
#define D_AWK_SUBSEP_DEFAULT "\034"

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_awk_value_tag
//   enum: discriminates the scalar model.  STRNUM is the tag that carries
//   awk's comparison subtlety: it is text that came from input and has the
//   shape of a number, so it compares numerically where an identical STRING
//   would compare as text.  EXTERN is the host seam.
enum d_awk_value_tag
{
    D_AWK_VAL_UNINIT = 0,
    D_AWK_VAL_STRING,
    D_AWK_VAL_NUMBER,
    D_AWK_VAL_STRNUM,
    D_AWK_VAL_ARRAY,
    D_AWK_VAL_EXTERN
};

// 1.2.2
// d_awk_value
//   struct: one scalar or array cell.  Layout is private; use the accessors
//   in sections 2 and 3.
struct d_awk_value;

// 1.2.3
// d_awk_array
//   struct: an associative array.  Keys are byte strings and may contain any
//   byte, including the subscript separator.
struct d_awk_array;

// 1.2.4
// d_awk_fn_extern_text
//   type: yields the text of an extern value.  The returned buffer must stay
//   valid until the value is reassigned or released.
typedef const char*
(*d_awk_fn_extern_text)(void*   _object,
                        void*   _user,
                        size_t* _out_length);


//==============================================================================
// 2.  SCALARS
//==============================================================================
// Assignment decides the tag, and the tag decides the comparison.  The two
// text-assigning entry points differ in exactly that: d_awk_value_set_string
// always yields a STRING, while d_awk_value_set_input applies the numeric
// string test and may yield a STRNUM.


// 2.1    Lifecycle
//------------------------------------------------------------------------------
struct d_awk_value* d_awk_value_new(void);
void                d_awk_value_free(struct d_awk_value* _value);

// 2.2    Assignment
//------------------------------------------------------------------------------
// Input-derived text -- a field, a getline result, an ARGV or ENVIRON element,
// a split() result, FILENAME -- goes through d_awk_value_set_input.  A string
// constant in the program text never does.
void                d_awk_value_set_uninit(struct d_awk_value* _value);
void                d_awk_value_set_number(struct d_awk_value* _value,
                                           double              _number);
bool                d_awk_value_set_string(struct d_awk_value* _value,
                                           const char*         _text,
                                           size_t              _length);
bool                d_awk_value_set_input(struct d_awk_value* _value,
                                          const char*         _text,
                                          size_t              _length);
void                d_awk_value_set_extern(struct d_awk_value*  _value,
                                           void*                _object,
                                           d_awk_fn_extern_text _text,
                                           void*                _user);
bool                d_awk_value_set_array(struct d_awk_value* _value,
                                          struct d_awk_array* _array);
bool                d_awk_value_copy(struct d_awk_value*       _target,
                                     const struct d_awk_value* _source);

// 2.3    Inspection
//------------------------------------------------------------------------------
enum d_awk_value_tag d_awk_value_tag_of(const struct d_awk_value* _value);
struct d_awk_array*  d_awk_value_array_of(const struct d_awk_value* _value);
bool                 d_awk_value_is_true(struct d_awk_value* _value);

// 2.4    Conversion
//------------------------------------------------------------------------------
// d_awk_value_text caches its result on the value, so the returned buffer is
// invalidated by the next assignment to that value.  An integral number
// converts as an integer whatever the format says, as POSIX requires.
const char*         d_awk_value_text(struct d_awk_value* _value,
                                     const char*         _format,
                                     size_t*             _out_length);
double              d_awk_value_number(struct d_awk_value* _value);
bool                d_awk_looks_numeric(const char* _text,
                                        size_t      _length,
                                        double*     _out_number);
double              d_awk_text_to_number(const char* _text,
                                         size_t      _length);
size_t              d_awk_number_to_text(double      _number,
                                         const char* _format,
                                         char*       _buffer,
                                         size_t      _capacity);

// 2.5    Comparison
//------------------------------------------------------------------------------
// Numeric when both operands are numeric, a numeric string, or uninitialised;
// otherwise textual, with any number converted through `_format`.
int                 d_awk_value_compare(struct d_awk_value* _left,
                                        struct d_awk_value* _right,
                                        const char*         _format);


//==============================================================================
// 3.  ASSOCIATIVE ARRAYS
//==============================================================================
// Referencing an element creates it; testing membership does not.  That is an
// observable awk behaviour, not an implementation detail, so the two are
// separate entry points rather than one with a flag.


// 3.1    Lifecycle
//------------------------------------------------------------------------------
struct d_awk_array* d_awk_array_new(void);
void                d_awk_array_free(struct d_awk_array* _array);
void                d_awk_array_clear(struct d_awk_array* _array);
size_t              d_awk_array_count(const struct d_awk_array* _array);

// 3.2    Element access
//------------------------------------------------------------------------------
struct d_awk_value* d_awk_array_lookup(struct d_awk_array* _array,
                                       const char*         _key,
                                       size_t              _length);
struct d_awk_value* d_awk_array_find(const struct d_awk_array* _array,
                                     const char*               _key,
                                     size_t                    _length);
bool                d_awk_array_delete(struct d_awk_array* _array,
                                       const char*         _key,
                                       size_t              _length);

// 3.3    Iteration
//------------------------------------------------------------------------------
// `_cursor` starts at zero and is advanced by each call.  Deleting during a
// walk is permitted; inserting during a walk is not.
bool                d_awk_array_next(const struct d_awk_array* _array,
                                     size_t*                   _cursor,
                                     const char**              _out_key,
                                     size_t*                   _out_length,
                                     struct d_awk_value**      _out_value);

// 3.4    Subscripts
//------------------------------------------------------------------------------
size_t              d_awk_subscript_join(char*              _buffer,
                                         size_t             _capacity,
                                         const char* const* _parts,
                                         const size_t*      _lengths,
                                         size_t             _count,
                                         const char*        _subsep);


#endif  // DJINTERP_TOOLS_DAWK_DVALUE_H
