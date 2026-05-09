/******************************************************************************
* djinterp [text]                                            html_template.hpp
*
*   Templated HTML element / document facades and the bundled default
* backend. The facades inherit from their XML counterparts so that
* memory layout is identical -- `html_element<_Backend>` adds NO
* members beyond `xml_node<_Backend>`, and `html_document<_Backend>`
* adds only an `html_version` byte and a `std::string` doctype to
* `xml_document<_Backend>`. All HTML-specific behaviour (DOCTYPE
* emission, void-element shorthand, class/id helpers, head/body
* shortcuts) lives in methods and free functions; never in storage.
*
*   ZERO OVERHEAD INVARIANT:
*   sizeof(html_element<B>) == sizeof(xml_node<B>) for every backend B.
*   The HTML methods are non-virtual and resolve at compile time.
*
*   DEFAULT BACKEND REUSE:
*   `html_default_backend` reuses XML's `internal::default_node` and
*   `internal::default_document` types verbatim. The backend exposes
*   BOTH `backend_tag = xml_default_backend_tag` (so XML traits classify
*   it as a complete XML backend) AND
*   `html_backend_tag = html_default_backend_tag` (so HTML traits
*   classify it as a complete HTML backend).
*
*
* path:      /inc/djinterp/core/text/html/html_template.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.08
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HTML DEFAULT BACKEND
II.   html_element<_Backend>
III.  html_document<_Backend>
IV.   FREE HELPERS / FACTORIES
*/

#ifndef DJINTERP_HTML_TEMPLATE_
#define DJINTERP_HTML_TEMPLATE_ 1

// std
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../xml/xml_template.hpp"


NS_DJINTERP

namespace html {


///////////////////////////////////////////////////////////////////////////////
///                I.   HTML DEFAULT BACKEND                                ///
///////////////////////////////////////////////////////////////////////////////

// html_default_backend
//   struct: bundled in-memory backend for the HTML facades.
// Reuses the XML default backend's storage types verbatim --
// `internal::default_node` and `internal::default_document` are
// already perfect for HTML's tag/attribute/child/text shape, so
// no parallel storage hierarchy is needed. Exposes BOTH the XML
// backend tag (for XML-trait classification) and the HTML backend
// tag (for HTML-trait classification) so the same type satisfies
// both protocols simultaneously.
struct html_default_backend
{
    // backend_tag
    //   type: XML backend identifier; lets `is_xml_backend` classify
    // this type as a complete XML backend.
    using backend_tag      = xml_default_backend_tag;

    // html_backend_tag
    //   type: HTML backend identifier; lets `is_html_backend`
    // classify this type as a complete HTML backend.
    using html_backend_tag = html_default_backend_tag;

    // node_type
    //   type: storage type for nodes -- shared with XML.
    using node_type        = internal::default_node;

    // attribute_type
    //   type: storage type for attributes -- shared with XML.
    using attribute_type   = xml_attribute;

    // document_type
    //   type: storage type for documents -- shared with XML.
    using document_type    = internal::default_document;

    // element_type
    //   type: alias for node_type. Satisfies the
    // `has_html_element_type_alias` trait.
    using element_type     = node_type;

    // make_document
    //   function: factory for a fresh empty document. Provided so
    // the same backend satisfies the XML backend protocol.
    static document_type
    make_document(
    )
    {
        return document_type();
    }

