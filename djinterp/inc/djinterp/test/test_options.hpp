/******************************************************************************
* djinterp [test]                                             test_options.hpp
*
*   The configuration vocabulary for the DTest subframework, in the two
* shapes the framework already dispatches over - the same split cli.hpp /
* cli_render.hpp draw, and for the same reason: a single configuration is
* authored once and consumed both at compile time (as a type) and at runtime
* (as a value).
*
*     PART A - RUNTIME CORE (C++11, ungated).  The canonical layer every
*       other test module compiles against.  A concrete `test_option_set`
*       aggregate of plain scalar knobs plus an ordered list of `test_route`
*       overrides, and the value enums those knobs range over.  test_kind.hpp
*       holds a `const test_option_set*` and test_session.hpp owns one for
*       quasi-global configuration, so this layer MUST be available at the
*       framework's C++11 floor - it carries no auto NTTP, no carrier, and no
*       option<> of its own.  `test_option_set::resolve(match_context)` folds
*       the routes for one node into a flat `test_resolved` verdict; that fold
*       is what realizes the "send THIS test elsewhere / time only THESE
*       tests" expressions.
*     PART B - COMPILE-TIME VOCABULARY (C++20).  The authoring surface "as per
*       the option subframework": a `test_option` node-kind enum keying an
*       option<> per configurable aspect (exactly the reading option.hpp
*       invites - "the key is the node-kind, the args are the payload"), the
*       payload carriers (val_t<>), the intention-revealing node sugar
*       (numbering_<>, line_width_<>, document_<>, ...), a parallel
*       `test_match` predicate vocabulary, and route_<>/routes_<> for the
*       ordered, repeatable override list.  A whole configuration is one
*       `compose_options_t<...>`; precedence (defaults (+) session (+) route)
*       is option_set_override_t, the note's precedence union.
*     PART C - LOWERING BRIDGE (C++20).  The one bridge between the halves,
*       `test_options_lower`, distils a type-level schema into a runtime
*       `test_option_set` - the exact role dji_cli_lower plays for the CLI AST,
*       so a configuration authored once as a type can drive the runtime
*       framework after a single lowering call.
*
*   ON THE FORMATTING DSL (cli.hpp / cli_render.hpp / cli_string.hpp):
*   The CLI language's CONTENT and CONTROL core - substitution, conditionals,
* iteration, Markov rewriting - is already sufficient to describe report
* content, so per-line formats here are ordinary `{key}` placeholder strings
* (the text_template.hpp model) carried as data on test_option_set.  Its
* FORMATTING set (bold / upper / lower / indent) was NOT sufficient for the
* aligned, width-bounded, word-wrapped output this configuration exposes
* (line_width, word_wrap, alignment); that gap is closed in the companion
* revision of cli_string.hpp, which adds the constexpr layout algebra (fill /
* repeat / align / truncate / wrap / rule) the printer composes when realizing
* a line under a resolved width.  No new `dji_cli` node-kind was required, so
* the closed node-kind enum and the runtime renderer's dispatch are untouched;
* the algebra is leaf string operations callable directly (and trivially
* promotable to node-kinds later - one enumerator plus one specialization each,
* per the recipe noted in cli_string.hpp).
*
*   PORTABILITY:
*   PART A is C++11 (the framework floor).  PARTS B and C self-suppress below
* C++20 / class-type non-type template arguments, mirroring cli.hpp; where the
* vocabulary is unavailable the runtime core and its value-level builders
* remain the portable path (a parser or hand construction can build a
* test_option_set directly).
*
*
* TABLE OF CONTENTS
* =================
* PART A - RUNTIME CORE (C++11)
*   A.I.    value enums              (doc type, sink, show, numbering, ...)
*   A.II.   match_context            (the facts a node presents to a route)
*   A.III.  test_predicate           (runtime match IR + evaluate + builders)
*   A.IV.   test_route               (one conditional override + builders)
*   A.V.    test_resolved            (a node's resolved, flat configuration)
*   A.VI.   test_option_set          (the concrete option aggregate + resolve)
*
* PART B - COMPILE-TIME VOCABULARY (C++20)
*   B.I.    test_option              (the configurable-aspect node-kind enum)
*   B.II.   payload carriers         (flag / count / choice / text via val_t)
*   B.III.  node sugar               (numbering_, line_width_, document_, ...)
*   B.IV.   test_match + predicates  (any_test, name_is<>, all_of<>, ...)
*   B.V.    route_ / routes_         (the ordered, repeatable override list)
*   B.VI.   test_config              (compose a whole schema in one statement)
*
* PART C - LOWERING BRIDGE (C++20)
*   C.I.    predicate lowering       (test_match AST -> test_predicate)
*   C.II.   route lowering           (route_ -> test_route)
*   C.III.  test_options_lower       (schema -> test_option_set)
*
*
* path:      /inc/djinterp/test/test_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.19
******************************************************************************/

#ifndef DJINTERP_TEST_OPTIONS_
#define DJINTERP_TEST_OPTIONS_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
#include <regex>
// djinterp
#include "../core/djinterp.hpp"                // NS_*, D_CONSTEXPR, D_NODISCARD,
#include "../core/meta/fixed_string.hpp"       // fixed_string<> (authoring NTTP)
#include "../core/meta/carrier.hpp"            // val_t<>, is_value_carrier_v
#include "../core/option/option.hpp"           // option<>
#include "../core/option/option_set.hpp"       // option_set<> + find / contains
#include "../core/option/option_override.hpp"  // option_set_override_t
#include "../core/option/option_compose.hpp"   // compose_options_t, defopt
#include "./test_common.hpp"                   // test_type_id, test_status

// The compile-time authoring vocabulary (PART B) and the lowering bridge
// (PART C) take class-type non-type template arguments and ride the option<>
// substrate; below C++20 they contribute nothing and the runtime core (PART A)
// plus its value-level builders remain the portable path.  These pulls are
// guarded so the C++11 floor never drags in the auto-NTTP headers.
#if !( D_ENV_LANG_IS_CPP20_OR_HIGHER &&                                       \
       D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS )
    #error ""
#endif


NS_DJINTERP
NS_TEST

