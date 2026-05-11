/******************************************************************************
* djinterp [css]                                                       css.hpp
*
*   Foundational CSS module for the djinterp framework. Models a
* CSS stylesheet as a tree of *rules*: style rules carry selector
* lists and declaration blocks; at-rules carry an at-keyword, an
* optional prelude, and either a declaration block or nested rules.
* The structure is deliberately distinct from `xml.hpp` / `html.hpp`
* / `markdown.hpp` -- CSS is not a tagged-element format and not a
* bipartite block/inline AST.
*
*   ZERO OVERHEAD:
*   - All kind enums are `std::uint8_t`-backed.
*   - Category and classification predicates are `D_CONSTEXPR` and
*     fold to compile-time constants when the kind is known
*     statically.
*   - Property name constants live as `D_STATIC_CONSTEXPR
*     const char*` literal pointers in `props::` -- zero allocation,
*     zero dynamic init.
*   - The bundled default backend stores rules in a single node type
*     with optional fields; adapter backends (libcss, katana-parser,
*     stylo / Servo, etc.) are free to use distinct types.
*
*   LIBRARY AGNOSTICISM:
*   This header pulls in nothing beyond the standard library. The
* trait layer (`css_template_traits.hpp`) detects backend types
*   structurally so any third-party AST can plug in.
*
*   LEVEL & MODULE GATING:
*   The runtime `css_level` enum identifies the document's target
* CSS level (1, 2.1, 3 modules-by-module, 4 drafts). A full
* compile-time `css_dialect<_Flag>` template, mirroring
* `html_dialect`, is a likely follow-on for static feature gating.
*
*
* path:      /inc/djinterp/core/util/css/css.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TYPES & CONSTANTS
II.   CSS LEVEL & SYNTAX MODE
III.  RULE KIND ENUM
IV.   AT-RULE KIND ENUM
V.    DECLARATION ORIGIN / IMPORTANCE
VI.   SELECTOR / COMBINATOR ENUMS
VII.  VALUE TYPE ENUM
VIII. RULE CATEGORY CLASSIFICATION
IX.   PROPERTY NAME NAMESPACE
X.    AT-RULE NAME NAMESPACE
XI.   PSEUDO-CLASS / PSEUDO-ELEMENT NAMESPACES
XII.  KIND <-> NAME MAPPING
XIII. BACKEND TAG DISPATCH
XIV.  BACKEND DETECTION
XV.   SUB-MODULE INCLUDES
*/

#ifndef DJINTERP_CSS_
#define DJINTERP_CSS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
// djinterp
#include "../../../djinterp.hpp"


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TYPES & CONSTANTS                            ///
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP

// NOTE: djinterp.hpp does not (yet) define NS_CSS. Using a plain
// namespace declaration here matching the html / markdown / font
// pattern; add `#define NS_CSS D_NAMESPACE(css)` alongside the
// other NS_* macros if the macro idiom is preferred.
namespace css {


// css_string_t
//   type: alias for the string type used throughout the css
// module. `std::string` matches the rest of the framework.
using css_string_t  = std::string;


// css_size_t
//   type: alias for the size type used for counts and indices.
using css_size_t    = std::size_t;


// D_CSS_DEFAULT_INDENT
//   constant: default whitespace string used per indentation
// level when serialising CSS source.
#ifndef D_CSS_DEFAULT_INDENT
    #define D_CSS_DEFAULT_INDENT        "  "
#endif


// D_CSS_DEFAULT_NEWLINE
//   constant: default line terminator emitted between rules
// and declarations.
#ifndef D_CSS_DEFAULT_NEWLINE
    #define D_CSS_DEFAULT_NEWLINE       "\n"
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.   CSS LEVEL & SYNTAX MODE                            ///
///////////////////////////////////////////////////////////////////////////////

// css_level
//   enum: identifies the document's target CSS level. CSS3 is
// modular -- "level 3" really means a particular module set --
// but the coarse level tag is still useful for runtime
// classification. CSS4 has no integrated spec; the value is
// used for "currently-in-draft" features.
enum class css_level : std::uint8_t
{
    css_1,
    css_2_1,
    css_3,
    css_4,
    custom,
    unspecified
};


// css_syntax_mode
//   enum: identifies dialect-level differences in source syntax
// (vanilla CSS, SCSS, Sass-indented, Less, Stylus, native CSS
// nesting). Drives parsing and emission rules; the AST itself
// is the same shape across all dialects.
enum class css_syntax_mode : std::uint8_t
{
    css,                // vanilla CSS
    scss,               // SCSS (curly-brace Sass)
    sass,               // Sass (indented syntax)
    less,               // Less
    stylus,             // Stylus
    css_nesting,        // native CSS nesting (CSS Nesting Module)
    postcss,            // PostCSS (treat as vanilla + plugins)
    custom,
    unspecified
};


///////////////////////////////////////////////////////////////////////////////
///                III.   RULE KIND ENUM                                    ///
///////////////////////////////////////////////////////////////////////////////

// css_rule_kind
//   enum: discriminator for top-level rule entries in a
// stylesheet. Backed by `std::uint8_t`. The `unknown` value
// covers proprietary and not-yet-modelled rule kinds.
enum class css_rule_kind : std::uint8_t
{
    // top-level
    stylesheet,             // root container

    // primary rule kinds
    style_rule,             // selector { declarations }
    at_rule,                // @keyword ...
    comment,                // /* ... */

