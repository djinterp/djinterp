/*******************************************************************************
* djinterp [testing]                      ordered_container_tests_algorithms.cpp
*
*   Section TU: the free order algorithms.  Each of starts_with / ends_with /
* contains_subsequence is exercised for the empty, partial, exact, mismatched,
* and over-long cases (the last reaching the early-out branches), then repeated
* on std::string.  A final compile-time block probes the enable_if guard: it
* admits ordered containers and rejects both associatives and non-containers.
*
*
* path:      /tests/djinterp/core/container/ordered_container_tests_algorithms.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "ordered_container_tests.hpp"   // normal mode


NS_DJINTERP
NS_TESTING

// tests_ordered_starts_with
//   an empty, partial, and full prefix all match; a mismatch and an over-long
// prefix (which runs off the end of the container) do not.
bool
tests_ordered_starts_with()
{
    std::vector<int> _c{1, 2, 3, 4};
    std::vector<int> _empty;

    D_OR_CHECK(starts_with(_c, _empty));
    D_OR_CHECK(starts_with(_c, std::vector<int>{1, 2}));
    D_OR_CHECK(starts_with(_c, std::vector<int>{1, 2, 3, 4}));
    D_OR_CHECK(!starts_with(_c, std::vector<int>{1, 3}));

    // prefix longer than the container: the c_it == c_end early-out.
    D_OR_CHECK(!starts_with(std::vector<int>{1, 2},
                            std::vector<int>{1, 2, 3}));

    // empty container: matches only the empty prefix.
    D_OR_CHECK(starts_with(_empty, _empty));
    D_OR_CHECK(!starts_with(_empty, std::vector<int>{1}));

    return true;
}

// tests_ordered_ends_with
//   an empty, partial, and full suffix all match; a mismatch and an over-long
// suffix (rejected up front by the size comparison) do not.
bool
tests_ordered_ends_with()
{
    std::vector<int> _c{1, 2, 3, 4};
    std::vector<int> _empty;

    D_OR_CHECK(ends_with(_c, _empty));
    D_OR_CHECK(ends_with(_c, std::vector<int>{3, 4}));
    D_OR_CHECK(ends_with(_c, std::vector<int>{1, 2, 3, 4}));
    D_OR_CHECK(!ends_with(_c, std::vector<int>{3, 3}));

    // suffix longer than the container: the s_sz > c_sz early-out.
    D_OR_CHECK(!ends_with(std::vector<int>{1, 2},
                          std::vector<int>{1, 2, 3}));

    return true;
}

// tests_ordered_contains_subsequence
//   a contiguous run is found at the head, interior, and tail; the empty run
//   is
// found; an absent or over-long run is not.
bool
tests_ordered_contains_subsequence()
{
    std::vector<int> _c{1, 2, 3, 4};
    std::vector<int> _empty;

    D_OR_CHECK(contains_subsequence(_c, std::vector<int>{1, 2}));
    D_OR_CHECK(contains_subsequence(_c, std::vector<int>{2, 3}));
    D_OR_CHECK(contains_subsequence(_c, std::vector<int>{3, 4}));
    D_OR_CHECK(contains_subsequence(_c, std::vector<int>{1, 2, 3, 4}));
    D_OR_CHECK(contains_subsequence(_c, _empty));

    D_OR_CHECK(!contains_subsequence(_c, std::vector<int>{5, 6}));
    D_OR_CHECK(!contains_subsequence(std::vector<int>{1, 2},
                                     std::vector<int>{1, 2, 3}));

    return true;
}

// tests_ordered_algorithms_on_string
//   std::string is an ordered container, so the same algorithms apply to it.
bool
tests_ordered_algorithms_on_string()
{
    std::string _s{"hello"};

    D_OR_CHECK(starts_with(_s, std::string{"he"}));
    D_OR_CHECK(ends_with(_s, std::string{"lo"}));
    D_OR_CHECK(contains_subsequence(_s, std::string{"ell"}));
    D_OR_CHECK(!starts_with(_s, std::string{"xy"}));

    return true;
}

// tests_ordered_algorithms_gating
//   the enable_if guard admits only ordered containers.  Because this is the
// ORDER axis, associatives (ordered or hashed) are rejected too, alongside
// non-containers - a compile-time property read off the probe traits.
bool
tests_ordered_algorithms_gating()
{
    // admitted: ordered containers.
    D_OR_CHECK((can_starts_with<std::vector<int>,
                                std::vector<int>>::value));
    D_OR_CHECK((can_ends_with<std::vector<int>,
                              std::vector<int>>::value));
    D_OR_CHECK((can_contains_subsequence<std::vector<int>,
                                         std::vector<int>>::value));
    D_OR_CHECK((can_starts_with<std::string, std::string>::value));

    // rejected: associatives - the ORDER axis excludes all of them.
    D_OR_CHECK((!can_starts_with<std::set<int>, std::set<int>>::value));
    D_OR_CHECK((!can_starts_with<std::unordered_set<int>,
                                 std::unordered_set<int>>::value));

    // rejected: a non-container, and a mixed pair with one non-ordered side.
    D_OR_CHECK((!can_starts_with<int, int>::value));
    D_OR_CHECK((!can_starts_with<std::vector<int>, int>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
