/******************************************************************************
* djinterp [text]                                              pdf_options.hpp
*
*   The configuration vocabulary for the PDF subsystem: one description of "how
* a PDF should look" - page geometry, default text presentation, and document
* metadata - that configures EITHER authoring surface (a pdf_template's flow
* layout or a pdf_document's drawing defaults) at EITHER binding time.
*
*   TWO FACES, ONE CONFIGURATION (the test_options pattern):
*     runtime      a concrete `pdf_options` aggregate of plain fields; build it,
*                  set what you care about, and apply_to(...) a target.  Every
*                  field is an ordinary value, so a parser or a config file can
*                  populate it at the boundary.
*     compile time a type-level `pdf_config<...>` schema authored in one
*                  statement from the option<> vocabulary (the SAME `option<>` /
*                  `val_t<>` / `fixed_string` surface the schema uses), lowered
*                  to a `pdf_options` by `pdf_options_lower<Schema>::build()`.
*                  Zero runtime parsing; the configuration IS a type.
*   Both produce a `pdf_options`; everything downstream takes only that, so the
* two faces never diverge.
*
*   WHAT IT CONFIGURES:
*     pdf_template   layout (page size + four margins) and the body text style
*                    (font, color, alignment, leading, cell indent) - the full
*                    presentation of the flow engine.
*     pdf_document   document metadata (title / author / subject / creator).
*                    A pdf_document draws at explicit points with per-call page
*                    sizes and text options rather than storing layout, so the
*                    page size and a ready-made pdf_text_options are EXPOSED
*                    (page(), to_text_options()) for the caller's add_page /
*                    text calls, while apply_to(pdf_document&) sets the one
*                    thing the façade does store - the Info metadata.
*
*   PRESETS vs VALUES:
*   Page size and text color are open value types (any custom extent / device
* color) at runtime, but the schema selects them from small preset enums
* (pdf_page_preset / pdf_color_preset) so they are clean non-type template
* arguments - exactly as a value enum stands in for a richer runtime type in
* test_options.  The presets lower through page_of_preset / color_of_preset,
* which the runtime face also exposes as convenience setters.
*
*   PORTABILITY:
*   The runtime core (PART A) and the application glue follow the PDF module's
* floor.  The schema and its lowering (PARTS B/C) ride the same C++20 /
* class-type-NTTP gate as the test_options vocabulary; below it the runtime
* `pdf_options` remains the portable path.
*
*
* TABLE OF CONTENTS
* =================
* PART A - RUNTIME CORE
*   A.I.    presets                  (pdf_page_preset / pdf_color_preset + maps)
*   A.II.   pdf_options              (the aggregate: fields, setters, views)
*   A.III.  application              (apply_to / make_template)
* PART B - COMPILE-TIME VOCABULARY (C++20)
*   B.I.    pdf_option               (the key enum)
*   B.II.   payload carriers         (choice / measure / count / text)
*   B.III.  node sugar               (page_, margins_, font_, align_, title_...)
*   B.IV.   pdf_config               (compose a whole schema in one statement)
* PART C - LOWERING BRIDGE (C++20)
*   C.I.    apply_pdf_option_helper  (per-key field setters)
*   C.II.   pdf_options_lower        (schema -> pdf_options)
*   C.III.  configure                (lower a schema straight onto a target)
*
*
* path:      /inc/djinterp/core/text/pdf/pdf_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.25
******************************************************************************/

#ifndef DJINTERP_TEXT_PDF_OPTIONS_
#define DJINTERP_TEXT_PDF_OPTIONS_ 1

// std
#include <cstddef>
#include <string>
#include <tuple>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/fixed_string.hpp"      // fixed_string<> (authoring NTTP)
#include "../../meta/carrier.hpp"           // val_t<>
#include "../../option/option.hpp"          // option<>
#include "../../option/option_set.hpp"      // option_set<> + flat_options_t
#include "../../option/option_override.hpp" // option_set_override_t
#include "../../option/option_compose.hpp"  // compose_options_t
#include "./pdf.hpp"                        // pdf_primitives + pdf_document façade
#include "./pdf_template.hpp"               // pdf_template + D_PDF_TPL_DEFAULT_*


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                        PART A - RUNTIME CORE                            ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////


// ===========================================================================
// A.I.  PRESETS
// ===========================================================================

