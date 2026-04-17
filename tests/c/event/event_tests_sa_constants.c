#include "./event_tests_sa.h"


/*
d_tests_sa_event_default_size
  Tests the D_EVENT_LISTENER_DEFAULT_SIZE constant.
  Tests the following:
  - Value is positive
  - Value equals 64
  - Value is a power of two
*/
bool
d_tests_sa_event_default_size
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        D_EVENT_LISTENER_DEFAULT_SIZE > 0,
        "default_size_positive",
        "D_EVENT_LISTENER_DEFAULT_SIZE should be positive",
        _counter) && result;

    result = d_assert_standalone(
        D_EVENT_LISTENER_DEFAULT_SIZE == 64,
        "default_size_value",
        "D_EVENT_LISTENER_DEFAULT_SIZE should equal 64",
        _counter) && result;

    result = d_assert_standalone(
        (D_EVENT_LISTENER_DEFAULT_SIZE &
            (D_EVENT_LISTENER_DEFAULT_SIZE - 1)) == 0,
        "default_size_power_of_two",
        "D_EVENT_LISTENER_DEFAULT_SIZE should be a power of two",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_id_type
  Tests the D_EVENT_ID_TYPE macro and d_event_id typedef.
  Tests the following:
  - d_event_id can hold zero, positive, and negative values
  - sizeof matches D_EVENT_ID_TYPE and int
*/
bool
d_tests_sa_event_id_type
(
    struct d_test_counter* _counter
)
{
    bool       result;
    d_event_id id;

    result = true;

    id = 0;

    result = d_assert_standalone(
        id == 0,
        "event_id_zero",
        "d_event_id should hold zero",
        _counter) && result;

    id = 42;

    result = d_assert_standalone(
        id == 42,
        "event_id_positive",
        "d_event_id should hold positive values",
        _counter) && result;

    id = -1;

    result = d_assert_standalone(
        id == -1,
        "event_id_negative",
        "d_event_id should hold negative values",
        _counter) && result;

    result = d_assert_standalone(
        sizeof(d_event_id) == sizeof(D_EVENT_ID_TYPE),
        "event_id_sizeof_macro",
        "sizeof(d_event_id) should equal sizeof(D_EVENT_ID_TYPE)",
        _counter) && result;

    result = d_assert_standalone(
        sizeof(d_event_id) == sizeof(int),
        "event_id_sizeof_int",
        "Default d_event_id should be sizeof(int)",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_struct
  Tests the d_event structure layout.
  Tests the following:
  - id, args, num_args members are accessible and assignable
  - sizeof(struct d_event) is non-zero
*/
bool
d_tests_sa_event_struct
(
    struct d_test_counter* _counter
)
{
    bool           result;
    struct d_event ev;
    int            dummy;

    result = true;
    dummy  = 42;

    ev.id       = 100;
    ev.args     = &dummy;
    ev.num_args = 1;

    result = d_assert_standalone(
        ev.id == 100 && ev.args == &dummy && ev.num_args == 1,
        "event_struct_members",
        "All d_event members should be accessible",
        _counter) && result;

    ev.args     = NULL;
    ev.num_args = 0;

    result = d_assert_standalone(
        ev.args == NULL && ev.num_args == 0,
        "event_struct_null_args",
        "d_event should accept NULL args and zero num_args",
        _counter) && result;

    result = d_assert_standalone(
        sizeof(struct d_event) > 0,
        "event_struct_sizeof",
        "sizeof(struct d_event) should be non-zero",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_listener_struct
  Tests the d_event_listener structure layout.
  Tests the following:
  - id, fn, enabled members are accessible and assignable
  - sizeof(struct d_event_listener) is non-zero
*/
bool
d_tests_sa_event_listener_struct
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_event_listener lis;

    result = true;

    lis.id      = 200;
    lis.fn      = NULL;
    lis.enabled = true;

    result = d_assert_standalone(
        lis.id == 200 && lis.fn == NULL && lis.enabled == true,
        "listener_struct_members",
        "All d_event_listener members should be accessible",
        _counter) && result;

    lis.enabled = false;

    result = d_assert_standalone(
        lis.enabled == false,
        "listener_struct_enabled_false",
        "enabled should accept false",
        _counter) && result;

    result = d_assert_standalone(
        sizeof(struct d_event_listener) > 0,
        "listener_struct_sizeof",
        "sizeof(struct d_event_listener) should be non-zero",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_constant_all
  Runs all constant and type tests.
*/
bool
d_tests_sa_event_constant_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constants and Types\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_event_default_size(_counter)    && result;
    result = d_tests_sa_event_id_type(_counter)         && result;
    result = d_tests_sa_event_struct(_counter)           && result;
    result = d_tests_sa_event_listener_struct(_counter)  && result;

    return result;
}
