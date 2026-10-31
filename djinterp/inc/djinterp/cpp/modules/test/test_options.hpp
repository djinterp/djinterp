/*******************************************************************************
* djinterp [cpp/test]                                           test_options.hpp
*
*   The configuration vocabulary for the DTest subframework.  Under the option
* subframework's value-carrying face, a configuration is no longer a hand-rolled
* aggregate: `test_option_set` IS an option_set whose every aspect is a keyed
* field<> slot (option_set.hpp), built on the same machinery option_generator.hpp
* exposes.  The framework reads that set through free accessors, never get<>() at
* call sites, which is what keeps a pre-C++20 backport viable.
*
*     PART A - RUNTIME CORE (C++11, ungated).  The canonical layer every other
*       test module compiles against: the value enums the knobs range over, the
*       match_context a node presents to a route, the runtime DTestPredicate IR
*       (+ value builders), test_route (+ make_route), and DTestResolved.  These
*       carry no auto NTTP, no carrier, and no option<> of their own, so they sit
*       at the framework's C++11 floor - test_kind.hpp holds a const
*       test_option_set* and test_session.hpp owns one.
*     PART B - CONFIGURATION SET (the two faces).  `test_option` keys one field<>
*       slot per aspect; `test_option_set` is the option_set over those slots
*       (C++20), or a plain-struct fallback (pre-C++20).  A uniform layer of free
*       accessors (line_width(opts), routes(opts), ...) is the ONLY read surface
*       the rest of the framework uses, so the two faces are interchangeable at
*       every call site.  default_test_options() seeds the framework defaults
*       (option_set value-initializes slots, so the non-trivial defaults are set
*       explicitly), and resolve() - necessarily a free function now - folds the
*       routes for one node into a flat DTestResolved verdict.
*     PART C - ROUTE AUTHORING (C++20).  The type-level vocabulary for writing
*       the conditional override list - a test_match predicate vocabulary and
*       route_<> - lowered by make_routes<...>() to the std::vector<test_route>
*       that seeds the `routes` slot.  Routes were always structural (a predicate
*       is a tree; the list is ordered and repeatable), so they were never an
*       DOptionGenerator concern; the flat key/value stream does not model them.
*
*   AUTHORING:
*   Scalar knobs are set imperatively on a default_test_options() result -
* opts.set<test_option::line_width>(100) - because make_option_set's flat stream
* takes NTTPs, and several slots (output_file, the format_* templates, routes,
* and the compress_opts / archive_opts aggregates) carry std::string / std::vector
* / class-type runtime values that cannot be non-type template arguments.  set<>
* is therefore the one idiom that covers EVERY knob; make_option_set remains
* available for a quick literal-only set of the NTTP-able knobs.
*
*   ON OUTPUT PACKAGING (compress_options.hpp / archive_options.hpp):
*   A configuration can ask that the report FILE be compressed or archived once
* the run finishes.  The selection knobs (pack, compressor, archive_format) are
* their own slots; the COMPLETE per-codec / per-format tuning rides two aggregate
* slots, compress_opts and archive_opts, which ARE the full surface of
* compress_options.hpp / archive_options.hpp.  Packaging is a whole-report
* decision, so it sits on the set beside output_file and is deliberately absent
* from the per-node DTestResolved.
*   PORTABILITY:
*   PART A is C++11 (the framework floor).  The value-carrying option_set face
* (PART B's test_option_set) is C++20 - the requires-guarded values constructor
* and auto-NTTP slots - so that TYPE is gated on D_ENV_LANG_IS_CPP20_OR_HIGHER,
* with a plain-struct #else arm.  PART C carries fixed_string literals and keeps
* the stricter D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS gate.  Because every
* consumer reads through the free accessors, dropping in the C++11 struct touches
* only the #else arms here - never the framework that consumes a test_option_set.
*
*
* path:      /inc/djinterp/cpp/modules/test/test_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.19
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================

      PART A - RUNTIME CORE (C++11)

      A.I.    value enums              (doc type, sink, show, packaging, ...)

      A.II.   match_context            (the facts a node presents to a route)

      A.III.  DTestPredicate           (runtime match IR + evaluate + builders)

      A.IV.   test_route               (one conditional override + builders)

      A.V.    DTestResolved            (a node's resolved, flat configuration)

      PART B - CONFIGURATION SET

      B.I.    test_option              (the slot key enum)

      B.II.   test_option_set          (option_set face | plain-struct face)

      B.III.  free accessors           (the portable read seam)

      B.IV.   default_test_options     (the defaults factory)

      B.V.    resolve                  (free fold: option set + node -> resolved)

      PART C - ROUTE AUTHORING (C++20)

      C.I.    payload carriers         (flag / choice / text via val_t)

      C.II.   override sugar           (numbering_, show_, destination_, ...)

      C.III.  test_match + predicates  (DAnyTest, DNameIs<>, all_of<>, ...)

      C.IV.   route_                   (one conditional override, type-level)

      C.V.    lowering + make_routes   (route_ ... -> std::vector<test_route>)
*/

#ifndef DJINTERP_TEST_TEST_OPTIONS_HPP
#define DJINTERP_TEST_TEST_OPTIONS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <regex>
#include <string>
#include <vector>
// djinterp
#include "../../djinterp.hpp"                // NS_*, D_*, env gating macros
#include "../../meta/fixed_string.hpp"       // fixed_string<> (authoring NTTP)
#include "../../meta/carrier.hpp"            // val_t<>
#include "../../option/option.hpp"           // option<>
#include "../../option/option_set.hpp"       // option_set<>, field<>
#include "../../option/option_generator.hpp" // make_option_set<>
#include "../../util/document/document_format.hpp" // document_format (the single
                                              //   document-format selector)
#include "../../util/compress/compress_options.hpp"   // compress_options
#include "../../util/archive/archive_options.hpp"    // archive_options
#include "./test_common.hpp"                   // DTestTypeId, test_status


NS_DJINTERP
NS_TEST

///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                   PART A - RUNTIME CORE  (C++11 floor)                  ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
///                A.I.  VALUE ENUMS                                        ///
///////////////////////////////////////////////////////////////////////////////

