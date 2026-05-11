/******************************************************************************
* djinterp [markdown]                                    markdown_template.hpp
*
*   Templated Markdown block / inline / document facades and the
* bundled default backend. Mirrors the templating pattern of
* `xml_template.hpp` and `html_template.hpp` but targets markdown's
* bipartite content model (blocks containing inlines, blocks
* containing blocks).
*
*   STORAGE MODEL:
*   For the default backend, blocks and inlines share a single
* underlying storage type (`internal::default_node`) carrying both
* `block_kind` and `inline_kind` discriminators plus optional fields
* for url / title / language / heading level / list metadata / etc.
* This keeps the backend simple and avoids a sum-type machinery
* dependency. Real backends (cmark, md4c, maddy) are free to use
* distinct types -- the trait layer detects them structurally.
*
*   FACADES:
*   - `markdown_block<_Backend>`    wraps `_Backend::block_type`
*   - `markdown_inline<_Backend>`   wraps `_Backend::inline_type`
*   - `markdown_document<_Backend>` holds a `_Backend::document_type`
*     by value and exposes the four render targets.
*
*   ZERO OVERHEAD:
*   The block/inline facades hold a single pointer to the backend
* node. They do not own the storage and do not add data members
* beyond that pointer.
*
*   RENDERING:
*   `markdown_document` exposes:
*     - render_to_markdown (round-trip to source)
*     - render_to_html     (HTML5 fragment)
*     - render_to_xml      (CommonMark XML AST format)
*     - render_to_plaintext
*   The default backend implements all four; adapter backends may
* override or rely on the default emission walking the AST.
*
*
* path:      /inc/djinterp/core/util/markdown/markdown_template.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DEFAULT BACKEND STORAGE
II.   markdown_block<_Backend>
III.  markdown_inline<_Backend>
IV.   INTERNAL EMISSION HELPERS
V.    markdown_document<_Backend>
VI.   markdown_default_backend
VII.  FREE HELPERS / FACTORIES
*/

#ifndef DJINTERP_MARKDOWN_TEMPLATE_
#define DJINTERP_MARKDOWN_TEMPLATE_ 1

// std
#include <cstddef>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"


NS_DJINTERP

namespace markdown {


///////////////////////////////////////////////////////////////////////////////
///                I.   DEFAULT BACKEND STORAGE                             ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace markdown
NS_INTERNAL

    // markdown_default_node
    //   struct: bundled in-memory storage type used by the
    // default backend for both blocks and inlines. Carries
    // both kind discriminators plus the union of all fields
    // every standard kind needs. Blocks set `is_block = true`
    // and use `block_kind`; inlines set `is_block = false` and
    // use `inline_kind`. A single struct keeps the default
    // backend trivially simple.
    struct markdown_default_node
    {
        // discrimination
        bool                                    is_block      = true;
        ::djinterp::markdown::markdown_block_kind   block_kind  =
            ::djinterp::markdown::markdown_block_kind::unknown;
        ::djinterp::markdown::markdown_inline_kind  inline_kind =
            ::djinterp::markdown::markdown_inline_kind::unknown;

        // common content
        std::string                             text;

        // link / image / autolink fields
        std::string                             url;
        std::string                             title;
        std::string                             alt_text;

        // code-block fields
        std::string                             language;
        std::string                             info_string;
        char                                    fence_char    = '`';
        unsigned                                fence_length  = 3;

        // heading fields
        int                                     heading_level = 0;
        ::djinterp::markdown::markdown_heading_style heading_style =
            ::djinterp::markdown::markdown_heading_style::atx;

        // list fields
        bool                                    list_ordered  = false;
        int                                     list_start    = 1;
        bool                                    list_loose    = false;
        char                                    list_bullet   = '-';
        bool                                    is_task       = false;
        bool                                    task_checked  = false;

        // table fields
        std::vector<::djinterp::markdown::markdown_table_alignment>
                                                column_alignments;

        // tree
        std::vector<std::unique_ptr<markdown_default_node>>     children;
        markdown_default_node*                                  parent = nullptr;
    };


    // markdown_default_document
    //   struct: bundled in-memory storage type for documents.
    // Holds the root block subtree plus the document-level
    // flavor tag.
    struct markdown_default_document
    {
        ::djinterp::markdown::markdown_flavor   flavor =
            ::djinterp::markdown::markdown_flavor::commonmark;
        std::unique_ptr<markdown_default_node>  root;
    };

NS_END  // internal
namespace markdown {


///////////////////////////////////////////////////////////////////////////////
///                II.   markdown_block<_Backend>                           ///
///////////////////////////////////////////////////////////////////////////////

// markdown_block
//   class: thin facade over a backend block storage node. Holds
// only a raw pointer to the node; does not own the storage.
template<typename _Backend>
class markdown_block
{
public:
    // node_type
    //   type: the backend's block storage type.
    using node_type = typename _Backend::block_type;


    /// constructors

    markdown_block()
    :   m_node(nullptr)
    {}

