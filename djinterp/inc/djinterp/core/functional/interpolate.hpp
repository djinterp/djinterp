/*******************************************************************************
* djinterp [core]                                                interpolate.hpp
*
*   The type-agnostic interpolation engine: the find-and-replace common to
* text_template and binary_template, factored out so neither owns it.  An
* interpolation is a single left-to-right fold of *scan events* into a *sink*,
* with the three independent axes of variation made into three policies:
*
*     scanner   -- WHERE the placeholders are (owns the syntax: braces vs
*                  sigils; owns runtime-scan vs pre-parsed).  Drives a pull
*                  cursor that yields one `piece` (a literal run or a key) per
*                  step, so output is written segment-by-segment with no
*                  intermediate structure.  Speaks the framework scanner/token
*                  vocabulary (input_type / item_type / result_type; a piece is
*                  the token, with kind_type + value_type).
*     resolver  -- WHAT a key becomes: (key) -> resolution<value>.  A miss
*                  leaves the placeholder untouched, which is what makes PARTIAL
*                  interpolation well-defined.  Resolvers compose: a chain tries
*                  each frame in order via the resolution's lazy `or_else` -- a
*                  later frame's lookup runs only when the earlier frame MISSES
*                  -- and composition is associative, so N chained frames
*                  collapse into ONE pass.  A predicate-gated frame composes its
*                  conditions with predicate.hpp (`all_of` / `predicate_and` /
*                  ...) when there is more than one.  A resolved value that is
*                  itself a template is expanded in turn by recursive_resolver
*                  (VALUE-level nesting, distinct from the scanner's flat
*                  no-nested-braces syntax; see VI).
*     sink      -- HOW output is assembled: literal(run) + value(resolved).
*                  interp_string_sink appends into a buffer; a byte sink encodes; a
*                  consumer/accumulator can stand in.  Runtime vs constexpr is a
*                  sink choice (a fixed-capacity / caller-provided buffer for
*                  constexpr, std::basic_string for runtime).
*
*   In the vocabulary of template.hpp the engine is the transformation F and
* `interpolate(t)` is F-hat(t) = F_t (the source-transformer a template names);
* the resolver is the source, the produced output is the sink.  Where
* template.hpp curries the TEMPLATE, the lazy `interpolation` builder additionally
* curries the SOURCE -- consecutive `.interpolate(...)` calls extend a resolver
* chain held in the type and do no work until a terminal forces the single pass.
*
*   This is deliberately NOT a parser/grammar: interpolation is a flat
* alternation of literal | placeholder with no nesting (first `}` closes), so a
* one-pass find loop is exact and minimal.  `parser<Type>` remains a valid
* scanner (the pre-parsed fast path for repeated renders); the engine itself is
* a fold, not a recognizer.
*
*   Requires C++17 (std::string_view backs the piece views); self-suppresses
* below it.  constexpr throughout; concepts gated on C++20.
*
*
* path:      /inc/djinterp/core/functional/interpolate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.15
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SCAN-EVENT VOCABULARY
      ---------------------
      i.    piece_kind
      ii.   piece                     -- the uniform scan event (token)

II.   SCANNERS
      --------
      i.    brace_scanner             -- {key}, { key } trimmed, {{ }} escaped
      ii.   sigil_scanner             -- $name style (configurable sigil)
      iii.  replay_scanner            -- replay a pre-scanned piece cache

III.  RESOLUTION & RESOLVERS
      ----------------------
      i.    resolution                -- found/value + lazy or_else (a min. maybe)
      ii.   empty_resolver            -- identity (everything passes through)
      iii.  map_resolver              -- inline {key,value} bindings
      iv.   lookup_resolver           -- adapt a callable (always-found)
      v.    chain_resolver            -- try a, else b (via or_else; associative)
      vi.   when_resolver             -- gate on a key predicate
      vii.  factories: bindings / lookup / chain / when

IV.   SINKS
      -----
      i.    interp_string_sink               -- append into a basic_string buffer

V.    THE ENGINE
      ----------
      i.    interpolate_into          -- the fold: scanner -> resolver -> sink

VI.   RECURSIVE EXPANSION
      -------------------
      i.    recursive_resolver        -- re-scan a hit's value (nested templates)
      ii.   recursive                 -- factory

VII.  THE LAZY FUNCTOR
      ----------------
      i.    interpolation             -- template + resolver chain, in the type
      ii.   interpolate / make_interpolation
      iii.  .recursive()              -- nested-template terminal option
      iv.   .prepare()                -- pre-parse for repeated rendering

VIII. PREPARED TEMPLATES  (pre-parse once, render many)
      -------------------------------------------------
      i.    prepared_interpolation    -- template + resolver over a piece cache
      ii.   prepare / make_prepared   -- factories (shared cache)
      iii.  prepare_into              -- fill a caller-owned cache container

IX.   CONCEPTS  (C++20)
      -----------------
*/

#ifndef DJINTERP_FUNCTIONAL_INTERPOLATE_HPP
#define DJINTERP_FUNCTIONAL_INTERPOLATE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <initializer_list>
#include <memory>            // std::shared_ptr (prepared-template cache)
#include <string>
#include <string_view>
#include <utility>
#include <vector>            // std::vector (default piece cache)
// djinterp
#include "../../djinterp.hpp"      // NS_*, D_CONSTEXPR, D_NODISCARD, clean_t
#include "../meta/type_utility.hpp"  // clean_t


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #include "../meta/concepts.hpp"
#endif

// std::string_view is the spine of the piece views; below C++17 this module
// contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP

// ===========================================================================
// I.   SCAN-EVENT VOCABULARY
// ===========================================================================

// piece_kind
//   enum: which sort of scan event a piece is.
enum class piece_kind
{
    literal,
    key
};

