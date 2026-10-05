/*******************************************************************************
* djinterp [core]                                                      match.hpp
*
*   Ordered first-match dispatch over patterns.  Where pattern.hpp gives a
* single four-faced view of one input and filter.hpp gives predicate-based
* selection over a collection, match.hpp is the layer between them: an
* ordered list of (pattern -> handler) clauses evaluated first-match-wins,
* producing an output.  It is the value-level analogue of a `switch` whose
* labels are patterns and whose arms receive the captures the matching
* pattern bound.
*
*   FIRST-MATCH-WINS IS ORDERED CHOICE.  Clauses are tried in registration
* order and the first whose pattern matches (and whose guard, if any, holds)
* commits -- the same total-order-makes-a-nondeterministic-combine-
* deterministic device as the parser's PEG ordered choice (or_) and as
* sequential override at execution time.  Registration order therefore *is*
* the clause precedence.
*
*   TWO PATHS (mirroring parser_expr/parser and filter's typed/erased
* forms):
*     - match_clause<Pattern, Handler>  -- a static, zero-erasure clause;
*           composes and inspects without indirection.
*     - matcher<Input, Output>          -- a runtime, std::function-erased
*           accumulation of clauses: the dispatch table.  Uniform Output
*           across arms, like a match expression.
*
*   HANDLERS.  A handler is invoked with the input and (optionally) the
* capture map the pattern produced -- whichever of
*     handler(const input&, const captures&)
*     handler(const input&)
* is well-formed, preferring the capture-aware form.  Capture-oblivious
* clauses skip extraction entirely.  A handler's result must be convertible
* to the matcher's Output.
*
*   OUTCOMES.  apply() yields maybe<Output> (nothing == no clause matched);
* apply_result() yields result<Output, match_status> (err == no match) for
* callers that want a typed reason.  A void-returning (side-effecting)
* matcher uses a unit Output type, per result.hpp's convention.
*
*   NOT HERE.  Mapping a matcher across a collection is a filter/range
* concern; only the thin apply_range() bridge lives here.  Option-set-driven
* configuration is layered on separately.
*
*
* path:      /inc/djinterp/core/paradigm/match/match.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    STATUS CODES
      ------------

II.   HANDLER & GUARD TRAITS
      ----------------------

III.  MATCH CLAUSE (static)
      ---------------------
      a. match_clause
      b. when

IV.   MATCHER (runtime, type-erased)
      ------------------------------

V.    FREE-FUNCTION HELPERS
      ---------------------
      a. make_matcher
      b. apply_range

VI.   STRUCTURAL TRAITS
      -----------------

VII.  C++20 CONCEPTS
      --------------
*/

#ifndef DJINTERP_PARADIGM_MATCH_MATCH_HPP
#define DJINTERP_PARADIGM_MATCH_MATCH_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_utility.hpp"  // void_t
#include "../../functional/maybe.hpp"
#include "../../functional/result.hpp"
#include "../pattern/pattern.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::int32_t


NS_DJINTERP


//   match.hpp names no new carrier: a clause is a pattern (pattern.hpp)
// paired with a handler, evaluation returns maybe / result (the functional
// carriers), and the ordered-choice semantics are shared verbatim with the
// parser's or_ and with sequential override.  The module is therefore pure
// composition over surfaces defined elsewhere.


///////////////////////////////////////////////////////////////////////////////
///                I.   STATUS CODES                                        ///
///////////////////////////////////////////////////////////////////////////////

// match_status
//   typedef: classifies the outcome of a match operation.  Mirrors
// pattern_status / parse_status so the three status vocabularies read
// alike.
typedef re_std::int32_t match_status;

// DMatchStatus*
//   constants: standard match status codes.  Callers may define
// additional codes at or above DMatchStatusUserBase.
constexpr match_status DMatchStatusOk          =  0;
constexpr match_status DMatchStatusNoMatch     =  1;
constexpr match_status DMatchStatusGuardFailed =  2;
constexpr match_status DMatchStatusUserBase    = 64;


