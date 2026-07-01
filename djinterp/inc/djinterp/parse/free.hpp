/******************************************************************************
* djinterp [parse]                                              parser/free.hpp
*
* The three strata — Applicative, Selective, Monad — as free constructions.
*   Per ch-parsing.tex the expressive power of a parser can be dialled:
* the same parsable carrier can be presented at three levels, each a
* free construction over the polynomial functor F of the grammar.
*
*       FreeAp  F A     Applicative — pure and ap only.  No branching
*                       on parsed values.  Rigid, static, the most
*                       inspectable; the whole computation is data.
*
*       FreeSel F A     Selective — also static branching via select
*                       on an Either-typed value.  Captures `case` /
*                       static-discriminant grammars.
*
*       Free    F A     Monad — full bind.  Value-driven branching;
*                       maximum expressive power; opaque past each
*                       bind point.
*
*   Each stratum is a wrapper around the type-erased parser<R, E>
* handle that restricts its combinator surface to the stratum's
* allowed operations.  Every wrapper is itself a parser_expr, so the
* strata interoperate with the rest of parser/ — they ARE parsers,
* just labelled with which subset of the protocol they used to build
* themselves.
*
*   The stratum is type-level discipline rather than rigid runtime
* enforcement: an exit hatch into the lower level (the underlying
* parser handle) is always available via .inner(), and upward lifts
* (ap_to_sel, sel_to_monad) make the hierarchy a chain.  This matches
* the formal claim: more power means less inspection; the lift is
* always allowed, the descent never.
*
*   Inspection is left as an extension point.  Each stratum carries
* the parser by value-erased handle, so deep static analysis (first
* sets, nullability) would require carrying the AST of the
* construction in addition to the handle — that's a forward-
* compatible elaboration this header does not preempt.
*
* CONTENTS
*   I.    free_ap_parser<R, E>             applicative-only stratum
*           ap_pure(v) / ap_lift(atom)
*           ap_apply(pf, pa) / ap_map(pa, f)
*   II.   free_sel_parser<R, E>            selective stratum
*           sel_pure(v) / sel_lift(atom)
*           sel_apply / sel_map
*           sel_select(p_either, p_handler)
*           sel_branch(p_either, p_left, p_right)
*   III.  free_parser<R, E>                monadic stratum
*           free_pure(v) / free_lift(atom)
*           free_apply / free_map / free_bind
*   IV.   Cross-stratum lifts
*           ap_to_sel  / sel_to_monad
*           ap_to_monad                    (composition)
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
#include <type_traits>
#include <utility>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/functional/result.hpp"
#include "../parse.hpp"
#include "./parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  internal — the either sum used by the selective stratum
// ================================================================

NS_INTERNAL

    // sel_either
    //   struct: a minimal Either<L, R> used by the selective
    // stratum's `select`.  Local to the parsing module so this
    // header doesn't bind to a particular either / variant choice
    // in the functional companion.
    template<typename _L,
             typename _R>
    struct sel_either
    {
        bool m_is_left;
        _L   m_left;
        _R   m_right;

        sel_either()
            : m_is_left(true),
              m_left   (),
              m_right  ()
        {}

        // left
        //   factory: an Either holding the left arm.
        D_NODISCARD
        static sel_either
        left(
            const _L& _l
        )
        {
            sel_either e;
            e.m_is_left = true;
            e.m_left    = _l;
            return e;
        }

        // right
        //   factory: an Either holding the right arm.
        D_NODISCARD
        static sel_either
        right(
            const _R& _r
        )
        {
            sel_either e;
            e.m_is_left = false;
            e.m_right   = _r;
            return e;
        }

        D_NODISCARD bool is_left()  const { return m_is_left;  }
        D_NODISCARD bool is_right() const { return !m_is_left; }
        D_NODISCARD const _L& left()  const { return m_left;  }
        D_NODISCARD const _R& right() const { return m_right; }
    };

NS_END  // internal


// ================================================================
//  I.   free_ap_parser  —  applicative stratum
// ================================================================

// free_ap_parser
//   class: the applicative-only stratum.  A FreeAp parser is built
// from pure (constant lifts) and ap (applicative apply), plus lifts
// of atomic parsers (any parser_expr counts as an F-action).  No
// bind: the structure of the computation is fixed at construction
// time and is independent of the values being parsed.
//
//   Internally carries the type-erased parser<_R, _E> handle; the
// stratum face is a discipline on the combinator surface used to
// build it.
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

    // parse_impl
    //   method: delegates to the wrapped parser.
    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return m_inner.parse(_state);
    }

    // inner
    //   accessor: the underlying erased handle, used by promotions
    // and by callers needing to drop down to the unrestricted
    // parser surface.
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
//   factory: pure of the FreeAp applicative.  Lifts a value into a
// FreeAp that succeeds with it and consumes no input.
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
//   factory: lifts any parser_expr into the FreeAp stratum — i.e.
// reifies an atomic F-action.  The result is a FreeAp whose
// underlying interpretation runs the lifted parser.
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
//   factory: applicative apply at the FreeAp stratum.  Runs the
// function-producing FreeAp, then the value-producing FreeAp, then
// applies the former to the latter.
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

    static_assert(
        std::is_same<typename _Pf::input_type,
                     typename _Pa::input_type>::value,
        "ap_apply: branches must share input_type");

    using state_type  = parse_state<element_t>;
    using output_type = parse_result<out_t>;

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
//   factory: post-processes a FreeAp result with a pure function.
// Equivalent to ap_apply(ap_pure(f), pa) up to the constant-fn
// indirection, named locally for ergonomic uses.
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
//  II.  free_sel_parser  —  selective stratum
// ================================================================

// free_sel_parser
//   class: the selective stratum.  Adds `select` to the applicative
// vocabulary: branching whose discriminant is a parsed value, but
// where the branches themselves are static.  Models "case … of Left
// l → … ; Right r → …" grammars where the inhabitant chosen depends
// on a parsed tag.
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


// sel_either_t
//   alias: convenience for the selective Either type at this
// stratum.
template<typename _L,
         typename _R>
using sel_either_t = internal::sel_either<_L, _R>;


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
//   factory: the selective `select` combinator.  Given a parser
// producing an Either<L, R> and a parser producing a handler L → R,
// runs the discriminant first; on Left l it runs the handler and
// applies it to l; on Right r it returns r directly.  The handler
// is run only when needed — this is what distinguishes select from
// applicative apply.
template<typename _L,
         typename _R,
         typename _E,
         typename _PEither,
         typename _PHandler>
D_NODISCARD
free_sel_parser<_R, _E>
sel_select(
    _PEither _p_either,
    _PHandler _p_handler
)
{
    static_assert(is_parser<_PEither>::value &&
                  is_parser<_PHandler>::value,
                  "sel_select: both arguments must be parsers");

    using state_type  = parse_state<_E>;
    using output_type = parse_result<_R>;
    using either_t    = sel_either_t<_L, _R>;

    parser<_R, _E> inner(
        [_p_either, _p_handler](state_type& _state) -> output_type
        {
            parse_result<either_t> re = _p_either.parse(_state);

            if (!re.ok())
            {
                return output_type(re.error());
            }

            const either_t& e = re.value();

            if (e.is_right())
            {
                return output_type(e.right());
            }

            parse_result<std::function<_R(_L)>> rh =
                _p_handler.parse(_state);

            if (!rh.ok())
            {
                return output_type(rh.error());
            }

            return output_type(rh.value()(e.left()));
        });

    return free_sel_parser<_R, _E>(
        static_cast<parser<_R, _E>&&>(inner));
}


// sel_branch
//   factory: the dual face of `select` — given a parser producing
// Either<L, R> and two handler parsers (one for each arm), runs the
// appropriate handler depending on which arm came back.  More
// ergonomic when both arms are non-trivial.
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

    using state_type  = parse_state<_E>;
    using output_type = parse_result<_Out>;
    using either_t    = sel_either_t<_L, _R>;

    parser<_Out, _E> inner(
        [_p_either, _p_left, _p_right](
            state_type& _state) -> output_type
        {
            parse_result<either_t> re = _p_either.parse(_state);

            if (!re.ok())
            {
                return output_type(re.error());
            }

            const either_t& e = re.value();

            if (e.is_left())
            {
                parse_result<std::function<_Out(_L)>> rh =
                    _p_left.parse(_state);

                if (!rh.ok())
                {
                    return output_type(rh.error());
                }

                return output_type(rh.value()(e.left()));
            }

            parse_result<std::function<_Out(_R)>> rh =
                _p_right.parse(_state);

            if (!rh.ok())
            {
                return output_type(rh.error());
            }

            return output_type(rh.value()(e.right()));
        });

    return free_sel_parser<_Out, _E>(
        static_cast<parser<_Out, _E>&&>(inner));
}


// ================================================================
//  III. free_parser  —  monadic stratum
// ================================================================

// free_parser
//   class: the monadic stratum.  Equivalent in expressive power to
// the parser<R, E> handle itself — full bind, value-driven
// branching.  The name aligns with the formal Free F A
// construction; in practice the wrapper just acknowledges that we
// have entered the level where static inspection ends and full
// computational power begins.
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

    free_parser()
        : m_inner()
    {}

    explicit free_parser(
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


// free_pure
//   factory: pure of the monadic stratum.
template<typename _R,
         typename _E = char>
D_NODISCARD
free_parser<_R, _E>
free_pure(
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

    return free_parser<_R, _E>(
        static_cast<parser<_R, _E>&&>(inner));
}


// free_lift
//   factory: lifts any parser_expr into the monadic stratum.
template<typename _Atom>
D_NODISCARD
free_parser<
    typename clean_t<_Atom>::result_type,
    typename clean_t<_Atom>::input_type>
free_lift(
    _Atom _atom
)
{
    static_assert(is_parser<_Atom>::value,
                  "free_lift: argument must be a parser expression");

    using result_t  = typename clean_t<_Atom>::result_type;
    using element_t = typename clean_t<_Atom>::input_type;

    return free_parser<result_t, element_t>(
        parser<result_t, element_t>(static_cast<_Atom&&>(_atom)));
}


// free_bind
//   factory: monadic bind at the Free stratum.  Runs _p; on success
// applies _f to its value to obtain a second free_parser; runs that
// at the state _p left behind.  This is where value-driven
// branching enters and the inspection chain ends.
template<typename _Pa,
         typename _F>
D_NODISCARD
auto free_bind(
    _Pa _p,
    _F  _f
)
-> typename std::decay<decltype(
       _f(std::declval<typename _Pa::result_type>()))>::type
{
    static_assert(is_parser<_Pa>::value,
                  "free_bind: argument must be a parser");

    using arg_t          = typename _Pa::result_type;
    using next_parser_t  =
        typename std::decay<decltype(
            _f(std::declval<arg_t>()))>::type;
    using next_result_t  = typename next_parser_t::result_type;
    using element_t      = typename _Pa::input_type;
    using state_type     = parse_state<element_t>;
    using output_type    = parse_result<next_result_t>;

    parser<next_result_t, element_t> inner(
        [_p, _f](state_type& _state) -> output_type
        {
            parse_result<arg_t> r = _p.parse(_state);

            if (!r.ok())
            {
                return output_type(r.error());
            }

            next_parser_t next = _f(r.value());

            return next.parse(_state);
        });

    return next_parser_t(
        static_cast<parser<next_result_t, element_t>&&>(inner));
}


// free_map
//   factory: Functor map at the Free stratum.
template<typename _Pa,
         typename _F>
D_NODISCARD
auto free_map(
    _Pa _pa,
    _F  _f
)
-> free_parser<
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

    return free_parser<out_t, element_t>(
        static_cast<parser<out_t, element_t>&&>(inner));
}


// ================================================================
//  IV.  Cross-stratum lifts
// ================================================================
//   The strata form a chain
//
//       FreeAp F A  ⊂  FreeSel F A  ⊂  Free F A
//
// — every applicative computation is a selective one, every
// selective is monadic.  The lifts here are upward only; descent
// is impossible by design (you cannot recover applicative-only
// structure from a monadic computation).

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
//   function: promotes a FreeSel parser to a Free (monadic) parser.
template<typename _R,
         typename _E>
D_NODISCARD
free_parser<_R, _E>
sel_to_monad(
    const free_sel_parser<_R, _E>& _p
)
{
    return free_parser<_R, _E>(_p.inner());
}

// ap_to_monad
//   function: convenience for the composition ap → sel → monad,
// promoting a FreeAp directly to a Free.
template<typename _R,
         typename _E>
D_NODISCARD
free_parser<_R, _E>
ap_to_monad(
    const free_ap_parser<_R, _E>& _p
)
{
    return free_parser<_R, _E>(_p.inner());
}


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_PARSER_FREE_
