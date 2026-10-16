/*******************************************************************************
* djinterp [core]                                     template_tests_algebra.cpp
*
*
* path:      /tests/djinterp/core/paradigm/template/template_tests_algebra.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : template_tests_algebra.cpp
//   Section II: the algebra.  Everything here orbits one identity,
//
//       F  =  ev . (F_hat x id) ,        F(t, alpha) = ev(F_hat(t), alpha) ,
//
// which is the whole reason the module exists.  `instantiate` is F_hat at a
// point, `evaluate` is ev, and `template_system` packages F so that the identity
// holds by construction.  tests_factorization_identity and
// tests_template_system_identity are the two tests that actually assert it; the
// rest establish that each piece behaves well enough for the identity to mean
// something.
//
//   The section also pins the properties that are easy to lose silently: that
// evaluate is exactly as noexcept as the call it forwards (D_NOEXCEPT_IF), that
// the factories decay what they store (clean_t), that the system's ctor is
// explicit, that sigma is deduced rather than assumed to be tau, and that the
// entire algebra folds in a constant expression.

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "template_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_evaluate_applies_transformer
  Verifies ev applies a source-transformer to a source and returns the sink.
  Tests the following:
  - evaluate(g, alpha) is g(alpha)
  - it works for a transformer whose sink is not the carrier
*/
bool
tests_evaluate_applies_transformer()
{
    exclaim g;
    measure m;

    const carrier a = "abc";

    bool ok = true;

    ok = ok && (evaluate(g, a) == "abc!");
    ok = ok && (evaluate(m, a) == 3u);

    return ok;
}

/*
tests_evaluate_accepts_rvalue_transformer
  Verifies ev perfect-forwards its transformer rather than requiring an lvalue.
  Tests the following:
  - a temporary transformer is accepted
  - so is the temporary that instantiate returns, which is the common spelling
*/
bool
tests_evaluate_accepts_rvalue_transformer()
{
    const carrier t = "greet";
    const carrier a = "world";

    bool ok = true;

    ok = ok && (evaluate(exclaim(), a) == "world!");
    ok = ok && (evaluate(instantiate(concat(), t), a) == "greet:world");

    return ok;
}

/*
tests_evaluate_propagates_noexcept
  Verifies evaluate's D_NOEXCEPT_IF: it is exactly as noexcept as the call it
  forwards, neither swallowing an exception specification nor inventing one.
  Tests the following:
  - evaluating a noexcept transformer is noexcept
  - evaluating a potentially-throwing transformer is not
*/
bool
tests_evaluate_propagates_noexcept()
{
    noexcept_unary safe;
    throwing_unary risky;

    const int a = 1;

    static_assert(noexcept(evaluate(safe, a)),
                  "a noexcept transformer makes evaluate noexcept");
    static_assert(!noexcept(evaluate(risky, a)),
                  "a throwing transformer does not");

    return ( noexcept(evaluate(safe, a)) && !noexcept(evaluate(risky, a)) );
}

/*
tests_instantiate_binds_the_template
  Verifies F_hat at a point: instantiate binds a template to a transformation and
  yields the source-transformer F_t : tau -> sigma.
  Tests the following:
  - the returned F_t applies F with the bound template
  - a second F_t over a different template is independent of the first
*/
bool
tests_instantiate_binds_the_template()
{
    concat F;

    auto greet_t = instantiate(F, carrier("greet"));
    auto shout_t = instantiate(F, carrier("shout"));

    bool ok = true;

    ok = ok && (greet_t(carrier("world")) == "greet:world");
    ok = ok && (shout_t(carrier("world")) == "shout:world");

    // the transformer really is an F_t: the module's own trait says so
    static_assert(is_source_transformer<decltype(greet_t), carrier>::value,
                  "instantiate yields a source-transformer");

    return ok;
}