// test_doc_type
//   type: the document format a report is rendered as -- an ALIAS of the
// framework's single format selector, `document_format`.  It was a DTest-local
// enum spelling a subset of the same concept, which forced a hand-written
// switch to bridge to the emit layer's `doc_format` and to output_packaging;
// the alias retires the bridge and, with it, the mis-mapping bugs that come of
// keeping three orderings in step by hand.
//
//   ONE SPELLING CHANGED: the plain default is `text`, not `txt` (the core
// enum's spelling wins, and "txt" survives as an accepted alias in
// format_from_name for config strings).  `xml` / `html` / `pdf` are unchanged,
// so a runner that only ever selected PDF is untouched.  `markdown` / `tex` /
// `wiki` are now selectable here for free -- the emit layer already renders
// markdown, and simply had no way to be asked for it.
using test_doc_type = ::djinterp::document_format;

// DTestSink
//   enum: a BITSET of output destinations - a report can be written to more
// than one at once (console AND a file), so the enumerators are powers of two
// and combine under the bitwise operators below.
enum class DTestSink : unsigned
{
    none    = 0u,
    console = 1u << 0,
    file    = 1u << 1
};

// test_sink_or
//   function: the union of two sink sets (operator| spelling below).
D_NODISCARD D_CONSTEXPR DTestSink operator|(
    DTestSink _a,
    DTestSink _b
)
{
    return static_cast<DTestSink>(static_cast<unsigned>(_a) | static_cast<unsigned>(_b));
}

// test_sink_and
//   function: the intersection of two sink sets (operator& spelling below).
D_NODISCARD D_CONSTEXPR DTestSink operator&(
    DTestSink _a,
    DTestSink _b
)
{
    return static_cast<DTestSink>(static_cast<unsigned>(_a) & static_cast<unsigned>(_b));
}

// test_sink_not
//   function: the complement of a sink set within the defined bits.
D_NODISCARD D_CONSTEXPR DTestSink operator~(
    DTestSink _a
)
{
    return static_cast<DTestSink>(
        ~static_cast<unsigned>(_a) &
        ( static_cast<unsigned>(DTestSink::console) |
          static_cast<unsigned>(DTestSink::file) ));
}

// operator|=
//   function: in-place sink union.
D_INLINE
DTestSink& operator|=(
    DTestSink& _a,
    DTestSink  _b
)
{
    _a = (_a | _b);

    return _a;
}

// test_sink_has
//   function: true iff _set contains every bit of _bit.  The boolean query
// the scoped enum cannot express implicitly.
D_NODISCARD D_CONSTEXPR bool test_sink_has(
    DTestSink _set,
    DTestSink _bit
)
{
    return ( (static_cast<unsigned>(_set) & static_cast<unsigned>(_bit)) ==
             static_cast<unsigned>(_bit) ) &&
           ( static_cast<unsigned>(_bit) != 0u );
}

// DTestSinkMode
//   enum: how a route's sink set combines with what it inherits - `add`
// unions, `replace` overrides.
enum class DTestSinkMode
{
    add,
    replace
};


// DTestShow
//   enum: which results reach the report.  `all` shows every test;
// `failures_only` and `failures_and_skipped` filter; `summary_only` emits just
// the closing tally; `silent` suppresses the report entirely.
enum class DTestShow
{
    all,
    failures_only,
    failures_and_skipped,
    summary_only,
    silent
};


// DTestNumberStyle
//   enum: how tests are numbered.  `none` omits numbering; `ordinal` is a flat
// 1, 2, 3...; `hierarchical` is a dotted path mirroring the tree.
enum class DTestNumberStyle
{
    none,
    ordinal,
    hierarchical
};


// DTestTimeUnit
//   enum: the unit timings are DISPLAYED in.  `automatic` picks a
// human-readable unit per magnitude.
enum class DTestTimeUnit
{
    ns,
    us,
    ms,
    s,
    automatic
};

// DTestWrapMode
//   enum: how an over-long line is broken to fit `line_width`.  `none` leaves
// it long; `word` breaks at word boundaries; `hard` breaks at the column.
enum class DTestWrapMode
{
    none,
    word,
    hard
};

// DTestTribool
//   enum: a route toggle with an INHERIT state.  `inherit` leaves the
// inherited value untouched; `off` / `on` force it.
enum class DTestTribool
{
    inherit,
    off,
    on
};


// DTestOutputPack
//   enum: what becomes of the report FILE once the run finishes.  `none` writes
// it verbatim; `compress` runs it through a single codec; `archive` wraps it as
// the sole entry of a container.  The default is `none`.
enum class DTestOutputPack
{
    none,
    compress,
    archive
};

// DTestOutputSplit
//   enum: whether a run emits as ONE document (the whole report) or ONE
// document PER MODULE.  Orthogonal to `pack`: per_module + archive = N docs in
// one container; per_module + none = N loose files; per_module + compress = N
// individually-codec'd docs.  Default whole_run.
enum class DTestOutputSplit
{
    whole_run,
    per_module
};

// test_compressor
//   enum: the standalone codec applied when `pack` is `compress`.  `store` is a
// passthrough; the rest each need their backend present at runtime.  The
// framework default is `gzip`.  Fine per-codec tuning rides compress_opts.
enum class test_compressor
{
    store,    // no compression (passthrough)
    deflate,  // raw DEFLATE
    zlib,     // zlib-wrapped DEFLATE
    gzip,     // gzip-wrapped DEFLATE (.gz)
    bzip2,    // bzip2
    xz,       // xz / lzma
    zstd,     // Zstandard
    lz4,      // LZ4 frame
    brotli    // Brotli
};

// test_archive_format
//   enum: the container used when `pack` is `archive`.  Each needs its backend
// at runtime; `rar` creation is tool-only.  The framework default is `zip`.
// Container-level tuning rides archive_opts.
enum class test_archive_format
{
    zip,       // .zip
    tar,       // .tar (uncompressed)
    tar_gz,    // .tar.gz (gzip-compressed tarball)
    gz,        // .gz (single-entry gzip stream)
    sevenzip,  // .7z
    rar        // .rar (creation is tool-only)
};


///////////////////////////////////////////////////////////////////////////////
///                A.II. MATCH CONTEXT                                      ///
///////////////////////////////////////////////////////////////////////////////

// match_context
//   struct: the facts one test node presents to a route predicate when the
// option set is resolved for it.  A predicate reads only from here, so matching
// is a pure function of the context.
struct match_context
{
    std::string              name;
    DTestTypeId             type_id;
    test_status              status;
    std::vector<std::string> tags;

