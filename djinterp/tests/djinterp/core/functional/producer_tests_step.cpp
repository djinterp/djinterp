/******************************************************************************
* djinterp [functional]                               producer_tests_step.cpp
*
*   Tests for the producer step type and its builders: producer_step<T>,
* make_step, and no_step.
*
* path:      /src/functional/producer_tests_step.cpp
******************************************************************************/

#include "./producer_tests.hpp"

#include <string>
#include <utility>


NS_DJINTERP
NS_TESTING


/*
test_producer_step
  Tests producer_step<T>, make_step, and no_step.
  Tests the following:
  - the default constructor yields an empty step (has_value == false)
  - the const-lvalue value constructor copies and sets has_value
  - the rvalue value constructor moves and sets has_value
  - make_step deduces and decays its argument (lvalue and rvalue forms)
  - make_step on a string moves the contents into the step
  - no_step<T> yields an empty step of the requested element type
  - producer_step works for non-trivial element types (std::string,
    std::pair) as well as scalars
*/
std::size_t
test_producer_step(
    test_registry& _reg
)
{
    std::size_t before;

    before = _reg.failures();

    // ---- default construction: empty step ----
    {
        producer_step<int> s;

        D_TESTING_CHECK(_reg, s.has_value == false);
    }

    // ---- const-lvalue value construction ----
    {
        int                v = 42;
        producer_step<int> s(v);

        D_TESTING_CHECK(_reg, s.has_value == true);
        D_TESTING_CHECK(_reg, s.value == 42);
    }

    // ---- rvalue value construction (move) ----
    {
        producer_step<std::string> s(std::string("hello"));

        D_TESTING_CHECK(_reg, s.has_value == true);
        D_TESTING_CHECK(_reg, s.value == "hello");
    }

    // ---- make_step from an lvalue (decays to value) ----
    {
        int  v = 7;
        producer_step<int> s = make_step(v);

        D_TESTING_CHECK(_reg, s.has_value == true);
        D_TESTING_CHECK(_reg, s.value == 7);
    }

    // ---- make_step from an rvalue ----
    {
        producer_step<int> s = make_step(99);

        D_TESTING_CHECK(_reg, s.has_value == true);
        D_TESTING_CHECK(_reg, s.value == 99);
    }

    // ---- make_step moves a string's contents ----
    {
        std::string                src = "movable";
        producer_step<std::string> s   = make_step(std::move(src));

        D_TESTING_CHECK(_reg, s.has_value == true);
        D_TESTING_CHECK(_reg, s.value == "movable");
    }

    // ---- no_step yields an empty step ----
    {
        producer_step<int> s = no_step<int>();

        D_TESTING_CHECK(_reg, s.has_value == false);
    }

    // ---- no_step for a non-trivial element type ----
    {
        producer_step<std::string> s = no_step<std::string>();

        D_TESTING_CHECK(_reg, s.has_value == false);
    }

    // ---- pair-valued step round-trips both members ----
    {
        producer_step<std::pair<int, int> > s =
            make_step(std::make_pair(3, 4));

        D_TESTING_CHECK(_reg, s.has_value == true);
        D_TESTING_CHECK(_reg, s.value.first == 3);
        D_TESTING_CHECK(_reg, s.value.second == 4);
    }

    return (_reg.failures() - before);
}


NS_END  // testing
NS_END  // djinterp
