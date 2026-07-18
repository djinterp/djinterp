/******************************************************************************
* djinterp [config]                                                 cfg_table.h
*
*   Build-time configuration for the table DSL subframework -- the two
* declaration front ends (table_builder, table_parser), the model they meet at,
* and the templating layer over it.  This header owns every user-overridable knob
* those modules answer to, validates it, and publishes the effective values they
* consume; the modules themselves carry no resolution logic and read the answers.
*
*   ONE FILE, NOT SEVEN.  The table DSL is seven headers but ONE subframework, and
* its knobs are cross-cutting by nature: the default cell-count strictness is read
* by table_options (as the compile-time default), by table_parser (as the runtime
* dialect's default), and by table_template (as the placeholder fit).  One knob,
* one canonical default, one file -- splitting per header would put the same
* default in three places, which is the drift the localization rule exists to
* prevent.
*
*   TWO CONFIGURATION MODELS
*     layer gate    D_CFG_TABLE_<LAYER> == 0 means "do not compile this layer".
*                   The optional legs -- the text front end, the compose leg, the
*                   templating -- so a project that only declares tables in types
*                   pays for nothing else.
*     policy default D_CFG_TABLE_DEFAULT_<CATEGORY> selects the grade a category
*                   assumes when a declaration names no option.  A small
*                   documented enum, resolved here into the enumerator the module
*                   pastes.
*
*   All boolean knobs are strict 0/1; every enum knob is validated against its
* range.  The shipped defaults fail loud and stay rectangular -- leniency is
* opted into, never inherited.
*
*   targets:  core/container/table/table_options.hpp
*                 -> D_INTERNAL_TABLE_DEFAULT_* (the policy grades)
*             core/container/table/table_model.hpp
*                 -> D_INTERNAL_TABLE_RENDER, D_INTERNAL_TABLE_DELIMITER, ...
*             core/container/table/table_parser.hpp
*                 -> D_INTERNAL_TABLE_PARSER + the dialect characters
*             core/container/table/table_template.hpp
*                 -> D_INTERNAL_TABLE_TEMPLATE, D_INTERNAL_TABLE_ELLIPSIS, ...
*             core/container/table/{table_shape,table_layout,table_builder}.hpp
*                 -> D_INTERNAL_TABLE_CONCEPTS, D_INTERNAL_TABLE_STATIC_ASSERTS
*   requires: cfg_common.h (D_CFG_IS_ON / D_CFG_NORM, D_CFG_TESTING, user pickup)
*
* path:      /config/core/container/table/cfg_table.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.16
******************************************************************************/

#ifndef DJINTERP_CFG_TABLE_
#define DJINTERP_CFG_TABLE_ 1

/*
TABLE OF CONTENTS
=================
0.    TABLE DSL CONFIGURATION
      -----------------------
      1.  Master Switch
          a.  D_CFG_TABLE_ALL
      2.  Layer Gates
          a.  D_CFG_TABLE_BUILDER      (the type DSL)
          b.  D_CFG_TABLE_RENDER       (the compose leg)
          c.  D_CFG_TABLE_PARSER       (the text DSL)
          d.  D_CFG_TABLE_TEMPLATE     (placeholders + interpolation)
          e.  D_CFG_TABLE_CONCEPTS     (the C++20 concept faces)
          f.  D_CFG_TABLE_STATIC_ASSERTS (the conformance assertions)
      3.  Policy Grade Vocabulary  (the small enums the knobs take)
      4.  Policy Defaults
          a.  D_CFG_TABLE_DEFAULT_CELL_COUNT
          b.  D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT
          c.  D_CFG_TABLE_DEFAULT_TYPE_CONFORMANCE
          d.  D_CFG_TABLE_DEFAULT_SHAPE
          e.  D_CFG_TABLE_DEFAULT_HEADER
          f.  D_CFG_TABLE_DEFAULT_DOMAIN
          g.  D_CFG_TABLE_DEFAULT_TRIM
          h.  D_CFG_TABLE_DEFAULT_PIPE
          i.  D_CFG_TABLE_DEFAULT_ANCHOR
      5.  Dialect Characters
          a.  D_CFG_TABLE_DELIMITER, D_CFG_TABLE_PAD,
              D_CFG_TABLE_SEPARATOR_FILL
          b.  D_CFG_TABLE_ELLIPSIS, D_CFG_TABLE_ITERATION_SIGIL
      6.  Configuration Validation
      7.  Effective (Derived) Values
          a.  D_INTERNAL_TABLE_<LAYER>            (the effective gates)
          b.  D_INTERNAL_TABLE_DEFAULT_<CATEGORY> (the enumerator to paste)
          c.  D_INTERNAL_TABLE_<CHARACTER>        (the dialect)
*/

