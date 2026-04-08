/******************************************************************************
* djinterp [threadsafe]                                     hazard_pointer.hpp
*
* Hazard pointer memory reclamation for lock-free data structures.
*   Provides the foundational building blocks for safe memory reclamation
* in lock-free containers.  A hazard pointer "protects" a node pointer so
* that concurrent threads know not to reclaim it.
*
* PROTOCOL:
*   1. Reader loads a shared pointer (e.g. head of a list).
*   2. Reader publishes it in a hazard record (protect).
*   3. Reader re-reads the shared pointer to confirm it hasn't changed.
*   4. If unchanged, the pointer is safe to dereference.
*   5. On scope exit, the hazard record is cleared.
*
*   Writers retire (logically delete) nodes, then periodically scan all
*   active hazard records.  A retired node is reclaimed only when no
*   hazard record protects it.
*
* TYPES:
*   hazard_record          — single atomic slot for one protected pointer
*   hazard_domain          — collection of records + retired node tracking
*   scoped_hazard          — RAII protect / clear
*   typed_hazard_ptr<T>    — type-safe scoped hazard with re-read validation
*   multi_hazard_domain    — domain with N slots per thread for multi-pointer
*                            algorithms (e.g. lock-free lists need 2 hazards)
*
* VERSIONING:
*   C++98/03:  unavailable (requires <atomic>)
*   C++11:     all custom types available
*   C++23:     thin wrappers around <hazard_pointer> when available
*
*
* path:      /inc/djinterp/sync/hazard_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_HAZARD_POINTER_
#define DJINTERP_THREADSAFE_HAZARD_POINTER_ 1

#ifndef DJINTERP_ENVIRONMENT_
    #error "hazard_pointer.hpp requires env.h to be included first"
#endif

#ifndef __cplusplus
    #error "hazard_pointer.hpp can only be used in C++ compilation mode"
#endif

// C++23 standard hazard pointers
#if D_ENV_LANG_IS_CPP23_OR_HIGHER
    #if __has_include(<hazard_pointer>)
        #include <hazard_pointer>
        #ifndef D_HAS_STD_HAZARD_POINTER
            #define D_HAS_STD_HAZARD_POINTER 1
        #endif
    #else
        #ifndef D_HAS_STD_HAZARD_POINTER
            #define D_HAS_STD_HAZARD_POINTER 0
        #endif
    #endif
#else
    #ifndef D_HAS_STD_HAZARD_POINTER
        #define D_HAS_STD_HAZARD_POINTER 0
    #endif
#endif


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <functional>
#include <vector>


NS_DJINTERP
NS_THREADSAFE

// =========================================================================
// I.   STANDARD LIBRARY DELEGATION (C++23)
// =========================================================================

#if D_HAS_STD_HAZARD_POINTER

    // delegate to <hazard_pointer>
    using hazard_pointer_domain =
        std::hazard_pointer_default_domain;

    template<typename _T>
    using hazard_pointer = std::hazard_pointer<_T>;

#else


// =========================================================================
// II.  HAZARD RECORD
// =========================================================================
// A single hazard pointer slot.  Each active thread
// claims one record from the domain and stores the
// pointer it needs to protect.
//
// The record is marked active/inactive via an atomic
// flag.  The protected pointer is read by the scanning
// routine to determine whether a retired node is safe
// to reclaim.

struct hazard_record
{
    std::atomic<void*> protected_ptr;
    std::atomic<bool>  active;

    hazard_record() noexcept
        : protected_ptr(nullptr)
        , active(false)
    {}
};


// =========================================================================
// III. HAZARD DOMAIN
// =========================================================================
// Collection of hazard records and retired-node tracking.
// One domain per instance (or shared across
// instances of the same type).
//
// The record array is allocated once at construction.
// The fast path (acquire / protect / clear / release)
// is allocation-free.

class hazard_domain
{
public:
    // max_threads
    //   the maximum number of concurrent threads.
    // Fixed at construction to avoid allocation on the
    // fast path.
    explicit hazard_domain(
        std::size_t _max_threads = 64) noexcept
        : m_records(nullptr)
        , m_capacity(_max_threads)
        , m_retired_count(0)
    {
        m_records = new (std::nothrow)
            hazard_record[_max_threads];
    }

    ~hazard_domain()
    {
        delete[] m_records;
    }

    hazard_domain(const hazard_domain&)            = delete;
    hazard_domain& operator=(const hazard_domain&) = delete;

    // --- record management ---

    // acquire
    //   claims a hazard record for the calling thread.
    // Returns a pointer to the record, or nullptr if
    // all records are in use.
    hazard_record* acquire() noexcept
    {
        if (!m_records)
        {
            return nullptr;
        }

        for (std::size_t i = 0;
             i < m_capacity; ++i)
        {
            bool expected = false;

            if (m_records[i].active
                    .compare_exchange_strong(
                        expected, true,
                        std::memory_order_acq_rel))
            {
                return &m_records[i];
            }
        }

        return nullptr;
    }

    // release
    //   returns a hazard record to the pool.
    void release(hazard_record* _rec) noexcept
    {
        if (_rec)
        {
            _rec->protected_ptr.store(
                nullptr,
                std::memory_order_release);

            _rec->active.store(
                false,
                std::memory_order_release);
        }
    }

    // --- scanning ---

    // is_protected
    //   returns true if any active hazard record
    // currently protects _ptr.
    bool is_protected(void* _ptr) const noexcept
    {
        if ( (!m_records) ||
             (!_ptr) )
        {
            return false;
        }

        for (std::size_t i = 0;
             i < m_capacity; ++i)
        {
            if ( m_records[i].active.load(
                     std::memory_order_acquire) &&
                 m_records[i].protected_ptr.load(
                     std::memory_order_acquire) ==
                         _ptr )
            {
                return true;
            }
        }

        return false;
    }

    // collect_protected
    //   gathers all currently protected pointers into
    // _out.  Used by bulk scan routines that need to
    // check many retired nodes at once (more efficient
    // than calling is_protected per node).
    void collect_protected(
        std::vector<void*>& _out) const
    {
        _out.clear();

        if (!m_records)
        {
            return;
        }

        for (std::size_t i = 0;
             i < m_capacity; ++i)
        {
            if (m_records[i].active.load(
                    std::memory_order_acquire))
            {
                void* p =
                    m_records[i].protected_ptr.load(
                        std::memory_order_acquire);

                if (p)
                {
                    _out.push_back(p);
                }
            }
        }
    }

    // --- retired count tracking ---

    std::size_t retired_count() const noexcept
    {
        return m_retired_count.load(
            std::memory_order_relaxed);
    }

    void increment_retired() noexcept
    {
        m_retired_count.fetch_add(
            1, std::memory_order_relaxed);
    }

    void decrement_retired(
        std::size_t _n = 1) noexcept
    {
        m_retired_count.fetch_sub(
            _n, std::memory_order_relaxed);
    }

    // --- queries ---

    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

    // should_scan
    //   heuristic: returns true when the retired count
    // exceeds a threshold proportional to the number
    // of hazard slots.  Typical threshold is 2× capacity.
    bool should_scan() const noexcept
    {
        return (m_retired_count.load(
            std::memory_order_relaxed) >=
                (m_capacity * 2));
    }

private:
    hazard_record*         m_records;
    std::size_t            m_capacity;
    std::atomic<std::size_t> m_retired_count;
};


// =========================================================================
// IV.  SCOPED HAZARD
// =========================================================================
// RAII hazard pointer protector.  Acquires a record from
// the domain on construction, releases on destruction.
//
// Usage:
//   scoped_hazard guard(domain);
//   Node* node = head.load();
//   guard.protect(node);
//   if (head.load() == node) {
//       // node is safe to dereference
//   }

class scoped_hazard
{
public:
    explicit scoped_hazard(
        hazard_domain& _domain) noexcept
        : m_domain(_domain)
        , m_record(_domain.acquire())
    {}

    ~scoped_hazard()
    {
        if (m_record)
        {
            m_domain.release(m_record);
        }
    }

    scoped_hazard(const scoped_hazard&)            = delete;
    scoped_hazard& operator=(const scoped_hazard&) = delete;

    // protect
    //   marks _ptr as protected.  Must be followed by
    // a re-read of the source pointer to confirm it
    // hasn't changed (standard hazard pointer protocol).
    void protect(void* _ptr) noexcept
    {
        if (m_record)
        {
            m_record->protected_ptr.store(
                _ptr,
                std::memory_order_release);
        }
    }

    // clear
    //   removes protection.
    void clear() noexcept
    {
        if (m_record)
        {
            m_record->protected_ptr.store(
                nullptr,
                std::memory_order_release);
        }
    }

    // valid
    //   true if a hazard record was successfully
    // acquired.
    bool valid() const noexcept
    {
        return (m_record != nullptr);
    }

private:
    hazard_domain& m_domain;
    hazard_record* m_record;
};


// =========================================================================
// V.   TYPED HAZARD POINTER
// =========================================================================
// Type-safe scoped hazard with built-in re-read
// validation.  Encapsulates the protect-then-verify
// loop that every hazard pointer user must perform.
//
// Usage:
//   typed_hazard_ptr<Node> hp(domain);
//   Node* safe = hp.protect_and_validate(head);
//   if (safe) {
//       // safe to dereference 'safe'
//   }

template<typename _T>
class typed_hazard_ptr
{
public:
    explicit typed_hazard_ptr(
        hazard_domain& _domain) noexcept
        : m_guard(_domain)
    {}

    typed_hazard_ptr(const typed_hazard_ptr&)            = delete;
    typed_hazard_ptr& operator=(const typed_hazard_ptr&) = delete;

    // protect_and_validate
    //   loads the pointer from _source, protects it,
    // then re-reads _source to confirm it hasn't changed.
    // Retries up to _max_retries times.
    // Returns the protected pointer, or nullptr if
    // validation fails.
    _T* protect_and_validate(
        const std::atomic<_T*>& _source,
        unsigned                _max_retries = 4) noexcept
    {
        for (unsigned i = 0;
             i < _max_retries; ++i)
        {
            _T* ptr = _source.load(
                std::memory_order_acquire);

            m_guard.protect(
                static_cast<void*>(ptr));

            // re-read and validate
            _T* reread = _source.load(
                std::memory_order_acquire);

            if (reread == ptr)
            {
                return ptr;
            }
        }

        // validation failed after all retries
        m_guard.clear();

        return nullptr;
    }

    // protect
    //   directly protects a known pointer (caller is
    // responsible for validation).
    void protect(_T* _ptr) noexcept
    {
        m_guard.protect(
            static_cast<void*>(_ptr));
    }

    // clear
    //   removes protection.
    void clear() noexcept
    {
        m_guard.clear();
    }

    // valid
    //   true if the underlying record was acquired.
    bool valid() const noexcept
    {
        return m_guard.valid();
    }

private:
    scoped_hazard m_guard;
};


// =========================================================================
// VI.  RETIRED LIST
// =========================================================================
// Per-thread list of nodes awaiting reclamation.  Each
// entry records the node pointer, the deleter function,
// and optionally the epoch at which it was retired.
//
// The scan routine checks all entries against the
// domain's active hazard records and reclaims those
// that are no longer protected.

template<typename _T>
class retired_list
{
public:
    using deleter_fn = std::function<void(_T*)>;

    struct entry
    {
        _T*           ptr;
        deleter_fn    deleter;
        std::uint64_t retire_epoch;
    };

    retired_list() = default;

    // retire
    //   adds a node to the retired list.
    void retire(
        _T*           _ptr,
        std::uint64_t _epoch = 0)
    {
        m_entries.push_back(
            { _ptr,
              [](_T* p) { delete p; },
              _epoch });
    }

    // retire (custom deleter)
    //   adds a node with a custom deleter.
    void retire(
        _T*           _ptr,
        deleter_fn    _deleter,
        std::uint64_t _epoch = 0)
    {
        m_entries.push_back(
            { _ptr,
              std::move(_deleter),
              _epoch });
    }

    // scan
    //   reclaims all entries that are not protected by
    // any active hazard record in _domain.
    // Returns the number of nodes reclaimed.
    std::size_t scan(
        const hazard_domain& _domain)
    {
        std::vector<void*> protected_ptrs;
        _domain.collect_protected(protected_ptrs);

        std::size_t reclaimed = 0;
        std::size_t wr = 0;

        for (std::size_t rd = 0;
             rd < m_entries.size(); ++rd)
        {
            void* raw = static_cast<void*>(
                m_entries[rd].ptr);

            bool is_safe = true;

            for (std::size_t j = 0;
                 j < protected_ptrs.size(); ++j)
            {
                if (protected_ptrs[j] == raw)
                {
                    is_safe = false;
                    break;
                }
            }

            if (is_safe)
            {
                m_entries[rd].deleter(
                    m_entries[rd].ptr);
                ++reclaimed;
            }
            else
            {
                if (wr != rd)
                {
                    m_entries[wr] =
                        std::move(m_entries[rd]);
                }

                ++wr;
            }
        }

        m_entries.resize(wr);

        return reclaimed;
    }

    // scan_by_epoch
    //   reclaims all entries whose retire_epoch is
    // older than _safe_epoch (regardless of hazard
    // records).  Used with epoch-based reclamation.
    // Returns the number of nodes reclaimed.
    std::size_t scan_by_epoch(
        std::uint64_t _safe_epoch)
    {
        std::size_t reclaimed = 0;
        std::size_t wr = 0;

        for (std::size_t rd = 0;
             rd < m_entries.size(); ++rd)
        {
            if (m_entries[rd].retire_epoch <
                _safe_epoch)
            {
                m_entries[rd].deleter(
                    m_entries[rd].ptr);
                ++reclaimed;
            }
            else
            {
                if (wr != rd)
                {
                    m_entries[wr] =
                        std::move(m_entries[rd]);
                }

                ++wr;
            }
        }

        m_entries.resize(wr);

        return reclaimed;
    }

    // --- queries ---

    std::size_t pending() const noexcept
    {
        return m_entries.size();
    }

    bool empty() const noexcept
    {
        return m_entries.empty();
    }

    void clear()
    {
        for (auto& e : m_entries)
        {
            e.deleter(e.ptr);
        }

        m_entries.clear();
    }

private:
    std::vector<entry> m_entries;
};


// =========================================================================
// VII. MULTI-SLOT HAZARD DOMAIN
// =========================================================================
// Variant of hazard_domain where each thread can protect
// up to _SlotsPerThread pointers simultaneously.
// Required for algorithms like lock-free linked lists
// where a thread must protect both the current and next
// node pointers during traversal.

template<std::size_t _SlotsPerThread = 2>
class multi_hazard_domain
{
public:
    static constexpr std::size_t slots_per_thread =
        _SlotsPerThread;

    explicit multi_hazard_domain(
        std::size_t _max_threads = 64) noexcept
        : m_records(nullptr)
        , m_capacity(_max_threads)
        , m_total_slots(
              _max_threads * _SlotsPerThread)
        , m_retired_count(0)
    {
        m_records = new (std::nothrow)
            hazard_record[m_total_slots];
    }

    ~multi_hazard_domain()
    {
        delete[] m_records;
    }

    multi_hazard_domain(
        const multi_hazard_domain&)            = delete;
    multi_hazard_domain& operator=(
        const multi_hazard_domain&)            = delete;

    // acquire_slot_group
    //   claims _SlotsPerThread consecutive records.
    // Returns a pointer to the first record in the
    // group, or nullptr if all groups are in use.
    // The caller can then index [0.._SlotsPerThread-1]
    // from the returned pointer.
    hazard_record* acquire_slot_group() noexcept
    {
        if (!m_records)
        {
            return nullptr;
        }

        for (std::size_t g = 0;
             g < m_capacity; ++g)
        {
            std::size_t base = g * _SlotsPerThread;

            // try to claim the first slot in the group
            bool expected = false;

            if (m_records[base].active
                    .compare_exchange_strong(
                        expected, true,
                        std::memory_order_acq_rel))
            {
                // mark remaining slots active
                for (std::size_t s = 1;
                     s < _SlotsPerThread; ++s)
                {
                    m_records[base + s].active.store(
                        true,
                        std::memory_order_release);
                }

                return &m_records[base];
            }
        }

        return nullptr;
    }

    // release_slot_group
    //   returns a group of _SlotsPerThread records.
    void release_slot_group(
        hazard_record* _group) noexcept
    {
        if (!_group)
        {
            return;
        }

        for (std::size_t s = 0;
             s < _SlotsPerThread; ++s)
        {
            _group[s].protected_ptr.store(
                nullptr,
                std::memory_order_release);

            _group[s].active.store(
                false,
                std::memory_order_release);
        }
    }

    // is_protected
    //   checks all active slots across all groups.
    bool is_protected(void* _ptr) const noexcept
    {
        if ( (!m_records) ||
             (!_ptr) )
        {
            return false;
        }

        for (std::size_t i = 0;
             i < m_total_slots; ++i)
        {
            if ( m_records[i].active.load(
                     std::memory_order_acquire) &&
                 m_records[i].protected_ptr.load(
                     std::memory_order_acquire) ==
                         _ptr )
            {
                return true;
            }
        }

        return false;
    }

    // collect_protected
    //   gathers all currently protected pointers.
    void collect_protected(
        std::vector<void*>& _out) const
    {
        _out.clear();

        if (!m_records)
        {
            return;
        }

        for (std::size_t i = 0;
             i < m_total_slots; ++i)
        {
            if (m_records[i].active.load(
                    std::memory_order_acquire))
            {
                void* p =
                    m_records[i].protected_ptr.load(
                        std::memory_order_acquire);

                if (p)
                {
                    _out.push_back(p);
                }
            }
        }
    }

    // --- retired count ---

    std::size_t retired_count() const noexcept
    {
        return m_retired_count.load(
            std::memory_order_relaxed);
    }

    void increment_retired() noexcept
    {
        m_retired_count.fetch_add(
            1, std::memory_order_relaxed);
    }

    void decrement_retired(
        std::size_t _n = 1) noexcept
    {
        m_retired_count.fetch_sub(
            _n, std::memory_order_relaxed);
    }

    // --- queries ---

    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

    std::size_t total_slots() const noexcept
    {
        return m_total_slots;
    }

    bool should_scan() const noexcept
    {
        return (m_retired_count.load(
            std::memory_order_relaxed) >=
                (m_total_slots * 2));
    }

private:
    hazard_record*           m_records;
    std::size_t              m_capacity;
    std::size_t              m_total_slots;
    std::atomic<std::size_t> m_retired_count;
};


// =========================================================================
// VIII. SCOPED MULTI-HAZARD
// =========================================================================
// RAII guard for a multi-slot hazard group.

template<std::size_t _SlotsPerThread>
class scoped_multi_hazard
{
public:
    using domain_type =
        multi_hazard_domain<_SlotsPerThread>;

    explicit scoped_multi_hazard(
        domain_type& _domain) noexcept
        : m_domain(_domain)
        , m_group(_domain.acquire_slot_group())
    {}

    ~scoped_multi_hazard()
    {
        if (m_group)
        {
            m_domain.release_slot_group(m_group);
        }
    }

    scoped_multi_hazard(
        const scoped_multi_hazard&)            = delete;
    scoped_multi_hazard& operator=(
        const scoped_multi_hazard&)            = delete;

    // protect
    //   marks _ptr as protected in slot _slot.
    void protect(
        std::size_t _slot,
        void*       _ptr) noexcept
    {
        if (m_group && _slot < _SlotsPerThread)
        {
            m_group[_slot].protected_ptr.store(
                _ptr,
                std::memory_order_release);
        }
    }

    // clear
    //   removes protection from slot _slot.
    void clear(std::size_t _slot) noexcept
    {
        if (m_group && _slot < _SlotsPerThread)
        {
            m_group[_slot].protected_ptr.store(
                nullptr,
                std::memory_order_release);
        }
    }

    // clear_all
    //   removes protection from all slots.
    void clear_all() noexcept
    {
        if (m_group)
        {
            for (std::size_t s = 0;
                 s < _SlotsPerThread; ++s)
            {
                m_group[s].protected_ptr.store(
                    nullptr,
                    std::memory_order_release);
            }
        }
    }

    bool valid() const noexcept
    {
        return (m_group != nullptr);
    }

private:
    domain_type&   m_domain;
    hazard_record* m_group;
};


#endif  // D_HAS_STD_HAZARD_POINTER


NS_END  // threadsafe
NS_END  // djinterp

#endif  // C++11


#endif  // DJINTERP_THREADSAFE_HAZARD_POINTER_