    explicit
    markdown_block(
        node_type*  _node
    )
    :   m_node(_node)
    {}


    /// identity

    // valid
    //   function: true if this facade wraps a non-null backend
    // node.
    bool valid() const { return (m_node != nullptr); }

    // backend_handle
    //   function: returns the raw backend node pointer.
    node_type* backend_handle() const { return m_node; }


    /// kind

    // block_kind
    //   function: returns the block kind discriminator.
    markdown_block_kind
    block_kind() const
    {
        return (m_node != nullptr) ? m_node->block_kind
                                   : markdown_block_kind::unknown;
    }

    // get_block_kind
    //   function: getter-form alias.
    markdown_block_kind
    get_block_kind() const
    {
        return this->block_kind();
    }

    // is_heading / is_code / is_list / is_table / is_quote
    bool is_heading() const  { return is_heading_block_kind(this->block_kind()); }
    bool is_code()    const  { return is_code_block_kind(this->block_kind()); }
    bool is_list()    const  { return is_list_block_kind(this->block_kind()); }
    bool is_table()   const  { return is_table_block_kind(this->block_kind()); }
    bool is_container() const{ return is_container_block_kind(this->block_kind()); }
    bool is_leaf()    const  { return is_leaf_block_kind(this->block_kind()); }


    /// content access

    // text
    //   function: returns this block's raw text content (only
    // meaningful for code blocks, html blocks, raw blocks).
    const markdown_string_t&
    text() const
    {
        return (m_node != nullptr) ? m_node->text : s_empty();
    }

    // get_text
    //   function: getter-form alias.
    const markdown_string_t&
    get_text() const
    {
        return this->text();
    }

    // set_text
    void
    set_text(
        const markdown_string_t&    _text
    )
    {
        if (m_node != nullptr)
        {
            m_node->text = _text;
        }
    }


    /// heading

    int
    heading_level() const
    {
        return (m_node != nullptr) ? m_node->heading_level : 0;
    }

    int
    get_heading_level() const
    {
        return this->heading_level();
    }


    /// code blocks

    const markdown_string_t&
    language() const
    {
        return (m_node != nullptr) ? m_node->language : s_empty();
    }

    const markdown_string_t&
    get_language() const
    {
        return this->language();
    }

    const markdown_string_t&
    info_string() const
    {
        return (m_node != nullptr) ? m_node->info_string : s_empty();
    }


    /// lists

    bool
    is_ordered() const
    {
        return (m_node != nullptr) ? m_node->list_ordered : false;
    }

    int
    list_start() const
    {
        return (m_node != nullptr) ? m_node->list_start : 1;
    }

    bool
    is_loose() const
    {
        return (m_node != nullptr) ? m_node->list_loose : false;
    }


    /// task list items

    bool
    is_checked() const
    {
        return (m_node != nullptr) ? m_node->task_checked : false;
    }


    /// tables

    const std::vector<markdown_table_alignment>&
    column_alignments() const
    {
        return (m_node != nullptr) ? m_node->column_alignments
                                   : s_empty_alignments();
    }


    /// children

    // child_count
    std::size_t
    child_count() const
    {
        return (m_node != nullptr) ? m_node->children.size() : 0;
    }

    // child_at
    //   function: returns a facade for the child at `_index`.
    // The returned facade may be a block or an inline depending
    // on this block's kind; check `child.is_block` if needed.
    markdown_block<_Backend>
    block_child_at(
        std::size_t     _index
    )   const
    {
        if ((m_node == nullptr) || (_index >= m_node->children.size()))
        {
            return markdown_block<_Backend>();
        }
        return markdown_block<_Backend>(m_node->children[_index].get());
    }

    // add_block
    //   function: takes ownership of an existing detached
    // block, appending it as a child of this block.
    void
    add_block(
        markdown_block<_Backend>&&  _child
    )
    {
        if ((m_node == nullptr) || (!_child.valid()))
        {
            return;
        }
        node_type*  raw = _child.backend_handle();
        raw->parent     = m_node;
        m_node->children.push_back(
            std::unique_ptr<node_type>(raw));
    }


private:
    static const markdown_string_t&
    s_empty()
    {
        static const markdown_string_t e;
        return e;
    }

    static const std::vector<markdown_table_alignment>&
    s_empty_alignments()
    {
        static const std::vector<markdown_table_alignment> e;
        return e;
    }


    // m_node
    //   field: raw pointer to the backend storage node.
    // Non-owning -- the storage is owned by the document or by
    // the parent block via unique_ptr.
    node_type*  m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                III.   markdown_inline<_Backend>                         ///
///////////////////////////////////////////////////////////////////////////////

// markdown_inline
//   class: thin facade over a backend inline storage node.
// Same shape as `markdown_block` but exposes inline-specific
// accessors.
template<typename _Backend>
class markdown_inline
{
public:
    using node_type = typename _Backend::inline_type;


    markdown_inline()
    :   m_node(nullptr)
    {}

    explicit
    markdown_inline(
        node_type*  _node
    )
    :   m_node(_node)
    {}


