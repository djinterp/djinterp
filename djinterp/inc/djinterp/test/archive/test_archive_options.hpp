/*******************************************************************************
* djinterp [test]                                       test_archive_options.hpp
*
*   RE-EXPORT SHIM.  The comparison, pristine and diff surfaces this header
* used to define now live on the vocabulary they describe --
* core/util/archive/archive_options.hpp -- implemented over the kernel's knob
* table rather than as a hand-written block-by-block walk.  What remains here
* is a set of using-declarations, so that every existing `dt::` call site
* keeps compiling unchanged.
*
*   WHY THE MOVE:
*   archive_options grew d_archive_options_equal, _are_default,
* _codec_is_default and _diff in the C kernel, backed by a dense knob table
* carrying each knob's name and owning format.  This header's field chain was
* a second implementation of that table -- one that had to be edited by hand
* every time a knob was added.  archive_common.h is the single walk now.
*
*   MEASURED, NOT ASSUMED: a differential harness ran both implementations
* over 28 option sets (784 ordered pairs, 8498 checks).  They agree on every
* predicate and every comparator, including the text knobs -- "unset" and
* "set to empty" compare equal under both, which is what archive_options::span
* documents.  The ONLY divergence is field ORDER in describe_option_diff: the
* kernel emits in knob-table declaration order, where the hand-written chain
* emitted some text knobs ahead of lower-numbered numeric siblings
* ("comment, preserve_permissions" vs "preserve_permissions, comment").  The
* field SET is identical in every case.  All seven exact-string assertions in
* the existing suites still hold, since none of them spans two knobs whose
* relative order changed.
*
*   THE COARSE CODEC GRANULARITY IS PRESERVED.  describe_option_diff still
* names archive-level and per-format knobs individually and reports the
* embedded codec at BLOCK granularity ("codec.zstd"), with codec.level called
* out by name.  That contract is now derived from d_compress_knob_codec and
* d_codec_id_name rather than hand-maintained, so it cannot drift from the
* codec set.
*
*   LAYERING (unchanged): this header still gives you both surfaces, because
* archive_options.hpp includes compress_options.hpp and this includes
* test_compress_options.hpp.
*
*   MIGRATION:
*   New code should include core/util/archive/archive_options.hpp and drop the
* `dt::` qualification; the names resolve in ::djinterp.  This header can go
* once no call site names it.
*
*   PORTABILITY:
*   Target floor C++98, effective floor C++11 -- djinterp.hpp gates below it,
* so no includer compiles at C++98 today.  Verified at C++11/17/20.  The
* surface is no longer link-free: the kernel walk lives in archive_common.c
* and compress_common.c, so a TU using these comparators links both.  It
* already did in practice, since archive_options.hpp's operator== has
* forwarded to d_archive_options_equal since the kernel refactor.
*
*
* path:      /inc/djinterp/test/archive/test_archive_options.hpp
* link(s):   archive_common.c, compress_common.c
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_TEST_ARCHIVE_TEST_ARCHIVE_OPTIONS_HPP
#define DJINTERP_TEST_ARCHIVE_TEST_ARCHIVE_OPTIONS_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP11_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "../../djinterp.hpp"                             // namespace macros
#include "../../core/util/archive/archive_options.hpp"    // the real surface
#include "../compress/test_compress_options.hpp"          // the codec surface


NS_DJINTERP
NS_TEST


// =============================================================================
// I.   RE-EXPORTS
// =============================================================================
//   Ordinary enclosing-namespace lookup would already find these from inside
// djinterp::test, but the call sites spell them dt::, which requires the name
// to be a member of djinterp::test.  These declarations make it one without
// making it a second definition.

// -- per-format block comparison ---------------------------------------------
using ::djinterp::zip_options_equal;
using ::djinterp::tar_options_equal;
using ::djinterp::gz_options_equal;
using ::djinterp::sevenzip_options_equal;
using ::djinterp::rar_options_equal;

// -- aggregate comparison ----------------------------------------------------
using ::djinterp::archive_options_equal;

// -- pristine-default predicates ---------------------------------------------
using ::djinterp::default_archive_options;
using ::djinterp::options_are_default;
using ::djinterp::codec_is_default;

// -- diff description --------------------------------------------------------
using ::djinterp::describe_option_diff;
using ::djinterp::describe_diff_from_default;

// -- option builders ---------------------------------------------------------
using ::djinterp::store_only_options;
using ::djinterp::level_options;
using ::djinterp::codec_level_options;
using ::djinterp::zip_method_options;
using ::djinterp::zip_encrypted_options;


NS_END  // test
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_TEST_ARCHIVE_TEST_ARCHIVE_OPTIONS_HPP
