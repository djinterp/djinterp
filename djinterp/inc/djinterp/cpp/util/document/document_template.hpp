/*******************************************************************************
* djinterp [cpp]                                           document_template.hpp
*
*   The foundational TEMPLATING module: a declarative document skeleton with
* named slots, bound to data at render time and emitted through any
* DDocumentRenderer.
*
*   WHY IT IS NOT A PDF MODULE.  The old pdf_template conflated four jobs:
* flow layout, token substitution, a named style registry, and an element list.
* Layout now belongs to DPdfCanvas, which measures with real font metrics and
* owns the cursor -- so what remained in pdf_template was never PDF-specific at
* all.  Substitution, styles-by-name and an element list are DOCUMENT concerns.
* Lifting them here makes one template render as monospace, Markdown, XML, HTML
* and PDF, and leaves exactly one layout engine in the PDF layer instead of two.
*
*   WHAT A TEMPLATE IS.  A sequence of ELEMENTS.  Every text field is a
* DTextTemplate, so `{title}` and `{run.total}` interpolate from a binding
* environment at render time.  An element carries a STYLE NAME, which becomes a
* `style` hint -- so a dialect that has a style registry (PDF's canvas_style,
* HTML's class) resolves it and one that does not simply drops it.  That is the
* whole of how one skeleton serves five dialects.
*
*   WHAT MAKES IT A TEMPLATE RATHER THAN A FORM: the REPEAT element.  It binds a
* sequence by name and emits its children once per item, with each item's fields
* scoped under the loop variable -- so a table of N rows, a section per module,
* or a card per test are all one declaration rather than a hand-written loop.
* Without repetition a "template" is only a document with holes.
*
*   WHAT IT DOES NOT DO.  It does not lay out: no cursor, no pagination, no
* measurement.  It emits semantic calls and the renderer decides realisation.
* It does not own a style REGISTRY either -- it names styles; the renderer binds
* them.  Both were pdf_template's, and both were the reason it could serve only
* one back end.
*
*   PORTABILITY:
*   C++11 baseline (DDocumentRenderer's floor).  The binding environment is a
* std::function, matching the rest of the document stack.
*
*
* path:      /inc/djinterp/cpp/util/document/document_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.24
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    BINDINGS                    (DBindingEnv + map / chain helpers)
      ---------------------------------------------------------------

II.   DTemplateElement            (the element vocabulary)
      ----------------------------------------------------

III.  DDocumentTemplate           (the skeleton + its builders)
      ---------------------------------------------------------

IV.   render_template             (bind + emit through a renderer)
      ------------------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_DOCUMENT_TEMPLATE_HPP
#define DJINTERP_UTIL_DOCUMENT_DOCUMENT_TEMPLATE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"                    // NS_*, D_NODISCARD, D_NOEXCEPT
#include "../../text/text_template.hpp"          // DTextTemplate (the token layer)
#include "./templates/document_attributes.hpp"   // DDocAttributes, doc_attr_*
#include "./templates/document_renderer.hpp"     // DDocumentRenderer


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   BINDINGS                                            ///
///////////////////////////////////////////////////////////////////////////////

// DBindingEnv
//   type: name -> value.  A callable rather than a container so a caller may
// project straight out of a live model (a report, an option set, a row) without
// first flattening it into a map.  An unbound name yields the empty string,
// which is what DTextTemplate's own lookup contract expects, so a partially
// bound template renders rather than failing.
using DBindingEnv = std::function<std::string(const std::string&)>;


// map_bindings
//   function: a DBindingEnv over a flat map -- the simple case, and what a
// caller reaches for when the data is already assembled.
D_NODISCARD inline DBindingEnv map_bindings(
    std::map<std::string, std::string> _values
)
{
    const std::map<std::string, std::string> _owned = std::move(_values);

    return DBindingEnv(
        [_owned](const std::string& _key) -> std::string
        {
            std::map<std::string, std::string>::const_iterator _it =
                _owned.find(_key);

            // an unbound name interpolates as empty, never as an error
            if (_it == _owned.end())
            {
                return std::string();
            }

            return _it->second;
        });
}


// scoped_bindings
//   function: _inner shadowing _outer under the prefix _scope -- the mechanism
// a repeat block uses to expose an item's fields.  A lookup of
// "<scope>.<field>" is answered by _inner with "<field>"; everything else falls
// through to _outer.  Composable, so nested repeats nest their scopes.
D_NODISCARD inline DBindingEnv scoped_bindings(
    const std::string& _scope,
    DBindingEnv        _inner,
    DBindingEnv        _outer
)
{
    const std::string _prefix = _scope + ".";

    return DBindingEnv(
        [_prefix, _inner, _outer](const std::string& _key) -> std::string
        {
            // inside the scope: strip the prefix and ask the inner env
            if ( (_key.size() > _prefix.size()) &&
                 (_key.compare(0, _prefix.size(), _prefix) == 0) )
            {
                return _inner ? _inner(_key.substr(_prefix.size()))
                              : std::string();
            }

            return _outer ? _outer(_key) : std::string();
        });
}


// DSequenceEnv
//   type: a bound sequence -- how many items, and a DBindingEnv for the _i-th.
// This is the shape a repeat element consumes.  A caller supplies it directly
// from its own model; nothing is copied into the template.
struct DSequenceEnv
{
    // count
    //   how many items the sequence yields.
    std::function<std::size_t()> count;

    // at
    //   the bindings for item _i.  Called count() times, in order.
    std::function<DBindingEnv(std::size_t)> at;

    DSequenceEnv()
        : count(),
          at()
    {}

    DSequenceEnv(
        std::function<std::size_t()>            _count,
        std::function<DBindingEnv(std::size_t)> _at
    )
        : count(std::move(_count)),
          at(std::move(_at))
    {}
};


// DSequenceLookup
//   type: sequence NAME -> the bound sequence.  A template names the sequences
// it repeats over; the caller resolves them, exactly as DBindingEnv resolves
// scalars.
using DSequenceLookup = std::function<DSequenceEnv(const std::string&)>;


///////////////////////////////////////////////////////////////////////////////
///                II.  DTemplateElement                                    ///
///////////////////////////////////////////////////////////////////////////////

// DElementKind
//   enum: what an element emits.  Deliberately the DDocumentRenderer verb set
// plus `repeat` and `slot` -- a template can say anything a renderer can hear,
// and nothing it cannot.
enum class DElementKind
{
    heading,
    paragraph,
    key_value,
    rule,
    space,
    page_break,
    list,          // children are its items
    table,         // children supply the columns; `sequence` supplies the rows
    repeat,        // children, once per item of `sequence`
    slot           // a named hole, filled by a caller-supplied emitter
};


// DTemplateElement
//   struct: one element of a skeleton.  `text` / `secondary` are
// token-bearing sources (interpolated at render time); `style` names a style the
// renderer may resolve; `children` nest; `sequence` names the sequence a repeat
// or table iterates.  Fields not meaningful for a kind are simply unused --
// which keeps one flat type rather than a variant hierarchy over ten shapes.
struct DTemplateElement
{
    DElementKind             kind;

    // text / secondary
    //   token-bearing sources.  `secondary` is the value half of a key_value,
    // and unused elsewhere.
    std::string              text;
    std::string              secondary;

    // style
    //   the style NAME emitted as a `style` hint.  Empty means unstyled.
    std::string              style;

    // level
    //   heading depth; ignored by other kinds.
    std::size_t              level;

    // amount
    //   vertical space in points; ignored by other kinds.
    double                   amount;

    // ordered
    //   whether a list is ordered.
    bool                     ordered;

    // sequence
    //   the sequence a repeat iterates, or a table draws its rows from.
    std::string              sequence;

    // scope
    //   the loop variable a repeat binds its items under.  Empty defaults to
    // the sequence name.
    std::string              scope;

    // attrs
    //   literal hints merged into every emission of this element -- alignment,
    // colour, width.  Not token-bearing: hints are structure, not content.
    DDocAttributes           attrs;

    std::vector<DTemplateElement> children;

    DTemplateElement()
        : kind(DElementKind::paragraph),
          text(),
          secondary(),
          style(),
          level(std::size_t(1)),
          amount(6.0),
          ordered(false),
          sequence(),
          scope(),
          attrs(),
          children()
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                III. DDocumentTemplate                                   ///
///////////////////////////////////////////////////////////////////////////////

// DDocumentTemplate
//   class: the skeleton -- an element sequence plus the document's own frame
// hints.  Built once and rendered many times against different bindings, which
// is the whole point of holding it as data rather than as code.
class DDocumentTemplate
{
public:
    using element_list = std::vector<DTemplateElement>;

    DDocumentTemplate()
        : m_elements(),
          m_frame()
    {}

    // add
    //   append one element.
    DDocumentTemplate& add(
        const DTemplateElement& _element
    )
    {
        m_elements.push_back(_element);

        return (*this);
    }

    // frame
    //   the hints handed to begin_document -- a title, a class, a page size
    // name.  Token-bearing values are NOT interpolated here; the frame is
    // structure.
    D_NODISCARD DDocAttributes& frame() D_NOEXCEPT
    {
        return m_frame;
    }

    D_NODISCARD const DDocAttributes& frame() const D_NOEXCEPT
    {
        return m_frame;
    }

    D_NODISCARD const element_list& elements() const D_NOEXCEPT
    {
        return m_elements;
    }

    D_NODISCARD element_list& elements() D_NOEXCEPT
    {
        return m_elements;
    }

private:
    element_list   m_elements;
    DDocAttributes m_frame;
};


// ---------------------------------------------------------------------------
//  builders -- the declarative face
// ---------------------------------------------------------------------------

// tmpl_heading / tmpl_paragraph / tmpl_key_value / ...
//   functions: one per element kind, so a skeleton reads as a declaration
// rather than as struct assignment.  Every `_text` argument is token-bearing.
D_NODISCARD inline DTemplateElement tmpl_heading(
    std::size_t        _level,
    const std::string& _text,
    const std::string& _style = std::string()
)
{
    DTemplateElement _e;

    _e.kind  = DElementKind::heading;
    _e.level = _level;
    _e.text  = _text;
    _e.style = _style;

    return _e;
}

D_NODISCARD inline DTemplateElement tmpl_paragraph(
    const std::string& _text,
    const std::string& _style = std::string()
)
{
    DTemplateElement _e;

    _e.kind  = DElementKind::paragraph;
    _e.text  = _text;
    _e.style = _style;

    return _e;
}

D_NODISCARD inline DTemplateElement tmpl_key_value(
    const std::string& _key,
    const std::string& _value,
    const std::string& _style = std::string()
)
{
    DTemplateElement _e;

    _e.kind      = DElementKind::key_value;
    _e.text      = _key;
    _e.secondary = _value;
    _e.style     = _style;

    return _e;
}

D_NODISCARD inline DTemplateElement tmpl_rule()
{
    DTemplateElement _e;

    _e.kind = DElementKind::rule;

    return _e;
}

D_NODISCARD inline DTemplateElement tmpl_space(
    double _amount = 6.0
)
{
    DTemplateElement _e;

    _e.kind   = DElementKind::space;
    _e.amount = _amount;

    return _e;
}

D_NODISCARD inline DTemplateElement tmpl_page_break()
{
    DTemplateElement _e;

    _e.kind = DElementKind::page_break;

    return _e;
}

D_NODISCARD inline DTemplateElement tmpl_list(
    bool                                 _ordered,
    const std::vector<DTemplateElement>& _items
)
{
    DTemplateElement _e;

    _e.kind     = DElementKind::list;
    _e.ordered  = _ordered;
    _e.children = _items;

    return _e;
}

// tmpl_repeat
//   function: emit _children once per item of the sequence named _sequence,
// with each item's fields reachable as `<scope>.<field>`.  Scope defaults to
// the sequence name, so `repeat("modules", ...)` exposes `{modules.name}`.
D_NODISCARD inline DTemplateElement tmpl_repeat(
    const std::string&                   _sequence,
    const std::vector<DTemplateElement>& _children,
    const std::string&                   _scope = std::string()
)
{
    DTemplateElement _e;

    _e.kind     = DElementKind::repeat;
    _e.sequence = _sequence;
    _e.scope    = _scope.empty() ? _sequence : _scope;
    _e.children = _children;

    return _e;
}

// tmpl_table
//   function: a table whose COLUMNS are declared by _columns (each a
// token-bearing header, carrying its own align hint in `attrs`) and whose ROWS
// come from the sequence named _sequence.  Each row's cells are the column
// elements' `text` re-interpolated in that row's scope -- so a column is
// declared once and serves both the header and every cell.
D_NODISCARD inline DTemplateElement tmpl_table(
    const std::string&                   _sequence,
    const std::vector<DTemplateElement>& _columns,
    const std::string&                   _scope = std::string()
)
{
    DTemplateElement _e;

    _e.kind     = DElementKind::table;
    _e.sequence = _sequence;
    _e.scope    = _scope.empty() ? _sequence : _scope;
    _e.children = _columns;

    return _e;
}

// tmpl_column
//   function: one column of a tmpl_table -- `_header` is the header text and
// `_cell` the token-bearing cell source evaluated per row.
D_NODISCARD inline DTemplateElement tmpl_column(
    const std::string& _header,
    const std::string& _cell,
    DTextAlignment     _align = DTextAlignment::left,
    const std::string& _style = std::string()
)
{
    DTemplateElement _e;

    _e.kind      = DElementKind::paragraph;   // a column is not itself emitted
    _e.text      = _header;
    _e.secondary = _cell;
    _e.style     = _style;

    _e.attrs.set(doc_attr_align, align_to_string(_align));

    return _e;
}

// tmpl_slot
//   function: a named hole the CALLER fills -- for content a template cannot
// express (a chart, a bespoke table, a nested document).  The renderer sees
// whatever the slot emitter emits, so a template stays declarative without
// having to grow a case for every irregular block.
D_NODISCARD inline DTemplateElement tmpl_slot(
    const std::string& _name
)
{
    DTemplateElement _e;

    _e.kind = DElementKind::slot;
    _e.text = _name;

    return _e;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  render_template                                     ///
///////////////////////////////////////////////////////////////////////////////

// DSlotFiller
//   type: slot NAME -> what it emits.  Returns false when the name is unknown,
// which leaves a graceful hole rather than a failure.
using DSlotFiller =
    std::function<bool(const std::string&, DDocumentRenderer&)>;


// DTemplateContext
//   struct: everything a render needs beyond the skeleton itself -- the
// scalars, the sequences, and the slot fillers.  Held together so a nested
// render (a repeat body) can rebind only the scalars and inherit the rest.
struct DTemplateContext
{
    DBindingEnv     values;
    DSequenceLookup sequences;
    DSlotFiller     slots;

    DTemplateContext()
        : values(),
          sequences(),
          slots()
    {}
};


NS_INTERNAL

    // interpolate_helper
    //   helper: _source with its `{tokens}` resolved against _env.  A source
    // with no tokens still round-trips through DTextTemplate, which is cheap
    // and keeps one code path.
    D_NODISCARD inline std::string interpolate_helper(
        const std::string& _source,
        const DBindingEnv& _env
    )
    {
        // nothing to bind against: the source stands as literal text
        if (!_env)
        {
            return _source;
        }

        using template_type = DTextTemplate<char>;
        using view_type     = typename template_type::view_type;

        const template_type _tmpl(_source);

        // DTextTemplate hands the lookup a view, not a string: build the key
        // once per token rather than making the environment view-aware
        return _tmpl.render(
            [&_env](const view_type& _key) -> std::string
            {
                return _env(std::string(_key.data(), _key.size()));
            });
    }

    // element_attrs_helper
    //   helper: the element's literal hints plus its style name.
    D_NODISCARD inline DDocAttributes element_attrs_helper(
        const DTemplateElement& _element
    )
    {
        DDocAttributes _attrs = _element.attrs;

        // an unstyled element carries no style hint at all, rather than an
        // empty one a renderer would then try to resolve
        if (!_element.style.empty())
        {
            _attrs.set(doc_attr_style, _element.style);
        }

        return _attrs;
    }

NS_END  // internal


// render_elements
//   function: emit _elements through _renderer against _context.  Declared
// ahead of itself because repeat and list bodies recurse.
inline
void render_elements(
    const std::vector<DTemplateElement>& _elements,
    const DTemplateContext&              _context,
    DDocumentRenderer&                   _renderer
);


NS_INTERNAL

    // render_repeat_helper
    //   helper: emit a repeat's children once per item, each in a scoped
    // environment.  An unresolvable or empty sequence emits nothing, which is
    // the correct rendering of "no items" rather than an error.
    inline
    void render_repeat_helper(
        const DTemplateElement& _element,
        const DTemplateContext& _context,
        DDocumentRenderer&      _renderer
    )
    {
        if (!_context.sequences)
        {
            return;
        }

        const DSequenceEnv _sequence = _context.sequences(_element.sequence);

        if ( (!_sequence.count) ||
             (!_sequence.at) )
        {
            return;
        }

        const std::size_t _n = _sequence.count();

        for (std::size_t _i = 0; _i < _n; ++_i)
        {
            DTemplateContext _inner = _context;

            _inner.values = scoped_bindings(_element.scope,
                                            _sequence.at(_i),
                                            _context.values);

            render_elements(_element.children, _inner, _renderer);
        }

        return;
    }

    // render_table_helper
    //   helper: emit a table -- the columns as headers, then one row per item
    // of the sequence, each cell being that column's `secondary` source
    // interpolated in the row's scope.
    inline
    void render_table_helper(
        const DTemplateElement& _element,
        const DTemplateContext& _context,
        DDocumentRenderer&      _renderer
    )
    {
        _renderer.begin_table(_element.attrs);

        for (std::size_t _c = 0; _c < _element.children.size(); ++_c)
        {
            const DTemplateElement& _col = _element.children[_c];

            _renderer.table_column(
                internal::interpolate_helper(_col.text, _context.values),
                internal::element_attrs_helper(_col));
        }

        // rows come from the sequence; a template with no sequence is a
        // header-only table, which is a legitimate (if empty) document
        if (_context.sequences &&
            (!_element.sequence.empty()))
        {
            const DSequenceEnv _sequence =
                _context.sequences(_element.sequence);

            if (_sequence.count && _sequence.at)
            {
                const std::size_t _n = _sequence.count();

                for (std::size_t _i = 0; _i < _n; ++_i)
                {
                    const DBindingEnv _row =
                        scoped_bindings(_element.scope,
                                        _sequence.at(_i),
                                        _context.values);

                    _renderer.begin_row(DDocAttributes());

                    for (std::size_t _c = 0;
                         _c < _element.children.size();
                         ++_c)
                    {
                        const DTemplateElement& _col = _element.children[_c];

                        _renderer.cell(
                            internal::interpolate_helper(_col.secondary, _row),
                            internal::element_attrs_helper(_col));
                    }

                    _renderer.end_row();
                }
            }
        }

        _renderer.end_table();

        return;
    }

NS_END  // internal


// render_elements
//   function: the walk.  Each element interpolates its text sources against
// the context's scalars and emits the matching renderer verb; repeat, list and
// table recurse.
inline
void render_elements(
    const std::vector<DTemplateElement>& _elements,
    const DTemplateContext&              _context,
    DDocumentRenderer&                   _renderer
)
{
    for (std::size_t _i = 0; _i < _elements.size(); ++_i)
    {
        const DTemplateElement& _e     = _elements[_i];
        const DDocAttributes    _attrs =
            internal::element_attrs_helper(_e);

        switch (_e.kind)
        {
            case DElementKind::heading:
            {
                _renderer.heading(
                    _e.level,
                    internal::interpolate_helper(_e.text, _context.values),
                    _attrs);

                break;
            }

            case DElementKind::paragraph:
            {
                _renderer.paragraph(
                    internal::interpolate_helper(_e.text, _context.values),
                    _attrs);

                break;
            }

            case DElementKind::key_value:
            {
                _renderer.key_value(
                    internal::interpolate_helper(_e.text, _context.values),
                    internal::interpolate_helper(_e.secondary,
                                                 _context.values),
                    _attrs);

                break;
            }

            case DElementKind::rule:
            {
                _renderer.rule(_attrs);

                break;
            }

            case DElementKind::space:
            {
                _renderer.vertical_space(_e.amount, _attrs);

                break;
            }

            case DElementKind::page_break:
            {
                _renderer.page_break();

                break;
            }

            case DElementKind::list:
            {
                _renderer.begin_list(_e.ordered, _attrs);

                for (std::size_t _k = 0; _k < _e.children.size(); ++_k)
                {
                    _renderer.list_item(
                        internal::interpolate_helper(_e.children[_k].text,
                                                     _context.values),
                        internal::element_attrs_helper(_e.children[_k]));
                }

                _renderer.end_list();

                break;
            }

            case DElementKind::table:
            {
                internal::render_table_helper(_e, _context, _renderer);

                break;
            }

            case DElementKind::repeat:
            {
                internal::render_repeat_helper(_e, _context, _renderer);

                break;
            }

            case DElementKind::slot:
            {
                // an unfilled slot is a hole, not a failure
                if (_context.slots)
                {
                    (void)_context.slots(_e.text, _renderer);
                }

                break;
            }
        }
    }

    return;
}


// render_template
//   function: render a whole skeleton as a complete document -- the frame is
// opened and closed around it, so the result stands alone.
//
// Usage:
//   DHtmlDocumentRenderer _r;
//   DTemplateContext _ctx;
//   _ctx.values = map_bindings({{"title", "Nightly"}});
//   render_template(_skeleton, _ctx, _r);
inline
void render_template(
    const DDocumentTemplate& _template,
    const DTemplateContext&  _context,
    DDocumentRenderer&       _renderer
)
{
    _renderer.begin_document(_template.frame());

    render_elements(_template.elements(), _context, _renderer);

    _renderer.end_document();

    return;
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_DOCUMENT_TEMPLATE_HPP
