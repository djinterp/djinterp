/******************************************************************************
* djinterp [text]                                          text_template.hpp
*
*   Full-featured marker-aware text template engine.  Given a format
* string and a set of named bindings, performs find-and-replace
* substitution at render time.  Designed to model any text template
* imaginable when composed with enough text functions.
*
*   Supersedes the inline text_template in text_function.hpp, which
* remains available as a lightweight alternative.
*
*   BINDING TYPES:
*   All bindings ultimately reduce to a resolver — a nullary callable
* returning std::string.  The different bind_* methods are typed
* convenience APIs that construct the appropriate resolver.
*
*     bind(key, string)               — literal string
*     bind(key, callable)             — function resolver (SFINAE)
*     bind_template(key, tmpl, fmt)   — nested template (recursive)
*     bind_list(key, item_tmpl, ...)  — iterated sub-template
*     bind_conditional(key, pred, T, F)— predicate-switched value
*     bind_section(key, pred)         — conditional block inclusion
*     bind_transform(key, src, fn)    — post-processed value
*
*   SECTION BLOCKS:
*   Sections are delimited by open and close markers within the
* format string, enabling conditional inclusion and iteration
* over blocks of template text:
*
*     %#key% ... %/key%    conditional section (included if truthy)
*     %^key% ... %/key%    inverted section   (included if falsy)
*
*   When bound as a section, the predicate controls inclusion.
*   When bound as a list, the body is iterated with per-item
*   bindings.  Section markers are configurable (default: #, /, ^).
*
*   ESCAPE:
*   A configurable escape character (default: backslash) preceding
* a prefix marker suppresses token expansion, emitting the literal
* marker instead.
*
*   NESTING:
*   Recursive template expansion (via nested template or section
* bindings) is depth-limited to prevent infinite recursion.
* Default: 16.
*
*   TEXT FUNCTION PROTOCOL:
*   text_template satisfies the text_function protocol via
* operator()(string) → string, making it composable in
* text_function chains, pipelines, and fn_builder objects.
*
*   PORTABILITY:
*   C++11 minimum.  C++17 enables if constexpr dispatch.
*
*
* TABLE OF CONTENTS
* =================
* I.    DEFAULTS
* II.   TEXT TEMPLATE
*       a. marker configuration
*       b. escape configuration
*       c. depth configuration
*       d. bind: string
*       e. bind: function
*       f. bind: nested template
*       g. bind: list
*       h. bind: conditional
*       i. bind: section
*       j. bind: transform
*       k. unbind / clear / query
*       l. render
*       m. render_to
*       n. render_with
*       o. operator() (text_function protocol)
*       p. binding_keys
* III.  CONVENIENCE FACTORIES
*
*
* path:      /inc/text/text_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEXT_TEMPLATE_
#define DJINTERP_TEXT_TEMPLATE_ 1

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "./text_function_traits.hpp"
#include "./text_template_traits.hpp"


NS_DJINTERP
NS_TEXT


///////////////////////////////////////////////////////////////////////////////
///                I.   DEFAULTS                                            ///
///////////////////////////////////////////////////////////////////////////////

#ifndef D_TEXT_TPL_DEFAULT_PREFIX
    #define D_TEXT_TPL_DEFAULT_PREFIX    "%"
#endif

#ifndef D_TEXT_TPL_DEFAULT_SUFFIX
    #define D_TEXT_TPL_DEFAULT_SUFFIX    "%"
#endif

#ifndef D_TEXT_TPL_DEFAULT_MAX_DEPTH
    #define D_TEXT_TPL_DEFAULT_MAX_DEPTH 16
#endif

#ifndef D_TEXT_TPL_DEFAULT_ESCAPE
    #define D_TEXT_TPL_DEFAULT_ESCAPE    '\\'
#endif

#ifndef D_TEXT_TPL_DEFAULT_SECTION_OPEN
    #define D_TEXT_TPL_DEFAULT_SECTION_OPEN   "#"
#endif

#ifndef D_TEXT_TPL_DEFAULT_SECTION_CLOSE
    #define D_TEXT_TPL_DEFAULT_SECTION_CLOSE  "/"
#endif

#ifndef D_TEXT_TPL_DEFAULT_SECTION_INVERT
    #define D_TEXT_TPL_DEFAULT_SECTION_INVERT "^"
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.  TEXT TEMPLATE                                        ///
///////////////////////////////////////////////////////////////////////////////

// text_template
//   class: full-featured marker-aware text template engine.
// Binds named specifiers to resolvers, then performs
// substitution on a format string at render time.  Supports
// section blocks, list iteration, conditionals, transforms,
// nested templates, and escape sequences.
//
//   Satisfies the text_function protocol: operator()(string)
// returns the rendered string.
//
// Usage:
//   text_template t;
//   t.bind("name", "djinterp");
//   t.bind("year", []() { return "2026"; });
//   auto out = t.render("Welcome to %name% (%year%)");
//   // out == "Welcome to djinterp (2026)"
//
//   // section block:
//   t.bind_section("show_footer", []() { return true; });
//   auto out2 = t.render("body%#show_footer%\nfooter%/show_footer%");
//   // out2 == "body\nfooter"
class text_template
{
public:
    using size_type     = std::size_t;
    using resolver_type = std::function<std::string()>;
    using predicate_type = std::function<bool()>;

private:
    // binding_entry
    //   struct: internal binding storage.  Every binding
    // reduces to a key and a resolver function.
    struct binding_entry
    {
        std::string   key;
        resolver_type resolver;
    };

    // section_entry
    //   struct: internal section binding storage.  A section
    // maps a key to a predicate that controls block inclusion.
    // When is_list is true, the section iterates instead.
    struct section_entry
    {
        std::string    key;
        predicate_type predicate;
        bool           is_list;
        resolver_type  list_resolver;
    };

public:
    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    text_template()
        : m_prefix(D_TEXT_TPL_DEFAULT_PREFIX),
          m_suffix(D_TEXT_TPL_DEFAULT_SUFFIX),
          m_section_open(D_TEXT_TPL_DEFAULT_SECTION_OPEN),
          m_section_close(D_TEXT_TPL_DEFAULT_SECTION_CLOSE),
          m_section_invert(D_TEXT_TPL_DEFAULT_SECTION_INVERT),
          m_escape(D_TEXT_TPL_DEFAULT_ESCAPE),
          m_max_depth(D_TEXT_TPL_DEFAULT_MAX_DEPTH),
          m_bindings(),
          m_sections()
    {}

    // from markers
    text_template(
            const std::string& _prefix,
            const std::string& _suffix
        )
            : m_prefix(_prefix),
              m_suffix(_suffix),
              m_section_open(D_TEXT_TPL_DEFAULT_SECTION_OPEN),
              m_section_close(D_TEXT_TPL_DEFAULT_SECTION_CLOSE),
              m_section_invert(D_TEXT_TPL_DEFAULT_SECTION_INVERT),
              m_escape(D_TEXT_TPL_DEFAULT_ESCAPE),
              m_max_depth(D_TEXT_TPL_DEFAULT_MAX_DEPTH),
              m_bindings(),
              m_sections()
    {}


    // =================================================================
    //  a. marker configuration
    // =================================================================

    const std::string&
    prefix() const noexcept
    {
        return m_prefix;
    }

    const std::string&
    suffix() const noexcept
    {
        return m_suffix;
    }

    void
    set_markers(
        const std::string& _prefix,
        const std::string& _suffix
    )
    {
        m_prefix = _prefix;
        m_suffix = _suffix;

        return;
    }

    // section marker accessors
    const std::string&
    section_open_marker() const noexcept
    {
        return m_section_open;
    }

    const std::string&
    section_close_marker() const noexcept
    {
        return m_section_close;
    }

    const std::string&
    section_invert_marker() const noexcept
    {
        return m_section_invert;
    }

    void
    set_section_markers(
        const std::string& _open,
        const std::string& _close,
        const std::string& _invert
    )
    {
        m_section_open   = _open;
        m_section_close  = _close;
        m_section_invert = _invert;

        return;
    }


    // =================================================================
    //  b. escape configuration
    // =================================================================

    char
    escape_char() const noexcept
    {
        return m_escape;
    }

    void
    set_escape_char(
        char _ch
    ) noexcept
    {
        m_escape = _ch;

        return;
    }


    // =================================================================
    //  c. depth configuration
    // =================================================================

    size_type
    max_depth() const noexcept
    {
        return m_max_depth;
    }

    void
    set_max_depth(
        size_type _depth
    ) noexcept
    {
        m_max_depth = _depth;

        return;
    }


    // =================================================================
    //  d. bind: string
    // =================================================================

    // bind
    //   binds a key to a literal string value.
    void
    bind(
        const std::string& _key,
        const std::string& _value
    )
    {
        std::string val = _value;

        set_binding(_key,
            [val]() -> std::string
            {
                return val;
            });

        return;
    }

    // bind (const char* overload)
    void
    bind(
        const std::string& _key,
        const char*        _value
    )
    {
        bind(_key, std::string(_value ? _value : ""));

        return;
    }


    // =================================================================
    //  e. bind: function
    // =================================================================

    // bind_function
    //   binds a key to a nullary resolver.
    void
    bind_function(
        const std::string& _key,
        resolver_type      _resolver
    )
    {
        set_binding(_key,
            static_cast<resolver_type&&>(_resolver));

        return;
    }

    // bind (generic callable, SFINAE-disambiguated from string)
    template<typename _Fn,
             typename = typename std::enable_if<
                 traits::is_nullary_text_function<
                     typename std::decay<_Fn>::type>::value &&
                 !std::is_convertible<_Fn, std::string>::value &&
                 !std::is_convertible<_Fn, const char*>::value
             >::type>
    void
    bind(
        const std::string& _key,
        _Fn&&              _resolver
    )
    {
        bind_function(
            _key,
            resolver_type(static_cast<_Fn&&>(_resolver)));

        return;
    }


    // =================================================================
    //  f. bind: nested template
    // =================================================================

    // bind_template
    //   binds a key to a nested text_template rendered with
    // _format at expansion time.  The nested template is not
    // owned; the caller must ensure it outlives this template.
    void
    bind_template(
        const std::string&   _key,
        const text_template& _nested,
        const std::string&   _format
    )
    {
        const text_template* nested = &_nested;
        std::string fmt = _format;

        set_binding(_key,
            [nested, fmt]() -> std::string
            {
                return nested->render(fmt);
            });

        return;
    }


    // =================================================================
    //  g. bind: list
    // =================================================================

    // bind_list
    //   binds a key to a list iteration.  The sub-template is
    // rendered _count times.  Before each iteration, the
    // template is cleared, auto-keys are injected, and
    // _bind_fn is invoked to populate per-item bindings.
    // Results are joined by _separator.
    //
    // _bind_fn signature:
    //   void(text_template& item, size_t index, size_t count)
    //
    // Auto-keys injected per item:
    //   _index    0-based index
    //   _number   1-based index
    //   _count    total item count
    //   _is_first "true" or "false"
    //   _is_last  "true" or "false"
    template<typename _BindFn>
    void
    bind_list(
        const std::string& _key,
        text_template&     _item_template,
        size_type          _count,
        _BindFn&&          _bind_fn,
        const std::string& _item_format,
        const std::string& _separator   = "",
        const std::string& _empty_text  = ""
    )
    {
        auto bind_fn    = static_cast<_BindFn&&>(_bind_fn);
        auto* item_tmpl = &_item_template;
        auto  count     = _count;
        auto  item_fmt  = _item_format;
        auto  sep       = _separator;
        auto  empty     = _empty_text;

        set_binding(_key,
            [bind_fn, item_tmpl, count,
             item_fmt, sep, empty]() -> std::string
            {
                if (count == 0)
                {
                    return empty;
                }

                std::string result;

                for (size_type i = 0; i < count; ++i)
                {
                    item_tmpl->clear_bindings();

                    // auto-keys
                    item_tmpl->bind("_index",
                        std::to_string(i));
                    item_tmpl->bind("_number",
                        std::to_string(i + 1));
                    item_tmpl->bind("_count",
                        std::to_string(count));
                    item_tmpl->bind("_is_first",
                        (i == 0) ? "true" : "false");
                    item_tmpl->bind("_is_last",
                        (i == count - 1) ? "true" : "false");

                    bind_fn(*item_tmpl, i, count);

                    result += item_tmpl->render(item_fmt);

                    if ( (!sep.empty()) &&
                         (i + 1 < count) )
                    {
                        result += sep;
                    }
                }

                return result;
            });

        return;
    }


    // =================================================================
    //  h. bind: conditional
    // =================================================================

    // bind_conditional
    //   binds a key to a predicate-switched value.  At render
    // time, if _predicate() returns true, _true_value is used;
    // otherwise _false_value.
    void
    bind_conditional(
        const std::string& _key,
        predicate_type     _predicate,
        const std::string& _true_value,
        const std::string& _false_value
    )
    {
        auto pred      = static_cast<predicate_type&&>(_predicate);
        auto true_val  = _true_value;
        auto false_val = _false_value;

        set_binding(_key,
            [pred, true_val, false_val]() -> std::string
            {
                return pred() ? true_val : false_val;
            });

        return;
    }

    // bind_conditional (function values)
    //   overload where both branches are resolver functions.
    void
    bind_conditional(
        const std::string& _key,
        predicate_type     _predicate,
        resolver_type      _true_resolver,
        resolver_type      _false_resolver
    )
    {
        auto pred     = static_cast<predicate_type&&>(_predicate);
        auto true_fn  = static_cast<resolver_type&&>(_true_resolver);
        auto false_fn = static_cast<resolver_type&&>(_false_resolver);

        set_binding(_key,
            [pred, true_fn, false_fn]() -> std::string
            {
                return pred() ? true_fn() : false_fn();
            });

        return;
    }


    // =================================================================
    //  i. bind: section
    // =================================================================

    // bind_section
    //   binds a section key to a predicate.  During rendering,
    // blocks delimited by %#key%...%/key% are included only
    // when the predicate returns true.  Blocks delimited by
    // %^key%...%/key% are included only when false (inverted).
    void
    bind_section(
        const std::string& _key,
        predicate_type     _predicate
    )
    {
        // remove existing section with same key
        for (auto it = m_sections.begin();
             it != m_sections.end();
             ++it)
        {
            if (it->key == _key)
            {
                m_sections.erase(it);
                break;
            }
        }

        section_entry entry;
        entry.key           = _key;
        entry.predicate     = static_cast<predicate_type&&>(
                                  _predicate);
        entry.is_list       = false;
        entry.list_resolver = nullptr;

        m_sections.push_back(
            static_cast<section_entry&&>(entry));

        return;
    }

    // bind_section (bool shorthand)
    //   binds a section to a fixed boolean value.
    void
    bind_section(
        const std::string& _key,
        bool               _value
    )
    {
        bool val = _value;

        bind_section(_key,
            [val]() -> bool { return val; });

        return;
    }


    // =================================================================
    //  j. bind: transform
    // =================================================================

    // bind_transform
    //   binds a key to a source string that is post-processed
    // by a transform function at render time.
    void
    bind_transform(
        const std::string& _key,
        const std::string& _source,
        std::function<std::string(const std::string&)> _transform
    )
    {
        auto src       = _source;
        auto transform = static_cast<
            std::function<std::string(const std::string&)>&&>(
                _transform);

        set_binding(_key,
            [src, transform]() -> std::string
            {
                return transform(src);
            });

        return;
    }

    // bind_transform (resolver source)
    //   overload where the source is itself a resolver.
    void
    bind_transform(
        const std::string& _key,
        resolver_type      _source,
        std::function<std::string(const std::string&)> _transform
    )
    {
        auto src_fn    = static_cast<resolver_type&&>(_source);
        auto transform = static_cast<
            std::function<std::string(const std::string&)>&&>(
                _transform);

        set_binding(_key,
            [src_fn, transform]() -> std::string
            {
                return transform(src_fn());
            });

        return;
    }


    // =================================================================
    //  k. unbind / clear / query
    // =================================================================

    // unbind
    void
    unbind(
        const std::string& _key
    )
    {
        for (auto it = m_bindings.begin();
             it != m_bindings.end();
             ++it)
        {
            if (it->key == _key)
            {
                m_bindings.erase(it);
                break;
            }
        }

        for (auto it = m_sections.begin();
             it != m_sections.end();
             ++it)
        {
            if (it->key == _key)
            {
                m_sections.erase(it);
                break;
            }
        }

        return;
    }

    // clear_bindings
    void
    clear_bindings()
    {
        m_bindings.clear();
        m_sections.clear();

        return;
    }

    // has_binding
    bool
    has_binding(
        const std::string& _key
    ) const
    {
        for (const auto& b : m_bindings)
        {
            if (b.key == _key)
            {
                return true;
            }
        }

        for (const auto& s : m_sections)
        {
            if (s.key == _key)
            {
                return true;
            }
        }

        return false;
    }

    // binding_count
    size_type
    binding_count() const noexcept
    {
        return m_bindings.size() + m_sections.size();
    }

    // binding_keys
    //   returns a vector of all bound key strings.
    std::vector<std::string>
    binding_keys() const
    {
        std::vector<std::string> keys;
        keys.reserve(m_bindings.size() + m_sections.size());

        for (const auto& b : m_bindings)
        {
            keys.push_back(b.key);
        }

        for (const auto& s : m_sections)
        {
            keys.push_back(s.key);
        }

        return keys;
    }


    // =================================================================
    //  l. render
    // =================================================================

    // render
    //   performs full template expansion: section blocks first,
    // then token substitution, with escape handling and depth
    // tracking.
    std::string
    render(
        const std::string& _format
    ) const
    {
        return render_impl(_format, m_max_depth);
    }


    // =================================================================
    //  m. render_to
    // =================================================================

    // render_to
    //   renders into a mutable std::string target.
    void
    render_to(
        std::string&       _target,
        const std::string& _format
    ) const
    {
        _target += render(_format);

        return;
    }


    // =================================================================
    //  n. render_with
    // =================================================================

    // render_with
    //   renders with one-shot additional string bindings that
    // do not persist after the call.
    std::string
    render_with(
        const std::string& _format,
        const std::vector<std::pair<std::string, std::string>>& _extra
    ) const
    {
        // temporarily add extra bindings by creating a copy
        text_template tmp = *this;

        for (const auto& pair : _extra)
        {
            tmp.bind(pair.first, pair.second);
        }

        return tmp.render(_format);
    }


    // =================================================================
    //  o. operator() (text_function protocol)
    // =================================================================

    // operator()
    //   makes text_template composable in text_function chains.
    std::string
    operator()(
        const std::string& _format
    ) const
    {
        return render(_format);
    }


private:
    // =================================================================
    //  internal: binding management
    // =================================================================

    // set_binding
    //   sets or updates a binding entry.
    void
    set_binding(
        const std::string& _key,
        resolver_type      _resolver
    )
    {
        for (auto& b : m_bindings)
        {
            if (b.key == _key)
            {
                b.resolver = static_cast<resolver_type&&>(
                    _resolver);

                return;
            }
        }

        binding_entry entry;
        entry.key      = _key;
        entry.resolver = static_cast<resolver_type&&>(
            _resolver);

        m_bindings.push_back(
            static_cast<binding_entry&&>(entry));

        return;
    }


    // =================================================================
    //  internal: render implementation
    // =================================================================

    // render_impl
    //   two-phase renderer:
    //     phase 1 — expand section blocks (#, ^, /)
    //     phase 2 — substitute tokens (prefix+key+suffix)
    std::string
    render_impl(
        const std::string& _format,
        size_type          _remaining_depth
    ) const
    {
        if (_remaining_depth == 0)
        {
            return _format;
        }

        // phase 1: expand section blocks
        std::string after_sections = expand_sections(
            _format, _remaining_depth - 1);

        // phase 2: substitute tokens
        return substitute_tokens(
            after_sections, _remaining_depth - 1);
    }


    // =================================================================
    //  internal: section expansion
    // =================================================================

    // expand_sections
    //   scans for section open/close/invert markers and
    // conditionally includes or omits the body.
    std::string
    expand_sections(
        const std::string& _format,
        size_type          _remaining_depth
    ) const
    {
        if (m_sections.empty())
        {
            return _format;
        }

        std::string result = _format;

        for (const auto& sec : m_sections)
        {
            // build open/close/invert tokens
            // e.g. %#key%, %/key%, %^key%
            std::string open_token =
                m_prefix + m_section_open + sec.key + m_suffix;
            std::string close_token =
                m_prefix + m_section_close + sec.key + m_suffix;
            std::string invert_token =
                m_prefix + m_section_invert + sec.key + m_suffix;

            bool pred_result = sec.predicate
                               ? sec.predicate()
                               : false;

            // handle normal sections: %#key%...%/key%
            result = expand_section_pair(
                result,
                open_token,
                close_token,
                pred_result,
                _remaining_depth);

            // handle inverted sections: %^key%...%/key%
            result = expand_section_pair(
                result,
                invert_token,
                close_token,
                !pred_result,
                _remaining_depth);
        }

        return result;
    }

    // expand_section_pair
    //   finds matching open/close pairs and conditionally
    // includes the body.  Handles multiple occurrences of
    // the same section in the format string.
    std::string
    expand_section_pair(
        const std::string& _input,
        const std::string& _open,
        const std::string& _close,
        bool               _include,
        size_type          _remaining_depth
    ) const
    {
        std::string result;
        size_type   pos = 0;

        while (pos < _input.size())
        {
            size_type open_pos = _input.find(_open, pos);

            if (open_pos == std::string::npos)
            {
                // no more sections — append remainder
                result.append(_input, pos, _input.size() - pos);
                break;
            }

            // append text before the section
            result.append(_input, pos, open_pos - pos);

            // find matching close
            size_type body_start = open_pos + _open.size();
            size_type close_pos  = _input.find(
                _close, body_start);

            if (close_pos == std::string::npos)
            {
                // unmatched open — preserve and stop
                result.append(_input, open_pos,
                              _input.size() - open_pos);
                break;
            }

            if (_include)
            {
                // extract body and recursively render
                std::string body = _input.substr(
                    body_start,
                    close_pos - body_start);

                if (_remaining_depth > 0)
                {
                    result += render_impl(
                        body, _remaining_depth);
                }
                else
                {
                    result += body;
                }
            }

            pos = close_pos + _close.size();
        }

        return result;
    }


    // =================================================================
    //  internal: token substitution
    // =================================================================

    // substitute_tokens
    //   scans for prefix+key+suffix tokens and replaces with
    // resolved values.  Handles the escape character.
    std::string
    substitute_tokens(
        const std::string& _input,
        size_type          _remaining_depth
    ) const
    {
        std::string out;
        out.reserve(_input.size());

        size_type pfx_len = m_prefix.size();
        size_type sfx_len = m_suffix.size();
        size_type i       = 0;

        while (i < _input.size())
        {
            // check escape character
            if ( (m_escape != '\0')                   &&
                 (_input[i] == static_cast<char>(m_escape)) &&
                 (i + 1 + pfx_len <= _input.size())   &&
                 (_input.compare(i + 1, pfx_len,
                                 m_prefix) == 0) )
            {
                // emit the prefix literally, skip escape char
                out += m_prefix;
                i += 1 + pfx_len;

                continue;
            }

            // check for prefix
            if ( (i + pfx_len <= _input.size()) &&
                 (_input.compare(i, pfx_len,
                                 m_prefix) == 0) )
            {
                size_type key_start = i + pfx_len;
                size_type sfx_pos   = _input.find(
                    m_suffix, key_start);

                if (sfx_pos != std::string::npos)
                {
                    std::string key = _input.substr(
                        key_start,
                        sfx_pos - key_start);

                    // look up binding
                    const resolver_type* resolver =
                        find_binding(key);

                    if (resolver)
                    {
                        std::string value = (*resolver)();

                        // recursively expand if depth remains
                        if (_remaining_depth > 0)
                        {
                            value = render_impl(
                                value,
                                _remaining_depth);
                        }

                        out += value;
                    }
                    else
                    {
                        // unbound key — preserve token
                        out += m_prefix;
                        out += key;
                        out += m_suffix;
                    }

                    i = sfx_pos + sfx_len;

                    continue;
                }
            }

            out += _input[i];
            ++i;
        }

        return out;
    }


    // =================================================================
    //  internal: lookup
    // =================================================================

    // find_binding
    //   returns a pointer to the resolver for _key, or
    // nullptr if not found.
    const resolver_type*
    find_binding(
        const std::string& _key
    ) const
    {
        for (const auto& b : m_bindings)
        {
            if (b.key == _key)
            {
                return &b.resolver;
            }
        }

        return nullptr;
    }

    // find_section
    //   returns a pointer to the section entry for _key, or
    // nullptr if not found.
    const section_entry*
    find_section(
        const std::string& _key
    ) const
    {
        for (const auto& s : m_sections)
        {
            if (s.key == _key)
            {
                return &s;
            }
        }

        return nullptr;
    }


    // =================================================================
    //  storage
    // =================================================================

    std::string                m_prefix;
    std::string                m_suffix;
    std::string                m_section_open;
    std::string                m_section_close;
    std::string                m_section_invert;
    char                       m_escape;
    size_type                  m_max_depth;
    std::vector<binding_entry> m_bindings;
    std::vector<section_entry> m_sections;
};


///////////////////////////////////////////////////////////////////////////////
///                III. CONVENIENCE FACTORIES                                ///
///////////////////////////////////////////////////////////////////////////////

// make_template
//   function: creates a text_template with given markers.
inline text_template
make_template(
    const std::string& _prefix = D_TEXT_TPL_DEFAULT_PREFIX,
    const std::string& _suffix = D_TEXT_TPL_DEFAULT_SUFFIX
)
{
    return text_template(_prefix, _suffix);
}

// make_curly_template
//   function: creates a text_template with "{" / "}" markers,
// matching the convention used by test_printer.
inline text_template
make_curly_template()
{
    return text_template("{", "}");
}

// make_dollar_template
//   function: creates a text_template with "${" / "}" markers,
// matching shell-style variable expansion.
inline text_template
make_dollar_template()
{
    return text_template("${", "}");
}

// make_mustache_template
//   function: creates a text_template with "{{" / "}}" markers,
// matching Mustache/Handlebars convention.
inline text_template
make_mustache_template()
{
    return text_template("{{", "}}");
}


NS_END  // text
NS_END  // djinterp


#endif  // DJINTERP_TEXT_TEMPLATE_
