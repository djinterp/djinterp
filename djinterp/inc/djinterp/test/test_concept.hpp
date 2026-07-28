/******************************************************************************
* djinterp [test]                                             test_concept.hpp
*
*   The concept-TESTING toolkit: the counterpart to test_traits.hpp for the
* other half of the framework's compile-time surface.  Where test_traits.hpp
* asks whether a SFINAE / detection trait behaves like a trait, this header
* asks whether a C++20 concept - or an ad-hoc constraint - behaves like a
* concept, and whether the concept face of a facility genuinely agrees with
* its trait face.
*
*   WHY IT HAS TO BE ITS OWN THING - AND WHY IT IS SMALLER THAN ITS SIBLING:
*   A concept is not tested the way a trait is tested, and the differences cut
* BOTH ways.  Three of them shape everything below:
*
*     1. The negative case is FREE.  A trait's honest negative - "this
*        expression is NOT valid for _Type" - cannot be spelled by writing the
*        expression, because an ill-formed expression is a hard error, not a
*        `false`; test_traits.hpp exists largely to route that negative through
*        a SFINAE context.  A concept needs none of it: `C<_Type>` is a well-
*        formed `bool` whether or not `_Type` satisfies C, and `!C<_Type>` is
*        never a diagnostic.  The requires-expression IS the SFINAE context.
*        So this header has no probe-declarator zoo for the negative; the
*        constraint itself is the probe.
*
*     2. Batteries do not SHORT-CIRCUIT dangerously.  test_traits.hpp forbids
*        conjunction / disjunction in a battery because they stop instantiating
*        at the first false / true, hiding a later cell that fails to COMPILE.
*        A well-formed concept over a type argument cannot fail to compile - it
*        is true or false - so a plain fold `( C<_Types> && ... )` is safe.
*        The only ways `C<T>` hard-errors are a broken concept DEFINITION
*        (caught once, at C's own point of definition) or a wrong-arity / wrong-
*        kind argument list (uniform across the pack); neither is hidden by a
*        short circuit.  The non-short-circuiting quantifiers are therefore not
*        re-derived here - they are borrowed, via (3), for the ONE thing they
*        still buy: forcing every cell to instantiate so a definition that is
*        secretly ill-formed for a hostile shape breaks the build.
*
*     3. SUBSUMPTION is a whole new axis, and concepts are NOT first-class.  A
*        trait answers a question; a concept answers a question AND carries a
*        position in a partial order that the compiler consults during overload
*        resolution.  Two concepts can give the same `bool` for every type and
*        still differ in that order - and a refinement ladder that only implies
*        (rather than subsumes) is a latent ambiguity, not an equivalent
*        spelling.  Nothing on the trait side has this shape.  Making it
*        testable is the bulk of what is genuinely new here (III).  It is made
*        harder by the fact that a concept cannot be passed as a template
*        argument the way a trait can: there is no `template<...> concept _C`
*        parameter.  So every reuse of the trait-side machinery (III, V, VI of
*        test_traits.hpp) has to go through a bridge that LIFTS a concept into a
*        one-parameter `bool_constant` trait (II).  That bridge is the linchpin
*        of this header; once a concept is lifted, the entire trait toolkit -
*        holds_for_all / any / none, count_holds, trait_across_cvref, the
*        hostile fixture lists - applies to it unchanged.
*
*   WHAT A CONCEPT TEST ACTUALLY ASKS:
*     1. is the constraint satisfied by _Type?                        (I, II)
*     2. does that answer hold across a SET of types, positively and
*        negatively - including the types nobody thought to try?   (II -> lift)
*     3. does it answer identically for _Type, const _Type&, ...?  (II -> lift)
*     4. does concept A SUBSUME concept B - and does a refinement
*        ladder genuinely subsume rather than merely imply?             (III)
*     5. when two overloads are constrained on A and B, which one
*        wins for _Type - and is the pair silently AMBIGUOUS?           (III)
*     6. does the concept face AGREE with the trait face, at a point,
*        across a battery, and across the hostile zoo - non-vacuously? (IV)
*
*   USES, DOES NOT RE-SPELL:
*   This header includes test_traits.hpp and stands on it.  The type-set
* quantifiers (holds_for_all / holds_for_any / holds_for_none / count_holds),
* the cv-ref report (trait_across_cvref / trait_ignores_cvref / cvref_report),
* the hostile fixture lists (D_TEST_HOSTILE_TYPES and its kin), the build-time
* pin (D_TEST_STATIC), the single-point agreement primitive
* (D_TEST_TRAIT_CONCEPT_AGREE) and the strict-subsumption bool
* (D_TEST_DECLARE_SUBSUMES) are all reused from there, unqualified, and are
* NOT redefined here.  What is added is only what a concept needs that a trait
* does not: the vocabulary to spell an ad-hoc constraint (I), the lift that
* opens the trait toolkit to concepts (II), the full subsumption REPORT that
* names direction / ambiguity / non-witness where D_TEST_DECLARE_SUBSUMES gives
* one bool (III), and the agreement DECLARATORS that turn "trait vs concept"
* into a trait the quantifiers can carry across a battery (IV).
*
*   HARD FAILURE vs REPORTED FAILURE:
*   As in test_traits.hpp, almost everything reduces to a `constexpr bool` a
* suite hands to its own check macro, so a broken concept shows up as a red
* line in the console and the PDF rather than a dead build.  D_TEST_STATIC (VI,
* inherited) is the opposite choice - it fails the BUILD - and is reserved for
* the handful of invariants whose regression should stop the line.
*
*   PORTABILITY:
*   Concepts are C++20.  The substantive body of this header therefore lives
* under `#if D_ENV_CPP_FEATURE_LANG_CONCEPTS` and SELF-SUPPRESSES to nothing
* below it, exactly as functional_concepts.hpp does - so a translation unit may
* include this header unconditionally and gate its own concept tests on the
* same macro (as the DTest suite headers already do for their concept blocks).
* The C++17 floor is inherited from test_traits.hpp, which this header includes;
* it is not lowered here.  This header does NOT include concepts.hpp: it tests
* whatever concept it is handed, library or user, and stays independent of any
* particular concept surface.
*
*
* TABLE OF CONTENTS
* =================
* I.    CONSTRAINT DECLARATORS
* II.   LIFTING & SATISFACTION
* III.  SUBSUMPTION & ORDERING
* IV.   TRAIT / CONCEPT AGREEMENT
* V.    FIXTURES  (concepts & witnesses)
* VI.   BUILD-TIME PINS
*
*
* path:      /inc/djinterp/test/test_concept.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.26
******************************************************************************/

