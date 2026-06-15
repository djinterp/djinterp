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
* path:      /inc/djinterp/text/text_template.hpp
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
#include "../djinterp.hpp"          // NS_*, D_NODISCARD, language gates
#include "../../parse/parser.hpp"   // parser<_Type> -- the placeholder parser


// std::string_view is the spine of the source contract; below C++17 this
// module contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


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
    using parser_type = parser<_Type>;

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


// ===========================================================================
// II.  interpolate
// ===========================================================================

// interpolate
//   class: the interpolation transformation as a functor -- it binds a
// text_template (the *template* t) and maps a *source* (a lookup) to the
// produced string (the *sink*).  This is F_t = F-hat(t): construct it once with
// a template, then call it with as many sources as desired.  `_Type` is the
// character type.
template<typename _Type = char>
class interpolate
{
public:
    using char_type     = _Type;
    using string_type   = std::basic_string<_Type>;
    using view_type     = std::basic_string_view<_Type>;
    using template_type = text_template<_Type>;

    // an interpolate over the empty template
    interpolate() = default;

    // bind an existing template
    explicit interpolate(
        template_type _template
    )
        : m_template(static_cast<template_type&&>(_template))
    {}

    // bind a template built from a format string
    explicit interpolate(
        string_type _format
    )
        : m_template(static_cast<string_type&&>(_format))
    {}

    // bind a template built from a C string
    explicit interpolate(
        const _Type* _format
    )
        : m_template(_format)
    {}

    // operator() -- apply the bound template to a source, returning the sink
    template<typename _Lookup>
    D_NODISCARD string_type
    operator()(
        _Lookup&& _source
    ) const
    {
        return m_template.render(static_cast<_Lookup&&>(_source));
    }

    D_NODISCARD string_type
    operator()(
        std::initializer_list<std::pair<view_type, view_type>> _source
    ) const
    {
        return m_template.render(_source);
    }

    // operator() -- append form: interpolate into an existing buffer
    template<typename _Lookup>
    void
    operator()(
        string_type& _out,
        _Lookup&&    _source
    ) const
    {
        m_template.render_to(_out, static_cast<_Lookup&&>(_source));

        return;
    }

    // bound -- the underlying template t
    D_NODISCARD const template_type&
    bound() const
    {
        return m_template;
    }

private:
    template_type m_template;
};


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEXT_TEMPLATE_
