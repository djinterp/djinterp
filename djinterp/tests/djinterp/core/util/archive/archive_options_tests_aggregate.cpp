/*******************************************************************************
* djinterp [test]                            archive_options_tests_aggregate.cpp
*
*   Tests for the archive_options aggregate (section III of
* archive_options.hpp): its own archive-level defaults, the stub-compatibility
* guarantee the header's INTEGRATION note makes, and the fact that every nested
* per-format block arrives at the same state a standalone default-constructed
* block would.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_options_tests_aggregate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_archive_options_defaults
  Verifies the archive-level fields of a default-constructed aggregate.
  Tests the following:
  - level defaults to -1 (backend default effort)
  - store_only defaults to false
  - comment defaults to empty (not named in the initialiser list)
  - preserve_permissions and preserve_mtime both default to true
  - the shared "is this pristine?" predicate agrees, and reports no diff
*/
bool
tests_archive_options_defaults()
{
    bool                  _ok = true;
    const archive_options o;

    D_AO_CHECK(o.level                == -1);
    D_AO_CHECK(o.store_only           == false);
    D_AO_CHECK(o.comment.empty());
    D_AO_CHECK(o.preserve_permissions == true);
    D_AO_CHECK(o.preserve_mtime       == true);

    D_AO_CHECK(::djinterp::test::options_are_default(o));
    D_AO_CHECK(::djinterp::test::describe_diff_from_default(o).empty());

    {
        const archive_options other;
        D_AO_CHECK_EQUAL_OPT(o, other);
    }

    return _ok;
}

/*
tests_archive_options_stub_superset
  Verifies the compatibility guarantee the header's INTEGRATION note makes: this
expanded archive_options is a strict superset of the minimal stub archive.hpp
once carried, with `level` and `store_only` and their defaults preserved, so
existing call sites are unaffected.
  Tests the following:
  - both stub fields exist with their stub defaults (-1 and false)
  - a default-constructed aggregate is accepted by the facade's default
    argument, i.e. the type is still default-constructible in that position
  - the two stub fields are independently settable and survive a copy
*/
bool
tests_archive_options_stub_superset()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    // the two stub fields, at their stub defaults
    {
        const archive_options o;
        D_AO_CHECK(o.level      == -1);
        D_AO_CHECK(o.store_only == false);
    }

    // a call site written against the stub still compiles and runs: the
    // facade's defaulted parameter default-constructs this very type
    {
        entry_list  items = sample_entries();
        byte_blob blob;

        D_AO_CHECK_STATUS(try_archive<tar>(items, blob), status_ok);
        D_AO_CHECK(!blob.empty());
    }

    // stub-era usage: set the two fields, pass by const reference, copy
    {
        archive_options o;
        o.level      = 6;
        o.store_only = true;

        const archive_options copy(o);
        D_AO_CHECK(copy.level      == 6);
        D_AO_CHECK(copy.store_only == true);
        D_AO_CHECK_EQUAL_OPT(copy, o);
    }

    return _ok;
}

/*
tests_archive_options_nested_blocks_default
  Verifies that constructing the aggregate default-constructs every nested block
-- none of them is left indeterminate or partially initialised by the aggregate
constructor, which names only the five archive-level fields in its initialiser
list.
  Tests the following:
  - each of zip, tar, gz, sevenzip, and rar inside a fresh aggregate compares
    equal to a standalone default-constructed block of that type
  - the embedded codec likewise matches a standalone default compress_options
  - spot-checked field values inside the nested blocks match their documented
    defaults, reached through the aggregate
*/
bool
tests_archive_options_nested_blocks_default()
{
    bool                  _ok = true;
    const archive_options o;

    {
        const zip_options      z;
        const tar_options      t;
        const gz_options       g;
        const sevenzip_options s;
        const rar_options      r;
        const compress_options c;

        D_AO_CHECK(::djinterp::test::zip_options_equal(o.zip, z));
        D_AO_CHECK(::djinterp::test::tar_options_equal(o.tar, t));
        D_AO_CHECK(::djinterp::test::gz_options_equal(o.gz, g));
        D_AO_CHECK(::djinterp::test::sevenzip_options_equal(o.sevenzip, s));
        D_AO_CHECK(::djinterp::test::rar_options_equal(o.rar, r));
        D_AO_CHECK(::djinterp::test::compress_options_equal(o.codec, c));
    }

    // reached through the aggregate, the nested defaults are the documented
    // ones (a partially initialised block would surface here)
    D_AO_CHECK(o.zip.method          == zip_method_default);
    D_AO_CHECK(o.zip.utf8_names      == true);
    D_AO_CHECK(o.tar.format          == tar_format_ustar);
    D_AO_CHECK(o.gz.store_mtime      == true);
    D_AO_CHECK(o.sevenzip.solid      == true);
    D_AO_CHECK(o.sevenzip.method     == sevenzip_method_lzma2);
    D_AO_CHECK(o.rar.level           == -1);
    D_AO_CHECK(o.codec.level         == -1);

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