// pdf_page_preset
//   enum: a named page extent for the schema and the convenience setters.  The
// runtime field is an open pdf_page_size (any custom extent); this names the
// standard presets so a page choice can be a non-type template argument.
enum class pdf_page_preset
{
    letter,
    legal,
    tabloid,
    a3,
    a4,
    a5
};

// pdf_color_preset
//   enum: a named device color, the schema/​setter counterpart of the open
// pdf_color runtime field.
enum class pdf_color_preset
{
    black,
    white,
    red,
    green,
    blue,
    gray
};

// page_of_preset
//   function: the pdf_page_size a pdf_page_preset names.
D_INLINE pdf_page_size
page_of_preset(
    pdf_page_preset _preset
)
{
    switch (_preset)
    {
        case pdf_page_preset::letter:  return pdf_page_size::letter();
        case pdf_page_preset::legal:   return pdf_page_size::legal();
        case pdf_page_preset::tabloid: return pdf_page_size::tabloid();
        case pdf_page_preset::a3:      return pdf_page_size::a3();
        case pdf_page_preset::a4:      return pdf_page_size::a4();
        case pdf_page_preset::a5:      return pdf_page_size::a5();
    }

    return pdf_page_size::letter();
}

// color_of_preset
//   function: the pdf_color a pdf_color_preset names.
D_INLINE pdf_color
color_of_preset(
    pdf_color_preset _preset
)
{
    switch (_preset)
    {
        case pdf_color_preset::black: return pdf_color::black();
        case pdf_color_preset::white: return pdf_color::white();
        case pdf_color_preset::red:   return pdf_color::red();
        case pdf_color_preset::green: return pdf_color::green();
        case pdf_color_preset::blue:  return pdf_color::blue();
        case pdf_color_preset::gray:  return pdf_color::gray();
    }

    return pdf_color::black();
}


// ===========================================================================
// A.II. pdf_options
// ===========================================================================

// pdf_options
//   struct: the whole PDF configuration as plain values - page geometry, the
// default text presentation (the template body style / the document text
// default), and document metadata.  Defaults match the pdf_template defaults
// exactly, so a default-constructed pdf_options applied to a fresh template is
// a no-op.  Fields are public (set them directly); the fluent setters and the
// `to_*` views are conveniences.  Strings left empty are "unset".
struct pdf_options
{
    // -- page geometry -------------------------------------------------------
    pdf_page_size  page;            // page extent (default letter)
    pdf_unit       margin_left;     // points (default D_PDF_TPL_DEFAULT_MARGIN)
    pdf_unit       margin_right;    // points
    pdf_unit       margin_top;      // points
    pdf_unit       margin_bottom;   // points

    // -- default text presentation -------------------------------------------
    pdf_base_font  font_family;     // base-14 face (default courier)
    pdf_unit       font_size;       // points (default D_PDF_TPL_DEFAULT_FONT_SIZE)
    pdf_color      text_color;      // device color (default black)
    pdf_text_align align;           // horizontal alignment (default left)
    pdf_unit       leading;         // line leading, points (default LEADING)
    std::size_t    indent_cells;    // left indent in character-cells (default 0)

    // -- document metadata (Info dictionary; empty = unset) ------------------
    std::string    title;
    std::string    author;
    std::string    subject;
    std::string    creator;

    pdf_options()
        : page(pdf_page_size::letter()),
          margin_left(D_PDF_TPL_DEFAULT_MARGIN),
          margin_right(D_PDF_TPL_DEFAULT_MARGIN),
          margin_top(D_PDF_TPL_DEFAULT_MARGIN),
          margin_bottom(D_PDF_TPL_DEFAULT_MARGIN),
          font_family(pdf_base_font::courier),
          font_size(D_PDF_TPL_DEFAULT_FONT_SIZE),
          text_color(pdf_color::black()),
          align(pdf_text_align::left),
          leading(D_PDF_TPL_DEFAULT_LEADING),
          indent_cells(0)
    {}

    // -- fluent setters (return *this so calls chain) ------------------------

    // set_page
    //   choose the page extent from a preset.
    pdf_options&
    set_page(
        pdf_page_preset _preset
    )
    {
        page = page_of_preset(_preset);

        return *this;
    }

