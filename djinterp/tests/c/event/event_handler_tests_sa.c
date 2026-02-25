#include "./event_handler_tests_sa.h"


/*
d_tests_sa_event_handler_all
  Module-level aggregation function that runs all event_handler tests.
  Executes tests for all categories:
  - Creation and destruction
  - Listener management (bind, unbind, get, enable/disable)
  - Event operations (fire, queue, process)
  - Query functions (listener_count, enabled_count, pending_events)
  - Integration, stress, and edge cases
*/
bool
d_tests_sa_event_handler_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_event_handler_creation_all(_counter)      && result;
    result = d_tests_sa_event_handler_listener_mgmt_all(_counter) && result;
    result = d_tests_sa_event_handler_event_ops_all(_counter)     && result;
    result = d_tests_sa_event_handler_query_all(_counter)         && result;
    result = d_tests_sa_event_handler_advanced_all(_counter)      && result;

    return result;
}