// piece
//   struct: one scan event -- the framework token specialized to interpolation.
// A `literal` carries the (escape-collapsed, contiguous) span to emit verbatim;
// a `key` carries the trimmed name to resolve plus the full delimited slice, so
// an unresolved key can be re-emitted untouched for a later resolver frame.
template<typename Type = char>
struct piece
{
    using char_type  = Type;
    using view_type  = std::basic_string_view<Type>;
    using kind_type  = piece_kind;
    using value_type = view_type;

    piece_kind m_kind = piece_kind::literal;
    view_type  m_span {};   // literal: text to emit
    view_type  m_key  {};   // key:     trimmed name to resolve
    view_type  m_raw  {};   // key:     full "{...}" slice (passthrough on a miss)

    D_NODISCARD D_CONSTEXPR bool
    is_key() const
    {
        return (m_kind == piece_kind::key);
    }
};


// ===========================================================================
// II.  SCANNERS
// ===========================================================================
//   A scanner is a pull cursor over a format: `next(piece&)` yields the next
// event and returns false at end.  It exposes the framework scanner typedefs
// (input_type, item_type, result_type) and models `has_find_method`-style
// repeated search.  Two are shipped; users may write their own -- the engine
// neither knows nor cares about the placeholder syntax.

// brace_scanner
//   class: scans `{key}` placeholders -- `{ key }` is trimmed, `{{`/`}}` are
// escaped literal braces, an unmatched `{` (or lone `}`) is kept literally, and
// the first `}` closes (no nesting).  Literal pieces are always contiguous
// source slices: an escaped brace is emitted as a one-char literal pointing at
// the first of the pair, so the whole scan stays zero-copy.
template<typename Type = char>
class brace_scanner
{
public:
    using char_type   = Type;
    using view_type   = std::basic_string_view<Type>;
    using size_type   = std::size_t;

    // input_type
    //   type: scanner contract -- the element type of the scanned input.
    using input_type  = char_type;

    // item_type
    //   type: scanner contract -- a discovered element (one scan event).
    using item_type   = piece<Type>;
    using piece_type  = piece<Type>;

    // result_type
    //   type: scanner contract -- what a successful scan step yields.
    using result_type = piece<Type>;

    D_CONSTEXPR explicit brace_scanner(
        view_type _source
    ) D_NOEXCEPT
        : m_source(_source)
    {}

    // next -- yield the next piece; false once the format is exhausted
    D_CONSTEXPR_CPP14 bool
    next(
        piece_type& _out
    )
    {
        // a queued brace/key piece, emitted after its preceding literal run
        if (m_has_pending)
        {
            _out          = m_pending;
            m_has_pending = false;
            return true;
        }

        const Type*    s = m_source.data();
        const size_type n = m_source.size();

        while (m_i < n)
        {
            const Type c = s[m_i];

            // opening or escaped brace
            if (c == Type('{'))
            {
                // escaped "{{" -> literal '{'
                if (((m_i + 1) < n) && (s[m_i + 1] == Type('{')))
                {
                    return m_emit(m_literal(m_i, 1), 2, _out);
                }

                // locate the closing brace (first '}' closes; no nesting)
                size_type close = m_i + 1;
                while ((close < n) && (s[close] != Type('}')))
                {
                    ++close;
                }

                // unmatched '{' -- keep it literally and keep scanning
                if (close >= n)
                {
                    ++m_i;
                    continue;
                }

                return m_emit_key(close, _out);
            }

            // escaped "}}" -> literal '}'
            if ( (c == Type('}'))      &&
                 ((m_i + 1) < n)        &&
                 (s[m_i + 1] == Type('}')) )
            {
                return m_emit(m_literal(m_i, 1), 2, _out);
            }

            // ordinary character (a lone '}' falls through, staying literal)
            ++m_i;
        }

        // trailing literal
        if (n > m_lit_start)
        {
            _out        = m_literal(m_lit_start, n - m_lit_start);
            m_lit_start = n;
            return true;
        }

        return false;
    }

private:
    // m_literal -- a literal piece over [_offset, _offset + _length)
    D_CONSTEXPR_CPP14 piece_type
    m_literal(
        size_type _offset,
        size_type _length
    ) const
    {
        piece_type _p;
        _p.m_kind = piece_kind::literal;
        _p.m_span = view_type(m_source.data() + _offset, _length);

        return _p;
    }

    // m_emit -- emit a brace piece, flushing any preceding literal first (the
    // brace piece is then queued), and advance past `_consumed` source chars.
    D_CONSTEXPR_CPP14 bool
    m_emit(
        piece_type  _brace,
        size_type   _consumed,
        piece_type& _out
    )
    {
        if (m_i > m_lit_start)
        {
            _out          = m_literal(m_lit_start, m_i - m_lit_start);
            m_pending     = _brace;
            m_has_pending = true;
        }
        else
        {
            _out = _brace;
        }

        m_i        += _consumed;
        m_lit_start = m_i;

        return true;
    }

    // m_emit_key -- form the key piece at [m_i, _close], flushing any preceding
    // literal first; the trimmed name is the lookup key, the full slice the raw.
    D_CONSTEXPR_CPP14 bool
    m_emit_key(
        size_type   _close,
        piece_type& _out
    )
    {
        const Type* s  = m_source.data();
        size_type    kb = m_i + 1;
        size_type    ke = _close;

        while ((kb < ke) && m_is_space(s[kb]))
        {
            ++kb;
        }

        while ((ke > kb) && m_is_space(s[ke - 1]))
        {
            --ke;
        }

        piece_type _key;
        _key.m_kind = piece_kind::key;
        _key.m_key  = view_type(s + kb, ke - kb);
        _key.m_raw  = view_type(s + m_i, (_close + 1) - m_i);

        return m_emit(_key, ((_close + 1) - m_i), _out);
    }

    // m_is_space -- ASCII-whitespace test for key trimming
    static D_CONSTEXPR bool
    m_is_space(
        Type _c
    )
    {
        return (_c == Type(' '))  || (_c == Type('\t')) ||
               (_c == Type('\n')) || (_c == Type('\r'));
    }

    view_type  m_source;
    size_type  m_i           = 0;
    size_type  m_lit_start   = 0;
    piece_type m_pending     {};
    bool       m_has_pending = false;
};