    // make_html_document
    //   function: factory for a fresh empty HTML document. Sets up a
    // root <html> element with empty <head> and <body> children so
    // callers can immediately start populating either subtree. The
    // returned object is the raw storage document, NOT the
    // `html_document` facade -- use `make_html5_document()` (free
    // function below) if you want a wrapped, version-stamped
    // document.
    static document_type
    make_html_document(
    )
    {
        document_type   doc;
        doc.version    = "1.0";
        doc.encoding   = xml_encoding::utf_8;
        doc.standalone = xml_standalone::unspecified;

        // Build <html><head/><body/></html> skeleton.
        std::unique_ptr<node_type> html_root_node(new node_type);
        html_root_node->kind = xml_node_kind::element;
        html_root_node->name = tags::html_root;

        std::unique_ptr<node_type> head_node(new node_type);
        head_node->kind   = xml_node_kind::element;
        head_node->name   = tags::head;
        head_node->parent = html_root_node.get();
        html_root_node->children.push_back(std::move(head_node));

        std::unique_ptr<node_type> body_node(new node_type);
        body_node->kind   = xml_node_kind::element;
        body_node->name   = tags::body;
        body_node->parent = html_root_node.get();
        html_root_node->children.push_back(std::move(body_node));

        doc.root = std::move(html_root_node);
        return doc;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.   html_element<_Backend>                             ///
///////////////////////////////////////////////////////////////////////////////

// html_element
//   class: thin facade over `xml_node<_Backend>` adding HTML
// semantics (kind enum, class-list manipulation, id accessors,
// void/block/inline queries). Inherits from `xml_node<_Backend>`
// so memory layout is unchanged -- `sizeof(html_element<B>) ==
// sizeof(xml_node<B>)`.
template<typename _Backend>
class html_element
:   public xml_node<_Backend>
{
public:
    // base_type
    //   type: alias for the underlying XML node facade.
    using base_type      = xml_node<_Backend>;

    // node_type
    //   type: backend storage node type, inherited from base.
    using node_type      = typename base_type::node_type;

    // attribute_type
    //   type: backend storage attribute type, inherited from base.
    using attribute_type = typename base_type::attribute_type;


    /// constructors

    // html_element (default)
    //   ctor: constructs a null element wrapping a null backend node.
    D_CONSTEXPR
    html_element(
    )
    :   base_type()
    {}

    // html_element (from raw node pointer)
    //   ctor: wraps the given backend node. Does not take ownership.
    explicit
    html_element(
        node_type* _node
    )
    :   base_type(_node)
    {}

    // html_element (from xml_node)
    //   ctor: re-wraps an existing xml_node as an html_element.
    // Zero-cost since the layout is identical.
    explicit
    html_element(
        const base_type& _node
    )
    :   base_type(_node)
    {}


    /// HTML kind queries

    // html_kind
    //   function: returns the `html_element_kind` discriminator
    // for this element by looking up its tag name. Returns
    // `html_element_kind::unknown` for non-elements or for tags
    // that fall outside the standard HTML5 registry.
    html_element_kind
    html_kind() const
    {
        if (!this->valid())
        {
            return html_element_kind::unknown;
        }
        return html_kind_from_name(this->name().c_str());
    }

    // get_html_kind
    //   function: getter-form alias for html_kind(). Provided so
    // the type satisfies `has_get_html_kind_method` as well.
    html_element_kind
    get_html_kind() const
    {
        return this->html_kind();
    }

    // is_void
    //   function: true if this element has no end tag (e.g. `<br>`,
    // `<img>`).
    bool
    is_void() const
    {
        return is_void_element_kind(this->html_kind());
    }

    // is_block
    //   function: true if this element is a block-level element.
    bool
    is_block() const
    {
        return is_block_element_kind(this->html_kind());
    }

    // is_inline
    //   function: true if this element is an inline-level element.
    bool
    is_inline() const
    {
        return is_inline_element_kind(this->html_kind());
    }

    // is_raw_text
    //   function: true if this element holds raw text (script,
    // style). Such elements must not have their text escaped on
    // emission.
    bool
    is_raw_text() const
    {
        return is_raw_text_element_kind(this->html_kind());
    }


    /// id helpers

    // get_id
    //   function: returns the `id` attribute value, or the empty
    // string if absent.
    xml_string_t
    get_id() const
    {
        const attribute_type* a = this->find_attribute(attrs::id);
        return (a != nullptr) ? a->value : xml_string_t();
    }

    // set_id
    //   function: sets the `id` attribute.
    void
    set_id(
        const xml_string_t& _id
    )
    {
        this->set_attribute(attrs::id, _id);
    }


    /// class-list helpers

    // class_list
    //   function: returns the `class` attribute split into
    // individual class names.
    std::vector<xml_string_t>
    class_list() const
    {
        std::vector<xml_string_t>   out;
        const attribute_type*       a = this->find_attribute(attrs::class_);
        if (a == nullptr)
        {
            return out;
        }

        const xml_string_t&     v = a->value;
        std::size_t             i = 0;
        const std::size_t       n = v.size();
        while (i < n)
        {
            // skip whitespace.
            while ((i < n) && ((v[i] == ' ') || (v[i] == '\t') ||
                               (v[i] == '\n') || (v[i] == '\r')))
            {
                ++i;
            }
            if (i >= n)
            {
                break;
            }
            const std::size_t   start = i;
            while ((i < n) && (v[i] != ' ') && (v[i] != '\t') &&
                              (v[i] != '\n') && (v[i] != '\r'))
            {
                ++i;
            }
            out.push_back(v.substr(start, i - start));
        }
        return out;
    }

    // has_class
    //   function: true if `_class` appears in this element's
    // class-list.
    bool
    has_class(
        const xml_string_t& _class
    )   const
    {
        const std::vector<xml_string_t> cs = this->class_list();
        for (std::size_t i = 0; i < cs.size(); ++i)
        {
            if (cs[i] == _class)
            {
                return true;
            }
        }
        return false;
    }

    // add_class
    //   function: appends `_class` to the class-list if not
    // already present.
    void
    add_class(
        const xml_string_t& _class
    )
    {
        if (this->has_class(_class))
        {
            return;
        }
        const attribute_type*   a = this->find_attribute(attrs::class_);
        xml_string_t            joined = (a != nullptr) ? a->value
                                                        : xml_string_t();
        if (!joined.empty())
        {
            joined += ' ';
        }
        joined += _class;
        this->set_attribute(attrs::class_, joined);
    }

    // remove_class
    //   function: removes `_class` from the class-list.
    void
    remove_class(
        const xml_string_t& _class
    )
    {
        const std::vector<xml_string_t> cs = this->class_list();
        xml_string_t                    joined;
        for (std::size_t i = 0; i < cs.size(); ++i)
        {
            if (cs[i] == _class)
            {
                continue;
            }
            if (!joined.empty())
            {
                joined += ' ';
            }
            joined += cs[i];
        }
        this->set_attribute(attrs::class_, joined);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                INTERNAL: HTML EMISSION HELPERS                          ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace html
NS_INTERNAL

    // html_escape_text_helper
    //   function: appends an HTML-escaped copy of `_in` to `_out`.
    inline void
    html_escape_text_helper(
        std::ostream&       _out,
        const xml_string_t& _in
    )
    {
        const std::size_t   n = _in.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            const char c = _in[i];
            switch (c)
            {
                case '&':  _out << "&amp;";  break;
                case '<':  _out << "&lt;";   break;
                case '>':  _out << "&gt;";   break;
                default:   _out << c;        break;
            }
        }
    }

    // html_escape_attr_helper
    //   function: like html_escape_text_helper but additionally
    // escapes the double-quote character (since attribute values
    // are emitted in double quotes).
    inline void
    html_escape_attr_helper(
        std::ostream&       _out,
        const xml_string_t& _in
    )
    {
        const std::size_t   n = _in.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            const char c = _in[i];
            switch (c)
            {
                case '&':   _out << "&amp;";  break;
                case '<':   _out << "&lt;";   break;
                case '>':   _out << "&gt;";   break;
                case '"':   _out << "&quot;"; break;
                default:    _out << c;        break;
            }
        }
    }

    // html_emit_node_helper
    //   function: recursive emitter for a single node tree under
    // HTML rules. `_xhtml` controls whether void elements are
    // self-closed (`<br/>` for XHTML) or open (`<br>` for HTML5/4).
    inline void
    html_emit_node_helper(
        std::ostream&             _out,
        const default_node&       _node,
        bool                      _xhtml,
        std::size_t               _depth,
        const char*               _indent
    )
    {
        // indent for the opening tag.
        for (std::size_t i = 0; i < _depth; ++i)
        {
            _out << _indent;
        }

        if (_node.kind == ::djinterp::xml_node_kind::text)
        {
            html_escape_text_helper(_out, _node.text);
            _out << '\n';
            return;
        }
        if (_node.kind == ::djinterp::xml_node_kind::comment)
        {
            _out << "<!--" << _node.text << "-->\n";
            return;
        }
        if (_node.kind != ::djinterp::xml_node_kind::element)
        {
            // unsupported / unknown -- skip silently.
            return;
        }

        // open tag.
        _out << '<' << _node.name;
        for (std::size_t i = 0; i < _node.attributes.size(); ++i)
        {
            const ::djinterp::xml_attribute& a = _node.attributes[i];
            _out << ' ' << a.name << "=\"";
            html_escape_attr_helper(_out, a.value);
            _out << '"';
        }

        // void element handling.
        const bool  void_elem =
            ::djinterp::html::is_void_element_name(_node.name.c_str());
        if (void_elem)
        {
            if (_xhtml)
            {
                _out << " />\n";
            }
            else
            {
                _out << ">\n";
            }
            return;
        }

        // raw-text element handling: emit text without escaping.
        const bool  raw_text =
            ::djinterp::html::is_raw_text_element_name(_node.name.c_str());

        // empty element shortcut.
        if (_node.children.empty() && _node.text.empty())
        {
            _out << "></" << _node.name << ">\n";
            return;
        }

        _out << ">";

        // text content (if no element children).
        if (_node.children.empty())
        {
            if (raw_text)
            {
                _out << _node.text;
            }
            else
            {
                html_escape_text_helper(_out, _node.text);
            }
            _out << "</" << _node.name << ">\n";
            return;
        }

        // mixed content: emit children, then close on its own line.
        _out << '\n';
        if (!_node.text.empty())
        {
            for (std::size_t i = 0; i < _depth + 1; ++i)
            {
                _out << _indent;
            }
            if (raw_text)
            {
                _out << _node.text;
            }
            else
            {
                html_escape_text_helper(_out, _node.text);
            }
            _out << '\n';
        }
        for (std::size_t i = 0; i < _node.children.size(); ++i)
        {
            if (_node.children[i] != nullptr)
            {
                html_emit_node_helper(
                    _out, *_node.children[i], _xhtml, _depth + 1, _indent);
            }
        }
        for (std::size_t i = 0; i < _depth; ++i)
        {
            _out << _indent;
        }
        _out << "</" << _node.name << ">\n";
    }

NS_END  // internal
namespace html {


///////////////////////////////////////////////////////////////////////////////
///                III.   html_document<_Backend>                           ///
///////////////////////////////////////////////////////////////////////////////

// html_document
//   class: facade over `xml_document<_Backend>` adding HTML
// document semantics (DOCTYPE declaration, version stamping,
// head/body/title shortcuts, HTML-aware emission).
template<typename _Backend>
class html_document
:   public xml_document<_Backend>
{
public:
    // base_type
    //   type: alias for the underlying XML document facade.
    using base_type     = xml_document<_Backend>;

    // backend_type
    //   type: alias for the backend.
    using backend_type  = _Backend;

    // node_type
    //   type: backend storage node type.
    using node_type     = typename base_type::node_type;

    // document_type
    //   type: backend storage document type.
    using document_type = typename base_type::document_type;

    // element_facade
    //   type: HTML facade returned by head_element / body_element.
    using element_facade = html_element<_Backend>;


    /// constructors

    // html_document (default)
    //   ctor: constructs an empty HTML5 document with an empty
    // root <html> tree.
    html_document(
    )
    :   base_type(),
        m_version(html_version::html5),
        m_doctype(D_HTML_DEFAULT_DOCTYPE)
    {}

    // html_document (from version)
    //   ctor: constructs an empty document targeting `_v`.
    explicit
    html_document(
        html_version    _v
    )
    :   base_type(),
        m_version(_v),
        m_doctype(html_doctype_string(_v))
    {}

    // html_document (from existing storage doc)
    //   ctor: wraps an already-built backend storage document.
    explicit
    html_document(
        document_type&& _doc,
        html_version    _v = html_version::html5
    )
    :   base_type(),
        m_version(_v),
        m_doctype(html_doctype_string(_v))
    {
        // The base xml_document holds m_doc by value; route the move
        // through its public set_root if available, otherwise via
        // direct backend access.
        // Default backend storage path:
        document_type& target = const_cast<document_type&>(
            this->backend_document());
        target = std::move(_doc);
    }


    /// version / doctype accessors

    // html_version
    //   function: returns the document's target HTML / XHTML
    // version.
    enum html_version
    html_version() const
    {
        return m_version;
    }

    // set_html_version
    //   function: updates the version and re-derives the default
    // doctype string accordingly.
    void
    set_html_version(
        enum html_version   _v
    )
    {
        m_version = _v;
        m_doctype = html_doctype_string(_v);
    }

    // doctype
    //   function: returns the DOCTYPE declaration that will be
    // emitted at the top of the document.
    const xml_string_t&
    doctype() const
    {
        return m_doctype;
    }

    // get_doctype
    //   function: getter-form alias for doctype().
    const xml_string_t&
    get_doctype() const
    {
        return m_doctype;
    }

    // set_doctype
    //   function: overrides the DOCTYPE string. Sets the version
    // to `custom` so the override is preserved on subsequent
    // emissions.
    void
    set_doctype(
        const xml_string_t& _doctype
    )
    {
        m_doctype = _doctype;
        m_version = html_version::custom;
    }


    /// head / body / title shortcuts

    // head_element
    //   function: returns a facade for the document's `<head>`
    // element. Returns a null facade if `<head>` is absent.
    element_facade
    head_element() const
    {
        node_type* root = const_cast<node_type*>(
            this->root_element_node());
        if (root == nullptr)
        {
            return element_facade();
        }
        for (std::size_t i = 0; i < root->children.size(); ++i)
        {
            node_type* c = root->children[i].get();
            if ((c != nullptr) && (c->name == tags::head))
            {
                return element_facade(c);
            }
        }
        return element_facade();
    }

    // body_element
    //   function: returns a facade for the document's `<body>`
    // element. Returns a null facade if `<body>` is absent.
    element_facade
    body_element() const
    {
        node_type* root = const_cast<node_type*>(
            this->root_element_node());
        if (root == nullptr)
        {
            return element_facade();
        }
        for (std::size_t i = 0; i < root->children.size(); ++i)
        {
            node_type* c = root->children[i].get();
            if ((c != nullptr) && (c->name == tags::body))
            {
                return element_facade(c);
            }
        }
        return element_facade();
    }

    // title
    //   function: returns the contents of the `<title>` element
    // inside `<head>`, or the empty string if absent.
    xml_string_t
    title() const
    {
        element_facade  head = this->head_element();
        if (!head.valid())
        {
            return xml_string_t();
        }
        node_type*  head_node = head.backend_handle();
        for (std::size_t i = 0; i < head_node->children.size(); ++i)
        {
            node_type*  c = head_node->children[i].get();
            if ((c != nullptr) && (c->name == tags::title))
            {
                return c->text;
            }
        }
        return xml_string_t();
    }

    // set_title
    //   function: sets the document's <title>. Creates the <head>
    // and <title> elements as necessary.
    void
    set_title(
        const xml_string_t& _title
    )
    {
        node_type*  root = const_cast<node_type*>(
            this->root_element_node());
        if (root == nullptr)
        {
            return;
        }

        // locate or create <head>.
        node_type*  head_node = nullptr;
        for (std::size_t i = 0; i < root->children.size(); ++i)
        {
            node_type* c = root->children[i].get();
            if ((c != nullptr) && (c->name == tags::head))
            {
                head_node = c;
                break;
            }
        }
        if (head_node == nullptr)
        {
            std::unique_ptr<node_type> nh(new node_type);
            nh->kind   = ::djinterp::xml_node_kind::element;
            nh->name   = tags::head;
            nh->parent = root;
            head_node  = nh.get();
            root->children.insert(root->children.begin(), std::move(nh));
        }

        // locate or create <title>.
        node_type*  title_node = nullptr;
        for (std::size_t i = 0; i < head_node->children.size(); ++i)
        {
            node_type* c = head_node->children[i].get();
            if ((c != nullptr) && (c->name == tags::title))
            {
                title_node = c;
                break;
            }
        }
        if (title_node == nullptr)
        {
            std::unique_ptr<node_type> nt(new node_type);
            nt->kind   = ::djinterp::xml_node_kind::element;
            nt->name   = tags::title;
            nt->parent = head_node;
            title_node = nt.get();
            head_node->children.push_back(std::move(nt));
        }

        title_node->text = _title;
    }


    /// HTML emission

    // write
    //   function: serialises the document to `_out` under HTML
    // emission rules (DOCTYPE first, void elements as `<br>` or
    // `<br/>` based on version, raw-text elements without
    // escaping). Always returns true.
    bool
    write(
        std::ostream&   _out
    )   const
    {
        const bool  xhtml = is_xhtml_version(m_version);

        if (!m_doctype.empty())
        {
            _out << m_doctype << '\n';
        }

        const node_type*    root = this->root_element_node();
        if (root != nullptr)
        {
            internal::html_emit_node_helper(
                _out, *root, xhtml, 0, D_HTML_DEFAULT_INDENT);
        }
        return true;
    }

    // write
    //   function: convenience overload that returns the serialised
    // document as a string.
    xml_string_t
    write() const
    {
        std::ostringstream  oss;
        this->write(oss);
        return oss.str();
    }


protected:
    // root_element_node
    //   function: returns a raw pointer to the document's root
    // backend node. For the default backend this is just the
    // `unique_ptr<default_node>` payload of the storage doc.
    // Subclasses / specialisations targeting third-party backends
    // should override this in a partial specialisation. The
    // default implementation handles the bundled backend.
    const node_type*
    root_element_node() const
    {
        const document_type& d = this->backend_document();
        return d.root.get();
    }


private:
    // m_version
    //   field: target HTML / XHTML version.
    enum html_version   m_version;

    // m_doctype
    //   field: cached DOCTYPE declaration string.
    xml_string_t        m_doctype;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   FREE HELPERS / FACTORIES                           ///
///////////////////////////////////////////////////////////////////////////////

// make_html5_document
//   function: builds a fresh HTML5 document containing an
// `<html>` root with `<head>` and `<body>` children. If `_title`
// is non-empty, also inserts a `<title>` element under `<head>`.
template<typename _Backend>
inline html_document<_Backend>
make_html5_document(
    const xml_string_t& _title = xml_string_t()
)
{
    html_document<_Backend>     doc(html_version::html5);

    // The default ctor leaves the underlying storage document with
    // a null root; bootstrap a skeleton via the backend factory.
    using doc_t = typename _Backend::document_type;
    doc_t skeleton = _Backend::make_html_document();
    doc = html_document<_Backend>(std::move(skeleton),
                                  html_version::html5);

    if (!_title.empty())
    {
        doc.set_title(_title);
    }
    return doc;
}


// make_xhtml_document
//   function: builds a fresh XHTML document targeting the given
// XHTML variant. Skeleton identical to make_html5_document but the
// emission flags and DOCTYPE differ.
template<typename _Backend>
inline html_document<_Backend>
make_xhtml_document(
    html_version            _v     = html_version::xhtml1_strict,
    const xml_string_t&     _title = xml_string_t()
)
{
    using doc_t = typename _Backend::document_type;
    doc_t skeleton = _Backend::make_html_document();
    html_document<_Backend>     doc(std::move(skeleton), _v);
    if (!_title.empty())
    {
        doc.set_title(_title);
    }
    return doc;
}


// make_div
//   function: returns a new freestanding `<div>` element wrapped
// in an `html_element` facade. The backend storage node is
// allocated via `new`; ownership transfers to whichever parent
// later adopts it via `add_child` (when using the default
// backend, that path moves the unique_ptr in).
template<typename _Backend>
inline html_element<_Backend>
make_div(
    const xml_string_t& _id    = xml_string_t(),
    const xml_string_t& _class = xml_string_t()
)
{
    using node_t = typename _Backend::node_type;
    node_t* n = new node_t;
    n->kind   = ::djinterp::xml_node_kind::element;
    n->name   = tags::div_;

    html_element<_Backend>  e(n);
    if (!_id.empty())
    {
        e.set_id(_id);
    }
    if (!_class.empty())
    {
        e.add_class(_class);
    }
    return e;
}


// make_span
//   function: returns a new freestanding `<span>` element.
template<typename _Backend>
inline html_element<_Backend>
make_span(
    const xml_string_t& _text  = xml_string_t(),
    const xml_string_t& _class = xml_string_t()
)
{
    using node_t = typename _Backend::node_type;
    node_t* n = new node_t;
    n->kind   = ::djinterp::xml_node_kind::element;
    n->name   = tags::span_;
    n->text   = _text;

    html_element<_Backend>  e(n);
    if (!_class.empty())
    {
        e.add_class(_class);
    }
    return e;
}


// make_anchor
//   function: returns a new freestanding `<a>` element with the
// given `href` and inner text.
template<typename _Backend>
inline html_element<_Backend>
make_anchor(
    const xml_string_t& _href,
    const xml_string_t& _text
)
{
    using node_t = typename _Backend::node_type;
    node_t* n = new node_t;
    n->kind   = ::djinterp::xml_node_kind::element;
    n->name   = tags::a;
    n->text   = _text;

    html_element<_Backend>  e(n);
    e.set_attribute(attrs::href, _href);
    return e;
}


// make_image
//   function: returns a new freestanding `<img>` (void) element.
template<typename _Backend>
inline html_element<_Backend>
make_image(
    const xml_string_t& _src,
    const xml_string_t& _alt = xml_string_t()
)
{
    using node_t = typename _Backend::node_type;
    node_t* n = new node_t;
    n->kind   = ::djinterp::xml_node_kind::element;
    n->name   = tags::img;

    html_element<_Backend>  e(n);
    e.set_attribute(attrs::src, _src);
    if (!_alt.empty())
    {
        e.set_attribute(attrs::alt, _alt);
    }
    return e;
}


// make_paragraph
//   function: returns a new freestanding `<p>` element with the
// given inner text.
template<typename _Backend>
inline html_element<_Backend>
make_paragraph(
    const xml_string_t& _text = xml_string_t()
)
{
    using node_t = typename _Backend::node_type;
    node_t* n = new node_t;
    n->kind   = ::djinterp::xml_node_kind::element;
    n->name   = tags::p;
    n->text   = _text;
    return html_element<_Backend>(n);
}


// make_heading
//   function: returns a new freestanding heading element. `_level`
// must be in [1,6]; values outside that range default to h1.
template<typename _Backend>
inline html_element<_Backend>
make_heading(
    int                 _level,
    const xml_string_t& _text = xml_string_t()
)
{
    const char* tag = (_level == 2) ? tags::h2
                    : (_level == 3) ? tags::h3
                    : (_level == 4) ? tags::h4
                    : (_level == 5) ? tags::h5
                    : (_level == 6) ? tags::h6
                    :                 tags::h1;

    using node_t = typename _Backend::node_type;
    node_t* n = new node_t;
    n->kind   = ::djinterp::xml_node_kind::element;
    n->name   = tag;
    n->text   = _text;
    return html_element<_Backend>(n);
}


// make_element_by_kind
//   function: returns a new freestanding element of the given
// kind with no children, attributes, or text. Useful for generic
// builders that walk an element-kind enum.
template<typename _Backend>
inline html_element<_Backend>
make_element_by_kind(
    html_element_kind   _kind
)
{
    using node_t = typename _Backend::node_type;
    node_t* n = new node_t;
    n->kind   = ::djinterp::xml_node_kind::element;
    n->name   = name_from_html_kind(_kind);
    return html_element<_Backend>(n);
}


}   // namespace html
NS_END  // djinterp


#endif  // DJINTERP_HTML_TEMPLATE_
