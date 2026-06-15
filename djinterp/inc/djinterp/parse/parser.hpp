/******************************************************************************
* djinterp [text]                                                  parser.hpp
*
*   The placeholder parser behind text_template, factored out so the structural
* front-half of interpolation stands on its own.  `parser` scans a format
* string with `{key}` placeholders ONCE into a flat segment list and owns both
* the format and the segments, so the cost of parsing is paid a single time and
* the produced structure can be queried, copied, or moved freely.
*
*   In the vocabulary of template.hpp the parser is the structure-building half
* of the text template-source-sink pipeline: it turns a format (the raw
* *template* text) into the segment list that text_template then renders
* against a source.  It is a *total* parser in the parse_outcome sense -- the
* grammar is lenient and never fails, so the error arm E is uninhabited and the
* whole format is consumed, leaving no remaining source.  It models the
* framework's parser contract from member_types.hpp, exposing `input_type` (the
* scanned element type) and `result_type` (the produced segment list).
*
*   Placeholders:
*     {key}        -- a placeholder naming `key`
*     { key }      -- surrounding whitespace inside the braces is trimmed
*     {{  and  }}  -- escaped literal `{` and `}`
*   Parsing is lenient and never throws: an unmatched `{` (or a lone `}`) is
* kept literally; the first `}` closes a placeholder (no nesting).  Segments are
* stored as offsets into the owned format string, so copying or moving a parser
* is cheap and never dangles; `text` resolves a segment back to its view.
*
*   Requires C++17 (std::string_view); self-suppresses below it.
*
* path:      /inc/djinterp/text/parser.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.14
******************************************************************************/

#ifndef DJINTERP_TEXT_PARSER_
#define DJINTERP_TEXT_PARSER_ 1

// std
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
// djinterp
#include "../djinterp.hpp"      // NS_*, D_NODISCARD, language gates


// std::string_view is the spine of the segment views; below C++17 this module
// contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


// ===========================================================================
// I.   parser
// ===========================================================================

// parser
//   class: the placeholder parser behind text_template -- it scans a format
// string with `{key}` placeholders ONCE into a flat segment list and owns both
// the format and the segments.  `_Type` is the character type (e.g. char,
// wchar_t).  Segments are offsets into the owned format, so a parser is
// zero-copy to query and cheap to copy or move without dangling.
template<typename _Type = char>
class parser
{
public:
    using char_type   = _Type;
    using string_type = std::basic_string<_Type>;
    using view_type   = std::basic_string_view<_Type>;
    using size_type   = std::size_t;

    // input_type
    //   type: parser contract -- the element type of the scanned input stream.
    using input_type  = char_type;

    // segment
    //   struct: one parsed piece of the format -- either a literal span or a
    // placeholder key, given as an offset/length into the owned format string.
    struct segment
    {
        bool      m_is_key;
        size_type m_offset;
        size_type m_length;
    };

    // segment_list
    //   type: the flat, in-order sequence of parsed segments.
    using segment_list = std::vector<segment>;

    // result_type
    //   type: parser contract -- the structure produced by a successful parse.
    using result_type  = segment_list;

    // empty parser (no format, no segments)
    parser() = default;

    // parse an owned format string
    explicit parser(
        string_type _format
    )
        : m_format(static_cast<string_type&&>(_format))
    {
        m_parse();
    }

    // parse a view (copied into the owned format)
    explicit parser(
        view_type _format
    )
        : m_format(_format)
    {
        m_parse();
    }

    // parse a C string
    explicit parser(
        const _Type* _format
    )
        : m_format(_format)
    {
        m_parse();
    }

    // format -- the original format string the segments index into
    D_NODISCARD const string_type&
    format() const
    {
        return m_format;
    }

    // segments -- the parsed pieces, in order of appearance
    D_NODISCARD const segment_list&
    segments() const
    {
        return m_segments;
    }

    // text -- the format span a segment refers to (literal text, or the trimmed
    // key name).  Encapsulates the offset arithmetic so callers never index the
    // owned format by hand.
    D_NODISCARD view_type
    text(
        const segment& _seg
    ) const
    {
        return view_type(m_format.data() + _seg.m_offset, _seg.m_length);
    }

