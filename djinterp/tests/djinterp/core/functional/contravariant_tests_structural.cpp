#include "contravariant_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_contravariant_value_type_of_instances
  contravariant_value_type reports the inner consumed type.
  Tests the following:
  - the reported type matches the instance's declared value_type
  - it tracks the element type across several instantiations of one template
  - it agrees with reading contravariant_traits<F>::value_type directly
  - it works for every registration style in the zoo
*/
bool
tests_contravariant_value_type_of_instances()
{
    // the serializer
    D_CV_CHECK((std::is_same<contravariant_value_type< to_string_of<long> >::type,
                             long>::value));
    D_CV_CHECK((std::is_same<contravariant_value_type< to_string_of<std::string> >::type,
                             std::string>::value));

    // the predicate and the literal-type predicate
    D_CV_CHECK((std::is_same<contravariant_value_type< predicate_of<int> >::type,
                             int>::value));
    D_CV_CHECK((std::is_same<
                    contravariant_value_type< ct_predicate<long, ct_is_big> >::type,
                    long>::value));

    // the hook-registered sink
    D_CV_CHECK((std::is_same<contravariant_value_type< sink_of<double> >::type,
                             double>::value));

    // and it is exactly what the traits declared
    D_CV_CHECK((std::is_same<contravariant_value_type< to_string_of<long> >::type,
                             contravariant_traits< to_string_of<long> >::value_type
                >::value));

    return true;
}

/*
tests_contravariant_value_type_alias_agreement
  The _t alias is a faithful shorthand.
  Tests the following:
  - contravariant_value_type_t<F> names the same type as the ::type member
  - the agreement holds for every instance in the zoo
  - it holds through a cv-ref spelling of the context as well
*/
bool
tests_contravariant_value_type_alias_agreement()
{
    D_CV_CHECK((std::is_same<contravariant_value_type_t< to_string_of<long> >,
                             contravariant_value_type< to_string_of<long> >::type
                >::value));
    D_CV_CHECK((std::is_same<contravariant_value_type_t< predicate_of<int> >,
                             contravariant_value_type< predicate_of<int> >::type
                >::value));
    D_CV_CHECK((std::is_same<contravariant_value_type_t< sink_of<double> >,
                             contravariant_value_type< sink_of<double> >::type
                >::value));
    D_CV_CHECK((std::is_same<contravariant_value_type_t<ref_consumer>,
                             contravariant_value_type<ref_consumer>::type
                >::value));

    // through a qualified context spelling
    D_CV_CHECK((std::is_same<contravariant_value_type_t< const to_string_of<long>& >,
                             contravariant_value_type< to_string_of<long> >::type
                >::value));

    // and the shorthand resolves to the expected type, not merely to itself
    D_CV_CHECK((std::is_same<contravariant_value_type_t< to_string_of<long> >,
                             long>::value));

    return true;
}

/*
tests_contravariant_value_type_decays_context
  The CONTEXT argument is decayed before the lookup.
  Tests the following:
  - const, volatile and const-volatile contexts report the same inner type
  - lvalue-, rvalue- and const-lvalue-reference contexts do too
  - the decay applies equally to the hook-registered family
  - decaying the context does not decay the reported inner type
*/
bool
tests_contravariant_value_type_decays_context()
{
    using show_type = to_string_of<long>;

    // cv-qualified contexts
    D_CV_CHECK((std::is_same<contravariant_value_type<const show_type>::type,
                             long>::value));
    D_CV_CHECK((std::is_same<contravariant_value_type<volatile show_type>::type,
                             long>::value));
    D_CV_CHECK((std::is_same<contravariant_value_type<const volatile show_type>::type,
                             long>::value));

    // reference contexts
    D_CV_CHECK((std::is_same<contravariant_value_type<show_type&>::type,
                             long>::value));
    D_CV_CHECK((std::is_same<contravariant_value_type<show_type&&>::type,
                             long>::value));
    D_CV_CHECK((std::is_same<contravariant_value_type<const show_type&>::type,
                             long>::value));

    // the hook-registered family decays identically
    D_CV_CHECK((std::is_same<contravariant_value_type< const sink_of<double>& >::type,
                             double>::value));

    // decaying the context leaves a qualified inner type alone
    D_CV_CHECK((std::is_same<contravariant_value_type<const ref_consumer&>::type,
                             const std::string&>::value));

    return true;
}

/*
tests_contravariant_value_type_preserves_qualifiers
  The reported inner type is verbatim what the instance declared.
  Tests the following:
  - a reference-to-const value_type is reported reference-to-const
  - it is NOT decayed to the bare object type
  - the _t alias reports the same qualified type
  - an unqualified instance is unaffected, so the difference is the
    declaration's and not the trait's
*/
bool
tests_contravariant_value_type_preserves_qualifiers()
{
    using reported_type = contravariant_value_type<ref_consumer>::type;

    // reported verbatim
    D_CV_CHECK((std::is_same<reported_type, const std::string&>::value));

    // and specifically not decayed
    D_CV_CHECK(!(std::is_same<reported_type, std::string>::value));
    D_CV_CHECK(std::is_reference<reported_type>::value);
    D_CV_CHECK(std::is_const<std::remove_reference<reported_type>::type>::value);

    // the alias agrees
    D_CV_CHECK((std::is_same<contravariant_value_type_t<ref_consumer>,
                             const std::string&>::value));

    // an unqualified declaration stays unqualified
    D_CV_CHECK(!std::is_reference<
                   contravariant_value_type_t< to_string_of<long> > >::value);

    return true;
}

