/*******************************************************************************
* djinterp [parse]                                                      free.hpp
*
* The three strata — Applicative, Selective, Monad — as free constructions.
*   Per ch-parsing.tex the expressive power of a parser can be dialled
* along three free constructions over the polynomial functor F:
*
*       FreeAp  F A     Applicative — pure and ap only.  No branching
*                       on parsed values; the whole computation is
*                       data and inspectable.
*       FreeSel F A     Selective — adds static branching via select
*                       on an Either-typed discriminant.
*       Free    F A     Monad — full bind; value-driven branching.
*
*   The monadic stratum is `functional::free<parser_layer<E>::at, R>`
* — the framework's canonical free-monad construction with F set to
* the parser-layer functor.  The applicative and selective strata are
* parser-flavoured wrappers because the functional companion does not
* (yet) carry free_applicative or free_selective; they will be
* re-pointed at functional types when those land.  All three wrappers
* are parser_exprs so they slot into CRTP composition in
* combinators.hpp.
*
*   PARSER LAYER.  parser_layer<E>::template at<X> = parser<X, E> —
* the single-template-arg functor for use as the F parameter to
* functional::free.  Each F-layer of a parsing_program holds a
* parser of the continuation, so a Roll node says "run this parser,
* take its result, and continue with the resulting sub-program".
* lift_free turns one atomic parser into a one-layer program;
* fold_free interprets a multi-layer program against an algebra,
* and to_parser specialises that algebra to "concatenate the
* parsers" — recovering a parser<R, E>.
*
*   INSPECTION.  parsing_program is a heap-allocated tree (the
* recursive subtrees live behind shared_ptr per functional::free).
* Its shape is amenable to any fold_free-shaped interpretation —
* parse, pretty-print, render-to-grammar-doc, static-analysis,
* trace.  free_parser caches one canonical interpretation (the
* parser) for the hot path; the program itself stays available via
* program() for the other faces.
*
* CONTENTS
*   I.    parser_layer<E>::template at        F as a template-template
*                                             parameter for functional::free
*   II.   parsing_program<R, E>               functional::free<...> alias
*   III.  lift_to_program  /  to_parser       construction / interpretation
*   IV.   free_parser<R, E>                   monadic stratum wrapper
*           free_pure(v) / free_lift(atom) / free_bind(p, f) / free_map(p, f)
*   V.    free_ap_parser<R, E>                applicative stratum wrapper
*           ap_pure(v) / ap_lift(atom)
*           ap_apply(pf, pa) / ap_map(pa, f)
*   VI.   free_sel_parser<R, E>               selective stratum wrapper
*           sel_pure(v) / sel_lift(atom)
*           sel_select(p_either, p_handler)
*           sel_branch(p_either, p_left, p_right)
*   VII.  Cross-stratum lifts
*           ap_to_sel / sel_to_monad / ap_to_monad
*
*
* path:      /inc/djinterp/parse/parser/free.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSER_FREE_HPP
#define DJINTERP_PARSE_PARSER_FREE_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

// the free-monad parser is built on core/functional/free.hpp, which is
// C++17; below C++17 this header is absent too (degrade, never error)
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../core/meta/type_utility.hpp"  // clean_t
#include "../../core/functional/result.hpp"
#include "../../core/functional/monad.hpp"
#include "../../core/functional/free.hpp"
#include "../selective.hpp"
#include "../parse.hpp"
#include "parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   parser_layer
// ================================================================

// parser_layer
//   meta: namespace holding the single-template-arg functor used
// as the F parameter to functional::free for parsing programs.
//   parser_layer<E>::template at<X> aliases to parser<X, E> — the
// type-erased parser handle, which is a registered Functor via
// the monad_traits / monad bridge in parser.hpp.  This is the
// canonical Σ-side polynomial functor for the parsing module: each
// F-layer is one atomic parser action.
//
//   The E template parameter is held in the enclosing struct so
// that `parser_layer<E>::template at` is itself a single-argument
// template-template parameter — exactly what functional::free
// requires.
template<typename E>
struct parser_layer
{
    template<typename X>
    using at = parser<X, E>;
};


// ================================================================
//  II.  parsing_program
// ================================================================

// parsing_program
//   alias: the formal free monad over the parser_layer functor.
// A value of this type is a tree of atomic parser actions (Roll
// nodes) ending in Pure leaves of type R; it can be inspected,
// optimised, or interpreted with fold_free into any algebra.  The
// canonical interpretation — fold-into-parser — is the to_parser
// function below; free_parser wraps a parsing_program plus its
// cached interpretation behind the parser_expr CRTP face.
//
//   This is `functional::free<F, R>` with F bound: same monadic
// surface, same lift_free / fold_free interface, same
// monad_traits / functor_traits registration — so monad_bind,
// monad_map, kleisli_compose, and the operator| pipeline all work
// on a parsing_program directly.
template<typename R,
         typename E = char>
using parsing_program =
    ::djinterp::free<parser_layer<E>::template at, R>;


// ================================================================
//  III. construction / interpretation
// ================================================================

// lift_to_program
//   function: lifts a single atomic parser into a parsing_program.
// One F-layer with Pure leaves at the result positions; the
// underlying construction is functional::lift_free specialised to
// the parser layer.
//
//   Any parser-flavoured value (a parser_expr-derived class or the
// erased parser<R, E> handle) is accepted; the SFINAE guard
// ensures the result type is well-typed.
template<typename Parser>
D_NODISCARD
auto lift_to_program(
    Parser _p
)
-> parsing_program<typename clean_t<Parser>::result_type,
                   typename clean_t<Parser>::input_type>
{
    static_assert(is_parser<Parser>::value,
                  "lift_to_program: argument must be a parser");

    using result_t  = typename clean_t<Parser>::result_type;
    using element_t = typename clean_t<Parser>::input_type;

    // Erase to the handle so the layer's F-type matches
    // parser_layer<element_t>::template at<result_t>.
    parser<result_t, element_t> handle(
        static_cast<Parser&&>(_p));

    return ::djinterp::lift_free<
        parser_layer<element_t>::template at,
        result_t>(handle);
}


// to_parser
//   function: interprets a parsing_program into a parser<R, E> by
// directly walking the tree at parse time — an iterative descent
// through Roll nodes, terminating at a Pure leaf.
//
//   The straightforward fold_free implementation builds nested
// closures (one per Roll layer), so a parse call walks through N
// layers of std::function indirection.  Since each F-layer for
// parser_layer is *linear* — a Roll's parser returns one shared_ptr
// to the next node — we can flatten the descent into a while loop:
// one closure for the whole program, N+1 parser invocations and
// N shared_ptr hops at parse time.  This is the operational form
// of the "free monad over a monad reduces to the monad" identity,
// recovered without going through the fold_free closure tower.
//
//   The parse_state is threaded through each layer's parser; offset
// is advanced as in any composed parser.  An error from any layer
// short-circuits the descent.  The shared_ptr is held in a local
// across iterations so each level survives long enough for the
// next one's parser to be reached.
template<typename R,
         typename E>
D_NODISCARD
parser<R, E>
to_parser(
    const parsing_program<R, E>& _program
)
{
    using state_type  = parse_state<E>;
    using output_type = parse_result<R>;
    using node_type   = parsing_program<R, E>;
    using child_ptr   = std::shared_ptr<node_type>;
    using layer_t     = typename node_type::layer_type;

    return parser<R, E>(
        [_program](state_type& _state) -> output_type
        {
            // First step: the program is held by the closure as a
            // value; descend from it.  After the first hop we hold
            // each level by shared_ptr to keep it alive across the
            // loop iteration.
            if (_program.is_pure())
            {
                return output_type(_program.pure_value());
            }

            const layer_t& root_layer = _program.layer();

            parse_result<child_ptr> first =
                root_layer.parse(_state);

            if (!first.ok())
            {
                return output_type(first.error());
            }

            child_ptr current = first.value();

            while (true)
            {
                if (current->is_pure())
                {
                    return output_type(current->pure_value());
                }

                parse_result<child_ptr> step =
                    current->layer().parse(_state);

                if (!step.ok())
                {
                    return output_type(step.error());
                }

                // shared_ptr assignment keeps the new level alive;
                // the previous current is released, which is safe
                // because its only purpose was to produce the new
                // shared_ptr we now hold.
                current = step.value();
            }
        });
}


// ================================================================
//  IV.  free_parser  —  monadic stratum
// ================================================================

// free_parser
//   class: the monadic stratum's parser_expr face.  Carries a
// parsing_program (the formal free-monad construction) for
// inspection and a cached parser<R, E> (its canonical
// interpretation) for the hot path.  Calling parse_impl uses the
// cached interpretation; program() exposes the underlying
// parsing_program for any non-parse fold.
//
//   The cache is built eagerly at construction so the hot-path
// cost is one std::function indirection — no per-call fold_free.
// If a different interpretation is wanted, fold_free or any other
// algebra can be run over program() directly.
template<typename R,
         typename E = char>
class free_parser
    : public parser_expr<free_parser<R, E>>
{
public:
    using input_type   = E;
    using element_type = E;
    using result_type  = R;
    using value_type   = R;
    using state_type   = parse_state<E>;
    using output_type  = parse_result<R>;
    using program_type = parsing_program<R, E>;

    // free_parser (default)
    //   constructor: an uninitialised free_parser whose program
    // always fails when interpreted.  Does not require R to be
    // default-constructible — the underlying fail parser never
    // produces an R value.
    free_parser()
        : m_program(m_make_fail_program()),
          m_parser ()
    {
        m_parser = to_parser(m_program);
    }

    explicit free_parser(
        program_type _program
    )
        : m_program(static_cast<program_type&&>(_program)),
          m_parser ()
    {
        m_parser = to_parser(m_program);
    }

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return m_parser.parse(_state);
    }

    // program
    //   accessor: the underlying free-monad construction, for
    // inspection and alternative interpretations.
    D_NODISCARD
    const program_type&
    program() const D_NOEXCEPT
    {
        return m_program;
    }

    // inner
    //   accessor: the cached interpretation as a parser handle.
    // Provided for symmetry with the applicative / selective
    // wrappers' inner().
    D_NODISCARD
    const parser<R, E>&
    inner() const D_NOEXCEPT
    {
        return m_parser;
    }

private:
    // m_make_fail_program
    //   helper: builds a one-layer parsing_program whose underlying
    // parser always fails.  Used by the default constructor so an
    // uninitialised free_parser never requires R to be default-
    // constructible.
    static program_type
    m_make_fail_program()
    {
        using state_type  = parse_state<E>;
        using output_type = parse_result<R>;

        parser<R, E> fail_p(
            [](state_type& _state) -> output_type
            {
                return output_type::make_error(
                    DParseStatusFailure,
                    _state.offset,
                    "free_parser: uninitialised");
            });

        return ::djinterp::lift_free<
            parser_layer<E>::template at,
            R>(fail_p);
    }

    program_type   m_program;
    parser<R, E> m_parser;
};


// free_pure
//   factory: monadic pure.  Lifts a value into a free_parser as a
// Pure leaf — the simplest parsing_program.
template<typename R,
         typename E = char>
D_NODISCARD
free_parser<R, E>
free_pure(
    R _value
)
{
    return free_parser<R, E>(
        parsing_program<R, E>::pure(
            static_cast<R&&>(_value)));
}


// free_lift
//   factory: lifts an atomic parser into a free_parser as one
// F-layer over Pure leaves.  This is the parser-side counterpart
// of lift_to_program — same operation, returns the wrapper.
template<typename Parser>
D_NODISCARD
auto free_lift(
    Parser _p
)
-> free_parser<typename clean_t<Parser>::result_type,
               typename clean_t<Parser>::input_type>
{
    using result_t  = typename clean_t<Parser>::result_type;
    using element_t = typename clean_t<Parser>::input_type;

    return free_parser<result_t, element_t>(
        lift_to_program(static_cast<Parser&&>(_p)));
}


// free_bind
//   factory: monadic bind on the parsing_program — runs the
// program, feeds the resulting value into _f to obtain a new
// program, splices that in.  Delegates to monad_bind from the
// functional companion since parsing_program is a registered
// monad.
template<typename R,
         typename E,
         typename F>
D_NODISCARD
auto free_bind(
    const free_parser<R, E>& _p,
    F                          _f
)
-> free_parser<
       typename std::decay<decltype(
           _f(std::declval<R>()))>::type::result_type,
       E>
{
    using next_parser_t =
        typename std::decay<decltype(
            _f(std::declval<R>()))>::type;
    using next_result_t =
        typename next_parser_t::result_type;

    // Build the continuation on the program side: A → program<U, E>.
    auto program_cont = [_f](const R& _v)
        -> parsing_program<next_result_t, E>
    {
        next_parser_t inner = _f(_v);
        return inner.program();
    };

    parsing_program<next_result_t, E> next_program =
        ::djinterp::monad_bind(_p.program(), program_cont);

    return free_parser<next_result_t, E>(
        static_cast<parsing_program<next_result_t, E>&&>(
            next_program));
}


// free_map
//   factory: Functor map at the monadic stratum.  Delegates to
// monad_map on the underlying parsing_program (functor via the
// monad bridge), so the resulting program's structure is
// preserved up to a re-mapped Pure leaf type.
template<typename R,
         typename E,
         typename F>
D_NODISCARD
auto free_map(
    const free_parser<R, E>& _p,
    F                          _f
)
-> free_parser<
       typename std::decay<decltype(
           _f(std::declval<R>()))>::type,
       E>
{
    using out_t =
        typename std::decay<decltype(
            _f(std::declval<R>()))>::type;

    parsing_program<out_t, E> mapped =
        ::djinterp::monad_map(_p.program(), _f);

    return free_parser<out_t, E>(
        static_cast<parsing_program<out_t, E>&&>(mapped));
}


// ================================================================
//  V.   free_ap_parser  —  applicative stratum
// ================================================================
//   Parser-flavoured wrapper.  The functional companion does not
// (yet) carry a free_applicative<F, A>; when it does, this class
// will be re-pointed to wrap that and use its protocol surface,
// the same way free_parser wraps functional::free.  The combinator
// surface here matches what free_applicative would expose.

// free_ap_parser
//   class: the applicative-only stratum.  Holds an underlying
// parser<R, E>; the restriction is on the combinator surface.
template<typename R,
         typename E = char>
class free_ap_parser
    : public parser_expr<free_ap_parser<R, E>>
{
public:
    using input_type   = E;
    using element_type = E;
    using result_type  = R;
    using value_type   = R;
    using state_type   = parse_state<E>;
    using output_type  = parse_result<R>;

    free_ap_parser()
        : m_inner()
    {}

    explicit free_ap_parser(
        parser<R, E> _inner
    )
        : m_inner(static_cast<parser<R, E>&&>(_inner))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return m_inner.parse(_state);
    }

    D_NODISCARD
    const parser<R, E>&
    inner() const D_NOEXCEPT
    {
        return m_inner;
    }

private:
    parser<R, E> m_inner;
};


// ap_pure
//   factory: pure of the FreeAp applicative.
template<typename R,
         typename E = char>
D_NODISCARD
free_ap_parser<R, E>
ap_pure(
    R _value
)
{
    using state_type  = parse_state<E>;
    using output_type = parse_result<R>;

    parser<R, E> inner(
        [_value](state_type& /*_state*/) -> output_type
        {
            return output_type(_value);
        });

    return free_ap_parser<R, E>(
        static_cast<parser<R, E>&&>(inner));
}


