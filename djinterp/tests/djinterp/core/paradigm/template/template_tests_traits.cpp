/*******************************************************************************
* djinterp [core]                                      template_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/paradigm/template/template_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : template_tests_traits.cpp
//   Section I: the transformation traits.  A source and a template are both
// inhabitants of tau - they are roles, not types - so there is nothing to detect
// for them.  What IS detectable is the shape of a transformation F : tau x tau
// -> sigma and of the source-transformer F_t : tau -> sigma it curries to.  Each
// is_X is defined as "the corresponding sink is detectable", so the sink traits
// and the predicates are tested together.
//
//   The load-bearing case is const-callability.  The algebra stores F BY VALUE
// and invokes it from CONST member functions (transformer_helper::operator(),
// template_system::apply).  A detector that probes _Fn as an rvalue therefore
// admits a functor whose operator() is non-const, and the failure surfaces as a
// hard error deep inside the algebra instead of at the trait - or, worse, at a
// C++20 concept that was supposed to be the gate.  The detector must probe
// through `const _Fn&`, and the two rejects_non_const_callable tests below are
// what pin that.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "template_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // free_transformation
    //   fixture: a plain function, so the traits can be checked against a
    // function POINTER as well as against class-type callables.
    std::string
    free_transformation(
        const std::string& _bound_template,
        const std::string& _source
    )
    {
        return _bound_template + "-" + _source;
    }

    using transformation_ptr = std::string (*)(const std::string&,
                                               const std::string&);
}


/*
tests_transformation_sink_deduces_sigma
  Verifies the sink type sigma is read off F(const tau&, const tau&).
  Tests the following:
  - concat, whose sink is the carrier, deduces std::string
*/
bool
tests_transformation_sink_deduces_sigma()
{
    using sigma = transformation_sink<concat, carrier>::type;

    static_assert(std::is_same<sigma, std::string>::value,
                  "sigma is deduced from F(tau, tau)");

    return std::is_same<sigma, std::string>::value;
}

/*
tests_transformation_sink_sigma_differs_from_carrier
  Verifies sigma is genuinely deduced rather than assumed to be tau.
  Tests the following:
  - total_len, whose sink is a size, deduces std::size_t and not the carrier
*/
bool
tests_transformation_sink_sigma_differs_from_carrier()
{
    using sigma = transformation_sink_t<total_len, carrier>;

    static_assert(std::is_same<sigma, std::size_t>::value,
                  "sigma is size_t, not the carrier");
    static_assert(!std::is_same<sigma, carrier>::value,
                  "sigma is not assumed to be tau");

    return ( std::is_same<sigma, std::size_t>::value &&
            !std::is_same<sigma, carrier>::value );
}

/*
tests_transformation_sink_is_nonesuch_when_not_callable
  Verifies the tagless detector's fallback: on substitution failure the variadic
  probe supplies nonesuch.
  Tests the following:
  - a type with no operator() has a sink of nonesuch
  - so does a binary callable that cannot take const tau& arguments
*/
bool
tests_transformation_sink_is_nonesuch_when_not_callable()
{
    using none  = transformation_sink_t<not_callable, carrier>;
    using none2 = transformation_sink_t<needs_mutable_ref, carrier>;

    static_assert(std::is_same<none, nonesuch>::value,
                  "no operator() -> nonesuch");
    static_assert(std::is_same<none2, nonesuch>::value,
                  "wrong parameter qualification -> nonesuch");

    return ( std::is_same<none, nonesuch>::value &&
             std::is_same<none2, nonesuch>::value );
}

/*
tests_transformation_sink_t_alias
  Verifies the convenience alias tracks the trait.
  Tests the following:
  - transformation_sink_t is transformation_sink<...>::type, for a detectable
    sink and for the nonesuch case alike
*/
bool
tests_transformation_sink_t_alias()
{
    static_assert(std::is_same<transformation_sink_t<concat, carrier>,
                               transformation_sink<concat, carrier>::type>::value,
                  "the alias tracks the trait");
    static_assert(std::is_same<transformation_sink_t<not_callable, carrier>,
                               transformation_sink<not_callable, carrier>::type>::value,
                  "including in the nonesuch case");

    return true;
}

/*
tests_is_transformation_accepts_binary
  Verifies a correctly-shaped F is recognized.
  Tests the following:
  - concat (sigma == tau) is a transformation
  - total_len (sigma != tau) is a transformation too - the sink is unconstrained
*/
bool
tests_is_transformation_accepts_binary()
{
    static_assert(is_transformation<concat, carrier>::value,
                  "concat is a transformation");
    static_assert(is_transformation<total_len, carrier>::value,
                  "so is total_len - sigma is free");

    return ( is_transformation<concat, carrier>::value &&
             is_transformation<total_len, carrier>::value );
}