// (0) root first: helpers, user overrides, testing flag / preset.
#include "../../../cfg_common.h"

// (0b) the environment this config's cascade detects from.  Layer 4 of the
// resolution cascade is "environment-detected", and the concepts gate (0.7a) is
// exactly that -- so the detection is pulled in HERE and resolved once, rather
// than left for each of the seven modules to re-derive.  Both are pure
// preprocessor and neither reaches back into this file, so there is no cycle:
//   env.h              -> D_ENV_LANG_IS_CPP20_OR_HIGHER
//   env_cpp_features.h -> D_ENV_CPP_FEATURE_LANG_CONCEPTS  (standalone)
// This mirrors djinterp.h's own order, which loads env before its qualifier
// config for the same reason.
#include "../../../../core/env/env.h"
#include "../../../../core/env/cpp/env_cpp_features.h"


// ================================================================
// 0.1  Master Switch
// ================================================================

// D_CFG_TABLE_ALL
//   brief: aggregate fallback for every layer gate below.  Each gate's default
// reads this when the gate itself is unset, so an individual override still
// wins: individual > aggregate > hard default.  Unset by default (the gates then
// take their own hard defaults).
// #ifndef D_CFG_TABLE_ALL
//     #define D_CFG_TABLE_ALL 1
// #endif


// ================================================================
// 0.2  Layer Gates
// ================================================================
//   Which legs of the DSL are compiled.  The model and its shape / layout are
// unconditional -- they are the carrier everything else names -- but the front
// ends and the templating are opt-out, so a project that only ever declares
// tables in types never compiles the text machinery.

// D_CFG_TABLE_BUILDER
//   brief: compile the type DSL (table_builder's declarators and fold).  1 = on.
#ifndef D_CFG_TABLE_BUILDER
#   if defined(D_CFG_TABLE_ALL)
#       define D_CFG_TABLE_BUILDER          D_CFG_TABLE_ALL
#   else
#       define D_CFG_TABLE_BUILDER          1
#   endif
#endif

// D_CFG_TABLE_RENDER
//   brief: compile the compose leg (render: model -> canonical text).  Required
// by the parser's round-trip laws, so the parser propagates it on (0.2b below).
#ifndef D_CFG_TABLE_RENDER
#   if defined(D_CFG_TABLE_ALL)
#       define D_CFG_TABLE_RENDER           D_CFG_TABLE_ALL
#   else
#       define D_CFG_TABLE_RENDER           1
#   endif
#endif

// D_CFG_TABLE_PARSER
//   brief: compile the text DSL (the grid scan and its span inference).
#ifndef D_CFG_TABLE_PARSER
#   if defined(D_CFG_TABLE_ALL)
#       define D_CFG_TABLE_PARSER           D_CFG_TABLE_ALL
#   else
#       define D_CFG_TABLE_PARSER           1
#   endif
#endif

