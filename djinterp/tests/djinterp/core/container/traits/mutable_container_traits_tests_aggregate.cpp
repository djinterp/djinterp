/*******************************************************************************
* djinterp [core]                   mutable_container_traits_tests_aggregate.cpp
*
*
* path:      /tests/djinterp/core/container/traits/mutable_container_traits_tests_aggregate.cpp
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
//   mutable_container_class republishes, as one flat set of members, everything
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
bool
tests_mutable_container_class_signals()
{
    bool ok = true;

    // a growable contiguous sequence
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_push_back  == true);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_push_front == false);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_insert     == false);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_erase      == true);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_clear      == true);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_resize     == true);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_overwrite_element
                   == true);

    // a double-ended sequence adds the front
    D_MC_CHECK(mutable_container_class<mc_deque_int>::can_push_front == true);

    // an equivalence discipline
    D_MC_CHECK(mutable_container_class<mc_set_int>::can_push_back  == false);
    D_MC_CHECK(mutable_container_class<mc_set_int>::can_push_front == false);
    D_MC_CHECK(mutable_container_class<mc_set_int>::can_insert     == true);
    D_MC_CHECK(mutable_container_class<mc_set_int>::can_erase      == true);
    D_MC_CHECK(mutable_container_class<mc_set_int>::can_clear      == true);
    D_MC_CHECK(mutable_container_class<mc_set_int>::can_resize     == false);
    D_MC_CHECK(mutable_container_class<mc_set_int>::can_overwrite_element
                   == false);

    // a fixed extent lights nothing structural
    D_MC_CHECK(mutable_container_class<mc_array_4>::can_push_back  == false);
    D_MC_CHECK(mutable_container_class<mc_array_4>::can_push_front == false);
    D_MC_CHECK(mutable_container_class<mc_array_4>::can_insert     == false);
    D_MC_CHECK(mutable_container_class<mc_array_4>::can_erase      == false);
    D_MC_CHECK(mutable_container_class<mc_array_4>::can_clear      == false);
    D_MC_CHECK(mutable_container_class<mc_array_4>::can_resize     == false);
    D_MC_CHECK(mutable_container_class<mc_array_4>::can_overwrite_element
                   == true);

    // every signal at once, and none at all
    D_MC_CHECK(mutable_container_class<mc_all_signals>::can_push_back  == true);
    D_MC_CHECK(mutable_container_class<mc_all_signals>::can_push_front == true);
    D_MC_CHECK(mutable_container_class<mc_all_signals>::can_insert     == true);
    D_MC_CHECK(mutable_container_class<mc_all_signals>::can_erase      == true);
    D_MC_CHECK(mutable_container_class<mc_all_signals>::can_clear      == true);
    D_MC_CHECK(mutable_container_class<mc_all_signals>::can_resize     == true);

    D_MC_CHECK(mutable_container_class<mc_bare>::can_push_back  == false);
    D_MC_CHECK(mutable_container_class<mc_bare>::can_push_front == false);
    D_MC_CHECK(mutable_container_class<mc_bare>::can_insert     == false);
    D_MC_CHECK(mutable_container_class<mc_bare>::can_erase      == false);
    D_MC_CHECK(mutable_container_class<mc_bare>::can_clear      == false);
    D_MC_CHECK(mutable_container_class<mc_bare>::can_resize     == false);
    D_MC_CHECK(mutable_container_class<mc_bare>::can_overwrite_element
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
bool
tests_mutable_container_class_capabilities()
{
    bool ok = true;

    // both capabilities
    D_MC_CHECK(mutable_container_class<mc_vector_int>::is_element_mutable
                   == true);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::is_structurally_mutable
                   == true);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::is_mutable   == true);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::is_immutable == false);

    // structural only
    D_MC_CHECK(mutable_container_class<mc_set_int>::is_element_mutable
                   == false);
    D_MC_CHECK(mutable_container_class<mc_set_int>::is_structurally_mutable
                   == true);
    D_MC_CHECK(mutable_container_class<mc_set_int>::is_mutable   == true);
    D_MC_CHECK(mutable_container_class<mc_set_int>::is_immutable == false);

    // element only
    D_MC_CHECK(mutable_container_class<mc_array_4>::is_element_mutable
                   == true);
    D_MC_CHECK(mutable_container_class<mc_array_4>::is_structurally_mutable
                   == false);
    D_MC_CHECK(mutable_container_class<mc_array_4>::is_mutable   == true);
    D_MC_CHECK(mutable_container_class<mc_array_4>::is_immutable == false);

    // neither, and a container
    D_MC_CHECK(mutable_container_class<mc_sized_only>::is_element_mutable
                   == false);
    D_MC_CHECK(mutable_container_class<mc_sized_only>::is_structurally_mutable
                   == false);
    D_MC_CHECK(mutable_container_class<mc_sized_only>::is_mutable   == false);
    D_MC_CHECK(mutable_container_class<mc_sized_only>::is_immutable == true);

    // neither, and NOT a container - is_mutable and is_immutable are both
    // false, so they are not complements of one another
    D_MC_CHECK(mutable_container_class<mc_bare>::is_mutable   == false);
    D_MC_CHECK(mutable_container_class<mc_bare>::is_immutable == false);
    D_MC_CHECK(mutable_container_class<int>::is_mutable       == false);
    D_MC_CHECK(mutable_container_class<int>::is_immutable     == false);

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
bool
tests_mutable_container_class_grade()
{
    bool ok = true;

    // the four corners
    D_MC_CHECK(mutable_container_class<mc_vector_int>::grade
                   == mutability::fully_mutable);
    D_MC_CHECK(mutable_container_class<mc_set_int>::grade
                   == mutability::structural_mutable);
    D_MC_CHECK(mutable_container_class<mc_list_int>::grade
                   == mutability::structural_mutable);
    D_MC_CHECK(mutable_container_class<mc_array_4>::grade
                   == mutability::element_mutable);
    D_MC_CHECK(mutable_container_class<mc_sized_only>::grade
                   == mutability::immutable);

    // grade_name spells that grade
    D_MC_CHECK(std::strcmp(mutable_container_class<mc_vector_int>::grade_name,
                           "fully_mutable") == 0);
    D_MC_CHECK(std::strcmp(mutable_container_class<mc_set_int>::grade_name,
                           "structural_mutable") == 0);
    D_MC_CHECK(std::strcmp(mutable_container_class<mc_array_4>::grade_name,
                           "element_mutable") == 0);
    D_MC_CHECK(std::strcmp(mutable_container_class<mc_sized_only>::grade_name,
                           "immutable") == 0);

    // the two members agree by construction
    D_MC_CHECK(std::strcmp(
                   mutable_container_class<mc_vector_int>::grade_name,
                   mutability_name(
                       mutable_container_class<mc_vector_int>::grade)) == 0);
    D_MC_CHECK(std::strcmp(
                   mutable_container_class<mc_map_ii>::grade_name,
                   mutability_name(
                       mutable_container_class<mc_map_ii>::grade)) == 0);

    // and the grade behaves as a grade
    D_MC_CHECK(is_fully_mutable_grade(
                   mutable_container_class<mc_vector_int>::grade) == true);
    D_MC_CHECK(grade_admits_structural_mutation(
                   mutable_container_class<mc_set_int>::grade) == true);
    D_MC_CHECK(grade_admits_element_mutation(
                   mutable_container_class<mc_set_int>::grade) == false);
    D_MC_CHECK(is_immutable_grade(
                   mutable_container_class<mc_sized_only>::grade) == true);

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
bool
tests_mutable_container_class_access()
{
    bool ok = true;

    // the three restrictions
    D_MC_CHECK(mutable_container_class<mc_vector_int>::access
                   == access_restriction::none);
    D_MC_CHECK(mutable_container_class<mc_set_int>::access
                   == access_restriction::element_const);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::access
                   == access_restriction::key_const);

    // access_name spells that restriction
    D_MC_CHECK(std::strcmp(mutable_container_class<mc_vector_int>::access_name,
                           "none") == 0);
    D_MC_CHECK(std::strcmp(mutable_container_class<mc_set_int>::access_name,
                           "element_const") == 0);
    D_MC_CHECK(std::strcmp(mutable_container_class<mc_map_ii>::access_name,
                           "key_const") == 0);

    // the two members agree by construction
    D_MC_CHECK(std::strcmp(
                   mutable_container_class<mc_map_ii>::access_name,
                   access_restriction_name(
                       mutable_container_class<mc_map_ii>::access)) == 0);

    // the shadowing predicate members
    D_MC_CHECK(mutable_container_class<mc_set_int>::constrains_element
                   == true);
    D_MC_CHECK(mutable_container_class<mc_set_int>::constrains_key
                   == false);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::constrains_element
                   == false);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::constrains_key
                   == true);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::constrains_element
                   == false);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::constrains_key
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
bool
tests_mutable_container_class_agreement()
{
    bool ok = true;

    // --- signals (vector: a mixed profile) ---
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_push_back
                   == has_push_back_signal<mc_vector_int>::value);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_push_front
                   == has_push_front_signal<mc_vector_int>::value);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_insert
                   == has_insert_signal<mc_vector_int>::value);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_erase
                   == has_erase_signal<mc_vector_int>::value);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_clear
                   == has_clear_signal<mc_vector_int>::value);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_resize
                   == has_resize_signal<mc_vector_int>::value);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_overwrite_element
                   == is_element_mutable_container<mc_vector_int>::value);

    // --- signals (map: a different profile) ---
    D_MC_CHECK(mutable_container_class<mc_map_ii>::can_push_back
                   == has_push_back_signal<mc_map_ii>::value);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::can_push_front
                   == has_push_front_signal<mc_map_ii>::value);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::can_insert
                   == has_insert_signal<mc_map_ii>::value);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::can_erase
                   == has_erase_signal<mc_map_ii>::value);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::can_clear
                   == has_clear_signal<mc_map_ii>::value);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::can_resize
                   == has_resize_signal<mc_map_ii>::value);

    // --- capabilities ---
    D_MC_CHECK(mutable_container_class<mc_array_4>::is_element_mutable
                   == is_element_mutable_container<mc_array_4>::value);
    D_MC_CHECK(mutable_container_class<mc_array_4>::is_structurally_mutable
                   == is_structurally_mutable_container<mc_array_4>::value);
    D_MC_CHECK(mutable_container_class<mc_array_4>::is_mutable
                   == is_mutable_container<mc_array_4>::value);
    D_MC_CHECK(mutable_container_class<mc_array_4>::is_immutable
                   == is_immutable_container<mc_array_4>::value);

    D_MC_CHECK(mutable_container_class<mc_sized_only>::is_element_mutable
                   == is_element_mutable_container<mc_sized_only>::value);
    D_MC_CHECK(mutable_container_class<mc_sized_only>::is_structurally_mutable
                   == is_structurally_mutable_container<mc_sized_only>::value);
    D_MC_CHECK(mutable_container_class<mc_sized_only>::is_mutable
                   == is_mutable_container<mc_sized_only>::value);
    D_MC_CHECK(mutable_container_class<mc_sized_only>::is_immutable
                   == is_immutable_container<mc_sized_only>::value);

    // --- grade and access ---
    D_MC_CHECK(mutable_container_class<mc_list_int>::grade
                   == mutability_of<mc_list_int>::value);
    D_MC_CHECK(mutable_container_class<mc_set_int>::grade
                   == mutability_of<mc_set_int>::value);
    D_MC_CHECK(std::strcmp(mutable_container_class<mc_list_int>::grade_name,
                           mutability_name(mutability_of<
                               mc_list_int>::value)) == 0);

    D_MC_CHECK(mutable_container_class<mc_set_int>::access
                   == access_restriction_of<mc_set_int>::value);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::access
                   == access_restriction_of<mc_map_ii>::value);
    D_MC_CHECK(std::strcmp(
                   mutable_container_class<mc_set_int>::access_name,
                   access_restriction_name(
                       access_restriction_of<mc_set_int>::value)) == 0);

    D_MC_CHECK(mutable_container_class<mc_set_int>::constrains_element
                   == djinterp::constrains_element<mc_set_int>::value);
    D_MC_CHECK(mutable_container_class<mc_map_ii>::constrains_key
                   == djinterp::constrains_key<mc_map_ii>::value);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::constrains_element
                   == djinterp::constrains_element<mc_vector_int>::value);
    D_MC_CHECK(mutable_container_class<mc_vector_int>::constrains_key
                   == djinterp::constrains_key<mc_vector_int>::value);

    // --- the two spellings of the element capability ---
    D_MC_CHECK(mutable_container_class<mc_vector_int>::can_overwrite_element
                   == mutable_container_class<
                          mc_vector_int>::is_element_mutable);
    D_MC_CHECK(mutable_container_class<mc_set_int>::can_overwrite_element
                   == mutable_container_class<mc_set_int>::is_element_mutable);
    D_MC_CHECK(mutable_container_class<mc_array_4>::can_overwrite_element
                   == mutable_container_class<mc_array_4>::is_element_mutable);

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
bool
tests_mutable_container_class_cv_ref_invariance()
{
    bool ok = true;

    // vector: fully mutable, no restriction
    D_MC_CHECK(mutable_container_class<const mc_vector_int>::grade
                   == mutable_container_class<mc_vector_int>::grade);
    D_MC_CHECK(mutable_container_class<volatile mc_vector_int>::grade
                   == mutable_container_class<mc_vector_int>::grade);
    D_MC_CHECK(mutable_container_class<mc_vector_int&>::grade
                   == mutable_container_class<mc_vector_int>::grade);
    D_MC_CHECK(mutable_container_class<const mc_vector_int&>::grade
                   == mutable_container_class<mc_vector_int>::grade);
    D_MC_CHECK(mutable_container_class<mc_vector_int&&>::grade
                   == mutable_container_class<mc_vector_int>::grade);
    D_MC_CHECK(mutable_container_class<const mc_vector_int>::is_mutable
                   == mutable_container_class<mc_vector_int>::is_mutable);
    D_MC_CHECK(mutable_container_class<const mc_vector_int&>::can_push_back
                   == mutable_container_class<mc_vector_int>::can_push_back);

    // map: structural, key frozen
    D_MC_CHECK(mutable_container_class<const mc_map_ii&>::access
                   == mutable_container_class<mc_map_ii>::access);
    D_MC_CHECK(mutable_container_class<mc_map_ii&&>::access
                   == mutable_container_class<mc_map_ii>::access);
    D_MC_CHECK(mutable_container_class<const mc_map_ii>::constrains_key
                   == mutable_container_class<mc_map_ii>::constrains_key);

    // set: structural, element frozen
    D_MC_CHECK(mutable_container_class<const mc_set_int&>::grade
                   == mutable_container_class<mc_set_int>::grade);
    D_MC_CHECK(
        mutable_container_class<const mc_set_int&>::constrains_element
            == mutable_container_class<mc_set_int>::constrains_element);
    D_MC_CHECK(mutable_container_class<volatile mc_set_int>::is_immutable
                   == mutable_container_class<mc_set_int>::is_immutable);

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
bool
tests_mutable_container_class_edge_types()
{
    bool ok = true;

    // void: the whole aggregate must instantiate
    D_MC_CHECK(mutable_container_class<void>::grade  == mutability::immutable);
    D_MC_CHECK(mutable_container_class<void>::access
                   == access_restriction::none);
    D_MC_CHECK(mutable_container_class<void>::is_mutable   == false);
    D_MC_CHECK(mutable_container_class<void>::is_immutable == false);
    D_MC_CHECK(std::strcmp(mutable_container_class<void>::grade_name,
                           "immutable") == 0);
    D_MC_CHECK(std::strcmp(mutable_container_class<void>::access_name,
                           "none") == 0);

    // scalars, pointers, raw arrays
    D_MC_CHECK(mutable_container_class<int>::grade == mutability::immutable);
    D_MC_CHECK(mutable_container_class<mc_int_ptr>::grade
                   == mutability::immutable);
    D_MC_CHECK(mutable_container_class<mc_int_array>::grade
                   == mutability::immutable);
    D_MC_CHECK(mutable_container_class<mc_int_array>::is_immutable == false);

    // pair and tuple carry no size(), so no container-hood
    D_MC_CHECK(mutable_container_class<mc_pair_ii>::grade
                   == mutability::immutable);
    D_MC_CHECK(mutable_container_class<mc_pair_ii>::is_immutable == false);
    D_MC_CHECK(mutable_container_class<mc_pair_ii>::access
                   == access_restriction::none);
    D_MC_CHECK(mutable_container_class<mc_tuple_i>::grade
                   == mutability::immutable);
    D_MC_CHECK(mutable_container_class<mc_tuple_i>::is_immutable == false);

    // a zero-extent array behaves as a positive-extent one
    D_MC_CHECK(mutable_container_class<mc_array_0>::grade
                   == mutable_container_class<mc_array_4>::grade);
    D_MC_CHECK(mutable_container_class<mc_array_0>::can_overwrite_element
                   == true);

    // near misses manufacture nothing
    D_MC_CHECK(mutable_container_class<mc_const_subscript_only>::is_mutable
                   == false);
    D_MC_CHECK(mutable_container_class<mc_const_data_only>::is_mutable
                   == false);
    D_MC_CHECK(mutable_container_class<mc_void_data>::is_mutable    == false);
    D_MC_CHECK(mutable_container_class<mc_erase_no_begin>::is_mutable
                   == false);
    D_MC_CHECK(mutable_container_class<mc_insert_two_arg>::is_mutable
                   == false);
    D_MC_CHECK(mutable_container_class<mc_subscript_wrong_value>::is_mutable
                   == false);

    // an observer that looks like a container
    D_MC_CHECK(mutable_container_class<mc_immutable_sized>::is_immutable
                   == true);
    D_MC_CHECK(mutable_container_class<mc_immutable_sized>::grade
                   == mutability::immutable);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