// sigil_scanner
//   class: scans `<sigil>name` placeholders (e.g. `$name`), where a name is a
// maximal run of name-characters.  The sigil and the name-character set are
// configurable; a doubled sigil escapes a literal one.  Demonstrates that the
// placeholder syntax is entirely the scanner's business -- the engine is
// unchanged.  (Name predicate defaults to alnum-or-underscore.)
template<typename Type = char>
class sigil_scanner
{
public:
    using char_type   = Type;
    using view_type   = std::basic_string_view<Type>;
    using size_type   = std::size_t;
    using input_type  = char_type;
    using item_type   = piece<Type>;
    using piece_type  = piece<Type>;
    using result_type = piece<Type>;

    D_CONSTEXPR sigil_scanner(
        view_type _source,
        Type      _sigil = Type('$')
    ) D_NOEXCEPT
        : m_source(_source),
          m_sigil(_sigil)
    {}

    D_CONSTEXPR_CPP14 bool
    next(
        piece_type& _out
    )
    {
        if (m_has_pending)
        {
            _out          = m_pending;
            m_has_pending = false;
            return true;
        }

        const Type*    s = m_source.data();
        const size_type n = m_source.size();

        while (m_i < n)
        {
            // a sigil opens a placeholder (or escapes itself when doubled)
            if (s[m_i] == m_sigil)
            {
                // doubled sigil -> a literal sigil
                if (((m_i + 1) < n) && (s[m_i + 1] == m_sigil))
                {
                    return m_emit(m_literal(m_i, 1), 2, _out);
                }

                size_type name_begin = m_i + 1;
                size_type name_end   = name_begin;
                while ((name_end < n) && m_is_name(s[name_end]))
                {
                    ++name_end;
                }

                // a bare sigil with no name -- keep it literally
                if (name_end == name_begin)
                {
                    ++m_i;
                    continue;
                }

                piece_type _key;
                _key.m_kind = piece_kind::key;
                _key.m_key  = view_type(s + name_begin, name_end - name_begin);
                _key.m_raw  = view_type(s + m_i, name_end - m_i);

                return m_emit(_key, (name_end - m_i), _out);
            }

            ++m_i;
        }

        if (n > m_lit_start)
        {
            _out        = m_literal(m_lit_start, n - m_lit_start);
            m_lit_start = n;
            return true;
        }

        return false;
    }

private:
    D_CONSTEXPR_CPP14 piece_type
    m_literal(
        size_type _offset,
        size_type _length
    ) const
    {
        piece_type _p;
        _p.m_kind = piece_kind::literal;
        _p.m_span = view_type(m_source.data() + _offset, _length);

        return _p;
    }

    D_CONSTEXPR_CPP14 bool
    m_emit(
        piece_type  _piece,
        size_type   _consumed,
        piece_type& _out
    )
    {
        if (m_i > m_lit_start)
        {
            _out          = m_literal(m_lit_start, m_i - m_lit_start);
            m_pending     = _piece;
            m_has_pending = true;
        }
        else
        {
            _out = _piece;
        }

        m_i        += _consumed;
        m_lit_start = m_i;

        return true;
    }

    static D_CONSTEXPR bool
    m_is_name(
        Type _c
    )
    {
        return ( ((_c >= Type('a')) && (_c <= Type('z'))) ||
                 ((_c >= Type('A')) && (_c <= Type('Z'))) ||
                 ((_c >= Type('0')) && (_c <= Type('9'))) ||
                 (_c == Type('_')) );
    }

    view_type  m_source;
    Type       m_sigil;
    size_type  m_i           = 0;
    size_type  m_lit_start   = 0;
    piece_type m_pending     {};
    bool       m_has_pending = false;
};


// replay_scanner
//   class: a scanner that REPLAYS a pre-scanned sequence of pieces instead of
// re-deriving it from the template text.  A template's scan is independent of
// the resolver and yields the same pieces every render, so a template rendered
// repeatedly can be scanned ONCE (see prepare / .prepare, section VIII) and
// its pieces replayed -- turning per-render cost into lookups + emits with no
// re-scan.  The pieces hold views into the original template, so that template
// must outlive the cache.  Cache is any forward-iterable sequence of
// piece<Type> (std::vector by default).
template<typename Type,
         typename Cache = std::vector<piece<Type>>>
class replay_scanner
{
public:
    using char_type   = Type;
    using view_type   = std::basic_string_view<Type>;
    using input_type  = Type;
    using item_type   = piece<Type>;
    using piece_type  = piece<Type>;
    using result_type = piece<Type>;

    explicit replay_scanner(
        const Cache& _cache
    )
        : m_it(_cache.begin()),
          m_end(_cache.end())
    {}

    // next -- yield the next cached piece; false once the cache is exhausted
    D_NODISCARD bool
    next(
        piece_type& _out
    )
    {
        if (m_it == m_end)
        {
            return false;
        }

        _out = *m_it;
        ++m_it;

        return true;
    }

private:
    using iterator_type = decltype(std::declval<const Cache&>().begin());

    iterator_type m_it;
    iterator_type m_end;
};


// ===========================================================================
// III. RESOLUTION & RESOLVERS
// ===========================================================================
//   A resolver maps a key to a `resolution<value>`: a hit replaces, a miss
// leaves the placeholder.  This is the upgrade over a bare (key) -> value
// lookup (which cannot distinguish "absent" from "empty"); the explicit miss is
// what lets a key survive one frame to be filled by the next.  resolution is a
// minimal maybe -- swap in djinterp::maybe<Value> wherever richer is wanted.

// resolution
//   struct: a resolver's answer -- a found flag and (when found) a value.  As
// the minimal maybe it carries the lazy `or_else` combinator that chaining is
// expressed in terms of (see chain_resolver).
template<typename Value>
struct resolution
{
    using value_type = Value;

    bool   m_found = false;
    Value m_value {};

