/*******************************************************************************
* djinterp [core]                                                 cli_string.hpp
*
*   A fixed-capacity compile-time string and the small constexpr string
* algebra the CLI layer renders into.  `dji_cli_string<Cap>` is a literal,
* structural class type: it is therefore usable as a C++20 non-type template
* parameter (so a literal or an identifier can be carried as a type), and -
* crucially - it is ONE type irrespective of its contents, so it may also be
* an option<> key (option_set requires a single key_type across a set, which a
* length-indexed string could never satisfy).
*
*   THE TWO STRING ROLES, AND WHY BOTH TYPES ARE PRESENT:
*     - fixed_string<N> (meta/fixed_string.hpp) is LENGTH-indexed: a distinct
*       type per content length.  It is the AUTHORING-BOUNDARY carrier - every
*       literal/identifier in a hand-written template is a `fixed_string` NTTP
*       (that is what makes `lit<"hi">` deduce), but a different type per length
*       cannot be a uniform option_set key nor hold a data-dependent-length
*       result.
*     - dji_cli_string<Cap> (here) is CAPACITY-indexed: one type for every
*       value.  It is the INTERNAL/key/algebra carrier - the option_set key
*       type and the result type of every operation below.
*   The bridge is the converting constructor `dji_cli_string(fixed_string<N>)`:
* an authored `fixed_string` is lifted into this capacity buffer exactly once,
* at the boundary, after which the engine and the environment keys stay uniform.
*
*   The capacity is a knob, not a contract: every `dji_cli_string<Cap>`
* occupies `Cap` bytes in the type system regardless of how many are used, and
* all unused tail bytes are value-initialized to '\0'.  That tail-zeroing is
* what makes NTTP identity behave: two strings spelling the same text are the
* same template argument because their member arrays compare equal byte-for-
* byte.  Build results only ever start from a zero-initialized buffer and write
* exactly `size` bytes, so the invariant is preserved by construction.
*
*   The constexpr operations here (concat / find / replace / case / indent) are
* the leaf and structural operations the compile-time evaluator composes; the
* `dji_cli_markov` driver is the normal-Markov-algorithm fixpoint that backs
* the regex/rewrite node-kind (with literal patterns at compile time - see
* dj_cli_render.hpp).  Everything is total: `dji_cli_markov` takes an explicit
* step budget so a divergent rule system terminates rather than exhausting the
* constant-evaluation step limit.
*
*   Requires C++20 (class-type / string non-type template arguments); the
* header self-suppresses below it, mirroring template.hpp and text_template.hpp.
*
*
* path:      /inc/djinterp/core/cli/cli_string.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    dji_cli_string             (fixed-capacity compile-time string)
      ---------------------------------------------------------------

II.   structural operations      (concat, find, replace_first, substr)
      ----------------------------------------------------------------

III.  formatting operations      (to_upper, to_lower, indent, truthy)
      ---------------------------------------------------------------

IV.   rewrite_rule + markov      (normal Markov algorithm to fixpoint)
      ----------------------------------------------------------------
*/

#ifndef DJINTERP_CLI_CLI_STRING_HPP
#define DJINTERP_CLI_CLI_STRING_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <array>
#include <cstddef>
// djinterp
#include "../../djinterp.hpp"          // NS_*, D_CONSTEXPR, D_NODISCARD
#include "../meta/fixed_string.hpp" // fixed_string<> (authoring-boundary NTTP)


// String non-type template parameters are the spine of this module; below
// C++20 it contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS


NS_DJINTERP


// ===========================================================================
// I.   dji_cli_string
// ===========================================================================

// dji_cli_string
//   class: a fixed-capacity (Cap) compile-time string.  A structural literal
// type - all members public and non-mutable - hence a valid C++20 non-type
// template parameter, and a single type for every value (so it can key an
// option_set).  The tail past `size` is always '\0', which gives well-defined
// NTTP identity: equal text => equal member array => equal template argument.
template<std::size_t Cap = 256>
struct dji_cli_string
{
    char        data[Cap] {};
    std::size_t size = 0;

    // empty string
    constexpr dji_cli_string() = default;

    // implicit lift from a string literal (drops the terminating '\0')
    template<std::size_t N>
    constexpr dji_cli_string(
        const char (&_lit)[N]
    )
    {
        std::size_t n = (N > 0) ? (N - 1) : 0;

        // never write past the fixed capacity
        if (n > Cap)
        {
            n = Cap;
        }

        for (std::size_t i = 0; i < n; ++i)
        {
            data[i] = _lit[i];
        }

        size = n;
    }

