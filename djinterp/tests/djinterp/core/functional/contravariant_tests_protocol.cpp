#include "contravariant_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_contravariant_traits_registration_members
  Registration surface of a contravariant_traits specialization.
  Tests the following:
  - value_type reports the inner consumed type A of F<A>
  - is_specialized is present and is std::true_type on the canonical instances
  - the same holds for every registration style in the zoo (named partial
    specialization, explicit specialization, and the SFINAE-hook family)
  - the trait's second parameter defaults to void, so the one-argument
    spelling names the same specialization as the two-argument one
*/
bool
tests_contravariant_traits_registration_members()
{
    using show_traits = contravariant_traits< to_string_of<long> >;
    using pred_traits = contravariant_traits< predicate_of<int> >;
    using sink_traits = contravariant_traits< sink_of<double> >;
    using ct_traits   = contravariant_traits< ct_predicate<long, ct_is_big> >;

    // the inner consumed type is what the instance declared
    D_CV_CHECK(std::is_same<show_traits::value_type, long>::value);
    D_CV_CHECK(std::is_same<pred_traits::value_type, int>::value);
    D_CV_CHECK(std::is_same<sink_traits::value_type, double>::value);
    D_CV_CHECK(std::is_same<ct_traits::value_type, long>::value);

    // the marker is present and affirmative on every canonical instance
    D_CV_CHECK(std::is_same<show_traits::is_specialized, std::true_type>::value);
    D_CV_CHECK(std::is_same<pred_traits::is_specialized, std::true_type>::value);
    D_CV_CHECK(std::is_same<sink_traits::is_specialized, std::true_type>::value);
    D_CV_CHECK(std::is_same<ct_traits::is_specialized, std::true_type>::value);

    // the second parameter defaults to void: both spellings are one type
    D_CV_CHECK((std::is_same<contravariant_traits< to_string_of<long> >,
                             contravariant_traits< to_string_of<long>, void > >::value));

    // an element-type change is a different specialization
    D_CV_CHECK(!(std::is_same<contravariant_traits< to_string_of<long> >,
                              contravariant_traits< to_string_of<int> > >::value));

    return true;
}

/*
tests_contravariant_traits_rebind_shape
  The optional rebind<U> member.
  Tests the following:
  - rebind<U> yields F<U> on the instances that supply it
  - rebind<A> is the identity on the instance's own element type
  - distinct U give distinct rebound types
  - rebind is genuinely optional: the SFINAE-hook instance omits it and is
    still a fully registered contravariant functor
*/
bool
tests_contravariant_traits_rebind_shape()
{
    using show_traits = contravariant_traits< to_string_of<long> >;
    using pred_traits = contravariant_traits< predicate_of<long> >;

    // rebind<U> is F<U>
    D_CV_CHECK((std::is_same<show_traits::rebind<std::string>,
                             to_string_of<std::string> >::value));
    D_CV_CHECK((std::is_same<pred_traits::rebind<std::string>,
                             predicate_of<std::string> >::value));

    // rebind at the instance's own element type is the identity
    D_CV_CHECK((std::is_same<show_traits::rebind<long>, to_string_of<long> >::value));

    // distinct targets give distinct types
    D_CV_CHECK(!(std::is_same<show_traits::rebind<int>,
                              show_traits::rebind<long> >::value));

    // present where supplied ...
    D_CV_CHECK((has_rebind<to_string_of<long>, std::string>::value));
    D_CV_CHECK((has_rebind<predicate_of<long>, std::string>::value));

    // ... and legitimately absent on the hook-registered family
    D_CV_CHECK(!(has_rebind<sink_of<long>, std::string>::value));

    // absence of rebind does not weaken the registration
    D_CV_CHECK(is_contravariant< sink_of<long> >::value);

    return true;
}

