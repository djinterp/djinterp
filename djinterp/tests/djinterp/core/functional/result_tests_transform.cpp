/******************************************************************************
* djinterp [test]                                   result_tests_transform.cpp
*
* Transform / fluent method tests for result<T, E> (section I methods that
* map or thread the active branch).
*
*   test_map      -- value-side map: applies on ok, propagates error on err,
*                    incl. type-changing map.
*   test_map_err  -- error-side map: applies on err, propagates value on ok,
*                    incl. type-changing map.
*   test_and_then -- monadic bind on ok; err short-circuits without invoking;
*                    inner err propagates.
*   test_or_else  -- ok passes through; err invokes recovery (to ok or to a
*                    different err).
*   test_match    -- pattern-match dispatch across ok / err arms.
******************************************************************************/

#include <string>
#include <cstddef>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


void test_map(test::test_handler& _h)
{
    // ok: function applied
    ri ok_in = ok<int, std::string>(5);
    result<int, std::string> doubled = ok_in.map(fn_double());
    test::record_assertion(_h, doubled.is_ok() && doubled.value() == 10,
        "map: applies to the ok value");

    // err: error propagates unchanged, value type still changes
    ri err_in = err<int, std::string>("bad");
    result<int, std::string> mapped_err = err_in.map(fn_double());
    test::record_assertion(_h,
        mapped_err.is_err() && mapped_err.error() == "bad",
        "map: propagates the error unchanged");

    // type-changing map (int -> std::string)
    result<std::string, std::string> as_str = ok_in.map(fn_to_string());
    test::record_assertion(_h, as_str.is_ok() && as_str.value() == "5",
        "map: changes the value type");

    return;
}


void test_map_err(test::test_handler& _h)
{
    // err: function applied, type-changing (std::string -> std::size_t)
    ri err_in = err<int, std::string>("hello");
    result<int, std::size_t> mapped = err_in.map_err(fn_err_len());
    test::record_assertion(_h, mapped.is_err() && mapped.error() == 5u,
        "map_err: transforms the error side");

    // ok: value propagates unchanged
    ri ok_in = ok<int, std::string>(3);
    result<int, std::size_t> ok_through = ok_in.map_err(fn_err_len());
    test::record_assertion(_h, ok_through.is_ok() && ok_through.value() == 3,
        "map_err: propagates the ok value unchanged");

    // same-type error transform
    result<int, std::string> tagged = err_in.map_err(fn_err_prefix());
    test::record_assertion(_h, tagged.is_err() && tagged.error() == "E:hello",
        "map_err: same-type error transform");

    return;
}


void test_and_then(test::test_handler& _h)
{
    // ok + inner ok
    ri even = ok<int, std::string>(8);
    ri r1 = even.and_then(fn_safe_halve());
    test::record_assertion(_h, r1.is_ok() && r1.value() == 4,
        "and_then: ok threads through to inner ok");

    // ok + inner err
    ri odd = ok<int, std::string>(7);
    ri r2 = odd.and_then(fn_safe_halve());
    test::record_assertion(_h, r2.is_err() && r2.error() == "odd",
        "and_then: inner err propagates");

    // err short-circuits (function not invoked, original error preserved)
    ri start_err = err<int, std::string>("start");
    ri r3 = start_err.and_then(fn_safe_halve());
    test::record_assertion(_h, r3.is_err() && r3.error() == "start",
        "and_then: err short-circuits with original error");

    return;
}


void test_or_else(test::test_handler& _h)
{
    // ok passes through unchanged (recovery not consulted)
    ri ok_in = ok<int, std::string>(5);
    ri r1 = ok_in.or_else(fn_recover_zero());
    test::record_assertion(_h, r1.is_ok() && r1.value() == 5,
        "or_else: ok passes through unchanged");

    // err recovers to ok
    ri err_in = err<int, std::string>("fail");
    ri r2 = err_in.or_else(fn_recover_zero());
    test::record_assertion(_h, r2.is_ok() && r2.value() == 0,
        "or_else: err recovers to an ok value");

    // err maps to a different err
    ri r3 = err_in.or_else(fn_rewrap_err());
    test::record_assertion(_h, r3.is_err() && r3.error() == "wrapped:fail",
        "or_else: err can map to a different error");

    return;
}


void test_match(test::test_handler& _h)
{
    // ok arm
    ri          ok_in = ok<int, std::string>(42);
    std::string a = ok_in.match(fn_ok_show(), fn_err_show());
    test::record_assertion(_h, a == "ok:42",
        "match: ok dispatches to the ok arm");

    // err arm
    ri          err_in = err<int, std::string>("nope");
    std::string b = err_in.match(fn_ok_show(), fn_err_show());
    test::record_assertion(_h, b == "err:nope",
        "match: err dispatches to the err arm");

    return;
}


NS_END  // testing
NS_END  // djinterp