    D_NODISCARD D_CONSTEXPR bool
    found() const
    {
        return m_found;
    }

    D_NODISCARD D_CONSTEXPR const Value&
    value() const
    {
        return m_value;
    }

    // or_else
    //   method: if this is a hit, returns it unchanged; otherwise invokes
    // `_alt` (a nullary factory returning a resolution of the same value type)
    // and returns its result.  Lazy -- `_alt` runs ONLY on a miss, so a chain
    // pays for a fallback frame's lookup solely when the earlier frame does not
    // hit.  This is the "try this, else that" step chain_resolver routes through.
    template<typename Alt>
    D_NODISCARD D_CONSTEXPR_CPP14 resolution
    or_else(
        Alt _alt
    ) const
    {
        if (m_found)
        {
            return *this;
        }

        return _alt();
    }
};

// resolved / unresolved
//   function: build a hit / a miss.
template<typename Value>
D_NODISCARD D_CONSTEXPR resolution<clean_t<Value>>
resolved(
    Value&& _value
)
{
    return resolution<clean_t<Value>>{true, static_cast<Value&&>(_value)};
}

template<typename Value>
D_NODISCARD D_CONSTEXPR resolution<Value>
unresolved()
{
    return resolution<Value>{false, Value{}};
}


// empty_resolver
//   class: resolves nothing -- the identity source.  Every key passes through
// untouched, so `interpolate(t)` with no bindings reproduces t (modulo escapes).
template<typename Type = char>
struct empty_resolver
{
    using view_type = std::basic_string_view<Type>;

    D_NODISCARD D_CONSTEXPR resolution<view_type>
    operator()(
        view_type
    ) const
    {
        return unresolved<view_type>();
    }
};


// map_resolver
//   class: a linear-scan lookup over inline {key, value} bindings; a missing
// key is a MISS (left as a placeholder).  Values are views -- zero-copy over
// the bindings' backing storage, which must outlive any deferred render (place
// literals or owned strings in the list, not per-call temporaries).
template<typename Type = char>
class map_resolver
{
public:
    using char_type  = Type;
    using view_type  = std::basic_string_view<Type>;
    using pair_type  = std::pair<view_type, view_type>;
    using value_type = view_type;

    map_resolver(
        std::initializer_list<pair_type> _bindings
    )
        : m_bindings(_bindings)
    {}

    D_NODISCARD resolution<view_type>
    operator()(
        view_type _key
    ) const
    {
        for (const pair_type& _entry : m_bindings)
        {
            if (_entry.first == _key)
            {
                return resolved(_entry.second);
            }
        }

        return unresolved<view_type>();
    }

private:
    std::vector<pair_type> m_bindings;
};


// lookup_resolver
//   class: adapt a plain callable (view_type) -> (convertible to a string) into
// a resolver that ALWAYS hits (the classic text_template policy: a missing key
// becomes an empty replacement).  It OWNS the produced value, because the
// resolver/sink split consumes the value slightly later than a hand-written
// render loop did -- a view over a returned temporary would dangle.
template<typename Type,
         typename Fn>
class lookup_resolver
{
public:
    using char_type   = Type;
    using view_type   = std::basic_string_view<Type>;
    using string_type = std::basic_string<Type>;
    using value_type  = string_type;

    D_CONSTEXPR explicit lookup_resolver(
        Fn _fn
    )
        : m_fn(_fn)
    {}

    D_NODISCARD resolution<string_type>
    operator()(
        view_type _key
    ) const
    {
        return resolved(string_type(m_fn(_key)));
    }

private:
    Fn m_fn;
};

// chain_resolver
//   class: try A, then B -- the first hit wins, a miss falls through.  The
// fall-through is the resolution's lazy `or_else`: B's lookup is evaluated
// ONLY when A misses.  Chain composition is associative, so an N-deep chain is
// one pass with at most N lookups.  Every frame in a chain must agree on the
// resolution value type.
template<typename A,
         typename B>
class chain_resolver
{
public:
    D_CONSTEXPR chain_resolver(
        A _a,
        B _b
    )
        : m_a(_a),
          m_b(_b)
    {}

    template<typename Key>
    D_NODISCARD D_CONSTEXPR auto
    operator()(
        Key _key
    ) const
    {
        // try A; on a miss, fall through to B via the resolution's lazy
        // or_else -- B(_key) runs only when A did not hit
        return m_a(_key).or_else(
            [this, _key]()
            {
                return m_b(_key);
            });
    }

private:
    A m_a;
    B m_b;
};

// when_resolver
//   class: gate a resolver on a key predicate.  Keys satisfying Pred are
// resolved by R; the rest fall through as a miss (left as placeholders) for a
// later frame.  This is how predicates are "sprinkled into the flow".  When the
// gate has more than one condition, compose the leaf predicates with
// predicate.hpp (`all_of(p1, p2, ...)`, `predicate_and`, `any_of`, ...) and
// pass the single composed predicate -- the gate stays predicate-agnostic.
template<typename Pred,
         typename R>
class when_resolver
{
public:
    D_CONSTEXPR when_resolver(
        Pred _pred,
        R     _resolver
    )
        : m_pred(_pred),
          m_resolver(_resolver)
    {}

    template<typename Key>
    D_NODISCARD D_CONSTEXPR_CPP14 auto
    operator()(
        Key _key
    ) const
    {
        // gate open: defer to the inner resolver
        if (m_pred(_key))
        {
            return m_resolver(_key);
        }

        // gate closed: a miss of the inner resolver's value type
        return decltype(m_resolver(_key)){false, {}};
    }

private:
    Pred m_pred;
    R     m_resolver;
};


// bindings -- a map_resolver from an inline {key, value} list
template<typename Type = char>
D_NODISCARD map_resolver<Type>
bindings(
    std::initializer_list<std::pair<std::basic_string_view<Type>,
                                    std::basic_string_view<Type>>> _list
)
{
    return map_resolver<Type>(_list);
}

// lookup -- a lookup_resolver from a callable (always-hit)
template<typename Type = char,
         typename Fn>