    // match_context
    //   constructor: an empty context (no name, id 0, pending, no tags).
    match_context()
        : name(),
          type_id(0),
          status(test_status::pending),
          tags()
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                A.III. TEST PREDICATE                                    ///
///////////////////////////////////////////////////////////////////////////////

// DTestMatchKind
//   enum: the runtime predicate node-kind - one value per way of matching a
// node, including the boolean combinators (all_of / any_of / negate).
enum class DTestMatchKind
{
    any,        // matches every node
    none,       // matches no node
    name_eq,    // name equals `text`
    name_has,   // name contains `text`
    name_re,    // name matches the std::regex `text`
    kind_is,    // type_id equals `type_id`
    status_is,  // status equals `status`
    tag_has,    // tags contains `text`
    all_of,     // every child predicate matches
    any_of,     // some child predicate matches
    negate      // the single child predicate does not match
};

// DTestPredicate
//   struct: the runtime match IR.  `kind` selects which fields are live; a
// value tree - the shape a route carries and a `.dj` parser (or make_routes)
// would yield.
struct DTestPredicate
{
    DTestMatchKind             kind;
    std::string                 text;
    DTestTypeId                type_id;
    test_status                 status;
    std::vector<DTestPredicate> kids;

    // DTestPredicate
    //   constructor: the always-true predicate (kind `any`).
    DTestPredicate()
        : kind(DTestMatchKind::any),
          text(),
          type_id(0),
          status(test_status::pending),
          kids()
    {}

    // evaluate
    //   function: true iff this predicate matches _ctx.  Pure; the combinators
    // recurse over `kids`.  name_re compiles `text` as a std::regex.
    D_NODISCARD bool evaluate(
        const match_context& _ctx
    ) const
    {
        switch (kind)
        {
            case DTestMatchKind::any:
            {
                return true;
            }

            case DTestMatchKind::none:
            {
                return false;
            }

            case DTestMatchKind::name_eq:
            {
                return (_ctx.name == text);
            }

            case DTestMatchKind::name_has:
            {
                return (_ctx.name.find(text) != std::string::npos);
            }

            case DTestMatchKind::name_re:
            {
                std::regex re(text);

                return std::regex_search(_ctx.name, re);
            }

            case DTestMatchKind::kind_is:
            {
                return (_ctx.type_id == type_id);
            }

            case DTestMatchKind::status_is:
            {
                return (_ctx.status == status);
            }

            case DTestMatchKind::tag_has:
            {
                std::size_t i = 0;

                // a node matches if any of its tags equals the operand
                for (i = 0; i < _ctx.tags.size(); ++i)
                {
                    if (_ctx.tags[i] == text)
                    {
                        return true;
                    }
                }

                return false;
            }

            case DTestMatchKind::all_of:
            {
                std::size_t i = 0;

                // conjunction: a single miss fails the whole
                for (i = 0; i < kids.size(); ++i)
                {
                    if (!kids[i].evaluate(_ctx))
                    {
                        return false;
                    }
                }

                return true;
            }

            case DTestMatchKind::any_of:
            {
                std::size_t i = 0;

                // disjunction: a single hit satisfies the whole
                for (i = 0; i < kids.size(); ++i)
                {
                    if (kids[i].evaluate(_ctx))
                    {
                        return true;
                    }
                }

                return false;
            }

            case DTestMatchKind::negate:
            {
                return kids.empty() ? true : !kids[0].evaluate(_ctx);
            }
        }

        return false;
    }
};


// --- value-level predicate builders ----------------------------------------
//   Free constructors: each yields a DTestPredicate, so a configuration's
// routes can be assembled by hand or by a parser without the type-level
// vocabulary in PART C.

// match_any
//   function: the always-true predicate.
D_NODISCARD D_INLINE DTestPredicate match_any()
{
    DTestPredicate p;
    p.kind = DTestMatchKind::any;

    return p;
}

// match_none
//   function: the always-false predicate.
D_NODISCARD D_INLINE DTestPredicate match_none()
{
    DTestPredicate p;
    p.kind = DTestMatchKind::none;

    return p;
}

// match_name
//   function: matches a node whose name equals _name.
D_NODISCARD D_INLINE DTestPredicate match_name(
    std::string _name
)
{
    DTestPredicate p;
    p.kind = DTestMatchKind::name_eq;
    p.text = static_cast<std::string&&>(_name);

    return p;
}

// match_name_contains
//   function: matches a node whose name contains _needle.
D_NODISCARD D_INLINE DTestPredicate match_name_contains(
    std::string _needle
)
{
    DTestPredicate p;
    p.kind = DTestMatchKind::name_has;
    p.text = static_cast<std::string&&>(_needle);

    return p;
}

// match_name_regex
//   function: matches a node whose name satisfies the std::regex _pattern.
D_NODISCARD D_INLINE DTestPredicate match_name_regex(
    std::string _pattern
)
{
    DTestPredicate p;
    p.kind = DTestMatchKind::name_re;
    p.text = static_cast<std::string&&>(_pattern);

    return p;
}

// match_kind
//   function: matches a node whose DTestTypeId equals _id.
D_NODISCARD D_INLINE DTestPredicate match_kind(
    DTestTypeId _id
)
{
    DTestPredicate p;
    p.kind    = DTestMatchKind::kind_is;
    p.type_id = _id;

    return p;
}

// match_status
//   function: matches a node whose status equals _status.
D_NODISCARD D_INLINE DTestPredicate match_status(
    test_status _status
)
{
    DTestPredicate p;
    p.kind   = DTestMatchKind::status_is;
    p.status = _status;

    return p;
}

// match_tag
//   function: matches a node carrying the tag _tag.
D_NODISCARD D_INLINE DTestPredicate match_tag(
    std::string _tag
)
{
    DTestPredicate p;
    p.kind = DTestMatchKind::tag_has;
    p.text = static_cast<std::string&&>(_tag);

    return p;
}

// match_all
//   function: the conjunction of _kids (matches iff every child matches).
D_NODISCARD D_INLINE DTestPredicate match_all(
    std::vector<DTestPredicate> _kids
)
{
    DTestPredicate p;
    p.kind = DTestMatchKind::all_of;
    p.kids = static_cast<std::vector<DTestPredicate>&&>(_kids);

    return p;
}

// match_any_of
//   function: the disjunction of _kids (matches iff some child matches).
D_NODISCARD D_INLINE DTestPredicate match_any_of(
    std::vector<DTestPredicate> _kids
)
{
    DTestPredicate p;
    p.kind = DTestMatchKind::any_of;
    p.kids = static_cast<std::vector<DTestPredicate>&&>(_kids);

    return p;
}

// match_not
//   function: the negation of _child.
D_NODISCARD D_INLINE DTestPredicate match_not(
    DTestPredicate _child
)
{
    DTestPredicate p;
    p.kind = DTestMatchKind::negate;
    p.kids.push_back(static_cast<DTestPredicate&&>(_child));

    return p;
}


///////////////////////////////////////////////////////////////////////////////
///                A.IV. TEST ROUTE                                         ///
///////////////////////////////////////////////////////////////////////////////

// test_route
//   struct: one conditional override.  When `match` matches a node, the route
// contributes its set fields to that node's resolved configuration.  Toggles
// are tristate; value overrides carry a `has_*` guard so an unset field is
// genuinely "no opinion".  `sinks` additionally carries an add-vs-replace
// `sink_mode`.  Routes are applied in declaration order (see resolve), so a
// later matching route wins on any scalar it sets.
struct test_route
{
    DTestPredicate match;

