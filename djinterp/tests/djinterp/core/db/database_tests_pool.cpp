/*******************************************************************************
* djinterp [core]                                        database_tests_pool.cpp
*
*
* path:      /tests/djinterp/core/db/database_tests_pool.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
// tests for db/database.hpp section IX (connection_pool / pooled_connection)
#include "database_tests.hpp"

NS_DJINTERP
namespace testing
{
    // make_config
    //   local helper: a pool_config with explicit sizing/validation.
    static pool_config
    make_config(std::size_t _min,
                std::size_t _max,
                bool        _validate_acquire = false,
                bool        _validate_return  = false)
    {
        pool_config config;
        config.min_connections     = _min;
        config.max_connections     = _max;
        config.validate_on_acquire = _validate_acquire;
        config.validate_on_return  = _validate_return;
        return config;
    }

    /*
    tests_pool_construction_preallocate
      Verifies the constructor pre-allocates min_connections (each created
      and connected) and that zero-minimum pools start empty.
      Tests the following:
      - idle == min_connections and each preallocated connection is connected
      - a zero-minimum pool constructs no connections
    */
    bool
    tests_pool_construction_preallocate()
    {
        pool_probe probe;
        {
            connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                                 make_config(3, 10));

            D_DB_CHECK(pool.idle_connections() == 3);
            D_DB_CHECK(pool.active_connections() == 0);
            D_DB_CHECK(pool.total_connections() == 3);
            D_DB_CHECK(probe.created == 3);
            D_DB_CHECK(probe.connect_calls == 3);
            D_DB_CHECK(probe.live == 3);
        }
        // pool destroyed -> all idle connections released
        D_DB_CHECK(probe.live == 0);

        pool_probe probe0;
        connection_pool<mock_pool_conn> empty_pool(make_pool_factory(&probe0),
                                                   make_config(0, 10));
        D_DB_CHECK(empty_pool.idle_connections() == 0);
        D_DB_CHECK(probe0.created == 0);

        return true;
    }

    /*
    tests_pool_acquire_from_idle
      Verifies acquiring when idle is non-empty (no validation) reuses an
      existing connection and that release returns it to idle.
      Tests the following:
      - acquire moves a connection from idle to active without creating one
      - the acquired connection is already connected
      - destroying the handle returns the connection to idle
    */
    bool
    tests_pool_acquire_from_idle()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(2, 10));

        {
            auto pc = pool.acquire();

            D_DB_CHECK(pool.idle_connections() == 1);
            D_DB_CHECK(pool.active_connections() == 1);
            D_DB_CHECK(pc.get() != nullptr);
            D_DB_CHECK(pc->connected() == true);
            D_DB_CHECK(probe.created == 2);   // no new connection created
        }

        // handle destroyed -> connection returned to idle
        D_DB_CHECK(pool.idle_connections() == 2);
        D_DB_CHECK(pool.active_connections() == 0);

        return true;
    }

    /*
    tests_pool_acquire_create_new
      Verifies acquiring from an empty pool below capacity creates a new,
      connected connection.
      Tests the following:
      - acquire on empty idle + under max creates and connects a connection
      - the new connection is active and connected
    */
    bool
    tests_pool_acquire_create_new()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(0, 5));

        {
            auto pc = pool.acquire();

            D_DB_CHECK(probe.created == 1);
            D_DB_CHECK(probe.connect_calls == 1);
            D_DB_CHECK(pool.active_connections() == 1);
            D_DB_CHECK(pool.idle_connections() == 0);
            D_DB_CHECK(pc->connected() == true);
        }

        D_DB_CHECK(pool.idle_connections() == 1);

        return true;
    }

    /*
    tests_pool_acquire_exhausted
      Verifies acquiring when idle is empty and total is at capacity throws.
      Tests the following:
      - a second acquire at max capacity throws connection_exception
    */
    bool
    tests_pool_acquire_exhausted()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(0, 1));

        {
            auto pc1 = pool.acquire();   // total == 1 == max
            D_DB_CHECK(pool.active_connections() == 1);

            D_DB_CHECK_THROWS(pool.acquire(), connection_exception);
        }

        return true;
    }

    /*
    tests_pool_acquire_validate_recreate
      Verifies the validate-on-acquire branches: a passing validation reuses
      the idle connection, a failing one recreates it.
      Tests the following:
      - validation passing reuses the idle connection (no recreate)
      - validation failing discards it and creates a replacement
    */
    bool
    tests_pool_acquire_validate_recreate()
    {
        // validation passes -> reuse
        {
            pool_probe probe;
            probe.ping_mode = 0;   // ping -> true
            connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                                 make_config(1, 10, true, false));

            auto pc = pool.acquire();
            D_DB_CHECK(probe.ping_calls == 1);
            D_DB_CHECK(probe.created == 1);   // reused, not recreated
            D_DB_CHECK(pool.active_connections() == 1);
            D_DB_CHECK(pool.idle_connections() == 0);
        }

        // validation fails -> recreate
        {
            pool_probe probe;
            probe.ping_mode = 1;   // ping -> false
            connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                                 make_config(1, 10, true, false));

            auto pc = pool.acquire();
            D_DB_CHECK(probe.ping_calls == 1);
            D_DB_CHECK(probe.created == 2);   // original discarded, one created
            D_DB_CHECK(probe.connect_calls == 2);
            D_DB_CHECK(pool.active_connections() == 1);
            D_DB_CHECK(pool.idle_connections() == 0);
        }

        return true;
    }

    /*
    tests_pool_release_basic
      Verifies releasing an acquired handle removes it from active and
      returns it to idle.
      Tests the following:
      - explicit release moves the connection from active back to idle
    */
    bool
    tests_pool_release_basic()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(1, 10));

        auto pc = pool.acquire();
        D_DB_CHECK(pool.active_connections() == 1);
        D_DB_CHECK(pool.idle_connections() == 0);

        pc.release();
        D_DB_CHECK(pool.active_connections() == 0);
        D_DB_CHECK(pool.idle_connections() == 1);

        return true;
    }

    /*
    tests_pool_release_null_and_notfound
      Verifies release handles a null connection and a connection absent from
      the active list.
      Tests the following:
      - releasing a null unique_ptr is a no-op
      - releasing an unknown connection still returns it to idle
    */
    bool
    tests_pool_release_null_and_notfound()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(0, 10));

        // null -> no-op
        pool.release(std::unique_ptr<mock_pool_conn>());
        D_DB_CHECK(pool.idle_connections() == 0);
        D_DB_CHECK(pool.active_connections() == 0);

        // foreign (not in active) -> not found, but returned to idle
        auto foreign = std::make_unique<mock_pool_conn>(&probe);
        pool.release(std::move(foreign));
        D_DB_CHECK(pool.idle_connections() == 1);
        D_DB_CHECK(pool.active_connections() == 0);

        return true;
    }

    /*
    tests_pool_release_validate_drop
      Verifies release with validate-on-return drops a connection that fails
      validation instead of returning it to idle.
      Tests the following:
      - a failing validate_on_return discards the connection (not re-pooled)
    */
    bool
    tests_pool_release_validate_drop()
    {
        pool_probe probe;
        probe.ping_mode = 1;   // ping -> false (validation fails)
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(0, 10, false, true));

        auto conn = std::make_unique<mock_pool_conn>(&probe);
        D_DB_CHECK(probe.live == 1);

        pool.release(std::move(conn));
        D_DB_CHECK(probe.ping_calls == 1);       // validation attempted
        D_DB_CHECK(pool.idle_connections() == 0);  // dropped, not pooled
        D_DB_CHECK(probe.live == 0);             // connection destroyed

        return true;
    }

    /*
    tests_pool_release_idle_cap
      Verifies release drops a connection when idle is already at capacity.
      Tests the following:
      - releasing into a full idle pool discards the connection
    */
    bool
    tests_pool_release_idle_cap()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(2, 2));   // idle already at max

        D_DB_CHECK(pool.idle_connections() == 2);
        D_DB_CHECK(probe.live == 2);

        auto foreign = std::make_unique<mock_pool_conn>(&probe);
        D_DB_CHECK(probe.live == 3);

        pool.release(std::move(foreign));   // idle (2) !< max (2) -> drop
        D_DB_CHECK(pool.idle_connections() == 2);
        D_DB_CHECK(probe.live == 2);

        return true;
    }

    /*
    tests_pool_clear
      Verifies clear empties both the idle and active bookkeeping.
      Tests the following:
      - clear drops all idle and active entries, zeroing the counters
    */
    bool
    tests_pool_clear()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(3, 10));

        {
            auto pc = pool.acquire();   // 2 idle + 1 active
            D_DB_CHECK(pool.total_connections() == 3);

            pool.clear();
            D_DB_CHECK(pool.idle_connections() == 0);
            D_DB_CHECK(pool.active_connections() == 0);
            D_DB_CHECK(pool.total_connections() == 0);
        }
        // pc destroyed after clear releases harmlessly back into idle

        return true;
    }

    /*
    tests_pool_resize_grow_shrink
      Verifies resize grows idle up to the new minimum and shrinks it down to
      the new maximum, updating the stored config.
      Tests the following:
      - growing creates connections until idle reaches the new minimum
      - shrinking pops connections until idle reaches the new maximum
      - the config reflects the new min/max
    */
    bool
    tests_pool_resize_grow_shrink()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(1, 10));
        D_DB_CHECK(pool.idle_connections() == 1);

        pool.resize(4, 10);   // grow
        D_DB_CHECK(pool.idle_connections() == 4);
        D_DB_CHECK(probe.created == 4);

        pool.resize(2, 2);    // shrink
        D_DB_CHECK(pool.idle_connections() == 2);
        D_DB_CHECK(pool.get_config().min_connections == 2);
        D_DB_CHECK(pool.get_config().max_connections == 2);

        return true;
    }

    /*
    tests_pool_counters_and_config
      Verifies the counters track active/idle/total, waiting_requests is
      zero, and set_config replaces the config.
      Tests the following:
      - active/idle/total counters reflect an outstanding acquisition
      - waiting_requests is always zero
      - get_config/set_config round-trip a configuration
    */
    bool
    tests_pool_counters_and_config()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(2, 10));

        D_DB_CHECK(pool.active_connections() == 0);
        D_DB_CHECK(pool.idle_connections() == 2);
        D_DB_CHECK(pool.total_connections() == 2);
        D_DB_CHECK(pool.waiting_requests() == 0);

        {
            auto pc = pool.acquire();
            D_DB_CHECK(pool.active_connections() == 1);
            D_DB_CHECK(pool.idle_connections() == 1);
            D_DB_CHECK(pool.total_connections() == 2);
            D_DB_CHECK(pool.waiting_requests() == 0);
        }

        D_DB_CHECK(pool.get_config().min_connections == 2);
        pool.set_config(make_config(5, 20));
        D_DB_CHECK(pool.get_config().min_connections == 5);
        D_DB_CHECK(pool.get_config().max_connections == 20);

        return true;
    }

    /*
    tests_pool_create_connection_failure
      Verifies create_connection throws when the factory yields nullptr.
      Tests the following:
      - acquiring while the factory returns null throws connection_exception
    */
    bool
    tests_pool_create_connection_failure()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(0, 5));

        probe.factory_null = true;   // factory now yields nullptr
        D_DB_CHECK_THROWS(pool.acquire(), connection_exception);

        return true;
    }

    /*
    tests_pool_validate_connection_branches
      Verifies validate_connection across null, ping-true, ping-false, and
      ping-throwing helpers (exercised via a testable subclass).
      Tests the following:
      - a null pointer validates false
      - ping true/false map to true/false
      - a throwing ping is caught and validates false
    */
    bool
    tests_pool_validate_connection_branches()
    {
        pool_probe probe;
        testable_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                           make_config(0, 10));

        // null -> false
        D_DB_CHECK(pool.validate_connection(nullptr) == false);

        auto conn = std::make_unique<mock_pool_conn>(&probe);

        probe.ping_mode = 0;   // ping -> true
        D_DB_CHECK(pool.validate_connection(conn.get()) == true);

        probe.ping_mode = 1;   // ping -> false
        D_DB_CHECK(pool.validate_connection(conn.get()) == false);

        probe.ping_mode = 2;   // ping throws -> caught -> false
        D_DB_CHECK(pool.validate_connection(conn.get()) == false);

        return true;
    }

    /*
    tests_pool_cleanup_idle
      Verifies cleanup_idle_connections is callable and leaves the pool
      unchanged (its body is a no-op placeholder).
      Tests the following:
      - calling cleanup_idle_connections does not alter the idle count
    */
    bool
    tests_pool_cleanup_idle()
    {
        pool_probe probe;
        testable_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                           make_config(2, 10));

        D_DB_CHECK(pool.idle_connections() == 2);
        pool.cleanup_idle_connections();
        D_DB_CHECK(pool.idle_connections() == 2);

        return true;
    }

    /*
    tests_pooled_connection_accessors
      Verifies the pooled_connection accessors (const and non-const).
      Tests the following:
      - get/operator->/operator* refer to the same underlying connection
      - the const overloads yield the same connection
    */
    bool
    tests_pooled_connection_accessors()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(1, 10));

        auto pc = pool.acquire();

        D_DB_CHECK(pc.get() != nullptr);
        D_DB_CHECK(pc.operator->() == pc.get());
        D_DB_CHECK(&(*pc) == pc.get());
        D_DB_CHECK(pc->connected() == true);

        const auto& cpc = pc;
        D_DB_CHECK(cpc.get() == pc.get());
        D_DB_CHECK(cpc.operator->() == pc.get());
        D_DB_CHECK(&(*cpc) == pc.get());
        D_DB_CHECK(cpc->connected() == true);

        return true;
    }

    /*
    tests_pooled_connection_move
      Verifies pooled_connection move construction, move assignment, and the
      self-move guard.
      Tests the following:
      - move construction transfers ownership; the source is emptied
      - move assignment releases the destination's connection then adopts
      - self-move assignment is a guarded no-op
    */
    bool
    tests_pooled_connection_move()
    {
        auto move_assign = [](pooled_connection<mock_pool_conn>& _dst,
                              pooled_connection<mock_pool_conn>& _src)
        {
            _dst = std::move(_src);
        };

        // move construction
        {
            pool_probe probe;
            connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                                 make_config(1, 10));

            auto a = pool.acquire();
            mock_pool_conn* raw = a.get();

            pooled_connection<mock_pool_conn> b(std::move(a));
            D_DB_CHECK(b.get() == raw);
            D_DB_CHECK(a.get() == nullptr);
        }

        // move assignment
        {
            pool_probe probe;
            connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                                 make_config(2, 10));

            auto a = pool.acquire();
            auto b = pool.acquire();
            mock_pool_conn* raw_a = a.get();
            D_DB_CHECK(pool.active_connections() == 2);

            b = std::move(a);            // releases b's original, adopts a's
            D_DB_CHECK(b.get() == raw_a);
            D_DB_CHECK(a.get() == nullptr);
            D_DB_CHECK(pool.active_connections() == 1);
            D_DB_CHECK(pool.idle_connections() == 1);
        }

        // self-move assignment (guarded no-op)
        {
            pool_probe probe;
            connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                                 make_config(1, 10));

            auto a = pool.acquire();
            mock_pool_conn* raw = a.get();

            move_assign(a, a);
            D_DB_CHECK(a.get() == raw);
            D_DB_CHECK(pool.active_connections() == 1);
        }

        return true;
    }

    /*
    tests_pooled_connection_release_idempotent
      Verifies release is idempotent: the first returns the connection, later
      calls are no-ops.
      Tests the following:
      - the first release returns the connection to idle and empties the handle
      - a second release does nothing
    */
    bool
    tests_pooled_connection_release_idempotent()
    {
        pool_probe probe;
        connection_pool<mock_pool_conn> pool(make_pool_factory(&probe),
                                             make_config(1, 10));

        auto pc = pool.acquire();
        D_DB_CHECK(pool.active_connections() == 1);

        pc.release();
        D_DB_CHECK(pc.get() == nullptr);
        D_DB_CHECK(pool.active_connections() == 0);
        D_DB_CHECK(pool.idle_connections() == 1);

        pc.release();   // idempotent no-op
        D_DB_CHECK(pool.active_connections() == 0);
        D_DB_CHECK(pool.idle_connections() == 1);

        return true;
    }

}   // namespace testing
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
