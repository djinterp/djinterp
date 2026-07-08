/******************************************************************************
* djinterp [test]                                                test_pack.hpp
*
*   The bridge between the DTest configuration vocabulary and the portable
* compression / archive facades.  test_options.hpp lets a configuration ask
* that the report FILE be compressed or placed in an archive (the `pack`,
* `compressor`, `archive_format` knobs and the embedded compress_options /
* archive_options); this header turns that runtime request into the right
* facade call and hands back the packed bytes.
*
*   THE DISPATCH:
*   The facades (compress.hpp / archive.hpp) select a codec / format with a
* TAG TYPE at compile time (codecs::gzip, formats::zip); the configuration
* selects with a runtime ENUM (test_compressor, test_archive_format), because a
* test_option_set is a runtime value.  The one thing this header does, then, is
* the runtime -> compile-time hop: a switch over the enum that instantiates the
* matching try_compress<Tag> / try_archive<Tag>.  Everything else (the actual
* backend work) lives behind the facades' non-template leaves, compiled once in
* compress.cpp / archive.cpp.
*
*   WHAT IT PRODUCES:
*   pack_report renders the assembled report bytes into `out` per the option
* set and returns a djinterp::status.  It does NOT touch the filesystem - the
* caller (test_runner) writes `out` to output_path - so the transform stays
* pure and testable.  An unavailable codec / format yields status_unavailable
* exactly as the facades describe, and the caller can fall back to writing the
* report unpacked.
*
*   DEPENDENCY:
*   Pulling in test_options.hpp and the facades in one translation unit requires
* that the facades already source their option structs from compress_option.hpp
* / archive_option.hpp (i.e. that compress.hpp / archive.hpp include those
* headers and no longer define their own compress_options / archive_options
* stub).  Otherwise djinterp::compress_options is defined twice.  This is the
* integration those headers were written to expect.
*
*   Portable to the framework floor (C++11); it adds only a switch over the
* runtime enums on top of the facades, which are themselves C++98 - C++23.
*
*
* TABLE OF CONTENTS
* =================
* I.    INTERNAL DISPATCH        (enum -> facade tag; entry-name helpers)
* II.   AVAILABILITY             (pack_enabled, codec/format runtime queries)
* III.  PACK REPORT              (the one public entry)
*
*
* path:      /inc/djinterp/test/test_pack.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.25
******************************************************************************/

#ifndef DJINTERP_TEST_PACK_
#define DJINTERP_TEST_PACK_ 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../core/djinterp.hpp"        // NS_*, D_INLINE, D_NODISCARD
#include "../core/compress.hpp"     // byte_buffer, status, codecs::, try_*
#include "../core/archive.hpp"         // entry, entry_list, formats::, archive
#include "./test_options.hpp"          // test_option_set + the packaging enums


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   INTERNAL DISPATCH
// =========================================================================

NS_INTERNAL

    // pack_compress_helper
    //   helper: the runtime -> compile-time hop for compression.  Maps a
    // test_compressor value onto the matching codec tag and forwards to the
    // facade's non-throwing try_compress, passing the codec knob-set through.
    // An unhandled enumerator returns status_invalid_argument (it cannot occur
    // for a well-formed test_option_set).
    D_INLINE status
    pack_compress_helper(
        test_compressor          _codec,
        const byte_buffer&       _in,
        const compress_options&  _opt,
        byte_buffer&             _out
    )
    {
        using namespace codecs;

        switch (_codec)
        {
            case test_compressor::store:
                return try_compress<store>(_in, _out, _opt);
            case test_compressor::deflate:
                return try_compress<deflate>(_in, _out, _opt);
            case test_compressor::zlib:
                return try_compress<zlib>(_in, _out, _opt);
            case test_compressor::gzip:
                return try_compress<gzip>(_in, _out, _opt);
            case test_compressor::bzip2:
                return try_compress<bzip2>(_in, _out, _opt);
            case test_compressor::xz:
                return try_compress<xz>(_in, _out, _opt);
            case test_compressor::zstd:
                return try_compress<zstd>(_in, _out, _opt);
            case test_compressor::lz4:
                return try_compress<lz4>(_in, _out, _opt);
            case test_compressor::brotli:
                return try_compress<brotli>(_in, _out, _opt);
        }

        return status_invalid_argument;
    }

    // pack_archive_helper
    //   helper: the runtime -> compile-time hop for archiving.  Maps a
    // test_archive_format value onto the matching format tag and forwards to
    // try_archive, passing the container knob-set through.
    D_INLINE status
    pack_archive_helper(
        test_archive_format     _format,
        const entry_list&       _items,
        const archive_options&  _opt,
        byte_buffer&            _out
    )
    {
        using namespace formats;

        switch (_format)
        {
            case test_archive_format::zip:
                return try_archive<zip>(_items, _out, _opt);
            case test_archive_format::tar:
                return try_archive<tar>(_items, _out, _opt);
            case test_archive_format::tar_gz:
                return try_archive<tar_gz>(_items, _out, _opt);
            case test_archive_format::gz:
                return try_archive<gz>(_items, _out, _opt);
            case test_archive_format::sevenzip:
                return try_archive<sevenzip>(_items, _out, _opt);
            case test_archive_format::rar:
                return try_archive<rar>(_items, _out, _opt);
        }

        return status_invalid_argument;
    }

    // pack_doc_ext_helper
    //   helper: the file extension matching a document format, for naming the
    // entry placed inside an archive.
    D_INLINE std::string
    pack_doc_ext_helper(
        test_doc_type _doc
    )
    {
        switch (_doc)
        {
            case test_doc_type::txt:  return std::string("txt");
            case test_doc_type::xml:  return std::string("xml");
            case test_doc_type::html: return std::string("html");
            case test_doc_type::pdf:  return std::string("pdf");
        }

        return std::string("txt");
    }

    // pack_entry_name_helper
    //   helper: the name the report takes as the sole entry of an archive,
    // derived from output_path.  The directory prefix is dropped and one
    // container extension is stripped (with the leftover ".tar" of a ".tar.gz"
    // removed too), then the document extension is applied - so "out/r.zip"
    // with an html document becomes "r.html".
    D_INLINE std::string
    pack_entry_name_helper(
        const std::string& _path,
        test_doc_type      _doc
    )
    {
        std::string            name = _path;
        std::string::size_type slash = name.find_last_of("/\\");

        // basename: drop any directory prefix
        if (slash != std::string::npos)
        {
            name = name.substr(slash + 1);
        }

        if (name.empty())
        {
            name = "report";
        }

        // strip a trailing container extension, and the residual ".tar" that a
        // ".tar.<comp>" name leaves behind
        std::string::size_type dot = name.find_last_of('.');
        if ( (dot != std::string::npos) &&
             (dot != 0) )
        {
            name.erase(dot);

            if ( (name.size() >= 4) &&
                 (name.compare(name.size() - 4, 4, ".tar") == 0) )
            {
                name.erase(name.size() - 4);
            }
        }

        return name + "." + pack_doc_ext_helper(_doc);
    }