// D_CFG_TABLE_TEMPLATE
//   brief: compile the templating layer ({key} interpolation and `name...` run
// expansion).  C++17 regardless -- interpolate.hpp's own floor.
#ifndef D_CFG_TABLE_TEMPLATE
#   if defined(D_CFG_TABLE_ALL)
#       define D_CFG_TABLE_TEMPLATE         D_CFG_TABLE_ALL
#   else
#       define D_CFG_TABLE_TEMPLATE         1
#   endif
#endif

// D_CFG_TABLE_CONCEPTS
//   brief: compile the C++20 concept faces (TableShape, TableBuilder, ...).
// Only ever consulted where the language actually has concepts; this knob turns
// them off even where it does.
#ifndef D_CFG_TABLE_CONCEPTS
#   if defined(D_CFG_TABLE_ALL)
#       define D_CFG_TABLE_CONCEPTS         D_CFG_TABLE_ALL
#   else
#       define D_CFG_TABLE_CONCEPTS         1
#   endif
#endif

// D_CFG_TABLE_STATIC_ASSERTS
//   brief: compile the conformance assertions (a declared cover must be valid; a
// row must fit its columns).  Keep 1: they are the DSL's diagnostics, and turning
// them off buys nothing at runtime -- they are compile-time only.  0 exists for
// bisecting a diagnostic that fires where it should not.
#ifndef D_CFG_TABLE_STATIC_ASSERTS
#   define D_CFG_TABLE_STATIC_ASSERTS       1
#endif

// --- 0.2b  Propagation: a layer that needs another turns it on -------------
//   Guarded, so an explicit user value still wins; the default above has already
// run, so this only ever upgrades an aggregate-off to on where a dependency
// demands it.  The parser's round-trip laws (renders_to_same_text,
// survives_round_trip) call render, so the parser needs the compose leg.

#if D_CFG_IS_ON(D_CFG_TABLE_PARSER)
#   ifndef D_CFG_TABLE_RENDER
#       define D_CFG_TABLE_RENDER           1
#   endif
#endif


// ================================================================
// 0.3  Policy Grade Vocabulary
// ================================================================
//   The small enums the default knobs take.  Named so an override reads as
// intent -- -DD_CFG_TABLE_DEFAULT_SHAPE=D_CFG_TABLE_SHAPE_JAGGED -- rather than
// as a magic number.  These mirror the enumerators of table_options.hpp exactly.

// D_CFG_TABLE_STRICTNESS_*
//   brief: how a supplied count must match a required one.  exact = a mismatch
// is an error; truncate = a surplus is dropped; pad = a shortfall is filled;
// lenient = either is accepted.
#define D_CFG_TABLE_STRICTNESS_EXACT        0
#define D_CFG_TABLE_STRICTNESS_TRUNCATE     1
#define D_CFG_TABLE_STRICTNESS_PAD          2
#define D_CFG_TABLE_STRICTNESS_LENIENT      3

// D_CFG_TABLE_TYPE_*
//   brief: whether a text cell must yield its declared column type.
#define D_CFG_TABLE_TYPE_REQUIRE            0
#define D_CFG_TABLE_TYPE_COERCE             1
#define D_CFG_TABLE_TYPE_IGNORE             2

// D_CFG_TABLE_SHAPE_*
//   brief: which domain shapes I_T a declaration admits.
#define D_CFG_TABLE_SHAPE_RECTANGULAR       0
#define D_CFG_TABLE_SHAPE_JAGGED            1
#define D_CFG_TABLE_SHAPE_SPARSE            2

// D_CFG_TABLE_HEADER_*
//   brief: how header rows and the header/body separator are treated.
#define D_CFG_TABLE_HEADER_REQUIRE_SEP      0
#define D_CFG_TABLE_HEADER_OPTIONAL_SEP     1
#define D_CFG_TABLE_HEADER_NONE             2

// D_CFG_TABLE_DOMAIN_*
//   brief: whether a cell-value interval is enforced.
#define D_CFG_TABLE_DOMAIN_IGNORE           0
#define D_CFG_TABLE_DOMAIN_ENFORCE          1

