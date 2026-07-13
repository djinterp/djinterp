/******************************************************************************
* djinterp [text]                                            text_template.hpp
*
*   Convenient, high-performance string interpolation.  `text_template` binds a
* `parser` (parser.hpp), which scans a format string with `{key}` placeholders
* ONCE into a flat segment list, and renders that structure against a
* caller-supplied source (a key -> value lookup), so the cost of parsing is
* paid a single time no matter how often the template is rendered.
* `interpolate` is the same operation as a functor: it binds a template and
* maps a source to the produced string.
*
*   In the vocabulary of template.hpp this is the template-source-sink schema
* specialized to text: the text_template is the *template* t, a lookup is the
* *source*, the produced string is the *sink*, and `interpolate(t)` is the
* source-transformer F_t = F-hat(t).  (The types here are heterogeneous --
* template, source and sink are distinct -- so this does not instantiate
* template_system, which is the homogeneous-carrier case; it realizes the same
* curry / evaluate factorization by hand.)
*
*   Placeholders:
*     {key}        -- substituted with the source's value for `key`
*     { key }      -- surrounding whitespace inside the braces is trimmed
*     {{  and  }}  -- escaped literal `{` and `}`
*   Parsing is lenient and never throws: an unmatched `{` (or a lone `}`) is
* emitted literally; the first `}` closes a placeholder (no nesting).  The bound
* parser stores segments as offsets into the owned format string, so copying or
* moving a text_template is cheap and never dangles.
*
*   Source contract: any callable `(view_type) -> V` where V is convertible to
* view_type (a string, a string_view, or a C string).  Build one inline with
* the initializer_list overloads, or pass a lambda closing over a map.  A
* temporary returned by the lookup is lifetime-correct -- it is appended before
* it is destroyed.
*
*   Requires C++17 (std::string_view); self-suppresses below it.
*
* path:      /inc/djinterp/core/text/text_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.14
******************************************************************************/

#ifndef DJINTERP_TEXT_TEMPLATE_
#define DJINTERP_TEXT_TEMPLATE_ 1

// std
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <initializer_list>
// djinterp
#include "../djinterp.hpp"                 // NS_*, D_NODISCARD, language gates


// std::string_view is the spine of the source contract; below C++17 this
// module contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


NS_INTERNAL

// ===========================================================================
// 0.   placeholder_parser  (the segment scanner text_template binds)
// ===========================================================================

// placeholder_parser
//   class: scans a format string with `{key}` placeholders ONCE into a flat
// list of literal/key segments, each stored as an (offset, length) into the
// owned format -- so copying or moving the parser is cheap and segments never
// dangle.  This is the small, self-contained scanner text_template was written
// against; it depends only on <string>/<string_view>/<vector> and is
// deliberately decoupled from the parse-combinator subframework (text_template
// needs a `{key}` scan, not a parser carrier).
//
//   Dialect (matches text_template's documented contract):
//     {key}        -- a key segment named `key`
//     { key }      -- surrounding whitespace inside the braces is trimmed
//     {{  }}       -- escaped literal `{` / `}` (emitted as a one-char literal)
//   Lenient and never throws: an unmatched `{` with no following `}` is emitted
// literally, a lone `}` is literal, and the first `}` closes a placeholder (no
// nesting).
template<typename _Type = char>
class placeholder_parser
{
public:
    using char_type   = _Type;
    using string_type = std::basic_string<_Type>;
    using view_type   = std::basic_string_view<_Type>;
    using size_type   = std::size_t;

    // segment
    //   struct: one scanned span -- a literal or a key -- as an offset and
    // length into the owned format string.  `m_is_key` discriminates the two.
    struct segment
    {
        size_type m_offset;
        size_type m_length;
        bool      m_is_key;

        segment()
            : m_offset(0),
              m_length(0),
              m_is_key(false)
        {}

        segment(
            size_type _offset,
            size_type _length,
            bool      _is_key
        )
            : m_offset(_offset),
              m_length(_length),
              m_is_key(_is_key)
        {}
    };

    placeholder_parser() = default;

    explicit placeholder_parser(
        string_type _format
    )
        : m_format(static_cast<string_type&&>(_format)),
          m_segments()
    {
        scan();
    }

    explicit placeholder_parser(
        view_type _format
    )
        : m_format(_format.data(), _format.size()),
          m_segments()
    {
        scan();
    }

    explicit placeholder_parser(
        const _Type* _format
    )
        : m_format(_format ? string_type(_format) : string_type()),
          m_segments()
    {
        scan();
    }

