/*******************************************************************************
* djinterp [c]                                                           table.h
*
*   TIER 1a -- the C FACE of the table.  Ergonomics only: macros and generated
* typed wrappers over the tier-0 kernel in table_common.h.  Not one line of
* table semantics lives here, and nothing in this file is required to use the
* table -- a caller content to write d_table_set(&t, r, c, &v) never includes
* it.  If a change to this header would change a cell, the change belongs in
* the core instead.
*
*   THE MODULE SPLIT, following the container subframework's own layout --
* `<mod>.h` beside `<mod>_common.h`, as array.h/array_common.h,
* vector.h/vector_common.h and registry.h/registry_common.h already do.  There
* is no umbrella: a consumer includes the face of its own language.
*
*     table_common.h / .c   the kernel.  The only place a table result is
*                           computed.
*     table.h       (this)  the C face -- what C code includes.
*     table.hpp             the C++ face -- what C++ code includes.
*
*   `_common` MARKS COMMONALITY OF ANY KIND -- across several modules, across
* the two languages, or both.  table_common.h is common in both ways at once:
* it is compiled by C and by C++, and it serves table, fixed_table and the
* overlay siblings from one declaration.  vector_common.h is common the other
* way, serving d_vector and d_vector_s within C.  One convention, one meaning.
*
*   C++ DOES NOT INCLUDE THIS FILE.  D_TABLE_PUSH_ROW below is a C99 compound
* literal, which ISO C++ has no equivalent for: including this header from C++
* is quiet (the macro is unexpanded), but USING that macro is an error under
* -pedantic-errors and a non-portable GNU extension without it -- measured,
* not
* assumed.  The C++ face wraps table_common.h directly and needs none of this.
*
*   NO table.c.  Everything here is a macro or a D_INLINE function, and
* D_INLINE is `static inline` in C, so this header exports no external symbol
* and needs no translation unit.
*
*   WHAT C GETS INSTEAD OF TEMPLATES.  C does not get templates, and no macro
* tower should pretend otherwise (goals 11).  What C does get is the same core
* objects and the same algebra over them, plus two conveniences that cost
* nothing at run time:
*
*     D_TABLE_DECLARE_TYPED(prefix, type) generates a family of `static
*   inline`
*                                          wrappers that forward to the kernel
*                                          with the cell size supplied and the
*                                          void* cast away.  Typed at the call
*                                          site, identical machine code.
*     D_TABLE_CELL(t, type, r, c) the same thing for a caller who does
*                                          not want a generated family.
*
*   These GENERATE nothing new.  Every wrapper is one forwarding call, so the
* Cost law holds by inspection: there is no state, no indirection, and (at any
* optimisation level above -O0) no call.
*
*   THE ALLOCATOR IS STILL THE CALLER'S. d_table_alloc_stdlib is the one place
* in the module where malloc is named, it is opt-in, and it is a plain value a
* caller passes or does not.  A freestanding build turns it off and loses
* nothing but the convenience.
*
*   PORTABILITY:
*   C99 baseline. The _Generic tier (D_TABLE_CELL_SIZE_OF) is C11 and degrades
* to sizeof, which is what it would have computed anyway.
*
*
* path:      /inc/djinterp/c/container/table/table.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.01
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    configuration           (the stdlib-strategy knob)
      --------------------------------------------------

II.   d_table_alloc_stdlib    (the one named malloc strategy)
      -------------------------------------------------------

III.  cell access macros      (typed access without a generated family)
      -----------------------------------------------------------------

IV.   D_TABLE_DECLARE_TYPED   (the generated typed family)
      ----------------------------------------------------

V.    iteration macros        (atomic order, row order, column order)
      ---------------------------------------------------------------
*/

#ifndef DJINTERP_C_CONTAINER_TABLE_TABLE_H
#define DJINTERP_C_CONTAINER_TABLE_TABLE_H 1

// djinterp
#include "./table_common.h"
#include "./table_domain.h"
#include "./table_carrier.h"
#include "./table_layout.h"
#include "./table_overlay.h"

#if ( (!defined(D_CFG_TABLE_STDLIB_ALLOC)) ||                                  \
      (D_CFG_IS_ON(D_CFG_TABLE_STDLIB_ALLOC)) )
    // std
    #include <stdlib.h>
#endif


D_EXTERN_C_BEGIN


// ===========================================================================
// I.   configuration
// ===========================================================================

// D_CFG_TABLE_STDLIB_ALLOC
//   macro: whether the convenience strategy backed by realloc/free is offered.
// On by default because naming a strategy is not hiding one; off for a
// freestanding build, which then supplies its own and loses nothing else.
#ifndef D_CFG_TABLE_STDLIB_ALLOC
    #define D_CFG_TABLE_STDLIB_ALLOC    1
