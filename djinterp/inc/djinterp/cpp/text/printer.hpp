/*******************************************************************************
* djinterp [cpp]                                                     printer.hpp
*
*   Common, protocol-generic pretty-printer for document node trees. The
* printer walks ANY type that satisfies the XML node protocol detected in
* `xml.hpp` (a name accessor plus an iterable child accessor) and emits a
* serialised, optionally indented form to a caller-supplied sink. It is
* "common" in the sense the previous design notes intended: the parts
* that do not vary by format -- tree traversal, indentation, the output
* destination, and the rendering options -- live here once, while the
* parts that DO vary by format -- escaping rules and the empty-element
* convention -- are supplied by a `Policy` (default `DXmlPrintPolicy`).
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
* callable as `sink(const char*, std::size_t)`. writer.hpp provides three
* -- `DStringSink` (append into an `DXmlString`), `DStreamSink` (write
* into a `std::ostream`) and `output_stream_sink` (write into any output
* stream) -- so file, console, string, and in-memory buffer destinations are
* all just different sinks, not different printers. Any callable with that
* signature (including a functional-style consumer) works directly.
*   Those three write through an output stream (cpp/util/output), which
* keeps a failed write: a sink returns nothing, so its ok() is where a
* failure shows. `DFilePrinter` writes through one it opens, and its
* print(), close() and print_document_to_file() report whether the whole
* document reached the file.
*
*   OPTIONS:
*   Rendering options are a small typed struct (`DPrintOptions`: indent
* string, newline, pretty flag) rather than a type-erased key/value bag,
* because the printer's own knobs are fixed and benefit from being
* checked and inlined. A dynamic option set (DKvPair / option_set) can
* populate a `DPrintOptions` at the boundary if open-ended configuration
* is wanted.
*
*   Requires C++17 (`if constexpr`); self-suppresses below it.
*
*
* path:      /inc/djinterp/cpp/text/printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PRINT OPTIONS
      -------------
      a. DPrintOptions

II.   OUTPUT SINKS
      ------------
      a. DStringSink
      b. DStreamSink

III.  PROTOCOL ACCESS (internal)
      --------------------------
      a. to_string_value
      b. read_name,  read_text,  read_kind
      c.    read_attributes,        read_children
            d. read_attribute_name,    read_attribute_value

IV.   PRINT POLICY
      ------------
      a. DElementForm
      b. DXmlPrintPolicy
      c.    DBoxedPrintPolicy
            d. box_policy, xml_policy

V.    DOCUMENT PRINTER
      ----------------
      a. DDocumentPrinter<Sink, Policy>

VI.   CONVENIENCE FUNCTIONS
      ---------------------
      a. print_node
      b. node_to_string
      c.    render

VII.  FILE PRINTER
      ------------
      a. DFilePrinter
      b. print_document_to_file
*/

#ifndef DJINTERP_TEXT_PRINTER_HPP
#define DJINTERP_TEXT_PRINTER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <memory>       // std::shared_ptr: DBoxedPrintPolicy
#include <ostream>      // std::ostream
#include <string>       // std::string, std::char_traits
#include <type_traits>  // std::enable_if, std::is_same
// djinterp
#include "../djinterp.hpp"                   // framework root
#include "./writer.hpp"                      // the sinks: DStringSink,
                                             // DStreamSink, output_stream_sink
#include "./xml/xml.hpp"                     // node protocol traits,
                                             // DXmlNodeKind
#include "../util/output/output_stream.hpp"  // DOutputStream: DFilePrinter


// `if constexpr` is the spine of the accessor dispatch; below C++17 this
// module contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   PRINT OPTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

// DPrintOptions
//   struct: the format-agnostic rendering knobs. `indent` is the string
// emitted once per nesting level, `newline` is the line terminator, and
// `pretty` gates both -- when false the document is emitted compactly on a
// single line and `indent` / `newline` are ignored.
struct DPrintOptions
{
    DXmlString indent;     // per-level indent string
    DXmlString   newline;    // line terminator
    bool         pretty;     // indent + line breaks when true

