/*******************************************************************************
* djinterp [cpp]                                           markdown_template.hpp
*
*   Templated Markdown block / inline / document facades and the
* bundled default backend. Mirrors the templating pattern of
* `xml_template.hpp` and `html_template.hpp` but targets markdown's
* bipartite content model (blocks containing inlines, blocks
* containing blocks).
*
*   STORAGE MODEL:
*   For the default backend, blocks and inlines share a single
* underlying storage type (`internal::DDefaultNode`) carrying both
* `block_kind` and `inline_kind` discriminators plus optional fields
* for url / title / language / heading level / list metadata / etc.
* This keeps the backend simple and avoids a sum-type machinery
* dependency. Real backends (cmark, md4c, maddy) are free to use
* distinct types -- the trait layer detects them structurally.
*
*   FACADES:
*   - `DMarkdownBlock<Backend>`    wraps `Backend::block_type`
*   - `DMarkdownInline<Backend>`   wraps `Backend::inline_type`
*   - `DMarkdownDocument<Backend>` holds a `Backend::document_type`
*     by value and exposes the four render targets.
*
*   ZERO OVERHEAD:
*   The block/inline facades hold a single pointer to the backend
* node. They do not own the storage and do not add data members
* beyond that pointer.
*
*   RENDERING:
*   `DMarkdownDocument` exposes:
*     - render_to_markdown (round-trip to source)
*     - render_to_html     (HTML5 fragment)
*     - render_to_xml      (CommonMark XML AST format)
*     - render_to_plaintext
*   The default backend implements all four; adapter backends may
* override or rely on the default emission walking the AST.
*   Markdown is written to read back as the same tree: text escapes
* every character CommonMark gives a meaning, '<' and '&' included;
* code spans and fences outnumber the backticks or tildes inside
* them; destinations go in angle brackets when they must; an ordered
* list is numbered from its start; an item's later lines are indented
* to its content's column; a list right after another of its kind takes
* the other marker, so the two stay apart. Two code spans side by side
* can't be written apart: Markdown reads their backticks as one run.
*   HTML follows CommonMark's and GFM's: tight items without paragraphs,
* destinations percent-encoded, header cells as th, and a column's
* alignment on its cells. A list is tight only where Markdown can write
* it tight: an item with two paragraphs makes its list loose, in every
* rendering.
*
*
* path:      /inc/djinterp/cpp/text/markdown/markdown_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.10
*                                                            revised: 2026.11.18
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DEFAULT BACKEND STORAGE
      -----------------------

II.   DMarkdownBlock<Backend>
      ------------------------

III.  DMarkdownInline<Backend>
      -------------------------

IV.   INTERNAL EMISSION HELPERS
      -------------------------

V.    DMarkdownDocument<Backend>
      ---------------------------

VI.   DMarkdownDefaultBackend
      ------------------------

VII.  FREE HELPERS / FACTORIES
      ------------------------
*/

#ifndef DJINTERP_TEXT_MARKDOWN_MARKDOWN_TEMPLATE_HPP
#define DJINTERP_TEXT_MARKDOWN_MARKDOWN_TEMPLATE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cctype>
#include <cstddef>
#include <cstring>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "../detached.hpp"      // DDetached, the factories' handles
#include "./markdown.hpp"          // DMarkdownBlockKind, DMarkdownInlineKind,
                                  // the markdown vocabulary


NS_DJINTERP

namespace markdown {


///////////////////////////////////////////////////////////////////////////////
///                I.   DEFAULT BACKEND STORAGE                             ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace markdown
NS_INTERNAL

    // DMarkdownDefaultNode
    //   struct: bundled in-memory storage type used by the
    // default backend for both blocks and inlines. Carries
    // both kind discriminators plus the union of all fields
    // every standard kind needs. Blocks set `is_block = true`
    // and use `block_kind`; inlines set `is_block = false` and
    // use `inline_kind`. A single struct keeps the default
    // backend trivially simple.
    struct DMarkdownDefaultNode
    {
        // discrimination
        bool                                    is_block      = true;
        ::djinterp::markdown::DMarkdownBlockKind   block_kind  =
            ::djinterp::markdown::DMarkdownBlockKind::unknown;
        ::djinterp::markdown::DMarkdownInlineKind  inline_kind =
            ::djinterp::markdown::DMarkdownInlineKind::unknown;

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
        ::djinterp::markdown::DMarkdownHeadingStyle heading_style =
            ::djinterp::markdown::DMarkdownHeadingStyle::atx;

        // list fields
        bool                                    list_ordered  = false;
        int                                     list_start    = 1;
        bool                                    list_loose    = false;
        char                                    list_bullet   = '-';
        bool                                    is_task       = false;
        bool                                    task_checked  = false;

        // table fields
        std::vector<::djinterp::markdown::DMarkdownTableAlignment>
                                                column_alignments;

        // tree
        std::vector<std::unique_ptr<DMarkdownDefaultNode>>      children;
        DMarkdownDefaultNode*                                   parent = nullptr;
    };


    // DMarkdownDefaultDocument
    //   struct: bundled in-memory storage type for documents.
    // Holds the root block subtree plus the document-level
    // flavor tag.
    struct DMarkdownDefaultDocument
    {
        ::djinterp::markdown::DMarkdownFlavor   flavor =
            ::djinterp::markdown::DMarkdownFlavor::commonmark;
        std::unique_ptr<DMarkdownDefaultNode>  root;
    };

NS_END  // internal
namespace markdown {


///////////////////////////////////////////////////////////////////////////////
///                II.   DMarkdownBlock<Backend>                           ///
///////////////////////////////////////////////////////////////////////////////

template<typename Backend>
class DMarkdownInline;

// DMarkdownBlock
//   class: thin facade over a backend block storage node. Holds
// only a raw pointer to the node; does not own the storage.
template<typename Backend>
class DMarkdownBlock
{
public:
    // node_type
    //   type: the backend's block storage type.
    using node_type = typename Backend::block_type;


    /// constructors

    DMarkdownBlock()
    :   m_node(nullptr)
    {}