// ap_lift
//   factory: lifts any atomic parser_expr into the applicative
// stratum.
template<typename Atom>
D_NODISCARD
free_ap_parser<
    typename clean_t<Atom>::result_type,
    typename clean_t<Atom>::input_type>
ap_lift(
    Atom _atom
)
{
    static_assert(is_parser<Atom>::value,
                  "ap_lift: argument must be a parser expression");

    using result_t  = typename clean_t<Atom>::result_type;
    using element_t = typename clean_t<Atom>::input_type;

    return free_ap_parser<result_t, element_t>(
        parser<result_t, element_t>(static_cast<Atom&&>(_atom)));
}


// ap_apply
//   factory: applicative apply.  Runs _pf, then _pa, then applies
// the former to the latter.  Both run in sequence, threading the
// residual.
template<typename Pf,
         typename Pa>
D_NODISCARD
auto ap_apply(
    Pf _pf,
    Pa _pa
)
-> free_ap_parser<
       typename std::decay<decltype(
           std::declval<typename Pf::result_type>()(
               std::declval<typename Pa::result_type>()))>::type,
       typename Pf::input_type>
{
    using fn_t      = typename Pf::result_type;
    using arg_t     = typename Pa::result_type;
    using out_t     =
        typename std::decay<decltype(
            std::declval<fn_t>()(
                std::declval<arg_t>()))>::type;
    using element_t = typename Pf::input_type;
    using state_type  = parse_state<element_t>;
    using output_type = parse_result<out_t>;

    static_assert(
        std::is_same<typename Pf::input_type,
                     typename Pa::input_type>::value,
        "ap_apply: branches must share input_type");

    parser<out_t, element_t> inner(
        [_pf, _pa](state_type& _state) -> output_type
        {
            parse_result<fn_t> rf = _pf.parse(_state);

            if (!rf.ok())
            {
                return output_type(rf.error());
            }

            parse_result<arg_t> ra = _pa.parse(_state);

            if (!ra.ok())
            {
                return output_type(ra.error());
            }

            return output_type(rf.value()(ra.value()));
        });

    return free_ap_parser<out_t, element_t>(
        static_cast<parser<out_t, element_t>&&>(inner));
}


