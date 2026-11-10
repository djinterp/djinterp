/*******************************************************************************
* djinterp [functional]                                  producer_tests_step.cpp
*
*   Tests for the producer step type and its builders: DProducerStep<T>,
* make_step, and no_step.
*
*
* path:      /tests/djinterp/cpp/functional/producer_tests_step.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.02
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <string>
#include <utility>
// djinterp
#include "./producer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_producer_step
  Tests DProducerStep<T>, make_step, and no_step.
  Tests the following:
  - the default constructor yields an empty step (has_value == false)
  - the const-lvalue value constructor copies and sets has_value
  - the rvalue value constructor moves and sets has_value
  - make_step deduces and decays its argument (lvalue and rvalue forms)
  - make_step on a string moves the contents into the step
  - no_step<T> yields an empty step of the requested element type
  - DProducerStep works for non-trivial element types (std::string,
    std::pair) as well as scalars
*/
bool test_producer_step()
{
    bool verdict = true;


    // ---- default construction: empty step ----
    {
        DProducerStep<int> s;

        D_PRODUCER_CHECK(s.has_value == false);
    }

    // ---- const-lvalue value construction ----
    {
        int                v = 42;
        DProducerStep<int> s(v);

        D_PRODUCER_CHECK(s.has_value == true);
        D_PRODUCER_CHECK(s.value == 42);
    }

    // ---- rvalue value construction (move) ----
    {
        DProducerStep<std::string> s(std::string("hello"));

        D_PRODUCER_CHECK(s.has_value == true);
        D_PRODUCER_CHECK(s.value == "hello");
    }

    // ---- make_step from an lvalue (decays to value) ----
    {
        int  v = 7;
        DProducerStep<int> s = make_step(v);

        D_PRODUCER_CHECK(s.has_value == true);
        D_PRODUCER_CHECK(s.value == 7);
    }

    // ---- make_step from an rvalue ----
    {
        DProducerStep<int> s = make_step(99);

        D_PRODUCER_CHECK(s.has_value == true);
        D_PRODUCER_CHECK(s.value == 99);
    }

    // ---- make_step moves a string's contents ----
    {
        std::string                src = "movable";
        DProducerStep<std::string> s   = make_step(std::move(src));

        D_PRODUCER_CHECK(s.has_value == true);
        D_PRODUCER_CHECK(s.value == "movable");
    }

    // ---- no_step yields an empty step ----
    {
        DProducerStep<int> s = no_step<int>();

        D_PRODUCER_CHECK(s.has_value == false);
    }

    // ---- no_step for a non-trivial element type ----
    {
        DProducerStep<std::string> s = no_step<std::string>();

        D_PRODUCER_CHECK(s.has_value == false);
    }

    // ---- pair-valued step round-trips both members ----
    {
        DProducerStep<std::pair<int, int> > s =
            make_step(std::make_pair(3, 4));

        D_PRODUCER_CHECK(s.has_value == true);
        D_PRODUCER_CHECK(s.value.first == 3);
        D_PRODUCER_CHECK(s.value.second == 4);
    }

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
