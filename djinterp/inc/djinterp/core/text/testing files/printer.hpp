/******************************************************************************
* djinterp [text]                                                  printer.hpp
*
*   Common, protocol-generic pretty-printer for document node trees. The
* printer walks ANY type that satisfies the XML node protocol detected in
* `xml.hpp` (a name accessor plus an iterable child accessor) and emits a
* serialised, optionally indented form to a caller-supplied sink. It is
* "common" in the sense the previous design notes intended: the parts
* that do not vary by format -- tree traversal, indentation, the output
* destination, and the rendering options -- live here once, while the
* parts that DO vary by format -- escaping rules and the empty-element
* convention -- are supplied by a `_Policy` (default `xml_print_policy`).
*
*   GENERIC OVER THE NODE PROTOCOL:
*   The printer never names a concrete node class. Every read goes through
* the structural traits: `name()` or `get_name()`, `children()` or
* `get_children()`, `attributes()` or `get_attributes()`, `text()` or
* `get_text()`, `kind()` or `node_type()`. A type produced by our own
* `xml_template.hpp`, by pugixml, by libxml++, or by a hand-rolled struct
* prints identically as long as it exposes the protocol. Dispatch between
* the two naming conventions is resolved at compile time with
* `if constexpr`, so there is no runtime cost and no virtual indirection.
*
*   DESTINATION:
*   The sink is a template parameter whose only requirement is to be
* callable as `sink(const char*, std::size_t)`. Two adapters are provided
* -- `string_sink` (append into an `xml_string_t`) and `stream_sink`
* (write into a `std::ostream`) -- so file, console, string, and in-memory
* buffer destinations are all just different sinks, not different
* printers. Any callable with that signature (including a functional-style
* consumer) works directly.
*
*   OPTIONS:
*   Rendering options are a small typed struct (`print_options`: indent
* string, newline, pretty flag) rather than a type-erased key/value bag,
* because the printer's own knobs are fixed and benefit from being
* checked and inlined. A dynamic option set (kv_pair / option_set) can
* populate a `print_options` at the boundary if open-ended configuration
* is wanted.
*
*   Requires C++17 (`if constexpr`); self-suppresses below it.
*
* path:      /inc/djinterp/core/text/printer.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PRINT OPTIONS
      -------------
      a. print_options

II.   OUTPUT SINKS
      ------------
      a. string_sink
      b. stream_sink

III.  PROTOCOL ACCESS (internal)
      --------------------------
      a. to_string_value
      b. read_name,  read_text,  read_kind
      c. read_attributes,        read_children
      d. read_attribute_name,    read_attribute_value

IV.   PRINT POLICY
      ------------
      a. element_form
      b. xml_print_policy
      c. boxed_print_policy
      d. box_policy, xml_policy

V.    DOCUMENT PRINTER
      ----------------
      a. document_printer<_Sink, _Policy>

VI.   CONVENIENCE FUNCTIONS
      ---------------------
      a. print_node
      b. node_to_string
      c. render

VII.  FILE PRINTER
      ------------
      a. file_printer
      b. print_document_to_file
*/

#ifndef DJINTERP_TEXT_PRINTER_
#define DJINTERP_TEXT_PRINTER_ 1

// std
#include <cstddef>
#include <memory>
#include <string>
#include <ostream>
#include <fstream>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./writer.hpp"           // output sinks (string_sink, stream_sink)
#include "./xml/xml.hpp"          // node protocol traits + xml_node_kind


// `if constexpr` is the spine of the accessor dispatch; below C++17 this
// module contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   PRINT OPTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

// print_options
//   struct: the format-agnostic rendering knobs. `indent` is the string
// emitted once per nesting level, `newline` is the line terminator, and
// `pretty` gates both -- when false the document is emitted compactly on a
// single line and `indent` / `newline` are ignored.
struct print_options
{
    xml_string_t indent;     // per-level indent string
    xml_string_t newline;    // line terminator
    bool         pretty;     // indent + line breaks when true