NS_END  // internal


// =========================================================================
// II.  AVAILABILITY
// =========================================================================

// pack_enabled
//   function: true iff the option set asks for any packaging (pack is not
// `none`).  The caller uses this to decide whether to buffer-and-pack the
// report file or stream it raw.
D_NODISCARD D_INLINE bool
pack_enabled(
    const test_option_set& _opts
)
{
    return (_opts.pack != test_output_pack::none);
}

// report_codec_available
//   function: runtime availability of the codec a test_compressor selects -
// whether the backend was built into this binary.  Lets the caller warn (and
// fall back) before attempting to compress.
D_NODISCARD D_INLINE bool
report_codec_available(
    test_compressor _codec
)
{
    using namespace codecs;

    switch (_codec)
    {
        case test_compressor::store:
            return codec_is_available<store>();
        case test_compressor::deflate:
            return codec_is_available<deflate>();
        case test_compressor::zlib:
            return codec_is_available<zlib>();
        case test_compressor::gzip:
            return codec_is_available<gzip>();
        case test_compressor::bzip2:
            return codec_is_available<bzip2>();
        case test_compressor::xz:
            return codec_is_available<xz>();
        case test_compressor::zstd:
            return codec_is_available<zstd>();
        case test_compressor::lz4:
            return codec_is_available<lz4>();
        case test_compressor::brotli:
            return codec_is_available<brotli>();
    }

    return false;
}

// report_format_available
//   function: runtime WRITE availability of the container a
// test_archive_format selects.
D_NODISCARD D_INLINE bool
report_format_available(
    test_archive_format _format
)
{
    using namespace formats;

    switch (_format)
    {
        case test_archive_format::zip:
            return format_is_writable<zip>();
        case test_archive_format::tar:
            return format_is_writable<tar>();
        case test_archive_format::tar_gz:
            return format_is_writable<tar_gz>();
        case test_archive_format::gz:
            return format_is_writable<gz>();
        case test_archive_format::sevenzip:
            return format_is_writable<sevenzip>();
        case test_archive_format::rar:
            return format_is_writable<rar>();
    }

    return false;
}


// =========================================================================
// III. PACK REPORT
// =========================================================================

// pack_report
//   function: render the assembled report bytes `_report` into `_out` per the
// option set's packaging configuration, returning a djinterp::status.
//     - pack == none      : `_report` is copied into `_out` (status_ok); the
//                           caller normally streams the report raw instead.
//     - pack == compress  : try_compress with the selected codec and
//                           opts.compress_opts.
//     - pack == archive   : try_archive of a single entry - the report, named
//                           from output_path and the document format - with the
//                           selected container and opts.archive_opts.
//   On a non-ok status `_out` is left empty and the caller may write `_report`
// unpacked so it is never lost.  This function performs NO file I/O.
//
// Parameter(s):
//   _opts:   the resolved option set (pack mode, selectors, knob-sets, path).
//   _report: the complete report bytes to package.
//   _out:    receives the packaged bytes on success.
// Return:
//   status_ok on success; otherwise a facade status (e.g. status_unavailable).
D_NODISCARD D_INLINE status
pack_report(
    const test_option_set&  _opts,
    const byte_buffer&      _report,
    byte_buffer&            _out
)
{
    _out.clear();

    switch (_opts.pack)
    {
        case test_output_pack::none:
        {
            _out = _report;

            return status_ok;
        }

        case test_output_pack::compress:
        {
            return internal::pack_compress_helper(
                _opts.compressor, _report, _opts.compress_opts, _out);
        }

        case test_output_pack::archive:
        {
            entry      item;
            entry_list items;

            item.name = internal::pack_entry_name_helper(
                _opts.output_path, _opts.document);
            item.data         = _report;
            item.is_directory = false;
            items.push_back(item);

            return internal::pack_archive_helper(
                _opts.archive_format, items, _opts.archive_opts, _out);
        }
    }

    return status_invalid_argument;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_PACK_
