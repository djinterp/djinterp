#include "./test_handler_tests_sa.h"


//=============================================================================
// GLOBAL EVENT CALLBACK TRACKERS
//=============================================================================

int g_event_setup_count = 0;
int g_event_start_count = 0;
int g_event_success_count = 0;
int g_event_failure_count = 0;
int g_event_end_count = 0;
int g_event_teardown_count = 0;

/*
reset_event_counters
  Resets all global event counters to zero.
  Used before each test to ensure clean state.
*/
void 
reset_event_counters
(
    void
)
{
    g_event_setup_count = 0;
    g_event_start_count = 0;
    g_event_success_count = 0;
    g_event_failure_count = 0;
    g_event_end_count = 0;
    g_event_teardown_count = 0;
}

//=============================================================================
// HELPER TEST FUNCTIONS
//=============================================================================

/*
handler_test_passing
  Simple test function that always returns success.
  Used for testing passing test execution behavior.

Parameter(s):
  _test: Test context (unused but required by signature)
Return:
  Always returns D_TEST_PASS (true)
*/
bool 
handler_test_passing
(
    struct d_test* _test
)
{
    return D_TEST_PASS;
}

/*
handler_test_failing
  Simple test function that always returns failure.
  Used for testing failing test execution behavior.

Parameter(s):
  _test: Test context (unused but required by signature)
Return:
  Always returns D_TEST_FAIL (false)
*/
bool 
handler_test_failing
(
    struct d_test* _test
)
{
    return D_TEST_FAIL;
}

/*
handler_test_with_assertion
  Test function that creates and checks an assertion.
  Used for testing assertion tracking.

Parameter(s):
  _test: Test context (unused but required by signature)
Return:
  D_TEST_PASS if assertion succeeds
*/
bool 
handler_test_with_assertion
(
    struct d_test* _test
)
{
    // create a passing assertion

    struct d_assertion* assertion = d_assert(true, 
                                             "Test assertion passed", 
                                             "Test assertion failed");
    bool result = assertion->result;

    d_assertion_free(assertion);
    
    return result ? D_TEST_PASS : 
                    D_TEST_FAIL;
}

//=============================================================================
// EVENT CALLBACK FUNCTIONS
//=============================================================================

/*
callback_setup
  Event callback for D_TEST_EVENT_SETUP.
  Increments global counter when called.
*/
void 
callback_setup
(
    size_t _size, 
    void** _elements
)
{
    g_event_setup_count++;
}

/*
callback_start
  Event callback for D_TEST_EVENT_START.
  Increments global counter when called.
*/
void 
callback_start
(
    size_t _size, 
    void** _elements
)
{
    g_event_start_count++;
}

/*
callback_success
  Event callback for D_TEST_EVENT_SUCCESS.
  Increments global counter when called.
*/
void 
callback_success
(
    size_t _size, 
    void** _elements
)
{
    g_event_success_count++;
}

/*
callback_failure
  Event callback for D_TEST_EVENT_FAILURE.
  Increments global counter when called.
*/
void 
callback_failure
(
    size_t _size, 
    void** _elements
)
{
    g_event_failure_count++;
}

/*
callback_end
  Event callback for D_TEST_EVENT_END.
  Increments global counter when called.
*/
void 
callback_end
(
    size_t _size, 
    void** _elements
)
{
    g_event_end_count++;
}

/*
callback_teardown
  Event callback for D_TEST_EVENT_TEAR_DOWN.
  Increments global counter when called.
*/
void 
callback_teardown
(
    size_t _size, 
    void** _elements
)
{
    g_event_teardown_count++;
}

//=============================================================================
// HANDLER CREATION AND DESTRUCTION TESTS
//=============================================================================