    bool           has_sinks;
    DTestSink      sinks;
    DTestSinkMode sink_mode;

    DTestTribool   timing;
    DTestTribool   numbering;
    DTestTribool   color;

    bool           has_show;
    DTestShow      show;

    bool           has_format;
    std::string    format;

    bool           has_document;
    test_doc_type  document;

    // test_route
    //   constructor: a route that matches everything and overrides nothing
    // (every toggle `inherit`, every value guard false).
    test_route()
        : match(),
          has_sinks(false),
          sinks(DTestSink::none),
          sink_mode(DTestSinkMode::add),
          timing(DTestTribool::inherit),
          numbering(DTestTribool::inherit),
          color(DTestTribool::inherit),
          has_show(false),
          show(DTestShow::all),
          has_format(false),
          format(),
          has_document(false),
          document(test_doc_type::text)
    {}
};


// make_route
//   function: a route guarded by _match overriding nothing yet.  Set the
// fields of interest on the result (it is a plain aggregate of public fields):
//
//   test_route r = make_route(match_name_contains("perf"));
//   r.timing = DTestTribool::on;        // time only the matching tests
D_NODISCARD D_INLINE test_route make_route(
    DTestPredicate _match
)
{
    test_route r;
    r.match = static_cast<DTestPredicate&&>(_match);

    return r;
}


///////////////////////////////////////////////////////////////////////////////
///                A.V.  TEST RESOLVED                                      ///
///////////////////////////////////////////////////////////////////////////////

// DTestResolved
//   struct: the flat, fully-decided configuration for ONE node - what the
// handler and printer consult after the base knobs and every matching route
// have been folded together.  No guards and no tristates remain.
struct DTestResolved
{
    bool              number_tests;
    DTestNumberStyle number_style;
    bool              show_timing;
    DTestTimeUnit    time_unit;
    bool              color;
    DTestSink         sinks;
    DTestShow         show;
    test_doc_type     document;
    std::string       format;
    std::size_t       line_width;
    bool              word_wrap;
    DTestWrapMode    wrap_mode;
    std::size_t       indent_width;
};


NS_INTERNAL