    // segments -- the scanned segment list (literal and key spans, in order).
    D_NODISCARD const std::vector<segment>&
    segments() const
    {
        return m_segments;
    }

    // text -- the view a segment resolves to within the owned format.  For a
    // key segment this is the (trimmed) key name; for a literal it is the
    // literal span (a single char for an escaped brace).
    D_NODISCARD view_type
    text(
        const segment& _seg
    ) const
    {
        return view_type(m_format.data() + _seg.m_offset, _seg.m_length);
    }

    // format -- the original (owned) format string.
    D_NODISCARD const string_type&
    format() const
    {
        return m_format;
    }

    // keys -- the placeholder names, in order of appearance.
    D_NODISCARD std::vector<view_type>
    keys() const
    {
        std::vector<view_type> _out;
        _out.reserve(key_count());

        for (const segment& _seg : m_segments)
        {
            if (_seg.m_is_key)
            {
                _out.push_back(text(_seg));
            }
        }

        return _out;
    }

    // literal_size -- total bytes contributed by literal segments (the lower
    // bound text_template reserves for the output buffer).
    D_NODISCARD size_type
    literal_size() const
    {
        size_type _total = 0;

        for (const segment& _seg : m_segments)
        {
            if (!_seg.m_is_key)
            {
                _total += _seg.m_length;
            }
        }

        return _total;
    }

    D_NODISCARD size_type
    key_count() const
    {
        size_type _count = 0;

        for (const segment& _seg : m_segments)
        {
            if (_seg.m_is_key)
            {
                ++_count;
            }
        }

        return _count;
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
    // is_space -- ASCII whitespace test for key trimming.
    static bool
    is_space(
        _Type _c
    )
    {
        return ( _c == static_cast<_Type>(' ')  ||
                 _c == static_cast<_Type>('\t') ||
                 _c == static_cast<_Type>('\n') ||
                 _c == static_cast<_Type>('\r') ||
                 _c == static_cast<_Type>('\f') ||
                 _c == static_cast<_Type>('\v') );
    }

    // push_literal -- record a literal segment for [_from, _to), if non-empty.
    void
    push_literal(
        size_type _from,
        size_type _to
    )
    {
        if (_to > _from)
        {
            m_segments.push_back(segment(_from, _to - _from, false));
        }

        return;
    }

    // scan -- the one-pass segmentation; see the dialect notes above.
    void
    scan()
    {
        const _Type     k_open  = static_cast<_Type>('{');
        const _Type     k_close = static_cast<_Type>('}');
        const size_type n       = m_format.size();
        const _Type*    p       = m_format.data();

        size_type i         = 0;
        size_type lit_start = 0;

        while (i < n)
        {
            const _Type c = p[i];

            if (c == k_open)
            {
                // escaped '{{' -> one literal '{'
                if (((i + 1) < n) && (p[i + 1] == k_open))
                {
                    push_literal(lit_start, i);
                    m_segments.push_back(segment(i, 1, false));
                    i        += 2;
                    lit_start = i;
                    continue;
                }

                // potential key: first '}' (if any) closes it
                size_type j = i + 1;

                while ((j < n) && (p[j] != k_close))
                {
                    ++j;
                }

                if (j < n)
                {
                    push_literal(lit_start, i);

                    size_type k_begin = i + 1;
                    size_type k_end   = j;

                    while ((k_begin < k_end) && is_space(p[k_begin]))
                    {
                        ++k_begin;
                    }

                    while ((k_end > k_begin) && is_space(p[k_end - 1]))
                    {
                        --k_end;
                    }

                    m_segments.push_back(
                        segment(k_begin, k_end - k_begin, true));

                    i         = j + 1;
                    lit_start = i;
                    continue;
                }

                // no closing brace: '{' is literal (stays in the run)
                ++i;
                continue;
            }

            if (c == k_close)
            {
                // escaped '}}' -> one literal '}'
                if (((i + 1) < n) && (p[i + 1] == k_close))
                {
                    push_literal(lit_start, i);
                    m_segments.push_back(segment(i, 1, false));
                    i        += 2;
                    lit_start = i;
                    continue;
                }

                // lone '}' is literal
                ++i;
                continue;
            }

            ++i;
        }

        push_literal(lit_start, n);

        return;
    }

    string_type          m_format;
    std::vector<segment> m_segments;
};

NS_END  // internal


// ===========================================================================
// I.   text_template
// ===========================================================================

// text_template
//   class: a format string with `{key}` placeholders, parsed once (by a bound
// `parser`) into a segment list and rendered many times.  `_Type` is the
// character type (e.g. char, wchar_t).  Convenient (render against inline
// bindings or any lookup) and high-performance (no per-render parsing;
// offset-based, zero-copy segments; a single output allocation, or none with
// render_to).
template<typename _Type = char>
class text_template
{
public:
    using char_type   = _Type;
    using string_type = std::basic_string<_Type>;
    using view_type   = std::basic_string_view<_Type>;
    using size_type   = std::size_t;
    using parser_type = internal::placeholder_parser<_Type>;