    // commonly accessed at-rule subkinds (also reachable via
    // `at_rule` + `css_at_rule_kind`; pulled into the rule
    // enum for convenient direct dispatch)
    media_rule,             // @media ...
    supports_rule,          // @supports ...
    keyframes_rule,         // @keyframes ...
    keyframe_rule,          // single keyframe block
    font_face_rule,         // @font-face
    page_rule,              // @page
    namespace_rule,         // @namespace
    import_rule,            // @import
    charset_rule,           // @charset
    document_rule,          // @document (Mozilla)
    container_rule,         // @container (CSS Containment)
    layer_rule,             // @layer
    scope_rule,             // @scope
    starting_style_rule,    // @starting-style
    counter_style_rule,     // @counter-style
    font_feature_values_rule,  // @font-feature-values
    property_rule,          // @property
    viewport_rule,          // @viewport (deprecated)

    // declaration block (when reified as a rule)
    declaration_block,

    // sentinel
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   AT-RULE KIND ENUM                                  ///
///////////////////////////////////////////////////////////////////////////////

// css_at_rule_kind
//   enum: refines `css_rule_kind::at_rule` with the specific
// at-keyword. Useful when a backend stores all at-rules under
// a single `at_rule` discriminator and uses this enum to
// further classify.
enum class css_at_rule_kind : std::uint8_t
{
    media,
    supports,
    keyframes,
    font_face,
    page,
    namespace_,             // trailing _ to avoid keyword
    import_,
    charset,
    document,
    container,
    layer,
    scope,
    starting_style,
    counter_style,
    font_feature_values,
    property,
    viewport,
    apply,                  // @apply (Tailwind / proposal)
    custom,                 // user-defined at-rule
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                V.   DECLARATION ORIGIN / IMPORTANCE                     ///
///////////////////////////////////////////////////////////////////////////////

// css_origin
//   enum: identifies the cascade origin of a declaration. Used
// by adapter backends that surface origin to client code; the
// default backend stores `author` for everything authored.
enum class css_origin : std::uint8_t
{
    user_agent,
    user,
    author,
    animation,
    transition,
    inline_,                // style="..." attribute
    unknown
};


// css_importance
//   enum: identifies whether a declaration carries `!important`.
enum class css_importance : std::uint8_t
{
    normal,
    important
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   SELECTOR / COMBINATOR ENUMS                        ///
///////////////////////////////////////////////////////////////////////////////

// css_selector_kind
//   enum: categorises a single simple selector. The default
// backend stores selectors as opaque strings, but exposes this
// enum for adapter backends that parse the selector grammar
// (libcss, katana-parser, stylo).
enum class css_selector_kind : std::uint8_t
{
    type,                   // div
    universal,              // *
    class_,                 // .name
    id,                     // #name
    attribute,              // [attr=value]
    pseudo_class,           // :hover
    pseudo_element,         // ::before
    nesting,                // & (CSS Nesting / SCSS / Less)
    selector_list,          // a, b, c
    compound,               // a.b#c[d]:e
    complex,                // a > b ~ c
    relative,               // > a (used inside :has(), :is(), etc.)
    unknown = 255
};


// css_combinator
//   enum: identifies a combinator between compound selectors.
enum class css_combinator : std::uint8_t
{
    none,                   // no combinator (compound)
    descendant,             // " " (whitespace)
    child,                  // >
    next_sibling,           // +
    subsequent_sibling,     // ~
    column,                 // || (CSS Selectors Level 4)
    nesting,                // & (used to splice in parent context)
    unknown = 255
};


// css_attribute_match
//   enum: identifies the match operator inside an attribute
// selector ([attr OP value]).
enum class css_attribute_match : std::uint8_t
{
    exists,                 // [attr]
    equals,                 // [attr=value]
    includes,               // [attr~=value]
    dash_match,             // [attr|=value]
    prefix,                 // [attr^=value]
    suffix,                 // [attr$=value]
    substring,              // [attr*=value]
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                VII.   VALUE TYPE ENUM                                   ///
///////////////////////////////////////////////////////////////////////////////

// css_value_kind
//   enum: identifies the syntactic category of a property
// value. The default backend stores values as opaque strings,
// but exposes this enum for adapter backends that classify
// values during parsing.
enum class css_value_kind : std::uint8_t
{
    keyword,                // auto, none, inherit
    identifier,             // bare ident not classified as keyword
    string,                 // "quoted"
    number,                 // 1.5
    integer,                // 42
    percentage,             // 50%
    dimension,              // 10px, 2em, 1.5rem
    color,                  // #fff, rgb(), hsl(), color()
    url,                    // url(...)
    function,               // calc(), var(), env(), linear-gradient(...)
    list_comma,             // a, b, c
    list_space,             // a b c
    list_slash,             // a / b
    custom_property_ref,    // var(--name)
    important_marker,       // !important (own token)
    initial,                // initial keyword
    inherit,                // inherit keyword
    unset,                  // unset keyword
    revert,                 // revert keyword
    revert_layer,           // revert-layer keyword
    raw,                    // unparsed token sequence
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                VIII.   RULE CATEGORY CLASSIFICATION                     ///
///////////////////////////////////////////////////////////////////////////////

// css_rule_category
//   enum: bitmask describing the categorical properties of a
// rule kind. Backed by `unsigned`.
enum css_rule_category : unsigned
{
    crc_none           = 0u,
    crc_root           = 1u <<  0,  // top-level container
    crc_style          = 1u <<  1,  // selector + declarations
    crc_at             = 1u <<  2,  // at-rule
    crc_conditional    = 1u <<  3,  // @media, @supports, @container, ...
    crc_grouping       = 1u <<  4,  // contains nested rules
    crc_descriptor     = 1u <<  5,  // declarations describe an entity
    crc_keyframes      = 1u <<  6,  // @keyframes / single keyframe
    crc_layer          = 1u <<  7,  // @layer (cascade layer)
    crc_namespace      = 1u <<  8,  // @namespace
    crc_import         = 1u <<  9,  // @import
    crc_charset        = 1u << 10,  // @charset (must be first)
    crc_comment        = 1u << 11,
    crc_declaration    = 1u << 12,  // bare declaration block
    crc_nestable       = 1u << 13   // may host nested rules (CSS Nesting)
};


// css_category_for_rule_kind
//   function: returns the category bitmask for a rule kind.
D_CONSTEXPR_INLINE unsigned
css_category_for_rule_kind(
    css_rule_kind   _k
)
{
    using K = css_rule_kind;
    return (_k == K::stylesheet)            ? (crc_root | crc_grouping)
         : (_k == K::style_rule)            ? (crc_style | crc_nestable)
         : (_k == K::at_rule)               ? (crc_at)
         : (_k == K::comment)               ? (crc_comment)

         : (_k == K::media_rule)            ? (crc_at | crc_conditional
                                               | crc_grouping | crc_nestable)
         : (_k == K::supports_rule)         ? (crc_at | crc_conditional
                                               | crc_grouping | crc_nestable)
         : (_k == K::container_rule)        ? (crc_at | crc_conditional
                                               | crc_grouping | crc_nestable)
         : (_k == K::scope_rule)            ? (crc_at | crc_conditional
                                               | crc_grouping | crc_nestable)
         : (_k == K::document_rule)         ? (crc_at | crc_conditional
                                               | crc_grouping)
         : (_k == K::starting_style_rule)   ? (crc_at | crc_conditional
                                               | crc_grouping)

         : (_k == K::keyframes_rule)        ? (crc_at | crc_grouping
                                               | crc_keyframes)
         : (_k == K::keyframe_rule)         ? (crc_keyframes
                                               | crc_descriptor)

         : (_k == K::layer_rule)            ? (crc_at | crc_grouping
                                               | crc_layer | crc_nestable)

         : (_k == K::font_face_rule)        ? (crc_at | crc_descriptor)
         : (_k == K::page_rule)             ? (crc_at | crc_descriptor
                                               | crc_grouping)
         : (_k == K::counter_style_rule)    ? (crc_at | crc_descriptor)
         : (_k == K::font_feature_values_rule) ? (crc_at | crc_descriptor
                                                  | crc_grouping)
         : (_k == K::property_rule)         ? (crc_at | crc_descriptor)
         : (_k == K::viewport_rule)         ? (crc_at | crc_descriptor)

         : (_k == K::namespace_rule)        ? (crc_at | crc_namespace)
         : (_k == K::import_rule)           ? (crc_at | crc_import)
         : (_k == K::charset_rule)          ? (crc_at | crc_charset)

         : (_k == K::declaration_block)     ? (crc_declaration)

         :    crc_none;
}


// is_at_rule_kind
//   function: true if the rule kind is an at-rule (any flavour).
D_CONSTEXPR_INLINE bool
is_at_rule_kind(
    css_rule_kind   _k
)
{
    return ((css_category_for_rule_kind(_k) & crc_at) != 0u);
}


// is_conditional_rule_kind
//   function: true for @media, @supports, @container, @scope,
// @document, @starting-style.
D_CONSTEXPR_INLINE bool
is_conditional_rule_kind(
    css_rule_kind   _k
)
{
    return ((css_category_for_rule_kind(_k) & crc_conditional) != 0u);
}


// is_grouping_rule_kind
//   function: true if the rule kind hosts nested rules.
D_CONSTEXPR_INLINE bool
is_grouping_rule_kind(
    css_rule_kind   _k
)
{
    return ((css_category_for_rule_kind(_k) & crc_grouping) != 0u);
}


// is_descriptor_rule_kind
//   function: true if the rule kind's body is a list of
// descriptors rather than property declarations (e.g.
// @font-face, @counter-style).
D_CONSTEXPR_INLINE bool
is_descriptor_rule_kind(
    css_rule_kind   _k
)
{
    return ((css_category_for_rule_kind(_k) & crc_descriptor) != 0u);
}


// is_nestable_rule_kind
//   function: true if the rule kind may itself host nested
// style rules (CSS Nesting). Style rules and conditional
// at-rules qualify; descriptor at-rules do not.
D_CONSTEXPR_INLINE bool
is_nestable_rule_kind(
    css_rule_kind   _k
)
{
    return ((css_category_for_rule_kind(_k) & crc_nestable) != 0u);
}


// is_keyframes_rule_kind
//   function: true for @keyframes and single keyframe rules.
D_CONSTEXPR_INLINE bool
is_keyframes_rule_kind(
    css_rule_kind   _k
)
{
    return ((css_category_for_rule_kind(_k) & crc_keyframes) != 0u);
}


///////////////////////////////////////////////////////////////////////////////
///                IX.   PROPERTY NAME NAMESPACE                            ///
///////////////////////////////////////////////////////////////////////////////

// props
//   namespace: `D_STATIC_CONSTEXPR const char*` constants for
// the most commonly-accessed CSS property names. Use these
// instead of literal strings when calling `set_property`,
// `find_declaration`, etc. The list is intentionally limited
// to high-traffic properties; less common ones can always be
// passed as raw strings without loss of functionality.
//   Underscore suffix on names that collide with C++ keywords
// or reserved identifiers, matching the html::tags / html::attrs
// convention.
namespace props {

