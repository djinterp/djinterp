/*******************************************************************************
* djinterp [tests]                                   test_stress_tests_chaos.cpp
*
*   Section TU: chaos_runner - the weighted op-mix runner.
*
*   Op selection is a seeded mt19937_64 draw per thread (seed ^ tid), so a
* given (seed, thread count, iteration count, weight table) reproduces the
* same per-op totals within a process regardless of scheduling.  That makes
* the determinism assertion exact; the distribution assertions use margins
* wide enough that they are not statistical coin flips.
*
*
* path:      /tests/djinterp/test/sync/test_stress_tests_chaos.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <string>
#include <vector>
// djinterp
#include "test_stress_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace {

// hs_null_op
//   local helper: an op action that does nothing.  The chaos tests care about
// which op was SELECTED, which the runner tallies for us.
inline dt::stress_op::operation_fn hs_null_op()
{
    return dt::stress_op::operation_fn(
        [](std::size_t /*tid*/, std::size_t /*iter*/) {});
}

// hs_sum
//   local helper: total of a size_type vector.
inline std::size_t hs_sum(const std::vector<std::size_t>& _v)
{
    std::size_t total = 0;

    for (std::size_t i = 0; i < _v.size(); ++i)
    {
        total += _v[i];
    }

    return total;
}

}  // anonymous namespace


bool tests_schaos_defaults()
{
    dt::chaos_runner chaos;

    D_HS_CHECK(chaos.op_count() == 0u);

    return true;
}

bool tests_schaos_add_op_named()
{
    dt::chaos_runner chaos;

    chaos.add_op(std::string("read"), 3, hs_null_op());
    D_HS_CHECK(chaos.op_count() == 1u);

    chaos.add_op(std::string("write"), 1, hs_null_op());
    D_HS_CHECK(chaos.op_count() == 2u);

    return true;
}

bool tests_schaos_add_op_anonymous()
{
    dt::chaos_runner chaos;

    chaos.add_op(hs_null_op());
    chaos.add_op(hs_null_op());

    D_HS_CHECK(chaos.op_count() == 2u);

    // the anonymous overload names its ops, which the report surfaces
    chaos.set_thread_count(1);
    chaos.set_iterations(10);

    const dt::stress_report rep = chaos.run();

    D_HS_CHECK(rep.op_names.size() == 2u);
    D_HS_CHECK(!rep.op_names[0].empty());
    D_HS_CHECK(!rep.op_names[1].empty());

    return true;
}

bool tests_schaos_run_without_ops()
{
    dt::chaos_runner chaos;
    chaos.set_thread_count(2);
    chaos.set_iterations(10);
    // no ops registered

    const dt::stress_report rep = chaos.run();

    D_HS_CHECK(rep.total_ops == 0u);
    D_HS_CHECK(rep.ops_per_thread.empty());
    D_HS_CHECK(rep.ops_per_kind.empty());

    return true;
}

bool tests_schaos_run_zero_threads()
{
    dt::chaos_runner chaos;
    chaos.set_thread_count(0);
    chaos.set_iterations(10);
    chaos.add_op(hs_null_op());

    const dt::stress_report rep = chaos.run();

    D_HS_CHECK(rep.total_ops == 0u);
    D_HS_CHECK(rep.ops_per_kind.empty());

    return true;
}

bool tests_schaos_run_total_ops()
{
    std::atomic<long> hits(0);

    dt::chaos_runner chaos;
    chaos.set_thread_count(3);
    chaos.set_iterations(40);
    chaos.add_op(
        std::string("only"),
        1,
        dt::stress_op::operation_fn(
            [&hits](std::size_t, std::size_t)
            {
                hits.fetch_add(1, std::memory_order_relaxed);
            }));

    const dt::stress_report rep = chaos.run();

    D_HS_CHECK(rep.total_ops == 120u);                            // 3 x 40
    D_HS_CHECK(hits.load(std::memory_order_relaxed) == 120);
    D_HS_CHECK(rep.ops_per_thread.size() == 3u);

    for (std::size_t i = 0; i < rep.ops_per_thread.size(); ++i)
    {
        D_HS_CHECK(rep.ops_per_thread[i] == 40u);
    }

    return true;
}

bool tests_schaos_run_op_names_reported()
{
    dt::chaos_runner chaos;
    chaos.set_thread_count(1);
    chaos.set_iterations(20);
    chaos.add_op(std::string("alpha"), 1, hs_null_op());
    chaos.add_op(std::string("beta"),  1, hs_null_op());
    chaos.add_op(std::string("gamma"), 1, hs_null_op());

    const dt::stress_report rep = chaos.run();

    // one name per registered op, in registration order
    D_HS_CHECK(rep.op_names.size() == 3u);
    D_HS_CHECK(rep.op_names[0] == "alpha");
    D_HS_CHECK(rep.op_names[1] == "beta");
    D_HS_CHECK(rep.op_names[2] == "gamma");
    D_HS_CHECK(rep.ops_per_kind.size() == 3u);

    return true;
}

bool tests_schaos_run_kind_counts_sum()
{
    dt::chaos_runner chaos;
    chaos.set_thread_count(2);
    chaos.set_iterations(100);
    chaos.add_op(std::string("a"), 1, hs_null_op());
    chaos.add_op(std::string("b"), 2, hs_null_op());
    chaos.add_op(std::string("c"), 3, hs_null_op());

    const dt::stress_report rep = chaos.run();

    // every executed op is tallied exactly once into some kind bucket
    D_HS_CHECK(hs_sum(rep.ops_per_kind) == rep.total_ops);
    D_HS_CHECK(hs_sum(rep.ops_per_thread) == rep.total_ops);
    D_HS_CHECK(rep.total_ops == 200u);

    return true;
}