    // empty template
    text_template() = default;

    // build from an owned format string
    explicit text_template(
        string_type _format
    )
        : m_parser(static_cast<string_type&&>(_format))
    {}

    // build from a view (copied into the owned format)
    explicit text_template(
        view_type _format
    )
        : m_parser(_format)
    {}

    // build from a C string
    explicit text_template(
        const _Type* _format
    )
        : m_parser(_format)
    {}

    // render_to -- append the interpolation of `_lookup` into `_out` (no result
    // allocation).  `_lookup` is callable (view_type) -> (convertible to
    // view_type); the threaded output buffer is the caller's to size.
    template<typename _Lookup>
    void
    render_to(
        string_type& _out,
        _Lookup&&    _lookup
    ) const
    {
        for (const auto& _seg : m_parser.segments())
        {
            const view_type _piece = m_parser.text(_seg);

            // literal: append the source span verbatim
            if (!_seg.m_is_key)
            {
                _out.append(_piece.data(), _piece.size());
                continue;
            }

            // key: append the looked-up value (temporaries stay alive for the
            // append via the forwarding reference)
            auto&&          _value = _lookup(_piece);
            const view_type _view(_value);
            _out.append(_view.data(), _view.size());
        }

        return;
    }

    // render_to -- inline-bindings convenience over the callable form
    void
    render_to(
        string_type&                                          _out,
        std::initializer_list<std::pair<view_type, view_type>> _bindings
    ) const
    {
        render_to(_out, m_make_lookup(_bindings));

        return;
    }

    // render -- the interpolation of `_lookup` as a freshly allocated string;
    // reserves a capacity estimate up front.
    template<typename _Lookup>
    D_NODISCARD string_type
    render(
        _Lookup&& _lookup
    ) const
    {
        string_type _out;
        _out.reserve(m_parser.literal_size()
                     + (m_parser.key_count() * k_value_reserve));
        render_to(_out, static_cast<_Lookup&&>(_lookup));

        return _out;
    }

    // render -- inline-bindings convenience
    D_NODISCARD string_type
    render(
        std::initializer_list<std::pair<view_type, view_type>> _bindings
    ) const
    {
        return render(m_make_lookup(_bindings));
    }

    // operator() -- render as a value (the functor face of a text_template)
    template<typename _Lookup>
    D_NODISCARD string_type
    operator()(
        _Lookup&& _lookup
    ) const
    {
        return render(static_cast<_Lookup&&>(_lookup));
    }

    D_NODISCARD string_type
    operator()(
        std::initializer_list<std::pair<view_type, view_type>> _bindings
    ) const
    {
        return render(_bindings);
    }

    // format -- the original format string
    D_NODISCARD const string_type&
    format() const
    {
        return m_parser.format();
    }

    // parsed -- the bound parser (the format's parsed structure: segments,
    // key/literal totals, segment-to-view resolution)
    D_NODISCARD const parser_type&
    parsed() const
    {
        return m_parser;
    }

    // keys -- the placeholder names, in order of appearance (for validation)
    D_NODISCARD std::vector<view_type>
    keys() const
    {
        return m_parser.keys();
    }

    D_NODISCARD size_type
    key_count() const
    {
        return m_parser.key_count();
    }

    D_NODISCARD size_type
    segment_count() const
    {
        return m_parser.segment_count();
    }

    D_NODISCARD bool
    empty() const
    {
        return m_parser.empty();
    }

private:
    // k_value_reserve
    //   constant: per-placeholder byte estimate used to size the output buffer.
    static const size_type k_value_reserve = 16;

    // m_make_lookup
    //   function: a linear-scan lookup over inline bindings; missing keys map to
    // the empty view.  Captures the list by reference -- it must outlive the
    // render call (it does: both are arguments to the same statement).
    static auto
    m_make_lookup(
        const std::initializer_list<std::pair<view_type, view_type>>& _bindings
    )
    {
        return [&_bindings](view_type _key) -> view_type
        {
            for (const std::pair<view_type, view_type>& _entry : _bindings)
            {
                if (_entry.first == _key)
                {
                    return _entry.second;
                }
            }

            return view_type{};
        };
    }

    parser_type m_parser;
};


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEXT_TEMPLATE_