    // apply_tribool_helper
    //   helper: fold a route tristate onto an inherited boolean - `inherit`
    // keeps _base, otherwise the tristate decides.
    D_NODISCARD D_INLINE bool apply_tribool_helper(
        bool         _base,
        DTestTribool _t
    )
    {
        if (_t == DTestTribool::inherit)
        {
            return _base;
        }

        return (_t == DTestTribool::on);
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                   PART B - CONFIGURATION SET                           ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
///                B.I.  TEST OPTION  (the slot key enum)                   ///
///////////////////////////////////////////////////////////////////////////////

// test_option
//   enum: one value per configurable aspect - the KEY of an option_set slot,
// `option<test_option::K, field<T>>`.  The packaging aggregates (compress_opts /
// archive_opts) are first-class slots carrying the COMPLETE tuning surface; the
// route list is a single slot of std::vector<test_route>.
enum class test_option
{
    // numbering / timing
    numbering,        // field<bool>
    number_style,     // field<DTestNumberStyle>
    timing,           // field<bool>
    time_unit,        // field<DTestTimeUnit>

    // layout
    line_width,       // field<std::size_t>
    word_wrap,        // field<bool>
    wrap_mode,        // field<DTestWrapMode>
    indent_width,     // field<std::size_t>

    // presentation
    color,            // field<bool>
    document,         // field<test_doc_type>

    // destinations
    destination,      // field<DTestSink>
    output_file,      // field<std::string>

    // filtering
    show,             // field<DTestShow>
    max_failures,     // field<std::size_t>
    stop_on_failure,  // field<bool>

    // formats ({key} placeholder templates)
    format_test,      // field<std::string>
    format_module,    // field<std::string>
    format_summary,   // field<std::string>

    // output packaging
    pack,             // field<DTestOutputPack>
    compressor,       // field<test_compressor>
    archive_format,   // field<test_archive_format>
    compress_opts,    // field<compress_options>   (complete codec tuning)
    archive_opts,     // field<archive_options>     (complete container tuning)
    split,            // field<DTestOutputSplit>   (whole-run vs per-module)

    // conditional overrides (the ordered list, as one slot)
    routes            // field<std::vector<test_route>>
};


///////////////////////////////////////////////////////////////////////////////
///                B.II. TEST OPTION SET  (the two faces)                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// test_option_set
//   type: the runtime configuration - one keyed field<> slot per aspect,
// addressed through the free accessors below (never get<>() at call sites).
// Held by pointer wherever the framework refers to it, so its std::string /
// std::vector / aggregate slots impose no constexpr burden on consumers.
using test_option_set = option_set<
    option<test_option::numbering,       field<bool>>,
    option<test_option::number_style,    field<DTestNumberStyle>>,
    option<test_option::timing,          field<bool>>,
    option<test_option::time_unit,       field<DTestTimeUnit>>,
    option<test_option::line_width,      field<std::size_t>>,
    option<test_option::word_wrap,       field<bool>>,
    option<test_option::wrap_mode,       field<DTestWrapMode>>,
    option<test_option::indent_width,    field<std::size_t>>,
    option<test_option::color,           field<bool>>,
    option<test_option::document,        field<test_doc_type>>,
    option<test_option::destination,     field<DTestSink>>,
    option<test_option::output_file,     field<std::string>>,
    option<test_option::show,            field<DTestShow>>,
    option<test_option::max_failures,    field<std::size_t>>,
    option<test_option::stop_on_failure, field<bool>>,
    option<test_option::format_test,     field<std::string>>,
    option<test_option::format_module,   field<std::string>>,
    option<test_option::format_summary,  field<std::string>>,
    option<test_option::pack,            field<DTestOutputPack>>,
    option<test_option::compressor,      field<test_compressor>>,
    option<test_option::archive_format,  field<test_archive_format>>,
    option<test_option::compress_opts,   field<compress_options>>,
    option<test_option::archive_opts,    field<archive_options>>,
    option<test_option::split,           field<DTestOutputSplit>>,
    option<test_option::routes,          field<std::vector<test_route>>>>;

#else  // pre-C++20: the plain-struct face the accessors fall back to.  Field
       // names match the accessor spellings below, so the seam is transparent.

// test_option_set
//   struct: the C++11 floor face - named members, default-constructed to the
// framework defaults.
struct test_option_set
{
    bool                number_tests;
    DTestNumberStyle   number_style;
    bool                show_timing;
    DTestTimeUnit      time_unit;
    std::size_t         line_width;
    bool                word_wrap;
    DTestWrapMode      wrap_mode;
    std::size_t         indent_width;
    bool                color;
    test_doc_type       document;
    DTestSink           sinks;
    std::string         output_path;
    DTestShow           show;
    std::size_t         max_failures;
    bool                stop_on_failure;
    std::string         format_test;
    std::string         format_module;
    std::string         format_summary;
    DTestOutputPack    pack;
    test_compressor     compressor;
    test_archive_format archive_format;
    compress_options    compress_opts;
    archive_options     archive_opts;

    DTestOutputSplit   split;

    std::vector<test_route> routes;

    // test_option_set
    //   constructor: the framework defaults - numbered, ordinal, untimed,
    // 80-column word-wrapped plain text to the console, showing all results,
    // no failure cap, output UNPACKED.
    test_option_set()
        : number_tests(true),
          number_style(DTestNumberStyle::ordinal),
          show_timing(false),
          time_unit(DTestTimeUnit::automatic),
          line_width(80),
          word_wrap(true),
          wrap_mode(DTestWrapMode::word),
          indent_width(2),
          color(true),
          document(test_doc_type::text),
          sinks(DTestSink::console),
          output_path(),
          show(DTestShow::all),
          max_failures(0),
          stop_on_failure(false),
          format_test("{index}. {name} [{status}] {duration}"),
          format_module("{name}"),
          format_summary("{passed}/{total} passed, {failed} failed, "
                         "{skipped} skipped"),
          pack(DTestOutputPack::none),
          compressor(test_compressor::gzip),
          archive_format(test_archive_format::zip),
          split(DTestOutputSplit::whole_run),
          routes()
          // compress_opts / archive_opts default-construct (backend defaults)
    {}
};

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///                B.III. FREE ACCESSORS  (the portable read seam)          ///
///////////////////////////////////////////////////////////////////////////////
//
//   ONE read accessor per knob - the single surface the rest of the framework
// reads through, so the two faces are interchangeable at every call site.  The
// two arms differ only in the body (slot fetch vs member fetch); the signature
// is spelled with an explicit `const T&`, so the C++11 arm needs no return-type
// deduction.

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // D_INTERNAL_TEST_RO
    //   macro: define a read accessor that fetches the option_set slot _key.
    #define D_INTERNAL_TEST_RO(_accessor, _key, _member, _type)               \
        D_NODISCARD D_INLINE const _type&                                     \
        _accessor(                                                            \
            const test_option_set& _o                                         \
        )                                                                     \
        {                                                                     \
            return _o.template get<_key>();                                   \
        }

#else

    // D_INTERNAL_TEST_RO
    //   macro: define a read accessor that fetches the struct member _member.
    #define D_INTERNAL_TEST_RO(_accessor, _key, _member, _type)               \
        D_NODISCARD D_INLINE const _type&                                     \
        _accessor(                                                            \
            const test_option_set& _o                                         \
        )                                                                     \
        {                                                                     \
            return _o._member;                                                \
        }

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

//   the accessor table.  (accessor, slot key, C++11 member, value type.)
D_INTERNAL_TEST_RO(numbering,       test_option::numbering,       number_tests,    bool)
D_INTERNAL_TEST_RO(number_style,    test_option::number_style,    number_style,    DTestNumberStyle)
D_INTERNAL_TEST_RO(show_timing,     test_option::timing,          show_timing,     bool)
D_INTERNAL_TEST_RO(time_unit,       test_option::time_unit,       time_unit,       DTestTimeUnit)
D_INTERNAL_TEST_RO(line_width,      test_option::line_width,      line_width,      std::size_t)
D_INTERNAL_TEST_RO(word_wrap,       test_option::word_wrap,       word_wrap,       bool)
D_INTERNAL_TEST_RO(wrap_mode,       test_option::wrap_mode,       wrap_mode,       DTestWrapMode)
D_INTERNAL_TEST_RO(indent_width,    test_option::indent_width,    indent_width,    std::size_t)
D_INTERNAL_TEST_RO(color,           test_option::color,           color,           bool)
D_INTERNAL_TEST_RO(document,        test_option::document,        document,        test_doc_type)
D_INTERNAL_TEST_RO(sinks,           test_option::destination,     sinks,           DTestSink)
D_INTERNAL_TEST_RO(output_path,     test_option::output_file,     output_path,     std::string)
D_INTERNAL_TEST_RO(show,            test_option::show,            show,            DTestShow)
D_INTERNAL_TEST_RO(max_failures,    test_option::max_failures,    max_failures,    std::size_t)
D_INTERNAL_TEST_RO(stop_on_failure, test_option::stop_on_failure, stop_on_failure, bool)
D_INTERNAL_TEST_RO(format_test,     test_option::format_test,     format_test,     std::string)
D_INTERNAL_TEST_RO(format_module,   test_option::format_module,   format_module,   std::string)
D_INTERNAL_TEST_RO(format_summary,  test_option::format_summary,  format_summary,  std::string)
D_INTERNAL_TEST_RO(pack,            test_option::pack,            pack,            DTestOutputPack)
D_INTERNAL_TEST_RO(compressor,      test_option::compressor,      compressor,      test_compressor)
D_INTERNAL_TEST_RO(archive_format,  test_option::archive_format,  archive_format,  test_archive_format)
D_INTERNAL_TEST_RO(compress_opts,   test_option::compress_opts,   compress_opts,   compress_options)
D_INTERNAL_TEST_RO(archive_opts,    test_option::archive_opts,    archive_opts,    archive_options)
D_INTERNAL_TEST_RO(split,           test_option::split,           split,           DTestOutputSplit)
D_INTERNAL_TEST_RO(routes,          test_option::routes,          routes,          std::vector<test_route>)

#undef D_INTERNAL_TEST_RO


///////////////////////////////////////////////////////////////////////////////
///                B.IV. DEFAULT TEST OPTIONS  (the factory)                ///
///////////////////////////////////////////////////////////////////////////////

// default_test_options
//   function: a fully-defaulted configuration - numbered, ordinal, untimed,
// 80-column word-wrapped plain text to the console, showing all results, no
// failure cap, written UNPACKED.  Needed (on the option_set face) because
// option_set value-initializes its slots, so the non-trivial floor defaults are
// seeded explicitly rather than riding an aggregate constructor.
//
// Return:
//   a complete, usable test_option_set.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

D_NODISCARD D_INLINE test_option_set default_test_options()
{
    test_option_set o;   // every slot value-initialized

    // seed only the knobs whose default is not the value-initialized one
    o.set<test_option::numbering>(true);
    o.set<test_option::number_style>(DTestNumberStyle::ordinal);
    o.set<test_option::time_unit>(DTestTimeUnit::automatic);
    o.set<test_option::line_width>(static_cast<std::size_t>(80));
    o.set<test_option::word_wrap>(true);
    o.set<test_option::wrap_mode>(DTestWrapMode::word);
    o.set<test_option::indent_width>(static_cast<std::size_t>(2));
    o.set<test_option::color>(true);
    o.set<test_option::destination>(DTestSink::console);
    o.set<test_option::compressor>(test_compressor::gzip);
    o.set<test_option::archive_format>(test_archive_format::zip);
    o.set<test_option::format_test>(
        std::string("{index}. {name} [{status}] {duration}"));
    o.set<test_option::format_module>(std::string("{name}"));
    o.set<test_option::format_summary>(
        std::string("{passed}/{total} passed, {failed} failed, "
                    "{skipped} skipped"));

    return o;
}

#else

D_NODISCARD D_INLINE test_option_set default_test_options()
{
    return test_option_set();   // the struct's constructor carries the defaults
}

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///                B.V.  RESOLVE  (free fold over accessors)                ///
///////////////////////////////////////////////////////////////////////////////

// resolve
//   function: the flat configuration for a node described by _ctx.  Seeds a
// DTestResolved from the base knobs (read through the accessors, so the body is
// face-agnostic), then folds every route whose predicate matches _ctx, IN
// DECLARATION ORDER: toggles fold through their tristate; guarded values
// overwrite (later wins); sinks union (add) or replace (replace) the inherited
// set.  Free rather than a member because the option_set face is an alias this
// header does not own - and the accessor seam is what makes that painless.
//
// Parameter(s):
//   _opts: the configuration to resolve against.
//   _ctx:  the facts of the node being resolved.
// Return:
//   the node's fully-decided configuration.
D_NODISCARD D_INLINE DTestResolved resolve(
    const test_option_set& _opts,
    const match_context&   _ctx
)
{
    DTestResolved r;
    std::size_t   i = 0;

    // seed from the base knobs (accessor reads - slot or member, transparently)
    r.number_tests = numbering(_opts);
    r.number_style = number_style(_opts);
    r.show_timing  = show_timing(_opts);
    r.time_unit    = time_unit(_opts);
    r.color        = color(_opts);
    r.sinks        = sinks(_opts);
    r.show         = show(_opts);
    r.document     = document(_opts);
    r.format       = format_test(_opts);
    r.line_width   = line_width(_opts);
    r.word_wrap    = word_wrap(_opts);
    r.wrap_mode    = wrap_mode(_opts);
    r.indent_width = indent_width(_opts);

    const std::vector<test_route>& rts = routes(_opts);

    // fold every matching route in declaration order
    for (i = 0; i < rts.size(); ++i)
    {
        const test_route& rt = rts[i];

        // a route contributes nothing unless its predicate matches
        if (!rt.match.evaluate(_ctx))
        {
            continue;
        }

        r.number_tests =
            internal::apply_tribool_helper(r.number_tests, rt.numbering);
        r.show_timing  =
            internal::apply_tribool_helper(r.show_timing, rt.timing);
        r.color        =
            internal::apply_tribool_helper(r.color, rt.color);

        // guarded scalar overrides: later matching route wins
        if (rt.has_show)
        {
            r.show = rt.show;
        }

        if (rt.has_format)
        {
            r.format = rt.format;
        }

        if (rt.has_document)
        {
            r.document = rt.document;
        }

        // sinks: union or replace per the route's mode
        if (rt.has_sinks)
        {
            if (rt.sink_mode == DTestSinkMode::replace)
            {
                r.sinks = rt.sinks;
            }
            else
            {
                r.sinks = (r.sinks | rt.sinks);
            }
        }
    }

    return r;
}


#if ( D_ENV_LANG_IS_CPP20_OR_HIGHER &&                                        \
      D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS )


///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                   PART C - ROUTE AUTHORING  (C++20)                     ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////

//   The type-level vocabulary for writing the conditional override list, lowered
// to the std::vector<test_route> that seeds the `routes` slot.  Routes are
// structural (a predicate is a tree; the list is ordered), so they ride the
// explicit option<> spelling rather than DOptionGenerator's flat stream.  The
// scalar knobs are NOT authored here - they are set imperatively (PART B.IV).


// ===========================================================================
// C.I.  payload carriers
// ===========================================================================
//   Each payload is a val_t<V> (meta/carrier.hpp), the framework's generic
// single-value carrier.  text carries a fixed_string authored at the literal
// boundary; the lowering reads its view().

// flag
//   alias: a boolean payload (a toggle's state).
template<bool B>
using flag = val_t<B>;

// choice
//   alias: an enumerated-value payload (a show mode, a sink, a document type).
template<auto Value>
using choice = val_t<Value>;

// text
//   alias: a string payload, carried as the authored fixed_string itself.
template<fixed_string Str>
using text = val_t<Str>;


// ===========================================================================
// C.II. override sugar
// ===========================================================================
//   The node aliases honored as ROUTE OVERRIDES - exactly the set the route
// layer folds onto a test_route (B.IV scalar knobs are set imperatively, so
// their sugar is retired).  Toggles read as tristates here.

// numbering_
//   type: force numbering on/off for the matching tests.
template<bool On = true>
using DNumbering = option<test_option::numbering, flag<On>>;

// timing_
//   type: time only / force-untime the matching tests.
template<bool On = true>
using DTiming = option<test_option::timing, flag<On>>;

// color_
//   type: force colorization on/off for the matching tests.
template<bool On = true>
using DColor = option<test_option::color, flag<On>>;

// show_
//   type: select which results reach the report for the matching tests.
template<DTestShow Show>
using DShow = option<test_option::show, choice<Show>>;

// document_
//   type: select the document format for the matching tests.
template<test_doc_type Doc>
using DDocument = option<test_option::document, choice<Doc>>;

// DFormatTest
//   type: set the per-test line template for the matching tests.
template<fixed_string Fmt>
using DFormatTest = option<test_option::format_test, text<Fmt>>;

// destination_
//   type: REPLACE the inherited destination for the matching tests - route
// them exclusively there.
template<DTestSink Sink>
using DDestination =
    option<test_option::destination, choice<Sink>, choice<DTestSinkMode::replace>>;

// DDestinationAdd
//   type: ADD a destination for the matching tests - "also send these here"
// while everything else continues to its inherited sink(s).
template<DTestSink Sink>
using DDestinationAdd =
    option<test_option::destination, choice<Sink>, choice<DTestSinkMode::add>>;


// ===========================================================================
// C.III. test_match + predicate vocabulary
// ===========================================================================

// test_match
//   enum: the type-level predicate node-kind, parallel to the runtime
// DTestMatchKind.  A predicate is `option<test_match::K, payload/child...>`.
enum class test_match
{
    any,        // matches every node
    none,       // matches no node
    name_eq,    // name equals the carried literal
    name_has,   // name contains the carried literal
    name_re,    // name matches the carried regex literal
    kind_is,    // type_id equals the carried id
    status_is,  // status equals the carried status
    tag_has,    // tags contains the carried literal
    all_of,     // every child predicate matches
    any_of,     // some child predicate matches
    negate      // the single child predicate does not match
};

// DAnyTest
//   type: the always-true predicate.
using DAnyTest = option<test_match::any>;

// DNoTest
//   type: the always-false predicate.
using DNoTest = option<test_match::none>;

// DNameIs
//   type: matches a node whose name equals Name.
template<fixed_string Name>
using DNameIs = option<test_match::name_eq, text<Name>>;

// name_has
//   type: matches a node whose name contains Needle.
template<fixed_string Needle>
using name_has = option<test_match::name_has, text<Needle>>;

// name_re
//   type: matches a node whose name satisfies the regex Pattern.
template<fixed_string Pattern>
using name_re = option<test_match::name_re, text<Pattern>>;

// kind_is
//   type: matches a node whose DTestTypeId equals Id.
template<DTestTypeId Id>
using kind_is = option<test_match::kind_is, choice<Id>>;

// status_is
//   type: matches a node whose status equals Status.
template<test_status Status>
using status_is = option<test_match::status_is, choice<Status>>;

// DTagIs
//   type: matches a node carrying the tag Tag.
template<fixed_string Tag>
using DTagIs = option<test_match::tag_has, text<Tag>>;

// all_of
//   type: the conjunction of the child predicates Predicates.
template<typename... Predicates>
using all_of = option<test_match::all_of, Predicates...>;

// any_of
//   type: the disjunction of the child predicates Predicates.
template<typename... Predicates>
using any_of = option<test_match::any_of, Predicates...>;

// not_
//   type: the negation of the child predicate Pred.
template<typename Pred>
using not_ = option<test_match::negate, Pred>;


// ===========================================================================
// C.IV. route_
// ===========================================================================

// DTestRouteTag
//   enum: the key of the route AUTHORING type.  Distinct from test_option (a
// route is not an option_set slot - the `routes` slot holds the LOWERED
// std::vector<test_route>), so route_ never collides with a configuration key.
enum class DTestRouteTag
{
    route
};

// route_
//   type: one conditional override - a predicate followed by override options.
// Predicate is a PART C.III predicate and each Override is a C.II alias (a
// toggle read as a tristate, or a guarded value).  The args are an opaque,
// ordered pack.  Lowered to a test_route by make_routes.
//
// Usage:
//   route_<name_has<"perf">, timing_<true>>                  // time only perf
//   route_<DNameIs<"login">, DDestinationAdd<DTestSink::console>>
template<typename    Predicate,
         typename... Overrides>
using DRoute = option<DTestRouteTag::route, Predicate, Overrides...>;


// ===========================================================================
// C.V.  lowering + make_routes
// ===========================================================================

NS_INTERNAL