/*
tests_contravariant_value_type_helper_soft_failure
  The extractor's soft-failure seam is the internal helper.
  Tests the following:
  - the helper resolves a `type` for every registered instance
  - it resolves none for fundamentals, void, std::string, a plain class, an
    incomplete class, or a pointer to an instance
  - the failure is a substitution failure (the probe compiles) rather than a
    hard error
*/
bool
tests_contravariant_value_type_helper_soft_failure()
{
    // resolves for the registered instances
    D_CV_CHECK(has_inner_value_type< to_string_of<long> >::value);
    D_CV_CHECK(has_inner_value_type< predicate_of<int> >::value);
    D_CV_CHECK(has_inner_value_type< sink_of<double> >::value);
    D_CV_CHECK(has_inner_value_type<ref_consumer>::value);
    D_CV_CHECK(has_inner_value_type<value_only>::value);

    // resolves nothing for everything unregistered -- softly
    D_CV_CHECK(!has_inner_value_type<int>::value);
    D_CV_CHECK(!has_inner_value_type<void>::value);
    D_CV_CHECK(!has_inner_value_type<std::string>::value);
    D_CV_CHECK(!has_inner_value_type<plain_probe>::value);
    D_CV_CHECK(!has_inner_value_type<never_defined>::value);
    D_CV_CHECK(!has_inner_value_type< to_string_of<long>* >::value);

    // the probe itself is a well-formed bool trait, which is the evidence the
    // failure was soft
    D_CV_CHECK(std::is_base_of<std::false_type,
                               has_inner_value_type<plain_probe> >::value);
    D_CV_CHECK(std::is_base_of<std::true_type,
                               has_inner_value_type< to_string_of<long> > >::value);

    return true;
}

/*
tests_contravariant_value_type_orthogonal_to_marker
  Detection and extraction ask independent questions.
  Tests the following:
  - a marker-without-value_type registration is detected but yields no inner
    type
  - a value_type-without-marker registration yields an inner type but is not
    detected
  - a complete registration answers yes to both
  - an unregistered type answers no to both
*/
bool
tests_contravariant_value_type_orthogonal_to_marker()
{
    // marker present, value_type absent
    D_CV_CHECK(is_contravariant<marker_only>::value);
    D_CV_CHECK(!has_inner_value_type<marker_only>::value);

    // value_type present, marker absent
    D_CV_CHECK(!is_contravariant<value_only>::value);
    D_CV_CHECK(has_inner_value_type<value_only>::value);
    D_CV_CHECK((std::is_same<contravariant_value_type<value_only>::type,
                             double>::value));

    // both present
    D_CV_CHECK(is_contravariant< to_string_of<long> >::value);
    D_CV_CHECK(has_inner_value_type< to_string_of<long> >::value);

    // neither present
    D_CV_CHECK(!is_contravariant<plain_probe>::value);
    D_CV_CHECK(!has_inner_value_type<plain_probe>::value);

    return true;
}

/*
tests_contravariant_value_type_nested_and_distinct
  The inner type tracks the instantiation exactly.
  Tests the following:
  - two instantiations of one template report different inner types
  - two different templates over one element type report the same inner type
  - a context nested over another context reports the inner CONTEXT, which is
    itself contravariant
  - the nesting can be read back one level at a time
*/
bool
tests_contravariant_value_type_nested_and_distinct()
{
    // distinct instantiations, distinct inner types
    D_CV_CHECK(!(std::is_same<contravariant_value_type_t< to_string_of<long> >,
                              contravariant_value_type_t< to_string_of<int> >
                 >::value));

    // distinct templates, one element type
    D_CV_CHECK((std::is_same<contravariant_value_type_t< to_string_of<int> >,
                             contravariant_value_type_t< predicate_of<int> >
                >::value));

    // a nested context reports the inner context itself
    using nested_type = to_string_of< predicate_of<int> >;

    D_CV_CHECK((std::is_same<contravariant_value_type_t<nested_type>,
                             predicate_of<int> >::value));

    // which is in turn a context, readable one more level down
    D_CV_CHECK(is_contravariant< contravariant_value_type_t<nested_type> >::value);
    D_CV_CHECK((std::is_same<
                    contravariant_value_type_t<
                        contravariant_value_type_t<nested_type> >,
                    int>::value));

    return true;
}

/*
tests_contravariant_value_type_of_hook_instance
  Hook-registered instances use the same extractor as named ones.
  Tests the following:
  - the inner type of a sink comes back through contravariant_value_type
  - it tracks the family's element type
  - the enable_if hook's own computation (element_type) is what is reported
  - a nested sink over a context reports that context
*/
bool
tests_contravariant_value_type_of_hook_instance()
{
    // the family, at several element types
    D_CV_CHECK((std::is_same<contravariant_value_type_t< sink_of<int> >,
                             int>::value));
    D_CV_CHECK((std::is_same<contravariant_value_type_t< sink_of<std::string> >,
                             std::string>::value));

    // what the hook computed is what is reported
    D_CV_CHECK((std::is_same<contravariant_value_type_t< sink_of<int> >,
                             sink_of<int>::element_type>::value));

    // through a qualified spelling of the context
    D_CV_CHECK((std::is_same<contravariant_value_type_t< const sink_of<int>& >,
                             int>::value));

    // and over a nested context
    D_CV_CHECK((std::is_same<contravariant_value_type_t< sink_of< predicate_of<char> > >,
                             predicate_of<char> >::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
