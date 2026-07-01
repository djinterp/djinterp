/******************************************************************************
* djinterp [event]                                             event_table.hpp
*
* The erased store:
*   Type-erased handler storage for the event system -- the realization of
* erasure. An injection kappa : T_e -> K presents each event type as a
* std::size_t key; the table maps keys to insertion-ordered words of
* type-erased handler entries. It provides insert, remove, enable/disable
* (the mask), lookup, iteration, statistics, pointwise merge, and clearing.
*
*   This is the C++ successor to the C event_table.h / event_table_common.h
* hash table, replacing the hand-rolled chaining hash table with a
* std::unordered_map backed by std::vector entry lists. The insertion-order
* guarantee within each event bucket is preserved (priority is word order).
*
*   This header absorbs the former event_table_traits.hpp and
* event_table_concepts.hpp.
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   erasure   kappa : T_e -> K        -- internal::type_key<_Event>()
*   key set   K                       -- std::size_t
*   word per type, rho_e              -- a bucket (vector of handler_entry)
*   mask      m : L -> {on,off}       -- handler_entry::enabled
*   bind / unbind                     -- insert / remove
*   enable / disable                  -- enable / disable (flip the mask)
*   merge     rho (+) rho'            -- merge (pointwise concatenation;
*                                        identity: the empty table)
*   fibre condition                   -- each erased entry touches the payload
*                                        only as its own type, because bind
*                                        and dispatch key by the same kappa.
*
* COMPONENTS:
*   djinterp::event_table_stats  - table statistics snapshot
*   djinterp::event_table        - type-erased handler storage
*   djinterp::event_table_traits<_Table>   - structural detection
*   djinterp::is_event_table_type          (C++20 concept)
*
* INTERNAL COMPONENTS:
*   djinterp::internal::type_key<_Event>  - per-type unique key (kappa)
*   djinterp::internal::handler_entry     - type-erased handler slot
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES  - move semantics
*   D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES    - using aliases
*   D_ENV_CPP_FEATURE_LANG_CONCEPTS           - concept constraints (C++20)
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
* 
* path:      /inc/djinterp/core/event/event_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_TABLE_
#define DJINTERP_EVENT_TABLE_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "event_table.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "event_table.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "event_table.hpp requires C++11 or higher"
#endif

// std
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <vector>
#include <unordered_map>
// djinterp
#include "./event_handler.hpp"


NS_DJINTERP


// =========================================================================
// I.   INTERNAL STORAGE TYPES
// =========================================================================

NS_INTERNAL

    // type_key
    //   function: the erasure injection kappa : T_e -> K. Returns a unique
    // identifier for each event type, usable as a key for the type-erased
    // handler map. Each instantiation of this function template has its own
    // static variable whose address serves as a globally unique per-type
    // token. The injection's only obligation (soundness of erasure) is that
    // an erased payload is touched only by code selected by its own key --
    // which holds because bind and dispatch both key by this same kappa.
    template<typename _Event>
    std::size_t 
    type_key()
    {
        static const char anchor = '\0';

        return reinterpret_cast<std::size_t>(&anchor);
    };

    // handler_entry
    //   struct: type-erased handler slot with mask support. The `invoke`
    // callable expects a void* pointing to the event's payload tuple
    // (the std::tuple of payload value domains) constructed by the dispatch
    // site in the registry, and returns the handler's verdict.
    struct handler_entry
    {
        handler_id                       id;
        std::function<verdict(void*)>    invoke;
        bool                             enabled;
    };

NS_END  // internal


// =========================================================================
// II.  TABLE STATISTICS
// =========================================================================

// event_table_stats
//   struct: snapshot of table metrics. Mirrors the C d_event_hash_stats
// interface adapted for the std::unordered_map backing store.
struct event_table_stats
{
    std::size_t total_buckets;
    std::size_t used_buckets;
    std::size_t total_entries;
    std::size_t enabled_entries;
    std::size_t event_type_count;
    std::size_t max_entries_per_type;
    double      average_entries_per_type;
    double      load_factor;
};


// =========================================================================
// III. EVENT TABLE
// =========================================================================

// event_table
//   class: type-erased handler storage. Maps event type keys (std::size_t)
// to insertion-ordered vectors of handler entries. Provides the raw storage
// operations that the registry builds its typed API upon.
class event_table
{
private:
    using entry_type = internal::handler_entry;
    using entry_list = std::vector<entry_type>;
    using table_type = std::unordered_map<std::size_t, entry_list>;

public:
    event_table()
        : m_next_id(1)
        , m_total_count(0)
        , m_enabled_count(0)
    {
    };

    // ---- insertion ----

    // insert
    //   inserts a handler entry into the bucket identified by _type_key.
    // The entry is appended to the end of the bucket, preserving insertion
    // order (bind appends a named letter to the word).
    // returns: the handler_id assigned to the new entry.
    handler_id insert(std::size_t                   _type_key,
                      std::function<verdict(void*)> _invoke)
    {
        handler_id hid;
        hid.value = m_next_id++;

        entry_type entry;
        entry.id      = hid;
        entry.invoke  = std::move(_invoke);
        entry.enabled = true;

        m_table[_type_key].push_back(std::move(entry));
        ++m_total_count;
        ++m_enabled_count;

        return hid;
    };

    // ---- removal ----

    // remove
    //   removes the handler identified by _id from any bucket (unbind
    // deletes the named letter).
    // returns: true if the handler was found and removed.
    bool remove(handler_id _id)
    {
        for (auto& kv : m_table)
        {
            auto& entries = kv.second;

            for (auto it = entries.begin();
                 it != entries.end();
                 ++it)
            {
                if (it->id == _id)
                {
                    if (it->enabled)
                    {
                        --m_enabled_count;
                    }

                    entries.erase(it);
                    --m_total_count;

                    return true;
                }
            }
        }

        return false;
    };

    // ---- enable / disable (the mask) ----

    // enable
    //   enables the handler identified by _id (flips its mask to on).
    // returns: true if the handler was found and enabled (was previously
    // disabled).
    bool enable(handler_id _id)
    {
        entry_type* entry = find_entry(_id);

        if ( (!entry) ||
             (entry->enabled) )
        {
            return false;
        }

        entry->enabled = true;
        ++m_enabled_count;

        return true;
    };

    // disable
    //   disables the handler identified by _id (flips its mask to off).
    // Disabled handlers remain stored but act as skip during dispatch.
    // returns: true if the handler was found and disabled (was previously
    // enabled).
    bool disable(handler_id _id)
    {
        entry_type* entry = find_entry(_id);

        if ( (!entry) ||
             (!entry->enabled) )
        {
            return false;
        }

        entry->enabled = false;
        --m_enabled_count;

        return true;
    };

    // ---- lookup ----

    // is_enabled
    //   queries whether the handler identified by _id is currently enabled.
    // returns: true if found and enabled, false otherwise.
    bool is_enabled(handler_id _id) const
    {
        const entry_type* entry = find_entry_const(_id);

        if (!entry)
        {
            return false;
        }

        return entry->enabled;
    };

    // contains
    //   queries whether a handler with the given _id exists in any bucket.
    bool contains(handler_id _id) const
    {
        return (find_entry_const(_id) != nullptr);
    };

    // ---- bucket access ----

    // entries_for
    //   returns a pointer to the entry list for the given type key, or
    // nullptr if no entries exist for that key.
    const entry_list* entries_for(std::size_t _type_key) const
    {
        auto it = m_table.find(_type_key);

        if (it == m_table.end())
        {
            return nullptr;
        }

        return &(it->second);
    };

    // entries_for (mutable)
    //   returns a pointer to the mutable entry list for the given type key,
    // or nullptr if no entries exist for that key.
    entry_list* entries_for(std::size_t _type_key)
    {
        auto it = m_table.find(_type_key);

        if (it == m_table.end())
        {
            return nullptr;
        }

        return &(it->second);
    };

    // has_entries_for
    //   returns true if at least one entry exists for the given type key.
    bool has_entries_for(std::size_t _type_key) const
    {
        auto it = m_table.find(_type_key);

        return ( (it != m_table.end()) &&
                 (!it->second.empty()) );
    };

    // count_for
    //   returns the number of entries for the given type key.
    std::size_t count_for(std::size_t _type_key) const
    {
        auto it = m_table.find(_type_key);

        if (it == m_table.end())
        {
            return 0;
        }

        return it->second.size();
    };

    // ---- iteration ----

    // for_each_entry
    //   invokes _fn for every entry in every bucket. The callable receives
    // (std::size_t type_key, const handler_entry& entry).
    template<typename _Callable>
    void for_each_entry(_Callable&& _fn) const
    {
        for (const auto& kv : m_table)
        {
            for (const auto& entry : kv.second)
            {
                _fn(kv.first, entry);
            }
        }
    };

    // for_each_entry_for
    //   invokes _fn for every entry in the bucket identified by _type_key.
    // The callable receives (const handler_entry& entry).
    template<typename _Callable>
    void for_each_entry_for(std::size_t  _type_key,
                            _Callable&&  _fn) const
    {
        auto it = m_table.find(_type_key);

        if (it != m_table.end())
        {
            for (const auto& entry : it->second)
            {
                _fn(entry);
            }
        }
    };

    // type_key_count
    //   returns the number of distinct event type keys in the table.
    std::size_t type_key_count() const
    {
        return m_table.size();
    };

    // ---- aggregate counts ----

    // total_count
    //   returns the total number of entries across all buckets.
    std::size_t total_count() const
    {
        return m_total_count;
    };

    // enabled_count
    //   returns the number of enabled entries across all buckets.
    std::size_t enabled_count() const
    {
        return m_enabled_count;
    };

    // ---- merge (pointwise concatenation) ----

    // merge
    //   appends every entry of _other into this table, key by key,
    // assigning fresh handler ids from this table's id space. Per-key order
    // is preserved with this table's existing entries first, realizing the
    // pointwise concatenation rho (+) rho' of the registry monoid (identity:
    // the empty table). Handler ids from _other are NOT preserved.
    // returns: the number of entries merged in.
    std::size_t merge(const event_table& _other)
    {
        std::size_t merged = 0;

        for (const auto& kv : _other.m_table)
        {
            auto& dst = m_table[kv.first];

            for (const auto& src : kv.second)
            {
                bool was_enabled = src.enabled;

                entry_type entry;
                entry.id.value = m_next_id++;
                entry.invoke   = src.invoke;   // std::function is copyable
                entry.enabled  = was_enabled;

                dst.push_back(std::move(entry));

                ++m_total_count;

                if (was_enabled)
                {
                    ++m_enabled_count;
                }

                ++merged;
            }
        }

        return merged;
    };

    // ---- statistics ----

    // get_stats
    //   computes and returns a snapshot of table metrics.
    event_table_stats get_stats() const
    {
        event_table_stats stats;
        stats.total_buckets        = m_table.bucket_count();
        stats.used_buckets         = 0;
        stats.total_entries        = m_total_count;
        stats.enabled_entries      = m_enabled_count;
        stats.event_type_count     = m_table.size();
        stats.max_entries_per_type = 0;

        // compute used buckets (unordered_map buckets with >=1 map entry,
        // not to be confused with our event type buckets)
        for (std::size_t i = 0; i < m_table.bucket_count(); ++i)
        {
            if (m_table.bucket_size(i) > 0)
            {
                ++stats.used_buckets;
            }
        }

        // compute max entries per event type
        for (const auto& kv : m_table)
        {
            std::size_t sz = kv.second.size();

            if (sz > stats.max_entries_per_type)
            {
                stats.max_entries_per_type = sz;
            }
        }

        // average entries per event type
        if (m_table.empty())
        {
            stats.average_entries_per_type = 0.0;
        }
        else
        {
            stats.average_entries_per_type =
                static_cast<double>(m_total_count) /
                static_cast<double>(m_table.size());
        }

        stats.load_factor = m_table.load_factor();

        return stats;
    };

    // ---- clear ----

    // clear
    //   removes all entries from all buckets.
    void clear()
    {
        m_table.clear();
        m_total_count   = 0;
        m_enabled_count = 0;
    };

    // clear_for
    //   removes all entries for the given type key.
    void clear_for(std::size_t _type_key)
    {
        auto it = m_table.find(_type_key);

        if (it != m_table.end())
        {
            // adjust counts before erasing
            for (const auto& entry : it->second)
            {
                if (entry.enabled)
                {
                    --m_enabled_count;
                }

                --m_total_count;
            }

            m_table.erase(it);
        }
    };

private:
    // find_entry
    //   locates a handler by id across all buckets.
    entry_type* find_entry(handler_id _id)
    {
        for (auto& kv : m_table)
        {
            for (auto& entry : kv.second)
            {
                if (entry.id == _id)
                {
                    return &entry;
                }
            }
        }

        return nullptr;
    };

    // find_entry_const
    //   const overload; locates a handler by id across all buckets.
    const entry_type* find_entry_const(handler_id _id) const
    {
        for (const auto& kv : m_table)
        {
            for (const auto& entry : kv.second)
            {
                if (entry.id == _id)
                {
                    return &entry;
                }
            }
        }

        return nullptr;
    };

    table_type    m_table;
    std::uint64_t m_next_id;
    std::size_t   m_total_count;
    std::size_t   m_enabled_count;
};


// =========================================================================
// IV.  STRUCTURAL DETECTION HELPERS
// =========================================================================

NS_INTERNAL

    // ---- core mutation ops ----

    // has_table_insert
    //   trait: detects insert(size_t, std::function<verdict(void*)>)
    // returning handler_id.
    template<typename _Table,
             typename = void>
    struct has_table_insert
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_insert<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Table&>().insert(
                    std::declval<std::size_t>(),
                    std::declval<std::function<verdict(void*)>>())),
                handler_id
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_remove
    //   trait: detects remove(handler_id) returning bool.
    template<typename _Table,
             typename = void>
    struct has_table_remove
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_remove<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Table&>().remove(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_enable
    //   trait: detects enable(handler_id) returning bool.
    template<typename _Table,
             typename = void>
    struct has_table_enable
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_enable<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Table&>().enable(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_disable
    //   trait: detects disable(handler_id) returning bool.
    template<typename _Table,
             typename = void>
    struct has_table_disable
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_disable<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Table&>().disable(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // ---- const query ops ----

    // has_table_is_enabled
    //   trait: detects is_enabled(handler_id) const returning bool.
    template<typename _Table,
             typename = void>
    struct has_table_is_enabled
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_is_enabled<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Table&>().is_enabled(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_contains
    //   trait: detects contains(handler_id) const returning bool.
    template<typename _Table,
             typename = void>
    struct has_table_contains
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_contains<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Table&>().contains(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_count_for
    //   trait: detects count_for(size_t) const returning size_t.
    template<typename _Table,
             typename = void>
    struct has_table_count_for
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_count_for<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Table&>().count_for(
                    std::declval<std::size_t>())),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_has_entries_for
    //   trait: detects has_entries_for(size_t) const returning bool.
    template<typename _Table,
             typename = void>
    struct has_table_has_entries_for
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_has_entries_for<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Table&>().has_entries_for(
                    std::declval<std::size_t>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_total_count
    //   trait: detects total_count() const returning size_t.
    template<typename _Table,
             typename = void>
    struct has_table_total_count
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_total_count<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Table&>().total_count()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_enabled_count
    //   trait: detects enabled_count() const returning size_t.
    template<typename _Table,
             typename = void>
    struct has_table_enabled_count
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_enabled_count<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Table&>().enabled_count()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_clear
    //   trait: detects clear() as a well-formed expression.
    template<typename _Table,
             typename = void>
    struct has_table_clear
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_clear<_Table,
        decltype(static_cast<void>(
            std::declval<_Table&>().clear()
        ))>
    {
        static constexpr bool value = true;
    };

    // ---- optional / extended ops ----

    // has_table_type_key_count
    //   trait: detects type_key_count() const returning size_t.
    template<typename _Table,
             typename = void>
    struct has_table_type_key_count
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_type_key_count<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Table&>().type_key_count()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_get_stats
    //   trait: detects get_stats() const as a well-formed expression.
    template<typename _Table,
             typename = void>
    struct has_table_get_stats
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_get_stats<_Table,
        decltype(static_cast<void>(
            std::declval<const _Table&>().get_stats()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_table_clear_for
    //   trait: detects clear_for(size_t) as a well-formed expression.
    template<typename _Table,
             typename = void>
    struct has_table_clear_for
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_clear_for<_Table,
        decltype(static_cast<void>(
            std::declval<_Table&>().clear_for(
                std::declval<std::size_t>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_table_merge
    //   trait: detects merge(const _Table&) returning size_t.
    template<typename _Table,
             typename = void>
    struct has_table_merge
    {
        static constexpr bool value = false;
    };

    template<typename _Table>
    struct has_table_merge<_Table,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Table&>().merge(
                    std::declval<const _Table&>())),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

NS_END  // internal


// =========================================================================
// V.   EVENT TABLE TRAITS
// =========================================================================

// event_table_traits
//   trait: compile-time structural detection for types that satisfy the
// event table interface required by the registry. Validates that _Table
// provides insert, remove, enable, disable, query, and clear operations
// with the expected signatures.
//
// note: does not verify semantic contracts (e.g. that remove actually
// removes, or that enable/disable are idempotent). This is a structural
// check only, consistent with the djinterp trait philosophy.
template<typename _Table>
struct event_table_traits
{
    // ---- core mutation detection ----

    // has_insert
    //   constant: true if insert(size_t, function) --> handler_id.
    static constexpr bool has_insert =
        internal::has_table_insert<clean_t<_Table>>::value;

    // has_remove
    //   constant: true if remove(handler_id) --> bool.
    static constexpr bool has_remove =
        internal::has_table_remove<clean_t<_Table>>::value;

    // has_enable
    //   constant: true if enable(handler_id) --> bool.
    static constexpr bool has_enable =
        internal::has_table_enable<clean_t<_Table>>::value;

    // has_disable
    //   constant: true if disable(handler_id) --> bool.
    static constexpr bool has_disable =
        internal::has_table_disable<clean_t<_Table>>::value;

    // ---- const query detection ----

    // has_is_enabled
    //   constant: true if is_enabled(handler_id) const --> bool.
    static constexpr bool has_is_enabled =
        internal::has_table_is_enabled<clean_t<_Table>>::value;

    // has_contains
    //   constant: true if contains(handler_id) const --> bool.
    static constexpr bool has_contains =
        internal::has_table_contains<clean_t<_Table>>::value;

    // has_count_for
    //   constant: true if count_for(size_t) const --> size_t.
    static constexpr bool has_count_for =
        internal::has_table_count_for<clean_t<_Table>>::value;

    // has_has_entries_for
    //   constant: true if has_entries_for(size_t) const --> bool.
    static constexpr bool has_has_entries_for =
        internal::has_table_has_entries_for<clean_t<_Table>>::value;

    // has_total_count
    //   constant: true if total_count() const --> size_t.
    static constexpr bool has_total_count =
        internal::has_table_total_count<clean_t<_Table>>::value;

    // has_enabled_count
    //   constant: true if enabled_count() const --> size_t.
    static constexpr bool has_enabled_count =
        internal::has_table_enabled_count<clean_t<_Table>>::value;

    // has_clear
    //   constant: true if clear() is well-formed.
    static constexpr bool has_clear =
        internal::has_table_clear<clean_t<_Table>>::value;

    // ---- composite detection ----

    // is_event_table
    //   constant: true if _Table provides all required operations for use
    // as a handler storage backend.
    static constexpr bool is_event_table =
        ( has_insert          &&
          has_remove          &&
          has_enable          &&
          has_disable         &&
          has_is_enabled      &&
          has_contains        &&
          has_count_for       &&
          has_has_entries_for &&
          has_total_count     &&
          has_enabled_count   &&
          has_clear );

    // ---- optional feature detection ----

    // has_type_key_count
    //   constant: true if type_key_count() const --> size_t.
    static constexpr bool has_type_key_count =
        internal::has_table_type_key_count<clean_t<_Table>>::value;

    // has_stats
    //   constant: true if get_stats() const is well-formed.
    static constexpr bool has_stats =
        internal::has_table_get_stats<clean_t<_Table>>::value;

    // has_clear_for
    //   constant: true if clear_for(size_t) is well-formed.
    static constexpr bool has_clear_for =
        internal::has_table_clear_for<clean_t<_Table>>::value;

    // has_merge
    //   constant: true if merge(const _Table&) --> size_t.
    static constexpr bool has_merge =
        internal::has_table_merge<clean_t<_Table>>::value;
};


// =========================================================================
// VI.  CONCEPT CONSTRAINTS (C++20+)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ---- core table concepts ----

// is_event_table_type
//   concept: constrains types that satisfy the event table structural
// requirements for use as a handler storage backend.
template<typename _Table>
concept is_event_table_type =
    event_table_traits<clean_t<_Table>>::is_event_table;

// event_table_type
//   concept: readable spelling of is_event_table_type.
template<typename _Type>
concept event_table_type =
    is_event_table_type<clean_t<_Type>>;

// non_event_table_type
//   concept: constrains types that do not satisfy the structural event
// table protocol.
template<typename _Type>
concept non_event_table_type =
    !event_table_type<_Type>;

// clearable_event_table_type
//   concept: constrains event tables supporting clear().
template<typename _Type>
concept clearable_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_clear;


// ---- count and query concepts ----

// counting_event_table_type
//   concept: constrains event tables supporting the full required count
// and query interface.
template<typename _Type>
concept counting_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_count_for &&
    event_table_traits<clean_t<_Type>>::has_has_entries_for &&
    event_table_traits<clean_t<_Type>>::has_total_count &&
    event_table_traits<clean_t<_Type>>::has_enabled_count;

// type_key_counting_event_table_type
//   concept: constrains event tables exposing type_key_count().
template<typename _Type>
concept type_key_counting_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_type_key_count;


// ---- extended feature concepts ----

// stats_event_table_type
//   concept: constrains event tables exposing get_stats().
template<typename _Type>
concept stats_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_stats;

// selectively_clearable_event_table_type
//   concept: constrains event tables supporting clear_for(type-key).
template<typename _Type>
concept selectively_clearable_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_clear_for;

// mergeable_event_table_type
//   concept: constrains event tables supporting pointwise merge.
template<typename _Type>
concept mergeable_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_merge;

// extended_event_table_type
//   concept: constrains event tables exposing all optional extension points
// currently tracked by event_table_traits.
template<typename _Type>
concept extended_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_type_key_count &&
    event_table_traits<clean_t<_Type>>::has_stats &&
    event_table_traits<clean_t<_Type>>::has_clear_for &&
    event_table_traits<clean_t<_Type>>::has_merge;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EVENT_TABLE_
