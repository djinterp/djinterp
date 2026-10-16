/*******************************************************************************
* djinterp [test]                              buffer_tests_policy_selection.cpp
*
*   Unit tests for the compile-time policy-selection traits:
*     - select_growth_policy<_Strategy>::type / select_growth_policy_t
*       maps each buffer_growth_strategy enumerator to the
*       corresponding growth policy type.
*     - select_cursor_policy<_Model>::type / select_cursor_policy_t
*       maps each buffer_cursor_model enumerator to the
*       corresponding cursor policy type.
*
*   Because every mapping is decided at compile time, each case is
* expressed twice: once as a static_assert (the real contract) and
* once as a runtime record_assertion call so the suite still emits
* a labeled, observable result for the report.
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests_policy_selection.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "buffer_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_select_growth_policy
  Covers select_growth_policy and its alias select_growth_policy_t.
  Tests the following:
  - buffer_growth_strategy::none         -> fixed_growth_policy
  - buffer_growth_strategy::linear       -> linear_growth_policy<>
  - buffer_growth_strategy::exponential  -> exponential_growth_policy
  - buffer_growth_strategy::page_aligned -> page_growth_policy<>
  - select_growth_policy_t<S> is identical to
    typename select_growth_policy<S>::type for every strategy
  - the selected policy carries the matching ::strategy constant
*/
void
tests_select_growth_policy(
    test_handler& _handler
)
{
    // --- none -> fixed_growth_policy ---
    {
        using selected =
            typename select_growth_policy<
                buffer_growth_strategy::none>::type;

        static_assert(
            std::is_same<selected, fixed_growth_policy>::value,
            "select_growth_policy<none>::type must be fixed_growth_policy");

        record_assertion(
            _handler,
            (std::is_same<selected, fixed_growth_policy>::value),
            "select_growth_policy<none>::type == fixed_growth_policy");

        record_assertion(
            _handler,
            selected::strategy == buffer_growth_strategy::none,
            "select_growth_policy<none>::type::strategy == none");

        record_assertion(
            _handler,
            selected::can_grow == false,
            "select_growth_policy<none>::type::can_grow == false");
    }

    // --- linear -> linear_growth_policy<> ---
    {
        using selected =
            typename select_growth_policy<
                buffer_growth_strategy::linear>::type;

        static_assert(
            std::is_same<selected, linear_growth_policy<>>::value,
            "select_growth_policy<linear>::type must be linear_growth_policy<>");

        record_assertion(
            _handler,
            (std::is_same<selected, linear_growth_policy<>>::value),
            "select_growth_policy<linear>::type == linear_growth_policy<>");

        record_assertion(
            _handler,
            selected::strategy == buffer_growth_strategy::linear,
            "select_growth_policy<linear>::type::strategy == linear");

        record_assertion(
            _handler,
            selected::can_grow == true,
            "select_growth_policy<linear>::type::can_grow == true");

        // default increment of linear_growth_policy<> is 4096
        record_assertion(
            _handler,
            selected::increment == 4096,
            "select_growth_policy<linear>::type::increment == 4096");
    }

    // --- exponential -> exponential_growth_policy ---
    {
        using selected =
            typename select_growth_policy<
                buffer_growth_strategy::exponential>::type;

        static_assert(
            std::is_same<selected, exponential_growth_policy>::value,
            "select_growth_policy<exponential>::type must be "
            "exponential_growth_policy");

        record_assertion(
            _handler,
            (std::is_same<selected, exponential_growth_policy>::value),
            "select_growth_policy<exponential>::type == exponential_growth_policy");

        record_assertion(
            _handler,
            selected::strategy == buffer_growth_strategy::exponential,
            "select_growth_policy<exponential>::type::strategy == exponential");

        record_assertion(
            _handler,
            selected::can_grow == true,
            "select_growth_policy<exponential>::type::can_grow == true");

        record_assertion(
            _handler,
            selected::min_capacity == 64,
            "select_growth_policy<exponential>::type::min_capacity == 64");
    }

    // --- page_aligned -> page_growth_policy<> ---
    {
        using selected =
            typename select_growth_policy<
                buffer_growth_strategy::page_aligned>::type;

        static_assert(
            std::is_same<selected, page_growth_policy<>>::value,
            "select_growth_policy<page_aligned>::type must be "
            "page_growth_policy<>");

        record_assertion(
            _handler,
            (std::is_same<selected, page_growth_policy<>>::value),
            "select_growth_policy<page_aligned>::type == page_growth_policy<>");

        record_assertion(
            _handler,
            selected::strategy == buffer_growth_strategy::page_aligned,
            "select_growth_policy<page_aligned>::type::strategy == page_aligned");

        record_assertion(
            _handler,
            selected::can_grow == true,
            "select_growth_policy<page_aligned>::type::can_grow == true");

        // default page size is 4096
        record_assertion(
            _handler,
            selected::page_size == 4096,
            "select_growth_policy<page_aligned>::type::page_size == 4096");
    }

    // --- alias parity: select_growth_policy_t<S> == typename ::type ---
    {
        // none
        static_assert(
            std::is_same<
                select_growth_policy_t<buffer_growth_strategy::none>,
                typename select_growth_policy<
                    buffer_growth_strategy::none>::type>::value,
            "select_growth_policy_t<none> must alias ::type");

        record_assertion(
            _handler,
            (std::is_same<
                select_growth_policy_t<buffer_growth_strategy::none>,
                typename select_growth_policy<
                    buffer_growth_strategy::none>::type>::value),
            "select_growth_policy_t<none> aliases ::type");

        // linear
        static_assert(
            std::is_same<
                select_growth_policy_t<buffer_growth_strategy::linear>,
                typename select_growth_policy<
                    buffer_growth_strategy::linear>::type>::value,
            "select_growth_policy_t<linear> must alias ::type");

        record_assertion(
            _handler,
            (std::is_same<
                select_growth_policy_t<buffer_growth_strategy::linear>,
                typename select_growth_policy<
                    buffer_growth_strategy::linear>::type>::value),
            "select_growth_policy_t<linear> aliases ::type");

        // exponential
        static_assert(
            std::is_same<
                select_growth_policy_t<buffer_growth_strategy::exponential>,
                typename select_growth_policy<
                    buffer_growth_strategy::exponential>::type>::value,
            "select_growth_policy_t<exponential> must alias ::type");

        record_assertion(
            _handler,
            (std::is_same<
                select_growth_policy_t<buffer_growth_strategy::exponential>,
                typename select_growth_policy<
                    buffer_growth_strategy::exponential>::type>::value),
            "select_growth_policy_t<exponential> aliases ::type");

        // page_aligned
        static_assert(
            std::is_same<
                select_growth_policy_t<buffer_growth_strategy::page_aligned>,
                typename select_growth_policy<
                    buffer_growth_strategy::page_aligned>::type>::value,
            "select_growth_policy_t<page_aligned> must alias ::type");

        record_assertion(
            _handler,
            (std::is_same<
                select_growth_policy_t<buffer_growth_strategy::page_aligned>,
                typename select_growth_policy<
                    buffer_growth_strategy::page_aligned>::type>::value),
            "select_growth_policy_t<page_aligned> aliases ::type");
    }

    // --- distinctness: no two enumerators map to the same policy ---
    {
        static_assert(
            !std::is_same<
                select_growth_policy_t<buffer_growth_strategy::none>,
                select_growth_policy_t<buffer_growth_strategy::linear>>::value,
            "none and linear must map to distinct policies");

        static_assert(
            !std::is_same<
                select_growth_policy_t<buffer_growth_strategy::linear>,
                select_growth_policy_t<buffer_growth_strategy::exponential>>::value,
            "linear and exponential must map to distinct policies");

        static_assert(
            !std::is_same<
                select_growth_policy_t<buffer_growth_strategy::exponential>,
                select_growth_policy_t<buffer_growth_strategy::page_aligned>>::value,
            "exponential and page_aligned must map to distinct policies");

        static_assert(
            !std::is_same<
                select_growth_policy_t<buffer_growth_strategy::none>,
                select_growth_policy_t<buffer_growth_strategy::page_aligned>>::value,
            "none and page_aligned must map to distinct policies");

        record_assertion(
            _handler,
            (!std::is_same<
                select_growth_policy_t<buffer_growth_strategy::none>,
                select_growth_policy_t<buffer_growth_strategy::linear>>::value),
            "select_growth_policy: none vs linear are distinct");

        record_assertion(
            _handler,
            (!std::is_same<
                select_growth_policy_t<buffer_growth_strategy::linear>,
                select_growth_policy_t<buffer_growth_strategy::exponential>>::value),
            "select_growth_policy: linear vs exponential are distinct");

        record_assertion(
            _handler,
            (!std::is_same<
                select_growth_policy_t<buffer_growth_strategy::exponential>,
                select_growth_policy_t<buffer_growth_strategy::page_aligned>>::value),
            "select_growth_policy: exponential vs page_aligned are distinct");

        record_assertion(
            _handler,
            (!std::is_same<
                select_growth_policy_t<buffer_growth_strategy::none>,
                select_growth_policy_t<buffer_growth_strategy::page_aligned>>::value),
            "select_growth_policy: none vs page_aligned are distinct");
    }

    return;
}