// ap_map
//   factory: applicative map — equivalent to ap_apply(ap_pure(f), p)
// up to the constant-fn indirection.
template<typename Pa,
         typename F>
D_NODISCARD
auto ap_map(
    Pa _pa,
    F   _f
)
-> free_ap_parser<
       typename std::decay<decltype(
           _f(std::declval<typename Pa::result_type>()))>::type,
       typename Pa::input_type>
{
    using arg_t     = typename Pa::result_type;
    using out_t     =
        typename std::decay<decltype(
            _f(std::declval<arg_t>()))>::type;
    using element_t = typename Pa::input_type;
    using state_type  = parse_state<element_t>;
    using output_type = parse_result<out_t>;

    parser<out_t, element_t> inner(
        [_pa, _f](state_type& _state) -> output_type
        {
            parse_result<arg_t> r = _pa.parse(_state);

            if (!r.ok())
            {
                return output_type(r.error());
            }

            return output_type(_f(r.value()));
        });

    return free_ap_parser<out_t, element_t>(
        static_cast<parser<out_t, element_t>&&>(inner));
}


// ================================================================
//  VI.  free_sel_parser  —  selective stratum
// ================================================================
//   Parser-flavoured wrapper.  Built on top of the functional
// companion's Selective protocol (functional/selective.hpp) via
// the monad bridge: parser<R, E> is a Monad, every Monad is a
// Selective, so sel_select and sel_branch delegate to the
// protocol's selective_select and selective_branch directly.
//
//   The Either type is functional::either<L, R>; sel_either_t is
// retained as a back-compatible alias.  A formal
// free_selective<F, A> over a polynomial functor F is future
// work in the functional layer; when it lands free_sel_parser
// re-points to wrap it the same way free_parser wraps
// functional::free.


