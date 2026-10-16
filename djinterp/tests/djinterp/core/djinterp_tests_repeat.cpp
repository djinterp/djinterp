/*******************************************************************************
* djinterp [core]                                      djinterp_tests_repeat.cpp
*
*
* path:      /tests/djinterp/core/djinterp_tests_repeat.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_repeat_zero
  Tests the following:
  - repeat<T,0>::type is std::tuple<> (empty) for several T
  - tuple_size of the zero case is 0
  - this is the std::conditional_t false branch of repeat<>
*/
bool
tests_repeat_zero()
{
    bool _ok = true;

    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::repeat<int, 0>::type, std::tuple<>>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::repeat<double, 0>::type, std::tuple<>>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::repeat<probe_target, 0>::type, std::tuple<>>::value));
    D_DT_CHECK(_ok, (std::tuple_size<typename ::djinterp::repeat<int, 0>::type>::value == 0));

    static_assert(std::is_same<typename ::djinterp::repeat<int, 0>::type, std::tuple<>>::value, "");
    return _ok;
}

/*
tests_repeat_counts
  Tests the following:
  - repeat<T,N>::type is std::tuple<T,...,T> with exactly N elements for
    N = 1, 2, 3, and 5 (exercising the internal recursive repeat_type_helper)
  - tuple_size matches N
  - each tuple element type is T
*/
bool
tests_repeat_counts()
{
    bool _ok = true;

    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::repeat<int, 1>::type, std::tuple<int>>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::repeat<int, 2>::type, std::tuple<int, int>>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::repeat<int, 3>::type, std::tuple<int, int, int>>::value));
    D_DT_CHECK(_ok, (std::is_same<typename ::djinterp::repeat<char, 5>::type, std::tuple<char, char, char, char, char>>::value));

    D_DT_CHECK(_ok, (std::tuple_size<typename ::djinterp::repeat<int, 1>::type>::value == 1));
    D_DT_CHECK(_ok, (std::tuple_size<typename ::djinterp::repeat<int, 3>::type>::value == 3));
    D_DT_CHECK(_ok, (std::tuple_size<typename ::djinterp::repeat<char, 5>::type>::value == 5));

    // element types
    D_DT_CHECK(_ok, (std::is_same<std::tuple_element_t<0, typename ::djinterp::repeat<int, 3>::type>, int>::value));
    D_DT_CHECK(_ok, (std::is_same<std::tuple_element_t<2, typename ::djinterp::repeat<int, 3>::type>, int>::value));

    static_assert(std::is_same<typename ::djinterp::repeat<int, 3>::type, std::tuple<int, int, int>>::value, "");
    return _ok;
}

/*
tests_repeat_t_alias_and_types
  Tests the following:
  - repeat_t<T,N> equals repeat<T,N>::type (zero and non-zero N)
  - the element type is preserved exactly, including non-trivial element types
    (the self marker, pointers, and references)
*/
bool
tests_repeat_t_alias_and_types()
{
    bool _ok = true;
    using ::djinterp::self;

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::repeat_t<int, 0>, typename ::djinterp::repeat<int, 0>::type >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::repeat_t<int, 4>, typename ::djinterp::repeat<int, 4>::type >::value));

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::repeat_t<self, 2>, std::tuple<self, self> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::repeat_t<int*, 2>, std::tuple<int*, int*> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::repeat_t<int&, 3>, std::tuple<int&, int&, int&> >::value));

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
