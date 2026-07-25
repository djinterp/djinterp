#include "passthrough_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_semantics_strip_cv_ref
  clean_t strips cv-ref before is_base_of runs.
  Tests the following:
  - is_passthrough<const D&>, <volatile D>, <D&&> agree with is_passthrough<D>
  - const-volatile-reference agrees as well
  - the stripping applies equally in the false direction
  - a reference to a non-passthrough stays false
*/
bool
tests_semantics_strip_cv_ref()
{
    // positive under every cv-ref spelling
    D_PT_CHECK(is_passthrough<pt_direct>::value);
    D_PT_CHECK(is_passthrough<const pt_direct>::value);
    D_PT_CHECK(is_passthrough<volatile pt_direct>::value);
    D_PT_CHECK(is_passthrough<pt_direct&>::value);
    D_PT_CHECK(is_passthrough<pt_direct&&>::value);
    D_PT_CHECK(is_passthrough<const pt_direct&>::value);
    D_PT_CHECK(is_passthrough<const volatile pt_direct&>::value);

    // negative under the same spellings
    D_PT_CHECK(!is_passthrough<plain>::value);
    D_PT_CHECK(!is_passthrough<const plain&>::value);
    D_PT_CHECK(!is_passthrough<plain&&>::value);
    D_PT_CHECK(!is_passthrough<volatile plain>::value);

    return true;
}

/*
tests_semantics_inheritance_is_transitive
  Detection follows the inheritance chain, not just the direct base.
  Tests the following:
  - a grandchild of the marker is a passthrough
  - the intermediate class is a passthrough too
  - both agree with the underlying is_base_of
  - a cv-ref spelling of the grandchild is still detected
*/
bool
tests_semantics_inheritance_is_transitive()
{
    // pt_child : pt_direct : passthrough_marker
    D_PT_CHECK(is_passthrough<pt_child>::value);
    D_PT_CHECK(is_passthrough<pt_direct>::value);

    D_PT_CHECK((std::is_base_of<passthrough_marker, pt_child>::value));

    // through a qualified spelling
    D_PT_CHECK(is_passthrough<const pt_child&>::value);

    return true;
}

/*
tests_semantics_private_inheritance_still_counts
  std::is_base_of ignores access, so private inheritance still counts.
  Tests the following:
  - a type privately inheriting the marker is a passthrough
  - this matches the underlying is_base_of, which is access-insensitive
  - a public inheritor is likewise detected (contrast on access, same result)
  - a cv-ref spelling of the private inheritor is still detected
*/
bool
tests_semantics_private_inheritance_still_counts()
{
    // pt_private : private passthrough_marker
    D_PT_CHECK(is_passthrough<pt_private>::value);
    D_PT_CHECK((std::is_base_of<passthrough_marker, pt_private>::value));

    // public inheritor, same detection outcome
    D_PT_CHECK(is_passthrough<pt_direct>::value);

    // qualified spelling of the private inheritor
    D_PT_CHECK(is_passthrough<const pt_private&>::value);

    return true;
}

/*
tests_semantics_multiple_inheritance_counts
  A mixed base list still counts if the marker is among the bases.
  Tests the following:
  - a type inheriting the marker plus an unrelated base is a passthrough
  - a type inheriting only the unrelated base is not
  - the marker's presence in the base list is decisive
  - the unrelated base alone is not a passthrough
*/
bool
tests_semantics_multiple_inheritance_counts()
{
    // pt_multi : unrelated_base, passthrough_marker
    D_PT_CHECK(is_passthrough<pt_multi>::value);

    // only the unrelated base -> not a passthrough
    D_PT_CHECK(!is_passthrough<derives_unrelated>::value);
    D_PT_CHECK(!is_passthrough<unrelated_base>::value);

    // the marker's presence is what flips it
    D_PT_CHECK(is_passthrough<pt_multi>::value !=
               is_passthrough<derives_unrelated>::value);

    return true;
}

/*
tests_semantics_marker_is_its_own_base
  std::is_base_of reports a class as its own base, so the marker is a
  passthrough.
  Tests the following:
  - is_passthrough<passthrough_marker> is true
  - this matches is_base_of<passthrough_marker, passthrough_marker>
  - a cv-ref spelling of the marker is still a passthrough
  - a DIFFERENT empty type is not a passthrough (the marker is specifically the
    base that counts)
*/
bool
tests_semantics_marker_is_its_own_base()
{
    D_PT_CHECK(is_passthrough<passthrough_marker>::value);
    D_PT_CHECK((std::is_base_of<passthrough_marker, passthrough_marker>::value));

    // qualified spelling
    D_PT_CHECK(is_passthrough<const passthrough_marker&>::value);

    // a different empty type is not the marker
    D_PT_CHECK(!is_passthrough<unrelated_base>::value);

    return true;
}

/*
tests_semantics_pointer_to_passthrough_is_not
  A pointer to a passthrough is not itself a passthrough.
  Tests the following:
  - a pointer to a direct passthrough is not a passthrough
  - a pointer to the marker is not a passthrough
  - the pointee itself is a passthrough (control)
  - a reference (unlike a pointer) IS stripped and stays detected (contrast)
*/
bool
tests_semantics_pointer_to_passthrough_is_not()
{
    D_PT_CHECK(!is_passthrough<pt_direct*>::value);
    D_PT_CHECK(!is_passthrough<passthrough_marker*>::value);

    // the pointee is a passthrough
    D_PT_CHECK(is_passthrough<pt_direct>::value);

    // a reference is stripped by clean_t (contrast with the pointer)
    D_PT_CHECK(is_passthrough<pt_direct&>::value);

    return true;
}

/*
tests_semantics_trait_shape_is_integral_constant
  is_passthrough has the shape of a standard boolean trait.
  Tests the following:
  - a true instance derives from std::true_type; a false one from
    std::false_type
  - value_type is bool and ::type is the matching integral_constant
  - ::value is a constant expression
  - the conversion operator agrees with ::value
*/
bool
tests_semantics_trait_shape_is_integral_constant()
{
    using yes = is_passthrough<pt_direct>;
    using no  = is_passthrough<plain>;

    D_PT_CHECK(std::is_base_of<std::true_type, yes>::value);
    D_PT_CHECK(std::is_base_of<std::false_type, no>::value);

    D_PT_CHECK(std::is_same<yes::value_type, bool>::value);
    D_PT_CHECK(std::is_same<no::value_type, bool>::value);
    D_PT_CHECK(std::is_same<yes::type, std::true_type>::value);
    D_PT_CHECK(std::is_same<no::type, std::false_type>::value);

    static_assert(yes::value, "passthrough must be detected");
    static_assert(!no::value, "non-passthrough must not be detected");

    D_PT_CHECK(static_cast<bool>(yes()) == yes::value);
    D_PT_CHECK(static_cast<bool>(no()) == no::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
