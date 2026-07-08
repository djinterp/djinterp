/******************************************************************************
* djinterp [test]                                        option_diff_tests.hpp
*
*   Shared surface for the option_diff.hpp unit suite.  Declares the six
* section block-providers - each defined in its own translation unit - and
* the test helpers those sections share:
*
*     - test_option_map   a runtime map TEST-DOUBLE modelling the duck-typed
*                         "option_set_map surface" PART B is generic over
*                         (::key_type, ::mapped_type, begin/end over entries
*                         with .key/.value, contains, find, insert,
*                         insert_or_assign).  option_diff.hpp ships no
*                         concrete type with this surface, so the runtime
*                         functions are exercised against this stand-in.
*     - tval / extract_tval
*                         a compile-time value-carrying option arg and a
*                         matching extractor, used to drive PART A's value-
*                         level traits (changed / unchanged / diff_count /
*                         value_equal), all of which take an extractor.
*     - opt / key_opt / oset / diff_key
*                         terse spellings for building the option_set<>
*                         fixtures the compile-time sections diff.
*
*   NOTE - option_diff.hpp default-extractor SHIM:
*   As of this writing option_diff.hpp declares its value-level traits with
* `template<typename> typename _Extract = extract_actual`, but no
* extract_actual is defined anywhere in the subframework (option_set_compare.hpp
* retired its extractors with the actual<> carrier and now ships none).  An
* undefined default template-template argument is a hard PARSE error, so the
* header will not include without the name being visible.  The forward
* declaration below makes the header parse; every value-level trait is then
* instantiated with an EXPLICIT extractor (extract_tval), so the undefined
* default is never itself instantiated.  The declaration is deletable once
* option_diff.hpp is fixed (either by removing the default - making _Extract
* required, as option_set_value_eq already is - or by defining a real
* extract_actual); it is compatible with either fix.
*
*   Standard tiering: PART A (the compile-time traits) is gated behind
* D_ENV_LANG_IS_CPP20_OR_HIGHER in each section, mirroring the option_set
* suite - A.V's diff_merge_t rides the concept-constrained option_set_override
* engine, so the trait half is effectively C++20-only.  Below that tier the
* PART A blocks emit empty.  PART B is standard-agnostic and ungated.
*
*
* path:      /tests/djinterp/core/option/option_diff_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
******************************************************************************/

#ifndef DJINTERP_OPTION_DIFF_TESTS_
#define DJINTERP_OPTION_DIFF_TESTS_ 1

// --- default-extractor shim (see header note) ----------------------------
//   Forward-declare the (currently undefined) default extractor BEFORE any
// include, so it is visible however option_diff.hpp is reached.  A raw
// namespace is used here (djinterp.hpp is not yet included, so NS_DJINTERP
// is not yet defined).  Never defined, never instantiated - the value-level
// traits are always called below with an explicit extractor (extract_tval).
namespace djinterp
{
    template<typename _Option>
    struct extract_actual;
}

// std
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

// djinterp [test] - framework runner + report options surface
#include "../../../../inc/djinterp/test/test_spec_runner.hpp"
#include "../../../../inc/djinterp/test/test_options.hpp"
// djinterp [option] - the header under test (extract_actual shim above)
#include "../../../../inc/djinterp/core/option/option_diff.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// shared compile-time (PART A) helpers
// ===========================================================================

// tval
//   type: a value-carrying option arg.  option<key, tval<v>> is the fixture
// shape the compile-time value-level tests diff; extract_tval pulls the v.
template<auto _Value>
struct tval
{};

// extract_tval
//   trait: unary extractor over option<>.  Yields value_present<v> for an
// option whose first arg is tval<v>, else value_absent.  Models the
// {value_absent | value_present<V>} carrier interface option_diff.hpp's
// value-level traits compare through internal::carrier_eq.
template<typename _Option>
struct extract_tval
{
    using type = ::djinterp::value_absent;
};

template<auto        _Key,
         auto        _Value,
         typename... _Rest>
struct extract_tval< ::djinterp::option<_Key, tval<_Value>, _Rest...> >
{
    using type = ::djinterp::value_present<_Value>;
};

// diff_key
//   enum: the key vocabulary the option_set<> fixtures are keyed on.
enum class diff_key
{
    alpha,
    beta,
    gamma,
    delta,
    epsilon
};

// opt
//   type: terse spelling of a value-carrying option<key, tval<value>>.
template<auto _Key,
         auto _Value>
using opt = ::djinterp::option<_Key, tval<_Value>>;

// key_opt
//   type: terse spelling of a value-ABSENT option<key> (no arg pack); used
// to test the value_absent branch of the extractor / carrier comparison.
template<auto _Key>
using key_opt = ::djinterp::option<_Key>;

