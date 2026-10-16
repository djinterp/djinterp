/*******************************************************************************
* djinterp [test]                                      buffer_tests_defaults.cpp
*
*   Unit tests for the default policy aliases declared in section
* VIII of buffer.hpp:
*     default_growth_policy = exponential_growth_policy
*     default_cursor_policy = dual_cursor_policy
*
*   The mappings are compile-time, so each check is anchored by a
* static_assert with std::is_same and mirrored by a runtime
* record_assertion call so the suite still reports a labeled
* result.
*
*   Beyond identity, this file also pins the cross-product
* expectations: the defaults compose into a fully-functional buffer
* (growable, with a read cursor), which is what concrete buffer
* implementations rely on when they declare template defaults.
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests_defaults.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "buffer_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_default_policy_aliases
  Covers the default_growth_policy and default_cursor_policy
  aliases.
  Tests the following:
  - default_growth_policy is exactly exponential_growth_policy
  - default_cursor_policy is exactly dual_cursor_policy
  - default_growth_policy::strategy == buffer_growth_strategy::exponential
  - default_cursor_policy::model    == buffer_cursor_model::dual
  - default_growth_policy::can_grow is true
  - default_cursor_policy::has_read_cursor is true
  - the default growth policy agrees with
    select_growth_policy_t<exponential>
  - the default cursor policy agrees with
    select_cursor_policy_t<dual>
  - a buffer instantiated with the defaults supports both write
    and read operations end-to-end
*/
void
tests_default_policy_aliases(
    test_handler& _handler
)
{
    // --- identity: default_growth_policy == exponential_growth_policy ---
    {
        static_assert(
            std::is_same<default_growth_policy,
                         exponential_growth_policy>::value,
            "default_growth_policy must be exponential_growth_policy");

        record_assertion(
            _handler,
            (std::is_same<default_growth_policy,
                          exponential_growth_policy>::value),
            "default_growth_policy == exponential_growth_policy");
    }

    // --- identity: default_cursor_policy == dual_cursor_policy ---
    {
        static_assert(
            std::is_same<default_cursor_policy,
                         dual_cursor_policy>::value,
            "default_cursor_policy must be dual_cursor_policy");

        record_assertion(
            _handler,
            (std::is_same<default_cursor_policy,
                          dual_cursor_policy>::value),
            "default_cursor_policy == dual_cursor_policy");
    }

    // --- constants exposed by the default growth policy ---
    {
        record_assertion(
            _handler,
            default_growth_policy::strategy
                == buffer_growth_strategy::exponential,
            "default_growth_policy::strategy == exponential");

        record_assertion(
            _handler,
            default_growth_policy::can_grow == true,
            "default_growth_policy::can_grow == true");

        record_assertion(
            _handler,
            default_growth_policy::min_capacity == 64,
            "default_growth_policy::min_capacity == 64");
    }

    // --- constants exposed by the default cursor policy ---
    {
        record_assertion(
            _handler,
            default_cursor_policy::model
                == buffer_cursor_model::dual,
            "default_cursor_policy::model == dual");

        record_assertion(
            _handler,
            default_cursor_policy::has_read_cursor == true,
            "default_cursor_policy::has_read_cursor == true");
    }

    // --- cross-check with select_growth_policy_t<exponential> ---
    {
        static_assert(
            std::is_same<
                default_growth_policy,
                select_growth_policy_t<
                    buffer_growth_strategy::exponential>>::value,
            "default_growth_policy must agree with "
            "select_growth_policy_t<exponential>");

        record_assertion(
            _handler,
            (std::is_same<
                default_growth_policy,
                select_growth_policy_t<
                    buffer_growth_strategy::exponential>>::value),
            "default_growth_policy == select_growth_policy_t<exponential>");
    }

    // --- cross-check with select_cursor_policy_t<dual> ---
    {
        static_assert(
            std::is_same<
                default_cursor_policy,
                select_cursor_policy_t<
                    buffer_cursor_model::dual>>::value,
            "default_cursor_policy must agree with "
            "select_cursor_policy_t<dual>");

        record_assertion(
            _handler,
            (std::is_same<
                default_cursor_policy,
                select_cursor_policy_t<
                    buffer_cursor_model::dual>>::value),
            "default_cursor_policy == select_cursor_policy_t<dual>");
    }

    // --- end-to-end: defaults compose into a usable buffer ---
    // The defaults are advertised as "sensible defaults for concrete
    // buffer implementations".  A meaningful self-check is that they
    // produce a buffer which can be written to, can grow, and can be
    // read from - i.e. they do not interact badly when composed.
    {
        const char src[] = "DEFAULTSCOMPOSE!";  // 16 bytes
        mock_buffer<default_growth_policy,
                    default_cursor_policy> b(8);  // tiny -> forces grow

        std::size_t n_w = b.write(src, 16);

        record_assertion(
            _handler,
            n_w == 16,
            "defaults: write of 16 bytes through grow returns 16");

        record_assertion(
            _handler,
            b.capacity() >= 16,
            "defaults: post-grow capacity satisfies the request");

        record_assertion(
            _handler,
            b.write_position() == 16,
            "defaults: write_position advances to 16");

        // The read surface is only available because the cursor
        // policy is dual; failing to compile here would mean the
        // default cursor policy lost has_read_cursor == true.
        record_assertion(
            _handler,
            b.readable() == 16,
            "defaults: readable() reports 16 unconsumed bytes");

        char        out[16];
        std::size_t n_r = b.read(out, 16);

        record_assertion(
            _handler,
            n_r == 16,
            "defaults: read returns 16");

        record_assertion(
            _handler,
            std::memcmp(out, src, 16) == 0,
            "defaults: round-trip payload matches byte-for-byte");

        record_assertion(
            _handler,
            b.read_position() == 16,
            "defaults: read_position advances to 16");

        record_assertion(
            _handler,
            b.readable() == 0,
            "defaults: readable() reports 0 after full consumption");
    }

    return;
}


NS_END  // test
NS_END  // djinterp
