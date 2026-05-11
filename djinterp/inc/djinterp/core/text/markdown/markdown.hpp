/******************************************************************************
* djinterp [markdown]                                             markdown.hpp
*
*   Foundational Markdown module for the djinterp framework. Models
* a CommonMark-aligned bipartite AST: documents contain *blocks*
* (paragraphs, headings, code blocks, lists, blockquotes, tables,
* etc.), which contain *inlines* (text, emphasis, links, images,
* code spans, etc.) and/or other blocks. The structure is
* deliberately distinct from `xml.hpp` / `html.hpp` -- markdown is
* not a tagged-element format, so inheriting from the XML facades
* would conflate two different content models.
*
*   ZERO OVERHEAD:
*   - Block kind and inline kind enums are `std::uint8_t`-backed.
*   - Category and classification predicates are `D_CONSTEXPR` and
*     fold to compile-time constants when the kind is known
*     statically.
*   - The bundled default backend stores blocks and inlines in a
*     single node type with optional fields; backends targeting
*     real parsers (cmark, md4c, maddy, hoedown, mmark) are free
*     to use distinct types.
*
*   LIBRARY AGNOSTICISM:
*   This header pulls in nothing beyond the standard library. The
* trait layer (`markdown_template_traits.hpp`) detects backend types
* structurally so any third-party AST can plug in.
*
*   FLAVOURS:
*   The runtime `markdown_flavor` enum identifies the document's
* target dialect (CommonMark, GitHub Flavored, GitLab, Pandoc,
* MultiMarkdown, kramdown, Markdown Extra). A full
* compile-time-policy `markdown_dialect<_Flag>` template, mirroring
* `html_dialect`, is a likely follow-on.
*
*
* path:      /inc/djinterp/core/util/markdown/markdown.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TYPES & CONSTANTS
II.   FLAVOUR & RENDER TARGET ENUMS
III.  BLOCK KIND ENUM
IV.   INLINE KIND ENUM
V.    BLOCK CATEGORY CLASSIFICATION
VI.   INLINE CATEGORY CLASSIFICATION
VII.  HEADING / LIST / CODE FENCE STYLE
VIII. BACKEND TAG DISPATCH
IX.   BACKEND DETECTION
X.    SUB-MODULE INCLUDES
*/

#ifndef DJINTERP_MARKDOWN_
#define DJINTERP_MARKDOWN_ 1

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