// oset
//   type: terse spelling of option_set<...>.
template<typename... _Options>
using oset = ::djinterp::option_set<_Options...>;

// same
//   trait: ORDER-SENSITIVE type equality.  key_list<...> results carry
// their keys in a defined order (added -> derived order; removed / common /
// changed / unchanged -> base order), so the trait tests assert on exact
// key_list<...> identity rather than set membership.
template<typename _Lhs,
         typename _Rhs>
struct same
    : std::false_type
{};

template<typename _T>
struct same<_T, _T>
    : std::true_type
{};

template<typename _Lhs,
         typename _Rhs>
D_CONSTEXPR_INLINE bool same_v = same<_Lhs, _Rhs>::value;


// ===========================================================================
// shared runtime (PART B) helper
// ===========================================================================

// test_option_map
//   struct: minimal TEST-DOUBLE for the runtime "option_set_map surface"
// the PART B functions are written against.  Not part of option_diff.hpp -
// the runtime half is duck-typed over any type exposing this shape, and the
// subframework ships no concrete one, so the algorithms are unit-tested
// through this stand-in.  Backed by an insertion-ordered vector so key order
// (which the returned key vectors preserve) is observable and deterministic.
template<typename _Key,
         typename _Value>
struct test_option_map
{
    using key_type    = _Key;
    using mapped_type = _Value;

    // entry
    //   struct: one key/value cell, exposing the public .key / .value the
    // PART B iteration relies on.
    struct entry
    {
        _Key   key;
        _Value value;
    };

    using storage        = std::vector<entry>;
    using iterator       = typename storage::iterator;
    using const_iterator = typename storage::const_iterator;

    storage m_entries;

    // iteration (const + non-const; merge mutates the target through the
    // non-const overloads, every query reads through the const ones)
    iterator       begin()       { return m_entries.begin(); }
    iterator       end()         { return m_entries.end();   }
    const_iterator begin() const { return m_entries.begin(); }
    const_iterator end()   const { return m_entries.end();   }

    // find
    //   linear lookup by key; returns end() when absent.
    const_iterator find(const _Key& _key) const
    {
        for (const_iterator it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            if (it->key == _key)
            {
                return it;
            }
        }
        return m_entries.end();
    }

    iterator find(const _Key& _key)
    {
        for (iterator it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            if (it->key == _key)
            {
                return it;
            }
        }
        return m_entries.end();
    }

    // contains
    //   membership test in terms of find.
    bool contains(const _Key& _key) const
    {
        return (find(_key) != m_entries.end());
    }

    // insert
    //   map-style add-if-absent (never overwrites).  option_merge only calls
    // this for keys already known absent, but keeping it non-clobbering makes
    // the double safe against duplicate keys under direct use.
    void insert(const _Key& _key, const _Value& _value)
    {
        if (find(_key) == m_entries.end())
        {
            m_entries.push_back(entry{ _key, _value });
        }
    }

    // insert_or_assign
    //   upsert: overwrite an existing value, else append.
    void insert_or_assign(const _Key& _key, const _Value& _value)
    {
        iterator it = find(_key);
        if (it != m_entries.end())
        {
            it->value = _value;
        }
        else
        {
            m_entries.push_back(entry{ _key, _value });
        }
    }

    // with
    //   chainable builder sugar for fixtures: map.with(k, v).with(...).
    test_option_map& with(const _Key& _key, const _Value& _value)
    {
        insert_or_assign(_key, _value);
        return *this;
    }

    // size
    //   entry count (used to assert merge growth).
    std::size_t size() const
    {
        return m_entries.size();
    }
};


// ===========================================================================
// section block-providers (one per translation unit)
// ===========================================================================
//
//   Each returns the section's block_spec; the runner assembles them into a
// single module_spec.  Names mirror the header's own section numbering.

// --- PART A - compile-time -------------------------------------------------

// A.II  key-level diff traits (+ the A.I key-list set primitives)
::djinterp::test::block_spec option_diff_ct_keys_block();

// A.III + A.IV  value-level diff traits + diff summary
::djinterp::test::block_spec option_diff_ct_values_block();

// A.V  merge bridge (merge_mode -> policy, diff_merge_t)
::djinterp::test::block_spec option_diff_ct_merge_block();

// --- PART B - runtime ------------------------------------------------------

// B.I  key-level diff (keys_in, added, removed, common)
::djinterp::test::block_spec option_diff_rt_keys_block();

// B.II  value-level diff (changed, unchanged, diff_count, sets_equal)
::djinterp::test::block_spec option_diff_rt_values_block();

// B.III  merge (option_merge, all modes)
::djinterp::test::block_spec option_diff_rt_merge_block();


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_OPTION_DIFF_TESTS_
