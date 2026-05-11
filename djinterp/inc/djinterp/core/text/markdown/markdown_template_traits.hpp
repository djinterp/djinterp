/******************************************************************************
* djinterp [markdown]                              markdown_template_traits.hpp
*
*   Structural SFINAE detection traits for Markdown block /
* inline / document / backend types. Mirrors the
* `xml_template_traits` and `html_template_traits` shape and idioms
* but targets the markdown bipartite content model: blocks vs
* inlines as distinct classifications, with shared accessors for
* text and children.
*
*   Both naming conventions are detected for every accessor:
* short-form (`text()`, `url()`, `level()`) AND getter-form
* (`get_text()`, `get_url()`, `get_level()`). This keeps backend
* adapters from being forced into one convention -- libcmark uses
* getter-form, md4c uses short-form, both classify automatically.
*
*   DETECTED PROTOCOLS:
*
*   markdown_block protocol:
*     A block exposes `block_kind()` (or `get_block_kind()`),
*     iterable children, and -- for inline-holder blocks -- a way
*     to access inline content.
*
*   markdown_inline protocol:
*     An inline exposes `inline_kind()` (or `get_inline_kind()`)
*     and, depending on kind, accessors for text / url / title /
*     alt-text / etc.
*
*   markdown_document protocol:
*     A document exposes `flavor()` plus a root-blocks accessor
*     and one or more `render_to_*` methods.
*
*   markdown_backend protocol:
*     A type tagged with `markdown_backend_tag` (detected by
*     `is_markdown_backend` in `markdown.hpp`) plus the nested
*     type aliases `block_type`, `inline_type`, `document_type`.
*
*
* path:      /inc/djinterp/core/util/markdown/markdown_template_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    KIND ACCESSOR DETECTION (BLOCK / INLINE)
II.   TEXT & RAW CONTENT ACCESSORS
III.  LINK / IMAGE ACCESSORS
IV.   CODE BLOCK ACCESSORS
V.    HEADING / LIST / TABLE ACCESSORS
VI.   CHILDREN / INLINES ACCESSORS
VII.  RENDER METHOD DETECTION
VIII. DOCUMENT FLAVOR DETECTION
IX.   COMPOSITE CLASSIFIERS
X.    CLASSIFICATION STRUCTS
XI.   BACKEND COMPLETENESS
XII.  VARIABLE TEMPLATES
*/

#ifndef DJINTERP_MARKDOWN_TEMPLATE_TRAITS_
#define DJINTERP_MARKDOWN_TEMPLATE_TRAITS_ 1

// std
#include <cstddef>
#include <ostream>
#include <string>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"


NS_DJINTERP

namespace markdown {


///////////////////////////////////////////////////////////////////////////////
///                I.   KIND ACCESSOR DETECTION (BLOCK / INLINE)            ///
///////////////////////////////////////////////////////////////////////////////

// has_block_kind_method
//   trait: true if `_Type` exposes `block_kind()` const.
template<typename _Type, typename = void>
struct has_block_kind_method : std::false_type
{};

template<typename _Type>
struct has_block_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().block_kind())
>> : std::true_type
{};


// has_get_block_kind_method
//   trait: true if `_Type` exposes `get_block_kind()` const.
template<typename _Type, typename = void>
struct has_get_block_kind_method : std::false_type
{};

template<typename _Type>
struct has_get_block_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_block_kind())
>> : std::true_type
{};


// has_block_kind_access
//   trait: true if either form is available.
template<typename _Type>
struct has_block_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_block_kind_method<_Type>::value ||
          has_get_block_kind_method<_Type>::value );
};


// has_inline_kind_method
//   trait: true if `_Type` exposes `inline_kind()` const.
template<typename _Type, typename = void>
struct has_inline_kind_method : std::false_type
{};

template<typename _Type>
struct has_inline_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().inline_kind())
>> : std::true_type
{};


// has_get_inline_kind_method
//   trait: true if `_Type` exposes `get_inline_kind()` const.
template<typename _Type, typename = void>
struct has_get_inline_kind_method : std::false_type
{};

template<typename _Type>
struct has_get_inline_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_inline_kind())
>> : std::true_type
{};