    // -- predicate lowering -------------------------------------------------

    // lower_predicate_helper
    //   trait: a type-level predicate (test_match AST) -> DTestPredicate
    // (specialized per node-kind below).
    template<typename Pred>
    struct lower_predicate_helper;

    // lower_predicate_value_helper
    //   function: lower_predicate_helper<Pred>::go() as a callable, so a child
    // predicate lowers inline within a combinator's brace-init.
    template<typename Pred>
    D_NODISCARD DTestPredicate lower_predicate_value_helper()
    {
        return lower_predicate_helper<Pred>::go();
    }

    template<>
    struct lower_predicate_helper<option<test_match::any>>
    {
        static DTestPredicate go() { return match_any(); }
    };

    template<>
    struct lower_predicate_helper<option<test_match::none>>
    {
        static DTestPredicate go() { return match_none(); }
    };

    template<fixed_string Name>
    struct lower_predicate_helper<option<test_match::name_eq, val_t<Name>>>
    {
        static
        DTestPredicate go()
        {
            return match_name(std::string(Name.view()));
        }
    };

    template<fixed_string Needle>
    struct lower_predicate_helper<option<test_match::name_has, val_t<Needle>>>
    {
        static
        DTestPredicate go()
        {
            return match_name_contains(std::string(Needle.view()));
        }
    };

