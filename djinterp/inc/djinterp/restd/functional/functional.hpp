/***********************************************************************
* restd                                                   functional.hpp
*
* umbrella header for `restd/functional/`.
*   Includes every granular header in the module so that user code can
* `#include "restd/functional/functional.hpp"` and pull in the whole
* shipped surface. Granular headers exist to keep TU compile times
* tight and the dependency graph explicit; this file is for callers
* that prefer convenience.
*
*   Symbols deferred to later milestones (`function`,
* `move_only_function`, `copyable_function`, `function_ref`, `bind`,
* `bind_front`, `bind_back`, the `placeholders::_N`, the searchers)
* are not part of this umbrella because their granular headers do not
* yet exist.
*
*
* path:      /inc/restd/functional/functional.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_UMBRELLA_
#define RESTD_FUNCTIONAL_UMBRELLA_ 1

// arithmetic operations -------------------------------------------------
#include "restd/functional/plus.hpp"
#include "restd/functional/minus.hpp"
#include "restd/functional/multiplies.hpp"
#include "restd/functional/divides.hpp"
#include "restd/functional/modulus.hpp"
#include "restd/functional/negate.hpp"

// comparisons -----------------------------------------------------------
#include "restd/functional/equal_to.hpp"
#include "restd/functional/not_equal_to.hpp"
#include "restd/functional/greater.hpp"
#include "restd/functional/less.hpp"
#include "restd/functional/greater_equal.hpp"
#include "restd/functional/less_equal.hpp"

// logical ---------------------------------------------------------------
#include "restd/functional/logical_and.hpp"
#include "restd/functional/logical_or.hpp"
#include "restd/functional/logical_not.hpp"

// bitwise ---------------------------------------------------------------
#include "restd/functional/bit_and.hpp"
#include "restd/functional/bit_or.hpp"
#include "restd/functional/bit_xor.hpp"
#include "restd/functional/bit_not.hpp"

// identity --------------------------------------------------------------
#include "restd/functional/identity.hpp"

// reference wrappers ----------------------------------------------------
#include "restd/functional/reference_wrapper.hpp"
#include "restd/functional/ref.hpp"
#include "restd/functional/cref.hpp"
#include "restd/functional/unwrap_reference.hpp"
#include "restd/functional/unwrap_ref_decay.hpp"

// invocation ------------------------------------------------------------
#include "restd/functional/invoke.hpp"
#include "restd/functional/invoke_r.hpp"
#include "restd/functional/mem_fn.hpp"
#include "restd/functional/not_fn.hpp"

// bind support traits (primary templates only -- bind itself deferred) -
#include "restd/functional/is_bind_expression.hpp"
#include "restd/functional/is_placeholder.hpp"

// exception types -------------------------------------------------------
#include "restd/functional/bad_function_call.hpp"

// hashing ---------------------------------------------------------------
#include "restd/functional/hash.hpp"

#endif // RESTD_FUNCTIONAL_UMBRELLA_
