/***********************************************************************
* restd                                                          utility.hpp
*
* umbrella header for restd's <utility> implementation.
*
* current contents:
*   foundational (shipped earlier):
*     swap, move, forward, declval, pair, make_pair.
*   tail (shipped 2026-05-09):
*     exchange, as_const, to_underlying, integer_sequence /
*     index_sequence / make_integer_sequence / make_index_sequence /
*     index_sequence_for, cmp_equal / cmp_not_equal / cmp_less /
*     cmp_greater / cmp_less_equal / cmp_greater_equal / in_range,
*     unreachable, forward_like, in_place_type / in_place_index.
*
* not yet implemented:
*   - move_if_noexcept   -- needs is_copy_constructible (not yet
*                            in restd's type_traits foundation).
*
* design notes:
*   - integer_sequence's recursive fallback is O(N) instantiation
*     depth; the builtin path (Clang/MSVC __make_integer_seq, GCC
*     __integer_pack) is O(1) and is preferred when available.
*   - cmp_* uses std::is_signed and std::make_unsigned (justified
*     localised exception); restd::make_unsigned not yet shipped.
*   - in_place_t / in_place themselves live in <optional> per the
*     project's earlier sequencing (optional shipped first); only
*     in_place_type / in_place_index are here.
*
*
* path:      /inc/djinterp/re_std/utility/utility.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.05.09
***********************************************************************/

#ifndef RESTD_UTILITY_
#define RESTD_UTILITY_ 1

#include "djinterp.hpp"

// ---- foundational ----
#include "restd/utility/swap.hpp"
#include "restd/utility/move.hpp"
#include "restd/utility/forward.hpp"
#include "restd/utility/declval.hpp"
#include "restd/utility/pair.hpp"
#include "restd/utility/make_pair.hpp"

// ---- tail (Phase 9, 2026-05-09) ----
#include "restd/utility/exchange.hpp"
#include "restd/utility/as_const.hpp"
#include "restd/utility/to_underlying.hpp"
#include "restd/utility/integer_sequence.hpp"
#include "restd/utility/intcmp.hpp"
#include "restd/utility/unreachable.hpp"
#include "restd/utility/forward_like.hpp"
#include "restd/utility/in_place_type.hpp"

#endif  // RESTD_UTILITY_
