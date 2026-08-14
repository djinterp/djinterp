/******************************************************************************
* djinterp [restd]                                             type_traits.hpp
*
* type_traits umbrella header:
*   Aggregates the granular per-symbol headers under
* /inc/djinterp/restd/type_traits/. Mirrors the standard <type_traits>
* convenience header.
*
*   GRANULARITY:
*   Every public symbol lives in its own .hpp file under this directory.
* This umbrella adds no symbols of its own.
*
*   PORTABILITY:
*   Each granular header gates its own contents based on the active C++
* standard tier and feature macros.
*
*
* TABLE OF CONTENTS
* =================
* I.    HELPER CLASSES
* II.   SFINAE / CONTROL PRIMITIVES
* III.  CV / POINTER / REFERENCE MODIFICATION
* IV.   ARRAY MODIFICATION
* V.    SIGN MODIFICATION
* VI.   PRIMARY TYPE CATEGORIES
* VII.  COMPOSITE TYPE CATEGORIES
* VIII. TYPE PROPERTIES
* IX.   TYPE-PROPERTY QUERIES (intrinsic-backed)
* X.    SUPPORTED OPERATIONS (constructible / assignable / destructible)
* XI.   TYPE RELATIONSHIPS
* XII.  ENUM SUPPORT
* XIII. TRANSFORMATIONS
* XIV.  LOGICAL OPERATORS
*
*
* path:      /inc/djinterp/restd/type_traits/type_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_
#define DJINTERP_RESTD_TYPE_TRAITS_ 1

// djinterp
#include "../../core/djinterp.hpp"


// =============================================================================
// I.   HELPER CLASSES
// =============================================================================

#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./bool_constant.hpp"


// =============================================================================
// II.  SFINAE / CONTROL PRIMITIVES
// =============================================================================

#include "./enable_if.hpp"
#include "./conditional.hpp"
#include "./type_identity.hpp"
#include "./void_t.hpp"


// =============================================================================
// III. CV / POINTER / REFERENCE MODIFICATION
// =============================================================================

#include "./remove_const.hpp"
#include "./remove_volatile.hpp"
#include "./remove_cv.hpp"
#include "./remove_pointer.hpp"
#include "./remove_reference.hpp"
#include "./remove_cvref.hpp"
#include "./add_const.hpp"
#include "./add_volatile.hpp"
#include "./add_cv.hpp"
#include "./add_pointer.hpp"
#include "./add_lvalue_reference.hpp"
#include "./add_rvalue_reference.hpp"


// =============================================================================
// IV.  ARRAY MODIFICATION
// =============================================================================

#include "./remove_extent.hpp"
#include "./remove_all_extents.hpp"


// =============================================================================
// V.   SIGN MODIFICATION
// =============================================================================

#include "./make_signed.hpp"
#include "./make_unsigned.hpp"


// =============================================================================
// VI.  PRIMARY TYPE CATEGORIES
// =============================================================================

#include "./is_void.hpp"
#include "./is_null_pointer.hpp"
#include "./is_integral.hpp"
#include "./is_floating_point.hpp"
#include "./is_array.hpp"
#include "./is_bounded_array.hpp"
#include "./is_unbounded_array.hpp"
#include "./is_enum.hpp"
//#include "./is_scoped_enum.hpp"
#include "./is_union.hpp"
#include "./is_class.hpp"
#include "./is_pointer.hpp"
#include "./is_lvalue_reference.hpp"
#include "./is_rvalue_reference.hpp"
#include "./is_function.hpp"
#include "./is_member_pointer.hpp"
#include "./is_member_object_pointer.hpp"
#include "./is_member_function_pointer.hpp"


// =============================================================================
// VII. COMPOSITE TYPE CATEGORIES
// =============================================================================

#include "./is_reference.hpp"
#include "./is_arithmetic.hpp"
#include "./is_fundamental.hpp"
#include "./is_scalar.hpp"
#include "./is_object.hpp"
#include "./is_compound.hpp"


// =============================================================================
// VIII.TYPE PROPERTIES
// =============================================================================

#include "./is_const.hpp"
#include "./is_volatile.hpp"
#include "./is_signed.hpp"
#include "./is_unsigned.hpp"


// =============================================================================
// IX.  TYPE-PROPERTY QUERIES (intrinsic-backed)
// =============================================================================

//#include "./is_trivial.hpp"
//#include "./is_trivially_copyable.hpp"
//#include "./is_standard_layout.hpp"
#include "./is_empty.hpp"
#include "./is_polymorphic.hpp"
//#include "./is_abstract.hpp"
#include "./is_final.hpp"
//#include "./is_aggregate.hpp"
//#include "./has_virtual_destructor.hpp"
//#include "./has_unique_object_representations.hpp"


// =============================================================================
// X.   SUPPORTED OPERATIONS
// =============================================================================
// Constructor / assignment / destructor probes. Every family ships in
// three flavors: regular, _trivially_, and _nothrow_. The trivially-
// and nothrow- variants depend on compiler intrinsics (with the
// nothrow- variants having portable fallbacks via noexcept probes).

// constructible
#include "./is_constructible.hpp"
#include "./is_trivially_constructible.hpp"
#include "./is_nothrow_constructible.hpp"

// default-constructible
#include "./is_default_constructible.hpp"
#include "./is_trivially_default_constructible.hpp"
#include "./is_nothrow_default_constructible.hpp"

// copy-constructible
#include "./is_copy_constructible.hpp"
#include "./is_trivially_copy_constructible.hpp"
#include "./is_nothrow_copy_constructible.hpp"

// move-constructible
#include "./is_move_constructible.hpp"
#include "./is_trivially_move_constructible.hpp"
#include "./is_nothrow_move_constructible.hpp"

// assignable
#include "./is_assignable.hpp"
#include "./is_trivially_assignable.hpp"
#include "./is_nothrow_assignable.hpp"

// copy-assignable
#include "./is_copy_assignable.hpp"
#include "./is_trivially_copy_assignable.hpp"
#include "./is_nothrow_copy_assignable.hpp"

// move-assignable
#include "./is_move_assignable.hpp"
#include "./is_trivially_move_assignable.hpp"
#include "./is_nothrow_move_assignable.hpp"

// destructible
#include "./is_destructible.hpp"
#include "./is_trivially_destructible.hpp"
#include "./is_nothrow_destructible.hpp"


// =============================================================================
// XI.  TYPE RELATIONSHIPS
// =============================================================================

#include "./is_same.hpp"
#include "./is_base_of.hpp"
#include "./is_convertible.hpp"


// =============================================================================
// XII. ENUM SUPPORT
// =============================================================================

#include "./underlying_type.hpp"


// =============================================================================
// XIII.TRANSFORMATIONS
// =============================================================================

#include "./alignment_of.hpp"
#include "./rank.hpp"
#include "./extent.hpp"
#include "./decay.hpp"
#include "./common_type.hpp"


// =============================================================================
// XIV. LOGICAL OPERATORS
// =============================================================================

#include "./conjunction.hpp"
#include "./disjunction.hpp"
#include "./negation.hpp"


#endif  // DJINTERP_RESTD_TYPE_TRAITS_
