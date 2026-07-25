/******************************************************************************
* djinterp [test]                          test_output_config_tests_lowering.cpp
*
*   Section II of the test_output_config suite: config lowering.
*
*   to_output_config is the whole point of the header - it takes a
* test_option_set (the C++20 configuration face) and produces the output_config
* the bundle and sinks consume (the C++17 emit vocabulary).  Two properties
* matter, and they pull in opposite directions:
*
*     WHAT IT SETS.  Five fields - pack / codec / format / compress_opts /
*     archive_opts - each lowered through the mapping it belongs to, and the two
*     aggregates copied across UNCHANGED and DEEPLY (archive_options carries a
*     nested compress_options of its own).
*
*     WHAT IT DOES NOT SET.  naming and archive_name are left at output_config's
*     defaults.  That is a documented promise, not an omission ("a caller that
*     wants file_name_pattern semantics installs a base_name_fn after lowering"),
*     and it is tested with EVERY knob turned - because the failure mode is a knob
*     leaking into a field it was never meant to reach.
*
*   requested_doc_formats is deliberately a vector of one: test_options carries a
* single `document` slot today, but the call sites are already shaped for a future
* format SET.  The tests pin both halves of that - the single element, and that it
* tracks the knob.
*
* path:      /tests/djinterp/test/test_output_config_tests_lowering.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "test_output_config_tests.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                to_output_config  -- WHAT IT SETS                         ///
///////////////////////////////////////////////////////////////////////////////

// tests_output_config_lowers_pack
//   the pack knob reaches output_config::pack, through to_pack_mode - for all
// three members, not just the interesting one.
bool
tests_output_config_lowers_pack()
{
    D_OC_CHECK(dt::to_output_config(
                   opts_with_pack(dt::test_output_pack::none)).pack == pack_mode::none);
    D_OC_CHECK(dt::to_output_config(
                   opts_with_pack(dt::test_output_pack::compress)).pack == pack_mode::compress);
    D_OC_CHECK(dt::to_output_config(
                   opts_with_pack(dt::test_output_pack::archive)).pack == pack_mode::archive);

    return true;
}


// tests_output_config_lowers_codec
//   the compressor knob reaches output_config::codec, for every codec.
bool
tests_output_config_lowers_codec()
{
    const dt::test_compressor _in[9] =
    {
        dt::test_compressor::store,   dt::test_compressor::deflate,
        dt::test_compressor::zlib,    dt::test_compressor::gzip,
        dt::test_compressor::bzip2,   dt::test_compressor::xz,
        dt::test_compressor::zstd,    dt::test_compressor::lz4,
        dt::test_compressor::brotli
    };

    std::size_t _i = 0;

    for (_i = 0; _i < 9; ++_i)
    {
        D_OC_CHECK(dt::to_output_config(opts_with_compressor(_in[_i])).codec ==
                   dt::to_codec_id(_in[_i]));
    }

    // spot-pinned by value, so the loop above cannot pass by agreeing with a
    // broken to_codec_id
    D_OC_CHECK(dt::to_output_config(
                   opts_with_compressor(dt::test_compressor::zstd)).codec == codec_id_zstd);

    return true;
}


// tests_output_config_lowers_format
//   the archive_format knob reaches output_config::format, for every container.
bool
tests_output_config_lowers_format()
{
    const dt::test_archive_format _in[6] =
    {
        dt::test_archive_format::zip,      dt::test_archive_format::tar,
        dt::test_archive_format::tar_gz,   dt::test_archive_format::gz,
        dt::test_archive_format::sevenzip, dt::test_archive_format::rar
    };

    std::size_t _i = 0;

    for (_i = 0; _i < 6; ++_i)
    {
        D_OC_CHECK(dt::to_output_config(opts_with_archive_format(_in[_i])).format ==
                   dt::to_format_id(_in[_i]));
    }

    D_OC_CHECK(dt::to_output_config(
                   opts_with_archive_format(dt::test_archive_format::sevenzip)).format ==
               format_id_sevenzip);

    return true;
}


// tests_output_config_copies_compress_opts_unchanged
//   "the per-codec aggregates copy across unchanged - compress_opts IS
// compress_options".  Asserted against MARKED values (no default produces them),
// so this cannot pass by two default-constructed structs happening to agree.
bool
tests_output_config_copies_compress_opts_unchanged()
{
    dt::test_option_set _o = make_opts();

    _o.set<dt::test_option::compress_opts>(marked_compress_options());

    const output_config _cfg = dt::to_output_config(_o);

    D_OC_CHECK(_cfg.compress_opts.level               == 7);
    D_OC_CHECK(_cfg.compress_opts.deflate.window_bits == 13);
    D_OC_CHECK(_cfg.compress_opts.deflate.mem_level   == 5);

    return true;
}


// tests_output_config_copies_archive_opts_unchanged
//   the same for the container aggregate - and archive_options owns a nested
// compress_options, so this also shows the copy is DEEP rather than a shallow
// field-by-field of the top level.
bool
tests_output_config_copies_archive_opts_unchanged()
{
    dt::test_option_set _o = make_opts();

    _o.set<dt::test_option::archive_opts>(marked_archive_options());

    const output_config _cfg = dt::to_output_config(_o);

    D_OC_CHECK(_cfg.archive_opts.level      == 6);
    D_OC_CHECK(_cfg.archive_opts.store_only == true);
    D_OC_CHECK(_cfg.archive_opts.comment    == "a marked archive");

    // the nested aggregate survived too
    D_OC_CHECK(_cfg.archive_opts.codec.level               == 7);
    D_OC_CHECK(_cfg.archive_opts.codec.deflate.window_bits == 13);

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                to_output_config  -- WHAT IT DOES *NOT* SET               ///
///////////////////////////////////////////////////////////////////////////////

// tests_output_config_leaves_naming_at_its_default
//   the documented promise: naming is left at output_config's default, so the
// write path falls back to its own naming policy.  A caller who wants
// file_name_pattern semantics installs a base_name_fn AFTER lowering - which is
// only possible if the lowering did not install one first.
bool
tests_output_config_leaves_naming_at_its_default()
{
    const output_config _default;
    const output_config _lowered = dt::to_output_config(make_opts());

    D_OC_CHECK(static_cast<bool>(_lowered.naming) ==
               static_cast<bool>(_default.naming));

    return true;
}


// tests_output_config_leaves_archive_name_at_its_default
bool
tests_output_config_leaves_archive_name_at_its_default()
{
    const output_config _default;
    const output_config _lowered = dt::to_output_config(make_opts());

    D_OC_CHECK(_lowered.archive_name == _default.archive_name);

    return true;
}


// tests_output_config_touches_nothing_else_however_configured
//   the strong form: turn EVERY knob the bridge reads, all at once, to
// non-default values - and naming / archive_name must STILL be at their defaults.
// The failure this guards against is a knob leaking into a field it was never
// meant to reach (say, archive_opts::comment being wired to archive_name because
// the names look alike).
bool
tests_output_config_touches_nothing_else_however_configured()
{
    dt::test_option_set _o = make_opts();

    _o.set<dt::test_option::pack>(dt::test_output_pack::archive);
    _o.set<dt::test_option::compressor>(dt::test_compressor::brotli);
    _o.set<dt::test_option::archive_format>(dt::test_archive_format::sevenzip);
    _o.set<dt::test_option::document>(dt::test_doc_type::pdf);
    _o.set<dt::test_option::compress_opts>(marked_compress_options());
    _o.set<dt::test_option::archive_opts>(marked_archive_options());

    const output_config _default;
    const output_config _cfg = dt::to_output_config(_o);

    // the five fields it DOES set, all lowered
    D_OC_CHECK(_cfg.pack   == pack_mode::archive);
    D_OC_CHECK(_cfg.codec  == codec_id_brotli);
    D_OC_CHECK(_cfg.format == format_id_sevenzip);
    D_OC_CHECK(_cfg.compress_opts.level == 7);
    D_OC_CHECK(_cfg.archive_opts.comment == "a marked archive");

    // ...and the two it does not, still at their defaults despite all of the above
    D_OC_CHECK(_cfg.archive_name == _default.archive_name);
    D_OC_CHECK(static_cast<bool>(_cfg.naming) == static_cast<bool>(_default.naming));

    return true;
}


// tests_output_config_lowering_goes_through_the_switch_not_a_cast
//   the cast hazards, followed all the way through the REAL lowering path.  The
// hazard section proves a cast would mis-map the mapping function; this proves the
// mis-mapping would actually reach output_config - i.e. the bug would ship.  An
// option set asking for a gzipped TARBALL must lower to format_id_tar_gz, and
// emphatically not to format_id_gz.
bool
tests_output_config_lowering_goes_through_the_switch_not_a_cast()
{
    const output_config _cfg =
        dt::to_output_config(opts_with_archive_format(dt::test_archive_format::tar_gz));

    D_OC_CHECK(_cfg.format == format_id_tar_gz);
    D_OC_CHECK(_cfg.format != format_id_gz);
    D_OC_CHECK(_cfg.format !=
               static_cast<format_id>(dt::test_archive_format::tar_gz));   // the bug

    // and the reverse selection
    const output_config _cfg2 =
        dt::to_output_config(opts_with_archive_format(dt::test_archive_format::gz));

    D_OC_CHECK(_cfg2.format == format_id_gz);
    D_OC_CHECK(_cfg2.format != format_id_tar_gz);

    return true;
}


// tests_output_config_lowers_the_framework_defaults
//   the default option set lowers to the documented default configuration:
// verbatim output, gzip if you were to compress, zip if you were to archive.
// Worth its own case because it is the config every run gets unless something
// says otherwise.
bool
tests_output_config_lowers_the_framework_defaults()
{
    const output_config _cfg = dt::to_output_config(make_opts());

    D_OC_CHECK(_cfg.pack   == pack_mode::none);      // documents emitted verbatim
    D_OC_CHECK(_cfg.codec  == codec_id_gzip);        // output_config's own default codec
    D_OC_CHECK(_cfg.format == format_id_zip);        // ...and container

    return true;
}


// tests_output_config_lowering_does_not_mutate_the_option_set
//   to_output_config takes its argument by const reference and is a pure
// projection: lowering the same set twice yields the same config, and the set is
// still readable afterwards.
bool
tests_output_config_lowering_does_not_mutate_the_option_set()
{
    const dt::test_option_set _o =
        opts_with_archive_format(dt::test_archive_format::tar_gz);

    const output_config _a = dt::to_output_config(_o);
    const output_config _b = dt::to_output_config(_o);

    D_OC_CHECK(_a.pack   == _b.pack);
    D_OC_CHECK(_a.codec  == _b.codec);
    D_OC_CHECK(_a.format == _b.format);

    // the source set is unchanged - the accessors still read what was set
    D_OC_CHECK(dt::archive_format(_o) == dt::test_archive_format::tar_gz);

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                requested_doc_formats                                     ///
///////////////////////////////////////////////////////////////////////////////

// tests_output_config_requested_doc_formats_is_single_element
//   "test_options carries one selected document type, so this yields a
// single-element list".  Exactly one - not zero, and not one per doc_format.
bool
tests_output_config_requested_doc_formats_is_single_element()
{
    const std::vector<dt::doc_format> _f =
        dt::requested_doc_formats(opts_with_document(dt::test_doc_type::html));

    D_OC_CHECK(_f.size() == 1u);
    D_OC_CHECK(!_f.empty());
    D_OC_CHECK(_f[0] == dt::doc_format::html);

    return true;
}


// tests_output_config_requested_doc_formats_tracks_the_document_knob
//   every one of the four selectable document types, through the real accessor
// path - so this covers the `document(opts)` read as well as to_doc_format.
bool
tests_output_config_requested_doc_formats_tracks_the_document_knob()
{
    const dt::test_doc_type _in[4] =
    {
        dt::test_doc_type::txt,  dt::test_doc_type::xml,
        dt::test_doc_type::html, dt::test_doc_type::pdf
    };

    const dt::doc_format _want[4] =
    {
        dt::doc_format::text, dt::doc_format::xml,
        dt::doc_format::html, dt::doc_format::pdf
    };

    std::size_t _i = 0;

    for (_i = 0; _i < 4; ++_i)
    {
        const std::vector<dt::doc_format> _f =
            dt::requested_doc_formats(opts_with_document(_in[_i]));

        D_OC_CHECK(_f.size() == 1u);
        D_OC_CHECK(_f[0] == _want[_i]);
    }

    return true;
}


// tests_output_config_requested_doc_formats_never_requests_markdown
//   the unreachability claim, at the level that actually matters: no
// CONFIGURATION can ask the framework to emit markdown, because no test_doc_type
// member maps to it.  markdown is a live doc_format with a real layout - it is
// simply not selectable from here yet.
bool
tests_output_config_requested_doc_formats_never_requests_markdown()
{
    const dt::test_doc_type _in[4] =
    {
        dt::test_doc_type::txt,  dt::test_doc_type::xml,
        dt::test_doc_type::html, dt::test_doc_type::pdf
    };

    std::size_t _i = 0;

    for (_i = 0; _i < 4; ++_i)
    {
        const std::vector<dt::doc_format> _f =
            dt::requested_doc_formats(opts_with_document(_in[_i]));

        D_OC_CHECK(_f[0] != dt::doc_format::markdown);
    }

    // ...not even from a corrupt selection, which degrades to text
    dt::test_option_set _bogus = make_opts();

    _bogus.set<dt::test_option::document>(out_of_range<dt::test_doc_type>());

    const std::vector<dt::doc_format> _f = dt::requested_doc_formats(_bogus);

    D_OC_CHECK(_f.size() == 1u);
    D_OC_CHECK(_f[0] == dt::doc_format::text);
    D_OC_CHECK(_f[0] != dt::doc_format::markdown);

    return true;
}


// tests_output_config_requested_doc_formats_returns_a_fresh_vector
//   it returns by value, so each call hands back an independent list - a caller
// may keep, extend, or scribble on the result without disturbing the next one.
// (This is the property that makes the "already shaped for a future format SET"
// design claim safe to rely on.)
bool
tests_output_config_requested_doc_formats_returns_a_fresh_vector()
{
    const dt::test_option_set _o = opts_with_document(dt::test_doc_type::pdf);

    std::vector<dt::doc_format> _a = dt::requested_doc_formats(_o);

    _a.push_back(dt::doc_format::text);
    _a.push_back(dt::doc_format::xml);

    const std::vector<dt::doc_format> _b = dt::requested_doc_formats(_o);

    D_OC_CHECK(_a.size() == 3u);      // the caller's copy, scribbled on
    D_OC_CHECK(_b.size() == 1u);      // a fresh one, untouched
    D_OC_CHECK(_b[0] == dt::doc_format::pdf);

    return true;
}


NS_END   // testing
NS_END   // djinterp