/*
tests_instantiate_decays_its_arguments
  Verifies the transformation and the template are stored decayed (clean_t), so
  that F_t owns them rather than referring to expiring arguments.
  Tests the following:
  - passing lvalues and passing rvalues produce the same transformer type
  - an F_t built from temporaries still works after those temporaries are gone
*/
bool
tests_instantiate_decays_its_arguments()
{
    concat        F;
    const carrier t = "greet";

    auto from_lvalues  = instantiate(F, t);
    auto from_rvalues  = instantiate(concat(), carrier("greet"));

    static_assert(std::is_same<decltype(from_lvalues),
                               decltype(from_rvalues)>::value,
                  "lvalue and rvalue arguments decay to the same transformer");
    static_assert(std::is_same<decltype(from_lvalues),
                               internal::transformer_helper<concat, carrier>>::value,
                  "and that type is transformer_helper<clean_t<F>, clean_t<tau>>");

    // the temporaries are long gone; the transformer still holds its own copies
    bool ok = true;

    ok = ok && (from_rvalues(carrier("world")) == "greet:world");
    ok = ok && (from_lvalues(carrier("world")) == "greet:world");

    return ok;
}

/*
tests_transformer_typedefs
  Verifies the transformer exposes the carrier and the sink it was deduced over.
  Tests the following:
  - carrier_type is tau
  - sink_type is the deduced sigma, including when sigma is not tau
*/
bool
tests_transformer_typedefs()
{
    using same_sink = internal::transformer_helper<concat, carrier>;
    using diff_sink = internal::transformer_helper<total_len, carrier>;

    static_assert(std::is_same<same_sink::carrier_type, carrier>::value,
                  "carrier_type is tau");
    static_assert(std::is_same<same_sink::sink_type, std::string>::value,
                  "sink_type is the deduced sigma");
    static_assert(std::is_same<diff_sink::sink_type, std::size_t>::value,
                  "and it follows F, not tau");

    return true;
}

/*
tests_factorization_identity
  Verifies THE identity the module exists for:

      F(t, alpha)  ==  evaluate(instantiate(F, t), alpha)

  i.e. F = ev . (F_hat x id).  Currying a transformation through a template and
  then evaluating the result at a source must agree with applying F directly.
  Tests the following:
  - the identity holds for a transformation whose sink is the carrier
  - it holds for one whose sink is NOT the carrier
  - it holds across several (template, source) pairs, including empty ones
*/
bool
tests_factorization_identity()
{
    concat    F;
    total_len G;

    const carrier pairs[][2] = {
        { carrier("greet"), carrier("world") },
        { carrier("a"),     carrier("b")     },
        { carrier(""),      carrier("x")     },
        { carrier("x"),     carrier("")      },
        { carrier(""),      carrier("")      }
    };

    bool ok = true;

    for (const carrier* pair : pairs)
    {
        const carrier& t = pair[0];
        const carrier& a = pair[1];

        // sigma == tau
        ok = ok && (evaluate(instantiate(F, t), a) == F(t, a));

        // sigma != tau
        ok = ok && (evaluate(instantiate(G, t), a) == G(t, a));
    }

    return ok;
}

/*
tests_template_system_apply
  Verifies apply is the uncurried F.
  Tests the following:
  - apply(t, alpha) equals F(t, alpha)
*/
bool
tests_template_system_apply()
{
    concat F;

    template_system<concat, carrier> sys(F);

    const carrier t = "greet";
    const carrier a = "world";

    bool ok = true;

    ok = ok && (sys.apply(t, a) == "greet:world");
    ok = ok && (sys.apply(t, a) == F(t, a));

    return ok;
}

/*
tests_template_system_operator_call
  Verifies operator() is an alias for apply.
  Tests the following:
  - sys(t, alpha) and sys.apply(t, alpha) agree
*/
bool
tests_template_system_operator_call()
{
    template_system<concat, carrier> sys{ concat() };

    const carrier t = "greet";
    const carrier a = "world";

    return (sys(t, a) == sys.apply(t, a)) && (sys(t, a) == "greet:world");
}

