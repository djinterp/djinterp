/******************************************************************************
* djinterp [restd]                                                align_val_t.hpp
*
* align_val_t header:
*   Strong typedef for an alignment value. Used by C++17's over-
* aligned new/delete operators to disambiguate "size to allocate"
* from "alignment required":
*
*     void* p = ::operator new(64, std::align_val_t{32});
*
*   STRATEGY:
*     C++17+: using-declaration from std::align_val_t.
*     C++11 - C++14: back-port as `enum class align_val_t : size_t {}`.
*                    Strong-typed; can't be implicitly converted from
*                    a bare size_t.
*     C++98 - C++03: back-port as a struct wrapper around size_t with
*                    explicit construction. No `enum class`; less safe
*                    against accidental implicit conversion but
*                    syntactically equivalent at the use site.
*
*   The operator new overloads taking align_val_t are NOT shipped —
* they're runtime-provided when std supports them (C++17+) and
* nonexistent earlier. restd surfaces the TYPE for clarity in
* user-facing APIs that need to express alignment as a strong
* parameter.
*
*
* path:      /inc/djinterp/restd/new/align_val_t.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_ALIGN_VAL_T_
#define DJINTERP_RESTD_ALIGN_VAL_T_ 1

#include <cstddef>
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
#include <new>
#endif


NS_RESTD


// ===========================================================================
// I.   ALIGN_VAL_T
// ===========================================================================

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// C++17+: std::align_val_t is available. Pass-through.
using std::align_val_t;

#elif D_ENV_LANG_IS_CPP11_OR_HIGHER

// C++11–C++14: back-port as strong-typed enum class. Same source-level
// semantics as std::align_val_t; only operator-new bindings differ.
enum class align_val_t : std::size_t {};

#else

// C++98–C++03: no enum class. Use a struct wrapper with explicit
// construction. Less safe against implicit-conversion bugs but
// syntactically equivalent at the use site (align_val_t(32)).
struct align_val_t
{
    explicit align_val_t(std::size_t _v) : value(_v) {}
    std::size_t value;

    operator std::size_t() const { return value; }
};

#endif


NS_END  // restd


#endif  // DJINTERP_RESTD_ALIGN_VAL_T_
