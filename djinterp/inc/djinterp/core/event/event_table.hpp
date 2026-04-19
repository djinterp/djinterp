/******************************************************************************
* djinterp [event]                                             event_table.hpp
*
* Event table:
*   Type-erased listener storage for the event system. Provides the low-level
* data structure that maps event type keys to ordered lists of listener
* entries, with insert, remove, enable, disable, lookup, iteration, and
* statistics operations.
*
*   This is the C++ successor to the C event_table.h / event_table_common.h
* hash table, replacing the hand-rolled chaining hash table with a
* std::unordered_map backed by std::vector entry lists. The insertion-order
* guarantee within each event bucket is preserved.
*
* COMPONENTS:
*   djinterp::event_table_stats  - table statistics snapshot
*   djinterp::event_table        - type-erased listener storage
*
* INTERNAL COMPONENTS:
*   djinterp::internal::type_id_value<_Event>  - per-type unique key
*   djinterp::internal::listener_entry         - type-erased listener slot
*
* DESIGN:
*   - The table is type-unaware: it operates on std::size_t keys and
*     type-erased listener_entry values. Typed operations (bind, dispatch)
*     are provided by listener_registry in event_listener.hpp.
*   - Entries within each bucket are stored in a std::vector, preserving
*     insertion order and providing cache-friendly iteration.
*   - Statistics mirror the C d_event_hash_stats interface, adapted
*     for the underlying std::unordered_map.
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES  - move semantics
*   D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES    - using aliases
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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <unordered_map>

#include "event_listener_traits.hpp"


NS_DJINTERP


// =========================================================================
// I.   INTERNAL STORAGE TYPES
// =========================================================================

NS_INTERNAL

    // type_id_value
    //   function: returns a unique identifier for each type, usable
    // as a key for the type-erased listener map. Each instantiation
    // of this function template has its own static variable whose
    // address serves as a globally unique per-type token.
    template<typename _Event>
    std::size_t type_id_value()
    {
        static const char anchor = '\0';

        return reinterpret_cast<std::size_t>(&anchor);
    };

    // listener_entry
    //   struct: type-erased listener slot with enable/disable
    // support. The `invoke` callable expects a void* pointing to a
    // std::tuple<event_context&, args_type> constructed by the
    // dispatch site in listener_registry.
    struct listener_entry
    {
        listener_id                    id;
        std::function<void(void*)>     invoke;
        bool                           enabled;
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
//   class: type-erased listener storage. Maps event type keys
// (std::size_t) to ordered vectors of listener entries. Provides
// the raw storage operations that listener_registry builds its
// typed API upon.
class event_table
{
private:
    using entry_type = internal::listener_entry;
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
    //   inserts a listener entry into the bucket identified by
    // _type_key. The entry is appended to the end of the bucket,
    // preserving insertion order.
    // returns: the listener_id assigned to the new entry.
    listener_id insert(std::size_t                _type_key,
                       std::function<void(void*)> _invoke)
    {
        listener_id lid;
        lid.value = m_next_id++;

        entry_type entry;
        entry.id      = lid;
        entry.invoke  = std::move(_invoke);
        entry.enabled = true;

        m_table[_type_key].push_back(std::move(entry));
        ++m_total_count;
        ++m_enabled_count;

        return lid;
    };

    // ---- removal ----

    // remove
    //   removes the listener identified by _id from any bucket.
    // returns: true if the listener was found and removed.
    bool remove(listener_id _id)
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

    // ---- enable / disable ----

    // enable
    //   enables the listener identified by _id.
    // returns: true if the listener was found and enabled (was
    // previously disabled).
    bool enable(listener_id _id)
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
    //   disables the listener identified by _id. Disabled listeners
    // remain stored but are skipped during dispatch.
    // returns: true if the listener was found and disabled (was
    // previously enabled).
    bool disable(listener_id _id)
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
    //   queries whether the listener identified by _id is currently
    // enabled.
    // returns: true if found and enabled, false otherwise.
    bool is_enabled(listener_id _id) const
    {
        const entry_type* entry = find_entry_const(_id);

        if (!entry)
        {
            return false;
        }

        return entry->enabled;
    };

    // contains
    //   queries whether a listener with the given _id exists in any
    // bucket.
    bool contains(listener_id _id) const
    {
        return (find_entry_const(_id) != nullptr);
    };

    // ---- bucket access ----

    // entries_for
    //   returns a pointer to the entry list for the given type key,
    // or nullptr if no entries exist for that key.
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
    //   returns a pointer to the mutable entry list for the given
    // type key, or nullptr if no entries exist for that key.
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
    //   returns true if at least one entry exists for the given
    // type key.
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
    //   invokes _fn for every entry in every bucket. The callable
    // receives (std::size_t type_key, const listener_entry& entry).
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
    //   invokes _fn for every entry in the bucket identified by
    // _type_key. The callable receives (const listener_entry& entry).
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

    // ---- statistics ----

    // get_stats
    //   computes and returns a snapshot of table metrics.
    event_table_stats get_stats() const
    {
        event_table_stats stats;
        stats.total_buckets      = m_table.bucket_count();
        stats.used_buckets       = 0;
        stats.total_entries      = m_total_count;
        stats.enabled_entries    = m_enabled_count;
        stats.event_type_count   = m_table.size();
        stats.max_entries_per_type = 0;

        // compute used buckets (unordered_map buckets with >=1
        // map entry, not to be confused with our event type
        // buckets)
        for (std::size_t i = 0; i < m_table.bucket_count(); ++i)
        {
            if (m_table.bucket_size(i) > 0)
            {
                ++stats.used_buckets;
            }
        }

        // compute max and average entries per event type
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
    //   locates a listener by id across all buckets.
    entry_type* find_entry(listener_id _id)
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
    //   const overload; locates a listener by id across all buckets.
    const entry_type* find_entry_const(listener_id _id) const
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


NS_END  // djinterp


#endif  // DJINTERP_EVENT_TABLE_
