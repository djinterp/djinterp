/******************************************************************************
* djinterp [cli]                                                       cli.hpp
*
*   Foundational CLI vocabulary.  The greatest common subset of types,
* enums, validation routines, and runtime interfaces shared by every
* CLI-touching module in djinterp:
*
*     - cli_kind / cli_arity / cli_form      : enums (constexpr-friendly)
*     - cli_name<fixed_string>               : validated long-form NTTP
*     - cli_short<char>                      : validated short-form NTTP
*     - cli_no_short                         : sentinel for "no short form"
*     - is_valid_cli_name / _short           : constexpr validation
*     - cli_descriptor                       : abstract runtime base class
*     - cli_descriptor_view                  : trivial metadata snapshot
*
*   This header is intentionally CLI-only.  It has ZERO dependency on
* option<>, option_set<>, or any binding / parser module.  Bridging the
* option system to the CLI surface lives in cli_bridge.hpp.
*
*   Detection traits (is_cli_name_v, etc.) live in cli_traits.hpp.
*   C++20 concept analogs live in cli_concepts.hpp.
*
*   CONSTEXPR / RUNTIME DUALITY
*
*   Every value type here is constexpr-pure: NTTPs, plain enums, and a
* trivially-copyable view aggregate.  Used as types they are pure
* compile-time vocabulary; used as values they cost nothing - no vtable,
* no heap, no indirection.
*
*   Virtual polymorphism appears in exactly one place, cli_descriptor,
* and only because it earns its keep: runtime composition needs
* heterogeneous containers (std::vector<const cli_descriptor*>) that
* dispatch over descriptors built from differently-typed compile-time
* sources.  For hot paths that only read metadata, cli_descriptor_view
* is the value-semantic snapshot that drops the indirection.
*
*   Behavior-bearing virtual layers (value parsing, default formatting,
* etc.) belong in derived headers (cli_bridge.hpp, cli_parser.hpp) as
* abstract subclasses of cli_descriptor - not here.
*
*
* path:      /inc/djinterp/core/cli/cli.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cli_kind / cli_arity / cli_form        (semantic / arity / surface)
II.   validation helpers                     (constexpr predicates)
III.  cli_name<fixed_string>                 (validated long-form)
IV.   cli_short<char> + cli_no_short         (validated short-form)
V.    cli_descriptor                         (abstract runtime base)
VI.   cli_descriptor_view + make_view        (trivial metadata snapshot)
*/

#ifndef DJINTERP_CLI_
#define DJINTERP_CLI_ 1

// std
#include <cstddef>
#include <string_view>
// djinterp
#include "../djinterp.hpp"
#include "../meta/fixed_string.hpp"


NS_DJINTERP


// ===========================================================================
// I.   cli_kind / cli_arity / cli_form
// ===========================================================================

// cli_kind
//   enum: the SEMANTIC class of a CLI entity, independent of its
// surface form in argv.  Consumed by the bridge to characterize each
// bound option, and by the parser to decide whether a following token
// should be consumed as a value.
enum class cli_kind : unsigned char
{
    flag,         // boolean presence (--verbose)
    value,        // single-value option (--output FILE)
    multi_value,  // list-value option   (--include DIR --include DIR)
    positional,   // positional argument (no prefix)
    subcommand    // subcommand token    (git COMMIT)
};

// cli_arity
//   enum: how many values an entity consumes.  Orthogonal to
// cli_kind so the parser can express e.g. "value option with
// optional value" (--threads vs --threads=8) cleanly.
enum class cli_arity : unsigned char
{
    none,         // no value (flag)
    one,          // exactly one
    optional,     // zero or one
    many,         // one or more
    any           // zero or more
};

// cli_form
//   enum: the SURFACE form of a CLI token, orthogonal to cli_kind.
// Used by the parser to classify raw argv tokens before semantic
// resolution.  The `positional` member coincides with cli_kind's
// positional - the two concepts collapse only at that one point.
enum class cli_form : unsigned char
{
    short_form,   // -x        (single-char with single dash)
    long_form,    // --xyz     (multi-char with double dash)
    positional,   // xyz       (no prefix)
    terminator    // --        (end-of-options sentinel)
};


// ===========================================================================
// II.  validation helpers
// ===========================================================================

NS_INTERNAL

    // ascii_is_alpha
    //   helper: lowercase or uppercase ASCII letter.  Locale-
    // independent on purpose - CLI names should not depend on
    // the user's current locale.
    constexpr bool
    ascii_is_alpha(char _c) noexcept
    {
        return ( ((_c >= 'a') && (_c <= 'z')) ||
                 ((_c >= 'A') && (_c <= 'Z')) );
    }

    // ascii_is_alnum
    //   helper: ASCII letter or digit.  Same locale-independence
    // rationale as ascii_is_alpha.
    constexpr bool
    ascii_is_alnum(char _c) noexcept
    {
        return ( ascii_is_alpha(_c) ||
                 ((_c >= '0') && (_c <= '9')) );
    }

NS_END  // internal


// is_valid_cli_name_char
//   trait: a char is permissible inside a long-form CLI name iff
// it is alphanumeric, '-', or '_'.  The leading character has a
// stricter rule (see is_valid_cli_name) and is not covered here.
constexpr bool
is_valid_cli_name_char(char _c) noexcept
{
    return ( internal::ascii_is_alnum(_c) ||
             (_c == '-')                  ||
             (_c == '_') );
}

