/*******************************************************************************
* djinterp [core]                                         reclamation_common.hpp
*
* Deferred-reclamation engine shared by the sync module's safe-memory schemes.
*   hazard_pointer.hpp and rcu.hpp each maintained their OWN list of retired
* nodes - the same {pointer, deleter, tag} record, the same two-cursor
* compaction, the same retire overloads - differing only in the rule for
* deciding a node is safe to free. This header factors the list and the
* compaction out, leaving each scheme to supply just that rule as a predicate.
*
* TYPES:
*   reclaim_list<T>       - the retired-node store: retire(), reclaim_if(pred),
*                           force_reclaim(), pending(), optional scan threshold
*   reclaim_list<T>::entry- the {ptr, deleter, tag} record a predicate reads
*   published_ptr<T>      - an atomic single-writer/many-reader pointer whose
*                           publish() swaps in a new value and hands back the
*                           old one to retire
*
* HOW THE TWO SCHEMES MAP ONTO reclaim_if:
*   rcu     list.reclaim_if([&](const entry& e){ return e.tag < safe_epoch; });
*   hazard  list.reclaim_if([&](const entry& e){ return !hazarded(e.ptr);   });
*   (rcu stamps each node with the retire epoch; hazard leaves the tag 0 and
*    tests the pointer against the active hazard records instead.)
*
* OWNERSHIP NOTE: like the two engines it replaces, reclaim_list does NOT free
*   pending nodes in its destructor - it cannot know they are unreferenced.
*   The owning container reclaims on teardown via force_reclaim(), exactly as
*   rcu_protected's destructor already does.
*
* VERSIONING:
*   C++98/03:  unavailable (requires <atomic> / <functional>)
*   C++11:     all types available
*
*
* path:      /inc/djinterp/core/sync/reclamation_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RETIRED-NODE LIST
      -----------------

II.   PUBLISHED POINTER
      -----------------
*/

#ifndef DJINTERP_SYNC_RECLAMATION_COMMON_HPP
#define DJINTERP_SYNC_RECLAMATION_COMMON_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <atomic>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./sync_common.hpp"
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::uint64_t


NS_DJINTERP

// I.    Retired-node list
// The store of nodes awaiting reclamation. A writer that unlinks a node
// retire()s it here instead of freeing it, because a reader may still be
// looking at the node; later a scan frees every node the caller's predicate
// says is now unreferenced. Both the hazard-pointer domain and the RCU epoch
// reclaimer are this list plus a predicate.

template<typename Type>
class reclaim_list : private noncopyable
{
public:
    // deleter_fn
    //   alias: how a retired node is freed. Defaults to `delete`, but a
    // caller may supply a custom deleter (pooled storage, array delete).
    using deleter_fn = std::function<void(Type*)>;

    // entry
    //   the retired-node record.  `tag` is a scheme-defined stamp: RCU puts
    // the retire epoch here; hazard leaves it 0. Public so that a reclaim_if
    // predicate can read both the pointer and the tag.
    struct entry
    {
        Type*            ptr;
        deleter_fn       deleter;
        re_std::uint64_t tag;
    };

    reclaim_list() noexcept
        : m_scan_threshold(0)
    {}

    // --- retiring ---

    // retire
    //   records _ptr for deferred deletion with the default deleter.  _tag is
    // the scheme's reclamation stamp (0 when unused).
    void retire(
        Type*            _ptr,
        re_std::uint64_t _tag = 0)
    {
        entry e;
        e.ptr     = _ptr;
        e.deleter = default_deleter;
        e.tag     = _tag;

        m_entries.push_back(std::move(e));
    }

    // retire (custom deleter)
    //   records _ptr with a caller-supplied deleter.
    void retire(
        Type*            _ptr,
        deleter_fn       _deleter,
        re_std::uint64_t _tag = 0)
    {
        entry e;
        e.ptr     = _ptr;
        e.deleter = std::move(_deleter);
        e.tag     = _tag;

        m_entries.push_back(std::move(e));
    }

    // --- reclamation ---

    // reclaim_if
    //   frees every retired node for which _safe(entry) is true, running its
    // deleter, and compacts the survivors (order preserved). Returns the
    // number of nodes reclaimed. This is the entire scan - the caller's
    // predicate is the only scheme-specific part.
    template<typename SafePred>
    std::size_t reclaim_if(SafePred _safe)
    {
        std::size_t reclaimed = 0;
        std::size_t wr        = 0;

        for (std::size_t rd = 0; rd < m_entries.size(); ++rd)
        {
            const entry& probe = m_entries[rd];

            if (_safe(probe))
            {
                m_entries[rd].deleter(m_entries[rd].ptr);
                ++reclaimed;
            }
            else
            {
                if (wr != rd)
                {
                    m_entries[wr] = std::move(m_entries[rd]);
                }

                ++wr;
            }
        }

        m_entries.resize(wr);

        return reclaimed;
    }