    // keys -- the placeholder names, in order of appearance (for validation)
    D_NODISCARD std::vector<view_type>
    keys() const
    {
        std::vector<view_type> _result;
        for (const segment& _seg : m_segments)
        {
            if (_seg.m_is_key)
            {
                _result.push_back(text(_seg));
            }
        }

        return _result;
    }

    // literal_size -- total bytes contributed by literal segments (used to size
    // a rendered output buffer up front)
    D_NODISCARD size_type
    literal_size() const
    {
        return m_literal_size;
    }

    D_NODISCARD size_type
    key_count() const
    {
        return m_key_count;
    }

    D_NODISCARD size_type
    segment_count() const
    {
        return m_segments.size();
    }

    D_NODISCARD bool
    empty() const
    {
        return m_format.empty();
    }

private:
    // m_parse
    //   function: scan the format once, filling the segment list and the
    // literal-size / key-count totals.
    void
    m_parse()
    {
        m_segments.clear();
        m_literal_size = 0;
        m_key_count    = 0;

        const size_type n         = m_format.size();
        const _Type*    s         = m_format.data();
        size_type       lit_start = 0;
        size_type       i         = 0;

        while (i < n)
        {
            const _Type c = s[i];

            // opening or escaped brace
            if (c == _Type('{'))
            {
                // escaped "{{" -> literal '{'
                if (((i + 1) < n) && (s[i + 1] == _Type('{')))
                {
                    if (i > lit_start)
                    {
                        m_push_literal(lit_start, i - lit_start);
                    }
                    m_push_literal(i, 1);
                    i        += 2;
                    lit_start = i;
                    continue;
                }

                // locate the closing brace (first '}' closes; no nesting)
                size_type close = i + 1;
                while ((close < n) && (s[close] != _Type('}')))
                {
                    ++close;
                }

                // unmatched '{' -- leniently keep it in the literal run
                if (close >= n)
                {
                    ++i;
                    continue;
                }

                // commit preceding literal, then the trimmed key
                if (i > lit_start)
                {
                    m_push_literal(lit_start, i - lit_start);
                }

                size_type key_begin = i + 1;
                size_type key_end   = close;
                m_trim(key_begin, key_end);
                m_push_key(key_begin, key_end - key_begin);

                i         = close + 1;
                lit_start = i;
                continue;
            }

            // escaped "}}" -> literal '}'
            if ((c == _Type('}'))   &&
                ((i + 1) < n)       &&
                (s[i + 1] == _Type('}')))
            {
                if (i > lit_start)
                {
                    m_push_literal(lit_start, i - lit_start);
                }
                m_push_literal(i, 1);
                i        += 2;
                lit_start = i;
                continue;
            }

            // ordinary character (a lone '}' falls through, staying literal)
            ++i;
        }

        // trailing literal
        if (n > lit_start)
        {
            m_push_literal(lit_start, n - lit_start);
        }

        return;
    }

    // m_push_literal
    //   function: append a literal segment and accumulate its size.
    void
    m_push_literal(
        size_type _offset,
        size_type _length
    )
    {
        m_segments.push_back(segment{false, _offset, _length});
        m_literal_size += _length;

        return;
    }

    // m_push_key
    //   function: append a key segment and bump the key count.
    void
    m_push_key(
        size_type _offset,
        size_type _length
    )
    {
        m_segments.push_back(segment{true, _offset, _length});
        ++m_key_count;

        return;
    }

    // m_trim
    //   function: shrink [_begin, _end) past leading and trailing whitespace.
    void
    m_trim(
        size_type& _begin,
        size_type& _end
    ) const
    {
        const _Type* s = m_format.data();

        while ((_begin < _end) && m_is_space(s[_begin]))
        {
            ++_begin;
        }

        while ((_end > _begin) && m_is_space(s[_end - 1]))
        {
            --_end;
        }

        return;
    }

    // m_is_space
    //   function: ASCII-whitespace test for key trimming.
    static bool
    m_is_space(
        _Type _c
    )
    {
        return (_c == _Type(' '))  || (_c == _Type('\t')) ||
               (_c == _Type('\n')) || (_c == _Type('\r'));
    }

    string_type  m_format;
    segment_list m_segments;
    size_type    m_literal_size = 0;
    size_type    m_key_count    = 0;
};


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEXT_PARSER_
