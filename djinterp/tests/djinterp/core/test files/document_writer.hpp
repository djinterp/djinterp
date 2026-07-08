/******************************************************************************
* djinterp [text]                                          document_writer.hpp
*
*   Builds a document tree incrementally through stable, position-holding
* cursors. The tree is stored in a flat, append-only arena
* (`std::vector<doc_node>`); a cursor is `{arena, d_index}` -- a comonadic
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
* serialisation fold (`document_printer` over `document()`) emits each
* region in its structural slot. Write order and document order are fully
* decoupled.
*
*   PROTOCOL BRIDGE:
*   `document()` returns a `doc_view` -- a lightweight, read-only view over
* the arena that satisfies the XML node protocol (name / kind / text /
* attributes / children). So the generic `document_printer` (and any
* protocol-generic consumer) serialises an arena-built document through the
* exact same code path as a pugixml or libxml++ document. The writer OWNS
* building; the node protocol is the meeting point with reading/printing.
*
*   This header depends only on `xml.hpp` (for `xml_string_t` /
* `xml_node_kind`); it does NOT depend on the printer -- serialisation is a
* separate concern applied to `document()`.
*
*   Requires C++17 (fold expressions in the fan-out helpers); self-
* suppresses below it.
*
* 
* path:      /inc/djinterp/core/text/document_writer.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ARENA NODE
      ----------
      a. doc_attribute, doc_node, doc_arena
      b. k_none
II.   NODE VIEW (protocol bridge)
      ---------------------------
      a. attribute_view / attribute_range
      b. child_range
      c. doc_view
III.  CURSOR
      ------
      a. cursor
IV.   DOCUMENT WRITER
      ---------------
      a. document_writer
V.    FAN-OUT HELPERS
      ---------------
      a. to_head
      b. broadcast
*/

#ifndef DJINTERP_TEXT_DOCUMENT_WRITER_
#define DJINTERP_TEXT_DOCUMENT_WRITER_ 1

// std
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./xml/xml.hpp"          // xml_string_t, xml_node_kind


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   ARENA NODE                                        ///
///////////////////////////////////////////////////////////////////////////////

// k_none
//   constant: the sentinel `d_index` meaning "no node" (no parent, no child,
// no sibling). `d_index` is the framework's signed index type.
constexpr d_index k_none = -1;


// doc_attribute
//   type: a single attribute as a (name, value) pair.
using doc_attribute = std::pair<xml_string_t, xml_string_t>;


// doc_node
//   struct: one cell of the document arena. Children are held by the
// left-child / right-sibling index encoding (`first_child` + `next_sibling`),
// with `last_child` cached so appending a child is O(1). Every link is a
// `d_index` into the owning arena, so nothing here is a pointer that could
// dangle on reallocation.
struct doc_node
{
    xml_string_t  name;
    xml_string_t  text;
    xml_node_kind kind         = xml_node_kind::element;

    d_index       parent       = k_none;
    d_index       first_child  = k_none;
    d_index       last_child   = k_none;
    d_index       next_sibling = k_none;

    std::vector<doc_attribute> attributes;
};


// doc_arena
//   type: the flat, append-only node store. Indices into it are stable for
// the arena's lifetime.
using doc_arena = std::vector<doc_node>;


///////////////////////////////////////////////////////////////////////////////
///                II.   NODE VIEW (protocol bridge)                        ///
///////////////////////////////////////////////////////////////////////////////

// attribute_view
//   class: a read-only view of one arena attribute. Exposes `name()` and
// `value()`, satisfying the XML attribute protocol.
class attribute_view
{
public:
    explicit attribute_view(
        const doc_attribute* _attribute
    )
        : m_attribute(_attribute)
    {}

    D_NODISCARD const xml_string_t&
    name() const
    {
        return m_attribute->first;
    }

    D_NODISCARD const xml_string_t&
    value() const
    {
        return m_attribute->second;
    }

private:
    const doc_attribute* m_attribute;
};


// attribute_range
//   class: an iterable over a node's attributes, yielding `attribute_view`s.
class attribute_range
{
public:
    explicit attribute_range(
        const std::vector<doc_attribute>* _attributes
    )
        : m_attributes(_attributes)
    {}

    class iterator
    {
    public:
        iterator(
            const std::vector<doc_attribute>* _attributes,
            std::size_t                       _index
        )
            : m_attributes(_attributes)
            , m_index(_index)
        {}

        D_NODISCARD attribute_view
        operator*() const
        {
            return attribute_view(&(*m_attributes)[m_index]);
        }

        iterator&
        operator++()
        {
            ++m_index;
            return *this;
        }

        D_NODISCARD bool
        operator!=(
            const iterator& _other
        ) const
        {
            return m_index != _other.m_index;
        }