#ifndef DJINTERP_TEST_CONCEPT_
#define DJINTERP_TEST_CONCEPT_ 1

#ifndef __cplusplus
    #error "test_concept.hpp requires C++ compilation"
#endif  // __cplusplus

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/meta/type_traits.hpp"    // detected_or_t, bool_constant
#include "./test_traits.hpp"               // the trait toolkit lifted into
#include "./test_common.hpp"


#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
    #error "test_concept.hpp requires C++17 or higher (via test_traits.hpp)"
#endif  // !D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                I.   CONSTRAINT DECLARATORS                               ///
///////////////////////////////////////////////////////////////////////////////
//
//   A trait test needs probe declarators because its subject - a raw
// expression - cannot be handed around without first being wrapped so that
// ill-formedness reads as `false`.  A concept test needs declarators for the
// opposite reason: its subject IS already a boolean constraint, but a test
// often wants to name a constraint that no library concept spells - a
// conjunction of two, a bare requires-expression, the exact thing an overload
// in the code under test is constrained on - so that the sections below can
// lift it, subsume against it, and agree a trait with it by NAME.
//
//   These emit concepts.  Every declarator names its type parameter `_Type`
// (and `_Other`, for the binary forms), so the constraint body is written
// against those names.  They are concept TEMPLATES, so they must be declared
// at namespace scope - never inside a test function - and, like the macros in
// test_traits.hpp section I, they are namespace-agnostic: the concept lands in
// whatever namespace the macro is invoked in (normally the suite's own
// djinterp::testing).
//
//   There is deliberately no separate "negative" declarator.  The negative of
// a constraint C is `!C`, which is always well-formed; the whole apparatus a
// trait test needs to make its negative testable is absent here because the
// language already provides it.

// D_TEST_CONSTRAINT
//   macro: names a one-parameter constraint over `_Type` from an arbitrary
// boolean constraint-expression.  This is the GENERAL declarator - the body is
// any expression usable in a requires-clause: a library concept-id, a
// conjunction / disjunction of several, an inline requires-expression, a fold,
// or any mixture.  Everything the other declarators do can be done here; they
// are conveniences for the common shapes.
//
//   The body may contain top-level commas (e.g. `std::same_as<_Type, int>`);
// it is spliced through `__VA_ARGS__`, so they pass through intact.
//
// Usage:
//   D_TEST_CONSTRAINT(sized_and_named,
//                     ::djinterp::sizeable<_Type> &&
//                     requires(const _Type& _t) { _t.name(); })
//   ...
//   sized_and_named<my_type>          // a bool, usable anywhere
#define D_TEST_CONSTRAINT(CONCEPT_NAME, ...)                                  \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = __VA_ARGS__;