    explicit
    DMarkdownBlock(
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
    DMarkdownBlockKind block_kind() const
    {
        return (m_node != nullptr) ? m_node->block_kind
                                   : DMarkdownBlockKind::unknown;
    }

    // get_block_kind
    //   function: getter-form alias.
    DMarkdownBlockKind get_block_kind() const
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
    const DMarkdownString& text() const
    {
        return (m_node != nullptr) ? m_node->text : s_empty();
    }

    // get_text
    //   function: getter-form alias.
    const DMarkdownString& get_text() const
    {
        return this->text();
    }

    // set_text
    void set_text(
        const DMarkdownString& _text
    )
    {
        if (m_node != nullptr)
        {
            m_node->text = _text;
        }
    }


    /// heading

    int heading_level() const
    {
        return (m_node != nullptr) ? m_node->heading_level : 0;
    }

    int get_heading_level() const
    {
        return this->heading_level();
    }


    /// code blocks

    const DMarkdownString& language() const
    {
        return (m_node != nullptr) ? m_node->language : s_empty();
    }

    const DMarkdownString& get_language() const
    {
        return this->language();
    }

    const DMarkdownString& info_string() const
    {
        return (m_node != nullptr) ? m_node->info_string : s_empty();
    }


    /// lists

    bool is_ordered() const
    {
        return (m_node != nullptr) ? m_node->list_ordered : false;
    }

    int list_start() const
    {
        return (m_node != nullptr) ? m_node->list_start : 1;
    }

    bool is_loose() const
    {
        return (m_node != nullptr) ? m_node->list_loose : false;
    }

    // set_loose
    //   function: makes a list loose (a blank line between its items, and
    // each item's text a paragraph) or tight.
    void set_loose(
        bool _loose
    )
    {
        if (m_node != nullptr)
        {
            m_node->list_loose = _loose;
        }

        return;
    }


    /// task list items

    // is_task
    //   function: true for a list item with a task's box (GFM).
    bool is_task() const
    {
        return (m_node != nullptr) ? m_node->is_task : false;
    }

    bool is_checked() const
    {
        return (m_node != nullptr) ? m_node->task_checked : false;
    }

    // set_task
    //   function: gives a list item a task's box, ticked or not.
    void set_task(
        bool _checked
    )
    {
        if (m_node != nullptr)
        {
            m_node->is_task      = true;
            m_node->task_checked = _checked;
        }

        return;
    }

    // clear_task
    //   function: takes a list item's box away, making it a plain item.
    void clear_task()
    {
        if (m_node != nullptr)
        {
            m_node->is_task      = false;
            m_node->task_checked = false;
        }

        return;
    }


    /// tables

    const std::vector<DMarkdownTableAlignment>& column_alignments() const
    {
        return (m_node != nullptr) ? m_node->column_alignments
                                   : s_empty_alignments();
    }

    // set_column_alignments
    //   function: a table's alignment for each column, from the first. A
    // column past the last given is unaligned (none).
    void set_column_alignments(
        const std::vector<DMarkdownTableAlignment>& _alignments
    )
    {
        if (m_node != nullptr)
        {
            m_node->column_alignments = _alignments;
        }

        return;
    }

    // set_column_alignment
    //   function: one column's alignment in a table, counted from 0. The
    // columns before it that have none yet are unaligned (none).
    void set_column_alignment(
        std::size_t             _column,
        DMarkdownTableAlignment _alignment
    )
    {
        if (m_node == nullptr)
        {
            return;
        }

        if (m_node->column_alignments.size() <= _column)
        {
            m_node->column_alignments.resize(_column + 1u,
                                             DMarkdownTableAlignment::none);
        }

        m_node->column_alignments[_column] = _alignment;

        return;
    }


    /// children

    // child_count
    std::size_t child_count() const
    {
        return (m_node != nullptr) ? m_node->children.size() : 0;
    }

    // child_at
    //   function: returns a facade for the child at `_index`.
    // The returned facade may be a block or an inline depending
    // on this block's kind; check `child.is_block` if needed.
    DMarkdownBlock<Backend> block_child_at(
        std::size_t _index
    ) const
    {
        if ((m_node == nullptr) || (_index >= m_node->children.size()))
        {
            return DMarkdownBlock<Backend>();
        }
        return DMarkdownBlock<Backend>(m_node->children[_index].get());
    }

    // add_block
    //   function: makes the block a factory made this block's last child,
    // taking it from its handle, and returns its facade. Only a handle is
    // taken, so a block already in a tree can't be given a second parent.
    // Refused, with an empty facade returned and the handle keeping its
    // block, when this block or the handle is empty, or when this block is
    // the handle's or lies under it.
    DMarkdownBlock<Backend> add_block(
        DDetached<DMarkdownBlock<Backend>>&& _child
    )
    {
        return DMarkdownBlock<Backend>(
            ::djinterp::internal::detached_access::adopt(m_node, _child));
    }

    // inline_child_at
    //   function: returns a facade for the inline child at
    // `_index` of a block whose body is inlines (a paragraph, a
    // heading, a table cell).
    DMarkdownInline<Backend> inline_child_at(
        std::size_t _index
    ) const
    {
        if ((m_node == nullptr) || (_index >= m_node->children.size()))
        {
            return DMarkdownInline<Backend>();
        }
        return DMarkdownInline<Backend>(m_node->children[_index].get());
    }

    // add_inline
    //   function: makes the inline a factory made the last of this block's
    // inline content, taking it from its handle, and returns its facade:
    // how text gets into a paragraph, a heading or a table cell. Refused as
    // add_block is. It needs a backend whose blocks and inlines are one
    // node type, as the default backend's are.
    DMarkdownInline<Backend> add_inline(
        DDetached<DMarkdownInline<Backend>>&& _child
    )
    {
        static_assert(std::is_same<typename Backend::inline_type,
                                   typename Backend::block_type>::value,
                      "add_inline needs a backend whose blocks and "
                      "inlines share one node type");

        return DMarkdownInline<Backend>(
            ::djinterp::internal::detached_access::adopt(m_node, _child));
    }


private:
    static
    const DMarkdownString& s_empty()
    {
        static const DMarkdownString e;
        return e;
    }

    static
    const std::vector<DMarkdownTableAlignment>& s_empty_alignments()
    {
        static const std::vector<DMarkdownTableAlignment> e;
        return e;
    }


    // m_node
    //   field: raw pointer to the backend storage node.
    // Non-owning -- the storage is owned by the document or by
    // the parent block via unique_ptr.
    node_type*  m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                III.   DMarkdownInline<Backend>                         ///
///////////////////////////////////////////////////////////////////////////////

// DMarkdownInline
//   class: thin facade over a backend inline storage node.
// Same shape as `DMarkdownBlock` but exposes inline-specific
// accessors.
template<typename Backend>
class DMarkdownInline
{
public:
    using node_type = typename Backend::inline_type;


    DMarkdownInline()
    :   m_node(nullptr)
    {}

    explicit
    DMarkdownInline(
        node_type*  _node
    )
    :   m_node(_node)
    {}


    /// identity

    bool       valid() const          { return (m_node != nullptr); }
    node_type* backend_handle() const { return m_node; }


    /// kind

    DMarkdownInlineKind inline_kind() const
    {
        return (m_node != nullptr) ? m_node->inline_kind
                                   : DMarkdownInlineKind::unknown;
    }

    DMarkdownInlineKind get_inline_kind() const
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

    const DMarkdownString& text() const
    {
        return (m_node != nullptr) ? m_node->text : s_empty();
    }

    const DMarkdownString& get_text() const
    {
        return this->text();
    }

    void set_text(
        const DMarkdownString& _text
    )
    {
        if (m_node != nullptr)
        {
            m_node->text = _text;
        }
    }


    /// link / image accessors

    const DMarkdownString& url() const
    {
        return (m_node != nullptr) ? m_node->url : s_empty();
    }

    const DMarkdownString& get_url() const
    {
        return this->url();
    }

    const DMarkdownString& title() const
    {
        return (m_node != nullptr) ? m_node->title : s_empty();
    }

    const DMarkdownString& get_title() const
    {
        return this->title();
    }

    const DMarkdownString& alt_text() const
    {
        return (m_node != nullptr) ? m_node->alt_text : s_empty();
    }

    const DMarkdownString& get_alt_text() const
    {
        return this->alt_text();
    }

    void set_url  (const DMarkdownString& _v) { if (m_node) m_node->url   = _v; }
    void set_title(const DMarkdownString& _v) { if (m_node) m_node->title = _v; }
    void set_alt  (const DMarkdownString& _v) { if (m_node) m_node->alt_text = _v; }


    /// children (for container inlines)

    std::size_t child_count() const
    {
        return (m_node != nullptr) ? m_node->children.size() : 0;
    }

    DMarkdownInline<Backend> inline_child_at(
        std::size_t _index
    ) const
    {
        if ((m_node == nullptr) || (_index >= m_node->children.size()))
        {
            return DMarkdownInline<Backend>();
        }
        return DMarkdownInline<Backend>(m_node->children[_index].get());
    }

    // add_inline
    //   function: makes the inline a factory made this inline's last child
    // (inside emphasis, strong, a link), taking it from its handle, and
    // returns its facade. Refused as DMarkdownBlock::add_block is.
    DMarkdownInline<Backend> add_inline(
        DDetached<DMarkdownInline<Backend>>&& _child
    )
    {
        return DMarkdownInline<Backend>(
            ::djinterp::internal::detached_access::adopt(m_node, _child));
    }


private:
    static
    const DMarkdownString& s_empty()
    {
        static const DMarkdownString e;
        return e;
    }


    node_type*  m_node;
};


// DMarkdownDetachedBlock
//   type: the handle Markdown's block factories return, owning a block made
// outside a tree until DMarkdownBlock::add_block takes it.
template<typename Backend>
using DMarkdownDetachedBlock = DDetached<DMarkdownBlock<Backend>>;

// DMarkdownDetachedInline
//   type: the handle Markdown's inline factories return, owning an inline
// made outside a tree until an add_inline takes it.
template<typename Backend>
using DMarkdownDetachedInline = DDetached<DMarkdownInline<Backend>>;


///////////////////////////////////////////////////////////////////////////////
///                IV.   INTERNAL EMISSION HELPERS                          ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace markdown
NS_INTERNAL

    // DMarkdownNodePtrs
    //   type: a node's children, as the default backend holds them.
    typedef std::vector<std::unique_ptr<DMarkdownDefaultNode>>
        DMarkdownNodePtrs;


    /// escaping

    // markdown_html_escape_helper
    //   function: writes an HTML-escaped copy of `_in` to `_out`.
    inline
    void markdown_html_escape_helper(
        std::ostream&      _out,
        const std::string& _in
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
    inline
    void markdown_html_attr_escape_helper(
        std::ostream&      _out,
        const std::string& _in
    )
    {
        markdown_html_escape_helper(_out, _in);
    }


    // markdown_md_escape_helper
    //   function: writes a markdown-escaped copy of `_in` to
    // `_out`. Backslash-escapes the CommonMark special characters
    // when they appear in text content -- '<' and '&' among them,
    // so text never reads back as HTML or as an entity.
    inline
    void markdown_md_escape_helper(
        std::ostream&      _out,
        const std::string& _in
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
                case '|':  case '~':  case '<':  case '&':
                    _out << '\\' << c;
                    break;
                default:
                    _out << c;
                    break;
            }
        }
    }