///////////////////////////////////////////////////////////////////////////////
///                A.I.  VALUE ENUMS                                        ///
///////////////////////////////////////////////////////////////////////////////

// test_doc_type
//   enum: the document format a report is rendered as.  `txt` is the plain
// default; `xml` / `html` / `pdf` select the corresponding emitter (the
// framework is assumed to carry those headers).
enum class test_doc_type
{
    txt,
    xml,
    html,
    pdf
};

// test_sink
//   enum: a BITSET of output destinations - a report can be written to more
// than one at once (console AND a file), so the enumerators are powers of two
// and combine under the bitwise operators below.  Scoped for type safety; the
// operators and `test_sink_has` restore the flag ergonomics.
enum class test_sink : unsigned
{
    none    = 0u,
    console = 1u << 0,
    file    = 1u << 1
};

// test_sink_or
//   function: the union of two sink sets (operator| spelling below).
D_NODISCARD D_CONSTEXPR test_sink
operator|(
    test_sink _a,
    test_sink _b
)
{
    return static_cast<test_sink>(
        static_cast<unsigned>(_a) | static_cast<unsigned>(_b));
}

// test_sink_and
//   function: the intersection of two sink sets (operator& spelling below).
D_NODISCARD D_CONSTEXPR test_sink
operator&(
    test_sink _a,
    test_sink _b
)
{
    return static_cast<test_sink>(
        static_cast<unsigned>(_a) & static_cast<unsigned>(_b));
}

// test_sink_not
//   function: the complement of a sink set within the defined bits.
D_NODISCARD D_CONSTEXPR test_sink
operator~(
    test_sink _a
)
{
    return static_cast<test_sink>(
        ~static_cast<unsigned>(_a) &
        ( static_cast<unsigned>(test_sink::console) |
          static_cast<unsigned>(test_sink::file) ));
}

// operator|=
//   function: in-place sink union.
D_INLINE test_sink&
operator|=(
    test_sink& _a,
    test_sink  _b
)
{
    _a = (_a | _b);

    return _a;
}

// test_sink_has
//   function: true iff _set contains every bit of _bit.  The boolean query
// the scoped enum cannot express implicitly.
D_NODISCARD D_CONSTEXPR bool
test_sink_has(
    test_sink _set,
    test_sink _bit
)
{
    return ( (static_cast<unsigned>(_set) & static_cast<unsigned>(_bit)) ==
             static_cast<unsigned>(_bit) ) &&
           ( static_cast<unsigned>(_bit) != 0u );
}


// test_sink_mode
//   enum: how a route's sink set combines with what it inherits - `add`
// unions (send this test THERE in addition to wherever everything else goes),
// `replace` overrides (send this test ONLY there).
enum class test_sink_mode
{
    add,
    replace
};


// test_show
//   enum: which results reach the report.  `all` shows every test;
// `failures_only` and `failures_and_skipped` filter to the interesting
// outcomes; `summary_only` emits just the closing tally; `silent` suppresses
// the report entirely (counters still accrue).
enum class test_show
{
    all,
    failures_only,
    failures_and_skipped,
    summary_only,
    silent
};


// test_number_style
//   enum: how tests are numbered in the report.  `none` omits numbering;
// `ordinal` is a flat 1, 2, 3...; `hierarchical` is a dotted path mirroring
// the tree (1, 1.1, 1.2, 2...).
enum class test_number_style
{
    none,
    ordinal,
    hierarchical
};


// test_time_unit
//   enum: the unit timings are DISPLAYED in (the measurement itself is the
// timer's `_Duration`; see test_timer.hpp).  `automatic` picks a
// human-readable unit per magnitude.
enum class test_time_unit
{
    ns,
    us,
    ms,
    s,
    automatic
};

// test_wrap_mode
//   enum: how an over-long line is broken to fit `line_width`.  `none` leaves
// it long; `word` breaks at word boundaries (greedy); `hard` breaks at the
// column regardless of word boundaries.  Realized by the cli_string layout
// algebra (dji_cli_wrap) the printer composes.
enum class test_wrap_mode
{
    none,
    word,
    hard
};

// test_tribool
//   enum: a route toggle with an INHERIT state.  `inherit` leaves the
// inherited value untouched; `off` / `on` force it.  Distinct from a plain
// bool precisely so a route can say "do not change numbering here" as opposed
// to "force numbering off here".
enum class test_tribool
{
    inherit,
    off,
    on
};

///////////////////////////////////////////////////////////////////////////////
///                A.II. MATCH CONTEXT                                      ///
///////////////////////////////////////////////////////////////////////////////

// match_context
//   struct: the facts one test node presents to a route predicate when the
// option set is resolved for it.  Populated by the handler from the node and
// its position; a predicate reads only from here, so matching is a pure
// function of the context.
struct match_context
{
    std::string              name;
    test_type_id             type_id;
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

// test_match_kind
//   enum: the runtime predicate node-kind - one value per way of matching a
// node, including the boolean combinators (all_of / any_of / negate) that
// recurse over child predicates.
enum class test_match_kind
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

// test_predicate
//   struct: the runtime match IR.  `kind` selects which fields are live:
// `text` is the operand of the string / tag / regex kinds, `type_id` of
// kind_is, `status` of status_is, and `kids` are the operands of the
// combinators.  A value tree - the shape a route carries and a `.dj` parser
// (or test_options_lower) would yield.
struct test_predicate
{
    test_match_kind             kind;
    std::string                 text;
    test_type_id                type_id;
    test_status                 status;
    std::vector<test_predicate> kids;

    // test_predicate
    //   constructor: the always-true predicate (kind `any`).
    test_predicate()
        : kind(test_match_kind::any),
          text(),
          type_id(0),
          status(test_status::pending),
          kids()
    {}