#endif


// ===========================================================================
// II.  d_table_alloc_stdlib
// ===========================================================================

#if D_CFG_IS_ON(D_CFG_TABLE_STDLIB_ALLOC)

// d_table_internal_stdlib_realloc
//   function: the reallocate hook of the stdlib strategy.
D_INLINE void*
d_table_internal_stdlib_realloc(
    void*  _context,
    void*  _block,
    size_t _old_size,
    size_t _new_size
)
{
    (void)_context;
    (void)_old_size;

    return realloc(_block, _new_size);
}

// d_table_internal_stdlib_free
//   function: the release hook of the stdlib strategy.
D_INLINE void
d_table_internal_stdlib_free(
    void*  _context,
    void*  _block,
    size_t _size
)
{
    (void)_context;
    (void)_size;

    free(_block);

    return;
}

// d_table_alloc_stdlib
//   function: the storage strategy backed by realloc and free. The one place
// this module names the standard allocator, and it names it only because the
// caller asked for it by writing this call.
D_NODISCARD D_INLINE struct d_table_alloc
d_table_alloc_stdlib(void)
{
    struct d_table_alloc result;

    result.reallocate = d_table_internal_stdlib_realloc;
    result.release    = d_table_internal_stdlib_free;
    result.context    = NULL;

    return result;
}

#endif  // D_CFG_TABLE_STDLIB_ALLOC


// ===========================================================================
// III. cell access macros
// ===========================================================================

// D_TABLE_CELL_SIZE_OF
//   macro: the cell width to declare a table of _type with. A cast of sizeof,
// so it is a constant expression in every tier.
#define D_TABLE_CELL_SIZE_OF(_type)         sizeof(_type)

// D_TABLE_EMPTY_OF
//   macro: an empty table of _cols columns holding cells of _type.
#define D_TABLE_EMPTY_OF(_type, _cols) \
    d_table_empty_of((size_t)(_cols), D_TABLE_CELL_SIZE_OF(_type))

// D_TABLE_OVER
//   macro: a table of _type laid over the caller's buffer.
#define D_TABLE_OVER(_type, _cells, _rows, _cols) \
    d_table_over((void*)(_cells),                 \
                 (size_t)(_rows),                 \
                 (size_t)(_cols),                 \
                 D_TABLE_CELL_SIZE_OF(_type),     \
                 (size_t)(_rows))

// D_TABLE_CELL
//   macro: a _type* to the cell at (_r, _c), unchecked. The caller has
// established validity with d_table_contains.
#define D_TABLE_CELL(_table, _type, _r, _c) \
    ((_type*)d_table_cell((_table), (size_t)(_r), (size_t)(_c)))

// D_TABLE_CELL_CONST
//   macro: a const _type* to the cell at (_r, _c), unchecked.
#define D_TABLE_CELL_CONST(_table, _type, _r, _c) \
    ((const _type*)d_table_cell_const((_table), (size_t)(_r), (size_t)(_c)))

// D_TABLE_AT
//   macro: a _type* to the cell at (_r, _c), or NULL when the index is outside
// I_T -- the checked access, typed.
#define D_TABLE_AT(_table, _type, _r, _c) \
    ((_type*)d_table_at((_table), (size_t)(_r), (size_t)(_c)))

// D_TABLE_VALUE
//   macro: the value of the cell at (_r, _c), unchecked.
#define D_TABLE_VALUE(_table, _type, _r, _c) \
    (*D_TABLE_CELL_CONST((_table), _type, (_r), (_c)))

// D_TABLE_ROW_OF
//   macro: a _type* to the first cell of the rank-1 subtable T[_r].
#define D_TABLE_ROW_OF(_table, _type, _r) \
    ((_type*)d_table_row((_table), (size_t)(_r)))

#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_TABLE_PUSH_ROW
//   macro: append a row written as a braced initialiser, e.g.
//     D_TABLE_PUSH_ROW(&t, int, &alloc, {1, 2, 3}); The width is deduced from
// the literal, so a row of the wrong length is a SHAPE error at run time
// rather than a silent truncation. C99 compound literal; the array's lifetime
// is the enclosing block, which outlives the call.
#define D_TABLE_PUSH_ROW(_table, _type, _alloc, ...)          \
    d_table_push_row(                                         \
        (_table),                                             \
        (const void*)((const _type[])__VA_ARGS__),            \
        (sizeof((const _type[])__VA_ARGS__) / sizeof(_type)), \
        (_alloc))
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS


// ===========================================================================
// IV.  D_TABLE_DECLARE_TYPED
// ===========================================================================