    /// identity

    bool       valid() const          { return (m_node != nullptr); }
    node_type* backend_handle() const { return m_node; }


    /// kind

    markdown_inline_kind
    inline_kind() const
    {
        return (m_node != nullptr) ? m_node->inline_kind
                                   : markdown_inline_kind::unknown;
    }

    markdown_inline_kind
    get_inline_kind() const
    {
        return this->inline_kind();
    }

    bool is_atomic()    const  { return is_atomic_inline_kind(this->inline_kind()); }
    bool is_container() const  { return is_container_inline_kind(this->inline_kind()); }
    bool is_emphasis()  const  { return is_emphasis_inline_kind(this->inline_kind()); }
    bool is_link()      const  { return is_link_inline_kind(this->inline_kind()); }
    bool is_image()     const  { return is_image_inline_kind(this->inline_kind()); }
    bool is_break()     const  { return is_break_inline_kind(this->inline_kind()); }


    /// content access

    const markdown_string_t&
    text() const
    {
        return (m_node != nullptr) ? m_node->text : s_empty();
    }

    const markdown_string_t&
    get_text() const
    {
        return this->text();
    }

    void
    set_text(
        const markdown_string_t&    _text
    )
    {
        if (m_node != nullptr)
        {
            m_node->text = _text;
        }
    }


    /// link / image accessors

    const markdown_string_t&
    url() const
    {
        return (m_node != nullptr) ? m_node->url : s_empty();
    }

    const markdown_string_t&
    get_url() const
    {
        return this->url();
    }

    const markdown_string_t&
    title() const
    {
        return (m_node != nullptr) ? m_node->title : s_empty();
    }

    const markdown_string_t&
    get_title() const
    {
        return this->title();
    }

    const markdown_string_t&
    alt_text() const
    {
        return (m_node != nullptr) ? m_node->alt_text : s_empty();
    }

    const markdown_string_t&
    get_alt_text() const
    {
        return this->alt_text();
    }

    void set_url  (const markdown_string_t& _v) { if (m_node) m_node->url   = _v; }
    void set_title(const markdown_string_t& _v) { if (m_node) m_node->title = _v; }
    void set_alt  (const markdown_string_t& _v) { if (m_node) m_node->alt_text = _v; }


    /// children (for container inlines)

    std::size_t
    child_count() const
    {
        return (m_node != nullptr) ? m_node->children.size() : 0;
    }

    markdown_inline<_Backend>
    inline_child_at(
        std::size_t     _index
    )   const
    {
        if ((m_node == nullptr) || (_index >= m_node->children.size()))
        {
            return markdown_inline<_Backend>();
        }
        return markdown_inline<_Backend>(m_node->children[_index].get());
    }

    void
    add_inline(
        markdown_inline<_Backend>&& _child
    )
    {
        if ((m_node == nullptr) || (!_child.valid()))
        {
            return;
        }
        node_type*  raw = _child.backend_handle();
        raw->parent     = m_node;
        m_node->children.push_back(
            std::unique_ptr<node_type>(raw));
    }


private:
    static const markdown_string_t&
    s_empty()
    {
        static const markdown_string_t e;
        return e;
    }


    node_type*  m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   INTERNAL EMISSION HELPERS                          ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace markdown
NS_INTERNAL

    // markdown_html_escape_helper
    //   function: writes an HTML-escaped copy of `_in` to `_out`.
    inline void
    markdown_html_escape_helper(
        std::ostream&       _out,
        const std::string&  _in
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
                case '\'':  _out << "&#39;";  break;
                default:    _out << c;        break;
            }
        }
    }


    // markdown_html_attr_escape_helper
    //   function: like html_escape_helper but suitable for use
    // inside double-quoted attribute values.
    inline void
    markdown_html_attr_escape_helper(
        std::ostream&       _out,
        const std::string&  _in
    )
    {
        markdown_html_escape_helper(_out, _in);
    }


    // markdown_md_escape_helper
    //   function: writes a markdown-escaped copy of `_in` to
    // `_out`. Backslash-escapes the standard CommonMark special
    // characters when they appear in text content.
    inline void
    markdown_md_escape_helper(
        std::ostream&       _out,
        const std::string&  _in
    )
    {
        const std::size_t   n = _in.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            const char c = _in[i];
            switch (c)
            {
                case '\\': case '`':  case '*':  case '_':
                case '{':  case '}':  case '[':  case ']':
                case '(':  case ')':  case '#':  case '+':
                case '-':  case '.':  case '!':  case '>':
                case '|':  case '~':
                    _out << '\\' << c;
                    break;
                default:
                    _out << c;
                    break;
            }
        }
    }