    // evaluate
    //   function: true iff this predicate matches _ctx.  Pure; the combinators
    // recurse over `kids`.  name_re compiles `text` as a std::regex.
    D_NODISCARD bool
    evaluate(
        const match_context& _ctx
    ) const
    {
        switch (kind)
        {
            case test_match_kind::any:
            {
                return true;
            }

            case test_match_kind::none:
            {
                return false;
            }

            case test_match_kind::name_eq:
            {
                return (_ctx.name == text);
            }

            case test_match_kind::name_has:
            {
                return (_ctx.name.find(text) != std::string::npos);
            }

            case test_match_kind::name_re:
            {
                std::regex re(text);

                return std::regex_search(_ctx.name, re);
            }

            case test_match_kind::kind_is:
            {
                return (_ctx.type_id == type_id);
            }

            case test_match_kind::status_is:
            {
                return (_ctx.status == status);
            }

            case test_match_kind::tag_has:
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

            case test_match_kind::all_of:
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

            case test_match_kind::any_of:
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

            case test_match_kind::negate:
            {
                return kids.empty() ? true : !kids[0].evaluate(_ctx);
            }
        }

        return false;
    }
};


// --- value-level predicate builders ----------------------------------------
//   Free constructors mirroring cli_render.hpp's runtime builders: each yields
// a test_predicate, so a configuration can be assembled by hand or by a parser
// without the type-level vocabulary in PART B.

// match_any
//   function: the always-true predicate.
D_NODISCARD D_INLINE test_predicate
match_any()
{
    test_predicate p;
    p.kind = test_match_kind::any;

    return p;
}

// match_none
//   function: the always-false predicate.
D_NODISCARD D_INLINE test_predicate
match_none()
{
    test_predicate p;
    p.kind = test_match_kind::none;

    return p;
}

// match_name
//   function: matches a node whose name equals _name.
D_NODISCARD D_INLINE test_predicate
match_name(
    std::string _name
)
{
    test_predicate p;
    p.kind = test_match_kind::name_eq;
    p.text = static_cast<std::string&&>(_name);

    return p;
}

// match_name_contains
//   function: matches a node whose name contains _needle.
D_NODISCARD D_INLINE test_predicate
match_name_contains(
    std::string _needle
)
{
    test_predicate p;
    p.kind = test_match_kind::name_has;
    p.text = static_cast<std::string&&>(_needle);

    return p;
}

// match_name_regex
//   function: matches a node whose name satisfies the std::regex _pattern.
D_NODISCARD D_INLINE test_predicate
match_name_regex(
    std::string _pattern
)
{
    test_predicate p;
    p.kind = test_match_kind::name_re;
    p.text = static_cast<std::string&&>(_pattern);

    return p;
}

// match_kind
//   function: matches a node whose test_type_id equals _id.
D_NODISCARD D_INLINE test_predicate
match_kind(
    test_type_id _id
)
{
    test_predicate p;
    p.kind    = test_match_kind::kind_is;
    p.type_id = _id;

    return p;
}

// match_status
//   function: matches a node whose status equals _status.
D_NODISCARD D_INLINE test_predicate
match_status(
    test_status _status
)
{
    test_predicate p;
    p.kind   = test_match_kind::status_is;
    p.status = _status;

    return p;
}

// match_tag
//   function: matches a node carrying the tag _tag.
D_NODISCARD D_INLINE test_predicate
match_tag(
    std::string _tag
)
{
    test_predicate p;
    p.kind = test_match_kind::tag_has;
    p.text = static_cast<std::string&&>(_tag);

    return p;
}

// match_all
//   function: the conjunction of _kids (matches iff every child matches).
D_NODISCARD D_INLINE test_predicate
match_all(
    std::vector<test_predicate> _kids
)
{
    test_predicate p;
    p.kind = test_match_kind::all_of;
    p.kids = static_cast<std::vector<test_predicate>&&>(_kids);

    return p;
}

// match_any_of
//   function: the disjunction of _kids (matches iff some child matches).
D_NODISCARD D_INLINE test_predicate
match_any_of(
    std::vector<test_predicate> _kids
)
{
    test_predicate p;
    p.kind = test_match_kind::any_of;
    p.kids = static_cast<std::vector<test_predicate>&&>(_kids);

    return p;
}

// match_not
//   function: the negation of _child.
D_NODISCARD D_INLINE test_predicate
match_not(
    test_predicate _child
)
{
    test_predicate p;
    p.kind = test_match_kind::negate;
    p.kids.push_back(static_cast<test_predicate&&>(_child));

    return p;
}


///////////////////////////////////////////////////////////////////////////////
///                A.IV. TEST ROUTE                                         ///
///////////////////////////////////////////////////////////////////////////////

// test_route
//   struct: one conditional override.  When `match` matches a node, the route
// contributes its set fields to that node's resolved configuration.  Toggles
// are tristate (a route may force or leave alone); value overrides carry a
// `has_*` guard so an unset field is genuinely "no opinion" rather than a
// default masquerading as one.  `sinks` additionally carries an add-vs-replace
// `sink_mode`, which is what lets one test be routed to a destination IN
// ADDITION TO the shared one (add) or INSTEAD of it (replace).
//
//   Routes are applied in declaration order (see test_option_set::resolve), so
// a later matching route wins on any scalar it sets - the value-level analogue
// of option_set_override's "later wins".
struct test_route
{
    test_predicate match;

    bool           has_sinks;
    test_sink      sinks;
    test_sink_mode sink_mode;

    test_tribool   timing;
    test_tribool   numbering;
    test_tribool   color;

    bool           has_show;
    test_show      show;

    bool           has_format;
    std::string    format;

    bool           has_document;
    test_doc_type  document;

