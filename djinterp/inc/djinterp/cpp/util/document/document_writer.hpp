/*******************************************************************************
* djinterp [cpp]                                             document_writer.hpp
*
*   Builds a document tree incrementally through stable, position-holding
* cursors. The tree is stored in a flat, append-only arena
* (`std::vector<DDocNode>`); a cursor is `{arena, dj_index}` -- a comonadic
* focus into that arena. Because the arena only ever grows, an index never
* invalidates: appending elsewhere may reallocate the vector's memory, but
* a node at index `i` is still at index `i`, so a cursor stays anchored to
* its node for the writer's whole life. That is what lets several cursors
* be held at once and written to in any interleaving -- the canonical
* "X inline AND in a List-of-X section" case.
*
*   DOCUMENT ORDER vs WRITE ORDER:
*   A node's place in the output is fixed by the TREE, not by when it was
* written. Build the skeleton once, capture a cursor per region, then
* stream into each region in whatever order data arrives; the single
* serialisation fold (`DDocumentPrinter` over `document()`) emits each
* region in its structural slot. Write order and document order are fully
* decoupled.
*
*   PROTOCOL BRIDGE:
*   `document()` returns a `DDocView` -- a lightweight, read-only view over
* the arena that satisfies the XML node protocol (name / kind / text /
* attributes / children). So the generic `DDocumentPrinter` (and any
* protocol-generic consumer) serialises an arena-built document through the
* exact same code path as a pugixml or libxml++ document. The writer OWNS
* building; the node protocol is the meeting point with reading/printing.
*
*   This header depends only on `xml.hpp` (for `DXmlString` /
* `DXmlNodeKind`); it does NOT depend on the printer -- serialisation is a
* separate concern applied to `document()`.
*
*   Requires C++17 (fold expressions in the fan-out helpers); self-
* suppresses below it.
*
*
* path:      /inc/djinterp/cpp/util/document/document_writer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ARENA NODE
      ----------
      a. DDocAttribute, DDocNode, DDocArena
      b. k_none

II.   NODE VIEW (protocol bridge)
      ---------------------------
      a. DAttributeView / DAttributeRange
      b. DChildRange
      c.    DDocView

III.  CURSOR
      ------
      a. cursor

IV.   DOCUMENT WRITER
      ---------------
      a. DDocumentWriter

V.    FAN-OUT HELPERS
      ---------------
      a. to_head
      b. broadcast
*/

#ifndef DJINTERP_UTIL_DOCUMENT_DOCUMENT_WRITER_HPP
#define DJINTERP_UTIL_DOCUMENT_DOCUMENT_WRITER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../../text/xml/xml.hpp"          // DXmlString, DXmlNodeKind


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   ARENA NODE                                        ///
///////////////////////////////////////////////////////////////////////////////

// k_none
//   constant: the sentinel `dj_index` meaning "no node" (no parent, no child,
// no sibling). `dj_index` is the framework's signed index type.
constexpr dj_index k_none = -1;


// DDocAttribute
//   type: a single attribute as a (name, value) pair.
using DDocAttribute = std::pair<DXmlString, DXmlString>;


// DDocNode
//   struct: one cell of the document arena. Children are held by the
// left-child / right-sibling index encoding (`first_child` + `next_sibling`),
// with `last_child` cached so appending a child is O(1). Every link is a
// `dj_index` into the owning arena, so nothing here is a pointer that could
// dangle on reallocation.
struct DDocNode
{
    DXmlString    name;
    DXmlString    text;
    DXmlNodeKind  kind         = DXmlNodeKind::element;

    dj_index       parent       = k_none;
    dj_index       first_child  = k_none;
    dj_index       last_child   = k_none;
    dj_index       next_sibling = k_none;

    std::vector<DDocAttribute> attributes;
};


// DDocArena
//   type: the flat, append-only node store. Indices into it are stable for
// the arena's lifetime.
using DDocArena = std::vector<DDocNode>;