    // markdown_url_encode_helper
    //   function: a destination as an href or a src: percent-encoded
    // as CommonMark's renderers normalise it -- every byte but letters,
    // digits and ;/?:@&=+$,-_.!~*'()# becomes %XX, and an existing %XX
    // is kept -- then escaped as an attribute's value.
    inline
    void markdown_url_encode_helper(
        std::ostream&      _out,
        const std::string& _url
    )
    {
        static const char* const hex  = "0123456789ABCDEF";
        static const char* const keep = ";/?:@&=+$,-_.!~*'()#";
        std::string              encoded;

        for (std::size_t i = 0; i < _url.size(); ++i)
        {
            const unsigned char c = static_cast<unsigned char>(_url[i]);
            const bool escape = (c == '%') && (i + 2 < _url.size()) &&
                                std::isxdigit(static_cast<unsigned char>(
                                    _url[i + 1])) &&
                                std::isxdigit(static_cast<unsigned char>(
                                    _url[i + 2]));

            if ( escape || std::isalnum(c) ||
                 ( (c != 0) && (std::strchr(keep, c) != nullptr) ) )
            {
                encoded += static_cast<char>(c);
                continue;
            }
            encoded += '%';
            encoded += hex[c >> 4];
            encoded += hex[c & 0x0F];
        }
        markdown_html_escape_helper(_out, encoded);
    }


    // markdown_longest_run_helper
    //   function: the length of the longest run of `_c` in `_in`.
    inline
    std::size_t markdown_longest_run_helper(
        const std::string& _in,
        char               _c
    )
    {
        std::size_t longest = 0;
        std::size_t run     = 0;
        for (std::size_t i = 0; i < _in.size(); ++i)
        {
            run     = (_in[i] == _c) ? (run + 1) : 0;
            longest = (run > longest) ? run : longest;
        }
        return longest;
    }


    // markdown_trim_newlines_helper
    //   function: `_in` without its trailing newlines.
    inline
    std::string markdown_trim_newlines_helper(
        const std::string& _in
    )
    {
        std::size_t end = _in.size();
        while ((end > 0) && (_in[end - 1] == '\n'))
        {
            --end;
        }
        return _in.substr(0, end);
    }


    // markdown_interrupts_helper
    //   function: whether a block can follow a paragraph on the next
    // line, in a tight list item, without joining it: CommonMark
    // lets a bullet list, an ordered list starting at 1, a fence, an
    // ATX heading or a block quote interrupt a paragraph.
    inline
    bool markdown_interrupts_helper(
        const DMarkdownDefaultNode&  _node
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        return (_node.block_kind == K::unordered_list)    ||
               ( (_node.block_kind == K::ordered_list) &&
                 (_node.list_start == 1) )                 ||
               (_node.block_kind == K::fenced_code_block) ||
               (_node.block_kind == K::block_quote)       ||
               is_heading_block_kind(_node.block_kind);
    }


    // markdown_is_tight_helper
    //   function: whether a list is written tight: unless it says it
    // is loose, and as long as every block in each of its items can
    // follow the one before it on the next line. Two paragraphs in an
    // item need a blank line between them, which CommonMark reads as
    // loose, so such a list is written loose in every rendering.
    inline
    bool markdown_is_tight_helper(
        const DMarkdownDefaultNode&  _list
    )
    {
        if (_list.list_loose)
        {
            return false;
        }
        for (std::size_t i = 0; i < _list.children.size(); ++i)
        {
            const DMarkdownDefaultNode* const item = _list.children[i].get();
            bool                              any  = false;

            for (std::size_t j = 0;
                 (item != nullptr) && (j < item->children.size());
                 ++j)
            {
                if (!item->children[j])
                {
                    continue;
                }
                if ( any && (!markdown_interrupts_helper(*item->children[j])) )
                {
                    return false;
                }
                any = true;
            }
        }
        return true;
    }


    /// HTML

    // markdown_inline_tag_helper
    //   function: the element an inline wrapping other inlines is
    // written as in HTML, or null for one that isn't such a wrapper.
    inline
    const char* markdown_inline_tag_helper(
        ::djinterp::markdown::DMarkdownInlineKind _kind
    )
    {
        using K = ::djinterp::markdown::DMarkdownInlineKind;

        return (_kind == K::emphasis)      ? "em"
             : (_kind == K::strong)        ? "strong"
             : (_kind == K::strikethrough) ? "del"
             : (_kind == K::highlight)     ? "mark"
             : (_kind == K::subscript)     ? "sub"
             : (_kind == K::superscript)   ? "sup"
             : (_kind == K::underline)     ? "u"
             :                               nullptr;
    }


    void markdown_emit_inline_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node);

    // markdown_emit_inlines_html_helper
    //   function: a node's inline children, in HTML.
    inline
    void markdown_emit_inlines_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        for (std::size_t i = 0; i < _node.children.size(); ++i)
        {
            if (_node.children[i])
            {
                markdown_emit_inline_html_helper(_out, *_node.children[i]);
            }
        }
    }


    // markdown_emit_link_html_helper
    //   function: a link, an image or an autolink, in HTML.
    inline
    void markdown_emit_link_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        using K = ::djinterp::markdown::DMarkdownInlineKind;

        const bool image = (_node.inline_kind == K::image);
        const bool email = (_node.inline_kind == K::autolink_email);