    // set_margins
    //   set all four margins to one value.
    pdf_options&
    set_margins(
        pdf_unit _all
    )
    {
        margin_left   = _all;
        margin_right  = _all;
        margin_top    = _all;
        margin_bottom = _all;

        return *this;
    }

    // set_margins
    //   set the four margins individually.
    pdf_options&
    set_margins(
        pdf_unit _left,
        pdf_unit _right,
        pdf_unit _top,
        pdf_unit _bottom
    )
    {
        margin_left   = _left;
        margin_right  = _right;
        margin_top    = _top;
        margin_bottom = _bottom;

        return *this;
    }

    // set_font
    //   set the default face and size.
    pdf_options&
    set_font(
        pdf_base_font _family,
        pdf_unit      _size
    )
    {
        font_family = _family;
        font_size   = _size;

        return *this;
    }

    // set_color
    //   set the default text color from a preset.
    pdf_options&
    set_color(
        pdf_color_preset _preset
    )
    {
        text_color = color_of_preset(_preset);

        return *this;
    }

    // -- projections (the views the targets consume) -------------------------

    // font
    //   the default text run's pdf_font (family + size).
    D_NODISCARD pdf_font
    font() const D_NOEXCEPT
    {
        return pdf_font(font_family, font_size);
    }

    // to_layout
    //   the page geometry as a pdf_layout (page extent + four margins).
    D_NODISCARD pdf_layout
    to_layout() const
    {
        pdf_layout _layout(page);

        _layout.margin_left   = margin_left;
        _layout.margin_right  = margin_right;
        _layout.margin_top    = margin_top;
        _layout.margin_bottom = margin_bottom;

        return _layout;
    }

    // to_text_style
    //   the default presentation as a pdf_text_style (font, color, alignment,
    // leading, cell indent) - the template body style.
    D_NODISCARD pdf_text_style
    to_text_style() const
    {
        pdf_text_style _style(font());

        _style.color        = text_color;
        _style.align        = align;
        _style.leading      = leading;
        _style.indent_cells = indent_cells;

        return _style;
    }

    // to_text_options
    //   the default presentation as a foundation pdf_text_options (font, color,
    // alignment, leading) - the per-call text default for a pdf_document.  Cell
    // indent is a layout concept and is not carried here, matching
    // pdf_text_style::to_text_options.
    D_NODISCARD pdf_text_options
    to_text_options() const
    {
        pdf_text_options _opts(font());

        _opts.color   = text_color;
        _opts.align   = align;
        _opts.leading = leading;

        return _opts;
    }


    // ===================================================================
    // A.III. APPLICATION
    // ===================================================================

    // apply_to (pdf_template)
    //   configure a flow-layout template: install the page geometry and the
    // body text style.  Element-level styling and bindings remain the
    // caller's; this sets the document-wide presentation a fresh template
    // would otherwise leave at its defaults.
    void
    apply_to(
        pdf_template& _tpl
    ) const
    {
        _tpl.set_layout(to_layout());
        _tpl.set_body_style(to_text_style());

        return;
    }

    // apply_to (pdf_document)
    //   configure a document façade: record the metadata fields the caller has
    // set (a pdf_document stores no layout, so page size and text options are
    // taken per call - see page() / to_text_options()).  Setting metadata
    // opens the document lazily, exactly as pdf_document::metadata does.
    void
    apply_to(
        pdf_document& _doc
    ) const
    {
        if (!title.empty())   { _doc.metadata("Title",   title);   }
        if (!author.empty())  { _doc.metadata("Author",  author);  }
        if (!subject.empty()) { _doc.metadata("Subject", subject); }
        if (!creator.empty()) { _doc.metadata("Creator", creator); }

        return;
    }
};


// make_template
//   function: a pdf_template built and configured from a pdf_options in one
// call - constructed with the option set's layout and given its body style.
D_NODISCARD D_INLINE pdf_template
make_template(
    const pdf_options& _opts
)
{
    pdf_template _tpl(_opts.to_layout());

    _tpl.set_body_style(_opts.to_text_style());

    return _tpl;
}


///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                PART B - COMPILE-TIME VOCABULARY  (C++20)                ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////
//
//   The authoring surface as a type.  A `pdf_config<...>` is composed from the
// node sugar below - each alias is an `option<key, payload>` over the SAME
// option<> / val_t<> / fixed_string vocabulary the rest of the framework uses -
// and PART C lowers it to a runtime pdf_options.  Suppressed below C++20 /
// class-type non-type template arguments, where the runtime face is the path.

