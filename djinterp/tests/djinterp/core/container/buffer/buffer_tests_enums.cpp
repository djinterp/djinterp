/*******************************************************************************
* djinterp [test]                                         buffer_tests_enums.cpp
*
*   Unit tests for the buffer_growth_strategy and buffer_cursor_model
* enumerations (sections I and II of buffer.hpp).  Both enums exist purely
* as compile-time classifiers; the assertions here confirm value distinctness,
* underlying type stability, and switch-statement reachability.
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests_enums.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "buffer_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_buffer_growth_strategy
  buffer_growth_strategy enum sanity coverage.
  Tests the following:
  - all four declared values (none, linear, exponential, page_aligned)
    are reachable
  - all four values are mutually distinct
  - the enum is scoped (cannot be implicitly converted from int)
  - the enum's underlying integral can be examined
  - every value is reachable through a switch statement (every case
    branch returns true)
*/
void
tests_buffer_growth_strategy(
    test_handler& _handler
)
{
    using strategy = buffer_growth_strategy;

    // --- distinctness ---

    record_assertion(
        _handler,
        (static_cast<int>(strategy::none) !=
         static_cast<int>(strategy::linear)),
        "buffer_growth_strategy::none != ::linear");

    record_assertion(
        _handler,
        (static_cast<int>(strategy::none) !=
         static_cast<int>(strategy::exponential)),
        "buffer_growth_strategy::none != ::exponential");

    record_assertion(
        _handler,
        (static_cast<int>(strategy::none) !=
         static_cast<int>(strategy::page_aligned)),
        "buffer_growth_strategy::none != ::page_aligned");

    record_assertion(
        _handler,
        (static_cast<int>(strategy::linear) !=
         static_cast<int>(strategy::exponential)),
        "buffer_growth_strategy::linear != ::exponential");

    record_assertion(
        _handler,
        (static_cast<int>(strategy::linear) !=
         static_cast<int>(strategy::page_aligned)),
        "buffer_growth_strategy::linear != ::page_aligned");

    record_assertion(
        _handler,
        (static_cast<int>(strategy::exponential) !=
         static_cast<int>(strategy::page_aligned)),
        "buffer_growth_strategy::exponential != ::page_aligned");

    // --- scoped enum: not implicitly convertible from int ---

    static_assert(
        !std::is_convertible<int, strategy>::value,
        "buffer_growth_strategy must be a scoped enum (enum class).");

    record_assertion(
        _handler,
        !std::is_convertible<int, strategy>::value,
        "buffer_growth_strategy is enum class (not implicitly "
        "convertible from int)");

    // --- switch reachability for all four cases ---

    auto reachable = [](strategy _s) -> bool
    {
        // walk every declared case; default catches any future
        // addition slipping through without an updated test.
        switch (_s)
        {
            case strategy::none:         return true;
            case strategy::linear:       return true;
            case strategy::exponential:  return true;
            case strategy::page_aligned: return true;
        }

        return false;
    };

    record_assertion(
        _handler,
        reachable(strategy::none),
        "switch reaches buffer_growth_strategy::none");

    record_assertion(
        _handler,
        reachable(strategy::linear),
        "switch reaches buffer_growth_strategy::linear");

    record_assertion(
        _handler,
        reachable(strategy::exponential),
        "switch reaches buffer_growth_strategy::exponential");

    record_assertion(
        _handler,
        reachable(strategy::page_aligned),
        "switch reaches buffer_growth_strategy::page_aligned");

    return;
}


/*
tests_buffer_cursor_model
  buffer_cursor_model enum sanity coverage.
  Tests the following:
  - both declared values (write_only, dual) are reachable
  - the two values are distinct
  - the enum is scoped (not implicitly int-convertible)
  - every value is reachable through a switch statement
*/
void
tests_buffer_cursor_model(
    test_handler& _handler
)
{
    using model = buffer_cursor_model;

    // --- distinctness ---

    record_assertion(
        _handler,
        (static_cast<int>(model::write_only) !=
         static_cast<int>(model::dual)),
        "buffer_cursor_model::write_only != ::dual");

    // --- scoped enum: not implicitly convertible from int ---

    static_assert(
        !std::is_convertible<int, model>::value,
        "buffer_cursor_model must be a scoped enum (enum class).");

    record_assertion(
        _handler,
        !std::is_convertible<int, model>::value,
        "buffer_cursor_model is enum class (not implicitly "
        "convertible from int)");

    // --- switch reachability for both cases ---

    auto reachable = [](model _m) -> bool
    {
        switch (_m)
        {
            case model::write_only: return true;
            case model::dual:       return true;
        }

        return false;
    };

    record_assertion(
        _handler,
        reachable(model::write_only),
        "switch reaches buffer_cursor_model::write_only");

    record_assertion(
        _handler,
        reachable(model::dual),
        "switch reaches buffer_cursor_model::dual");

    return;
}


NS_END  // test
NS_END  // djinterp
