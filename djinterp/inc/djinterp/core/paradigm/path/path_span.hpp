/*******************************************************************************
* djinterp [core]                                                  path_span.hpp
*
* The C++ face of c/text/path_span.h: a generic path read from its rendering.
*   path_span wraps a d_path_span -- it copies no text and allocates nothing
* but what str() and join() return -- and walks its labels with a forward
* iterator, so a loop, or from C++11 a range-for, reads a path label by label.
* It is C++98 throughout, as the C layer beneath it is C99.
*
* path:      /inc/djinterp/core/paradigm/path/path_span.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_PARADIGM_PATH_PATH_SPAN_HPP
#define DJINTERP_PARADIGM_PATH_PATH_SPAN_HPP 1

// std
#include <cstddef>   // std::size_t, std::ptrdiff_t
#include <iterator>  // std::forward_iterator_tag
#include <string>    // std::string
// djinterp
#include "../../../djinterp.hpp"        // framework root
#include "../../../c/text/path_span.h"  // d_path_span, d_path_grammar


NS_DJINTERP

// path_span
//   class: a path read from text it does not own, by a grammar
// (d_path_grammar) -- a file path, an n-ary tree's path or a URL's path
// alike. The text and the grammar must outlive it.
class path_span
{
public:
    // const_iterator
    //   class: steps through a path's labels, of which the anchor is none.
    class const_iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef d_path_label              value_type;
        typedef std::ptrdiff_t            difference_type;
        typedef const d_path_label*       pointer;
        typedef const d_path_label&       reference;

        const_iterator()
            : m_span(D_NULLPTR),
              m_cursor(0),
              m_label(no_label())
        {}

        explicit const_iterator(
            const d_path_span* _span
        )
            : m_span(_span),
              m_cursor(0),
              m_label(no_label())
        {
            step();
        }

        reference
        operator*() const
        {
            return m_label;
        }

        pointer
        operator->() const
        {
            return &m_label;
        }

        const_iterator&
        operator++()
        {
            step();

            return *this;
        }

        const_iterator
        operator++(int)
        {
            const const_iterator was(*this);

            step();

            return was;
        }

        bool
        operator==(
            const const_iterator& _other
        ) const
        {
            return ( (m_span == _other.m_span) &&
                     (m_cursor == _other.m_cursor) );
        }

        bool
        operator!=(
            const const_iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        static d_path_label
        no_label()
        {
            const d_path_label none = { D_NULLPTR, 0 };

            return none;
        }

        void
        step()
        {
            // past the last label, an iterator is end()'s
            if ( (m_span) &&
                 (!d_path_span_next(m_span,
                                    &m_cursor,
                                    &m_label)) )
            {
                m_span   = D_NULLPTR;
                m_cursor = 0;
                m_label  = no_label();
            }

            return;
        }

        const d_path_span* m_span;
        std::size_t        m_cursor;
        d_path_label       m_label;
    };

    path_span()
        : m_span(no_span())
    {}

    path_span(
        const char*           _text,
        const d_path_grammar& _grammar
    )
        : m_span(d_path_span_of(_text,
                                D_PATH_SPAN_TERMINATED,
                                &_grammar))
    {}

    path_span(
        const char*           _text,
        std::size_t           _length,
        const d_path_grammar& _grammar
    )
        : m_span(d_path_span_of(_text,
                                _length,
                                &_grammar))
    {}

    explicit path_span(
        const d_path_span& _span
    )
        : m_span(_span)
    {}

    const_iterator
    begin() const
    {
        return const_iterator(&m_span);
    }

    const_iterator
    end() const
    {
        return const_iterator();
    }

    // the parts and the order, as c/text/path_span.h has them
    std::size_t  level() const    { return d_path_span_level(&m_span); }
    bool         anchored() const { return d_path_span_is_anchored(&m_span); }
    d_path_label anchor() const   { return d_path_span_anchor(&m_span); }
    d_path_label last() const     { return d_path_span_last(&m_span); }
    d_path_label rest() const     { return d_path_span_rest(&m_span); }

    path_span
    parent() const
    {
        return path_span(d_path_span_parent(&m_span));
    }

    std::size_t
    meet(
        const path_span& _other
    ) const
    {
        return d_path_span_meet(&m_span,
                                &_other.m_span);
    }

    bool
    starts_with(
        const path_span& _prefix
    ) const
    {
        return d_path_span_starts_with(&m_span,
                                       &_prefix.m_span);
    }

    std::size_t
    render(
        char*       _buf,
        std::size_t _size
    ) const
    {
        return d_path_span_render(&m_span,
                                  _buf,
                                  _size);
    }

    // the rendering, as a string
    std::string
    str() const
    {
        const std::size_t need = d_path_span_render(&m_span,
                                                    D_NULLPTR,
                                                    0);
        std::string       text(need + 1, '\0');

        (void)d_path_span_render(&m_span,
                                 &text[0],
                                 text.size());
        text.resize(need);

        return text;
    }

    // _more taken from this path, rendered (d_path_span_render_join)
    std::string
    join(
        const path_span& _more
    ) const
    {
        const std::size_t need = d_path_span_render_join(&m_span,
                                                         &_more.m_span,
                                                         D_NULLPTR,
                                                         0);
        std::string       text(need + 1, '\0');

        (void)d_path_span_render_join(&m_span,
                                      &_more.m_span,
                                      &text[0],
                                      text.size());
        text.resize(need);

        return text;
    }

    const d_path_span&
    get() const
    {
        return m_span;
    }

private:
    static d_path_span
    no_span()
    {
        const d_path_span none = { D_NULLPTR, 0, 0, 0, D_NULLPTR };

        return none;
    }

    d_path_span m_span;
};

NS_END  // djinterp


#endif  // DJINTERP_PARADIGM_PATH_PATH_SPAN_HPP
