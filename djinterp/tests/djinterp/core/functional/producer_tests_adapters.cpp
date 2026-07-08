/******************************************************************************
* djinterp [functional]                          producer_tests_adapters.cpp
*
*   Tests for the producers that wrap another producer: take_n, drop_n,
* concat, interleave, transform, and filter. Also exercises the CRTP
* producer_base::collect() terminal that every helper inherits.
*
* path:      /src/functional/producer_tests_adapters.cpp
******************************************************************************/

#include "./producer_tests.hpp"

#include <vector>
#include <string>


NS_DJINTERP
NS_TESTING


/*
test_producer_adapters
  Tests the wrapping producers and the inherited collect() terminal.
  Tests the following:
  - take_n bounds an infinite producer to n values
  - take_n with n larger than the inner producer's length stops at the
    inner exhaustion (does not over-pull)
  - take_n(p, 0) yields nothing
  - drop_n skips the first n values then forwards the rest
  - drop_n with n >= the inner length yields nothing (and does not hang)
  - drop_n(p, 0) forwards everything
  - concat emits all of the first then all of the second producer
  - concat with an empty first (or second) behaves as identity
  - interleave alternates one pull from each, in first-second order
  - interleave stops as soon as either side exhausts (shorter side bounds)
  - transform applies the function to each value, including a type change
    (int -> std::string via a length-style map is avoided; we use doubling)
  - transform propagates exhaustion
  - filter keeps only matching values and drops the rest
  - filter that matches nothing yields an empty result
  - collect() on a non-adapter producer (range) works via the CRTP base
*/
void
test_producer_adapters(
    test::test_handler& _h
)
{

    // ---- take_n bounds an infinite producer ----
    {
        std::vector<int> got = take_n(repeat(1), 4).collect();
        std::vector<int> want(4, 1);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- take_n with n > inner length stops at inner exhaustion ----
    {
        // range(0,3) yields 0,1,2 (length 3); ask for 10
        std::vector<int> got = take_n(range(0, 3), 10).collect();
        std::vector<int> want;
        want.push_back(0); want.push_back(1); want.push_back(2);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- take_n(p, 0) yields nothing ----
    {
        std::vector<int> got = take_n(repeat(5), 0).collect();

        D_TEST_CHECK(_h, got.empty());
    }

    // ---- drop_n skips the first n ----
    {
        // range 0..6 -> 0,1,2,3,4,5 ; drop 2 -> 2,3,4,5
        std::vector<int> got = drop_n(range(0, 6), 2).collect();
        std::vector<int> want;
        want.push_back(2); want.push_back(3);
        want.push_back(4); want.push_back(5);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- drop_n past the inner length yields nothing ----
    {
        std::vector<int> got = drop_n(range(0, 3), 10).collect();

        D_TEST_CHECK(_h, got.empty());
    }

    // ---- drop_n(p, 0) forwards everything ----
    {
        std::vector<int> got  = drop_n(range(0, 4), 0).collect();
        std::vector<int> want = range(0, 4).collect();

        D_TEST_CHECK(_h, got == want);
    }

    // ---- concat: 1,2 then 100,101 ----
    {
        std::vector<int> got = concat(range(1, 3), range(100, 102)).collect();
        std::vector<int> want;
        want.push_back(1); want.push_back(2);
        want.push_back(100); want.push_back(101);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- concat with empty first behaves as identity ----
    {
        std::vector<int> got  = concat(empty<int>(), range(1, 4)).collect();
        std::vector<int> want = range(1, 4).collect();

        D_TEST_CHECK(_h, got == want);
    }

    // ---- concat with empty second behaves as identity ----
    {
        std::vector<int> got  = concat(range(1, 4), empty<int>()).collect();
        std::vector<int> want = range(1, 4).collect();

        D_TEST_CHECK(_h, got == want);
    }

    // ---- interleave alternates first, second, first, second, ... ----
    {
        // first: 1,2,3 ; second: 10,20,30 -> 1,10,2,20,3,30
        std::vector<int> got =
            interleave(range(1, 4), range(10, 40, 10)).collect();
        std::vector<int> want;
        want.push_back(1); want.push_back(10);
        want.push_back(2); want.push_back(20);
        want.push_back(3); want.push_back(30);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- interleave stops when the first (turn-leading) side exhausts ----
    {
        // first: 1 (len 1) ; second: 10,20 -> 1,10, then first exhausts
        // collect stops at the first empty step; the leading side empties
        // on the third pull (turn back to first), so output is 1,10
        std::vector<int> got =
            interleave(single(1), range(10, 30, 10)).collect();
        std::vector<int> want;
        want.push_back(1); want.push_back(10);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- transform doubles each value ----
    {
        std::vector<int> got = transform(range(1, 4), times_two()).collect();
        std::vector<int> want;
        want.push_back(2); want.push_back(4); want.push_back(6);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- transform changes the value type (int -> std::string) ----
    {
        struct to_string_map
        {
            std::string
            operator()(int _v) const
            {
                // simple deterministic mapping without <sstream>
                return std::string(static_cast<std::size_t>(_v), 'a');
            }
        };

        std::vector<std::string> got =
            transform(range(1, 4), to_string_map()).collect();

        D_TEST_CHECK(_h, got.size() == 3);
        if (got.size() == 3)
        {
            D_TEST_CHECK(_h, got[0] == "a");
            D_TEST_CHECK(_h, got[1] == "aa");
            D_TEST_CHECK(_h, got[2] == "aaa");
        }
    }

    // ---- transform propagates exhaustion (empty in -> empty out) ----
    {
        std::vector<int> got =
            transform(empty<int>(), times_two()).collect();

        D_TEST_CHECK(_h, got.empty());
    }

    // ---- filter keeps only even values ----
    {
        // range 0..6 -> 0,1,2,3,4,5 ; even -> 0,2,4
        std::vector<int> got = filter(range(0, 6), is_even_pred()).collect();
        std::vector<int> want;
        want.push_back(0); want.push_back(2); want.push_back(4);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- filter that matches nothing yields empty ----
    {
        struct never
        {
            bool operator()(int) const { return false; }
        };

        std::vector<int> got = filter(range(0, 5), never()).collect();

        D_TEST_CHECK(_h, got.empty());
    }

    // ---- composed pipeline: range -> filter even -> transform x2 ----
    {
        // 0..10 -> evens 0,2,4,6,8 -> doubled 0,4,8,12,16
        std::vector<int> got =
            transform(filter(range(0, 10), is_even_pred()),
                      times_two()).collect();
        std::vector<int> want;
        want.push_back(0);  want.push_back(4);  want.push_back(8);
        want.push_back(12); want.push_back(16);

        D_TEST_CHECK(_h, got == want);
    }

    // ---- CRTP collect() on a plain (non-adapter) producer ----
    {
        std::vector<int> got = range(1, 4).collect();
        std::vector<int> want;
        want.push_back(1); want.push_back(2); want.push_back(3);

        D_TEST_CHECK(_h, got == want);
    }

    return;
}


NS_END  // testing
NS_END  // djinterp