/*
tests_is_transformation_rejects_unary
  Verifies arity is enforced: a source-transformer is not a transformation.
  Tests the following:
  - a callable taking a single source is not a transformation over the carrier
*/
bool
tests_is_transformation_rejects_unary()
{
    static_assert(!is_transformation<unary_only, carrier>::value,
                  "a unary callable is an F_t, not an F");

    return !is_transformation<unary_only, carrier>::value;
}

/*
tests_is_transformation_rejects_not_callable
  Verifies a type with no call operator is rejected.
  Tests the following:
  - not_callable is not a transformation
*/
bool
tests_is_transformation_rejects_not_callable()
{
    static_assert(!is_transformation<not_callable, carrier>::value,
                  "no operator(), no transformation");

    return !is_transformation<not_callable, carrier>::value;
}

/*
tests_is_transformation_rejects_mutable_ref_parameters
  Verifies parameter qualification is enforced.  The algebra hands F its template
  and source as const tau&, so a binary callable demanding non-const references
  is not a transformation over the carrier however plausible its arity.
  Tests the following:
  - needs_mutable_ref is rejected
*/
bool
tests_is_transformation_rejects_mutable_ref_parameters()
{
    static_assert(!is_transformation<needs_mutable_ref, carrier>::value,
                  "F is invoked with const tau& arguments");

    return !is_transformation<needs_mutable_ref, carrier>::value;
}

/*
tests_is_transformation_rejects_non_const_callable
  Verifies const-callability is enforced - the soundness case.  transformer_helper
  and template_system both store F by value and invoke it from CONST member
  functions, so a functor whose operator() is non-const cannot be held by the
  algebra at all.  A detector that probes _Fn as an rvalue reports such a functor
  as a transformation, and the contradiction then surfaces as a hard error deep
  inside the algebra rather than at the trait that was meant to catch it.
  Tests the following:
  - mutable_binary, correctly shaped but non-const-callable, is NOT a
    transformation
  - the const-callable counterpart still is, so the rule is const-callability and
    not something coarser
*/
bool
tests_is_transformation_rejects_non_const_callable()
{
    static_assert(!is_transformation<mutable_binary, carrier>::value,
                  "the algebra invokes F from a const member; F must be "
                  "const-callable");
    static_assert(is_transformation<concat, carrier>::value,
                  "and the const-callable counterpart is unaffected");

    return ( !is_transformation<mutable_binary, carrier>::value &&
              is_transformation<concat, carrier>::value );
}

/*
tests_is_transformation_v_agrees_with_trait
  Verifies the variable-template shorthand tracks the trait.
  Tests the following:
  - is_transformation_v equals is_transformation<...>::value, positive and
    negative
*/
bool
tests_is_transformation_v_agrees_with_trait()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_transformation_v<concat, carrier> ==
                  is_transformation<concat, carrier>::value,
                  "_v tracks the trait (positive)");
    static_assert(is_transformation_v<unary_only, carrier> ==
                  is_transformation<unary_only, carrier>::value,
                  "_v tracks the trait (negative)");

    return ( is_transformation_v<concat, carrier> &&
            !is_transformation_v<unary_only, carrier> );
#else
    // no variable templates on this dialect; the trait itself is covered above.
    return true;
#endif
}

/*
tests_source_transformer_sink_deduces_sigma
  Verifies the sink of an F_t is read off g(const tau&).
  Tests the following:
  - unary_only deduces std::string
  - a unary stage whose sink is a size deduces std::size_t
*/
bool
tests_source_transformer_sink_deduces_sigma()
{
    using sigma  = source_transformer_sink<unary_only, carrier>::type;
    using sigma2 = source_transformer_sink_t<measure, carrier>;

    static_assert(std::is_same<sigma, std::string>::value,
                  "sigma is deduced from g(tau)");
    static_assert(std::is_same<sigma2, std::size_t>::value,
                  "and it need not be the carrier");

    return ( std::is_same<sigma, std::string>::value &&
             std::is_same<sigma2, std::size_t>::value );
}

/*
tests_source_transformer_sink_is_nonesuch_when_not_callable
  Verifies the fallback for the unary probe.
  Tests the following:
  - a type with no operator() has a source-transformer sink of nonesuch
  - so does a binary callable, which cannot be applied to a single source
*/
bool
tests_source_transformer_sink_is_nonesuch_when_not_callable()
{
    using none  = source_transformer_sink_t<not_callable, carrier>;
    using none2 = source_transformer_sink_t<concat, carrier>;

    static_assert(std::is_same<none, nonesuch>::value,
                  "no operator() -> nonesuch");
    static_assert(std::is_same<none2, nonesuch>::value,
                  "a binary F is not applicable to one source");

    return ( std::is_same<none, nonesuch>::value &&
             std::is_same<none2, nonesuch>::value );
}

