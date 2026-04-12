/******************************************************************************
* djinterp [text]                                           text_function.hpp
*
*   Core text transformation module.  Provides three interlocking types
* that compose into a flexible, chainable text processing pipeline:
*
*   text_function<_Input, _Output>:
*     A typed wrapper around any callable that transforms _Input into
*     string output.  Supports chaining via then() and operator|,
*     reusing the existing functional framework (compose, pipe) with
*     zero overhead.  A text_function is itself callable, so it
*     composes naturally with fn_builder, pipeline, and predicate
*     combinators.
*
*   text_specifier:
*     A single named binding that maps a key string to either a
*     literal string value or a resolver function (nullary callable
*     returning string).  The resolver form enables computed and
*     dynamic content — the function is invoked at render time.
*
*   text_template:
*     A marker-aware template engine.  Given a format string, a set
*     of specifier bindings, and configurable prefix/suffix markers
*     (defaulting to "%" / "%"), performs find-and-replace
*     substitution at render time.  Bindings may map to:
*       - plain strings
*       - resolver functions (string-returning callables)
*       - nested text_templates (recursive expansion)
*       - list bindings (a sub-template iterated over an index
*         range with a per-item callback)
*
*     This is the C++ analogue of the C text_template module,
*     replacing heap-allocated C strings and function pointers with
*     std::string, std::function, and SFINAE-gated generics.
*
*   CHAINING:
*   text_function objects chain left-to-right via then():
*     auto pipeline = fn_a.then(fn_b).then(fn_c);
*     std::string result = pipeline(input);
*   This composes as pipe(fn_a, pipe(fn_b, fn_c))(input), i.e.
*   fn_c(fn_b(fn_a(input))).
*
*   text_template is itself a text_function: its operator() accepts
*   a format string and returns the rendered result.  This means a
*   template can be placed anywhere in a text_function chain.
*
*   PORTABILITY:
*   C++11 minimum.  Uses D_CONSTEXPR, D_NODISCARD, and the
* functional module's composition utilities.  C++17 enables
* if constexpr dispatch paths.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEXT FUNCTION
* II.   TEXT SPECIFIER
* III.  TEXT TEMPLATE
* IV.   LIST BINDING
* V.    STRING INTERPOLATION
* VI.   CHAINING UTILITIES
* VII.  CONVENIENCE FACTORIES
*
*
* path:      /inc/text/text_function.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEXT_FUNCTION_
#define DJINTERP_TEXT_FUNCTION_ 1

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "../djinterp.hpp"
#include "./text_function_traits.hpp"


NS_DJINTERP
NS_TEXT


///////////////////////////////////////////////////////////////////////////////
///                I.   TEXT FUNCTION                                        ///
///////////////////////////////////////////////////////////////////////////////

// text_function
//   class: a typed wrapper around any callable that transforms
// _Input into std::string output.  Supports chaining via
// then() and operator|.
//
//   The wrapped callable is stored via std::function for type
// erasure.  For zero-overhead composition, use the free
// functions chain_text() or pipe_text() which return
// composed callables without std::function indirection.
//
// Template parameters:
//   _Input: the input type (default: const std::string&).
//
// Usage:
//   text_function<int> fn([](int x) {
//       return std::to_string(x * 2);
//   });
//   std::string result = fn(42);  // "84"
//
//   auto chain = fn.then(text_function<>([](const std::string& s) {
//       return "[" + s + "]";
//   }));
//   std::string result2 = chain(42);  // "[84]"
template<typename _Input = const std::string&>
class text_function
{
public:
    using input_type  = _Input;
    using output_type = std::string;
    using fn_type     = std::function<std::string(_Input)>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default: identity for string input, empty string otherwise
    text_function()
        : m_fn([](const _Input&) -> std::string
          {
              return std::string();
          })
    {}

    // from callable
    template<typename _Fn,
             typename = typename std::enable_if<
                 !std::is_same<
                     typename std::decay<_Fn>::type,
                     text_function>::value
             >::type>
    explicit text_function(
            _Fn&& _fn
        )
            : m_fn(static_cast<_Fn&&>(_fn))
    {}

    // from std::function
    explicit text_function(
            fn_type _fn
        )
            : m_fn(static_cast<fn_type&&>(_fn))
    {}

    // -----------------------------------------------------------------
    //  invocation
    // -----------------------------------------------------------------

    // operator()
    //   invokes the wrapped callable.
    std::string
    operator()(
        _Input _input
    ) const
    {
        return m_fn(_input);
    }

    // -----------------------------------------------------------------
    //  chaining
    // -----------------------------------------------------------------

    // then
    //   chains this text_function with another, producing a
    // new text_function that applies this function first, then
    // passes the result to _next.
    //
    // Usage:
    //   auto chain = fn_a.then(fn_b);
    //   // chain(x) == fn_b(fn_a(x))
    template<typename _NextFn>
    text_function<_Input>
    then(
        _NextFn&& _next
    ) const
    {
        auto prev = m_fn;
        auto next = static_cast<_NextFn&&>(_next);

        return text_function<_Input>(
            [prev, next](_Input _input) -> std::string
            {
                return next(prev(_input));
            });
    }

    // operator|
    //   pipe operator for chaining.  Equivalent to then().
    template<typename _NextFn>
    text_function<_Input>
    operator|(
        _NextFn&& _next
    ) const
    {
        return then(static_cast<_NextFn&&>(_next));
    }

    // -----------------------------------------------------------------
    //  access
    // -----------------------------------------------------------------

    // function
    //   returns a const reference to the underlying
    // std::function.
    const fn_type&
    function() const noexcept
    {
        return m_fn;
    }

    // valid
    //   returns true if the function is non-empty.
    bool
    valid() const noexcept
    {
        return static_cast<bool>(m_fn);
    }

private:
    fn_type m_fn;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEXT SPECIFIER                                       ///
///////////////////////////////////////////////////////////////////////////////

// text_specifier
//   class: a single named binding mapping a key to either a
// literal string value or a resolver function.  The resolver
// is a nullary callable returning std::string, invoked at
// render time for dynamic content.
//
// Usage:
//   text_specifier s("name", "djinterp");
//   text_specifier d("date", []() -> std::string {
//       return "2026-04-11";
//   });
//   std::string val = s.resolve();  // "djinterp"
//   std::string dat = d.resolve();  // "2026-04-11"
class text_specifier
{
public:
    using resolver_type = std::function<std::string()>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    text_specifier()
        : m_key(),
          m_value(),
          m_resolver(nullptr),
          m_is_dynamic(false)
    {}

    // from key and literal value
    text_specifier(
            const std::string& _key,
            const std::string& _value
        )
            : m_key(_key),
              m_value(_value),
              m_resolver(nullptr),
              m_is_dynamic(false)
    {}

    // from key and const char* value
    text_specifier(
            const std::string& _key,
            const char*        _value
        )
            : m_key(_key),
              m_value(_value ? _value : ""),
              m_resolver(nullptr),
              m_is_dynamic(false)
    {}

    // from key and resolver function
    template<typename _Fn,
             typename = typename std::enable_if<
                 traits::is_nullary_text_function<
                     typename std::decay<_Fn>::type>::value &&
                 !std::is_convertible<_Fn, std::string>::value &&
                 !std::is_convertible<_Fn, const char*>::value
             >::type>
    text_specifier(
            const std::string& _key,
            _Fn&&              _resolver
        )
            : m_key(_key),
              m_value(),
              m_resolver(static_cast<_Fn&&>(_resolver)),
              m_is_dynamic(true)
    {}

    // -----------------------------------------------------------------
    //  access
    // -----------------------------------------------------------------

    // key
    //   returns the specifier key.
    const std::string&
    key() const noexcept
    {
        return m_key;
    }

    // value
    //   returns the literal value (empty if dynamic).
    const std::string&
    value() const noexcept
    {
        return m_value;
    }

    // is_dynamic
    //   returns true if this specifier uses a resolver
    // function rather than a literal value.
    bool
    is_dynamic() const noexcept
    {
        return m_is_dynamic;
    }

    // resolve
    //   returns the resolved value: invokes the resolver
    // if dynamic, otherwise returns the literal value.
    std::string
    resolve() const
    {
        if ( (m_is_dynamic) &&
             (m_resolver) )
        {
            return m_resolver();
        }

        return m_value;
    }

    // -----------------------------------------------------------------
    //  mutation
    // -----------------------------------------------------------------

    // set_value
    //   sets a literal value, clearing any resolver.
    void
    set_value(
        const std::string& _value
    )
    {
        m_value      = _value;
        m_resolver   = nullptr;
        m_is_dynamic = false;

        return;
    }

    // set_resolver
    //   sets a resolver function, marking this specifier
    // as dynamic.
    template<typename _Fn>
    void
    set_resolver(
        _Fn&& _resolver
    )
    {
        m_resolver   = static_cast<_Fn&&>(_resolver);
        m_is_dynamic = true;

        return;
    }

private:
    std::string   m_key;
    std::string   m_value;
    resolver_type m_resolver;
    bool          m_is_dynamic;
};


///////////////////////////////////////////////////////////////////////////////
///                III. TEXT TEMPLATE                                        ///
///////////////////////////////////////////////////////////////////////////////

// D_TEXT_TEMPLATE_DEFAULT_PREFIX
//   constant: default prefix marker.
#ifndef D_TEXT_TEMPLATE_CPP_DEFAULT_PREFIX
    #define D_TEXT_TEMPLATE_CPP_DEFAULT_PREFIX "%"
#endif

// D_TEXT_TEMPLATE_DEFAULT_SUFFIX
//   constant: default suffix marker.
#ifndef D_TEXT_TEMPLATE_CPP_DEFAULT_SUFFIX
    #define D_TEXT_TEMPLATE_CPP_DEFAULT_SUFFIX "%"
#endif

// D_TEXT_TEMPLATE_DEFAULT_MAX_DEPTH
//   constant: default maximum nesting depth.
#ifndef D_TEXT_TEMPLATE_CPP_DEFAULT_MAX_DEPTH
    #define D_TEXT_TEMPLATE_CPP_DEFAULT_MAX_DEPTH 16
#endif


// text_template
//   class: marker-aware text template engine.  Binds named
// specifiers to values or functions, then performs
// find-and-replace substitution on a format string at
// render time.
//
//   Supports configurable prefix/suffix markers, nested
// template bindings (recursive expansion), function
// bindings (dynamic content), and list bindings (iterated
// sub-template expansion).
//
//   text_template satisfies the text_function protocol:
// operator()(string) returns the rendered string, making it
// composable in text_function chains.
//
// Usage:
//   text_template tmpl;
//   tmpl.bind("name", "djinterp");
//   tmpl.bind("version", "1.0");
//   std::string out = tmpl.render("Welcome to %name% v%version%");
//   // out == "Welcome to djinterp v1.0"
class text_template
{
public:
    using size_type         = std::size_t;
    using specifier_list    = std::vector<text_specifier>;
    using nested_list       = std::vector<
                                  std::pair<std::string,
                                            const text_template*>>;
    using resolver_type     = std::function<std::string()>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    text_template()
        : m_prefix(D_TEXT_TEMPLATE_CPP_DEFAULT_PREFIX),
          m_suffix(D_TEXT_TEMPLATE_CPP_DEFAULT_SUFFIX),
          m_specifiers(),
          m_nested(),
          m_max_depth(D_TEXT_TEMPLATE_CPP_DEFAULT_MAX_DEPTH)
    {}

    // from markers
    text_template(
            const std::string& _prefix,
            const std::string& _suffix
        )
            : m_prefix(_prefix),
              m_suffix(_suffix),
              m_specifiers(),
              m_nested(),
              m_max_depth(D_TEXT_TEMPLATE_CPP_DEFAULT_MAX_DEPTH)
    {}

    // -----------------------------------------------------------------
    //  marker configuration
    // -----------------------------------------------------------------

    // prefix
    const std::string&
    prefix() const noexcept
    {
        return m_prefix;
    }

    // suffix
    const std::string&
    suffix() const noexcept
    {
        return m_suffix;
    }

    // set_markers
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

    // max_depth
    size_type
    max_depth() const noexcept
    {
        return m_max_depth;
    }

    // set_max_depth
    void
    set_max_depth(
        size_type _depth
    ) noexcept
    {
        m_max_depth = _depth;

        return;
    }

    // -----------------------------------------------------------------
    //  binding: string values
    // -----------------------------------------------------------------

    // bind
    //   binds a key to a literal string value.  If the key
    // already exists, updates its value.
    void
    bind(
        const std::string& _key,
        const std::string& _value
    )
    {
        for (auto& spec : m_specifiers)
        {
            if (spec.key() == _key)
            {
                spec.set_value(_value);

                return;
            }
        }

        m_specifiers.emplace_back(_key, _value);

        return;
    }

    // -----------------------------------------------------------------
    //  binding: function values
    // -----------------------------------------------------------------

    // bind_function
    //   binds a key to a nullary resolver function invoked
    // at render time.
    void
    bind_function(
        const std::string& _key,
        resolver_type      _resolver
    )
    {
        for (auto& spec : m_specifiers)
        {
            if (spec.key() == _key)
            {
                spec.set_resolver(
                    static_cast<resolver_type&&>(_resolver));

                return;
            }
        }

        m_specifiers.emplace_back(
            _key,
            static_cast<resolver_type&&>(_resolver));

        return;
    }

    // bind (generic callable)
    //   binds a key to any nullary callable returning string.
    // Disambiguated from the string overload by SFINAE.
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

    // -----------------------------------------------------------------
    //  binding: nested templates
    // -----------------------------------------------------------------

    // bind_template
    //   binds a key to a nested text_template.  The nested
    // template is not owned; the caller must ensure it
    // outlives this template.  At render time, the nested
    // template's render() is called with an empty format
    // string (the template is expected to carry its own
    // format state, or the key's occurrence in the format
    // string acts as the format).
    void
    bind_template(
        const std::string&  _key,
        const text_template& _nested
    )
    {
        // remove from specifiers if present
        unbind(_key);

        m_nested.emplace_back(_key, &_nested);

        return;
    }

    // -----------------------------------------------------------------
    //  binding: list iteration
    // -----------------------------------------------------------------

    // bind_list
    //   binds a key to a list iteration.  At render time,
    // the sub-template is rendered _count times with
    // _bind_fn invoked for each index to populate per-item
    // bindings.  Results are joined by _separator.
    //
    //   _bind_fn signature:
    //     void(text_template& item_tmpl, size_t index,
    //          size_t count)
    template<typename _BindFn>
    void
    bind_list(
        const std::string& _key,
        text_template&     _item_template,
        size_type          _count,
        _BindFn&&          _bind_fn,
        const std::string& _item_format,
        const std::string& _separator = ""
    )
    {
        // capture by value for safety
        auto bind_fn       = static_cast<_BindFn&&>(_bind_fn);
        auto* item_tmpl    = &_item_template;
        auto  count        = _count;
        auto  item_fmt     = _item_format;
        auto  sep          = _separator;

        bind_function(
            _key,
            [bind_fn, item_tmpl, count,
             item_fmt, sep]() -> std::string
            {
                std::string result;

                for (size_type i = 0; i < count; ++i)
                {
                    // clear and populate per-item bindings
                    item_tmpl->clear_bindings();

                    // inject auto-keys
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

                    // user callback populates domain keys
                    bind_fn(*item_tmpl, i, count);

                    // render this item
                    result += item_tmpl->render(item_fmt);

                    // separator between items
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

    // -----------------------------------------------------------------
    //  unbinding
    // -----------------------------------------------------------------

    // unbind
    //   removes a binding by key.
    void
    unbind(
        const std::string& _key
    )
    {
        // remove from specifiers
        for (auto it = m_specifiers.begin();
             it != m_specifiers.end();
             ++it)
        {
            if (it->key() == _key)
            {
                m_specifiers.erase(it);
                break;
            }
        }

        // remove from nested
        for (auto it = m_nested.begin();
             it != m_nested.end();
             ++it)
        {
            if (it->first == _key)
            {
                m_nested.erase(it);
                break;
            }
        }

        return;
    }

    // clear_bindings
    //   removes all bindings.
    void
    clear_bindings()
    {
        m_specifiers.clear();
        m_nested.clear();

        return;
    }

    // -----------------------------------------------------------------
    //  query
    // -----------------------------------------------------------------

    // has_binding
    //   returns true if _key is bound.
    bool
    has_binding(
        const std::string& _key
    ) const
    {
        for (const auto& spec : m_specifiers)
        {
            if (spec.key() == _key)
            {
                return true;
            }
        }

        for (const auto& nest : m_nested)
        {
            if (nest.first == _key)
            {
                return true;
            }
        }

        return false;
    }

    // binding_count
    //   returns the total number of bindings.
    size_type
    binding_count() const noexcept
    {
        return m_specifiers.size() + m_nested.size();
    }

    // specifiers
    //   returns a const reference to the specifier list.
    const specifier_list&
    specifiers() const noexcept
    {
        return m_specifiers;
    }

    // -----------------------------------------------------------------
    //  rendering
    // -----------------------------------------------------------------

    // render
    //   performs marker-aware substitution on _format.
    // Scans for prefix+key+suffix tokens and replaces each
    // with the resolved value of the matching binding.
    // Unmatched tokens are preserved as-is.
    //
    //   Nested template bindings are expanded recursively,
    // decrementing the depth counter.  Exceeding max_depth
    // halts expansion and preserves the token.
    std::string
    render(
        const std::string& _format
    ) const
    {
        return render_impl(_format, m_max_depth);
    }

    // operator()
    //   text_function protocol: renders the format string.
    // Makes text_template composable in text_function chains.
    std::string
    operator()(
        const std::string& _format
    ) const
    {
        return render(_format);
    }

private:
    // -----------------------------------------------------------------
    //  render implementation
    // -----------------------------------------------------------------

    // render_impl
    //   recursive renderer with depth tracking.
    std::string
    render_impl(
        const std::string& _format,
        size_type          _remaining_depth
    ) const
    {
        std::string out;
        out.reserve(_format.size());

        size_type prefix_len = m_prefix.size();
        size_type suffix_len = m_suffix.size();
        size_type i          = 0;

        while (i < _format.size())
        {
            // look for prefix
            if ( (i + prefix_len <= _format.size()) &&
                 (_format.compare(i, prefix_len,
                                  m_prefix) == 0) )
            {
                // find matching suffix
                size_type key_start = i + prefix_len;
                size_type suf_pos   = _format.find(
                    m_suffix, key_start);

                if (suf_pos != std::string::npos)
                {
                    std::string key = _format.substr(
                        key_start,
                        suf_pos - key_start);

                    std::string resolved;
                    bool        found = false;

                    // check specifiers
                    found = resolve_specifier(
                        key, resolved);

                    // check nested templates
                    if ( (!found) &&
                         (_remaining_depth > 0) )
                    {
                        found = resolve_nested(
                            key, resolved,
                            _remaining_depth - 1);
                    }

                    if (found)
                    {
                        out += resolved;
                    }
                    else
                    {
                        // preserve unmatched token
                        out += m_prefix;
                        out += key;
                        out += m_suffix;
                    }

                    i = suf_pos + suffix_len;

                    continue;
                }
            }

            out += _format[i];
            ++i;
        }

        return out;
    }

    // resolve_specifier
    //   looks up _key in the specifier list and resolves
    // its value.
    bool
    resolve_specifier(
        const std::string& _key,
        std::string&       _out
    ) const
    {
        for (const auto& spec : m_specifiers)
        {
            if (spec.key() == _key)
            {
                _out = spec.resolve();

                return true;
            }
        }

        return false;
    }

    // resolve_nested
    //   looks up _key in the nested template list and
    // renders the nested template.
    bool
    resolve_nested(
        const std::string& _key,
        std::string&       _out,
        size_type          _remaining_depth
    ) const
    {
        for (const auto& nest : m_nested)
        {
            if (nest.first == _key)
            {
                if (nest.second)
                {
                    // render the nested template with an
                    // empty format — nested templates carry
                    // their own bindings but need a format
                    // to render.  Typically the user passes
                    // the format at the nested level.
                    _out = "";
                }

                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    std::string    m_prefix;
    std::string    m_suffix;
    specifier_list m_specifiers;
    nested_list    m_nested;
    size_type      m_max_depth;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   STRING INTERPOLATION                                ///
///////////////////////////////////////////////////////////////////////////////

// string_interpolation
//   function: performs batch find-and-replace on _input using
// an array of key/value pairs.  Each key is optionally
// wrapped with _prefix and _suffix before searching.
//
//   If _prefix and _suffix are both empty, keys are matched
// as bare substrings.  If only _prefix is non-empty, keys
// are matched as prefix+key.  If only _suffix is non-empty,
// keys are matched as key+suffix.  If both are non-empty,
// keys are matched as prefix+key+suffix.
//
//   Replacements are applied left-to-right in a single
// pass.  Each occurrence of every wrapped key is replaced.
// The function does not re-scan replacement text, so
// substitutions cannot trigger further expansions.
//
// Parameter(s):
//   _input:   the input string to interpolate.
//   _pairs:   array of {key, replacement} pairs.
//   _prefix:  marker prefix prepended to each key before
//             matching (may be empty).
//   _suffix:  marker suffix appended to each key before
//             matching (may be empty).
//
// Return:
//   a new string with all matched tokens replaced.
//
// Usage:
//   std::vector<std::pair<std::string, std::string>> pairs = {
//       {"name", "djinterp"},
//       {"version", "1.0"}
//   };
//   auto out = string_interpolation(
//       "Welcome to %name% v%version%",
//       pairs, "%", "%");
//   // out == "Welcome to djinterp v1.0"
//
//   // bare keys (no markers):
//   auto out2 = string_interpolation(
//       "Hello NAME, your ID is ID_NUM",
//       {{"NAME", "Alice"}, {"ID_NUM", "42"}});
//   // out2 == "Hello Alice, your ID is 42"
template<typename _PairContainer>
inline std::string
string_interpolation(
    const std::string&  _input,
    const _PairContainer& _pairs,
    const std::string&  _prefix = "",
    const std::string&  _suffix = ""
)
{
    std::string result = _input;

    for (const auto& pair : _pairs)
    {
        // build the token: prefix + key + suffix
        std::string token;
        token.reserve(
            _prefix.size() +
            pair.first.size() +
            _suffix.size());

        token += _prefix;
        token += pair.first;
        token += _suffix;

        if (token.empty())
        {
            continue;
        }

        // replace all occurrences
        std::size_t token_len = token.size();
        std::size_t repl_len  = pair.second.size();
        std::size_t pos       = 0;

        while (pos < result.size())
        {
            pos = result.find(token, pos);

            if (pos == std::string::npos)
            {
                break;
            }

            result.replace(pos, token_len, pair.second);
            pos += repl_len;
        }
    }

    return result;
}

// string_interpolation (initializer_list overload)
//   function: convenience overload accepting an initializer
// list of pairs.
//
// Usage:
//   auto out = string_interpolation(
//       "{greeting}, {target}!",
//       {{"greeting", "Hello"}, {"target", "world"}},
//       "{", "}");
//   // out == "Hello, world!"
inline std::string
string_interpolation(
    const std::string& _input,
    std::initializer_list<
        std::pair<std::string, std::string>> _pairs,
    const std::string& _prefix = "",
    const std::string& _suffix = ""
)
{
    std::vector<std::pair<std::string, std::string>> vec(
        _pairs.begin(), _pairs.end());

    return string_interpolation(_input, vec, _prefix, _suffix);
}

// string_interpolation (C-style arrays)
//   function: overload accepting parallel C-string arrays
// and a count, mirroring the C d_str_interp_quick()
// interface.
//
// Parameter(s):
//   _input:  the input string.
//   _keys:   array of key C-strings.
//   _values: array of replacement C-strings.
//   _count:  number of key/value pairs.
//   _prefix: marker prefix (may be nullptr for empty).
//   _suffix: marker suffix (may be nullptr for empty).
//
// Usage:
//   const char* keys[]   = {"name", "ver"};
//   const char* values[] = {"djinterp", "1.0"};
//   auto out = string_interpolation(
//       "%name% v%ver%", keys, values, 2, "%", "%");
inline std::string
string_interpolation(
    const std::string& _input,
    const char* const* _keys,
    const char* const* _values,
    std::size_t        _count,
    const char*        _prefix = "",
    const char*        _suffix = ""
)
{
    std::vector<std::pair<std::string, std::string>> pairs;
    pairs.reserve(_count);

    std::string pfx = _prefix ? _prefix : "";
    std::string sfx = _suffix ? _suffix : "";

    for (std::size_t i = 0; i < _count; ++i)
    {
        if ( (_keys[i]) &&
             (_values[i]) )
        {
            pairs.emplace_back(
                std::string(_keys[i]),
                std::string(_values[i]));
        }
    }

    return string_interpolation(_input, pairs, pfx, sfx);
}


///////////////////////////////////////////////////////////////////////////////
///                VI.  CHAINING UTILITIES                                   ///
///////////////////////////////////////////////////////////////////////////////

// chain_text
//   function: chains two text functions left-to-right.
// chain_text(f, g)(x) = g(f(x)).
template<typename _F1,
         typename _F2>
auto
chain_text(
    _F1&& _first,
    _F2&& _second
) -> text_function<const std::string&>
{
    auto f1 = static_cast<_F1&&>(_first);
    auto f2 = static_cast<_F2&&>(_second);

    return text_function<const std::string&>(
        [f1, f2](const std::string& _input) -> std::string
        {
            return f2(f1(_input));
        });
}


// fn_for_each
//   function: a text function factory that applies a
// transformation function to each element of a string
// container, joining results with a separator.
//
// Usage:
//   std::vector<std::string> items = {"a", "b", "c"};
//   auto fn = fn_for_each(items, [](const std::string& s) {
//       return "[" + s + "]";
//   }, ", ");
//   std::string result = fn();  // "[a], [b], [c]"
template<typename _Container,
         typename _Fn>
std::function<std::string()>
fn_for_each(
    const _Container&  _items,
    _Fn&&              _transform,
    const std::string& _separator = ""
)
{
    auto items     = _items;
    auto transform = static_cast<_Fn&&>(_transform);
    auto sep       = _separator;

    return [items, transform, sep]() -> std::string
    {
        std::string result;
        bool        first = true;

        for (const auto& item : items)
        {
            if (!first && !sep.empty())
            {
                result += sep;
            }

            result += transform(item);
            first = false;
        }

        return result;
    };
}


///////////////////////////////////////////////////////////////////////////////
///                VII. CONVENIENCE FACTORIES                                ///
///////////////////////////////////////////////////////////////////////////////

// make_text_function
//   function: wraps any callable into a text_function.
template<typename _Fn>
text_function<const std::string&>
make_text_function(
    _Fn&& _fn
)
{
    return text_function<const std::string&>(
        static_cast<_Fn&&>(_fn));
}

// make_text_template
//   function: creates a text_template with the given markers.
inline text_template
make_text_template(
    const std::string& _prefix = D_TEXT_TEMPLATE_CPP_DEFAULT_PREFIX,
    const std::string& _suffix = D_TEXT_TEMPLATE_CPP_DEFAULT_SUFFIX
)
{
    return text_template(_prefix, _suffix);
}

// make_curly_template
//   function: creates a text_template with "{" / "}" markers
// (matching the existing test_printer format convention).
inline text_template
make_curly_template()
{
    return text_template("{", "}");
}


NS_END  // text
NS_END  // djinterp


#endif  // DJINTERP_TEXT_FUNCTION_
