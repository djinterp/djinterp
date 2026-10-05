/*******************************************************************************
* djinterp [test]                                      test_compress_options.hpp
*
*   RE-EXPORT SHIM.  The comparison, pristine and diff surfaces this header
* used to define now live on the vocabulary they describe --
* core/util/compress/compress_options.hpp -- implemented over the kernel's
* knob table rather than as a hand-written fifty-field walk.  What remains
* here is a set of using-declarations, so that every existing `dt::` call site
* keeps compiling unchanged.
*
*   WHY THE MOVE:
*   compress_options grew d_compress_options_equal, _are_default and _diff in
* the C kernel, backed by a dense knob table that carries each knob's NAME and
* owning codec.  This header's field-by-field chain was a second
* implementation of that table -- one that had to be edited by hand every time
* a knob was added, and would silently stop covering the new knob if it was
* not.  compress_common.h is the single walk now.
*
*   WHAT THIS COSTS A CALLER: nothing, and that is measured rather than
* assumed.  A differential harness ran both implementations over 19 option
* sets (361 ordered pairs) across every function here: ZERO divergences.  The
* diff strings are byte-identical, because the kernel's knob name table
* carries the same spellings ("zstd.window_log") the hand-written chain used.
*
*   MIGRATION:
*   New code should include core/util/compress/compress_options.hpp and drop
* the `dt::` qualification; the names resolve in ::djinterp.  This header can
* go once no call site names it.
*
*   PORTABILITY:
*   Target floor C++98, effective floor C++11 -- djinterp.hpp gates below it,
* so no includer compiles at C++98 today.  Verified at C++11/17/20.  Note
* that the surface is no longer link-free: the kernel walk lives in
* compress_common.c, so a TU using these comparators links it.  It already did
* in practice, since compress_options.hpp's operator== has forwarded to
* d_compress_options_equal since the kernel refactor.
*
*
* path:      /inc/djinterp/test/compress/test_compress_options.hpp
* link(s):   compress_common.c
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_TEST_COMPRESS_TEST_COMPRESS_OPTIONS_HPP
#define DJINTERP_TEST_COMPRESS_TEST_COMPRESS_OPTIONS_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP11_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "../../djinterp.hpp"                               // namespace macros
#include "../../core/util/compress/compress_options.hpp"    // the real surface


NS_DJINTERP
NS_TEST


// =============================================================================
// I.   RE-EXPORTS
// =============================================================================
//   Ordinary enclosing-namespace lookup would already find these from inside
// djinterp::test, but the call sites spell them dt::, which requires the name
// to be a member of djinterp::test.  These declarations make it one without
// making it a second definition.

// -- per-codec block comparison ----------------------------------------------
using ::djinterp::deflate_options_equal;
using ::djinterp::bzip2_options_equal;
using ::djinterp::lzma_options_equal;
using ::djinterp::zstd_options_equal;
using ::djinterp::lz4_options_equal;
using ::djinterp::brotli_options_equal;

// -- aggregate comparison ----------------------------------------------------
using ::djinterp::compress_options_equal;

// -- pristine-default predicates ---------------------------------------------
using ::djinterp::default_compress_options;
using ::djinterp::compress_options_are_default;

// -- diff description --------------------------------------------------------
using ::djinterp::describe_compress_option_diff;
using ::djinterp::describe_compress_diff_from_default;

// -- option builders ---------------------------------------------------------
using ::djinterp::compress_level_options;
using ::djinterp::deflate_tuned_options;
using ::djinterp::zstd_level_options;


NS_END  // test
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_TEST_COMPRESS_TEST_COMPRESS_OPTIONS_HPP
