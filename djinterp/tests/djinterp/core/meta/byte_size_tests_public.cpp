/******************************************************************************
* djinterp [test]                                    byte_size_tests_public.cpp
*
*   Section V of byte_size.hpp: the public functions dynamic_byte_size and
* total_byte_size, viewed as a whole rather than by internal branch.
*
*   Checks:
*     - the defining identity total_byte_size == static_byte_size + dynamic_byte_size,
*       across contiguous / node / static / nested containers;
*     - a static-sited container's total equals its (exact) static size, its
*       dynamic being zero;
*     - const-qualified inputs are accepted and give the same figures (the
*       functions take const _Container& and clean_t internally);
*     - integration sanity: total is never less than the exact static part.
*
*
* path:      /inc/djinterp/test/byte_size_tests_public.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.03
******************************************************************************/

#include "byte_size_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace byte_size_test_types;


// =========================================================================
// I.   total == static + dynamic
// =========================================================================

bool
tests_byte_size_total_identity()
{
    bool _ok = true;

    std::vector<int> v{1, 2, 3, 4, 5};
    _ok = _ok && (total_byte_size(v)
               == static_byte_size<std::vector<int>>::value + dynamic_byte_size(v));

    std::list<int> l{1, 2, 3};
    _ok = _ok && (total_byte_size(l)
               == static_byte_size<std::list<int>>::value + dynamic_byte_size(l));

    std::forward_list<int> fl{1, 2, 3, 4};
    _ok = _ok && (total_byte_size(fl)
               == static_byte_size<std::forward_list<int>>::value + dynamic_byte_size(fl));

    std::string s("a heap-backed string value beyond small-buffer length here");
    _ok = _ok && (total_byte_size(s)
               == static_byte_size<std::string>::value + dynamic_byte_size(s));

    std::vector<std::vector<int>> vv{{1, 2, 3}, {4, 5}};
    _ok = _ok && (total_byte_size(vv)
               == static_byte_size<std::vector<std::vector<int>>>::value
                + dynamic_byte_size(vv));

    return _ok;
}


// =========================================================================
// II.  STATIC-SITED: total == exact static, dynamic == 0
// =========================================================================

bool
tests_byte_size_total_static_sited()
{
    bool _ok = true;

    std::array<int, 10> a{};
    _ok = _ok && (dynamic_byte_size(a) == 0u);
    _ok = _ok && (total_byte_size(a) == static_byte_size<std::array<int, 10>>::value);
    _ok = _ok && (total_byte_size(a) == sizeof(std::array<int, 10>));

    inline_bag<double, 5> bag{};
    _ok = _ok && (dynamic_byte_size(bag) == 0u);
    _ok = _ok && (total_byte_size(bag) == sizeof(inline_bag<double, 5>));

    return _ok;
}


// =========================================================================
// III. CONST-QUALIFIED INPUTS
// =========================================================================

bool
tests_byte_size_total_const_input()
{
    bool _ok = true;

    const std::vector<int> cv{1, 2, 3, 4};
    _ok = _ok && (dynamic_byte_size(cv) == cv.capacity() * sizeof(int));
    _ok = _ok && (total_byte_size(cv)
               == static_byte_size<std::vector<int>>::value + dynamic_byte_size(cv));

    const std::vector<std::vector<int>> cvv{{1, 2}, {3, 4, 5}};
    {
        std::size_t _expect = cvv.capacity() * sizeof(std::vector<int>);
        for (const auto& _inner : cvv)
        {
            _expect += _inner.capacity() * sizeof(int);
        }
        _ok = _ok && (dynamic_byte_size(cvv) == _expect);
    }

    return _ok;
}


// =========================================================================
// IV.  INTEGRATION SANITY
// =========================================================================

bool
tests_byte_size_integration()
{
    bool _ok = true;

    // total is never below the exact static part
    std::vector<int> v(100, 7);
    _ok = _ok && (total_byte_size(v) >= static_byte_size<std::vector<int>>::value);

    std::list<std::string> ls{std::string("x"), std::string("y")};
    _ok = _ok && (total_byte_size(ls) >= static_byte_size<std::list<std::string>>::value);

    std::array<int, 8> a{};
    _ok = _ok && (total_byte_size(a) >= static_byte_size<std::array<int, 8>>::value);

    // a populated dynamic container's total strictly exceeds its static part
    // once it holds a nonzero capacity of nonzero-size cells
    _ok = _ok && (v.capacity() > 0);
    _ok = _ok && (total_byte_size(v) > static_byte_size<std::vector<int>>::value);

    return _ok;
}


NS_END  // testing
NS_END  // djinterp
