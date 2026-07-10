/******************************************************************************
* djinterp [test]                                   byte_size_tests_descent.cpp
*
*   Section IV of byte_size.hpp: byte_dynamic_helper's two specializations --
* the primary (element is a leaf: stop at this level's own region) and the
* recursing one (element is a nested container: own region plus the dynamic
* footprint of every element).
*
*   All checks are run-time (dynamic sizes read capacity()/size()); each
* compares dynamic_byte_size against an expectation built by walking the same
* nesting with the same surface reads.  Coverage:
*     - leaf element (container of scalars) -> primary helper, own region only;
*     - nested container -> recursing helper, own + sum over elements, at depth
*       two and three;
*     - the c_str leaf frontier: a container of strings is NOT descended into
*       (its strings' own heap is excluded), matching content_equality.hpp;
*     - recursion THROUGH static-sited elements, which are entered but add zero
*       (vector<inline_bag>, array<array>);
*     - a mixed nest (contiguous outer, node inner);
*     - empty outer and empty inner containers.
*
*
* path:      /inc/djinterp/test/byte_size_tests_descent.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.03
******************************************************************************/

#include "byte_size_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace byte_size_test_types;


// =========================================================================
// I.   LEAF ELEMENT -> primary helper (no descent)
// =========================================================================

bool
tests_byte_size_descent_leaf_element()
{
    bool _ok = true;

    // element is a scalar: the recursion gate is false, so the primary helper
    // reports this level's own region only.
    std::vector<int> v{1, 2, 3, 4, 5};
    _ok = _ok && (dynamic_byte_size(v) == v.capacity() * sizeof(int));

    std::list<double> l{1.0, 2.0, 3.0};
    _ok = _ok && (dynamic_byte_size(l) == l.size() * sizeof(double));

    return _ok;
}


// =========================================================================
// II.  NESTED CONTAINER -> recursing helper (own + sum over elements)
// =========================================================================

bool
tests_byte_size_descent_nested()
{
    bool _ok = true;

    // depth two: outer buffer + each inner buffer
    std::vector<std::vector<int>> vv{{1, 2, 3}, {4, 5}, {6}};
    {
        std::size_t _expect = vv.capacity() * sizeof(std::vector<int>);
        for (const auto& _inner : vv)
        {
            _expect += _inner.capacity() * sizeof(int);
        }
        _ok = _ok && (dynamic_byte_size(vv) == _expect);
    }

    // depth three
    std::vector<std::vector<std::vector<int>>> v3{{{1, 2}, {3}}, {{4, 5, 6}}};
    {
        std::size_t _expect = v3.capacity() * sizeof(std::vector<std::vector<int>>);
        for (const auto& _v2 : v3)
        {
            _expect += _v2.capacity() * sizeof(std::vector<int>);
            for (const auto& _v1 : _v2)
            {
                _expect += _v1.capacity() * sizeof(int);
            }
        }
        _ok = _ok && (dynamic_byte_size(v3) == _expect);
    }

    return _ok;
}


// =========================================================================
// III. c_str LEAF FRONTIER  (strings are not descended into)
// =========================================================================

bool
tests_byte_size_descent_c_str_frontier()
{
    bool _ok = true;

    // long strings (heap-backed past SBO), yet the descent treats each as a
    // leaf atom: only the outer buffer of string OBJECTS is counted.
    std::vector<std::string> vs{
        std::string("a long string well past small-buffer optimisation length"),
        std::string("another sufficiently long heap-backed string value here")
    };
    _ok = _ok && (dynamic_byte_size(vs) == vs.capacity() * sizeof(std::string));

    // same frontier for a node container of strings
    std::list<std::string> ls{
        std::string("yet another long heap string for the list case here ok"),
        std::string("second long heap string to keep the node store nonempty")
    };
    _ok = _ok && (dynamic_byte_size(ls) == ls.size() * sizeof(std::string));

    return _ok;
}


// =========================================================================
// IV.  RECURSION THROUGH STATIC-SITED ELEMENTS  (entered, contribute zero)
// =========================================================================

bool
tests_byte_size_descent_static_elements()
{
    bool _ok = true;

    // vector<inline_bag>: inner is iterable (so descended) but static (0 heap),
    // so only the outer buffer of inline_bag OBJECTS counts.
    std::vector<inline_bag<int, 3>> vb(2);
    _ok = _ok && (dynamic_byte_size(vb) == vb.capacity() * sizeof(inline_bag<int, 3>));

    // array<array<int,3>,2>: static all the way down -> zero dynamic, but the
    // recursing helper still descends (summing zeros).
    std::array<std::array<int, 3>, 2> aa{};
    _ok = _ok && (dynamic_byte_size(aa) == 0u);

    return _ok;
}


// =========================================================================
// V.   MIXED NEST  (contiguous outer, node inner)
// =========================================================================

bool
tests_byte_size_descent_mixed()
{
    bool _ok = true;

    std::vector<std::list<int>> vl{{1, 2, 3}, {4, 5}, {6, 7, 8, 9}};
    {
        std::size_t _expect = vl.capacity() * sizeof(std::list<int>);
        for (const auto& _inner : vl)
        {
            _expect += _inner.size() * sizeof(int);   // inner is a node store
        }
        _ok = _ok && (dynamic_byte_size(vl) == _expect);
    }

    // contiguous outer, node-by-distance inner (forward_list)
    std::vector<std::forward_list<int>> vf{{1, 2}, {3, 4, 5}};
    {
        std::size_t _expect = vf.capacity() * sizeof(std::forward_list<int>);
        for (const auto& _inner : vf)
        {
            _expect += static_cast<std::size_t>(
                std::distance(_inner.begin(), _inner.end())) * sizeof(int);
        }
        _ok = _ok && (dynamic_byte_size(vf) == _expect);
    }

    return _ok;
}


// =========================================================================
// VI.  EMPTY OUTER / EMPTY INNER
// =========================================================================

bool
tests_byte_size_descent_empty()
{
    bool _ok = true;

    // empty outer -> own region only (capacity may be zero)
    std::vector<std::vector<int>> ve;
    _ok = _ok && (dynamic_byte_size(ve) == ve.capacity() * sizeof(std::vector<int>));

    // populated outer, all inners empty -> outer buffer + sum of zeros
    std::vector<std::vector<int>> vei(3);
    {
        std::size_t _expect = vei.capacity() * sizeof(std::vector<int>);
        for (const auto& _inner : vei)
        {
            _expect += _inner.capacity() * sizeof(int);   // each 0
        }
        _ok = _ok && (dynamic_byte_size(vei) == _expect);
    }

    return _ok;
}


NS_END  // testing
NS_END  // djinterp