    private:
        const std::vector<doc_attribute>* m_attributes;
        std::size_t                       m_index;
    };

    D_NODISCARD iterator
    begin() const
    {
        return iterator(m_attributes, 0);
    }

    D_NODISCARD iterator
    end() const
    {
        return iterator(m_attributes, m_attributes->size());
    }

private:
    const std::vector<doc_attribute>* m_attributes;
};


// child_range
//   class: an iterable over a node's children, walking the sibling chain
// (first_child -> next_sibling -> ... -> k_none) and yielding a `doc_view`
// for each. Declared before doc_view and completed inline once doc_view is
// defined (see below the class).
class doc_view;

class child_range
{
public:
    child_range(
        const doc_arena* _arena,
        d_index          _first
    )
        : m_arena(_arena)
        , m_first(_first)
    {}

    class iterator
    {
    public:
        iterator(
            const doc_arena* _arena,
            d_index          _node
        )
            : m_arena(_arena)
            , m_node(_node)
        {}

        D_NODISCARD doc_view
        operator*() const;                  // completed after doc_view

        iterator&
        operator++()
        {
            m_node = (*m_arena)[static_cast<std::size_t>(m_node)].next_sibling;
            return *this;
        }

        D_NODISCARD bool
        operator!=(
            const iterator& _other
        ) const
        {
            return m_node != _other.m_node;
        }

    private:
        const doc_arena* m_arena;
        d_index          m_node;
    };

    D_NODISCARD iterator
    begin() const
    {
        return iterator(m_arena, m_first);
    }

    D_NODISCARD iterator
    end() const
    {
        return iterator(m_arena, k_none);
    }

private:
    const doc_arena* m_arena;
    d_index          m_first;
};


// doc_view
//   class: a read-only view of one arena node that satisfies the XML node
// protocol -- name / kind / text / attributes / children. This is the
// bridge that lets `document_printer` serialise an arena-built document.
class doc_view
{
public:
    doc_view(
        const doc_arena* _arena,
        d_index          _node
    )
        : m_arena(_arena)
        , m_node(_node)
    {}

    D_NODISCARD const xml_string_t&
    name() const
    {
        return m_at().name;
    }

    D_NODISCARD const xml_string_t&
    text() const
    {
        return m_at().text;
    }

    D_NODISCARD xml_node_kind
    kind() const
    {
        return m_at().kind;
    }

    D_NODISCARD attribute_range
    attributes() const
    {
        return attribute_range(&m_at().attributes);
    }

    D_NODISCARD child_range
    children() const
    {
        return child_range(m_arena, m_at().first_child);
    }

private:
    const doc_node&
    m_at() const
    {
        return (*m_arena)[static_cast<std::size_t>(m_node)];
    }

    const doc_arena* m_arena;
    d_index          m_node;
};


// child_range::iterator::operator* -- now that doc_view is complete.
inline doc_view
child_range::iterator::operator*() const
{
    return doc_view(m_arena, m_node);
}


///////////////////////////////////////////////////////////////////////////////
///                  III.   CURSOR                                          ///
///////////////////////////////////////////////////////////////////////////////

// cursor
//   class: a position-holding handle into a document arena -- `{arena,
// d_index}`, ~16 bytes, trivially copyable. A cursor names a node and stays
// valid for the arena's life, so many cursors can be held at once and
// written to in any order. Building operations mutate the arena (through the
// borrowed pointer) but never move the handle itself.
//
//   append_child : add a child under the focus, return the focus (chain
//                  siblings: c.append_child("a").append_child("b") ...).
//   open_child   : add a child, return the CHILD's cursor (descend/configure).
//   append       : add a sibling after the focus, return the new sibling.
//   text / attr  : set the focus's text / add an attribute, return the focus.
class cursor
{
public:
    cursor(
        doc_arena* _arena,
        d_index    _node
    )
        : m_arena(_arena)
        , m_node(_node)
    {}

    // append_child -- add a child element; return this focus (sibling chain).
    cursor&
    append_child(
        xml_string_t _name
    )
    {
        const d_index _child =
            m_add(static_cast<xml_string_t&&>(_name), m_node);
        m_link_child(m_node, _child);

        return *this;
    }

    // open_child -- add a child element; return the child's cursor (descend).
    D_NODISCARD cursor
    open_child(
        xml_string_t _name
    )
    {
        const d_index _child =
            m_add(static_cast<xml_string_t&&>(_name), m_node);
        m_link_child(m_node, _child);

        return cursor(m_arena, _child);
    }