// sel_either_t
//   alias: convenience for the functional Either at this stratum.
// Sources from the framework's Selective protocol rather than a
// local definition so a select / branch built against the
// protocol vocabulary slots straight in.
template<typename L,
         typename R>
using sel_either_t = ::djinterp::either<L, R>;


// free_sel_parser
//   class: the selective stratum.  Adds `select` to the
// applicative vocabulary; branches whose discriminant is a parsed
// value, but where the branches themselves are static.
template<typename R,
         typename E = char>
class free_sel_parser
    : public parser_expr<free_sel_parser<R, E>>
{
public:
    using input_type   = E;
    using element_type = E;
    using result_type  = R;
    using value_type   = R;
    using state_type   = parse_state<E>;
    using output_type  = parse_result<R>;

    free_sel_parser()
        : m_inner()
    {}

    explicit free_sel_parser(
        parser<R, E> _inner
    )
        : m_inner(static_cast<parser<R, E>&&>(_inner))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return m_inner.parse(_state);
    }

    D_NODISCARD
    const parser<R, E>&
    inner() const D_NOEXCEPT
    {
        return m_inner;
    }

private:
    parser<R, E> m_inner;
};


// sel_pure
//   factory: pure of the selective.
template<typename R,
         typename E = char>
D_NODISCARD
free_sel_parser<R, E>
sel_pure(
    R _value
)
{
    using state_type  = parse_state<E>;
    using output_type = parse_result<R>;

    parser<R, E> inner(
        [_value](state_type& /*_state*/) -> output_type
        {
            return output_type(_value);
        });

    return free_sel_parser<R, E>(
        static_cast<parser<R, E>&&>(inner));
}


