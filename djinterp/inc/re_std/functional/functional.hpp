/***********************************************************************
* re_std                                                  functional.hpp
*
* umbrella header for `re_std/functional/`.
*   Includes every granular header in the module so that user code can
* `#include "re_std/functional/functional.hpp"` and pull in the whole
* shipped surface. Granular headers exist to keep TU compile times
* tight and the dependency graph explicit; this file is for callers
* that prefer convenience.
*
*   Symbols deferred to later milestones (`move_only_function`,
* `copyable_function`, `function_ref`, `bind`, `bind_front`,
* `bind_back`, the `placeholders::_N`, the searchers) are not part of
* this umbrella because their granular headers do not yet exist.
*
*
* path:      /inc/djinterp/re_std/functional/functional.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.07.25
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_UMBRELLA_
#define DJINTERP_RE_STD_FUNCTIONAL_UMBRELLA_ 1

// arithmetic operations -------------------------------------------------
#include "re_std/functional/plus.hpp"
#include "re_std/functional/minus.hpp"
#include "re_std/functional/multiplies.hpp"
#include "re_std/functional/divides.hpp"
#include "re_std/functional/modulus.hpp"
#include "re_std/functional/negate.hpp"

// comparisons -----------------------------------------------------------
#include "re_std/functional/equal_to.hpp"
#include "re_std/functional/not_equal_to.hpp"
#include "re_std/functional/greater.hpp"
#include "re_std/functional/less.hpp"
#include "re_std/functional/greater_equal.hpp"
#include "re_std/functional/less_equal.hpp"

// logical ---------------------------------------------------------------
#include "re_std/functional/logical_and.hpp"
#include "re_std/functional/logical_or.hpp"
#include "re_std/functional/logical_not.hpp"

// bitwise ---------------------------------------------------------------
#include "re_std/functional/bit_and.hpp"
#include "re_std/functional/bit_or.hpp"
#include "re_std/functional/bit_xor.hpp"
#include "re_std/functional/bit_not.hpp"

// identity --------------------------------------------------------------
#include "re_std/functional/identity.hpp"

// reference wrappers ----------------------------------------------------
#include "re_std/functional/reference_wrapper.hpp"
#include "re_std/functional/ref.hpp"
#include "re_std/functional/cref.hpp"
#include "re_std/functional/unwrap_reference.hpp"
#include "re_std/functional/unwrap_ref_decay.hpp"

// invocation ------------------------------------------------------------
#include "re_std/functional/invoke.hpp"
#include "re_std/functional/invoke_r.hpp"
#include "re_std/functional/mem_fn.hpp"
#include "re_std/functional/not_fn.hpp"

// bind support traits (primary templates only -- bind itself deferred) -
#include "re_std/functional/is_bind_expression.hpp"
#include "re_std/functional/is_placeholder.hpp"

// exception types -------------------------------------------------------
#include "re_std/functional/bad_function_call.hpp"

// callable wrappers -----------------------------------------------------
// (function throws bad_function_call above; include order is otherwise
//  immaterial thanks to the include guards.)
#include "re_std/functional/function.hpp"

// hashing ---------------------------------------------------------------
#include "re_std/functional/hash.hpp"

#endif  // DJINTERP_RE_STD_FUNCTIONAL_UMBRELLA_
