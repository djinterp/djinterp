#include "contravariant_tests.hpp"


NS_DJINTERP
NS_TESTING

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

/*
tests_contravariant_concept_accepts_instances
  The Contravariant concept is satisfied by every registered instance.
  Tests the following:
  - the serializer, predicate and literal-type predicate satisfy it
  - the hook-registered family satisfies it
  - a nested context satisfies it
  - cv-qualified and reference spellings satisfy it, since the concept keys on
    the same is_contravariant that decays its argument
*/
bool
tests_contravariant_concept_accepts_instances()
{
    // the named instances
    D_CV_CHECK(Contravariant< to_string_of<long> >);
    D_CV_CHECK(Contravariant< to_string_of<std::string> >);
    D_CV_CHECK(Contravariant< predicate_of<int> >);
    D_CV_CHECK((Contravariant< ct_predicate<long, ct_is_big> >));

    // the hook-registered family
    D_CV_CHECK(Contravariant< sink_of<int> >);
    D_CV_CHECK(Contravariant< sink_of<std::string> >);

    // a nested context
    D_CV_CHECK(Contravariant< to_string_of< predicate_of<int> > >);

    // and a marker-carrying trait-shape registration
    D_CV_CHECK(Contravariant<marker_only>);

    // cv-ref spellings, via the decaying detector
    D_CV_CHECK(Contravariant< const to_string_of<long> >);
    D_CV_CHECK(Contravariant< to_string_of<long>& >);
    D_CV_CHECK(Contravariant< const sink_of<int>& >);

    return true;
}

/*
tests_contravariant_concept_rejects_non_instances
  The Contravariant concept is not satisfied by non-instances.
  Tests the following:
  - fundamentals and void do not satisfy it
  - a plain class and an incomplete class do not satisfy it
  - a value_type-without-marker registration does not satisfy it
  - a pointer to an instance does not satisfy it
*/
bool
tests_contravariant_concept_rejects_non_instances()
{
    D_CV_CHECK(!Contravariant<int>);
    D_CV_CHECK(!Contravariant<double>);
    D_CV_CHECK(!Contravariant<void>);
    D_CV_CHECK(!Contravariant<std::string>);

    D_CV_CHECK(!Contravariant<plain_probe>);
    D_CV_CHECK(!Contravariant<never_defined>);

    // registered value_type, but no marker
    D_CV_CHECK(!Contravariant<value_only>);

    // a hostile marker
    D_CV_CHECK(!Contravariant<hostile_marked>);

    // a pointer to an instance is not an instance
    D_CV_CHECK(!Contravariant< to_string_of<long>* >);

    return true;
}

/*
tests_contravariant_concept_agrees_with_trait
  The concept and the trait are the same predicate.
  Tests the following:
  - Contravariant<T> equals is_contravariant<T>::value across the battery
  - the agreement holds for the hook family
  - the agreement holds through cv-ref spellings
  - the agreement holds on the marker edge cases
*/
bool
tests_contravariant_concept_agrees_with_trait()
{
    D_CV_CHECK(Contravariant< to_string_of<long> > ==
               is_contravariant< to_string_of<long> >::value);
    D_CV_CHECK(Contravariant< predicate_of<int> > ==
               is_contravariant< predicate_of<int> >::value);
    D_CV_CHECK(Contravariant< sink_of<double> > ==
               is_contravariant< sink_of<double> >::value);

    D_CV_CHECK(Contravariant<int> == is_contravariant<int>::value);
    D_CV_CHECK(Contravariant<void> == is_contravariant<void>::value);
    D_CV_CHECK(Contravariant<plain_probe> == is_contravariant<plain_probe>::value);

    // the marker edge cases
    D_CV_CHECK(Contravariant<marker_only> == is_contravariant<marker_only>::value);
    D_CV_CHECK(Contravariant<value_only> == is_contravariant<value_only>::value);
    D_CV_CHECK(Contravariant<false_marked> == is_contravariant<false_marked>::value);
    D_CV_CHECK(Contravariant<hostile_marked> ==
               is_contravariant<hostile_marked>::value);

    // through a qualified spelling
    D_CV_CHECK(Contravariant< const to_string_of<long>& > ==
               is_contravariant< const to_string_of<long>& >::value);

    return true;
}

/*
tests_contravariant_concept_constrains_overload_resolution
  The concept works as a constraint, not merely as a bool.
  Tests the following:
  - a Contravariant-constrained overload is chosen for an instance over an
    unconstrained one
  - the constrained overload is chosen for the hook family too
  - the unconstrained overload is chosen for a non-instance
  - a nested context still selects the constrained overload
*/
bool
tests_contravariant_concept_constrains_overload_resolution()
{
    to_string_of<long>        show_long = make_show_long();
    predicate_of<int>         is_pos{ [](const int& _value) { return _value > 0; } };
    std::vector<std::string>  log;
    sink_of<int>              raw{
        &log,
        [](const int& _value) { return long_to_text(_value); } };

    // instances pick the constrained overload
    D_CV_CHECK(std::string(overload_taken(show_long)) == "contravariant");
    D_CV_CHECK(std::string(overload_taken(is_pos)) == "contravariant");
    D_CV_CHECK(std::string(overload_taken(raw)) == "contravariant");

    // non-instances fall through to the generic one
    D_CV_CHECK(std::string(overload_taken(42)) == "generic");
    D_CV_CHECK(std::string(overload_taken(std::string("x"))) == "generic");

    plain_probe probe;

    D_CV_CHECK(std::string(overload_taken(probe)) == "generic");

    // a nested context still resolves to the constrained overload
    to_string_of< predicate_of<int> > nested{ nullptr };

    D_CV_CHECK(std::string(overload_taken(nested)) == "contravariant");

    return true;
}

/*
tests_contravariant_concept_composes_in_requires_clause
  Contravariant composes into a larger, short-circuiting concept.
  Tests the following:
  - contramappable_with is satisfied when the type is an instance and the
    adapter fits
  - it is NOT satisfied when the adapter has the wrong arity, even for an
    instance
  - it is NOT satisfied for a non-instance, and the conjunction short-circuits
    so the contramap expression is never formed on it
  - the composed concept still agrees with a hand-written conjunction
*/
bool
tests_contravariant_concept_composes_in_requires_clause()
{
    using unary_type  = long (*)(const std::string&);
    using binary_type = long (*)(const std::string&, int);

    // instance + fitting adapter
    D_CV_CHECK((contramappable_with<unary_type, to_string_of<long> >));
    D_CV_CHECK((contramappable_with<unary_type, predicate_of<long> >));
    D_CV_CHECK((contramappable_with<unary_type, sink_of<long> >));

    // instance + ill-fitting adapter -> the requires-clause fails
    D_CV_CHECK((!contramappable_with<binary_type, to_string_of<long> >));

    // non-instance -> rejected by the FIRST operand; the contramap expression
    // in the second operand is never even formed (which is why this compiles)
    D_CV_CHECK((!contramappable_with<unary_type, int>));
    D_CV_CHECK((!contramappable_with<unary_type, plain_probe>));
    D_CV_CHECK((!contramappable_with<unary_type, void>));

    // the composed concept equals the conjunction it stands for
    D_CV_CHECK((contramappable_with<unary_type, to_string_of<long> >) ==
               (Contravariant< to_string_of<long> > &&
                can_contramap<unary_type, to_string_of<long> >::value));
    D_CV_CHECK((contramappable_with<binary_type, to_string_of<long> >) ==
               (Contravariant< to_string_of<long> > &&
                can_contramap<binary_type, to_string_of<long> >::value));

    return true;
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_END  // testing
NS_END  // djinterp