    // box / layout
    D_STATIC_CONSTEXPR const char* display          = "display";
    D_STATIC_CONSTEXPR const char* position         = "position";
    D_STATIC_CONSTEXPR const char* top              = "top";
    D_STATIC_CONSTEXPR const char* right            = "right";
    D_STATIC_CONSTEXPR const char* bottom           = "bottom";
    D_STATIC_CONSTEXPR const char* left             = "left";
    D_STATIC_CONSTEXPR const char* float_           = "float";
    D_STATIC_CONSTEXPR const char* clear            = "clear";
    D_STATIC_CONSTEXPR const char* z_index          = "z-index";
    D_STATIC_CONSTEXPR const char* overflow         = "overflow";
    D_STATIC_CONSTEXPR const char* overflow_x       = "overflow-x";
    D_STATIC_CONSTEXPR const char* overflow_y       = "overflow-y";
    D_STATIC_CONSTEXPR const char* visibility       = "visibility";
    D_STATIC_CONSTEXPR const char* opacity          = "opacity";

    // box dimensions
    D_STATIC_CONSTEXPR const char* width            = "width";
    D_STATIC_CONSTEXPR const char* height           = "height";
    D_STATIC_CONSTEXPR const char* min_width        = "min-width";
    D_STATIC_CONSTEXPR const char* min_height       = "min-height";
    D_STATIC_CONSTEXPR const char* max_width        = "max-width";
    D_STATIC_CONSTEXPR const char* max_height       = "max-height";
    D_STATIC_CONSTEXPR const char* aspect_ratio     = "aspect-ratio";
    D_STATIC_CONSTEXPR const char* box_sizing       = "box-sizing";