/*
tests_template_system_instantiate
  Verifies the system's instantiate is F_hat: it decodes a template into its
  transformer.
  Tests the following:
  - the returned transformer applies F with the bound template
  - its type is the system's advertised transformer_type
*/
bool
tests_template_system_instantiate()
{
    template_system<concat, carrier> sys{ concat() };

    auto greet_t = sys.instantiate(carrier("greet"));

    static_assert(std::is_same<decltype(greet_t),
                               template_system<concat, carrier>::transformer_type>::value,
                  "instantiate returns the advertised transformer_type");

    return (greet_t(carrier("world")) == "greet:world");
}

/*
tests_template_system_identity
  Verifies the factorization holds inside the packaged system:

      sys.apply(t, alpha)  ==  evaluate(sys.instantiate(t), alpha)

  This is the same identity as tests_factorization_identity, but asserted against
  the class that claims to make it true BY CONSTRUCTION.
  Tests the following:
  - the two agree across several (template, source) pairs
  - and for a system whose sink is not the carrier
*/
bool
tests_template_system_identity()
{
    template_system<concat, carrier>    sys{ concat() };
    template_system<total_len, carrier> lensys{ total_len() };

    const carrier pairs[][2] = {
        { carrier("greet"), carrier("world") },
        { carrier("x"),     carrier("y")     },
        { carrier(""),      carrier("")      }
    };

    bool ok = true;

    for (const carrier* pair : pairs)
    {
        const carrier& t = pair[0];
        const carrier& a = pair[1];

        ok = ok && (sys.apply(t, a) == evaluate(sys.instantiate(t), a));
        ok = ok && (lensys.apply(t, a) == evaluate(lensys.instantiate(t), a));
    }

    return ok;
}

/*
tests_template_system_transformation_accessor
  Verifies the underlying F remains reachable.
  Tests the following:
  - transformation() yields a const reference to the stored F
  - calling it directly agrees with apply
*/
bool
tests_template_system_transformation_accessor()
{
    template_system<concat, carrier> sys{ concat() };

    static_assert(std::is_same<decltype(sys.transformation()),
                               const concat&>::value,
                  "transformation() yields a const reference to F");

    const carrier t = "greet";
    const carrier a = "world";

    return (sys.transformation()(t, a) == sys.apply(t, a));
}

/*
tests_template_system_typedefs
  Verifies the system advertises its own components.
  Tests the following:
  - carrier_type, sink_type, transformation_type and transformer_type are all
    what the system was built over
*/
bool
tests_template_system_typedefs()
{
    using sys_t = template_system<concat, carrier>;

    static_assert(std::is_same<sys_t::carrier_type, carrier>::value,
                  "carrier_type is tau");
    static_assert(std::is_same<sys_t::sink_type, std::string>::value,
                  "sink_type is the deduced sigma");
    static_assert(std::is_same<sys_t::transformation_type, concat>::value,
                  "transformation_type is F");
    static_assert(std::is_same<sys_t::transformer_type,
                               internal::transformer_helper<concat, carrier>>::value,
                  "transformer_type is the F_t instantiate returns");

    return true;
}

/*
tests_template_system_ctor_is_explicit
  Verifies a transformation does not silently become a system.
  Tests the following:
  - the constructor is explicit, so F is not convertible to template_system
  - but it is constructible from one
*/
bool
tests_template_system_ctor_is_explicit()
{
    using sys_t = template_system<concat, carrier>;

    static_assert(!std::is_convertible<concat, sys_t>::value,
                  "the ctor is explicit");
    static_assert(std::is_constructible<sys_t, const concat&>::value,
                  "but it does construct explicitly");

    return ( !std::is_convertible<concat, sys_t>::value &&
              std::is_constructible<sys_t, const concat&>::value );
}