// is_valid_cli_name
//   trait: a long-form CLI name is non-empty, begins with a
// letter, and contains only valid name characters thereafter.
// The terminating null is not inspected.
//
//   Canonical rule enforced by cli_name<> at compile time and
// reused by the bridge / parser when validating dynamically-
// supplied names.
template<std::size_t _N>
constexpr bool
is_valid_cli_name(fixed_string<_N> _s) noexcept
{
    if (_s.size() == 0)
    {
        return false;
    }

    if (!internal::ascii_is_alpha(_s.data[0]))
    {
        return false;
    }

    // walk remaining chars
    for (std::size_t i = 1; i < _s.size(); ++i)
    {
        if (!is_valid_cli_name_char(_s.data[i]))
        {
            return false;
        }
    }

    return true;
}

// is_valid_cli_short
//   trait: a short-form CLI flag character must be alphanumeric.
// '\0' is reserved as the "no short form" sentinel (see
// cli_no_short) and is rejected here.
constexpr bool
is_valid_cli_short(char _c) noexcept
{
    return internal::ascii_is_alnum(_c);
}


// ===========================================================================
// III. cli_name<fixed_string>
// ===========================================================================

// cli_name
//   type: validated long-form CLI name carrier.  Wraps a
// fixed_string NTTP and enforces the naming rules at compile
// time via static_assert.  Exposes both compile-time (`::value`,
// `::size`) and runtime (`view()`) access.
//
// Example:
//   using verbose_name = cli_name<"verbose">;
//   constexpr std::size_t      n  = verbose_name::size;   // 7
//   constexpr std::string_view sv = verbose_name::view(); // "verbose"
template<fixed_string _S>
struct cli_name
{
    static_assert(is_valid_cli_name(_S),
                  "cli_name: invalid CLI name.  Must be non-empty, "
                  "begin with an ASCII letter, and contain only "
                  "alphanumerics, '-', or '_'.");

    using carrier_type = decltype(_S);

    static constexpr auto        value = _S;
    static constexpr std::size_t size  = _S.size();

    // view
    //   accessor: runtime-friendly string_view over the payload.
    static constexpr std::string_view
    view() noexcept
    {
        return _S.view();
    }
};


// ===========================================================================
// IV.  cli_short<char> + cli_no_short
// ===========================================================================

// cli_no_short
//   value: sentinel returned by runtime descriptors that have no
// short form.  '\0' is chosen because it cannot appear in any
// valid cli_short<> (rejected by is_valid_cli_short).
inline constexpr char cli_no_short = '\0';

// cli_short
//   type: validated short-form CLI flag carrier.  Single char
// NTTP, validated alphanumeric.  Where "no short form" is
// desired, omit cli_short entirely and let the descriptor's
// short_form() accessor return cli_no_short.
template<char _C>
struct cli_short
{
    static_assert(is_valid_cli_short(_C),
                  "cli_short: short-form character must be ASCII "
                  "alphanumeric.");

    static constexpr char value = _C;
};


// ===========================================================================
// V.   cli_descriptor
// ===========================================================================

// cli_descriptor
//   class: abstract runtime base for any CLI-described entity.
// Provides a polymorphic view over the metadata that every CLI
// surface needs, regardless of where the underlying type lives
// (binding, bridge-materialized record, parser-constructed entry).
//
//   Metadata-only base.  Derived modules layer behavior-bearing
// subclasses on top (e.g. cli_value_descriptor with parse_value /
// format_default), keeping this base cheap so heterogeneous
// containers of the form
//     std::vector<const cli_descriptor*>
// remain efficient to walk when only metadata is needed.
//
//   For hot paths that need neither behavior nor identity-
// stable references, prefer cli_descriptor_view - it captures
// the same metadata as a trivially-copyable value type, with
// no virtual dispatch.
struct cli_descriptor
{
    virtual ~cli_descriptor() = default;

    // name
    //   accessor: long-form CLI name (without any leading dashes).
    // Never empty for a well-formed descriptor.
    virtual std::string_view name() const noexcept = 0;

    // short_form
    //   accessor: short-form character, or cli_no_short (== '\0')
    // if this entity has no short form.
    virtual char short_form() const noexcept = 0;

    // kind
    //   accessor: semantic class of the entity (see cli_kind).
    virtual cli_kind kind() const noexcept = 0;

    // arity
    //   accessor: how many values this entity consumes.
    virtual cli_arity arity() const noexcept = 0;

    // help
    //   accessor: short human-readable description for --help.
    // Empty view if none was provided.
    virtual std::string_view help() const noexcept = 0;

    // hidden
    //   accessor: true iff this entity should be omitted from
    // --help / usage output.
    virtual bool hidden() const noexcept = 0;
};


// ===========================================================================
// VI.  cli_descriptor_view + make_view
// ===========================================================================

// cli_descriptor_view
//   type: trivial value-semantic snapshot of a descriptor's
// metadata.  Use when you want to pass descriptor information
// around without paying for virtual dispatch, or to materialize
// a descriptor view directly from compile-time data.
//
//   Non-owning: the string_views reference storage that must
// outlive the view (typically static / fixed_string payloads,
// which live forever).
struct cli_descriptor_view
{
    std::string_view name;
    std::string_view help;
    char             short_form;
    cli_kind         kind;
    cli_arity        arity;
    bool             hidden;
};

// make_view
//   helper: snapshot a polymorphic cli_descriptor into a value-
// semantic view.  Cheap; performs six virtual calls once at
// construction, then the resulting view is dispatch-free.
inline cli_descriptor_view
make_view(const cli_descriptor& _d) noexcept
{
    return cli_descriptor_view{
        _d.name(),
        _d.help(),
        _d.short_form(),
        _d.kind(),
        _d.arity(),
        _d.hidden()
    };
}


NS_END  // djinterp


#endif  // DJINTERP_CLI_