// D_TABLE_DECLARE_TYPED
//   macro: generate a typed wrapper family over the kernel for cells of _type,
// every name carrying _prefix. The generated functions are the C answer to
// what C++ spells table<_type>: the same objects, the same algebra, the type
// restored at the call site, and no state of their own.
//
//     D_TABLE_DECLARE_TYPED(d_int_table, int)
//
//     struct d_table t = d_int_table_empty(3);
//     d_int_table_push(&t, row, 3, &alloc);
//     int v = d_int_table_get(&t, 1, 2); The table is still a struct d_table
// -- the family adds no type, so a typed
// call site and an untyped one interoperate, and the lowered form stays the
// one the interpreter and the wire format already know.
#define D_TABLE_DECLARE_TYPED(_prefix, _type)                              \
                                                                           \
    D_NODISCARD D_INLINE struct d_table                                    \
    _prefix##_empty(size_t _cols)                                          \
    {                                                                      \
        return d_table_empty_of(_cols, D_TABLE_CELL_SIZE_OF(_type));       \
    }                                                                      \
                                                                           \
    D_NODISCARD D_INLINE struct d_table                                    \
    _prefix##_over(_type* _cells,                                          \
                   size_t _rows,                                           \
                   size_t _cols)                                           \
    {                                                                      \
        return d_table_over2((void*)_cells, _rows, _cols,                  \
                             D_TABLE_CELL_SIZE_OF(_type), _rows);          \
    }                                                                      \
                                                                           \
    D_NODISCARD D_INLINE _type                                             \
    _prefix##_get(const struct d_table* _table,                            \
                  size_t                _row,                              \
                  size_t                _col)                              \
    {                                                                      \
        return *(const _type*)d_table_cell_const(_table, _row, _col);      \
    }                                                                      \
                                                                           \
    D_INLINE enum d_table_status                                           \
    _prefix##_set(struct d_table* _table,                                  \
                  size_t          _row,                                    \
                  size_t          _col,                                    \
                  _type           _value)                                  \
    {                                                                      \
        return d_table_set(_table, _row, _col, (const void*)&_value);      \
    }                                                                      \
                                                                           \
    D_NODISCARD D_INLINE _type*                                            \
    _prefix##_at(struct d_table* _table,                                   \
                 size_t          _row,                                     \
                 size_t          _col)                                     \
    {                                                                      \
        return (_type*)d_table_at(_table, _row, _col);                     \
    }                                                                      \
                                                                           \
    D_NODISCARD D_INLINE _type*                                            \
    _prefix##_row(struct d_table* _table,                                  \
                  size_t          _row)                                    \
    {                                                                      \
        return (_type*)d_table_row(_table, _row);                          \
    }                                                                      \
                                                                           \
    D_INLINE enum d_table_status                                           \
    _prefix##_push(struct d_table*             _table,                     \
                   const _type*                _cells,                     \
                   size_t                      _width,                     \
                   const struct d_table_alloc* _alloc)                     \
    {                                                                      \
        return d_table_push_row(_table, (const void*)_cells,               \
                                _width, _alloc);                           \
    }                                                                      \
                                                                           \
    D_INLINE enum d_table_status                                           \
    _prefix##_fill(struct d_table* _table,                                 \
                   _type           _value)                                 \
    {                                                                      \
        return d_table_fill(_table, (const void*)&_value);                 \
    }                                                                      \
                                                                           \
    D_NODISCARD D_INLINE const _type*                                      \
    _prefix##_resolve(const struct d_table*        _table,                 \
                      const struct d_table_layout* _layout,                \
                      size_t                       _row,                   \
                      size_t                       _col)                   \
    {                                                                      \
        return (const _type*)d_table_resolve(_table, _layout, _row, _col); \
    }


// ===========================================================================
// V.   iteration macros
// ===========================================================================

// D_TABLE_FOR_EACH_ROW
//   macro: sweep the rows in order, binding _r to each row index. The row
// order is the first coordinate of the lexicographic order.
#define D_TABLE_FOR_EACH_ROW(_table, _r) \
    for ((_r) = 0; (_r) < (size_t)(_table)->rows; ++(_r))

// D_TABLE_FOR_EACH_COLUMN
//   macro: sweep the columns in order, binding _c to each column index.
#define D_TABLE_FOR_EACH_COLUMN(_table, _c) \
    for ((_c) = 0; (_c) < (size_t)(_table)->cols; ++(_c))

// D_TABLE_FOR_EACH_CELL
//   macro: sweep every atomic position in the table's own order -- row-major,
// which is the lexicographic order on multi-indices. Binds _r and _c.
#define D_TABLE_FOR_EACH_CELL(_table, _r, _c) \
    D_TABLE_FOR_EACH_ROW((_table), (_r))      \
        D_TABLE_FOR_EACH_COLUMN((_table), (_c))


D_EXTERN_C_END


#endif  // DJINTERP_C_CONTAINER_TABLE_TABLE_H
