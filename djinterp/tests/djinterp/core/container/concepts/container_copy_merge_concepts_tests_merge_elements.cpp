#include "container_copy_merge_concepts_tests.hpp"


#if D_CM_CONCEPTS_ENABLED

NS_DJINTERP
NS_TESTING

/*
tests_ccmc_elements_same_type
  containers sharing a value_type are element-compatible.
  Tests the following:
  - a container with itself, across all six disciplines
  - containers of DIFFERING disciplines but the same element - vector<int> with
    set<int>, set<int> with multiset<int> - are compatible, since the element
    clause is read independently of the discipline
  - map with multimap, whose shared value_type is pair<const int, int>
  - std::string with vector<char>, whose shared element is char
  The discipline-crossing cases matter: they are the ones that separate this
  concept from mergeable_with, which refuses exactly them.
*/
bool
tests_ccmc_elements_same_type()
{
    // the diagonal, across the six disciplines
    D_CM_CHECK(( dt::holds_for_all<cm_elements_self_c,
                                   std::vector<int>,
                                   std::set<int>,
                                   std::multiset<int>,
                                   std::map<int, int>,
                                   std::multimap<int, int>,
                                   std::array<int, 4>,
                                   std::string>::value ));

    // same element, different discipline - compatible here, and NOT mergeable
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                            std::set<int>> ));
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::set<int>,
                                                            std::multiset<int>> ));
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<
                     std::map<int, int>,
                     std::multimap<int, int>> ));

    // same element, different container template
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::string,
                                                            std::vector<char>> ));
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                            std::array<int, 4>> ));

    return true;
}

/*
tests_ccmc_elements_convertible
  the element clause admits a conversion in EITHER direction.
  Tests the following:
  - vector<int> with vector<long>, whose elements convert both ways
  - vector<int> with vector<cm_from_int>, whose elements convert one way only
    (int to cm_from_int, never back)
  - the one-way case in BOTH operand positions, since the clause ORs the two
    directions and neither position is privileged
  - the underlying element traits, so the test confirms WHICH disjunct carried
    each case rather than only that some disjunct did
*/
bool
tests_ccmc_elements_convertible()
{
    // mutually convertible elements
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                            std::vector<long>> ));
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::vector<long>,
                                                            std::vector<int>> ));

    // one-way convertible: int -> cm_from_int only
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<
                     std::vector<int>,
                     std::vector<cm_from_int>> ));
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<
                     std::vector<cm_from_int>,
                     std::vector<int>> ));

    // confirm which disjunct did the work in the one-way case
    D_CM_CHECK(( !::djinterp::elements_same_type<
                     std::vector<int>,
                     std::vector<cm_from_int>>::value ));
    D_CM_CHECK(( ::djinterp::elements_convertible<
                     std::vector<int>,
                     std::vector<cm_from_int>>::value ));
    D_CM_CHECK(( !::djinterp::elements_convertible<
                     std::vector<cm_from_int>,
                     std::vector<int>>::value ));

    return true;
}

/*
tests_ccmc_elements_incompatible
  elements that convert in neither direction refute the concept.
  Tests the following:
  - vector<int> with vector<cm_opaque>, a class convertible neither to nor from
    int, in both operand positions
  - vector<int> with vector<std::string>, the same refusal for a std type
  - set<int> with set<cm_opaque>, so the refusal does not depend on the
    discipline being a sequence
  - the container clause holds in every one of these, so the refusal is
    attributable to the element clause alone
*/
bool
tests_ccmc_elements_incompatible()
{
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     std::vector<int>,
                     std::vector<cm_opaque>> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     std::vector<cm_opaque>,
                     std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     std::vector<int>,
                     std::vector<std::string>> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     std::set<int>,
                     std::set<cm_opaque>> ));

    // both operands ARE containers - the refusal is the element clause's
    D_CM_CHECK(::djinterp::is_iterable_container<std::vector<int>>::value);
    D_CM_CHECK(::djinterp::is_iterable_container<std::vector<cm_opaque>>::value);

    return true;
}