// has_inline_kind_access
//   trait: true if either form is available.
template<typename _Type>
struct has_inline_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_inline_kind_method<_Type>::value ||
          has_get_inline_kind_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                II.   TEXT & RAW CONTENT ACCESSORS                       ///
///////////////////////////////////////////////////////////////////////////////

// has_text_method
//   trait: true if `_Type` exposes `text()` const.
template<typename _Type, typename = void>
struct has_text_method : std::false_type
{};

template<typename _Type>
struct has_text_method<_Type, void_t<
    decltype(std::declval<const _Type&>().text())
>> : std::true_type
{};


// has_get_text_method
//   trait: true if `_Type` exposes `get_text()` const.
template<typename _Type, typename = void>
struct has_get_text_method : std::false_type
{};

template<typename _Type>
struct has_get_text_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_text())
>> : std::true_type
{};


// has_text_access
//   trait: true if either form is available.
template<typename _Type>
struct has_text_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_text_method<_Type>::value ||
          has_get_text_method<_Type>::value );
};


// has_set_text_method
//   trait: true if `_Type` exposes `set_text(string)`.
template<typename _Type, typename = void>
struct has_set_text_method : std::false_type
{};

template<typename _Type>
struct has_set_text_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_text(std::declval<const std::string&>()))
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                III.   LINK / IMAGE ACCESSORS                            ///
///////////////////////////////////////////////////////////////////////////////

// has_url_method / has_get_url_method / has_url_access
template<typename _Type, typename = void>
struct has_url_method : std::false_type {};
template<typename _Type>
struct has_url_method<_Type, void_t<
    decltype(std::declval<const _Type&>().url())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_url_method : std::false_type {};
template<typename _Type>
struct has_get_url_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_url())
>> : std::true_type {};

template<typename _Type>
struct has_url_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_url_method<_Type>::value || has_get_url_method<_Type>::value );
};


// has_title_method / has_get_title_method / has_title_access
template<typename _Type, typename = void>
struct has_title_method : std::false_type {};
template<typename _Type>
struct has_title_method<_Type, void_t<
    decltype(std::declval<const _Type&>().title())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_title_method : std::false_type {};
template<typename _Type>
struct has_get_title_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_title())
>> : std::true_type {};

template<typename _Type>
struct has_title_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_title_method<_Type>::value || has_get_title_method<_Type>::value );
};


// has_alt_text_method / has_get_alt_text_method / has_alt_text_access
template<typename _Type, typename = void>
struct has_alt_text_method : std::false_type {};
template<typename _Type>
struct has_alt_text_method<_Type, void_t<
    decltype(std::declval<const _Type&>().alt_text())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_alt_text_method : std::false_type {};
template<typename _Type>
struct has_get_alt_text_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_alt_text())
>> : std::true_type {};

template<typename _Type>
struct has_alt_text_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_alt_text_method<_Type>::value ||
          has_get_alt_text_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   CODE BLOCK ACCESSORS                               ///
///////////////////////////////////////////////////////////////////////////////

// has_language_method / has_get_language_method / has_language_access
template<typename _Type, typename = void>
struct has_language_method : std::false_type {};
template<typename _Type>
struct has_language_method<_Type, void_t<
    decltype(std::declval<const _Type&>().language())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_language_method : std::false_type {};
template<typename _Type>
struct has_get_language_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_language())
>> : std::true_type {};

template<typename _Type>
struct has_language_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_language_method<_Type>::value ||
          has_get_language_method<_Type>::value );
};


// has_info_string_method
//   trait: true if `_Type` exposes `info_string()` const --
// the full info string after the opening fence (which may
// contain language plus additional metadata).
template<typename _Type, typename = void>
struct has_info_string_method : std::false_type {};
template<typename _Type>
struct has_info_string_method<_Type, void_t<
    decltype(std::declval<const _Type&>().info_string())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                V.   HEADING / LIST / TABLE ACCESSORS                    ///
///////////////////////////////////////////////////////////////////////////////

// has_heading_level_method
//   trait: true if `_Type` exposes `heading_level()` const
// returning an integer 1..6.
template<typename _Type, typename = void>
struct has_heading_level_method : std::false_type {};
template<typename _Type>
struct has_heading_level_method<_Type, void_t<
    decltype(std::declval<const _Type&>().heading_level())
>> : std::true_type {};


