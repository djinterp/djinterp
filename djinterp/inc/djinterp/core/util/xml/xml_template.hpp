/******************************************************************************
* djinterp [text]                                             xml_template.hpp
*
*   Templated, library-agnostic XML attribute / node / document types.
* Each user-facing class is parameterised over a `_Backend` type that
* provides the concrete underlying representation. A default in-memory
* backend (`xml_default_backend`) is supplied here; adapters for
* libxml++, pugixml, tinyxml2, and other libraries live in their own
* headers and specialise the template.
*
*   DESIGN:
*   - `xml_attribute` is backend-agnostic and held by value: name +
*     value, both `xml_string_t`. Trivially copyable, no library
*     coupling at the leaf.
*   - `xml_node<_Backend>` wraps a backend handle and exposes the
*     standard structural protocol (name, kind, attributes, children,
*     text). Methods are thin pass-throughs to the backend; non-method
*     functionality (find_child, set_attribute, etc.) is provided by
*     free helper functions.
*   - `xml_document<_Backend>` owns the backend's document handle and
*     exposes the prolog (version, encoding, standalone) plus parse,
*     write, and save.
*   - The default backend uses std::vector for attributes and
*     std::vector<std::unique_ptr<...>> for children, giving stable
*     tree ownership without bringing in any library.
*
*   ADAPTER POINT:
*   To bind libxml++ (or any other library) into this template:
*     1. Define a `xmlpp_backend` struct whose nested
*        `node_type` / `attribute_type` / `document_type` are the
*        library's native types (xmlpp::Element, xmlpp::Attribute,
*        xmlpp::Document).
*     2. Tag it with `using backend_tag = xml_libxmlpp_backend_tag;`.
*     3. Write a specialisation `xml_node<xmlpp_backend>` that
*        forwards to the library's API. The structural traits in
*        `xml_template_traits.hpp` will detect it automatically.
*
*
* path:      /inc/djinterp/core/text/xml/xml_template.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.08
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    XML ATTRIBUTE
      ---------------
      a. xml_attribute
            - name, value
            - default + parameterised constructors
            - operator==

II.   DEFAULT BACKEND
      ------------------
      a. default_node     (internal in-memory node)
      b. default_document (internal in-memory document)
      c. xml_default_backend (tag + nested type aliases)

III.  TEMPLATED NODE
      -----------------
      a. xml_node<_Backend>
            - backend_handle accessor
            - name / kind / value
            - attributes / children iteration
            - find_child / find_attribute
            - add_child / set_attribute / remove_*
            - text / set_text

IV.   TEMPLATED DOCUMENT
      ---------------------
      a. xml_document<_Backend>
            - default + parameterised constructors
            - root_element accessor
            - version / encoding / standalone
            - parse / write / save

V.    FREE FUNCTIONS
      ----------------
      a. make_attribute
      b. make_default_document
      c. find_child_by_path  (slash-delimited XPath subset)
*/

#ifndef DJINTERP_XML_TEMPLATE_
#define DJINTERP_XML_TEMPLATE_ 1

// std
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "./xml_template_traits.hpp"


NS_DJINTERP


// forward declarations
template<typename _Backend>
class xml_node;

template<typename _Backend>
class xml_document;


///////////////////////////////////////////////////////////////////////////////
///                I.   XML ATTRIBUTE                                        ///
///////////////////////////////////////////////////////////////////////////////

// xml_attribute
//   struct: backend-agnostic name/value pair representing a single
// XML attribute. Held by value; copyable and movable; trivially
// destructible. Backends that need a richer attribute (namespace,
// prefix, parent pointer) may carry their own type and be detected
// via `is_xml_attribute`.
struct xml_attribute
{
    xml_string_t name;
    xml_string_t value;

    // xml_attribute (default)
    //   constructor: initialises both fields to empty strings.
    D_CONSTEXPR xml_attribute()
        : name(), value()
    {}