// D_TEST_CONSTRAINT_2
//   macro: the binary form of D_TEST_CONSTRAINT, over `_Type` and `_Other` -
// the shape every cross-type constraint wants (comparability, assignability,
// convertibility, common-reference).
//
// Usage:
//   D_TEST_CONSTRAINT_2(mutually_comparable,
//                       ::djinterp::equality_comparable_with<_Type, _Other>)
//   ...
//   mutually_comparable<my_type, other_type>
#define D_TEST_CONSTRAINT_2(CONCEPT_NAME, ...)                               \
    template<typename _Type,                                                  \
             typename _Other>                                                 \
    concept CONCEPT_NAME = __VA_ARGS__;

// D_TEST_REQUIRES
//   macro: names a one-parameter constraint whose body is a REQUIREMENT
// SEQUENCE - the concept-side counterpart to test_traits.hpp's
// D_TEST_EXPR_PROBE.  A mutable lvalue `_t` of type `_Type&` is in scope, and
// each requirement is written against it and terminated with `;` (a simple
// requirement `_t.foo();`, a compound requirement `{ _t.size() } ->
// std::convertible_to<std::size_t>;`, a type requirement `typename
// _Type::value_type;`, or a nested requirement `requires C<_Type>;`).
//
//   Where D_TEST_EXPR_PROBE has to be READ through a detection trait to turn
// its well-formedness into a bool, this yields the bool directly: an
// unsatisfiable requirement makes the concept `false`, never a diagnostic.
// For finer value-category control than a bound `_Type&` lvalue gives, reach
// for D_TEST_CONSTRAINT with an explicit requires-expression
// (`requires(const _Type& _t) { ... }`) or `std::declval` instead.
//
// Usage:
//   D_TEST_REQUIRES(has_push_pop,
//                   _t.push(std::declval<int>());
//                   _t.pop();)
//   ...
//   has_push_pop<my_stack>
#define D_TEST_REQUIRES(CONCEPT_NAME, ...)                                   \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires(_Type& _t) {                              \
        __VA_ARGS__                                                           \
    };

// D_TEST_REQUIRES_2
//   macro: the binary form of D_TEST_REQUIRES.  Two mutable lvalues, `_t` of
// type `_Type&` and `_o` of type `_Other&`, are in scope for the requirement
// sequence.
//
// Usage:
//   D_TEST_REQUIRES_2(assignable_from_other,
//                     _t = _o;)
//   ...
//   assignable_from_other<my_type, other_type>
#define D_TEST_REQUIRES_2(CONCEPT_NAME, ...)                                 \
    template<typename _Type,                                                  \
             typename _Other>                                                 \
    concept CONCEPT_NAME = requires(_Type& _t, _Other& _o) {                  \
        __VA_ARGS__                                                           \
    };


///////////////////////////////////////////////////////////////////////////////
///                II.  LIFTING & SATISFACTION                               ///
///////////////////////////////////////////////////////////////////////////////
//
//   POINT satisfaction needs nothing from this header: `C<my_type>` is already
// a `constexpr bool` a suite can hand straight to its check macro, and
// `!C<my_type>` is the negative.  What a concept CANNOT do on its own is travel
// into the trait-side machinery - the quantifiers, the cv-ref report, the
// hostile fixture lists - because a concept is not a template argument: there
// is no `template<typename> concept` parameter to receive it.
//
//   The bridge is to LIFT the concept into a one-parameter `bool_constant`
// trait.  That trait IS a `template<typename> typename`, so it drops straight
// into everything in test_traits.hpp that takes one:
//
//     holds_for_all<lift, T...>     the positive battery     (no short circuit)
//     holds_for_any<lift, T...>     at least one
//     holds_for_none<lift, T...>    the negative battery
//     count_holds<lift, T...>       how many - and, run over the hostile zoo,
//                                   the blunt instrument that FORCES `C<T>` to
//                                   form for every hostile T: a concept whose
//                                   definition is secretly ill-formed for an
//                                   unclassifiable shape (void, a function
//                                   type, ...) breaks the build here, exactly
//                                   as a non-SFINAE-friendly trait does
//     trait_across_cvref<lift, T>   the cv-ref cells + first_disagreement()
//     trait_ignores_cvref<lift, T>  the cv-ref agreement bool
//
//   A concept that forgets to strip cv-ref is a common defect - it answers
// about `const _Type&` differently from `_Type` - and lifting is what lets the
// cv-ref report from test_traits.hpp section V find it without a line of new
// machinery.
//
//   Batteries fold safely for concepts (see the header preamble), so
// holds_for_all over a lifted concept and the bare fold `( C<T> && ... )`
// agree; the lift is preferred only because it also unlocks the fixtures and
// the cv-ref report, and because it keeps a concept battery spelled the same
// way as a trait battery.