    // lift from a fixed_string authoring carrier (the authoring-boundary
    // bridge).  A literal/identifier is written as a `fixed_string<N>` NTTP
    // and converted into this capacity buffer once, here; `data` keeps its
    // zero-initialized tail, so the NTTP-identity invariant holds.  Distinct
    // parameter type from the literal constructor above, so the two never
    // collide.
    template<std::size_t N>
    constexpr dji_cli_string(
        const fixed_string<N>& _fs
    )
    {
        std::size_t n = _fs.size();

        // never write past the fixed capacity
        if (n > Cap)
        {
            n = Cap;
        }

        for (std::size_t i = 0; i < n; ++i)
        {
            data[i] = _fs.data[i];
        }

        size = n;
    }

    D_NODISCARD constexpr char
    operator[](
        std::size_t _i
    ) const
    {
        return data[_i];
    }

    D_NODISCARD constexpr bool
    empty() const
    {
        return size == 0;
    }

    // content equality (compares only the live prefix; tail is '\0' by
    // invariant, so this agrees with NTTP identity).
    D_NODISCARD constexpr bool
    operator==(
        const dji_cli_string& _other
    ) const
    {
        if (size != _other.size)
        {
            return false;
        }

        for (std::size_t i = 0; i < size; ++i)
        {
            if (data[i] != _other.data[i])
            {
                return false;
            }
        }

        return true;
    }

    D_NODISCARD constexpr bool
    operator!=(
        const dji_cli_string& _other
    ) const
    {
        return !(*this == _other);
    }
};


// ===========================================================================
// II.  structural operations
// ===========================================================================

// dji_cli_concat
//   function: the concatenation _a ++ _b, truncated at the capacity Cap.
// Builds into a fresh zero-initialized buffer, preserving the tail invariant.
template<std::size_t Cap>
D_NODISCARD D_CONSTEXPR dji_cli_string<Cap>
dji_cli_concat(
    const dji_cli_string<Cap>& _a,
    const dji_cli_string<Cap>& _b
)
{
    dji_cli_string<Cap> result;
    std::size_t          i = 0;

    for (; i < _a.size && i < Cap; ++i)
    {
        result.data[i] = _a.data[i];
    }

    std::size_t j = 0;

    for (; j < _b.size && (i + j) < Cap; ++j)
    {
        result.data[i + j] = _b.data[j];
    }

    result.size = (i + j);

    return result;
}

// dji_cli_find
//   function: index of the first occurrence of _pattern in _text at or after
// _from, or _text.size when absent.  The empty pattern matches at _from.
template<std::size_t Cap>
D_NODISCARD D_CONSTEXPR std::size_t
dji_cli_find(
    const dji_cli_string<Cap>& _text,
    const dji_cli_string<Cap>& _pattern,
    std::size_t                 _from
)
{
    if (_pattern.size == 0)
    {
        return (_from <= _text.size) ? _from : _text.size;
    }

    if (_pattern.size > _text.size)
    {
        return _text.size;
    }

    for (std::size_t i = _from; (i + _pattern.size) <= _text.size; ++i)
    {
        bool hit = true;

        for (std::size_t k = 0; k < _pattern.size; ++k)
        {
            if (_text.data[i + k] != _pattern.data[k])
            {
                hit = false;
                break;
            }
        }

        if (hit)
        {
            return i;
        }
    }

    return _text.size;
}

// dji_cli_replace_first
//   function: replace the first occurrence of _pattern in _text (in place)
// with _replacement; returns whether a replacement occurred.  A non-match or
// an empty pattern is a no-op returning false.
template<std::size_t Cap>
D_CONSTEXPR bool
dji_cli_replace_first(
    dji_cli_string<Cap>&       _text,
    const dji_cli_string<Cap>& _pattern,
    const dji_cli_string<Cap>& _replacement
)
{
    std::size_t at = dji_cli_find(_text, _pattern, 0);

    if (at == _text.size || _pattern.size == 0)
    {
        return false;
    }

    dji_cli_string<Cap> out;
    std::size_t          w = 0;

    // prefix before the match
    for (std::size_t i = 0; i < at; ++i)
    {
        out.data[w++] = _text.data[i];
    }

    // the replacement
    for (std::size_t i = 0; i < _replacement.size && w < Cap; ++i)
    {
        out.data[w++] = _replacement.data[i];
    }

    // suffix after the match
    for (std::size_t i = (at + _pattern.size); i < _text.size && w < Cap; ++i)
    {
        out.data[w++] = _text.data[i];
    }

    out.size = w;
    _text    = out;

    return true;
}

// dji_cli_substr
//   function: the substring of _text of length _len beginning at _pos
// (clamped to the available range).
template<std::size_t Cap>
D_NODISCARD D_CONSTEXPR dji_cli_string<Cap>
dji_cli_substr(
    const dji_cli_string<Cap>& _text,
    std::size_t                 _pos,
    std::size_t                 _len
)
{
    dji_cli_string<Cap> out;

    if (_pos >= _text.size)
    {
        return out;
    }

    std::size_t n = _text.size - _pos;

    if (_len < n)
    {
        n = _len;
    }

    for (std::size_t i = 0; i < n; ++i)
    {
        out.data[i] = _text.data[_pos + i];
    }

    out.size = n;

    return out;
}