/*
tests_source_transformer_sink_t_alias
  Verifies the convenience alias tracks the trait.
  Tests the following:
  - source_transformer_sink_t is source_transformer_sink<...>::type
*/
bool
tests_source_transformer_sink_t_alias()
{
    static_assert(std::is_same<source_transformer_sink_t<unary_only, carrier>,
                               source_transformer_sink<unary_only, carrier>::type>::value,
                  "the alias tracks the trait");

    return true;
}

/*
tests_is_source_transformer_accepts_unary
  Verifies a correctly-shaped F_t is recognized.
  Tests the following:
  - a callable taking one source is a source-transformer
  - one whose sink differs from the carrier is too
*/
bool
tests_is_source_transformer_accepts_unary()
{
    static_assert(is_source_transformer<unary_only, carrier>::value,
                  "a unary callable is an F_t");
    static_assert(is_source_transformer<measure, carrier>::value,
                  "sigma is free here as well");

    return ( is_source_transformer<unary_only, carrier>::value &&
             is_source_transformer<measure, carrier>::value );
}

/*
tests_is_source_transformer_rejects_binary
  Verifies arity is enforced in the other direction.
  Tests the following:
  - a transformation is not itself a source-transformer over the carrier
*/
bool
tests_is_source_transformer_rejects_binary()
{
    static_assert(!is_source_transformer<concat, carrier>::value,
                  "an F takes a template AND a source");
    static_assert(!is_source_transformer<not_callable, carrier>::value,
                  "and the uncallable is neither");

    return ( !is_source_transformer<concat, carrier>::value &&
             !is_source_transformer<not_callable, carrier>::value );
}

/*
tests_is_source_transformer_rejects_non_const_callable
  Verifies const-callability is enforced for F_t as well - the same soundness
  case.  The kleisli helpers store their parsers by value and invoke them from
  const member functions, and the canonical F_t (transformer_helper) is itself
  const-callable, so a source-transformer must be const-callable too.
  Tests the following:
  - mutable_unary, correctly shaped but non-const-callable, is NOT a
    source-transformer
  - the const-callable counterpart still is
*/
bool
tests_is_source_transformer_rejects_non_const_callable()
{
    static_assert(!is_source_transformer<mutable_unary, carrier>::value,
                  "an F_t is stored and invoked from a const member");
    static_assert(is_source_transformer<unary_only, carrier>::value,
                  "and the const-callable counterpart is unaffected");

    return ( !is_source_transformer<mutable_unary, carrier>::value &&
              is_source_transformer<unary_only, carrier>::value );
}

/*
tests_is_source_transformer_v_agrees_with_trait
  Verifies the variable-template shorthand tracks the trait.
  Tests the following:
  - is_source_transformer_v equals the trait's value, positive and negative
*/
bool
tests_is_source_transformer_v_agrees_with_trait()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_source_transformer_v<unary_only, carrier> ==
                  is_source_transformer<unary_only, carrier>::value,
                  "_v tracks the trait (positive)");
    static_assert(is_source_transformer_v<concat, carrier> ==
                  is_source_transformer<concat, carrier>::value,
                  "_v tracks the trait (negative)");

    return ( is_source_transformer_v<unary_only, carrier> &&
            !is_source_transformer_v<concat, carrier> );
#else
    return true;
#endif
}

/*
tests_traits_accept_lambdas_and_function_pointers
  Verifies the traits work for the callables an actual caller reaches for, not
  just for hand-written functors.
  Tests the following:
  - a lambda of transformation shape qualifies, and its sink is deduced
  - a plain function pointer qualifies, and its sink is deduced
*/
bool
tests_traits_accept_lambdas_and_function_pointers()
{
    auto lambda = [](const carrier& _bound_template, const carrier& _source)
    {
        return _bound_template + "|" + _source;
    };

    using lambda_type = decltype(lambda);

    static_assert(is_transformation<lambda_type, carrier>::value,
                  "a lambda is a transformation");
    static_assert(std::is_same<transformation_sink_t<lambda_type, carrier>,
                               std::string>::value,
                  "and its sink deduces");

    static_assert(is_transformation<transformation_ptr, carrier>::value,
                  "a function pointer is a transformation");
    static_assert(std::is_same<transformation_sink_t<transformation_ptr, carrier>,
                               std::string>::value,
                  "and its sink deduces");

    // and they really are usable, not merely accepted by the trait
    const carrier t = "T";
    const carrier a = "A";

    transformation_ptr fp = &free_transformation;

    bool ok = true;

    ok = ok && (evaluate(instantiate(lambda, t), a) == "T|A");
    ok = ok && (evaluate(instantiate(fp, t), a) == "T-A");

    return ok;
}


NS_END  // testing
NS_END  // djinterp