///////////////////////////////////////////////////////////////////////////////
///                II.   NODE VIEW (protocol bridge)                        ///
///////////////////////////////////////////////////////////////////////////////

// DAttributeView
//   class: a read-only view of one arena attribute. Exposes `name()` and
// `value()`, satisfying the XML attribute protocol.
class DAttributeView
{
public:
    explicit DAttributeView(
        const DDocAttribute* _attribute
    )
        : m_attribute(_attribute)
    {}

    D_NODISCARD const DXmlString& name() const
    {
        return m_attribute->first;
    }

    D_NODISCARD const DXmlString& value() const
    {
        return m_attribute->second;
    }

private:
    const DDocAttribute* m_attribute;
};


// DAttributeRange
//   class: an iterable over a node's attributes, yielding `DAttributeView`s.
class DAttributeRange
{
public:
    explicit DAttributeRange(
        const std::vector<DDocAttribute>* _attributes
    )
        : m_attributes(_attributes)
    {}

    class iterator
    {
    public:
        iterator(
            const std::vector<DDocAttribute>* _attributes,
            std::size_t                       _index
        )
            : m_attributes(_attributes)
            , m_index(_index)
        {}

        D_NODISCARD DAttributeView operator*() const
        {
            return DAttributeView(&(*m_attributes)[m_index]);
        }

        iterator& operator++()
        {
            ++m_index;
            return *this;
        }

        D_NODISCARD bool operator!=(
            const iterator& _other
        ) const
        {
            return m_index != _other.m_index;
        }

    private:
        const std::vector<DDocAttribute>* m_attributes;
        std::size_t                       m_index;
    };

    D_NODISCARD iterator begin() const
    {
        return iterator(m_attributes, 0);
    }

    D_NODISCARD iterator end() const
    {
        return iterator(m_attributes, m_attributes->size());
    }

private:
    const std::vector<DDocAttribute>* m_attributes;
};


// DChildRange
//   class: an iterable over a node's children, walking the sibling chain
// (first_child -> next_sibling -> ... -> k_none) and yielding a `DDocView`
// for each. Declared before DDocView and completed inline once DDocView is
// defined (see below the class).
class DDocView;

class DChildRange
{
public:
    DChildRange(
        const DDocArena* _arena,
        dj_index          _first
    )
        : m_arena(_arena)
        , m_first(_first)
    {}

    class iterator
    {
    public:
        iterator(
            const DDocArena* _arena,
            dj_index          _node
        )
            : m_arena(_arena)
            , m_node(_node)
        {}

        D_NODISCARD DDocView operator*() const;                  // completed after DDocView

        iterator& operator++()
        {
            m_node = (*m_arena)[static_cast<std::size_t>(m_node)].next_sibling;
            return *this;
        }

        D_NODISCARD bool operator!=(
            const iterator& _other
        ) const
        {
            return m_node != _other.m_node;
        }

    private:
        const DDocArena* m_arena;
        dj_index          m_node;
    };

    D_NODISCARD iterator begin() const
    {
        return iterator(m_arena, m_first);
    }

    D_NODISCARD iterator end() const
    {
        return iterator(m_arena, k_none);
    }

private:
    const DDocArena* m_arena;
    dj_index          m_first;
};


// DDocView
//   class: a read-only view of one arena node that satisfies the XML node
// protocol -- name / kind / text / attributes / children. This is the
// bridge that lets `DDocumentPrinter` serialise an arena-built document.
class DDocView
{
public:
    DDocView(
        const DDocArena* _arena,
        dj_index          _node
    )
        : m_arena(_arena)
        , m_node(_node)
    {}

    D_NODISCARD const DXmlString& name() const
    {
        return m_at().name;
    }

    D_NODISCARD const DXmlString& text() const
    {
        return m_at().text;
    }

    D_NODISCARD DXmlNodeKind kind() const
    {
        return m_at().kind;
    }

    D_NODISCARD DAttributeRange attributes() const
    {
        return DAttributeRange(&m_at().attributes);
    }