    // test_route
    //   constructor: a route that matches everything and overrides nothing
    // (every toggle `inherit`, every value guard false).  Set `match` and the
    // fields of interest after construction, or use make_route.
    test_route()
        : match(),
          has_sinks(false),
          sinks(test_sink::none),
          sink_mode(test_sink_mode::add),
          timing(test_tribool::inherit),
          numbering(test_tribool::inherit),
          color(test_tribool::inherit),
          has_show(false),
          show(test_show::all),
          has_format(false),
          format(),
          has_document(false),
          document(test_doc_type::txt)
    {}
};


// make_route
//   function: a route guarded by _match overriding nothing yet.  Set the
// fields of interest on the result (it is a plain aggregate of public fields):
//
//   test_route r = make_route(match_name_contains("perf"));
//   r.timing = test_tribool::on;        // time only the matching tests
D_NODISCARD D_INLINE test_route
make_route(
    test_predicate _match
)
{
    test_route r;
    r.match = static_cast<test_predicate&&>(_match);

    return r;
}


///////////////////////////////////////////////////////////////////////////////
///                A.V.  TEST RESOLVED                                      ///
///////////////////////////////////////////////////////////////////////////////

// test_resolved
//   struct: the flat, fully-decided configuration for ONE node - what the
// handler and printer actually consult after the base knobs and every matching
// route have been folded together.  No guards and no tristates remain: every
// field is a concrete decision.
struct test_resolved
{
    bool              number_tests;
    test_number_style number_style;
    bool              show_timing;
    test_time_unit    time_unit;
    bool              color;
    test_sink         sinks;
    test_show         show;
    test_doc_type     document;
    std::string       format;
    std::size_t       line_width;
    bool              word_wrap;
    test_wrap_mode    wrap_mode;
    std::size_t       indent_width;
};


///////////////////////////////////////////////////////////////////////////////
///                A.VI. TEST OPTION SET                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // apply_tribool_helper
    //   helper: fold a route tristate onto an inherited boolean - `inherit`
    // keeps _base, otherwise the tristate decides.
    D_NODISCARD D_INLINE bool
    apply_tribool_helper(
        bool         _base,
        test_tribool _t
    )
    {
        if (_t == test_tribool::inherit)
        {
            return _base;
        }

        return (_t == test_tribool::on);
    }

NS_END  // internal


// test_option_set
//   struct: the concrete configuration aggregate for the DTest framework -
// the type test_kind.hpp points at and test_session.hpp owns.  Scalar knobs
// for every configurable aspect (numbering, timing, width, wrap, document,
// destinations, filtering), an ordered list of conditional `routes`, and the
// per-line `{key}` format templates the printer interpolates.
//
//   This is the runtime face of the PART B vocabulary: a schema authored as a
// type lowers (PART C) into exactly this aggregate.  It is a plain value type
// with sensible defaults - usable with no configuration at all - and is held
// by pointer wherever the framework refers to it, so its std::string /
// std::vector members impose no constexpr burden on the C++11 consumers.
struct test_option_set
{
    // --- scalar knobs ---
    bool              number_tests;     // emit a test number at all
    test_number_style number_style;     // ordinal vs hierarchical numbering
    bool              show_timing;      // emit per-test timing
    test_time_unit    time_unit;        // unit timings are displayed in
    std::size_t       line_width;       // column budget for a report line
    bool              word_wrap;        // wrap over-long lines
    test_wrap_mode    wrap_mode;        // how a wrapped line breaks
    std::size_t       indent_width;     // columns per nesting level
    test_doc_type     document;         // output document format
    test_sink         sinks;            // base output destination(s)
    std::string       output_path;      // file path when `file` is a sink
    test_show         show;             // which results reach the report
    std::size_t       max_failures;     // stop after N failures (0 = no limit)
    bool              stop_on_failure;  // stop at the first failure
    bool              color;            // colorize console output

    // --- conditional overrides (ordered, repeatable) ---
    std::vector<test_route> routes;

    // --- per-line format templates ({key} placeholders, text_template model) ---
    std::string format_test;            // one test line
    std::string format_module;          // one interior/module line
    std::string format_summary;         // the closing summary line

    // test_option_set
    //   constructor: the framework defaults - numbered, ordinal, untimed,
    // 80-column word-wrapped plain-text to the console, showing all results,
    // no failure cap.  A default-constructed set is a complete, usable
    // configuration.
    test_option_set()
        : number_tests(true),
          number_style(test_number_style::ordinal),
          show_timing(false),
          time_unit(test_time_unit::automatic),
          line_width(80),
          word_wrap(true),
          wrap_mode(test_wrap_mode::word),
          indent_width(2),
          document(test_doc_type::txt),
          sinks(test_sink::console),
          output_path(),
          show(test_show::all),
          max_failures(0),
          stop_on_failure(false),
          color(true),
          routes(),
          format_test("{index}. {name} [{status}] {duration}"),
          format_module("{name}"),
          format_summary("{passed}/{total} passed, {failed} failed, "
                         "{skipped} skipped")
    {}

    // resolve
    //   function: the flat configuration for a node described by _ctx.  Seeds
    // a test_resolved from the base scalar knobs, then applies every route
    // whose predicate matches _ctx, IN DECLARATION ORDER:
    //     - toggles fold through their tristate (inherit / off / on);
    //     - guarded values overwrite when the route sets them (later wins);
    //     - sinks either union (sink_mode add) or replace (sink_mode replace)
    //       the inherited set.
    //   The sink rule is what realizes "write this one test to the console AND
    // the file everything else goes to" (a matching route with sinks =
    // console, mode add, over a base of file) and "send only these to a
    // separate file" (mode replace).
    //
    // Parameter(s):
    //   _ctx: the facts of the node being resolved.
    // Return:
    //   the node's fully-decided configuration.
    D_NODISCARD test_resolved
    resolve(
        const match_context& _ctx
    ) const
    {
        test_resolved r;
        std::size_t   i = 0;

        // seed from the base knobs
        r.number_tests = number_tests;
        r.number_style = number_style;
        r.show_timing  = show_timing;
        r.time_unit    = time_unit;
        r.color        = color;
        r.sinks        = sinks;
        r.show         = show;
        r.document     = document;
        r.format       = format_test;
        r.line_width   = line_width;
        r.word_wrap    = word_wrap;
        r.wrap_mode    = wrap_mode;
        r.indent_width = indent_width;

        // fold every matching route in declaration order
        for (i = 0; i < routes.size(); ++i)
        {
            const test_route& rt = routes[i];

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
                if (rt.sink_mode == test_sink_mode::replace)
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
};


#if ( D_ENV_LANG_IS_CPP20_OR_HIGHER &&                                        \
      D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS )


///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                PART B - COMPILE-TIME VOCABULARY  (C++20)                ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////


// ===========================================================================
// B.I.  test_option
// ===========================================================================

// test_option
//   enum: the node-kind signature of the configuration language - one value
// per configurable aspect of the framework.  A configuration option is
// `option<test_option::K, payload...>`; this enum is the closed set of K the
// lowering bridge dispatches over.  Adding an aspect is one enumerator here
// plus one applier specialization in PART C - exactly the discipline cli.hpp's
// `dji_cli` enum and cli_render.hpp's render specializations follow.
//
//   `route` and `routes` are the two structural keys: `route` carries one
// conditional override (a predicate followed by override options), and
// `routes` carries an ORDERED pack of them under a single key - ordered and
// repeatable, so unlike the scalar keys it deliberately does not live as a
// unique option_set entry (the spine is a pack, exactly as a CLI template's
// children are a `seq` rather than an option_set).
enum class test_option
{
    // numbering
    numbering,        // option<numbering, flag<true>>            number tests?
    number_style,     // option<number_style, choice<style>>      ordinal / ...

