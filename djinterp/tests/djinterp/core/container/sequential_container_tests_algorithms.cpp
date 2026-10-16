/*******************************************************************************
* djinterp [testing]                   sequential_container_tests_algorithms.cpp
*
*   Section TU: the free order algorithms.  Behaviourally identical to the
* ordered suite (empty / partial / exact / mismatch / over-long, then
* std::string).  The gating block is where the two axes diverge: the
* is_sequential_container guard admits ordered associatives as well as
* sequences, and rejects only the hash-ordered ones and non-containers.
*
*
* path:      /tests/djinterp/core/container/sequential_container_tests_algorithms.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "sequential_container_tests.hpp"   // normal mode: header + macro


NS_DJINTERP
NS_TESTING

// tests_sequential_starts_with
//   an empty, partial, and full prefix match; a mismatch and an over-long
// prefix do not.
bool
tests_sequential_starts_with()
{
    std::vector<int> _c{1, 2, 3, 4};
    std::vector<int> _empty;

    D_SQ_CHECK(starts_with(_c, _empty));
    D_SQ_CHECK(starts_with(_c, std::vector<int>{1, 2}));
    D_SQ_CHECK(starts_with(_c, std::vector<int>{1, 2, 3, 4}));
    D_SQ_CHECK(!starts_with(_c, std::vector<int>{1, 3}));

    D_SQ_CHECK(!starts_with(std::vector<int>{1, 2},
                            std::vector<int>{1, 2, 3}));

    D_SQ_CHECK(starts_with(_empty, _empty));
    D_SQ_CHECK(!starts_with(_empty, std::vector<int>{1}));

    return true;
}

// tests_sequential_ends_with
//   an empty, partial, and full suffix match; a mismatch and an over-long
// suffix do not.
bool
tests_sequential_ends_with()
{
    std::vector<int> _c{1, 2, 3, 4};
    std::vector<int> _empty;

    D_SQ_CHECK(ends_with(_c, _empty));
    D_SQ_CHECK(ends_with(_c, std::vector<int>{3, 4}));
    D_SQ_CHECK(ends_with(_c, std::vector<int>{1, 2, 3, 4}));
    D_SQ_CHECK(!ends_with(_c, std::vector<int>{3, 3}));

    D_SQ_CHECK(!ends_with(std::vector<int>{1, 2},
                          std::vector<int>{1, 2, 3}));

    return true;
}

// tests_sequential_contains_subsequence
//   a run is found at the head, interior, and tail; the empty run is found; an
// absent or over-long run is not.
bool
tests_sequential_contains_subsequence()
{
    std::vector<int> _c{1, 2, 3, 4};
    std::vector<int> _empty;

    D_SQ_CHECK(contains_subsequence(_c, std::vector<int>{1, 2}));
    D_SQ_CHECK(contains_subsequence(_c, std::vector<int>{2, 3}));
    D_SQ_CHECK(contains_subsequence(_c, std::vector<int>{3, 4}));
    D_SQ_CHECK(contains_subsequence(_c, std::vector<int>{1, 2, 3, 4}));
    D_SQ_CHECK(contains_subsequence(_c, _empty));

    D_SQ_CHECK(!contains_subsequence(_c, std::vector<int>{5, 6}));
    D_SQ_CHECK(!contains_subsequence(std::vector<int>{1, 2},
                                     std::vector<int>{1, 2, 3}));

    return true;
}

// tests_sequential_algorithms_on_string
//   std::string is a sequential container, so the algorithms apply to it.
bool
tests_sequential_algorithms_on_string()
{
    std::string _s{"hello"};

    D_SQ_CHECK(starts_with(_s, std::string{"he"}));
    D_SQ_CHECK(ends_with(_s, std::string{"lo"}));
    D_SQ_CHECK(contains_subsequence(_s, std::string{"ell"}));
    D_SQ_CHECK(!starts_with(_s, std::string{"xy"}));

    return true;
}

// tests_sequential_algorithms_gating
//   the enable_if guard admits sequential containers.  Because the sequential
// verdict includes the ordered associatives (no hasher), starts_with is
// callable on std::set here - the divergence from the ORDER axis - while the
// hashed associatives and non-containers stay rejected.
bool
tests_sequential_algorithms_gating()
{
    // admitted: sequences.
    D_SQ_CHECK((can_starts_with<std::vector<int>,
                                std::vector<int>>::value));
    D_SQ_CHECK((can_ends_with<std::vector<int>,
                              std::vector<int>>::value));
    D_SQ_CHECK((can_contains_subsequence<std::vector<int>,
                                         std::vector<int>>::value));
    D_SQ_CHECK((can_starts_with<std::string, std::string>::value));

    // admitted here (and NOT on the ORDER axis): an ordered associative.
    D_SQ_CHECK((can_starts_with<std::set<int>, std::set<int>>::value));

    // rejected: a hashed associative (it carries a hasher).
    D_SQ_CHECK((!can_starts_with<std::unordered_set<int>,
                                 std::unordered_set<int>>::value));

    // rejected: a non-container, and a mixed pair with one non-sequential
    // side.
    D_SQ_CHECK((!can_starts_with<int, int>::value));
    D_SQ_CHECK((!can_starts_with<std::vector<int>, int>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