// sel_lift
//   factory: lifts any parser_expr into the selective stratum.
template<typename Atom>
D_NODISCARD
free_sel_parser<
    typename clean_t<Atom>::result_type,
    typename clean_t<Atom>::input_type>
sel_lift(
    Atom _atom
)
{
    static_assert(is_parser<Atom>::value,
                  "sel_lift: argument must be a parser expression");

    using result_t  = typename clean_t<Atom>::result_type;
    using element_t = typename clean_t<Atom>::input_type;

    return free_sel_parser<result_t, element_t>(
        parser<result_t, element_t>(static_cast<Atom&&>(_atom)));
}


// sel_select
//   factory: selective `select`.  Delegates to the functional
// companion's selective_select via the monad bridge — parser<R, E>
// participates as a Selective automatically because it is a Monad.
// The handler-effect type is parser<std::function<R(L)>, E>; the
// discriminant-effect type is parser<either<L, R>, E>.
template<typename L,
         typename R,
         typename E,
         typename PEither,
         typename PHandler>
D_NODISCARD
free_sel_parser<R, E>
sel_select(
    PEither   _p_either,
    PHandler _p_handler
)
{
    static_assert(is_parser<PEither>::value &&
                  is_parser<PHandler>::value,
                  "sel_select: both arguments must be parsers");

    using either_t = sel_either_t<L, R>;

    // Erase the inputs to the handle types the selective protocol
    // expects.  parser<either_t, E> is the discriminant; parser<
    // std::function<R(L)>, E> is the handler.
    parser<either_t, E>                       disc(_p_either);
    parser<std::function<R(L)>, E>          handler(_p_handler);

    parser<R, E> inner =
        ::djinterp::selective_select(disc, handler);

    return free_sel_parser<R, E>(
        static_cast<parser<R, E>&&>(inner));
}


