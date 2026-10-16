/*******************************************************************************
* djinterp [testing]                       sequential_container_tests_traits.cpp
*
*   Section TU: the is_sequential_container verdict and the DSequentialKind /
* sequential_kind storage classifier, both defined in the header under test.
* Two results are pinned rather than "fixed" - the trait's TRUE verdict on the
* ordered associatives (see SUITE NOTE 1) and the list_like classification of
* std::deque (SUITE NOTE 2) - because they are the module's actual behaviour.
*
*
* path:      /tests/djinterp/core/container/sequential_container_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "sequential_container_tests.hpp"   // normal mode: header + macro


NS_DJINTERP
NS_TESTING

// tests_sequential_sequences_are_sequential
//   every positional sequence is iterable and has no hasher, so it qualifies.
bool
tests_sequential_sequences_are_sequential()
{
    D_SQ_CHECK(is_sequential_container<std::vector<int>>::value);
    D_SQ_CHECK((is_sequential_container<std::array<int, 4>>::value));
    D_SQ_CHECK(is_sequential_container<std::string>::value);
    D_SQ_CHECK(is_sequential_container<std::list<int>>::value);
    D_SQ_CHECK(is_sequential_container<std::forward_list<int>>::value);
    D_SQ_CHECK(is_sequential_container<std::deque<int>>::value);

    // cv / reference qualifiers are cleaned before the classifier runs.
    D_SQ_CHECK(is_sequential_container<const std::vector<int>>::value);
    D_SQ_CHECK(is_sequential_container<std::vector<int>&>::value);

    // the _v companion agrees with the ::value form.
    D_SQ_CHECK(is_sequential_container_v<std::vector<int>>);
    D_SQ_CHECK(!is_sequential_container_v<int>);

    return true;
}

// tests_sequential_ordered_associatives_pinned
//   SUITE NOTE 1.  std::set / std::multiset / std::map / std::multimap carry
//   no
// `hasher`, so this trait admits them - true here, though the module's own
// header note says associatives are excluded.  ordered_container.hpp is the
// corrected verdict; this test pins what sequential_container.hpp actually
// does.
bool
tests_sequential_ordered_associatives_pinned()
{
    D_SQ_CHECK(is_sequential_container<std::set<int>>::value);
    D_SQ_CHECK(is_sequential_container<std::multiset<int>>::value);
    D_SQ_CHECK((is_sequential_container<std::map<int, int>>::value));
    D_SQ_CHECK((is_sequential_container<std::multimap<int, int>>::value));

    return true;
}

// tests_sequential_hashed_associatives_excluded
//   the unordered_ associatives carry a `hasher`, so they are excluded - the
// one associative family this trait does catch.
bool
tests_sequential_hashed_associatives_excluded()
{
    D_SQ_CHECK(!is_sequential_container<std::unordered_set<int>>::value);
    D_SQ_CHECK(!is_sequential_container<std::unordered_multiset<int>>::value);
    D_SQ_CHECK(
        (!is_sequential_container<std::unordered_map<int, int>>::value));
    D_SQ_CHECK(
        (!is_sequential_container<std::unordered_multimap<int, int>>::value));

    return true;
}

// tests_sequential_non_containers_excluded
//   a scalar or a pointer is not an iterable container.
bool
tests_sequential_non_containers_excluded()
{
    D_SQ_CHECK(!is_sequential_container<int>::value);
    D_SQ_CHECK(!is_sequential_container<double>::value);
    D_SQ_CHECK(!is_sequential_container<int*>::value);

    return true;
}

// tests_sequential_kind_classification
//   sequential_kind maps each backing store to its DSequentialKind:
//   string_like
// (has c_str), array_like (data() + random access), list_like (bidirectional,
// no data), forward_list_like (forward only), and none for a scalar.
bool
tests_sequential_kind_classification()
{
    D_SQ_CHECK(sequential_kind_v<std::string>
               == DSequentialKind::string_like);

    D_SQ_CHECK(sequential_kind_v<std::vector<int>>
               == DSequentialKind::array_like);
    D_SQ_CHECK((sequential_kind_v<std::array<int, 4>>
                == DSequentialKind::array_like));

    D_SQ_CHECK(sequential_kind_v<std::list<int>>
               == DSequentialKind::list_like);

    D_SQ_CHECK(sequential_kind_v<std::forward_list<int>>
               == DSequentialKind::forward_list_like);

    D_SQ_CHECK(sequential_kind_v<int> == DSequentialKind::none);

    return true;
}

// tests_sequential_kind_deque_pinned
//   SUITE NOTE 2.  std::deque has no data() accessor, so it misses the
// array_like arm; being bidirectional it is then caught by the list_like arm
// before the deque_like arm is reached.  Under the standard iterator hierarchy
// the deque_like arm cannot be reached by any conforming container, so this
// test pins the real classification and names the dead arm.
bool
tests_sequential_kind_deque_pinned()
{
    D_SQ_CHECK(sequential_kind_v<std::deque<int>>
               == DSequentialKind::list_like);

    // corollary: it is emphatically not classified as the (unreachable)
    // deque_like, nor as string_like/none.
    D_SQ_CHECK(sequential_kind_v<std::deque<int>>
               != DSequentialKind::deque_like);
    D_SQ_CHECK(sequential_kind_v<std::deque<int>>
               != DSequentialKind::none);

    return true;
}

// tests_sequential_kind_v_matches
//   the sequential_kind_v alias is the value of sequential_kind<>::value.
bool
tests_sequential_kind_v_matches()
{
    D_SQ_CHECK(sequential_kind_v<std::vector<int>>
               == sequential_kind<std::vector<int>>::value);
    D_SQ_CHECK(sequential_kind_v<std::string>
               == sequential_kind<std::string>::value);
    D_SQ_CHECK(sequential_kind_v<std::list<int>>
               == sequential_kind<std::list<int>>::value);
    D_SQ_CHECK(sequential_kind_v<int>
               == sequential_kind<int>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
