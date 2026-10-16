/*******************************************************************************
* djinterp [testing]                     ordered_container_tests_classifiers.cpp
*
*   Section TU: the order-axis verdict this file re-exports.  Its point is the
* correction called out in the module preamble - positional identity excludes
* ALL associatives, keyed containers are unordered - so the associative case is
* checked exhaustively across the ordered and the hash-ordered families.
*
*
* path:      /tests/djinterp/core/container/ordered_container_tests_classifiers.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "ordered_container_tests.hpp"   // normal mode


NS_DJINTERP
NS_TESTING

// tests_ordered_sequences_are_ordered
//   every positional sequence carries an order.
bool
tests_ordered_sequences_are_ordered()
{
    D_OR_CHECK(is_ordered_container<std::vector<int>>::value);
    D_OR_CHECK(is_ordered_container<std::deque<int>>::value);
    D_OR_CHECK((is_ordered_container<std::array<int, 4>>::value));
    D_OR_CHECK(is_ordered_container<std::list<int>>::value);
    D_OR_CHECK(is_ordered_container<std::forward_list<int>>::value);
    D_OR_CHECK(is_ordered_container<std::string>::value);

    // cv / reference qualifiers are cleaned before the classifier runs.
    D_OR_CHECK(is_ordered_container<const std::vector<int>>::value);
    D_OR_CHECK(is_ordered_container<std::vector<int>&>::value);

    return true;
}

// tests_ordered_associatives_are_unordered
//   the headline: no associative has positional identity - not the ordered
// ones, and not the hash-ordered ones.  (The former sequential verdict caught
// only the hash-ordered set; this one excludes every associative.)
bool
tests_ordered_associatives_are_unordered()
{
    // ordered (comparator-keyed) associatives: unordered on the ORDER axis.
    D_OR_CHECK(!is_ordered_container<std::set<int>>::value);
    D_OR_CHECK(!is_ordered_container<std::multiset<int>>::value);
    D_OR_CHECK((!is_ordered_container<std::map<int, int>>::value));
    D_OR_CHECK((!is_ordered_container<std::multimap<int, int>>::value));

    // hash-ordered associatives: likewise unordered.
    D_OR_CHECK(!is_ordered_container<std::unordered_set<int>>::value);
    D_OR_CHECK(!is_ordered_container<std::unordered_multiset<int>>::value);
    D_OR_CHECK((!is_ordered_container<std::unordered_map<int, int>>::value));
    D_OR_CHECK(
        (!is_ordered_container<std::unordered_multimap<int, int>>::value));

    return true;
}

// tests_ordered_non_containers_are_unordered
//   a scalar or a pointer is not an iterable container, so it is not ordered.
bool
tests_ordered_non_containers_are_unordered()
{
    D_OR_CHECK(!is_ordered_container<int>::value);
    D_OR_CHECK(!is_ordered_container<double>::value);
    D_OR_CHECK(!is_ordered_container<int*>::value);

    return true;
}

// tests_ordered_unordered_predicate
//   is_unordered_container is the complementary reading over containers: true
// for the keyed families, false for the positional sequences.
bool
tests_ordered_unordered_predicate()
{
    D_OR_CHECK(is_unordered_container<std::set<int>>::value);
    D_OR_CHECK((is_unordered_container<std::map<int, int>>::value));
    D_OR_CHECK(is_unordered_container<std::unordered_set<int>>::value);
    D_OR_CHECK((is_unordered_container<std::unordered_map<int, int>>::value));

    D_OR_CHECK(!is_unordered_container<std::vector<int>>::value);
    D_OR_CHECK(!is_unordered_container<std::list<int>>::value);
    D_OR_CHECK(!is_unordered_container<std::string>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