/*
tests_ccmc_elements_container_guard
  the is_iterable_container guard is load-bearing, not decoration.
  Tests the following:
  - int with int is refuted, and would NOT be without the guard: neither
    operand has a value_type, so both element types degrade to void, and
    is_same<void, void> satisfies the element clause on its own
  - the same for cm_opaque, and for cm_positional, which has a value_type and
    no traversal
  - the element clause is confirmed to be TRUE for these pairs in isolation, so
    the test demonstrates the guard is what refutes them rather than asserting
    a refusal and leaving the reason unexamined
  This is the interaction that makes the guard's narrowness matter: member
  begin()/end() detection is what keeps C arrays, function types and every
  other elementless shape out of the compatible set.
*/
bool
tests_ccmc_elements_container_guard()
{
    // refuted overall
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<int, int> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<cm_opaque,
                                                             cm_opaque> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<cm_positional,
                                                             cm_positional> ));

    // ...yet the element clause alone is SATISFIED, both element types being
    // void - the guard is the whole of the refusal
    D_CM_CHECK(( ::djinterp::elements_same_type<int, int>::value ));
    D_CM_CHECK(( ::djinterp::elements_same_type<cm_opaque, cm_opaque>::value ));
    D_CM_CHECK(( std::is_same<::djinterp::element_type_of_t<int>,
                              void>::value ));

    // and the guard is what fails
    D_CM_CHECK(!::djinterp::is_iterable_container<int>::value);
    D_CM_CHECK(!::djinterp::is_iterable_container<cm_positional>::value);

    // a value_type without a traversal is still not a container here
    D_CM_CHECK(( std::is_same<::djinterp::element_type_of_t<cm_positional>,
                              int>::value ));

    return true;
}

/*
tests_ccmc_elements_void_element
  FINDING F3, pinned: two iterable ranges with no value_type are mutually
  element-compatible.
  Tests the following:
  - cm_elementless_range passes the container guard (it has begin()/end())
  - its element type is void, there being no value_type to read
  - the pair is therefore element-compatible, because is_same<void, void> holds
  - the gap is CONTAINED: an elementless range is compatible with no container
    that does have elements, in either operand position, because
    is_convertible is false between void and any non-void type
  Recorded rather than worked around. It is a curiosity because the containment
  holds; it would be a hazard if it did not.
*/
bool
tests_ccmc_elements_void_element()
{
    // it is a container, and it has no element type
    D_CM_CHECK(::djinterp::is_iterable_container<cm_elementless_range>::value);
    D_CM_CHECK(( std::is_same<
                     ::djinterp::element_type_of_t<cm_elementless_range>,
                     void>::value ));

    // so two of them are "compatible" - the finding
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<
                     cm_elementless_range,
                     cm_elementless_range> ));

    // and the containment: nothing with real elements is compatible with it
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     cm_elementless_range,
                     std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     std::vector<int>,
                     cm_elementless_range> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     cm_elementless_range,
                     cm_plain_seq> ));

    D_CM_NOTE("F3: two elementless iterable ranges are mutually "
              "element-compatible (both element types degrade to void, and "
              "is_same<void,void> holds); the gap is contained, since void is "
              "convertible to and from nothing else");

    return true;
}

/*
tests_ccmc_elements_symmetry
  the concept is symmetric in its operands.
  Tests the following:
  - swapping _From and _To never changes the answer, across the compatible,
    the one-way-convertible, the incompatible, the non-container and the
    elementless cases
  The element clause ORs both conversion directions and the container guard
  applies to both operands, so symmetry is structural - which is what makes
  this concept's _From / _To naming harmless, in contrast to
  merge_may_overflow_into (finding F1), whose operand order is load-bearing.
*/
bool
tests_ccmc_elements_symmetry()
{
    D_CM_CHECK((    ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                               std::set<int>>
                 == ::djinterp::merge_elements_compatible_with<std::set<int>,
                                                               std::vector<int>> ));

    D_CM_CHECK((    ::djinterp::merge_elements_compatible_with<
                        std::vector<int>,
                        std::vector<cm_from_int>>
                 == ::djinterp::merge_elements_compatible_with<
                        std::vector<cm_from_int>,
                        std::vector<int>> ));

    D_CM_CHECK((    ::djinterp::merge_elements_compatible_with<
                        std::vector<int>,
                        std::vector<cm_opaque>>
                 == ::djinterp::merge_elements_compatible_with<
                        std::vector<cm_opaque>,
                        std::vector<int>> ));

    D_CM_CHECK((    ::djinterp::merge_elements_compatible_with<int,
                                                               std::vector<int>>
                 == ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                               int> ));

    D_CM_CHECK((    ::djinterp::merge_elements_compatible_with<
                        cm_elementless_range,
                        std::vector<int>>
                 == ::djinterp::merge_elements_compatible_with<
                        std::vector<int>,
                        cm_elementless_range> ));

    return true;
}

