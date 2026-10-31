/*******************************************************************************
* djinterp [cpp]                           metadata_concepts_tests_detection.cpp
*
*
* path:      /tests/djinterp/cpp/util/metadata/metadata_concepts_tests_detection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "metadata_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_metadata_concepts_shapes
  The `metadata` concepts detect each shape independently.
  Tests the following:
  - a data member, a const member function and a nested type alias are
    each satisfied by exactly the concept targeting that shape, and by
    neither of the other two
  - the unifying concept holds whenever any one of them does
  - a class carrying none of the names satisfies nothing
  Stated directly rather than only through the agreement section, so that
  a fault common to BOTH implementations - a shape genuinely undetectable
  by either - shows up as a failure here rather than as agreement on a
  wrong answer.
*/
bool tests_metadata_concepts_shapes()
{
    // each shape by its own concept
    D_MC_CHECK(concepts::has_metadata_data_member<DMcMember>);
    D_MC_CHECK(concepts::has_metadata_method<DMcMethod>);
    D_MC_CHECK(concepts::has_metadata_nested_type<DMcNested>);

    // and by neither of the other two
    D_MC_CHECK(!concepts::has_metadata_method<DMcMember>);
    D_MC_CHECK(!concepts::has_metadata_nested_type<DMcMember>);
    D_MC_CHECK(!concepts::has_metadata_data_member<DMcMethod>);
    D_MC_CHECK(!concepts::has_metadata_nested_type<DMcMethod>);
    D_MC_CHECK(!concepts::has_metadata_data_member<DMcNested>);
    D_MC_CHECK(!concepts::has_metadata_method<DMcNested>);

    // the unifier holds for each shape in turn
    D_MC_CHECK(concepts::has_metadata<DMcMember>);
    D_MC_CHECK(concepts::has_metadata<DMcMethod>);
    D_MC_CHECK(concepts::has_metadata<DMcNested>);
    D_MC_CHECK(concepts::has_metadata<DMcAllNames>);

    // absence
    D_MC_CHECK(!concepts::has_metadata_data_member<DMcNone>);
    D_MC_CHECK(!concepts::has_metadata_method<DMcNone>);
    D_MC_CHECK(!concepts::has_metadata_nested_type<DMcNone>);
    D_MC_CHECK(!concepts::has_metadata<DMcNone>);
    D_MC_CHECK(!concepts::has_metadata<int>);

    return true;
}

/*
tests_metadata_concepts_mirrors
  The `metadata_type` and `metadata_container_type` concept families.
  Tests the following:
  - the same battery as above for each of the two mirrored names
  - cross-name independence: a type carrying exactly one of the three
    canonical names satisfies that family's unifier and neither of the
    others.  The names share a prefix, so a requires-expression that
    reached for the wrong one is invisible on inspection and detectable
    only against a fixture carrying a single name
*/
bool tests_metadata_concepts_mirrors()
{
    // `metadata_type`, each shape by its own concept
    D_MC_CHECK(concepts::has_metadata_type_data_member<DMctMember>);
    D_MC_CHECK(concepts::has_metadata_type_method<DMctMethod>);
    D_MC_CHECK(concepts::has_metadata_type_nested_type<DMctNested>);
    D_MC_CHECK(!concepts::has_metadata_type_method<DMctMember>);
    D_MC_CHECK(!concepts::has_metadata_type_data_member<DMctMethod>);
    D_MC_CHECK(!concepts::has_metadata_type_nested_type<DMctMember>);
    D_MC_CHECK(concepts::has_metadata_type<DMctMember>);
    D_MC_CHECK(concepts::has_metadata_type<DMctMethod>);
    D_MC_CHECK(concepts::has_metadata_type<DMctNested>);
    D_MC_CHECK(!concepts::has_metadata_type<DMcNone>);

    // `metadata_container_type`, each shape by its own concept
    D_MC_CHECK(
     concepts::has_metadata_container_type_data_member<DMccMember>);
    D_MC_CHECK(
     concepts::has_metadata_container_type_method<DMccMethod>);
    D_MC_CHECK(
     concepts::has_metadata_container_type_nested_type<DMccNested>);
    D_MC_CHECK(
     !concepts::has_metadata_container_type_method<DMccMember>);
    D_MC_CHECK(
     !concepts::has_metadata_container_type_data_member<DMccMethod>);
    D_MC_CHECK(concepts::has_metadata_container_type<DMccMember>);
    D_MC_CHECK(concepts::has_metadata_container_type<DMccMethod>);
    D_MC_CHECK(concepts::has_metadata_container_type<DMccNested>);
    D_MC_CHECK(!concepts::has_metadata_container_type<DMcNone>);

    // cross-name independence, in all three shapes
    D_MC_CHECK(concepts::has_metadata<DMcMember> &&
               !concepts::has_metadata_type<DMcMember> &&
               !concepts::has_metadata_container_type<DMcMember>);
    D_MC_CHECK(concepts::has_metadata_type<DMctNested> &&
               !concepts::has_metadata<DMctNested> &&
               !concepts::has_metadata_container_type<DMctNested>);
    D_MC_CHECK(concepts::has_metadata_container_type<DMccMethod> &&
               !concepts::has_metadata<DMccMethod> &&
               !concepts::has_metadata_type<DMccMethod>);

    // all three at once, for a conforming type
    D_MC_CHECK(concepts::has_metadata<DMcAllNames> &&
               concepts::has_metadata_type<DMcAllNames> &&
               concepts::has_metadata_container_type<DMcAllNames>);

    return true;
}