/*
tests_contravariant_detects_registered_instances
  is_contravariant on the positive side.
  Tests the following:
  - every instance in the zoo is detected
  - detection holds across several element types for one template
  - detection holds for a context nested over another context
  - detection holds for the trait-shape-only registrations that carry a marker
*/
bool
tests_contravariant_detects_registered_instances()
{
    // the serializer, over several element types
    D_CV_CHECK(is_contravariant< to_string_of<long> >::value);
    D_CV_CHECK(is_contravariant< to_string_of<int> >::value);
    D_CV_CHECK(is_contravariant< to_string_of<std::string> >::value);

    // the predicate and the literal-type predicate
    D_CV_CHECK(is_contravariant< predicate_of<long> >::value);
    D_CV_CHECK(is_contravariant< predicate_of<std::string> >::value);
    D_CV_CHECK((is_contravariant< ct_predicate<long, ct_is_big> >::value));

    // the hook-registered sink
    D_CV_CHECK(is_contravariant< sink_of<int> >::value);
    D_CV_CHECK(is_contravariant< sink_of<std::string> >::value);

    // a context whose consumed type is itself a context
    D_CV_CHECK(is_contravariant< to_string_of< predicate_of<int> > >::value);

    // a marker-carrying registration with no contramap is still "registered";
    // the detector answers a structural question, not a behavioural one
    D_CV_CHECK(is_contravariant<ref_consumer>::value);
    D_CV_CHECK(is_contravariant<marker_only>::value);

    return true;
}

/*
tests_contravariant_rejects_unregistered_types
  is_contravariant on the negative side.
  Tests the following:
  - fundamentals, void and std::string are not contravariant
  - a complete class with no registration is not contravariant
  - an INCOMPLETE class is a clean false rather than a hard error, because
    contravariant_traits is undefined for it either way
  - a pointer to an instance is not itself an instance
  - the trait template's own instantiations are not contexts
  - a value_type-only registration (no marker) is not detected
*/
bool
tests_contravariant_rejects_unregistered_types()
{
    // fundamentals and void
    D_CV_CHECK(!is_contravariant<int>::value);
    D_CV_CHECK(!is_contravariant<double>::value);
    D_CV_CHECK(!is_contravariant<void>::value);
    D_CV_CHECK(!is_contravariant<std::nullptr_t>::value);

    // ordinary library and user class types
    D_CV_CHECK(!is_contravariant<std::string>::value);
    D_CV_CHECK(!is_contravariant<plain_probe>::value);

    // an incomplete type answers false without requiring completeness
    D_CV_CHECK(!is_contravariant<never_defined>::value);

    // a pointer to an instance is not an instance
    D_CV_CHECK(!is_contravariant< to_string_of<long>* >::value);
    D_CV_CHECK(!is_contravariant< predicate_of<long>* >::value);

    // the trait itself is a description of a context, not a context
    D_CV_CHECK(!is_contravariant< contravariant_traits< to_string_of<long> > >::value);

    // a registration without the marker is not detected
    D_CV_CHECK(!is_contravariant<value_only>::value);

    return true;
}

/*
tests_contravariant_detection_decays_cv_ref
  Detection is applied to the decayed type.
  Tests the following:
  - const, volatile and const-volatile spellings agree with the bare type
  - lvalue-reference, rvalue-reference and const-lvalue-reference spellings
    agree with the bare type
  - the same holds in the negative direction, so decay is not smuggling a
    positive answer in
*/
bool
tests_contravariant_detection_decays_cv_ref()
{
    using show_type = to_string_of<long>;

    // cv-qualified spellings of a registered instance
    D_CV_CHECK(is_contravariant<const show_type>::value);
    D_CV_CHECK(is_contravariant<volatile show_type>::value);
    D_CV_CHECK(is_contravariant<const volatile show_type>::value);

    // reference spellings of a registered instance
    D_CV_CHECK(is_contravariant<show_type&>::value);
    D_CV_CHECK(is_contravariant<show_type&&>::value);
    D_CV_CHECK(is_contravariant<const show_type&>::value);
    D_CV_CHECK(is_contravariant<const volatile show_type&>::value);

    // and on the hook-registered family, whose enable_if sees the decayed type
    D_CV_CHECK(is_contravariant< const sink_of<int>& >::value);
    D_CV_CHECK(is_contravariant< sink_of<int>&& >::value);

    // the negative direction decays identically
    D_CV_CHECK(!is_contravariant<const plain_probe>::value);
    D_CV_CHECK(!is_contravariant<plain_probe&>::value);
    D_CV_CHECK(!is_contravariant<plain_probe&&>::value);
    D_CV_CHECK(!is_contravariant<const plain_probe&>::value);
    D_CV_CHECK(!is_contravariant<const int&>::value);

    return true;
}

