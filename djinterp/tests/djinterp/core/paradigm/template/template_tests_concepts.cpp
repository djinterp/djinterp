/*******************************************************************************
* djinterp [core]                                    template_tests_concepts.cpp
*
*
* path:      /tests/djinterp/core/paradigm/template/template_tests_concepts.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : template_tests_concepts.cpp
//   Section V: the C++20 concept parallels, following the module's trait-triple
// convention - where an is_X trait exists, a concept lives alongside it.
//
//   A concept is only worth having if it is SOUND: if it admits a type, that type
// must actually work where the concept gates it.  That is the whole burden of
// this section, and it is why
// tests_concept_transformation_for_rejects_non_const_callable is here rather than
// being treated as a duplicate of the trait test.  The algebra stores F by value
// and invokes it from const member functions, so a functor whose operator() is
// non-const cannot be held.  With the shipped detector - which probed _Fn as an
// rvalue - `transformation_for` ADMITTED such a functor, and the contradiction
// surfaced as a hard error deep inside transformer_helper / template_system.  A
// constraint that accepts what the constrained code then rejects is precisely the
// failure mode concepts exist to prevent.
//
//   The whole section is gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS.  On a dialect
// without concepts the predicates stand down and report success, so the module
// spec is identical across dialects and the runner needs no conditional wiring.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "template_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace
{
    // build_system
    //   fixture: a template CONSTRAINED by the concept under test.  Its only job
    // is to be SFINAE-friendly at the call site, so the suite can ask "would this
    // call compile?" without the answer being a hard error.  If the concept is
    // sound, this is callable for exactly the types the algebra can hold.
    template<typename _Fn>
        requires transformation_for<_Fn, carrier>
    auto
    build_system(
        _Fn _fn
    )
    {
        return make_template_system<carrier>(_fn);
    }

    // buildable
    //   trait: true iff build_system(_Fn{}) is a valid call - i.e. iff the
    // constraint admits _Fn.  Detected, not asserted, so a rejection is an
    // answer rather than a compile error.
    template<typename _Fn, typename = void>
    struct buildable : std::false_type
    {};

    template<typename _Fn>
    struct buildable<_Fn,
                     std::void_t<decltype(build_system(std::declval<_Fn>()))>>
        : std::true_type
    {};
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


/*
tests_concept_transformation_for
  Verifies the concept admits a transformation.
  Tests the following:
  - a correctly-shaped F satisfies transformation_for over the carrier
  - so does one whose sink is not the carrier, and a lambda of the same shape
  - a unary callable and an uncallable do not
*/
bool
tests_concept_transformation_for()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    auto lambda = [](const carrier& _bound_template, const carrier& _source)
    {
        return _bound_template + _source;
    };

    static_assert(transformation_for<concat, carrier>,
                  "concat is a transformation");
    static_assert(transformation_for<total_len, carrier>,
                  "sigma need not be tau");
    static_assert(transformation_for<decltype(lambda), carrier>,
                  "a lambda qualifies");

    static_assert(!transformation_for<unary_only, carrier>,
                  "a unary callable does not");
    static_assert(!transformation_for<not_callable, carrier>,
                  "nor does an uncallable");

    return transformation_for<concat, carrier> &&
           !transformation_for<unary_only, carrier>;
#else
    // no concepts on this dialect; the parallel traits are covered in section I.
    return true;
#endif
}

/*
tests_concept_transformation_for_rejects_non_const_callable
  Verifies the concept is SOUND: it must not admit a functor the algebra cannot
  hold.  transformer_helper and template_system store F by value and invoke it
  from const member functions, so a non-const-callable functor is unusable - and a
  concept that admitted it would push the failure from the constraint (where it
  belongs, as a clean "constraints not satisfied") into the guts of the algebra.
  Tests the following:
  - mutable_binary does NOT satisfy transformation_for
  - a template constrained on the concept is therefore NOT callable with it
  - the same constrained template IS callable with a const-callable F, so the
    constraint is discriminating rather than merely restrictive
*/
bool
tests_concept_transformation_for_rejects_non_const_callable()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(!transformation_for<mutable_binary, carrier>,
                  "the algebra invokes F from a const member; the concept must "
                  "not admit a non-const-callable functor");
    static_assert(transformation_for<concat, carrier>,
                  "while the const-callable counterpart is admitted");

    // the constraint really gates the call, and it gates it the right way round
    static_assert(!buildable<mutable_binary>::value,
                  "a constrained template rejects it AT THE CONSTRAINT");
    static_assert(buildable<concat>::value,
                  "and accepts a transformation the algebra can hold");

    return ( !transformation_for<mutable_binary, carrier> &&
              transformation_for<concat, carrier> &&
             !buildable<mutable_binary>::value &&
              buildable<concat>::value );
#else
    return true;
#endif
}