// D_CFG_TABLE_TRIM_*
//   brief: whether a text cell's surrounding whitespace is stripped.
#define D_CFG_TABLE_TRIM_TRIM               0
#define D_CFG_TABLE_TRIM_KEEP               1

// D_CFG_TABLE_PIPE_*
//   brief: whether a row's outer delimiters are mandatory.
#define D_CFG_TABLE_PIPE_REQUIRE_BORDERS    0
#define D_CFG_TABLE_PIPE_OPTIONAL_BORDERS   1

// D_CFG_TABLE_ANCHOR_*
//   brief: which position names a merged cell.  lex_least is the formal default
// (anchor(C) = min_lex R_C); declared names it where it was written.
#define D_CFG_TABLE_ANCHOR_LEX_LEAST        0
#define D_CFG_TABLE_ANCHOR_DECLARED         1


// ================================================================
// 0.4  Policy Defaults
// ================================================================
//   The grade a category assumes when a declaration names no option.  The
// SINGLE canonical home for each: table_options.hpp reads the derived value in
// section 0.7 and defines no default of its own.
//
//   The shipped values fail loud and stay rectangular.  That is deliberate: a
// table that silently swallowed a short row would be a table you could not trust,
// so leniency is opted into per declaration (or set here, once, per project).

// D_CFG_TABLE_DEFAULT_CELL_COUNT
//   brief: how strictly a row's width must match the declared columns.
#ifndef D_CFG_TABLE_DEFAULT_CELL_COUNT
#   define D_CFG_TABLE_DEFAULT_CELL_COUNT       D_CFG_TABLE_STRICTNESS_EXACT
#endif

// D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT
//   brief: how strictly a multi-cell placeholder's run must fill its span.
#ifndef D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT
#   define D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT  D_CFG_TABLE_STRICTNESS_EXACT
#endif

// D_CFG_TABLE_DEFAULT_TYPE_CONFORMANCE
//   brief: whether a text cell must parse to its column type.
#ifndef D_CFG_TABLE_DEFAULT_TYPE_CONFORMANCE
#   define D_CFG_TABLE_DEFAULT_TYPE_CONFORMANCE D_CFG_TABLE_TYPE_REQUIRE
#endif

// D_CFG_TABLE_DEFAULT_SHAPE
//   brief: which domain shapes a declaration admits.
#ifndef D_CFG_TABLE_DEFAULT_SHAPE
#   define D_CFG_TABLE_DEFAULT_SHAPE            D_CFG_TABLE_SHAPE_RECTANGULAR
#endif

// D_CFG_TABLE_DEFAULT_HEADER
//   brief: whether a separator is required to open a header block.
#ifndef D_CFG_TABLE_DEFAULT_HEADER
#   define D_CFG_TABLE_DEFAULT_HEADER           D_CFG_TABLE_HEADER_OPTIONAL_SEP
#endif

// D_CFG_TABLE_DEFAULT_DOMAIN
//   brief: whether a cell-value interval is enforced.  Off: the domain overlay
// is opt-in, mirroring constrained_table's own optional domain.
#ifndef D_CFG_TABLE_DEFAULT_DOMAIN
#   define D_CFG_TABLE_DEFAULT_DOMAIN           D_CFG_TABLE_DOMAIN_IGNORE
#endif

// D_CFG_TABLE_DEFAULT_TRIM
//   brief: whether cell text is trimmed.
#ifndef D_CFG_TABLE_DEFAULT_TRIM
#   define D_CFG_TABLE_DEFAULT_TRIM             D_CFG_TABLE_TRIM_TRIM
#endif

// D_CFG_TABLE_DEFAULT_PIPE
//   brief: whether a row's outer delimiters are mandatory.
#ifndef D_CFG_TABLE_DEFAULT_PIPE
#   define D_CFG_TABLE_DEFAULT_PIPE             D_CFG_TABLE_PIPE_REQUIRE_BORDERS
#endif