    // markdown_emit_inline_html_helper
    //   function: walks an inline subtree under HTML emission
    // rules.
    inline void
    markdown_emit_inline_html_helper(
        std::ostream&                   _out,
        const markdown_default_node&    _node
    )
    {
        using K = ::djinterp::markdown::markdown_inline_kind;

        switch (_node.inline_kind)
        {
            case K::text:
                markdown_html_escape_helper(_out, _node.text);
                break;
            case K::soft_break:
                _out << '\n';
                break;
            case K::hard_break:
            case K::hard_line_break:
                _out << "<br />\n";
                break;
            case K::emphasis:
                _out << "<em>";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</em>";
                break;
            case K::strong:
                _out << "<strong>";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</strong>";
                break;
            case K::strikethrough:
                _out << "<del>";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</del>";
                break;
            case K::highlight:
                _out << "<mark>";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</mark>";
                break;
            case K::subscript:
                _out << "<sub>";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</sub>";
                break;
            case K::superscript:
                _out << "<sup>";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</sup>";
                break;
            case K::code_span:
                _out << "<code>";
                markdown_html_escape_helper(_out, _node.text);
                _out << "</code>";
                break;
            case K::link:
                _out << "<a href=\"";
                markdown_html_attr_escape_helper(_out, _node.url);
                _out << '"';
                if (!_node.title.empty())
                {
                    _out << " title=\"";
                    markdown_html_attr_escape_helper(_out, _node.title);
                    _out << '"';
                }
                _out << '>';
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</a>";
                break;
            case K::image:
                _out << "<img src=\"";
                markdown_html_attr_escape_helper(_out, _node.url);
                _out << "\" alt=\"";
                markdown_html_attr_escape_helper(_out, _node.alt_text);
                _out << '"';
                if (!_node.title.empty())
                {
                    _out << " title=\"";
                    markdown_html_attr_escape_helper(_out, _node.title);
                    _out << '"';
                }
                _out << " />";
                break;
            case K::autolink:
            case K::autolink_email:
                _out << "<a href=\"";
                if (_node.inline_kind == K::autolink_email)
                {
                    _out << "mailto:";
                }
                markdown_html_attr_escape_helper(_out, _node.url);
                _out << "\">";
                markdown_html_escape_helper(_out, _node.url);
                _out << "</a>";
                break;
            case K::html_inline:
                _out << _node.text;
                break;
            case K::math_inline:
                _out << "<span class=\"math inline\">";
                markdown_html_escape_helper(_out, _node.text);
                _out << "</span>";
                break;
            default:
                // unsupported / unknown -- emit text fallback
                markdown_html_escape_helper(_out, _node.text);
                break;
        }
    }


