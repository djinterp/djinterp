/*******************************************************************************
* djinterp [core]                                 database_tests_transaction.cpp
*
*
* path:      /tests/djinterp/core/db/database_tests_transaction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// tests for db/database.hpp section VII (transaction RAII template)
#include "database_tests.hpp"

NS_DJINTERP
namespace testing
{
    /*
    tests_transaction_begin_commit
      Verifies the plain constructor begins on the connection and that commit
      finalizes without a destructor rollback.
      Tests the following:
      - construction calls begin_transaction() and marks the txn active
      - the default isolation level is default_level
      - commit forwards, marks inactive, and suppresses the dtor rollback
    */
    bool
    tests_transaction_begin_commit()
    {
        txn_probe probe;
        mock_txn_conn conn(&probe);

        {
            transaction<mock_txn_conn> txn(conn);

            D_DB_CHECK(probe.begin_calls == 1);
            D_DB_CHECK(txn.is_active() == true);
            D_DB_CHECK(txn.get_isolation_level() == isolation_level::default_level);

            txn.commit();

            D_DB_CHECK(probe.commit_calls == 1);
            D_DB_CHECK(txn.is_active() == false);
        }

        // committed -> no destructor rollback
        D_DB_CHECK(probe.rollback_calls == 0);

        return true;
    }

    /*
    tests_transaction_begin_isolation
      Verifies the isolation-taking constructor forwards the level.
      Tests the following:
      - construction calls begin_transaction(isolation_level) with the level
      - get_isolation_level reflects the constructed level
    */
    bool
    tests_transaction_begin_isolation()
    {
        txn_probe probe;
        mock_txn_conn conn(&probe);

        {
            transaction<mock_txn_conn> txn(conn, isolation_level::serializable);

            D_DB_CHECK(probe.begin_iso_calls == 1);
            D_DB_CHECK(probe.begin_calls == 0);
            D_DB_CHECK(probe.last_iso == isolation_level::serializable);
            D_DB_CHECK(txn.get_isolation_level() == isolation_level::serializable);
            D_DB_CHECK(txn.is_active() == true);

            txn.commit();
        }

        D_DB_CHECK(probe.rollback_calls == 0);

        return true;
    }

    /*
    tests_transaction_rollback
      Verifies explicit rollback forwards and deactivates, without a second
      rollback from the destructor.
      Tests the following:
      - rollback forwards to the connection and marks the txn inactive
      - the destructor does not roll back an already rolled-back txn
    */
    bool
    tests_transaction_rollback()
    {
        txn_probe probe;
        mock_txn_conn conn(&probe);

        {
            transaction<mock_txn_conn> txn(conn);
            txn.rollback();

            D_DB_CHECK(probe.rollback_calls == 1);
            D_DB_CHECK(txn.is_active() == false);
        }

        // no additional rollback at destruction
        D_DB_CHECK(probe.rollback_calls == 1);

        return true;
    }

    /*
    tests_transaction_dtor_rollback
      Verifies destructor behavior across the active/committed matrix,
      including suppression of a throwing rollback.
      Tests the following:
      - an active, uncommitted txn rolls back at destruction
      - a committed txn does not roll back at destruction
      - a rollback that throws during destruction is suppressed
    */
    bool
    tests_transaction_dtor_rollback()
    {
        // active + uncommitted -> destructor rolls back
        {
            txn_probe probe;
            mock_txn_conn conn(&probe);
            {
                transaction<mock_txn_conn> txn(conn);
                (void)txn;
            }
            D_DB_CHECK(probe.rollback_calls == 1);
        }

        // committed -> destructor does NOT roll back
        {
            txn_probe probe;
            mock_txn_conn conn(&probe);
            {
                transaction<mock_txn_conn> txn(conn);
                txn.commit();
            }
            D_DB_CHECK(probe.commit_calls == 1);
            D_DB_CHECK(probe.rollback_calls == 0);
        }

        // rollback throws during destruction -> suppressed (no escape)
        {
            txn_probe probe;
            probe.rollback_throws = true;
            mock_txn_conn conn(&probe);

            D_DB_CHECK_NOTHROW(
                ([&]() { transaction<mock_txn_conn> txn(conn); (void)txn; }()));

            D_DB_CHECK(probe.rollback_calls == 1);
        }

        return true;
    }

    /*
    tests_transaction_inactive_throws
      Verifies operations on an inactive transaction throw
      transaction_exception.
      Tests the following:
      - a second commit and a rollback after commit both throw
      - a savepoint operation on an inactive txn throws
    */
    bool
    tests_transaction_inactive_throws()
    {
        txn_probe probe;
        mock_txn_conn conn(&probe);

        transaction<mock_txn_conn> txn(conn);
        txn.commit();
        D_DB_CHECK(txn.is_active() == false);

        D_DB_CHECK_THROWS(txn.commit(), transaction_exception);
        D_DB_CHECK_THROWS(txn.rollback(), transaction_exception);
        D_DB_CHECK_THROWS(txn.create_savepoint("sp"), transaction_exception);
        D_DB_CHECK_THROWS(txn.rollback_to_savepoint("sp"), transaction_exception);
        D_DB_CHECK_THROWS(txn.release_savepoint("sp"), transaction_exception);

        // the failed operations must not have reached the connection
        D_DB_CHECK(probe.commit_calls == 1);
        D_DB_CHECK(probe.rollback_calls == 0);
        D_DB_CHECK(probe.savepoint_calls == 0);
        D_DB_CHECK(probe.rollback_sp_calls == 0);
        D_DB_CHECK(probe.release_sp_calls == 0);

        return true;
    }

    /*
    tests_transaction_move_ctor
      Verifies move construction transfers ownership and neutralizes the
      moved-from transaction.
      Tests the following:
      - the destination becomes active; the source becomes inactive
      - only one begin occurred and the moved-from source does not roll back
    */
    bool
    tests_transaction_move_ctor()
    {
        txn_probe probe;
        mock_txn_conn conn(&probe);

        {
            transaction<mock_txn_conn> a(conn);
            transaction<mock_txn_conn> b(std::move(a));

            D_DB_CHECK(b.is_active() == true);
            D_DB_CHECK(a.is_active() == false);
            D_DB_CHECK(probe.begin_calls == 1);

            b.commit();
            D_DB_CHECK(probe.commit_calls == 1);
        }

        // neither the moved-from source nor the committed destination rolls back
        D_DB_CHECK(probe.rollback_calls == 0);

        return true;
    }

    /*
    tests_transaction_move_assign
      Verifies move assignment rolls back the destination's existing
      transaction, transfers ownership, and guards against self-assignment.
      Tests the following:
      - assigning into an active txn rolls back its original connection
      - the source is neutralized and the destination adopts the source
      - self-move is a guarded no-op (no rollback, stays active)
    */
    bool
    tests_transaction_move_assign()
    {
        // route a self-move through a two-parameter helper so -Wself-move is
        // not triggered while still aliasing at run time
        auto move_assign = [](transaction<mock_txn_conn>& _dst,
                              transaction<mock_txn_conn>& _src)
        {
            _dst = std::move(_src);
        };

        // move-assign into an active transaction
        {
            txn_probe p1;
            txn_probe p2;
            mock_txn_conn c1(&p1);
            mock_txn_conn c2(&p2);

            {
                transaction<mock_txn_conn> a(c1);
                transaction<mock_txn_conn> b(c2);

                b = std::move(a);   // b's original (c2) txn is rolled back

                D_DB_CHECK(p2.rollback_calls == 1);
                D_DB_CHECK(a.is_active() == false);
                D_DB_CHECK(b.is_active() == true);

                b.commit();         // commits the adopted (c1) txn
                D_DB_CHECK(p1.commit_calls == 1);
            }

            D_DB_CHECK(p1.rollback_calls == 0);
            D_DB_CHECK(p2.rollback_calls == 1);
        }

        // self-move assignment is a guarded no-op
        {
            txn_probe probe;
            mock_txn_conn conn(&probe);

            transaction<mock_txn_conn> a(conn);
            move_assign(a, a);      // this == &other -> guard skips everything

            D_DB_CHECK(a.is_active() == true);
            D_DB_CHECK(probe.rollback_calls == 0);

            a.commit();
        }

        // the destination's rollback throws during reassignment -> suppressed
        {
            txn_probe p_throw;
            p_throw.rollback_throws = true;
            mock_txn_conn c_throw(&p_throw);

            txn_probe p_src;
            mock_txn_conn c_src(&p_src);

            {
                transaction<mock_txn_conn> src(c_src);
                transaction<mock_txn_conn> dst(c_throw);

                // dst is active+uncommitted; its rollback throws but must be
                // swallowed by the move-assignment's catch(...)
                D_DB_CHECK_NOTHROW((dst = std::move(src)));

                D_DB_CHECK(p_throw.rollback_calls == 1);
                D_DB_CHECK(dst.is_active() == true);
                D_DB_CHECK(src.is_active() == false);

                dst.commit();                       // commits the adopted (c_src) txn
                D_DB_CHECK(p_src.commit_calls == 1);
            }
        }

        return true;
    }

    /*
    tests_transaction_savepoints
      Verifies the SFINAE-gated savepoint operations forward their names.
      Tests the following:
      - create_savepoint/rollback_to_savepoint/release_savepoint forward once
      - each forwards the savepoint name argument
    */
    bool
    tests_transaction_savepoints()
    {
        txn_probe probe;
        mock_txn_conn conn(&probe);

        transaction<mock_txn_conn> txn(conn);

        txn.create_savepoint("sp1");
        D_DB_CHECK(probe.savepoint_calls == 1);
        D_DB_CHECK(probe.last_savepoint == "sp1");

        txn.rollback_to_savepoint("sp2");
        D_DB_CHECK(probe.rollback_sp_calls == 1);
        D_DB_CHECK(probe.last_savepoint == "sp2");

        txn.release_savepoint("sp3");
        D_DB_CHECK(probe.release_sp_calls == 1);
        D_DB_CHECK(probe.last_savepoint == "sp3");

        txn.commit();

        return true;
    }

    /*
    tests_transaction_savepoint_sfinae
      Verifies the savepoint operations are enabled only when the connection
      provides them, and that a savepoint-less connection still transacts.
      Tests the following:
      - the savepoint gate is ON for a connection with savepoints, OFF without
      - a savepoint-less connection supports begin/commit
    */
    bool
    tests_transaction_savepoint_sfinae()
    {
        static_assert(txn_has_savepoint<transaction<mock_txn_conn>>::value,
                      "savepoints must be enabled for mock_txn_conn");
        static_assert(!txn_has_savepoint<transaction<mock_txn_conn_nosp>>::value,
                      "savepoints must be disabled for mock_txn_conn_nosp");

        txn_probe probe;
        mock_txn_conn_nosp conn(&probe);
        {
            transaction<mock_txn_conn_nosp> txn(conn);
            D_DB_CHECK(probe.begin_calls == 1);
            txn.commit();
            D_DB_CHECK(probe.commit_calls == 1);

            // a second commit on the now-inactive txn throws
            D_DB_CHECK_THROWS(txn.commit(), transaction_exception);
            D_DB_CHECK(probe.commit_calls == 1);
        }

        // a savepoint-less transaction also rolls back at destruction
        {
            transaction<mock_txn_conn_nosp> txn(conn);
            (void)txn;
        }
        D_DB_CHECK(probe.rollback_calls == 1);

        return true;
    }

    /*
    tests_transaction_accessors
      Verifies is_active and get_isolation_level track state and level.
      Tests the following:
      - is_active is true after begin and false after commit
      - get_isolation_level reflects the constructor-supplied level
    */
    bool
    tests_transaction_accessors()
    {
        txn_probe probe;
        mock_txn_conn conn(&probe);

        {
            transaction<mock_txn_conn> txn(conn);
            D_DB_CHECK(txn.is_active() == true);
            D_DB_CHECK(txn.get_isolation_level() == isolation_level::default_level);
            txn.commit();
            D_DB_CHECK(txn.is_active() == false);
        }

        {
            transaction<mock_txn_conn> txn(conn, isolation_level::repeatable_read);
            D_DB_CHECK(txn.get_isolation_level() == isolation_level::repeatable_read);
            txn.commit();
        }

        return true;
    }

}   // namespace testing
NS_END  // djinterp
