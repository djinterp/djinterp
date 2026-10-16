/*******************************************************************************
* djinterp [core]                          metadata_concepts_tests_detection.cpp
*
*
* path:      /tests/djinterp/core/util/metadata/metadata_concepts_tests_detection.cpp
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
bool
tests_metadata_concepts_shapes()
{
    // each shape by its own concept
    D_MC_CHECK(concepts::has_metadata_data_member<mc_member>);
    D_MC_CHECK(concepts::has_metadata_method<mc_method>);
    D_MC_CHECK(concepts::has_metadata_nested_type<mc_nested>);

    // and by neither of the other two
    D_MC_CHECK(!concepts::has_metadata_method<mc_member>);
    D_MC_CHECK(!concepts::has_metadata_nested_type<mc_member>);
    D_MC_CHECK(!concepts::has_metadata_data_member<mc_method>);
    D_MC_CHECK(!concepts::has_metadata_nested_type<mc_method>);
    D_MC_CHECK(!concepts::has_metadata_data_member<mc_nested>);
    D_MC_CHECK(!concepts::has_metadata_method<mc_nested>);

    // the unifier holds for each shape in turn
    D_MC_CHECK(concepts::has_metadata<mc_member>);
    D_MC_CHECK(concepts::has_metadata<mc_method>);
    D_MC_CHECK(concepts::has_metadata<mc_nested>);
    D_MC_CHECK(concepts::has_metadata<mc_all_names>);

    // absence
    D_MC_CHECK(!concepts::has_metadata_data_member<mc_none>);
    D_MC_CHECK(!concepts::has_metadata_method<mc_none>);
    D_MC_CHECK(!concepts::has_metadata_nested_type<mc_none>);
    D_MC_CHECK(!concepts::has_metadata<mc_none>);
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
bool
tests_metadata_concepts_mirrors()
{
    // `metadata_type`, each shape by its own concept
    D_MC_CHECK(concepts::has_metadata_type_data_member<mct_member>);
    D_MC_CHECK(concepts::has_metadata_type_method<mct_method>);
    D_MC_CHECK(concepts::has_metadata_type_nested_type<mct_nested>);
    D_MC_CHECK(!concepts::has_metadata_type_method<mct_member>);
    D_MC_CHECK(!concepts::has_metadata_type_data_member<mct_method>);
    D_MC_CHECK(!concepts::has_metadata_type_nested_type<mct_member>);
    D_MC_CHECK(concepts::has_metadata_type<mct_member>);
    D_MC_CHECK(concepts::has_metadata_type<mct_method>);
    D_MC_CHECK(concepts::has_metadata_type<mct_nested>);
    D_MC_CHECK(!concepts::has_metadata_type<mc_none>);

    // `metadata_container_type`, each shape by its own concept
    D_MC_CHECK(
     concepts::has_metadata_container_type_data_member<mcc_member>);
    D_MC_CHECK(
     concepts::has_metadata_container_type_method<mcc_method>);
    D_MC_CHECK(
     concepts::has_metadata_container_type_nested_type<mcc_nested>);
    D_MC_CHECK(
     !concepts::has_metadata_container_type_method<mcc_member>);
    D_MC_CHECK(
     !concepts::has_metadata_container_type_data_member<mcc_method>);
    D_MC_CHECK(concepts::has_metadata_container_type<mcc_member>);
    D_MC_CHECK(concepts::has_metadata_container_type<mcc_method>);
    D_MC_CHECK(concepts::has_metadata_container_type<mcc_nested>);
    D_MC_CHECK(!concepts::has_metadata_container_type<mc_none>);

    // cross-name independence, in all three shapes
    D_MC_CHECK(concepts::has_metadata<mc_member> &&
               !concepts::has_metadata_type<mc_member> &&
               !concepts::has_metadata_container_type<mc_member>);
    D_MC_CHECK(concepts::has_metadata_type<mct_nested> &&
               !concepts::has_metadata<mct_nested> &&
               !concepts::has_metadata_container_type<mct_nested>);
    D_MC_CHECK(concepts::has_metadata_container_type<mcc_method> &&
               !concepts::has_metadata<mcc_method> &&
               !concepts::has_metadata_type<mcc_method>);

    // all three at once, for a conforming type
    D_MC_CHECK(concepts::has_metadata<mc_all_names> &&
               concepts::has_metadata_type<mc_all_names> &&
               concepts::has_metadata_container_type<mc_all_names>);

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
bool
tests_metadata_concepts_edge_shapes()
{
    // a non-const member function satisfies nothing
    D_MC_CHECK(!concepts::has_metadata_method<mc_nonconst_method>);
    D_MC_CHECK(
     !concepts::has_metadata_data_member<mc_nonconst_method>);
    D_MC_CHECK(
     !concepts::has_metadata_nested_type<mc_nonconst_method>);
    D_MC_CHECK(!concepts::has_metadata<mc_nonconst_method>);

    // the const-qualified counterpart does, isolating const-ness as the
    // only difference between the two fixtures
    D_MC_CHECK(concepts::has_metadata_method<mc_method>);

    // a callable data member satisfies two shapes at once
    D_MC_CHECK(concepts::has_metadata_data_member<mc_callable_member>);
    D_MC_CHECK(concepts::has_metadata_method<mc_callable_member>);
    D_MC_CHECK(
     !concepts::has_metadata_nested_type<mc_callable_member>);
    D_MC_CHECK(concepts::has_metadata<mc_callable_member>);

    // an ordinary data member satisfies only one, so the overlap belongs
    // to the member's type rather than to data members generally
    D_MC_CHECK(concepts::has_metadata_data_member<mc_member> &&
               !concepts::has_metadata_method<mc_member>);

    // both edges are reported identically by the trait implementation
    D_MC_AGREES(has_metadata_method, mc_nonconst_method);
    D_MC_AGREES(has_metadata_data_member, mc_callable_member);
    D_MC_AGREES(has_metadata_method, mc_callable_member);

    return true;
}


NS_END  // testing
NS_END  // djinterp