    // print_options (default)
    //   constructor: two-space indent, LF newline, pretty-printing on.
    print_options()
        : indent(D_XML_DEFAULT_INDENT),
          newline("\n"),
          pretty(true)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                  II.   OUTPUT SINKS                                     ///
///////////////////////////////////////////////////////////////////////////////

//   The output sinks (`string_sink`, `stream_sink`) now live in
// `writer.hpp` -- the generic IO foundation this header includes -- because
// a sink is an IO concept, not a printing-a-document concept. They remain in
// the `djinterp` namespace, so existing references (e.g. in `node_to_string`
// below) are unchanged. Any callable `(const char*, std::size_t)` is equally
// a valid sink.


///////////////////////////////////////////////////////////////////////////////
///                III.   PROTOCOL ACCESS (internal)                        ///
///////////////////////////////////////////////////////////////////////////////

//   These helpers read the node protocol uniformly, hiding the short-form
// vs getter-form naming split behind compile-time dispatch. Each is valid
// only when the corresponding accessor exists; callers guard with the same
// traits (or with the node-level contract enforced by document_printer).

NS_INTERNAL

    // to_string_value
    //   function: coerces whatever an accessor returns (a string, a
    // string_view, a const char*, a reference to any of these) into an
    // owned xml_string_t for emission or escaping.
    template<typename _Source>
    inline xml_string_t
    to_string_value(
        const _Source& _source
    )
    {
        return xml_string_t(_source);
    }


    // read_name
    //   function: the node's name via name() or get_name().
    template<typename _Node>
    inline xml_string_t
    read_name(
        const _Node& _node
    )
    {
        if constexpr (has_name_method<_Node>::value)
        {
            return to_string_value(_node.name());
        }
        else
        {
            return to_string_value(_node.get_name());
        }
    }


    // read_text
    //   function: the node's text content via text() or get_text().
    template<typename _Node>
    inline xml_string_t
    read_text(
        const _Node& _node
    )
    {
        if constexpr (has_text_method<_Node>::value)
        {
            return to_string_value(_node.text());
        }
        else
        {
            return to_string_value(_node.get_text());
        }
    }


    // read_kind
    //   function: the node's kind via kind() or node_type(), normalised to
    // xml_node_kind. Backends whose discriminator is a distinct enum are
    // assumed numerically compatible with xml_node_kind.
    template<typename _Node>
    inline xml_node_kind
    read_kind(
        const _Node& _node
    )
    {
        if constexpr (has_kind_method<_Node>::value)
        {
            return static_cast<xml_node_kind>(_node.kind());
        }
        else
        {
            return static_cast<xml_node_kind>(_node.node_type());
        }
    }


    // read_attributes
    //   function: the node's attribute range via attributes() or
    // get_attributes().
    template<typename _Node>
    inline decltype(auto)
    read_attributes(
        const _Node& _node
    )
    {
        if constexpr (has_attributes_method<_Node>::value)
        {
            return _node.attributes();
        }
        else
        {
            return _node.get_attributes();
        }
    }


    // read_children
    //   function: the node's child range via children() or get_children().
    template<typename _Node>
    inline decltype(auto)
    read_children(
        const _Node& _node
    )
    {
        if constexpr (has_children_method<_Node>::value)
        {
            return _node.children();
        }
        else
        {
            return _node.get_children();
        }
    }


    // read_attribute_name
    //   function: an attribute's name via name() or get_name().
    template<typename _Attribute>
    inline xml_string_t
    read_attribute_name(
        const _Attribute& _attribute
    )
    {
        if constexpr (has_name_method<_Attribute>::value)
        {
            return to_string_value(_attribute.name());
        }
        else
        {
            return to_string_value(_attribute.get_name());
        }
    }


