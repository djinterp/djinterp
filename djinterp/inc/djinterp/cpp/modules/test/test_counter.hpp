/*******************************************************************************
* djinterp [cpp][test]                                          test_counter.hpp
*
*   DTest's counters for the C++ face: a test counter is the generic counter
* (cpp/util/counter/counter.hpp) plus the step one occurrence moves it by, and
* a tally is a set of test counters kept by key type.
*
* NOTHING HERE KNOWS WHAT IS COUNTED
* ----------------------------------
*   The owner's rulings of 2026-11-09 and 11-10 made DTest default-agnostic: a
* tally counts under whichever tag types its caller names -- a test passing, an
* assertion, a retry, any type at all -- and holds whichever counters its caller
* tracks. C's counterpart is c/modules/test/test_counter.h, which has the same
* two types.
*
* DERIVED
* -------
*   test_counter<V> is a djinterp::counter<V>: every operation and accessor of
* the generic counter -- the five operations, bounds, children -- is its own.
* What it adds is a step, and record(), which moves it by one step for one
* occurrence.
*
* THE TALLY
* ---------
*   test_tally<V> keeps one test counter per key type, created at zero the first
* time the key is tracked. Counters live in a std::deque, so a counter's address
* -- which a handler may hold -- never moves while the tally lives. Unlike C's,
* it has no capacity, so it never refuses a key and has no overflow.
*
* COUNTED BY HAND
* ---------------
*   Nothing here knows about events. A suite records and increments as it goes;
* one that wants events to count for it binds the handlers of
* test_counter_event.hpp, the tie-in.
*
* PORTABILITY
* -----------
*   Requires C++17 or later, as counter.hpp does; below that this file is
* empty.
*
* HOW TO USE
* ----------
*   Count by hand; a tally is keyed by any type:
*
*     struct retried;
*     test_tally<> tally;
*
*     tally.record<retried>();            // one step
*     tally.increment<retried>(2);
*     tally.count<retried>();             // 3, at the knobs' step
*
*   To have events count instead, bind the handlers of test_counter_event.hpp.
*
* OPTIONS
* -------
*   test_counter_options<V> sets how a counter starts: its step, how far one
* occurrence moves it, and whether it is held at or above zero. Its defaults are
* the knobs'; a tally hands its options to every counter it tracks.
*
* CONFIGURATION
* -------------
*   D_CFG_TEST_COUNTER_STEP and D_CFG_TEST_COUNTER_FLOOR, in
* config/c/modules/test/cfg_test_counter.h, the same knobs as C's.
*
*
* path:      /inc/djinterp/cpp/modules/test/test_counter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.08
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE TEST COUNTER
    ----------------
    1.  test_counter_options
    2.  test_counter
2.  THE TALLY
    ---------
    1.  test_tally
*/

#ifndef DJINTERP_TEST_COUNTER_HPP
#define DJINTERP_TEST_COUNTER_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but cpp/util/counter/counter.hpp,
// which it includes, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <deque>
#include <limits>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"                                 // framework root
#include "../../../config/c/modules/test/cfg_test_counter.h"  // the knobs
#include "../../util/counter/counter.hpp"                     // counter
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP
NS_TEST


NS_INTERNAL

    /**
     * @brief The number a tally tracks a key type by: the address of a
     *        variable each instantiation has of its own, which needs no RTTI.
     *
     * @tparam Key  the key type.
     * @return the number.
     */
    template<typename Key>
    std::size_t test_key() noexcept
    {
        static const char anchor = '\0';

        return reinterpret_cast<std::size_t>(&anchor);
    }

    // test_key_has_name
    //   trait: whether a key type has a static name(), for reports.
    template<typename Key,
             typename = void>
    struct test_key_has_name : std::false_type
    {
    };

    // test_key_has_name
    //   trait: the case of a key type with a static name().
    template<typename Key>
    struct test_key_has_name<Key,
                             std::void_t<decltype(Key::name())>>
        : std::true_type
    {
    };

NS_END  // internal


//==============================================================================
// 1.  THE TEST COUNTER
//==============================================================================


// 1.1    test_counter_options
//------------------------------------------------------------------------------
// test_counter_options
//   struct: how a test counter starts. Value-initialized, it holds the knobs'
// defaults; C's counterpart is struct dj_test_counter_options.
//
// Example:
//   test_counter_options<> options;   // the knobs'
//   options.step      = 2;
//   options.has_floor = true;
//   test_counter<>         c(options);
//   test_tally<>           t(options);
template<typename ValueType = re_std::int64_t>
struct test_counter_options
{
    // how far one occurrence moves a counter; not 0
    ValueType step      = static_cast<ValueType>(D_INTERNAL_TEST_COUNTER_STEP);
    // hold the counter at or above zero
    bool      has_floor = (D_INTERNAL_TEST_COUNTER_FLOOR != 0);
};


