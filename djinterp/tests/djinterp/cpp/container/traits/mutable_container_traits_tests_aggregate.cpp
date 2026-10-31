/*******************************************************************************
* djinterp [cpp]                    mutable_container_traits_tests_aggregate.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/mutable_container_traits_tests_aggregate.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "mutable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================
//   DMutableContainerClass republishes, as one flat set of members, everything
// the four sections above compute.  It adds no logic of its own, so the tests
// that matter most are the AGREEMENT tests: every member must equal the trait
// it mirrors, for every carrier.  A snapshot that drifts from its sources is
// worse than no snapshot at all.


/*
tests_mutable_container_class_signals
  Verifies the seven per-signal members of the snapshot.
  Tests the following:
  - a fully mutable sequence lights the members its interface earns, and only
    those: vector appends at the back but not the front, and its insert is
    positional, so can_push_front and can_insert stay false
  - an associative discipline lights insert / erase / clear and nothing else
  - a fixed-extent container lights no structural member at all
  - can_overwrite_element tracks the element capability rather than any single
    signal
*/
bool tests_mutable_container_class_signals()
{
    bool ok = true;

    // a growable contiguous sequence
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_push_back  == true);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_push_front == false);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_insert     == false);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_erase      == true);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_clear      == true);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_resize     == true);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_overwrite_element
                   == true);

    // a double-ended sequence adds the front
    D_MC_CHECK(DMutableContainerClass<DMcDequeInt>::can_push_front == true);

    // an equivalence discipline
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::can_push_back  == false);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::can_push_front == false);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::can_insert     == true);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::can_erase      == true);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::can_clear      == true);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::can_resize     == false);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::can_overwrite_element
                   == false);

    // a fixed extent lights nothing structural
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::can_push_back  == false);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::can_push_front == false);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::can_insert     == false);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::can_erase      == false);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::can_clear      == false);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::can_resize     == false);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::can_overwrite_element
                   == true);

    // every signal at once, and none at all
    D_MC_CHECK(DMutableContainerClass<DMcAllSignals>::can_push_back  == true);
    D_MC_CHECK(DMutableContainerClass<DMcAllSignals>::can_push_front == true);
    D_MC_CHECK(DMutableContainerClass<DMcAllSignals>::can_insert     == true);
    D_MC_CHECK(DMutableContainerClass<DMcAllSignals>::can_erase      == true);
    D_MC_CHECK(DMutableContainerClass<DMcAllSignals>::can_clear      == true);
    D_MC_CHECK(DMutableContainerClass<DMcAllSignals>::can_resize     == true);

    D_MC_CHECK(DMutableContainerClass<DMcBare>::can_push_back  == false);
    D_MC_CHECK(DMutableContainerClass<DMcBare>::can_push_front == false);
    D_MC_CHECK(DMutableContainerClass<DMcBare>::can_insert     == false);
    D_MC_CHECK(DMutableContainerClass<DMcBare>::can_erase      == false);
    D_MC_CHECK(DMutableContainerClass<DMcBare>::can_clear      == false);
    D_MC_CHECK(DMutableContainerClass<DMcBare>::can_resize     == false);
    D_MC_CHECK(DMutableContainerClass<DMcBare>::can_overwrite_element
                   == false);

    return ok;
}

/*
tests_mutable_container_class_capabilities
  Verifies the four capability members of the snapshot.
  Tests the following:
  - the four corners are reported correctly: vector (both), set (structural),
    array (element), and a size-only observer (neither)
  - is_immutable requires container-hood, so it stays false for a bare type
    with no mutators and no size()
  - is_mutable and is_immutable are not complements: a type that is neither
    mutable nor a container reads false for both
*/
bool tests_mutable_container_class_capabilities()
{
    bool ok = true;

    // both capabilities
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::is_element_mutable
                   == true);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::is_structurally_mutable
                   == true);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::is_mutable   == true);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::is_immutable == false);

    // structural only
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::is_element_mutable
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::is_structurally_mutable
                   == true);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::is_mutable   == true);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::is_immutable == false);

    // element only
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::is_element_mutable
                   == true);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::is_structurally_mutable
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::is_mutable   == true);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::is_immutable == false);

    // neither, and a container
    D_MC_CHECK(DMutableContainerClass<DMcSizedOnly>::is_element_mutable
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcSizedOnly>::is_structurally_mutable
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcSizedOnly>::is_mutable   == false);
    D_MC_CHECK(DMutableContainerClass<DMcSizedOnly>::is_immutable == true);

    // neither, and NOT a container - is_mutable and is_immutable are both
    // false, so they are not complements of one another
    D_MC_CHECK(DMutableContainerClass<DMcBare>::is_mutable   == false);
    D_MC_CHECK(DMutableContainerClass<DMcBare>::is_immutable == false);
    D_MC_CHECK(DMutableContainerClass<int>::is_mutable       == false);
    D_MC_CHECK(DMutableContainerClass<int>::is_immutable     == false);

    return ok;
}