    // margin / padding
    D_STATIC_CONSTEXPR const char* margin           = "margin";
    D_STATIC_CONSTEXPR const char* margin_top       = "margin-top";
    D_STATIC_CONSTEXPR const char* margin_right     = "margin-right";
    D_STATIC_CONSTEXPR const char* margin_bottom    = "margin-bottom";
    D_STATIC_CONSTEXPR const char* margin_left      = "margin-left";
    D_STATIC_CONSTEXPR const char* padding          = "padding";
    D_STATIC_CONSTEXPR const char* padding_top      = "padding-top";
    D_STATIC_CONSTEXPR const char* padding_right    = "padding-right";
    D_STATIC_CONSTEXPR const char* padding_bottom   = "padding-bottom";
    D_STATIC_CONSTEXPR const char* padding_left     = "padding-left";

    // border
    D_STATIC_CONSTEXPR const char* border           = "border";
    D_STATIC_CONSTEXPR const char* border_top       = "border-top";
    D_STATIC_CONSTEXPR const char* border_right     = "border-right";
    D_STATIC_CONSTEXPR const char* border_bottom    = "border-bottom";
    D_STATIC_CONSTEXPR const char* border_left      = "border-left";
    D_STATIC_CONSTEXPR const char* border_width     = "border-width";
    D_STATIC_CONSTEXPR const char* border_style     = "border-style";
    D_STATIC_CONSTEXPR const char* border_color     = "border-color";
    D_STATIC_CONSTEXPR const char* border_radius    = "border-radius";

    D_STATIC_CONSTEXPR const char* outline          = "outline";
    D_STATIC_CONSTEXPR const char* outline_width    = "outline-width";
    D_STATIC_CONSTEXPR const char* outline_style    = "outline-style";
    D_STATIC_CONSTEXPR const char* outline_color    = "outline-color";
    D_STATIC_CONSTEXPR const char* outline_offset   = "outline-offset";

    // background
    D_STATIC_CONSTEXPR const char* background          = "background";
    D_STATIC_CONSTEXPR const char* background_color    = "background-color";
    D_STATIC_CONSTEXPR const char* background_image    = "background-image";
    D_STATIC_CONSTEXPR const char* background_position = "background-position";
    D_STATIC_CONSTEXPR const char* background_repeat   = "background-repeat";
    D_STATIC_CONSTEXPR const char* background_size     = "background-size";
    D_STATIC_CONSTEXPR const char* background_clip     = "background-clip";
    D_STATIC_CONSTEXPR const char* background_origin   = "background-origin";

    // text & font
    D_STATIC_CONSTEXPR const char* color             = "color";
    D_STATIC_CONSTEXPR const char* font              = "font";
    D_STATIC_CONSTEXPR const char* font_family       = "font-family";
    D_STATIC_CONSTEXPR const char* font_size         = "font-size";
    D_STATIC_CONSTEXPR const char* font_weight       = "font-weight";
    D_STATIC_CONSTEXPR const char* font_style        = "font-style";
    D_STATIC_CONSTEXPR const char* font_variant      = "font-variant";
    D_STATIC_CONSTEXPR const char* font_stretch      = "font-stretch";
    D_STATIC_CONSTEXPR const char* line_height       = "line-height";
    D_STATIC_CONSTEXPR const char* letter_spacing    = "letter-spacing";
    D_STATIC_CONSTEXPR const char* word_spacing      = "word-spacing";
    D_STATIC_CONSTEXPR const char* text_align        = "text-align";
    D_STATIC_CONSTEXPR const char* text_decoration   = "text-decoration";
    D_STATIC_CONSTEXPR const char* text_transform    = "text-transform";
    D_STATIC_CONSTEXPR const char* text_indent       = "text-indent";
    D_STATIC_CONSTEXPR const char* text_overflow     = "text-overflow";
    D_STATIC_CONSTEXPR const char* white_space       = "white-space";
    D_STATIC_CONSTEXPR const char* word_break        = "word-break";
    D_STATIC_CONSTEXPR const char* word_wrap         = "word-wrap";
    D_STATIC_CONSTEXPR const char* direction         = "direction";