    // DPrintOptions (default)
    //   constructor: two-space indent, LF newline, pretty-printing on.
    DPrintOptions()
        : indent(D_XML_DEFAULT_INDENT),
          newline("\n"),
          pretty(true)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                  II.   OUTPUT SINKS                                     ///
///////////////////////////////////////////////////////////////////////////////

//   The output sinks (`DStringSink`, `DStreamSink`, `output_stream_sink`) live
// in `writer.hpp` -- the generic IO foundation this header includes -- because
// a sink is an IO concept, not a printing-a-document concept. They remain in
// the `djinterp` namespace, so existing references (e.g. in `node_to_string`
// below) are unchanged. Any callable `(const char*, std::size_t)` is equally
// a valid sink; each of the three keeps a failed write in its stream, read
// with ok().


///////////////////////////////////////////////////////////////////////////////
///                III.   PROTOCOL ACCESS (internal)                        ///
///////////////////////////////////////////////////////////////////////////////

//   These helpers read the node protocol uniformly, hiding the short-form
// vs getter-form naming split behind compile-time dispatch. Each is valid
// only when the corresponding accessor exists; callers guard with the same
// traits (or with the node-level contract enforced by DDocumentPrinter).

NS_INTERNAL

    // to_string_value
    //   function: coerces whatever an accessor returns (a string, a
    // string_view, a const char*, a reference to any of these) into an
    // owned DXmlString for emission or escaping.
    template<typename Source>
    inline
    DXmlString to_string_value(
        const Source& _source
    )
    {
        return DXmlString(_source);
    }