        _out << (image ? "<img src=\"" : "<a href=\"")
             << (email ? "mailto:" : "");
        markdown_url_encode_helper(_out, _node.url);
        _out << '"';
        if (image)
        {
            _out << " alt=\"";
            markdown_html_attr_escape_helper(_out, _node.alt_text);
            _out << '"';
        }
        if (!_node.title.empty())
        {
            _out << " title=\"";
            markdown_html_attr_escape_helper(_out, _node.title);
            _out << '"';
        }
        if (image)
        {
            _out << " />";
            return;
        }
        _out << '>';
        if (_node.inline_kind == K::link)
        {
            markdown_emit_inlines_html_helper(_out, _node);
        }
        else
        {
            markdown_html_escape_helper(_out, _node.url);
        }
        _out << "</a>";
    }


    // markdown_emit_inline_html_helper
    //   function: walks an inline subtree under HTML emission
    // rules.
    inline
    void markdown_emit_inline_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        using K = ::djinterp::markdown::DMarkdownInlineKind;

        const char* const tag = markdown_inline_tag_helper(_node.inline_kind);

        if (tag != nullptr)
        {
            _out << '<' << tag << '>';
            markdown_emit_inlines_html_helper(_out, _node);
            _out << "</" << tag << '>';
            return;
        }

        switch (_node.inline_kind)
        {
            case K::soft_break:
                _out << '\n';
                break;
            case K::hard_break:
            case K::hard_line_break:
                _out << "<br />\n";
                break;
            case K::code_span:
                _out << "<code>";
                markdown_html_escape_helper(_out, _node.text);
                _out << "</code>";
                break;
            case K::link:
            case K::image:
            case K::autolink:
            case K::autolink_email:
                markdown_emit_link_html_helper(_out, _node);
                break;
            case K::html_inline:
            case K::entity:
                _out << _node.text;
                break;
            case K::math_inline:
                _out << "<span class=\"math inline\">";
                markdown_html_escape_helper(_out, _node.text);
                _out << "</span>";
                break;
            default:
                // text, and the kinds with no element of their own
                markdown_html_escape_helper(_out, _node.text);
                break;
        }
    }


    void markdown_emit_block_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node);

    // markdown_emit_blocks_html_helper
    //   function: a node's block children, in HTML.
    inline
    void markdown_emit_blocks_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        for (std::size_t i = 0; i < _node.children.size(); ++i)
        {
            if (_node.children[i])
            {
                markdown_emit_block_html_helper(_out, *_node.children[i]);
            }
        }
    }


    // markdown_emit_item_html_helper
    //   function: one list item. In a tight list a paragraph is
    // written as its inlines alone, as CommonMark has it:
    // <li>text</li>, and a block after inlines starts a new line.
    inline
    void markdown_emit_item_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _item,
        bool                         _tight
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        bool inline_last = false;

        _out << "<li>";
        if (_item.is_task)
        {
            _out << "<input type=\"checkbox\" disabled"
                 << (_item.task_checked ? " checked" : "") << " /> ";
        }
        if (!_tight)
        {
            _out << '\n';
        }
        for (std::size_t i = 0; i < _item.children.size(); ++i)
        {
            const DMarkdownDefaultNode* const child = _item.children[i].get();

            if (child == nullptr)
            {
                continue;
            }
            if ( _tight && (child->block_kind == K::paragraph) )
            {
                markdown_emit_inlines_html_helper(_out, *child);
                inline_last = true;
                continue;
            }
            if (inline_last)
            {
                _out << '\n';
            }
            markdown_emit_block_html_helper(_out, *child);
            inline_last = false;
        }
        _out << "</li>\n";
    }


    // markdown_emit_list_html_helper
    //   function: an ordered or unordered list, and its items.
    inline
    void markdown_emit_list_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _list
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        const bool ordered = (_list.block_kind == K::ordered_list);
        const bool tight   = markdown_is_tight_helper(_list);

        _out << (ordered ? "<ol" : "<ul");
        if ( ordered && (_list.list_start != 1) )
        {
            _out << " start=\"" << _list.list_start << '"';
        }
        _out << ">\n";
        for (std::size_t i = 0; i < _list.children.size(); ++i)
        {
            const DMarkdownDefaultNode* const child = _list.children[i].get();

            if (child == nullptr)
            {
                continue;
            }
            if ( (child->block_kind == K::list_item) ||
                 (child->block_kind == K::task_list_item) )
            {
                markdown_emit_item_html_helper(_out, *child, tight);
            }
            else
            {
                markdown_emit_block_html_helper(_out, *child);
            }
        }
        _out << (ordered ? "</ol>\n" : "</ul>\n");
    }


    // markdown_emit_row_html_helper
    //   function: a table row: header cells as th, body cells as td,
    // each with its column's alignment, as GFM writes them.
    inline
    void markdown_emit_row_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _table,
        const DMarkdownDefaultNode&  _row,
        bool                         _header
    )
    {
        using A = ::djinterp::markdown::DMarkdownTableAlignment;

        const char* const cell = _header ? "th" : "td";
        std::size_t       column = 0;

        _out << "<tr>\n";
        for (std::size_t i = 0; i < _row.children.size(); ++i)
        {
            if (!_row.children[i])
            {
                continue;
            }
            const A align = (column < _table.column_alignments.size())
                                ? _table.column_alignments[column]
                                : A::none;
            _out << '<' << cell
                 << ( (align == A::left)   ? " align=\"left\""
                    : (align == A::center) ? " align=\"center\""
                    : (align == A::right)  ? " align=\"right\""
                    :                        "" )
                 << '>';
            markdown_emit_inlines_html_helper(_out, *_row.children[i]);
            _out << "</" << cell << ">\n";
            ++column;
        }
        _out << "</tr>\n";
    }


    // markdown_emit_rows_html_helper
    //   function: a table section's rows, or a row standing directly
    // in the table.
    inline
    void markdown_emit_rows_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _table,
        const DMarkdownDefaultNode&  _section,
        bool                         _header
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        if (_section.block_kind == K::table_row)
        {
            markdown_emit_row_html_helper(_out, _table, _section, _header);
            return;
        }
        for (std::size_t i = 0; i < _section.children.size(); ++i)
        {
            if (_section.children[i])
            {
                markdown_emit_row_html_helper(_out, _table,
                                              *_section.children[i], _header);
            }
        }
    }


    // markdown_emit_table_html_helper
    //   function: a table: its header section as thead, its body
    // section, and any row standing in the table, as tbody.
    inline
    void markdown_emit_table_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _table
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        _out << "<table>\n";
        for (std::size_t i = 0; i < _table.children.size(); ++i)
        {
            const DMarkdownDefaultNode* const part = _table.children[i].get();

            if (part == nullptr)
            {
                continue;
            }
            const bool header = (part->block_kind == K::table_header);
            _out << (header ? "<thead>\n" : "<tbody>\n");
            markdown_emit_rows_html_helper(_out, _table, *part, header);
            _out << (header ? "</thead>\n" : "</tbody>\n");
        }
        _out << "</table>\n";
    }


    // markdown_emit_code_html_helper
    //   function: a code block, with its language as a class.
    inline
    void markdown_emit_code_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
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
    }


    // markdown_emit_block_html_helper
    //   function: walks a block subtree under HTML emission
    // rules. Recurses into children for container blocks.
    inline
    void markdown_emit_block_html_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        switch (_node.block_kind)
        {
            case K::heading_1: case K::heading_2: case K::heading_3:
            case K::heading_4: case K::heading_5: case K::heading_6:
            {
                const int lvl = heading_level_from_kind(_node.block_kind);
                _out << "<h" << lvl << '>';
                markdown_emit_inlines_html_helper(_out, _node);
                _out << "</h" << lvl << ">\n";
                break;
            }
            case K::paragraph:
                _out << "<p>";
                markdown_emit_inlines_html_helper(_out, _node);
                _out << "</p>\n";
                break;
            case K::thematic_break:
                _out << "<hr />\n";
                break;
            case K::indented_code_block:
            case K::fenced_code_block:
                markdown_emit_code_html_helper(_out, _node);
                break;
            case K::block_quote:
                _out << "<blockquote>\n";
                markdown_emit_blocks_html_helper(_out, _node);
                _out << "</blockquote>\n";
                break;
            case K::ordered_list:
            case K::unordered_list:
                markdown_emit_list_html_helper(_out, _node);
                break;
            case K::list_item:
            case K::task_list_item:
                markdown_emit_item_html_helper(_out, _node, false);
                break;
            case K::table:
                markdown_emit_table_html_helper(_out, _node);
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
                // the document, and kinds with no element of their own
                markdown_emit_blocks_html_helper(_out, _node);
                break;
        }
    }


    /// Markdown

    // markdown_emit_code_span_md_helper
    //   function: a code span whose backticks outnumber any run of
    // them inside it, padded with a space where the content starts
    // or ends with a backtick, or both starts and ends with a space.
    inline
    void markdown_emit_code_span_md_helper(
        std::ostream&      _out,
        const std::string& _code
    )
    {
        const std::string fence(markdown_longest_run_helper(_code, '`') + 1,
                                '`');
        const bool        blank = (_code.find_first_not_of(' ') ==
                                   std::string::npos);
        const bool        pad   = (!_code.empty()) &&
                                  ( (_code[0] == '`') ||
                                    (_code[_code.size() - 1] == '`') ||
                                    ( (_code[0] == ' ') &&
                                      (_code[_code.size() - 1] == ' ') &&
                                      !blank ) );

        _out << fence << (pad ? " " : "") << _code << (pad ? " " : "")
             << fence;
    }


    // markdown_emit_destination_md_helper
    //   function: a link's destination: as it is, or in angle
    // brackets, with '<', '>' and '\' escaped, when it is empty or
    // holds a space, a parenthesis, an angle bracket or a backslash.
    inline
    void markdown_emit_destination_md_helper(
        std::ostream&      _out,
        const std::string& _url
    )
    {
        if ( (!_url.empty()) &&
             (_url.find_first_of(" ()<>\\") == std::string::npos) )
        {
            _out << _url;
            return;
        }
        _out << '<';
        for (std::size_t i = 0; i < _url.size(); ++i)
        {
            const char c = _url[i];
            if ( (c == '<') || (c == '>') || (c == '\\') )
            {
                _out << '\\';
            }
            _out << c;
        }
        _out << '>';
    }


    // markdown_emit_title_md_helper
    //   function: a link's title, if it has one: quoted, with '"' and
    // '\' escaped.
    inline
    void markdown_emit_title_md_helper(
        std::ostream&      _out,
        const std::string& _title
    )
    {
        if (_title.empty())
        {
            return;
        }
        _out << " \"";
        for (std::size_t i = 0; i < _title.size(); ++i)
        {
            if ( (_title[i] == '"') || (_title[i] == '\\') )
            {
                _out << '\\';
            }
            _out << _title[i];
        }
        _out << '"';
    }


    // markdown_emit_inline_md_helper
    //   function: round-trip emitter for an inline subtree.
    inline
    void markdown_emit_inline_md_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        using K = ::djinterp::markdown::DMarkdownInlineKind;

        const char* const mark = (_node.inline_kind == K::emphasis) ? "*"
                               : (_node.inline_kind == K::strong)   ? "**"
                               : (_node.inline_kind == K::strikethrough)
                                                                    ? "~~"
                               : (_node.inline_kind == K::link)     ? "["
                               :                                      nullptr;

        if (mark != nullptr)
        {
            _out << mark;
            for (std::size_t i = 0; i < _node.children.size(); ++i)
            {
                if (_node.children[i])
                {
                    markdown_emit_inline_md_helper(_out, *_node.children[i]);
                }
            }
            if (_node.inline_kind != K::link)
            {
                _out << mark;
                return;
            }
            _out << "](";
            markdown_emit_destination_md_helper(_out, _node.url);
            markdown_emit_title_md_helper(_out, _node.title);
            _out << ')';
            return;
        }

        switch (_node.inline_kind)
        {
            case K::text:
                markdown_md_escape_helper(_out, _node.text);
                break;
            case K::soft_break:        _out << '\n';     break;
            case K::hard_break:        _out << "  \n";   break;
            case K::hard_line_break:   _out << "\\\n";   break;
            case K::code_span:
                markdown_emit_code_span_md_helper(_out, _node.text);
                break;
            case K::image:
                _out << "![";
                markdown_md_escape_helper(_out, _node.alt_text);
                _out << "](";
                markdown_emit_destination_md_helper(_out, _node.url);
                markdown_emit_title_md_helper(_out, _node.title);
                _out << ')';
                break;
            case K::autolink:
            case K::autolink_email:
                _out << '<' << _node.url << '>';
                break;
            case K::math_inline:
                _out << '$' << _node.text << '$';
                break;
            default:
                // html_inline, and kinds with no syntax of their own
                _out << _node.text;
                break;
        }
    }


    // markdown_emit_fenced_md_helper
    //   function: a fenced code block. The fence outnumbers any run
    // of its character in the code, so no line of the code can close
    // it, and a backtick fence becomes a tilde fence when the info
    // string holds a backtick, which a backtick fence can't carry.
    inline
    void markdown_emit_fenced_md_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        const std::string& info   = (!_node.info_string.empty())
                                        ? _node.info_string
                                        : _node.language;
        const char         symbol = ( (_node.fence_char == '~') ||
                                      (info.find('`') != std::string::npos) )
                                        ? '~'
                                        : '`';
        std::size_t        length = (_node.fence_length > 3)
                                        ? _node.fence_length
                                        : 3;
        const std::size_t  inside = markdown_longest_run_helper(_node.text,
                                                                symbol);

        if (inside >= length)
        {
            length = inside + 1;
        }

        const std::string fence(length, symbol);

        _out << fence << info << '\n' << _node.text;
        if ((!_node.text.empty()) && (_node.text.back() != '\n'))
        {
            _out << '\n';
        }
        _out << fence << "\n\n";
    }


    // markdown_emit_prefixed_md_helper
    //   function: writes `_text` a line at a time: the first line
    // after `_first`, every later line that isn't empty after
    // `_rest`, and an empty line after `_blank`.
    inline
    void markdown_emit_prefixed_md_helper(
        std::ostream&      _out,
        const std::string& _text,
        const std::string& _first,
        const std::string& _rest,
        const std::string& _blank
    )
    {
        std::size_t start = 0;

        for (;;)
        {
            const std::size_t nl   = _text.find('\n', start);
            const std::string line = _text.substr(start,
                                                  (nl == std::string::npos)
                                                      ? std::string::npos
                                                      : (nl - start));

            _out << ( (start == 0)   ? _first
                    : line.empty()   ? _blank
                    :                  _rest )
                 << line << '\n';
            if (nl == std::string::npos)
            {
                break;
            }
            start = nl + 1;
        }
    }


    void markdown_emit_block_md_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node);

    // markdown_emit_inlines_md_helper
    //   function: a node's inline children, as Markdown.
    inline
    void markdown_emit_inlines_md_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        for (std::size_t i = 0; i < _node.children.size(); ++i)
        {
            if (_node.children[i])
            {
                markdown_emit_inline_md_helper(_out, *_node.children[i]);
            }
        }
    }

    // markdown_list_bullet_helper
    //   function: an unordered list's bullet: its first item's, when
    // that is '-', '+' or '*', and '-' otherwise. CommonMark gives a
    // list one bullet, so the first item's is every item's.
    inline
    char markdown_list_bullet_helper(
        const DMarkdownDefaultNode&  _list
    )
    {
        for (std::size_t i = 0; i < _list.children.size(); ++i)
        {
            if (_list.children[i])
            {
                const char c = _list.children[i]->list_bullet;
                return ( (c == '+') || (c == '*') ) ? c : '-';
            }
        }
        return '-';
    }


    void markdown_emit_list_md_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _list,
        char                         _mark);

    // markdown_render_children_md_helper
    //   function: a container's blocks as Markdown, one text each, as
    // CommonMark would read them back. A list right after a list of
    // its kind takes the other marker ('*' or '-', ')' or '.') so the
    // two stay two lists, and an indented code block after a list is
    // preceded by cmark's "<!-- end list -->", so it doesn't join the
    // list's last item.
    inline
    std::vector<std::string> markdown_render_children_md_helper(
        const DMarkdownDefaultNode&  _node
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        std::vector<std::string>    out;
        const DMarkdownDefaultNode* previous = nullptr;
        char                        bullet   = 0;
        char                        delim    = 0;

        for (std::size_t i = 0; i < _node.children.size(); ++i)
        {
            const DMarkdownDefaultNode* const child = _node.children[i].get();
            std::ostringstream                one;

            if (child == nullptr)
            {
                continue;
            }
            const bool after = (previous != nullptr) &&
                               (previous->block_kind == child->block_kind);
            if (child->block_kind == K::unordered_list)
            {
                const char first = markdown_list_bullet_helper(*child);
                bullet = (after && (first == bullet))
                             ? ((first == '-') ? '*' : '-')
                             : first;
                markdown_emit_list_md_helper(one, *child, bullet);
            }
            else if (child->block_kind == K::ordered_list)
            {
                delim = (after && (delim == '.')) ? ')' : '.';
                markdown_emit_list_md_helper(one, *child, delim);
            }
            else
            {
                if ( (child->block_kind == K::indented_code_block) &&
                     (previous != nullptr) &&
                     is_list_block_kind(previous->block_kind) )
                {
                    out.push_back("<!-- end list -->\n\n");
                }
                markdown_emit_block_md_helper(one, *child);
            }
            out.push_back(one.str());
            previous = child;
        }
        return out;
    }


    // markdown_emit_item_md_helper
    //   function: one list item: its marker, a task's box, and its
    // blocks, every later line indented to the content's column so
    // that it stays in the item. In a tight list each block follows
    // the one before it on the next line; in a loose one, after a
    // blank line.
    inline
    void markdown_emit_item_md_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _item,
        const std::string&           _marker,
        bool                         _tight
    )
    {
        const std::vector<std::string> blocks =
            markdown_render_children_md_helper(_item);
        std::string                    body;

        for (std::size_t i = 0; i < blocks.size(); ++i)
        {
            if (i > 0)
            {
                body += _tight ? "\n" : "\n\n";
            }
            body += markdown_trim_newlines_helper(blocks[i]);
        }

        std::string first = _marker;
        if (_item.is_task)
        {
            first += _item.task_checked ? "[x] " : "[ ] ";
        }
        markdown_emit_prefixed_md_helper(_out, body, first,
                                         std::string(_marker.size(), ' '),
                                         "");
    }


    // markdown_emit_list_md_helper
    //   function: a list's items, each with its marker: an ordered
    // list's numbers, from its start, with `_mark` as the delimiter,
    // or `_mark` as every item's bullet. A loose list puts a blank
    // line between items, and every list ends with one.
    inline
    void markdown_emit_list_md_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _list,
        char                         _mark
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        const bool ordered = (_list.block_kind == K::ordered_list);
        const bool tight   = markdown_is_tight_helper(_list);
        long       number  = (_list.list_start > 0) ? _list.list_start : 0;
        bool       first   = true;

        for (std::size_t i = 0; i < _list.children.size(); ++i)
        {
            const DMarkdownDefaultNode* const item = _list.children[i].get();

            if (item == nullptr)
            {
                continue;
            }
            if ( (!first) && (!tight) )
            {
                _out << '\n';
            }
            markdown_emit_item_md_helper(
                _out, *item,
                ordered ? (std::to_string(number++) + _mark + ' ')
                        : (std::string(1, _mark) + ' '),
                tight);
            first = false;
        }
        _out << '\n';
    }


    // markdown_emit_block_md_helper
    //   function: round-trip emitter for a block subtree.
    inline
    void markdown_emit_block_md_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        switch (_node.block_kind)
        {
            case K::heading_1: case K::heading_2: case K::heading_3:
            case K::heading_4: case K::heading_5: case K::heading_6:
                _out << std::string(static_cast<std::size_t>(
                            heading_level_from_kind(_node.block_kind)), '#')
                     << ' ';
                markdown_emit_inlines_md_helper(_out, _node);
                _out << "\n\n";
                break;
            case K::paragraph:
                markdown_emit_inlines_md_helper(_out, _node);
                _out << "\n\n";
                break;
            case K::thematic_break:
                _out << "***\n\n";
                break;
            case K::fenced_code_block:
                markdown_emit_fenced_md_helper(_out, _node);
                break;
            case K::indented_code_block:
                markdown_emit_prefixed_md_helper(
                    _out, markdown_trim_newlines_helper(_node.text),
                    "    ", "    ", "");
                _out << '\n';
                break;
            case K::block_quote:
            {
                std::string inner;
                for (const std::string& block :
                     markdown_render_children_md_helper(_node))
                {
                    inner += block;
                }
                markdown_emit_prefixed_md_helper(
                    _out, markdown_trim_newlines_helper(inner),
                    "> ", "> ", ">");
                _out << '\n';
                break;
            }
            case K::ordered_list:
                markdown_emit_list_md_helper(_out, _node, '.');
                break;
            case K::unordered_list:
                markdown_emit_list_md_helper(
                    _out, _node, markdown_list_bullet_helper(_node));
                break;
            case K::list_item:
            case K::task_list_item:
                markdown_emit_item_md_helper(_out, _node, "- ", true);
                break;
            case K::html_block:
            case K::raw_block:
                _out << markdown_trim_newlines_helper(_node.text) << "\n\n";
                break;
            case K::math_block:
                _out << "$$\n" << markdown_trim_newlines_helper(_node.text)
                     << "\n$$\n\n";
                break;
            default:
                // the document, and kinds with no syntax of their own
                for (const std::string& block :
                     markdown_render_children_md_helper(_node))
                {
                    _out << block;
                }
                break;
        }
    }


    /// plain text

    // markdown_emit_block_plaintext_helper
    //   function: strips formatting and emits text content only: an
    // image's alternative text, and an autolink's address. A block's
    // text ends its line; a table's cells are separated by tabs, and
    // its rows end their lines.
    inline
    void markdown_emit_block_plaintext_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        using K = ::djinterp::markdown::DMarkdownBlockKind;

        using I = ::djinterp::markdown::DMarkdownInlineKind;

        const bool row = _node.is_block && (_node.block_kind == K::table_row);
        const I    ik  = _node.inline_kind;

        _out << _node.text;
        if ( (!_node.is_block) && (ik == I::image) )
        {
            _out << _node.alt_text;
        }
        else if ( (!_node.is_block) &&
                  ( (ik == I::autolink) || (ik == I::autolink_email) ) )
        {
            _out << _node.url;
        }
        for (std::size_t i = 0, cells = 0; i < _node.children.size(); ++i)
        {
            if (!_node.children[i])
            {
                continue;
            }
            if ( row && (cells++ > 0) )
            {
                _out << '\t';
            }
            markdown_emit_block_plaintext_helper(_out, *_node.children[i]);
        }
        if (!_node.is_block)
        {
            return;
        }
        // a code block's text ends its own last line
        const K    k     = _node.block_kind;
        const bool ended = (!_node.text.empty()) &&
                           (_node.text[_node.text.size() - 1] == '\n');
        if ( (k == K::paragraph)      || is_heading_block_kind(k) ||
             (k == K::thematic_break) || (k == K::table_row)      ||
             ( is_code_block_kind(k) && (!ended) ) )
        {
            _out << '\n';
        }
    }


    /// CommonMark XML

    // markdown_xml_escape_helper
    //   function: XML character data or an attribute's value: '&',
    // '<', '>' and '"' as entities.
    inline
    void markdown_xml_escape_helper(
        std::ostream&      _out,
        const std::string& _in
    )
    {
        for (std::size_t i = 0; i < _in.size(); ++i)
        {
            switch (_in[i])
            {
                case '&':  _out << "&amp;";  break;
                case '<':  _out << "&lt;";   break;
                case '>':  _out << "&gt;";   break;
                case '"':  _out << "&quot;"; break;
                default:   _out << _in[i];   break;
            }
        }
    }


    // markdown_xml_name_helper
    //   function: a node's element name in CommonMark's XML: cmark's
    // names for CommonMark's kinds, and the kind's own name for the
    // extensions (table, table_row, strikethrough, math_inline, ...).
    inline
    const char* markdown_xml_name_helper(
        const DMarkdownDefaultNode&  _node
    )
    {
        using B = ::djinterp::markdown::DMarkdownBlockKind;
        using I = ::djinterp::markdown::DMarkdownInlineKind;

        if (_node.is_block)
        {
            const B k = _node.block_kind;
            return (k == B::document)            ? "document"
                 : is_heading_block_kind(k)      ? "heading"
                 : (k == B::paragraph)           ? "paragraph"
                 : (k == B::thematic_break)      ? "thematic_break"
                 : ( is_code_block_kind(k) && (k != B::math_block) )
                                                 ? "code_block"
                 : (k == B::block_quote)         ? "block_quote"
                 : is_list_block_kind(k)         ? "list"
                 : is_list_item_block_kind(k)    ? "item"
                 : (k == B::html_block)          ? "html_block"
                 : (k == B::table)               ? "table"
                 : (k == B::table_header)        ? "table_header"
                 : (k == B::table_body)          ? "table_body"
                 : (k == B::table_row)           ? "table_row"
                 : (k == B::table_cell)          ? "table_cell"
                 : (k == B::math_block)          ? "math_block"
                 :                                 "custom_block";
        }
        const I k = _node.inline_kind;
        return (k == I::text)            ? "text"
             : (k == I::soft_break)      ? "softbreak"
             : is_break_inline_kind(k)   ? "linebreak"
             : (k == I::emphasis)        ? "emph"
             : (k == I::strong)          ? "strong"
             : (k == I::strikethrough)   ? "strikethrough"
             : (k == I::code_span)       ? "code"
             : is_link_inline_kind(k)    ? "link"
             : (k == I::image)           ? "image"
             : (k == I::html_inline)     ? "html_inline"
             : (k == I::math_inline)     ? "math_inline"
             :                             "custom_inline";
    }


    // markdown_emit_xml_attributes_helper
    //   function: a node's attributes in CommonMark's XML: a
    // heading's level, a list's type, start, delimiter and
    // tightness, a code block's info, a link's or an image's
    // destination and title, and a task item's state.
    inline
    void markdown_emit_xml_attributes_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node
    )
    {
        using B = ::djinterp::markdown::DMarkdownBlockKind;
        using I = ::djinterp::markdown::DMarkdownInlineKind;

        if (!_node.is_block)
        {
            const I k = _node.inline_kind;
            if ( is_link_inline_kind(k) || (k == I::image) )
            {
                _out << " destination=\""
                     << ((k == I::autolink_email) ? "mailto:" : "");
                markdown_xml_escape_helper(_out, _node.url);
                _out << "\" title=\"";
                markdown_xml_escape_helper(_out, _node.title);
                _out << '"';
            }
            return;
        }
        const B k = _node.block_kind;
        if (k == B::document)
        {
            _out << " xmlns=\"http://commonmark.org/xml/1.0\"";
        }
        else if (is_heading_block_kind(k))
        {
            _out << " level=\"" << heading_level_from_kind(k) << '"';
        }
        else if (is_list_block_kind(k))
        {
            if (k == B::ordered_list)
            {
                _out << " type=\"ordered\" start=\"" << _node.list_start
                     << "\" delim=\"period\"";
            }
            else
            {
                _out << " type=\"bullet\"";
            }
            _out << " tight=\""
                 << (markdown_is_tight_helper(_node) ? "true" : "false")
                 << '"';
        }
        else if ( is_code_block_kind(k) && (k != B::math_block) &&
                  ( (!_node.info_string.empty()) ||
                    (!_node.language.empty()) ) )
        {
            _out << " info=\"";
            markdown_xml_escape_helper(_out, (!_node.info_string.empty())
                                                 ? _node.info_string
                                                 : _node.language);
            _out << '"';
        }
        else if (_node.is_task)
        {
            _out << " task=\"" << (_node.task_checked ? "checked" : "open")
                 << '"';
        }
    }


    // markdown_xml_literal_helper
    //   function: the text a node carries in CommonMark's XML as its
    // character data, or null for one whose children are its content.
    inline
    const std::string* markdown_xml_literal_helper(
        const DMarkdownDefaultNode&  _node
    )
    {
        using I = ::djinterp::markdown::DMarkdownInlineKind;

        if (_node.is_block)
        {
            const unsigned raw = ::djinterp::markdown::mbc_code |
                                 ::djinterp::markdown::mbc_raw;
            return ((markdown_category_for_block_kind(_node.block_kind) & raw)
                    != 0u)
                       ? &_node.text
                       : nullptr;
        }
        const I k = _node.inline_kind;
        if ( (k == I::autolink) || (k == I::autolink_email) )
        {
            return &_node.url;
        }
        return ( (k == I::image) || is_container_inline_kind(k) ||
                 is_break_inline_kind(k) )
                   ? nullptr
                   : &_node.text;
    }


    // markdown_emit_block_xml_helper
    //   function: a node and its subtree in CommonMark's XML, indented
    // two spaces a level as cmark writes it. A node's text is kept as
    // it is (xml:space="preserve"); an autolink's is its address, as
    // a text child, and an image's alternative text is a text child.
    inline
    void markdown_emit_block_xml_helper(
        std::ostream&                _out,
        const DMarkdownDefaultNode&  _node,
        std::size_t                  _depth
    )
    {
        using I = ::djinterp::markdown::DMarkdownInlineKind;

        const std::string        indent(2 * _depth, ' ');
        const char* const        name    = markdown_xml_name_helper(_node);
        const std::string* const literal = markdown_xml_literal_helper(_node);
        const bool               auto_   = (!_node.is_block) &&
                                           ( (_node.inline_kind ==
                                              I::autolink) ||
                                             (_node.inline_kind ==
                                              I::autolink_email) );
        const bool               image   = (!_node.is_block) &&
                                           (_node.inline_kind == I::image);

        _out << indent << '<' << name;
        markdown_emit_xml_attributes_helper(_out, _node);
        if ( (literal != nullptr) && (!auto_) )
        {
            _out << " xml:space=\"preserve\">";
            markdown_xml_escape_helper(_out, *literal);
            _out << "</" << name << ">\n";
            return;
        }
        if ( (!auto_) && (!image) && _node.children.empty() )
        {
            _out << " />\n";
            return;
        }
        _out << ">\n";
        if ( auto_ || (image && (!_node.alt_text.empty())) )
        {
            _out << indent << "  <text xml:space=\"preserve\">";
            markdown_xml_escape_helper(_out, auto_ ? _node.url
                                                   : _node.alt_text);
            _out << "</text>\n";
        }
        for (std::size_t i = 0; i < _node.children.size(); ++i)
        {
            if (_node.children[i])
            {
                markdown_emit_block_xml_helper(_out, *_node.children[i],
                                               _depth + 1);
            }
        }
        _out << indent << "</" << name << ">\n";
    }