D_NODISCARD D_CONSTEXPR lookup_resolver<Type, clean_t<Fn>>
lookup(
    Fn&& _fn
)
{
    return lookup_resolver<Type, clean_t<Fn>>(static_cast<Fn&&>(_fn));
}

// chain -- compose two resolvers (first hit wins, second tried lazily)
template<typename A,
         typename B>
D_NODISCARD D_CONSTEXPR chain_resolver<clean_t<A>, clean_t<B>>
chain(
    A&& _a,
    B&& _b
)
{
    return chain_resolver<clean_t<A>, clean_t<B>>(
        static_cast<A&&>(_a),
        static_cast<B&&>(_b));
}

// when -- gate a resolver on a key predicate (compose multi-condition gates
// with predicate.hpp before passing them here)
template<typename Pred,
         typename R>
D_NODISCARD D_CONSTEXPR when_resolver<clean_t<Pred>, clean_t<R>>
when(
    Pred&& _pred,
    R&&    _resolver
)
{
    return when_resolver<clean_t<Pred>, clean_t<R>>(
        static_cast<Pred&&>(_pred),
        static_cast<R&&>(_resolver));
}


// ===========================================================================
// IV.  SINKS
// ===========================================================================
//   A sink exposes literal(run) and value(resolved).  These are the two -- and
// only two -- operations the scanner/resolver pair hand off, so any output
// shape is uniform here: a growable buffer, a fixed-capacity constexpr buffer,
// a byte encoder (for binary), or a wrapped consumer/accumulator.

// interp_string_sink
//   class: appends each emitted span into a caller-owned basic_string -- no
// result allocation of its own (the buffer is the caller's to size and reuse).
template<typename Type = char>
class interp_string_sink
{
public:
    using char_type   = Type;
    using view_type   = std::basic_string_view<Type>;
    using string_type = std::basic_string<Type>;

    D_CONSTEXPR explicit interp_string_sink(
        string_type& _out
    )
        : m_out(_out)
    {}

    D_CONSTEXPR_CPP14 void
    literal(
        view_type _run
    )
    {
        m_out.append(_run.data(), _run.size());

        return;
    }

    template<typename Value>
    D_CONSTEXPR_CPP14 void
    value(
        const Value& _value
    )
    {
        const view_type _view(_value);
        m_out.append(_view.data(), _view.size());

        return;
    }

private:
    string_type& m_out;
};


// ===========================================================================
// V.   THE ENGINE
// ===========================================================================

// interpolate_into
//   function: the fold.  Drives `_scanner` (already bound to its template) one
// piece at a time: a literal is emitted verbatim; a key is resolved and its
// value emitted, or -- on a miss -- the raw placeholder is emitted untouched.
// The resolution is held in a local so its value outlives the sink call.  This
// is the single point where scanner, resolver, and sink meet; it knows nothing
// of syntax, value types, or buffering.
template<typename Sink,
         typename Scanner,
         typename Resolver>
D_CONSTEXPR_CPP14 void
interpolate_into(
    Sink&           _sink,
    Scanner          _scanner,
    const Resolver& _resolver
)
{
    typename Scanner::piece_type _p;

    while (_scanner.next(_p))
    {
        // literal run: emit verbatim
        if (!_p.is_key())
        {
            _sink.literal(_p.m_span);
            continue;
        }

        // key: resolve, then emit the value or leave the placeholder
        auto _r = _resolver(_p.m_key);
        if (_r.found())
        {
            _sink.value(_r.value());
        }
        else
        {
            _sink.literal(_p.m_raw);
        }
    }

    return;
}


// ===========================================================================
// VI.  RECURSIVE EXPANSION
// ===========================================================================
//   The engine emits a hit's value verbatim -- it does not look inside it.
// "Nested templates" means lifting exactly that: when a resolved value is
// ITSELF a template (it contains placeholders), feed it back through the engine
// so those placeholders resolve too.  This is nesting at the VALUE level and is
// orthogonal to the scanner's flat, no-nested-braces SYNTAX -- the recursive
// resolver is a plain decorator; the scanner, the fold, and every other
// resolver stay untouched.

// recursive_resolver
//   class: a resolver decorator that makes resolved VALUES interpolable.  On a
// hit it re-scans the inner resolver's value with the SAME scanner and resolves
// the placeholders found there, one level at a time, to a depth bound.  The
// bound is what guarantees termination: a self- or mutually-referential binding
// (a -> "{b}", b -> "{a}") expands until the budget is spent, then the deepest
// still-unresolved placeholder is left intact -- a passthrough, never a loop.
// The expanded value is freshly built, so value_type is the owning string type
// (it sits beside lookup_resolver among the owning-value frames, not the view
// frames).  Re-scanning resolves against this same wrapped resolver, so a
// nested key sees exactly what the wrapped resolver sees.
template<typename Type,
         typename Inner,
         typename Scanner = brace_scanner<Type>>
class recursive_resolver
{
public:
    using char_type   = Type;
    using view_type   = std::basic_string_view<Type>;
    using string_type = std::basic_string<Type>;
    using value_type  = string_type;

    D_CONSTEXPR recursive_resolver(
        Inner       _inner,
        std::size_t _max_depth = 16
    )
        : m_inner(_inner),
          m_max_depth(_max_depth)
    {}

    D_NODISCARD resolution<string_type>
    operator()(
        view_type _key
    ) const
    {
        return m_expand(_key, m_max_depth);
    }

private:
    // m_expand -- resolve _key through the inner resolver; on a hit, re-scan
    // the value with one less budget so a nested key expands in turn.  A miss
    // stays a miss; a spent budget emits the value without expanding further.
    D_NODISCARD resolution<string_type>
    m_expand(
        view_type   _key,
        std::size_t _budget
    ) const
    {
        const auto _hit = m_inner(_key);
        if (!_hit.found())
        {
            return unresolved<string_type>();
        }

        const view_type _value(_hit.value());

        // budget spent: emit the value as-is, nested placeholders and all
        if (_budget == 0)
        {
            return resolved(string_type(_value));
        }

        // re-scan the value; each nested key resolves one level deeper
        string_type        _out;
        interp_string_sink<Type> _sink(_out);
        interpolate_into(
            _sink,
            Scanner(_value),
            [this, _budget](view_type _nested)
            {
                return m_expand(_nested, _budget - 1);
            });

        return resolved(static_cast<string_type&&>(_out));
    }