bool tests_schaos_run_weight_bias()
{
    // a 99:1 weight split over 2000 draws: the heavy op must dominate by a
    // wide margin.  The margin asserted (5x) is far below the expected 99x,
    // so this is not a statistical coin flip.
    dt::chaos_runner chaos;
    chaos.set_thread_count(1);
    chaos.set_iterations(2000);
    chaos.set_seed(4242u);
    chaos.add_op(std::string("heavy"), 99, hs_null_op());
    chaos.add_op(std::string("light"), 1,  hs_null_op());

    const dt::stress_report rep = chaos.run();

    D_HS_CHECK(rep.ops_per_kind.size() == 2u);
    D_HS_CHECK(rep.ops_per_kind[0] > rep.ops_per_kind[1]);
    D_HS_CHECK(rep.ops_per_kind[0] > (rep.ops_per_kind[1] * 5));
    D_HS_CHECK(hs_sum(rep.ops_per_kind) == 2000u);

    return true;
}

bool tests_schaos_run_zero_weight_treated_as_one()
{
    // a zero weight is normalised to 1 when the cumulative table is built,
    // so a zero-weight op still participates rather than being unreachable.
    dt::chaos_runner chaos;
    chaos.set_thread_count(1);
    chaos.set_iterations(1000);
    chaos.set_seed(7u);
    chaos.add_op(std::string("zero"), 0, hs_null_op());
    chaos.add_op(std::string("one"),  1, hs_null_op());

    const dt::stress_report rep = chaos.run();

    D_HS_CHECK(rep.ops_per_kind.size() == 2u);
    D_HS_CHECK(rep.ops_per_kind[0] > 0u);   // the zero-weight op was selected
    D_HS_CHECK(rep.ops_per_kind[1] > 0u);
    D_HS_CHECK(hs_sum(rep.ops_per_kind) == 1000u);

    return true;
}

bool tests_schaos_seed_determinism()
{
    // same seed + same configuration => identical per-op totals.  Each
    // thread draws from its own mt19937_64(seed ^ tid) and performs exactly
    // `iterations` draws, so scheduling cannot perturb the result.
    dt::chaos_runner first;
    first.set_thread_count(2);
    first.set_iterations(300);
    first.set_seed(0xABCDEFu);
    first.add_op(std::string("a"), 1, hs_null_op());
    first.add_op(std::string("b"), 2, hs_null_op());
    first.add_op(std::string("c"), 3, hs_null_op());

    dt::chaos_runner second;
    second.set_thread_count(2);
    second.set_iterations(300);
    second.set_seed(0xABCDEFu);
    second.add_op(std::string("a"), 1, hs_null_op());
    second.add_op(std::string("b"), 2, hs_null_op());
    second.add_op(std::string("c"), 3, hs_null_op());

    const dt::stress_report a = first.run();
    const dt::stress_report b = second.run();

    D_HS_CHECK(a.ops_per_kind.size() == b.ops_per_kind.size());
    D_HS_CHECK(a.total_ops == b.total_ops);

    for (std::size_t i = 0; i < a.ops_per_kind.size(); ++i)
    {
        D_HS_CHECK(a.ops_per_kind[i] == b.ops_per_kind[i]);
    }

    // and re-running the same object reproduces it too
    const dt::stress_report again = first.run();
    for (std::size_t i = 0; i < a.ops_per_kind.size(); ++i)
    {
        D_HS_CHECK(again.ops_per_kind[i] == a.ops_per_kind[i]);
    }

    return true;
}

bool tests_schaos_seed_changes_mix()
{
    // The seed must actually reach the draw.  Comparing a single pair of
    // seeds could coincide by chance, so several alternative seeds are
    // tried and at least one must produce a different mix.
    const std::size_t iters = 400;

    std::vector<std::size_t> baseline;

    {
        dt::chaos_runner chaos;
        chaos.set_thread_count(1);
        chaos.set_iterations(iters);
        chaos.set_seed(1u);
        chaos.add_op(std::string("a"), 1, hs_null_op());
        chaos.add_op(std::string("b"), 1, hs_null_op());
        chaos.add_op(std::string("c"), 1, hs_null_op());

        baseline = chaos.run().ops_per_kind;
    }

    D_HS_CHECK(baseline.size() == 3u);

    bool differs = false;

    for (unsigned s = 2; (s <= 6) && !differs; ++s)
    {
        dt::chaos_runner chaos;
        chaos.set_thread_count(1);
        chaos.set_iterations(iters);
        chaos.set_seed(s);
        chaos.add_op(std::string("a"), 1, hs_null_op());
        chaos.add_op(std::string("b"), 1, hs_null_op());
        chaos.add_op(std::string("c"), 1, hs_null_op());

        const std::vector<std::size_t> other = chaos.run().ops_per_kind;

        for (std::size_t i = 0; i < other.size(); ++i)
        {
            if (other[i] != baseline[i])
            {
                differs = true;
                break;
            }
        }
    }

    D_HS_CHECK(differs);

    return true;
}


NS_END  // testing
NS_END  // djinterp