// ===========================================================================
// III. formatting operations
// ===========================================================================

// dji_cli_to_upper
//   function: ASCII upper-casing of _text.
template<std::size_t Cap>
D_NODISCARD D_CONSTEXPR dji_cli_string<Cap>
dji_cli_to_upper(
    dji_cli_string<Cap> _text
)
{
    for (std::size_t i = 0; i < _text.size; ++i)
    {
        char c = _text.data[i];

        if (c >= 'a' && c <= 'z')
        {
            _text.data[i] = static_cast<char>(c - 32);
        }
    }

    return _text;
}

// dji_cli_to_lower
//   function: ASCII lower-casing of _text.
template<std::size_t Cap>
D_NODISCARD D_CONSTEXPR dji_cli_string<Cap>
dji_cli_to_lower(
    dji_cli_string<Cap> _text
)
{
    for (std::size_t i = 0; i < _text.size; ++i)
    {
        char c = _text.data[i];

        if (c >= 'A' && c <= 'Z')
        {
            _text.data[i] = static_cast<char>(c + 32);
        }
    }

    return _text;
}

// dji_cli_indent
//   function: prefix every line of _text with _width spaces (the start of the
// string and the character after every '\n' begin a line).
template<std::size_t Cap>
D_NODISCARD D_CONSTEXPR dji_cli_string<Cap>
dji_cli_indent(
    const dji_cli_string<Cap>& _text,
    std::size_t                 _width
)
{
    dji_cli_string<Cap> out;
    std::size_t          w        = 0;
    bool                 line_top = true;

    for (std::size_t i = 0; i < _text.size && w < Cap; ++i)
    {
        // emit the indent at the head of each line
        if (line_top)
        {
            for (std::size_t s = 0; s < _width && w < Cap; ++s)
            {
                out.data[w++] = ' ';
            }

            line_top = false;
        }

        char c        = _text.data[i];
        out.data[w++] = c;

        if (c == '\n')
        {
            line_top = true;
        }
    }

    out.size = w;

    return out;
}

// dji_cli_truthy
//   function: the CLI layer's notion of truth for if_ - a string is true
// unless it is empty, "false", or "0".
template<std::size_t Cap>
D_NODISCARD D_CONSTEXPR bool
dji_cli_truthy(
    const dji_cli_string<Cap>& _text
)
{
    if (_text.size == 0)
    {
        return false;
    }

    if ( (_text == dji_cli_string<Cap>("false")) ||
         (_text == dji_cli_string<Cap>("0")) )
    {
        return false;
    }

    return true;
}


// ===========================================================================
// IV.  dji_cli_rewrite_rule + dji_cli_markov
// ===========================================================================

// dji_cli_rewrite_rule
//   struct: one ordered production of a normal Markov algorithm - replace the
// first occurrence of `pattern` with `replacement`; if `terminal`, halt the
// whole system after firing.
template<std::size_t Cap = 256>
struct dji_cli_rewrite_rule
{
    dji_cli_string<Cap> pattern;
    dji_cli_string<Cap> replacement;
    bool                 terminal = false;
};

// dji_cli_markov
//   function: run an ordered rule system over _text to a fixpoint, normal-
// algorithm style: scan rules top to bottom, apply the first whose pattern
// occurs (leftmost occurrence), then restart from the top; stop when a
// terminal rule fires, when no rule matches, or when the step _budget is
// spent.  The budget makes the operation total even for divergent systems -
// essential under constant evaluation, where non-termination is a hard error.
template<std::size_t Cap,
         std::size_t R>
D_NODISCARD D_CONSTEXPR dji_cli_string<Cap>
dji_cli_markov(
    dji_cli_string<Cap>                              _text,
    const std::array<dji_cli_rewrite_rule<Cap>, R>& _rules,
    std::size_t                                       _budget
)
{
    while (_budget-- > 0)
    {
        bool fired = false;

        // first matching rule, top to bottom
        for (std::size_t i = 0; i < R; ++i)
        {
            if (dji_cli_replace_first(
                    _text, _rules[i].pattern, _rules[i].replacement))
            {
                fired = true;

                // a terminal production halts the entire system
                if (_rules[i].terminal)
                {
                    return _text;
                }

                break;      // otherwise restart the scan from the top
            }
        }

        if (!fired)
        {
            break;
        }
    }

    return _text;
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER ...

#endif  // floor, for now


#endif  // DJINTERP_CLI_CLI_STRING_HPP