    // append -- add a sibling immediately after the focus; return its cursor.
    D_NODISCARD cursor
    append(
        xml_string_t _name
    )
    {
        const d_index _parent = m_at(m_node).parent;
        const d_index _sibling =
            m_add(static_cast<xml_string_t&&>(_name), _parent);

        // splice _sibling in after m_node (references taken AFTER m_add, which
        // may have reallocated the arena).
        doc_node& _focus = m_at(m_node);
        m_at(_sibling).next_sibling = _focus.next_sibling;
        _focus.next_sibling         = _sibling;
        if (_parent != k_none)
        {
            doc_node& _p = m_at(_parent);
            if (_p.last_child == m_node)
            {
                _p.last_child = _sibling;
            }
        }

        return cursor(m_arena, _sibling);
    }

    // text -- set the focus's text content; return the focus.
    cursor&
    text(
        xml_string_t _text
    )
    {
        m_at(m_node).text = static_cast<xml_string_t&&>(_text);
        return *this;
    }

    // attr -- append an attribute to the focus; return the focus.
    cursor&
    attr(
        xml_string_t _name,
        xml_string_t _value
    )
    {
        m_at(m_node).attributes.emplace_back(
            static_cast<xml_string_t&&>(_name),
            static_cast<xml_string_t&&>(_value));
        return *this;
    }

    // kind -- set the focus's node kind; return the focus.
    cursor&
    kind(
        xml_node_kind _kind
    )
    {
        m_at(m_node).kind = _kind;
        return *this;
    }

    // view -- a read-only protocol view of the focused node.
    D_NODISCARD doc_view
    view() const
    {
        return doc_view(m_arena, m_node);
    }

    // index -- the focus's arena index (its stable identity).
    D_NODISCARD d_index
    index() const
    {
        return m_node;
    }

private:
    doc_node&
    m_at(
        d_index _node
    )
    {
        return (*m_arena)[static_cast<std::size_t>(_node)];
    }

    // m_add -- append a fresh element node; return its index. May reallocate
    // the arena, so callers must re-fetch any references afterwards.
    d_index
    m_add(
        xml_string_t _name,
        d_index      _parent
    )
    {
        const d_index _index = static_cast<d_index>(m_arena->size());
        m_arena->emplace_back();
        doc_node& _node = m_at(_index);
        _node.name   = static_cast<xml_string_t&&>(_name);
        _node.parent = _parent;

        return _index;
    }

    // m_link_child -- attach `_child` as the last child of `_parent`, O(1).
    void
    m_link_child(
        d_index _parent,
        d_index _child
    )
    {
        doc_node& _p = m_at(_parent);
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

    doc_arena* m_arena;
    d_index    m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                  IV.   DOCUMENT WRITER                                  ///
///////////////////////////////////////////////////////////////////////////////

// document_writer
//   class: owns the document arena and hands out cursors. The arena lives
// behind a `unique_ptr` so the writer stays movable WITHOUT moving the arena
// storage -- a cursor borrows `arena()` and remains valid as long as the
// writer lives (iterator-like). Call `root(name)` once to create the root and
// obtain the first cursor; `document()` returns a protocol view of the root
// for serialisation.
class document_writer
{
public:
    document_writer()
        : m_arena(new doc_arena())
    {}

    // root -- create the root element and return a cursor at it. Call once,
    // before any other building.
    cursor
    root(
        xml_string_t _name
    )
    {
        const d_index _index = static_cast<d_index>(m_arena->size());
        m_arena->emplace_back();
        doc_node& _node = (*m_arena)[static_cast<std::size_t>(_index)];
        _node.name   = static_cast<xml_string_t&&>(_name);
        _node.parent = k_none;

        return cursor(m_arena.get(), _index);
    }

    // document -- a read-only protocol view of the root node, for printing.
    D_NODISCARD doc_view
    document() const
    {
        return doc_view(m_arena.get(), 0);
    }

    // at -- a cursor at an arbitrary stable index (e.g. one saved earlier).
    D_NODISCARD cursor
    at(
        d_index _index
    )
    {
        return cursor(m_arena.get(), _index);
    }

    // node_count -- number of nodes built so far.
    D_NODISCARD std::size_t
    node_count() const
    {
        return m_arena->size();
    }

    // arena -- the underlying store (advanced/interop use).
    D_NODISCARD const doc_arena&
    arena() const
    {
        return *m_arena;
    }

private:
    std::unique_ptr<doc_arena> m_arena;
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
template<typename _Render>
D_NODISCARD auto
to_head(
    cursor       _head,
    xml_string_t _element,
    _Render      _render
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
template<typename... _Sinks>
D_NODISCARD auto
broadcast(
    _Sinks... _sinks
)
{
    return [_sinks...](const auto& _input) mutable
    {
        ( _sinks(_input), ... );
    };
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEXT_DOCUMENT_WRITER_
