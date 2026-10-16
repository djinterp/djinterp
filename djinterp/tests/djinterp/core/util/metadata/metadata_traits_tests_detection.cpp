/*******************************************************************************
* djinterp [core]                            metadata_traits_tests_detection.cpp
*
*
* path:      /tests/djinterp/core/util/metadata/metadata_traits_tests_detection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "metadata_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_metadata_shapes
  Each shape of the `metadata` name is detected by its own trait.
  Tests the following:
  - a data member, a const member function and a nested type alias are
    each recognised by exactly the trait that targets that shape
  - each trait rejects the OTHER two shapes, so the three are genuinely
    independent probes rather than three spellings of one check
  - a class carrying none of them registers on nothing
  - names that merely resemble the canonical one - meta_data, metadata_,
    Metadata - are not detected, so matching is exact rather than fuzzy
*/
bool
tests_metadata_shapes()
{
    // each shape is seen by its own trait
    D_MT_CHECK(has_metadata_data_member<md_member>::value);
    D_MT_CHECK(has_metadata_method<md_method>::value);
    D_MT_CHECK(has_metadata_nested_type<md_nested>::value);

    // ... and by neither of the other two
    D_MT_CHECK(!has_metadata_method<md_member>::value);
    D_MT_CHECK(!has_metadata_nested_type<md_member>::value);
    D_MT_CHECK(!has_metadata_data_member<md_method>::value);
    D_MT_CHECK(!has_metadata_nested_type<md_method>::value);
    D_MT_CHECK(!has_metadata_data_member<md_nested>::value);
    D_MT_CHECK(!has_metadata_method<md_nested>::value);

    // a type with none of them
    D_MT_CHECK(!has_metadata_data_member<md_none>::value);
    D_MT_CHECK(!has_metadata_method<md_none>::value);
    D_MT_CHECK(!has_metadata_nested_type<md_none>::value);

    // near-miss names are not the canonical name
    D_MT_CHECK(!has_metadata_data_member<md_similar>::value);
    D_MT_CHECK(!has_metadata_method<md_similar>::value);
    D_MT_CHECK(!has_metadata_nested_type<md_similar>::value);

    return true;
}

/*
tests_metadata_unifier
  `has_metadata` as the disjunction over the three shapes.
  Tests the following:
  - it is true whenever any one shape is present, whichever that is
  - it is false when none is
  - it agrees with the OR of the three component traits on every fixture,
    which is the property that keeps it honest if a fourth shape is ever
    added and wired into only some of them
*/
bool
tests_metadata_unifier()
{
    // true for each shape in turn
    D_MT_CHECK(has_metadata<md_member>::value);
    D_MT_CHECK(has_metadata<md_method>::value);
    D_MT_CHECK(has_metadata<md_nested>::value);
    D_MT_CHECK(has_metadata<md_all_names>::value);

    // false when the name is absent
    D_MT_CHECK(!has_metadata<md_none>::value);
    D_MT_CHECK(!has_metadata<md_similar>::value);
    D_MT_CHECK(!has_metadata<md_nonconst_method>::value);

    // and it is exactly the OR of its three components
    D_MT_CHECK(has_metadata<md_member>::value ==
               (has_metadata_data_member<md_member>::value ||
                has_metadata_method<md_member>::value      ||
                has_metadata_nested_type<md_member>::value));
    D_MT_CHECK(has_metadata<md_method>::value ==
               (has_metadata_data_member<md_method>::value ||
                has_metadata_method<md_method>::value      ||
                has_metadata_nested_type<md_method>::value));
    D_MT_CHECK(has_metadata<md_none>::value ==
               (has_metadata_data_member<md_none>::value ||
                has_metadata_method<md_none>::value      ||
                has_metadata_nested_type<md_none>::value));

    return true;
}

/*
tests_metadata_const_requirement
  What the expression-based probes actually require.
  Tests the following:
  - every probe goes through `const _Type&`, so a member function that
    exists and is spelled correctly but is NOT const-qualified is detected
    by nothing at all.  The traits answer "is this name reachable through
    a const handle", which is narrower than "does this name exist", and
    md_nonconst_method is the fixture that separates the two
  - the nested-type probe is unaffected, since naming a type needs no
    object and therefore no const handle - so a const-less class can still
    satisfy the alias shape
  - the module's stated mutual exclusivity of the three shapes does NOT
    hold: a data member of callable type makes `t.metadata` and
    `t.metadata()` both well-formed, so two shapes report true at once.
    The exclusivity argument is about the class member name space, which
    is real, but the probes test EXPRESSIONS, and one member can satisfy
    two expression forms.  Section III shows the extractors survive it
*/
bool
tests_metadata_const_requirement()
{
    // a non-const member function is reachable through no probe
    D_MT_CHECK(!has_metadata_method<md_nonconst_method>::value);
    D_MT_CHECK(!has_metadata_data_member<md_nonconst_method>::value);
    D_MT_CHECK(!has_metadata_nested_type<md_nonconst_method>::value);
    D_MT_CHECK(!has_metadata<md_nonconst_method>::value);

    // the const-qualified equivalent is detected, which isolates
    // const-ness as the only difference between the two fixtures
    D_MT_CHECK(has_metadata_method<md_method>::value);

    // naming a nested type needs no object, so that shape is immune
    D_MT_CHECK(has_metadata_nested_type<md_nested>::value);

    // a callable data member satisfies TWO shapes at once
    D_MT_CHECK(has_metadata_data_member<md_callable_member>::value);
    D_MT_CHECK(has_metadata_method<md_callable_member>::value);
    D_MT_CHECK(!has_metadata_nested_type<md_callable_member>::value);
    D_MT_CHECK(has_metadata<md_callable_member>::value);

    // ... whereas an ordinary, non-callable member satisfies only one,
    // so the overlap is a property of the member's type and not of every
    // data member
    D_MT_CHECK(has_metadata_data_member<md_member>::value &&
               !has_metadata_method<md_member>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