// D_TEST_CONCEPT_TRAIT
//   macro: lifts a one-parameter concept CONCEPT into a `bool_constant` trait
// named TRAIT_NAME, plus its `_v` companion.  The emitted trait's `::value` is
// `CONCEPT<_Type>`; feed the trait to any one-parameter facility in
// test_traits.hpp.  CONCEPT is pasted, so a qualified name is fine.
//
//   (Concepts imply variable templates - both are C++20 - so the `_v`
// companion is emitted unconditionally within this concept-gated header.)
//
// Usage:
//   D_TEST_CONCEPT_TRAIT(sizeable_t, ::djinterp::sizeable)
//   ...
//   holds_for_all<sizeable_t, std::vector<int>, std::string>::value
//   holds_for_none<sizeable_t, D_TEST_HOSTILE_TYPES>::value
//   trait_across_cvref<sizeable_t, std::vector<int>>().agrees()
#define D_TEST_CONCEPT_TRAIT(TRAIT_NAME, CONCEPT)                            \
    template<typename _Type>                                                  \
    struct TRAIT_NAME                                                         \
        : ::djinterp::bool_constant<CONCEPT<_Type>>                           \
    {};                                                                       \
                                                                              \
    template<typename _Type>                                                  \
    D_CONSTEXPR bool TRAIT_NAME##_v = CONCEPT<_Type>;

// D_TEST_CONCEPT_TRAIT_2
//   macro: the binary form - lifts a two-parameter concept into a
// `bool_constant` trait over `_Type` and `_Other` (plus `_v`).  To hand a
// binary lift to the one-parameter quantifiers, bind one argument with a thin
// alias, e.g. `template<typename _T> using bound = my_lift<_T, int>;` and pass
// `bound`.  Variadic concepts (three-plus arguments) need a hand-written
// wrapper on the same pattern; there is no general macro for arbitrary arity.
//
// Usage:
//   D_TEST_CONCEPT_TRAIT_2(convertible_t, ::djinterp::convertible_to)
//   ...
//   convertible_t<int, long>::value
#define D_TEST_CONCEPT_TRAIT_2(TRAIT_NAME, CONCEPT)                          \
    template<typename _Type,                                                  \
             typename _Other>                                                 \
    struct TRAIT_NAME                                                         \
        : ::djinterp::bool_constant<CONCEPT<_Type, _Other>>                   \
    {};                                                                       \
                                                                              \
    template<typename _Type,                                                  \
             typename _Other>                                                 \
    D_CONSTEXPR bool TRAIT_NAME##_v = CONCEPT<_Type, _Other>;


///////////////////////////////////////////////////////////////////////////////
///                III. SUBSUMPTION & ORDERING                               ///
///////////////////////////////////////////////////////////////////////////////
//
//   Subsumption is the fact about concepts that has no analog on the trait
// side, and it is the fact most easily gotten wrong.  Two concepts can be
// satisfied by exactly the same types and still stand in a different place in
// the partial order the compiler uses to break ties between constrained
// overloads.  The framework layers refinement ladders throughout - a stronger
// concept "refines" a weaker one - and states that the stronger overload wins.
// It wins only if the stronger concept genuinely SUBSUMES the weaker; a ladder
// assembled by copy-pasting a weaker concept's requirements into a stronger one
// satisfies every IMPLICATION test (every type modelling the stronger models
// the weaker) and then makes every pair of overloads constrained on the two
// AMBIGUOUS.  Implication is a fact about satisfaction; subsumption is a fact
// about the constraint's SPELLING.  Only the second is what overload resolution
// consults, and only the second is what this section measures.
//
//   Subsumption cannot be asked of the compiler as a bool - it has no operator
// - so it is OBSERVED through its one effect: partial ordering of constrained
// overloads.  The observation is only meaningful at a WITNESS type that
// satisfies both concepts, because a concept a witness does not satisfy
// contributes no candidate to compete.  (If MORE genuinely subsumes LESS then
// every model of MORE is a model of LESS, so any type satisfying MORE is
// automatically a common witness - the natural witness for a ladder rung is a
// type satisfying the strongest concept on it.)
//
//   The mechanism, following test_traits.hpp's D_TEST_DECLARE_SUBSUMES: three
// overloads of a ranked constexpr function - one constrained on A (rank 1), one
// on B (rank 2), one UNCONSTRAINED (rank 3).  A satisfied constrained candidate
// is preferred over the unconstrained one, so the fallback is chosen only when
// the witness satisfies neither.  The call is made inside a detection probe, so
// an AMBIGUOUS overload set - two satisfied constraints neither of which is
// more constrained - is a substitution failure that reads back as a sentinel
// rather than a diagnostic.  The four observationally-distinct outcomes:
//
//     code 1   the witness satisfies A, and A is at least as constrained as B
//              here (A wins outright: A strictly subsumes B, or B is not
//              satisfied so A is unopposed)
//     code 2   symmetric - B wins outright
//     code 0   the witness satisfies BOTH and neither strictly subsumes the
//              other: the overload set is ambiguous.  This is the ambiguity
//              trap - and it covers two sub-cases the ranking alone cannot
//              separate: A and B are subsumption-EQUIVALENT, or A and B are
//              INCOMPARABLE.  Distinguishing those needs a second observation -
//              whether A and B are satisfied by the SAME set of types - which
//              is a lifted-agreement question (IV): equivalent constraints
//              agree everywhere, incomparable ones differ on some type
//     code 3   the witness satisfies NEITHER, so no ordering was observed
//
//   Because a witness satisfying only one concept also yields code 1 or 2, the
// direction bools below are gated on `both`: they report a genuine subsumption
// ONLY at a common witness, and are all false (with `both` false) otherwise, so
// a mis-chosen witness cannot masquerade as a subsumption result.

