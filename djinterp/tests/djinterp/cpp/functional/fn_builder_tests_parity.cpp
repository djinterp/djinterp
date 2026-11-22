/*******************************************************************************
* djinterp [cpp][functional]                         fn_builder_tests_parity.cpp
*
* Parity tests (section viii): the C++ face against the C kernel.
*   test_parity_c_kernel -- the same chains, built with DFnBuilder and with
*                           c/functional/fn_builder.h, over the same inputs,
*                           give the same elements in the same order. The C
*                           kernel runs its steps in call order (since
*                           2026.11.11), as the C++ face always has; before
*                           then the two disagreed whenever a filter followed
*                           a map.
*
*
* path:      /tests/djinterp/cpp/functional/fn_builder_tests_parity.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

// std
#include <cstddef>
#include <vector>
// djinterp
#include "./fn_builder_tests.hpp"

extern "C"
{
    #include "../../../../inc/djinterp/c/functional/fn_builder.h"  // the C kernel
}


NS_DJINTERP
NS_TESTING

NS_INTERNAL

    // the C kernel's callbacks: the same functions DFnBuilder runs below

    bool c_double(const void* _in, void* _out, void* _context)
    {
        (void)_context;
        *static_cast<int*>(_out) = *static_cast<const int*>(_in) * 2;

        return true;
    }

    bool c_add_ten(const void* _in, void* _out, void* _context)
    {
        (void)_context;
        *static_cast<int*>(_out) = *static_cast<const int*>(_in) + 10;

        return true;
    }

    bool c_negate(const void* _in, void* _out, void* _context)
    {
        (void)_context;
        *static_cast<int*>(_out) = -*static_cast<const int*>(_in);

        return true;
    }

    bool c_is_even(const void* _element, void* _context)
    {
        (void)_context;

        return (*static_cast<const int*>(_element) % 2) == 0;
    }

    bool c_over_five(const void* _element, void* _context)
    {
        (void)_context;

        return *static_cast<const int*>(_element) > 5;
    }

    // run_c
    //   function: runs a C chain over _input; the elements it keeps, in
    // order, or an empty vector with *_ok false when the run failed.
    std::vector<int> run_c(
        const dj_fn_builder*    _builder,
        const std::vector<int>& _input,
        bool*                   _ok
    )
    {
        std::vector<int> out(_input.size());
        std::size_t      n = 0;

        *_ok = d_fn_builder_execute(_builder,
                                    _input.data(),
                                    _input.size(),
                                    sizeof(int),
                                    out.data(),
                                    &n);
        out.resize(*_ok ? n : 0);

        return out;
    }

NS_END  // internal


bool test_parity_c_kernel()
{
    bool verdict = true;

    const std::vector<int> input = { -9, -4, 0, 1, 2, 3, 4, 5, 6, 7 };
    const auto double_it = [](int _x) { return _x * 2; };
    const auto add_ten   = [](int _x) { return _x + 10; };
    const auto negate    = [](int _x) { return -_x; };
    const auto is_even   = [](int _x) { return (_x % 2) == 0; };
    const auto over_five = [](int _x) { return _x > 5; };

    bool ok = false;

    // map -> filter: the filter sees the doubled value
    {
        dj_fn_builder* c = d_fn_builder_new();

        d_fn_builder_filter(d_fn_builder_map(c, internal::c_double),
                            internal::c_over_five);

        const std::vector<int> from_c = internal::run_c(c, input, &ok);
        const std::vector<int> from_cpp = DFnBuilder<int>::create()
            .map(double_it)
            .filter(over_five)
            .execute(input);

        record_assertion(verdict,
            ok && (from_c == from_cpp) && (from_c.size() == 5),
            "parity: map -> filter (the filter sees the image)");
        d_fn_builder_free(c);
    }

    // filter -> map: the filter sees the input
    {
        dj_fn_builder* c = d_fn_builder_new();

        d_fn_builder_map(d_fn_builder_filter(c, internal::c_over_five),
                         internal::c_double);

        const std::vector<int> from_c = internal::run_c(c, input, &ok);
        const std::vector<int> from_cpp = DFnBuilder<int>::create()
            .filter(over_five)
            .map(double_it)
            .execute(input);

        record_assertion(verdict,
            ok && (from_c == from_cpp) && (from_c.size() == 2),
            "parity: filter -> map (the filter sees the input)");
        d_fn_builder_free(c);
    }

    // a filter between transformers, and a second filter last
    {
        dj_fn_builder* c = d_fn_builder_new();

        d_fn_builder_map(c, internal::c_add_ten);
        d_fn_builder_where(c, internal::c_over_five);
        d_fn_builder_and_then(c, internal::c_negate);
        d_fn_builder_filter(c, internal::c_is_even);

        const std::vector<int> from_c = internal::run_c(c, input, &ok);
        const std::vector<int> from_cpp = DFnBuilder<int>::create()
            .map(add_ten)
            .where(over_five)
            .and_then(negate)
            .filter(is_even)
            .execute(input);

        record_assertion(verdict,
            ok && (from_c == from_cpp) && (!from_c.empty()),
            "parity: map -> where -> and_then -> filter");
        d_fn_builder_free(c);
    }

    // three transformers, then a filter
    {
        dj_fn_builder* c = d_fn_builder_new();

        d_fn_builder_map(c, internal::c_double);
        d_fn_builder_and_then(c, internal::c_add_ten);
        d_fn_builder_and_then(c, internal::c_negate);
        d_fn_builder_filter(c, internal::c_is_even);

        const std::vector<int> from_c = internal::run_c(c, input, &ok);
        const std::vector<int> from_cpp = DFnBuilder<int>::create()
            .map(double_it)
            .and_then(add_ten)
            .and_then(negate)
            .filter(is_even)
            .execute(input);

        record_assertion(verdict,
            ok && (from_c == from_cpp) && (from_c.size() == input.size()),
            "parity: three transformers, then a filter");
        d_fn_builder_free(c);
    }

    // the empty chain copies every element on both faces
    {
        dj_fn_builder* c = d_fn_builder_new();

        const std::vector<int> from_c = internal::run_c(c, input, &ok);

        record_assertion(verdict,
            ok && (from_c == DFnBuilder<int>::create().execute(input)) &&
            (from_c == input),
            "parity: the empty chain copies every element");
        d_fn_builder_free(c);
    }

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