/*
tests_metadata_concepts_edge_shapes
  The two cases where "the name is present" and "the concept is satisfied"
  come apart.
  Tests the following:
  - every requires-expression binds `const _Type&`, so a member function
    that exists but is NOT const-qualified satisfies nothing.  The concept
    asks whether the name is reachable through a const handle, which is
    narrower than whether it exists
  - a data member of callable type satisfies the data-member AND the
    member-function concepts simultaneously, because both `t.metadata` and
    `t.metadata()` are well-formed expressions for it.  The traits header
    describes the three shapes as mutually exclusive on the grounds that
    the class member name space forbids overlap; that is true of NAMES but
    not of the expressions these concepts actually test
  Both edges are checked in the concept form as well as the trait form
  because the two are separate implementations - agreeing on the ordinary
  cases is no guarantee of agreeing on these.
*/
bool tests_metadata_concepts_edge_shapes()
{
    // a non-const member function satisfies nothing
    D_MC_CHECK(!concepts::has_metadata_method<DMcNonconstMethod>);
    D_MC_CHECK(
     !concepts::has_metadata_data_member<DMcNonconstMethod>);
    D_MC_CHECK(
     !concepts::has_metadata_nested_type<DMcNonconstMethod>);
    D_MC_CHECK(!concepts::has_metadata<DMcNonconstMethod>);

    // the const-qualified counterpart does, isolating const-ness as the
    // only difference between the two fixtures
    D_MC_CHECK(concepts::has_metadata_method<DMcMethod>);

    // a callable data member satisfies two shapes at once
    D_MC_CHECK(concepts::has_metadata_data_member<DMcCallableMember>);
    D_MC_CHECK(concepts::has_metadata_method<DMcCallableMember>);
    D_MC_CHECK(
     !concepts::has_metadata_nested_type<DMcCallableMember>);
    D_MC_CHECK(concepts::has_metadata<DMcCallableMember>);

    // an ordinary data member satisfies only one, so the overlap belongs
    // to the member's type rather than to data members generally
    D_MC_CHECK(concepts::has_metadata_data_member<DMcMember> &&
               !concepts::has_metadata_method<DMcMember>);

    // both edges are reported identically by the trait implementation
    D_MC_AGREES(has_metadata_method, DMcNonconstMethod);
    D_MC_AGREES(has_metadata_data_member, DMcCallableMember);
    D_MC_AGREES(has_metadata_method, DMcCallableMember);

    return true;
}


NS_END  // testing
NS_END  // djinterp
