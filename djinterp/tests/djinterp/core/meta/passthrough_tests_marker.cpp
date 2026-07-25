#include "passthrough_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_marker_is_empty_and_inheritable
  passthrough_marker is an empty, inheritable base.
  Tests the following:
  - it is a class type
  - it is empty (no non-static data members)
  - it is standard-layout (a plain tag)
  - a type inheriting it is recognized, which is the marker's whole purpose
*/
bool
tests_marker_is_empty_and_inheritable()
{
    D_PT_CHECK(std::is_class<passthrough_marker>::value);
    D_PT_CHECK(std::is_empty<passthrough_marker>::value);
    D_PT_CHECK(std::is_standard_layout<passthrough_marker>::value);

    // the point of the marker: inheriting it opts a type in
    D_PT_CHECK(is_passthrough<pt_direct>::value);

    return true;
}

/*
tests_marker_direct_inheritor_is_passthrough
  A direct public inheritor of the marker is a passthrough.
  Tests the following:
  - pt_direct is a passthrough
  - the result is exactly true (a bool trait, not merely truthy)
  - a second, independently defined direct inheritor is also detected
  - the marker being a base is what drives it (is_base_of holds)
*/
bool
tests_marker_direct_inheritor_is_passthrough()
{
    D_PT_CHECK(is_passthrough<pt_direct>::value);
    D_PT_CHECK(is_passthrough<pt_direct>::value == true);

    // the multi-inheritance fixture is also a direct inheritor of the marker
    D_PT_CHECK(is_passthrough<pt_multi>::value);

    // the underlying relationship
    D_PT_CHECK((std::is_base_of<passthrough_marker, pt_direct>::value));

    return true;
}

/*
tests_marker_non_inheritor_is_not_passthrough
  A type that does not inherit the marker is not a passthrough.
  Tests the following:
  - a struct inheriting nothing is not a passthrough
  - a struct inheriting an UNRELATED base is not a passthrough
  - inheriting "something" is therefore not inheriting the marker
  - the unrelated base itself is not a passthrough
*/
bool
tests_marker_non_inheritor_is_not_passthrough()
{
    D_PT_CHECK(!is_passthrough<plain>::value);
    D_PT_CHECK(!is_passthrough<derives_unrelated>::value);
    D_PT_CHECK(!is_passthrough<unrelated_base>::value);

    // inheriting an unrelated base is not the same as inheriting the marker
    D_PT_CHECK(is_passthrough<derives_unrelated>::value !=
               is_passthrough<pt_direct>::value);

    return true;
}

/*
tests_marker_fundamentals_and_void_are_not_passthrough
  Non-class types are not passthroughs.
  Tests the following:
  - fundamental types are not passthroughs
  - void is not a passthrough
  - a pointer type is not a passthrough
  - an enum is not a passthrough
*/
bool
tests_marker_fundamentals_and_void_are_not_passthrough()
{
    D_PT_CHECK(!is_passthrough<int>::value);
    D_PT_CHECK(!is_passthrough<double>::value);
    D_PT_CHECK(!is_passthrough<char>::value);
    D_PT_CHECK(!is_passthrough<void>::value);
    D_PT_CHECK(!is_passthrough<int*>::value);

    enum an_enum { a, b };
    D_PT_CHECK(!is_passthrough<an_enum>::value);

    return true;
}

/*
tests_marker_variable_companion_agrees
  is_passthrough_v tracks is_passthrough<T>::value.
  Tests the following:
  - the companion equals the trait in the positive case
  - and in the negative case
  - it holds across several fixtures
  - it is usable in a constant expression
*/
bool
tests_marker_variable_companion_agrees()
{
    D_PT_CHECK(is_passthrough_v<pt_direct> == is_passthrough<pt_direct>::value);
    D_PT_CHECK(is_passthrough_v<plain> == is_passthrough<plain>::value);
    D_PT_CHECK(is_passthrough_v<pt_multi> == is_passthrough<pt_multi>::value);
    D_PT_CHECK(is_passthrough_v<int> == is_passthrough<int>::value);

    static_assert(is_passthrough_v<pt_direct>, "companion is a constant expression");
    static_assert(!is_passthrough_v<plain>, "companion rejects non-passthroughs");

    return true;
}

NS_END  // testing
NS_END  // djinterp
