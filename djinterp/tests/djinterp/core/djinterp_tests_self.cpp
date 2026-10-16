/*******************************************************************************
* djinterp [core]                                        djinterp_tests_self.cpp
*
*
* path:      /tests/djinterp/core/djinterp_tests_self.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_self_is_self_positive
  Tests the following:
  - is_self<self>::value is true and is_self_v<self> is true (specialization)
  - is_self<self> derives from std::true_type
  - self is an empty, trivial marker type
*/
bool
tests_self_is_self_positive()
{
    bool _ok = true;
    using ::djinterp::self;
    using ::djinterp::is_self;

    D_DT_CHECK(_ok, is_self<self>::value == true);
    D_DT_CHECK(_ok, ::djinterp::is_self_v<self> == true);
    D_DT_CHECK(_ok, (std::is_base_of<std::true_type, is_self<self>>::value));

    // self is an empty, trivial marker
    D_DT_CHECK(_ok, std::is_empty<self>::value == true);
    D_DT_CHECK(_ok, std::is_trivial<self>::value == true);

    static_assert(is_self<self>::value, "");
    static_assert(::djinterp::is_self_v<self>, "");
    return _ok;
}

/*
tests_self_is_self_negative
  Tests the following:
  - is_self is false for unrelated types (int, void, a user type, a tuple of
    self) via the primary template
  - is_self matches the marker EXACTLY: cv-qualified self, reference-to-self,
    and pointer-to-self are all NOT recognized as self
*/
bool
tests_self_is_self_negative()
{
    bool _ok = true;
    using ::djinterp::self;
    using ::djinterp::is_self;

    D_DT_CHECK(_ok, is_self<int>::value == false);
    D_DT_CHECK(_ok, is_self<void>::value == false);
    D_DT_CHECK(_ok, is_self<probe_target>::value == false);
    D_DT_CHECK(_ok, (is_self<std::tuple<self>>::value) == false);
    D_DT_CHECK(_ok, ::djinterp::is_self_v<int> == false);

    // exact-match semantics: each of these is a distinct type from `self`
    D_DT_CHECK(_ok, is_self<const self>::value == false);
    D_DT_CHECK(_ok, is_self<volatile self>::value == false);
    D_DT_CHECK(_ok, is_self<self&>::value == false);
    D_DT_CHECK(_ok, is_self<self&&>::value == false);
    D_DT_CHECK(_ok, is_self<self*>::value == false);

    static_assert(!is_self<int>::value, "");
    static_assert(!::djinterp::is_self_v<self*>, "");
    return _ok;
}

NS_END  // testing
NS_END  // djinterp
