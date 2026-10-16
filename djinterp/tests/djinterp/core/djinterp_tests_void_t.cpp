/*******************************************************************************
* djinterp [core]                                      djinterp_tests_void_t.cpp
*
*
* path:      /tests/djinterp/core/djinterp_tests_void_t.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp_tests.hpp"


NS_DJINTERP
NS_TESTING

// dt_has_type_member
//   trait: void_t-based detector for a nested ::type (failure case).
template<typename _T,
         typename = void>
struct dt_has_type_member : std::false_type
{};

// dt_has_type_member (success case)
//   trait: partial specialization selected when typename _T::type is valid.
template<typename _T>
struct dt_has_type_member<_T, ::djinterp::void_t<typename _T::type>>
    : std::true_type
{};

// dt_has_value_member
//   trait: void_t-based detector for a nested ::value (failure case).
template<typename _T,
         typename = void>
struct dt_has_value_member : std::false_type
{};

// dt_has_value_member (success case)
//   trait: partial specialization selected when _T::value names something.
template<typename _T>
struct dt_has_value_member<_T, ::djinterp::void_t<decltype(_T::value)>>
    : std::true_type
{};

// dt_carrier_with_type
//   type: carrier exposing a nested ::type for the detector tests.
struct dt_carrier_with_type
{
    using type = int;
};

// dt_carrier_empty
//   type: carrier exposing no nested members.
struct dt_carrier_empty
{};


/*
tests_void_t_aliases_to_void
  Tests the following:
  - void_t<> with zero arguments is void
  - void_t with one, two, and several arguments (including cv-qualified,
    reference, pointer, and void types) is always void
  - void_t fed a type produced by the header itself is still void
*/
bool
tests_void_t_aliases_to_void()
{
    bool _ok = true;

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::void_t<>, void >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::void_t<int>, void >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::void_t<int, double>, void >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::void_t<const int&, char*, void>, void >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::void_t<int, int, int, int, int>, void >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::void_t<typename ::djinterp::clean<const int&>::type>, void >::value));

    static_assert(std::is_same< ::djinterp::void_t<int, char>, void >::value, "");
    return _ok;
}

/*
tests_void_t_detection_idiom
  Tests the following:
  - void_t functions as the SFINAE sink in the detection idiom
  - positive and negative detection of a nested ::type
  - positive and negative detection of a nested ::value
  - interoperates with the header's own clean<> (exposes ::type) and
    abs_value<> (exposes ::value)
*/
bool
tests_void_t_detection_idiom()
{
    bool _ok = true;

    D_DT_CHECK(_ok, dt_has_type_member<dt_carrier_with_type>::value == true);
    D_DT_CHECK(_ok, dt_has_type_member<dt_carrier_empty>::value == false);
    D_DT_CHECK(_ok, dt_has_type_member<int>::value == false);

    // clean<int> exposes ::type, so the detector must see it
    D_DT_CHECK(_ok, dt_has_type_member< ::djinterp::clean<int> >::value == true);

    // abs_value<int,-3> exposes a static ::value member
    D_DT_CHECK(_ok, dt_has_value_member< ::djinterp::abs_value<int, -3> >::value == true);
    D_DT_CHECK(_ok, dt_has_value_member<dt_carrier_empty>::value == false);

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
