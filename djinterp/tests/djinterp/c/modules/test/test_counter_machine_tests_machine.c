/*******************************************************************************
* djinterp [c][test]                        test_counter_machine_tests_machine.c
*
* The test_counter_machine suite, block "machine": test_counter.h's
* option_env schema and d_test_tally_options_from_env, on a frame over
* storage of the test's own.
*
*
* path:      /tests/djinterp/c/modules/test/test_counter_machine_tests_machine.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./test_counter_machine_tests.h"  // corresponding header

// std
#include <stdio.h>   // snprintf
#include <string.h>  // strcmp


#if ( D_INTERNAL_TEST_COUNTER_MACHINE && RE_STD_HAS_INT64 )

    // dj_tests_test_counter_machine_storage
    //   struct: room for one frame that declares the schema and binds each
    // of its variables a few times over.
    D_OPTION_ENV_DECLARE(dj_tests_test_counter_machine_storage,
                         16,
                         D_TEST_COUNTER_ENV_VAR_COUNT,
                         256);

    /**
     * @brief Sets up a frame over `_storage`, declaring the schema when
     *        `_is_declared`.
     *
     * @param[out] _env          the frame.
     * @param[out] _storage      its storage.
     * @param[in]  _is_declared  whether the frame declares the schema.
     * @return `true` when the frame was made, and declared if asked.
     */
    static bool
    d_tests_test_counter_machine_frame(
        struct dj_option_env*                         _env,
        struct dj_tests_test_counter_machine_storage* _storage,
        bool                                          _is_declared
    )
    {
        const struct dj_tests_test_counter_machine_storage empty =
            D_OPTION_ENV_STORAGE_INIT;

        *_storage = empty;

        // the frame, then the schema if asked
        if (!D_OPTION_ENV_VIEW(_env,
                               *_storage).is_ok)
        {
            return false;
        }

        return ( (!_is_declared) ||
                 (d_option_env_declare_all(_env,
                                           d_test_counter_env_vars(),
                                           D_TEST_COUNTER_ENV_VAR_COUNT)
                      .is_ok) );
    }

    /*
    d_tests_test_counter_machine_schema
      The schema.
      Tests the following:
      - four rows: tally_limit, tally_when_full, counter_step, counter_floor
      - each default is its knob's value, and names the knob
      - tally_limit's range is 1 to the capacity; tally_when_full takes two
        names
      - a frame declares the schema once; a second declaration is refused
    */
    bool
    d_tests_test_counter_machine_schema(void)
    {
        const struct dj_option_env_var*              vars    =
            d_test_counter_env_vars();
        struct dj_tests_test_counter_machine_storage storage;
        struct dj_option_env                         env;

        D_TEST_REQUIRE(vars != NULL);
        D_TEST_REQUIRE(D_TEST_COUNTER_ENV_VAR_COUNT == 4u);
        D_TEST_REQUIRE(strcmp(vars[0].name,
                              "tally_limit") == 0);
        D_TEST_REQUIRE(strcmp(vars[1].name,
                              "tally_when_full") == 0);
        D_TEST_REQUIRE(strcmp(vars[2].name,
                              "counter_step") == 0);
        D_TEST_REQUIRE(strcmp(vars[3].name,
                              "counter_floor") == 0);

        D_TEST_REQUIRE(vars[0].integer ==
                       (int64_t)D_INTERNAL_TEST_TALLY_CAPACITY);
        D_TEST_REQUIRE(vars[0].minimum == 1);
        D_TEST_REQUIRE(vars[0].maximum ==
                       (int64_t)D_INTERNAL_TEST_TALLY_CAPACITY);
        D_TEST_REQUIRE(vars[1].integer ==
                       (int64_t)D_INTERNAL_TEST_TALLY_WHEN_FULL);
        D_TEST_REQUIRE(vars[1].name_count == 2u);
        D_TEST_REQUIRE(vars[2].integer ==
                       (int64_t)D_INTERNAL_TEST_COUNTER_STEP);
        D_TEST_REQUIRE(vars[3].integer ==
                       ((D_INTERNAL_TEST_COUNTER_FLOOR != 0) ? 1 : 0));

        // each names its knob
        for (uint32_t i = 0; i < D_TEST_COUNTER_ENV_VAR_COUNT; ++i)
        {
            D_TEST_REQUIRE(vars[i].knob != NULL);
        }

        D_TEST_REQUIRE(d_tests_test_counter_machine_frame(&env,
                                                          &storage,
                                                          true));
        D_TEST_REQUIRE(!d_option_env_declare_all(&env,
                                                 vars,
                                                 D_TEST_COUNTER_ENV_VAR_COUNT)
                            .is_ok);

        return true;
    }

    /*
    d_tests_test_counter_machine_defaults
      Reading a frame that changes nothing.
      Tests the following:
      - a frame that declares the schema and binds nothing reads the knobs
      - so does a frame that does not declare it, and no frame at all
      - the options read need somewhere to go
    */
    bool
    d_tests_test_counter_machine_defaults(void)
    {
        struct dj_tests_test_counter_machine_storage storage;
        struct dj_option_env                         env;
        struct dj_test_tally_options                 knobs;
        struct dj_test_tally_options                 read;

        d_test_tally_options_init(&knobs);

        D_TEST_REQUIRE(d_tests_test_counter_machine_frame(&env,
                                                          &storage,
                                                          true));
        D_TEST_REQUIRE(d_test_tally_options_from_env(&env,
                                                     D_OPTION_ENV_GLOBAL,
                                                     &read));
        D_TEST_REQUIRE(read.limit == knobs.limit);
        D_TEST_REQUIRE(read.when_full == knobs.when_full);
        D_TEST_REQUIRE(read.counter.step == knobs.counter.step);
        D_TEST_REQUIRE(read.counter.has_floor == knobs.counter.has_floor);

        D_TEST_REQUIRE(d_tests_test_counter_machine_frame(&env,
                                                          &storage,
                                                          false));
        D_TEST_REQUIRE(d_test_tally_options_from_env(&env,
                                                     D_OPTION_ENV_GLOBAL,
                                                     &read));
        D_TEST_REQUIRE(read.limit == knobs.limit);
        D_TEST_REQUIRE(read.counter.step == knobs.counter.step);

        D_TEST_REQUIRE(d_test_tally_options_from_env(NULL,
                                                     D_OPTION_ENV_GLOBAL,
                                                     &read));
        D_TEST_REQUIRE(read.limit == knobs.limit);
        D_TEST_REQUIRE(!d_test_tally_options_from_env(&env,
                                                      D_OPTION_ENV_GLOBAL,
                                                      NULL));

        return true;
    }

    /*
    d_tests_test_counter_machine_bound
      Reading what a program bound.
      Tests the following:
      - bindings made by text, one statement or a script, are read into the
        tally options
      - a tally set up from them counts as they say
    */
    bool
    d_tests_test_counter_machine_bound(void)
    {
        struct dj_tests_test_counter_machine_storage storage;
        struct dj_option_env                         env;
        struct dj_test_tally_options                 read;
        struct dj_test_tally                         tally;

        D_TEST_REQUIRE(d_tests_test_counter_machine_frame(&env,
                                                          &storage,
                                                          true));
        D_TEST_REQUIRE(d_option_env_put(&env,
                                        "tally_limit=1").is_ok);
        D_TEST_REQUIRE(d_option_env_load(&env,
                                         "tally_when_full=overflow; "
                                         "counter_step=5; "
                                         "counter_floor=true",
                                         NULL).is_ok);

        D_TEST_REQUIRE(d_test_tally_options_from_env(&env,
                                                     D_OPTION_ENV_GLOBAL,
                                                     &read));
        D_TEST_REQUIRE(read.limit == 1);
        D_TEST_REQUIRE(read.when_full == D_TEST_TALLY_FULL_OVERFLOW);
        D_TEST_REQUIRE(read.counter.step == 5);
        D_TEST_REQUIRE(read.counter.has_floor);

        D_TEST_REQUIRE(d_test_tally_init(&tally,
                                         &read));
        D_TEST_REQUIRE(d_test_tally_record(&tally,
                                           (dj_test_key)1));
        D_TEST_REQUIRE(d_test_tally_record(&tally,
                                           (dj_test_key)2));
        D_TEST_REQUIRE(d_test_tally_count(&tally,
                                          (dj_test_key)1) == 5);
        D_TEST_REQUIRE(d_test_tally_overflow(&tally) == 5);

        return true;
    }

    /*
    d_tests_test_counter_machine_refused
      What the schema keeps out.
      Tests the following:
      - a tally_limit of 0 or above the capacity is refused when bound
      - a tally_when_full that names neither policy is refused
      - a counter_step of 0, which the schema takes, is read as the knob's
    */
    bool
    d_tests_test_counter_machine_refused(void)
    {
        struct dj_tests_test_counter_machine_storage storage;
        struct dj_option_env                         env;
        struct dj_test_tally_options                 knobs;
        struct dj_test_tally_options                 read;
        char                                         over[48];

        d_test_tally_options_init(&knobs);
        (void)snprintf(over,
                       sizeof(over),
                       "tally_limit=%d",
                       (int)D_INTERNAL_TEST_TALLY_CAPACITY + 1);

        D_TEST_REQUIRE(d_tests_test_counter_machine_frame(&env,
                                                          &storage,
                                                          true));
        D_TEST_REQUIRE(!d_option_env_put(&env,
                                         "tally_limit=0").is_ok);
        D_TEST_REQUIRE(!d_option_env_put(&env,
                                         over).is_ok);
        D_TEST_REQUIRE(!d_option_env_put(&env,
                                         "tally_when_full=sometimes").is_ok);
        D_TEST_REQUIRE(d_option_env_put(&env,
                                        "counter_step=0").is_ok);

        D_TEST_REQUIRE(d_test_tally_options_from_env(&env,
                                                     D_OPTION_ENV_GLOBAL,
                                                     &read));
        D_TEST_REQUIRE(read.limit == knobs.limit);
        D_TEST_REQUIRE(read.when_full == knobs.when_full);
        D_TEST_REQUIRE(read.counter.step == knobs.counter.step);

        return true;
    }

#else

    // the machine is not built: each test says so, and skips

    bool
    d_tests_test_counter_machine_schema(void)
    {
        D_TEST_SKIP("built without D_CFG_TEST_COUNTER_MACHINE");

        return true;
    }

    bool
    d_tests_test_counter_machine_defaults(void)
    {
        D_TEST_SKIP("built without D_CFG_TEST_COUNTER_MACHINE");

        return true;
    }

    bool
    d_tests_test_counter_machine_bound(void)
    {
        D_TEST_SKIP("built without D_CFG_TEST_COUNTER_MACHINE");

        return true;
    }

    bool
    d_tests_test_counter_machine_refused(void)
    {
        D_TEST_SKIP("built without D_CFG_TEST_COUNTER_MACHINE");

        return true;
    }

#endif  // D_INTERNAL_TEST_COUNTER_MACHINE && RE_STD_HAS_INT64