/*
tests_mutable_container_class_grade
  Verifies the grade members of the snapshot.
  Tests the following:
  - the grade reported for each of the four corners
  - grade_name is the spelling of that same grade, so the two members cannot
    drift apart
  - the grade satisfies its own corner predicate and the matching admissions
*/
bool tests_mutable_container_class_grade()
{
    bool ok = true;

    // the four corners
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::grade
                   == mutability::fully_mutable);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::grade
                   == mutability::structural_mutable);
    D_MC_CHECK(DMutableContainerClass<DMcListInt>::grade
                   == mutability::structural_mutable);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::grade
                   == mutability::element_mutable);
    D_MC_CHECK(DMutableContainerClass<DMcSizedOnly>::grade
                   == mutability::immutable);

    // grade_name spells that grade
    D_MC_CHECK(std::strcmp(DMutableContainerClass<DMcVectorInt>::grade_name,
                           "fully_mutable") == 0);
    D_MC_CHECK(std::strcmp(DMutableContainerClass<DMcSetInt>::grade_name,
                           "structural_mutable") == 0);
    D_MC_CHECK(std::strcmp(DMutableContainerClass<DMcArray4>::grade_name,
                           "element_mutable") == 0);
    D_MC_CHECK(std::strcmp(DMutableContainerClass<DMcSizedOnly>::grade_name,
                           "immutable") == 0);

    // the two members agree by construction
    D_MC_CHECK(std::strcmp(
                   DMutableContainerClass<DMcVectorInt>::grade_name,
                   mutability_name(
                       DMutableContainerClass<DMcVectorInt>::grade)) == 0);
    D_MC_CHECK(std::strcmp(
                   DMutableContainerClass<DMcMapIi>::grade_name,
                   mutability_name(
                       DMutableContainerClass<DMcMapIi>::grade)) == 0);

    // and the grade behaves as a grade
    D_MC_CHECK(is_fully_mutable_grade(
                   DMutableContainerClass<DMcVectorInt>::grade) == true);
    D_MC_CHECK(grade_admits_structural_mutation(
                   DMutableContainerClass<DMcSetInt>::grade) == true);
    D_MC_CHECK(grade_admits_element_mutation(
                   DMutableContainerClass<DMcSetInt>::grade) == false);
    D_MC_CHECK(is_immutable_grade(
                   DMutableContainerClass<DMcSizedOnly>::grade) == true);

    return ok;
}

/*
tests_mutable_container_class_access
  Verifies the access-dimension members of the snapshot.  Note that two of them
  - constrains_element and constrains_key - deliberately shadow the namespace-
  scope traits of the same name inside the class; these tests read them through
  the class, which is the spelling a caller uses.
  Tests the following:
  - the restriction reported for each discipline
  - access_name is the spelling of that same restriction
  - the two shadowing members carry the standalone predicates' answers
*/
bool tests_mutable_container_class_access()
{
    bool ok = true;

    // the three restrictions
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::access
                   == access_restriction::none);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::access
                   == access_restriction::element_const);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::access
                   == access_restriction::key_const);

    // access_name spells that restriction
    D_MC_CHECK(std::strcmp(DMutableContainerClass<DMcVectorInt>::access_name,
                           "none") == 0);
    D_MC_CHECK(std::strcmp(DMutableContainerClass<DMcSetInt>::access_name,
                           "element_const") == 0);
    D_MC_CHECK(std::strcmp(DMutableContainerClass<DMcMapIi>::access_name,
                           "key_const") == 0);

    // the two members agree by construction
    D_MC_CHECK(std::strcmp(
                   DMutableContainerClass<DMcMapIi>::access_name,
                   access_restriction_name(
                       DMutableContainerClass<DMcMapIi>::access)) == 0);

    // the shadowing predicate members
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::constrains_element
                   == true);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::constrains_key
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::constrains_element
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::constrains_key
                   == true);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::constrains_element
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::constrains_key
                   == false);

    return ok;
}