// sel_branch
//   factory: selective `branch` — given Either<L, R> and two
// handlers, run the appropriate one.  Delegates to
// selective_branch via the monad bridge.
template<typename L,
         typename R,
         typename Out,
         typename E,
         typename PEither,
         typename PLeft,
         typename PRight>
D_NODISCARD
free_sel_parser<Out, E>
sel_branch(
    PEither _p_either,
    PLeft    _p_left,
    PRight   _p_right
)
{
    static_assert(is_parser<PEither>::value &&
                  is_parser<PLeft>::value   &&
                  is_parser<PRight>::value,
                  "sel_branch: all three arguments must be parsers");

    using either_t = sel_either_t<L, R>;

    parser<either_t, E>                        disc(_p_either);
    parser<std::function<Out(L)>, E>         fl(_p_left);
    parser<std::function<Out(R)>, E>         fr(_p_right);

    parser<Out, E> inner =
        ::djinterp::selective_branch(disc, fl, fr);

    return free_sel_parser<Out, E>(
        static_cast<parser<Out, E>&&>(inner));
}


// ================================================================
//  VII. Cross-stratum lifts
// ================================================================
//   The strata form a chain
//
//       FreeAp F A  ⊂  FreeSel F A  ⊂  Free F A
//
// — every applicative computation is a selective one, every
// selective is monadic.  Descent is impossible by design: once a
// program has used a stratum-specific operator, it can no longer
// be analysed at a lower stratum.
//
//   For ap_to_monad and sel_to_monad, the underlying parser
// handle is lifted as a single F-layer into the canonical
// parsing_program — the resulting free_parser is a one-layer
// free-monad construction over the same atomic action.

// ap_to_sel
//   function: promotes a FreeAp parser to a FreeSel parser.
template<typename R,
         typename E>
D_NODISCARD
free_sel_parser<R, E>
ap_to_sel(
    const free_ap_parser<R, E>& _p
)
{
    return free_sel_parser<R, E>(_p.inner());
}

// sel_to_monad
//   function: promotes a FreeSel parser to a Free (monadic)
// parser, lifting the underlying handle into one F-layer of the
// canonical free-monad construction.
template<typename R,
         typename E>
D_NODISCARD
free_parser<R, E>
sel_to_monad(
    const free_sel_parser<R, E>& _p
)
{
    return free_parser<R, E>(
        lift_to_program(_p.inner()));
}

// ap_to_monad
//   function: composition of the two lifts — promote a FreeAp
// directly to a Free.
template<typename R,
         typename E>
D_NODISCARD
free_parser<R, E>
ap_to_monad(
    const free_ap_parser<R, E>& _p
)
{
    return free_parser<R, E>(
        lift_to_program(_p.inner()));
}


NS_END  // parse
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_PARSE_PARSER_FREE_HPP
