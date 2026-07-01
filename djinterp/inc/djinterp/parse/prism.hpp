/******************************************************************************
* djinterp [parse]                                                    prism.hpp
*
* Composition and parsing as a prism into the surface stream.
*   Per ch-parsing.tex, parsing and composition are the two legs of a
* prism on the language Lang ⊆ Σ*:
*
*       compose : μF  → Σ*           total,  injective,  contravariant
*       parse   : Σ* ⇀ μF            partial (defined on Lang), covariant
*
*   They form a profunctor over Σ* (covariant on the parse side,
* contravariant on the compose side) and satisfy the round-trip
* laws
*
*       parse(compose t) = t                        for every t ∈ μF
*       compose(parse s) = s        for every s ∈ Lang ⊆ Σ*
*
* — i.e. compose is a section of parse on Lang, and parse is a
* retraction of compose.  Together they witness the prism
*
*       μF  ⇆  Σ*                    (= Lang on the round trip)
*
* whose obvious presentation in C++ is a pair of functions, one in
* each direction, related by these laws.  This header carries that
* pair as a single value type, plus the dimap transport that makes
* the prism a profunctor.
*
*   The compose direction (μF → Σ*) is realised as a catamorphism
* — cata[φ] for a print algebra φ : F Σ* → Σ*.  Each variant of F
* describes one constructor of μF; φ tells the fold how to render
* that constructor given its children's already-rendered surfaces.
* The total, injective output is the parsable carrier's surface form.
*
*   The parse direction is whatever parser<_Carrier, _Element> the
* caller has built — typically using the combinators in parser/.
* The prism just bundles the two and exposes the round-trip face.
*
* CONTENTS
*   I.    compose_traits<_Carrier>   per-carrier print algebra lookup
*   II.   compose(t)                 cata[φ] : μF → Σ*
*   III.  prism<_Carrier, _Element>  the (parse, compose) pair
*   IV.   dimap                      profunctor transport across iso
*   V.    round-trip law helpers     compile- and run-time checks
*
* path:      /inc/djinterp/parse/prism.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_PARSE_PRISM_
#define DJINTERP_PARSE_PRISM_ 1

// std
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "./parse.hpp"
#include "./parser/parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   compose_traits
// ================================================================

// compose_traits
//   trait: primary template — undefined by default.  A parsable
// carrier _Carrier ≅ μF participates in composition by specialising
// compose_traits<_Carrier> with a static `algebra(t)` that returns
// the surface string for an instance.  This is the C++ stand-in for
// the formal print algebra φ : F Σ* → Σ*: the implementation
// pattern-matches on _Carrier's variants (the constructors of F)
// and concatenates the already-rendered surfaces of its children
// (the recursive subtrees).
//
//   By design the trait is structural rather than virtual: a
// carrier shape changes a single specialisation here, not a
// virtual dispatch hierarchy across the value type.
//
//   _Carrier   the parsable type D ≅ μF.
//   _Element   the surface stream element type (char by default).
template<typename _Carrier,
         typename _Element = char>
struct compose_traits;


NS_INTERNAL

    // is_composable_helper
    //   helper: SFINAE detector for whether compose_traits<_T> is
    // specialised — looks for the static algebra() entry point.
    template<typename _T,
             typename _Element = char,
             typename = void>
    struct is_composable_helper : std::false_type
    {};

    template<typename _T,
             typename _Element>
    struct is_composable_helper<
        _T,
        _Element,
        void_t<decltype(
            compose_traits<_T, _Element>::algebra(
                std::declval<const _T&>()))>
    > : std::true_type
    {};

NS_END  // internal


// is_composable
//   trait: true iff _T has a compose_traits specialisation able to
// fold it into a string.
template<typename _T,
         typename _Element = char>
struct is_composable
    : internal::is_composable_helper<_T, _Element>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T,
             typename _Element = char>
    static constexpr bool is_composable_v =
        is_composable<_T, _Element>::value;
#endif


// ================================================================
//  II.  compose
// ================================================================

// compose
//   function: cata[φ] : μF → Σ*.  Folds the parsable structure into
// its surface form by recursively rendering children and applying
// the print algebra at each level.  The work is delegated to
// compose_traits<_Carrier>::algebra, which is the per-carrier
// specialisation of φ.
//
//   Total on every value of _Carrier (since the formal definition
// requires φ to be defined on F Σ* in full).  Injective when the
// carrier shape is decodable — the surface form recovers the
// structure uniquely under parse — which is the property
// witnessing the prism on the language Lang.
//
//   Calling compose on a type without a compose_traits
// specialisation is a compile error.
template<typename _Carrier,
         typename _Element = char>
D_NODISCARD
auto compose(
    const _Carrier& _t
)
-> decltype(compose_traits<_Carrier, _Element>::algebra(_t))
{
    static_assert(
        is_composable<_Carrier, _Element>::value,
        "compose: _Carrier needs a compose_traits specialisation "
        "(static algebra(const _Carrier&) -> Σ*).");

    return compose_traits<_Carrier, _Element>::algebra(_t);
}


// ================================================================
//  III. prism
// ================================================================

// prism
//   class: bundles the parse and compose legs of the round trip
// into a single value.  The two functions are dual: parse covariant
// in the carrier, compose contravariant.  On Lang they are mutual
// inverses (see the round-trip laws in V).
//
//   The prism does no work at call time beyond delegating to its
// two components — it exists so the relationship between the
// directions is visible in one place, and so a downstream module
// can take a prism as a parameter and use either leg.
//
//   _Carrier   the parsable type D ≅ μF.
//   _Element   the surface element type (char by default).
template<typename _Carrier,
         typename _Element = char>
class prism
{
public:
    using carrier_type  = _Carrier;
    using element_type  = _Element;
    using state_type    = parse_state<_Element>;
    using surface_type  = std::basic_string<_Element>;
    using parser_type   = parser<_Carrier, _Element>;
    using compose_fn    =
        std::function<surface_type(const _Carrier&)>;

    prism()
        : m_parse(),
          m_compose()
    {}

    prism(
        parser_type _parse,
        compose_fn  _compose
    )
        : m_parse  (static_cast<parser_type&&>(_parse)),
          m_compose(static_cast<compose_fn&&>(_compose))
    {}


    // parse
    //   method: the partial direction Σ* ⇀ μF.  Returns success on
    // every s ∈ Lang and an error otherwise.
    D_NODISCARD
    parse_result<_Carrier>
    parse(
        state_type& _state
    ) const
    {
        return m_parse.parse(_state);
    }

    // parse (string overload)
    //   convenience: builds a parse_state over the surface string
    // and runs the parser.
    D_NODISCARD
    parse_result<_Carrier>
    parse(
        const surface_type& _surface
    ) const
    {
        state_type s(_surface.data(), _surface.size(), 0);

        return m_parse.parse(s);
    }

    // compose
    //   method: the total direction μF → Σ*.  Returns the surface
    // form of _t.
    D_NODISCARD
    surface_type
    compose(
        const _Carrier& _t
    ) const
    {
        return m_compose(_t);
    }

    // parser
    //   accessor: the underlying parse leg as a handle.
    D_NODISCARD
    const parser_type&
    parser_leg() const D_NOEXCEPT
    {
        return m_parse;
    }

    // compose_leg
    //   accessor: the underlying compose function.
    D_NODISCARD
    const compose_fn&
    compose_leg() const D_NOEXCEPT
    {
        return m_compose;
    }

private:
    parser_type m_parse;
    compose_fn  m_compose;
};


// make_prism
//   factory: build a prism from a parser-side leg and a compose-
// side function.  The compose function may be supplied directly,
// or use make_prism_via_traits when _Carrier already has a
// compose_traits specialisation.
template<typename _Carrier,
         typename _Element = char>
D_NODISCARD
prism<_Carrier, _Element>
make_prism(
    parser<_Carrier, _Element>                          _parse,
    std::function<std::basic_string<_Element>(
        const _Carrier&)>                               _compose
)
{
    return prism<_Carrier, _Element>(
        static_cast<parser<_Carrier, _Element>&&>(_parse),
        static_cast<std::function<std::basic_string<_Element>(
            const _Carrier&)>&&>(_compose));
}

// make_prism_via_traits
//   factory: build a prism whose compose leg is the compose_traits
// specialisation registered for _Carrier.  Compile error if no
// such specialisation exists.
template<typename _Carrier,
         typename _Element = char>
D_NODISCARD
prism<_Carrier, _Element>
make_prism_via_traits(
    parser<_Carrier, _Element> _parse
)
{
    static_assert(
        is_composable<_Carrier, _Element>::value,
        "make_prism_via_traits: _Carrier needs a compose_traits "
        "specialisation.");

    using surface_type = std::basic_string<_Element>;

    return prism<_Carrier, _Element>(
        static_cast<parser<_Carrier, _Element>&&>(_parse),
        [](const _Carrier& _t) -> surface_type
        {
            return compose_traits<_Carrier, _Element>::algebra(_t);
        });
}


// ================================================================
//  IV.  dimap  —  profunctor transport
// ================================================================

// dimap
//   function: transports a prism across an isomorphism on the
// carrier side.  If _Iso = (f : _Carrier → _Other, g : _Other →
// _Carrier) is a bijection, then dimap(p, f, g) produces a prism
// over _Other whose parse leg recovers an _Other via f after the
// underlying _Carrier parse, and whose compose leg pre-applies g
// before the underlying compose.
//
//   The profunctor law — covariance on parse, contravariance on
// compose — is what makes this signature read as `dimap(p, f, g)`:
// f flows in the parse direction (post-applied), g in the compose
// direction (pre-applied).
//
//   _Carrier   the underlying parsable type.
//   _Other     the carrier the new prism produces / consumes.
//   _Element   the surface element type.
template<typename _Carrier,
         typename _Other,
         typename _Element,
         typename _Forward,
         typename _Backward>
D_NODISCARD
prism<_Other, _Element>
dimap(
    const prism<_Carrier, _Element>& _p,
    _Forward                         _f,
    _Backward                        _g
)
{
    using state_type   = parse_state<_Element>;
    using surface_type = std::basic_string<_Element>;
    using parser_type  = parser<_Other, _Element>;
    using compose_fn   = std::function<surface_type(const _Other&)>;

    parser_type lifted_parse(
        [_p, _f](state_type& _state) -> parse_result<_Other>
        {
            parse_result<_Carrier> r = _p.parse(_state);

            if (!r.ok())
            {
                return parse_result<_Other>(r.error());
            }

            return parse_result<_Other>(_f(r.value()));
        });

    compose_fn lifted_compose =
        [_p, _g](const _Other& _o) -> surface_type
        {
            return _p.compose(_g(_o));
        };

    return prism<_Other, _Element>(
        static_cast<parser_type&&>(lifted_parse),
        static_cast<compose_fn&&>(lifted_compose));
}


// ================================================================
//  V.   round-trip law helpers
// ================================================================

// check_compose_parse
//   function: verifies parse(compose t) == t for a given t —
// i.e. compose followed by parse returns the original structure.
// This is the "compose is a section" half of the prism law and
// the cheap direction to test: it succeeds on every t ∈ μF.
//
//   Returns true on success, false if either compose/parse failed
// or the round trip changed the value.  _Carrier must support
// operator== for the equality check.
template<typename _Carrier,
         typename _Element>
D_NODISCARD
bool
check_compose_parse(
    const prism<_Carrier, _Element>& _p,
    const _Carrier&                  _t
)
{
    std::basic_string<_Element> surface = _p.compose(_t);

    parse_state<_Element> state(
        surface.data(), surface.size(), 0);

    parse_result<_Carrier> r = _p.parse(state);

    if (!r.ok())
    {
        return false;
    }

    return (r.value() == _t);
}

// check_parse_compose
//   function: verifies compose(parse s) == s for a given s — the
// "parse is a retraction on Lang" half of the prism law.  Returns
// false if s ∉ Lang (i.e. the parser fails) or if the round trip
// changed the string.  Equality on surface strings is std::basic_
// string::operator==, which is character-wise.
template<typename _Carrier,
         typename _Element>
D_NODISCARD
bool
check_parse_compose(
    const prism<_Carrier, _Element>& _p,
    const std::basic_string<_Element>& _s
)
{
    parse_state<_Element> state(_s.data(), _s.size(), 0);

    parse_result<_Carrier> r = _p.parse(state);

    if (!r.ok())
    {
        // s ∉ Lang — the law applies only on Lang, so this isn't
        // a violation but also isn't a confirmation.  Treat it as
        // "not in scope".
        return false;
    }

    return (_p.compose(r.value()) == _s);
}


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_PRISM_