///////////////////////////////////////////////////////////////////////////////
///                II.  HANDLER & GUARD TRAITS                              ///
///////////////////////////////////////////////////////////////////////////////
//
//   A handler is accepted in either of two shapes -- capture-aware
//   (input, captures) or capture-oblivious (input) -- with the
//   capture-aware form preferred where both are viable.  These traits
//   detect the shapes and name the resulting type; the invoke helper
//   performs the ranked dispatch.
//

NS_INTERNAL

    // no_result
    //   type: sentinel returned by the result picker when a candidate
    // handler matches neither accepted shape.
    struct no_result
    {};

    // handler_takes_input_captures
    //   trait: detects a callable handler(const In&, const Caps&).
    template<typename Handler,
             typename In,
             typename Caps,
             typename = void>
    struct handler_takes_input_captures : std::false_type
    {};

    template<typename Handler,
             typename In,
             typename Caps>
    struct handler_takes_input_captures<Handler, In, Caps, void_t<
        decltype(
            std::declval<const Handler&>()(
                std::declval<const In&>(),
                std::declval<const Caps&>()))
    >> : std::true_type
    {};

    // handler_takes_input
    //   trait: detects a callable handler(const In&).
    template<typename Handler,
             typename In,
             typename = void>
    struct handler_takes_input : std::false_type
    {};

    template<typename Handler,
             typename In>
    struct handler_takes_input<Handler, In, void_t<
        decltype(
            std::declval<const Handler&>()(
                std::declval<const In&>()))
    >> : std::true_type
    {};

    // handler_result_picker
    //   trait: resolves the handler's return type, preferring the
    // capture-aware shape.  Primary template (neither shape) yields the
    // no_result sentinel.
    template<typename Handler,
             typename In,
             typename Caps,
             bool CaptureAware = handler_takes_input_captures<
                                      Handler, In, Caps>::value,
             bool InputOnly     = handler_takes_input<
                                      Handler, In>::value>
    struct handler_result_picker
    {
        using type = no_result;
    };

    template<typename Handler,
             typename In,
             typename Caps,
             bool InputOnly>
    struct handler_result_picker<Handler, In, Caps, true, InputOnly>
    {
        using type = decltype(
            std::declval<const Handler&>()(
                std::declval<const In&>(),
                std::declval<const Caps&>()));
    };

    template<typename Handler,
             typename In,
             typename Caps>
    struct handler_result_picker<Handler, In, Caps, false, true>
    {
        using type = decltype(
            std::declval<const Handler&>()(
                std::declval<const In&>()));
    };

    // invoke_handler (capture-aware, rank 0)
    //   helper: preferred overload -- calls handler(input, captures).
    template<typename Handler,
             typename In,
             typename Caps>
    auto
    invoke_handler(
        const Handler& _handler,
        const In&      _in,
        const Caps&    _caps,
        int             /*_rank*/
    )
    -> decltype(_handler(_in, _caps))
    {
        return _handler(_in, _caps);
    }

    // invoke_handler (capture-oblivious, rank 1)
    //   helper: fallback overload -- calls handler(input).
    template<typename Handler,
             typename In,
             typename Caps>
    auto
    invoke_handler(
        const Handler& _handler,
        const In&      _in,
        const Caps&    /*_caps*/,
        long            /*_rank*/
    )
    -> decltype(_handler(_in))
    {
        return _handler(_in);
    }

NS_END  // internal


// match_handler_result
//   trait: the return type of Handler when applied to Input (with the
// pattern's Captures available), or the no_result sentinel if Handler
// fits neither accepted shape.
template<typename Handler,
         typename Input,
         typename Captures>
struct match_handler_result
{
    using type = typename internal::handler_result_picker<
        Handler, Input, Captures>::type;
};

// match_handler_result_t
//   type: convenience alias for match_handler_result<...>::type.
template<typename Handler,
         typename Input,
         typename Captures>
using match_handler_result_t =
    typename match_handler_result<Handler, Input, Captures>::type;


// is_match_handler
//   trait: true iff Handler is callable in one of the accepted shapes
// -- (input, captures) or (input).
template<typename Handler,
         typename Input,
         typename Captures>