#if ( D_ENV_LANG_IS_CPP20_OR_HIGHER &&                                        \
      D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS )


// ===========================================================================
// B.I.  pdf_option
// ===========================================================================

// pdf_option
//   enum: the configuration keys.  Each names one aspect of a pdf_options and
// has exactly one node-sugar alias (B.III) and one lowering specialization
// (C.I), so the three stay in lockstep.
enum class pdf_option
{
    page,            // page extent (from a pdf_page_preset)
    margins,         // all four margins at once
    margin_left,     // one side ...
    margin_right,
    margin_top,
    margin_bottom,
    font,            // default face (a pdf_base_font)
    font_size,       // default size, points
    text_color,      // default color (from a pdf_color_preset)
    align,           // default alignment
    leading,         // default line leading, points
    indent,          // default left indent, character-cells
    title,           // metadata: title ...
    author,
    subject,
    creator
};


// ===========================================================================
// B.II. PAYLOAD CARRIERS
// ===========================================================================
//   Every payload is a `val_t<V>` (meta/carrier.hpp); these named aliases keep
// the node sugar readable while the stored type stays a plain value carrier, so
// the lowering matches `val_t<...>` directly.  Points are authored as whole
// integers here (the runtime pdf_options keeps full pdf_unit precision for
// callers that need fractional values).

// choice
//   alias: an enumerated-value payload (a preset, a face, an alignment).
template<auto _Value>
using choice = val_t<_Value>;

// measure
//   alias: a length in points, authored as a whole number.
template<std::size_t _Points>
using measure = val_t<_Points>;

// count
//   alias: a non-negative count (character-cells of indent).
template<std::size_t _N>
using count = val_t<_N>;

// text
//   alias: a string payload (a metadata field), carried as the authored
// fixed_string; the lowering reads its view().
template<fixed_string _Str>
using text = val_t<_Str>;


// ===========================================================================
// B.III. NODE SUGAR
// ===========================================================================
//   Intention-revealing one-option aliases.  Compose them into a pdf_config
// (B.IV); each lowers via the matching C.I specialization.

// page_
//   type: select the page extent from a preset.
template<pdf_page_preset _Preset>
using page_ = option<pdf_option::page, choice<_Preset>>;

// margins_
//   type: set all four margins to one point value.
template<std::size_t _Points>
using margins_ = option<pdf_option::margins, measure<_Points>>;

// margin_left_ / margin_right_ / margin_top_ / margin_bottom_
//   type: set one margin (in points), overriding a prior margins_.
template<std::size_t _Points>
using margin_left_ = option<pdf_option::margin_left, measure<_Points>>;

template<std::size_t _Points>
using margin_right_ = option<pdf_option::margin_right, measure<_Points>>;

template<std::size_t _Points>
using margin_top_ = option<pdf_option::margin_top, measure<_Points>>;

template<std::size_t _Points>
using margin_bottom_ = option<pdf_option::margin_bottom, measure<_Points>>;

// font_
//   type: set the default base-14 face.
template<pdf_base_font _Family>
using font_ = option<pdf_option::font, choice<_Family>>;

// font_size_
//   type: set the default font size, in points.
template<std::size_t _Points>
using font_size_ = option<pdf_option::font_size, measure<_Points>>;

// text_color_
//   type: set the default text color from a preset.
template<pdf_color_preset _Preset>
using text_color_ = option<pdf_option::text_color, choice<_Preset>>;

// align_
//   type: set the default horizontal alignment.
template<pdf_text_align _Align>
using align_ = option<pdf_option::align, choice<_Align>>;

// leading_
//   type: set the default line leading, in points.
template<std::size_t _Points>
using leading_ = option<pdf_option::leading, measure<_Points>>;

// indent_
//   type: set the default left indent, in character-cells.
template<std::size_t _Cells>
using indent_ = option<pdf_option::indent, count<_Cells>>;

// title_ / author_ / subject_ / creator_
//   type: set a document metadata field.
template<fixed_string _Str>
using title_ = option<pdf_option::title, text<_Str>>;

template<fixed_string _Str>
using author_ = option<pdf_option::author, text<_Str>>;