    // flexbox
    D_STATIC_CONSTEXPR const char* flex              = "flex";
    D_STATIC_CONSTEXPR const char* flex_direction    = "flex-direction";
    D_STATIC_CONSTEXPR const char* flex_wrap         = "flex-wrap";
    D_STATIC_CONSTEXPR const char* flex_flow         = "flex-flow";
    D_STATIC_CONSTEXPR const char* flex_grow         = "flex-grow";
    D_STATIC_CONSTEXPR const char* flex_shrink       = "flex-shrink";
    D_STATIC_CONSTEXPR const char* flex_basis        = "flex-basis";
    D_STATIC_CONSTEXPR const char* justify_content   = "justify-content";
    D_STATIC_CONSTEXPR const char* align_items       = "align-items";
    D_STATIC_CONSTEXPR const char* align_self        = "align-self";
    D_STATIC_CONSTEXPR const char* align_content     = "align-content";
    D_STATIC_CONSTEXPR const char* gap               = "gap";
    D_STATIC_CONSTEXPR const char* row_gap           = "row-gap";
    D_STATIC_CONSTEXPR const char* column_gap        = "column-gap";
    D_STATIC_CONSTEXPR const char* order             = "order";

    // grid
    D_STATIC_CONSTEXPR const char* grid              = "grid";
    D_STATIC_CONSTEXPR const char* grid_template     = "grid-template";
    D_STATIC_CONSTEXPR const char* grid_template_rows
        = "grid-template-rows";
    D_STATIC_CONSTEXPR const char* grid_template_columns
        = "grid-template-columns";
    D_STATIC_CONSTEXPR const char* grid_template_areas
        = "grid-template-areas";
    D_STATIC_CONSTEXPR const char* grid_area         = "grid-area";
    D_STATIC_CONSTEXPR const char* grid_row          = "grid-row";
    D_STATIC_CONSTEXPR const char* grid_column       = "grid-column";
    D_STATIC_CONSTEXPR const char* grid_auto_rows    = "grid-auto-rows";
    D_STATIC_CONSTEXPR const char* grid_auto_columns = "grid-auto-columns";
    D_STATIC_CONSTEXPR const char* grid_auto_flow    = "grid-auto-flow";
    D_STATIC_CONSTEXPR const char* place_items       = "place-items";
    D_STATIC_CONSTEXPR const char* place_content     = "place-content";
    D_STATIC_CONSTEXPR const char* place_self        = "place-self";

    // transform / transition / animation
    D_STATIC_CONSTEXPR const char* transform                = "transform";
    D_STATIC_CONSTEXPR const char* transform_origin         = "transform-origin";
    D_STATIC_CONSTEXPR const char* transition               = "transition";
    D_STATIC_CONSTEXPR const char* transition_property      = "transition-property";
    D_STATIC_CONSTEXPR const char* transition_duration      = "transition-duration";
    D_STATIC_CONSTEXPR const char* transition_timing_function
        = "transition-timing-function";
    D_STATIC_CONSTEXPR const char* transition_delay         = "transition-delay";
    D_STATIC_CONSTEXPR const char* animation                = "animation";
    D_STATIC_CONSTEXPR const char* animation_name           = "animation-name";
    D_STATIC_CONSTEXPR const char* animation_duration       = "animation-duration";
    D_STATIC_CONSTEXPR const char* animation_timing_function
        = "animation-timing-function";
    D_STATIC_CONSTEXPR const char* animation_delay          = "animation-delay";
    D_STATIC_CONSTEXPR const char* animation_iteration_count
        = "animation-iteration-count";
    D_STATIC_CONSTEXPR const char* animation_direction      = "animation-direction";
    D_STATIC_CONSTEXPR const char* animation_fill_mode      = "animation-fill-mode";
    D_STATIC_CONSTEXPR const char* animation_play_state     = "animation-play-state";

    // misc
    D_STATIC_CONSTEXPR const char* cursor             = "cursor";
    D_STATIC_CONSTEXPR const char* pointer_events     = "pointer-events";
    D_STATIC_CONSTEXPR const char* user_select        = "user-select";
    D_STATIC_CONSTEXPR const char* will_change        = "will-change";
    D_STATIC_CONSTEXPR const char* content            = "content";
    D_STATIC_CONSTEXPR const char* list_style         = "list-style";
    D_STATIC_CONSTEXPR const char* list_style_type    = "list-style-type";
    D_STATIC_CONSTEXPR const char* list_style_position= "list-style-position";
    D_STATIC_CONSTEXPR const char* list_style_image   = "list-style-image";
    D_STATIC_CONSTEXPR const char* table_layout       = "table-layout";
    D_STATIC_CONSTEXPR const char* border_collapse    = "border-collapse";
    D_STATIC_CONSTEXPR const char* border_spacing     = "border-spacing";
    D_STATIC_CONSTEXPR const char* caption_side       = "caption-side";
    D_STATIC_CONSTEXPR const char* empty_cells        = "empty-cells";
    D_STATIC_CONSTEXPR const char* vertical_align     = "vertical-align";
    D_STATIC_CONSTEXPR const char* box_shadow         = "box-shadow";
    D_STATIC_CONSTEXPR const char* text_shadow        = "text-shadow";
    D_STATIC_CONSTEXPR const char* filter             = "filter";
    D_STATIC_CONSTEXPR const char* backdrop_filter    = "backdrop-filter";
    D_STATIC_CONSTEXPR const char* clip_path          = "clip-path";
    D_STATIC_CONSTEXPR const char* mix_blend_mode     = "mix-blend-mode";
    D_STATIC_CONSTEXPR const char* mask               = "mask";