// D_TEST_DECLARE_ORDERING
//   macro: emits `TRAIT_NAME<_Witness>` - a carrier trait whose static members
// report the constraint ordering of CONCEPT_A against CONCEPT_B as observed at
// `_Witness`.  This is the RICH report; for the plain boolean "does the more-
// constrained concept strictly subsume the less-constrained one", the one-name
// form is test_traits.hpp's D_TEST_DECLARE_SUBSUMES.
//
//   Members of TRAIT_NAME<_Witness>:
//     code            the raw outcome (1 / 2 / 0 / 3, as above)
//     both            _Witness satisfies both concepts (the reading is
//                     meaningful iff this is true)
//     a_subsumes_b    both && A strictly subsumes B here      (code 1 at a
//                     common witness)
//     b_subsumes_a    both && B strictly subsumes A here      (code 2)
//     unordered       both && neither subsumes the other      (code 0) - the
//                     ambiguity trap; assert this is FALSE for a healthy ladder
//                     rung, TRUE to pin two concepts as siblings that must
//                     never overload against each other
//     neither         _Witness satisfies neither concept
//
//   Like D_TEST_DECLARE_SUBSUMES it opens an `internal` namespace for its
// ranked helpers, so it must be invoked at namespace scope inside djinterp.
// Names derive from TRAIT_NAME, so each invocation needs a distinct one.  Both
// concepts are pasted, so either may be qualified.
//
// Usage:
//   D_TEST_DECLARE_ORDERING(refine_vs_base,
//                           fixtures::refines_a,      // A: written via has_a
//                           fixtures::has_a)          // B: the base
//   ...
//   refine_vs_base<fixtures::owns_ab>::a_subsumes_b   // true  (genuine refine)
//
//   D_TEST_DECLARE_ORDERING(restate_vs_base,
//                           fixtures::restates_a,     // A: copy-pasted body
//                           fixtures::has_a)          // B: the base
//   ...
//   restate_vs_base<fixtures::owns_ab>::unordered     // true  (the trap)
#define D_TEST_DECLARE_ORDERING(TRAIT_NAME, CONCEPT_A, CONCEPT_B)            \
    NS_INTERNAL                                                               \
                                                                             \
        /* TRAIT_NAME##_rank                                              */  \
        /*   function: the A-constrained overload (rank 1).              */   \
        template<typename _Type>                                             \
            requires CONCEPT_A<_Type>                                        \
        D_CONSTEXPR int                                                      \
        TRAIT_NAME##_rank()                                                  \
        {                                                                    \
            return 1;                                                        \
        }                                                                    \
                                                                             \
        /* TRAIT_NAME##_rank                                              */  \
        /*   function: the B-constrained overload (rank 2).              */   \
        template<typename _Type>                                             \
            requires CONCEPT_B<_Type>                                        \
        D_CONSTEXPR int                                                      \
        TRAIT_NAME##_rank()                                                  \
        {                                                                    \
            return 2;                                                        \
        }                                                                    \
                                                                             \
        /* TRAIT_NAME##_rank                                              */  \
        /*   function: the unconstrained fallback (rank 3).  Chosen only */  \
        /* when the witness satisfies neither constraint.                */  \
        template<typename _Type>                                             \
        D_CONSTEXPR int                                                      \
        TRAIT_NAME##_rank()                                                  \
        {                                                                    \
            return 3;                                                        \
        }                                                                    \
                                                                             \
        /* TRAIT_NAME##_probe                                            */  \
        /*   trait: the ranked call, in a detection context - an         */  \
        /* ambiguous overload set is a substitution failure here, not a  */  \
        /* diagnostic.                                                   */   \
        template<typename _Type>                                             \
        using TRAIT_NAME##_probe =                                           \
            std::integral_constant<int, TRAIT_NAME##_rank<_Type>()>;         \
                                                                             \
    NS_END  /* internal */                                                   \
                                                                             \
    /* TRAIT_NAME                                                        */  \
    /*   trait: the constraint-ordering report for CONCEPT_A against     */  \
    /* CONCEPT_B, observed at _Witness.                                  */   \
    template<typename _Witness>                                              \
    struct TRAIT_NAME                                                        \
    {                                                                        \
        static D_CONSTEXPR int code =                                        \
            ::djinterp::detected_or_t<                                       \
                std::integral_constant<int, 0>,                             \
                internal::TRAIT_NAME##_probe,                                \
                _Witness>::value;                                            \
                                                                             \
        static D_CONSTEXPR bool both =                                       \
            ( CONCEPT_A<_Witness> && CONCEPT_B<_Witness> );                  \
                                                                             \
        static D_CONSTEXPR bool a_subsumes_b = ( both && (code == 1) );      \
        static D_CONSTEXPR bool b_subsumes_a = ( both && (code == 2) );      \
        static D_CONSTEXPR bool unordered    = ( both && (code == 0) );      \
                                                                             \
        static D_CONSTEXPR bool neither =                                    \
            ( (!CONCEPT_A<_Witness>) && (!CONCEPT_B<_Witness>) );            \
    };