NS_END  // internal
namespace markdown {


///////////////////////////////////////////////////////////////////////////////
///                V.   DMarkdownDocument<Backend>                         ///
///////////////////////////////////////////////////////////////////////////////

// DMarkdownDocument
//   class: facade over the backend's storage document type plus
// flavor metadata. Owns the storage by value and exposes the
// four render targets.
template<typename Backend>
class DMarkdownDocument
{
public:
    using backend_type  = Backend;
    using document_type = typename Backend::document_type;
    using node_type     = typename Backend::block_type;
    using block_facade  = DMarkdownBlock<Backend>;


    /// constructors

    DMarkdownDocument()
    :   m_doc(Backend::make_markdown_document())
    {}

    explicit
    DMarkdownDocument(
        DMarkdownFlavor     _flavor
    )
    :   m_doc(Backend::make_markdown_document())
    {
        m_doc.flavor = _flavor;
    }

    explicit
    DMarkdownDocument(
        document_type&&     _doc
    )
    :   m_doc(std::move(_doc))
    {}


    /// flavor

    // flavor
    DMarkdownFlavor flavor() const
    {
        return m_doc.flavor;
    }

    // get_flavor
    DMarkdownFlavor get_flavor() const
    {
        return m_doc.flavor;
    }

    // set_flavor
    void set_flavor(
        DMarkdownFlavor _flavor
    )
    {
        m_doc.flavor = _flavor;
    }


