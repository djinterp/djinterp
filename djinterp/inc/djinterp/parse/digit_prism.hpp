/******************************************************************************
* djinterp [parse]                                  example/digit_prism.hpp
*
* Worked example: the prism as cata / ana over a live polynomial functor.
*   The smallest non-trivial μF that exercises the whole apparatus:
*
*       D            = std::vector<char>            (a list of digits)
*       F(A)         = 1 + (char × A)               the list base functor
*       D ≅ μF       with 1 = nil,  cons = char × A
*
*   compose = cata[φ] : μF → Σ*      φ : F(Σ*) → Σ*   (this file's algebra)
*   parse   =  ana(partial) : Σ* ⇀ μF                 many(digit())
*   unfold  =  ana[ψ]  : Σ* → μF      ψ : Σ* → F(Σ*)   (the *total* dual)
*
*   Two things this example is meant to prove, in running code:
*
*   1. The base functor F is built entirely from the poly_* vocabulary
*      (polynomial.hpp) --
*
*          list_base<A> =
*              poly_sum< poly_unit<A>,
*                        poly_product< poly_const<char, A>,
*                                      poly_var<A> > >
*
*      -- so F is a genuine, registered Functor, and `compose` is a
*      real catamorphism (functional/recursion.hpp's cata) rather
*      than a hand-rolled recursive hook.  The recursive position is
*      the poly_var<A>; cata threads the fold through exactly there.
*
*   2. Because the fold is cata over recursive_traits<D>, the *very
*      same* structure map serves every interpretation.  `compose`
*      hands cata a print algebra; an evaluator hands it an
*      arithmetic algebra; a pretty-printer a layout algebra -- with
*      no change to the recursion.  Swapping the algebra is the only
*      edit.  (An eval algebra is given in section V to discharge
*      exactly this claim.)
*
*   The carrier stays a plain std::vector<char> -- recursive_traits /
* corecursive_traits expose it as an F-layer via head/tail, so no
* rewrite into an explicit fixed point is needed.  That is the point
* of the structure-map approach: native carriers participate as-is.
*
*   USAGE
*       using namespace djinterp::parse;
*       using namespace djinterp::parse::example;
*
*       auto p = make_digit_prism();
*       digit_list t = unfold_digits("123");           // ana : "123" -> ['1','2','3']
*       std::string s = p.compose(t);                  // cata : ['1','2','3'] -> "123"
*       parse_result<digit_list> back = p.parse(s);    // back == t
*       long n = eval_digits(t);                        // cata (other algebra) -> 123
*
*
* path:      /inc/djinterp/parse/digit_prism.hpp
* link(s):   ch-parsing.tex, ch-recursion.tex
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_PARSE_EXAMPLE_DIGIT_PRISM_
#define DJINTERP_PARSE_EXAMPLE_DIGIT_PRISM_ 1

// std
#include <string>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "../core/functional/recursion.hpp"
#include "parse.hpp"
#include "parser/parser.hpp"
#include "primitives.hpp"
#include "parser/combinators.hpp"
#include "../core/functional/polynomial.hpp"
#include "prism.hpp"


// ================================================================
//  I.   the carrier and its base functor
// ================================================================

NS_DJINTERP
NS_PARSE

namespace example {

// digit_list
//   alias: the carrier D -- a sequence of ASCII decimal digits.
using digit_list = std::vector<char>;


// list_base
//   alias: the base functor F(A) = 1 + (char × A), spelled in the
// poly_* vocabulary.  The left arm is nil (poly_unit); the right
// arm is cons: a head char (poly_const, X phantom) times a tail
// (poly_var, the recursive position).  Because every constituent
// is a registered Functor, list_base<A> is one too -- which is what
// lets cata fold it.
template<typename _A>
using list_base =
    poly_sum< poly_unit<_A>,
              poly_product< poly_const<char, _A>,
                            poly_var<_A> > >;

}  // namespace example

NS_END  // parse
NS_END  // djinterp


// ================================================================
//  II.  the structure maps  (recursive_traits / corecursive_traits)
// ================================================================
//   These live at djinterp:: scope, beside the recursion-scheme
// primaries.  They say "digit_list IS a mu(list_base)" by exposing
// one layer in each direction -- project peels head/tail, embed
// prepends.  With them in place, cata and ana work over digit_list
// with no further glue.

NS_DJINTERP

// recursive_traits<digit_list>
//   specialisation: the fold-side structure map.  project reads one
// layer of F from the vector -- nil when empty, else cons(front,
// rest) -- with the tail carried in the poly_var recursive slot.
template<>
struct recursive_traits< ::djinterp::parse::example::digit_list, void>
{
    using is_specialized = std::true_type;

    template<typename _A>
    using base = ::djinterp::parse::example::list_base<_A>;

    static
    ::djinterp::parse::example::list_base<
        ::djinterp::parse::example::digit_list>
    project(
        const ::djinterp::parse::example::digit_list& _v
    )
    {
        namespace ex = ::djinterp::parse::example;
        namespace ps = ::djinterp::parse;

        using carrier = ex::digit_list;
        using nil_t   = ps::poly_unit<carrier>;
        using head_t  = ps::poly_const<char, carrier>;
        using tail_t  = ps::poly_var<carrier>;
        using cons_t  = ps::poly_product<head_t, tail_t>;
        using layer_t = ex::list_base<carrier>;

        if (_v.empty())
        {
            return layer_t::inj_left(nil_t());
        }

        carrier rest(_v.begin() + 1, _v.end());

        return layer_t::inj_right(
            cons_t(head_t(_v.front()), tail_t(rest)));
    }
};


// corecursive_traits<digit_list>
//   specialisation: the build-side structure map.  embed assembles
// one vector from an F-layer whose children are already vectors --
// nil yields the empty vector, cons prepends the head to the tail.
template<>
struct corecursive_traits< ::djinterp::parse::example::digit_list, void>
{
    using is_specialized = std::true_type;

    template<typename _A>
    using base = ::djinterp::parse::example::list_base<_A>;

    static
    ::djinterp::parse::example::digit_list
    embed(
        const ::djinterp::parse::example::list_base<
            ::djinterp::parse::example::digit_list>& _layer
    )
    {
        namespace ex = ::djinterp::parse::example;

        using carrier = ex::digit_list;

        if (_layer.is_left)
        {
            return carrier();
        }

        char           head = _layer.right.first.content;
        const carrier& rest = _layer.right.second.content;

        carrier out;
        out.reserve(rest.size() + 1);
        out.push_back(head);
        out.insert(out.end(), rest.begin(), rest.end());

        return out;
    }
};

NS_END  // djinterp


// ================================================================
//  III. the print algebra + the prism
// ================================================================

NS_DJINTERP
NS_PARSE

// compose_traits<digit_list, char>
//   specialisation: the print algebra φ : F(Σ*) → Σ*.  It sees ONE
// layer whose recursive child has already been rendered to a string
// by the surrounding cata: nil -> "", cons(head, tail_string) ->
// head ++ tail_string.  No recursion here -- that is cata's job.
template<>
struct compose_traits<example::digit_list, char>
{
    using is_specialized = std::true_type;

    static
    std::string
    algebra(
        const recursive_traits<example::digit_list>::template base<
            std::string>& _layer
    )
    {
        if (_layer.is_left)
        {
            return std::string();
        }

        char               head = _layer.right.first.content;
        const std::string& tail = _layer.right.second.content;

        std::string out;
        out.reserve(tail.size() + 1);
        out.push_back(head);
        out.append(tail);

        return out;
    }
};


namespace example {

// make_digit_parser
//   factory: the parse leg -- many(digit()) collecting a digit_list.
D_NODISCARD
inline parser<digit_list, char>
make_digit_parser()
{
    return parser<digit_list, char>(many(digit()));
}


// make_digit_prism
//   factory: bundles the parse leg with the compose leg.  The
// compose leg is cata[φ] over recursive_traits<digit_list> with
// φ = compose_traits<digit_list>::algebra -- supplied automatically
// by make_prism_via_traits.
D_NODISCARD
inline prism<digit_list, char>
make_digit_prism()
{
    return make_prism_via_traits<digit_list, char>(
        make_digit_parser());
}


// ================================================================
//  IV.  the dual:  ana[ψ] -- build the list by unfolding a string
// ================================================================

// digit_coalgebra
//   function: the F-coalgebra ψ : Σ* → F(Σ*).  One unfold step:
// empty string -> nil, else cons(first char, rest string).  The
// tail seed rides in the poly_var slot, mirroring project exactly.
D_NODISCARD
inline list_base<std::string>
digit_coalgebra(
    const std::string& _s
)
{
    using nil_t   = poly_unit<std::string>;
    using head_t  = poly_const<char, std::string>;
    using tail_t  = poly_var<std::string>;
    using cons_t  = poly_product<head_t, tail_t>;
    using layer_t = list_base<std::string>;

    if (_s.empty())
    {
        return layer_t::inj_left(nil_t());
    }

    return layer_t::inj_right(
        cons_t(head_t(_s[0]), tail_t(_s.substr(1))));
}


// unfold_digits
//   function: ana[ψ] : Σ* → μF.  The total dual of parse -- it
// unfolds a string into a digit_list via corecursive_traits<
// digit_list>::embed.  (parse is the *partial* ana, defined only on
// the digit language; this total one accepts any string char-for-
// char.)  A round trip unfold_digits then compose is the identity
// on strings, witnessing the ana/cata duality directly.
D_NODISCARD
inline digit_list
unfold_digits(
    const std::string& _s
)
{
    return ::djinterp::ana<digit_list, std::string>(
        digit_coalgebra, _s);
}


// ================================================================
//  V.   swap the algebra, keep the recursion:  an evaluator
// ================================================================
//   Nothing above is compose-specific except `algebra`.  To show
// that, here is a *different* fold over the *same* recursive_traits<
// digit_list> -- it reads the digit list as a base-10 numeral.
// There is no new recursion: eval_digits is cata again, with an
// arithmetic algebra in place of the print algebra.  This is the
// "compose and evaluate are one catamorphism" claim, discharged.

NS_INTERNAL

    // eval_acc
    //   the value carried up the fold: the numeric value of the
    // suffix seen so far, plus the place multiplier for the next
    // (more significant) digit, i.e. 10^(length of that suffix).
    struct eval_acc
    {
        long value;
        long scale;

        eval_acc()
            : value(0),
              scale(1)
        {}

        eval_acc(long _v, long _s)
            : value(_v),
              scale(_s)
        {}
    };

    // digit_value_algebra
    //   the F-algebra for base-10 evaluation.  nil -> (0, 1);
    // cons(d, (v, m)) -> ( d*m + v , m*10 ) -- the fold arrives
    // tail-first, so the head is the most significant digit and its
    // place value is the tail's accumulated scale.
    inline eval_acc
    digit_value_algebra(
        const recursive_traits<example::digit_list>::template base<
            eval_acc>& _layer
    )
    {
        if (_layer.is_left)
        {
            return eval_acc(0, 1);
        }

        char            head = _layer.right.first.content;
        const eval_acc& tail = _layer.right.second.content;

        long digit = static_cast<long>(head - '0');

        return eval_acc(
            (digit * tail.scale) + tail.value,
            tail.scale * 10);
    }

NS_END  // internal


// eval_digits
//   function: cata over the SAME structure map, arithmetic algebra.
// Reads the digit list as a base-10 integer.  ['1','2','3'] -> 123.
D_NODISCARD
inline long
eval_digits(
    const digit_list& _v
)
{
    internal::eval_acc r =
        ::djinterp::cata<internal::eval_acc, digit_list>(
            internal::digit_value_algebra, _v);

    return r.value;
}

}  // namespace example


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_EXAMPLE_DIGIT_PRISM_