/*
tests_mutable_container_class_agreement
  Verifies that the snapshot is a faithful mirror: every member equals the
  standalone trait it republishes, checked across carriers that occupy
  different cells of the model.
  Tests the following:
  - all seven signal members against their signals
  - all four capability members against their umbrellas
  - grade, grade_name, access, access_name and the two predicates against their
    traits
  - can_overwrite_element and is_element_mutable, which are two spellings of
    one capability, agree with each other
*/
bool tests_mutable_container_class_agreement()
{
    bool ok = true;

    // --- signals (vector: a mixed profile) ---
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_push_back
                   == has_push_back_signal<DMcVectorInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_push_front
                   == has_push_front_signal<DMcVectorInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_insert
                   == has_insert_signal<DMcVectorInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_erase
                   == has_erase_signal<DMcVectorInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_clear
                   == has_clear_signal<DMcVectorInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_resize
                   == has_resize_signal<DMcVectorInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_overwrite_element
                   == is_element_mutable_container<DMcVectorInt>::value);

    // --- signals (map: a different profile) ---
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::can_push_back
                   == has_push_back_signal<DMcMapIi>::value);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::can_push_front
                   == has_push_front_signal<DMcMapIi>::value);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::can_insert
                   == has_insert_signal<DMcMapIi>::value);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::can_erase
                   == has_erase_signal<DMcMapIi>::value);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::can_clear
                   == has_clear_signal<DMcMapIi>::value);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::can_resize
                   == has_resize_signal<DMcMapIi>::value);

    // --- capabilities ---
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::is_element_mutable
                   == is_element_mutable_container<DMcArray4>::value);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::is_structurally_mutable
                   == is_structurally_mutable_container<DMcArray4>::value);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::is_mutable
                   == is_mutable_container<DMcArray4>::value);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::is_immutable
                   == is_immutable_container<DMcArray4>::value);

    D_MC_CHECK(DMutableContainerClass<DMcSizedOnly>::is_element_mutable
                   == is_element_mutable_container<DMcSizedOnly>::value);
    D_MC_CHECK(DMutableContainerClass<DMcSizedOnly>::is_structurally_mutable
                   == is_structurally_mutable_container<DMcSizedOnly>::value);
    D_MC_CHECK(DMutableContainerClass<DMcSizedOnly>::is_mutable
                   == is_mutable_container<DMcSizedOnly>::value);
    D_MC_CHECK(DMutableContainerClass<DMcSizedOnly>::is_immutable
                   == is_immutable_container<DMcSizedOnly>::value);

    // --- grade and access ---
    D_MC_CHECK(DMutableContainerClass<DMcListInt>::grade
                   == mutability_of<DMcListInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::grade
                   == mutability_of<DMcSetInt>::value);
    D_MC_CHECK(std::strcmp(DMutableContainerClass<DMcListInt>::grade_name,
                           mutability_name(mutability_of<
                               DMcListInt>::value)) == 0);

    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::access
                   == access_restriction_of<DMcSetInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::access
                   == access_restriction_of<DMcMapIi>::value);
    D_MC_CHECK(std::strcmp(
                   DMutableContainerClass<DMcSetInt>::access_name,
                   access_restriction_name(
                       access_restriction_of<DMcSetInt>::value)) == 0);

    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::constrains_element
                   == djinterp::constrains_element<DMcSetInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi>::constrains_key
                   == djinterp::constrains_key<DMcMapIi>::value);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::constrains_element
                   == djinterp::constrains_element<DMcVectorInt>::value);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::constrains_key
                   == djinterp::constrains_key<DMcVectorInt>::value);

    // --- the two spellings of the element capability ---
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt>::can_overwrite_element
                   == DMutableContainerClass<
                          DMcVectorInt>::is_element_mutable);
    D_MC_CHECK(DMutableContainerClass<DMcSetInt>::can_overwrite_element
                   == DMutableContainerClass<DMcSetInt>::is_element_mutable);
    D_MC_CHECK(DMutableContainerClass<DMcArray4>::can_overwrite_element
                   == DMutableContainerClass<DMcArray4>::is_element_mutable);

    return ok;
}