// has_get_heading_level_method
template<typename _Type, typename = void>
struct has_get_heading_level_method : std::false_type {};
template<typename _Type>
struct has_get_heading_level_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_heading_level())
>> : std::true_type {};


// has_heading_level_access
template<typename _Type>
struct has_heading_level_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_heading_level_method<_Type>::value ||
          has_get_heading_level_method<_Type>::value );
};


// has_list_ordered_method
//   trait: true if `_Type` exposes `is_ordered()` const.
template<typename _Type, typename = void>
struct has_list_ordered_method : std::false_type {};
template<typename _Type>
struct has_list_ordered_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_ordered())
>> : std::true_type {};


// has_list_start_method
//   trait: true if `_Type` exposes `list_start()` const.
template<typename _Type, typename = void>
struct has_list_start_method : std::false_type {};
template<typename _Type>
struct has_list_start_method<_Type, void_t<
    decltype(std::declval<const _Type&>().list_start())
>> : std::true_type {};


// has_task_checked_method
//   trait: true if `_Type` exposes `is_checked()` const for
// task list items.
template<typename _Type, typename = void>
struct has_task_checked_method : std::false_type {};
template<typename _Type>
struct has_task_checked_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_checked())
>> : std::true_type {};


// has_table_alignment_method
//   trait: true if `_Type` exposes `column_alignments()` const.
template<typename _Type, typename = void>
struct has_table_alignment_method : std::false_type {};
template<typename _Type>
struct has_table_alignment_method<_Type, void_t<
    decltype(std::declval<const _Type&>().column_alignments())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VI.   CHILDREN / INLINES ACCESSORS                       ///
///////////////////////////////////////////////////////////////////////////////

// has_children_method
//   trait: true if `_Type` exposes `children()` const.
template<typename _Type, typename = void>
struct has_children_method : std::false_type {};
template<typename _Type>
struct has_children_method<_Type, void_t<
    decltype(std::declval<const _Type&>().children())
>> : std::true_type {};


// has_get_children_method
template<typename _Type, typename = void>
struct has_get_children_method : std::false_type {};
template<typename _Type>
struct has_get_children_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_children())
>> : std::true_type {};


// has_children_access
template<typename _Type>
struct has_children_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_children_method<_Type>::value ||
          has_get_children_method<_Type>::value );
};


// has_child_count_method
//   trait: true if `_Type` exposes `child_count()` const.
template<typename _Type, typename = void>
struct has_child_count_method : std::false_type {};
template<typename _Type>
struct has_child_count_method<_Type, void_t<
    decltype(std::declval<const _Type&>().child_count())
>> : std::true_type {};


// has_inlines_method
//   trait: true if `_Type` exposes `inlines()` const --
// inline-holder blocks (paragraphs, headings, table cells)
// expose this in addition to or instead of `children()`.
template<typename _Type, typename = void>
struct has_inlines_method : std::false_type {};
template<typename _Type>
struct has_inlines_method<_Type, void_t<
    decltype(std::declval<const _Type&>().inlines())
>> : std::true_type {};


// has_blocks_method
//   trait: true if `_Type` exposes `blocks()` const --
// block-holder containers (document, blockquote, list_item)
// expose this in addition to or instead of `children()`.
template<typename _Type, typename = void>
struct has_blocks_method : std::false_type {};
template<typename _Type>
struct has_blocks_method<_Type, void_t<
    decltype(std::declval<const _Type&>().blocks())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VII.   RENDER METHOD DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////