// NOTE: djinterp.hpp does not (yet) define NS_MARKDOWN. Using a
// plain namespace declaration here matching the html / font / color
// pattern; add `#define NS_MARKDOWN D_NAMESPACE(markdown)` alongside
// the other NS_* macros if the macro idiom is preferred.
namespace markdown {


// markdown_string_t
//   type: alias for the string type used throughout the
// markdown module. `std::string` matches the rest of the
// framework.
using markdown_string_t = std::string;


// markdown_size_t
//   type: alias for the size type used for counts and indices.
using markdown_size_t   = std::size_t;


// D_MARKDOWN_DEFAULT_INDENT
//   constant: default whitespace string used per indentation
// level when serialising markdown source.
#ifndef D_MARKDOWN_DEFAULT_INDENT
    #define D_MARKDOWN_DEFAULT_INDENT       "  "
#endif


// D_MARKDOWN_DEFAULT_BULLET
//   constant: default bullet character for unordered lists.
// CommonMark allows `-`, `+`, or `*`; `-` is the convention.
#ifndef D_MARKDOWN_DEFAULT_BULLET
    #define D_MARKDOWN_DEFAULT_BULLET       '-'
#endif


// D_MARKDOWN_DEFAULT_FENCE
//   constant: default fence character for fenced code blocks.
// CommonMark allows backtick or tilde.
#ifndef D_MARKDOWN_DEFAULT_FENCE
    #define D_MARKDOWN_DEFAULT_FENCE        '`'
#endif


// D_MARKDOWN_DEFAULT_FENCE_LENGTH
//   constant: minimum fence length for fenced code blocks.
#ifndef D_MARKDOWN_DEFAULT_FENCE_LENGTH
    #define D_MARKDOWN_DEFAULT_FENCE_LENGTH 3
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.   FLAVOUR & RENDER TARGET ENUMS                      ///
///////////////////////////////////////////////////////////////////////////////

// markdown_flavor
//   enum: identifies the document's target markdown dialect.
// Drives which extensions (tables, strikethrough, task lists,
// footnotes, math, etc.) are parsed and rendered.
enum class markdown_flavor : std::uint8_t
{
    commonmark,         // CommonMark 0.30 baseline
    github,             // GitHub Flavored Markdown (GFM)
    gitlab,             // GitLab Flavored Markdown (GLFM)
    pandoc,             // Pandoc-extended markdown
    multimarkdown,      // MultiMarkdown (MMD)
    kramdown,           // kramdown (Jekyll default)
    markdown_extra,     // PHP Markdown Extra
    djot,               // Djot (CommonMark successor proposal)
    custom,             // user-defined extension set
    unspecified
};


// markdown_render_target
//   enum: identifies an output format for document rendering.
// Documents expose `render_to_*` methods per target; backends
// implement whichever they support.
enum class markdown_render_target : std::uint8_t
{
    markdown,           // round-trip to markdown source
    html,               // HTML4 / HTML5 fragment
    xhtml,              // XHTML fragment
    xml,                // CommonMark XML AST format
    plaintext,          // strip all formatting
    terminal_ansi,      // ANSI-coloured terminal output
    latex,              // LaTeX source
    json,               // structured AST as JSON
    unknown
};


///////////////////////////////////////////////////////////////////////////////
///                III.   BLOCK KIND ENUM                                   ///
///////////////////////////////////////////////////////////////////////////////

// markdown_block_kind
//   enum: discriminator for every standard markdown block-level
// element. Backed by `std::uint8_t`. The `unknown` value is
// used for elements not in this list (custom directives,
// flavour-specific extensions not yet modelled).
enum class markdown_block_kind : std::uint8_t
{
    // document root
    document,

    // headings
    heading_1, heading_2, heading_3, heading_4, heading_5, heading_6,

    // paragraphs and breaks
    paragraph,
    thematic_break,             // ---, ***, ___

    // code
    indented_code_block,        // 4-space indented
    fenced_code_block,          // ``` or ~~~

    // quoted / containers
    block_quote,

    // lists
    ordered_list,
    unordered_list,
    list_item,
    task_list_item,             // GFM extension

    // tables (GFM / Pandoc / MMD)
    table,
    table_header,
    table_body,
    table_row,
    table_cell,

    // raw passthrough
    html_block,
    raw_block,                  // generic raw block (Pandoc)

    // references
    link_reference_definition,
    footnote_definition,        // Pandoc / GFM

    // definitions (Pandoc / Markdown Extra)
    definition_list,
    definition_term,
    definition_description,

    // math (Pandoc / KaTeX-flavoured)
    math_block,                 // $$...$$ or ```math

    // metadata
    yaml_front_matter,          // --- ... --- at top of doc
    toml_front_matter,          // +++ ... +++ at top of doc

    // sentinel
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   INLINE KIND ENUM                                   ///
///////////////////////////////////////////////////////////////////////////////

// markdown_inline_kind
//   enum: discriminator for every standard markdown
// inline-level element. Backed by `std::uint8_t`.
enum class markdown_inline_kind : std::uint8_t
{
    // text
    text,                       // plain text run
    soft_break,                 // newline rendered as space
    hard_break,                 // two spaces + newline / `\` + newline

    // emphasis
    emphasis,                   // *text* or _text_
    strong,                     // **text** or __text__
    strikethrough,              // ~~text~~ (GFM)
    highlight,                  // ==text== (Pandoc)
    subscript,                  // ~text~ (Pandoc)
    superscript,                // ^text^ (Pandoc)
    underline,                  // some flavours

    // code
    code_span,                  // `code`

    // links / refs
    link,                       // [text](url) or reference
    image,                      // ![alt](url)
    autolink,                   // <https://example.com>
    autolink_email,             // <user@example.com>
    footnote_reference,         // [^name]
    citation,                   // [@key] (Pandoc)

    // raw / passthrough
    html_inline,                // raw inline HTML
    entity,                     // &amp; &#x20; etc.
    hard_line_break,            // explicit line break