    // timing
    timing,           // option<timing, flag<true>>               time tests?
    time_unit,        // option<time_unit, choice<unit>>          display unit

    // layout
    line_width,       // option<line_width, count<80>>            column budget
    word_wrap,        // option<word_wrap, flag<true>>            wrap long lines
    wrap_mode,        // option<wrap_mode, choice<mode>>          how to break
    indent_width,     // option<indent_width, count<2>>           columns/level

    // presentation
    color,            // option<color, flag<true>>                colorize?
    document,         // option<document, choice<doc_type>>       txt/xml/html/pdf

    // destinations
    destination,      // option<destination, choice<sink>, choice<mode>>
    output_file,      // option<output_file, text<"path">>        file sink path

    // filtering
    show,             // option<show, choice<show>>               which results
    max_failures,     // option<max_failures, count<0>>           failure cap
    stop_on_failure,  // option<stop_on_failure, flag<true>>      halt on first

    // formats ({key} placeholder templates)
    format_test,      // option<format_test, text<"...">>         per-test line
    format_module,    // option<format_module, text<"...">>       per-module line
    format_summary,   // option<format_summary, text<"...">>      summary line

    // conditional overrides
    route,            // option<route, predicate, override...>    one route
    routes            // option<routes, route..., route...>       ordered pack
};


// ===========================================================================
// B.II. payload carriers
// ===========================================================================
//   Every payload is a `val_t<V>` (meta/carrier.hpp), the framework's generic
// single-value carrier, exactly as cli.hpp's text/name/width are.  The bare
// aliases keep the node sugar readable while the type stays a plain value
// carrier (so is_value_carrier_v classifies it, and the lowering matches
// val_t<...> directly).  text carries a `fixed_string` authored at the literal
// boundary; the lowering reads its `view()`.

// flag
//   alias: a boolean payload (a toggle's state).
template<bool _B>
using flag = val_t<_B>;

// count
//   alias: a non-negative integer payload (a width, a cap).
template<std::size_t _N>
using count = val_t<_N>;

// choice
//   alias: an enumerated-value payload (a document type, a show mode, a sink,
// ...).  Generic over the enum so one carrier serves every value enum.
template<auto _Value>
using choice = val_t<_Value>;

// text
//   alias: a string payload - a file path or a `{key}` format template -
// carried as the authored `fixed_string` itself (distinct types per length,
// which is fine: a payload is never an option_set key, so uniformity is not
// required here).
template<fixed_string _Str>
using text = val_t<_Str>;


// ===========================================================================
// B.III. node sugar
// ===========================================================================
//   Intention-revealing aliases so a configuration authored by hand reads as a
// declaration rather than a wall of option<>.  Each is exactly its option<>
// form and carries no semantics of its own; the node-kind key plus the payload
// carrier are the whole content.  These same aliases are reused as a route's
// override entries (PART B.V), where toggles take on their tristate reading.

// numbering_
//   type: number tests in the report (default on).
template<bool _On = true>
using numbering_ = option<test_option::numbering, flag<_On>>;

// number_style_
//   type: select the numbering style (ordinal, hierarchical, none).
template<test_number_style _Style>
using number_style_ = option<test_option::number_style, choice<_Style>>;

// timing_
//   type: emit per-test timing (default on).
template<bool _On = true>
using timing_ = option<test_option::timing, flag<_On>>;

// time_unit_
//   type: select the unit timings are displayed in.
template<test_time_unit _Unit>
using time_unit_ = option<test_option::time_unit, choice<_Unit>>;

// line_width_
//   type: set the per-line column budget.
template<std::size_t _Cols>
using line_width_ = option<test_option::line_width, count<_Cols>>;

// word_wrap_
//   type: wrap lines that exceed the budget (default on).
template<bool _On = true>
using word_wrap_ = option<test_option::word_wrap, flag<_On>>;

// wrap_mode_
//   type: select how a wrapped line breaks (word vs hard vs none).
template<test_wrap_mode _Mode>
using wrap_mode_ = option<test_option::wrap_mode, choice<_Mode>>;

// indent_width_
//   type: set the columns added per nesting level.
template<std::size_t _Cols>
using indent_width_ = option<test_option::indent_width, count<_Cols>>;

// color_
//   type: colorize console output (default on).
template<bool _On = true>
using color_ = option<test_option::color, flag<_On>>;

// document_
//   type: select the output document format (txt, xml, html, pdf).
template<test_doc_type _Doc>
using document_ = option<test_option::document, choice<_Doc>>;

// destination_
//   type: set the output destination(s), REPLACING what is inherited.  As a
// base option it establishes where everything goes; as a route override it
// reroutes the matching tests exclusively there.
template<test_sink _Sink>
using destination_ =
    option<test_option::destination, choice<_Sink>, choice<test_sink_mode::replace>>;

// destination_add_
//   type: ADD an output destination to what is inherited.  Meaningful in a
// route: "also send the matching tests here" while everything else continues
// to its inherited sink(s) - the "console AND the shared file" expression.
template<test_sink _Sink>
using destination_add_ =
    option<test_option::destination, choice<_Sink>, choice<test_sink_mode::add>>;

// output_file_
//   type: set the file-sink path (the `file` destination writes here).
template<fixed_string _Path>
using output_file_ = option<test_option::output_file, text<_Path>>;

// show_
//   type: select which results reach the report.
template<test_show _Show>
using show_ = option<test_option::show, choice<_Show>>;

// max_failures_
//   type: stop the run after _N failures (0 = no limit).
template<std::size_t _N>
using max_failures_ = option<test_option::max_failures, count<_N>>;

// stop_on_failure_
//   type: halt at the first failure (default on when written).
template<bool _On = true>
using stop_on_failure_ = option<test_option::stop_on_failure, flag<_On>>;

// format_test_
//   type: set the per-test line template (a `{key}` placeholder string).
template<fixed_string _Fmt>
using format_test_ = option<test_option::format_test, text<_Fmt>>;

// format_module_
//   type: set the per-module line template.
template<fixed_string _Fmt>
using format_module_ = option<test_option::format_module, text<_Fmt>>;

// format_summary_
//   type: set the closing summary line template.
template<fixed_string _Fmt>
using format_summary_ = option<test_option::format_summary, text<_Fmt>>;


// ===========================================================================
// B.IV. test_match + predicate vocabulary
// ===========================================================================

// test_match
//   enum: the type-level predicate node-kind, parallel to the runtime
// test_match_kind.  A predicate is `option<test_match::K, payload/child...>`;
// the combinators all_of / any_of / not_ carry child predicates as their pack.
// Kept a SEPARATE enum from test_option because a predicate lives in a route's
// arg pack, not in the option_set of configuration keys - distinct vocabularies
// for distinct slots, exactly as cli.hpp keeps node-kinds out of the variable
// environment.
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

// any_test
//   type: the always-true predicate.
using any_test = option<test_match::any>;

// no_test
//   type: the always-false predicate.
using no_test = option<test_match::none>;

// name_is
//   type: matches a node whose name equals _Name.
template<fixed_string _Name>
using name_is = option<test_match::name_eq, text<_Name>>;

// name_has
//   type: matches a node whose name contains _Needle.
template<fixed_string _Needle>
using name_has = option<test_match::name_has, text<_Needle>>;

// name_re
//   type: matches a node whose name satisfies the regex _Pattern.
template<fixed_string _Pattern>
using name_re = option<test_match::name_re, text<_Pattern>>;

// kind_is
//   type: matches a node whose test_type_id equals _Id.
template<test_type_id _Id>
using kind_is = option<test_match::kind_is, choice<_Id>>;

// status_is
//   type: matches a node whose status equals _Status.
template<test_status _Status>
using status_is = option<test_match::status_is, choice<_Status>>;

// tag_is
//   type: matches a node carrying the tag _Tag.
template<fixed_string _Tag>
using tag_is = option<test_match::tag_has, text<_Tag>>;

// all_of
//   type: the conjunction of the child predicates _Preds.
template<typename... _Preds>
using all_of = option<test_match::all_of, _Preds...>;

// any_of
//   type: the disjunction of the child predicates _Preds.
template<typename... _Preds>
using any_of = option<test_match::any_of, _Preds...>;

// not_
//   type: the negation of the child predicate _Pred.
template<typename _Pred>
using not_ = option<test_match::negate, _Pred>;


// ===========================================================================
// B.V.  route_ / routes_
// ===========================================================================

// route_
//   type: one conditional override - a predicate followed by override
// options.  `option<test_option::route, _Predicate, _Overrides...>`, where
// _Predicate is a PART B.IV predicate and each _Override is a PART B.III node
// alias (a toggle read as a tristate, or a guarded value).  The args are an
// opaque, ordered pack, so mixing the predicate's `test_match` key with the
// overrides' `test_option` keys is well-formed (the single-key-type rule binds
// option_set entries, not an option's arg pack).
//
// Usage:
//   route_<name_has<"perf">, timing_<true>>                  // time only perf
//   route_<name_is<"login">, destination_add_<test_sink::console>>
template<typename    _Predicate,
         typename... _Overrides>
using route_ = option<test_option::route, _Predicate, _Overrides...>;

// routes_
//   type: an ORDERED, repeatable pack of routes under one key.  Routes cannot
// each be a distinct option_set entry (they share the key `test_option::route`,
// which the set's uniqueness rule forbids), so the whole sequence rides a
// single `test_option::routes` key - the same reason a CLI template's children
// are a `seq` and not an option_set.  Declaration order is significant: later
// matching routes win (PART A's resolve folds them in order).
template<typename... _Routes>
using routes_ = option<test_option::routes, _Routes...>;


// ===========================================================================
// B.VI. test_config
// ===========================================================================

// test_config
//   type: a whole configuration schema authored in ONE statement - the
// option_set produced by folding the given surfaces from empty under
// override_replace (compose_options_t).  Reads as a declaration of the
// configuration; lower it to a runtime test_option_set with
// test_options_lower (PART C).
//
//   Layering / precedence is option_set_override_t: a base schema overridden
// by a session schema overridden by per-call deltas is exactly the note's
// precedence union (+), the same engine `let` scoping uses in the CLI.
//
// Usage:
//   using my_config = test_config<
//       numbering_<true>,
//       number_style_<test_number_style::hierarchical>,
//       timing_<false>,
//       line_width_<100>,
//       document_<test_doc_type::html>,
//       destination_<test_sink::file>,
//       output_file_<"results.html">,
//       show_<test_show::failures_only>,
//       format_test_<"{index}. {name} -> {status} ({duration})">,
//       routes_<
//           route_<name_has<"perf">,  timing_<true>>,
//           route_<name_is<"login">,  destination_add_<test_sink::console>>
//       >
//   >;
template<typename... _Surfaces>
using test_config = compose_options_t<_Surfaces...>;


///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                   PART C - LOWERING BRIDGE  (C++20)                     ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////
//
//   The one bridge between the halves: distil a type-level schema into a
// runtime test_option_set, so a configuration authored once as a type drives
// the runtime framework after a single lowering call.  This is the exact role
// dji_cli_lower plays for the CLI AST, and the shape mirrors it: per-node-kind
// specializations, a small fold over the schema's flat option tuple.


NS_INTERNAL

