#include "member_types_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_semantics_strip_cv_ref_via_clean_t
  The probe strips cv-ref through clean_t before looking for the typedef.
  Tests the following:
  - has_X<const T&>, has_X<volatile T>, has_X<T&&> agree with has_X<T> (true)
  - const-volatile-reference agrees as well
  - the same stripping applies in the false direction (an empty type stays
    undetected under every cv-ref spelling)
  - a plain reference to a container is detected like the container
*/
bool
tests_semantics_strip_cv_ref_via_clean_t()
{
    // positive: only_value has value_type, under every cv-ref spelling
    D_MT_CHECK(has_value_type<only_value>::value);
    D_MT_CHECK(has_value_type<const only_value>::value);
    D_MT_CHECK(has_value_type<volatile only_value>::value);
    D_MT_CHECK(has_value_type<only_value&>::value);
    D_MT_CHECK(has_value_type<only_value&&>::value);
    D_MT_CHECK(has_value_type<const only_value&>::value);
    D_MT_CHECK(has_value_type<const volatile only_value&>::value);

    // negative: has_none never has it, under the same spellings
    D_MT_CHECK(!has_value_type<has_none>::value);
    D_MT_CHECK(!has_value_type<const has_none&>::value);
    D_MT_CHECK(!has_value_type<has_none&&>::value);

    // a reference to a real container is detected like the container
    D_MT_CHECK(has_value_type<const std::vector<int>&>::value);
    D_MT_CHECK(has_size_type<std::vector<int>&>::value);

    return true;
}

/*
tests_semantics_pointer_is_not_seen_through
  clean_t strips cv-ref but does NOT dereference a pointer.
  Tests the following:
  - has_X<T*> is false even when T has the typedef
  - has_X<const T*> and has_X<T* const> are likewise false
  - the pointee itself is still detected (control)
  - a pointer to a container is not a container for detection purposes
*/
bool
tests_semantics_pointer_is_not_seen_through()
{
    // only_value has value_type, but a pointer to it does not
    D_MT_CHECK(has_value_type<only_value>::value);          // control
    D_MT_CHECK(!has_value_type<only_value*>::value);
    D_MT_CHECK(!has_value_type<const only_value*>::value);
    D_MT_CHECK(!has_value_type<only_value* const>::value);

    // a pointer to a container is not seen through either
    D_MT_CHECK(!has_value_type<std::vector<int>*>::value);
    D_MT_CHECK(has_value_type<std::vector<int> >::value);   // control

    return true;
}

/*
tests_semantics_value_member_is_not_a_typedef
  The probe detects a nested TYPE, not a nested value.
  Tests the following:
  - a struct with `static constexpr int value_type` is NOT detected
  - a struct with the actual typedef IS detected (control)
  - the distinction holds under a cv-ref spelling too
  - other detectors are unaffected by the stray value member
*/
bool
tests_semantics_value_member_is_not_a_typedef()
{
    // value_shaped has a value named value_type, not a type
    D_MT_CHECK(!has_value_type<value_shaped>::value);
    D_MT_CHECK(!has_value_type<const value_shaped&>::value);

    // a real typedef is detected
    D_MT_CHECK(has_value_type<only_value>::value);

    // the stray value member does not trip a different detector
    D_MT_CHECK(!has_key_type<value_shaped>::value);
    D_MT_CHECK(!has_size_type<value_shaped>::value);

    return true;
}

/*
tests_semantics_void_typedef_is_present
  A typedef to void is still a typedef.
  Tests the following:
  - has_value_type is true when value_type is void
  - it is true under a cv-ref spelling too
  - a type WITHOUT the typedef remains false (contrast)
  - the void-typedef fixture does not trip unrelated detectors
*/
bool
tests_semantics_void_typedef_is_present()
{
    D_MT_CHECK(has_value_type<void_typedef>::value);
    D_MT_CHECK(has_value_type<const void_typedef&>::value);

    // contrast with a type that has no such typedef
    D_MT_CHECK(!has_value_type<has_none>::value);

    // unrelated detectors stay false
    D_MT_CHECK(!has_key_type<void_typedef>::value);
    D_MT_CHECK(!has_input_type<void_typedef>::value);

    return true;
}

/*
tests_semantics_incomplete_type_is_soft_false
  Detection on an incomplete type is a clean false, not a hard error.
  Tests the following:
  - has_X<incomplete_type> compiles and is false (this TU compiling is half the
    assertion)
  - it is false for several different detectors
  - a cv-ref spelling of the incomplete type is likewise a soft false
  - a complete type with the typedef still resolves (control)
*/
bool
tests_semantics_incomplete_type_is_soft_false()
{
    // incomplete_type is declared, never defined
    D_MT_CHECK(!has_value_type<incomplete_type>::value);
    D_MT_CHECK(!has_key_type<incomplete_type>::value);
    D_MT_CHECK(!has_size_type<incomplete_type>::value);

    // cv-ref spelling of an incomplete type is still a soft false
    D_MT_CHECK(!has_value_type<const incomplete_type&>::value);

    // control: a complete type resolves normally
    D_MT_CHECK(has_value_type<only_value>::value);

    return true;
}

/*
tests_semantics_inherited_typedef_is_visible
  A nested typedef inherited from a base is visible to the detector.
  Tests the following:
  - a struct deriving from has_all is detected as having the typedefs
  - several inherited typedefs are visible, not just one
  - the derived type reports the same as its base for those detectors
  - a detector for a typedef the base lacks stays false
*/
bool
tests_semantics_inherited_typedef_is_visible()
{
    D_MT_CHECK(has_value_type<derived_from_has_all>::value);
    D_MT_CHECK(has_input_type<derived_from_has_all>::value);
    D_MT_CHECK(has_allocator_type<derived_from_has_all>::value);

    // parity with the base
    D_MT_CHECK(has_value_type<derived_from_has_all>::value ==
               has_value_type<has_all>::value);

    // has_all declares all ten, so there is no "absent" inherited typedef to
    // contrast; confirm an unrelated fixture's detector is still independent
    D_MT_CHECK(!has_value_type<has_none>::value);

    return true;
}

/*
tests_semantics_trait_shape_is_integral_constant
  Each detector has the shape of a standard boolean trait.
  Tests the following:
  - a true detector derives from std::true_type; a false one from
    std::false_type
  - value_type is bool and ::type is the matching integral_constant
  - ::value is a constant expression usable in static_assert
  - the conversion operator agrees with ::value
*/
bool
tests_semantics_trait_shape_is_integral_constant()
{
    using yes = has_value_type<only_value>;
    using no  = has_value_type<has_none>;

    // integral_constant lineage
    D_MT_CHECK(std::is_base_of<std::true_type, yes>::value);
    D_MT_CHECK(std::is_base_of<std::false_type, no>::value);

    // nested members
    D_MT_CHECK(std::is_same<yes::value_type, bool>::value);
    D_MT_CHECK(std::is_same<no::value_type, bool>::value);
    D_MT_CHECK(std::is_same<yes::type, std::true_type>::value);
    D_MT_CHECK(std::is_same<no::type, std::false_type>::value);

    // constant expression
    static_assert(yes::value, "present typedef must be detected");
    static_assert(!no::value, "absent typedef must not be detected");

    // conversion operator agrees with value
    D_MT_CHECK(static_cast<bool>(yes()) == yes::value);
    D_MT_CHECK(static_cast<bool>(no()) == no::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