/*
tests_ccmc_elements_cvref
  the concept answers identically under every cv-ref qualification.
  Tests the following:
  - all eight forms of a satisfying pair agree and are true (all)
  - all eight forms of a refuting pair agree and are false (none), so the
    invariance is not vacuous
  - the qualification applied to ONE operand only, which the diagonal report
    cannot reach: const-lvalue-reference and rvalue-reference forms in each
    position separately
  The concept cleans both arguments up front, so this is a claim it makes
  structurally; the trait behind it holds only because its element delegates
  clean too - see finding F6.
*/
bool
tests_ccmc_elements_cvref()
{
    D_CONSTEXPR dt::cvref_report vec =
        dt::trait_across_cvref<cm_elements_self_c, std::vector<int>>();
    D_CONSTEXPR dt::cvref_report plain =
        dt::trait_across_cvref<cm_elements_self_c, int>();

    D_CM_CHECK(vec.agrees());
    D_CM_CHECK(vec.all());
    D_CM_CHECK(vec.first_disagreement() == nullptr);

    D_CM_CHECK(plain.agrees());
    D_CM_CHECK(plain.none());

    // one operand qualified at a time - the asymmetric forms the diagonal
    // report cannot express
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<
                     const std::vector<int>&,
                     std::vector<int>> ));
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<
                     std::vector<int>,
                     const std::vector<int>&> ));
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<
                     std::vector<int>&&,
                     const volatile std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     const std::vector<int>&,
                     std::vector<cm_opaque>&&> ));

    return true;
}

/*
tests_ccmc_elements_hostile
  the concept survives, and rejects, the full adversarial zoo.
  Tests the following:
  - holds_for_none over D_TEST_HOSTILE_TYPES paired with itself
  - count_holds is exactly zero, in the form that instantiates every cell, so a
    definition ill-formed for any hostile shape breaks the BUILD here
  - each half of the zoo separately, so a regression names its half
  Unlike the copy concept (finding F2) this battery runs over the FULL zoo,
  including the incomplete fixture: nothing on this path names a
  completeness-requiring trait, and every probe it does name - member
  begin()/end(), a nested value_type, std::is_convertible on void - is a
  substitution failure rather than a diagnostic for an incomplete type.
*/
bool
tests_ccmc_elements_hostile()
{
    D_CM_CHECK(( dt::holds_for_none<cm_elements_self_c,
                                    D_TEST_HOSTILE_TYPES>::value ));

    D_CM_CHECK(( dt::count_holds<cm_elements_self_c,
                                 D_TEST_HOSTILE_TYPES>::value == 0u ));

    D_CM_CHECK(( dt::holds_for_none<cm_elements_self_c,
                                    D_TEST_HOSTILE_CLASS_TYPES>::value ));
    D_CM_CHECK(( dt::holds_for_none<cm_elements_self_c,
                                    D_TEST_HOSTILE_NONCLASS_TYPES>::value ));

    // a hostile shape paired with a real container is refused too
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     dt::fixtures::incomplete,
                     std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     std::vector<int>,
                     dt::fixtures::array_type> ));
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     dt::fixtures::greedy,
                     std::vector<int>> ));

    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_CM_CONCEPTS_ENABLED