    // logical properties
    D_STATIC_CONSTEXPR const char* inset                = "inset";
    D_STATIC_CONSTEXPR const char* inset_inline         = "inset-inline";
    D_STATIC_CONSTEXPR const char* inset_block          = "inset-block";
    D_STATIC_CONSTEXPR const char* inline_size          = "inline-size";
    D_STATIC_CONSTEXPR const char* block_size           = "block-size";
    D_STATIC_CONSTEXPR const char* margin_inline        = "margin-inline";
    D_STATIC_CONSTEXPR const char* margin_block         = "margin-block";
    D_STATIC_CONSTEXPR const char* padding_inline       = "padding-inline";
    D_STATIC_CONSTEXPR const char* padding_block        = "padding-block";

    // common custom-property prefixes (just the prefix; full
    // names are application-defined).
    D_STATIC_CONSTEXPR const char* custom_prefix        = "--";

}   // namespace props


///////////////////////////////////////////////////////////////////////////////
///                X.   AT-RULE NAME NAMESPACE                              ///
///////////////////////////////////////////////////////////////////////////////

// at_rules
//   namespace: `D_STATIC_CONSTEXPR const char*` constants for
// the standard at-rule keywords (without the leading `@`).
namespace at_rules {

    D_STATIC_CONSTEXPR const char* media               = "media";
    D_STATIC_CONSTEXPR const char* supports            = "supports";
    D_STATIC_CONSTEXPR const char* keyframes           = "keyframes";
    D_STATIC_CONSTEXPR const char* font_face           = "font-face";
    D_STATIC_CONSTEXPR const char* page                = "page";
    D_STATIC_CONSTEXPR const char* namespace_          = "namespace";
    D_STATIC_CONSTEXPR const char* import_             = "import";
    D_STATIC_CONSTEXPR const char* charset             = "charset";
    D_STATIC_CONSTEXPR const char* document            = "document";
    D_STATIC_CONSTEXPR const char* container           = "container";
    D_STATIC_CONSTEXPR const char* layer               = "layer";
    D_STATIC_CONSTEXPR const char* scope               = "scope";
    D_STATIC_CONSTEXPR const char* starting_style      = "starting-style";
    D_STATIC_CONSTEXPR const char* counter_style       = "counter-style";
    D_STATIC_CONSTEXPR const char* font_feature_values = "font-feature-values";
    D_STATIC_CONSTEXPR const char* property            = "property";
    D_STATIC_CONSTEXPR const char* viewport            = "viewport";
    D_STATIC_CONSTEXPR const char* apply               = "apply";

}   // namespace at_rules


///////////////////////////////////////////////////////////////////////////////
///                XI.   PSEUDO-CLASS / PSEUDO-ELEMENT NAMESPACES           ///
///////////////////////////////////////////////////////////////////////////////

// pseudo_classes
//   namespace: name constants for standard pseudo-classes
// (without the leading `:`).
namespace pseudo_classes {

    // user action
    D_STATIC_CONSTEXPR const char* hover                = "hover";
    D_STATIC_CONSTEXPR const char* active               = "active";
    D_STATIC_CONSTEXPR const char* focus                = "focus";
    D_STATIC_CONSTEXPR const char* focus_visible        = "focus-visible";
    D_STATIC_CONSTEXPR const char* focus_within         = "focus-within";
    D_STATIC_CONSTEXPR const char* visited              = "visited";
    D_STATIC_CONSTEXPR const char* link                 = "link";
    D_STATIC_CONSTEXPR const char* target               = "target";
    D_STATIC_CONSTEXPR const char* target_within        = "target-within";

    // tree-structural
    D_STATIC_CONSTEXPR const char* root                 = "root";
    D_STATIC_CONSTEXPR const char* empty_               = "empty";
    D_STATIC_CONSTEXPR const char* first_child          = "first-child";
    D_STATIC_CONSTEXPR const char* last_child           = "last-child";
    D_STATIC_CONSTEXPR const char* only_child           = "only-child";
    D_STATIC_CONSTEXPR const char* first_of_type        = "first-of-type";
    D_STATIC_CONSTEXPR const char* last_of_type         = "last-of-type";
    D_STATIC_CONSTEXPR const char* only_of_type         = "only-of-type";
    D_STATIC_CONSTEXPR const char* nth_child            = "nth-child";
    D_STATIC_CONSTEXPR const char* nth_last_child       = "nth-last-child";
    D_STATIC_CONSTEXPR const char* nth_of_type          = "nth-of-type";
    D_STATIC_CONSTEXPR const char* nth_last_of_type     = "nth-last-of-type";