    // markdown_emit_block_html_helper
    //   function: walks a block subtree under HTML emission
    // rules. Recurses into children for container blocks.
    inline void
    markdown_emit_block_html_helper(
        std::ostream&                   _out,
        const markdown_default_node&    _node
    )
    {
        using K = ::djinterp::markdown::markdown_block_kind;

        switch (_node.block_kind)
        {
            case K::document:
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                break;

            case K::heading_1: case K::heading_2: case K::heading_3:
            case K::heading_4: case K::heading_5: case K::heading_6:
            {
                const int   lvl = _node.heading_level;
                _out << "<h" << lvl << '>';
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</h" << lvl << ">\n";
                break;
            }

            case K::paragraph:
                _out << "<p>";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</p>\n";
                break;

            case K::thematic_break:
                _out << "<hr />\n";
                break;

            case K::indented_code_block:
            case K::fenced_code_block:
                _out << "<pre><code";
                if (!_node.language.empty())
                {
                    _out << " class=\"language-";
                    markdown_html_attr_escape_helper(_out, _node.language);
                    _out << '"';
                }
                _out << '>';
                markdown_html_escape_helper(_out, _node.text);
                _out << "</code></pre>\n";
                break;

            case K::block_quote:
                _out << "<blockquote>\n";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</blockquote>\n";
                break;

            case K::ordered_list:
            {
                _out << "<ol";
                if (_node.list_start != 1)
                {
                    _out << " start=\"" << _node.list_start << '"';
                }
                _out << ">\n";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</ol>\n";
                break;
            }

            case K::unordered_list:
                _out << "<ul>\n";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</ul>\n";
                break;

            case K::list_item:
            case K::task_list_item:
                _out << "<li>";
                if (_node.is_task)
                {
                    _out << "<input type=\"checkbox\" disabled";
                    if (_node.task_checked)
                    {
                        _out << " checked";
                    }
                    _out << " /> ";
                }
                _out << '\n';
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</li>\n";
                break;

            case K::table:
                _out << "<table>\n";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</table>\n";
                break;

            case K::table_header:
                _out << "<thead>\n";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</thead>\n";
                break;

            case K::table_body:
                _out << "<tbody>\n";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</tbody>\n";
                break;

            case K::table_row:
                _out << "<tr>";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</tr>\n";
                break;

            case K::table_cell:
                _out << "<td>";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "</td>";
                break;

            case K::html_block:
            case K::raw_block:
                _out << _node.text;
                if ((!_node.text.empty()) && (_node.text.back() != '\n'))
                {
                    _out << '\n';
                }
                break;

            case K::math_block:
                _out << "<div class=\"math display\">";
                markdown_html_escape_helper(_out, _node.text);
                _out << "</div>\n";
                break;

            default:
                // unsupported / unknown -- emit children if any
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_html_helper(
                            _out, *_node.children[i]);
                    }
                }
                break;
        }
    }


    // markdown_emit_inline_md_helper
    //   function: round-trip emitter for an inline subtree.
    inline void
    markdown_emit_inline_md_helper(
        std::ostream&                   _out,
        const markdown_default_node&    _node
    )
    {
        using K = ::djinterp::markdown::markdown_inline_kind;

        switch (_node.inline_kind)
        {
            case K::text:
                markdown_md_escape_helper(_out, _node.text);
                break;
            case K::soft_break:        _out << '\n';     break;
            case K::hard_break:        _out << "  \n";   break;
            case K::hard_line_break:   _out << "\\\n";   break;
            case K::emphasis:
                _out << '*';
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_md_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << '*';
                break;
            case K::strong:
                _out << "**";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_md_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "**";
                break;
            case K::strikethrough:
                _out << "~~";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_md_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "~~";
                break;
            case K::code_span:
                _out << '`' << _node.text << '`';
                break;
            case K::link:
                _out << '[';
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_md_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "](" << _node.url;
                if (!_node.title.empty())
                {
                    _out << " \"" << _node.title << '"';
                }
                _out << ')';
                break;
            case K::image:
                _out << "![" << _node.alt_text << "](" << _node.url;
                if (!_node.title.empty())
                {
                    _out << " \"" << _node.title << '"';
                }
                _out << ')';
                break;
            case K::autolink:
            case K::autolink_email:
                _out << '<' << _node.url << '>';
                break;
            case K::html_inline:
                _out << _node.text;
                break;
            case K::math_inline:
                _out << '$' << _node.text << '$';
                break;
            default:
                _out << _node.text;
                break;
        }
    }


    // markdown_emit_block_md_helper
    //   function: round-trip emitter for a block subtree.
    inline void
    markdown_emit_block_md_helper(
        std::ostream&                   _out,
        const markdown_default_node&    _node,
        std::size_t                     _list_depth = 0
    )
    {
        using K = ::djinterp::markdown::markdown_block_kind;

        switch (_node.block_kind)
        {
            case K::document:
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_md_helper(
                            _out, *_node.children[i], _list_depth);
                    }
                }
                break;

            case K::heading_1: case K::heading_2: case K::heading_3:
            case K::heading_4: case K::heading_5: case K::heading_6:
            {
                const int   lvl = _node.heading_level;
                for (int i = 0; i < lvl; ++i)
                {
                    _out << '#';
                }
                _out << ' ';
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_md_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "\n\n";
                break;
            }

            case K::paragraph:
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_inline_md_helper(
                            _out, *_node.children[i]);
                    }
                }
                _out << "\n\n";
                break;

            case K::thematic_break:
                _out << "---\n\n";
                break;

            case K::fenced_code_block:
            {
                const std::string fence(
                    (_node.fence_length > 0) ? _node.fence_length : 3,
                    _node.fence_char);
                _out << fence;
                if (!_node.info_string.empty())
                {
                    _out << _node.info_string;
                }
                else if (!_node.language.empty())
                {
                    _out << _node.language;
                }
                _out << '\n' << _node.text;
                if ((!_node.text.empty()) && (_node.text.back() != '\n'))
                {
                    _out << '\n';
                }
                _out << fence << "\n\n";
                break;
            }

            case K::indented_code_block:
            {
                const std::string&  txt = _node.text;
                std::size_t         start = 0;
                while (start <= txt.size())
                {
                    const std::size_t   nl = txt.find('\n', start);
                    _out << "    "
                         << txt.substr(start,
                                       (nl == std::string::npos)
                                           ? (txt.size() - start)
                                           : (nl - start))
                         << '\n';
                    if (nl == std::string::npos)
                    {
                        break;
                    }
                    start = nl + 1;
                }
                _out << '\n';
                break;
            }

            case K::block_quote:
            {
                std::ostringstream  inner;
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_md_helper(
                            inner, *_node.children[i], _list_depth);
                    }
                }
                const std::string   buf = inner.str();
                std::size_t         start = 0;
                while (start < buf.size())
                {
                    const std::size_t   nl = buf.find('\n', start);
                    _out << "> "
                         << buf.substr(start,
                                       (nl == std::string::npos)
                                           ? (buf.size() - start)
                                           : (nl - start + 1));
                    if (nl == std::string::npos)
                    {
                        break;
                    }
                    start = nl + 1;
                }
                _out << '\n';
                break;
            }

            case K::ordered_list:
            case K::unordered_list:
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_md_helper(
                            _out, *_node.children[i], _list_depth + 1);
                    }
                }
                if (_list_depth == 0)
                {
                    _out << '\n';
                }
                break;

            case K::list_item:
            case K::task_list_item:
            {
                for (std::size_t i = 1; i < _list_depth; ++i)
                {
                    _out << "  ";
                }
                _out << _node.list_bullet << ' ';
                if (_node.is_task)
                {
                    _out << '[' << (_node.task_checked ? 'x' : ' ') << "] ";
                }
                std::ostringstream  inner;
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_md_helper(
                            inner, *_node.children[i], _list_depth);
                    }
                }
                const std::string&  buf = inner.str();
                // strip trailing blank line for compact items
                std::size_t         end = buf.size();
                while ((end > 0) && (buf[end - 1] == '\n'))
                {
                    --end;
                }
                _out << buf.substr(0, end) << '\n';
                break;
            }

            case K::html_block:
            case K::raw_block:
                _out << _node.text;
                if ((!_node.text.empty()) && (_node.text.back() != '\n'))
                {
                    _out << '\n';
                }
                _out << '\n';
                break;

            case K::math_block:
                _out << "$$\n" << _node.text;
                if ((!_node.text.empty()) && (_node.text.back() != '\n'))
                {
                    _out << '\n';
                }
                _out << "$$\n\n";
                break;

            default:
                // unsupported / unknown -- emit children if any
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        markdown_emit_block_md_helper(
                            _out, *_node.children[i], _list_depth);
                    }
                }
                break;
        }
    }


    // markdown_emit_block_plaintext_helper
    //   function: strips formatting and emits text content only.
    inline void
    markdown_emit_block_plaintext_helper(
        std::ostream&                   _out,
        const markdown_default_node&    _node
    )
    {
        if ( !_node.text.empty() &&
             (_node.is_block) )
        {
            _out << _node.text;
        }
        if ( !_node.is_block )
        {
            // inline: emit text and walk children
            _out << _node.text;
        }
        for (std::size_t i = 0; i < _node.children.size(); ++i)
        {
            if (_node.children[i])
            {
                markdown_emit_block_plaintext_helper(
                    _out, *_node.children[i]);
            }
        }
        // append a newline after block-level elements
        if (_node.is_block)
        {
            using K = ::djinterp::markdown::markdown_block_kind;
            const auto k = _node.block_kind;
            if ( (k == K::paragraph)     ||
                 (k == K::heading_1)     || (k == K::heading_2) ||
                 (k == K::heading_3)     || (k == K::heading_4) ||
                 (k == K::heading_5)     || (k == K::heading_6) ||
                 (k == K::thematic_break) ||
                 (k == K::indented_code_block) ||
                 (k == K::fenced_code_block) ||
                 (k == K::math_block) )
            {
                _out << '\n';
            }
        }
    }