//   REFINEMENT LADDERS.  A ladder C0 (strongest) refines C1 refines ... refines
// Cn (weakest) is a chain in which each concept STRICTLY subsumes the next.
// There is no dedicated ladder macro because a ladder is just a conjunction of
// adjacent orderings observed at one witness - a type satisfying C0, which by
// subsumption satisfies every rung - and reads cleanly as such:
//
//   D_TEST_DECLARE_ORDERING(rung_01, c0, c1)
//   D_TEST_DECLARE_ORDERING(rung_12, c1, c2)
//   ...
//   const bool ladder_ok =
//       rung_01<witness_c0>::a_subsumes_b &&
//       rung_12<witness_c0>::a_subsumes_b;
//
//   Asserting the ladder over the STRONGEST witness is deliberate: it is the
// one witness guaranteed to satisfy both operands of every rung, so every
// a_subsumes_b in the conjunction is read at a common witness and none degrades
// to an unopposed win.


///////////////////////////////////////////////////////////////////////////////
///                IV.  TRAIT / CONCEPT AGREEMENT                            ///
///////////////////////////////////////////////////////////////////////////////
//
//   The framework carries, for almost every trait, a concept face, and asserts
// in header after header that "the two agree by construction".  They agree by
// construction only while the concept keeps forwarding to the trait; the day
// someone tightens one and not the other, the trait-constrained overload and
// the concept-constrained overload begin to disagree about the same type, and
// nothing in the build says so.  A constraint `IsFoo` and a trait `is_foo`
// that are meant to be the same predicate must therefore be TESTED to be the
// same predicate.
//
//   The single-point check already exists, in test_traits.hpp:
//
//     D_TEST_TRAIT_CONCEPT_AGREE(TRAIT, CONCEPT, ARGS...)
//       -> ( (TRAIT<ARGS...>::value) == (CONCEPT<ARGS...>) )
//
// It is the right primitive for "agree for THIS type"; it is not enough on its
// own, for two reasons.  First, a point is not a battery: the place a trait and
// its concept drift is on the type nobody re-checked, which is exactly the
// hostile zoo.  Second, agreement is VACUOUSLY satisfied by two predicates that
// are both uniformly false - "they never disagree" is trivially true when
// neither is ever true - so an agreement test that does not also pin polarity
// proves nothing.
//
//   The declarator below closes the first gap by turning "trait vs concept"
// into a one-parameter trait, at which point the entire quantifier + fixture
// arsenal of test_traits.hpp applies TO AGREEMENT ITSELF - agreement across a
// battery, across the hostile zoo, even across cv-ref forms.  The second gap is
// closed by pairing that agreement with a polarity pin, shown below.

