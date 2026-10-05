/*******************************************************************************
* djinterp [parse]                                                     prism.hpp
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
*   The parse direction is whatever parser<Carrier, Element> the
* caller has built — typically using the combinators in parser/.
* The prism just bundles the two and exposes the round-trip face.
*
* CONTENTS
*   I.    compose_traits<Carrier>   per-carrier print algebra lookup
*   II.   compose(t)                 cata[φ] : μF → Σ*
*   III.  prism<Carrier, Element>  the (parse, compose) pair
*   IV.   dimap                      profunctor transport across iso
*   V.    round-trip law helpers     compile- and run-time checks
*
*
* path:      /inc/djinterp/parse/prism.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSE_PRISM_HPP
#define DJINTERP_PARSE_PRISM_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../core/meta/type_utility.hpp"  // void_t
#include "../core/functional/profunctor.hpp"
#include "../core/functional/recursion.hpp"
#include "./parse.hpp"
#include "./parser/parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   compose_traits
// ================================================================

// compose_traits
//   trait: primary template — undefined by default.  A parsable
// carrier Carrier ≅ μF supplies its *print algebra* by specialising
//
//     using is_specialized = std::true_type;
//     static Σ* algebra(
//         const recursive_traits<Carrier>::template base<Σ*>& layer);
//
// — the F-algebra φ : F(Σ*) → Σ*, acting on ONE layer of F whose
// recursive children have *already* been rendered to Σ* by the
// surrounding catamorphism.  The carrier declares its structure
// separately (base functor + project) through
// recursive_traits<Carrier> in functional/recursion.hpp; compose
// is then literally cata[φ] over that structure.
//
//   This is the split that makes φ an ordinary F-algebra rather
// than a bespoke whole-tree hook: the structural recursion belongs
// to recursion.hpp and is written once, while φ describes a single
// layer.  The very same recursive_traits<Carrier> then powers
// evaluate = cata[φ_eval], type-check = cata[φ_types], pretty-print
// = cata[φ_doc], and so on — only the algebra changes.  That is
// what makes `D ≅ μF` load-bearing rather than merely documented.
//
//   Carrier    the parsable type D ≅ μF.
//   Element    the surface stream element type (char by default).
template<typename Carrier,
         typename Element = char,
         typename Enable   = void>
struct compose_traits;


NS_INTERNAL

    // is_composable_helper
    //   helper: SFINAE detector requiring BOTH that T declares a
    // fold structure (recursive_traits<T>, so the layer type
    // base<Σ*> is well-formed) AND that compose_traits<T> supplies
    // the print algebra over that layer.  Either half missing →
    // not composable.
    template<typename T,
             typename Element = char,
             typename = void>
    struct is_composable_helper : std::false_type
    {};

    template<typename T,
             typename Element>
    struct is_composable_helper<
        T,
        Element,
        void_t<
            typename compose_traits<T, Element>::is_specialized,
            decltype(
                compose_traits<T, Element>::algebra(
                    std::declval<
                        const typename recursive_traits<T>::
                            template base<
                                std::basic_string<Element> >&>()))>
    > : std::true_type
    {};

NS_END  // internal


// is_composable
//   trait: true iff T has both a recursive_traits structure map
// and a compose_traits print algebra over it — i.e. compose can
// fold it into a string.
template<typename T,
         typename Element = char>
struct is_composable
    : internal::is_composable_helper<T, Element>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T,
             typename Element = char>
    static constexpr bool is_composable_v =
        is_composable<T, Element>::value;
#endif


// ================================================================
//  II.  compose
// ================================================================

// compose
//   function: cata[φ] : μF → Σ*.  This is now a *genuine*
// catamorphism — it hands the print algebra φ = compose_traits<
// Carrier>::algebra to functional/recursion.hpp's cata, which
// peels each layer via recursive_traits<Carrier>::project, folds
// every child to its surface string, and applies φ to assemble the
// layer.  The recursion is not restated here; only φ is carrier-
// specific.
//
//   Total on every value of Carrier (φ is defined on all of
// F(Σ*)).  Injective when the surface uniquely determines the
// structure — the property that, together with `parse` as the
// partial inverse ana, witnesses the prism on the language Lang.
//
//   Calling compose on a carrier lacking either a recursive_traits
// structure map or a compose_traits algebra is a compile error
// naming the missing half.
template<typename Carrier,
         typename Element = char>
D_NODISCARD
std::basic_string<Element>
compose(
    const Carrier& _t
)
{
    static_assert(
        is_recursive<Carrier>::value,
        "compose: Carrier needs a recursive_traits specialisation "
        "(base functor + project : C -> F(C)).");

    static_assert(
        is_composable<Carrier, Element>::value,
        "compose: Carrier needs a compose_traits specialisation "
        "(the print algebra phi : F(Σ*) -> Σ*).");

    using surface_type = std::basic_string<Element>;
    using layer_type   =
        typename recursive_traits<Carrier>::template base<
            surface_type>;

    return ::djinterp::cata<surface_type, Carrier>(
        [](const layer_type& _layer) -> surface_type
        {
            return compose_traits<Carrier, Element>::algebra(
                _layer);
        },
        _t);
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
//   Carrier    the parsable type D ≅ μF.
//   Element    the surface element type (char by default).
template<typename Carrier,
         typename Element = char>
class prism
{
public:
    using carrier_type  = Carrier;
    using element_type  = Element;
    using state_type    = parse_state<Element>;
    using surface_type  = std::basic_string<Element>;
    using parser_type   = parser<Carrier, Element>;
    using compose_fn    =
        std::function<surface_type(const Carrier&)>;

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
    parse_result<Carrier>
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
    parse_result<Carrier>
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
        const Carrier& _t
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
// or use make_prism_via_traits when Carrier already has a
// compose_traits specialisation.
template<typename Carrier,
         typename Element = char>
D_NODISCARD
prism<Carrier, Element>
make_prism(
    parser<Carrier, Element>                          _parse,
    std::function<std::basic_string<Element>(
        const Carrier&)>                               _compose
)
{
    return prism<Carrier, Element>(
        static_cast<parser<Carrier, Element>&&>(_parse),
        static_cast<std::function<std::basic_string<Element>(
            const Carrier&)>&&>(_compose));
}

// make_prism_via_traits
//   factory: build a prism whose compose leg is the compose_traits
// specialisation registered for Carrier.  Compile error if no
// such specialisation exists.
template<typename Carrier,
         typename Element = char>
D_NODISCARD
prism<Carrier, Element>
make_prism_via_traits(
    parser<Carrier, Element> _parse
)
{
    static_assert(
        is_composable<Carrier, Element>::value,
        "make_prism_via_traits: Carrier needs a compose_traits "
        "specialisation.");

    using surface_type = std::basic_string<Element>;

    return prism<Carrier, Element>(
        static_cast<parser<Carrier, Element>&&>(_parse),
        [](const Carrier& _t) -> surface_type
        {
            // the compose leg is cata[φ] over recursive_traits<
            // Carrier> with φ = compose_traits<Carrier>::algebra
            return ::djinterp::parse::compose<Carrier, Element>(
                _t);
        });
}


// ================================================================
//  IV.  dimap  —  profunctor transport
// ================================================================
//   prism<C, E> is a profunctor over its carrier C — covariant on
// the parse side (post-applied), contravariant on the compose side
// (pre-applied).  The profunctor_traits<prism<C, E>> specialisation
// (at djinterp:: scope below) is the protocol obligation; the
// local `dimap` here is a thin convenience that delegates to the
// protocol, so call sites read as the prism literature does
// (`dimap(p, f, g)` with f the iso forward and g the inverse).

// dimap
//   function: transports a prism across an isomorphism on the
// carrier side.  Delegates to profunctor_traits<prism<C, E>>::
// dimap; included here so a downstream module reading prism.hpp
// finds the operation without an extra include of profunctor.hpp.
//
//   The profunctor law — covariance on parse, contravariance on
// compose — is what makes the signature read as `dimap(p, f, g)`:
// f flows in the parse direction (post-applied), g in the compose
// direction (pre-applied).  If Iso = (f : C → D, g : D → C) is a
// bijection then dimap(p, f, g) is the prism over D.
//
//   Carrier    the underlying parsable type.
//   Other      the carrier the new prism produces / consumes.
//   Element    the surface element type.
template<typename Carrier,
         typename Other,
         typename Element,
         typename Forward,
         typename Backward>
D_NODISCARD
prism<Other, Element>
dimap(
    const prism<Carrier, Element>& _p,
    Forward                          _f,
    Backward                         _g
)
{
    // profunctor_traits expects (pre, post); we receive (post, pre)
    // in the prism order.  Reorder at the boundary.
    return ::djinterp::profunctor_traits<
               prism<Carrier, Element>
           >::dimap(_p, _g, _f);
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
// or the round trip changed the value.  Carrier must support
// operator== for the equality check.
template<typename Carrier,
         typename Element>
D_NODISCARD
bool
check_compose_parse(
    const prism<Carrier, Element>& _p,
    const Carrier&                  _t
)
{
    std::basic_string<Element> surface = _p.compose(_t);

    parse_state<Element> state(
        surface.data(), surface.size(), 0);

    parse_result<Carrier> r = _p.parse(state);

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
template<typename Carrier,
         typename Element>
D_NODISCARD
bool
check_parse_compose(
    const prism<Carrier, Element>& _p,
    const std::basic_string<Element>& _s
)
{
    parse_state<Element> state(_s.data(), _s.size(), 0);

    parse_result<Carrier> r = _p.parse(state);

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


// ================================================================
//  profunctor_traits<prism<Carrier, Element>>
// ================================================================
//   Lives at djinterp:: scope — the same namespace as the primary
// template in functional/profunctor.hpp.  The prism is the rare
// case where the two profunctor parameters coincide (A = B = the
// carrier), and dimap transports both via the iso pair.

template<typename Carrier,
         typename Element>
struct profunctor_traits<parse::prism<Carrier, Element>, void>
{
    using is_specialized = std::true_type;

    // dimap
    //   contracts pre : D → Carrier (compose direction) and post
    // : Carrier → D (parse direction) into a prism<D, Element>.
    template<typename Pre,
             typename Post>
    static
    auto dimap(
        const parse::prism<Carrier, Element>& _p,
        Pre                                     _pre,
        Post                                    _post
    )
    -> parse::prism<
           typename std::decay<decltype(
               _post(std::declval<Carrier>()))>::type,
           Element>
    {
        using d_type =
            typename std::decay<decltype(
                _post(std::declval<Carrier>()))>::type;
        using state_type   = parse::parse_state<Element>;
        using surface_type = std::basic_string<Element>;
        using parser_type  = parse::parser<d_type, Element>;
        using compose_fn   =
            std::function<surface_type(const d_type&)>;

        parser_type lifted_parse(
            [_p, _post](state_type& _state)
                -> parse::parse_result<d_type>
            {
                parse::parse_result<Carrier> r = _p.parse(_state);

                if (!r.ok())
                {
                    return parse::parse_result<d_type>(r.error());
                }

                return parse::parse_result<d_type>(
                    _post(r.value()));
            });

        compose_fn lifted_compose =
            [_p, _pre](const d_type& _d) -> surface_type
            {
                return _p.compose(_pre(_d));
            };

        return parse::prism<d_type, Element>(
            static_cast<parser_type&&>(lifted_parse),
            static_cast<compose_fn&&>(lifted_compose));
    }
};


NS_END  // djinterp


NS_DJINTERP
NS_PARSE


// (parse:: namespace is reopened so a downstream include sees it
// in the expected scope; nothing further is declared in this file.)


NS_END  // parse
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_PRISM_HPP