NS_END  // internal
namespace markdown {


///////////////////////////////////////////////////////////////////////////////
///                V.   markdown_document<_Backend>                         ///
///////////////////////////////////////////////////////////////////////////////

// markdown_document
//   class: facade over the backend's storage document type plus
// flavor metadata. Owns the storage by value and exposes the
// four render targets.
template<typename _Backend>
class markdown_document
{
public:
    using backend_type  = _Backend;
    using document_type = typename _Backend::document_type;
    using node_type     = typename _Backend::block_type;
    using block_facade  = markdown_block<_Backend>;


    /// constructors

    markdown_document()
    :   m_doc(_Backend::make_markdown_document())
    {}

    explicit
    markdown_document(
        markdown_flavor     _flavor
    )
    :   m_doc(_Backend::make_markdown_document())
    {
        m_doc.flavor = _flavor;
    }

    explicit
    markdown_document(
        document_type&&     _doc
    )
    :   m_doc(std::move(_doc))
    {}


    /// flavor

    // flavor
    markdown_flavor
    flavor() const
    {
        return m_doc.flavor;
    }

    // get_flavor
    markdown_flavor
    get_flavor() const
    {
        return m_doc.flavor;
    }

    // set_flavor
    void
    set_flavor(
        markdown_flavor     _flavor
    )
    {
        m_doc.flavor = _flavor;
    }


    /// root access

    // root_block
    //   function: returns a facade for the document root block.
    block_facade
    root_block() const
    {
        return block_facade(m_doc.root.get());
    }

    // backend_document
    //   function: const access to the underlying storage doc.
    const document_type&
    backend_document() const
    {
        return m_doc;
    }


    /// rendering

    // render_to_markdown
    //   function: emits this document as markdown source. The
    // emission targets a CommonMark-compatible round-trip; for
    // GFM / Pandoc / etc. extensions the bundled emitter falls
    // back to CommonMark-equivalent output where possible.
    void
    render_to_markdown(
        std::ostream&   _out
    )   const
    {
        if (m_doc.root)
        {
            ::djinterp::internal::markdown_emit_block_md_helper(
                _out, *m_doc.root, 0);
        }
    }

    // render_to_markdown
    //   function: convenience overload returning a string.
    markdown_string_t
    render_to_markdown() const
    {
        std::ostringstream  oss;
        this->render_to_markdown(oss);
        return oss.str();
    }


