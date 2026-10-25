/*******************************************************************************
* djinterp [net]                                           net_tests_sa_assert.h
*
* The assertion the net standalone suites count with.
*   The net foundation, TCP and HTTP suites need two things from a test
* framework: an assertion that tallies into a `struct d_test_counter` and
* reports a failure, and a way to zero that tally. This header declares both,
* so the suites depend neither on the retired standalone runner nor on the C
* test framework, which is being rewritten; `test_counter.h` supplies only the
* tally type.
*   The names are the suites' own. They do not reuse the retired runner's
* `d_assert_standalone` and `d_test_counter_reset`: a test-local definition
* under a framework name would collide with the framework's own at link time
* as soon as one binary held both.
*
*
* path:      /tests/djinterp/net/net_tests_sa_assert.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_NET_NET_TESTS_SA_ASSERT_H
#define DJINTERP_NET_NET_TESTS_SA_ASSERT_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"           // framework root
#include "../../../inc/djinterp/test/c/test_counter.h"  // struct d_test_counter


D_EXTERN_C_BEGIN


/**
 * @brief Counts one assertion, and reports it when it fails.
 *
 * @note A failure is printed as it happens, on its own line beneath the
 *       output of the test that made it. A pass prints nothing.
 *
 * @param[in]     _condition  the condition asserted.
 * @param[in]     _test_name  the test the assertion belongs to, printed with a
 *                            failure.
 * @param[in]     _message    what the assertion checks, printed with a
 *                            failure.
 * @param[in,out] _counter    the tally to count into, or `NULL` to count
 *                            nothing and print nothing.
 * @post With a counter, `assertions_total` has grown by one, and
 *       `assertions_passed` by one when `_condition` held.
 * @return `_condition`.
 */
bool d_tests_net_assert(bool                   _condition,
                        const char*            _test_name,
                        const char*            _message,
                        struct d_test_counter* _counter);

/**
 * @brief Zeroes every tally in a counter.
 *
 * @param[in,out] _counter  the counter to zero; `NULL` is ignored.
 */
void d_tests_net_counter_reset(struct d_test_counter* _counter);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_TESTS_SA_ASSERT_H