    // math
    math_inline,                // $...$

    // mentions / shortcodes (GitHub / Discord / Slack)
    mention_user,               // @user
    mention_team,               // @team
    issue_reference,            // #123
    emoji_shortcode,            // :smile:

    // sentinel
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                V.   BLOCK CATEGORY CLASSIFICATION                       ///
///////////////////////////////////////////////////////////////////////////////

// markdown_block_category
//   enum: bitmask describing the categorical properties of a
// block. Backed by `unsigned`.
enum markdown_block_category : unsigned
{
    mbc_none           = 0u,
    mbc_leaf           = 1u <<  0,  // contains only inlines / text
    mbc_container      = 1u <<  1,  // contains other blocks
    mbc_heading        = 1u <<  2,
    mbc_code           = 1u <<  3,  // raw text content
    mbc_list           = 1u <<  4,
    mbc_list_item      = 1u <<  5,
    mbc_table          = 1u <<  6,
    mbc_quote          = 1u <<  7,
    mbc_definition     = 1u <<  8,
    mbc_metadata       = 1u <<  9,  // front-matter etc.
    mbc_math           = 1u << 10,
    mbc_raw            = 1u << 11,  // html_block, raw_block
    mbc_inline_holder  = 1u << 12,  // body is inlines, not blocks
    mbc_block_holder   = 1u << 13   // body is blocks, not inlines
};


// markdown_category_for_block_kind
//   function: returns the category bitmask for a block kind.
D_CONSTEXPR_INLINE unsigned
markdown_category_for_block_kind(
    markdown_block_kind     _k
)
{
    using K = markdown_block_kind;
    return (_k == K::document)        ? (mbc_container | mbc_block_holder)

         : ( (_k == K::heading_1) || (_k == K::heading_2) ||
             (_k == K::heading_3) || (_k == K::heading_4) ||
             (_k == K::heading_5) || (_k == K::heading_6) )
                                      ? (mbc_leaf | mbc_heading
                                         | mbc_inline_holder)

         : (_k == K::paragraph)       ? (mbc_leaf | mbc_inline_holder)
         : (_k == K::thematic_break)  ? (mbc_leaf)

         : (_k == K::indented_code_block) ? (mbc_leaf | mbc_code)
         : (_k == K::fenced_code_block)   ? (mbc_leaf | mbc_code)

         : (_k == K::block_quote)     ? (mbc_container | mbc_quote
                                         | mbc_block_holder)

         : (_k == K::ordered_list)    ? (mbc_container | mbc_list
                                         | mbc_block_holder)
         : (_k == K::unordered_list)  ? (mbc_container | mbc_list
                                         | mbc_block_holder)
         : (_k == K::list_item)       ? (mbc_container | mbc_list_item
                                         | mbc_block_holder)
         : (_k == K::task_list_item)  ? (mbc_container | mbc_list_item
                                         | mbc_block_holder)

         : (_k == K::table)           ? (mbc_container | mbc_table
                                         | mbc_block_holder)
         : (_k == K::table_header)    ? (mbc_container | mbc_table
                                         | mbc_block_holder)
         : (_k == K::table_body)      ? (mbc_container | mbc_table
                                         | mbc_block_holder)
         : (_k == K::table_row)       ? (mbc_container | mbc_table
                                         | mbc_block_holder)
         : (_k == K::table_cell)      ? (mbc_leaf | mbc_table
                                         | mbc_inline_holder)

         : (_k == K::html_block)      ? (mbc_leaf | mbc_raw)
         : (_k == K::raw_block)       ? (mbc_leaf | mbc_raw)

         : (_k == K::link_reference_definition) ? (mbc_leaf)
         : (_k == K::footnote_definition)       ? (mbc_container
                                                   | mbc_block_holder)

         : (_k == K::definition_list)        ? (mbc_container | mbc_definition
                                                | mbc_block_holder)
         : (_k == K::definition_term)        ? (mbc_leaf | mbc_definition
                                                | mbc_inline_holder)
         : (_k == K::definition_description) ? (mbc_container | mbc_definition
                                                | mbc_block_holder)

         : (_k == K::math_block)             ? (mbc_leaf | mbc_math
                                                | mbc_code)

         : (_k == K::yaml_front_matter)      ? (mbc_leaf | mbc_metadata
                                                | mbc_raw)
         : (_k == K::toml_front_matter)      ? (mbc_leaf | mbc_metadata
                                                | mbc_raw)

         : mbc_none;
}


// is_container_block_kind
//   function: true if the block kind contains other blocks.
D_CONSTEXPR_INLINE bool
is_container_block_kind(
    markdown_block_kind     _k
)
{
    return ((markdown_category_for_block_kind(_k) & mbc_container) != 0u);
}


// is_leaf_block_kind
//   function: true if the block kind is a leaf (no nested
// blocks; may contain inlines).
D_CONSTEXPR_INLINE bool
is_leaf_block_kind(
    markdown_block_kind     _k
)
{
    return ((markdown_category_for_block_kind(_k) & mbc_leaf) != 0u);
}


// is_heading_block_kind
//   function: true for heading_1 through heading_6.
D_CONSTEXPR_INLINE bool
is_heading_block_kind(
    markdown_block_kind     _k
)
{
    return ((markdown_category_for_block_kind(_k) & mbc_heading) != 0u);
}


// is_code_block_kind
//   function: true for indented and fenced code blocks plus
// math blocks (which use code-style raw text).
D_CONSTEXPR_INLINE bool
is_code_block_kind(
    markdown_block_kind     _k
)
{
    return ((markdown_category_for_block_kind(_k) & mbc_code) != 0u);
}


// is_list_block_kind
//   function: true for ordered_list and unordered_list (NOT
// list items; use is_list_item_block_kind for those).
D_CONSTEXPR_INLINE bool
is_list_block_kind(
    markdown_block_kind     _k
)
{
    return ((markdown_category_for_block_kind(_k) & mbc_list) != 0u);
}


// is_list_item_block_kind
//   function: true for list_item and task_list_item.
D_CONSTEXPR_INLINE bool
is_list_item_block_kind(
    markdown_block_kind     _k
)
{
    return ((markdown_category_for_block_kind(_k) & mbc_list_item) != 0u);
}


// is_table_block_kind
//   function: true for any table-model block.
D_CONSTEXPR_INLINE bool
is_table_block_kind(
    markdown_block_kind     _k
)
{
    return ((markdown_category_for_block_kind(_k) & mbc_table) != 0u);
}


// heading_level_from_kind
//   function: returns 1..6 for heading_1..heading_6, 0 for
// non-heading kinds.
D_CONSTEXPR_INLINE int
heading_level_from_kind(
    markdown_block_kind     _k
)
{
    using K = markdown_block_kind;
    return (_k == K::heading_1) ? 1
         : (_k == K::heading_2) ? 2
         : (_k == K::heading_3) ? 3
         : (_k == K::heading_4) ? 4
         : (_k == K::heading_5) ? 5
         : (_k == K::heading_6) ? 6
         :                        0;
}


// heading_kind_from_level
//   function: inverse of `heading_level_from_kind`. Levels
// outside [1,6] return `unknown`.
D_CONSTEXPR_INLINE markdown_block_kind
heading_kind_from_level(
    int     _level
)
{
    using K = markdown_block_kind;
    return (_level == 1) ? K::heading_1
         : (_level == 2) ? K::heading_2
         : (_level == 3) ? K::heading_3
         : (_level == 4) ? K::heading_4
         : (_level == 5) ? K::heading_5
         : (_level == 6) ? K::heading_6
         :                 K::unknown;
}


///////////////////////////////////////////////////////////////////////////////
///                VI.   INLINE CATEGORY CLASSIFICATION                     ///
///////////////////////////////////////////////////////////////////////////////

// markdown_inline_category
//   enum: bitmask describing the categorical properties of an
// inline. Backed by `unsigned`.
enum markdown_inline_category : unsigned
{
    mic_none           = 0u,
    mic_atomic         = 1u <<  0,  // no nested inlines
    mic_container      = 1u <<  1,  // wraps other inlines
    mic_emphasis       = 1u <<  2,
    mic_strong         = 1u <<  3,
    mic_link           = 1u <<  4,
    mic_image          = 1u <<  5,
    mic_code           = 1u <<  6,
    mic_break          = 1u <<  7,
    mic_raw            = 1u <<  8,
    mic_reference      = 1u <<  9,  // footnote / citation reference
    mic_math           = 1u << 10,
    mic_mention        = 1u << 11   // @user, #123, :emoji:
};


// markdown_category_for_inline_kind
//   function: returns the category bitmask for an inline kind.
D_CONSTEXPR_INLINE unsigned
markdown_category_for_inline_kind(
    markdown_inline_kind    _k
)
{
    using K = markdown_inline_kind;
    return (_k == K::text)               ? (mic_atomic)
         : (_k == K::soft_break)         ? (mic_atomic | mic_break)
         : (_k == K::hard_break)         ? (mic_atomic | mic_break)
         : (_k == K::hard_line_break)    ? (mic_atomic | mic_break)

         : (_k == K::emphasis)           ? (mic_container | mic_emphasis)
         : (_k == K::strong)             ? (mic_container | mic_strong)
         : (_k == K::strikethrough)      ? (mic_container | mic_emphasis)
         : (_k == K::highlight)          ? (mic_container | mic_emphasis)
         : (_k == K::subscript)          ? (mic_container | mic_emphasis)
         : (_k == K::superscript)        ? (mic_container | mic_emphasis)
         : (_k == K::underline)          ? (mic_container | mic_emphasis)

         : (_k == K::code_span)          ? (mic_atomic | mic_code)

         : (_k == K::link)               ? (mic_container | mic_link)
         : (_k == K::image)              ? (mic_atomic | mic_image)
         : (_k == K::autolink)           ? (mic_atomic | mic_link)
         : (_k == K::autolink_email)     ? (mic_atomic | mic_link)
         : (_k == K::footnote_reference) ? (mic_atomic | mic_reference)
         : (_k == K::citation)           ? (mic_atomic | mic_reference)

         : (_k == K::html_inline)        ? (mic_atomic | mic_raw)
         : (_k == K::entity)             ? (mic_atomic | mic_raw)

         : (_k == K::math_inline)        ? (mic_atomic | mic_math)

         : (_k == K::mention_user)       ? (mic_atomic | mic_mention)
         : (_k == K::mention_team)       ? (mic_atomic | mic_mention)
         : (_k == K::issue_reference)    ? (mic_atomic | mic_mention)
         : (_k == K::emoji_shortcode)    ? (mic_atomic | mic_mention)

         :    mic_none;
}


// is_atomic_inline_kind
//   function: true if the inline holds no nested inline children.
D_CONSTEXPR_INLINE bool
is_atomic_inline_kind(
    markdown_inline_kind    _k
)
{
    return ((markdown_category_for_inline_kind(_k) & mic_atomic) != 0u);
}


// is_container_inline_kind
//   function: true if the inline wraps other inlines.
D_CONSTEXPR_INLINE bool
is_container_inline_kind(
    markdown_inline_kind    _k
)
{
    return ((markdown_category_for_inline_kind(_k) & mic_container) != 0u);
}


// is_emphasis_inline_kind
//   function: true for any emphasis-like inline (em / strong /
// strikethrough / highlight / sub / sup / underline).
D_CONSTEXPR_INLINE bool
is_emphasis_inline_kind(
    markdown_inline_kind    _k
)
{
    return ( (markdown_category_for_inline_kind(_k)
              & (mic_emphasis | mic_strong)) != 0u );
}


// is_link_inline_kind
//   function: true for link / autolink / autolink_email.
D_CONSTEXPR_INLINE bool
is_link_inline_kind(
    markdown_inline_kind    _k
)
{
    return ((markdown_category_for_inline_kind(_k) & mic_link) != 0u);
}


// is_image_inline_kind
//   function: true for image only.
D_CONSTEXPR_INLINE bool
is_image_inline_kind(
    markdown_inline_kind    _k
)
{
    return ((markdown_category_for_inline_kind(_k) & mic_image) != 0u);
}


// is_break_inline_kind
//   function: true for soft_break, hard_break, hard_line_break.
D_CONSTEXPR_INLINE bool
is_break_inline_kind(
    markdown_inline_kind    _k
)
{
    return ((markdown_category_for_inline_kind(_k) & mic_break) != 0u);
}


///////////////////////////////////////////////////////////////////////////////
///                VII.   HEADING / LIST / CODE FENCE STYLE                 ///
///////////////////////////////////////////////////////////////////////////////

// markdown_heading_style
//   enum: ATX (`# Heading`) vs Setext (`Heading\n=======`).
enum class markdown_heading_style : std::uint8_t
{
    atx,            // # H1, ## H2, ...
    atx_closed,     // # H1 #, ## H2 ##, ...
    setext,         // H1\n=== or H2\n---
    auto_choose     // ATX for h3-h6, setext for h1-h2
};


// markdown_list_style
//   enum: which bullet character / numeral style to use.
enum class markdown_list_style : std::uint8_t
{
    dash,           // -
    plus,           // +
    asterisk,       // *
    period_number,  // 1.
    paren_number    // 1)
};


// markdown_code_fence_style
//   enum: backtick or tilde fences for fenced code blocks.
enum class markdown_code_fence_style : std::uint8_t
{
    backtick,       // ```
    tilde,          // ~~~
    indented        // 4-space indented (no fence)
};


// markdown_table_alignment
//   enum: per-column alignment for table cells.
enum class markdown_table_alignment : std::uint8_t
{
    none,
    left,           // :---
    center,         // :---:
    right           // ---:
};


///////////////////////////////////////////////////////////////////////////////
///                VIII.   BACKEND TAG DISPATCH                             ///
///////////////////////////////////////////////////////////////////////////////

// markdown_backend_tag
//   struct: empty base tag for all markdown backend tag types.
struct markdown_backend_tag
{};

// markdown_default_backend_tag
//   struct: tag identifying the bundled in-memory backend.
struct markdown_default_backend_tag : markdown_backend_tag
{};

// markdown_cmark_backend_tag
//   struct: tag identifying a libcmark-based adapter backend.
struct markdown_cmark_backend_tag : markdown_backend_tag
{};

// markdown_md4c_backend_tag
//   struct: tag identifying an md4c-based adapter backend.
struct markdown_md4c_backend_tag : markdown_backend_tag
{};

// markdown_maddy_backend_tag
//   struct: tag identifying a maddy-based adapter backend.
struct markdown_maddy_backend_tag : markdown_backend_tag
{};

// markdown_hoedown_backend_tag
//   struct: tag identifying a hoedown-based adapter backend.
struct markdown_hoedown_backend_tag : markdown_backend_tag
{};

// markdown_mmark_backend_tag
//   struct: tag identifying a mmark / cmark-gfm-based adapter
// backend.
struct markdown_mmark_backend_tag : markdown_backend_tag
{};


///////////////////////////////////////////////////////////////////////////////
///                IX.   BACKEND DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace markdown
NS_INTERNAL

    // has_markdown_backend_tag_helper
    //   trait: SFINAE helper detecting a nested
    // `markdown_backend_tag` alias. Primary template (failure).
    template<typename _Type,
             typename = void>
    struct has_markdown_backend_tag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_markdown_backend_tag_helper (specialization)
    //   trait: success case.
    template<typename _Type>
    struct has_markdown_backend_tag_helper<
        _Type,
        void_t<typename _Type::markdown_backend_tag>
    >
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal
namespace markdown {


// is_markdown_backend
//   trait: true if `_Type` has a nested
// `markdown_backend_tag` type.
template<typename    _Type>
struct is_markdown_backend
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::internal::has_markdown_backend_tag_helper<
            clean_t<_Type>>::value;
};


// is_markdown_backend_v
//   constant: convenience accessor.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_STATIC_CONSTEXPR bool is_markdown_backend_v =
        is_markdown_backend<_Type>::value;
#endif


}   // namespace markdown
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                X.   SUB-MODULE INCLUDES                                 ///
///////////////////////////////////////////////////////////////////////////////

#include "./markdown_template_traits.hpp"
#include "./markdown_template.hpp"
#include "./markdown_template_concepts.hpp"


#endif  // DJINTERP_MARKDOWN_