// 1.2    test_counter
//------------------------------------------------------------------------------
// test_counter
//   class: the generic counter, as DTest's, with a step to count by.
//
// Example:
//   test_counter<int> retries;
//   retries.record();             // one occurrence: up by the step
//   retries.increment(4);         // the generic counter's own
//   retries.value();              // 5, at the knobs' step
template<typename ValueType = re_std::int64_t>
class test_counter : public counter<ValueType>
{
public:
    using base_type    = counter<ValueType>;
    using value_type   = ValueType;
    using options_type = test_counter_options<ValueType>;

    // the generic counter's constructors: (), and (initial, min, max); either
    // takes the knobs' step
    using base_type::base_type;

    /**
     * @brief Constructs a counter at zero from `_options`.
     *
     * @param[in] _options  the step, and whether to hold the counter at or
     *                      above zero; a step of 0 takes the knobs'.
     */
    explicit test_counter(
        const options_type& _options
    ) noexcept
        : base_type(value_type{0},
                    (_options.has_floor)
                        ? value_type{0}
                        : std::numeric_limits<value_type>::lowest(),
                    std::numeric_limits<value_type>::max()),
          m_step( (_options.step != value_type{0})
                      ? _options.step
                      : options_type().step )
    {
    }

    /**
     * @brief Returns how far one occurrence moves this counter.
     *
     * @return the step record() moves by.
     */
    value_type step() const noexcept
    {
        return m_step;
    }

    /**
     * @brief Records one occurrence: moves this counter by its step.
     *
     * @return as counter::increment.
     */
    bool record()
    {
        return this->increment(m_step);
    }

private:
    value_type m_step = options_type().step;  // per occurrence
};


//==============================================================================
// 2.  THE TALLY
//==============================================================================


// 2.1    test_tally
//------------------------------------------------------------------------------
// test_tally
//   class: test counters kept by key type, in the order they were first
// tracked. A key is any type, complete or not; one with a static name() lends
// it to reports.
//
// Example:
//   struct retried;
//   test_tally<> tally;
//   tally.record<retried>();             // one occurrence, by the step
//   tally.increment<retried>(2);
//   tally.count<retried>();              // 3, at the knobs' step
template<typename ValueType = re_std::int64_t>
class test_tally
{
public:
    using value_type   = ValueType;
    using counter_type = test_counter<ValueType>;
    using options_type = test_counter_options<ValueType>;
    using size_type    = std::size_t;

    /**
     * @brief Constructs an empty tally whose counters take the knobs'
     *        options.
     */
    test_tally() = default;

    /**
     * @brief Constructs an empty tally whose counters take `_options`.
     *
     * @param[in] _options  every tracked counter's step and floor.
     */
    explicit test_tally(
        const options_type& _options
    )
        : m_options(_options),
          m_entries()
    {
    }

    /**
     * @brief Returns the options every counter this tally tracks starts
     *        from.
     *
     * @return the options.
     */
    const options_type& options() const noexcept
    {
        return m_options;
    }

    /**
     * @brief Returns the counter for `Key`, tracking the key at zero the
     *        first time.
     *
     * @tparam Key  the key type.
     * @return the key's counter; its address is stable while the tally
     *         lives and is not cleared.
     */
    template<typename Key>
    counter_type& track()
    {
        return track_key(key_of<Key>(),
                         name_of<Key>());
    }

    /**
     * @brief Returns the counter for `Key`, if it is tracked.
     *
     * @tparam Key  the key type.
     * @return the key's counter, or nullptr when it is not tracked.
     */
    template<typename Key>
    counter_type* find() noexcept
    {
        return find_key(key_of<Key>());
    }

    /**
     * @brief Returns the counter for `Key`, if it is tracked.
     *
     * @tparam Key  the key type.
     * @return the key's counter, or nullptr when it is not tracked.
     */
    template<typename Key>
    const counter_type* find() const noexcept
    {
        return const_cast<test_tally*>(this)->find_key(key_of<Key>());
    }

    /**
     * @brief Records one occurrence of `Key`: moves its counter by its step,
     *        tracking the key if it must.
     *
     * @tparam Key  the key type.
     * @return as counter::increment.
     */
    template<typename Key>
    bool record()
    {
        return track<Key>().record();
    }

    /**
     * @brief Moves the counter for `Key` up by `_amount`, tracking the key if
     *        it must.
     *
     * @tparam Key  the key type.
     * @param[in] _amount  how far to move it; negative moves it down.
     * @return as counter::increment.
     */
    template<typename Key>
    bool increment(
        value_type _amount
    )
    {
        return track<Key>().increment(_amount);
    }