    Inner       m_inner;
    std::size_t m_max_depth;
};


// recursive -- wrap a resolver so its hits are themselves interpolated (nested
// templates), bounded by _max_depth.  The scanner defaults to brace_scanner;
// pass the scanner the outer template uses so a nested placeholder shares its
// syntax.  Because the result carries an owning string value, a recursive frame
// composes with other owning-value frames (lookup, recursive) but not with the
// view-valued bindings / empty frames -- apply it as the last resolver step.
template<typename Type     = char,
         typename Scanner = brace_scanner<Type>,
         typename Inner>
D_NODISCARD recursive_resolver<Type, clean_t<Inner>, Scanner>
recursive(
    Inner&&    _inner,
    std::size_t _max_depth = 16
)
{
    return recursive_resolver<Type, clean_t<Inner>, Scanner>(
        static_cast<Inner&&>(_inner),
        _max_depth);
}


// forward declaration: the pre-parsed counterpart of `interpolation` (section
// VIII).  `interpolation::prepare` (below) returns one.
template<typename Type,
         typename Resolver,
         typename Scanner = brace_scanner<Type>,
         typename Cache    = std::vector<piece<Type>>>
class prepared_interpolation;


// ===========================================================================
// VII. THE LAZY FUNCTOR
// ===========================================================================

// interpolation
//   class: a bound template together with a resolver CHAIN carried in the type
// (no std::function, fully inlinable).  `.interpolate(...)` returns a new
// interpolation whose type appends a frame and does no work; a terminal
// (`str`, `into`, or the string conversion) forces ONE pass over the whole
// chain.  Consecutive `.interpolate(...)` calls therefore collapse into a
// single pass with no intermediate buffer -- at compile time too, since the
// chain is a type.  This is F_t with its source curried: template.hpp curries
// the template, this curries the source.
template<typename Type      = char,
         typename Resolver = empty_resolver<char>,
         typename Scanner   = brace_scanner<char>>
class interpolation
{
public:
    using char_type     = Type;
    using view_type     = std::basic_string_view<Type>;
    using string_type   = std::basic_string<Type>;
    using scanner_type  = Scanner;
    using resolver_type = Resolver;

    D_CONSTEXPR interpolation(
        view_type _template,
        Resolver _resolver
    )
        : m_template(_template),
          m_resolver(_resolver)
    {}

    // interpolate -- append a resolver frame (the collapsed chain is a new type)
    template<typename R2>
    D_NODISCARD D_CONSTEXPR
    interpolation<Type, chain_resolver<Resolver, clean_t<R2>>, Scanner>
    interpolate(
        R2&& _r2
    ) const
    {
        using chained = chain_resolver<Resolver, clean_t<R2>>;

        return interpolation<Type, chained, Scanner>(
            m_template,
            chained(m_resolver, static_cast<R2&&>(_r2)));
    }

    // interpolate -- inline-bindings convenience over the resolver form
    D_NODISCARD
    interpolation<Type, chain_resolver<Resolver, map_resolver<Type>>, Scanner>
    interpolate(
        std::initializer_list<std::pair<view_type, view_type>> _list
    ) const
    {
        return interpolate(map_resolver<Type>(_list));
    }

    // interpolate_if -- append a predicate-gated resolver frame.  For more than
    // one condition, compose the leaf predicates with predicate.hpp (e.g.
    // `all_of(p1, p2, ...)`) and pass the single combined predicate.
    template<typename Pred,
             typename R2>
    D_NODISCARD D_CONSTEXPR auto
    interpolate_if(
        Pred&& _pred,
        R2&&   _r2
    ) const
    {
        return interpolate(when(static_cast<Pred&&>(_pred),
                                static_cast<R2&&>(_r2)));
    }

    // recursive -- wrap the whole resolver chain so resolved VALUES are
    // themselves interpolated (nested templates), to a depth bound.  A nested
    // key resolves against the entire chain built so far; the result's value
    // type becomes the string type, so call this as the last resolver step
    // before a terminal.
    D_NODISCARD
    interpolation<Type, recursive_resolver<Type, Resolver, Scanner>, Scanner>
    recursive(
        std::size_t _max_depth = 16
    ) const
    {
        using recursive_t = recursive_resolver<Type, Resolver, Scanner>;

        return interpolation<Type, recursive_t, Scanner>(
            m_template,
            recursive_t(m_resolver, _max_depth));
    }

    // str -- force the single pass, returning a freshly allocated string
    D_NODISCARD string_type
    str() const
    {
        string_type _out;
        _out.reserve(m_template.size());
        interp_string_sink<Type> _sink(_out);
        interpolate_into(_sink, Scanner(m_template), m_resolver);

        return _out;
    }

    // into -- force the single pass into a caller-supplied sink (no allocation)
    template<typename Sink>
    void
    into(
        Sink& _sink
    ) const
    {
        interpolate_into(_sink, Scanner(m_template), m_resolver);

        return;
    }

    // prepare -- pre-parse the template ONCE and hand back a prepared
    // interpolation that replays the cached pieces, so repeated renders skip
    // the scan (section VIII).  Carries the current resolver chain across; the
    // shared cache makes the result cheap to copy.  The cache holds views into
    // the template, so the template must outlive the result.
    D_NODISCARD
    prepared_interpolation<Type, Resolver, Scanner, std::vector<piece<Type>>>
    prepare() const
    {
        using cache_type = std::vector<piece<Type>>;

        auto         _cache = std::make_shared<cache_type>();
        Scanner      _scanner(m_template);
        piece<Type> _p;
        while (_scanner.next(_p)) { _cache->push_back(_p); }

        return prepared_interpolation<Type, Resolver, Scanner, cache_type>(
            std::shared_ptr<const cache_type>(_cache),
            m_resolver,
            m_template);
    }

