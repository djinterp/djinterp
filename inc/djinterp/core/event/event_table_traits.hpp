/******************************************************************************
* djinterp [event]                                      event_table_traits.hpp
*
* Event table type traits:
*   Compile-time structural detection for types that satisfy the event table
* interface. Follows the same SFINAE detection pattern used throughout the
* djinterp::traits library (cpp_named11.h, etc.) to verify that a type
* provides the operations required by listener_registry.
*
*   This enables custom table implementations to be validated at compile
* time without inheriting from a base class.
*
* COMPONENTS:
*   djinterp::event_table_traits<_Table>
*     - has_insert           : insert(size_t, function) → listener_id
*     - has_remove           : remove(listener_id) → bool
*     - has_enable           : enable(listener_id) → bool
*     - has_disable          : disable(listener_id) → bool
*     - has_is_enabled       : is_enabled(listener_id) const → bool
*     - has_contains         : contains(listener_id) const → bool
*     - has_count_for        : count_for(size_t) const → size_t
*     - has_has_entries_for  : has_entries_for(size_t) const → bool
*     - has_total_count      : total_count() const → size_t
*     - has_enabled_count    : enabled_count() const → size_t
*     - has_clear            : clear() → void
*     - is_event_table       : all required operations present
*     - has_stats            : get_stats() const is well-formed
*     - has_iteration        : for_each_entry(callable) const
*     - has_type_key_count   : type_key_count() const → size_t
*
*   djinterp::is_event_table   (C++20 concept)
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES - parameter packs
*   D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES    - using aliases
*   D_ENV_CPP_FEATURE_LANG_CONCEPTS           - concept constraints (C++20)
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
* 
* path:      /inc/djinterp/core/event/event_table_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.12
******************************************************************************/

#ifndef DJINTERP_EVENT_TABLE_TRAITS_
#define DJINTERP_EVENT_TABLE_TRAITS_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "event_table_traits.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "event_table_traits.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "event_table_traits.hpp requires C++11 or higher"
#endif

#include <cstddef>
#include <functional>
#include <type_traits>

#include "event_listener_traits.hpp"


NS_DJINTERP


// =========================================================================
// I.   STRUCTURAL DETECTION HELPERS
// =========================================================================

NS_INTERNAL

    // ---- core mutation ops ----

    // has_table_insert
    //   trait: detects insert(size_t, std::function<void(void*)>)
    // returning listener_id.
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
                    std::declval<std::function<void(void*)>>())),
                listener_id
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_remove
    //   trait: detects remove(listener_id) returning bool.
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
                    std::declval<listener_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_enable
    //   trait: detects enable(listener_id) returning bool.
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
                    std::declval<listener_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_disable
    //   trait: detects disable(listener_id) returning bool.
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
                    std::declval<listener_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // ---- const query ops ----

    // has_table_is_enabled
    //   trait: detects is_enabled(listener_id) const returning bool.
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
                    std::declval<listener_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_table_contains
    //   trait: detects contains(listener_id) const returning bool.
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
                    std::declval<listener_id>())),
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

NS_END  // internal


// =========================================================================
// II.  EVENT TABLE TRAITS
// =========================================================================

// event_table_traits
//   trait: compile-time structural detection for types that satisfy
// the event table interface required by listener_registry. Validates
// that _Table provides insert, remove, enable, disable, query, and
// clear operations with the expected signatures.
//
// note: does not verify semantic contracts (e.g., that remove
// actually removes, or that enable/disable are idempotent). This
// is a structural check only, consistent with the djinterp trait
// philosophy.
template<typename _Table>
struct event_table_traits
{
    // ---- core mutation detection ----

    // has_insert
    //   constant: true if insert(size_t, function) → listener_id.
    static constexpr bool has_insert =
        internal::has_table_insert<_Table>::value;

    // has_remove
    //   constant: true if remove(listener_id) → bool.
    static constexpr bool has_remove =
        internal::has_table_remove<_Table>::value;

    // has_enable
    //   constant: true if enable(listener_id) → bool.
    static constexpr bool has_enable =
        internal::has_table_enable<_Table>::value;

    // has_disable
    //   constant: true if disable(listener_id) → bool.
    static constexpr bool has_disable =
        internal::has_table_disable<_Table>::value;

    // ---- const query detection ----

    // has_is_enabled
    //   constant: true if is_enabled(listener_id) const → bool.
    static constexpr bool has_is_enabled =
        internal::has_table_is_enabled<_Table>::value;

    // has_contains
    //   constant: true if contains(listener_id) const → bool.
    static constexpr bool has_contains =
        internal::has_table_contains<_Table>::value;

    // has_count_for
    //   constant: true if count_for(size_t) const → size_t.
    static constexpr bool has_count_for =
        internal::has_table_count_for<_Table>::value;

    // has_has_entries_for
    //   constant: true if has_entries_for(size_t) const → bool.
    static constexpr bool has_has_entries_for =
        internal::has_table_has_entries_for<_Table>::value;

    // has_total_count
    //   constant: true if total_count() const → size_t.
    static constexpr bool has_total_count =
        internal::has_table_total_count<_Table>::value;

    // has_enabled_count
    //   constant: true if enabled_count() const → size_t.
    static constexpr bool has_enabled_count =
        internal::has_table_enabled_count<_Table>::value;

    // has_clear
    //   constant: true if clear() is well-formed.
    static constexpr bool has_clear =
        internal::has_table_clear<_Table>::value;

    // ---- composite detection ----

    // is_event_table
    //   constant: true if _Table provides all required operations
    // for use as a listener storage backend.
    static constexpr bool is_event_table =
        ( has_insert        &&
          has_remove        &&
          has_enable        &&
          has_disable       &&
          has_is_enabled    &&
          has_contains      &&
          has_count_for     &&
          has_has_entries_for &&
          has_total_count   &&
          has_enabled_count &&
          has_clear );

    // ---- optional feature detection ----

    // has_type_key_count
    //   constant: true if type_key_count() const → size_t.
    static constexpr bool has_type_key_count =
        internal::has_table_type_key_count<_Table>::value;

    // has_stats
    //   constant: true if get_stats() const is well-formed.
    static constexpr bool has_stats =
        internal::has_table_get_stats<_Table>::value;

    // has_clear_for
    //   constant: true if clear_for(size_t) is well-formed.
    static constexpr bool has_clear_for =
        internal::has_table_clear_for<_Table>::value;
};


// =========================================================================
// III. CONCEPT CONSTRAINTS (C++20+)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// is_event_table
//   concept: constrains types that satisfy the event table structural
// requirements for use as a listener storage backend.
template<typename _Table>
concept is_event_table_type =
    event_table_traits<_Table>::is_event_table;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EVENT_TABLE_TRAITS_