    /// root access

    // root_block
    //   function: returns a facade for the document root block.
    block_facade root_block() const
    {
        return block_facade(m_doc.root.get());
    }

    // backend_document
    //   function: const access to the underlying storage doc.
    const document_type& backend_document() const
    {
        return m_doc;
    }


    /// rendering

    // render_to_markdown
    //   function: emits this document as markdown source. The
    // emission targets a CommonMark-compatible round-trip; for
    // GFM / Pandoc / etc. extensions the bundled emitter falls
    // back to CommonMark-equivalent output where possible.
    void render_to_markdown(
        std::ostream& _out
    ) const
    {
        if (m_doc.root)
        {
            ::djinterp::internal::markdown_emit_block_md_helper(
                _out, *m_doc.root);
        }
    }

    // render_to_markdown
    //   function: convenience overload returning a string.
    DMarkdownString render_to_markdown() const
    {
        std::ostringstream  oss;
        this->render_to_markdown(oss);
        return oss.str();
    }


    // render_to_html
    //   function: emits this document as an HTML5 fragment.
    void render_to_html(
        std::ostream& _out
    ) const
    {
        if (m_doc.root)
        {
            ::djinterp::internal::markdown_emit_block_html_helper(
                _out, *m_doc.root);
        }
    }

    DMarkdownString render_to_html() const
    {
        std::ostringstream  oss;
        this->render_to_html(oss);
        return oss.str();
    }