    // read_attribute_value
    //   function: an attribute's value via value() or get_value().
    template<typename _Attribute>
    inline xml_string_t
    read_attribute_value(
        const _Attribute& _attribute
    )
    {
        if constexpr (has_value_method<_Attribute>::value)
        {
            return to_string_value(_attribute.value());
        }
        else
        {
            return to_string_value(_attribute.get_value());
        }
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                  IV.   PRINT POLICY                                     ///
///////////////////////////////////////////////////////////////////////////////

// element_form
//   enum: how an element is closed -- the one place markup dialects diverge.
//     paired       <name>...</name>   (or <name></name> when empty)
//     self_closing <name/>            (XML empty elements)
//     voidlike     <name>             (HTML void elements: no content, no
//                                       closing tag, and no trailing slash)
// A policy returns one of these from `form_of`; the printer renders to match.
enum class element_form
{
    paired,
    self_closing,
    voidlike
};


//   A PRINT POLICY supplies the three things a markup dialect varies:
// `escape_text` and `escape_attribute` (string -> escaped string) and
// `form_of(name, is_empty) -> element_form`. Methods may be static or
// instance (they are called on an instance). A policy can be a TYPE chosen at
// compile time (zero cost, inlined) or, via `boxed_print_policy` below, a
// VALUE chosen at runtime -- both satisfy the same contract and plug into the
// same `document_printer`.


// xml_print_policy
//   struct: the default format policy -- XML serialisation rules. Supplies
// text and attribute escaping and the empty-element convention. Methods
// are stateless and callable on an instance, so a custom policy may use
// either static or non-static members. Swap this parameter to retarget the
// printer at another markup dialect (e.g. an HTML policy with void-element
// rules).
struct xml_print_policy
{
    // escape_text
    //   function: escapes the three characters that are significant in XML
    // character data: &, <, and >.
    static xml_string_t
    escape_text(
        const xml_string_t& _in
    )
    {
        xml_string_t _out;
        _out.reserve(_in.size());

        for (char _c : _in)
        {
            switch (_c)
            {
                case '&':  _out += "&amp;"; break;
                case '<':  _out += "&lt;";  break;
                case '>':  _out += "&gt;";  break;
                default:   _out += _c;      break;
            }
        }

        return _out;
    }


    // escape_attribute
    //   function: escapes character data plus the double quote, for values
    // emitted inside double-quoted attributes.
    static xml_string_t
    escape_attribute(
        const xml_string_t& _in
    )
    {
        xml_string_t _out;
        _out.reserve(_in.size());

        for (char _c : _in)
        {
            switch (_c)
            {
                case '&':  _out += "&amp;";  break;
                case '<':  _out += "&lt;";   break;
                case '>':  _out += "&gt;";   break;
                case '"':  _out += "&quot;"; break;
                default:   _out += _c;       break;
            }
        }

        return _out;
    }


    // form_of
    //   function: how to close an element. In XML an empty element self-closes
    // (`<tag/>`) and a non-empty one is paired; the name is irrelevant.
    static element_form
    form_of(
        const xml_string_t& /*_name*/,
        bool                _is_empty
    )
    {
        return _is_empty ? element_form::self_closing
                         : element_form::paired;
    }
};


// boxed_print_policy
//   class: a type-erased print policy -- the RUNTIME face of the contract.
// It holds any concrete policy behind a small interface and forwards
// `escape_text` / `escape_attribute` / `form_of` through it, so the format
// can be chosen as a value at runtime. Because it satisfies the same contract
// as a concrete policy, `document_printer<Sink, boxed_print_policy>` and
// `render` accept it unchanged -- the only difference from a compile-time
// policy is one indirection per call. Uses the same shared concept/model
// erasure as `boxed_consumer` / `boxed_accumulator`.
class boxed_print_policy
{
public:
    template<typename _Policy,
             typename = typename std::enable_if<
                 !std::is_same<clean_t<_Policy>, boxed_print_policy>::value>::type>
    boxed_print_policy(
        _Policy _policy
    )
        : m_self(std::make_shared<model_t<clean_t<_Policy>>>(
              static_cast<_Policy&&>(_policy)))
    {}

    D_NODISCARD xml_string_t
    escape_text(
        const xml_string_t& _in
    ) const
    {
        return m_self->escape_text(_in);
    }

    D_NODISCARD xml_string_t
    escape_attribute(
        const xml_string_t& _in
    ) const
    {
        return m_self->escape_attribute(_in);
    }

    D_NODISCARD element_form
    form_of(
        const xml_string_t& _name,
        bool                _is_empty
    ) const
    {
        return m_self->form_of(_name, _is_empty);
    }

private:
    // concept_t -- the erased policy interface.
    struct concept_t
    {
        virtual ~concept_t() = default;

        virtual xml_string_t escape_text(const xml_string_t&)      const = 0;
        virtual xml_string_t escape_attribute(const xml_string_t&) const = 0;
        virtual element_form form_of(const xml_string_t&, bool)    const = 0;
    };

    // model_t -- a concrete policy wearing the interface.
    template<typename _Policy>
    struct model_t final : concept_t
    {
        explicit model_t(
            _Policy _policy
        )
            : m_policy(static_cast<_Policy&&>(_policy))
        {}

        xml_string_t
        escape_text(
            const xml_string_t& _in
        ) const override
        {
            return m_policy.escape_text(_in);
        }

        xml_string_t
        escape_attribute(
            const xml_string_t& _in
        ) const override
        {
            return m_policy.escape_attribute(_in);
        }

        element_form
        form_of(
            const xml_string_t& _name,
            bool                _is_empty
        ) const override
        {
            return m_policy.form_of(_name, _is_empty);
        }

        _Policy m_policy;
    };

    std::shared_ptr<const concept_t> m_self;
};


// box_policy
//   function: wrap any concrete policy as a `boxed_print_policy`.
template<typename _Policy>
D_NODISCARD inline boxed_print_policy
box_policy(
    _Policy _policy
)
{
    return boxed_print_policy(static_cast<_Policy&&>(_policy));
}


// xml_policy
//   function: the XML policy as a runtime value.
D_NODISCARD inline boxed_print_policy
xml_policy()
{
    return boxed_print_policy(xml_print_policy{});
}


///////////////////////////////////////////////////////////////////////////////
///                  V.   DOCUMENT PRINTER                                      ///
///////////////////////////////////////////////////////////////////////////////

// document_printer
//   class: the protocol-generic printer. `_Sink` is any callable
// `(const char*, std::size_t)`; `_Policy` supplies the format-specific
// escaping and empty-element rules. Construct with a sink (and optionally
// options and a policy), then call `print` / `operator()` with any node
// that satisfies the node protocol.
template<typename _Sink,
         typename _Policy = xml_print_policy>
class document_printer
{
public:
    using sink_type   = _Sink;
    using policy_type = _Policy;

    // document_printer (sink)
    //   constructor: bind a sink; default options and policy.
    explicit document_printer(
        _Sink _sink
    )
        : m_sink(static_cast<_Sink&&>(_sink))
        , m_options()
        , m_policy()
    {}

    // document_printer (sink, options)
    //   constructor: bind a sink and rendering options.
    document_printer(
        _Sink         _sink,
        print_options _options
    )
        : m_sink(static_cast<_Sink&&>(_sink))
        , m_options(static_cast<print_options&&>(_options))
        , m_policy()
    {}

    // document_printer (sink, options, policy)
    //   constructor: bind a sink, options, and an explicit policy instance.
    document_printer(
        _Sink         _sink,
        print_options _options,
        _Policy       _policy
    )
        : m_sink(static_cast<_Sink&&>(_sink))
        , m_options(static_cast<print_options&&>(_options))
        , m_policy(static_cast<_Policy&&>(_policy))
    {}

    // print
    //   function: serialise `_node` and its subtree to the bound sink.
    template<typename _Node>
    void
    print(
        const _Node& _node
    )
    {
        static_assert(
            is_xml_node<_Node>::value,
            "document_printer::print requires a type satisfying the XML node "
            "protocol (a name accessor and child traversal).");
        static_assert(
            ( has_children_method<_Node>::value ||
              has_get_children_method<_Node>::value ),
            "document_printer::print requires an iterable child accessor "
            "(children() or get_children()).");

        m_print(_node, 0);

        return;
    }

    // operator()
    //   function: the call face of a printer; forwards to print.
    template<typename _Node>
    void
    operator()(
        const _Node& _node
    )
    {
        print(_node);

        return;
    }

    // options
    //   function: the bound rendering options (mutable, for reuse).
    D_NODISCARD print_options&
    options()
    {
        return m_options;
    }

private:
    // m_put (literal)
    //   function: emit a NUL-terminated literal to the sink.
    void
    m_put(
        const char* _literal
    )
    {
        m_sink(_literal, std::char_traits<char>::length(_literal));

        return;
    }

    // m_put (string)
    //   function: emit an xml_string_t to the sink.
    void
    m_put(
        const xml_string_t& _string
    )
    {
        m_sink(_string.data(), _string.size());

        return;
    }

    // m_indent
    //   function: emit `_depth` indent strings, when pretty-printing.
    void
    m_indent(
        xml_size_t _depth
    )
    {
        if (!m_options.pretty)
        {
            return;
        }

        for (xml_size_t _i = 0; _i < _depth; ++_i)
        {
            m_put(m_options.indent);
        }

        return;
    }

    // m_newline
    //   function: emit the line terminator, when pretty-printing.
    void
    m_newline()
    {
        if (m_options.pretty)
        {
            m_put(m_options.newline);
        }

        return;
    }

    // m_print
    //   function: dispatch a node on its kind and serialise it.
    template<typename _Node>
    void
    m_print(
        const _Node& _node,
        xml_size_t   _depth
    )
    {
        const xml_node_kind _kind = m_kind_of(_node);

        switch (_kind)
        {
            case xml_node_kind::text:
            {
                m_indent(_depth);
                if constexpr (has_text_method<_Node>::value ||
                              has_get_text_method<_Node>::value)
                {
                    m_put(m_policy.escape_text(internal::read_text(_node)));
                }
                m_newline();
                break;
            }

            case xml_node_kind::comment:
            {
                m_indent(_depth);
                m_put("<!--");
                if constexpr (has_text_method<_Node>::value ||
                              has_get_text_method<_Node>::value)
                {
                    m_put(internal::read_text(_node));
                }
                m_put("-->");
                m_newline();
                break;
            }

            case xml_node_kind::cdata_section:
            {
                m_indent(_depth);
                m_put("<![CDATA[");
                if constexpr (has_text_method<_Node>::value ||
                              has_get_text_method<_Node>::value)
                {
                    m_put(internal::read_text(_node));
                }
                m_put("]]>");
                m_newline();
                break;
            }

            case xml_node_kind::element:
            default:
            {
                m_print_element(_node, _depth);
                break;
            }
        }

        return;
    }

    // m_print_element
    //   function: serialise an element node -- open tag, attributes, and
    // either a self-close, inline text, or an indented child block.
    template<typename _Node>
    void
    m_print_element(
        const _Node& _node,
        xml_size_t   _depth
    )
    {
        const xml_string_t _name = internal::read_name(_node);

        m_indent(_depth);
        m_put("<");
        m_put(_name);
        m_print_attributes(_node);

        // resolve content: element children take precedence over a text
        // accessor (the mixed-content case emits children and drops a
        // node-level text member, which is the documented v1 limitation).
        auto&&     _children = internal::read_children(_node);
        const bool _has_children =
            ( m_begin(_children) != m_end(_children) );

        xml_string_t _text;
        bool         _has_text = false;
        if constexpr (has_text_method<_Node>::value ||
                      has_get_text_method<_Node>::value)
        {
            if (!_has_children)
            {
                _text     = internal::read_text(_node);
                _has_text = !_text.empty();
            }
        }

        const bool         _empty = !_has_children && !_has_text;
        const element_form _form  = m_policy.form_of(_name, _empty);

        if (_form == element_form::voidlike)
        {
            // void element (HTML <br>, <img>, ...): open tag only -- no
            // content and no closing tag. Any stray children/text are dropped.
            m_put(">");
            m_newline();

            return;
        }

        if (_empty)
        {
            if (_form == element_form::self_closing)
            {
                m_put("/>");
            }
            else
            {
                m_put("></");
                m_put(_name);
                m_put(">");
            }
            m_newline();

            return;
        }

        m_put(">");

        if (_has_children)
        {
            m_newline();
            for (auto _it = m_begin(_children);
                 _it != m_end(_children);
                 ++_it)
            {
                m_print(*_it, _depth + 1);
            }
            m_indent(_depth);
            m_put("</");
            m_put(_name);
            m_put(">");
        }
        else
        {
            m_put(m_policy.escape_text(_text));
            m_put("</");
            m_put(_name);
            m_put(">");
        }

        m_newline();

        return;
    }

    // m_print_attributes
    //   function: emit ` name="value"` for each attribute, when the node
    // exposes an attribute container.
    template<typename _Node>
    void
    m_print_attributes(
        const _Node& _node
    )
    {
        if constexpr (has_attributes_method<_Node>::value ||
                      has_get_attributes_method<_Node>::value)
        {
            auto&& _attributes = internal::read_attributes(_node);
            for (const auto& _attribute : _attributes)
            {
                using attribute_type = clean_t<decltype(_attribute)>;
                static_assert(
                    is_xml_attribute<attribute_type>::value,
                    "document_printer: the attribute type yielded by the node "
                    "must expose a name and a value accessor.");

                m_put(" ");
                m_put(internal::read_attribute_name(_attribute));
                m_put("=\"");
                m_put(m_policy.escape_attribute(
                    internal::read_attribute_value(_attribute)));
                m_put("\"");
            }
        }

        return;
    }

    // m_kind_of
    //   function: the node's kind, defaulting to `element` when the node
    // exposes no kind accessor (the minimum protocol omits it).
    template<typename _Node>
    static xml_node_kind
    m_kind_of(
        const _Node& _node
    )
    {
        if constexpr (has_kind_method<_Node>::value ||
                      has_get_kind_method<_Node>::value)
        {
            return internal::read_kind(_node);
        }
        else
        {
            return xml_node_kind::element;
        }
    }

    // m_begin / m_end
    //   function: ADL-aware range access so any iterable child or attribute
    // range works, not only those with member begin()/end().
    template<typename _Range>
    static auto
    m_begin(
        _Range&& _range
    )
        -> decltype(std::begin(_range))
    {
        using std::begin;
        return begin(_range);
    }

    template<typename _Range>
    static auto
    m_end(
        _Range&& _range
    )
        -> decltype(std::end(_range))
    {
        using std::end;
        return end(_range);
    }

    _Sink         m_sink;
    print_options m_options;
    _Policy       m_policy;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   CONVENIENCE FUNCTIONS                              ///
///////////////////////////////////////////////////////////////////////////////

// print_node
//   function: print `_node` to `_sink` with `_options`, constructing a
// throwaway printer. The default policy (xml_print_policy) is used.
template<typename _Sink,
         typename _Node>
inline void
print_node(
    _Sink         _sink,
    const _Node&  _node,
    print_options _options = print_options()
)
{
    document_printer<_Sink> _printer(
        static_cast<_Sink&&>(_sink),
        static_cast<print_options&&>(_options));
    _printer.print(_node);

    return;
}


// node_to_string
//   function: serialise `_node` to a freshly allocated xml_string_t with
// the default policy.
template<typename _Node>
D_NODISCARD inline xml_string_t
node_to_string(
    const _Node&  _node,
    print_options _options = print_options()
)
{
    xml_string_t _out;
    document_printer<string_sink> _printer(
        string_sink(_out),
        static_cast<print_options&&>(_options));
    _printer.print(_node);

    return _out;
}


// render
//   function: serialise `_node` to a string with an explicit policy. The
// policy may be a concrete type (compile-time, zero cost) or a
// `boxed_print_policy` (runtime-chosen) -- the same call covers both. This is
// the seam where compile-time and runtime format selection meet.
template<typename _Node,
         typename _Policy>
D_NODISCARD inline xml_string_t
render(
    const _Node&  _node,
    _Policy       _policy,
    print_options _options = print_options()
)
{
    xml_string_t _out;
    document_printer<string_sink, _Policy> _printer(
        string_sink(_out),
        static_cast<print_options&&>(_options),
        static_cast<_Policy&&>(_policy));
    _printer.print(_node);

    return _out;
}


///////////////////////////////////////////////////////////////////////////////
///                  VII.   FILE PRINTER                                    ///
///////////////////////////////////////////////////////////////////////////////

// file_printer
//   class: a byte sink targeting a file, with a convenience to print a whole
// document into it. As a sink it is text-agnostic -- `operator()(const char*,
// std::size_t)` writes raw bytes -- so it composes with anything that writes
// to a sink. `print(node)` serialises a document to the file via
// `document_printer`. The file is opened in binary mode so bytes pass through
// verbatim; check `good()` after construction.
class file_printer
{
public:
    explicit file_printer(
        const std::string& _path
    )
        : m_file(_path, std::ios::out | std::ios::binary)
    {}

    // operator() -- the sink face: write raw bytes to the file.
    void
    operator()(
        const char* _data,
        std::size_t _size
    )
    {
        m_file.write(_data, static_cast<std::streamsize>(_size));

        return;
    }

    // print -- serialise a document (any node-protocol type) into the file.
    template<typename _Node>
    void
    print(
        const _Node&  _node,
        print_options _options = print_options()
    )
    {
        document_printer<stream_sink> _printer(
            stream_sink(m_file),
            static_cast<print_options&&>(_options));
        _printer.print(_node);

        return;
    }

    // good -- whether the underlying file stream is healthy.
    D_NODISCARD bool
    good() const
    {
        return m_file.good();
    }

private:
    std::ofstream m_file;
};


// print_document_to_file
//   function: serialise `_node` to the file at `_path` with the default
// policy, in one call.
template<typename _Node>
inline void
print_document_to_file(
    const std::string& _path,
    const _Node&       _node,
    print_options      _options = print_options()
)
{
    file_printer _printer(_path);
    _printer.print(_node, static_cast<print_options&&>(_options));

    return;
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEXT_PRINTER_