// D_TEST_DECLARE_AGREEMENT
//   macro: emits `TRAIT_NAME<_Type>` - a `bool_constant` trait true iff
// `TRAIT<_Type>::value` and `CONCEPT<_Type>` give the SAME answer for `_Type` -
// plus its `_v` companion.  Because it is a one-parameter trait, it feeds
// directly into the section III quantifiers of test_traits.hpp:
//
//     holds_for_all<TRAIT_NAME, D_TEST_HOSTILE_TYPES>::value
//       -> the trait and the concept agree across the entire hostile zoo (and,
//          because count_holds instantiates every cell, this simultaneously
//          proves the trait is SFINAE-friendly over the zoo - a hard-erroring
//          trait breaks the build here; the concept side is a bool for every
//          type by construction)
//     trait_ignores_cvref<TRAIT_NAME, _Type>::value
//       -> they agree identically under every cv-ref qualification of _Type
//
//   TRAIT and CONCEPT are pasted in that order (matching
// D_TEST_TRAIT_CONCEPT_AGREE), so either may be qualified.  Pass the trait's
// NAME and the concept's NAME, not instantiations.
//
// NON-VACUITY.  Agreement alone does not distinguish "the same true predicate"
// from "two predicates that are both always false".  Pin polarity alongside it,
// using the lifted concept (II) or the trait:
//
//   D_TEST_DECLARE_AGREEMENT(foo_agree, is_foo, IsFoo)
//   D_TEST_CONCEPT_TRAIT(is_foo_c, IsFoo)
//   ...
//   const bool same_predicate =
//       holds_for_all<foo_agree, D_TEST_HOSTILE_TYPES>::value &&  // agree, and
//       holds_for_any<is_foo_c, known_foo_a, known_foo_b>::value &&  // not
//       holds_for_none<is_foo_c, D_TEST_HOSTILE_TYPES>::value;       // vacuous
//
// Usage:
//   D_TEST_DECLARE_AGREEMENT(sized_agree, is_sized, sizeable)
//   ...
//   holds_for_all<sized_agree, std::vector<int>, D_TEST_HOSTILE_TYPES>::value
#define D_TEST_DECLARE_AGREEMENT(TRAIT_NAME, TRAIT, CONCEPT)                 \
    template<typename _Type>                                                  \
    struct TRAIT_NAME                                                         \
        : ::djinterp::bool_constant<                                          \
            ( (TRAIT<_Type>::value) == (CONCEPT<_Type>) )>                    \
    {};                                                                       \
                                                                              \
    template<typename _Type>                                                  \
    D_CONSTEXPR bool TRAIT_NAME##_v =                                         \
        ( (TRAIT<_Type>::value) == (CONCEPT<_Type>) );

// D_TEST_DECLARE_AGREEMENT_2
//   macro: the binary form - emits `TRAIT_NAME<_Type, _Other>`, true iff a
// two-parameter TRAIT and a two-parameter CONCEPT agree for the pair (plus
// `_v`).  Bind one argument with a thin alias to feed it to the one-parameter
// quantifiers, exactly as for D_TEST_CONCEPT_TRAIT_2.
//
// Usage:
//   D_TEST_DECLARE_AGREEMENT_2(conv_agree, is_convertible, convertible_to)
//   ...
//   conv_agree<int, long>::value
#define D_TEST_DECLARE_AGREEMENT_2(TRAIT_NAME, TRAIT, CONCEPT)               \
    template<typename _Type,                                                  \
             typename _Other>                                                 \
    struct TRAIT_NAME                                                         \
        : ::djinterp::bool_constant<                                          \
            ( (TRAIT<_Type, _Other>::value) == (CONCEPT<_Type, _Other>) )>   \
    {};                                                                       \
                                                                              \
    template<typename _Type,                                                  \
             typename _Other>                                                 \
    D_CONSTEXPR bool TRAIT_NAME##_v =                                         \
        ( (TRAIT<_Type, _Other>::value) == (CONCEPT<_Type, _Other>) );


///////////////////////////////////////////////////////////////////////////////
///                V.   FIXTURES  (concepts & witnesses)                     ///
///////////////////////////////////////////////////////////////////////////////
//
//   test_traits.hpp ships a zoo of hostile TYPES - the shapes that make a
// careless trait wrong rather than merely false - and this header reuses every
// one of them through the lift (II).  What it cannot borrow is example
// CONCEPTS, because subsumption and agreement are relations BETWEEN constraints
// and a self-test of sections III and IV needs constraints with a known
// answer.  The four concepts below are that set, and they exist to make the
// implication-versus-subsumption distinction concrete:
//
//     has_a          the base constraint: `_t.a()`
//     refines_a      a GENUINE refinement of has_a - spelled `has_a<_Type> &&
//                    has_b<_Type>`, so `has_a`'s atomic constraint literally
//                    appears in it and it therefore SUBSUMES has_a
//     restates_a     the same SATISFACTION as refines_a - spelled as one
//                    requires-expression `{ _t.a(); _t.b(); }` - so it is a
//                    fresh atomic constraint that does NOT subsume has_a: every
//                    type modelling it models has_a (implication), yet the two
//                    are unordered (the trap)
//     has_z          an UNRELATED constraint: `_t.z()` - disjoint satisfaction
//                    from has_a and unordered with it
//
//   With the witness structs, `refines_a` vs `has_a` at `owns_ab` reads
// a_subsumes_b, while `restates_a` vs `has_a` at `owns_ab` reads unordered -
// the two outcomes the ordering report exists to tell apart.  The witnesses
// declare, and do not define, their members: a concept names them only in the
// unevaluated context of a requires-expression, so there is nothing to link.

