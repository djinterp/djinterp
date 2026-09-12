/******************************************************************************
* djinterp [parse]                                              parse_context.hpp
*
* Parse session bundle.
*   `parse_context` is the per-session bundle threaded through a
* parser invocation.  It owns the output (a `symbol_tree`), the
* string table the symbol payload's *_id fields index into, the
* diagnostics accumulated during the parse, and the running parse
* statistics.  Multiple parsers — libclang frontends, cpp_scanner,
* and any future parser writing into the symbol model — share one
* context for a session so the symbol tree and string table stay
* coherent and diagnostics aggregate in one place.
*
*   The context is *not* a parser_expr.  It does not appear in the
* parse function `P A = Σ* → maybe⟨A × Σ*⟩` — the formal carrier is
* unchanged.  The context is the *side-effecting* face of a parse:
* writes to the symbol_tree, reads/writes to the string_table, and
* diagnostics are the I/O surface that lives outside the pure
* parser_expr CRTP composition.  Pure parsing (parser combinators
* over parse_state<E>) does not touch parse_context; the boundary
* is the frontend / scanner that knows which symbol_data to emit
* on a successful parse.
*
*   STRING TABLE.  The `symbol_data` payload (symbol_model.hpp)
* carries uint32_t `*_id` fields — `name_id`, `type_id`,
* `signature_id`, `comment_id` — that index into a string table.
* The exact string_table type is project-specific and not visible
* from here, so parse_context is templated on the string-table
* type with the convention that the table exposes
*
*       std::uint32_t intern(const std::string& s);
*       const std::string& lookup(std::uint32_t id) const;
*
* — anything providing those two members works.  Callers supplying
* a project string_table get a strongly-typed context; callers who
* don't can default to the supplied identity stub.
*
*   DIAGNOSTICS.  `parse_error` from parse.hpp is the diagnostic
* type.  The context owns a vector of them; record_error appends
* and clear_errors resets.  When a scanner-side wrapper translates
* a scan failure into a context diagnostic, it carries the
* parse_error directly — same type, no conversion.
*
* CONTENTS
*   I.    parse_stats                       per-session counters
*   II.   identity_string_table             default string-table stub
*   III.  parse_context<StringTable>        the session bundle
*   IV.   accessors / mutators              symbol_tree, string_table,
*                                           diagnostics, stats
*   V.    is_parse_context + concept
*
*
* path:      /inc/djinterp/parse/parse_context.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_PARSE_PARSE_CONTEXT_
#define DJINTERP_PARSE_PARSE_CONTEXT_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./parse.hpp"
#include "./symbol_model.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   parse_stats
// ================================================================

// parse_stats
//   struct: running counters maintained by a parser writing into
// a parse_context.  Symbols emitted, diagnostics raised, files
// fully consumed, bytes consumed across all files in this session.
struct parse_stats
{
    std::size_t symbols_emitted;
    std::size_t diagnostics_raised;
    std::size_t files_consumed;
    std::size_t bytes_consumed;

    parse_stats()
        : symbols_emitted   (0),
          diagnostics_raised(0),
          files_consumed    (0),
          bytes_consumed    (0)
    {
    }
};


// ================================================================
//  II.  identity_string_table
// ================================================================

// identity_string_table
//   class: the default string-table stub.  Interns strings into a
// growing vector and returns their index.  Has the two-method
// surface (`intern`, `lookup`) parse_context expects; callers
// supplying their own string_table type may use any conforming
// implementation in its place.
//
//   This is a no-frills default — adequate for tests and small
// frontends, replaced by a project string_table for the real
// libclang / cpp_scanner pipeline.
class identity_string_table
{
public:
    identity_string_table()
        : m_strings(),
          m_index  ()
    {
        // Reserve slot 0 for the empty string so a zero id is a
        // valid "no payload" marker.
        m_strings.push_back(std::string());
        m_index[std::string()] = 0;
    }

    // intern
    //   method: interns _s, returning its uint32_t id.  Repeat
    // calls with the same string return the same id.
    D_NODISCARD
    std::uint32_t
    intern(
        const std::string& _s
    )
    {
        std::unordered_map<std::string, std::uint32_t>::const_iterator it =
            m_index.find(_s);

        if (it != m_index.end())
        {
            return (it->second);
        }

        std::uint32_t id =
            static_cast<std::uint32_t>(m_strings.size());

        m_strings.push_back(_s);
        m_index[_s] = id;

        return id;
    }

    // lookup
    //   method: returns the string at _id.  Precondition: _id is a
    // value previously returned by intern (or zero for empty).
    D_NODISCARD
    const std::string&
    lookup(
        std::uint32_t _id
    ) const
    {
        return m_strings[_id];
    }

    // size
    //   method: number of interned strings (including the empty
    // sentinel at id 0).
    D_NODISCARD
    std::size_t
    size() const D_NOEXCEPT
    {
        return m_strings.size();
    }

    // clear
    //   method: resets to the post-construction state — empty
    // sentinel at id 0, nothing else.
    void
    clear()
    {
        m_strings.clear();
        m_index  .clear();

        m_strings.push_back(std::string());
        m_index[std::string()] = 0;

        return;
    }

private:
    std::vector<std::string>                       m_strings;
    std::unordered_map<std::string, std::uint32_t> m_index;
};


// ================================================================
//  III. parse_context
// ================================================================

// parse_context
//   class: the per-session bundle.  Owns the symbol_tree, the
// string_table, the diagnostics, and the parse_stats; threaded
// through frontends / scanners by reference so a single session
// accumulates one coherent set of outputs.
//
//   Templated on _StringTable so projects with their own
// string_table type bind directly; the default
// `identity_string_table` provides a working stub for tests and
// simple frontends.
//
//   No copy: a parse_context owns mutable state, including the
// symbol_tree arena, and copying it would silently fork the
// session.  Move is left implicit on the underlying members.
template<typename _StringTable = identity_string_table>
class parse_context
{
public:
    using string_table_type = _StringTable;
    using symbol_tree_type  = symbol_tree;
    using diagnostic_type   = parse_error;
    using diagnostics_type  = std::vector<parse_error>;
    using stats_type        = parse_stats;

    parse_context()
        : m_symbols    (),
          m_strings    (),
          m_diagnostics(),
          m_stats      ()
    {
    }


    // ----------------------------------------------------------
    //  symbol_tree access
    // ----------------------------------------------------------

    // symbols
    //   accessor: the output symbol tree.  Mutable so frontends
    // can allocate into it.
    D_NODISCARD
    symbol_tree_type&
    symbols() D_NOEXCEPT
    {
        return m_symbols;
    }

    D_NODISCARD
    const symbol_tree_type&
    symbols() const D_NOEXCEPT
    {
        return m_symbols;
    }


    // ----------------------------------------------------------
    //  string_table access
    // ----------------------------------------------------------

    // strings
    //   accessor: the string table for symbol_data's *_id fields.
    D_NODISCARD
    string_table_type&
    strings() D_NOEXCEPT
    {
        return m_strings;
    }

    D_NODISCARD
    const string_table_type&
    strings() const D_NOEXCEPT
    {
        return m_strings;
    }

    // intern
    //   convenience: forwards to m_strings.intern.  The vast
    // majority of frontend code interns strings directly through
    // the context, so this saves the verbose `ctx.strings().
    // intern(s)` at every call site.
    D_NODISCARD
    std::uint32_t
    intern(
        const std::string& _s
    )
    {
        return m_strings.intern(_s);
    }


    // ----------------------------------------------------------
    //  diagnostics
    // ----------------------------------------------------------

    // diagnostics
    //   accessor: the accumulated diagnostics for this session.
    D_NODISCARD
    const diagnostics_type&
    diagnostics() const D_NOEXCEPT
    {
        return m_diagnostics;
    }

    // record_error
    //   method: appends a parse_error to the diagnostics and bumps
    // diagnostics_raised.
    void
    record_error(
        const parse_error& _error
    )
    {
        m_diagnostics.push_back(_error);
        m_stats.diagnostics_raised += 1;

        return;
    }

    // record_error  (status + offset + message)
    //   method: convenience for the common case where the caller
    // has the parts but not a constructed parse_error.
    void
    record_error(
        parse_status       _status,
        std::size_t        _offset,
        const std::string& _message
    )
    {
        record_error(parse_error(_status, _offset, _message));

        return;
    }

    // clear_diagnostics
    //   method: drops accumulated diagnostics.  Does not reset the
    // diagnostics_raised counter on m_stats — stats track the
    // session history, diagnostics track the current snapshot.
    void
    clear_diagnostics()
    {
        m_diagnostics.clear();

        return;
    }

    // has_errors
    //   method: true iff at least one diagnostic has been recorded
    // since the last clear_diagnostics.
    D_NODISCARD
    bool
    has_errors() const D_NOEXCEPT
    {
        return (!m_diagnostics.empty());
    }


    // ----------------------------------------------------------
    //  stats
    // ----------------------------------------------------------

    // stats
    //   accessor: the running stats.  Mutable so frontends bump
    // counters directly.
    D_NODISCARD
    stats_type&
    stats() D_NOEXCEPT
    {
        return m_stats;
    }

    D_NODISCARD
    const stats_type&
    stats() const D_NOEXCEPT
    {
        return m_stats;
    }


    // ----------------------------------------------------------
    //  reset
    // ----------------------------------------------------------

    // reset
    //   method: returns the context to post-construction state —
    // empties the symbol tree, clears the string table back to
    // its empty sentinel, drops diagnostics, zeros stats.
    void
    reset()
    {
        m_symbols    .clear();
        m_strings    .clear();
        m_diagnostics.clear();
        m_stats      = stats_type();

        return;
    }


private:
    // No copy — a parse_context owns mutable state.
    parse_context(const parse_context&);
    parse_context& operator=(const parse_context&);

    symbol_tree_type  m_symbols;
    string_table_type m_strings;
    diagnostics_type  m_diagnostics;
    stats_type        m_stats;
};


// ================================================================
//  IV.  identity traits + concept
// ================================================================

NS_INTERNAL

    // is_parse_context_helper
    template<typename _T,
             typename = void>
    struct is_parse_context_helper : std::false_type
    {};

    template<typename _T>
    struct is_parse_context_helper<
        _T,
        void_t<typename clean_t<_T>::string_table_type,
               typename clean_t<_T>::symbol_tree_type,
               typename clean_t<_T>::diagnostics_type,
               typename clean_t<_T>::stats_type>
    > : std::true_type
    {};

NS_END  // internal

// is_parse_context
//   trait: structural check for parse_context conformance.  A
// project may roll its own parse_context-like type; what matters
// downstream is the four nested typedefs above plus the standard
// accessors symbols / strings / diagnostics / stats.
template<typename _T>
struct is_parse_context : internal::is_parse_context_helper<_T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    static constexpr bool is_parse_context_v =
        is_parse_context<_T>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // parse_context_concept
    //   concept: a parse session bundle — has the four nested
    // typedefs and corresponding accessors.
    template<typename _T>
    concept parse_context_concept = is_parse_context<_T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_PARSE_CONTEXT_
