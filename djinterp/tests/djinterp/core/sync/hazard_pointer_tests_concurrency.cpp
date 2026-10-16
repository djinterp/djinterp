/*******************************************************************************
* djinterp [core]                           hazard_pointer_tests_concurrency.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_concurrency.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]              hazard_pointer_tests_concurrency.cpp
//   Section: CONCURRENCY - the claims that only many threads can test.
//
//   The single-threaded sections established what the operations DO; this one
// establishes that they hold up when readers and a writer actually run at once.
// The centrepiece is the safety property in its true, concurrent form
// (tests_a_protected_node_survives_a_concurrent_scan): a reader publishes
// protection for a node and keeps reading it while a writer, on another thread,
// retires that very node and scans - and the scan must not free it out from
// under the reader.  If it did, the reader's next dereference would be a
// use-after-free; here it is a data race TSan would catch and a wrong count the
// test would catch.
//
//   The tests are deterministic, not probabilistic:
//     - threads meet at a rendezvous (an atomic "go" flag every thread spins
// on)
//       so the reader and the writer genuinely overlap, rather than lining up
// by
//       luck;
//     - a reader that must be seen protecting BEFORE the writer scans announces
//       that with its own flag, and the writer waits for it;
//     - every thread is joined, and the leak-accounting test asserts the live
//       node count is zero at the end;
//     - spin-waits SLEEP between polls, so on a single-core box a spinning
//       thread cannot starve the one it is waiting for.
//   A failure here is a wrong count, a leak, or a TSan race - never a hang.
//
//   Gated on D_HAS_CUSTOM_HAZARD throughout.

// djinterp
#include "hazard_pointer_tests.hpp"


NS_DJINTERP
NS_TESTING

#if D_HAS_CUSTOM_HAZARD

namespace
{
    // k_acquire_agents
    //   constant: how many threads pile into acquire() at once.
    D_STATIC D_CONSTEXPR int k_acquire_agents = 8;

    // acquirer
    //   function: spin to the start line, claim a record, record which one, and
    // hold it.  If every thread gets a DISTINCT non-null record, the CAS in
    // acquire() is doing its job under contention.
    void
    acquirer(
        hazard_domain*           _domain,
        const std::atomic<bool>* _go,
        std::atomic<int>*        _ready,
        std::atomic<hazard_record*>* _slot
    )
    {
        _ready->fetch_add(1);

        while (!_go->load())
        {
            std::this_thread::yield();
        }

        _slot->store(_domain->acquire());

        return;
    }

    // group_acquirer
    //   function: the same, for multi_hazard_domain's group claim.
    void
    group_acquirer(
        multi_hazard_domain<2>*  _domain,
        const std::atomic<bool>* _go,
        std::atomic<int>*        _ready,
        std::atomic<hazard_record*>* _slot
    )
    {
        _ready->fetch_add(1);

        while (!_go->load())
        {
            std::this_thread::yield();
        }

        _slot->store(_domain->acquire_slot_group());

        return;
    }
}

#endif  // D_HAS_CUSTOM_HAZARD

/*
tests_concurrent_acquire_hands_out_distinct_records
  many threads claiming records at once each get a different one.
  Tests the following:
  - k_acquire_agents threads, released together, each call acquire() once on a
    domain with exactly that many records
  - every thread gets a non-null record, and no two get the SAME record
  - so the CAS on the active flag genuinely serializes the claims: two threads
    cannot both win the same slot
  A non-atomic claim would let two threads read "inactive" and both take a slot;
  the distinctness check is what would catch that.
*/
bool
tests_concurrent_acquire_hands_out_distinct_records()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(k_acquire_agents);

    std::atomic<bool>            go(false);
    std::atomic<int>             ready(0);
    std::atomic<hazard_record*>  slots[k_acquire_agents];
    std::vector<std::thread>     threads;
    int                          i = 0;

    for (i = 0; i < k_acquire_agents; ++i)
    {
        slots[i].store(nullptr);
    }

    for (i = 0; i < k_acquire_agents; ++i)
    {
        threads.push_back(std::thread(&acquirer,
                                      &domain,
                                      &go,
                                      &ready,
                                      &slots[i]));
    }

    const bool all_ready =
        spin_until_count(ready, k_acquire_agents, k_deadline_ms);

    go.store(true);

    for (i = 0; i < static_cast<int>(threads.size()); ++i)
    {
        threads[static_cast<std::size_t>(i)].join();
    }

    // every thread got a non-null record, and all are distinct
    bool all_non_null = true;
    bool all_distinct = true;
    int  a = 0;
    int  b = 0;

    for (a = 0; a < k_acquire_agents; ++a)
    {
        if (slots[a].load() == nullptr)
        {
            all_non_null = false;
        }

        for (b = a + 1; b < k_acquire_agents; ++b)
        {
            if (slots[a].load() == slots[b].load())
            {
                all_distinct = false;
            }
        }
    }

    return ( all_ready     &&
             all_non_null  &&
             all_distinct );