    // render_to_html
    //   function: emits this document as an HTML5 fragment.
    void
    render_to_html(
        std::ostream&   _out
    )   const
    {
        if (m_doc.root)
        {
            ::djinterp::internal::markdown_emit_block_html_helper(
                _out, *m_doc.root);
        }
    }

    markdown_string_t
    render_to_html() const
    {
        std::ostringstream  oss;
        this->render_to_html(oss);
        return oss.str();
    }


    // render_to_plaintext
    //   function: emits this document with all formatting
    // stripped.
    void
    render_to_plaintext(
        std::ostream&   _out
    )   const
    {
        if (m_doc.root)
        {
            ::djinterp::internal::markdown_emit_block_plaintext_helper(
                _out, *m_doc.root);
        }
    }

    markdown_string_t
    render_to_plaintext() const
    {
        std::ostringstream  oss;
        this->render_to_plaintext(oss);
        return oss.str();
    }


    // render_to_xml
    //   function: emits this document in CommonMark XML AST
    // format. Stub for the default backend; intended to be
    // overridden by adapter backends that have native XML
    // emission (e.g. cmark's native XML renderer).
    void
    render_to_xml(
        std::ostream&   _out
    )   const
    {
        // Minimal CommonMark XML wrapper around a markdown
        // round-trip, suitable as a starting point for adapter
        // backends to override.
        _out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        _out << "<document xmlns=\"http://commonmark.org/xml/1.0\">\n";
        ::djinterp::internal::markdown_html_escape_helper(
            _out, this->render_to_markdown());
        _out << "</document>\n";
    }

    markdown_string_t
    render_to_xml() const
    {
        std::ostringstream  oss;
        this->render_to_xml(oss);
        return oss.str();
    }


private:
    // m_doc
    //   field: backend storage document, held by value.
    document_type   m_doc;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   markdown_default_backend                           ///
///////////////////////////////////////////////////////////////////////////////

// markdown_default_backend
//   struct: bundled in-memory backend for the markdown facades.
// Uses a single underlying storage type for both blocks and
// inlines (`internal::markdown_default_node`) carrying both
// kind discriminators plus the union of all per-kind fields.
struct markdown_default_backend
{
    using markdown_backend_tag = ::djinterp::markdown::markdown_default_backend_tag;

    using block_type    = ::djinterp::internal::markdown_default_node;
    using inline_type   = ::djinterp::internal::markdown_default_node;
    using document_type = ::djinterp::internal::markdown_default_document;

