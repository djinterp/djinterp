/******************************************************************************
* djinterp [parse]                                              parser/free.hpp
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
* path:      /inc/djinterp/parse/parser/free.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_PARSE_PARSER_FREE_
#define DJINTERP_PARSE_PARSER_FREE_ 1

// std
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/functional/result.hpp"
#include "../../core/functional/monad.hpp"
#include "../../core/functional/free.hpp"
#include "../../core/functional/selective.hpp"
#include "../parse.hpp"
#include "./parser.hpp"


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
template<typename _E>
struct parser_layer
{
    template<typename _X>
    using at = parser<_X, _E>;
};


// ================================================================
//  II.  parsing_program
// ================================================================

// parsing_program
//   alias: the formal free monad over the parser_layer functor.
// A value of this type is a tree of atomic parser actions (Roll
// nodes) ending in Pure leaves of type _R; it can be inspected,
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
template<typename _R,
         typename _E = char>
using parsing_program =
    ::djinterp::free<parser_layer<_E>::template at, _R>;


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
template<typename _Parser>
D_NODISCARD
auto lift_to_program(
    _Parser _p
)
-> parsing_program<typename clean_t<_Parser>::result_type,
                   typename clean_t<_Parser>::input_type>
{
    static_assert(is_parser<_Parser>::value,
                  "lift_to_program: argument must be a parser");

    using result_t  = typename clean_t<_Parser>::result_type;
    using element_t = typename clean_t<_Parser>::input_type;

    // Erase to the handle so the layer's F-type matches
    // parser_layer<element_t>::template at<result_t>.
    parser<result_t, element_t> handle(
        static_cast<_Parser&&>(_p));

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
template<typename _R,
         typename _E>
D_NODISCARD
parser<_R, _E>
to_parser(
    const parsing_program<_R, _E>& _program
)
{
    using state_type  = parse_state<_E>;
    using output_type = parse_result<_R>;
    using node_type   = parsing_program<_R, _E>;
    using child_ptr   = std::shared_ptr<node_type>;
    using layer_t     = typename node_type::layer_type;

    return parser<_R, _E>(
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
template<typename _R,
         typename _E = char>
class free_parser
    : public parser_expr<free_parser<_R, _E>>
{
public:
    using input_type   = _E;
    using element_type = _E;
    using result_type  = _R;
    using value_type   = _R;
    using state_type   = parse_state<_E>;
    using output_type  = parse_result<_R>;
    using program_type = parsing_program<_R, _E>;

    // free_parser (default)
    //   constructor: an uninitialised free_parser whose program
    // always fails when interpreted.  Does not require _R to be
    // default-constructible — the underlying fail parser never
    // produces an _R value.
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
    const parser<_R, _E>&
    inner() const D_NOEXCEPT
    {
        return m_parser;
    }

private:
    // m_make_fail_program
    //   helper: builds a one-layer parsing_program whose underlying
    // parser always fails.  Used by the default constructor so an
    // uninitialised free_parser never requires _R to be default-
    // constructible.
    static program_type
    m_make_fail_program()
    {
        using state_type  = parse_state<_E>;
        using output_type = parse_result<_R>;

        parser<_R, _E> fail_p(
            [](state_type& _state) -> output_type
            {
                return output_type::make_error(
                    DParseStatusFailure,
                    _state.offset,
                    "free_parser: uninitialised");
            });

        return ::djinterp::lift_free<
            parser_layer<_E>::template at,
            _R>(fail_p);
    }

    program_type   m_program;
    parser<_R, _E> m_parser;
};


// free_pure
//   factory: monadic pure.  Lifts a value into a free_parser as a
// Pure leaf — the simplest parsing_program.
template<typename _R,
         typename _E = char>
D_NODISCARD
free_parser<_R, _E>
free_pure(
    _R _value
)
{
    return free_parser<_R, _E>(
        parsing_program<_R, _E>::pure(
            static_cast<_R&&>(_value)));
}


// free_lift
//   factory: lifts an atomic parser into a free_parser as one
// F-layer over Pure leaves.  This is the parser-side counterpart
// of lift_to_program — same operation, returns the wrapper.
template<typename _Parser>
D_NODISCARD
auto free_lift(
    _Parser _p
)
-> free_parser<typename clean_t<_Parser>::result_type,
               typename clean_t<_Parser>::input_type>
{
    using result_t  = typename clean_t<_Parser>::result_type;
    using element_t = typename clean_t<_Parser>::input_type;

    return free_parser<result_t, element_t>(
        lift_to_program(static_cast<_Parser&&>(_p)));
}


// free_bind
//   factory: monadic bind on the parsing_program — runs the
// program, feeds the resulting value into _f to obtain a new
// program, splices that in.  Delegates to monad_bind from the
// functional companion since parsing_program is a registered
// monad.
template<typename _R,
         typename _E,
         typename _F>
D_NODISCARD
auto free_bind(
    const free_parser<_R, _E>& _p,
    _F                         _f
)
-> free_parser<
       typename std::decay<decltype(
           _f(std::declval<_R>()))>::type::result_type,
       _E>
{
    using next_parser_t =
        typename std::decay<decltype(
            _f(std::declval<_R>()))>::type;
    using next_result_t =
        typename next_parser_t::result_type;

    // Build the continuation on the program side: A → program<U, E>.
    auto program_cont = [_f](const _R& _v)
        -> parsing_program<next_result_t, _E>
    {
        next_parser_t inner = _f(_v);
        return inner.program();
    };

    parsing_program<next_result_t, _E> next_program =
        ::djinterp::monad_bind(_p.program(), program_cont);

    return free_parser<next_result_t, _E>(
        static_cast<parsing_program<next_result_t, _E>&&>(
            next_program));
}


// free_map
//   factory: Functor map at the monadic stratum.  Delegates to
// monad_map on the underlying parsing_program (functor via the
// monad bridge), so the resulting program's structure is
// preserved up to a re-mapped Pure leaf type.
template<typename _R,
         typename _E,
         typename _F>
D_NODISCARD
auto free_map(
    const free_parser<_R, _E>& _p,
    _F                         _f
)
-> free_parser<
       typename std::decay<decltype(
           _f(std::declval<_R>()))>::type,
       _E>
{
    using out_t =
        typename std::decay<decltype(
            _f(std::declval<_R>()))>::type;

    parsing_program<out_t, _E> mapped =
        ::djinterp::monad_map(_p.program(), _f);

    return free_parser<out_t, _E>(
        static_cast<parsing_program<out_t, _E>&&>(mapped));
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
template<typename _R,
         typename _E = char>
class free_ap_parser
    : public parser_expr<free_ap_parser<_R, _E>>
{
public:
    using input_type   = _E;
    using element_type = _E;
    using result_type  = _R;
    using value_type   = _R;
    using state_type   = parse_state<_E>;
    using output_type  = parse_result<_R>;

    free_ap_parser()
        : m_inner()
    {}

    explicit free_ap_parser(
        parser<_R, _E> _inner
    )
        : m_inner(static_cast<parser<_R, _E>&&>(_inner))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return m_inner.parse(_state);
    }

    D_NODISCARD
    const parser<_R, _E>&
    inner() const D_NOEXCEPT
    {
        return m_inner;
    }

private:
    parser<_R, _E> m_inner;
};


// ap_pure
//   factory: pure of the FreeAp applicative.
template<typename _R,
         typename _E = char>
D_NODISCARD
free_ap_parser<_R, _E>
ap_pure(
    _R _value
)
{
    using state_type  = parse_state<_E>;
    using output_type = parse_result<_R>;

    parser<_R, _E> inner(
        [_value](state_type& /*_state*/) -> output_type
        {
            return output_type(_value);
        });

    return free_ap_parser<_R, _E>(
        static_cast<parser<_R, _E>&&>(inner));
}


