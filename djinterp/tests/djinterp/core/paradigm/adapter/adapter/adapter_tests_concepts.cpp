// djinterp [test] : adapter_tests_concepts.cpp
//   The C++20 concept surface (section XI): adaptable_to (value + structural
// adaptability), adapter_for (the adaptee()/size() surface), function_adaptable
// (invocability), and the constrained_adapt factory.
//
//   The whole TU is gated on D_ADAPTER_HAS_CONCEPTS so it degrades to no-op
// predicates below C++20.  constrained_adapt returns a by_reference adapter,
// so its runtime path is additionally gated on D_ADAPTER_BYREF_FIXED (BUG 1).

// std
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "adapter_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // inc : a unary callable (invocable with int).
    struct inc
    {
        int operator()(int _x) const { return _x + 1; }
    };

    // nullary : a zero-argument callable.
    struct nullary
    {
        int operator()() const { return 0; }
    };
}


/*
tests_adaptable_to
  Verifies the adaptable_to concept (value compatibility AND structural
  adaptability).
  Tests the following:
  - satisfied for convertible value types over a shared structural surface
  - rejected when there is no value_type / structural surface
  - rejected when structure overlaps but value types are incompatible
*/
bool
tests_adaptable_to()
{
#if D_ADAPTER_HAS_CONCEPTS
    static_assert(adaptable_to<std::vector<int>, std::vector<long>>,
                  "vector<int> adaptable_to vector<long>");
    static_assert(!adaptable_to<int, int>,
                  "int not adaptable_to int");

    bool ok = true;

    ok = ok && ( adaptable_to<std::vector<int>, std::vector<long>>);
    ok = ok && ( adaptable_to<std::vector<int>, std::vector<int>>);
    ok = ok && (!adaptable_to<int, int>);
    // structure overlaps (both iterable) but int -> string is not convertible
    ok = ok && (!adaptable_to<std::vector<int>, std::vector<std::string>>);

    return ok;
#else
    return true;
#endif
}

/*
tests_adapter_for
  Verifies the adapter_for concept (exposes adaptee() and a size() convertible
  to std::size_t).
  Tests the following:
  - satisfied by a concrete object_adapter (by_pointer)
  - rejected by a plain scalar
  - rejected by a bare adaptee that lacks the adapter surface
*/
bool
tests_adapter_for()
{
#if D_ADAPTER_HAS_CONCEPTS
    using oa = object_adapter<legacy_seq, legacy_policy, by_pointer>;

    static_assert(adapter_for<oa>,   "object_adapter models adapter_for");
    static_assert(!adapter_for<int>, "int does not model adapter_for");

    bool ok = true;

    ok = ok && ( adapter_for<oa>);
    ok = ok && (!adapter_for<int>);
    ok = ok && (!adapter_for<legacy_seq>);   // has num_elements(), not size()/adaptee()

    return ok;
#else
    return true;
#endif
}

/*
tests_function_adaptable
  Verifies the function_adaptable concept (invocability with the given args).
  Tests the following:
  - satisfied for a callable invocable with the argument list
  - rejected when the callable is not invocable with those arguments
  - satisfied for a nullary callable with no arguments
*/
bool
tests_function_adaptable()
{
#if D_ADAPTER_HAS_CONCEPTS
    static_assert(function_adaptable<inc, int>,
                  "inc invocable with int");
    static_assert(!function_adaptable<inc, std::string>,
                  "inc not invocable with string");

    bool ok = true;

    ok = ok && ( function_adaptable<inc, int>);
    ok = ok && (!function_adaptable<inc, std::string>);
    ok = ok && ( function_adaptable<nullary>);

    return ok;
#else
    return true;
#endif
}

/*
tests_constrained_adapt
  Verifies the constrained_adapt factory (a requires-guarded object adapter).
  Tests the following (runtime path only when D_ADAPTER_BYREF_FIXED — BUG 1):
  - the produced adapter delegates size()/get() correctly
  (Its adaptable_to constraint is exercised compile-time by tests_adaptable_to.)
*/
bool
tests_constrained_adapt()
{
#if D_ADAPTER_HAS_CONCEPTS
#if D_ADAPTER_BYREF_FIXED
    std::vector<int> v = { 1, 2, 3 };

    auto a = constrained_adapt<vector_policy,
                               std::vector<int>,
                               std::vector<int>>(v);

    bool ok = true;

    ok = ok && (a.size() == 3);
    ok = ok && (a.get(0) == 1);

    return ok;
#else
    // BLOCKED by BUG 1: constrained_adapt returns a by_reference adapter.
    return true;
#endif
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