// has_render_to_markdown_method
//   trait: true if `_Type` exposes
// `render_to_markdown(std::ostream&)` const.
template<typename _Type, typename = void>
struct has_render_to_markdown_method : std::false_type {};
template<typename _Type>
struct has_render_to_markdown_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_markdown(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_render_to_html_method
template<typename _Type, typename = void>
struct has_render_to_html_method : std::false_type {};
template<typename _Type>
struct has_render_to_html_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_html(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_render_to_xml_method
template<typename _Type, typename = void>
struct has_render_to_xml_method : std::false_type {};
template<typename _Type>
struct has_render_to_xml_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_xml(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_render_to_plaintext_method
template<typename _Type, typename = void>
struct has_render_to_plaintext_method : std::false_type {};
template<typename _Type>
struct has_render_to_plaintext_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_plaintext(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_any_render_method
//   trait: true if `_Type` exposes at least one of the
// supported render targets.
template<typename _Type>
struct has_any_render_method
{
    D_STATIC_CONSTEXPR bool value = (
           has_render_to_markdown_method<_Type>::value
        || has_render_to_html_method<_Type>::value
        || has_render_to_xml_method<_Type>::value
        || has_render_to_plaintext_method<_Type>::value
    );
};


///////////////////////////////////////////////////////////////////////////////
///                VIII.   DOCUMENT FLAVOR DETECTION                        ///
///////////////////////////////////////////////////////////////////////////////

// has_flavor_method
//   trait: true if `_Type` exposes `flavor()` const returning
// a `markdown_flavor` enum value.
template<typename _Type, typename = void>
struct has_flavor_method : std::false_type {};
template<typename _Type>
struct has_flavor_method<_Type, void_t<
    decltype(std::declval<const _Type&>().flavor())
>> : std::true_type {};


// has_get_flavor_method
template<typename _Type, typename = void>
struct has_get_flavor_method : std::false_type {};
template<typename _Type>
struct has_get_flavor_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_flavor())
>> : std::true_type {};


// has_flavor_access
template<typename _Type>
struct has_flavor_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_flavor_method<_Type>::value ||
          has_get_flavor_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                IX.   COMPOSITE CLASSIFIERS                              ///
///////////////////////////////////////////////////////////////////////////////

// is_markdown_block
//   trait: true if `_Type` satisfies the markdown block
// protocol -- block kind accessor plus children access.
template<typename _Type>
struct is_markdown_block
{
    D_STATIC_CONSTEXPR bool value =
        ( has_block_kind_access<_Type>::value &&
          ( has_children_access<_Type>::value ||
            has_inlines_method<_Type>::value  ||
            has_blocks_method<_Type>::value   ||
            has_text_access<_Type>::value ) );
};


// is_markdown_block_loose
//   trait: looser variant -- block-kind accessor alone is
// enough to be considered a candidate.
template<typename _Type>
struct is_markdown_block_loose
{
    D_STATIC_CONSTEXPR bool value =
        has_block_kind_access<_Type>::value;
};


// is_markdown_inline
//   trait: true if `_Type` satisfies the markdown inline
// protocol -- inline kind accessor plus text-or-children.
template<typename _Type>
struct is_markdown_inline
{
    D_STATIC_CONSTEXPR bool value =
        ( has_inline_kind_access<_Type>::value &&
          ( has_text_access<_Type>::value     ||
            has_children_access<_Type>::value ||
            has_url_access<_Type>::value ) );
};


// is_markdown_inline_loose
//   trait: looser variant -- inline-kind accessor alone.
template<typename _Type>
struct is_markdown_inline_loose
{
    D_STATIC_CONSTEXPR bool value =
        has_inline_kind_access<_Type>::value;
};


// is_markdown_document
//   trait: true if `_Type` satisfies the markdown document
// protocol -- a flavor accessor and at least one render method.
template<typename _Type>
struct is_markdown_document
{
    D_STATIC_CONSTEXPR bool value =
        ( has_flavor_access<_Type>::value &&
          has_any_render_method<_Type>::value );
};


// is_markdown_document_loose
//   trait: looser variant -- flavor accessor OR any render
// method qualifies.
template<typename _Type>
struct is_markdown_document_loose
{
    D_STATIC_CONSTEXPR bool value =
        ( has_flavor_access<_Type>::value ||
          has_any_render_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                X.   CLASSIFICATION STRUCTS                              ///
///////////////////////////////////////////////////////////////////////////////

// markdown_block_class
//   struct: comprehensive classification of a block-shaped type.
template<typename _Type>
struct markdown_block_class
{
    D_STATIC_CONSTEXPR bool is_block            =
        is_markdown_block<_Type>::value;
    D_STATIC_CONSTEXPR bool has_kind            =
        has_block_kind_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_text            =
        has_text_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_children        =
        has_children_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_inlines         =
        has_inlines_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_blocks          =
        has_blocks_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_heading_level   =
        has_heading_level_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_language        =
        has_language_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_list_ordered    =
        has_list_ordered_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_list_start      =
        has_list_start_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_task_checked    =
        has_task_checked_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_table_alignment =
        has_table_alignment_method<_Type>::value;
};


// markdown_inline_class
//   struct: comprehensive classification of an inline-shaped
// type.
template<typename _Type>
struct markdown_inline_class
{
    D_STATIC_CONSTEXPR bool is_inline           =
        is_markdown_inline<_Type>::value;
    D_STATIC_CONSTEXPR bool has_kind            =
        has_inline_kind_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_text            =
        has_text_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_url             =
        has_url_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_title           =
        has_title_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_alt_text        =
        has_alt_text_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_children        =
        has_children_access<_Type>::value;
};


// markdown_document_class
//   struct: comprehensive classification of a document-shaped
// type.
template<typename _Type>
struct markdown_document_class
{
    D_STATIC_CONSTEXPR bool is_doc              =
        is_markdown_document<_Type>::value;
    D_STATIC_CONSTEXPR bool has_flavor          =
        has_flavor_access<_Type>::value;
    D_STATIC_CONSTEXPR bool renders_markdown    =
        has_render_to_markdown_method<_Type>::value;
    D_STATIC_CONSTEXPR bool renders_html        =
        has_render_to_html_method<_Type>::value;
    D_STATIC_CONSTEXPR bool renders_xml         =
        has_render_to_xml_method<_Type>::value;
    D_STATIC_CONSTEXPR bool renders_plaintext   =
        has_render_to_plaintext_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_blocks          =
        has_blocks_method<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                XI.   BACKEND COMPLETENESS                               ///
///////////////////////////////////////////////////////////////////////////////

// has_block_type_alias
//   trait: true if `_Type` exposes a nested `block_type`
// alias naming the concrete block storage class.
template<typename _Type, typename = void>
struct has_block_type_alias : std::false_type {};
template<typename _Type>
struct has_block_type_alias<_Type, void_t<
    typename _Type::block_type
>> : std::true_type {};


// has_inline_type_alias
//   trait: true if `_Type` exposes a nested `inline_type`
// alias.
template<typename _Type, typename = void>
struct has_inline_type_alias : std::false_type {};
template<typename _Type>
struct has_inline_type_alias<_Type, void_t<
    typename _Type::inline_type
>> : std::true_type {};


// has_document_type_alias
//   trait: true if `_Type` exposes a nested `document_type`
// alias.
template<typename _Type, typename = void>
struct has_document_type_alias : std::false_type {};
template<typename _Type>
struct has_document_type_alias<_Type, void_t<
    typename _Type::document_type
>> : std::true_type {};


// has_make_markdown_document_method
//   trait: true if `_Type` exposes a static factory
// `make_markdown_document()` returning a `document_type`.
template<typename _Type, typename = void>
struct has_make_markdown_document_method : std::false_type {};
template<typename _Type>
struct has_make_markdown_document_method<_Type, void_t<
    decltype(_Type::make_markdown_document())
>> : std::true_type {};


// is_markdown_backend_complete
//   trait: true if `_Type` exposes the full markdown backend
// protocol -- every nested type alias.
template<typename _Type>
struct is_markdown_backend_complete
{
    D_STATIC_CONSTEXPR bool value =
        ( has_block_type_alias<_Type>::value    &&
          has_inline_type_alias<_Type>::value   &&
          has_document_type_alias<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XII.   VARIABLE TEMPLATES                                ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_block_kind_access_v =
        has_block_kind_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_inline_kind_access_v =
        has_inline_kind_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_text_access_v =
        has_text_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_url_access_v =
        has_url_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_heading_level_access_v =
        has_heading_level_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_any_render_method_v =
        has_any_render_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_markdown_block_v =
        is_markdown_block<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_markdown_block_loose_v =
        is_markdown_block_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_markdown_inline_v =
        is_markdown_inline<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_markdown_inline_loose_v =
        is_markdown_inline_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_markdown_document_v =
        is_markdown_document<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_markdown_document_loose_v =
        is_markdown_document_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_markdown_backend_complete_v =
        is_markdown_backend_complete<_Type>::value;

#endif  // variable templates


}   // namespace markdown
NS_END  // djinterp


#endif  // DJINTERP_MARKDOWN_TEMPLATE_TRAITS_