    // ===================================================================
    // C.I.  predicate lowering
    // ===================================================================

    // lower_predicate_helper
    //   trait: a type-level predicate (test_match AST) -> test_predicate
    // (primary specialized per node-kind below).
    template<typename _Pred>
    struct lower_predicate_helper;

    // lower_predicate_value_helper
    //   function: lower_predicate_helper<_Pred>::go() as a callable, so a
    // child predicate lowers inline within a combinator's brace-init.
    template<typename _Pred>
    D_NODISCARD test_predicate
    lower_predicate_value_helper()
    {
        return lower_predicate_helper<_Pred>::go();
    }

    template<>
    struct lower_predicate_helper<option<test_match::any>>
    {
        static test_predicate go() { return match_any(); }
    };

    template<>
    struct lower_predicate_helper<option<test_match::none>>
    {
        static test_predicate go() { return match_none(); }
    };

    template<fixed_string _Name>
    struct lower_predicate_helper<option<test_match::name_eq, val_t<_Name>>>
    {
        static test_predicate go()
        {
            return match_name(std::string(_Name.view()));
        }
    };

    template<fixed_string _Needle>
    struct lower_predicate_helper<option<test_match::name_has, val_t<_Needle>>>
    {
        static test_predicate go()
        {
            return match_name_contains(std::string(_Needle.view()));
        }
    };