#else
    return true;
#endif
}

/*
tests_a_protected_node_survives_a_concurrent_scan
  THE SAFETY PROPERTY, concurrently.  A reader protecting a node keeps it alive
  across a writer's scan on another thread.
  Tests the following:
  - a reader thread publishes protection for a node and signals that it has done
    so, then waits
  - the main thread (the writer) waits for that signal, retires the node, and
    scans
  - the scan reclaims ZERO nodes: the node is protected, so it is spared
  - the node is still live; the reader is then released, clears its protection,
    and a final scan reclaims it
  - nothing leaks
  This is the single-threaded spare test lifted into two real threads, with the
  publish happening-before the scan by construction.  It is the property the
  whole header exists to provide, and the reader's held reference to the node is
  only safe because the scan honoured it.
*/
bool
tests_a_protected_node_survives_a_concurrent_scan()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    hazard_domain              domain(4);
    retired_list<counted_node> list;

    counted_node* node = new counted_node(42);

    std::atomic<bool> protected_flag(false);
    std::atomic<bool> release_reader(false);

    hazard_record* record = domain.acquire();

    std::thread reader(
        [record, node, &protected_flag, &release_reader]()
        {
            // publish protection, then announce it
            record->protected_ptr.store(node);
            protected_flag.store(true);

            // hold the protection until told to let go
            while (!release_reader.load())
            {
                std::this_thread::yield();
            }

            record->protected_ptr.store(nullptr);
        });

    // the writer waits until the reader is definitely protecting
    const bool reader_is_protecting = spin_until(protected_flag, k_deadline_ms);

    // retire the node and scan while the reader holds it
    list.retire(node);

    const std::size_t reclaimed_while_protected = list.scan(domain);

    const bool spared =
        ( (reclaimed_while_protected == 0) &&
          (probe_live().load() == 1)       &&
          (list.pending() == 1) );

    // release the reader, let it clear, then reclaim
    release_reader.store(true);
    reader.join();

    const std::size_t reclaimed_after = list.scan(domain);

    domain.release(record);

    return ( reader_is_protecting          &&
             spared                        &&
             (reclaimed_after == 1)        &&
             (probe_live().load() == 0)    &&
             (probe_deletions().load() == 1) );
#else
    return true;
#endif
}

/*
tests_validation_under_a_churning_source_is_sound
  protect_and_validate never hands back a bad pointer, however hard the source
  churns.
  Tests the following:
  - a writer thread flips an atomic source between two live nodes as fast as it
    can
  - meanwhile the main thread calls protect_and_validate thousands of times
  - every non-null pointer it returns is one of the two real nodes - never a
    torn or bogus value
  - and whenever it returns a node, that node was the re-read-confirmed value,
    so the protocol's guarantee held under contention
  The source never frees either node (this is a test of the validation loop, not
  of reclamation), so a returned pointer only has to be VALID, not protected
  against a concurrent delete.  What is checked is that the loop's load /
  publish
  / re-read logic is race-free: it either converges on a stable pointer or gives
  up cleanly.
*/
bool
tests_validation_under_a_churning_source_is_sound()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(4);

    int node_a = 1;
    int node_b = 2;

    std::atomic<int*>  source(&node_a);
    std::atomic<bool>  stop(false);
    std::atomic<bool>  writer_ready(false);

    std::thread writer(
        [&source, &node_a, &node_b, &stop, &writer_ready]()
        {
            int* values[2] = { &node_a, &node_b };
            int  i = 0;

            writer_ready.store(true);

            while (!stop.load())
            {
                source.store(values[i & 1]);
                ++i;
            }
        });

    const bool writer_up = spin_until(writer_ready, k_deadline_ms);

    int  bad_returns = 0;
    int  k = 0;

    for (k = 0; k < 5000; ++k)
    {
        typed_hazard_ptr<int> hazard(domain);

        int* got = hazard.protect_and_validate(source, 8);

        // a returned pointer must be one of the two live nodes
        if ( (got != nullptr) &&
             (got != &node_a)  &&
             (got != &node_b) )
        {
            ++bad_returns;
        }

        hazard.clear();
    }

    stop.store(true);
    writer.join();

    return ( writer_up &&
             (bad_returns == 0) );
#else
    return true;
#endif
}