/*
d_tests_sa_handler_new
  Comprehensive test for d_test_handler_new function.
  
  Tests:
  - Creating handler with NULL config (should use defaults)
  - Creating handler with custom config
  - Creating handler with each preset configuration
  - Proper initialization of all handler fields
  - Results initialized to zero
  - Event handler is NULL (no event system)
  - Default config stored correctly
  - current_depth initialized to 0
  - abort_on_failure initialized to false

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_new
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_new ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Create handler with NULL config
    struct d_test_handler* handler_null = d_test_handler_new(NULL);
    
    if (!d_assert_and_count_standalone(handler_null != NULL,
        "d_test_handler_new succeeds with NULL config",
        "d_test_handler_new should succeed with NULL config", _test_info))
    {
        all_assertions_passed = false;
    }

    if (handler_null)
    {
        if (!d_assert_and_count_standalone(handler_null->event_handler == NULL,
            "Handler created without event system",
            "Handler should be created without event system", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(handler_null->current_depth == 0,
            "Handler current_depth initialized to 0",
            "Handler current_depth should be initialized to 0", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(handler_null->abort_on_failure == false,
            "Handler abort_on_failure initialized to false",
            "Handler abort_on_failure should be initialized to false", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(handler_null->results.tests_run == 0,
            "Handler results.tests_run initialized to 0",
            "Handler results.tests_run should be initialized to 0", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(
            (handler_null->results.tests_passed      == 0 &&
             handler_null->results.tests_failed      == 0 &&
             handler_null->results.assertions_run    == 0 &&
             handler_null->results.assertions_passed == 0 &&
             handler_null->results.assertions_failed == 0 &&
             handler_null->results.blocks_run        == 0 &&
             handler_null->results.max_depth         == 0),
            "All handler result fields initialized to 0",
            "All handler result fields should be initialized to 0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler_null);
    }

    // Test 2: Create handler with custom config
    struct d_test_config* custom_config = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_handler* handler_custom = d_test_handler_new(custom_config);
    
    if (!d_assert_and_count_standalone(handler_custom != NULL,
        "d_test_handler_new succeeds with custom config",
        "d_test_handler_new should succeed with custom config", _test_info))
    {
        all_assertions_passed = false;
    }

    if (handler_custom)
    {
        if (!d_assert_and_count_standalone(handler_custom->default_config == custom_config,
            "Handler stores default config reference",
            "Handler should store default config reference", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(handler_custom->event_handler == NULL,
            "Handler without events has NULL event_handler",
            "Handler without events should have NULL event_handler", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler_custom);
    }
    
    d_test_config_free(custom_config);

    // Test 3: Create handlers with all preset configurations
    uint32_t presets[] = {
        D_TEST_CONFIG_PRESET_SILENT,
        D_TEST_CONFIG_PRESET_MINIMAL,
        D_TEST_CONFIG_PRESET_NORMAL,
        D_TEST_CONFIG_PRESET_VERBOSE,
        D_TEST_CONFIG_PRESET_VERBOSE_STACK
    };

    bool all_presets_work = true;
    for (int i = 0; i < 5; i++)
    {
        struct d_test_config* preset_config = d_test_config_new_preset(presets[i]);
        struct d_test_handler* preset_handler = d_test_handler_new(preset_config);
        
        if (!preset_handler || preset_handler->default_config != preset_config)
        {
            all_presets_work = false;
        }
        
        d_test_handler_free(preset_handler);
        d_test_config_free(preset_config);
    }

    if (!d_assert_and_count_standalone(all_presets_work,
        "d_test_handler_new works with all preset configs",
        "d_test_handler_new should work with all preset configs", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 4: Multiple sequential handler creations
    for (int i = 0; i < 10; i++)
    {
        struct d_test_handler* temp = d_test_handler_new(NULL);
        if (temp)
        {
            d_test_handler_free(temp);
        }
    }

    if (!d_assert_and_count_standalone(true,
        "Multiple sequential handler creations succeed",
        "Multiple sequential handler creations should succeed", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_new unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_new unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_new_with_events
  Tests d_test_handler_new_with_events with different event capacities.
  
  Tests:
  - Creating handler with event system
  - Different event capacities (small, medium, large)
  - Event handler properly allocated
  - All other fields properly initialized
  - Zero event capacity handling
  - Large event capacity handling

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_new_with_events
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_new_with_events ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Create handler with event system (standard capacity)
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new_with_events(config, 10);
    
    if (!d_assert_and_count_standalone(handler != NULL,
        "d_test_handler_new_with_events succeeds",
        "d_test_handler_new_with_events should succeed", _test_info))
    {
        all_assertions_passed = false;
    }

    if (handler)
    {
        if (!d_assert_and_count_standalone(handler->event_handler != NULL,
            "Handler has event system allocated",
            "Handler should have event system allocated", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(handler->default_config == config,
            "Handler stores config reference",
            "Handler should store config reference", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(
            handler->current_depth == 0 &&
            handler->abort_on_failure == false,
            "Handler fields properly initialized with events",
            "Handler fields should be properly initialized with events", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 2: Create handler with small event capacity
    struct d_test_config* small_config = d_test_config_new(D_TEST_MODE_MINIMAL);
    struct d_test_handler* small_handler = d_test_handler_new_with_events(small_config, 1);
    
    if (!d_assert_and_count_standalone(small_handler != NULL,
        "Handler creation succeeds with small event capacity",
        "Handler creation should succeed with small event capacity", _test_info))
    {
        all_assertions_passed = false;
    }

    if (small_handler)
    {
        if (!d_assert_and_count_standalone(small_handler->event_handler != NULL,
            "Small capacity handler has event system",
            "Small capacity handler should have event system", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(small_handler);
    }
    
    d_test_config_free(small_config);

    // Test 3: Create handler with large event capacity
    struct d_test_config* large_config = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_handler* large_handler = d_test_handler_new_with_events(large_config, 100);
    
    if (!d_assert_and_count_standalone(large_handler != NULL,
        "Handler creation succeeds with large event capacity",
        "Handler creation should succeed with large event capacity", _test_info))
    {
        all_assertions_passed = false;
    }

    if (large_handler)
    {
        d_test_handler_free(large_handler);
    }
    
    d_test_config_free(large_config);

    // Test 4: Create handlers with various capacities
    size_t capacities[] = { 5, 10, 20, 50 };
    bool all_capacities_work = true;
    
    for (int i = 0; i < 4; i++)
    {
        struct d_test_config* temp_config = d_test_config_new(D_TEST_MODE_NORMAL);
        struct d_test_handler* temp_handler = d_test_handler_new_with_events(temp_config, capacities[i]);
        
        if (!temp_handler || !temp_handler->event_handler)
        {
            all_capacities_work = false;
        }
        
        d_test_handler_free(temp_handler);
        d_test_config_free(temp_config);
    }

    if (!d_assert_and_count_standalone(all_capacities_work,
        "Handler creation works with various event capacities",
        "Handler creation should work with various event capacities", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 5: Multiple sequential creations with events
    for (int i = 0; i < 5; i++)
    {
        struct d_test_config* seq_config = d_test_config_new(D_TEST_MODE_NORMAL);
        struct d_test_handler* seq_handler = d_test_handler_new_with_events(seq_config, 10);
        
        if (seq_handler)
        {
            d_test_handler_free(seq_handler);
        }
        
        d_test_config_free(seq_config);
    }

    if (!d_assert_and_count_standalone(true,
        "Multiple sequential handler with events creations succeed",
        "Multiple sequential handler with events creations should succeed", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_new_with_events unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_new_with_events unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_free
  Tests d_test_handler_free with valid and NULL handlers.
  
  Tests:
  - Freeing NULL handler (should not crash)
  - Freeing handler without event system
  - Freeing handler with event system
  - Freeing handlers with different configs
  - Multiple sequential frees
  - Proper cleanup of event system

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_free
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_free ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Free NULL handler (should not crash)
    d_test_handler_free(NULL);
    
    if (!d_assert_and_count_standalone(true,
        "Freeing NULL handler does not crash",
        "Freeing NULL handler should not crash", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Free handler without event system
    struct d_test_config* no_event_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* no_event_handler = d_test_handler_new(no_event_config);
    
    if (no_event_handler)
    {
        d_test_handler_free(no_event_handler);
        
        if (!d_assert_and_count_standalone(true,
            "Freeing handler without events succeeds",
            "Freeing handler without events should succeed", _test_info))
        {
            all_assertions_passed = false;
        }
    }
    
    d_test_config_free(no_event_config);

    // Test 3: Free handler with event system
    struct d_test_config* event_config = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_handler* event_handler = d_test_handler_new_with_events(event_config, 10);
    
    if (event_handler)
    {
        d_test_handler_free(event_handler);
        
        if (!d_assert_and_count_standalone(true,
            "Freeing handler with events succeeds",
            "Freeing handler with events should succeed", _test_info))
        {
            all_assertions_passed = false;
        }
    }
    
    d_test_config_free(event_config);

    // Test 4: Free handlers with all preset configs
    uint32_t presets[] = {
        D_TEST_CONFIG_PRESET_SILENT,
        D_TEST_CONFIG_PRESET_MINIMAL,
        D_TEST_CONFIG_PRESET_NORMAL,
        D_TEST_CONFIG_PRESET_VERBOSE
    };

    for (int i = 0; i < 4; i++)
    {
        struct d_test_config* preset_config = d_test_config_new_preset(presets[i]);
        struct d_test_handler* preset_handler = d_test_handler_new(preset_config);
        
        if (preset_handler)
        {
            d_test_handler_free(preset_handler);
        }
        
        d_test_config_free(preset_config);
    }

    if (!d_assert_and_count_standalone(true,
        "Freeing handlers with all presets succeeds",
        "Freeing handlers with all presets should succeed", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 5: Multiple sequential frees
    for (int i = 0; i < 10; i++)
    {
        struct d_test_config* temp_config = d_test_config_new(D_TEST_MODE_NORMAL);
        struct d_test_handler* temp_handler = d_test_handler_new_with_events(temp_config, 5);
        
        if (temp_handler)
        {
            d_test_handler_free(temp_handler);
        }
        
        d_test_config_free(temp_config);
    }

    if (!d_assert_and_count_standalone(true,
        "Multiple sequential handler frees succeed",
        "Multiple sequential handler frees should succeed", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_free unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_free unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

//=============================================================================
// EVENT LISTENER REGISTRATION TESTS
//=============================================================================


/*
d_tests_sa_handler_listener_enable_disable
  Tests listener enable/disable functionality.
  
  Tests:
  - Registering enabled listener and verifying it fires
  - Registering disabled listener and verifying it doesn't fire
  - Enabling a disabled listener
  - Disabling an enabled listener
  - Multiple enable/disable cycles

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_listener_enable_disable
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler listener enable/disable ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Enabled listener fires when event is emitted
    reset_event_counters();
    struct d_test_config* enabled_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* enabled_handler = d_test_handler_new_with_events(enabled_config, 10);
    
    if (enabled_handler)
    {
        // Register enabled listener
        d_test_handler_register_listener(enabled_handler, D_TEST_EVENT_START, callback_start, true);
        
        // Create a simple test to trigger events
        struct d_test* test = d_test_new(handler_test_passing, NULL);
        
        if (test)
        {
            // Run test (should trigger START event)
            d_test_handler_run_test(enabled_handler, test, NULL);
            
            if (!d_assert_and_count_standalone(g_event_start_count > 0,
                "Enabled listener callback is called",
                "Enabled listener callback should be called", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_free(test);
        }

        d_test_handler_free(enabled_handler);
    }
    
    d_test_config_free(enabled_config);

    // Test 2: Disabled listener does not fire
    reset_event_counters();
    struct d_test_config* disabled_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* disabled_handler = d_test_handler_new_with_events(disabled_config, 10);
    
    if (disabled_handler)
    {
        // Register disabled listener
        d_test_handler_register_listener(disabled_handler, D_TEST_EVENT_START, callback_start, false);
        
        struct d_test* test = d_test_new(handler_test_passing, NULL);
        
        if (test)
        {
            // Run test (should NOT trigger START event callback)
            d_test_handler_run_test(disabled_handler, test, NULL);
            
            if (!d_assert_and_count_standalone(g_event_start_count == 0,
                "Disabled listener callback is not called",
                "Disabled listener callback should not be called", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_free(test);
        }

        d_test_handler_free(disabled_handler);
    }
    
    d_test_config_free(disabled_config);

    // Test 3: Enable a disabled listener
    reset_event_counters();
    struct d_test_config* enable_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* enable_handler = d_test_handler_new_with_events(enable_config, 10);
    
    if (enable_handler)
    {
        // Register disabled listener
        d_test_handler_register_listener(enable_handler, D_TEST_EVENT_START, callback_start, false);
        
        // Enable it
        bool enable_result = d_test_handler_enable_listener(enable_handler, D_TEST_EVENT_START);
        
        if (!d_assert_and_count_standalone(enable_result,
            "Enabling disabled listener succeeds",
            "Enabling disabled listener should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        // Now run a test - callback should fire
        struct d_test* test = d_test_new(handler_test_passing, NULL);
        
        if (test)
        {
            d_test_handler_run_test(enable_handler, test, NULL);
            
            if (!d_assert_and_count_standalone(g_event_start_count > 0,
                "After enabling, listener callback is called",
                "After enabling, listener callback should be called", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_free(test);
        }

        d_test_handler_free(enable_handler);
    }
    
    d_test_config_free(enable_config);

    // Test 4: Disable an enabled listener
    reset_event_counters();
    struct d_test_config* disable_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* disable_handler = d_test_handler_new_with_events(disable_config, 10);
    
    if (disable_handler)
    {
        // Register enabled listener
        d_test_handler_register_listener(disable_handler, D_TEST_EVENT_START, callback_start, true);
        
        // Disable it
        bool disable_result = d_test_handler_disable_listener(disable_handler, D_TEST_EVENT_START);
        
        if (!d_assert_and_count_standalone(disable_result,
            "Disabling enabled listener succeeds",
            "Disabling enabled listener should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        // Now run a test - callback should NOT fire
        struct d_test* test = d_test_new(handler_test_passing, NULL);
        
        if (test)
        {
            d_test_handler_run_test(disable_handler, test, NULL);
            
            if (!d_assert_and_count_standalone(g_event_start_count == 0,
                "After disabling, listener callback is not called",
                "After disabling, listener callback should not be called", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_free(test);
        }

        d_test_handler_free(disable_handler);
    }
    
    d_test_config_free(disable_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_listener_enable_disable unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_listener_enable_disable unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_register_listener
  Tests d_test_handler_register_listener with various event types.
  
  Tests:
  - Registering listener on handler without events (should fail)
  - Registering listener on handler with events (should succeed)
  - Registering NULL callback (should fail)
  - Registering listeners for all event types
  - Registering enabled and disabled listeners
  - Registering multiple listeners

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_register_listener
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_register_listener ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Register listener on NULL handler (should fail)
    bool null_result = d_test_handler_register_listener(NULL, D_TEST_EVENT_START, callback_start, true);
    
    if (!d_assert_and_count_standalone(null_result == false,
        "Registering listener on NULL handler fails",
        "Registering listener on NULL handler should fail", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Register listener on handler without event system (should fail)
    struct d_test_config* no_event_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* no_event_handler = d_test_handler_new(no_event_config);
    
    if (no_event_handler)
    {
        bool no_event_result = d_test_handler_register_listener(
            no_event_handler, D_TEST_EVENT_START, callback_start, true);
        
        if (!d_assert_and_count_standalone(no_event_result == false,
            "Registering listener on handler without events fails",
            "Registering listener on handler without events should fail", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(no_event_handler);
    }
    
    d_test_config_free(no_event_config);

    // Test 3: Register NULL callback (should fail)
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new_with_events(config, 10);
    
    if (handler)
    {
        bool null_callback_result = d_test_handler_register_listener(
            handler, D_TEST_EVENT_START, NULL, true);
        
        if (!d_assert_and_count_standalone(null_callback_result == false,
            "Registering NULL callback fails",
            "Registering NULL callback should fail", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 4: Register listener successfully
    struct d_test_config* valid_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* valid_handler = d_test_handler_new_with_events(valid_config, 10);
    
    if (valid_handler)
    {
        bool register_result = d_test_handler_register_listener(
            valid_handler, D_TEST_EVENT_START, callback_start, true);
        
        if (!d_assert_and_count_standalone(register_result == true,
            "Registering valid listener succeeds",
            "Registering valid listener should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(valid_handler);
    }
    
    d_test_config_free(valid_config);

    // Test 5: Register listeners for all event types
    struct d_test_config* all_events_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* all_events_handler = d_test_handler_new_with_events(all_events_config, 20);
    
    if (all_events_handler)
    {
        bool setup_reg = d_test_handler_register_listener(
            all_events_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        bool start_reg = d_test_handler_register_listener(
            all_events_handler, D_TEST_EVENT_START, callback_start, true);
        bool success_reg = d_test_handler_register_listener(
            all_events_handler, D_TEST_EVENT_SUCCESS, callback_success, true);
        bool failure_reg = d_test_handler_register_listener(
            all_events_handler, D_TEST_EVENT_FAILURE, callback_failure, true);
        bool end_reg = d_test_handler_register_listener(
            all_events_handler, D_TEST_EVENT_END, callback_end, true);
        bool teardown_reg = d_test_handler_register_listener(
            all_events_handler, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);
        
        if (!d_assert_and_count_standalone(
            setup_reg && start_reg && success_reg && 
            failure_reg && end_reg && teardown_reg,
            "Registering listeners for all event types succeeds",
            "Registering listeners for all event types should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(all_events_handler);
    }
    
    d_test_config_free(all_events_config);

    // Test 6: Register enabled and disabled listeners
    struct d_test_config* enabled_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* enabled_handler = d_test_handler_new_with_events(enabled_config, 10);
    
    if (enabled_handler)
    {
        bool enabled_result = d_test_handler_register_listener(
            enabled_handler, D_TEST_EVENT_START, callback_start, true);
        bool disabled_result = d_test_handler_register_listener(
            enabled_handler, D_TEST_EVENT_END, callback_end, false);
        
        if (!d_assert_and_count_standalone(enabled_result && disabled_result,
            "Registering both enabled and disabled listeners succeeds",
            "Registering both enabled and disabled listeners should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(enabled_handler);
    }
    
    d_test_config_free(enabled_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_register_listener unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_register_listener unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_unregister_listener
  Tests d_test_handler_unregister_listener behavior.
  
  Tests:
  - Unregistering from NULL handler (should fail)
  - Unregistering from handler without events (should fail)
  - Unregistering non-existent listener (should fail)
  - Unregistering existing listener (should succeed)
  - Unregistering multiple listeners

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_unregister_listener
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_unregister_listener ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Unregister from NULL handler (should fail)
    bool null_result = d_test_handler_unregister_listener(NULL, D_TEST_EVENT_START);
    
    if (!d_assert_and_count_standalone(null_result == false,
        "Unregistering from NULL handler fails",
        "Unregistering from NULL handler should fail", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Unregister from handler without events (should fail)
    struct d_test_config* no_event_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* no_event_handler = d_test_handler_new(no_event_config);
    
    if (no_event_handler)
    {
        bool no_event_result = d_test_handler_unregister_listener(
            no_event_handler, D_TEST_EVENT_START);
        
        if (!d_assert_and_count_standalone(no_event_result == false,
            "Unregistering from handler without events fails",
            "Unregistering from handler without events should fail", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(no_event_handler);
    }
    
    d_test_config_free(no_event_config);

    // Test 3: Unregister non-existent listener (behavior depends on implementation)
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new_with_events(config, 10);
    
    if (handler)
    {
        // Try to unregister a listener that was never registered
        bool unregister_result = d_test_handler_unregister_listener(
            handler, D_TEST_EVENT_START);
        
        // Note: This test just verifies the function doesn't crash
        // The return value may vary based on implementation
        if (!d_assert_and_count_standalone(true,
            "Unregistering non-existent listener does not crash",
            "Unregistering non-existent listener should not crash", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 4: Register then unregister listener
    struct d_test_config* reg_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* reg_handler = d_test_handler_new_with_events(reg_config, 10);
    
    if (reg_handler)
    {
        // Register a listener
        bool register_result = d_test_handler_register_listener(
            reg_handler, D_TEST_EVENT_START, callback_start, true);
        
        // Unregister it
        bool unregister_result = d_test_handler_unregister_listener(
            reg_handler, D_TEST_EVENT_START);
        
        if (!d_assert_and_count_standalone(register_result && unregister_result,
            "Register then unregister listener succeeds",
            "Register then unregister listener should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(reg_handler);
    }
    
    d_test_config_free(reg_config);

    // Test 5: Register multiple, unregister specific ones
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new_with_events(multi_config, 20);
    
    if (multi_handler)
    {
        // Register multiple listeners
        d_test_handler_register_listener(multi_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        d_test_handler_register_listener(multi_handler, D_TEST_EVENT_START, callback_start, true);
        d_test_handler_register_listener(multi_handler, D_TEST_EVENT_END, callback_end, true);
        
        // Unregister one
        bool unregister_one = d_test_handler_unregister_listener(
            multi_handler, D_TEST_EVENT_START);
        
        if (!d_assert_and_count_standalone(unregister_one,
            "Unregistering one of multiple listeners succeeds",
            "Unregistering one of multiple listeners should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        // Unregister all
        bool unregister_all = 
            d_test_handler_unregister_listener(multi_handler, D_TEST_EVENT_SETUP) &&
            d_test_handler_unregister_listener(multi_handler, D_TEST_EVENT_END);
        
        if (!d_assert_and_count_standalone(unregister_all,
            "Unregistering all remaining listeners succeeds",
            "Unregistering all remaining listeners should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_unregister_listener unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_unregister_listener unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}


//=============================================================================
// TEST EXECUTION TESTS
//=============================================================================

/*
d_tests_sa_handler_run_test
  Tests d_test_handler_run_test with passing and failing tests.
  
  Tests:
  - Running NULL test (should fail)
  - Running test on NULL handler (should fail)
  - Running passing test updates statistics correctly
  - Running failing test updates statistics correctly
  - Tests with NULL config use handler default
  - Tests with custom config use provided config
  - Statistics accumulate across multiple runs

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_run_test
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_run_test ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Run NULL test (should fail)
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new(config);
    
    if (handler)
    {
        bool null_result = d_test_handler_run_test(handler, NULL, NULL);
        
        if (!d_assert_and_count_standalone(null_result == false,
            "Running NULL test fails",
            "Running NULL test should fail", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 2: Run test on NULL handler (should fail)
    struct d_test* test = d_test_new(handler_test_passing, NULL);
    
    if (test)
    {
        bool null_handler_result = d_test_handler_run_test(NULL, test, NULL);
        
        if (!d_assert_and_count_standalone(null_handler_result == false,
            "Running test on NULL handler fails",
            "Running test on NULL handler should fail", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_free(test);
    }

    // Test 3: Run passing test - verify statistics
    struct d_test_config* pass_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* pass_handler = d_test_handler_new(pass_config);
    
    if (pass_handler)
    {
        struct d_test* pass_test = d_test_new(handler_test_passing, NULL);
        
        if (pass_test)
        {
            bool pass_result = d_test_handler_run_test(pass_handler, pass_test, NULL);
            
            if (!d_assert_and_count_standalone(pass_result == true,
                "Running passing test succeeds",
                "Running passing test should succeed", _test_info))
            {
                all_assertions_passed = false;
            }

            // Check statistics
            struct d_test_result results = d_test_handler_get_results(pass_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 1,
                "tests_run incremented for passing test",
                "tests_run should be incremented for passing test", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_passed == 1,
                "tests_passed incremented for passing test",
                "tests_passed should be incremented for passing test", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_failed == 0,
                "tests_failed not incremented for passing test",
                "tests_failed should not be incremented for passing test", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_free(pass_test);
        }

        d_test_handler_free(pass_handler);
    }
    
    d_test_config_free(pass_config);

    // Test 4: Run failing test - verify statistics
    struct d_test_config* fail_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fail_handler = d_test_handler_new(fail_config);
    
    if (fail_handler)
    {
        struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
        
        if (fail_test)
        {
            bool fail_result = d_test_handler_run_test(fail_handler, fail_test, NULL);
            
            if (!d_assert_and_count_standalone(fail_result == false,
                "Running failing test fails",
                "Running failing test should fail", _test_info))
            {
                all_assertions_passed = false;
            }

            // Check statistics
            struct d_test_result results = d_test_handler_get_results(fail_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 1,
                "tests_run incremented for failing test",
                "tests_run should be incremented for failing test", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_passed == 0,
                "tests_passed not incremented for failing test",
                "tests_passed should not be incremented for failing test", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_failed == 1,
                "tests_failed incremented for failing test",
                "tests_failed should be incremented for failing test", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_free(fail_test);
        }

        d_test_handler_free(fail_handler);
    }
    
    d_test_config_free(fail_config);

    // Test 5: Run multiple tests - verify accumulation
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new(multi_config);
    
    if (multi_handler)
    {
        // Run 3 passing tests and 2 failing tests
        for (int i = 0; i < 3; i++)
        {
            struct d_test* pass_test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(multi_handler, pass_test, NULL);
            d_test_free(pass_test);
        }

        for (int i = 0; i < 2; i++)
        {
            struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
            d_test_handler_run_test(multi_handler, fail_test, NULL);
            d_test_free(fail_test);
        }

        struct d_test_result results = d_test_handler_get_results(multi_handler);
        
        if (!d_assert_and_count_standalone(results.tests_run == 5,
            "tests_run accumulates correctly (5 total)",
            "tests_run should accumulate correctly (5 total)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.tests_passed == 3,
            "tests_passed accumulates correctly (3 passed)",
            "tests_passed should accumulate correctly (3 passed)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.tests_failed == 2,
            "tests_failed accumulates correctly (2 failed)",
            "tests_failed should accumulate correctly (2 failed)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Test 6: Test with custom run config
    struct d_test_config* default_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_config* run_config = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_handler* config_handler = d_test_handler_new(default_config);
    
    if (config_handler)
    {
        struct d_test* config_test = d_test_new(handler_test_passing, NULL);
        
        if (config_test)
        {
            bool config_result = d_test_handler_run_test(config_handler, config_test, run_config);
            
            if (!d_assert_and_count_standalone(config_result == true,
                "Running test with custom run config succeeds",
                "Running test with custom run config should succeed", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_free(config_test);
        }

        d_test_handler_free(config_handler);
    }
    
    d_test_config_free(default_config);
    d_test_config_free(run_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_run_test unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_run_test unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_run_block
  Tests d_test_handler_run_block with various block configurations.
  
  Tests:
  - Running NULL block (should fail)
  - Running block on NULL handler (should fail)
  - Running empty block (should succeed)
  - Running block with passing tests
  - Running block with failing tests
  - Running block with mixed pass/fail tests
  - Block statistics (blocks_run, depth tracking)
  - Override behavior in blocks

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_run_block
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_run_block ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Run NULL block (should fail)
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new(config);
    
    if (handler)
    {
        bool null_result = d_test_handler_run_block(handler, NULL, NULL);
        
        if (!d_assert_and_count_standalone(null_result == false,
            "Running NULL block fails",
            "Running NULL block should fail", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 2: Run block on NULL handler (should fail)
    struct d_test_block* block = d_test_block_new(10, NULL);
    
    if (block)
    {
        bool null_handler_result = d_test_handler_run_block(NULL, block, NULL);
        
        if (!d_assert_and_count_standalone(null_handler_result == false,
            "Running block on NULL handler fails",
            "Running block on NULL handler should fail", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_block_free(block);
    }

    // Test 3: Run empty block (should succeed)
    struct d_test_config* empty_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* empty_handler = d_test_handler_new(empty_config);
    
    if (empty_handler)
    {
        struct d_test_block* empty_block = d_test_block_new(10, NULL);
        
        if (empty_block)
        {
            bool empty_result = d_test_handler_run_block(empty_handler, empty_block, NULL);
            
            if (!d_assert_and_count_standalone(empty_result == true,
                "Running empty block succeeds",
                "Running empty block should succeed", _test_info))
            {
                all_assertions_passed = false;
            }

            struct d_test_result results = d_test_handler_get_results(empty_handler);
            
            if (!d_assert_and_count_standalone(results.blocks_run == 1,
                "blocks_run incremented for empty block",
                "blocks_run should be incremented for empty block", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(empty_block);
        }

        d_test_handler_free(empty_handler);
    }
    
    d_test_config_free(empty_config);

    // Test 4: Run block with passing tests
    struct d_test_config* pass_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* pass_handler = d_test_handler_new(pass_config);
    
    if (pass_handler)
    {
        struct d_test_block* pass_block = d_test_block_new(10, NULL);
        
        if (pass_block)
        {
            // Add 3 passing tests to block
            for (int i = 0; i < 3; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                pass_block->tests[i] = *type;
                free(type);
            }
            pass_block->count = 3;

            bool pass_result = d_test_handler_run_block(pass_handler, pass_block, NULL);
            
            if (!d_assert_and_count_standalone(pass_result == true,
                "Running block with passing tests succeeds",
                "Running block with passing tests should succeed", _test_info))
            {
                all_assertions_passed = false;
            }

            struct d_test_result results = d_test_handler_get_results(pass_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 3,
                "All 3 tests in block executed",
                "All 3 tests in block should be executed", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_passed == 3,
                "All 3 tests passed",
                "All 3 tests should pass", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.blocks_run == 1,
                "Block counted in blocks_run",
                "Block should be counted in blocks_run", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(pass_block);
        }

        d_test_handler_free(pass_handler);
    }
    
    d_test_config_free(pass_config);

    // Test 5: Run block with failing tests
    struct d_test_config* fail_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fail_handler = d_test_handler_new(fail_config);
    
    if (fail_handler)
    {
        struct d_test_block* fail_block = d_test_block_new(10, NULL);
        
        if (fail_block)
        {
            // Add 2 failing tests to block
            for (int i = 0; i < 2; i++)
            {
                struct d_test* test = d_test_new(handler_test_failing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                fail_block->tests[i] = *type;
                free(type);
            }
            fail_block->count = 2;

            bool fail_result = d_test_handler_run_block(fail_handler, fail_block, NULL);
            
            if (!d_assert_and_count_standalone(fail_result == false,
                "Running block with failing tests fails",
                "Running block with failing tests should fail", _test_info))
            {
                all_assertions_passed = false;
            }

            struct d_test_result results = d_test_handler_get_results(fail_handler);
            
            if (!d_assert_and_count_standalone(results.tests_failed == 2,
                "Both failing tests counted",
                "Both failing tests should be counted", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(fail_block);
        }

        d_test_handler_free(fail_handler);
    }
    
    d_test_config_free(fail_config);

    // Test 6: Run block with mixed pass/fail tests
    struct d_test_config* mixed_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* mixed_handler = d_test_handler_new(mixed_config);
    
    if (mixed_handler)
    {
        struct d_test_block* mixed_block = d_test_block_new(10, NULL);
        
        if (mixed_block)
        {
            // Add 2 passing tests
            for (int i = 0; i < 2; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                mixed_block->tests[i] = *type;
                free(type);
            }

            // Add 1 failing test
            struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
            struct d_test_type* fail_type = d_test_type_new(D_TEST_TYPE_TEST, fail_test);
            mixed_block->tests[2] = *fail_type;
            free(fail_type);

            mixed_block->count = 3;

            bool mixed_result = d_test_handler_run_block(mixed_handler, mixed_block, NULL);
            
            if (!d_assert_and_count_standalone(mixed_result == false,
                "Running block with mixed results fails overall",
                "Running block with mixed results should fail overall", _test_info))
            {
                all_assertions_passed = false;
            }

            struct d_test_result results = d_test_handler_get_results(mixed_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 3,
                "All 3 tests executed despite failure",
                "All 3 tests should be executed despite failure", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_passed == 2,
                "2 tests passed",
                "2 tests should pass", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_failed == 1,
                "1 test failed",
                "1 test should fail", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(mixed_block);
        }

        d_test_handler_free(mixed_handler);
    }
    
    d_test_config_free(mixed_config);

    // Test 7: Depth tracking increases for block execution
    struct d_test_config* depth_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* depth_handler = d_test_handler_new(depth_config);
    
    if (depth_handler)
    {
        struct d_test_block* depth_block = d_test_block_new(5, NULL);
        
        if (depth_block)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            depth_block->tests[0] = *type;
            depth_block->count = 1;
            free(type);

            d_test_handler_run_block(depth_handler, depth_block, NULL);

            struct d_test_result results = d_test_handler_get_results(depth_handler);
            
            if (!d_assert_and_count_standalone(results.max_depth >= 1,
                "max_depth tracked for block execution",
                "max_depth should be tracked for block execution", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(depth_block);
        }

        d_test_handler_free(depth_handler);
    }
    
    d_test_config_free(depth_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_run_block unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_run_block unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_run_test_type
  Tests d_test_handler_run_test_type with both test and block types.
  
  Tests:
  - Running NULL test_type (should fail)
  - Running test_type on NULL handler (should fail)
  - Running test_type wrapping a test
  - Running test_type wrapping a block
  - Statistics updated correctly for both types
  - Type discriminator handled correctly

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_run_test_type
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_run_test_type ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Run NULL test_type (should fail)
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new(config);
    
    if (handler)
    {
        bool null_result = d_test_handler_run_test_type(handler, NULL, NULL);
        
        if (!d_assert_and_count_standalone(null_result == false,
            "Running NULL test_type fails",
            "Running NULL test_type should fail", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 2: Run test_type on NULL handler (should fail)
    struct d_test* test = d_test_new(handler_test_passing, NULL);
    struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
    
    if (test_type)
    {
        bool null_handler_result = d_test_handler_run_test_type(NULL, test_type, NULL);
        
        if (!d_assert_and_count_standalone(null_handler_result == false,
            "Running test_type on NULL handler fails",
            "Running test_type on NULL handler should fail", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_type_free(test_type);
    }

    // Test 3: Run test_type wrapping a passing test
    struct d_test_config* test_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* test_handler = d_test_handler_new(test_config);
    
    if (test_handler)
    {
        struct d_test* wrapped_test = d_test_new(handler_test_passing, NULL);
        struct d_test_type* wrapped_type = d_test_type_new(D_TEST_TYPE_TEST, wrapped_test);
        
        if (wrapped_type)
        {
            bool test_result = d_test_handler_run_test_type(test_handler, wrapped_type, NULL);
            
            if (!d_assert_and_count_standalone(test_result == true,
                "Running test_type wrapping passing test succeeds",
                "Running test_type wrapping passing test should succeed", _test_info))
            {
                all_assertions_passed = false;
            }

            struct d_test_result results = d_test_handler_get_results(test_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 1,
                "Test execution counted for test_type",
                "Test execution should be counted for test_type", _test_info))
            {
                all_assertions_passed = false;
            }

            // Don't free wrapped_test - d_test_type_free will handle it
            d_test_type_free(wrapped_type);
        }

        d_test_handler_free(test_handler);
    }
    
    d_test_config_free(test_config);

    // Test 4: Run test_type wrapping a failing test
    struct d_test_config* fail_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fail_handler = d_test_handler_new(fail_config);
    
    if (fail_handler)
    {
        struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
        struct d_test_type* fail_type = d_test_type_new(D_TEST_TYPE_TEST, fail_test);
        
        if (fail_type)
        {
            bool fail_result = d_test_handler_run_test_type(fail_handler, fail_type, NULL);
            
            if (!d_assert_and_count_standalone(fail_result == false,
                "Running test_type wrapping failing test fails",
                "Running test_type wrapping failing test should fail", _test_info))
            {
                all_assertions_passed = false;
            }

            struct d_test_result results = d_test_handler_get_results(fail_handler);
            
            if (!d_assert_and_count_standalone(results.tests_failed == 1,
                "Test failure counted for test_type",
                "Test failure should be counted for test_type", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_type_free(fail_type);
        }

        d_test_handler_free(fail_handler);
    }
    
    d_test_config_free(fail_config);

    // Test 5: Run test_type wrapping a block
    struct d_test_config* block_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* block_handler = d_test_handler_new(block_config);
    
    if (block_handler)
    {
        struct d_test_block* block = d_test_block_new(10, NULL);
        
        if (block)
        {
            // Add tests to block
            for (int i = 0; i < 3; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[i] = *type;
                free(type);
            }
            block->count = 3;

            struct d_test_type* block_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, block);
            
            if (block_type)
            {
                bool block_result = d_test_handler_run_test_type(block_handler, block_type, NULL);
                
                if (!d_assert_and_count_standalone(block_result == true,
                    "Running test_type wrapping block succeeds",
                    "Running test_type wrapping block should succeed", _test_info))
                {
                    all_assertions_passed = false;
                }

                struct d_test_result results = d_test_handler_get_results(block_handler);
                
                if (!d_assert_and_count_standalone(results.tests_run == 3,
                    "All tests in wrapped block executed",
                    "All tests in wrapped block should be executed", _test_info))
                {
                    all_assertions_passed = false;
                }

                if (!d_assert_and_count_standalone(results.blocks_run == 1,
                    "Block execution counted for test_type",
                    "Block execution should be counted for test_type", _test_info))
                {
                    all_assertions_passed = false;
                }

                d_test_type_free(block_type);
            }
        }

        d_test_handler_free(block_handler);
    }
    
    d_test_config_free(block_config);

    // Test 6: Run multiple test_types of different types
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new(multi_config);
    
    if (multi_handler)
    {
        // Run a test_type wrapping a test
        struct d_test* test1 = d_test_new(handler_test_passing, NULL);
        struct d_test_type* type1 = d_test_type_new(D_TEST_TYPE_TEST, test1);
        d_test_handler_run_test_type(multi_handler, type1, NULL);
        d_test_type_free(type1);

        // Run a test_type wrapping a block
        struct d_test_block* block1 = d_test_block_new(5, NULL);
        struct d_test* test2 = d_test_new(handler_test_passing, NULL);
        struct d_test_type* type2 = d_test_type_new(D_TEST_TYPE_TEST, test2);
        block1->tests[0] = *type2;
        block1->count = 1;
        free(type2);

        struct d_test_type* block_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, block1);
        d_test_handler_run_test_type(multi_handler, block_type, NULL);
        d_test_type_free(block_type);

        struct d_test_result results = d_test_handler_get_results(multi_handler);
        
        if (!d_assert_and_count_standalone(results.tests_run == 2,
            "Both test_types executed correctly",
            "Both test_types should be executed correctly", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.blocks_run == 1,
            "Block test_type counted in blocks",
            "Block test_type should be counted in blocks", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_run_test_type unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_run_test_type unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_nested_execution
  Tests nested block execution with depth tracking.
  
  Tests:
  - Single level nesting (block contains test)
  - Two level nesting (block contains block contains test)
  - Three level nesting
  - Depth tracking increments and decrements correctly
  - max_depth calculation with various nesting levels
  - current_depth resets after execution
  - Statistics accumulate correctly across nesting levels

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_nested_execution
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler nested execution ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Single level - block with test (depth should be 1)
    struct d_test_config* level1_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* level1_handler = d_test_handler_new(level1_config);
    
    if (level1_handler)
    {
        struct d_test_block* block = d_test_block_new(10, NULL);
        
        if (block)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            d_test_handler_run_block(level1_handler, block, NULL);

            struct d_test_result results = d_test_handler_get_results(level1_handler);
            
            if (!d_assert_and_count_standalone(results.max_depth == 1,
                "Single level nesting has max_depth of 1",
                "Single level nesting should have max_depth of 1", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(level1_handler->current_depth == 0,
                "current_depth resets to 0 after execution",
                "current_depth should reset to 0 after execution", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(level1_handler);
    }
    
    d_test_config_free(level1_config);

    // Test 2: Two level nesting (depth should be 2)
    struct d_test_config* level2_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* level2_handler = d_test_handler_new(level2_config);
    
    if (level2_handler)
    {
        struct d_test_block* outer_block = d_test_block_new(10, NULL);
        struct d_test_block* inner_block = d_test_block_new(5, NULL);
        
        if (outer_block && inner_block)
        {
            // Add test to inner block
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner_block->tests[0] = *test_type;
            inner_block->count = 1;
            free(test_type);

            // Add inner block to outer block
            struct d_test_type* block_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner_block);
            outer_block->tests[0] = *block_type;
            outer_block->count = 1;
            free(block_type);

            d_test_handler_run_block(level2_handler, outer_block, NULL);

            struct d_test_result results = d_test_handler_get_results(level2_handler);
            
            if (!d_assert_and_count_standalone(results.max_depth == 2,
                "Two level nesting has max_depth of 2",
                "Two level nesting should have max_depth of 2", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.blocks_run == 2,
                "Both blocks counted in blocks_run",
                "Both blocks should be counted in blocks_run", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_run == 1,
                "Nested test executed",
                "Nested test should be executed", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer_block);
        }

        d_test_handler_free(level2_handler);
    }
    
    d_test_config_free(level2_config);

    // Test 3: Three level nesting (depth should be 3)
    struct d_test_config* level3_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* level3_handler = d_test_handler_new(level3_config);
    
    if (level3_handler)
    {
        struct d_test_block* outer = d_test_block_new(10, NULL);
        struct d_test_block* middle = d_test_block_new(5, NULL);
        struct d_test_block* inner = d_test_block_new(3, NULL);
        
        if (outer && middle && inner)
        {
            // Add test to innermost block
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            // Add inner to middle
            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            middle->tests[0] = *inner_type;
            middle->count = 1;
            free(inner_type);

            // Add middle to outer
            struct d_test_type* middle_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, middle);
            outer->tests[0] = *middle_type;
            outer->count = 1;
            free(middle_type);

            d_test_handler_run_block(level3_handler, outer, NULL);

            struct d_test_result results = d_test_handler_get_results(level3_handler);
            
            if (!d_assert_and_count_standalone(results.max_depth == 3,
                "Three level nesting has max_depth of 3",
                "Three level nesting should have max_depth of 3", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.blocks_run == 3,
                "All three blocks counted",
                "All three blocks should be counted", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(level3_handler->current_depth == 0,
                "current_depth resets after three-level nesting",
                "current_depth should reset after three-level nesting", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(level3_handler);
    }
    
    d_test_config_free(level3_config);

    // Test 4: Multiple nested structures, max_depth tracks highest
    struct d_test_config* max_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* max_handler = d_test_handler_new(max_config);
    
    if (max_handler)
    {
        // Run a depth-1 block
        struct d_test_block* shallow = d_test_block_new(5, NULL);
        if (shallow)
        {
            struct d_test* test1 = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type1 = d_test_type_new(D_TEST_TYPE_TEST, test1);
            shallow->tests[0] = *type1;
            shallow->count = 1;
            free(type1);

            d_test_handler_run_block(max_handler, shallow, NULL);
            d_test_block_free(shallow);
        }

        // Run a depth-3 block
        struct d_test_block* deep_outer = d_test_block_new(5, NULL);
        struct d_test_block* deep_middle = d_test_block_new(5, NULL);
        struct d_test_block* deep_inner = d_test_block_new(5, NULL);
        
        if (deep_outer && deep_middle && deep_inner)
        {
            struct d_test* test2 = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type2 = d_test_type_new(D_TEST_TYPE_TEST, test2);
            deep_inner->tests[0] = *type2;
            deep_inner->count = 1;
            free(type2);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, deep_inner);
            deep_middle->tests[0] = *inner_type;
            deep_middle->count = 1;
            free(inner_type);

            struct d_test_type* middle_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, deep_middle);
            deep_outer->tests[0] = *middle_type;
            deep_outer->count = 1;
            free(middle_type);

            d_test_handler_run_block(max_handler, deep_outer, NULL);
            d_test_block_free(deep_outer);
        }

        struct d_test_result results = d_test_handler_get_results(max_handler);
        
        if (!d_assert_and_count_standalone(results.max_depth == 3,
            "max_depth tracks highest depth across multiple runs",
            "max_depth should track highest depth across multiple runs", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(max_handler);
    }
    
    d_test_config_free(max_config);

    // Test 5: Mixed nesting with direct tests and blocks
    struct d_test_config* mixed_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* mixed_handler = d_test_handler_new(mixed_config);
    
    if (mixed_handler)
    {
        struct d_test_block* mixed = d_test_block_new(10, NULL);
        
        if (mixed)
        {
            // Add a direct test (no depth increase)
            struct d_test* test1 = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type1 = d_test_type_new(D_TEST_TYPE_TEST, test1);
            mixed->tests[0] = *type1;
            free(type1);

            // Add a nested block (depth increases)
            struct d_test_block* nested = d_test_block_new(5, NULL);
            if (nested)
            {
                struct d_test* test2 = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type2 = d_test_type_new(D_TEST_TYPE_TEST, test2);
                nested->tests[0] = *type2;
                nested->count = 1;
                free(type2);

                struct d_test_type* nested_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, nested);
                mixed->tests[1] = *nested_type;
                free(nested_type);
            }

            mixed->count = 2;

            d_test_handler_run_block(mixed_handler, mixed, NULL);

            struct d_test_result results = d_test_handler_get_results(mixed_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 2,
                "Both direct and nested tests executed",
                "Both direct and nested tests should be executed", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.max_depth == 2,
                "max_depth accounts for nested block",
                "max_depth should account for nested block", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(mixed);
        }

        d_test_handler_free(mixed_handler);
    }
    
    d_test_config_free(mixed_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_nested_execution unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_nested_execution unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_config_override
  Tests configuration override behavior during execution.
  
  Tests:
  - Runtime config overrides block config
  - Runtime config overrides test config
  - NULL runtime config uses default handler config
  - Block override_block flag behavior with handler
  - Priority: runtime > block (with override) > test > default
  - Different configs at different nesting levels

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_config_override
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler config override ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Runtime config overrides block config
    struct d_test_config* default_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_config* block_config = d_test_config_new(D_TEST_MODE_MINIMAL);
    struct d_test_config* runtime_config = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_handler* handler = d_test_handler_new(default_config);
    
    if (handler)
    {
        struct d_test_block* block = d_test_block_new(5, block_config);
        
        if (block)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            // Run with runtime config (should override block config)
            bool result = d_test_handler_run_block(handler, block, runtime_config);
            
            if (!d_assert_and_count_standalone(result == true,
                "Runtime config override execution succeeds",
                "Runtime config override execution should succeed", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(default_config);
    d_test_config_free(block_config);
    d_test_config_free(runtime_config);

    // Test 2: NULL runtime config uses handler default
    struct d_test_config* default2 = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_handler* handler2 = d_test_handler_new(default2);
    
    if (handler2)
    {
        struct d_test* test = d_test_new(handler_test_passing, NULL);
        
        if (test)
        {
            // Run with NULL runtime config (should use handler default)
            bool result = d_test_handler_run_test(handler2, test, NULL);
            
            if (!d_assert_and_count_standalone(result == true,
                "NULL runtime config uses handler default",
                "NULL runtime config should use handler default", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_free(test);
        }

        d_test_handler_free(handler2);
    }
    
    d_test_config_free(default2);

    // Test 3: Block override_block flag respected by handler
    struct d_test_config* override_default = d_test_config_new(D_TEST_MODE_SILENT);
    struct d_test_config* override_block = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_config* override_test = d_test_config_new(D_TEST_MODE_MINIMAL);
    struct d_test_handler* override_handler = d_test_handler_new(override_default);
    
    if (override_handler)
    {
        // Create block with override=true
        struct d_test_block* block = d_test_block_new_with_override(5, override_block, true);
        
        if (block)
        {
            // Create test with its own config (should be overridden by block)
            struct d_test* test = d_test_new(handler_test_passing, override_test);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            // Run block (block config should override test config)
            bool result = d_test_handler_run_block(override_handler, block, NULL);
            
            if (!d_assert_and_count_standalone(result == true,
                "Block override flag respected during execution",
                "Block override flag should be respected during execution", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(override_handler);
    }
    
    d_test_config_free(override_default);
    d_test_config_free(override_block);
    d_test_config_free(override_test);

    // Test 4: Priority hierarchy - runtime > block override > test > default
    struct d_test_config* priority_default = d_test_config_new(D_TEST_MODE_SILENT);
    struct d_test_config* priority_test = d_test_config_new(D_TEST_MODE_MINIMAL);
    struct d_test_config* priority_block = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_config* priority_runtime = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_handler* priority_handler = d_test_handler_new(priority_default);
    
    if (priority_handler)
    {
        struct d_test_block* block = d_test_block_new_with_override(5, priority_block, true);
        
        if (block)
        {
            struct d_test* test = d_test_new(handler_test_passing, priority_test);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            // Runtime config should have highest priority
            bool result = d_test_handler_run_block(priority_handler, block, priority_runtime);
            
            if (!d_assert_and_count_standalone(result == true,
                "Config priority hierarchy respected",
                "Config priority hierarchy should be respected", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(priority_handler);
    }
    
    d_test_config_free(priority_default);
    d_test_config_free(priority_test);
    d_test_config_free(priority_block);
    d_test_config_free(priority_runtime);

    // Test 5: Different configs at different nesting levels
    struct d_test_config* nested_default = d_test_config_new(D_TEST_MODE_SILENT);
    struct d_test_handler* nested_handler = d_test_handler_new(nested_default);
    
    if (nested_handler)
    {
        struct d_test_config* outer_config = d_test_config_new(D_TEST_MODE_MINIMAL);
        struct d_test_config* inner_config = d_test_config_new(D_TEST_MODE_VERBOSE);
        
        struct d_test_block* outer = d_test_block_new(5, outer_config);
        struct d_test_block* inner = d_test_block_new(5, inner_config);
        
        if (outer && inner)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            bool result = d_test_handler_run_block(nested_handler, outer, NULL);
            
            if (!d_assert_and_count_standalone(result == true,
                "Different configs at nesting levels work correctly",
                "Different configs at nesting levels should work correctly", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_config_free(outer_config);
        d_test_config_free(inner_config);
        d_test_handler_free(nested_handler);
    }
    
    d_test_config_free(nested_default);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_config_override unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_config_override unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

//=============================================================================
// ASSERTION TRACKING TESTS
//=============================================================================

/*
d_tests_sa_handler_record_assertion
  Tests d_test_handler_record_assertion with passing and failing assertions.
  
  Tests:
  - Recording assertion on NULL handler (should not crash)
  - Recording NULL assertion (should not crash)
  - Recording passing assertion updates statistics correctly
  - Recording failing assertion updates statistics correctly
  - Multiple assertions accumulate correctly
  - assertions_run counter increments for all assertions

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_record_assertion
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_record_assertion ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Record assertion on NULL handler (should not crash)
    struct d_assertion* assertion = d_assert(true, "Test", "Test");
    d_test_handler_record_assertion(NULL, assertion);
    d_assertion_free(assertion);
    
    if (!d_assert_and_count_standalone(true,
        "Recording assertion on NULL handler does not crash",
        "Recording assertion on NULL handler should not crash", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Record NULL assertion (should not crash)
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new(config);
    
    if (handler)
    {
        d_test_handler_record_assertion(handler, NULL);
        
        if (!d_assert_and_count_standalone(true,
            "Recording NULL assertion does not crash",
            "Recording NULL assertion should not crash", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 3: Record passing assertion
    struct d_test_config* pass_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* pass_handler = d_test_handler_new(pass_config);
    
    if (pass_handler)
    {
        struct d_assertion* pass_assertion = d_assert(true, "Passed", "Failed");
        d_test_handler_record_assertion(pass_handler, pass_assertion);
        
        struct d_test_result results = d_test_handler_get_results(pass_handler);
        
        if (!d_assert_and_count_standalone(results.assertions_run == 1,
            "assertions_run incremented for passing assertion",
            "assertions_run should be incremented for passing assertion", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_passed == 1,
            "assertions_passed incremented for passing assertion",
            "assertions_passed should be incremented for passing assertion", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_failed == 0,
            "assertions_failed not incremented for passing assertion",
            "assertions_failed should not be incremented for passing assertion", _test_info))
        {
            all_assertions_passed = false;
        }

        d_assertion_free(pass_assertion);
        d_test_handler_free(pass_handler);
    }
    
    d_test_config_free(pass_config);

    // Test 4: Record failing assertion
    struct d_test_config* fail_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fail_handler = d_test_handler_new(fail_config);
    
    if (fail_handler)
    {
        struct d_assertion* fail_assertion = d_assert(false, "Passed", "Failed");
        d_test_handler_record_assertion(fail_handler, fail_assertion);
        
        struct d_test_result results = d_test_handler_get_results(fail_handler);
        
        if (!d_assert_and_count_standalone(results.assertions_run == 1,
            "assertions_run incremented for failing assertion",
            "assertions_run should be incremented for failing assertion", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_passed == 0,
            "assertions_passed not incremented for failing assertion",
            "assertions_passed should not be incremented for failing assertion", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_failed == 1,
            "assertions_failed incremented for failing assertion",
            "assertions_failed should be incremented for failing assertion", _test_info))
        {
            all_assertions_passed = false;
        }

        d_assertion_free(fail_assertion);
        d_test_handler_free(fail_handler);
    }
    
    d_test_config_free(fail_config);

    // Test 5: Record multiple assertions (mixed pass/fail)
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new(multi_config);
    
    if (multi_handler)
    {
        // Record 3 passing assertions
        for (int i = 0; i < 3; i++)
        {
            struct d_assertion* pass = d_assert(true, "Pass", "Fail");
            d_test_handler_record_assertion(multi_handler, pass);
            d_assertion_free(pass);
        }

        // Record 2 failing assertions
        for (int i = 0; i < 2; i++)
        {
            struct d_assertion* fail = d_assert(false, "Pass", "Fail");
            d_test_handler_record_assertion(multi_handler, fail);
            d_assertion_free(fail);
        }

        struct d_test_result results = d_test_handler_get_results(multi_handler);
        
        if (!d_assert_and_count_standalone(results.assertions_run == 5,
            "assertions_run accumulates correctly (5 total)",
            "assertions_run should accumulate correctly (5 total)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_passed == 3,
            "assertions_passed accumulates correctly (3 passed)",
            "assertions_passed should accumulate correctly (3 passed)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_failed == 2,
            "assertions_failed accumulates correctly (2 failed)",
            "assertions_failed should accumulate correctly (2 failed)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Test 6: Assertions from different types (eq, neq, lt, etc.)
    struct d_test_config* types_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* types_handler = d_test_handler_new(types_config);
    
    if (types_handler)
    {
        int a = 5, b = 10;
        
        // Record various assertion types
        struct d_assertion* eq = d_assert_eq(&a, &a, NULL, "Equal", "Not equal");
        struct d_assertion* neq = d_assert_neq(&a, &b, NULL, "Not equal", "Equal");
        struct d_assertion* lt = d_assert_lt(&a, &b, NULL, "Less than", "Not less");
        struct d_assertion* gt = d_assert_gt(&b, &a, NULL, "Greater", "Not greater");
        
        d_test_handler_record_assertion(types_handler, eq);
        d_test_handler_record_assertion(types_handler, neq);
        d_test_handler_record_assertion(types_handler, lt);
        d_test_handler_record_assertion(types_handler, gt);
        
        struct d_test_result results = d_test_handler_get_results(types_handler);
        
        if (!d_assert_and_count_standalone(results.assertions_run == 4,
            "Different assertion types all tracked",
            "Different assertion types should all be tracked", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_passed == 4,
            "All different assertion types passed",
            "All different assertion types should pass", _test_info))
        {
            all_assertions_passed = false;
        }

        d_assertion_free(eq);
        d_assertion_free(neq);
        d_assertion_free(lt);
        d_assertion_free(gt);
        d_test_handler_free(types_handler);
    }
    
    d_test_config_free(types_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] `d_test_handler_record_assertion` unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] `d_test_handler_record_assertion` unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_assertion_statistics
  Tests assertion statistics accumulation across test execution.
  
  Tests:
  - Assertions counted during test execution
  - Assertions accumulated across multiple tests
  - Assertions in nested blocks counted correctly
  - Assertion statistics independent of test pass/fail
  - Manual recording vs automatic tracking consistency

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_assertion_statistics
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing `d_test_handler assertion` statistics ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Manual assertion recording accumulates
    struct d_test_config* manual_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* manual_handler = d_test_handler_new(manual_config);
    
    if (manual_handler)
    {
        // Manually record 10 passing and 5 failing assertions
        for (int i = 0; i < 10; i++)
        {
            struct d_assertion* pass = d_assert(true, "Pass", "Fail");
            d_test_handler_record_assertion(manual_handler, pass);
            d_assertion_free(pass);
        }

        for (int i = 0; i < 5; i++)
        {
            struct d_assertion* fail = d_assert(false, "Pass", "Fail");
            d_test_handler_record_assertion(manual_handler, fail);
            d_assertion_free(fail);
        }

        struct d_test_result results = d_test_handler_get_results(manual_handler);
        
        if (!d_assert_and_count_standalone(results.assertions_run == 15,
            "Manual assertion recording accumulates (15 total)",
            "Manual assertion recording should accumulate (15 total)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_passed == 10,
            "Manual passing assertions tracked (10 passed)",
            "Manual passing assertions should be tracked (10 passed)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_failed == 5,
            "Manual failing assertions tracked (5 failed)",
            "Manual failing assertions should be tracked (5 failed)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(manual_handler);
    }
    
    d_test_config_free(manual_config);

    // Test 2: Assertions persist across multiple test executions
    struct d_test_config* persist_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* persist_handler = d_test_handler_new(persist_config);
    
    if (persist_handler)
    {
        // First, record some manual assertions
        struct d_assertion* assertion1 = d_assert(true, "Pass", "Fail");
        d_test_handler_record_assertion(persist_handler, assertion1);
        d_assertion_free(assertion1);

        // Then run some tests
        struct d_test* test1 = d_test_new(handler_test_passing, NULL);
        struct d_test* test2 = d_test_new(handler_test_failing, NULL);
        
        d_test_handler_run_test(persist_handler, test1, NULL);
        d_test_handler_run_test(persist_handler, test2, NULL);
        
        // Then record more manual assertions
        struct d_assertion* assertion2 = d_assert(false, "Pass", "Fail");
        d_test_handler_record_assertion(persist_handler, assertion2);
        d_assertion_free(assertion2);

        struct d_test_result results = d_test_handler_get_results(persist_handler);
        
        if (!d_assert_and_count_standalone(results.assertions_run == 2,
            "Assertions persist across test executions (2 manual)",
            "Assertions should persist across test executions (2 manual)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.tests_run == 2,
            "Tests also tracked alongside assertions",
            "Tests should also be tracked alongside assertions", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_free(test1);
        d_test_free(test2);
        d_test_handler_free(persist_handler);
    }
    
    d_test_config_free(persist_config);

    // Test 3: Assertions in nested blocks
    struct d_test_config* nested_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* nested_handler = d_test_handler_new(nested_config);
    
    if (nested_handler)
    {
        // Record assertions at handler level
        struct d_assertion* handler_assertion = d_assert(true, "Handler", "Handler");
        d_test_handler_record_assertion(nested_handler, handler_assertion);
        d_assertion_free(handler_assertion);

        // Create nested blocks and record assertions within them
        struct d_test_block* outer = d_test_block_new(5, NULL);
        struct d_test_block* inner = d_test_block_new(3, NULL);
        
        if (outer && inner)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            // Run the nested structure
            d_test_handler_run_block(nested_handler, outer, NULL);

            // Record more assertions after execution
            struct d_assertion* post_assertion = d_assert(false, "Post", "Post");
            d_test_handler_record_assertion(nested_handler, post_assertion);
            d_assertion_free(post_assertion);

            struct d_test_result results = d_test_handler_get_results(nested_handler);
            
            if (!d_assert_and_count_standalone(results.assertions_run == 2,
                "Assertions tracked across nested execution",
                "Assertions should be tracked across nested execution", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(nested_handler);
    }
    
    d_test_config_free(nested_config);

    // Test 4: Large number of assertions
    struct d_test_config* large_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* large_handler = d_test_handler_new(large_config);
    
    if (large_handler)
    {
        // Record 100 assertions
        for (int i = 0; i < 100; i++)
        {
            bool result = (i % 3 != 0); // Fail every third assertion
            struct d_assertion* assertion = d_assert(result, "Pass", "Fail");
            d_test_handler_record_assertion(large_handler, assertion);
            d_assertion_free(assertion);
        }

        struct d_test_result results = d_test_handler_get_results(large_handler);
        
        if (!d_assert_and_count_standalone(results.assertions_run == 100,
            "Large number of assertions tracked (100)",
            "Large number of assertions should be tracked (100)", _test_info))
        {
            all_assertions_passed = false;
        }

        // 67 pass (0,1,2,4,5,7,8,...), 33 fail (3,6,9,12,...)
        if (!d_assert_and_count_standalone(results.assertions_passed == 66,
            "Correct pass count in large set (66)",                        
            "Correct pass count should be tracked in large set (66)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.assertions_failed == 34,
            "Correct fail count in large set (34)",                        
            "Correct fail count should be tracked in large set (34)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(large_handler);
    }
    
    d_test_config_free(large_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_assertion_statistics unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_assertion_statistics unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

//=============================================================================
// RESULT QUERY TESTS
//=============================================================================

/*
d_tests_sa_handler_get_results
  Tests d_test_handler_get_results accuracy.
  
  Tests:
  - Getting results from NULL handler returns empty results
  - Getting results from fresh handler returns all zeros
  - Getting results after test execution returns correct values
  - All result fields populated correctly
  - Results reflect current state accurately

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_get_results
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_get_results ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Get results from NULL handler (returns empty/zero results)
    struct d_test_result null_results = d_test_handler_get_results(NULL);
    
    if (!d_assert_and_count_standalone(
        null_results.tests_run == 0 &&
        null_results.tests_passed == 0 &&
        null_results.tests_failed == 0 &&
        null_results.assertions_run == 0 &&
        null_results.assertions_passed == 0 &&
        null_results.assertions_failed == 0 &&
        null_results.blocks_run == 0 &&
        null_results.max_depth == 0,
        "Getting results from NULL handler returns empty results",
        "Getting results from NULL handler should return empty results", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Get results from fresh handler (should be all zeros)
    struct d_test_config* fresh_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fresh_handler = d_test_handler_new(fresh_config);
    
    if (fresh_handler)
    {
        struct d_test_result fresh_results = d_test_handler_get_results(fresh_handler);
        
        if (!d_assert_and_count_standalone(
            fresh_results.tests_run == 0 &&
            fresh_results.tests_passed == 0 &&
            fresh_results.tests_failed == 0 &&
            fresh_results.assertions_run == 0 &&
            fresh_results.assertions_passed == 0 &&
            fresh_results.assertions_failed == 0 &&
            fresh_results.blocks_run == 0 &&
            fresh_results.max_depth == 0,
            "Fresh handler returns zero results",
            "Fresh handler should return zero results", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(fresh_handler);
    }
    
    d_test_config_free(fresh_config);

    // Test 3: Get results after running tests
    struct d_test_config* run_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* run_handler = d_test_handler_new(run_config);
    
    if (run_handler)
    {
        // Run 3 passing tests and 2 failing tests
        for (int i = 0; i < 3; i++)
        {
            struct d_test* pass_test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(run_handler, pass_test, NULL);
            d_test_free(pass_test);
        }

        for (int i = 0; i < 2; i++)
        {
            struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
            d_test_handler_run_test(run_handler, fail_test, NULL);
            d_test_free(fail_test);
        }

        struct d_test_result run_results = d_test_handler_get_results(run_handler);
        
        if (!d_assert_and_count_standalone(run_results.tests_run == 5,
            "tests_run correct after execution (5)",
            "tests_run should be correct after execution (5)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(run_results.tests_passed == 3,
            "tests_passed correct after execution (3)",
            "tests_passed should be correct after execution (3)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(run_results.tests_failed == 2,
            "tests_failed correct after execution (2)",
            "tests_failed should be correct after execution (2)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(run_handler);
    }
    
    d_test_config_free(run_config);

    // Test 4: Get results with assertions
    struct d_test_config* assert_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* assert_handler = d_test_handler_new(assert_config);
    
    if (assert_handler)
    {
        // Record assertions
        for (int i = 0; i < 10; i++)
        {
            struct d_assertion* pass = d_assert(true, "Pass", "Fail");
            d_test_handler_record_assertion(assert_handler, pass);
            d_assertion_free(pass);
        }

        for (int i = 0; i < 5; i++)
        {
            struct d_assertion* fail = d_assert(false, "Pass", "Fail");
            d_test_handler_record_assertion(assert_handler, fail);
            d_assertion_free(fail);
        }

        struct d_test_result assert_results = d_test_handler_get_results(assert_handler);
        
        if (!d_assert_and_count_standalone(assert_results.assertions_run == 15,
            "assertions_run correct (15)",
            "assertions_run should be correct (15)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(assert_results.assertions_passed == 10,
            "assertions_passed correct (10)",
            "assertions_passed should be correct (10)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(assert_results.assertions_failed == 5,
            "assertions_failed correct (5)",
            "assertions_failed should be correct (5)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(assert_handler);
    }
    
    d_test_config_free(assert_config);

    // Test 5: Get results with blocks and depth
    struct d_test_config* block_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* block_handler = d_test_handler_new(block_config);
    
    if (block_handler)
    {
        // Create nested blocks (depth 2)
        struct d_test_block* outer = d_test_block_new(5, NULL);
        struct d_test_block* inner = d_test_block_new(3, NULL);
        
        if (outer && inner)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            d_test_handler_run_block(block_handler, outer, NULL);

            struct d_test_result block_results = d_test_handler_get_results(block_handler);
            
            if (!d_assert_and_count_standalone(block_results.blocks_run == 2,
                "blocks_run correct for nested blocks (2)",
                "blocks_run should be correct for nested blocks (2)", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(block_results.max_depth == 2,
                "max_depth correct for nested blocks (2)",
                "max_depth should be correct for nested blocks (2)", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(block_handler);
    }
    
    d_test_config_free(block_config);

    // Test 6: All fields populated in complex scenario
    struct d_test_config* complex_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* complex_handler = d_test_handler_new(complex_config);
    
    if (complex_handler)
    {
        // Run tests
        struct d_test* test1 = d_test_new(handler_test_passing, NULL);
        struct d_test* test2 = d_test_new(handler_test_failing, NULL);
        d_test_handler_run_test(complex_handler, test1, NULL);
        d_test_handler_run_test(complex_handler, test2, NULL);
        
        // Record assertions
        struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
        d_test_handler_record_assertion(complex_handler, assertion);
        d_assertion_free(assertion);
        
        // Run block
        struct d_test_block* block = d_test_block_new(5, NULL);
        if (block)
        {
            struct d_test* test3 = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test3);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            d_test_handler_run_block(complex_handler, block, NULL);
            d_test_block_free(block);
        }

        struct d_test_result complex_results = d_test_handler_get_results(complex_handler);
        
        if (!d_assert_and_count_standalone(
            complex_results.tests_run > 0 &&
            complex_results.tests_passed > 0 &&
            complex_results.tests_failed > 0 &&
            complex_results.assertions_run > 0 &&
            complex_results.assertions_passed > 0 &&
            complex_results.blocks_run > 0 &&
            complex_results.max_depth > 0,
            "All result fields populated in complex scenario",
            "All result fields should be populated in complex scenario", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_free(test1);
        d_test_free(test2);
        d_test_handler_free(complex_handler);
    }
    
    d_test_config_free(complex_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_get_results unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_get_results unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_reset_results
  Tests d_test_handler_reset_results functionality.
  
  Tests:
  - Resetting NULL handler (should not crash)
  - Resetting fresh handler (no-op)
  - Resetting handler with accumulated results
  - All result fields reset to zero
  - current_depth reset to zero
  - Handler usable after reset

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_reset_results
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_reset_results ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Reset NULL handler (should not crash)
    d_test_handler_reset_results(NULL);
    
    if (!d_assert_and_count_standalone(true,
        "Resetting NULL handler does not crash",
        "Resetting NULL handler should not crash", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Reset fresh handler (no-op but should work)
    struct d_test_config* fresh_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fresh_handler = d_test_handler_new(fresh_config);
    
    if (fresh_handler)
    {
        d_test_handler_reset_results(fresh_handler);
        
        struct d_test_result results = d_test_handler_get_results(fresh_handler);
        
        if (!d_assert_and_count_standalone(
            results.tests_run == 0 &&
            results.tests_passed == 0 &&
            results.tests_failed == 0 &&
            results.assertions_run == 0 &&
            results.assertions_passed == 0 &&
            results.assertions_failed == 0 &&
            results.blocks_run == 0 &&
            results.max_depth == 0,
            "Fresh handler still zero after reset",
            "Fresh handler should still be zero after reset", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(fresh_handler);
    }
    
    d_test_config_free(fresh_config);

    // Test 3: Reset handler with accumulated results
    struct d_test_config* reset_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* reset_handler = d_test_handler_new(reset_config);
    
    if (reset_handler)
    {
        // Accumulate some results
        struct d_test* test1 = d_test_new(handler_test_passing, NULL);
        struct d_test* test2 = d_test_new(handler_test_failing, NULL);
        d_test_handler_run_test(reset_handler, test1, NULL);
        d_test_handler_run_test(reset_handler, test2, NULL);
        
        struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
        d_test_handler_record_assertion(reset_handler, assertion);
        d_assertion_free(assertion);

        // Verify results exist
        struct d_test_result before_reset = d_test_handler_get_results(reset_handler);
        bool has_results = (before_reset.tests_run > 0 || 
                           before_reset.assertions_run > 0);
        
        if (!d_assert_and_count_standalone(has_results,
            "Handler has accumulated results before reset",
            "Handler should have accumulated results before reset", _test_info))
        {
            all_assertions_passed = false;
        }

        // Reset
        d_test_handler_reset_results(reset_handler);

        // Verify all fields are zero
        struct d_test_result after_reset = d_test_handler_get_results(reset_handler);
        
        if (!d_assert_and_count_standalone(
            after_reset.tests_run == 0 &&
            after_reset.tests_passed == 0 &&
            after_reset.tests_failed == 0 &&
            after_reset.assertions_run == 0 &&
            after_reset.assertions_passed == 0 &&
            after_reset.assertions_failed == 0 &&
            after_reset.blocks_run == 0 &&
            after_reset.max_depth == 0,
            "All result fields reset to zero",
            "All result fields should be reset to zero", _test_info))
        {
            all_assertions_passed = false;
        }

        // Verify current_depth also reset
        if (!d_assert_and_count_standalone(reset_handler->current_depth == 0,
            "current_depth reset to zero",
            "current_depth should be reset to zero", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_free(test1);
        d_test_free(test2);
        d_test_handler_free(reset_handler);
    }
    
    d_test_config_free(reset_config);

    // Test 4: Handler usable after reset
    struct d_test_config* reuse_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* reuse_handler = d_test_handler_new(reuse_config);
    
    if (reuse_handler)
    {
        // Run tests, reset, run again
        struct d_test* test1 = d_test_new(handler_test_passing, NULL);
        d_test_handler_run_test(reuse_handler, test1, NULL);
        d_test_free(test1);

        d_test_handler_reset_results(reuse_handler);

        struct d_test* test2 = d_test_new(handler_test_passing, NULL);
        bool after_reset = d_test_handler_run_test(reuse_handler, test2, NULL);
        d_test_free(test2);
        
        if (!d_assert_and_count_standalone(after_reset == true,
            "Handler usable after reset",
            "Handler should be usable after reset", _test_info))
        {
            all_assertions_passed = false;
        }

        struct d_test_result reuse_results = d_test_handler_get_results(reuse_handler);
        
        if (!d_assert_and_count_standalone(reuse_results.tests_run == 1,
            "New results accumulate after reset",
            "New results should accumulate after reset", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(reuse_handler);
    }
    
    d_test_config_free(reuse_config);

    // Test 5: Multiple reset cycles
    struct d_test_config* cycle_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* cycle_handler = d_test_handler_new(cycle_config);
    
    if (cycle_handler)
    {
        for (int cycle = 0; cycle < 3; cycle++)
        {
            // Run some tests
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(cycle_handler, test, NULL);
            d_test_free(test);

            // Reset
            d_test_handler_reset_results(cycle_handler);

            // Verify reset
            struct d_test_result results = d_test_handler_get_results(cycle_handler);
            if (results.tests_run != 0)
            {
                all_assertions_passed = false;
                break;
            }
        }

        if (!d_assert_and_count_standalone(all_assertions_passed,
            "Multiple reset cycles work correctly",
            "Multiple reset cycles should work correctly", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(cycle_handler);
    }
    
    d_test_config_free(cycle_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_reset_results unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_reset_results unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_print_results
  Tests d_test_handler_print_results output.
  
  Tests:
  - Printing results from NULL handler (should not crash)
  - Printing results with NULL label (should not crash)
  - Printing results with valid label
  - Printing empty results
  - Printing results with various values
  - Output formatting is reasonable (no verification of exact format)

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_print_results
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_print_results ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Print results from NULL handler (should not crash)
    printf("%sTesting print with NULL handler:\n", D_INDENT);
    d_test_handler_print_results(NULL, "NULL Handler");
    
    if (!d_assert_and_count_standalone(true,
        "Printing NULL handler results does not crash",
        "Printing NULL handler results should not crash", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Print results with NULL label (should not crash)
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new(config);
    
    if (handler)
    {
        printf("%sTesting print with NULL label:\n", D_INDENT);
        d_test_handler_print_results(handler, NULL);
        
        if (!d_assert_and_count_standalone(true,
            "Printing with NULL label does not crash",
            "Printing with NULL label should not crash", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 3: Print empty results
    struct d_test_config* empty_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* empty_handler = d_test_handler_new(empty_config);
    
    if (empty_handler)
    {
        printf("%sTesting print with empty results:\n", D_INDENT);
        d_test_handler_print_results(empty_handler, "Empty Results");
        
        if (!d_assert_and_count_standalone(true,
            "Printing empty results succeeds",
            "Printing empty results should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(empty_handler);
    }
    
    d_test_config_free(empty_config);

    // Test 4: Print results with data
    struct d_test_config* data_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* data_handler = d_test_handler_new(data_config);
    
    if (data_handler)
    {
        // Run tests and record assertions
        struct d_test* test1 = d_test_new(handler_test_passing, NULL);
        struct d_test* test2 = d_test_new(handler_test_failing, NULL);
        d_test_handler_run_test(data_handler, test1, NULL);
        d_test_handler_run_test(data_handler, test2, NULL);
        
        struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
        d_test_handler_record_assertion(data_handler, assertion);
        d_assertion_free(assertion);

        printf("%sTesting print with populated results:\n", D_INDENT);
        d_test_handler_print_results(data_handler, "Populated Results");
        
        if (!d_assert_and_count_standalone(true,
            "Printing populated results succeeds",
            "Printing populated results should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_free(test1);
        d_test_free(test2);
        d_test_handler_free(data_handler);
    }
    
    d_test_config_free(data_config);

    // Test 5: Print results multiple times
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new(multi_config);
    
    if (multi_handler)
    {
        printf("%sTesting multiple prints:\n", D_INDENT);
        d_test_handler_print_results(multi_handler, "First Print");
        d_test_handler_print_results(multi_handler, "Second Print");
        d_test_handler_print_results(multi_handler, "Third Print");
        
        if (!d_assert_and_count_standalone(true,
            "Multiple prints succeed",
            "Multiple prints should succeed", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_print_results unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_print_results unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_get_pass_rate
  Tests d_test_handler_get_pass_rate calculations.
  
  Tests:
  - Pass rate from NULL handler (should return 0.0)
  - Pass rate with no tests run (should return 0.0)
  - Pass rate with all tests passing (should return 100.0)
  - Pass rate with all tests failing (should return 0.0)
  - Pass rate with mixed results (should return correct percentage)
  - Floating point precision

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_get_pass_rate
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_get_pass_rate ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Pass rate from NULL handler
    double null_rate = d_test_handler_get_pass_rate(NULL);
    
    if (!d_assert_and_count_standalone(null_rate == 0.0,
        "Pass rate from NULL handler is 0.0",
        "Pass rate from NULL handler should be 0.0", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Pass rate with no tests run
    struct d_test_config* empty_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* empty_handler = d_test_handler_new(empty_config);
    
    if (empty_handler)
    {
        double empty_rate = d_test_handler_get_pass_rate(empty_handler);
        
        if (!d_assert_and_count_standalone(empty_rate == 0.0,
            "Pass rate with no tests is 0.0",
            "Pass rate with no tests should be 0.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(empty_handler);
    }
    
    d_test_config_free(empty_config);

    // Test 3: Pass rate with all tests passing (100%)
    struct d_test_config* all_pass_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* all_pass_handler = d_test_handler_new(all_pass_config);
    
    if (all_pass_handler)
    {
        // Run 5 passing tests
        for (int i = 0; i < 5; i++)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(all_pass_handler, test, NULL);
            d_test_free(test);
        }

        double all_pass_rate = d_test_handler_get_pass_rate(all_pass_handler);
        
        if (!d_assert_and_count_standalone(all_pass_rate == 100.0,
            "Pass rate with all passing is 100.0",
            "Pass rate with all passing should be 100.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(all_pass_handler);
    }
    
    d_test_config_free(all_pass_config);

    // Test 4: Pass rate with all tests failing (0%)
    struct d_test_config* all_fail_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* all_fail_handler = d_test_handler_new(all_fail_config);
    
    if (all_fail_handler)
    {
        // Run 5 failing tests
        for (int i = 0; i < 5; i++)
        {
            struct d_test* test = d_test_new(handler_test_failing, NULL);
            d_test_handler_run_test(all_fail_handler, test, NULL);
            d_test_free(test);
        }

        double all_fail_rate = d_test_handler_get_pass_rate(all_fail_handler);
        
        if (!d_assert_and_count_standalone(all_fail_rate == 0.0,
            "Pass rate with all failing is 0.0",
            "Pass rate with all failing should be 0.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(all_fail_handler);
    }
    
    d_test_config_free(all_fail_config);

    // Test 5: Pass rate with mixed results (50%)
    struct d_test_config* mixed_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* mixed_handler = d_test_handler_new(mixed_config);
    
    if (mixed_handler)
    {
        // Run 3 passing and 3 failing (50%)
        for (int i = 0; i < 3; i++)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(mixed_handler, test, NULL);
            d_test_free(test);
        }

        for (int i = 0; i < 3; i++)
        {
            struct d_test* test = d_test_new(handler_test_failing, NULL);
            d_test_handler_run_test(mixed_handler, test, NULL);
            d_test_free(test);
        }

        double mixed_rate = d_test_handler_get_pass_rate(mixed_handler);
        
        if (!d_assert_and_count_standalone(mixed_rate == 50.0,
            "Pass rate with 50% passing is 50.0",
            "Pass rate with 50% passing should be 50.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(mixed_handler);
    }
    
    d_test_config_free(mixed_config);

    // Test 6: Pass rate with different percentages
    struct d_test_config* percent_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* percent_handler = d_test_handler_new(percent_config);
    
    if (percent_handler)
    {
        // Run 7 passing out of 10 (70%)
        for (int i = 0; i < 7; i++)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(percent_handler, test, NULL);
            d_test_free(test);
        }

        for (int i = 0; i < 3; i++)
        {
            struct d_test* test = d_test_new(handler_test_failing, NULL);
            d_test_handler_run_test(percent_handler, test, NULL);
            d_test_free(test);
        }

        double percent_rate = d_test_handler_get_pass_rate(percent_handler);
        
        // Allow small floating point tolerance
        bool correct_rate = (percent_rate >= 69.9 && percent_rate <= 70.1);
        
        if (!d_assert_and_count_standalone(correct_rate,
            "Pass rate with 70% passing is approximately 70.0",
            "Pass rate with 70% passing should be approximately 70.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(percent_handler);
    }
    
    d_test_config_free(percent_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_get_pass_rate unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_get_pass_rate unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_get_assertion_rate
  Tests d_test_handler_get_assertion_rate calculations.
  
  Tests:
  - Assertion rate from NULL handler (should return 0.0)
  - Assertion rate with no assertions (should return 0.0)
  - Assertion rate with all assertions passing (should return 100.0)
  - Assertion rate with all assertions failing (should return 0.0)
  - Assertion rate with mixed results (should return correct percentage)
  - Floating point precision

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_get_assertion_rate
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_get_assertion_rate ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Assertion rate from NULL handler
    double null_rate = d_test_handler_get_assertion_rate(NULL);
    
    if (!d_assert_and_count_standalone(null_rate == 0.0,
        "Assertion rate from NULL handler is 0.0",
        "Assertion rate from NULL handler should be 0.0", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Assertion rate with no assertions
    struct d_test_config* empty_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* empty_handler = d_test_handler_new(empty_config);
    
    if (empty_handler)
    {
        double empty_rate = d_test_handler_get_assertion_rate(empty_handler);
        
        if (!d_assert_and_count_standalone(empty_rate == 0.0,
            "Assertion rate with no assertions is 0.0",
            "Assertion rate with no assertions should be 0.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(empty_handler);
    }
    
    d_test_config_free(empty_config);

    // Test 3: Assertion rate with all passing (100%)
    struct d_test_config* all_pass_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* all_pass_handler = d_test_handler_new(all_pass_config);
    
    if (all_pass_handler)
    {
        // Record 10 passing assertions
        for (int i = 0; i < 10; i++)
        {
            struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
            d_test_handler_record_assertion(all_pass_handler, assertion);
            d_assertion_free(assertion);
        }

        double all_pass_rate = d_test_handler_get_assertion_rate(all_pass_handler);
        
        if (!d_assert_and_count_standalone(all_pass_rate == 100.0,
            "Assertion rate with all passing is 100.0",
            "Assertion rate with all passing should be 100.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(all_pass_handler);
    }
    
    d_test_config_free(all_pass_config);

    // Test 4: Assertion rate with all failing (0%)
    struct d_test_config* all_fail_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* all_fail_handler = d_test_handler_new(all_fail_config);
    
    if (all_fail_handler)
    {
        // Record 10 failing assertions
        for (int i = 0; i < 10; i++)
        {
            struct d_assertion* assertion = d_assert(false, "Pass", "Fail");
            d_test_handler_record_assertion(all_fail_handler, assertion);
            d_assertion_free(assertion);
        }

        double all_fail_rate = d_test_handler_get_assertion_rate(all_fail_handler);
        
        if (!d_assert_and_count_standalone(all_fail_rate == 0.0,
            "Assertion rate with all failing is 0.0",
            "Assertion rate with all failing should be 0.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(all_fail_handler);
    }
    
    d_test_config_free(all_fail_config);

    // Test 5: Assertion rate with mixed results (60%)
    struct d_test_config* mixed_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* mixed_handler = d_test_handler_new(mixed_config);
    
    if (mixed_handler)
    {
        // Record 6 passing and 4 failing (60%)
        for (int i = 0; i < 6; i++)
        {
            struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
            d_test_handler_record_assertion(mixed_handler, assertion);
            d_assertion_free(assertion);
        }

        for (int i = 0; i < 4; i++)
        {
            struct d_assertion* assertion = d_assert(false, "Pass", "Fail");
            d_test_handler_record_assertion(mixed_handler, assertion);
            d_assertion_free(assertion);
        }

        double mixed_rate = d_test_handler_get_assertion_rate(mixed_handler);
        
        // Allow small floating point tolerance
        bool correct_rate = (mixed_rate >= 59.9 && mixed_rate <= 60.1);
        
        if (!d_assert_and_count_standalone(correct_rate,
            "Assertion rate with 60% passing is approximately 60.0",
            "Assertion rate with 60% passing should be approximately 60.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(mixed_handler);
    }
    
    d_test_config_free(mixed_config);

    // Test 6: Assertion rate with different percentages
    struct d_test_config* percent_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* percent_handler = d_test_handler_new(percent_config);
    
    if (percent_handler)
    {
        // Record 17 passing out of 20 (85%)
        for (int i = 0; i < 17; i++)
        {
            struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
            d_test_handler_record_assertion(percent_handler, assertion);
            d_assertion_free(assertion);
        }

        for (int i = 0; i < 3; i++)
        {
            struct d_assertion* assertion = d_assert(false, "Pass", "Fail");
            d_test_handler_record_assertion(percent_handler, assertion);
            d_assertion_free(assertion);
        }

        double percent_rate = d_test_handler_get_assertion_rate(percent_handler);
        
        // Allow small floating point tolerance
        bool correct_rate = (percent_rate >= 84.9 && percent_rate <= 85.1);
        
        if (!d_assert_and_count_standalone(correct_rate,
            "Assertion rate with 85% passing is approximately 85.0",
            "Assertion rate with 85% passing should be approximately 85.0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(percent_handler);
    }
    
    d_test_config_free(percent_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_get_assertion_rate unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_get_assertion_rate unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

//=============================================================================
// EVENT EMISSION TESTS
//=============================================================================

/*
d_tests_sa_handler_emit_event
  Tests d_test_handler_emit_event for all lifecycle events.
  
  Tests:
  - Emitting events on NULL handler (should not crash)
  - Emitting events on handler without event system (should not crash)
  - Emitting each lifecycle event type
  - Event context parameter passing
  - Events fire registered callbacks
  - Multiple events can be emitted sequentially

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_emit_event
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler_emit_event ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Emit event on NULL handler (should not crash)
    struct d_test_context null_context = {
        .handler = NULL,
        .current_test = NULL,
        .current_block = NULL,
        .event_type = D_TEST_EVENT_START,
        .depth = 0,
        .last_assertion = NULL
    };
    
    d_test_handler_emit_event(NULL, D_TEST_EVENT_START, &null_context);
    
    if (!d_assert_and_count_standalone(true,
        "Emitting event on NULL handler does not crash",
        "Emitting event on NULL handler should not crash", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Emit event on handler without event system (should not crash)
    struct d_test_config* no_events_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* no_events_handler = d_test_handler_new(no_events_config);
    
    if (no_events_handler)
    {
        struct d_test_context context = {
            .handler = no_events_handler,
            .current_test = NULL,
            .current_block = NULL,
            .event_type = D_TEST_EVENT_START,
            .depth = 0,
            .last_assertion = NULL
        };
        
        d_test_handler_emit_event(no_events_handler, D_TEST_EVENT_START, &context);
        
        if (!d_assert_and_count_standalone(true,
            "Emitting event on handler without event system does not crash",
            "Emitting event on handler without event system should not crash", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(no_events_handler);
    }
    
    d_test_config_free(no_events_config);

    // Test 3: Emit SETUP event
    reset_event_counters();
    struct d_test_config* setup_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* setup_handler = d_test_handler_new_with_events(setup_config, 10);
    
    if (setup_handler)
    {
        d_test_handler_register_listener(setup_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        
        struct d_test_context context = {
            .handler = setup_handler,
            .current_test = NULL,
            .current_block = NULL,
            .event_type = D_TEST_EVENT_SETUP,
            .depth = 0,
            .last_assertion = NULL
        };
        
        d_test_handler_emit_event(setup_handler, D_TEST_EVENT_SETUP, &context);
        
        if (!d_assert_and_count_standalone(g_event_setup_count == 1,
            "SETUP event fires callback",
            "SETUP event should fire callback", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(setup_handler);
    }
    
    d_test_config_free(setup_config);

    // Test 4: Emit all lifecycle events
    reset_event_counters();
    struct d_test_config* all_events_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* all_events_handler = d_test_handler_new_with_events(all_events_config, 20);
    
    if (all_events_handler)
    {
        // Register all listeners
        d_test_handler_register_listener(all_events_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        d_test_handler_register_listener(all_events_handler, D_TEST_EVENT_START, callback_start, true);
        d_test_handler_register_listener(all_events_handler, D_TEST_EVENT_SUCCESS, callback_success, true);
        d_test_handler_register_listener(all_events_handler, D_TEST_EVENT_FAILURE, callback_failure, true);
        d_test_handler_register_listener(all_events_handler, D_TEST_EVENT_END, callback_end, true);
        d_test_handler_register_listener(all_events_handler, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);
        
        struct d_test_context context = {
            .handler = all_events_handler,
            .current_test = NULL,
            .current_block = NULL,
            .event_type = D_TEST_EVENT_SETUP,
            .depth = 0,
            .last_assertion = NULL
        };
        
        // Emit all events
        d_test_handler_emit_event(all_events_handler, D_TEST_EVENT_SETUP, &context);
        d_test_handler_emit_event(all_events_handler, D_TEST_EVENT_START, &context);
        d_test_handler_emit_event(all_events_handler, D_TEST_EVENT_SUCCESS, &context);
        d_test_handler_emit_event(all_events_handler, D_TEST_EVENT_END, &context);
        d_test_handler_emit_event(all_events_handler, D_TEST_EVENT_TEAR_DOWN, &context);
        
        if (!d_assert_and_count_standalone(
            g_event_setup_count == 1 &&
            g_event_start_count == 1 &&
            g_event_success_count == 1 &&
            g_event_end_count == 1 &&
            g_event_teardown_count == 1,
            "All lifecycle events fire their callbacks",
            "All lifecycle events should fire their callbacks", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(all_events_handler);
    }
    
    d_test_config_free(all_events_config);

    // Test 5: Multiple emissions of same event
    reset_event_counters();
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new_with_events(multi_config, 10);
    
    if (multi_handler)
    {
        d_test_handler_register_listener(multi_handler, D_TEST_EVENT_START, callback_start, true);
        
        struct d_test_context context = {
            .handler = multi_handler,
            .current_test = NULL,
            .current_block = NULL,
            .event_type = D_TEST_EVENT_START,
            .depth = 0,
            .last_assertion = NULL
        };
        
        // Emit same event multiple times
        for (int i = 0; i < 5; i++)
        {
            d_test_handler_emit_event(multi_handler, D_TEST_EVENT_START, &context);
        }
        
        if (!d_assert_and_count_standalone(g_event_start_count == 5,
            "Multiple emissions of same event all fire callback",
            "Multiple emissions of same event should all fire callback", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Test 6: Events with NULL context (should not crash)
    struct d_test_config* null_ctx_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* null_ctx_handler = d_test_handler_new_with_events(null_ctx_config, 10);
    
    if (null_ctx_handler)
    {
        d_test_handler_emit_event(null_ctx_handler, D_TEST_EVENT_START, NULL);
        
        if (!d_assert_and_count_standalone(true,
            "Emitting event with NULL context does not crash",
            "Emitting event with NULL context should not crash", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(null_ctx_handler);
    }
    
    d_test_config_free(null_ctx_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_emit_event unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_emit_event unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_event_lifecycle
  Tests event firing during test execution.
  
  Tests:
  - Events fire during test execution
  - Complete lifecycle events for passing test
  - Complete lifecycle events for failing test
  - Events fire in correct order
  - Events fire for nested blocks
  - Event context contains correct information

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_event_lifecycle
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler event lifecycle ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Events fire during passing test execution
    reset_event_counters();
    struct d_test_config* pass_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* pass_handler = d_test_handler_new_with_events(pass_config, 20);
    
    if (pass_handler)
    {
        // Register all event listeners
        d_test_handler_register_listener(pass_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        d_test_handler_register_listener(pass_handler, D_TEST_EVENT_START, callback_start, true);
        d_test_handler_register_listener(pass_handler, D_TEST_EVENT_SUCCESS, callback_success, true);
        d_test_handler_register_listener(pass_handler, D_TEST_EVENT_FAILURE, callback_failure, true);
        d_test_handler_register_listener(pass_handler, D_TEST_EVENT_END, callback_end, true);
        d_test_handler_register_listener(pass_handler, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);
        
        struct d_test* test = d_test_new(handler_test_passing, NULL);
        d_test_handler_run_test(pass_handler, test, NULL);
        
        if (!d_assert_and_count_standalone(
            g_event_setup_count == 1 &&
            g_event_start_count == 1 &&
            g_event_success_count == 1 &&
            g_event_failure_count == 0 &&  // Should NOT fire for passing test
            g_event_end_count == 1 &&
            g_event_teardown_count == 1,
            "Correct events fire during passing test execution",
            "Correct events should fire during passing test execution", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_free(test);
        d_test_handler_free(pass_handler);
    }
    
    d_test_config_free(pass_config);

    // Test 2: Events fire during failing test execution
    reset_event_counters();
    struct d_test_config* fail_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fail_handler = d_test_handler_new_with_events(fail_config, 20);
    
    if (fail_handler)
    {
        // Register all event listeners
        d_test_handler_register_listener(fail_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        d_test_handler_register_listener(fail_handler, D_TEST_EVENT_START, callback_start, true);
        d_test_handler_register_listener(fail_handler, D_TEST_EVENT_SUCCESS, callback_success, true);
        d_test_handler_register_listener(fail_handler, D_TEST_EVENT_FAILURE, callback_failure, true);
        d_test_handler_register_listener(fail_handler, D_TEST_EVENT_END, callback_end, true);
        d_test_handler_register_listener(fail_handler, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);
        
        struct d_test* test = d_test_new(handler_test_failing, NULL);
        d_test_handler_run_test(fail_handler, test, NULL);
        
        if (!d_assert_and_count_standalone(
            g_event_setup_count == 1 &&
            g_event_start_count == 1 &&
            g_event_success_count == 0 &&  // Should NOT fire for failing test
            g_event_failure_count == 1 &&
            g_event_end_count == 1 &&
            g_event_teardown_count == 1,
            "Correct events fire during failing test execution",
            "Correct events should fire during failing test execution", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_free(test);
        d_test_handler_free(fail_handler);
    }
    
    d_test_config_free(fail_config);

    // Test 3: Events fire for multiple tests
    reset_event_counters();
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new_with_events(multi_config, 20);
    
    if (multi_handler)
    {
        d_test_handler_register_listener(multi_handler, D_TEST_EVENT_START, callback_start, true);
        d_test_handler_register_listener(multi_handler, D_TEST_EVENT_END, callback_end, true);
        
        // Run 3 tests
        for (int i = 0; i < 3; i++)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(multi_handler, test, NULL);
            d_test_free(test);
        }
        
        if (!d_assert_and_count_standalone(
            g_event_start_count == 3 &&
            g_event_end_count == 3,
            "Events fire for each test execution",
            "Events should fire for each test execution", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Test 4: Events fire during block execution
    reset_event_counters();
    struct d_test_config* block_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* block_handler = d_test_handler_new_with_events(block_config, 20);
    
    if (block_handler)
    {
        d_test_handler_register_listener(block_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        d_test_handler_register_listener(block_handler, D_TEST_EVENT_START, callback_start, true);
        d_test_handler_register_listener(block_handler, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);
        
        struct d_test_block* block = d_test_block_new(10, NULL);
        
        if (block)
        {
            // Add 2 tests to block
            for (int i = 0; i < 2; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[i] = *type;
                free(type);
            }
            block->count = 2;

            d_test_handler_run_block(block_handler, block, NULL);
            
            // Block should fire setup/teardown, each test should fire start
            if (!d_assert_and_count_standalone(
                g_event_setup_count >= 2 &&  // At least 2 (block + tests)
                g_event_start_count >= 2 &&   // At least 2 (for 2 tests)
                g_event_teardown_count >= 2,
                "Events fire during block execution",
                "Events should fire during block execution", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(block_handler);
    }
    
    d_test_config_free(block_config);

    // Test 5: Events fire for nested blocks
    reset_event_counters();
    struct d_test_config* nested_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* nested_handler = d_test_handler_new_with_events(nested_config, 20);
    
    if (nested_handler)
    {
        d_test_handler_register_listener(nested_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        d_test_handler_register_listener(nested_handler, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);
        
        struct d_test_block* outer = d_test_block_new(10, NULL);
        struct d_test_block* inner = d_test_block_new(5, NULL);
        
        if (outer && inner)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            d_test_handler_run_block(nested_handler, outer, NULL);
            
            // Should have setup/teardown for outer block, inner block, and test
            if (!d_assert_and_count_standalone(
                g_event_setup_count >= 3 &&
                g_event_teardown_count >= 3,
                "Events fire for nested block structure",
                "Events should fire for nested block structure", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(nested_handler);
    }
    
    d_test_config_free(nested_config);

    // Test 6: Disabled listeners don't fire
    reset_event_counters();
    struct d_test_config* disabled_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* disabled_handler = d_test_handler_new_with_events(disabled_config, 20);
    
    if (disabled_handler)
    {
        // Register disabled listener
        d_test_handler_register_listener(disabled_handler, D_TEST_EVENT_START, callback_start, false);
        
        struct d_test* test = d_test_new(handler_test_passing, NULL);
        d_test_handler_run_test(disabled_handler, test, NULL);
        
        if (!d_assert_and_count_standalone(g_event_start_count == 0,
            "Disabled listener does not fire during execution",
            "Disabled listener should not fire during execution", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_free(test);
        d_test_handler_free(disabled_handler);
    }
    
    d_test_config_free(disabled_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_event_lifecycle unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_event_lifecycle unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

//=============================================================================
// STATISTICS AND DEPTH TRACKING TESTS
//=============================================================================

/*
d_tests_sa_handler_depth_tracking
  Tests depth tracking during nested block execution.
  
  Tests:
  - Depth starts at 0
  - Depth increments when entering block
  - Depth decrements when exiting block
  - Depth tracking with single block
  - Depth tracking with nested blocks
  - current_depth resets to 0 after execution

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_depth_tracking
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler depth tracking ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Fresh handler starts at depth 0
    struct d_test_config* fresh_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fresh_handler = d_test_handler_new(fresh_config);
    
    if (fresh_handler)
    {
        if (!d_assert_and_count_standalone(fresh_handler->current_depth == 0,
            "Fresh handler has current_depth of 0",
            "Fresh handler should have current_depth of 0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(fresh_handler);
    }
    
    d_test_config_free(fresh_config);

    // Test 2: Depth resets to 0 after single block execution
    struct d_test_config* single_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* single_handler = d_test_handler_new(single_config);
    
    if (single_handler)
    {
        struct d_test_block* block = d_test_block_new(10, NULL);
        
        if (block)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            d_test_handler_run_block(single_handler, block, NULL);
            
            if (!d_assert_and_count_standalone(single_handler->current_depth == 0,
                "current_depth resets to 0 after single block execution",
                "current_depth should reset to 0 after single block execution", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(single_handler);
    }
    
    d_test_config_free(single_config);

    // Test 3: max_depth tracks single level
    struct d_test_config* max1_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* max1_handler = d_test_handler_new(max1_config);
    
    if (max1_handler)
    {
        struct d_test_block* block = d_test_block_new(5, NULL);
        
        if (block)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            d_test_handler_run_block(max1_handler, block, NULL);

            struct d_test_result results = d_test_handler_get_results(max1_handler);
            
            if (!d_assert_and_count_standalone(results.max_depth == 1,
                "max_depth is 1 for single level nesting",
                "max_depth should be 1 for single level nesting", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(max1_handler);
    }
    
    d_test_config_free(max1_config);

    // Test 4: Depth resets after nested block execution
    struct d_test_config* nested_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* nested_handler = d_test_handler_new(nested_config);
    
    if (nested_handler)
    {
        struct d_test_block* outer = d_test_block_new(10, NULL);
        struct d_test_block* inner = d_test_block_new(5, NULL);
        
        if (outer && inner)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            d_test_handler_run_block(nested_handler, outer, NULL);
            
            if (!d_assert_and_count_standalone(nested_handler->current_depth == 0,
                "current_depth resets to 0 after nested execution",
                "current_depth should reset to 0 after nested execution", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(nested_handler);
    }
    
    d_test_config_free(nested_config);

    // Test 5: Multiple sequential executions maintain correct depth
    struct d_test_config* seq_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* seq_handler = d_test_handler_new(seq_config);
    
    if (seq_handler)
    {
        bool all_depths_correct = true;
        
        for (int i = 0; i < 3; i++)
        {
            struct d_test_block* block = d_test_block_new(5, NULL);
            if (block)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[0] = *type;
                block->count = 1;
                free(type);

                d_test_handler_run_block(seq_handler, block, NULL);
                
                if (seq_handler->current_depth != 0)
                {
                    all_depths_correct = false;
                }

                d_test_block_free(block);
            }
        }
        
        if (!d_assert_and_count_standalone(all_depths_correct,
            "Depth tracking correct across multiple sequential executions",
            "Depth tracking should be correct across multiple sequential executions", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(seq_handler);
    }
    
    d_test_config_free(seq_config);

    // Test 6: Depth tracking with mixed tests and blocks
    struct d_test_config* mixed_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* mixed_handler = d_test_handler_new(mixed_config);
    
    if (mixed_handler)
    {
        struct d_test_block* block = d_test_block_new(10, NULL);
        
        if (block)
        {
            // Add direct test (doesn't increase depth)
            struct d_test* test1 = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type1 = d_test_type_new(D_TEST_TYPE_TEST, test1);
            block->tests[0] = *type1;
            free(type1);

            // Add nested block (increases depth)
            struct d_test_block* nested = d_test_block_new(5, NULL);
            if (nested)
            {
                struct d_test* test2 = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type2 = d_test_type_new(D_TEST_TYPE_TEST, test2);
                nested->tests[0] = *type2;
                nested->count = 1;
                free(type2);

                struct d_test_type* nested_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, nested);
                block->tests[1] = *nested_type;
                free(nested_type);
            }

            block->count = 2;

            d_test_handler_run_block(mixed_handler, block, NULL);
            
            if (!d_assert_and_count_standalone(mixed_handler->current_depth == 0,
                "Depth tracking correct with mixed tests and blocks",
                "Depth tracking should be correct with mixed tests and blocks", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(mixed_handler);
    }
    
    d_test_config_free(mixed_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_depth_tracking unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_depth_tracking unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_max_depth
  Tests max_depth calculation.
  
  Tests:
  - max_depth starts at 0
  - max_depth for single level (should be 1)
  - max_depth for two levels (should be 2)
  - max_depth for three levels (should be 3)
  - max_depth tracks highest across multiple executions
  - max_depth persists across executions until reset

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_max_depth
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler max_depth ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Fresh handler has max_depth 0
    struct d_test_config* fresh_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fresh_handler = d_test_handler_new(fresh_config);
    
    if (fresh_handler)
    {
        struct d_test_result results = d_test_handler_get_results(fresh_handler);
        
        if (!d_assert_and_count_standalone(results.max_depth == 0,
            "Fresh handler has max_depth of 0",
            "Fresh handler should have max_depth of 0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(fresh_handler);
    }
    
    d_test_config_free(fresh_config);

    // Test 2: Single block (depth 1)
    struct d_test_config* depth1_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* depth1_handler = d_test_handler_new(depth1_config);
    
    if (depth1_handler)
    {
        struct d_test_block* block = d_test_block_new(5, NULL);
        
        if (block)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            d_test_handler_run_block(depth1_handler, block, NULL);

            struct d_test_result results = d_test_handler_get_results(depth1_handler);
            
            if (!d_assert_and_count_standalone(results.max_depth == 1,
                "Single block has max_depth of 1",
                "Single block should have max_depth of 1", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(depth1_handler);
    }
    
    d_test_config_free(depth1_config);

    // Test 3: Two level nesting (depth 2)
    struct d_test_config* depth2_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* depth2_handler = d_test_handler_new(depth2_config);
    
    if (depth2_handler)
    {
        struct d_test_block* outer = d_test_block_new(5, NULL);
        struct d_test_block* inner = d_test_block_new(3, NULL);
        
        if (outer && inner)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            d_test_handler_run_block(depth2_handler, outer, NULL);

            struct d_test_result results = d_test_handler_get_results(depth2_handler);
            
            if (!d_assert_and_count_standalone(results.max_depth == 2,
                "Two level nesting has max_depth of 2",
                "Two level nesting should have max_depth of 2", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(depth2_handler);
    }
    
    d_test_config_free(depth2_config);

    // Test 4: Three level nesting (depth 3)
    struct d_test_config* depth3_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* depth3_handler = d_test_handler_new(depth3_config);
    
    if (depth3_handler)
    {
        struct d_test_block* level1 = d_test_block_new(5, NULL);
        struct d_test_block* level2 = d_test_block_new(5, NULL);
        struct d_test_block* level3 = d_test_block_new(5, NULL);
        
        if (level1 && level2 && level3)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            level3->tests[0] = *test_type;
            level3->count = 1;
            free(test_type);

            struct d_test_type* level3_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, level3);
            level2->tests[0] = *level3_type;
            level2->count = 1;
            free(level3_type);

            struct d_test_type* level2_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, level2);
            level1->tests[0] = *level2_type;
            level1->count = 1;
            free(level2_type);

            d_test_handler_run_block(depth3_handler, level1, NULL);

            struct d_test_result results = d_test_handler_get_results(depth3_handler);
            
            if (!d_assert_and_count_standalone(results.max_depth == 3,
                "Three level nesting has max_depth of 3",
                "Three level nesting should have max_depth of 3", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(level1);
        }

        d_test_handler_free(depth3_handler);
    }
    
    d_test_config_free(depth3_config);

    // Test 5: max_depth tracks highest across multiple executions
    struct d_test_config* track_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* track_handler = d_test_handler_new(track_config);
    
    if (track_handler)
    {
        // Run shallow block (depth 1)
        struct d_test_block* shallow = d_test_block_new(5, NULL);
        if (shallow)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            shallow->tests[0] = *type;
            shallow->count = 1;
            free(type);

            d_test_handler_run_block(track_handler, shallow, NULL);
            d_test_block_free(shallow);
        }

        // Run deeper block (depth 3)
        struct d_test_block* deep1 = d_test_block_new(5, NULL);
        struct d_test_block* deep2 = d_test_block_new(5, NULL);
        struct d_test_block* deep3 = d_test_block_new(5, NULL);
        
        if (deep1 && deep2 && deep3)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            deep3->tests[0] = *test_type;
            deep3->count = 1;
            free(test_type);

            struct d_test_type* deep3_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, deep3);
            deep2->tests[0] = *deep3_type;
            deep2->count = 1;
            free(deep3_type);

            struct d_test_type* deep2_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, deep2);
            deep1->tests[0] = *deep2_type;
            deep1->count = 1;
            free(deep2_type);

            d_test_handler_run_block(track_handler, deep1, NULL);
            d_test_block_free(deep1);
        }

        // Run another shallow block (depth 1)
        struct d_test_block* shallow2 = d_test_block_new(5, NULL);
        if (shallow2)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            shallow2->tests[0] = *type;
            shallow2->count = 1;
            free(type);

            d_test_handler_run_block(track_handler, shallow2, NULL);
            d_test_block_free(shallow2);
        }

        struct d_test_result results = d_test_handler_get_results(track_handler);
        
        if (!d_assert_and_count_standalone(results.max_depth == 3,
            "max_depth tracks highest depth across executions (3)",
            "max_depth should track highest depth across executions (3)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(track_handler);
    }
    
    d_test_config_free(track_config);

    // Test 6: max_depth persists until reset
    struct d_test_config* persist_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* persist_handler = d_test_handler_new(persist_config);
    
    if (persist_handler)
    {
        // Run nested block
        struct d_test_block* outer = d_test_block_new(5, NULL);
        struct d_test_block* inner = d_test_block_new(5, NULL);
        
        if (outer && inner)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            d_test_handler_run_block(persist_handler, outer, NULL);
            d_test_block_free(outer);
        }

        struct d_test_result before_reset = d_test_handler_get_results(persist_handler);
        
        if (!d_assert_and_count_standalone(before_reset.max_depth == 2,
            "max_depth persists before reset",
            "max_depth should persist before reset", _test_info))
        {
            all_assertions_passed = false;
        }

        // Reset
        d_test_handler_reset_results(persist_handler);

        struct d_test_result after_reset = d_test_handler_get_results(persist_handler);
        
        if (!d_assert_and_count_standalone(after_reset.max_depth == 0,
            "max_depth resets to 0 after reset",
            "max_depth should reset to 0 after reset", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(persist_handler);
    }
    
    d_test_config_free(persist_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_max_depth unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_max_depth unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_block_counting
  Tests block count tracking.
  
  Tests:
  - blocks_run starts at 0
  - Single block increments to 1
  - Multiple blocks increment correctly
  - Nested blocks count each level
  - blocks_run persists across executions until reset
  - blocks_run independent of block success/failure

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_block_counting
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler block counting ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Fresh handler has blocks_run 0
    struct d_test_config* fresh_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fresh_handler = d_test_handler_new(fresh_config);
    
    if (fresh_handler)
    {
        struct d_test_result results = d_test_handler_get_results(fresh_handler);
        
        if (!d_assert_and_count_standalone(results.blocks_run == 0,
            "Fresh handler has blocks_run of 0",
            "Fresh handler should have blocks_run of 0", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(fresh_handler);
    }
    
    d_test_config_free(fresh_config);

    // Test 2: Single block increments blocks_run to 1
    struct d_test_config* single_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* single_handler = d_test_handler_new(single_config);
    
    if (single_handler)
    {
        struct d_test_block* block = d_test_block_new(5, NULL);
        
        if (block)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            d_test_handler_run_block(single_handler, block, NULL);

            struct d_test_result results = d_test_handler_get_results(single_handler);
            
            if (!d_assert_and_count_standalone(results.blocks_run == 1,
                "Single block execution sets blocks_run to 1",
                "Single block execution should set blocks_run to 1", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(single_handler);
    }
    
    d_test_config_free(single_config);

    // Test 3: Multiple sequential blocks accumulate
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new(multi_config);
    
    if (multi_handler)
    {
        // Run 5 blocks
        for (int i = 0; i < 5; i++)
        {
            struct d_test_block* block = d_test_block_new(5, NULL);
            if (block)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[0] = *type;
                block->count = 1;
                free(type);

                d_test_handler_run_block(multi_handler, block, NULL);
                d_test_block_free(block);
            }
        }

        struct d_test_result results = d_test_handler_get_results(multi_handler);
        
        if (!d_assert_and_count_standalone(results.blocks_run == 5,
            "Multiple blocks accumulate correctly (5)",
            "Multiple blocks should accumulate correctly (5)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Test 4: Nested blocks count each level
    struct d_test_config* nested_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* nested_handler = d_test_handler_new(nested_config);
    
    if (nested_handler)
    {
        struct d_test_block* outer = d_test_block_new(5, NULL);
        struct d_test_block* inner = d_test_block_new(3, NULL);
        
        if (outer && inner)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            d_test_handler_run_block(nested_handler, outer, NULL);

            struct d_test_result results = d_test_handler_get_results(nested_handler);
            
            if (!d_assert_and_count_standalone(results.blocks_run == 2,
                "Nested blocks count each level (2)",
                "Nested blocks should count each level (2)", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(nested_handler);
    }
    
    d_test_config_free(nested_config);

    // Test 5: Three level nesting counts all blocks
    struct d_test_config* deep_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* deep_handler = d_test_handler_new(deep_config);
    
    if (deep_handler)
    {
        struct d_test_block* level1 = d_test_block_new(5, NULL);
        struct d_test_block* level2 = d_test_block_new(5, NULL);
        struct d_test_block* level3 = d_test_block_new(5, NULL);
        
        if (level1 && level2 && level3)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            level3->tests[0] = *test_type;
            level3->count = 1;
            free(test_type);

            struct d_test_type* level3_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, level3);
            level2->tests[0] = *level3_type;
            level2->count = 1;
            free(level3_type);

            struct d_test_type* level2_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, level2);
            level1->tests[0] = *level2_type;
            level1->count = 1;
            free(level2_type);

            d_test_handler_run_block(deep_handler, level1, NULL);

            struct d_test_result results = d_test_handler_get_results(deep_handler);
            
            if (!d_assert_and_count_standalone(results.blocks_run == 3,
                "Three level nesting counts all blocks (3)",
                "Three level nesting should count all blocks (3)", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(level1);
        }

        d_test_handler_free(deep_handler);
    }
    
    d_test_config_free(deep_config);

    // Test 6: Empty block still counts
    struct d_test_config* empty_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* empty_handler = d_test_handler_new(empty_config);
    
    if (empty_handler)
    {
        struct d_test_block* empty_block = d_test_block_new(5, NULL);
        
        if (empty_block)
        {
            // Don't add any tests - leave it empty
            d_test_handler_run_block(empty_handler, empty_block, NULL);

            struct d_test_result results = d_test_handler_get_results(empty_handler);
            
            if (!d_assert_and_count_standalone(results.blocks_run == 1,
                "Empty block still counts in blocks_run",
                "Empty block should still count in blocks_run", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(empty_block);
        }

        d_test_handler_free(empty_handler);
    }
    
    d_test_config_free(empty_config);

    // Test 7: blocks_run independent of pass/fail
    struct d_test_config* passfail_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* passfail_handler = d_test_handler_new(passfail_config);
    
    if (passfail_handler)
    {
        // Run block with passing test
        struct d_test_block* pass_block = d_test_block_new(5, NULL);
        if (pass_block)
        {
            struct d_test* pass_test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* pass_type = d_test_type_new(D_TEST_TYPE_TEST, pass_test);
            pass_block->tests[0] = *pass_type;
            pass_block->count = 1;
            free(pass_type);

            d_test_handler_run_block(passfail_handler, pass_block, NULL);
            d_test_block_free(pass_block);
        }

        // Run block with failing test
        struct d_test_block* fail_block = d_test_block_new(5, NULL);
        if (fail_block)
        {
            struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
            struct d_test_type* fail_type = d_test_type_new(D_TEST_TYPE_TEST, fail_test);
            fail_block->tests[0] = *fail_type;
            fail_block->count = 1;
            free(fail_type);

            d_test_handler_run_block(passfail_handler, fail_block, NULL);
            d_test_block_free(fail_block);
        }

        struct d_test_result results = d_test_handler_get_results(passfail_handler);
        
        if (!d_assert_and_count_standalone(results.blocks_run == 2,
            "blocks_run counts both passing and failing blocks",
            "blocks_run should count both passing and failing blocks", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(passfail_handler);
    }
    
    d_test_config_free(passfail_config);

    // Test 8: blocks_run resets
    struct d_test_config* reset_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* reset_handler = d_test_handler_new(reset_config);
    
    if (reset_handler)
    {
        struct d_test_block* block = d_test_block_new(5, NULL);
        if (block)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            d_test_handler_run_block(reset_handler, block, NULL);
            d_test_block_free(block);
        }

        d_test_handler_reset_results(reset_handler);

        struct d_test_result results = d_test_handler_get_results(reset_handler);
        
        if (!d_assert_and_count_standalone(results.blocks_run == 0,
            "blocks_run resets to 0 after reset",
            "blocks_run should reset to 0 after reset", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(reset_handler);
    }
    
    d_test_config_free(reset_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_block_counting unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_block_counting unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

//=============================================================================
// ABORT ON FAILURE TESTS
//=============================================================================

/*
d_tests_sa_handler_abort_on_failure
  Tests abort_on_failure behavior.
  
  Tests:
  - abort_on_failure starts as false
  - Setting abort_on_failure to true
  - Test execution stops after first failure when enabled
  - Block execution continues when disabled (default)
  - abort_on_failure affects nested blocks
  - Statistics reflect early abort

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_abort_on_failure
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler abort_on_failure ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Fresh handler has abort_on_failure false
    struct d_test_config* fresh_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* fresh_handler = d_test_handler_new(fresh_config);
    
    if (fresh_handler)
    {
        if (!d_assert_and_count_standalone(fresh_handler->abort_on_failure == false,
            "Fresh handler has abort_on_failure set to false",
            "Fresh handler should have abort_on_failure set to false", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(fresh_handler);
    }
    
    d_test_config_free(fresh_config);

    // Test 2: Setting abort_on_failure to true
    struct d_test_config* set_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* set_handler = d_test_handler_new(set_config);
    
    if (set_handler)
    {
        set_handler->abort_on_failure = true;
        
        if (!d_assert_and_count_standalone(set_handler->abort_on_failure == true,
            "abort_on_failure can be set to true",
            "abort_on_failure should be able to be set to true", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(set_handler);
    }
    
    d_test_config_free(set_config);

    // Test 3: With abort disabled, all tests run despite failures
    struct d_test_config* disabled_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* disabled_handler = d_test_handler_new(disabled_config);
    
    if (disabled_handler)
    {
        disabled_handler->abort_on_failure = false;
        
        struct d_test_block* block = d_test_block_new(10, NULL);
        
        if (block)
        {
            // Add 2 passing tests
            for (int i = 0; i < 2; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[i] = *type;
                free(type);
            }
            
            // Add 1 failing test in the middle
            struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
            struct d_test_type* fail_type = d_test_type_new(D_TEST_TYPE_TEST, fail_test);
            block->tests[2] = *fail_type;
            free(fail_type);
            
            // Add 2 more passing tests after the failure
            for (int i = 3; i < 5; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[i] = *type;
                free(type);
            }
            
            block->count = 5;

            d_test_handler_run_block(disabled_handler, block, NULL);

            struct d_test_result results = d_test_handler_get_results(disabled_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 5,
                "With abort disabled, all 5 tests run despite failure",
                "With abort disabled, all 5 tests should run despite failure", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_passed == 4,
                "4 tests passed with abort disabled",
                "4 tests should pass with abort disabled", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_failed == 1,
                "1 test failed with abort disabled",
                "1 test should fail with abort disabled", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(disabled_handler);
    }
    
    d_test_config_free(disabled_config);

    // Test 4: With abort enabled, execution stops after first failure
    struct d_test_config* enabled_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* enabled_handler = d_test_handler_new(enabled_config);
    
    if (enabled_handler)
    {
        enabled_handler->abort_on_failure = true;
        
        struct d_test_block* block = d_test_block_new(10, NULL);
        
        if (block)
        {
            // Add 2 passing tests
            for (int i = 0; i < 2; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[i] = *type;
                free(type);
            }
            
            // Add 1 failing test
            struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
            struct d_test_type* fail_type = d_test_type_new(D_TEST_TYPE_TEST, fail_test);
            block->tests[2] = *fail_type;
            free(fail_type);
            
            // Add 2 more tests that should NOT run due to abort
            for (int i = 3; i < 5; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[i] = *type;
                free(type);
            }
            
            block->count = 5;

            d_test_handler_run_block(enabled_handler, block, NULL);

            struct d_test_result results = d_test_handler_get_results(enabled_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 3,
                "With abort enabled, only 3 tests run before abort",
                "With abort enabled, only 3 tests should run before abort", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_passed == 2,
                "2 tests passed before abort",
                "2 tests should pass before abort", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_failed == 1,
                "1 test failed triggering abort",
                "1 test should fail triggering abort", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(enabled_handler);
    }
    
    d_test_config_free(enabled_config);

    // Test 5: abort_on_failure with no failures runs all tests
    struct d_test_config* noabort_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* noabort_handler = d_test_handler_new(noabort_config);
    
    if (noabort_handler)
    {
        noabort_handler->abort_on_failure = true;
        
        struct d_test_block* block = d_test_block_new(10, NULL);
        
        if (block)
        {
            // Add 5 passing tests - no failures to trigger abort
            for (int i = 0; i < 5; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[i] = *type;
                free(type);
            }
            
            block->count = 5;

            d_test_handler_run_block(noabort_handler, block, NULL);

            struct d_test_result results = d_test_handler_get_results(noabort_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 5,
                "With abort enabled but no failures, all tests run",
                "With abort enabled but no failures, all tests should run", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.tests_passed == 5,
                "All 5 tests passed",
                "All 5 tests should pass", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(noabort_handler);
    }
    
    d_test_config_free(noabort_config);

    // Test 6: abort_on_failure with nested blocks
    struct d_test_config* nested_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* nested_handler = d_test_handler_new(nested_config);
    
    if (nested_handler)
    {
        nested_handler->abort_on_failure = true;
        
        struct d_test_block* outer = d_test_block_new(10, NULL);
        
        if (outer)
        {
            // Add passing test to outer block
            struct d_test* test1 = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type1 = d_test_type_new(D_TEST_TYPE_TEST, test1);
            outer->tests[0] = *type1;
            free(type1);
            
            // Add inner block with failing test
            struct d_test_block* inner = d_test_block_new(5, NULL);
            if (inner)
            {
                struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
                struct d_test_type* fail_type = d_test_type_new(D_TEST_TYPE_TEST, fail_test);
                inner->tests[0] = *fail_type;
                inner->count = 1;
                free(fail_type);

                struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
                outer->tests[1] = *inner_type;
                free(inner_type);
            }
            
            // Add another test that should NOT run due to abort
            struct d_test* test2 = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type2 = d_test_type_new(D_TEST_TYPE_TEST, test2);
            outer->tests[2] = *type2;
            free(type2);
            
            outer->count = 3;

            d_test_handler_run_block(nested_handler, outer, NULL);

            struct d_test_result results = d_test_handler_get_results(nested_handler);
            
            // Should run: outer test 1, inner failing test, then abort
            if (!d_assert_and_count_standalone(results.tests_run == 2,
                "Nested abort stops after inner block failure",
                "Nested abort should stop after inner block failure", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(nested_handler);
    }
    
    d_test_config_free(nested_config);

    // Test 7: Toggling abort_on_failure mid-execution (via multiple blocks)
    struct d_test_config* toggle_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* toggle_handler = d_test_handler_new(toggle_config);
    
    if (toggle_handler)
    {
        // First block with abort disabled
        toggle_handler->abort_on_failure = false;
        
        struct d_test_block* block1 = d_test_block_new(5, NULL);
        if (block1)
        {
            struct d_test* pass = d_test_new(handler_test_passing, NULL);
            struct d_test_type* pass_type = d_test_type_new(D_TEST_TYPE_TEST, pass);
            block1->tests[0] = *pass_type;
            free(pass_type);
            
            struct d_test* fail = d_test_new(handler_test_failing, NULL);
            struct d_test_type* fail_type = d_test_type_new(D_TEST_TYPE_TEST, fail);
            block1->tests[1] = *fail_type;
            free(fail_type);
            
            struct d_test* pass2 = d_test_new(handler_test_passing, NULL);
            struct d_test_type* pass2_type = d_test_type_new(D_TEST_TYPE_TEST, pass2);
            block1->tests[2] = *pass2_type;
            free(pass2_type);
            
            block1->count = 3;

            d_test_handler_run_block(toggle_handler, block1, NULL);
            d_test_block_free(block1);
        }

        struct d_test_result after_first = d_test_handler_get_results(toggle_handler);
        bool first_all_ran = (after_first.tests_run == 3);
        
        // Second block with abort enabled
        toggle_handler->abort_on_failure = true;
        
        struct d_test_block* block2 = d_test_block_new(5, NULL);
        if (block2)
        {
            struct d_test* pass = d_test_new(handler_test_passing, NULL);
            struct d_test_type* pass_type = d_test_type_new(D_TEST_TYPE_TEST, pass);
            block2->tests[0] = *pass_type;
            free(pass_type);
            
            struct d_test* fail = d_test_new(handler_test_failing, NULL);
            struct d_test_type* fail_type = d_test_type_new(D_TEST_TYPE_TEST, fail);
            block2->tests[1] = *fail_type;
            free(fail_type);
            
            struct d_test* pass2 = d_test_new(handler_test_passing, NULL);
            struct d_test_type* pass2_type = d_test_type_new(D_TEST_TYPE_TEST, pass2);
            block2->tests[2] = *pass2_type;
            free(pass2_type);
            
            block2->count = 3;

            d_test_handler_run_block(toggle_handler, block2, NULL);
            d_test_block_free(block2);
        }

        struct d_test_result after_second = d_test_handler_get_results(toggle_handler);
        bool second_aborted = (after_second.tests_run == 5); // 3 from first + 2 from second (aborted)
        
        if (!d_assert_and_count_standalone(first_all_ran && second_aborted,
            "Toggling abort_on_failure affects subsequent executions",
            "Toggling abort_on_failure should affect subsequent executions", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(toggle_handler);
    }
    
    d_test_config_free(toggle_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_abort_on_failure unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_abort_on_failure unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

//=============================================================================
// EDGE CASES AND ERROR HANDLING TESTS
//=============================================================================

/*
d_tests_sa_handler_null_parameters
  Tests NULL parameter handling across all functions.
  
  Tests:
  - All major functions handle NULL handler gracefully
  - All major functions handle NULL parameters gracefully
  - No crashes or undefined behavior with NULL inputs
  - Functions return appropriate values for NULL inputs

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_null_parameters
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler NULL parameter handling ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: d_test_handler_new with NULL config
    struct d_test_handler* null_config_handler = d_test_handler_new(NULL);
    
    if (!d_assert_and_count_standalone(null_config_handler != NULL,
        "d_test_handler_new handles NULL config",
        "d_test_handler_new should handle NULL config", _test_info))
    {
        all_assertions_passed = false;
    }
    
    if (null_config_handler)
    {
        d_test_handler_free(null_config_handler);
    }

    // Test 2: d_test_handler_free with NULL
    d_test_handler_free(NULL);
    
    if (!d_assert_and_count_standalone(true,
        "d_test_handler_free handles NULL handler",
        "d_test_handler_free should handle NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 3: d_test_handler_run_test with NULL handler
    struct d_test* test = d_test_new(handler_test_passing, NULL);
    bool null_handler_result = d_test_handler_run_test(NULL, test, NULL);
    
    if (!d_assert_and_count_standalone(null_handler_result == false,
        "d_test_handler_run_test returns false for NULL handler",
        "d_test_handler_run_test should return false for NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }
    
    d_test_free(test);

    // Test 4: d_test_handler_run_test with NULL test
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new(config);
    
    if (handler)
    {
        bool null_test_result = d_test_handler_run_test(handler, NULL, NULL);
        
        if (!d_assert_and_count_standalone(null_test_result == false,
            "d_test_handler_run_test returns false for NULL test",
            "d_test_handler_run_test should return false for NULL test", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 5: d_test_handler_run_block with NULL handler
    struct d_test_block* block = d_test_block_new(5, NULL);
    bool null_handler_block = d_test_handler_run_block(NULL, block, NULL);
    
    if (!d_assert_and_count_standalone(null_handler_block == false,
        "d_test_handler_run_block returns false for NULL handler",
        "d_test_handler_run_block should return false for NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }
    
    d_test_block_free(block);

    // Test 6: d_test_handler_run_block with NULL block
    struct d_test_config* block_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* block_handler = d_test_handler_new(block_config);
    
    if (block_handler)
    {
        bool null_block_result = d_test_handler_run_block(block_handler, NULL, NULL);
        
        if (!d_assert_and_count_standalone(null_block_result == false,
            "d_test_handler_run_block returns false for NULL block",
            "d_test_handler_run_block should return false for NULL block", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(block_handler);
    }
    
    d_test_config_free(block_config);

    // Test 7: d_test_handler_run_test_type with NULL handler
    struct d_test* type_test = d_test_new(handler_test_passing, NULL);
    struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, type_test);
    
    if (test_type)
    {
        bool null_handler_type = d_test_handler_run_test_type(NULL, test_type, NULL);
        
        if (!d_assert_and_count_standalone(null_handler_type == false,
            "d_test_handler_run_test_type returns false for NULL handler",
            "d_test_handler_run_test_type should return false for NULL handler", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_type_free(test_type);
    }

    // Test 8: d_test_handler_run_test_type with NULL test_type
    struct d_test_config* type_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* type_handler = d_test_handler_new(type_config);
    
    if (type_handler)
    {
        bool null_type_result = d_test_handler_run_test_type(type_handler, NULL, NULL);
        
        if (!d_assert_and_count_standalone(null_type_result == false,
            "d_test_handler_run_test_type returns false for NULL test_type",
            "d_test_handler_run_test_type should return false for NULL test_type", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(type_handler);
    }
    
    d_test_config_free(type_config);

    // Test 9: d_test_handler_record_assertion with NULL handler
    struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
    d_test_handler_record_assertion(NULL, assertion);
    
    if (!d_assert_and_count_standalone(true,
        "d_test_handler_record_assertion handles NULL handler",
        "d_test_handler_record_assertion should handle NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }
    
    d_assertion_free(assertion);

    // Test 10: d_test_handler_record_assertion with NULL assertion
    struct d_test_config* assert_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* assert_handler = d_test_handler_new(assert_config);
    
    if (assert_handler)
    {
        d_test_handler_record_assertion(assert_handler, NULL);
        
        if (!d_assert_and_count_standalone(true,
            "d_test_handler_record_assertion handles NULL assertion",
            "d_test_handler_record_assertion should handle NULL assertion", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(assert_handler);
    }
    
    d_test_config_free(assert_config);

    // Test 11: d_test_handler_get_results with NULL handler
    struct d_test_result null_results = d_test_handler_get_results(NULL);
    
    if (!d_assert_and_count_standalone(
        null_results.tests_run == 0 &&
        null_results.tests_passed == 0 &&
        null_results.tests_failed == 0,
        "d_test_handler_get_results returns empty results for NULL handler",
        "d_test_handler_get_results should return empty results for NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 12: d_test_handler_reset_results with NULL handler
    d_test_handler_reset_results(NULL);
    
    if (!d_assert_and_count_standalone(true,
        "d_test_handler_reset_results handles NULL handler",
        "d_test_handler_reset_results should handle NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 13: d_test_handler_print_results with NULL handler
    d_test_handler_print_results(NULL, "Test Label");
    
    if (!d_assert_and_count_standalone(true,
        "d_test_handler_print_results handles NULL handler",
        "d_test_handler_print_results should handle NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 14: d_test_handler_get_pass_rate with NULL handler
    double null_pass_rate = d_test_handler_get_pass_rate(NULL);
    
    if (!d_assert_and_count_standalone(null_pass_rate == 0.0,
        "d_test_handler_get_pass_rate returns 0.0 for NULL handler",
        "d_test_handler_get_pass_rate should return 0.0 for NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 15: d_test_handler_get_assertion_rate with NULL handler
    double null_assert_rate = d_test_handler_get_assertion_rate(NULL);
    
    if (!d_assert_and_count_standalone(null_assert_rate == 0.0,
        "d_test_handler_get_assertion_rate returns 0.0 for NULL handler",
        "d_test_handler_get_assertion_rate should return 0.0 for NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 16: d_test_handler_register_listener with NULL handler
    bool null_register = d_test_handler_register_listener(NULL, D_TEST_EVENT_START, callback_start, true);
    
    if (!d_assert_and_count_standalone(null_register == false,
        "d_test_handler_register_listener returns false for NULL handler",
        "d_test_handler_register_listener should return false for NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 17: d_test_handler_unregister_listener with NULL handler
    bool null_unregister = d_test_handler_unregister_listener(NULL, D_TEST_EVENT_START);
    
    if (!d_assert_and_count_standalone(null_unregister == false,
        "d_test_handler_unregister_listener returns false for NULL handler",
        "d_test_handler_unregister_listener should return false for NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 18: d_test_handler_emit_event with NULL handler
    struct d_test_context context = {
        .handler = NULL,
        .current_test = NULL,
        .current_block = NULL,
        .event_type = D_TEST_EVENT_START,
        .depth = 0,
        .last_assertion = NULL
    };
    
    d_test_handler_emit_event(NULL, D_TEST_EVENT_START, &context);
    
    if (!d_assert_and_count_standalone(true,
        "d_test_handler_emit_event handles NULL handler",
        "d_test_handler_emit_event should handle NULL handler", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_null_parameters unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_null_parameters unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_empty_blocks
  Tests empty block execution.
  
  Tests:
  - Running empty block succeeds
  - Empty block increments blocks_run
  - Empty block doesn't affect test counts
  - Nested empty blocks
  - Empty blocks with events
  - Empty blocks don't crash or error

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_empty_blocks
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler empty blocks ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Running empty block succeeds
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new(config);
    
    if (handler)
    {
        struct d_test_block* empty_block = d_test_block_new(10, NULL);
        
        if (empty_block)
        {
            // Don't add any tests - leave count at 0
            bool result = d_test_handler_run_block(handler, empty_block, NULL);
            
            if (!d_assert_and_count_standalone(result == true,
                "Running empty block succeeds",
                "Running empty block should succeed", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(empty_block);
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 2: Empty block increments blocks_run
    struct d_test_config* count_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* count_handler = d_test_handler_new(count_config);
    
    if (count_handler)
    {
        struct d_test_block* empty = d_test_block_new(5, NULL);
        
        if (empty)
        {
            d_test_handler_run_block(count_handler, empty, NULL);

            struct d_test_result results = d_test_handler_get_results(count_handler);
            
            if (!d_assert_and_count_standalone(results.blocks_run == 1,
                "Empty block increments blocks_run",
                "Empty block should increment blocks_run", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(empty);
        }

        d_test_handler_free(count_handler);
    }
    
    d_test_config_free(count_config);

    // Test 3: Empty block doesn't affect test counts
    struct d_test_config* test_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* test_handler = d_test_handler_new(test_config);
    
    if (test_handler)
    {
        struct d_test_block* empty = d_test_block_new(5, NULL);
        
        if (empty)
        {
            d_test_handler_run_block(test_handler, empty, NULL);

            struct d_test_result results = d_test_handler_get_results(test_handler);
            
            if (!d_assert_and_count_standalone(
                results.tests_run == 0 &&
                results.tests_passed == 0 &&
                results.tests_failed == 0,
                "Empty block doesn't affect test counts",
                "Empty block should not affect test counts", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(empty);
        }

        d_test_handler_free(test_handler);
    }
    
    d_test_config_free(test_config);

    // Test 4: Multiple sequential empty blocks
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new(multi_config);
    
    if (multi_handler)
    {
        // Run 3 empty blocks
        for (int i = 0; i < 3; i++)
        {
            struct d_test_block* empty = d_test_block_new(5, NULL);
            if (empty)
            {
                d_test_handler_run_block(multi_handler, empty, NULL);
                d_test_block_free(empty);
            }
        }

        struct d_test_result results = d_test_handler_get_results(multi_handler);
        
        if (!d_assert_and_count_standalone(results.blocks_run == 3,
            "Multiple empty blocks all counted",
            "Multiple empty blocks should all be counted", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.tests_run == 0,
            "No tests run across empty blocks",
            "No tests should run across empty blocks", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Test 5: Nested empty blocks
    struct d_test_config* nested_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* nested_handler = d_test_handler_new(nested_config);
    
    if (nested_handler)
    {
        struct d_test_block* outer = d_test_block_new(5, NULL);
        struct d_test_block* inner = d_test_block_new(3, NULL);
        
        if (outer && inner)
        {
            // Inner block is empty, add it to outer
            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            d_test_handler_run_block(nested_handler, outer, NULL);

            struct d_test_result results = d_test_handler_get_results(nested_handler);
            
            if (!d_assert_and_count_standalone(results.blocks_run == 2,
                "Nested empty blocks both counted",
                "Nested empty blocks should both be counted", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.max_depth == 2,
                "Nested empty blocks track depth",
                "Nested empty blocks should track depth", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(nested_handler);
    }
    
    d_test_config_free(nested_config);

    // Test 6: Empty blocks with event system
    reset_event_counters();
    struct d_test_config* event_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* event_handler = d_test_handler_new_with_events(event_config, 10);
    
    if (event_handler)
    {
        d_test_handler_register_listener(event_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        d_test_handler_register_listener(event_handler, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);
        
        struct d_test_block* empty = d_test_block_new(5, NULL);
        
        if (empty)
        {
            d_test_handler_run_block(event_handler, empty, NULL);
            
            // Events should still fire for empty block
            if (!d_assert_and_count_standalone(
                g_event_setup_count > 0 || g_event_teardown_count > 0,
                "Empty block fires events",
                "Empty block should fire events", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(empty);
        }

        d_test_handler_free(event_handler);
    }
    
    d_test_config_free(event_config);

    // Test 7: Empty block with non-NULL config
    struct d_test_config* block_cfg = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_config* handler_cfg = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* cfg_handler = d_test_handler_new(handler_cfg);
    
    if (cfg_handler)
    {
        struct d_test_block* configured_empty = d_test_block_new(5, block_cfg);
        
        if (configured_empty)
        {
            bool result = d_test_handler_run_block(cfg_handler, configured_empty, NULL);
            
            if (!d_assert_and_count_standalone(result == true,
                "Empty block with config succeeds",
                "Empty block with config should succeed", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(configured_empty);
        }

        d_test_handler_free(cfg_handler);
    }
    
    d_test_config_free(handler_cfg);
    d_test_config_free(block_cfg);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_empty_blocks unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_empty_blocks unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_no_events
  Tests handler with no event system.
  
  Tests:
  - Handler created without events works correctly
  - Test execution works without events
  - Block execution works without events
  - Registering listeners fails without event system
  - Emitting events doesn't crash without event system
  - All statistics tracked correctly without events

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_no_events
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler with no events ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Handler created without events has NULL event_handler
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new(config);
    
    if (handler)
    {
        if (!d_assert_and_count_standalone(handler->event_handler == NULL,
            "Handler created without events has NULL event_handler",
            "Handler created without events should have NULL event_handler", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 2: Test execution works without events
    struct d_test_config* test_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* test_handler = d_test_handler_new(test_config);
    
    if (test_handler)
    {
        struct d_test* test = d_test_new(handler_test_passing, NULL);
        bool result = d_test_handler_run_test(test_handler, test, NULL);
        
        if (!d_assert_and_count_standalone(result == true,
            "Test execution works without events",
            "Test execution should work without events", _test_info))
        {
            all_assertions_passed = false;
        }

        struct d_test_result results = d_test_handler_get_results(test_handler);
        
        if (!d_assert_and_count_standalone(results.tests_run == 1,
            "Test statistics tracked without events",
            "Test statistics should be tracked without events", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_free(test);
        d_test_handler_free(test_handler);
    }
    
    d_test_config_free(test_config);

    // Test 3: Block execution works without events
    struct d_test_config* block_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* block_handler = d_test_handler_new(block_config);
    
    if (block_handler)
    {
        struct d_test_block* block = d_test_block_new(5, NULL);
        
        if (block)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
            block->tests[0] = *type;
            block->count = 1;
            free(type);

            bool result = d_test_handler_run_block(block_handler, block, NULL);
            
            if (!d_assert_and_count_standalone(result == true,
                "Block execution works without events",
                "Block execution should work without events", _test_info))
            {
                all_assertions_passed = false;
            }

            struct d_test_result results = d_test_handler_get_results(block_handler);
            
            if (!d_assert_and_count_standalone(
                results.tests_run == 1 && results.blocks_run == 1,
                "Block statistics tracked without events",
                "Block statistics should be tracked without events", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(block_handler);
    }
    
    d_test_config_free(block_config);

    // Test 4: Registering listener fails without event system
    struct d_test_config* register_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* register_handler = d_test_handler_new(register_config);
    
    if (register_handler)
    {
        bool register_result = d_test_handler_register_listener(
            register_handler, D_TEST_EVENT_START, callback_start, true);
        
        if (!d_assert_and_count_standalone(register_result == false,
            "Registering listener fails without event system",
            "Registering listener should fail without event system", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(register_handler);
    }
    
    d_test_config_free(register_config);

    // Test 5: Unregistering listener fails without event system
    struct d_test_config* unreg_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* unreg_handler = d_test_handler_new(unreg_config);
    
    if (unreg_handler)
    {
        bool unregister_result = d_test_handler_unregister_listener(
            unreg_handler, D_TEST_EVENT_START);
        
        if (!d_assert_and_count_standalone(unregister_result == false,
            "Unregistering listener fails without event system",
            "Unregistering listener should fail without event system", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(unreg_handler);
    }
    
    d_test_config_free(unreg_config);

    // Test 6: Emitting events doesn't crash without event system
    struct d_test_config* emit_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* emit_handler = d_test_handler_new(emit_config);
    
    if (emit_handler)
    {
        struct d_test_context context = {
            .handler = emit_handler,
            .current_test = NULL,
            .current_block = NULL,
            .event_type = D_TEST_EVENT_START,
            .depth = 0,
            .last_assertion = NULL
        };
        
        d_test_handler_emit_event(emit_handler, D_TEST_EVENT_START, &context);
        
        if (!d_assert_and_count_standalone(true,
            "Emitting event without event system does not crash",
            "Emitting event without event system should not crash", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(emit_handler);
    }
    
    d_test_config_free(emit_config);

    // Test 7: Multiple test/block executions work without events
    struct d_test_config* multi_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* multi_handler = d_test_handler_new(multi_config);
    
    if (multi_handler)
    {
        // Run some tests
        for (int i = 0; i < 3; i++)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(multi_handler, test, NULL);
            d_test_free(test);
        }

        // Run some blocks
        for (int i = 0; i < 2; i++)
        {
            struct d_test_block* block = d_test_block_new(5, NULL);
            if (block)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[0] = *type;
                block->count = 1;
                free(type);

                d_test_handler_run_block(multi_handler, block, NULL);
                d_test_block_free(block);
            }
        }

        struct d_test_result results = d_test_handler_get_results(multi_handler);
        
        if (!d_assert_and_count_standalone(
            results.tests_run == 5 && results.blocks_run == 2,
            "Multiple executions work correctly without events",
            "Multiple executions should work correctly without events", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(multi_handler);
    }
    
    d_test_config_free(multi_config);

    // Test 8: Assertion recording works without events
    struct d_test_config* assert_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* assert_handler = d_test_handler_new(assert_config);
    
    if (assert_handler)
    {
        struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
        d_test_handler_record_assertion(assert_handler, assertion);
        d_assertion_free(assertion);

        struct d_test_result results = d_test_handler_get_results(assert_handler);
        
        if (!d_assert_and_count_standalone(results.assertions_run == 1,
            "Assertion recording works without events",
            "Assertion recording should work without events", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(assert_handler);
    }
    
    d_test_config_free(assert_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_no_events unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_no_events unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

//=============================================================================
// INTEGRATION TESTS
//=============================================================================

/*
d_tests_sa_handler_complex_execution
  Tests complex execution scenarios with multiple blocks.
  
  Tests:
  - Multiple levels of nesting with mixed pass/fail
  - Large number of tests and blocks
  - Complex block structures
  - Configuration overrides at different levels
  - Depth tracking in complex scenarios
  - Statistics accuracy with complex execution

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_complex_execution
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler complex execution ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Multi-level nesting with mixed results
    struct d_test_config* complex_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* complex_handler = d_test_handler_new(complex_config);
    
    if (complex_handler)
    {
        struct d_test_block* level1 = d_test_block_new(10, NULL);
        struct d_test_block* level2a = d_test_block_new(5, NULL);
        struct d_test_block* level2b = d_test_block_new(5, NULL);
        struct d_test_block* level3 = d_test_block_new(3, NULL);
        
        if (level1 && level2a && level2b && level3)
        {
            // Level 3: innermost - 2 passing tests
            for (int i = 0; i < 2; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                level3->tests[i] = *type;
                free(type);
            }
            level3->count = 2;

            // Level 2a: contains level3 and a failing test
            struct d_test_type* level3_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, level3);
            level2a->tests[0] = *level3_type;
            free(level3_type);

            struct d_test* fail_test = d_test_new(handler_test_failing, NULL);
            struct d_test_type* fail_type = d_test_type_new(D_TEST_TYPE_TEST, fail_test);
            level2a->tests[1] = *fail_type;
            free(fail_type);
            
            level2a->count = 2;

            // Level 2b: just passing tests
            for (int i = 0; i < 2; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                level2b->tests[i] = *type;
                free(type);
            }
            level2b->count = 2;

            // Level 1: contains both level2 blocks and direct tests
            struct d_test* direct_test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* direct_type = d_test_type_new(D_TEST_TYPE_TEST, direct_test);
            level1->tests[0] = *direct_type;
            free(direct_type);

            struct d_test_type* level2a_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, level2a);
            level1->tests[1] = *level2a_type;
            free(level2a_type);

            struct d_test_type* level2b_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, level2b);
            level1->tests[2] = *level2b_type;
            free(level2b_type);

            level1->count = 3;

            d_test_handler_run_block(complex_handler, level1, NULL);

            struct d_test_result results = d_test_handler_get_results(complex_handler);
            
            // Should have: 1 direct + 2 from level3 + 1 fail from level2a + 2 from level2b = 6 tests
            if (!d_assert_and_count_standalone(results.tests_run == 6,
                "Complex execution runs correct number of tests (6)",
                "Complex execution should run correct number of tests (6)", _test_info))
            {
                all_assertions_passed = false;
            }

            // Should have: 5 passing (all except the one failing test)
            if (!d_assert_and_count_standalone(results.tests_passed == 5,
                "Complex execution tracks passing tests correctly (5)",
                "Complex execution should track passing tests correctly (5)", _test_info))
            {
                all_assertions_passed = false;
            }

            // Should have: 1 failing
            if (!d_assert_and_count_standalone(results.tests_failed == 1,
                "Complex execution tracks failing tests correctly (1)",
                "Complex execution should track failing tests correctly (1)", _test_info))
            {
                all_assertions_passed = false;
            }

            // Should have: 4 blocks (level1, level2a, level2b, level3)
            if (!d_assert_and_count_standalone(results.blocks_run == 4,
                "Complex execution tracks all blocks (4)",
                "Complex execution should track all blocks (4)", _test_info))
            {
                all_assertions_passed = false;
            }

            // Max depth should be 3
            if (!d_assert_and_count_standalone(results.max_depth == 3,
                "Complex execution tracks max depth correctly (3)",
                "Complex execution should track max depth correctly (3)", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(level1);
        }

        d_test_handler_free(complex_handler);
    }
    
    d_test_config_free(complex_config);

    // Test 2: Large number of tests and blocks
    struct d_test_config* large_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* large_handler = d_test_handler_new(large_config);
    
    if (large_handler)
    {
        // Create 10 blocks with 10 tests each
        for (int b = 0; b < 10; b++)
        {
            struct d_test_block* block = d_test_block_new(15, NULL);
            if (block)
            {
                for (int t = 0; t < 10; t++)
                {
                    // Make every 5th test fail
                    bool should_pass = ((b * 10 + t) % 5 != 0);
                    struct d_test* test = d_test_new(
                        should_pass ? handler_test_passing : handler_test_failing, NULL);
                    struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                    block->tests[t] = *type;
                    free(type);
                }
                block->count = 10;

                d_test_handler_run_block(large_handler, block, NULL);
                d_test_block_free(block);
            }
        }

        struct d_test_result results = d_test_handler_get_results(large_handler);
        
        if (!d_assert_and_count_standalone(results.tests_run == 100,
            "Large execution runs 100 tests",
            "Large execution should run 100 tests", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(results.blocks_run == 10,
            "Large execution runs 10 blocks",
            "Large execution should run 10 blocks", _test_info))
        {
            all_assertions_passed = false;
        }

        // 20 should fail (every 5th test)
        if (!d_assert_and_count_standalone(results.tests_failed == 20,
            "Large execution tracks failures correctly (20)",
            "Large execution should track failures correctly (20)", _test_info))
        {
            all_assertions_passed = false;
        }

        // 80 should pass
        if (!d_assert_and_count_standalone(results.tests_passed == 80,
            "Large execution tracks passes correctly (80)",
            "Large execution should track passes correctly (80)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(large_handler);
    }
    
    d_test_config_free(large_config);

    // Test 3: Configuration overrides at different levels
    struct d_test_config* default_cfg = d_test_config_new(D_TEST_MODE_SILENT);
    struct d_test_config* block_cfg = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_config* runtime_cfg = d_test_config_new(D_TEST_MODE_VERBOSE);
    struct d_test_handler* override_handler = d_test_handler_new(default_cfg);
    
    if (override_handler)
    {
        struct d_test_block* outer = d_test_block_new_with_override(10, block_cfg, true);
        struct d_test_block* inner = d_test_block_new(5, NULL);
        
        if (outer && inner)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
            inner->tests[0] = *test_type;
            inner->count = 1;
            free(test_type);

            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            outer->count = 1;
            free(inner_type);

            // Runtime config should override block config
            bool result = d_test_handler_run_block(override_handler, outer, runtime_cfg);
            
            if (!d_assert_and_count_standalone(result == true,
                "Complex config override execution succeeds",
                "Complex config override execution should succeed", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(override_handler);
    }
    
    d_test_config_free(default_cfg);
    d_test_config_free(block_cfg);
    d_test_config_free(runtime_cfg);

    // Test 4: Mixed empty and populated blocks
    struct d_test_config* mixed_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* mixed_handler = d_test_handler_new(mixed_config);
    
    if (mixed_handler)
    {
        struct d_test_block* outer = d_test_block_new(10, NULL);
        
        if (outer)
        {
            // Add empty block
            struct d_test_block* empty = d_test_block_new(5, NULL);
            struct d_test_type* empty_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, empty);
            outer->tests[0] = *empty_type;
            free(empty_type);

            // Add populated block
            struct d_test_block* populated = d_test_block_new(5, NULL);
            if (populated)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* test_type = d_test_type_new(D_TEST_TYPE_TEST, test);
                populated->tests[0] = *test_type;
                populated->count = 1;
                free(test_type);

                struct d_test_type* pop_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, populated);
                outer->tests[1] = *pop_type;
                free(pop_type);
            }

            // Add direct test
            struct d_test* direct = d_test_new(handler_test_passing, NULL);
            struct d_test_type* direct_type = d_test_type_new(D_TEST_TYPE_TEST, direct);
            outer->tests[2] = *direct_type;
            free(direct_type);

            outer->count = 3;

            d_test_handler_run_block(mixed_handler, outer, NULL);

            struct d_test_result results = d_test_handler_get_results(mixed_handler);
            
            if (!d_assert_and_count_standalone(results.tests_run == 2,
                "Mixed blocks run correct number of tests (2)",
                "Mixed blocks should run correct number of tests (2)", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(results.blocks_run == 3,
                "Mixed blocks count all blocks including empty (3)",
                "Mixed blocks should count all blocks including empty (3)", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(mixed_handler);
    }
    
    d_test_config_free(mixed_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_complex_execution unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_complex_execution unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_event_integration
  Tests event system integration during complex execution.
  
  Tests:
  - Events fire correctly in nested structures
  - Event order is correct during complex execution
  - Multiple listeners for same event
  - Events with context data
  - Event statistics match execution statistics

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_event_integration
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler event integration ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Events fire correctly in nested structure
    reset_event_counters();
    struct d_test_config* nested_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* nested_handler = d_test_handler_new_with_events(nested_config, 30);
    
    if (nested_handler)
    {
        // Register listeners for all events
        d_test_handler_register_listener(nested_handler, D_TEST_EVENT_SETUP, callback_setup, true);
        d_test_handler_register_listener(nested_handler, D_TEST_EVENT_START, callback_start, true);
        d_test_handler_register_listener(nested_handler, D_TEST_EVENT_SUCCESS, callback_success, true);
        d_test_handler_register_listener(nested_handler, D_TEST_EVENT_FAILURE, callback_failure, true);
        d_test_handler_register_listener(nested_handler, D_TEST_EVENT_END, callback_end, true);
        d_test_handler_register_listener(nested_handler, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);

        struct d_test_block* outer = d_test_block_new(10, NULL);
        struct d_test_block* inner = d_test_block_new(5, NULL);
        
        if (outer && inner)
        {
            // Inner: 2 tests
            for (int i = 0; i < 2; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                inner->tests[i] = *type;
                free(type);
            }
            inner->count = 2;

            // Outer: inner block + 1 direct test
            struct d_test_type* inner_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, inner);
            outer->tests[0] = *inner_type;
            free(inner_type);

            struct d_test* direct = d_test_new(handler_test_passing, NULL);
            struct d_test_type* direct_type = d_test_type_new(D_TEST_TYPE_TEST, direct);
            outer->tests[1] = *direct_type;
            free(direct_type);

            outer->count = 2;

            d_test_handler_run_block(nested_handler, outer, NULL);

            // Should have setup/start/success/end/teardown for each test (3 total)
            if (!d_assert_and_count_standalone(g_event_start_count >= 3,
                "START events fire for all tests in nested structure",
                "START events should fire for all tests in nested structure", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(g_event_success_count >= 3,
                "SUCCESS events fire for all passing tests",
                "SUCCESS events should fire for all passing tests", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(g_event_failure_count == 0,
                "No FAILURE events with all passing tests",
                "No FAILURE events should fire with all passing tests", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(nested_handler);
    }
    
    d_test_config_free(nested_config);

    // Test 2: Events fire for both passing and failing tests
    reset_event_counters();
    struct d_test_config* mixed_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* mixed_handler = d_test_handler_new_with_events(mixed_config, 20);
    
    if (mixed_handler)
    {
        d_test_handler_register_listener(mixed_handler, D_TEST_EVENT_SUCCESS, callback_success, true);
        d_test_handler_register_listener(mixed_handler, D_TEST_EVENT_FAILURE, callback_failure, true);

        struct d_test_block* block = d_test_block_new(10, NULL);
        
        if (block)
        {
            // 3 passing
            for (int i = 0; i < 3; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[i] = *type;
                free(type);
            }

            // 2 failing
            for (int i = 3; i < 5; i++)
            {
                struct d_test* test = d_test_new(handler_test_failing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[i] = *type;
                free(type);
            }

            block->count = 5;

            d_test_handler_run_block(mixed_handler, block, NULL);

            if (!d_assert_and_count_standalone(g_event_success_count == 3,
                "SUCCESS events match passing test count (3)",
                "SUCCESS events should match passing test count (3)", _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_and_count_standalone(g_event_failure_count == 2,
                "FAILURE events match failing test count (2)",
                "FAILURE events should match failing test count (2)", _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_block_free(block);
        }

        d_test_handler_free(mixed_handler);
    }
    
    d_test_config_free(mixed_config);

    // Test 3: Multiple sequential executions with events
    reset_event_counters();
    struct d_test_config* seq_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* seq_handler = d_test_handler_new_with_events(seq_config, 20);
    
    if (seq_handler)
    {
        d_test_handler_register_listener(seq_handler, D_TEST_EVENT_START, callback_start, true);
        d_test_handler_register_listener(seq_handler, D_TEST_EVENT_END, callback_end, true);

        // Run 5 tests sequentially
        for (int i = 0; i < 5; i++)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(seq_handler, test, NULL);
            d_test_free(test);
        }

        if (!d_assert_and_count_standalone(g_event_start_count == 5,
            "Sequential execution fires START for each test (5)",
            "Sequential execution should fire START for each test (5)", _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_and_count_standalone(g_event_end_count == 5,
            "Sequential execution fires END for each test (5)",
            "Sequential execution should fire END for each test (5)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(seq_handler);
    }
    
    d_test_config_free(seq_config);

    // Test 4: Events work correctly after reset
    reset_event_counters();
    struct d_test_config* reset_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* reset_handler = d_test_handler_new_with_events(reset_config, 20);
    
    if (reset_handler)
    {
        d_test_handler_register_listener(reset_handler, D_TEST_EVENT_START, callback_start, true);

        // Run tests
        struct d_test* test1 = d_test_new(handler_test_passing, NULL);
        d_test_handler_run_test(reset_handler, test1, NULL);
        d_test_free(test1);

        int count_before_reset = g_event_start_count;

        // Reset handler
        d_test_handler_reset_results(reset_handler);

        // Run more tests
        struct d_test* test2 = d_test_new(handler_test_passing, NULL);
        d_test_handler_run_test(reset_handler, test2, NULL);
        d_test_free(test2);

        if (!d_assert_and_count_standalone(g_event_start_count > count_before_reset,
            "Events continue firing after handler reset",
            "Events should continue firing after handler reset", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(reset_handler);
    }
    
    d_test_config_free(reset_config);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_event_integration unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_event_integration unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_statistics_accuracy
  Tests statistics accuracy in complex scenarios.
  
  Tests:
  - All statistics accurate after complex execution
  - Pass rates calculated correctly
  - Assertion rates calculated correctly
  - Statistics remain consistent across resets
  - Statistics independent of event system

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_handler_statistics_accuracy
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_test_handler statistics accuracy ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;

    // Test 1: Comprehensive statistics check
    struct d_test_config* config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* handler = d_test_handler_new(config);
    
    if (handler)
    {
        // Run 10 tests (7 pass, 3 fail)
        for (int i = 0; i < 7; i++)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(handler, test, NULL);
            d_test_free(test);
        }

        for (int i = 0; i < 3; i++)
        {
            struct d_test* test = d_test_new(handler_test_failing, NULL);
            d_test_handler_run_test(handler, test, NULL);
            d_test_free(test);
        }

        // Record 20 assertions (15 pass, 5 fail)
        for (int i = 0; i < 15; i++)
        {
            struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
            d_test_handler_record_assertion(handler, assertion);
            d_assertion_free(assertion);
        }

        for (int i = 0; i < 5; i++)
        {
            struct d_assertion* assertion = d_assert(false, "Pass", "Fail");
            d_test_handler_record_assertion(handler, assertion);
            d_assertion_free(assertion);
        }

        // Run 3 blocks
        for (int b = 0; b < 3; b++)
        {
            struct d_test_block* block = d_test_block_new(5, NULL);
            if (block)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                struct d_test_type* type = d_test_type_new(D_TEST_TYPE_TEST, test);
                block->tests[0] = *type;
                block->count = 1;
                free(type);

                d_test_handler_run_block(handler, block, NULL);
                d_test_block_free(block);
            }
        }

        struct d_test_result results = d_test_handler_get_results(handler);
        
        // Total tests: 10 direct + 3 from blocks = 13
        if (!d_assert_and_count_standalone(results.tests_run == 13,
            "Total tests run accurate (13)",
            "Total tests run should be accurate (13)", _test_info))
        {
            all_assertions_passed = false;
        }

        // Passed: 7 direct + 3 from blocks = 10
        if (!d_assert_and_count_standalone(results.tests_passed == 10,
            "Total tests passed accurate (10)",
            "Total tests passed should be accurate (10)", _test_info))
        {
            all_assertions_passed = false;
        }

        // Failed: 3
        if (!d_assert_and_count_standalone(results.tests_failed == 3,
            "Total tests failed accurate (3)",
            "Total tests failed should be accurate (3)", _test_info))
        {
            all_assertions_passed = false;
        }

        // Assertions: 20
        if (!d_assert_and_count_standalone(results.assertions_run == 20,
            "Total assertions run accurate (20)",
            "Total assertions run should be accurate (20)", _test_info))
        {
            all_assertions_passed = false;
        }

        // Assertions passed: 15
        if (!d_assert_and_count_standalone(results.assertions_passed == 15,
            "Total assertions passed accurate (15)",
            "Total assertions passed should be accurate (15)", _test_info))
        {
            all_assertions_passed = false;
        }

        // Assertions failed: 5
        if (!d_assert_and_count_standalone(results.assertions_failed == 5,
            "Total assertions failed accurate (5)",
            "Total assertions failed should be accurate (5)", _test_info))
        {
            all_assertions_passed = false;
        }

        // Blocks: 3
        if (!d_assert_and_count_standalone(results.blocks_run == 3,
            "Total blocks run accurate (3)",
            "Total blocks run should be accurate (3)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(handler);
    }
    
    d_test_config_free(config);

    // Test 2: Pass rate accuracy
    struct d_test_config* rate_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* rate_handler = d_test_handler_new(rate_config);
    
    if (rate_handler)
    {
        // Run 8 passing and 2 failing (80% pass rate)
        for (int i = 0; i < 8; i++)
        {
            struct d_test* test = d_test_new(handler_test_passing, NULL);
            d_test_handler_run_test(rate_handler, test, NULL);
            d_test_free(test);
        }

        for (int i = 0; i < 2; i++)
        {
            struct d_test* test = d_test_new(handler_test_failing, NULL);
            d_test_handler_run_test(rate_handler, test, NULL);
            d_test_free(test);
        }

        double pass_rate = d_test_handler_get_pass_rate(rate_handler);
        
        bool rate_correct = (pass_rate >= 79.9 && pass_rate <= 80.1);
        
        if (!d_assert_and_count_standalone(rate_correct,
            "Pass rate calculated correctly (80%)",
            "Pass rate should be calculated correctly (80%)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(rate_handler);
    }
    
    d_test_config_free(rate_config);

    // Test 3: Assertion rate accuracy
    struct d_test_config* assert_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* assert_handler = d_test_handler_new(assert_config);
    
    if (assert_handler)
    {
        // 18 passing, 2 failing (90% pass rate)
        for (int i = 0; i < 18; i++)
        {
            struct d_assertion* assertion = d_assert(true, "Pass", "Fail");
            d_test_handler_record_assertion(assert_handler, assertion);
            d_assertion_free(assertion);
        }

        for (int i = 0; i < 2; i++)
        {
            struct d_assertion* assertion = d_assert(false, "Pass", "Fail");
            d_test_handler_record_assertion(assert_handler, assertion);
            d_assertion_free(assertion);
        }

        double assertion_rate = d_test_handler_get_assertion_rate(assert_handler);
        
        bool rate_correct = (assertion_rate >= 89.9 && assertion_rate <= 90.1);
        
        if (!d_assert_and_count_standalone(rate_correct,
            "Assertion rate calculated correctly (90%)",
            "Assertion rate should be calculated correctly (90%)", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(assert_handler);
    }
    
    d_test_config_free(assert_config);

    // Test 4: Statistics consistent across reset cycles
    struct d_test_config* cycle_config = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* cycle_handler = d_test_handler_new(cycle_config);
    
    if (cycle_handler)
    {
        for (int cycle = 0; cycle < 3; cycle++)
        {
            // Run tests
            for (int i = 0; i < 5; i++)
            {
                struct d_test* test = d_test_new(handler_test_passing, NULL);
                d_test_handler_run_test(cycle_handler, test, NULL);
                d_test_free(test);
            }

            struct d_test_result results = d_test_handler_get_results(cycle_handler);
            
            if (results.tests_run != 5)
            {
                all_assertions_passed = false;
                break;
            }

            // Reset for next cycle
            d_test_handler_reset_results(cycle_handler);
        }

        if (!d_assert_and_count_standalone(all_assertions_passed,
            "Statistics consistent across reset cycles",
            "Statistics should be consistent across reset cycles", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(cycle_handler);
    }
    
    d_test_config_free(cycle_config);

    // Test 5: Statistics accurate with and without events
    struct d_test_config* no_events_cfg = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* no_events_handler = d_test_handler_new(no_events_cfg);
    
    struct d_test_config* with_events_cfg = d_test_config_new(D_TEST_MODE_NORMAL);
    struct d_test_handler* with_events_handler = d_test_handler_new_with_events(with_events_cfg, 20);
    
    if (no_events_handler && with_events_handler)
    {
        // Run identical tests on both
        for (int i = 0; i < 5; i++)
        {
            struct d_test* test1 = d_test_new(handler_test_passing, NULL);
            struct d_test* test2 = d_test_new(handler_test_passing, NULL);
            
            d_test_handler_run_test(no_events_handler, test1, NULL);
            d_test_handler_run_test(with_events_handler, test2, NULL);
            
            d_test_free(test1);
            d_test_free(test2);
        }

        struct d_test_result no_events_results = d_test_handler_get_results(no_events_handler);
        struct d_test_result with_events_results = d_test_handler_get_results(with_events_handler);
        
        bool stats_match = (
            no_events_results.tests_run == with_events_results.tests_run &&
            no_events_results.tests_passed == with_events_results.tests_passed &&
            no_events_results.tests_failed == with_events_results.tests_failed
        );
        
        if (!d_assert_and_count_standalone(stats_match,
            "Statistics match with and without event system",
            "Statistics should match with and without event system", _test_info))
        {
            all_assertions_passed = false;
        }

        d_test_handler_free(no_events_handler);
        d_test_handler_free(with_events_handler);
    }
    
    d_test_config_free(no_events_cfg);
    d_test_config_free(with_events_cfg);

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_statistics_accuracy unit test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_statistics_accuracy unit test failed\n", D_INDENT);
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

//=============================================================================
// COMPREHENSIVE TEST SUITE
//=============================================================================

/*
d_tests_sa_test_handler_all
  Runs all test_handler unit tests.
  
  This is the master test function that executes all test_handler tests
  in organized categories. It provides a comprehensive validation of the
  test handler implementation.

Parameter(s):
  _test_info: Test counter for tracking results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_test_handler_all
(
    struct d_test_counter* _test_info
)
{
    printf("\n");
    printf("================================================================================\n");
    printf("RUNNING TEST HANDLER STANDALONE TESTS\n");
    printf("================================================================================\n");
    printf("\n");

    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_categories_passed = true;

    // Handler Creation and Destruction Tests
    printf("--- HANDLER CREATION AND DESTRUCTION TESTS ---\n");
    if (!d_tests_sa_handler_new(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_new_with_events(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_free(_test_info)) all_categories_passed = false;
    printf("\n");

    // Event Listener Registration Tests
    printf("--- EVENT LISTENER REGISTRATION TESTS ---\n");
    if (!d_tests_sa_handler_register_listener(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_unregister_listener(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_listener_enable_disable(_test_info)) all_categories_passed = false;
    printf("\n");

    // Test Execution Tests
    printf("--- TEST EXECUTION TESTS ---\n");
    if (!d_tests_sa_handler_run_test(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_run_block(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_run_test_type(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_nested_execution(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_config_override(_test_info)) all_categories_passed = false;
    printf("\n");

    // Assertion Tracking Tests
    printf("--- ASSERTION TRACKING TESTS ---\n");
    if (!d_tests_sa_handler_record_assertion(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_assertion_statistics(_test_info)) all_categories_passed = false;
    printf("\n");

    // Result Query Tests
    printf("--- RESULT QUERY TESTS ---\n");
    if (!d_tests_sa_handler_get_results(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_reset_results(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_print_results(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_get_pass_rate(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_get_assertion_rate(_test_info)) all_categories_passed = false;
    printf("\n");

    // Event Emission Tests
    printf("--- EVENT EMISSION TESTS ---\n");
    if (!d_tests_sa_handler_emit_event(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_event_lifecycle(_test_info)) all_categories_passed = false;
    printf("\n");

    // Statistics and Depth Tracking Tests
    printf("--- STATISTICS AND DEPTH TRACKING TESTS ---\n");
    if (!d_tests_sa_handler_depth_tracking(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_max_depth(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_block_counting(_test_info)) all_categories_passed = false;
    printf("\n");

    // Abort on Failure Tests
    printf("--- ABORT ON FAILURE TESTS ---\n");
    if (!d_tests_sa_handler_abort_on_failure(_test_info)) all_categories_passed = false;
    printf("\n");

    // Edge Cases and Error Handling Tests
    printf("--- EDGE CASES AND ERROR HANDLING TESTS ---\n");
    if (!d_tests_sa_handler_null_parameters(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_empty_blocks(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_no_events(_test_info)) all_categories_passed = false;
    printf("\n");

    // Integration Tests
    printf("--- INTEGRATION TESTS ---\n");
    if (!d_tests_sa_handler_complex_execution(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_event_integration(_test_info)) all_categories_passed = false;
    if (!d_tests_sa_handler_statistics_accuracy(_test_info)) all_categories_passed = false;
    printf("\n");

    // Print summary
    printf("================================================================================\n");
    printf("TEST HANDLER STANDALONE TESTS SUMMARY\n");
    printf("================================================================================\n");
    
    size_t tests_passed_in_suite = _test_info->tests_passed - initial_tests_passed;
    size_t total_handler_tests = 34; // Total number of test functions
    
    printf("Handler Tests Passed: %zu/%zu\n", tests_passed_in_suite, total_handler_tests);
    
    if (all_categories_passed && tests_passed_in_suite == total_handler_tests)
    {
        printf("Result: [SUCCESS] All test handler tests passed!\n");
    }
    else
    {
        printf("Result: [FAILURE] Some test handler tests failed.\n");
    }
    
    printf("================================================================================\n");
    printf("\n");

    return all_categories_passed;
}