    // the functor / value face: render on conversion to a string
    D_NODISCARD operator string_type() const
    {
        return str();
    }

    // template_view -- the bound format
    D_NODISCARD D_CONSTEXPR view_type
    template_view() const
    {
        return m_template;
    }

    // resolver -- the bound resolver chain
    D_NODISCARD D_CONSTEXPR const Resolver&
    resolver() const
    {
        return m_resolver;
    }

private:
    view_type m_template;
    Resolver m_resolver;
};


// interpolate -- seed an interpolation over a template with no bindings yet
// (the identity; chain frames onto it with `.interpolate(...)`).  The scanner
// defaults to brace_scanner; pass another for a different placeholder syntax.
template<typename Type     = char,
         typename Scanner = brace_scanner<Type>>
D_NODISCARD D_CONSTEXPR
interpolation<Type, empty_resolver<Type>, Scanner>
interpolate(
    std::basic_string_view<Type> _template
)
{
    return interpolation<Type, empty_resolver<Type>, Scanner>(
        _template, empty_resolver<Type>{});
}

// make_interpolation -- seed an interpolation with an initial resolver bound
template<typename Type,
         typename Resolver,
         typename Scanner = brace_scanner<Type>>
D_NODISCARD D_CONSTEXPR
interpolation<Type, clean_t<Resolver>, Scanner>
make_interpolation(
    std::basic_string_view<Type> _template,
    Resolver&&                   _resolver
)
{
    return interpolation<Type, clean_t<Resolver>, Scanner>(
        _template, static_cast<Resolver&&>(_resolver));
}


// ===========================================================================
// VIII. PREPARED TEMPLATES
// ===========================================================================
//   `interpolation` re-scans the template on every render.  When the SAME
// template is rendered many times -- the common templating workload -- that
// scan is repeated work, because the piece sequence depends only on the
// template and the scanner, never on the data.  `prepared_interpolation` scans
// ONCE into a shared piece cache and replays it (replay_scanner, section II) on
// each render: same fold, same resolvers, no re-scan.  It mirrors the live
// builder's surface (`.interpolate`, `.interpolate_if`, `.recursive`, `.str`,
// `.into`), and adds resolver-argument terminals for the hot path -- hand the
// per-render bindings straight to a terminal and the prepared object stays put,
// the shared cache reused in place with nothing re-parsed.
//
//   The cache is shared (std::shared_ptr<const Cache>), so threading it through
// the fluent chain copies only a refcount, never the pieces.  Cache defaults to
// std::vector<piece<Type>> but is a template knob; for full control of storage
// and lifetime, fill your own container with prepare_into and render it through
// a replay_scanner.  In every case the pieces are views into the template, so
// the template must outlive the cache.

// prepared_interpolation
//   class: a template + resolver pair backed by a pre-scanned, shared piece
// cache -- the pre-parsed counterpart of interpolation.  Built by prepare,
// make_prepared, or interpolation::prepare.
template<typename Type,
         typename Resolver,
         typename Scanner,
         typename Cache>
class prepared_interpolation
{
public:
    using char_type     = Type;
    using view_type     = std::basic_string_view<Type>;
    using string_type   = std::basic_string<Type>;
    using scanner_type  = Scanner;
    using resolver_type = Resolver;
    using cache_type    = Cache;
    using cache_pointer = std::shared_ptr<const Cache>;

    prepared_interpolation(
        cache_pointer _cache,
        Resolver      _resolver,
        view_type     _template
    )
        : m_cache(_cache),
          m_resolver(_resolver),
          m_template(_template)
    {}

    // -- resolver building: each returns a prepared interpolation SHARING the
    //    cache (a refcount bump, never a piece copy) --

    // interpolate -- append a resolver frame (lazy; folds in on a terminal)
    template<typename R2>
    D_NODISCARD
    prepared_interpolation<Type, chain_resolver<Resolver, clean_t<R2>>, Scanner, Cache>
    interpolate(
        R2&& _r2
    ) const
    {
        using chained = chain_resolver<Resolver, clean_t<R2>>;

        return prepared_interpolation<Type, chained, Scanner, Cache>(
            m_cache,
            chained(m_resolver, static_cast<R2&&>(_r2)),
            m_template);
    }

    // interpolate -- inline-bindings convenience over the resolver form
    D_NODISCARD
    prepared_interpolation<Type, chain_resolver<Resolver, map_resolver<Type>>, Scanner, Cache>
    interpolate(
        std::initializer_list<std::pair<view_type, view_type>> _list
    ) const
    {
        return interpolate(map_resolver<Type>(_list));
    }

    // interpolate_if -- append a predicate-gated frame.  For more than one
    // condition compose the leaves with predicate.hpp and pass one predicate.
    template<typename Pred,
             typename R2>
    D_NODISCARD auto
    interpolate_if(
        Pred&& _pred,
        R2&&   _r2
    ) const
    {
        return interpolate(when(static_cast<Pred&&>(_pred),
                                static_cast<R2&&>(_r2)));
    }

    // recursive -- expand resolved VALUES as templates (nested templates).  The
    // cached pieces are the OUTER scan; nested value scans still run live (they
    // are data, not cacheable) using Scanner.
    D_NODISCARD
    prepared_interpolation<Type, recursive_resolver<Type, Resolver, Scanner>, Scanner, Cache>
    recursive(
        std::size_t _max_depth = 16
    ) const
    {
        using recursive_t = recursive_resolver<Type, Resolver, Scanner>;

        return prepared_interpolation<Type, recursive_t, Scanner, Cache>(
            m_cache,
            recursive_t(m_resolver, _max_depth),
            m_template);
    }

    // -- terminals: replay the cache, never re-scan --