NS_FIXTURES

// owns_a
//   struct: exposes a() only.  Satisfies has_a; refutes refines_a / restates_a
// (no b()).  The single-concept witness for has_a.
struct owns_a
{
    void a();
};

// owns_ab
//   struct: exposes a() and b().  Satisfies has_a, has_b, refines_a, and
// restates_a - the common witness at which the ordering of refines_a /
// restates_a against has_a is read.
struct owns_ab
{
    void a();

    void b();
};

// owns_z
//   struct: exposes z() only.  Satisfies has_z alone - the witness that
// satisfies neither operand of a has_a-based ordering (code 3 / neither).
struct owns_z
{
    void z();
};

// has_a
//   concept: the base constraint - `_Type` has a callable a().
template<typename _Type>
concept has_a = requires(_Type& _t)
{
    _t.a();
};

// has_b
//   concept: helper constraint - `_Type` has a callable b().  Named so that
// refines_a can be spelled in terms of it.
template<typename _Type>
concept has_b = requires(_Type& _t)
{
    _t.b();
};

// refines_a
//   concept: a genuine refinement of has_a.  Spelled as `has_a<_Type> &&
// has_b<_Type>`, so has_a's atomic constraint is a conjunct of it and it
// SUBSUMES has_a.  Same satisfaction as restates_a, different spelling - and
// the difference is the whole point.
template<typename _Type>
concept refines_a = has_a<_Type> && has_b<_Type>;

// restates_a
//   concept: the same satisfaction as refines_a, spelled as ONE requires-
// expression rather than in terms of has_a.  It is a fresh atomic constraint,
// so it does NOT subsume has_a even though every type modelling it models has_a
// - the implication-is-not-subsumption fixture.
template<typename _Type>
concept restates_a = requires(_Type& _t)
{
    _t.a();
    _t.b();
};

// has_z
//   concept: an unrelated constraint - `_Type` has a callable z().  Disjoint
// satisfaction from has_a and unordered with it; the fixture for an
// incomparable pair.
template<typename _Type>
concept has_z = requires(_Type& _t)
{
    _t.z();
};

NS_END  // fixtures


///////////////////////////////////////////////////////////////////////////////
///                VI.  BUILD-TIME PINS                                      ///
///////////////////////////////////////////////////////////////////////////////
//
//   The framework's default is a REPORTED failure: every check above reduces to
// a `constexpr bool` a suite hands to its check macro, and a regression shows
// up as a red line in the console and the PDF.  D_TEST_STATIC - inherited from
// test_traits.hpp, not redefined here - is the opposite choice: it fails the
// BUILD, and so is reserved for the handful of concept-side invariants whose
// regression should stop the line, because everything downstream of them is
// meaningless once they break.
//
//   Two are worth pinning this way.  First, that a refinement ladder still
// SUBSUMES rather than merely implies: the moment a rung degrades to
// implication, every pair of overloads constrained on it becomes ambiguous, so
// a suite that only reported it would be reporting from code that no longer
// compiles at its real call sites.  Second, that a trait and its concept face
// are still the SAME predicate on the type they are most likely to have drifted
// on: a stale concept face silently inverts overload resolution at every site
// that prefers the concept, and no downstream test of either face alone catches
// it.
//
// Usage:
//   // pin: refines_a still strictly subsumes has_a (the ladder rung holds)
//   D_TEST_DECLARE_ORDERING(refine_pin, fixtures::refines_a, fixtures::has_a)
//   D_TEST_STATIC(refine_pin<fixtures::owns_ab>::a_subsumes_b);
//
//   // pin: is_foo and IsFoo still agree across the hostile zoo
//   D_TEST_DECLARE_AGREEMENT(foo_pin, is_foo, IsFoo)
//   D_TEST_STATIC(
//       holds_for_all<foo_pin, D_TEST_HOSTILE_TYPES>::value);


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_CONCEPT_
