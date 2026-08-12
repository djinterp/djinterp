/******************************************************************************
* djinterp [test]                                              test_common.hpp
*
*   The C++ fork of the DTest vocabulary: the scoped status enum, the callable
* id alias, and the reserved zero handle.
*
*   THIS IS A SPLIT, NOT A MERGE.  There was no C++ face to merge with: this
* content sat inside test_common.h in two `#ifdef __cplusplus` blocks,
* and the layout rules put C declarations under c/test/ and C++ declarations
* here (revision.md §2).  The C header is included FIRST and is load-bearing --
* every declaration below names a type that header defines.
*
*   THE NAMESPACE MOVED, AND IT IS THE ONE CHANGE HERE THAT CAN BREAK A CALLER.
* Both blocks opened a bare `namespace djinterp` while every face in this tier
* opens `djinterp::test`, so `test_status` sat two levels away from the call
* sites using it.  They now open NS_DJINTERP / NS_TEST like everything else.
*
*   The hazard is that the fix is almost SILENT: inside a face already in
* `djinterp::test`, unqualified `test_status` resolved by enclosing-scope lookup
* before and resolves the same way now.  What breaks is the qualified external
* spelling -- `djinterp::test_status` becomes `djinterp::test::test_status`.
* Enumerated before the move across the files in this package and found zero
* sites; the 39 consumers of the C kernel outside it were not reachable from
* here and must be checked with:
*
*     grep -rn 'djinterp::test_status\|djinterp::test_callable_id\|\
*               djinterp::k_no_callable' inc/ src/ test/
*
* path:      /inc/djinterp/test/test_common.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_TEST_COMMON
#define DJINTERP_TEST_COMMON 1

// c++
#include <cstdint>
// djinterp
#include "../core/djinterp.hpp"
//   MANDATORY AND FIRST.  D_EXTERN_C and the namespace macros come from here,
// and revision.md §8 removed the `#if !defined(...) #error` guard that used to
// name this file when the macro was missing.  That trades a compile-time
// diagnostic naming the header for a link error naming nothing, so the include
// below is load-bearing rather than conventional.
#include "../c/test/test_common.h"       // the C fork: every type named below


NS_DJINTERP
NS_TEST

//   THE SCOPED ENUM, so `test_status::passed` at eleven call sites is
// untouched.  The underlying type is fixed at int32_t so the two languages
// agree about width -- an unfixed enum class has an implementation-defined
// underlying type, which is a §4 determinacy break at a boundary this value
// crosses.
//
//   It reads the same five macros the C enum reads, so the two notations
// cannot drift: there is one value set and it lives in the C header.
enum class test_status : int32_t
{
    passed  = D_TEST_STATUS_PASSED,
    failed  = D_TEST_STATUS_FAILED,
    skipped = D_TEST_STATUS_SKIPPED,
    pending = D_TEST_STATUS_PENDING,
    error   = D_TEST_STATUS_ERROR
};

//   THE CALLABLE ID, in the C++ spelling the faces use.
//
//   AN ALIAS AND NOT A SECOND TYPE.  `d_test_callable_id` is the type; this
// is its name on the C++ side, so a value crosses the boundary with no
// conversion and `test_callable.hpp` keeps the spelling its call sites already
// use.
//
//   NO `_t` HERE, DELIBERATELY.  The C type took the suffix in revision.md §4;
// this is a C++ type in `djinterp::test` and follows the C++ guide, not the C
// one.
//
//   (The kernel used to explain at this point that the alias had to be declared
// AFTER the typedef because "an alias cannot name a type declared later in the
// same file."  That constraint died with the split -- this is a separate file
// and it includes the C header first, so there is no ordering to respect.  The
// paragraph is deleted rather than left standing.)
typedef d_test_callable_id test_callable_id;

//   `static const` rather than `constexpr` so the declaration is legal at the
// C++11 floor with no qualifier gate; it is a compile-time constant either way.
static const test_callable_id k_no_callable = D_TEST_NO_CALLABLE;

NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_COMMON
