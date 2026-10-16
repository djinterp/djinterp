/*******************************************************************************
* djinterp [core]                                       djinterp_tests_clean.cpp
*
*
* path:      /tests/djinterp/core/djinterp_tests_clean.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_clean_basic
  Tests the following:
  - clean<T>::type strips lvalue and rvalue references
  - clean<T>::type strips top-level const, volatile, and const volatile
  - a plain type is returned unchanged
*/
bool
tests_clean_basic()
{
    bool _ok = true;

    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::clean<int>::type, int>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::clean<int&>::type, int>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::clean<int&&>::type, int>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::clean<const int>::type, int>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::clean<volatile int>::type, int>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::clean<const volatile int>::type, int>::value));

    static_assert(std::is_same<typename ::djinterp::clean<const int&>::type, int>::value, "");
    return _ok;
}

/*
tests_clean_edges
  Tests the following:
  - references-to-cv collapse to the bare type (const int& -> int)
  - only TOP-LEVEL cv is removed: a pointer-to-const keeps its low-level const
    (const int* -> const int*), while a const pointer loses its top-level
    const (int* const -> int*)
  - reference + pointer cv combinations apply both rules in turn
  - a user type behaves identically
*/
bool
tests_clean_edges()
{
    bool _ok = true;
    using ::djinterp::clean_t;

    // reference + cv
    D_DT_CHECK(_ok, (std::is_same<clean_t<const int&>, int>::value));
    D_DT_CHECK(_ok, (std::is_same<clean_t<volatile int&>, int>::value));
    D_DT_CHECK(_ok, (std::is_same<clean_t<const volatile int&&>, int>::value));

    // low-level (pointee) const preserved vs top-level const removed
    D_DT_CHECK(_ok, (std::is_same<clean_t<const int*>, const int*>::value));
    D_DT_CHECK(_ok, (std::is_same<clean_t<int* const>, int*>::value));
    D_DT_CHECK(_ok, (std::is_same<clean_t<const int* const>, const int*>::value));
    D_DT_CHECK(_ok, (std::is_same<clean_t<const int*&>, const int*>::value));
    D_DT_CHECK(_ok, (std::is_same<clean_t<int* const&>, int*>::value));

    // a user type, referenced and const-qualified
    D_DT_CHECK(_ok, (std::is_same<clean_t<const probe_target&>, probe_target>::value));

    // arrays: top-level cv on the element is stripped, extents are preserved,
    // and a reference-to-array decays to the bare array
    D_DT_CHECK(_ok, (std::is_same<clean_t<int[3]>, int[3]>::value));
    D_DT_CHECK(_ok, (std::is_same<clean_t<const int[3]>, int[3]>::value));
    D_DT_CHECK(_ok, (std::is_same<clean_t<const int(&)[3]>, int[3]>::value));

    return _ok;
}

/*
tests_clean_t_alias
  Tests the following:
  - clean_t<T> is exactly clean<T>::type across a spread of inputs, including
    deeply qualified pointer/reference combinations and a user type
*/
bool
tests_clean_t_alias()
{
    bool _ok = true;

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::clean_t<int>, typename ::djinterp::clean<int>::type >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::clean_t<const int&>, typename ::djinterp::clean<const int&>::type >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::clean_t<volatile int* const&>, typename ::djinterp::clean<volatile int* const&>::type >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::clean_t<probe_target&&>, typename ::djinterp::clean<probe_target&&>::type >::value));

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