/*
tests_template_system_sink_differs_from_carrier
  Verifies sigma is deduced from F rather than assumed to be tau.
  Tests the following:
  - a system over total_len has a size_t sink and returns sizes
*/
bool
tests_template_system_sink_differs_from_carrier()
{
    template_system<total_len, carrier> sys{ total_len() };

    static_assert(std::is_same<decltype(sys)::sink_type, std::size_t>::value,
                  "sigma follows F");
    static_assert(!std::is_same<decltype(sys)::sink_type,
                                decltype(sys)::carrier_type>::value,
                  "and need not be tau");

    return (sys.apply(carrier("greet"), carrier("world")) == 10u);
}

/*
tests_template_system_is_itself_a_transformation
  Verifies the algebra is closed: a packaged system exposes operator()(t, alpha),
  so it satisfies the module's own transformation trait and could serve as the F
  of another system.
  Tests the following:
  - template_system<concat, tau> is a transformation over tau
  - its sink deduces to the same sigma
*/
bool
tests_template_system_is_itself_a_transformation()
{
    using sys_t = template_system<concat, carrier>;

    static_assert(is_transformation<sys_t, carrier>::value,
                  "a system is itself a transformation");
    static_assert(std::is_same<transformation_sink_t<sys_t, carrier>,
                               std::string>::value,
                  "with the same sink");

    // and it really does compose: a system over a system
    sys_t inner{ concat() };

    auto outer = make_template_system<carrier>(inner);

    return (outer.apply(carrier("greet"), carrier("world")) ==
            inner.apply(carrier("greet"), carrier("world")));
}

/*
tests_make_template_system
  Verifies the factory: the carrier is explicit (it is not deducible from a
  generic binary F) and the sink is deduced.
  Tests the following:
  - the returned system is template_system<clean_t<F>, tau>
  - an rvalue transformation is decayed into it
  - the system behaves as one built directly
*/
bool
tests_make_template_system()
{
    auto sys = make_template_system<carrier>(concat());

    static_assert(std::is_same<decltype(sys),
                               template_system<concat, carrier>>::value,
                  "F is decayed, tau is explicit, sigma is deduced");

    concat F;
    auto from_lvalue = make_template_system<carrier>(F);

    static_assert(std::is_same<decltype(from_lvalue), decltype(sys)>::value,
                  "an lvalue F decays to the same system type");

    const carrier t = "greet";
    const carrier a = "world";

    return (sys.apply(t, a) == "greet:world") &&
           (from_lvalue.apply(t, a) == sys.apply(t, a));
}

/*
tests_algebra_is_constexpr
  Verifies every public entry point in the algebra folds in a constant
  expression - each one carries D_CONSTEXPR, and that claim is only worth
  anything if it is actually exercised.  The carrier here is int, since
  std::string is not a literal type before C++20.
  Tests the following:
  - instantiate, evaluate and the factorization identity fold at compile time
  - template_system's ctor, apply, operator() and instantiate all fold
  - make_template_system folds
*/
bool
tests_algebra_is_constexpr()
{
    D_CONSTEXPR auto transformer = instantiate(mul(), 6);
    D_CONSTEXPR int  sink        = evaluate(transformer, 7);

    static_assert(sink == 42, "instantiate + evaluate fold");
    static_assert(evaluate(instantiate(mul(), 6), 7) == mul()(6, 7),
                  "the factorization identity holds at compile time");

    D_CONSTEXPR auto sys = make_template_system<int>(mul());

    static_assert(sys.apply(6, 7) == 42, "apply folds");
    static_assert(sys(6, 7) == 42, "operator() folds");
    static_assert(evaluate(sys.instantiate(6), 7) == 42,
                  "the system identity holds at compile time");
    static_assert(sys.transformation()(6, 7) == 42, "transformation() folds");

    return (sink == 42);
}


NS_END  // testing
NS_END  // djinterp