    template<fixed_string Pattern>
    struct lower_predicate_helper<option<test_match::name_re, val_t<Pattern>>>
    {
        static
        DTestPredicate go()
        {
            return match_name_regex(std::string(Pattern.view()));
        }
    };

    template<DTestTypeId Id>
    struct lower_predicate_helper<option<test_match::kind_is, val_t<Id>>>
    {
        static DTestPredicate go() { return match_kind(Id); }
    };

    template<test_status Status>
    struct lower_predicate_helper<option<test_match::status_is, val_t<Status>>>
    {
        static DTestPredicate go() { return match_status(Status); }
    };

    template<fixed_string Tag>
    struct lower_predicate_helper<option<test_match::tag_has, val_t<Tag>>>
    {
        static
        DTestPredicate go()
        {
            return match_tag(std::string(Tag.view()));
        }
    };

    template<typename... Predicates>
    struct lower_predicate_helper<option<test_match::all_of, Predicates...>>
    {
        static
        DTestPredicate go()
        {
            return match_all(
                std::vector<DTestPredicate>{
                    lower_predicate_value_helper<Predicates>()... });
        }
    };

    template<typename... Predicates>
    struct lower_predicate_helper<option<test_match::any_of, Predicates...>>
    {
        static
        DTestPredicate go()
        {
            return match_any_of(
                std::vector<DTestPredicate>{
                    lower_predicate_value_helper<Predicates>()... });
        }
    };