template<fixed_string _Str>
using subject_ = option<pdf_option::subject, text<_Str>>;

template<fixed_string _Str>
using creator_ = option<pdf_option::creator, text<_Str>>;


// ===========================================================================
// B.IV. pdf_config
// ===========================================================================

// pdf_config
//   type: a whole PDF configuration authored in ONE statement - the option_set
// produced by folding the given surfaces (compose_options_t).  Lower it to a
// runtime pdf_options with pdf_options_lower (PART C), or apply it straight to
// a target with configure<Schema>(target) (C.III).
//
// Usage:
//   using report_pdf = pdf_config<
//       page_<pdf_page_preset::a4>,
//       margins_<72>,
//       margin_bottom_<54>,
//       font_<pdf_base_font::helvetica>,
//       font_size_<11>,
//       leading_<14>,
//       align_<pdf_text_align::left>,
//       text_color_<pdf_color_preset::black>,
//       title_<"DTest report">,
//       author_<"djinterp">
//   >;
template<typename... _Surfaces>
using pdf_config = compose_options_t<_Surfaces...>;


///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                    PART C - LOWERING BRIDGE  (C++20)                    ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////
//
//   Distil a type-level pdf_config into a runtime pdf_options: per-key field
// setters and a small fold over the schema's flat option tuple.  build()
// seeds the pdf_options
// defaults and applies every option, so anything the schema omits keeps its
// default.


NS_INTERNAL

    // pdf_always_false
    //   helper: type-dependent false, so the primary's static_assert fires only
    // when an unrecognized option is actually instantiated.
    template<typename...>
    struct pdf_always_false : std::false_type
    {};


    // ===================================================================
    // C.I.  apply_pdf_option_helper
    // ===================================================================

    // apply_pdf_option_helper
    //   trait: set the field a single schema option names on a pdf_options.
    // The primary is a friendly hard error; one specialization per pdf_option
    // key follows.
    template<typename _Option>
    struct apply_pdf_option_helper
    {
        static void to(pdf_options&)
        {
            static_assert(pdf_always_false<_Option>::value,
                "pdf_options_lower: unrecognized pdf_option key in the schema. "
                "Every key needs an apply_pdf_option_helper specialization.");
        }
    };

    // -- page geometry -------------------------------------------------------

    template<pdf_page_preset _Preset>
    struct apply_pdf_option_helper<option<pdf_option::page, val_t<_Preset>>>
    {
        static void to(pdf_options& _o) { _o.page = page_of_preset(_Preset); }
    };

    template<std::size_t _Points>
    struct apply_pdf_option_helper<option<pdf_option::margins, val_t<_Points>>>
    {
        static void to(pdf_options& _o)
        {
            const pdf_unit _v = static_cast<pdf_unit>(_Points);

            _o.margin_left   = _v;
            _o.margin_right  = _v;
            _o.margin_top    = _v;
            _o.margin_bottom = _v;
        }
    };

    template<std::size_t _Points>
    struct apply_pdf_option_helper<
        option<pdf_option::margin_left, val_t<_Points>>>
    {
        static void to(pdf_options& _o)
        {
            _o.margin_left = static_cast<pdf_unit>(_Points);
        }
    };

    template<std::size_t _Points>
    struct apply_pdf_option_helper<
        option<pdf_option::margin_right, val_t<_Points>>>
    {
        static void to(pdf_options& _o)
        {
            _o.margin_right = static_cast<pdf_unit>(_Points);
        }
    };

    template<std::size_t _Points>
    struct apply_pdf_option_helper<
        option<pdf_option::margin_top, val_t<_Points>>>
    {
        static void to(pdf_options& _o)
        {
            _o.margin_top = static_cast<pdf_unit>(_Points);
        }
    };

    template<std::size_t _Points>
    struct apply_pdf_option_helper<
        option<pdf_option::margin_bottom, val_t<_Points>>>
    {
        static void to(pdf_options& _o)
        {
            _o.margin_bottom = static_cast<pdf_unit>(_Points);
        }
    };

    // -- text presentation ---------------------------------------------------

    template<pdf_base_font _Family>
    struct apply_pdf_option_helper<option<pdf_option::font, val_t<_Family>>>
    {
        static void to(pdf_options& _o) { _o.font_family = _Family; }
    };

    template<std::size_t _Points>
    struct apply_pdf_option_helper<option<pdf_option::font_size, val_t<_Points>>>
    {
        static void to(pdf_options& _o)
        {
            _o.font_size = static_cast<pdf_unit>(_Points);
        }
    };

    template<pdf_color_preset _Preset>
    struct apply_pdf_option_helper<
        option<pdf_option::text_color, val_t<_Preset>>>
    {
        static void to(pdf_options& _o) { _o.text_color = color_of_preset(_Preset); }
    };

    template<pdf_text_align _Align>
    struct apply_pdf_option_helper<option<pdf_option::align, val_t<_Align>>>
    {
        static void to(pdf_options& _o) { _o.align = _Align; }
    };

    template<std::size_t _Points>
    struct apply_pdf_option_helper<option<pdf_option::leading, val_t<_Points>>>
    {
        static void to(pdf_options& _o)
        {
            _o.leading = static_cast<pdf_unit>(_Points);
        }
    };

    template<std::size_t _Cells>
    struct apply_pdf_option_helper<option<pdf_option::indent, val_t<_Cells>>>
    {
        static void to(pdf_options& _o) { _o.indent_cells = _Cells; }
    };

    // -- metadata ------------------------------------------------------------

    template<fixed_string _Str>
    struct apply_pdf_option_helper<option<pdf_option::title, val_t<_Str>>>
    {
        static void to(pdf_options& _o) { _o.title = std::string(_Str.view()); }
    };

    template<fixed_string _Str>
    struct apply_pdf_option_helper<option<pdf_option::author, val_t<_Str>>>
    {
        static void to(pdf_options& _o) { _o.author = std::string(_Str.view()); }
    };

    template<fixed_string _Str>
    struct apply_pdf_option_helper<option<pdf_option::subject, val_t<_Str>>>
    {
        static void to(pdf_options& _o) { _o.subject = std::string(_Str.view()); }
    };

    template<fixed_string _Str>
    struct apply_pdf_option_helper<option<pdf_option::creator, val_t<_Str>>>
    {
        static void to(pdf_options& _o) { _o.creator = std::string(_Str.view()); }
    };


    // ===================================================================
    // C.II. lowering driver
    // ===================================================================

    // lower_pdf_schema_helper
    //   trait: fold every option in a schema's flat option tuple onto a
    // pdf_options.
    template<typename _Tuple>
    struct lower_pdf_schema_helper;

    template<typename... _Options>
    struct lower_pdf_schema_helper<std::tuple<_Options...>>
    {
        static void to(pdf_options& _o)
        {
            int _sink[] = { 0,
                ( apply_pdf_option_helper<_Options>::to(_o), 0 )... };
            (void) _sink;
        }
    };