/*
tests_concurrent_protect_and_scan_leak_nothing
  a full mixed workload comes back to zero live nodes.
  Tests the following:
  - several reader threads repeatedly acquire a record, protect a node from a
    shared pool, briefly hold it, then release - while the main thread retires
    nodes and scans
  - every node retired is eventually reclaimed: after all threads join and a
    final clear, the live node count is zero
  - no node is double-freed (the deletion count equals the number of nodes)
  This is the accounting a hazard scheme exists to keep, run end to end under
  contention: a node is freed exactly once, and never while a reader protects
  it.  The readers protect by ADDRESS from a fixed set, so a spared node stays
  valid for the reader that spared it.
*/
bool
tests_concurrent_protect_and_scan_leak_nothing()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    const int     node_count = 32;
    hazard_domain domain(k_agents + 1);

    // a fixed pool of nodes, retired over the course of the test
    std::vector<counted_node*> nodes;
    int                        i = 0;

    for (i = 0; i < node_count; ++i)
    {
        nodes.push_back(new counted_node(i));
    }

    std::atomic<bool> go(false);
    std::atomic<int>  ready(0);
    std::atomic<int>  next_index(0);

    // readers: repeatedly protect a node by address, hold briefly, release
    std::vector<std::thread> readers;
    int                      t = 0;

    for (t = 0; t < k_agents; ++t)
    {
        readers.push_back(std::thread(
            [&domain, &nodes, &go, &ready, node_count]()
            {
                ready.fetch_add(1);

                while (!go.load())
                {
                    std::this_thread::yield();
                }

                int r = 0;

                for (r = 0; r < 200; ++r)
                {
                    scoped_hazard guard(domain);

                    if (guard.valid())
                    {
                        // protect an arbitrary node from the pool by address
                        counted_node* target =
                            nodes[static_cast<std::size_t>(r % node_count)];

                        guard.protect(target);

                        // a brief hold, then the guard releases at scope exit
                    }
                }
            }));
    }

    const bool all_ready = spin_until_count(ready, k_agents, k_deadline_ms);

    go.store(true);

    // the writer retires every node and scans repeatedly.  Because a node may
    // be
    // protected at the moment of a scan, retire-and-scan is looped until the
    // list drains after the readers have finished.
    retired_list<counted_node> list;

    for (i = 0; i < node_count; ++i)
    {
        list.retire(nodes[static_cast<std::size_t>(i)]);
        (void)list.scan(domain);
    }

    for (t = 0; t < static_cast<int>(readers.size()); ++t)
    {
        readers[static_cast<std::size_t>(t)].join();
    }

    // the readers are done, so nothing is protected now: a final scan must
    // reclaim everything still pending
    (void)list.scan(domain);

    const bool nothing_leaked =
        ( list.empty()                                   &&
          (probe_live().load() == 0)                     &&
          (probe_deletions().load() == node_count) );

    return (all_ready && nothing_leaked);
#else
    return true;
#endif
}

/*
tests_group_acquisition_is_atomic_under_contention
  many threads claiming groups at once each get a distinct group.
  Tests the following:
  - k_agents threads, released together, each call acquire_slot_group() once on
    a
    domain with exactly that many groups
  - every thread gets a non-null group, and no two groups overlap: their first
    records are all distinct AND at least _SlotsPerThread apart
  - so the group claim's CAS-the-first-slot protocol serializes correctly under
    contention: two threads cannot both stake the same group
*/
bool
tests_group_acquisition_is_atomic_under_contention()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(k_agents);

    std::atomic<bool>            go(false);
    std::atomic<int>             ready(0);
    std::atomic<hazard_record*>  slots[k_agents];
    std::vector<std::thread>     threads;
    int                          i = 0;

    for (i = 0; i < k_agents; ++i)
    {
        slots[i].store(nullptr);
    }

    for (i = 0; i < k_agents; ++i)
    {
        threads.push_back(std::thread(&group_acquirer,
                                      &domain,
                                      &go,
                                      &ready,
                                      &slots[i]));
    }

    const bool all_ready = spin_until_count(ready, k_agents, k_deadline_ms);

    go.store(true);

    for (i = 0; i < static_cast<int>(threads.size()); ++i)
    {
        threads[static_cast<std::size_t>(i)].join();
    }

    bool all_non_null = true;
    bool all_disjoint = true;
    int  a = 0;
    int  b = 0;

    for (a = 0; a < k_agents; ++a)
    {
        if (slots[a].load() == nullptr)
        {
            all_non_null = false;
        }

        for (b = a + 1; b < k_agents; ++b)
        {
            hazard_record* ga = slots[a].load();
            hazard_record* gb = slots[b].load();

            // two valid groups must not be the same, and must not overlap:
            // their bases differ by at least _SlotsPerThread records
            if ( (ga != nullptr) &&
                 (gb != nullptr) )
            {
                const long distance =
                    static_cast<long>(ga > gb ? (ga - gb) : (gb - ga));

                if (distance < 2)
                {
                    all_disjoint = false;
                }
            }
        }
    }

    return ( all_ready     &&
             all_non_null  &&
             all_disjoint );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