/*
tests_concept_source_transformer_for
  Verifies the concept admits a source-transformer F_t.
  Tests the following:
  - a unary callable over the carrier satisfies source_transformer_for
  - the F_t that instantiate returns satisfies it - the module's own product
  - a binary transformation does not
*/
bool
tests_concept_source_transformer_for()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    auto transformer = instantiate(concat(), carrier("greet"));

    static_assert(source_transformer_for<unary_only, carrier>,
                  "a unary callable is an F_t");
    static_assert(source_transformer_for<measure, carrier>,
                  "sigma need not be tau");
    static_assert(source_transformer_for<decltype(transformer), carrier>,
                  "and instantiate's own product qualifies");

    static_assert(!source_transformer_for<concat, carrier>,
                  "a binary F is not an F_t");
    static_assert(!source_transformer_for<not_callable, carrier>,
                  "nor is an uncallable");

    return source_transformer_for<decltype(transformer), carrier> &&
           !source_transformer_for<concat, carrier>;
#else
    return true;
#endif
}

/*
tests_concept_source_transformer_for_rejects_non_const_callable
  Verifies the source-transformer concept is sound for the same reason: the kleisli
  helpers store their parsers by value and invoke them from const member functions,
  and the canonical F_t (transformer_helper) is itself const-callable.
  Tests the following:
  - mutable_unary does NOT satisfy source_transformer_for
  - the const-callable counterpart does
*/
bool
tests_concept_source_transformer_for_rejects_non_const_callable()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(!source_transformer_for<mutable_unary, carrier>,
                  "an F_t is stored and invoked from a const member");
    static_assert(source_transformer_for<unary_only, carrier>,
                  "the const-callable counterpart is admitted");

    return ( !source_transformer_for<mutable_unary, carrier> &&
              source_transformer_for<unary_only, carrier> );
#else
    return true;
#endif
}

/*
tests_concept_parse_outcome_c
  Verifies the concept admits the parser sink.
  Tests the following:
  - every parse_outcome specialization satisfies parse_outcome_c
  - unrelated types do not
  - the sink a real parser produces satisfies it
*/
bool
tests_concept_parse_outcome_c()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    using produced = source_transformer_sink_t<digit_p, carrier>;

    static_assert(parse_outcome_c<int_outcome>,
                  "a parse_outcome satisfies the concept");
    static_assert(parse_outcome_c<char_outcome>,
                  "for any specialization");
    static_assert(parse_outcome_c<produced>,
                  "including the sink a real parser produces");

    static_assert(!parse_outcome_c<int>,
                  "an unrelated type does not");
    static_assert(!parse_outcome_c<carrier>,
                  "nor does the carrier");

    return parse_outcome_c<int_outcome> && !parse_outcome_c<int>;
#else
    return true;
#endif
}

/*
tests_concepts_agree_with_their_traits
  Verifies the trait-triple convention holds: each concept is exactly its trait,
  with no drift between the two spellings.
  Tests the following:
  - transformation_for agrees with is_transformation, positive and negative
  - source_transformer_for agrees with is_source_transformer
  - parse_outcome_c agrees with is_parse_outcome
*/
bool
tests_concepts_agree_with_their_traits()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(transformation_for<concat, carrier> ==
                  is_transformation<concat, carrier>::value,
                  "transformation_for == is_transformation (positive)");
    static_assert(transformation_for<mutable_binary, carrier> ==
                  is_transformation<mutable_binary, carrier>::value,
                  "transformation_for == is_transformation (negative)");

    static_assert(source_transformer_for<unary_only, carrier> ==
                  is_source_transformer<unary_only, carrier>::value,
                  "source_transformer_for == is_source_transformer (positive)");
    static_assert(source_transformer_for<mutable_unary, carrier> ==
                  is_source_transformer<mutable_unary, carrier>::value,
                  "source_transformer_for == is_source_transformer (negative)");

    static_assert(parse_outcome_c<int_outcome> ==
                  is_parse_outcome<int_outcome>::value,
                  "parse_outcome_c == is_parse_outcome (positive)");
    static_assert(parse_outcome_c<int> == is_parse_outcome<int>::value,
                  "parse_outcome_c == is_parse_outcome (negative)");

    return true;
#else
    return true;
#endif
}

/*
tests_concept_constrains_a_template
  Verifies the concept does the job it exists for: gating a template so that only
  the types the algebra can actually hold reach it, and so that the ones it cannot
  are turned away AT THE CONSTRAINT rather than deep inside.
  Tests the following:
  - a constrained template accepts a transformation and produces a working system
  - it rejects a unary callable, an uncallable, and a non-const-callable functor
  - the rejections are detectable (SFINAE-friendly), i.e. they are constraint
    failures rather than hard errors
*/
bool
tests_concept_constrains_a_template()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(buildable<concat>::value,
                  "a transformation passes the constraint");
    static_assert(buildable<total_len>::value,
                  "including one whose sink is not the carrier");

    static_assert(!buildable<unary_only>::value,
                  "a unary callable is turned away");
    static_assert(!buildable<not_callable>::value,
                  "so is an uncallable");
    static_assert(!buildable<mutable_binary>::value,
                  "and so is a non-const-callable functor");

    // and what passes really does work
    auto sys = build_system(concat());

    return (sys.apply(carrier("greet"), carrier("world")) == "greet:world");
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