// D_CFG_TABLE_DEFAULT_ANCHOR
//   brief: which position names a merged cell.  The formal default.
#ifndef D_CFG_TABLE_DEFAULT_ANCHOR
#   define D_CFG_TABLE_DEFAULT_ANCHOR           D_CFG_TABLE_ANCHOR_LEX_LEAST
#endif


// ================================================================
// 0.5  Dialect Characters
// ================================================================
//   What the text surface is spelled with.  Render and parse must agree on these
// or the round trip cannot close, which is exactly why they are configured once
// here rather than defaulted twice.

// D_CFG_TABLE_DELIMITER
//   brief: the cell delimiter.  Its POSITIONS carry the merge spans, so this is
// the load-bearing character of the dialect.
#ifndef D_CFG_TABLE_DELIMITER
#   define D_CFG_TABLE_DELIMITER            '|'
#endif

// D_CFG_TABLE_PAD
//   brief: the padding character render aligns cells with.
#ifndef D_CFG_TABLE_PAD
#   define D_CFG_TABLE_PAD                  ' '
#endif

// D_CFG_TABLE_SEPARATOR_FILL
//   brief: the fill of the rule that closes a header block.
#ifndef D_CFG_TABLE_SEPARATOR_FILL
#   define D_CFG_TABLE_SEPARATOR_FILL       '-'
#endif

// D_CFG_TABLE_ELLIPSIS
//   brief: the suffix marking a MULTI-cell placeholder ("headers...").
#ifndef D_CFG_TABLE_ELLIPSIS
#   define D_CFG_TABLE_ELLIPSIS             "..."
#endif

// D_CFG_TABLE_ITERATION_SIGIL
//   brief: the character marking an iterated cell ("bar#" / "#gamma").
#ifndef D_CFG_TABLE_ITERATION_SIGIL
#   define D_CFG_TABLE_ITERATION_SIGIL      '#'
#endif


// ================================================================
// 0.6  Configuration Validation
// ================================================================
//   Cheap, and it catches a typo here rather than as a baffling grade downstream.
// Booleans first, then each enum against its range.

#if ( !D_CFG_IS_ON(D_CFG_TABLE_BUILDER) && !D_CFG_IS_OFF(D_CFG_TABLE_BUILDER) )
#   error "D_CFG_TABLE_BUILDER must be 0 or 1"
#endif
#if ( !D_CFG_IS_ON(D_CFG_TABLE_RENDER) && !D_CFG_IS_OFF(D_CFG_TABLE_RENDER) )
#   error "D_CFG_TABLE_RENDER must be 0 or 1"
#endif
#if ( !D_CFG_IS_ON(D_CFG_TABLE_PARSER) && !D_CFG_IS_OFF(D_CFG_TABLE_PARSER) )
#   error "D_CFG_TABLE_PARSER must be 0 or 1"
#endif
#if ( !D_CFG_IS_ON(D_CFG_TABLE_TEMPLATE) && !D_CFG_IS_OFF(D_CFG_TABLE_TEMPLATE) )
#   error "D_CFG_TABLE_TEMPLATE must be 0 or 1"
#endif
#if ( !D_CFG_IS_ON(D_CFG_TABLE_CONCEPTS) && !D_CFG_IS_OFF(D_CFG_TABLE_CONCEPTS) )
#   error "D_CFG_TABLE_CONCEPTS must be 0 or 1"
#endif
#if ( !D_CFG_IS_ON(D_CFG_TABLE_STATIC_ASSERTS) && \
      !D_CFG_IS_OFF(D_CFG_TABLE_STATIC_ASSERTS) )
#   error "D_CFG_TABLE_STATIC_ASSERTS must be 0 or 1"
#endif