/*
tests_select_cursor_policy
  Covers select_cursor_policy and its alias select_cursor_policy_t.
  Tests the following:
  - buffer_cursor_model::write_only -> write_only_cursor_policy
  - buffer_cursor_model::dual       -> dual_cursor_policy
  - select_cursor_policy_t<M> is identical to
    typename select_cursor_policy<M>::type for both models
  - the selected policy carries the matching ::model constant
  - the two cursor models map to distinct, non-equivalent policy
    types
*/
void
tests_select_cursor_policy(
    test_handler& _handler
)
{
    // --- write_only -> write_only_cursor_policy ---
    {
        using selected =
            typename select_cursor_policy<
                buffer_cursor_model::write_only>::type;

        static_assert(
            std::is_same<selected, write_only_cursor_policy>::value,
            "select_cursor_policy<write_only>::type must be "
            "write_only_cursor_policy");

        record_assertion(
            _handler,
            (std::is_same<selected, write_only_cursor_policy>::value),
            "select_cursor_policy<write_only>::type == write_only_cursor_policy");

        record_assertion(
            _handler,
            selected::model == buffer_cursor_model::write_only,
            "select_cursor_policy<write_only>::type::model == write_only");

        record_assertion(
            _handler,
            selected::has_read_cursor == false,
            "select_cursor_policy<write_only>::type::has_read_cursor == false");
    }

    // --- dual -> dual_cursor_policy ---
    {
        using selected =
            typename select_cursor_policy<
                buffer_cursor_model::dual>::type;

        static_assert(
            std::is_same<selected, dual_cursor_policy>::value,
            "select_cursor_policy<dual>::type must be dual_cursor_policy");

        record_assertion(
            _handler,
            (std::is_same<selected, dual_cursor_policy>::value),
            "select_cursor_policy<dual>::type == dual_cursor_policy");

        record_assertion(
            _handler,
            selected::model == buffer_cursor_model::dual,
            "select_cursor_policy<dual>::type::model == dual");

        record_assertion(
            _handler,
            selected::has_read_cursor == true,
            "select_cursor_policy<dual>::type::has_read_cursor == true");
    }

    // --- alias parity: select_cursor_policy_t<M> == typename ::type ---
    {
        // write_only
        static_assert(
            std::is_same<
                select_cursor_policy_t<buffer_cursor_model::write_only>,
                typename select_cursor_policy<
                    buffer_cursor_model::write_only>::type>::value,
            "select_cursor_policy_t<write_only> must alias ::type");

        record_assertion(
            _handler,
            (std::is_same<
                select_cursor_policy_t<buffer_cursor_model::write_only>,
                typename select_cursor_policy<
                    buffer_cursor_model::write_only>::type>::value),
            "select_cursor_policy_t<write_only> aliases ::type");

        // dual
        static_assert(
            std::is_same<
                select_cursor_policy_t<buffer_cursor_model::dual>,
                typename select_cursor_policy<
                    buffer_cursor_model::dual>::type>::value,
            "select_cursor_policy_t<dual> must alias ::type");

        record_assertion(
            _handler,
            (std::is_same<
                select_cursor_policy_t<buffer_cursor_model::dual>,
                typename select_cursor_policy<
                    buffer_cursor_model::dual>::type>::value),
            "select_cursor_policy_t<dual> aliases ::type");
    }

    // --- distinctness: the two models map to different policies ---
    {
        static_assert(
            !std::is_same<
                select_cursor_policy_t<buffer_cursor_model::write_only>,
                select_cursor_policy_t<buffer_cursor_model::dual>>::value,
            "write_only and dual must map to distinct policies");

        record_assertion(
            _handler,
            (!std::is_same<
                select_cursor_policy_t<buffer_cursor_model::write_only>,
                select_cursor_policy_t<buffer_cursor_model::dual>>::value),
            "select_cursor_policy: write_only vs dual are distinct");
    }

    return;
}


NS_END  // test
NS_END  // djinterp