// ap_lift
//   factory: lifts any atomic parser_expr into the applicative
// stratum.
template<typename _Atom>
D_NODISCARD
free_ap_parser<
    typename clean_t<_Atom>::result_type,
    typename clean_t<_Atom>::input_type>
ap_lift(
    _Atom _atom
)
{
    static_assert(is_parser<_Atom>::value,
                  "ap_lift: argument must be a parser expression");

    using result_t  = typename clean_t<_Atom>::result_type;
    using element_t = typename clean_t<_Atom>::input_type;

    return free_ap_parser<result_t, element_t>(
        parser<result_t, element_t>(static_cast<_Atom&&>(_atom)));
}


// ap_apply
//   factory: applicative apply.  Runs _pf, then _pa, then applies
// the former to the latter.  Both run in sequence, threading the
// residual.
template<typename _Pf,
         typename _Pa>
D_NODISCARD
auto ap_apply(
    _Pf _pf,
    _Pa _pa
)
-> free_ap_parser<
       typename std::decay<decltype(
           std::declval<typename _Pf::result_type>()(
               std::declval<typename _Pa::result_type>()))>::type,
       typename _Pf::input_type>
{
    using fn_t      = typename _Pf::result_type;
    using arg_t     = typename _Pa::result_type;
    using out_t     =
        typename std::decay<decltype(
            std::declval<fn_t>()(
                std::declval<arg_t>()))>::type;
    using element_t = typename _Pf::input_type;
    using state_type  = parse_state<element_t>;
    using output_type = parse_result<out_t>;

    static_assert(
        std::is_same<typename _Pf::input_type,
                     typename _Pa::input_type>::value,
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
template<typename _Pa,
         typename _F>
D_NODISCARD
auto ap_map(
    _Pa _pa,
    _F  _f
)
-> free_ap_parser<
       typename std::decay<decltype(
           _f(std::declval<typename _Pa::result_type>()))>::type,
       typename _Pa::input_type>
{
    using arg_t     = typename _Pa::result_type;
    using out_t     =
        typename std::decay<decltype(
            _f(std::declval<arg_t>()))>::type;
    using element_t = typename _Pa::input_type;
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
template<typename _L,
         typename _R>
using sel_either_t = ::djinterp::either<_L, _R>;


// free_sel_parser
//   class: the selective stratum.  Adds `select` to the
// applicative vocabulary; branches whose discriminant is a parsed
// value, but where the branches themselves are static.
template<typename _R,
         typename _E = char>
class free_sel_parser
    : public parser_expr<free_sel_parser<_R, _E>>
{
public:
    using input_type   = _E;
    using element_type = _E;
    using result_type  = _R;
    using value_type   = _R;
    using state_type   = parse_state<_E>;
    using output_type  = parse_result<_R>;

    free_sel_parser()
        : m_inner()
    {}

    explicit free_sel_parser(
        parser<_R, _E> _inner
    )
        : m_inner(static_cast<parser<_R, _E>&&>(_inner))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return m_inner.parse(_state);
    }

    D_NODISCARD
    const parser<_R, _E>&
    inner() const D_NOEXCEPT
    {
        return m_inner;
    }

private:
    parser<_R, _E> m_inner;
};


// sel_pure
//   factory: pure of the selective.
template<typename _R,
         typename _E = char>
D_NODISCARD
free_sel_parser<_R, _E>
sel_pure(
    _R _value
)
{
    using state_type  = parse_state<_E>;
    using output_type = parse_result<_R>;

    parser<_R, _E> inner(
        [_value](state_type& /*_state*/) -> output_type
        {
            return output_type(_value);
        });

    return free_sel_parser<_R, _E>(
        static_cast<parser<_R, _E>&&>(inner));
}


// sel_lift
//   factory: lifts any parser_expr into the selective stratum.
template<typename _Atom>
D_NODISCARD
free_sel_parser<
    typename clean_t<_Atom>::result_type,
    typename clean_t<_Atom>::input_type>
sel_lift(
    _Atom _atom
)
{
    static_assert(is_parser<_Atom>::value,
                  "sel_lift: argument must be a parser expression");

    using result_t  = typename clean_t<_Atom>::result_type;
    using element_t = typename clean_t<_Atom>::input_type;

    return free_sel_parser<result_t, element_t>(
        parser<result_t, element_t>(static_cast<_Atom&&>(_atom)));
}


// sel_select
//   factory: selective `select`.  Delegates to the functional
// companion's selective_select via the monad bridge — parser<R, E>
// participates as a Selective automatically because it is a Monad.
// The handler-effect type is parser<std::function<R(L)>, E>; the
// discriminant-effect type is parser<either<L, R>, E>.
template<typename _L,
         typename _R,
         typename _E,
         typename _PEither,
         typename _PHandler>
D_NODISCARD
free_sel_parser<_R, _E>
sel_select(
    _PEither  _p_either,
    _PHandler _p_handler
)
{
    static_assert(is_parser<_PEither>::value &&
                  is_parser<_PHandler>::value,
                  "sel_select: both arguments must be parsers");

    using either_t = sel_either_t<_L, _R>;

    // Erase the inputs to the handle types the selective protocol
    // expects.  parser<either_t, _E> is the discriminant; parser<
    // std::function<_R(_L)>, _E> is the handler.
    parser<either_t, _E>                       disc(_p_either);
    parser<std::function<_R(_L)>, _E>          handler(_p_handler);

    parser<_R, _E> inner =
        ::djinterp::selective_select(disc, handler);

    return free_sel_parser<_R, _E>(
        static_cast<parser<_R, _E>&&>(inner));
}


// sel_branch
//   factory: selective `branch` — given Either<L, R> and two
// handlers, run the appropriate one.  Delegates to
// selective_branch via the monad bridge.
template<typename _L,
         typename _R,
         typename _Out,
         typename _E,
         typename _PEither,
         typename _PLeft,
         typename _PRight>
D_NODISCARD
free_sel_parser<_Out, _E>
sel_branch(
    _PEither _p_either,
    _PLeft   _p_left,
    _PRight  _p_right
)
{
    static_assert(is_parser<_PEither>::value &&
                  is_parser<_PLeft>::value   &&
                  is_parser<_PRight>::value,
                  "sel_branch: all three arguments must be parsers");

    using either_t = sel_either_t<_L, _R>;

    parser<either_t, _E>                        disc(_p_either);
    parser<std::function<_Out(_L)>, _E>         fl(_p_left);
    parser<std::function<_Out(_R)>, _E>         fr(_p_right);

    parser<_Out, _E> inner =
        ::djinterp::selective_branch(disc, fl, fr);

    return free_sel_parser<_Out, _E>(
        static_cast<parser<_Out, _E>&&>(inner));
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
template<typename _R,
         typename _E>
D_NODISCARD
free_sel_parser<_R, _E>
ap_to_sel(
    const free_ap_parser<_R, _E>& _p
)
{
    return free_sel_parser<_R, _E>(_p.inner());
}

// sel_to_monad
//   function: promotes a FreeSel parser to a Free (monadic)
// parser, lifting the underlying handle into one F-layer of the
// canonical free-monad construction.
template<typename _R,
         typename _E>
D_NODISCARD
free_parser<_R, _E>
sel_to_monad(
    const free_sel_parser<_R, _E>& _p
)
{
    return free_parser<_R, _E>(
        lift_to_program(_p.inner()));
}

// ap_to_monad
//   function: composition of the two lifts — promote a FreeAp
// directly to a Free.
template<typename _R,
         typename _E>
D_NODISCARD
free_parser<_R, _E>
ap_to_monad(
    const free_ap_parser<_R, _E>& _p
)
{
    return free_parser<_R, _E>(
        lift_to_program(_p.inner()));
}


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_PARSER_FREE_