/*
tests_mutable_container_class_cv_ref_invariance
  Verifies that the snapshot inherits the clean_t contract from every trait it
  mirrors.
  Tests the following:
  - the grade, the access restriction and the capability members are identical
    for the bare type, its const and volatile forms, and its lvalue and rvalue
    reference forms
  - checked on carriers from three different cells of the model
*/
bool tests_mutable_container_class_cv_ref_invariance()
{
    bool ok = true;

    // vector: fully mutable, no restriction
    D_MC_CHECK(DMutableContainerClass<const DMcVectorInt>::grade
                   == DMutableContainerClass<DMcVectorInt>::grade);
    D_MC_CHECK(DMutableContainerClass<volatile DMcVectorInt>::grade
                   == DMutableContainerClass<DMcVectorInt>::grade);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt&>::grade
                   == DMutableContainerClass<DMcVectorInt>::grade);
    D_MC_CHECK(DMutableContainerClass<const DMcVectorInt&>::grade
                   == DMutableContainerClass<DMcVectorInt>::grade);
    D_MC_CHECK(DMutableContainerClass<DMcVectorInt&&>::grade
                   == DMutableContainerClass<DMcVectorInt>::grade);
    D_MC_CHECK(DMutableContainerClass<const DMcVectorInt>::is_mutable
                   == DMutableContainerClass<DMcVectorInt>::is_mutable);
    D_MC_CHECK(DMutableContainerClass<const DMcVectorInt&>::can_push_back
                   == DMutableContainerClass<DMcVectorInt>::can_push_back);

    // map: structural, key frozen
    D_MC_CHECK(DMutableContainerClass<const DMcMapIi&>::access
                   == DMutableContainerClass<DMcMapIi>::access);
    D_MC_CHECK(DMutableContainerClass<DMcMapIi&&>::access
                   == DMutableContainerClass<DMcMapIi>::access);
    D_MC_CHECK(DMutableContainerClass<const DMcMapIi>::constrains_key
                   == DMutableContainerClass<DMcMapIi>::constrains_key);

    // set: structural, element frozen
    D_MC_CHECK(DMutableContainerClass<const DMcSetInt&>::grade
                   == DMutableContainerClass<DMcSetInt>::grade);
    D_MC_CHECK(
        DMutableContainerClass<const DMcSetInt&>::constrains_element
            == DMutableContainerClass<DMcSetInt>::constrains_element);
    D_MC_CHECK(DMutableContainerClass<volatile DMcSetInt>::is_immutable
                   == DMutableContainerClass<DMcSetInt>::is_immutable);

    return ok;
}

/*
tests_mutable_container_class_edge_types
  Verifies that the snapshot instantiates and answers sensibly for types that
  are not containers at all - the case an aggregate is most likely to break on,
  since it touches every trait in the header at once.
  Tests the following:
  - void instantiates: no probe forms a reference to it in a way that escapes
    the immediate context, so every member reads as the empty answer
  - scalars, pointers, raw arrays, pair and tuple all read immutable and
    unrestricted, and none reads as an immutable CONTAINER
  - a zero-extent array is element_mutable, exactly as a positive-extent one is
  - the near-miss probes report no capability, so the snapshot does not
    manufacture one where the individual traits found none
*/
bool tests_mutable_container_class_edge_types()
{
    bool ok = true;

    // void: the whole aggregate must instantiate
    D_MC_CHECK(DMutableContainerClass<void>::grade  == mutability::immutable);
    D_MC_CHECK(DMutableContainerClass<void>::access
                   == access_restriction::none);
    D_MC_CHECK(DMutableContainerClass<void>::is_mutable   == false);
    D_MC_CHECK(DMutableContainerClass<void>::is_immutable == false);
    D_MC_CHECK(std::strcmp(DMutableContainerClass<void>::grade_name,
                           "immutable") == 0);
    D_MC_CHECK(std::strcmp(DMutableContainerClass<void>::access_name,
                           "none") == 0);

    // scalars, pointers, raw arrays
    D_MC_CHECK(DMutableContainerClass<int>::grade == mutability::immutable);
    D_MC_CHECK(DMutableContainerClass<DMcIntPtr>::grade
                   == mutability::immutable);
    D_MC_CHECK(DMutableContainerClass<DMcIntArray>::grade
                   == mutability::immutable);
    D_MC_CHECK(DMutableContainerClass<DMcIntArray>::is_immutable == false);

    // pair and tuple carry no size(), so no container-hood
    D_MC_CHECK(DMutableContainerClass<DMcPairIi>::grade
                   == mutability::immutable);
    D_MC_CHECK(DMutableContainerClass<DMcPairIi>::is_immutable == false);
    D_MC_CHECK(DMutableContainerClass<DMcPairIi>::access
                   == access_restriction::none);
    D_MC_CHECK(DMutableContainerClass<DMcTupleI>::grade
                   == mutability::immutable);
    D_MC_CHECK(DMutableContainerClass<DMcTupleI>::is_immutable == false);

    // a zero-extent array behaves as a positive-extent one
    D_MC_CHECK(DMutableContainerClass<DMcArray0>::grade
                   == DMutableContainerClass<DMcArray4>::grade);
    D_MC_CHECK(DMutableContainerClass<DMcArray0>::can_overwrite_element
                   == true);

    // near misses manufacture nothing
    D_MC_CHECK(DMutableContainerClass<DMcConstSubscriptOnly>::is_mutable
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcConstDataOnly>::is_mutable
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcVoidData>::is_mutable    == false);
    D_MC_CHECK(DMutableContainerClass<DMcEraseNoBegin>::is_mutable
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcInsertTwoArg>::is_mutable
                   == false);
    D_MC_CHECK(DMutableContainerClass<DMcSubscriptWrongValue>::is_mutable
                   == false);

    // an observer that looks like a container
    D_MC_CHECK(DMutableContainerClass<DMcImmutableSized>::is_immutable
                   == true);
    D_MC_CHECK(DMutableContainerClass<DMcImmutableSized>::grade
                   == mutability::immutable);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