    template<fixed_string _Pattern>
    struct lower_predicate_helper<option<test_match::name_re, val_t<_Pattern>>>
    {
        static test_predicate go()
        {
            return match_name_regex(std::string(_Pattern.view()));
        }
    };

    template<test_type_id _Id>
    struct lower_predicate_helper<option<test_match::kind_is, val_t<_Id>>>
    {
        static test_predicate go() { return match_kind(_Id); }
    };

    template<test_status _Status>
    struct lower_predicate_helper<option<test_match::status_is, val_t<_Status>>>
    {
        static test_predicate go() { return match_status(_Status); }
    };

    template<fixed_string _Tag>
    struct lower_predicate_helper<option<test_match::tag_has, val_t<_Tag>>>
    {
        static test_predicate go()
        {
            return match_tag(std::string(_Tag.view()));
        }
    };

    template<typename... _Preds>
    struct lower_predicate_helper<option<test_match::all_of, _Preds...>>
    {
        static test_predicate go()
        {
            return match_all(
                std::vector<test_predicate>{
                    lower_predicate_value_helper<_Preds>()... });
        }
    };

    template<typename... _Preds>
    struct lower_predicate_helper<option<test_match::any_of, _Preds...>>
    {
        static test_predicate go()
        {
            return match_any_of(
                std::vector<test_predicate>{
                    lower_predicate_value_helper<_Preds>()... });
        }
    };

    template<typename _Pred>
    struct lower_predicate_helper<option<test_match::negate, _Pred>>
    {
        static test_predicate go()
        {
            return match_not(lower_predicate_value_helper<_Pred>());
        }
    };


    // ===================================================================
    // C.II. route lowering
    // ===================================================================

    // apply_route_override_helper
    //   trait: fold one override option onto a test_route (primary handles the
    // toggles + values the route layer supports; an unsupported key trips the
    // static_assert so it is caught at the authoring boundary rather than
    // silently ignored).
    template<typename _Override>
    struct apply_route_override_helper
    {
        static void to(test_route&)
        {
            static_assert(sizeof(_Override) == 0,
                "test route: this option is not supported as a route "
                "override.  Routes honor numbering_/timing_/color_ (as "
                "tristate toggles), show_, document_, format_test_, and "
                "destination_/destination_add_.");
        }
    };

    template<bool _On>
    struct apply_route_override_helper<option<test_option::numbering, val_t<_On>>>
    {
        static void to(test_route& _r)
        {
            _r.numbering = _On ? test_tribool::on : test_tribool::off;
        }
    };

    template<bool _On>
    struct apply_route_override_helper<option<test_option::timing, val_t<_On>>>
    {
        static void to(test_route& _r)
        {
            _r.timing = _On ? test_tribool::on : test_tribool::off;
        }
    };

    template<bool _On>
    struct apply_route_override_helper<option<test_option::color, val_t<_On>>>
    {
        static void to(test_route& _r)
        {
            _r.color = _On ? test_tribool::on : test_tribool::off;
        }
    };

    template<test_show _Show>
    struct apply_route_override_helper<option<test_option::show, val_t<_Show>>>
    {
        static void to(test_route& _r)
        {
            _r.has_show = true;
            _r.show     = _Show;
        }
    };

    template<test_doc_type _Doc>
    struct apply_route_override_helper<option<test_option::document, val_t<_Doc>>>
    {
        static void to(test_route& _r)
        {
            _r.has_document = true;
            _r.document     = _Doc;
        }
    };

    template<fixed_string _Fmt>
    struct apply_route_override_helper<option<test_option::format_test, val_t<_Fmt>>>
    {
        static void to(test_route& _r)
        {
            _r.has_format = true;
            _r.format     = std::string(_Fmt.view());
        }
    };

    template<test_sink _Sink,
             test_sink_mode _Mode>
    struct apply_route_override_helper<
        option<test_option::destination, val_t<_Sink>, val_t<_Mode>>>
    {
        static void to(test_route& _r)
        {
            _r.has_sinks  = true;
            _r.sinks      = _Sink;
            _r.sink_mode  = _Mode;
        }
    };


    // lower_route_helper
    //   trait: one route_ option -> test_route.  Lowers the predicate child,
    // then folds each override onto the route in order.
    template<typename _Route>
    struct lower_route_helper;

    template<typename    _Predicate,
             typename... _Overrides>
    struct lower_route_helper<option<test_option::route, _Predicate, _Overrides...>>
    {
        static test_route go()
        {
            test_route r =
                make_route(lower_predicate_value_helper<_Predicate>());

            // fold the overrides left to right (pack-expansion over a brace
            // list sequences the side effects)
            int sink[] = { 0,
                ( apply_route_override_helper<_Overrides>::to(r), 0 )... };
            (void) sink;

            return r;
        }
    };


    // ===================================================================
    // C.III. test_options_lower driver
    // ===================================================================

    // apply_option_helper
    //   trait: fold one configuration option onto a test_option_set (primary
    // handles every scalar key + the route container; an unsupported key trips
    // the static_assert).
    template<typename _Option>
    struct apply_option_helper
    {
        static void to(test_option_set&)
        {
            static_assert(sizeof(_Option) == 0,
                "test_options_lower: unrecognized test_option key in the "
                "schema.  Every key in test_option must have a matching "
                "applier here.");
        }
    };

