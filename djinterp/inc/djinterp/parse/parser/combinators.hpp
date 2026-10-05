/*******************************************************************************
* djinterp [parse]                                               combinators.hpp
*
* Higher-level combinators — each a concrete CRTP class.
*   The named combinators here are the workhorses of a typical
* grammar: choice (`or_`), sequencing (`seq`, `seq_l`, `seq_r`,
* `ap`), repetition (`many`, `many1`, `skip_many`, `count`),
* grouping (`optional`, `sep_by`, `end_by`, `between`),
* post-processing (`map`, `bind`), look-ahead (`look_ahead`,
* `not_followed_by`), and diagnostic shaping (`label`, `try_`).
*
*   Each combinator is a class inheriting from parser_expr; the
* corresponding factory function returns that class by value with
* its template parameters deduced from the call site.  Composition
* through these is fully visible to the compiler — there is no type
* erasure on the static path, and the compiler can inline a deeply
* nested combinator tree end to end.
*
*   The same operations live on the type-erased parser<R, E> handle
* via the functional protocols (monad_traits, alternative_traits,
* applicative_traits in parser.hpp).  Use those when erasure is
* needed (storage, recursion, return values, pipeline operators);
* use the factories below when zero overhead matters.
*
* CONTENTS
*   I.    Functor / post-processing
*           map_parser            / map(p, f)
*   II.   Monad
*           bind_parser           / bind(p, f)
*   III.  Sequencing
*           seq_parser            / seq(p, q)        -> pair
*           seq_l_parser          / seq_l(p, q)      -> left
*           seq_r_parser          / seq_r(p, q)      -> right
*           ap_parser             / ap(pf, pa)       -> applicative
*   IV.   Choice
*           alt_parser            / or_(p, q)        -> PEG ordered
*           one_of_parser         / one_of_parser({...})  n-ary
*   V.    Repetition
*           many_parser           / many(p)          -> vector
*           many1_parser          / many1(p)
*           skip_many_parser      / skip_many(p)
*           skip_many1_parser     / skip_many1(p)
*           count_parser          / count(n, p)
*   VI.   Grouping
*           optional_parser       / optional(p)      -> maybe
*           sep_by_parser         / sep_by(p, sep)
*           sep_by1_parser        / sep_by1(p, sep)
*           end_by_parser         / end_by(p, sep)
*           between_parser        / between(o, p, c)
*   VII.  Look-ahead and commit
*           look_ahead_parser     / look_ahead(p)
*           not_followed_by_parser/ not_followed_by(p)
*           try_parser            / try_(p)
*   VIII. Diagnostic
*           label_parser          / label(p, msg)
*
*
* path:      /inc/djinterp/parse/parser/combinators.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSER_COMBINATORS_HPP
#define DJINTERP_PARSE_PARSER_COMBINATORS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../../core/meta/type_utility.hpp"  // clean_t
#include "../../core/functional/maybe.hpp"
#include "../parse.hpp"
#include "./parser.hpp"
#include "../primitives.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  internal — shape helpers
// ================================================================
//   Compile-time inferences of result/input/element types from one
// or two child parsers.  Kept here so each combinator class can
// declare its nested types in one line.

NS_INTERNAL

    // child_input
    //   trait: input_type of a child parser.
    template<typename P>
    using child_input =
        typename clean_t<P>::input_type;

    // child_result
    //   trait: result_type of a child parser.
    template<typename P>
    using child_result =
        typename clean_t<P>::result_type;

    // call_result_of
    //   trait: the decayed return type of f(arg).
    template<typename F,
             typename Arg>
    using call_result_of =
        typename std::decay<decltype(
            std::declval<F>()(std::declval<Arg>()))>::type;

NS_END  // internal


// ================================================================
//  I.   Functor / post-processing
// ================================================================

// map_parser
//   class: applies a pure function to the result of an inner parser.
// Equivalent to the Functor map; named locally so it composes
// statically.
template<typename P,
         typename F>
class map_parser
    : public parser_expr<map_parser<P, F>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using result_type  =
        internal::call_result_of<F, internal::child_result<P>>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    map_parser(
        P _p,
        F _f
    )
        : m_p(static_cast<P&&>(_p)),
          m_f(static_cast<F&&>(_f))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        parse_result<internal::child_result<P>> r = m_p.parse(_state);

        if (!r.ok())
        {
            return output_type(r.error());
        }

        return output_type(m_f(r.value()));
    }

private:
    P m_p;
    F m_f;
};

// map
//   factory: applies _f to whatever _p produces.
template<typename P,
         typename F>
D_NODISCARD
map_parser<P, F>
map(
    P _p,
    F _f
)
{
    static_assert(is_parser<P>::value,
                  "map: P must be a parser expression");

    return map_parser<P, F>(
        static_cast<P&&>(_p),
        static_cast<F&&>(_f));
}


// ================================================================
//  II.  Monad
// ================================================================

// bind_parser
//   class: monadic bind.  Runs _p; on success applies _f to its
// value to obtain a second parser; runs that parser at the state
// _p left behind.  The result_type is whatever _f's returned
// parser produces.
template<typename P,
         typename F>
class bind_parser
    : public parser_expr<bind_parser<P, F>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using next_parser_type =
        internal::call_result_of<F, internal::child_result<P>>;
    using result_type  =
        typename next_parser_type::result_type;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    bind_parser(
        P _p,
        F _f
    )
        : m_p(static_cast<P&&>(_p)),
          m_f(static_cast<F&&>(_f))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        parse_result<internal::child_result<P>> r = m_p.parse(_state);

        if (!r.ok())
        {
            return output_type(r.error());
        }

        next_parser_type next = m_f(r.value());

        return next.parse(_state);
    }

private:
    P m_p;
    F m_f;
};

// bind
//   factory: monadic bind on parser expressions.
template<typename P,
         typename F>
D_NODISCARD
bind_parser<P, F>
bind(
    P _p,
    F _f
)
{
    static_assert(is_parser<P>::value,
                  "bind: P must be a parser expression");

    return bind_parser<P, F>(
        static_cast<P&&>(_p),
        static_cast<F&&>(_f));
}


// ================================================================
//  III. Sequencing
// ================================================================

// seq_parser
//   class: run _p then _q; both must succeed; return the pair.  The
// basic concatenation at the parser layer — the operational dual of
// the grammar's × product.
template<typename P,
         typename Q>
class seq_parser
    : public parser_expr<seq_parser<P, Q>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using result_type  = std::pair<
        internal::child_result<P>,
        internal::child_result<Q>>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    static_assert(
        std::is_same<internal::child_input<P>,
                     internal::child_input<Q>>::value,
        "seq_parser: branches must share input_type");

    seq_parser(
        P _p,
        Q _q
    )
        : m_p(static_cast<P&&>(_p)),
          m_q(static_cast<Q&&>(_q))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        parse_result<internal::child_result<P>> r1 = m_p.parse(_state);

        if (!r1.ok())
        {
            return output_type(r1.error());
        }

        parse_result<internal::child_result<Q>> r2 = m_q.parse(_state);

        if (!r2.ok())
        {
            return output_type(r2.error());
        }

        return output_type(
            result_type(r1.value(), r2.value()));
    }

private:
    P m_p;
    Q m_q;
};

// seq
//   factory: pair-returning sequencing of two parsers.
template<typename P,
         typename Q>
D_NODISCARD
seq_parser<P, Q>
seq(
    P _p,
    Q _q
)
{
    static_assert(is_parser<P>::value && is_parser<Q>::value,
                  "seq: both arguments must be parser expressions");

    return seq_parser<P, Q>(
        static_cast<P&&>(_p),
        static_cast<Q&&>(_q));
}


// seq_l_parser
//   class: run _p then _q; both must succeed; return _p's result
// (keep left).
template<typename P,
         typename Q>
class seq_l_parser
    : public parser_expr<seq_l_parser<P, Q>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using result_type  = internal::child_result<P>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    static_assert(
        std::is_same<internal::child_input<P>,
                     internal::child_input<Q>>::value,
        "seq_l_parser: branches must share input_type");

    seq_l_parser(
        P _p,
        Q _q
    )
        : m_p(static_cast<P&&>(_p)),
          m_q(static_cast<Q&&>(_q))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        output_type r1 = m_p.parse(_state);

        if (!r1.ok())
        {
            return r1;
        }

        parse_result<internal::child_result<Q>> r2 = m_q.parse(_state);

        if (!r2.ok())
        {
            return output_type(r2.error());
        }

        return r1;
    }

private:
    P m_p;
    Q m_q;
};

// seq_l
//   factory: keep-left sequencing.
template<typename P,
         typename Q>
D_NODISCARD
seq_l_parser<P, Q>
seq_l(
    P _p,
    Q _q
)
{
    static_assert(is_parser<P>::value && is_parser<Q>::value,
                  "seq_l: both arguments must be parser expressions");

    return seq_l_parser<P, Q>(
        static_cast<P&&>(_p),
        static_cast<Q&&>(_q));
}


// seq_r_parser
//   class: run _p then _q; both must succeed; return _q's result
// (keep right).
template<typename P,
         typename Q>
class seq_r_parser
    : public parser_expr<seq_r_parser<P, Q>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using result_type  = internal::child_result<Q>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    static_assert(
        std::is_same<internal::child_input<P>,
                     internal::child_input<Q>>::value,
        "seq_r_parser: branches must share input_type");

    seq_r_parser(
        P _p,
        Q _q
    )
        : m_p(static_cast<P&&>(_p)),
          m_q(static_cast<Q&&>(_q))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        parse_result<internal::child_result<P>> r1 = m_p.parse(_state);

        if (!r1.ok())
        {
            return output_type(r1.error());
        }

        return m_q.parse(_state);
    }

private:
    P m_p;
    Q m_q;
};

// seq_r
//   factory: keep-right sequencing.
template<typename P,
         typename Q>
D_NODISCARD
seq_r_parser<P, Q>
seq_r(
    P _p,
    Q _q
)
{
    static_assert(is_parser<P>::value && is_parser<Q>::value,
                  "seq_r: both arguments must be parser expressions");

    return seq_r_parser<P, Q>(
        static_cast<P&&>(_p),
        static_cast<Q&&>(_q));
}


// ap_parser
//   class: applicative apply.  Pf is a parser producing a callable;
// Pa is a parser producing that callable's argument.  Runs in
// sequence, applies, threads the residual.
template<typename Pf,
         typename Pa>
class ap_parser
    : public parser_expr<ap_parser<Pf, Pa>>
{
public:
    using input_type   = internal::child_input<Pf>;
    using element_type = input_type;
    using fn_type      = internal::child_result<Pf>;
    using arg_type     = internal::child_result<Pa>;
    using result_type  =
        internal::call_result_of<fn_type, arg_type>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    static_assert(
        std::is_same<internal::child_input<Pf>,
                     internal::child_input<Pa>>::value,
        "ap_parser: branches must share input_type");

    ap_parser(
        Pf _pf,
        Pa _pa
    )
        : m_pf(static_cast<Pf&&>(_pf)),
          m_pa(static_cast<Pa&&>(_pa))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        parse_result<fn_type> rf = m_pf.parse(_state);

        if (!rf.ok())
        {
            return output_type(rf.error());
        }

        parse_result<arg_type> ra = m_pa.parse(_state);

        if (!ra.ok())
        {
            return output_type(ra.error());
        }

        return output_type(rf.value()(ra.value()));
    }

private:
    Pf m_pf;
    Pa m_pa;
};

// ap
//   factory: applicative apply.
template<typename Pf,
         typename Pa>
D_NODISCARD
ap_parser<Pf, Pa>
ap(
    Pf _pf,
    Pa _pa
)
{
    static_assert(is_parser<Pf>::value && is_parser<Pa>::value,
                  "ap: both arguments must be parser expressions");

    return ap_parser<Pf, Pa>(
        static_cast<Pf&&>(_pf),
        static_cast<Pa&&>(_pa));
}


// ================================================================
//  IV.  Choice
// ================================================================

// alt_parser
//   class: PEG ordered choice.  Run _p; on success commit; on
// failure restore the input offset and run _q.  Left-biased and
// leftmost-wins, exactly as ch-parsing prescribes.
template<typename P,
         typename Q>
class alt_parser
    : public parser_expr<alt_parser<P, Q>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using result_type  = internal::child_result<P>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    static_assert(
        std::is_same<internal::child_input<P>,
                     internal::child_input<Q>>::value,
        "alt_parser: branches must share input_type");
    static_assert(
        std::is_same<internal::child_result<P>,
                     internal::child_result<Q>>::value,
        "alt_parser: branches must share result_type");

    alt_parser(
        P _p,
        Q _q
    )
        : m_p(static_cast<P&&>(_p)),
          m_q(static_cast<Q&&>(_q))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        std::size_t saved = _state.offset;

        output_type r = m_p.parse(_state);

        if (r.ok())
        {
            return r;
        }

        _state.offset = saved;

        return m_q.parse(_state);
    }

private:
    P m_p;
    Q m_q;
};

// or_
//   factory: PEG ordered choice between two parsers.  Trailing
// underscore avoids the C++ alternative-token keyword `or`.
template<typename P,
         typename Q>
D_NODISCARD
alt_parser<P, Q>
or_(
    P _p,
    Q _q
)
{
    static_assert(is_parser<P>::value && is_parser<Q>::value,
                  "or_: both arguments must be parser expressions");

    return alt_parser<P, Q>(
        static_cast<P&&>(_p),
        static_cast<Q&&>(_q));
}


// ================================================================
//  V.   Repetition
// ================================================================

// many_parser
//   class: zero or more applications of an inner parser, collected
// into a vector.  Never fails.  Stops on the first failing iteration
// (offset restored before stopping) or on a zero-width match.
template<typename P>
class many_parser
    : public parser_expr<many_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using result_type  = std::vector<inner_result>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    explicit many_parser(
        P _p
    )
        : m_p(static_cast<P&&>(_p))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        result_type acc;
        std::size_t saved;

        while (true)
        {
            saved = _state.offset;

            parse_result<inner_result> r = m_p.parse(_state);

            if (!r.ok())
            {
                _state.offset = saved;
                break;
            }

            // guard against zero-width matches to avoid infinite
            // looping when the inner parser succeeds without
            // consuming input.
            if (_state.offset == saved)
            {
                acc.push_back(r.value());
                break;
            }

            acc.push_back(r.value());
        }

        return output_type(static_cast<result_type&&>(acc));
    }

private:
    P m_p;
};

// many
//   factory: zero or more applications, collected into a vector.
template<typename P>
D_NODISCARD
many_parser<P>
many(
    P _p
)
{
    static_assert(is_parser<P>::value,
                  "many: argument must be a parser expression");

    return many_parser<P>(static_cast<P&&>(_p));
}


// many1_parser
//   class: one or more applications.  Fails iff the first iteration
// fails.
template<typename P>
class many1_parser
    : public parser_expr<many1_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using result_type  = std::vector<inner_result>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    explicit many1_parser(
        P _p
    )
        : m_p(static_cast<P&&>(_p))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        result_type acc;
        std::size_t saved;

        parse_result<inner_result> first = m_p.parse(_state);

        if (!first.ok())
        {
            return output_type(first.error());
        }

        acc.push_back(first.value());

        while (true)
        {
            saved = _state.offset;

            parse_result<inner_result> r = m_p.parse(_state);

            if (!r.ok())
            {
                _state.offset = saved;
                break;
            }

            if (_state.offset == saved)
            {
                acc.push_back(r.value());
                break;
            }

            acc.push_back(r.value());
        }

        return output_type(static_cast<result_type&&>(acc));
    }

private:
    P m_p;
};

// many1
//   factory: one or more applications.
template<typename P>
D_NODISCARD
many1_parser<P>
many1(
    P _p
)
{
    static_assert(is_parser<P>::value,
                  "many1: argument must be a parser expression");

    return many1_parser<P>(static_cast<P&&>(_p));
}


// skip_many_parser
//   class: zero or more applications, results discarded.
template<typename P>
class skip_many_parser
    : public parser_expr<skip_many_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using result_type  = bool;
    using value_type   = bool;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<bool>;

    explicit skip_many_parser(
        P _p
    )
        : m_p(static_cast<P&&>(_p))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        std::size_t saved;

        while (true)
        {
            saved = _state.offset;

            parse_result<inner_result> r = m_p.parse(_state);

            if (!r.ok())
            {
                _state.offset = saved;
                break;
            }

            if (_state.offset == saved)
            {
                break;
            }
        }

        return output_type(true);
    }

private:
    P m_p;
};

// skip_many
//   factory: zero or more, results discarded.
template<typename P>
D_NODISCARD
skip_many_parser<P>
skip_many(
    P _p
)
{
    static_assert(is_parser<P>::value,
                  "skip_many: argument must be a parser expression");

    return skip_many_parser<P>(static_cast<P&&>(_p));
}


// skip_many1_parser
//   class: one or more applications, results discarded.
template<typename P>
class skip_many1_parser
    : public parser_expr<skip_many1_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using result_type  = bool;
    using value_type   = bool;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<bool>;

    explicit skip_many1_parser(
        P _p
    )
        : m_p(static_cast<P&&>(_p))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        std::size_t saved;

        parse_result<inner_result> first = m_p.parse(_state);

        if (!first.ok())
        {
            return output_type(first.error());
        }

        while (true)
        {
            saved = _state.offset;

            parse_result<inner_result> r = m_p.parse(_state);

            if (!r.ok())
            {
                _state.offset = saved;
                break;
            }

            if (_state.offset == saved)
            {
                break;
            }
        }

        return output_type(true);
    }

private:
    P m_p;
};

// skip_many1
//   factory: one or more, results discarded.
template<typename P>
D_NODISCARD
skip_many1_parser<P>
skip_many1(
    P _p
)
{
    static_assert(is_parser<P>::value,
                  "skip_many1: argument must be a parser expression");

    return skip_many1_parser<P>(static_cast<P&&>(_p));
}


// count_parser
//   class: exactly _n applications; fails on any iteration failure.
template<typename P>
class count_parser
    : public parser_expr<count_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using result_type  = std::vector<inner_result>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    count_parser(
        std::size_t _n,
        P           _p
    )
        : m_n(_n),
          m_p(static_cast<P&&>(_p))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        result_type acc;
        std::size_t i;

        acc.reserve(m_n);

        for (i = 0; i < m_n; ++i)
        {
            parse_result<inner_result> r = m_p.parse(_state);

            if (!r.ok())
            {
                return output_type(r.error());
            }

            acc.push_back(r.value());
        }

        return output_type(static_cast<result_type&&>(acc));
    }

private:
    std::size_t m_n;
    P           m_p;
};

// count
//   factory: exactly _n applications.
template<typename P>
D_NODISCARD
count_parser<P>
count(
    std::size_t _n,
    P           _p
)
{
    static_assert(is_parser<P>::value,
                  "count: argument must be a parser expression");

    return count_parser<P>(_n, static_cast<P&&>(_p));
}


// ================================================================
//  VI.  Grouping
// ================================================================

// optional_parser
//   class: zero or one application of an inner parser; result is a
// maybe.  Never fails.
template<typename P>
class optional_parser
    : public parser_expr<optional_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using result_type  = functional::maybe<inner_result>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    explicit optional_parser(
        P _p
    )
        : m_p(static_cast<P&&>(_p))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        std::size_t saved = _state.offset;

        parse_result<inner_result> r = m_p.parse(_state);

        if (r.ok())
        {
            return output_type(result_type(r.value()));
        }

        _state.offset = saved;

        return output_type(result_type());
    }

private:
    P m_p;
};

// optional
//   factory: zero or one application; returns a maybe.
template<typename P>
D_NODISCARD
optional_parser<P>
optional(
    P _p
)
{
    static_assert(is_parser<P>::value,
                  "optional: argument must be a parser expression");

    return optional_parser<P>(static_cast<P&&>(_p));
}


// sep_by1_parser
//   class: one or more applications of _p separated by _sep, i.e.
// p (sep p)*.  Returns the collected values.
template<typename P,
         typename Sep>
class sep_by1_parser
    : public parser_expr<sep_by1_parser<P, Sep>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using sep_result   = internal::child_result<Sep>;
    using result_type  = std::vector<inner_result>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    static_assert(
        std::is_same<internal::child_input<P>,
                     internal::child_input<Sep>>::value,
        "sep_by1_parser: branches must share input_type");

    sep_by1_parser(
        P    _p,
        Sep _sep
    )
        : m_p  (static_cast<P&&>(_p)),
          m_sep(static_cast<Sep&&>(_sep))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        result_type acc;
        std::size_t saved;

        parse_result<inner_result> first = m_p.parse(_state);

        if (!first.ok())
        {
            return output_type(first.error());
        }

        acc.push_back(first.value());

        while (true)
        {
            saved = _state.offset;

            parse_result<sep_result> sr = m_sep.parse(_state);

            if (!sr.ok())
            {
                _state.offset = saved;
                break;
            }

            parse_result<inner_result> r = m_p.parse(_state);

            if (!r.ok())
            {
                // sep matched but no element followed: restore to
                // before the sep so the consumed sep doesn't leak.
                _state.offset = saved;
                break;
            }

            acc.push_back(r.value());
        }

        return output_type(static_cast<result_type&&>(acc));
    }

private:
    P    m_p;
    Sep m_sep;
};

// sep_by1
//   factory: one or more _p separated by _sep.
template<typename P,
         typename Sep>
D_NODISCARD
sep_by1_parser<P, Sep>
sep_by1(
    P    _p,
    Sep _sep
)
{
    static_assert(is_parser<P>::value && is_parser<Sep>::value,
                  "sep_by1: both arguments must be parser expressions");

    return sep_by1_parser<P, Sep>(
        static_cast<P&&>(_p),
        static_cast<Sep&&>(_sep));
}


// sep_by_parser
//   class: zero or more _p separated by _sep.  Always succeeds;
// empty input yields an empty vector.
template<typename P,
         typename Sep>
class sep_by_parser
    : public parser_expr<sep_by_parser<P, Sep>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using sep_result   = internal::child_result<Sep>;
    using result_type  = std::vector<inner_result>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    static_assert(
        std::is_same<internal::child_input<P>,
                     internal::child_input<Sep>>::value,
        "sep_by_parser: branches must share input_type");

    sep_by_parser(
        P    _p,
        Sep _sep
    )
        : m_inner(static_cast<P&&>(_p),
                  static_cast<Sep&&>(_sep))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        std::size_t saved = _state.offset;

        output_type r = m_inner.parse_impl(_state);

        if (r.ok())
        {
            return r;
        }

        _state.offset = saved;

        return output_type(result_type());
    }

private:
    sep_by1_parser<P, Sep> m_inner;
};

// sep_by
//   factory: zero or more _p separated by _sep.
template<typename P,
         typename Sep>
D_NODISCARD
sep_by_parser<P, Sep>
sep_by(
    P    _p,
    Sep _sep
)
{
    static_assert(is_parser<P>::value && is_parser<Sep>::value,
                  "sep_by: both arguments must be parser expressions");

    return sep_by_parser<P, Sep>(
        static_cast<P&&>(_p),
        static_cast<Sep&&>(_sep));
}


// end_by_parser
//   class: zero or more applications of _p followed by _sep:
// (p sep)*.  Each element is required to have a trailing sep.
template<typename P,
         typename Sep>
class end_by_parser
    : public parser_expr<end_by_parser<P, Sep>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using result_type  = std::vector<inner_result>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    static_assert(
        std::is_same<internal::child_input<P>,
                     internal::child_input<Sep>>::value,
        "end_by_parser: branches must share input_type");

    end_by_parser(
        P    _p,
        Sep _sep
    )
        : m_inner(seq_l_parser<P, Sep>(
              static_cast<P&&>(_p),
              static_cast<Sep&&>(_sep)))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return m_inner.parse_impl(_state);
    }

private:
    many_parser<seq_l_parser<P, Sep>> m_inner;
};

// end_by
//   factory: (p sep)*.
template<typename P,
         typename Sep>
D_NODISCARD
end_by_parser<P, Sep>
end_by(
    P    _p,
    Sep _sep
)
{
    static_assert(is_parser<P>::value && is_parser<Sep>::value,
                  "end_by: both arguments must be parser expressions");

    return end_by_parser<P, Sep>(
        static_cast<P&&>(_p),
        static_cast<Sep&&>(_sep));
}


// between_parser
//   class: open _p close — succeeds iff all three succeed; returns
// _p's result.  The bracketing combinator.
template<typename Open,
         typename P,
         typename Close>
class between_parser
    : public parser_expr<between_parser<Open, P, Close>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using result_type  = internal::child_result<P>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    static_assert(
        std::is_same<internal::child_input<Open>,
                     internal::child_input<P>>::value     &&
        std::is_same<internal::child_input<P>,
                     internal::child_input<Close>>::value,
        "between_parser: branches must share input_type");

    between_parser(
        Open   _open,
        P      _p,
        Close _close
    )
        : m_open (static_cast<Open&&>(_open)),
          m_p    (static_cast<P&&>(_p)),
          m_close(static_cast<Close&&>(_close))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        parse_result<internal::child_result<Open>>
            ro = m_open.parse(_state);

        if (!ro.ok())
        {
            return output_type(ro.error());
        }

        output_type r = m_p.parse(_state);

        if (!r.ok())
        {
            return r;
        }

        parse_result<internal::child_result<Close>>
            rc = m_close.parse(_state);

        if (!rc.ok())
        {
            return output_type(rc.error());
        }

        return r;
    }

private:
    Open   m_open;
    P      m_p;
    Close m_close;
};

// between
//   factory: open _p close, keeping _p's result.
template<typename Open,
         typename P,
         typename Close>
D_NODISCARD
between_parser<Open, P, Close>
between(
    Open   _open,
    P      _p,
    Close _close
)
{
    static_assert(
        ( is_parser<Open>::value  &&
          is_parser<P>::value     &&
          is_parser<Close>::value ),
        "between: all three arguments must be parser expressions");

    return between_parser<Open, P, Close>(
        static_cast<Open&&>(_open),
        static_cast<P&&>(_p),
        static_cast<Close&&>(_close));
}


// ================================================================
//  VII. Look-ahead and commit
// ================================================================

// look_ahead_parser
//   class: runs _p, restores the input offset (success or failure),
// returns _p's result.  The peek combinator.
template<typename P>
class look_ahead_parser
    : public parser_expr<look_ahead_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using result_type  = internal::child_result<P>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    explicit look_ahead_parser(
        P _p
    )
        : m_p(static_cast<P&&>(_p))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        std::size_t saved = _state.offset;

        output_type r = m_p.parse(_state);

        _state.offset = saved;

        return r;
    }

private:
    P m_p;
};

// look_ahead
//   factory: peek — run, then restore.
template<typename P>
D_NODISCARD
look_ahead_parser<P>
look_ahead(
    P _p
)
{
    static_assert(is_parser<P>::value,
                  "look_ahead: argument must be a parser expression");

    return look_ahead_parser<P>(static_cast<P&&>(_p));
}


// not_followed_by_parser
//   class: succeeds with `true` iff _p would fail at the current
// offset; never consumes input.  PEG's negative look-ahead.
template<typename P>
class not_followed_by_parser
    : public parser_expr<not_followed_by_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using inner_result = internal::child_result<P>;
    using result_type  = bool;
    using value_type   = bool;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<bool>;

    explicit not_followed_by_parser(
        P _p
    )
        : m_p(static_cast<P&&>(_p))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        std::size_t saved = _state.offset;

        parse_result<inner_result> r = m_p.parse(_state);

        _state.offset = saved;

        if (r.ok())
        {
            return output_type::make_error(
                DParseStatusFailure,
                saved,
                "not_followed_by: inner parser succeeded");
        }

        return output_type(true);
    }

private:
    P m_p;
};

// not_followed_by
//   factory: PEG negative look-ahead.
template<typename P>
D_NODISCARD
not_followed_by_parser<P>
not_followed_by(
    P _p
)
{
    static_assert(is_parser<P>::value,
                  "not_followed_by: argument must be a parser expression");

    return not_followed_by_parser<P>(static_cast<P&&>(_p));
}


// try_parser
//   class: runs _p; on failure restores the input offset.  Ordered
// choice already does this between branches; try_parser is the named
// no-op for documenting intent where backtracking is structural.
template<typename P>
class try_parser
    : public parser_expr<try_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using result_type  = internal::child_result<P>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    explicit try_parser(
        P _p
    )
        : m_p(static_cast<P&&>(_p))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        std::size_t saved = _state.offset;

        output_type r = m_p.parse(_state);

        if (!r.ok())
        {
            _state.offset = saved;
        }

        return r;
    }

private:
    P m_p;
};

// try_
//   factory: backtracking wrap.  Trailing underscore avoids the
// C++ `try` keyword.
template<typename P>
D_NODISCARD
try_parser<P>
try_(
    P _p
)
{
    static_assert(is_parser<P>::value,
                  "try_: argument must be a parser expression");

    return try_parser<P>(static_cast<P&&>(_p));
}


// ================================================================
//  VIII. Diagnostic
// ================================================================

// label_parser
//   class: replaces the error message on failure; passes through on
// success.  Used at grammar rule boundaries so diagnostics reflect
// the rule that failed rather than its deepest atomic mismatch.
template<typename P>
class label_parser
    : public parser_expr<label_parser<P>>
{
public:
    using input_type   = internal::child_input<P>;
    using element_type = input_type;
    using result_type  = internal::child_result<P>;
    using value_type   = result_type;
    using state_type   = parse_state<input_type>;
    using output_type  = parse_result<result_type>;

    label_parser(
        P                  _p,
        const std::string& _message
    )
        : m_p      (static_cast<P&&>(_p)),
          m_message(_message)
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        std::size_t saved = _state.offset;

        output_type r = m_p.parse(_state);

        if (r.ok())
        {
            return r;
        }

        return output_type::make_error(
            r.error().status(),
            saved,
            m_message);
    }

private:
    P           m_p;
    std::string m_message;
};

// label
//   factory: relabels the failure message of an inner parser.
template<typename P>
D_NODISCARD
label_parser<P>
label(
    P                  _p,
    const std::string& _message
)
{
    static_assert(is_parser<P>::value,
                  "label: argument must be a parser expression");

    return label_parser<P>(static_cast<P&&>(_p), _message);
}


NS_END  // parse
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_PARSER_COMBINATORS_HPP