    // form / input state
    D_STATIC_CONSTEXPR const char* checked_             = "checked";
    D_STATIC_CONSTEXPR const char* disabled             = "disabled";
    D_STATIC_CONSTEXPR const char* enabled              = "enabled";
    D_STATIC_CONSTEXPR const char* required_            = "required";
    D_STATIC_CONSTEXPR const char* optional_            = "optional";
    D_STATIC_CONSTEXPR const char* read_only            = "read-only";
    D_STATIC_CONSTEXPR const char* read_write           = "read-write";
    D_STATIC_CONSTEXPR const char* placeholder_shown    = "placeholder-shown";
    D_STATIC_CONSTEXPR const char* default_             = "default";
    D_STATIC_CONSTEXPR const char* valid                = "valid";
    D_STATIC_CONSTEXPR const char* invalid              = "invalid";
    D_STATIC_CONSTEXPR const char* in_range             = "in-range";
    D_STATIC_CONSTEXPR const char* out_of_range         = "out-of-range";
    D_STATIC_CONSTEXPR const char* indeterminate        = "indeterminate";

    // logical
    D_STATIC_CONSTEXPR const char* not_                 = "not";
    D_STATIC_CONSTEXPR const char* is_                  = "is";
    D_STATIC_CONSTEXPR const char* where                = "where";
    D_STATIC_CONSTEXPR const char* has                  = "has";

    // languages / locale
    D_STATIC_CONSTEXPR const char* lang                 = "lang";
    D_STATIC_CONSTEXPR const char* dir                  = "dir";

}   // namespace pseudo_classes


// pseudo_elements
//   namespace: name constants for standard pseudo-elements
// (without the leading `::`).
namespace pseudo_elements {