    template<bool _On>
    struct apply_option_helper<option<test_option::numbering, val_t<_On>>>
    {
        static void to(test_option_set& _o) { _o.number_tests = _On; }
    };

    template<test_number_style _Style>
    struct apply_option_helper<option<test_option::number_style, val_t<_Style>>>
    {
        static void to(test_option_set& _o) { _o.number_style = _Style; }
    };

    template<bool _On>
    struct apply_option_helper<option<test_option::timing, val_t<_On>>>
    {
        static void to(test_option_set& _o) { _o.show_timing = _On; }
    };

    template<test_time_unit _Unit>
    struct apply_option_helper<option<test_option::time_unit, val_t<_Unit>>>
    {
        static void to(test_option_set& _o) { _o.time_unit = _Unit; }
    };

    template<std::size_t _Cols>
    struct apply_option_helper<option<test_option::line_width, val_t<_Cols>>>
    {
        static void to(test_option_set& _o) { _o.line_width = _Cols; }
    };

    template<bool _On>
    struct apply_option_helper<option<test_option::word_wrap, val_t<_On>>>
    {
        static void to(test_option_set& _o) { _o.word_wrap = _On; }
    };

    template<test_wrap_mode _Mode>
    struct apply_option_helper<option<test_option::wrap_mode, val_t<_Mode>>>
    {
        static void to(test_option_set& _o) { _o.wrap_mode = _Mode; }
    };

    template<std::size_t _Cols>
    struct apply_option_helper<option<test_option::indent_width, val_t<_Cols>>>
    {
        static void to(test_option_set& _o) { _o.indent_width = _Cols; }
    };

    template<bool _On>
    struct apply_option_helper<option<test_option::color, val_t<_On>>>
    {
        static void to(test_option_set& _o) { _o.color = _On; }
    };

    template<test_doc_type _Doc>
    struct apply_option_helper<option<test_option::document, val_t<_Doc>>>
    {
        static void to(test_option_set& _o) { _o.document = _Doc; }
    };

    //   destination at the base level sets the sink set directly; the mode is
    // carried for uniformity with the route form and is not consulted here
    // (the base IS the inherited set).
    template<test_sink _Sink,
             test_sink_mode _Mode>
    struct apply_option_helper<
        option<test_option::destination, val_t<_Sink>, val_t<_Mode>>>
    {
        static void to(test_option_set& _o) { _o.sinks = _Sink; }
    };

    template<fixed_string _Path>
    struct apply_option_helper<option<test_option::output_file, val_t<_Path>>>
    {
        static void to(test_option_set& _o)
        {
            _o.output_path = std::string(_Path.view());
        }
    };

    template<test_show _Show>
    struct apply_option_helper<option<test_option::show, val_t<_Show>>>
    {
        static void to(test_option_set& _o) { _o.show = _Show; }
    };

    template<std::size_t _N>
    struct apply_option_helper<option<test_option::max_failures, val_t<_N>>>
    {
        static void to(test_option_set& _o) { _o.max_failures = _N; }
    };

    template<bool _On>
    struct apply_option_helper<option<test_option::stop_on_failure, val_t<_On>>>
    {
        static void to(test_option_set& _o) { _o.stop_on_failure = _On; }
    };

    template<fixed_string _Fmt>
    struct apply_option_helper<option<test_option::format_test, val_t<_Fmt>>>
    {
        static void to(test_option_set& _o)
        {
            _o.format_test = std::string(_Fmt.view());
        }
    };

    template<fixed_string _Fmt>
    struct apply_option_helper<option<test_option::format_module, val_t<_Fmt>>>
    {
        static void to(test_option_set& _o)
        {
            _o.format_module = std::string(_Fmt.view());
        }
    };

    template<fixed_string _Fmt>
    struct apply_option_helper<option<test_option::format_summary, val_t<_Fmt>>>
    {
        static void to(test_option_set& _o)
        {
            _o.format_summary = std::string(_Fmt.view());
        }
    };

    //   a lone route key (a single route authored without the routes_ wrapper)
    template<typename    _Predicate,
             typename... _Overrides>
    struct apply_option_helper<
        option<test_option::route, _Predicate, _Overrides...>>
    {
        static void to(test_option_set& _o)
        {
            _o.routes.push_back(
                lower_route_helper<
                    option<test_option::route, _Predicate, _Overrides...>>::go());
        }
    };

    //   the routes container: lower each route in declaration order
    template<typename... _Routes>
    struct apply_option_helper<option<test_option::routes, _Routes...>>
    {
        static void to(test_option_set& _o)
        {
            int sink[] = { 0,
                ( _o.routes.push_back(lower_route_helper<_Routes>::go()), 0 )... };
            (void) sink;
        }
    };


    // lower_schema_helper
    //   trait: fold every option in a schema's flat option tuple onto a
    // test_option_set.
    template<typename _Tuple>
    struct lower_schema_helper;

    template<typename... _Options>
    struct lower_schema_helper<std::tuple<_Options...>>
    {
        static void to(test_option_set& _o)
        {
            int sink[] = { 0,
                ( apply_option_helper<_Options>::to(_o), 0 )... };
            (void) sink;
        }
    };

NS_END  // internal


// test_options_lower
//   trait: distil a type-level configuration _Schema (an option_set, typically
// a test_config<...>) into a runtime test_option_set.  `build()` seeds the
// framework defaults and then applies every option in the schema, so any
// aspect the schema leaves unspecified keeps its default - the type-level
// counterpart of constructing a test_option_set and setting only the fields
// you care about.
//
// Usage:
//   static const test_option_set g_opts =
//       test_options_lower<my_config>::build();
//   // hand &g_opts to a test_kind, or assign into a session's options.
template<typename _Schema>
struct test_options_lower
{
    D_NODISCARD static test_option_set
    build()
    {
        test_option_set out;   // framework defaults
        internal::lower_schema_helper<
            typename _Schema::flat_options_t>::to(out);

        return out;
    }
};


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && ... NONTYPE_TEMPLATE_ARGS


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_OPTIONS_