// a layer that needs another must not be left contradicting it
#if ( D_CFG_IS_ON(D_CFG_TABLE_PARSER) && D_CFG_IS_OFF(D_CFG_TABLE_RENDER) )
#   error "D_CFG_TABLE_PARSER requires D_CFG_TABLE_RENDER: the parser's "        \
          "round-trip laws call render.  Either leave RENDER on, or turn the "   \
          "PARSER off."
#endif

#if ( (D_CFG_NORM(D_CFG_TABLE_DEFAULT_CELL_COUNT) < 0) ||                        \
      (D_CFG_NORM(D_CFG_TABLE_DEFAULT_CELL_COUNT) > 3) )
#   error "D_CFG_TABLE_DEFAULT_CELL_COUNT must be a D_CFG_TABLE_STRICTNESS_* value"
#endif
#if ( (D_CFG_NORM(D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT) < 0) ||                   \
      (D_CFG_NORM(D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT) > 3) )
#   error "D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT must be a D_CFG_TABLE_STRICTNESS_* value"
#endif
#if ( (D_CFG_NORM(D_CFG_TABLE_DEFAULT_TYPE_CONFORMANCE) < 0) ||                  \
      (D_CFG_NORM(D_CFG_TABLE_DEFAULT_TYPE_CONFORMANCE) > 2) )
#   error "D_CFG_TABLE_DEFAULT_TYPE_CONFORMANCE must be a D_CFG_TABLE_TYPE_* value"
#endif
#if ( (D_CFG_NORM(D_CFG_TABLE_DEFAULT_SHAPE) < 0) ||                             \
      (D_CFG_NORM(D_CFG_TABLE_DEFAULT_SHAPE) > 2) )
#   error "D_CFG_TABLE_DEFAULT_SHAPE must be a D_CFG_TABLE_SHAPE_* value"
#endif
#if ( (D_CFG_NORM(D_CFG_TABLE_DEFAULT_HEADER) < 0) ||                            \
      (D_CFG_NORM(D_CFG_TABLE_DEFAULT_HEADER) > 2) )
#   error "D_CFG_TABLE_DEFAULT_HEADER must be a D_CFG_TABLE_HEADER_* value"
#endif
#if ( (D_CFG_NORM(D_CFG_TABLE_DEFAULT_DOMAIN) < 0) ||                            \
      (D_CFG_NORM(D_CFG_TABLE_DEFAULT_DOMAIN) > 1) )
#   error "D_CFG_TABLE_DEFAULT_DOMAIN must be a D_CFG_TABLE_DOMAIN_* value"
#endif
#if ( (D_CFG_NORM(D_CFG_TABLE_DEFAULT_TRIM) < 0) ||                              \
      (D_CFG_NORM(D_CFG_TABLE_DEFAULT_TRIM) > 1) )
#   error "D_CFG_TABLE_DEFAULT_TRIM must be a D_CFG_TABLE_TRIM_* value"
#endif
#if ( (D_CFG_NORM(D_CFG_TABLE_DEFAULT_PIPE) < 0) ||                              \
      (D_CFG_NORM(D_CFG_TABLE_DEFAULT_PIPE) > 1) )
#   error "D_CFG_TABLE_DEFAULT_PIPE must be a D_CFG_TABLE_PIPE_* value"
#endif
#if ( (D_CFG_NORM(D_CFG_TABLE_DEFAULT_ANCHOR) < 0) ||                            \
      (D_CFG_NORM(D_CFG_TABLE_DEFAULT_ANCHOR) > 1) )
#   error "D_CFG_TABLE_DEFAULT_ANCHOR must be a D_CFG_TABLE_ANCHOR_* value"
#endif


// ================================================================
// 0.7  Effective (Derived) Values
// ================================================================
//   What the modules actually read.  Not user-settable.

// --- 0.7a  Effective layer gates ---

