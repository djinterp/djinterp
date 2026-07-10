// djinterp [test] : adapter_tests_function.cpp
//   The function-adapter layer (section VIII): function_adapter (both the
// transforming primary template and the void-transform passthrough
// specialization), result_adapter, argument_adapter (per-position transforms,
// C++14+), and compose_adapter — each in its const and non-const operator()
// forms.

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "adapter_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // add3 : sums three ints (the inner callable for the transform tests).
    struct add3
    {
        int
        operator()(
            int _a,
            int _b,
            int _c
        ) const
        {
            return _a + _b + _c;
        }
    };

    // inc : increments an int (the per-argument transform).
    struct inc
    {
        int
        operator()(
            int _x
        ) const
        {
            return _x + 1;
        }
    };

    // cstr_len : length of a C string (result_adapter inner callable).
    struct cstr_len
    {
        std::size_t
        operator()(
            const char* _s
        ) const
        {
            std::size_t n = 0;

            // walk to the terminator
            while (_s[n] != '\0')
            {
                ++n;
            }

            return n;
        }
    };

    // size_to_int : narrows a size_t to int (result transform).
    struct size_to_int
    {
        int
        operator()(
            std::size_t _n
        ) const
        {
            return static_cast<int>(_n);
        }
    };

    // times2 / plus10 : per-position argument transforms for argument_adapter.
    struct times2
    {
        int
        operator()(
            int _x
        ) const
        {
            return _x * 2;
        }
    };

    struct plus10
    {
        int
        operator()(
            int _x
        ) const
        {
            return _x + 10;
        }
    };

    // combine2 : folds two ints so per-position transforms are distinguishable.
    struct combine2
    {
        int
        operator()(
            int _a,
            int _b
        ) const
        {
            return (_a * 100) + _b;
        }
    };

    // square / plus1 : the two stages for compose_adapter.
    struct square
    {
        int
        operator()(
            int _x
        ) const
        {
            return _x * _x;
        }
    };

    struct plus1
    {
        int
        operator()(
            int _x
        ) const
        {
            return _x + 1;
        }
    };
}


/*
tests_function_adapter_transform
  Verifies function_adapter's transforming primary template.
  Tests the following:
  - each argument is passed through the transform, then to the inner callable
  - the transform-type / function-type aliases resolve
*/
bool
tests_function_adapter_transform()
{
    function_adapter<add3, inc> fa(add3{}, inc{});

    static_assert(std::is_same<decltype(fa)::function_type, add3>::value,
                  "function_type");
    static_assert(std::is_same<decltype(fa)::transform_type, inc>::value,
                  "transform_type");

    bool ok = true;

    // add3(inc(1), inc(2), inc(3)) == 2 + 3 + 4
    ok = ok && (fa(1, 2, 3) == 9);
    ok = ok && (fa(10, 20, 30) == 63);

    return ok;
}

/*
tests_function_adapter_transform_const
  Verifies function_adapter's const operator() (transforming template).
  Tests the following:
  - a const adapter invokes correctly through the const overload
*/
bool
tests_function_adapter_transform_const()
{
    const function_adapter<add3, inc> fa(add3{}, inc{});

    bool ok = true;

    ok = ok && (fa(1, 2, 3) == 9);

    return ok;
}

/*
tests_function_adapter_passthrough
  Verifies the void-transform function_adapter specialization.
  Tests the following:
  - arguments are forwarded unchanged to the inner callable
  - the function-type alias resolves
*/
bool
tests_function_adapter_passthrough()
{
    function_adapter<add3> fp(add3{});

    static_assert(std::is_same<decltype(fp)::function_type, add3>::value,
                  "function_type");

    bool ok = true;

    ok = ok && (fp(1, 2, 3) == 6);
    ok = ok && (fp(4, 5, 6) == 15);

    return ok;
}

/*
tests_function_adapter_passthrough_const
  Verifies the const operator() of the passthrough specialization.
  Tests the following:
  - a const passthrough adapter forwards correctly
*/
bool
tests_function_adapter_passthrough_const()
{
    const function_adapter<add3> fp(add3{});

    bool ok = true;

    ok = ok && (fp(1, 2, 3) == 6);

    return ok;
}

/*
tests_result_adapter
  Verifies result_adapter (post-processing of the return value).
  Tests the following:
  - the inner result is passed through the result transform
  - the function-type / transform-type aliases resolve
*/
bool
tests_result_adapter()
{
    result_adapter<cstr_len, size_to_int> ra(cstr_len{}, size_to_int{});

    static_assert(std::is_same<decltype(ra)::function_type, cstr_len>::value,
                  "function_type");
    static_assert(std::is_same<decltype(ra)::transform_type, size_to_int>::value,
                  "transform_type");

    bool ok = true;

    ok = ok && (ra("hello") == 5);
    ok = ok && (ra("") == 0);

    return ok;
}

/*
tests_result_adapter_const
  Verifies result_adapter's const operator().
  Tests the following:
  - a const result adapter transforms correctly through the const overload
*/
bool
tests_result_adapter_const()
{
    const result_adapter<cstr_len, size_to_int> ra(cstr_len{}, size_to_int{});

    bool ok = true;

    ok = ok && (ra("hi") == 2);

    return ok;
}

/*
tests_argument_adapter
  Verifies argument_adapter (per-position argument transforms via a tuple).
  Tests the following:
  - each argument is routed through its own positional transform
*/
bool
tests_argument_adapter()
{
    argument_adapter<combine2, std::tuple<times2, plus10>>
        aa(combine2{}, std::make_tuple(times2{}, plus10{}));

    bool ok = true;

    // combine2(times2(3), plus10(4)) == combine2(6, 14) == 614
    ok = ok && (aa(3, 4) == 614);
    // combine2(times2(1), plus10(0)) == combine2(2, 10) == 210
    ok = ok && (aa(1, 0) == 210);

    return ok;
}

/*
tests_compose_adapter
  Verifies compose_adapter (outer(inner(args...))).
  Tests the following:
  - the inner callable runs first, then the outer on its result
*/
bool
tests_compose_adapter()
{
    compose_adapter<square, plus1> ca(square{}, plus1{});

    bool ok = true;

    // square(plus1(3)) == square(4) == 16
    ok = ok && (ca(3) == 16);
    // square(plus1(0)) == square(1) == 1
    ok = ok && (ca(0) == 1);

    return ok;
}

/*
tests_compose_adapter_const
  Verifies compose_adapter's const operator().
  Tests the following:
  - a const compose adapter evaluates correctly through the const overload
*/
bool
tests_compose_adapter_const()
{
    const compose_adapter<square, plus1> ca(square{}, plus1{});

    bool ok = true;

    // square(plus1(2)) == square(3) == 9
    ok = ok && (ca(2) == 9);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