    // render_to_plaintext
    //   function: emits this document with all formatting
    // stripped.
    void render_to_plaintext(
        std::ostream& _out
    ) const
    {
        if (m_doc.root)
        {
            ::djinterp::internal::markdown_emit_block_plaintext_helper(
                _out, *m_doc.root);
        }
    }

    DMarkdownString render_to_plaintext() const
    {
        std::ostringstream  oss;
        this->render_to_plaintext(oss);
        return oss.str();
    }


    // render_to_xml
    //   function: emits this document in CommonMark's XML form, as
    // cmark writes it: an element per node, indented two spaces a
    // level, text kept as it is. Kinds CommonMark lacks are written
    // under their own names (table, strikethrough, math_inline, ...).
    void render_to_xml(
        std::ostream& _out
    ) const
    {
        _out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
             << "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n";
        if (m_doc.root)
        {
            ::djinterp::internal::markdown_emit_block_xml_helper(
                _out, *m_doc.root, 0);
        }
    }

    DMarkdownString render_to_xml() const
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
///                VI.   DMarkdownDefaultBackend                           ///
///////////////////////////////////////////////////////////////////////////////

// DMarkdownDefaultBackend
//   struct: bundled in-memory backend for the markdown facades.
// Uses a single underlying storage type for both blocks and
// inlines (`internal::DMarkdownDefaultNode`) carrying both
// kind discriminators plus the union of all per-kind fields.
struct DMarkdownDefaultBackend
{
    using markdown_backend_tag = ::djinterp::markdown::DMarkdownDefaultBackendTag;

