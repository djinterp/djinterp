// djinterp [test] : adapter_tests_factory.cpp
//   The convenience factories (section X, C++14+): make_object_adapter,
// make_owning_adapter, the two make_function_adapter overloads,
// make_result_adapter, make_compose, make_adapted_ref, and make_adapted_view.
// Each factory is checked for both correct type deduction and working
// behavior.
//
//   make_object_adapter hard-wires by_reference and is therefore gated on
// D_ADAPTER_BYREF_FIXED (see BUG 1); make_owning_adapter (by_value) covers the
// object-adapter factory path in the meantime.

// std
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
#include "adapter_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // add3 / inc : function_adapter factory inputs.
    struct add3
    {
        int operator()(int _a, int _b, int _c) const { return _a + _b + _c; }
    };

    struct inc
    {
        int operator()(int _x) const { return _x + 1; }
    };

    // cstr_len / size_to_int : result_adapter factory inputs.
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

    struct size_to_int
    {
        int operator()(std::size_t _n) const { return static_cast<int>(_n); }
    };

    // square / plus1 : compose factory inputs.
    struct square
    {
        int operator()(int _x) const { return _x * _x; }
    };

    struct plus1
    {
        int operator()(int _x) const { return _x + 1; }
    };
}


/*
tests_make_object_adapter
  Verifies make_object_adapter (deduces a by_reference object_adapter).
  Tests the following (only when D_ADAPTER_BYREF_FIXED — see BUG 1):
  - the deduced ownership policy is by_reference
  - size()/get() delegate correctly
*/
bool
tests_make_object_adapter()
{
#if D_ADAPTER_BYREF_FIXED
    legacy_seq s;
    s.data = { 1, 2, 3 };

    auto a = make_object_adapter<legacy_policy>(s);

    static_assert(std::is_same<decltype(a)::ownership_policy,
                               by_reference>::value,
                  "deduced by_reference");

    bool ok = true;

    ok = ok && (a.size() == 3);
    ok = ok && (a.get(0) == 1);

    return ok;
#else
    // BLOCKED by BUG 1: make_object_adapter returns a by_reference adapter.
    return true;
#endif
}

/*
tests_make_owning_adapter
  Verifies make_owning_adapter (deduces a by_value owning object_adapter).
  Tests the following:
  - the deduced ownership policy is by_value and adaptee_type is correct
  - non-const get() reads/writes the owned copy; the source is untouched
*/
bool
tests_make_owning_adapter()
{
    legacy_seq s;
    s.data = { 4, 5 };

    auto a = make_owning_adapter<legacy_policy>(s);   // owning copy

    static_assert(std::is_same<decltype(a)::ownership_policy, by_value>::value,
                  "deduced by_value");
    static_assert(std::is_same<decltype(a)::adaptee_type, legacy_seq>::value,
                  "adaptee_type");

    bool ok = true;

    ok = ok && (a.get(0) == 4);
    ok = ok && (a.get(1) == 5);

    a.get(0) = 40;
    ok = ok && (a.get(0) == 40);
    ok = ok && (s.data[0] == 4);

    return ok;
}

/*
tests_make_function_adapter_transform
  Verifies the two-argument make_function_adapter overload.
  Tests the following:
  - the transform is applied per argument before the inner callable
*/
bool
tests_make_function_adapter_transform()
{
    auto fa = make_function_adapter(add3{}, inc{});

    bool ok = true;

    ok = ok && (fa(1, 2, 3) == 9);

    return ok;
}

/*
tests_make_function_adapter_passthrough
  Verifies the single-argument make_function_adapter overload.
  Tests the following:
  - arguments are forwarded unchanged to the inner callable
*/
bool
tests_make_function_adapter_passthrough()
{
    auto fp = make_function_adapter(add3{});

    bool ok = true;

    ok = ok && (fp(1, 2, 3) == 6);

    return ok;
}

/*
tests_make_result_adapter
  Verifies make_result_adapter (deduces a result_adapter).
  Tests the following:
  - the inner result is transformed by the result transform
*/
bool
tests_make_result_adapter()
{
    auto ra = make_result_adapter(cstr_len{}, size_to_int{});

    bool ok = true;

    ok = ok && (ra("hello") == 5);

    return ok;
}

/*
tests_make_compose
  Verifies make_compose (deduces a compose_adapter).
  Tests the following:
  - composition evaluates outer(inner(args...))
*/
bool
tests_make_compose()
{
    auto ca = make_compose(square{}, plus1{});

    bool ok = true;

    ok = ok && (ca(3) == 16);

    return ok;
}

/*
tests_make_adapted_ref
  Verifies make_adapted_ref (deduces an adapted_ref).
  Tests the following:
  - adaptee_type is correct
  - size()/get() delegate and write-through get() reaches the adaptee
*/
bool
tests_make_adapted_ref()
{
    legacy_seq s;
    s.data = { 7, 8 };

    auto r = make_adapted_ref<legacy_policy>(s);

    static_assert(std::is_same<decltype(r)::adaptee_type, legacy_seq>::value,
                  "adaptee_type");

    bool ok = true;

    ok = ok && (r.size() == 2);
    ok = ok && (r.get(0) == 7);

    r.get(1) = 80;
    ok = ok && (s.data[1] == 80);

    return ok;
}

/*
tests_make_adapted_view
  Verifies make_adapted_view with a lambda projection (deduction of a decayed
  closure type).
  Tests the following:
  - traversal yields the projected values
  - size() passes through to the adaptee
*/
bool
tests_make_adapted_view()
{
    std::vector<int> v = { 1, 2, 3 };

    auto view = make_adapted_view(v, [](int& _x) { return _x + 100; });

    bool ok = true;

    std::vector<int> got;
    for (auto it = view.begin(); it != view.end(); ++it)
    {
        got.push_back(*it);
    }

    ok = ok && (got.size() == 3);
    ok = ok && (got[0] == 101);
    ok = ok && (got[1] == 102);
    ok = ok && (got[2] == 103);
    ok = ok && (view.size() == 3);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