// D_INTERNAL_TABLE_BUILDER / _RENDER / _PARSER / _TEMPLATE
//   brief: effective enable for each leg, after the aggregate and the
// propagation above.  The module reads exactly this and gates its own body.
#if D_CFG_IS_ON(D_CFG_TABLE_BUILDER)
#   define D_INTERNAL_TABLE_BUILDER         1
#else
#   define D_INTERNAL_TABLE_BUILDER         0
#endif

#if D_CFG_IS_ON(D_CFG_TABLE_RENDER)
#   define D_INTERNAL_TABLE_RENDER          1
#else
#   define D_INTERNAL_TABLE_RENDER          0
#endif

#if D_CFG_IS_ON(D_CFG_TABLE_PARSER)
#   define D_INTERNAL_TABLE_PARSER          1
#else
#   define D_INTERNAL_TABLE_PARSER          0
#endif

#if D_CFG_IS_ON(D_CFG_TABLE_TEMPLATE)
#   define D_INTERNAL_TABLE_TEMPLATE        1
#else
#   define D_INTERNAL_TABLE_TEMPLATE        0
#endif

// D_INTERNAL_TABLE_STATIC_ASSERTS
//   brief: effective enable for the conformance assertions.
#if D_CFG_IS_ON(D_CFG_TABLE_STATIC_ASSERTS)
#   define D_INTERNAL_TABLE_STATIC_ASSERTS  1
#else
#   define D_INTERNAL_TABLE_STATIC_ASSERTS  0
#endif

// D_INTERNAL_TABLE_CONCEPTS
//   brief: effective enable for the C++20 concept faces -- the knob AND the
// language actually having concepts.  This is the whole reason the module can
// read one symbol: the env detection is resolved here, not re-derived there.
#if ( D_CFG_IS_ON(D_CFG_TABLE_CONCEPTS) &&                                       \
      defined(D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                  \
      (D_ENV_LANG_IS_CPP20_OR_HIGHER == 1) &&                                    \
      defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                                \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )
#   define D_INTERNAL_TABLE_CONCEPTS        1
#else
#   define D_INTERNAL_TABLE_CONCEPTS        0
#endif

// --- 0.7b  Effective policy defaults ---
//   Resolved from the knob's integer into the ENUMERATOR the module pastes:
//
//       default_cell_count = table_strictness::D_INTERNAL_TABLE_DEFAULT_CELL_COUNT;
//
// so table_options.hpp carries no mapping of its own -- it reads the answer.

#if   (D_CFG_TABLE_DEFAULT_CELL_COUNT == D_CFG_TABLE_STRICTNESS_EXACT)
#   define D_INTERNAL_TABLE_DEFAULT_CELL_COUNT          exact
#elif (D_CFG_TABLE_DEFAULT_CELL_COUNT == D_CFG_TABLE_STRICTNESS_TRUNCATE)
#   define D_INTERNAL_TABLE_DEFAULT_CELL_COUNT          truncate
#elif (D_CFG_TABLE_DEFAULT_CELL_COUNT == D_CFG_TABLE_STRICTNESS_PAD)
#   define D_INTERNAL_TABLE_DEFAULT_CELL_COUNT          pad
#else
#   define D_INTERNAL_TABLE_DEFAULT_CELL_COUNT          lenient
#endif

#if   (D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT == D_CFG_TABLE_STRICTNESS_EXACT)
#   define D_INTERNAL_TABLE_DEFAULT_PLACEHOLDER_FIT     exact
#elif (D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT == D_CFG_TABLE_STRICTNESS_TRUNCATE)
#   define D_INTERNAL_TABLE_DEFAULT_PLACEHOLDER_FIT     truncate
#elif (D_CFG_TABLE_DEFAULT_PLACEHOLDER_FIT == D_CFG_TABLE_STRICTNESS_PAD)
#   define D_INTERNAL_TABLE_DEFAULT_PLACEHOLDER_FIT     pad
#else
#   define D_INTERNAL_TABLE_DEFAULT_PLACEHOLDER_FIT     lenient
#endif