    // make_markdown_document
    //   function: factory for a fresh empty document with a
    // root `document` block ready to receive children.
    static document_type
    make_markdown_document()
    {
        document_type   d;
        d.flavor = markdown_flavor::commonmark;
        std::unique_ptr<block_type> root(new block_type);
        root->is_block   = true;
        root->block_kind = markdown_block_kind::document;
        d.root           = std::move(root);
        return d;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                VII.   FREE HELPERS / FACTORIES                          ///
///////////////////////////////////////////////////////////////////////////////

// make_markdown_document
//   function: factory returning a freshly-built document
// facade for the given backend.
template<typename _Backend>
inline markdown_document<_Backend>
make_markdown_document(
    markdown_flavor     _flavor = markdown_flavor::commonmark
)
{
    return markdown_document<_Backend>(_flavor);
}


// make_paragraph
//   function: returns a new freestanding paragraph block.
template<typename _Backend>
inline markdown_block<_Backend>
make_paragraph()
{
    using node_t = typename _Backend::block_type;
    node_t* n = new node_t;
    n->is_block   = true;
    n->block_kind = markdown_block_kind::paragraph;
    return markdown_block<_Backend>(n);
}


// make_heading
//   function: returns a new freestanding heading block of the
// given level (1..6; clamped).
template<typename _Backend>
inline markdown_block<_Backend>
make_heading(
    int     _level
)
{
    if (_level < 1) { _level = 1; }
    if (_level > 6) { _level = 6; }
    using node_t = typename _Backend::block_type;
    node_t* n = new node_t;
    n->is_block      = true;
    n->block_kind    = heading_kind_from_level(_level);
    n->heading_level = _level;
    return markdown_block<_Backend>(n);
}


// make_fenced_code_block
//   function: returns a new freestanding fenced code block
// with the given language tag and code body.
template<typename _Backend>
inline markdown_block<_Backend>
make_fenced_code_block(
    const markdown_string_t&    _code,
    const markdown_string_t&    _language = markdown_string_t()
)
{
    using node_t = typename _Backend::block_type;
    node_t* n = new node_t;
    n->is_block    = true;
    n->block_kind  = markdown_block_kind::fenced_code_block;
    n->text        = _code;
    n->language    = _language;
    n->info_string = _language;
    return markdown_block<_Backend>(n);
}


// make_thematic_break
//   function: returns a new freestanding thematic break block.
template<typename _Backend>
inline markdown_block<_Backend>
make_thematic_break()
{
    using node_t = typename _Backend::block_type;
    node_t* n = new node_t;
    n->is_block   = true;
    n->block_kind = markdown_block_kind::thematic_break;
    return markdown_block<_Backend>(n);
}


// make_block_quote
//   function: returns a new freestanding block quote.
template<typename _Backend>
inline markdown_block<_Backend>
make_block_quote()
{
    using node_t = typename _Backend::block_type;
    node_t* n = new node_t;
    n->is_block   = true;
    n->block_kind = markdown_block_kind::block_quote;
    return markdown_block<_Backend>(n);
}


// make_list
//   function: returns a new freestanding list block.
template<typename _Backend>
inline markdown_block<_Backend>
make_list(
    bool    _ordered = false,
    int     _start   = 1
)
{
    using node_t = typename _Backend::block_type;
    node_t* n = new node_t;
    n->is_block     = true;
    n->block_kind   = _ordered ? markdown_block_kind::ordered_list
                               : markdown_block_kind::unordered_list;
    n->list_ordered = _ordered;
    n->list_start   = _start;
    return markdown_block<_Backend>(n);
}


// make_list_item
//   function: returns a new freestanding list item block.
template<typename _Backend>
inline markdown_block<_Backend>
make_list_item()
{
    using node_t = typename _Backend::block_type;
    node_t* n = new node_t;
    n->is_block    = true;
    n->block_kind  = markdown_block_kind::list_item;
    n->list_bullet = D_MARKDOWN_DEFAULT_BULLET;
    return markdown_block<_Backend>(n);
}


// make_text
//   function: returns a new freestanding text inline.
template<typename _Backend>
inline markdown_inline<_Backend>
make_text(
    const markdown_string_t&    _text
)
{
    using node_t = typename _Backend::inline_type;
    node_t* n = new node_t;
    n->is_block    = false;
    n->inline_kind = markdown_inline_kind::text;
    n->text        = _text;
    return markdown_inline<_Backend>(n);
}


// make_emphasis
//   function: returns a new freestanding emphasis inline.
template<typename _Backend>
inline markdown_inline<_Backend>
make_emphasis()
{
    using node_t = typename _Backend::inline_type;
    node_t* n = new node_t;
    n->is_block    = false;
    n->inline_kind = markdown_inline_kind::emphasis;
    return markdown_inline<_Backend>(n);
}


// make_strong
//   function: returns a new freestanding strong inline.
template<typename _Backend>
inline markdown_inline<_Backend>
make_strong()
{
    using node_t = typename _Backend::inline_type;
    node_t* n = new node_t;
    n->is_block    = false;
    n->inline_kind = markdown_inline_kind::strong;
    return markdown_inline<_Backend>(n);
}


// make_code_span
//   function: returns a new freestanding code span inline.
template<typename _Backend>
inline markdown_inline<_Backend>
make_code_span(
    const markdown_string_t&    _code
)
{
    using node_t = typename _Backend::inline_type;
    node_t* n = new node_t;
    n->is_block    = false;
    n->inline_kind = markdown_inline_kind::code_span;
    n->text        = _code;
    return markdown_inline<_Backend>(n);
}


// make_link
//   function: returns a new freestanding link inline.
template<typename _Backend>
inline markdown_inline<_Backend>
make_link(
    const markdown_string_t&    _url,
    const markdown_string_t&    _title = markdown_string_t()
)
{
    using node_t = typename _Backend::inline_type;
    node_t* n = new node_t;
    n->is_block    = false;
    n->inline_kind = markdown_inline_kind::link;
    n->url         = _url;
    n->title       = _title;
    return markdown_inline<_Backend>(n);
}


// make_image
//   function: returns a new freestanding image inline.
template<typename _Backend>
inline markdown_inline<_Backend>
make_image(
    const markdown_string_t&    _url,
    const markdown_string_t&    _alt   = markdown_string_t(),
    const markdown_string_t&    _title = markdown_string_t()
)
{
    using node_t = typename _Backend::inline_type;
    node_t* n = new node_t;
    n->is_block    = false;
    n->inline_kind = markdown_inline_kind::image;
    n->url         = _url;
    n->alt_text    = _alt;
    n->title       = _title;
    return markdown_inline<_Backend>(n);
}


// make_autolink
//   function: returns a new freestanding autolink inline.
template<typename _Backend>
inline markdown_inline<_Backend>
make_autolink(
    const markdown_string_t&    _url
)
{
    using node_t = typename _Backend::inline_type;
    node_t* n = new node_t;
    n->is_block    = false;
    n->inline_kind = markdown_inline_kind::autolink;
    n->url         = _url;
    return markdown_inline<_Backend>(n);
}


// make_hard_break
//   function: returns a new freestanding hard line break.
template<typename _Backend>
inline markdown_inline<_Backend>
make_hard_break()
{
    using node_t = typename _Backend::inline_type;
    node_t* n = new node_t;
    n->is_block    = false;
    n->inline_kind = markdown_inline_kind::hard_break;
    return markdown_inline<_Backend>(n);
}


}   // namespace markdown
NS_END  // djinterp


#endif  // DJINTERP_MARKDOWN_TEMPLATE_