    D_NODISCARD DChildRange children() const
    {
        return DChildRange(m_arena, m_at().first_child);
    }

private:
    const DDocNode& m_at() const
    {
        return (*m_arena)[static_cast<std::size_t>(m_node)];
    }

    const DDocArena* m_arena;
    dj_index          m_node;
};


// DChildRange::iterator::operator* -- now that DDocView is complete.
inline
DDocView DChildRange::iterator::operator*() const
{
    return DDocView(m_arena, m_node);
}


///////////////////////////////////////////////////////////////////////////////
///                  III.   CURSOR                                          ///
///////////////////////////////////////////////////////////////////////////////

// DCursor
//   class: a position-holding handle into a document arena -- `{arena,
// dj_index}`, ~16 bytes, trivially copyable. A cursor names a node and stays
// valid for the arena's life, so many cursors can be held at once and
// written to in any order. Building operations mutate the arena (through the
// borrowed pointer) but never move the handle itself.
//
//   append_child : add a child under the focus, return the focus (chain
//                  siblings: c.append_child("a").append_child("b") ...).
//   open_child   : add a child, return the CHILD's cursor (descend/configure).
//   append       : add a sibling after the focus, return the new sibling.
//   text / attr  : set the focus's text / add an attribute, return the focus.
class DCursor
{
public:
    DCursor(
        DDocArena* _arena,
        dj_index    _node
    )
        : m_arena(_arena)
        , m_node(_node)
    {}

    // append_child -- add a child element; return this focus (sibling chain).
    DCursor& append_child(
        DXmlString _name
    )
    {
        const dj_index _child =
            m_add(static_cast<DXmlString&&>(_name), m_node);
        m_link_child(m_node, _child);

        return *this;
    }

    // open_child -- add a child element; return the child's cursor (descend).
    D_NODISCARD DCursor open_child(
        DXmlString _name
    )
    {
        const dj_index _child =
            m_add(static_cast<DXmlString&&>(_name), m_node);
        m_link_child(m_node, _child);

        return DCursor(m_arena, _child);
    }

    // append -- add a sibling immediately after the focus; return its cursor.
    D_NODISCARD DCursor append(
        DXmlString _name
    )
    {
        const dj_index _parent = m_at(m_node).parent;
        const dj_index _sibling =
            m_add(static_cast<DXmlString&&>(_name), _parent);

        // splice _sibling in after m_node (references taken AFTER m_add, which
        // may have reallocated the arena).
        DDocNode& _focus = m_at(m_node);
        m_at(_sibling).next_sibling = _focus.next_sibling;
        _focus.next_sibling         = _sibling;
        if (_parent != k_none)
        {
            DDocNode& _p = m_at(_parent);
            if (_p.last_child == m_node)
            {
                _p.last_child = _sibling;
            }
        }

        return DCursor(m_arena, _sibling);
    }

    // text -- set the focus's text content; return the focus.
    DCursor& text(
        DXmlString _text
    )
    {
        m_at(m_node).text = static_cast<DXmlString&&>(_text);
        return *this;
    }

    // attr -- append an attribute to the focus; return the focus.
    DCursor& attr(
        DXmlString _name,
        DXmlString _value
    )
    {
        m_at(m_node).attributes.emplace_back(
            static_cast<DXmlString&&>(_name),
            static_cast<DXmlString&&>(_value));
        return *this;
    }

    // kind -- set the focus's node kind; return the focus.
    DCursor& kind(
        DXmlNodeKind _kind
    )
    {
        m_at(m_node).kind = _kind;
        return *this;
    }

    // view -- a read-only protocol view of the focused node.
    D_NODISCARD DDocView view() const
    {
        return DDocView(m_arena, m_node);
    }

    // index -- the focus's arena index (its stable identity).
    D_NODISCARD dj_index index() const
    {
        return m_node;
    }

private:
    DDocNode& m_at(
        dj_index _node
    )
    {
        return (*m_arena)[static_cast<std::size_t>(_node)];
    }

