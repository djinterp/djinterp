/******************************************************************************
* djinterp [test]                                    dtuple_tests_utilities.cpp
*
*   Unit tests for the tuple-utilities section of dtuple.hpp:
*     - tuple_to_pack  (runtime apply / expand into a callable)
*
*   tuple_to_pack expands the elements of its tuple input into the
* argument list of the supplied callable.  Tests verify:
*     - empty tuple input: the callable is called with no arguments
*     - single-element tuple: one positional argument
*     - multi-element tuple: each element passed in order
*     - heterogeneous tuple: each element's type is preserved
*     - lvalue and rvalue tuple inputs both forward correctly
*     - the callable can be either a function pointer, a stateful
*       lambda, or a std::function-like object (we use lambdas here)
*
*   The expansion is observed by capturing argument values inside the
* callable; the test then compares the captured values against the
* expected sequence.
*
*
* path:      /inc/djinterp/test/dtuple_tests_utilities.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include <string>

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   COMPILE-TIME SHAPE SANITY
// =========================================================================
//   tuple_to_pack itself is a runtime function -- there is no metafunction
// here to static_assert against.  What we CAN check at compile time is
// that the helper is callable in all expected shapes (the file simply
// failing to compile is the sentinel).  These dummy instantiations live
// in an unevaluated context so they cost nothing at link time.

namespace
{
    template<typename _Tuple, typename _Fn>
    auto sfinae_probe(int) -> decltype(tuple_to_pack(std::declval<_Tuple>(),
                                                     std::declval<_Fn>()),
                                       std::true_type{});

    template<typename, typename>
    auto sfinae_probe(...) -> std::false_type;

    // each instantiation forces the compiler to fully type-check a call
    // shape we expect to be valid
    static_assert(decltype(sfinae_probe<std::tuple<>, void(*)()>(0))::value,
                  "tuple_to_pack should accept an empty tuple and a nullary callable");
    static_assert(decltype(sfinae_probe<std::tuple<int>, void(*)(int)>(0))::value,
                  "tuple_to_pack should accept a one-element tuple and a unary callable");
    static_assert(decltype(sfinae_probe<std::tuple<int, char>, void(*)(int, char)>(0))::value,
                  "tuple_to_pack should accept a two-element tuple and a binary callable");
}


// =========================================================================
// II.  RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_utilities_all(
    test_handler& _test_handler
)
{
    // ---- empty tuple: callable is invoked with zero args ----
    {
        bool called = false;

        tuple_to_pack(std::tuple<>{}, [&called]() { called = true; });

        record_assertion(_test_handler, 
            called,
            "tuple_to_pack: empty tuple invokes nullary callable");
    }

    // ---- single-element tuple ----
    {
        int captured = 0;

        tuple_to_pack(std::tuple<int>{42},
                      [&captured](int _a) { captured = _a; });

        record_assertion(_test_handler, 
            (captured == 42),
            "tuple_to_pack: single-element tuple forwards the value");
    }

    // ---- two-element heterogeneous tuple ----
    {
        int  cap_int  = 0;
        char cap_char = '\0';

        tuple_to_pack(std::tuple<int, char>{7, 'k'},
                      [&cap_int, &cap_char](int _a, char _b)
                      {
                          cap_int  = _a;
                          cap_char = _b;
                      });

        record_assertion(_test_handler, 
            ((cap_int == 7) && (cap_char == 'k')),
            "tuple_to_pack: two-element heterogeneous tuple expanded positionally");
    }

    // ---- three-element tuple, mixed numeric types ----
    {
        int    cap_a = 0;
        double cap_b = 0.0;
        long   cap_c = 0;

        tuple_to_pack(std::tuple<int, double, long>{1, 2.5, 99L},
                      [&cap_a, &cap_b, &cap_c](int _a, double _b, long _c)
                      {
                          cap_a = _a;
                          cap_b = _b;
                          cap_c = _c;
                      });

        record_assertion(_test_handler, 
            ((cap_a == 1) && (cap_b == 2.5) && (cap_c == 99L)),
            "tuple_to_pack: three-element tuple forwards each value in order");
    }

    // ---- five-element tuple, ensures get<_I> indices go beyond 4 ----
    {
        int sum = 0;

        tuple_to_pack(std::tuple<int, int, int, int, int>{1, 2, 3, 4, 5},
                      [&sum](int _a, int _b, int _c, int _d, int _e)
                      {
                          sum = _a + _b + _c + _d + _e;
                      });

        record_assertion(_test_handler, 
            (sum == 15),
            "tuple_to_pack: five-element tuple visits every index");
    }

    // ---- non-trivial element type (std::string) ----
    {
        std::string cap_first;
        std::string cap_second;

        std::tuple<std::string, std::string> ss{
            std::string("hello"),
            std::string("world")
        };

        tuple_to_pack(ss,
                      [&cap_first, &cap_second](const std::string& _a,
                                                const std::string& _b)
                      {
                          cap_first  = _a;
                          cap_second = _b;
                      });

        record_assertion(_test_handler, 
            ((cap_first == "hello") && (cap_second == "world")),
            "tuple_to_pack: non-trivial elements (std::string) forwarded");
    }

    // ---- rvalue tuple input ----
    {
        int captured = 0;

        tuple_to_pack(std::tuple<int, int>{10, 20},
                      [&captured](int _a, int _b) { captured = _a + _b; });

        record_assertion(_test_handler, 
            (captured == 30),
            "tuple_to_pack: rvalue tuple input forwarded correctly");
    }

    // ---- lvalue tuple input is not consumed (still readable) ----
    {
        std::tuple<int, int> lv{4, 5};
        int captured = 0;

        tuple_to_pack(lv,
                      [&captured](int _a, int _b) { captured = _a * _b; });

        record_assertion(_test_handler, 
            (captured == 20),
            "tuple_to_pack: lvalue tuple input forwarded");
        record_assertion(_test_handler, 
            ((std::get<0>(lv) == 4) && (std::get<1>(lv) == 5)),
            "tuple_to_pack: lvalue tuple values intact after the call");
    }

    // ---- stateful callable invoked once ----
    {
        int call_count = 0;
        int sum        = 0;

        auto add_three = [&call_count, &sum](int _a, int _b, int _c)
        {
            ++call_count;
            sum = _a + _b + _c;
        };

        tuple_to_pack(std::tuple<int, int, int>{2, 4, 6}, add_three);

        record_assertion(_test_handler, 
            (call_count == 1),
            "tuple_to_pack: callable invoked exactly once");
        record_assertion(_test_handler, 
            (sum == 12),
            "tuple_to_pack: stateful lambda observed correct args");
    }

    return;
}


NS_END  // testing
NS_END  // djinterp
