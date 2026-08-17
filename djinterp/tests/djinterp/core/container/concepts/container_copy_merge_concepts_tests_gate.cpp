#include "container_copy_merge_concepts_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING

/*
tests_ccmc_language_gate
  the suite and the header agree on the language gate.
  Tests the following:
  - above the gate, D_ENV_LANG_IS_CPP20_OR_HIGHER and
    D_ENV_CPP_FEATURE_LANG_CONCEPTS are both set
  - above the gate, all four concepts are reachable and evaluable as constant
    expressions, so the header's contents are genuinely present
  - below the gate, the header is empty by design and the block records a skip
    rather than a failure
  This is the ONLY block outside the C++20 guard, so a build below the gate
  still produces a report instead of an empty one.
*/
bool
tests_ccmc_language_gate()
{
#if D_CM_CONCEPTS_ENABLED

    // the two macros the header's own #if tests, asserted separately so a
    // failure names which half of the condition moved
    D_CM_CHECK(D_ENV_LANG_IS_CPP20_OR_HIGHER != 0);
    D_CM_CHECK(D_ENV_CPP_FEATURE_LANG_CONCEPTS != 0);

    // all four concepts are present and evaluable - if the header had
    // self-suppressed under a condition the suite does not share, these would
    // not name anything
    D_CM_CHECK(::djinterp::copyable_container<std::vector<int>>);
    D_CM_CHECK(( ::djinterp::mergeable_with<std::vector<int>,
                                            std::vector<int>> ));
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                            std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::vector<int>,
                                                      std::vector<int>> ));

#else

    D_CM_NOTE("concepts unavailable: the module under test self-suppresses "
              "to nothing below C++20 + __cpp_concepts, and every other block "
              "in this suite is compiled out under the identical condition");

#endif  // D_CM_CONCEPTS_ENABLED

    return true;
}

NS_END  // testing
NS_END  // djinterp