/*
tests_contravariant_detection_decays_arrays_and_functions
  std::decay is applied in full, not merely cv-ref stripped.
  Tests the following:
  - an array of instances decays to a pointer, which is not an instance
  - a reference to such an array behaves the same way
  - a function type decays to a function pointer, which is not an instance
  - the bare instance remains detected, so the loss is decay's doing
*/
bool
tests_contravariant_detection_decays_arrays_and_functions()
{
    using show_type = to_string_of<long>;

    // control: the element type itself is an instance
    D_CV_CHECK(is_contravariant<show_type>::value);

    // arrays decay to pointers
    D_CV_CHECK(!is_contravariant<show_type[3]>::value);
    D_CV_CHECK(!is_contravariant<show_type(&)[3]>::value);
    D_CV_CHECK(!is_contravariant<const show_type[3]>::value);

    // and the decayed pointer really is what is being asked about
    D_CV_CHECK((std::is_same<std::decay<show_type[3]>::type, show_type*>::value));

    // function types decay to function pointers
    D_CV_CHECK(!is_contravariant<void(long)>::value);
    D_CV_CHECK(!is_contravariant<show_type(long)>::value);
    D_CV_CHECK(!is_contravariant<void(&)(long)>::value);

    return true;
}

/*
tests_contravariant_detection_bool_trait_shape
  is_contravariant has the shape of a standard boolean trait.
  Tests the following:
  - it derives from std::true_type / std::false_type as appropriate
  - value_type is bool and type is the corresponding integral_constant
  - value is a constant expression usable in static_assert
  - the implicit conversion and (C++14+) operator() agree with value
*/
bool
tests_contravariant_detection_bool_trait_shape()
{
    using yes_trait = is_contravariant< to_string_of<long> >;
    using no_trait  = is_contravariant<plain_probe>;

    // derives from the right integral_constant
    D_CV_CHECK(std::is_base_of<std::true_type, yes_trait>::value);
    D_CV_CHECK(std::is_base_of<std::false_type, no_trait>::value);

    // the inherited nested types
    D_CV_CHECK(std::is_same<yes_trait::value_type, bool>::value);
    D_CV_CHECK(std::is_same<no_trait::value_type, bool>::value);
    D_CV_CHECK(std::is_same<yes_trait::type, std::true_type>::value);
    D_CV_CHECK(std::is_same<no_trait::type, std::false_type>::value);

    // value is a constant expression
    static_assert(yes_trait::value, "registered instance must be detected");
    static_assert(!no_trait::value, "unregistered type must not be detected");

    // the conversion operator agrees with value
    D_CV_CHECK(static_cast<bool>(yes_trait()) == yes_trait::value);
    D_CV_CHECK(static_cast<bool>(no_trait()) == no_trait::value);

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // integral_constant::operator() (C++14 onward)
    D_CV_CHECK(yes_trait()() == true);
    D_CV_CHECK(no_trait()() == false);
#endif

    return true;
}

/*
tests_contravariant_enable_hook_specialization
  The trait's second parameter is a working SFINAE hook.
  Tests the following:
  - a specialization keyed on enable_if registers a whole family at once
  - such a family is detected exactly like a named specialization
  - the hook's predicate is what selects it: a sibling type the predicate
    rejects stays undetected
  - the hook specialization and a named one coexist without ambiguity
*/
bool
tests_contravariant_enable_hook_specialization()
{
    // the family predicate itself
    D_CV_CHECK(is_sink_context< sink_of<int> >::value);
    D_CV_CHECK(is_sink_context< sink_of<std::string> >::value);
    D_CV_CHECK(!is_sink_context<plain_probe>::value);
    D_CV_CHECK(!is_sink_context< to_string_of<long> >::value);

    // every member of the family is registered by the one specialization
    D_CV_CHECK(is_contravariant< sink_of<int> >::value);
    D_CV_CHECK(is_contravariant< sink_of<long> >::value);
    D_CV_CHECK(is_contravariant< sink_of< predicate_of<int> > >::value);

    // a type the hook rejects is not dragged in with it
    D_CV_CHECK(!is_contravariant<plain_probe>::value);

    // the hook exposes the same members a named specialization does
    D_CV_CHECK((std::is_same<contravariant_traits< sink_of<int> >::value_type,
                             int>::value));

    // named and hook specializations resolve independently
    D_CV_CHECK((std::is_same<contravariant_traits< to_string_of<int> >::value_type,
                             int>::value));
    D_CV_CHECK(is_contravariant< to_string_of<int> >::value);

    return true;
}