#if   (D_CFG_TABLE_DEFAULT_TYPE_CONFORMANCE == D_CFG_TABLE_TYPE_REQUIRE)
#   define D_INTERNAL_TABLE_DEFAULT_TYPE_CONFORMANCE    require
#elif (D_CFG_TABLE_DEFAULT_TYPE_CONFORMANCE == D_CFG_TABLE_TYPE_COERCE)
#   define D_INTERNAL_TABLE_DEFAULT_TYPE_CONFORMANCE    coerce
#else
#   define D_INTERNAL_TABLE_DEFAULT_TYPE_CONFORMANCE    ignore
#endif

#if   (D_CFG_TABLE_DEFAULT_SHAPE == D_CFG_TABLE_SHAPE_RECTANGULAR)
#   define D_INTERNAL_TABLE_DEFAULT_SHAPE               rectangular
#elif (D_CFG_TABLE_DEFAULT_SHAPE == D_CFG_TABLE_SHAPE_JAGGED)
#   define D_INTERNAL_TABLE_DEFAULT_SHAPE               jagged
#else
#   define D_INTERNAL_TABLE_DEFAULT_SHAPE               sparse
#endif

#if   (D_CFG_TABLE_DEFAULT_HEADER == D_CFG_TABLE_HEADER_REQUIRE_SEP)
#   define D_INTERNAL_TABLE_DEFAULT_HEADER              require_separator
#elif (D_CFG_TABLE_DEFAULT_HEADER == D_CFG_TABLE_HEADER_OPTIONAL_SEP)
#   define D_INTERNAL_TABLE_DEFAULT_HEADER              optional_separator
#else
#   define D_INTERNAL_TABLE_DEFAULT_HEADER              no_headers
#endif

#if   (D_CFG_TABLE_DEFAULT_DOMAIN == D_CFG_TABLE_DOMAIN_IGNORE)
#   define D_INTERNAL_TABLE_DEFAULT_DOMAIN              ignore
#else
#   define D_INTERNAL_TABLE_DEFAULT_DOMAIN              enforce
#endif

#if   (D_CFG_TABLE_DEFAULT_TRIM == D_CFG_TABLE_TRIM_TRIM)
#   define D_INTERNAL_TABLE_DEFAULT_TRIM                trim
#else
#   define D_INTERNAL_TABLE_DEFAULT_TRIM                keep
#endif

#if   (D_CFG_TABLE_DEFAULT_PIPE == D_CFG_TABLE_PIPE_REQUIRE_BORDERS)
#   define D_INTERNAL_TABLE_DEFAULT_PIPE                require_borders
#else
#   define D_INTERNAL_TABLE_DEFAULT_PIPE                optional_borders
#endif

#if   (D_CFG_TABLE_DEFAULT_ANCHOR == D_CFG_TABLE_ANCHOR_LEX_LEAST)
#   define D_INTERNAL_TABLE_DEFAULT_ANCHOR              lexicographic_least
#else
#   define D_INTERNAL_TABLE_DEFAULT_ANCHOR              declared
#endif

// --- 0.7c  Effective dialect ---
//   Pass-throughs today, but named as D_INTERNAL_* so the modules read one
// vocabulary and a future derivation (a dialect preset, say) has somewhere to go.

#define D_INTERNAL_TABLE_DELIMITER          D_CFG_TABLE_DELIMITER
#define D_INTERNAL_TABLE_PAD                D_CFG_TABLE_PAD
#define D_INTERNAL_TABLE_SEPARATOR_FILL     D_CFG_TABLE_SEPARATOR_FILL
#define D_INTERNAL_TABLE_ELLIPSIS           D_CFG_TABLE_ELLIPSIS
#define D_INTERNAL_TABLE_ITERATION_SIGIL    D_CFG_TABLE_ITERATION_SIGIL


#endif  // DJINTERP_CFG_TABLE_
