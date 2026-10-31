/*******************************************************************************
* djinterp [re_std]                                               functional.hpp
*
* functional support header:
* umbrella header for `re_std/cpp/functional/`.
*   Includes every granular header in the module so that user code can
* `#include "re_std/cpp/functional/functional.hpp"` and pull in the whole
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
* path:      /inc/re_std/cpp/functional/functional.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.07.25
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_FUNCTIONAL_HPP
#define RE_STD_FUNCTIONAL_FUNCTIONAL_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
// arithmetic operations -------------------------------------------------
#include "re_std/cpp/functional/plus.hpp"
#include "re_std/cpp/functional/minus.hpp"
#include "re_std/cpp/functional/multiplies.hpp"
#include "re_std/cpp/functional/divides.hpp"
#include "re_std/cpp/functional/modulus.hpp"
#include "re_std/cpp/functional/negate.hpp"
// comparisons -----------------------------------------------------------
#include "re_std/cpp/functional/equal_to.hpp"
#include "re_std/cpp/functional/not_equal_to.hpp"
#include "re_std/cpp/functional/greater.hpp"
#include "re_std/cpp/functional/less.hpp"
#include "re_std/cpp/functional/greater_equal.hpp"
#include "re_std/cpp/functional/less_equal.hpp"
// logical ---------------------------------------------------------------
#include "re_std/cpp/functional/logical_and.hpp"
#include "re_std/cpp/functional/logical_or.hpp"
#include "re_std/cpp/functional/logical_not.hpp"
// bitwise ---------------------------------------------------------------
#include "re_std/cpp/functional/bit_and.hpp"
#include "re_std/cpp/functional/bit_or.hpp"
#include "re_std/cpp/functional/bit_xor.hpp"
#include "re_std/cpp/functional/bit_not.hpp"
// identity --------------------------------------------------------------
#include "re_std/cpp/functional/identity.hpp"
// reference wrappers ----------------------------------------------------
#include "re_std/cpp/functional/reference_wrapper.hpp"
#include "re_std/cpp/functional/ref.hpp"
#include "re_std/cpp/functional/cref.hpp"
#include "re_std/cpp/functional/unwrap_reference.hpp"
#include "re_std/cpp/functional/unwrap_ref_decay.hpp"
// invocation ------------------------------------------------------------
#include "re_std/cpp/functional/invoke.hpp"
#include "re_std/cpp/functional/invoke_r.hpp"
#include "re_std/cpp/functional/mem_fn.hpp"
#include "re_std/cpp/functional/not_fn.hpp"
// bind support traits (primary templates only -- bind itself deferred) -
#include "re_std/cpp/functional/is_bind_expression.hpp"
#include "re_std/cpp/functional/is_placeholder.hpp"
// exception types -------------------------------------------------------
#include "re_std/cpp/functional/bad_function_call.hpp"
// callable wrappers -----------------------------------------------------
// (function throws bad_function_call above; include order is otherwise
//  immaterial thanks to the include guards.)
#include "re_std/cpp/functional/function.hpp"
// hashing ---------------------------------------------------------------
#include "re_std/cpp/functional/hash.hpp"

#endif  // floor, for now


#endif  // RE_STD_FUNCTIONAL_FUNCTIONAL_HPP