struct is_match_handler
{
    static constexpr bool value =
        ( internal::handler_takes_input_captures<
              Handler, Input, Captures>::value ||
          internal::handler_takes_input<
              Handler, Input>::value );
};

// is_input_handler
//   trait: true iff Handler is callable as (const Input&) -- the shape
// accepted by matcher::otherwise.
template<typename Handler,
         typename Input>
struct is_input_handler
{
    static constexpr bool value =
        internal::handler_takes_input<Handler, Input>::value;
};


// is_match_guard
//   trait: true iff Guard is callable as (const Input&) with a
// bool-convertible result -- the shape accepted by the guarded when().
template<typename Guard,
         typename Input,
         typename = void>
struct is_match_guard : std::false_type
{};

template<typename Guard,
         typename Input>
struct is_match_guard<Guard, Input, void_t<
    decltype(
        static_cast<bool>(
            std::declval<const Guard&>()(
                std::declval<const Input&>())))
>> : std::true_type
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_match_handler_v
//   constant: shorthand for is_match_handler<...>::value.
template<typename Handler,
         typename Input,
         typename Captures>
static D_CONSTEXPR bool is_match_handler_v =
    is_match_handler<Handler, Input, Captures>::value;

// is_match_guard_v
//   constant: shorthand for is_match_guard<Guard, Input>::value.
template<typename Guard,
         typename Input>
static D_CONSTEXPR bool is_match_guard_v =
    is_match_guard<Guard, Input>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///                III. MATCH CLAUSE (static)                              ///
///////////////////////////////////////////////////////////////////////////////

// =================================================================
//  a. match_clause
// =================================================================

// match_clause
//   class: a static (pattern, handler) pairing -- one arm of a match,
// with no type erasure.  Exposes the applicability test and the arm
// action separately, and a try_apply() that fuses them.  The pattern's
// input_type and capture_map_type are inherited through the pattern<>
// CRTP base, so the clause draws them from the pattern rather than
// redeclaring them.
template<typename Pattern,
         typename Handler>
class match_clause
{
public:
    using pattern_type     = Pattern;
    using handler_type     = Handler;
    using input_type       = typename Pattern::input_type;
    using capture_map_type = typename Pattern::capture_map_type;
    using result_type      =
        match_handler_result_t<Handler, input_type, capture_map_type>;

    static_assert(
        is_pattern<Pattern>::value,
        "match_clause: Pattern must satisfy the pattern protocol "
        "(the four do_* faces of pattern.hpp).");

    static_assert(
        !std::is_same<result_type, internal::no_result>::value,
        "match_clause: Handler must be callable as "
        "(const input_type&, const captures&) or (const input_type&).");

    match_clause(
        Pattern _pattern,
        Handler _handler
    )
        : m_pattern(std::move(_pattern)),
          m_handler(std::move(_handler))
    {}

    // matches
    //   method: true iff the input conforms to the clause's pattern.
    D_NODISCARD
    bool
    matches(
        const input_type& _in
    ) const
    {
        return m_pattern.match(_in);
    }

    // run
    //   method: invokes the handler on the input, threading the pattern's
    // captures when the handler accepts them.  Does not itself test the
    // pattern -- see try_apply for the guarded form.
    D_NODISCARD
    result_type
    run(
        const input_type& _in
    ) const
    {
        capture_map_type caps = m_pattern.extract(_in).captures;

        return internal::invoke_handler(m_handler, _in, caps, 0);
    }

    // try_apply
    //   method: matches ? just(run(in)) : nothing.  The clause as a
    // partial function.
    D_NODISCARD
    maybe<result_type>
    try_apply(
        const input_type& _in
    ) const
    {
        if (m_pattern.match(_in))
        {
            return just(run(_in));
        }

        return nothing<result_type>();
    }

    // introspection
    D_NODISCARD const Pattern& pattern() const { return m_pattern; }
    D_NODISCARD const Handler& handler() const { return m_handler; }

private:
    Pattern m_pattern;
    Handler m_handler;
};


// =================================================================
//  b. when
// =================================================================