    // xml_attribute (parameterised)
    //   constructor: initialises from a name and value.
    xml_attribute(
        const xml_string_t& _name,
        const xml_string_t& _value
    )
        : name(_name),
          value(_value)
    {}

    // operator==
    //   function: equality comparison on both fields.
    bool
    operator==(
        const xml_attribute& _other
    ) const
    {
        return ( (name  == _other.name) &&
                 (value == _other.value) );
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.   DEFAULT BACKEND                                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // default_node
    //   struct: internal in-memory node used by the default
    // backend. Owns its attributes by value and its children by
    // unique_ptr to keep tree-shaped ownership cycle-free.
    struct default_node
    {
        xml_node_kind                              kind;
        xml_string_t                               name;
        xml_string_t                               text;
        std::vector<xml_attribute>                 attributes;
        std::vector<std::unique_ptr<default_node>> children;
        default_node*                              parent;

        // default_node (default)
        //   constructor: element with empty name and no parent.
        default_node()
            : kind(xml_node_kind::element),
              name(),
              text(),
              attributes(),
              children(),
              parent(nullptr)
        {}

        // default_node (parameterised)
        //   constructor: builds a node of a given kind and name.
        default_node(
            xml_node_kind       _kind,
            const xml_string_t& _name
        )
            : kind(_kind),
              name(_name),
              text(),
              attributes(),
              children(),
              parent(nullptr)
        {}
    };


    // default_document
    //   struct: internal in-memory document used by the default
    // backend. Owns the root node and carries the prolog fields.
    struct default_document
    {
        std::unique_ptr<default_node> root;
        xml_string_t                  version;
        xml_string_t                  encoding;
        xml_standalone                standalone;

        // default_document (default)
        //   constructor: empty document with default prolog and
        // no root element.
        default_document()
            : root(),
              version(D_XML_DEFAULT_VERSION),
              encoding(D_XML_DEFAULT_ENCODING),
              standalone(xml_standalone::unspecified)
        {}
    };

NS_END  // internal


// xml_default_backend
//   struct: bundled in-memory backend. Satisfies `is_xml_backend`
// (carries `backend_tag`) and `is_xml_backend_complete` (exposes
// every nested type alias and `make_document`).
struct xml_default_backend
{
    using backend_tag    = xml_default_backend_tag;
    using node_type      = internal::default_node;
    using attribute_type = xml_attribute;
    using document_type  = internal::default_document;

    // make_document
    //   function: factory for an empty default-backend document.
    static document_type
    make_document()
    {
        return document_type();
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III.   TEMPLATED NODE                                     ///
///////////////////////////////////////////////////////////////////////////////

// xml_node
//   class: templated facade over a backend node handle. The
// primary template targets the default backend's `default_node`;
// adapters for other libraries should specialise this template
// for their own backend type. Methods are kept structurally
// compatible with the trait detection in
// `xml_template_traits.hpp`.
template<typename _Backend>
class xml_node
{
public:
    using backend_type   = _Backend;
    using node_type      = typename _Backend::node_type;
    using attribute_type = typename _Backend::attribute_type;
    using size_type      = xml_size_t;

    // xml_node (default)
    //   constructor: empty handle, points to no backend node.
    xml_node()
        : m_node(nullptr)
    {}

    // xml_node (handle)
    //   constructor: wraps an existing backend node pointer.
    // The node is non-owning; lifetime belongs to the document.
    explicit xml_node(
        node_type* _node
    )
        : m_node(_node)
    {}

    // backend_handle
    //   function: returns the underlying backend node pointer.
    // Required by adapter code that needs to drop down to the
    // native API.
    node_type*
    backend_handle() const
    {
        return m_node;
    }

    // valid
    //   function: true if this facade refers to a backend node.
    bool
    valid() const
    {
        return (m_node != nullptr);
    }

    // ---------------------------------------------------------------
    //  accessors
    // ---------------------------------------------------------------

    // name
    //   function: returns the element / attribute / PI target
    // name. Empty for kinds that have no name (text, comment).
    const xml_string_t&
    name() const
    {
        return m_node->name;
    }

    // kind
    //   function: returns the node-kind discriminator.
    xml_node_kind
    kind() const
    {
        return m_node->kind;
    }

    // text
    //   function: returns the immediate text content. For an
    // element with multiple text children, returns the first
    // child's text; richer concatenation is provided by the
    // free function `collect_text`.
    const xml_string_t&
    text() const
    {
        return m_node->text;
    }

    // attributes
    //   function: returns a const reference to the attribute
    // container, suitable for range-for iteration.
    const std::vector<attribute_type>&
    attributes() const
    {
        return m_node->attributes;
    }

    // attribute_count
    //   function: returns the number of attributes on this node.
    size_type
    attribute_count() const
    {
        return m_node->attributes.size();
    }

    // child_count
    //   function: returns the number of direct children.
    size_type
    child_count() const
    {
        return m_node->children.size();
    }

    // parent
    //   function: returns a facade wrapping the parent node, or
    // an empty facade if this is the document root.
    xml_node
    parent() const
    {
        return xml_node(m_node->parent);
    }

    // ---------------------------------------------------------------
    //  attribute lookup / mutation
    // ---------------------------------------------------------------

    // find_attribute
    //   function: returns a pointer to the named attribute, or
    // nullptr if not present.
    const attribute_type*
    find_attribute(
        const xml_string_t& _name
    ) const
    {
        for (const attribute_type& a : m_node->attributes)
        {
            if (a.name == _name)
            {
                return &a;
            }
        }

        return nullptr;
    }

    // set_attribute
    //   function: sets or replaces an attribute. Returns true
    // if a new attribute was added, false if an existing one
    // was overwritten.
    bool
    set_attribute(
        const xml_string_t& _name,
        const xml_string_t& _value
    )
    {
        for (attribute_type& a : m_node->attributes)
        {
            if (a.name == _name)
            {
                a.value = _value;

                return false;
            }
        }

        m_node->attributes.push_back(
            attribute_type(_name, _value));

        return true;
    }

    // remove_attribute
    //   function: removes a named attribute. Returns true if an
    // attribute was removed, false otherwise.
    bool
    remove_attribute(
        const xml_string_t& _name
    )
    {
        for (auto it  = m_node->attributes.begin();
             it      != m_node->attributes.end();
             ++it)
        {
            if (it->name == _name)
            {
                m_node->attributes.erase(it);

                return true;
            }
        }

        return false;
    }

    // ---------------------------------------------------------------
    //  child traversal / mutation
    // ---------------------------------------------------------------

    // first_child
    //   function: returns a facade wrapping the first child, or
    // an empty facade if the node has no children.
    xml_node
    first_child() const
    {
        if (m_node->children.empty())
        {
            return xml_node();
        }

        return xml_node(m_node->children.front().get());
    }

    // find_child
    //   function: returns a facade wrapping the first child
    // matching `_name`, or an empty facade if none exists.
    xml_node
    find_child(
        const xml_string_t& _name
    ) const
    {
        for (const std::unique_ptr<node_type>& c : m_node->children)
        {
            if (c->name == _name)
            {
                return xml_node(c.get());
            }
        }

        return xml_node();
    }

    // add_child
    //   function: appends a new element child with the given
    // name and returns a facade wrapping it.
    xml_node
    add_child(
        const xml_string_t& _name
    )
    {
        std::unique_ptr<node_type> child(
            new node_type(xml_node_kind::element, _name));
        child->parent = m_node;

        node_type* raw = child.get();
        m_node->children.push_back(std::move(child));

        return xml_node(raw);
    }

    // remove_child
    //   function: removes the first child with the given name.
    // Returns true on success.
    bool
    remove_child(
        const xml_string_t& _name
    )
    {
        for (auto it  = m_node->children.begin();
             it      != m_node->children.end();
             ++it)
        {
            if ((*it)->name == _name)
            {
                m_node->children.erase(it);

                return true;
            }
        }

        return false;
    }

    // set_text
    //   function: replaces this node's immediate text content.
    void
    set_text(
        const xml_string_t& _text
    )
    {
        m_node->text = _text;

        return;
    }

private:
    node_type* m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   TEMPLATED DOCUMENT                                  ///
///////////////////////////////////////////////////////////////////////////////

// xml_document
//   class: templated facade over a backend document. Owns the
// backend's document instance by value and exposes the prolog
// plus parse / write / save. Adapters for other libraries should
// specialise this template for their own document type.
template<typename _Backend>
class xml_document
{
public:
    using backend_type   = _Backend;
    using node_type      = typename _Backend::node_type;
    using document_type  = typename _Backend::document_type;
    using node_facade    = xml_node<_Backend>;

    // xml_document (default)
    //   constructor: builds an empty document via the backend
    // factory.
    xml_document()
        : m_doc(_Backend::make_document())
    {}

    // xml_document (root name)
    //   constructor: builds a document and immediately attaches
    // a root element with the given name.
    explicit xml_document(
        const xml_string_t& _root_name
    )
        : m_doc(_Backend::make_document())
    {
        m_doc.root.reset(new node_type(
            xml_node_kind::element, _root_name));
    }

    // ---------------------------------------------------------------
    //  prolog accessors
    // ---------------------------------------------------------------

    // version
    //   function: returns the XML version string.
    const xml_string_t&
    version() const
    {
        return m_doc.version;
    }

    // encoding
    //   function: returns the encoding string (e.g. "UTF-8").
    const xml_string_t&
    encoding() const
    {
        return m_doc.encoding;
    }

    // standalone
    //   function: returns the standalone tri-state.
    xml_standalone
    standalone() const
    {
        return m_doc.standalone;
    }

    // set_version / set_encoding / set_standalone
    //   function: prolog mutators.
    void set_version(const xml_string_t& _v)
    {
        m_doc.version = _v;

        return;
    }

    void set_encoding(const xml_string_t& _e)
    {
        m_doc.encoding = _e;

        return;
    }

    void set_standalone(xml_standalone _s)
    {
        m_doc.standalone = _s;

        return;
    }

    // ---------------------------------------------------------------
    //  root access
    // ---------------------------------------------------------------

    // root_element
    //   function: returns a node facade wrapping the root
    // element. Empty if no root has been attached.
    node_facade
    root_element() const
    {
        return node_facade(m_doc.root.get());
    }

    // document_element
    //   function: DOM-compatible alias for `root_element`.
    node_facade
    document_element() const
    {
        return root_element();
    }

    // set_root
    //   function: replaces the root element with a new element
    // of the given name. Returns a facade wrapping the new root.
    node_facade
    set_root(
        const xml_string_t& _name
    )
    {
        m_doc.root.reset(new node_type(
            xml_node_kind::element, _name));

        return root_element();
    }

    // ---------------------------------------------------------------
    //  I/O
    // ---------------------------------------------------------------

    // parse
    //   function: ingests serialised XML. The default-backend
    // implementation is intentionally minimal; full-featured
    // parsing is the job of an adapted library backend. Returns
    // true on (apparent) success.
    bool
    parse(
        const xml_string_t& _xml
    )
    {
        // default backend: not a real parser. Subclass / specialise
        // this template, or use a libxml++/pugixml backend, to get
        // a complete implementation.
        (void)_xml;

        return false;
    }

    // write
    //   function: serialises the document to a string. The
    // default implementation produces a minimal well-formed
    // document; adapter specialisations should defer to the
    // underlying library.
    xml_string_t
    write() const
    {
        xml_string_t out;
        out.reserve(256);

        out += "<?xml version=\"";
        out += m_doc.version;
        out += "\" encoding=\"";
        out += m_doc.encoding;
        out += "\"";

        if (m_doc.standalone == xml_standalone::yes)
        {
            out += " standalone=\"yes\"";
        }
        else if (m_doc.standalone == xml_standalone::no)
        {
            out += " standalone=\"no\"";
        }

        out += "?>\n";

        if (m_doc.root)
        {
            write_node_recursive(out, *m_doc.root, 0);
        }

        return out;
    }

    // save
    //   function: serialises to a file path via write(). The
    // default-backend implementation is a stub returning false;
    // adapter specialisations should perform actual file I/O.
    bool
    save(
        const xml_string_t& _path
    ) const
    {
        // default backend: no I/O. Specialise for backends that
        // do real disk writes.
        (void)_path;

        return false;
    }

private:
    // write_node_recursive
    //   function: serialises a default-backend node into an
    // accumulating string. Internal-only helper.
    static void
    write_node_recursive(
        xml_string_t&    _out,
        const node_type& _n,
        int              _depth
    )
    {
        // indent
        for (int i = 0; i < _depth; ++i)
        {
            _out += D_XML_DEFAULT_INDENT;
        }

        _out += "<";
        _out += _n.name;

        // attributes
        for (const xml_attribute& a : _n.attributes)
        {
            _out += " ";
            _out += a.name;
            _out += "=\"";
            _out += a.value;
            _out += "\"";
        }

        // empty-element shorthand when no children and no text
        if ( (_n.children.empty()) &&
             (_n.text.empty()) )
        {
            _out += "/>\n";

            return;
        }

        _out += ">";

        if (!_n.text.empty())
        {
            _out += _n.text;
        }

        if (!_n.children.empty())
        {
            _out += "\n";

            for (const std::unique_ptr<node_type>& c : _n.children)
            {
                write_node_recursive(_out, *c, _depth + 1);
            }

            for (int i = 0; i < _depth; ++i)
            {
                _out += D_XML_DEFAULT_INDENT;
            }
        }

        _out += "</";
        _out += _n.name;
        _out += ">\n";

        return;
    }

    document_type m_doc;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   FREE FUNCTIONS                                       ///
///////////////////////////////////////////////////////////////////////////////

// make_attribute
//   function: convenience factory for an `xml_attribute`.
inline xml_attribute
make_attribute(
    const xml_string_t& _name,
    const xml_string_t& _value
)
{
    return xml_attribute(_name, _value);
}


// make_default_document
//   function: convenience factory for a default-backend
// document, optionally pre-attached with a root element.
inline xml_document<xml_default_backend>
make_default_document(
    const xml_string_t& _root_name = xml_string_t()
)
{
    if (_root_name.empty())
    {
        return xml_document<xml_default_backend>();
    }

    return xml_document<xml_default_backend>(_root_name);
}


// find_child_by_path
//   function: walks a slash-delimited path of element names from
// `_start`, returning the matched node or an empty facade. Path
// segments separated by `/`; leading and trailing separators
// are ignored. Implements a minimal XPath-like subset, sufficient
// for typical document traversal.
template<typename _Backend>
inline xml_node<_Backend>
find_child_by_path(
    xml_node<_Backend>  _start,
    const xml_string_t& _path
)
{
    xml_node<_Backend> current = _start;
    xml_size_t         pos     = 0;

    while (pos < _path.size())
    {
        // skip leading separators
        while ( (pos < _path.size()) &&
                (_path[pos] == '/') )
        {
            ++pos;
        }

        if (pos >= _path.size())
        {
            break;
        }

        // find next segment
        xml_size_t next = _path.find('/', pos);

        if (next == xml_string_t::npos)
        {
            next = _path.size();
        }

        xml_string_t segment = _path.substr(pos, next - pos);

        if (!current.valid())
        {
            return xml_node<_Backend>();
        }

        current = current.find_child(segment);
        pos     = next;
    }

    return current;
}


NS_END  // djinterp


#endif  // DJINTERP_XML_TEMPLATE_
