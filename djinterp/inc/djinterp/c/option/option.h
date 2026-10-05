/*******************************************************************************
* djinterp [c]                                                          option.h
*
* The C face of the option module. Tier 1a -- C ERGONOMICS over the tier 0
* core, adding no semantics of its own.
*
* WHAT A FACE IS ALLOWED TO BE.
*   Everything here expands to a call into `option_common.h` or to a brace
* initialiser for `struct d_option`. Nothing computes an answer the core does
* not already compute. If a macro below were deleted, every program using it
* could be rewritten by hand against the core -- longer, but identical. That
* is the test a face has to pass, and it is the same test the C++ face passes
* by deriving rather than reimplementing.
*
* THE SCHEMA TABLE IS THE POINT OF THIS HEADER.
*   The rest is convenience. `D_OPTION_SCHEMA` is the single declaration of a
* key set -- its keys, their value types, their defaults, their flags -- and
* it is an X-macro, so it can be expanded into an enum, into a cell table,
* into a registry, into CLI binding, and into documentation, from one source.
*
*   This matters beyond ergonomics. The testing roadmap's cvar stage asks for
* exactly this and says why: the C test framework and the C++ option registry
* each grew their own key declaration, and the two now have to be reconciled.
* A schema declared once and read by both languages is the reconciliation, and
* the C macro table is the natural place for it because C++ can expand a C
* macro and C cannot expand a C++ template.
*
*   The archive work reached the same shape from the other direction: fifty
* option fields whose comparison was re-derived by hand at every boundary,
* collapsed into one loop over a knob table. A schema table is that knob table,
* declared rather than discovered.
*
* _Generic IS USED WHERE IT EXISTS AND SKIPPED WHERE IT DOES NOT.
*   The typed constructors below take the value's TYPE as an argument, so they
* work at C99. The `_Generic` forms take the value's EXPRESSION and deduce the
* type, which is nicer and needs C11. Per the Tier law the C11 forms are
* ABSENT below C11 rather than broken there, and the C99 forms remain the
* portable path -- a caller that uses only those compiles everywhere.
*
*
* path:      /inc/djinterp/c/option/option.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_OPTION_OPTION_H
#define DJINTERP_C_OPTION_OPTION_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "../meta/type_info.h"
#include "./option_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN

///////////////////////////////////////////////////////////////////////////////
///             I.    KEY AND TYPE CAPTURE                                  ///
///////////////////////////////////////////////////////////////////////////////

// D_OPTION_KEY
//   macro: widen a key to the core's key representation.
//   Enumeration constants are integral, so this is a cast -- but it is a cast
// with a name, and the name is what stops a caller reaching for `(uint64_t)`
// directly and thereby losing the one place a future interning step would
// need to be inserted. A class-type key has no integral value and cannot use
// this macro; see D_OPTION_FLAG_INTERNED_KEY in option_common.h.
#define D_OPTION_KEY(key)               ((uint64_t)(key))

// D_OPTION_TYPE
//   macro: the type descriptor for a TYPE NAME. C99-clean, because it names
// the type rather than deducing it.
#define D_OPTION_TYPE(T)                D_TYPE_OF_TYPE(T)

// D_OPTION_SIZE / D_OPTION_ALIGN
//   macro: the slot width and alignment a value type needs. Alignment is
// computed by the offsetof trick rather than by `_Alignof` so the spelling is
// the same at C99 and C11.
#define D_OPTION_SIZE(T)                ((uint32_t)sizeof(T))
#define D_OPTION_ALIGN(T)                                                    \
    ((uint32_t)offsetof(struct { char c_; T t_; }, t_))


///////////////////////////////////////////////////////////////////////////////
///             II.   TYPED CELL CONSTRUCTION                               ///
///////////////////////////////////////////////////////////////////////////////

// D_OPTION_CELL
//   macro: a `struct d_option` initialiser for a key of type `KT` carrying a
// value of type `VT`, at a given offset. The offset is normally assigned by
// `d_option_set_add`, so this form is for STATIC tables -- a schema laid out
// at translation time, which is the C analogue of what the C++ face computes
// from its option pack.
#define D_OPTION_CELL(key, KT, VT, offset)                                    \
    D_OPTION_INIT(D_OPTION_KEY(key),                                          \
                  D_OPTION_TYPE(KT),                                          \
                  D_OPTION_TYPE(VT),                                          \
                  (offset),                                                   \
                  D_OPTION_SIZE(VT),                                          \
                  D_OPTION_FLAG_NONE)

// D_OPTION_CELL_UNARY
//   macro: a presence-only cell -- no value, no slot, no width.
#define D_OPTION_CELL_UNARY(key, KT)                                          \
    D_OPTION_INIT(D_OPTION_KEY(key),                                          \
                  D_OPTION_TYPE(KT),                                          \
                  (d_type_info16)0,                                           \
                  0u,                                                         \
                  0u,                                                         \
                  D_OPTION_FLAG_UNARY)


///////////////////////////////////////////////////////////////////////////////
///             III.  THE SCHEMA TABLE                                      ///
///////////////////////////////////////////////////////////////////////////////
//
//   A schema is declared ONCE, as an X-macro, and expanded into whatever a
// given translation unit needs. The contract for the callback is fixed:
//
//     X(NAME, KEY, KEY_TYPE, VALUE_TYPE, DEFAULT, FLAGS)
//
//       NAME        identifier fragment -- the key's spelling in generated
//                   symbols. Must be a valid identifier; it is pasted.
//       KEY         the key's value, an integral or enumeration constant.
//       KEY_TYPE    the key's declared type, as a type name.
//       VALUE_TYPE  the value's type, as a type name. It must be a COMPLETE
//                   type in every row, including unary ones: the emitters
//                   apply `sizeof` to this column unconditionally, and a
//                   macro cannot test a type name for being `void`. Write
//                   `char` for a unary key -- what makes the row unary is the
//                   FLAGS column, not this one, and the byte is never used.
//       DEFAULT     the initial value, a constant expression of VALUE_TYPE.
//                   Ignored for a unary key -- write 0.
//       FLAGS       D_OPTION_FLAG_* bits, or D_OPTION_FLAG_NONE.
//
//   Declared like this -- note that a real schema's rows are joined by
// backslashes, which are omitted here because a `//` comment line ending in a
// backslash IS a line continuation and would swallow the line after it:
//
//     #define WINDOW_OPTIONS(X)
//         X(title,   win_opt_title,   enum win_opt, const char*, "",  0)
//         X(width,   win_opt_width,   enum win_opt, int32_t,     800, 0)
//         X(height,  win_opt_height,  enum win_opt, int32_t,     600, 0)
//         X(visible, win_opt_visible, enum win_opt, char,        0,
//                                                  D_OPTION_FLAG_UNARY)
//
//   ...and then expanded by the emitters in option_set_c.h, or by any
// consumer of your own -- a CLI binder, a help printer, a registry, the
// parity oracle's parseable row. The framework provides emitters; it does not
// require you to use only those.
//
//   DEFAULTS ARE PINNED IN THE SCHEMA AND TAKEN FROM NOWHERE ELSE. The
// archive work settled this the hard way: a default consulted from a backend
// differs between backends and between versions of one backend, which
// reintroduces exactly the environmental parity break the shared core exists
// to remove. The schema is the one place a default is written.

// D_OPTION_SCHEMA_IS_UNARY
//   macro: whether a schema row declares a unary key. The test is on the
// FLAGS column rather than on VALUE_TYPE, because a macro cannot compare a
// type name to `void` -- so `D_OPTION_FLAG_UNARY` is what makes a row unary
// and `void` in the VALUE_TYPE column is documentation.
#define D_OPTION_SCHEMA_IS_UNARY(flags)                                       \
    ((((uint32_t)(flags)) & D_OPTION_FLAG_UNARY) != 0u)

// D_OPTION_SCHEMA_COUNT_ROW / D_OPTION_SCHEMA_COUNT
//   macro: the number of rows in a schema, as a constant expression. Used to
// size the storage a set needs, so the storage cannot fall out of step with
// the schema it holds.
#define D_OPTION_SCHEMA_COUNT_ROW(name, key, KT, VT, dflt, flags)    + 1u
#define D_OPTION_SCHEMA_COUNT(SCHEMA)                                         \
    ((uint32_t)(0u SCHEMA(D_OPTION_SCHEMA_COUNT_ROW)))

// D_OPTION_SCHEMA_ENUM_ROW / D_OPTION_SCHEMA_ENUM
//   macro: expand a schema into enumeration constants, so the key names and
// the schema cannot drift apart. Emitted as `#define`s rather than as an
// `enum` for the reason option_common.h states -- an enum has implementation-
// defined size, and these values reach a wire format.
#define D_OPTION_SCHEMA_ENUM_ROW(name, key, KT, VT, dflt, flags)              \
    D_OPTION_KEY_##name = (key),

// D_OPTION_SCHEMA_NAME_ROW
//   macro: expand a schema into a `const char*` name table, in row order.
// This is what a report, a CLI, and the parity oracle's parseable row all
// need, and having it fall out of the schema is what keeps a renamed key from
// printing under its old spelling.
#define D_OPTION_SCHEMA_NAME_ROW(name, key, KT, VT, dflt, flags)              \
    #name,


///////////////////////////////////////////////////////////////////////////////
///             IV.   TYPE-GENERIC HELPERS (C11+)                           ///
///////////////////////////////////////////////////////////////////////////////
//
//   The deduced forms. Absent below C11 rather than degraded, per the Tier
// law; the C99 forms in sections I and II remain the portable path and every
// macro here has one.

#if D_ENV_LANG_IS_C11_OR_HIGHER

// D_OPTION_TYPE_OF
//   macro: the type descriptor for an EXPRESSION, deduced. The C99 spelling
// is D_OPTION_TYPE(T), which names the type instead.
#define D_OPTION_TYPE_OF(expr)          D_TYPE_OF_EXPR(expr)

// D_OPTION_CELL_OF
//   macro: a cell for a key and a value EXPRESSION, with the value's type,
// width and descriptor all deduced from it. The C99 spelling is
// D_OPTION_CELL(key, KT, VT, offset).
#define D_OPTION_CELL_OF(key, key_expr, value_expr, offset)                  \
    D_OPTION_INIT(D_OPTION_KEY(key),                                         \
                  D_OPTION_TYPE_OF(key_expr),                                \
                  D_OPTION_TYPE_OF(value_expr),                              \
                  (offset),                                                  \
                  (uint32_t)sizeof(value_expr),                              \
                  D_OPTION_FLAG_NONE)

#endif  // D_ENV_LANG_IS_C11_OR_HIGHER


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_OPTION_OPTION_H
