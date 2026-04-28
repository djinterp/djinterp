/******************************************************************************
* djinterp [css]                                                 css_match.hpp
*
* CSS selector matching:
*   This header makes the css subsystem useful to any application -- not
* just HTML.  It defines an abstract `style_target_view` that hosts fill
* in to expose their own objects to the matcher: a type name, an
* optional id, a list of classes, attribute pairs, a tree position, and
* a pseudo-state bitmask whose meaning the host registers.
*
*   Given a target view and a parsed selector, the matcher returns a
* boolean answer with no allocation on the hot path.  Complex selectors
* are matched right-to-left -- the standard browser optimisation that
* lets failing chains bail out early.
*
* Contents:
*   - style_target_attribute     name/value pair attached to a target
*   - style_target_view          host-supplied read-only view
*   - style_pseudo_callback      hook for functional / unregistered pseudos
*   - style_match_options        bitmask registry + extension hook
*   - match_simple   /  match_compound  /  match_complex
*   - match_selector_list        with optional specificity capture
*
*
* path:      /inc/cpp/css/css_match.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_CSS_MATCH_
#define DJINTERP_CSS_MATCH_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include "../core/djinterp.hpp"
#include "./css_ast.hpp"


NS_DJINTERP
NS_CSS


// ================================================================
//  style_target_attribute
// ================================================================

// style_target_attribute
//   struct: a single name/value pair attached to a target.  Both
// pointers are owned by the host and must remain valid for the
// duration of any match call that observes them.
struct style_target_attribute
{
    const char*     name;
    const char*     value;

    style_target_attribute()
        : name  (nullptr)
        , value (nullptr)
    {}

    style_target_attribute(const char* _name,
                           const char* _value)
        : name  (_name)
        , value (_value)
    {}
};


// ================================================================
//  style_target_view
// ================================================================

// style_target_view
//   struct: a read-only view of a stylable host-application
// object.  The host populates whichever fields are meaningful in
// its domain; unknown / inapplicable fields can be left at their
// defaults (null pointers, zero counts).
//
//   For a tree-aware host that wants to support combinators, the
// `parent` and `prev_sibling` pointers must transitively expose
// the rest of the relevant ancestors / earlier siblings.  Nothing
// is allocated by this struct -- pointers are borrowed.
//
// Field summary:
//   type_name           e.g. "button".  May be null for objects
//                       with no type (matches `*` only).
//   id                  e.g. "submit".  May be null.
//   classes             C-string array; not null-terminated as a
//                       sequence -- length is `classes_count`.
//   attributes          attribute array; length `attributes_count`.
//   parent              ptr to parent's view, or null at root.
//   prev_sibling        ptr to immediately-preceding sibling, or
//                       null if first child.
//   index_in_parent     0-based position among siblings.
//   parent_child_count  number of siblings (incl. self).
//   state_flags         host-defined pseudo-state bitmask; bits
//                       are mapped to names by the registry in
//                       style_match_options.
struct style_target_view
{
    const char*                         type_name;
    const char*                         id;

    const char* const*                  classes;
    std::size_t                         classes_count;

    const style_target_attribute*       attributes;
    std::size_t                         attributes_count;

    const style_target_view*            parent;
    const style_target_view*            prev_sibling;

    std::size_t                         index_in_parent;
    std::size_t                         parent_child_count;

    std::uint64_t                       state_flags;

    style_target_view()
        : type_name           (nullptr)
        , id                  (nullptr)
        , classes             (nullptr)
        , classes_count       (0u)
        , attributes          (nullptr)
        , attributes_count    (0u)
        , parent              (nullptr)
        , prev_sibling        (nullptr)
        , index_in_parent     (0u)
        , parent_child_count  (0u)
        , state_flags         (0u)
    {}
};


// ================================================================
//  style_pseudo_callback
// ================================================================

// style_pseudo_callback
//   typedef: callback signature for evaluating functional and
// otherwise-unregistered pseudo-class selectors.  Returning true
// means the pseudo matched the target.
//
//   _name is the pseudo-class identifier (no leading colon).
//   _arg is the raw text between the parens, or "" if argless.
//   _target is the target being tested.
//   _user is the opaque pointer provided in style_match_options.
typedef bool (*style_pseudo_callback)
(
    const char*                 _name,
    const char*                 _arg,
    const style_target_view&    _target,
    void*                       _user
);


// ================================================================
//  style_match_options
// ================================================================

// style_match_options
//   struct: configuration for selector matching.  Hosts register
// the bit position of each pseudo-state they care about (e.g.
// "hover" -> bit 0, "focus" -> bit 1) and may install a callback
// that handles any pseudo-class not in the bit registry.
//
//   The struct is intentionally trivial -- no virtual dispatch.
// Pass it by const reference into match_* functions.
struct style_match_options
{
    // state_bits
    //   map from pseudo-class name -> bit index in
    // style_target_view::state_flags.  Linear scan; intended for
    // small (~16) registries.  Hosts with many states should
    // build their own faster lookup and bypass this struct.
    std::vector<std::pair<std::string, unsigned> >  state_bits;

    style_pseudo_callback                            pseudo_callback;
    void*                                            pseudo_user;

    style_match_options()
        : state_bits      ()
        , pseudo_callback (nullptr)
        , pseudo_user     (nullptr)
    {}

    // register_state
    //   member: convenience for adding a state-bit mapping.
    void
    register_state(const std::string& _name,
                   unsigned           _bit)
    {
        state_bits.push_back(std::make_pair(_name, _bit));

        return;
    }

    // find_state_bit
    //   member: returns true and writes the bit index to _out if
    // _name is registered, false otherwise.
    bool
    find_state_bit(const std::string& _name,
                   unsigned&          _out) const
    {
        std::size_t i;

        for (i = 0u; i < state_bits.size(); ++i)
        {
            if (state_bits[i].first == _name)
            {
                _out = state_bits[i].second;

                return true;
            }
        }

        return false;
    }
};


// ================================================================
//  internal -- low-level matching helpers
// ================================================================

NS_INTERNAL

    // safe_cstr
    //   function: returns _p, or "" if _p is null.  Used so the
    // matcher can compare against host-supplied strings without
    // null-checking at every comparison site.
    inline const char*
    safe_cstr(const char* _p)
    {
        return (_p != nullptr) ? _p : "";
    }

    // ascii_streq
    //   function: case-sensitive C-string equality.
    inline bool
    ascii_streq(const char* _a,
                const char* _b)
    {
        return (std::strcmp(safe_cstr(_a), safe_cstr(_b)) == 0);
    }

    // ascii_streq_ci
    //   function: ASCII case-insensitive C-string equality.
    inline bool
    ascii_streq_ci(const char* _a,
                   const char* _b)
    {
        const char* a;
        const char* b;

        a = safe_cstr(_a);
        b = safe_cstr(_b);

        while ( (*a != '\0') &&
                (*b != '\0') )
        {
            char ca = *a;
            char cb = *b;

            if ( (ca >= 'A') && (ca <= 'Z') )
            {
                ca = static_cast<char>(ca + ('a' - 'A'));
            }

            if ( (cb >= 'A') && (cb <= 'Z') )
            {
                cb = static_cast<char>(cb + ('a' - 'A'));
            }

            if (ca != cb)
            {
                return false;
            }

            a += 1;
            b += 1;
        }

        return ( (*a == '\0') &&
                 (*b == '\0') );
    }

    // find_attribute
    //   function: linear search of a target's attributes for the
    // one whose name equals _name (case-sensitive).  Returns null
    // if none.
    inline const style_target_attribute*
    find_attribute(const style_target_view& _t,
                   const char*              _name)
    {
        std::size_t i;

        for (i = 0u; i < _t.attributes_count; ++i)
        {
            if (ascii_streq(_t.attributes[i].name, _name))
            {
                return &_t.attributes[i];
            }
        }

        return nullptr;
    }

    // attr_value_compare
    //   function: implements the seven attribute-selector ops on
    // a host attribute value vs. a selector value, honouring the
    // case-insensitive flag from the selector.
    inline bool
    attr_value_compare(const char*     _attr_value,
                       DCssAttributeOp _op,
                       const char*     _sel_value,
                       bool            _case_insensitive)
    {
        const char* av;
        const char* sv;

        av = safe_cstr(_attr_value);
        sv = safe_cstr(_sel_value);

        switch (_op)
        {
            case DCssAttributeOpPresence:
                return true;

            case DCssAttributeOpExact:
                return _case_insensitive
                            ? ascii_streq_ci(av, sv)
                            : ascii_streq(av, sv);

            case DCssAttributeOpPrefix:
            {
                std::size_t sl = std::strlen(sv);

                if (sl == 0u)
                {
                    return false;
                }

                if (_case_insensitive)
                {
                    std::size_t i;

                    if (std::strlen(av) < sl)
                    {
                        return false;
                    }

                    for (i = 0u; i < sl; ++i)
                    {
                        char ca = av[i];
                        char cb = sv[i];

                        if ( (ca >= 'A') && (ca <= 'Z') )
                        {
                            ca = static_cast<char>(ca + ('a' - 'A'));
                        }

                        if ( (cb >= 'A') && (cb <= 'Z') )
                        {
                            cb = static_cast<char>(cb + ('a' - 'A'));
                        }

                        if (ca != cb)
                        {
                            return false;
                        }
                    }

                    return true;
                }

                return (std::strncmp(av, sv, sl) == 0);
            }

            case DCssAttributeOpSuffix:
            {
                std::size_t al = std::strlen(av);
                std::size_t sl = std::strlen(sv);

                if (sl == 0u)
                {
                    return false;
                }

                if (al < sl)
                {
                    return false;
                }

                if (_case_insensitive)
                {
                    return ascii_streq_ci(av + (al - sl), sv);
                }

                return (std::strcmp(av + (al - sl), sv) == 0);
            }

            case DCssAttributeOpSubstring:
            {
                if (*sv == '\0')
                {
                    return false;
                }

                if (_case_insensitive)
                {
                    // simple O(n*m) ci substring search
                    std::size_t al = std::strlen(av);
                    std::size_t sl = std::strlen(sv);
                    std::size_t i;

                    if (al < sl)
                    {
                        return false;
                    }

                    for (i = 0u; i + sl <= al; ++i)
                    {
                        std::size_t j;
                        bool        ok;

                        ok = true;

                        for (j = 0u; j < sl; ++j)
                        {
                            char ca = av[i + j];
                            char cb = sv[j];

                            if ( (ca >= 'A') && (ca <= 'Z') )
                            {
                                ca = static_cast<char>(ca + ('a' - 'A'));
                            }

                            if ( (cb >= 'A') && (cb <= 'Z') )
                            {
                                cb = static_cast<char>(cb + ('a' - 'A'));
                            }

                            if (ca != cb)
                            {
                                ok = false;
                                break;
                            }
                        }

                        if (ok)
                        {
                            return true;
                        }
                    }

                    return false;
                }

                return (std::strstr(av, sv) != nullptr);
            }

            case DCssAttributeOpWord:
            {
                // any whitespace-delimited token equal to sv
                std::size_t sl = std::strlen(sv);
                const char* p  = av;

                if (sl == 0u)
                {
                    return false;
                }

                while (*p != '\0')
                {
                    while ( (*p != '\0') &&
                            ( (*p == ' ')  ||
                              (*p == '\t') ||
                              (*p == '\n') ||
                              (*p == '\r') ||
                              (*p == '\f') ) )
                    {
                        p += 1;
                    }

                    if (*p == '\0')
                    {
                        break;
                    }

                    const char* end = p;

                    while ( (*end != '\0') &&
                            (*end != ' ')  &&
                            (*end != '\t') &&
                            (*end != '\n') &&
                            (*end != '\r') &&
                            (*end != '\f') )
                    {
                        end += 1;
                    }

                    if (static_cast<std::size_t>(end - p) == sl)
                    {
                        bool eq = true;

                        if (_case_insensitive)
                        {
                            std::size_t k;

                            for (k = 0u; k < sl; ++k)
                            {
                                char ca = p[k];
                                char cb = sv[k];

                                if ( (ca >= 'A') && (ca <= 'Z') )
                                {
                                    ca = static_cast<char>(
                                        ca + ('a' - 'A')
                                    );
                                }

                                if ( (cb >= 'A') && (cb <= 'Z') )
                                {
                                    cb = static_cast<char>(
                                        cb + ('a' - 'A')
                                    );
                                }

                                if (ca != cb)
                                {
                                    eq = false;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            eq = (std::strncmp(p, sv, sl) == 0);
                        }

                        if (eq)
                        {
                            return true;
                        }
                    }

                    p = end;
                }

                return false;
            }

            case DCssAttributeOpLang:
            {
                // exact match, OR av startswith sv "-"
                std::size_t sl = std::strlen(sv);
                std::size_t al = std::strlen(av);

                if (_case_insensitive
                        ? ascii_streq_ci(av, sv)
                        : ascii_streq(av, sv))
                {
                    return true;
                }

                if (al > sl)
                {
                    if (av[sl] == '-')
                    {
                        if (_case_insensitive)
                        {
                            std::size_t i;

                            for (i = 0u; i < sl; ++i)
                            {
                                char ca = av[i];
                                char cb = sv[i];

                                if ( (ca >= 'A') && (ca <= 'Z') )
                                {
                                    ca = static_cast<char>(
                                        ca + ('a' - 'A')
                                    );
                                }

                                if ( (cb >= 'A') && (cb <= 'Z') )
                                {
                                    cb = static_cast<char>(
                                        cb + ('a' - 'A')
                                    );
                                }

                                if (ca != cb)
                                {
                                    return false;
                                }
                            }

                            return true;
                        }

                        return (std::strncmp(av, sv, sl) == 0);
                    }
                }

                return false;
            }
        }

        return false;
    }

NS_END  // internal


// ================================================================
//  match_simple
// ================================================================

// forward decl for negation recursion
inline bool
match_compound(const css_compound_selector&    _c,
               const style_target_view&        _t,
               const style_match_options&      _opt);

// match_simple
//   function: tests whether a single simple selector matches the
// given target.  Pure -- no allocation, no globals.
inline bool
match_simple(const css_simple_selector&  _s,
             const style_target_view&    _t,
             const style_match_options&  _opt)
{
    switch (_s.kind)
    {
        case DCssSimpleKindUniversal:
            return true;

        case DCssSimpleKindType:
            return internal::ascii_streq(_t.type_name,
                                         _s.name.c_str());

        case DCssSimpleKindId:
            return internal::ascii_streq(_t.id, _s.name.c_str());

        case DCssSimpleKindClass:
        {
            std::size_t i;

            for (i = 0u; i < _t.classes_count; ++i)
            {
                if (internal::ascii_streq(_t.classes[i],
                                          _s.name.c_str()))
                {
                    return true;
                }
            }

            return false;
        }

        case DCssSimpleKindAttribute:
        {
            const style_target_attribute* a;

            a = internal::find_attribute(_t, _s.name.c_str());

            if (!a)
            {
                return false;
            }

            return internal::attr_value_compare(
                a->value,
                _s.attribute_op,
                _s.attribute_value.c_str(),
                _s.attribute_case_insensitive
            );
        }

        case DCssSimpleKindPseudoClass:
        {
            unsigned bit;

            // structural pseudos that we evaluate ourselves
            if (_s.name == std::string("first-child"))
            {
                return (_t.index_in_parent == 0u);
            }

            if (_s.name == std::string("last-child"))
            {
                return ( (_t.parent_child_count > 0u) &&
                         (_t.index_in_parent ==
                          (_t.parent_child_count - 1u)) );
            }

            if (_s.name == std::string("only-child"))
            {
                return (_t.parent_child_count == 1u);
            }

            if (_s.name == std::string("root"))
            {
                return (_t.parent == nullptr);
            }

            if (_s.name == std::string("empty"))
            {
                return (_t.parent_child_count == 0u);
            }

            // registered state pseudo (e.g. :hover -> bit 0)
            if (_opt.find_state_bit(_s.name, bit))
            {
                std::uint64_t mask =
                    (static_cast<std::uint64_t>(1u) << bit);

                return ( (_t.state_flags & mask) != 0u );
            }

            // fall through to host callback for everything else
            if (_opt.pseudo_callback)
            {
                return _opt.pseudo_callback(
                    _s.name.c_str(),
                    _s.pseudo_arg.c_str(),
                    _t,
                    _opt.pseudo_user
                );
            }

            return false;
        }

        case DCssSimpleKindPseudoElement:
        {
            // pseudo-elements (::before, ::after, ::placeholder)
            // are usually addressed via the pseudo callback
            // since they refer to "rendered things" rather than
            // the target itself.
            if (_opt.pseudo_callback)
            {
                return _opt.pseudo_callback(
                    _s.name.c_str(),
                    "",
                    _t,
                    _opt.pseudo_user
                );
            }

            return false;
        }

        case DCssSimpleKindNegation:
        {
            if (!_s.negated)
            {
                return false;
            }

            return (!match_compound(*_s.negated, _t, _opt));
        }
    }

    return false;
}


// ================================================================
//  match_compound
// ================================================================

// match_compound
//   function: tests whether every simple in the compound matches
// the target.  Empty compounds match unconditionally (this case
// does not appear in valid output of the parser, but we return
// the natural answer for it).
inline bool
match_compound(const css_compound_selector&    _c,
               const style_target_view&        _t,
               const style_match_options&      _opt)
{
    std::size_t i;

    for (i = 0u; i < _c.simples.size(); ++i)
    {
        if (!match_simple(_c.simples[i], _t, _opt))
        {
            return false;
        }
    }

    return true;
}


// ================================================================
//  match_complex
// ================================================================

// match_complex
//   function: tests whether the complex selector matches the
// target.  Walks right-to-left: the rightmost compound is matched
// against `_t`, then for each preceding compound a relevant
// related target is sought via the combinator that introduced
// the next compound.
//
//   Combinator semantics:
//     descendant       -- some ancestor of current must match
//     child            -- the parent of current must match
//     adjacent_sibling -- the immediately-preceding sibling must
//     general_sibling  -- some earlier sibling must match
inline bool
match_complex(const css_complex_selector&     _c,
              const style_target_view&        _t,
              const style_match_options&      _opt)
{
    // collect compounds in document order: head first, then tail.
    // We do not actually allocate -- index in [0..N-1] resolves
    // through the structure directly.
    std::size_t total;
    std::size_t i;

    total = 1u + _c.tail.size();

    // rightmost compound must match the target
    {
        const css_compound_selector& last = (_c.tail.empty())
                                                  ? _c.head
                                                  : _c.tail.back().compound;

        if (!match_compound(last, _t, _opt))
        {
            return false;
        }
    }

    // walk leftward through tail combinators, pairing each with
    // the compound immediately preceding it.
    const style_target_view* current = &_t;

    for (i = total - 1u; i > 0u; --i)
    {
        // the combinator at chain position `i` introduces the
        // compound at position `i`; we need to find a target for
        // the compound at position `i-1` that satisfies the
        // combinator relative to `current`.
        DCssCombinator                  combo;
        const css_compound_selector*    prev_cmp;

        // tail is indexed 0..tail.size()-1 corresponding to chain
        // positions 1..tail.size().
        combo    = _c.tail[i - 1u].combinator;
        prev_cmp = (i == 1u)
                       ? &_c.head
                       : &_c.tail[i - 2u].compound;

        switch (combo)
        {
            case DCssCombinatorChild:
            {
                if (!current->parent)
                {
                    return false;
                }

                if (!match_compound(*prev_cmp,
                                    *current->parent,
                                    _opt))
                {
                    return false;
                }

                current = current->parent;
                break;
            }

            case DCssCombinatorDescendant:
            {
                const style_target_view* anc = current->parent;
                bool                     found = false;

                while (anc != nullptr)
                {
                    if (match_compound(*prev_cmp, *anc, _opt))
                    {
                        found   = true;
                        current = anc;
                        break;
                    }

                    anc = anc->parent;
                }

                if (!found)
                {
                    return false;
                }

                break;
            }

            case DCssCombinatorAdjacentSibling:
            {
                if (!current->prev_sibling)
                {
                    return false;
                }

                if (!match_compound(*prev_cmp,
                                    *current->prev_sibling,
                                    _opt))
                {
                    return false;
                }

                current = current->prev_sibling;
                break;
            }

            case DCssCombinatorGeneralSibling:
            {
                const style_target_view* sib = current->prev_sibling;
                bool                     found = false;

                while (sib != nullptr)
                {
                    if (match_compound(*prev_cmp, *sib, _opt))
                    {
                        found   = true;
                        current = sib;
                        break;
                    }

                    sib = sib->prev_sibling;
                }

                if (!found)
                {
                    return false;
                }

                break;
            }
        }
    }

    return true;
}


// ================================================================
//  match_selector_list
// ================================================================

// match_selector_list
//   function: returns true if any selector in the list matches
// the target.  When _matched_specificity is non-null and at
// least one selector matches, it receives the highest specificity
// among matching selectors -- this is the value the cascade uses.
//
//   When no selector matches and _matched_specificity is non-null
// the pointee is left unmodified.
inline bool
match_selector_list(const css_selector_list&    _list,
                    const style_target_view&    _t,
                    const style_match_options&  _opt,
                    std::uint32_t*              _matched_specificity)
{
    bool          any  = false;
    std::uint32_t best = 0u;
    std::size_t   i;

    for (i = 0u; i < _list.selectors.size(); ++i)
    {
        if (match_complex(_list.selectors[i], _t, _opt))
        {
            any = true;

            if (_list.selectors[i].specificity > best)
            {
                best = _list.selectors[i].specificity;
            }
        }
    }

    if (any && _matched_specificity)
    {
        *_matched_specificity = best;
    }

    return any;
}


NS_END  // css
NS_END  // djinterp


#endif  // DJINTERP_CSS_MATCH_