    // force_reclaim
    //   frees ALL retired nodes unconditionally and empties the list. Only
    // safe when no reader can still reach them - i.e. during teardown.
    // Returns the number reclaimed.
    std::size_t force_reclaim()
    {
        const std::size_t count = m_entries.size();

        for (std::size_t i = 0; i < m_entries.size(); ++i)
        {
            m_entries[i].deleter(m_entries[i].ptr);
        }

        m_entries.clear();

        return count;
    }

    // --- queries ---

    // pending
    //   the number of nodes awaiting reclamation.
    std::size_t pending() const noexcept
    {
        return m_entries.size();
    }

    // --- optional auto-scan threshold ---

    // set_scan_threshold
    //   arms should_scan(): once pending() reaches _n, should_scan() returns
    // true.  0 (the default) disables it. The list never scans on its own -
    // this only tells the owner WHEN a scan is worthwhile, since only the
    // owner holds the predicate.
    void set_scan_threshold(std::size_t _n) noexcept
    {
        m_scan_threshold = _n;
    }

    // should_scan
    //   true when a threshold is armed and the pending count has reached it.
    bool should_scan() const noexcept
    {
        return (m_scan_threshold != 0) &&
               (m_entries.size() >= m_scan_threshold);
    }

private:
    // default_deleter
    //   the plain `delete` used by the one-argument retire.
    static void
    default_deleter(Type* _ptr)
    {
        delete _ptr;
    }

    std::vector<entry> m_entries;
    std::size_t        m_scan_threshold;
};


// II.   Published pointer
// A single-writer / many-reader atomic pointer. Readers load() with acquire
// ordering to see a fully-constructed value; the writer publish()es a new one
// with release ordering and gets the previous pointer back to retire(). This
// is the publish/retire half of the RCU and hazard-pointer containers, split
// out from the reclamation policy above; pair it with a reclaim_list.

template<typename Type>
class published_ptr : private noncopyable
{
public:
    published_ptr() noexcept
        : m_ptr(nullptr)
    {}

    explicit published_ptr(Type* _initial) noexcept
        : m_ptr(_initial)
    {}

    // store
    //   installs _initial WITHOUT handing back what was there. This is for
    // INITIALIZATION ONLY - a constructor that has to compute the pointee
    // before it can hand it over, and so cannot use the pointer constructor
    // above. Anywhere a value is genuinely being REPLACED, use publish():
    // store() drops the previous pointer on the floor, which for a live
    // container means leaking it (or worse, leaving it unretired while a
    // reader still holds it). Release ordering matches publish()'s, so a
    // reader that acquires the pointer sees a fully-constructed pointee.
    void store(
        Type*            _initial,
        std::memory_order _order =
            std::memory_order_release) noexcept
    {
        m_ptr.store(_initial, _order);
    }

    // load
    //   reads the current pointer. Acquire ordering pairs with publish()'s
    // release so the pointee's construction is visible to the reader.
    Type* load(
        std::memory_order _order =
            std::memory_order_acquire) const noexcept
    {
        return m_ptr.load(_order);
    }

    // publish
    //   installs _next and returns the pointer it replaced, which the caller
    // then retires. Release ordering ensures a reader that later acquires
    // _next sees its fully-constructed state.
    Type* publish(
        Type*            _next,
        std::memory_order _order =
            std::memory_order_acq_rel) noexcept
    {
        return m_ptr.exchange(_next, _order);
    }

    // compare_publish
    //   installs _next only if the current pointer is still _expected - a
    // writer-side CAS for lock-free update loops. On success the old value
    // is _expected (unchanged); on failure _expected is updated to the
    // current value. Returns whether the swap happened.
    bool compare_publish(
        Type*&           _expected,
        Type*            _next,
        std::memory_order _success =
            std::memory_order_acq_rel,
        std::memory_order _failure =
            std::memory_order_acquire) noexcept
    {
        return m_ptr.compare_exchange_strong(
            _expected, _next, _success, _failure);
    }

private:
    std::atomic<Type*> m_ptr;
};


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_SYNC_RECLAMATION_COMMON_HPP