    // m_add -- append a fresh element node; return its index. May reallocate
    // the arena, so callers must re-fetch any references afterwards.
    dj_index m_add(
        DXmlString   _name,
        dj_index     _parent
    )
    {
        const dj_index _index = static_cast<dj_index>(m_arena->size());
        m_arena->emplace_back();
        DDocNode& _node = m_at(_index);
        _node.name   = static_cast<DXmlString&&>(_name);
        _node.parent = _parent;

        return _index;
    }

    // m_link_child -- attach `_child` as the last child of `_parent`, O(1).
    void m_link_child(
        dj_index _parent,
        dj_index _child
    )
    {
        DDocNode& _p = m_at(_parent);
        if (_p.first_child == k_none)
        {
            _p.first_child = _child;
            _p.last_child  = _child;
        }
        else
        {
            m_at(_p.last_child).next_sibling = _child;
            _p.last_child                    = _child;
        }

        return;
    }

    DDocArena* m_arena;
    dj_index    m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                  IV.   DOCUMENT WRITER                                  ///
///////////////////////////////////////////////////////////////////////////////

// DDocumentWriter
//   class: owns the document arena and hands out cursors. The arena lives
// behind a `unique_ptr` so the writer stays movable WITHOUT moving the arena
// storage -- a cursor borrows `arena()` and remains valid as long as the
// writer lives (iterator-like). Call `root(name)` once to create the root and
// obtain the first cursor; `document()` returns a protocol view of the root
// for serialisation.
class DDocumentWriter
{
public:
    DDocumentWriter()
        : m_arena(new DDocArena())
    {}

    // root -- create the root element and return a cursor at it. Call once,
    // before any other building.
    DCursor root(
        DXmlString _name
    )
    {
        const dj_index _index = static_cast<dj_index>(m_arena->size());
        m_arena->emplace_back();
        DDocNode& _node = (*m_arena)[static_cast<std::size_t>(_index)];
        _node.name   = static_cast<DXmlString&&>(_name);
        _node.parent = k_none;

        return DCursor(m_arena.get(), _index);
    }

    // document -- a read-only protocol view of the root node, for printing.
    D_NODISCARD DDocView document() const
    {
        return DDocView(m_arena.get(), 0);
    }

    // at -- a cursor at an arbitrary stable index (e.g. one saved earlier).
    D_NODISCARD DCursor at(
        dj_index _index
    )
    {
        return DCursor(m_arena.get(), _index);
    }

    // node_count -- number of nodes built so far.
    D_NODISCARD std::size_t node_count() const
    {
        return m_arena->size();
    }

    // arena -- the underlying store (advanced/interop use).
    D_NODISCARD const DDocArena& arena() const
    {
        return *m_arena;
    }

private:
    std::unique_ptr<DDocArena> m_arena;
};


///////////////////////////////////////////////////////////////////////////////
///                  V.   FAN-OUT HELPERS                                   ///
///////////////////////////////////////////////////////////////////////////////

// to_head
//   function: adapt a cursor + a renderer into an event consumer. Each call
// appends `render(event)` as the text of a fresh `element` child under the
// head -- turning a positional cursor into a `void(const Event&)` sink. The
// head is captured by value (it is a cheap, stable handle) and keeps
// appending under the same node.
template<typename Render>
D_NODISCARD auto to_head(
    DCursor      _head,
    DXmlString   _element,
    Render       _render
)
{
    return [_head, _element, _render](const auto& _event) mutable
    {
        _head.open_child(_element).text(_render(_event));
    };
}


// broadcast
//   function: fan one input out to several sinks (the `tee` pattern). Returns
// a sink that invokes every `_sink(input)` in order -- e.g. feed one event to
// both an inline head and a List-of-X head. Equivalent in spirit to
// `consumers::tee`; provided here so the document-building side is
// self-contained.
template<typename... Sinks>
D_NODISCARD auto broadcast(
    Sinks... _sinks
)
{
    return [_sinks...](const auto& _input) mutable
    {
        ( _sinks(_input), ... );
    };
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_DOCUMENT_WRITER_HPP