    /**
     * @brief Returns the count for `Key`.
     *
     * @tparam Key  the key type.
     * @return the counter's value, or zero for a key not tracked.
     */
    template<typename Key>
    value_type count() const noexcept
    {
        const counter_type* const found = find<Key>();

        return (found) ? found->value() : value_type{0};
    }

    /**
     * @brief Adds every counter of `_other` to this tally, key by key.
     *
     * @note A key this tally does not track yet is tracked first. Merging
     *       a tally into itself doubles each count's movement once.
     *
     * @param[in] _other  the tally merged from.
     * @return `true` when every counter merged in full.
     */
    bool merge(
        const test_tally& _other
    )
    {
        const size_type entries  = _other.m_entries.size();
        bool            is_exact = true;

        // by index, so a self-merge sees only the entries it started with
        for (size_type i = 0; i < entries; ++i)
        {
            const entry&  from = _other.m_entries[i];
            counter_type& into = track_key(from.key,
                                           from.name);

            // a cut merge spoils the result but not the rest
            if (!into.merge(from.counter))
            {
                is_exact = false;
            }
        }

        return is_exact;
    }

    /**
     * @brief Returns every counter to its start; the keys stay tracked.
     */
    void reset()
    {
        // each counter, handlers untouched
        for (entry& each : m_entries)
        {
            each.counter.reset();
        }

        return;
    }

    /**
     * @brief Forgets every tracked key; the options stay.
     *
     * @pre No handler is bound to a counter of the tally.
     * @post The tally is empty; every counter it returned is invalid.
     */
    void clear() noexcept
    {
        m_entries.clear();

        return;
    }

    /**
     * @brief Returns how many keys are tracked.
     *
     * @return the count of tracked keys.
     */
    size_type size() const noexcept
    {
        return m_entries.size();
    }

    /**
     * @brief Returns the name of the `_index`th tracked key, for reports.
     *
     * @param[in] _index  the position, in the order keys were tracked.
     * @return the key type's name(), or "" for a key with none or an index
     *         out of range.
     */
    const char* name(
        size_type _index
    ) const noexcept
    {
        return (_index < m_entries.size()) ? m_entries[_index].name : "";
    }

    /**
     * @brief Returns the `_index`th tracked counter, for reports.
     *
     * @param[in] _index  the position; less than size().
     * @return the counter.
     */
    const counter_type& at(
        size_type _index
    ) const
    {
        return m_entries[_index].counter;
    }

private:
    // entry
    //   struct: one tracked key: its number, its name and its counter.
    struct entry
    {
        std::size_t  key;      // internal::test_key<Key>()
        const char*  name;     // Key::name(), or ""
        counter_type counter;  // the count
    };

    /**
     * @brief The number a key type is tracked by.
     *
     * @tparam Key  the key type.
     * @return DTest's per-type number, which needs no RTTI.
     */
    template<typename Key>
    static std::size_t key_of() noexcept
    {
        return internal::test_key<std::remove_cv_t<
                   std::remove_reference_t<Key>>>();
    }

    /**
     * @brief The name a key type reports by.
     *
     * @tparam Key  the key type.
     * @return Key::name() where the type has one, and "" otherwise.
     */
    template<typename Key>
    static const char* name_of() noexcept
    {
        // only a type with a name() has one to give
        if constexpr (internal::test_key_has_name<Key>::value)
        {
            return Key::name();
        }
        else
        {
            return "";
        }
    }

    /**
     * @brief The counter tracked under `_key`, if any.
     *
     * @param[in] _key  the key's number.
     * @return its counter, or nullptr.
     */
    counter_type* find_key(
        std::size_t _key
    ) noexcept
    {
        // the first, and only, entry for the key
        for (entry& each : m_entries)
        {
            // this key's
            if (each.key == _key)
            {
                return &each.counter;
            }
        }

        return nullptr;
    }

    /**
     * @brief The counter tracked under `_key`, tracking it at zero, with the
     *        tally's options, if new.
     *
     * @param[in] _key   the key's number.
     * @param[in] _name  the key's name, for a new entry.
     * @return the counter.
     */
    counter_type& track_key(
        std::size_t _key,
        const char* _name
    )
    {
        counter_type* found = find_key(_key);

        // a new key gets a counter at zero, as the options say
        if (!found)
        {
            m_entries.push_back(entry{_key,
                                      _name,
                                      counter_type(m_options)});
            found = &m_entries.back().counter;
        }

        return *found;
    }

    options_type      m_options;  // every new counter's
    std::deque<entry> m_entries;  // by first tracking; addresses stable
};


NS_END  // test
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_TEST_COUNTER_HPP