    template<typename Pred>
    struct lower_predicate_helper<option<test_match::negate, Pred>>
    {
        static
        DTestPredicate go()
        {
            return match_not(lower_predicate_value_helper<Pred>());
        }
    };


    // -- route override lowering --------------------------------------------

    // DApplyRouteOverrideHelper
    //   trait: fold one override option onto a test_route (primary handles the
    // toggles + values the route layer supports; an unsupported key trips the
    // static_assert so it is caught at the authoring boundary).
    template<typename Override>
    struct DApplyRouteOverrideHelper
    {
        static
        void to(
            test_route&
        )
        {
            static_assert(sizeof(Override) == 0,
                "test route: this option is not supported as a route "
                "override.  Routes honor numbering_/timing_/color_ (as "
                "tristate toggles), show_, document_, format_test_, and "
                "destination_/destination_add_.");
        }
    };

    template<bool On>
    struct DApplyRouteOverrideHelper<option<test_option::numbering, val_t<On>>>
    {
        static
        void to(
            test_route& _r
        )
        {
            _r.numbering = On ? DTestTribool::on : DTestTribool::off;
        }
    };

    template<bool On>
    struct DApplyRouteOverrideHelper<option<test_option::timing, val_t<On>>>
    {
        static
        void to(
            test_route& _r
        )
        {
            _r.timing = On ? DTestTribool::on : DTestTribool::off;
        }
    };

    template<bool On>
    struct DApplyRouteOverrideHelper<option<test_option::color, val_t<On>>>
    {
        static
        void to(
            test_route& _r
        )
        {
            _r.color = On ? DTestTribool::on : DTestTribool::off;
        }
    };

    template<DTestShow Show>
    struct DApplyRouteOverrideHelper<option<test_option::show, val_t<Show>>>
    {
        static
        void to(
            test_route& _r
        )
        {
            _r.has_show = true;
            _r.show     = Show;
        }
    };

    template<test_doc_type Doc>
    struct DApplyRouteOverrideHelper<option<test_option::document, val_t<Doc>>>
    {
        static
        void to(
            test_route& _r
        )
        {
            _r.has_document = true;
            _r.document     = Doc;
        }
    };

    template<fixed_string Fmt>
    struct DApplyRouteOverrideHelper<option<test_option::format_test, val_t<Fmt>>>
    {
        static
        void to(
            test_route& _r
        )
        {
            _r.has_format = true;
            _r.format     = std::string(Fmt.view());
        }
    };

    template<DTestSink      Sink,
             DTestSinkMode Mode>
    struct DApplyRouteOverrideHelper<
        option<test_option::destination, val_t<Sink>, val_t<Mode>>>
    {
        static
        void to(
            test_route& _r
        )
        {
            _r.has_sinks = true;
            _r.sinks     = Sink;
            _r.sink_mode = Mode;
        }
    };


    // -- route lowering -----------------------------------------------------

    // lower_route_helper
    //   trait: one route_ option -> test_route.  Lowers the predicate child,
    // then folds each override onto the route in order.
    template<typename Route>
    struct lower_route_helper;

    template<typename    Predicate,
             typename... Overrides>
    struct lower_route_helper<
        option<DTestRouteTag::route, Predicate, Overrides...>>
    {
        static
        test_route go()
        {
            test_route r =
                make_route(lower_predicate_value_helper<Predicate>());

            // fold the overrides left to right (pack-expansion over a brace
            // list sequences the side effects)
            int sink[] = { 0,
                ( DApplyRouteOverrideHelper<Overrides>::to(r), 0 )... };
            (void) sink;

            return r;
        }
    };

NS_END  // internal


// make_routes
//   function: lower a pack of route_<> authoring types to a runtime
// std::vector<test_route>, in declaration order, ready to seed the `routes`
// slot.  This free function replaces the former routes_ wrapper, which existed
// only to pack multiple routes under one option_set key.
//
// Usage:
//   opts.set<test_option::routes>(make_routes<
//       route_<name_has<"perf">, timing_<true>>,
//       route_<DNameIs<"login">, DDestinationAdd<DTestSink::console>> >());
template<typename... Routes>
D_NODISCARD std::vector<test_route> make_routes()
{
    return std::vector<test_route>{
        internal::lower_route_helper<Routes>::go()... };
}


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && ... NONTYPE_TEMPLATE_ARGS


NS_END  // test
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_TEST_TEST_OPTIONS_HPP
