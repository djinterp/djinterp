/******************************************************************************
* djinterp [container]                            runtime_container_traits.hpp
*
*   Structural traits for the RUNTIME end of a container's Lifetime axis.  A
* container is "runtime-only" when its value cannot be settled in a constant
* expression - heap-backed dynamic storage, virtual functions, or accessors not
* declared constexpr - so its defining data come into being only as the program
* runs.  This module is the structural complement of
* constexpr_container_traits.hpp and shares its container_lifetime verdict:
*       is_runtime_container<T>
*         == "looks like a container" AND container_lifetime<T> is runtime-only
*   i.e. the container shape is present but the compile-time bit is absent.
*
*   DETECTION signals:
*     1. the type "looks like a container" - exposes a size() accessor;
*     2. its container_lifetime carries no compile-time bit (DYNAMIC lifetime);
*     3. (stronger) heap-storage indicators - an allocator_type alias, or a
*        reserve(size_type) member - which actively REQUIRE runtime storage.
*
*   LIFETIME vs STORAGE:
*   "Runtime-only" here is a Lifetime statement (WHEN the data are fixed), not a
* Storage one (WHERE the cells live); the two axes are linked but distinct, and
* the only entailment runs from inline storage to a compile-time-expressible
* size.  requires_runtime_storage isolates the genuine storage signals.
*
*   PORTABILITY:
*   C++11 baseline.  Every `_v` companion is emitted through the canonical
* trait_detect macros (inline variable on C++17+, variable template on C++14,
* absent on C++11 - the `::value` member is always present).
*
*
* path:      /inc/djinterp/core/container/traits/runtime_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_RUNTIME_CONTAINER_TRAITS_
#define DJINTERP_RUNTIME_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"               // clean_t, NS_*, D_ENV_*
#include "../../meta/trait_detect.hpp"      // D_TYPE_TRAIT_* macros
#include "../../meta/lifetime.hpp"          // lifetime, is_runtime_only
#include "./constexpr_container_traits.hpp" // container_lifetime


NS_DJINTERP


// ===========================================================================
// I.   Structural signals
// ===========================================================================
//   All probes strip cv-ref via clean_t so the answer agrees for T, const T,
// T&.

// has_size_accessor_signal
//   trait: structural detection of any size() accessor (constexpr or not).
// The "is a container" signal for this module.
D_TYPE_TRAIT_TRUE(has_size_accessor_signal,
    decltype(std::declval<const clean_t<_Type>&>().size()))

// has_allocator_alias
//   trait: detects an `allocator_type` member alias - a strong signal of
// heap-backed dynamic storage.
D_TYPE_TRAIT_HAS_TYPE(has_allocator_alias, allocator_type)

// has_reserve_method_signal
//   trait: detects `reserve(size_type)`, present on growable runtime
// containers.
D_TYPE_TRAIT_TRUE(has_reserve_method_signal,
    decltype(std::declval<clean_t<_Type>&>().reserve(std::declval<std::size_t>())))


// ===========================================================================
// II.  Classification umbrella
// ===========================================================================

// is_runtime_container
//   trait: true iff _Type is container-shaped yet NOT constexpr-capable - its
// container_lifetime is the runtime stage exclusively (a DYNAMIC lifetime).
template<typename _Type>
struct is_runtime_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_size_accessor_signal<clean_type>::value
          && is_runtime_only(container_lifetime<clean_type>::value) );
};

D_TYPE_TRAIT_VALUE_BOOL(is_runtime_container)

// requires_runtime_storage
//   trait: stronger signal - the container actively requires runtime-only
// storage (a heap allocator and/or growable capacity).
template<typename _Type>
struct requires_runtime_storage
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_allocator_alias<clean_type>::value
          || has_reserve_method_signal<clean_type>::value );
};

D_TYPE_TRAIT_VALUE_BOOL(requires_runtime_storage)


// ===========================================================================
// III. Aggregate snapshot
// ===========================================================================

// runtime_container_class
//   struct: a one-stop summary of the runtime signals and the resulting
// Lifetime verdict, for diagnostics and agent-facing reports.
template<typename _Type>
struct runtime_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool is_container_shape =
        has_size_accessor_signal<clean_type>::value;
    static constexpr bool is_runtime =
        is_runtime_container<clean_type>::value;
    static constexpr bool requires_runtime =
        requires_runtime_storage<clean_type>::value;
    static constexpr bool has_allocator =
        has_allocator_alias<clean_type>::value;
    static constexpr lifetime life =
        container_lifetime<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_RUNTIME_CONTAINER_TRAITS_