/*
tests_contravariant_marker_presence_not_truth
  The detector keys on the presence of is_specialized, not on its value.
  Tests the following:
  - a specialization declaring is_specialized = std::false_type is DETECTED
  - the declared marker really is std::false_type, so the registration is not
    being misread
  - a marker-only registration (no value_type at all) is likewise detected,
    since the marker is the whole obligation the detector checks
*/
bool
tests_contravariant_marker_presence_not_truth()
{
    // the marker says false ...
    D_CV_CHECK((std::is_same<contravariant_traits<false_marked>::is_specialized,
                             std::false_type>::value));
    D_CV_CHECK(!contravariant_traits<false_marked>::is_specialized::value);

    // ... and the type is detected anyway: presence is the test
    D_CV_CHECK(is_contravariant<false_marked>::value);

    // a marker with no value_type beside it is enough on its own
    D_CV_CHECK(is_contravariant<marker_only>::value);

    // while a value_type with no marker beside it is not
    D_CV_CHECK(!is_contravariant<value_only>::value);

    return true;
}

/*
tests_contravariant_marker_must_be_value_initializable
  The detector value-initializes the marker, so the marker must permit it.
  Tests the following:
  - a marker type with a deleted default constructor makes is_specialized{}
    ill-formed, and detection SFINAEs back to false
  - the specialization itself is nonetheless perfectly visible, so the false
    is the detector's doing and not a missing registration
  - a value-initializable marker on the same shape of registration is detected
*/
bool
tests_contravariant_marker_must_be_value_initializable()
{
    // the marker type cannot be value-initialized
    D_CV_CHECK(!std::is_default_constructible<no_default>::value);

    // the registration is there: its members resolve
    D_CV_CHECK((std::is_same<contravariant_traits<hostile_marked>::value_type,
                             int>::value));
    D_CV_CHECK((std::is_same<contravariant_traits<hostile_marked>::is_specialized,
                             no_default>::value));

    // yet detection is false, because is_specialized{} is ill-formed
    D_CV_CHECK(!is_contravariant<hostile_marked>::value);

    // an otherwise identical registration with a usable marker is detected
    D_CV_CHECK(std::is_default_constructible<std::true_type>::value);
    D_CV_CHECK(is_contravariant<ref_consumer>::value);

    // the extractor is unaffected: it never touches the marker
    D_CV_CHECK(has_inner_value_type<hostile_marked>::value);

    return true;
}

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
/*
tests_contravariant_variable_template_agreement
  The C++14 variable-template shorthand.
  Tests the following:
  - is_contravariant_v agrees with is_contravariant<T>::value on every
    registered instance
  - it agrees on every rejected type as well
  - it agrees through cv-ref spellings
  - it is itself a constant expression
*/
bool
tests_contravariant_variable_template_agreement()
{
    // positive terms
    D_CV_CHECK(is_contravariant_v< to_string_of<long> > ==
               is_contravariant< to_string_of<long> >::value);
    D_CV_CHECK(is_contravariant_v< predicate_of<int> > ==
               is_contravariant< predicate_of<int> >::value);
    D_CV_CHECK(is_contravariant_v< sink_of<double> > ==
               is_contravariant< sink_of<double> >::value);
    D_CV_CHECK(is_contravariant_v<marker_only> ==
               is_contravariant<marker_only>::value);

    // negative terms
    D_CV_CHECK(is_contravariant_v<int> == is_contravariant<int>::value);
    D_CV_CHECK(is_contravariant_v<void> == is_contravariant<void>::value);
    D_CV_CHECK(is_contravariant_v<plain_probe> ==
               is_contravariant<plain_probe>::value);
    D_CV_CHECK(is_contravariant_v<never_defined> ==
               is_contravariant<never_defined>::value);
    D_CV_CHECK(is_contravariant_v<hostile_marked> ==
               is_contravariant<hostile_marked>::value);

    // cv-ref spellings
    D_CV_CHECK(is_contravariant_v< const to_string_of<long>& > ==
               is_contravariant< const to_string_of<long>& >::value);

    // and the shorthand is a constant expression in its own right
    static_assert(is_contravariant_v< to_string_of<long> >,
                  "is_contravariant_v must be usable in a constant expression");
    static_assert(!is_contravariant_v<plain_probe>,
                  "is_contravariant_v must reject unregistered types");

    return true;
}
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

NS_END  // testing
NS_END  // djinterp