NS_END  // internal


// pdf_options_lower
//   trait: distil a type-level configuration _Schema (an option_set, typically
// a pdf_config<...>) into a runtime pdf_options.  build() seeds the defaults
// and applies every option in the schema, so anything left unspecified keeps
// its default - the type-level counterpart of constructing a pdf_options and
// setting only the fields you care about.
//
// Usage:
//   static const pdf_options g_opts = pdf_options_lower<report_pdf>::build();
//   g_opts.apply_to(my_template);
template<typename _Schema>
struct pdf_options_lower
{
    D_NODISCARD static pdf_options
    build()
    {
        pdf_options _out;   // defaults
        internal::lower_pdf_schema_helper<
            typename _Schema::flat_options_t>::to(_out);

        return _out;
    }
};


// ===========================================================================
// C.III. configure
// ===========================================================================

// configure
//   function: lower a schema and apply it to a target in one call - the
// type-level counterpart of `opts.apply_to(target)`.  One overload per
// authoring surface; both build the same pdf_options first.
//
// Usage:
//   configure<report_pdf>(my_template);
//   configure<report_pdf>(my_document);
template<typename _Schema>
void
configure(
    pdf_template& _tpl
)
{
    pdf_options_lower<_Schema>::build().apply_to(_tpl);

    return;
}

template<typename _Schema>
void
configure(
    pdf_document& _doc
)
{
    pdf_options_lower<_Schema>::build().apply_to(_doc);

    return;
}


#endif  // C++20 && class-type NTTP


NS_END  // djinterp


#endif  // DJINTERP_TEXT_PDF_OPTIONS_