    // read_name
    //   function: the node's name via name() or get_name().
    template<typename Node>
    inline
    DXmlString read_name(
        const Node& _node
    )
    {
        if constexpr (has_name_method<Node>::value)
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
    template<typename Node>
    inline
    DXmlString read_text(
        const Node& _node
    )
    {
        if constexpr (has_text_method<Node>::value)
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
    // DXmlNodeKind. Backends whose discriminator is a distinct enum are
    // assumed numerically compatible with DXmlNodeKind.
    template<typename Node>
    inline
    DXmlNodeKind read_kind(
        const Node& _node
    )
    {
        if constexpr (has_kind_method<Node>::value)
        {
            return static_cast<DXmlNodeKind>(_node.kind());
        }
        else
        {
            return static_cast<DXmlNodeKind>(_node.node_type());
        }
    }


    // read_attributes
    //   function: the node's attribute range via attributes() or
    // get_attributes().
    template<typename Node>
    inline
    decltype(auto) read_attributes(
        const Node& _node
    )
    {
        if constexpr (has_attributes_method<Node>::value)
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
    template<typename Node>
    inline
    decltype(auto) read_children(
        const Node& _node
    )
    {
        if constexpr (has_children_method<Node>::value)
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
    template<typename Attribute>
    inline
    DXmlString read_attribute_name(
        const Attribute& _attribute
    )
    {
        if constexpr (has_name_method<Attribute>::value)
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
    template<typename Attribute>
    inline
    DXmlString read_attribute_value(
        const Attribute& _attribute
    )
    {
        if constexpr (has_value_method<Attribute>::value)
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

// DElementForm
//   enum: how an element is closed -- the one place markup dialects diverge.
//     paired       <name>...</name>   (or <name></name> when empty)
//     self_closing <name/>            (XML empty elements)
//     voidlike     <name>             (HTML void elements: no content, no
//                                       closing tag, and no trailing slash)
// A policy returns one of these from `form_of`; the printer renders to match.
enum class DElementForm
{
    paired,
    self_closing,
    voidlike
};


//   A PRINT POLICY supplies the three things a markup dialect varies:
// `escape_text` and `escape_attribute` (string -> escaped string) and
// `form_of(name, is_empty) -> DElementForm`. Methods may be static or
// instance (they are called on an instance). A policy can be a TYPE chosen at
// compile time (zero cost, inlined) or, via `DBoxedPrintPolicy` below, a
// VALUE chosen at runtime -- both satisfy the same contract and plug into the
// same `DDocumentPrinter`.


// DXmlPrintPolicy
//   struct: the default format policy -- XML serialisation rules. Supplies
// text and attribute escaping and the empty-element convention. Methods
// are stateless and callable on an instance, so a custom policy may use
// either static or non-static members. Swap this parameter to retarget the
// printer at another markup dialect (e.g. an HTML policy with void-element
// rules).
struct DXmlPrintPolicy
{
    // escape_text
    //   function: escapes the three characters that are significant in XML
    // character data: &, <, and >.
    static
    DXmlString escape_text(
        const DXmlString& _in
    )
    {
        DXmlString _out;
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
    static
    DXmlString escape_attribute(
        const DXmlString& _in
    )
    {
        DXmlString _out;
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
    static DElementForm
    form_of(
        const DXmlString&   /*_name*/,
        bool                _is_empty
    )
    {
        return _is_empty ? DElementForm::self_closing
                         : DElementForm::paired;
    }
};


// DBoxedPrintPolicy
//   class: a type-erased print policy -- the RUNTIME face of the contract.
// It holds any concrete policy behind a small interface and forwards
// `escape_text` / `escape_attribute` / `form_of` through it, so the format
// can be chosen as a value at runtime. Because it satisfies the same contract
// as a concrete policy, `DDocumentPrinter<Sink, DBoxedPrintPolicy>` and
// `render` accept it unchanged -- the only difference from a compile-time
// policy is one indirection per call. Uses the same shared concept/model
// erasure as `DBoxedConsumer` / `DBoxedAccumulator`.
class DBoxedPrintPolicy
{
public:
    template<typename Policy,
             typename = typename std::enable_if<
                 !std::is_same<clean_t<Policy>, DBoxedPrintPolicy>::value>::type>
    DBoxedPrintPolicy(
        Policy _policy
    )
        : m_self(std::make_shared<model_t<clean_t<Policy>>>(
              static_cast<Policy&&>(_policy)))
    {}

    D_NODISCARD DXmlString escape_text(
        const DXmlString& _in
    ) const
    {
        return m_self->escape_text(_in);
    }

    D_NODISCARD DXmlString escape_attribute(
        const DXmlString& _in
    ) const
    {
        return m_self->escape_attribute(_in);
    }

    D_NODISCARD DElementForm form_of(
        const DXmlString&   _name,
        bool                _is_empty
    ) const
    {
        return m_self->form_of(_name, _is_empty);
    }

private:
    // concept_t -- the erased policy interface.
    struct concept_t
    {
        virtual
        ~concept_t() = default;

        virtual
        DXmlString escape_text(
            const DXmlString&
        ) const = 0;
        virtual
        DXmlString escape_attribute(
            const DXmlString&
        ) const = 0;
        virtual
        DElementForm form_of(
            const DXmlString&,
            bool
        ) const = 0;
    };

    // model_t -- a concrete policy wearing the interface.
    template<typename Policy>
    struct model_t final : concept_t
    {
        explicit model_t(
            Policy _policy
        )
            : m_policy(static_cast<Policy&&>(_policy))
        {}

        DXmlString escape_text(
            const DXmlString& _in
        ) const override
        {
            return m_policy.escape_text(_in);
        }

        DXmlString escape_attribute(
            const DXmlString& _in
        ) const override
        {
            return m_policy.escape_attribute(_in);
        }

        DElementForm form_of(
            const DXmlString&   _name,
            bool                _is_empty
        ) const override
        {
            return m_policy.form_of(_name, _is_empty);
        }

        Policy m_policy;
    };

    std::shared_ptr<const concept_t> m_self;
};


// box_policy
//   function: wrap any concrete policy as a `DBoxedPrintPolicy`.
template<typename Policy>
D_NODISCARD inline DBoxedPrintPolicy box_policy(
    Policy _policy
)
{
    return DBoxedPrintPolicy(static_cast<Policy&&>(_policy));
}


// xml_policy
//   function: the XML policy as a runtime value.
D_NODISCARD inline DBoxedPrintPolicy xml_policy()
{
    return DBoxedPrintPolicy(DXmlPrintPolicy{});
}


///////////////////////////////////////////////////////////////////////////////
///                  V.   DOCUMENT PRINTER                                      ///
///////////////////////////////////////////////////////////////////////////////

// DDocumentPrinter
//   class: the protocol-generic printer. `Sink` is any callable
// `(const char*, std::size_t)`; `Policy` supplies the format-specific
// escaping and empty-element rules. Construct with a sink (and optionally
// options and a policy), then call `print` / `operator()` with any node
// that satisfies the node protocol.
template<typename Sink,
         typename Policy = DXmlPrintPolicy>
class DDocumentPrinter
{
public:
    using sink_type   = Sink;
    using policy_type = Policy;

    // DDocumentPrinter (sink)
    //   constructor: bind a sink; default options and policy.
    explicit DDocumentPrinter(
        Sink _sink
    )
        : m_sink(static_cast<Sink&&>(_sink))
        , m_options()
        , m_policy()
    {}

    // DDocumentPrinter (sink, options)
    //   constructor: bind a sink and rendering options.
    DDocumentPrinter(
        Sink          _sink,
        DPrintOptions _options
    )
        : m_sink(static_cast<Sink&&>(_sink))
        , m_options(static_cast<DPrintOptions&&>(_options))
        , m_policy()
    {}

    // DDocumentPrinter (sink, options, policy)
    //   constructor: bind a sink, options, and an explicit policy instance.
    DDocumentPrinter(
        Sink          _sink,
        DPrintOptions _options,
        Policy        _policy
    )
        : m_sink(static_cast<Sink&&>(_sink))
        , m_options(static_cast<DPrintOptions&&>(_options))
        , m_policy(static_cast<Policy&&>(_policy))
    {}

    // print
    //   function: serialise `_node` and its subtree to the bound sink.
    template<typename Node>
    void
    print(
        const Node& _node
    )
    {
        static_assert(
            is_xml_node<Node>::value,
            "DDocumentPrinter::print requires a type satisfying the XML node "
            "protocol (a name accessor and child traversal).");
        static_assert(
            ( has_children_method<Node>::value ||
              has_get_children_method<Node>::value ),
            "DDocumentPrinter::print requires an iterable child accessor "
            "(children() or get_children()).");

        m_print(_node, 0);

        return;
    }

    // operator()
    //   function: the call face of a printer; forwards to print.
    template<typename Node>
    void
    operator()(
        const Node& _node
    )
    {
        print(_node);

        return;
    }

    // sink
    //   function: the bound sink, to ask it after printing whether every
    // byte arrived (the sinks in writer.hpp answer with ok()).
    D_NODISCARD const Sink&
    sink() const
    {
        return m_sink;
    }

    // options
    //   function: the bound rendering options (mutable, for reuse).
    D_NODISCARD DPrintOptions&
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
    //   function: emit an DXmlString to the sink.
    void
    m_put(
        const DXmlString& _string
    )
    {
        m_sink(_string.data(), _string.size());

        return;
    }

    // m_indent
    //   function: emit `_depth` indent strings, when pretty-printing.
    void
    m_indent(
        DXmlSize _depth
    )
    {
        if (!m_options.pretty)
        {
            return;
        }

        for (DXmlSize _i = 0; _i < _depth; ++_i)
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
    template<typename Node>
    void
    m_print(
        const Node& _node,
        DXmlSize   _depth
    )
    {
        const DXmlNodeKind _kind = m_kind_of(_node);

        switch (_kind)
        {
            case DXmlNodeKind::text:
            {
                m_indent(_depth);
                if constexpr (has_text_method<Node>::value ||
                              has_get_text_method<Node>::value)
                {
                    m_put(m_policy.escape_text(internal::read_text(_node)));
                }
                m_newline();
                break;
            }

            case DXmlNodeKind::comment:
            {
                m_indent(_depth);
                m_put("<!--");
                if constexpr (has_text_method<Node>::value ||
                              has_get_text_method<Node>::value)
                {
                    m_put(internal::read_text(_node));
                }
                m_put("-->");
                m_newline();
                break;
            }

            case DXmlNodeKind::cdata_section:
            {
                m_indent(_depth);
                m_put("<![CDATA[");
                if constexpr (has_text_method<Node>::value ||
                              has_get_text_method<Node>::value)
                {
                    m_put(internal::read_text(_node));
                }
                m_put("]]>");
                m_newline();
                break;
            }

            case DXmlNodeKind::element:
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
    template<typename Node>
    void
    m_print_element(
        const Node& _node,
        DXmlSize   _depth
    )
    {
        const DXmlString _name = internal::read_name(_node);

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

        DXmlString   _text;
        bool         _has_text = false;
        if constexpr (has_text_method<Node>::value ||
                      has_get_text_method<Node>::value)
        {
            if (!_has_children)
            {
                _text     = internal::read_text(_node);
                _has_text = !_text.empty();
            }
        }

        const bool         _empty = !_has_children && !_has_text;
        const DElementForm _form  = m_policy.form_of(_name, _empty);

        if (_form == DElementForm::voidlike)
        {
            // void element (HTML <br>, <img>, ...): open tag only -- no
            // content and no closing tag. Any stray children/text are dropped.
            m_put(">");
            m_newline();

            return;
        }

        if (_empty)
        {
            if (_form == DElementForm::self_closing)
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
    template<typename Node>
    void
    m_print_attributes(
        const Node& _node
    )
    {
        if constexpr (has_attributes_method<Node>::value ||
                      has_get_attributes_method<Node>::value)
        {
            auto&& _attributes = internal::read_attributes(_node);
            for (const auto& _attribute : _attributes)
            {
                using attribute_type = clean_t<decltype(_attribute)>;
                static_assert(
                    is_xml_attribute<attribute_type>::value,
                    "DDocumentPrinter: the attribute type yielded by the node "
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
    template<typename Node>
    static DXmlNodeKind
    m_kind_of(
        const Node& _node
    )
    {
        if constexpr (has_kind_method<Node>::value ||
                      has_get_kind_method<Node>::value)
        {
            return internal::read_kind(_node);
        }
        else
        {
            return DXmlNodeKind::element;
        }
    }

    // m_begin / m_end
    //   function: ADL-aware range access so any iterable child or attribute
    // range works, not only those with member begin()/end().
    template<typename Range>
    static auto
    m_begin(
        Range&& _range
    )
        -> decltype(std::begin(_range))
    {
        using std::begin;

        return begin(_range);
    }

    template<typename Range>
    static auto
    m_end(
        Range&& _range
    )
        -> decltype(std::end(_range))
    {
        using std::end;

        return end(_range);
    }

    Sink          m_sink;
    DPrintOptions m_options;
    Policy        m_policy;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   CONVENIENCE FUNCTIONS                              ///
///////////////////////////////////////////////////////////////////////////////

// print_node
//   function: print `_node` to `_sink` with `_options`, constructing a
// throwaway printer. The default policy (DXmlPrintPolicy) is used.
template<typename Sink,
         typename Node>
inline
void print_node(
    Sink          _sink,
    const Node&   _node,
    DPrintOptions _options = DPrintOptions()
)
{
    DDocumentPrinter<Sink> _printer(
        static_cast<Sink&&>(_sink),
        static_cast<DPrintOptions&&>(_options));
    _printer.print(_node);

    return;
}


// node_to_string
//   function: serialise `_node` to a freshly allocated DXmlString with
// the default policy.
template<typename Node>
D_NODISCARD inline DXmlString node_to_string(
    const Node&   _node,
    DPrintOptions _options = DPrintOptions()
)
{
    DXmlString _out;
    DDocumentPrinter<DStringSink> _printer(
        DStringSink(_out),
        static_cast<DPrintOptions&&>(_options));
    _printer.print(_node);

    return _out;
}


// render
//   function: serialise `_node` to a string with an explicit policy. The
// policy may be a concrete type (compile-time, zero cost) or a
// `DBoxedPrintPolicy` (runtime-chosen) -- the same call covers both. This is
// the seam where compile-time and runtime format selection meet.
template<typename Node,
         typename Policy>
D_NODISCARD inline DXmlString render(
    const Node&   _node,
    Policy        _policy,
    DPrintOptions _options = DPrintOptions()
)
{
    DXmlString _out;
    DDocumentPrinter<DStringSink, Policy> _printer(
        DStringSink(_out),
        static_cast<DPrintOptions&&>(_options),
        static_cast<Policy&&>(_policy));
    _printer.print(_node);

    return _out;
}


///////////////////////////////////////////////////////////////////////////////
///                  VII.   FILE PRINTER                                    ///
///////////////////////////////////////////////////////////////////////////////

// DFilePrinter
//   class: a byte sink targeting a file, with a convenience to print a whole
// document into it. As a sink it is text-agnostic -- `operator()(const char*,
// std::size_t)` writes raw bytes -- so it composes with anything that writes
// to a sink. `print(node)` serialises a document to the file via
// `DDocumentPrinter`. The file is opened in binary mode through an output
// stream (c/filesys underneath), which keeps the first failure -- a file that
// would not open, a write that did not land -- so good(), print() and close()
// all report it; close() also reports the last flush, as fclose does.
class DFilePrinter
{
public:
    explicit DFilePrinter(
        const std::string& _path
    ) D_NOEXCEPT
        : m_stream()
    {
        (void)m_stream.open_file(_path.c_str());
    }

    // operator() -- the sink face: write raw bytes to the file.
    void operator()(
        const char* _data,
        std::size_t _size
    ) D_NOEXCEPT
    {
        (void)m_stream.write(_data, _size);

        return;
    }

    // print -- serialise a document (any node-protocol type) into the file;
    // true while every byte so far has reached it.
    template<typename Node>
    bool print(
        const Node&   _node,
        DPrintOptions _options = DPrintOptions()
    )
    {
        DDocumentPrinter<output_stream_sink> _printer(
            output_stream_sink(m_stream),
            static_cast<DPrintOptions&&>(_options));
        _printer.print(_node);

        return m_stream.is_valid();
    }

    // good -- whether the file opened and every write so far reached it.
    D_NODISCARD bool good() const D_NOEXCEPT
    {
        return m_stream.is_valid();
    }

    // close -- flushes and closes the file; true only when nothing failed,
    // the final flush included. The destructor closes too, unreported.
    bool close() D_NOEXCEPT
    {
        return m_stream.close();
    }

    // stream -- the stream the printer writes through, for its status.
    D_NODISCARD const DOutputStream& stream() const D_NOEXCEPT
    {
        return m_stream;
    }

private:
    DOutputStream m_stream;
};


// print_document_to_file
//   function: serialise `_node` to the file at `_path` with the default
// policy, in one call. True only when the file opened, the whole document
// was written, and it closed cleanly.
template<typename Node>
inline
bool print_document_to_file(
    const std::string& _path,
    const Node&        _node,
    DPrintOptions      _options = DPrintOptions()
)
{
    DFilePrinter _printer(_path);

    const bool printed =
        _printer.print(_node, static_cast<DPrintOptions&&>(_options));

    return _printer.close() && printed;
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_TEXT_PRINTER_HPP