    D_STATIC_CONSTEXPR const char* before               = "before";
    D_STATIC_CONSTEXPR const char* after                = "after";
    D_STATIC_CONSTEXPR const char* first_line           = "first-line";
    D_STATIC_CONSTEXPR const char* first_letter         = "first-letter";
    D_STATIC_CONSTEXPR const char* selection            = "selection";
    D_STATIC_CONSTEXPR const char* placeholder          = "placeholder";
    D_STATIC_CONSTEXPR const char* marker               = "marker";
    D_STATIC_CONSTEXPR const char* backdrop             = "backdrop";
    D_STATIC_CONSTEXPR const char* file_selector_button = "file-selector-button";
    D_STATIC_CONSTEXPR const char* part                 = "part";
    D_STATIC_CONSTEXPR const char* slotted              = "slotted";
    D_STATIC_CONSTEXPR const char* highlight            = "highlight";
    D_STATIC_CONSTEXPR const char* spelling_error       = "spelling-error";
    D_STATIC_CONSTEXPR const char* grammar_error        = "grammar-error";
    D_STATIC_CONSTEXPR const char* target_text          = "target-text";

}   // namespace pseudo_elements


///////////////////////////////////////////////////////////////////////////////
///                XII.   KIND <-> NAME MAPPING                             ///
///////////////////////////////////////////////////////////////////////////////

// css_internal_streq
//   function: constexpr null-terminated string equality.
// Mirrors the helper used in `html.hpp` so this header stays
// dependency-free of the html module.
D_CONSTEXPR_INLINE bool
css_internal_streq(
    const char*     _a,
    const char*     _b
)
{
    return ( (*_a == *_b) &&
             ( (*_a == '\0') ||
               (css_internal_streq(_a + 1, _b + 1)) ) );
}


// at_rule_kind_from_name
//   function: maps an at-keyword (without the leading `@`) to
// its `css_at_rule_kind`. Returns `unknown` for unrecognised
// keywords.
D_CONSTEXPR_INLINE css_at_rule_kind
at_rule_kind_from_name(
    const char*     _name
)
{
    using K = css_at_rule_kind;
    return (!_name)                                              ? K::unknown
         : (css_internal_streq(_name, at_rules::media))          ? K::media
         : (css_internal_streq(_name, at_rules::supports))       ? K::supports
         : (css_internal_streq(_name, at_rules::keyframes))      ? K::keyframes
         : (css_internal_streq(_name, at_rules::font_face))      ? K::font_face
         : (css_internal_streq(_name, at_rules::page))           ? K::page
         : (css_internal_streq(_name, at_rules::namespace_))     ? K::namespace_
         : (css_internal_streq(_name, at_rules::import_))        ? K::import_
         : (css_internal_streq(_name, at_rules::charset))        ? K::charset
         : (css_internal_streq(_name, at_rules::document))       ? K::document
         : (css_internal_streq(_name, at_rules::container))      ? K::container
         : (css_internal_streq(_name, at_rules::layer))          ? K::layer
         : (css_internal_streq(_name, at_rules::scope))          ? K::scope
         : (css_internal_streq(_name, at_rules::starting_style)) ? K::starting_style
         : (css_internal_streq(_name, at_rules::counter_style))  ? K::counter_style
         : (css_internal_streq(_name, at_rules::font_feature_values))
                                                                 ? K::font_feature_values
         : (css_internal_streq(_name, at_rules::property))       ? K::property
         : (css_internal_streq(_name, at_rules::viewport))       ? K::viewport
         : (css_internal_streq(_name, at_rules::apply))          ? K::apply
         :    K::unknown;
}


// name_from_at_rule_kind
//   function: maps a `css_at_rule_kind` to its at-keyword
// string (without the leading `@`).
D_CONSTEXPR_INLINE const char*
name_from_at_rule_kind(
    css_at_rule_kind    _k
)
{
    using K = css_at_rule_kind;
    return (_k == K::media)               ? at_rules::media
         : (_k == K::supports)            ? at_rules::supports
         : (_k == K::keyframes)           ? at_rules::keyframes
         : (_k == K::font_face)           ? at_rules::font_face
         : (_k == K::page)                ? at_rules::page
         : (_k == K::namespace_)          ? at_rules::namespace_
         : (_k == K::import_)             ? at_rules::import_
         : (_k == K::charset)             ? at_rules::charset
         : (_k == K::document)            ? at_rules::document
         : (_k == K::container)           ? at_rules::container
         : (_k == K::layer)               ? at_rules::layer
         : (_k == K::scope)               ? at_rules::scope
         : (_k == K::starting_style)      ? at_rules::starting_style
         : (_k == K::counter_style)       ? at_rules::counter_style
         : (_k == K::font_feature_values) ? at_rules::font_feature_values
         : (_k == K::property)            ? at_rules::property
         : (_k == K::viewport)            ? at_rules::viewport
         : (_k == K::apply)               ? at_rules::apply
         :    "";
}


// rule_kind_from_at_rule_kind
//   function: maps an at-rule subkind to the corresponding
// dedicated `css_rule_kind` value (e.g.
// `css_at_rule_kind::media` -> `css_rule_kind::media_rule`).
// Returns `at_rule` for kinds without a dedicated rule kind.
D_CONSTEXPR_INLINE css_rule_kind
rule_kind_from_at_rule_kind(
    css_at_rule_kind    _k
)
{
    using A = css_at_rule_kind;
    using R = css_rule_kind;
    return (_k == A::media)               ? R::media_rule
         : (_k == A::supports)            ? R::supports_rule
         : (_k == A::keyframes)           ? R::keyframes_rule
         : (_k == A::font_face)           ? R::font_face_rule
         : (_k == A::page)                ? R::page_rule
         : (_k == A::namespace_)          ? R::namespace_rule
         : (_k == A::import_)             ? R::import_rule
         : (_k == A::charset)             ? R::charset_rule
         : (_k == A::document)            ? R::document_rule
         : (_k == A::container)           ? R::container_rule
         : (_k == A::layer)               ? R::layer_rule
         : (_k == A::scope)               ? R::scope_rule
         : (_k == A::starting_style)      ? R::starting_style_rule
         : (_k == A::counter_style)       ? R::counter_style_rule
         : (_k == A::font_feature_values) ? R::font_feature_values_rule
         : (_k == A::property)            ? R::property_rule
         : (_k == A::viewport)            ? R::viewport_rule
         :    R::at_rule;
}


// is_custom_property_name
//   function: true if `_name` begins with the CSS custom-
// property prefix (`--`).
D_CONSTEXPR_INLINE bool
is_custom_property_name(
    const char*     _name
)
{
    return ( (_name != nullptr)  &&
             (_name[0] == '-')   &&
             (_name[1] == '-') );
}


///////////////////////////////////////////////////////////////////////////////
///                XIII.   BACKEND TAG DISPATCH                             ///
///////////////////////////////////////////////////////////////////////////////

// css_backend_tag
//   struct: empty base tag for all CSS backend tag types.
struct css_backend_tag
{};

// css_default_backend_tag
//   struct: tag identifying the bundled in-memory backend.
struct css_default_backend_tag : css_backend_tag
{};

// css_libcss_backend_tag
//   struct: tag identifying a NetSurf libcss adapter backend.
struct css_libcss_backend_tag : css_backend_tag
{};

// css_katana_backend_tag
//   struct: tag identifying a katana-parser adapter backend.
struct css_katana_backend_tag : css_backend_tag
{};

// css_stylo_backend_tag
//   struct: tag identifying a Servo / stylo-based adapter
// backend.
struct css_stylo_backend_tag : css_backend_tag
{};

// css_postcss_backend_tag
//   struct: tag identifying a PostCSS-bridge backend (e.g.
// invoking a PostCSS pipeline through an FFI / IPC layer).
struct css_postcss_backend_tag : css_backend_tag
{};


///////////////////////////////////////////////////////////////////////////////
///                XIV.   BACKEND DETECTION                                 ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace css
NS_INTERNAL

    // has_css_backend_tag_helper
    //   trait: SFINAE helper detecting a nested
    // `css_backend_tag` alias. Primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_css_backend_tag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_css_backend_tag_helper (specialization)
    //   trait: success case.
    template<typename _Type>
    struct has_css_backend_tag_helper<
        _Type,
        void_t<typename _Type::css_backend_tag>
    >
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal
namespace css {


// is_css_backend
//   trait: true if `_Type` has a nested `css_backend_tag`
// type.
template<typename    _Type>
struct is_css_backend
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::internal::has_css_backend_tag_helper<
            clean_t<_Type>>::value;
};


// is_css_backend_v
//   constant: convenience accessor.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_STATIC_CONSTEXPR bool is_css_backend_v =
        is_css_backend<_Type>::value;
#endif


}   // namespace css
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                XV.   SUB-MODULE INCLUDES                                ///
///////////////////////////////////////////////////////////////////////////////

#include "./css_template_traits.hpp"
#include "./css_template.hpp"
#include "./css_template_concepts.hpp"


#endif  // DJINTERP_CSS_