    using block_type    = ::djinterp::internal::DMarkdownDefaultNode;
    using inline_type   = ::djinterp::internal::DMarkdownDefaultNode;
    using document_type = ::djinterp::internal::DMarkdownDefaultDocument;

    // make_markdown_document
    //   function: factory for a fresh empty document with a
    // root `document` block ready to receive children.
    static
    document_type make_markdown_document()
    {
        document_type   d;
        d.flavor = DMarkdownFlavor::commonmark;
        std::unique_ptr<block_type> root(new block_type);
        root->is_block   = true;
        root->block_kind = DMarkdownBlockKind::document;
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
template<typename Backend>
inline
DMarkdownDocument<Backend> make_markdown_document(
    DMarkdownFlavor _flavor = DMarkdownFlavor::commonmark
)
{
    return DMarkdownDocument<Backend>(_flavor);
}


// make_paragraph
//   function: returns a new freestanding paragraph block. Like every factory
// below, it returns the node in a handle (DMarkdownDetachedBlock, or
// DMarkdownDetachedInline for an inline), which frees it unless add_block or
// add_inline takes it; get() gives its facade, to build under it first.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_paragraph()
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block   = true;
    n->block_kind = DMarkdownBlockKind::paragraph;
    return made;
}


// make_heading
//   function: returns a new freestanding heading block of the
// given level (1..6; clamped).
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_heading(
    int _level
)
{
    if (_level < 1) { _level = 1; }
    if (_level > 6) { _level = 6; }
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block      = true;
    n->block_kind    = heading_kind_from_level(_level);
    n->heading_level = _level;
    return made;
}


// make_fenced_code_block
//   function: returns a new freestanding fenced code block
// with the given language tag and code body.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_fenced_code_block(
    const DMarkdownString& _code,
    const DMarkdownString& _language = DMarkdownString()
)
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = true;
    n->block_kind  = DMarkdownBlockKind::fenced_code_block;
    n->text        = _code;
    n->language    = _language;
    n->info_string = _language;
    return made;
}


// make_thematic_break
//   function: returns a new freestanding thematic break block.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_thematic_break()
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block   = true;
    n->block_kind = DMarkdownBlockKind::thematic_break;
    return made;
}


// make_block_quote
//   function: returns a new freestanding block quote.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_block_quote()
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block   = true;
    n->block_kind = DMarkdownBlockKind::block_quote;
    return made;
}


// make_list
//   function: returns a new freestanding list block.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_list(
    bool _ordered = false,
    int  _start = 1
)
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block     = true;
    n->block_kind   = _ordered ? DMarkdownBlockKind::ordered_list
                               : DMarkdownBlockKind::unordered_list;
    n->list_ordered = _ordered;
    n->list_start   = _start;
    return made;
}


// make_list_item
//   function: returns a new freestanding list item block.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_list_item()
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = true;
    n->block_kind  = DMarkdownBlockKind::list_item;
    n->list_bullet = D_MARKDOWN_DEFAULT_BULLET;
    return made;
}


// make_table
//   function: returns a new freestanding table (GFM), with an alignment for
// each column, from the first; a column past the last given is unaligned.
// Its children are a header (make_table_header), then a body
// (make_table_body), each holding rows.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_table(
    const std::vector<DMarkdownTableAlignment>& _alignments =
        std::vector<DMarkdownTableAlignment>()
)
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block          = true;
    n->block_kind        = DMarkdownBlockKind::table;
    n->column_alignments = _alignments;
    return made;
}


// make_table_header
//   function: returns a new freestanding table header: the section holding
// a table's first row, whose cells head their columns.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_table_header()
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block   = true;
    n->block_kind = DMarkdownBlockKind::table_header;
    return made;
}


// make_table_body
//   function: returns a new freestanding table body: the section holding a
// table's other rows.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_table_body()
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block   = true;
    n->block_kind = DMarkdownBlockKind::table_body;
    return made;
}


// make_table_row
//   function: returns a new freestanding table row, holding cells.
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_table_row()
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block   = true;
    n->block_kind = DMarkdownBlockKind::table_row;
    return made;
}


// make_table_cell
//   function: returns a new freestanding table cell, whose content is
// inlines (add_inline).
template<typename Backend>
inline
DMarkdownDetachedBlock<Backend> make_table_cell()
{
    using node_t = typename Backend::block_type;
    DMarkdownDetachedBlock<Backend> made =
        DMarkdownDetachedBlock<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block   = true;
    n->block_kind = DMarkdownBlockKind::table_cell;
    return made;
}


// make_text
//   function: returns a new freestanding text inline.
template<typename Backend>
inline
DMarkdownDetachedInline<Backend> make_text(
    const DMarkdownString& _text
)
{
    using node_t = typename Backend::inline_type;
    DMarkdownDetachedInline<Backend> made =
        DMarkdownDetachedInline<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = false;
    n->inline_kind = DMarkdownInlineKind::text;
    n->text        = _text;
    return made;
}


// make_emphasis
//   function: returns a new freestanding emphasis inline.
template<typename Backend>
inline
DMarkdownDetachedInline<Backend> make_emphasis()
{
    using node_t = typename Backend::inline_type;
    DMarkdownDetachedInline<Backend> made =
        DMarkdownDetachedInline<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = false;
    n->inline_kind = DMarkdownInlineKind::emphasis;
    return made;
}


// make_strong
//   function: returns a new freestanding strong inline.
template<typename Backend>
inline
DMarkdownDetachedInline<Backend> make_strong()
{
    using node_t = typename Backend::inline_type;
    DMarkdownDetachedInline<Backend> made =
        DMarkdownDetachedInline<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = false;
    n->inline_kind = DMarkdownInlineKind::strong;
    return made;
}


// make_code_span
//   function: returns a new freestanding code span inline.
template<typename Backend>
inline
DMarkdownDetachedInline<Backend> make_code_span(
    const DMarkdownString& _code
)
{
    using node_t = typename Backend::inline_type;
    DMarkdownDetachedInline<Backend> made =
        DMarkdownDetachedInline<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = false;
    n->inline_kind = DMarkdownInlineKind::code_span;
    n->text        = _code;
    return made;
}


// make_link
//   function: returns a new freestanding link inline.
template<typename Backend>
inline
DMarkdownDetachedInline<Backend> make_link(
    const DMarkdownString& _url,
    const DMarkdownString& _title = DMarkdownString()
)
{
    using node_t = typename Backend::inline_type;
    DMarkdownDetachedInline<Backend> made =
        DMarkdownDetachedInline<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = false;
    n->inline_kind = DMarkdownInlineKind::link;
    n->url         = _url;
    n->title       = _title;
    return made;
}


// make_image
//   function: returns a new freestanding image inline.
template<typename Backend>
inline
DMarkdownDetachedInline<Backend> make_image(
    const DMarkdownString& _url,
    const DMarkdownString& _alt = DMarkdownString(),
    const DMarkdownString& _title = DMarkdownString()
)
{
    using node_t = typename Backend::inline_type;
    DMarkdownDetachedInline<Backend> made =
        DMarkdownDetachedInline<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = false;
    n->inline_kind = DMarkdownInlineKind::image;
    n->url         = _url;
    n->alt_text    = _alt;
    n->title       = _title;
    return made;
}


// make_autolink
//   function: returns a new freestanding autolink inline.
template<typename Backend>
inline
DMarkdownDetachedInline<Backend> make_autolink(
    const DMarkdownString& _url
)
{
    using node_t = typename Backend::inline_type;
    DMarkdownDetachedInline<Backend> made =
        DMarkdownDetachedInline<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = false;
    n->inline_kind = DMarkdownInlineKind::autolink;
    n->url         = _url;
    return made;
}


// make_hard_break
//   function: returns a new freestanding hard line break.
template<typename Backend>
inline
DMarkdownDetachedInline<Backend> make_hard_break()
{
    using node_t = typename Backend::inline_type;
    DMarkdownDetachedInline<Backend> made =
        DMarkdownDetachedInline<Backend>::make();
    node_t* n = made.backend_handle();
    n->is_block    = false;
    n->inline_kind = DMarkdownInlineKind::hard_break;
    return made;
}


}   // namespace markdown
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_TEXT_MARKDOWN_MARKDOWN_TEMPLATE_HPP