// when
//   function: constructs a match_clause from a pattern and a handler.
// The spelling reads as the head of a matcher arm -- when(p, h).
template<typename Pattern,
         typename Handler>
D_NODISCARD
match_clause<typename std::decay<Pattern>::type,
             typename std::decay<Handler>::type>
when
(
    Pattern&& _pattern,
    Handler&& _handler
)
{
    return match_clause<
        typename std::decay<Pattern>::type,
        typename std::decay<Handler>::type>(
            std::forward<Pattern>(_pattern),
            std::forward<Handler>(_handler));
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  MATCHER (runtime, type-erased)                     ///
///////////////////////////////////////////////////////////////////////////////

// matcher
//   class: an ordered dispatch table -- a growable list of clauses over a
// shared input_type, each yielding a shared output_type.  Clauses are
// erased to (test, run) function pairs (mirroring filter's std::function
// choice over virtual dispatch), so heterogeneous patterns and handlers
// coexist in one container.  apply() evaluates first-match-wins.
template<typename Input,
         typename Output>
class matcher
{
public:
    using input_type  = Input;
    using output_type = Output;
    using status_type = match_status;

    matcher()
        : m_clauses  (),
          m_otherwise()
    {}

    // -----------------------------------------------------------------
    //  clause registration
    // -----------------------------------------------------------------

    // when
    //   method: appends a clause -- when the input matches _pattern, the
    // arm _handler produces the output.  Registration order is clause
    // precedence.  Returns *this for chaining.
    template<typename Pattern,
             typename Handler>
    matcher&
    when
    (
        Pattern _pattern,
        Handler _handler
    )
    {
        static_assert(
            is_pattern<Pattern>::value,
            "matcher::when: Pattern must satisfy the pattern protocol.");

        using caps_type = typename Pattern::capture_map_type;
        using result_t  =
            match_handler_result_t<Handler, input_type, caps_type>;

        static_assert(
            !std::is_same<result_t, internal::no_result>::value,
            "matcher::when: Handler must be callable as "
            "(const input_type&, const captures&) or (const input_type&).");

        static_assert(
            std::is_convertible<result_t, output_type>::value,
            "matcher::when: handler result must be convertible to Output.");

        erased_clause clause;

        clause.test = make_test(_pattern);
        clause.run  = make_run(
            std::integral_constant<bool,
                internal::handler_takes_input_captures<
                    Handler, input_type, caps_type>::value>(),
            _pattern,
            _handler);

        m_clauses.push_back(std::move(clause));

        return *this;
    }

    // when (guarded)
    //   method: appends a clause gated by an additional guard predicate;
    // the clause applies only when _pattern matches AND _guard(input) is
    // true.  Returns *this for chaining.
    template<typename Pattern,
             typename Guard,
             typename Handler>
    matcher&
    when
    (
        Pattern _pattern,
        Guard    _guard,
        Handler _handler
    )
    {
        static_assert(
            is_pattern<Pattern>::value,
            "matcher::when: Pattern must satisfy the pattern protocol.");

        static_assert(
            is_match_guard<Guard, input_type>::value,
            "matcher::when: Guard must be callable as "
            "(const input_type&) -> bool.");

        using caps_type = typename Pattern::capture_map_type;
        using result_t  =
            match_handler_result_t<Handler, input_type, caps_type>;

        static_assert(
            !std::is_same<result_t, internal::no_result>::value,
            "matcher::when: Handler must be callable as "
            "(const input_type&, const captures&) or (const input_type&).");

        static_assert(
            std::is_convertible<result_t, output_type>::value,
            "matcher::when: handler result must be convertible to Output.");

        erased_clause clause;

        clause.test = make_guarded_test(_pattern, _guard);
        clause.run  = make_run(
            std::integral_constant<bool,
                internal::handler_takes_input_captures<
                    Handler, input_type, caps_type>::value>(),
            _pattern,
            _handler);

        m_clauses.push_back(std::move(clause));

        return *this;
    }

    // otherwise
    //   method: sets the fallback arm, run when no clause matched.  The
    // handler is a callable (const input_type&) -> Output.  Returns *this.
    template<typename Handler,
             typename std::enable_if<
                 is_input_handler<Handler, input_type>::value,
                 int>::type = 0>
    matcher&
    otherwise
    (
        Handler _handler
    )
    {
        m_otherwise = [_handler](const input_type& _in) -> output_type
        {
            return _handler(_in);
        };

        return *this;
    }

    // otherwise_value
    //   method: sets a constant fallback value, returned when no clause
    // matched.  Returns *this.
    matcher&
    otherwise_value
    (
        const output_type& _value
    )
    {
        m_otherwise = [_value](const input_type& /*_in*/) -> output_type
        {
            return _value;
        };

        return *this;
    }

    // -----------------------------------------------------------------
    //  evaluation
    // -----------------------------------------------------------------

    // apply
    //   method: evaluates first-match-wins.  Returns just(output) for the
    // first matching clause; if none match, just(otherwise(input)) when a
    // fallback is set, else nothing.
    D_NODISCARD
    maybe<output_type>
    apply
    (
        const input_type& _in
    ) const
    {
        for (const erased_clause& _c : m_clauses)
        {
            if (_c.test(_in))
            {
                return just(_c.run(_in));
            }
        }

        if (m_otherwise)
        {
            return just(m_otherwise(_in));
        }

        return nothing<output_type>();
    }

    // apply_result
    //   method: as apply(), but reports the unmatched case as a typed
    // error -- err(DMatchStatusNoMatch) -- rather than nothing.
    D_NODISCARD
    result<output_type, status_type>
    apply_result
    (
        const input_type& _in
    ) const
    {
        for (const erased_clause& _c : m_clauses)
        {
            if (_c.test(_in))
            {
                return ok<output_type, status_type>(_c.run(_in));
            }
        }

        if (m_otherwise)
        {
            return ok<output_type, status_type>(m_otherwise(_in));
        }

        return err<output_type, status_type>(DMatchStatusNoMatch);
    }

    // -----------------------------------------------------------------
    //  introspection
    // -----------------------------------------------------------------

    D_NODISCARD
    std::size_t   size()          const { return m_clauses.size();  }

    D_NODISCARD
    bool          empty()         const { return m_clauses.empty(); }

    D_NODISCARD
    bool          has_otherwise() const
    {
        return static_cast<bool>(m_otherwise);
    }

private:
    // erased_clause
    //   struct: a type-erased (test, run) pair -- the runtime form of one
    // clause.  test decides applicability; run produces the output.
    struct erased_clause
    {
        std::function<bool(const input_type&)>        test;
        std::function<output_type(const input_type&)> run;
    };

    // make_test
    //   helper: erases a pattern to its applicability test.
    template<typename Pattern>
    static std::function<bool(const input_type&)>
    make_test
    (
        const Pattern& _pattern
    )
    {
        return [_pattern](const input_type& _in) -> bool
        {
            return _pattern.match(_in);
        };
    }

    // make_guarded_test
    //   helper: erases a pattern and guard to a conjoined test.
    template<typename Pattern,
             typename Guard>
    static std::function<bool(const input_type&)>
    make_guarded_test
    (
        const Pattern& _pattern,
        const Guard&   _guard
    )
    {
        return [_pattern, _guard](const input_type& _in) -> bool
        {
            return ( _pattern.match(_in) &&
                     static_cast<bool>(_guard(_in)) );
        };
    }

    // make_run (capture-aware)
    //   helper: the arm extracts the pattern's captures and threads them
    // to the handler.
    template<typename Pattern,
             typename Handler>
    static std::function<output_type(const input_type&)>
    make_run
    (
        std::true_type  /*_capture_aware*/,
        const Pattern& _pattern,
        const Handler& _handler
    )
    {
        return [_pattern, _handler](const input_type& _in) -> output_type
        {
            typename Pattern::capture_map_type caps =
                _pattern.extract(_in).captures;

            return _handler(_in, caps);
        };
    }

    // make_run (capture-oblivious)
    //   helper: the arm ignores captures, so extraction is skipped.
    template<typename Pattern,
             typename Handler>
    static std::function<output_type(const input_type&)>
    make_run
    (
        std::false_type /*_capture_aware*/,
        const Pattern& /*_pattern*/,
        const Handler& _handler
    )
    {
        return [_handler](const input_type& _in) -> output_type
        {
            return _handler(_in);
        };
    }

    std::vector<erased_clause>                    m_clauses;
    std::function<output_type(const input_type&)> m_otherwise;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   FREE-FUNCTION HELPERS                              ///
///////////////////////////////////////////////////////////////////////////////

// =================================================================
//  a. make_matcher
// =================================================================

// make_matcher
//   function: constructs an empty matcher<Input, Output>, letting the
// caller open a fluent chain without spelling the class template twice.
template<typename Input,
         typename Output>
D_NODISCARD
matcher<Input, Output>
make_matcher()
{
    return matcher<Input, Output>();
}


// =================================================================
//  b. apply_range
// =================================================================

// apply_range
//   function: applies a matcher across [_first, _last), collecting the
// output of each element that matched (unmatched elements are skipped,
// unless the matcher carries an otherwise arm, in which case every
// element yields).  This is the thin bridge to collection matching;
// richer set-theoretic collection dispatch belongs to the filter module.
template<typename Matcher,
         typename Iterator>
D_NODISCARD
std::vector<typename Matcher::output_type>
apply_range
(
    const Matcher& _matcher,
    Iterator        _first,
    Iterator        _last
)
{
    std::vector<typename Matcher::output_type> out;

    // fold the matcher across the range, keeping the hits
    for (Iterator _it = _first; _it != _last; ++_it)
    {
        maybe<typename Matcher::output_type> r = _matcher.apply(*_it);

        if (r.has_value())
        {
            out.push_back(r.value());
        }
    }

    return out;
}


///////////////////////////////////////////////////////////////////////////////
///                VI.  STRUCTURAL TRAITS                                  ///
///////////////////////////////////////////////////////////////////////////////

// is_match_clause
//   trait: detects the static clause surface -- a nested input_type plus
// a matches(input) -> bool face.  Satisfied by match_clause<> and any
// user type of the same shape.
template<typename Type,
         typename = void>
struct is_match_clause : std::false_type
{};

template<typename Type>
struct is_match_clause<Type, void_t<
    typename Type::input_type,
    decltype(
        static_cast<bool>(
            std::declval<const Type&>().matches(
                std::declval<const typename Type::input_type&>())))
>> : std::true_type
{};


// is_matcher_like
//   trait: detects the runtime matcher surface -- nested input_type and
// output_type plus an apply(input) face.  Element-agnostic beyond those.
template<typename Type,
         typename = void>
struct is_matcher_like : std::false_type
{};

template<typename Type>
struct is_matcher_like<Type, void_t<
    typename Type::input_type,
    typename Type::output_type,
    decltype(
        std::declval<const Type&>().apply(
            std::declval<const typename Type::input_type&>()))
>> : std::true_type
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_match_clause_v
//   constant: shorthand for is_match_clause<Type>::value.
template<typename Type>
static D_CONSTEXPR bool is_match_clause_v = is_match_clause<Type>::value;

// is_matcher_like_v
//   constant: shorthand for is_matcher_like<Type>::value.
template<typename Type>
static D_CONSTEXPR bool is_matcher_like_v = is_matcher_like<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///                VII. C++20 CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////

#if ( defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                             \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )

// match_handler_c
//   concept: constrains handlers accepted by a clause over the given
// input and capture-map types.
template<typename Handler,
         typename Input,
         typename Captures>
concept match_handler_c =
    is_match_handler<Handler, Input, Captures>::value;

// match_guard_c
//   concept: constrains guard predicates accepted by the guarded when().
template<typename Guard,
         typename Input>
concept match_guard_c = is_match_guard<Guard, Input>::value;

// match_clause_c
//   concept: satisfied by types exposing the static clause surface.
template<typename Type>
concept match_clause_c = is_match_clause<Type>::value;

// matcher_c
//   concept: satisfied by types exposing the runtime matcher surface.
template<typename Type>
concept matcher_c = is_matcher_like<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARADIGM_MATCH_MATCH_HPP
