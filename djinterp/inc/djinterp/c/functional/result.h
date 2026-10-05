/*******************************************************************************
* djinterp [c]                                                          result.h
*
* The `result` carrier: one value, or one error, never both and never neither.
*   Table 7.2 gives `result<T,E>` as a kind `* -> * -> *` carrier modelling
* Functor, Applicative, Monad, Foldable, Traversable and Bifunctor -- and
* explicitly NOT Alternative. That exclusion is a stated non-property and is
* honoured here: this header supplies no `alt` and no `aempty`, because there
* is no way to choose between two errors without inventing one.
*
* STORAGE. As with maybe.h, the typed cell and the generic view are separate:
*   - `D_RESULT_DECLARE(name, vtype, etype)` emits a tag plus a UNION of the two
*     payloads, which is the faithful lowering of "value xor error" and keeps
*     `sizeof` at the larger arm rather than their sum.
*   - `struct d_result` is a borrowed view onto such a cell, carrying both
*     widths so one implementation of `map` and `map_err` serves every
*     instantiation.
* A view borrows; the cell must outlive it.
*
* THE UNION IS WHY WIDTHS TRAVEL IN PAIRS. Writing an error overwrites the
* value slot and vice versa, so every operation has to know which arm it is
* touching and how wide that arm is. A single `payload_size` would be enough to
* copy bytes but not enough to reject a mismatched destination, which is the
* error worth catching.
*
* BIFUNCTOR IS AVAILABLE HERE, THE PROTOCOL IS NOT. `d_result_bimap` maps both
* arms of THIS carrier and needs nothing higher-kinded. A `struct d_bifunctor`
* that any two-parameter carrier could implement is kind `* -> * -> *` work and
* is deliberately not attempted; the parity assessment recommends declaring the
* Bifunctor and Profunctor PROTOCOLS C++-only, which this header does not
* contradict.
*
*
* path:      /inc/djinterp/c/functional/result.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_RESULT_H
#define DJINTERP_C_FUNCTIONAL_RESULT_H 1

// std
#include <stddef.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./reducer.h"
#include "./producer.h"
#include "./maybe.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// d_result
//   struct: a borrowed, type-erased view onto a result cell.
// `payload` points at the union slot shared by the two arms; which arm is live
// is decided by `is_ok`.
struct d_result
{
    bool*  is_ok;       // the cell's tag
    void*  payload;     // the cell's union slot
    size_t value_size;  // byte width of the success arm
    size_t error_size;  // byte width of the error arm
};

// D_RESULT_FIELDS
//   macro: THE declaration of a result cell's layout, and the only one. See the
// note on D_MAYBE_FIELDS: the C macro below and the C++ template in
// functional_face.hpp both expand this text, so neither restates the members.
// The union is the point: a result is value XOR error, so the arms share
// storage.
#define D_RESULT_FIELDS(vtype,                                              \
                        etype)                                              \
    bool is_ok;                                                             \
    union                                                                   \
    {                                                                       \
        vtype value;                                                        \
        etype error;                                                        \
    } payload;

// D_RESULT_DECLARE
//   macro: declares a result cell holding either arm by value.
#define D_RESULT_DECLARE(name,                                              \
                         vtype,                                             \
                         etype)                                             \
    struct name                                                             \
    {                                                                       \
        D_RESULT_FIELDS(vtype, etype)                                       \
    }

// D_RESULT_VIEW
//   macro: builds a view onto a cell declared with D_RESULT_DECLARE.
// Both widths are read from the cell's own union members, so a view can never
// disagree with the storage it describes.
#define D_RESULT_VIEW(cell)                                                 \
    d_result_view(&(cell).is_ok,                                            \
                  &(cell).payload,                                          \
                  sizeof((cell).payload.value),                             \
                  sizeof((cell).payload.error))

// I.     construction
struct d_result d_result_view(bool*  _is_ok,
                              void*  _payload,
                              size_t _value_size,
                              size_t _error_size);
bool            d_result_is_valid(const struct d_result* _result);

// II.    introduction
bool     d_result_ok(struct d_result* _result,
                     const void*      _value);
bool     d_result_err(struct d_result* _result,
                      const void*      _error);

// III.   inspection
bool     d_result_is_ok(const struct d_result* _result);
bool     d_result_is_err(const struct d_result* _result);
bool     d_result_value_or(const struct d_result* _result,
                           void*                  _out,
                           const void*            _fallback);
bool     d_result_error(const struct d_result* _result,
                        void*                  _out);

// IV.    Functor, Bifunctor and Monad, over this carrier
bool     d_result_map(const struct d_result* _in,
                      struct d_result*       _out,
                      fn_transformer         _transform,
                      void*                  _context);
bool     d_result_map_err(const struct d_result* _in,
                          struct d_result*       _out,
                          fn_transformer         _transform,
                          void*                  _context);
bool     d_result_bimap(const struct d_result* _in,
                        struct d_result*       _out,
                        fn_transformer         _on_value,
                        fn_transformer         _on_error,
                        void*                  _context);
bool     d_result_bind(const struct d_result* _in,
                       struct d_result*       _out,
                       fn_kleisli             _arrow,
                       void*                  _context);

// V.     conversion to maybe (forgets the error)
bool     d_result_to_maybe(const struct d_result* _result,
                           struct d_maybe*        _out);

// VI.    Foldable, via the tier 1 spine
size_t   d_result_drive(const struct d_result*   _result,
                        const struct d_reducer*  _reducer,
                        struct d_reducing_state* _state);
bool     d_result_fold(const struct d_result* _result,
                       void*                  _accumulator,
                       fn_fold                _step,
                       void*                  _context);
struct d_producer d_result_producer(struct d_producer_array_state* _state,
                                    const struct d_result*         _result);

// VII.   layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct d_result, payload) == sizeof(bool*),
                "d_result layout drift");
D_STATIC_ASSERT(offsetof(struct d_result, error_size) ==
                    (sizeof(bool*) + sizeof(void*) + sizeof(size_t)),
                "d_result layout drift");


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FUNCTIONAL_RESULT_H