    // str -- render with the bound resolver chain
    D_NODISCARD string_type
    str() const
    {
        string_type _out;
        _out.reserve(m_template.size());
        interp_string_sink<Type> _sink(_out);
        interpolate_into(_sink, replay_scanner<Type, Cache>(*m_cache), m_resolver);

        return _out;
    }

    // into -- render with the bound resolver chain into a caller sink
    template<typename Sink>
    void
    into(
        Sink& _sink
    ) const
    {
        interpolate_into(_sink, replay_scanner<Type, Cache>(*m_cache), m_resolver);

        return;
    }

    // -- hot-path terminals: supply the resolver per render.  The cache is
    //    reused in place and the bound resolver is bypassed (it is the identity
    //    for a freshly prepared template), so reach for these when each render
    //    carries its own complete bindings --

    // str -- render with a supplied resolver
    template<typename R2>
    D_NODISCARD string_type
    str(
        const R2& _resolver
    ) const
    {
        string_type _out;
        _out.reserve(m_template.size());
        interp_string_sink<Type> _sink(_out);
        interpolate_into(_sink, replay_scanner<Type, Cache>(*m_cache), _resolver);

        return _out;
    }

    // into -- render with a supplied resolver into a caller sink
    template<typename Sink,
             typename R2>
    void
    into(
        Sink&     _sink,
        const R2& _resolver
    ) const
    {
        interpolate_into(_sink, replay_scanner<Type, Cache>(*m_cache), _resolver);

        return;
    }

    // the functor / value face: render on conversion to a string
    D_NODISCARD operator string_type() const
    {
        return str();
    }

    // template_view -- the bound format
    D_NODISCARD D_CONSTEXPR view_type
    template_view() const
    {
        return m_template;
    }

    // resolver -- the bound resolver chain
    D_NODISCARD D_CONSTEXPR const Resolver&
    resolver() const
    {
        return m_resolver;
    }

    // pieces -- the cached scan (one entry per literal run or placeholder)
    D_NODISCARD const Cache&
    pieces() const
    {
        return *m_cache;
    }

private:
    cache_pointer m_cache;
    Resolver      m_resolver;
    view_type     m_template;
};


// prepare -- scan a template ONCE with Scanner and hand back a prepared
// interpolation (resolver still empty; chain frames with `.interpolate(...)` or
// supply them per render at a terminal).  The cache is shared, so the result is
// cheap to copy.  Cache defaults to std::vector<piece<Type>>.  The cache holds
// views into _template, so _template must outlive the result.
template<typename Type     = char,
         typename Scanner = brace_scanner<Type>,
         typename Cache    = std::vector<piece<Type>>>
D_NODISCARD
prepared_interpolation<Type, empty_resolver<Type>, Scanner, Cache>
prepare(
    std::basic_string_view<Type> _template
)
{
    auto         _cache = std::make_shared<Cache>();
    Scanner      _scanner(_template);
    piece<Type> _p;
    while (_scanner.next(_p)) { _cache->push_back(_p); }

    return prepared_interpolation<Type, empty_resolver<Type>, Scanner, Cache>(
        std::shared_ptr<const Cache>(_cache),
        empty_resolver<Type>{},
        _template);
}

// make_prepared -- prepare a template with an initial resolver already bound
template<typename Type,
         typename Resolver,
         typename Scanner = brace_scanner<Type>,
         typename Cache    = std::vector<piece<Type>>>
D_NODISCARD
prepared_interpolation<Type, clean_t<Resolver>, Scanner, Cache>
make_prepared(
    std::basic_string_view<Type> _template,
    Resolver&&                   _resolver
)
{
    auto         _cache = std::make_shared<Cache>();
    Scanner      _scanner(_template);
    piece<Type> _p;
    while (_scanner.next(_p)) { _cache->push_back(_p); }

    return prepared_interpolation<Type, clean_t<Resolver>, Scanner, Cache>(
        std::shared_ptr<const Cache>(_cache),
        static_cast<Resolver&&>(_resolver),
        _template);
}

// prepare_into -- fill a CALLER-OWNED container with the scanned pieces, for
// full control of storage and lifetime.  Render it through a replay_scanner over
// the same container, e.g.
//   interpolate_into(sink, replay_scanner<char>(cache), resolver);
// Cache is any back-insertable sequence of piece<Type>; the pieces are views
// into _template, so it must outlive the container.
template<typename Type     = char,
         typename Scanner = brace_scanner<Type>,
         typename Cache>
void
prepare_into(
    Cache&                       _cache,
    std::basic_string_view<Type> _template
)
{
    Scanner      _scanner(_template);
    piece<Type> _p;
    while (_scanner.next(_p)) { _cache.push_back(_p); }

    return;
}


NS_END  // djinterp


// ===========================================================================
// IX.  CONCEPTS  (C++20)
// ===========================================================================
//   Concept parallels of the section-II/III/IV contracts, following the
// trait-triple convention: a scanner yields pieces, a resolver answers keys
// with a found/value, a sink accepts literal/value.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_DJINTERP

// scanner_for
//   concept: Scanner is a pull cursor over Type -- it exposes piece_type and
// `next(piece_type&) -> bool`.
template<typename Scanner,
         typename Type = char>
concept scanner_for = requires(Scanner _s, typename Scanner::piece_type& _p)
{
    typename Scanner::piece_type;
    { _s.next(_p) } -> std::convertible_to<bool>;
};

// resolver_for
//   concept: Resolver answers a key view with something exposing found() and
// value().
template<typename Resolver,
         typename Type = char>
concept resolver_for =
    requires(Resolver _r, std::basic_string_view<Type> _key)
    {
        { _r(_key).found() } -> std::convertible_to<bool>;
        _r(_key).value();
    };

// sink_for
//   concept: Sink accepts a literal run and a resolved value.
template<typename Sink,
         typename Type = char>
concept sink_for = requires(Sink _s, std::basic_string_view<Type> _span)
{
    _s.literal(_span);
    _s.value(_span);
};

NS_END  // djinterp

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_INTERPOLATE_HPP
