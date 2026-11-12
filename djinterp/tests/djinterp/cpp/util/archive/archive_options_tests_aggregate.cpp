/*******************************************************************************
* djinterp [test]                            archive_options_tests_aggregate.cpp
*
*   Tests for the archive_options aggregate (section III of
* archive_options.hpp): its archive-level knobs are pristine until set and
* resolve to the build's defaults; it is still the stub's superset, so call
* sites written against the stub are unaffected; and constructing it
* constructs every nested block pristine.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/cpp/util/archive/archive_options_tests_aggregate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.11.11
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_archive_options_defaults
  Verifies the archive-level fields of a fresh aggregate, and what they
resolve to.
  Tests the following:
  - level, store_only, preserve_permissions and preserve_mtime are
    archive_knob_unset, and comment is empty
  - the shared "is this pristine?" predicate agrees, and reports no diff
  - resolved: the build's level and store_only (cfg_archive.h), and both
    preserve_* flags on
*/
bool tests_archive_options_defaults()
{
    bool                  _ok = true;
    const archive_options o;

    D_AO_CHECK(o.level                == archive_knob_unset);
    D_AO_CHECK(o.store_only           == archive_knob_unset);
    D_AO_CHECK(o.preserve_permissions == archive_knob_unset);
    D_AO_CHECK(o.preserve_mtime       == archive_knob_unset);
    D_AO_CHECK(o.comment.empty());

    D_AO_CHECK(::djinterp::test::options_are_default(o));
    D_AO_CHECK(::djinterp::test::describe_diff_from_default(o).empty());

    {
        const archive_options other;
        D_AO_CHECK_EQUAL_OPT(o, other);
    }
    {
        const dj_archive_options r = resolved_for(format_id_tar, o);

        D_AO_CHECK(r.level                == D_INTERNAL_ARCHIVE_LEVEL);
        D_AO_CHECK(r.store_only           == D_INTERNAL_ARCHIVE_STORE_ONLY);
        D_AO_CHECK(r.preserve_permissions == 1);
        D_AO_CHECK(r.preserve_mtime       == 1);
    }

    return _ok;
}

/*
tests_archive_options_stub_superset
  Verifies the compatibility guarantee the header's INTEGRATION note makes:
archive_options is a strict superset of the minimal stub archive.hpp once
carried, so existing call sites are unaffected.  The stub's two fields keep
their names; fresh, they now say nothing and the build's defaults apply.
  Tests the following:
  - both stub fields exist, unset when fresh
  - a fresh aggregate is accepted by the facade's default argument, i.e. the
    type is still default-constructible in that position
  - the two stub fields are independently settable and survive a copy
*/
bool tests_archive_options_stub_superset()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    // the two stub fields, saying nothing
    {
        const archive_options o;
        D_AO_CHECK(o.level      == archive_knob_unset);
        D_AO_CHECK(o.store_only == archive_knob_unset);
    }

    // a call site written against the stub still compiles and runs: the
    // facade's defaulted parameter default-constructs this very type
    {
        entry_list items = sample_entries();
        byte_blob  blob;

        D_AO_CHECK_STATUS(try_archive<tar>(items, blob), status_ok);
        D_AO_CHECK(!blob.empty());
    }

    // stub-era usage: set the two fields, pass by const reference, copy
    {
        archive_options o;
        o.level      = 6;
        o.store_only = 1;

        const archive_options copy(o);
        D_AO_CHECK(copy.level      == 6);
        D_AO_CHECK(copy.store_only == 1);
        D_AO_CHECK_EQUAL_OPT(copy, o);
    }

    return _ok;
}

/*
tests_archive_options_nested_blocks_default
  Verifies that constructing the aggregate constructs every nested block
pristine -- none of them is left indeterminate or partially initialised by
the aggregate constructor, which names only the archive-level fields in its
initialiser list.
  Tests the following:
  - each of zip, tar, gz, sevenzip, and rar inside a fresh aggregate compares
    equal to a standalone fresh block of that type
  - the embedded codec likewise matches a standalone fresh compress_options
  - spot-checked fields inside the nested blocks are archive_knob_unset,
    reached through the aggregate
*/
bool tests_archive_options_nested_blocks_default()
{
    bool                  _ok = true;
    const archive_options o;

    {
        const DZipOptions      z;
        const DTarOptions      t;
        const DGzOptions       g;
        const DSevenzipOptions s;
        const DRarOptions      r;
        const compress_options c;

        D_AO_CHECK(::djinterp::test::zip_options_equal(o.zip, z));
        D_AO_CHECK(::djinterp::test::tar_options_equal(o.tar, t));
        D_AO_CHECK(::djinterp::test::gz_options_equal(o.gz, g));
        D_AO_CHECK(::djinterp::test::sevenzip_options_equal(o.sevenzip, s));
        D_AO_CHECK(::djinterp::test::rar_options_equal(o.rar, r));
        D_AO_CHECK(::djinterp::test::compress_options_equal(o.codec, c));
    }

    // reached through the aggregate (a partially initialised block would
    // surface here)
    D_AO_CHECK(o.zip.method      == archive_knob_unset);
    D_AO_CHECK(o.zip.utf8_names  == archive_knob_unset);
    D_AO_CHECK(o.tar.format      == archive_knob_unset);
    D_AO_CHECK(o.gz.store_mtime  == archive_knob_unset);
    D_AO_CHECK(o.sevenzip.solid  == archive_knob_unset);
    D_AO_CHECK(o.sevenzip.method == archive_knob_unset);
    D_AO_CHECK(o.rar.level       == archive_knob_unset);
    D_AO_CHECK(o.codec.level     == archive_knob_unset);

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
