/*******************************************************************************
* djinterp [core]                                                 mem_common.hpp
*
* C++ face of the memory subframework's shared vocabulary.
*   This header adds NOTHING to what mem_common.h already computes. It renames
* the C vocabulary into the djinterp namespace and supplies the traits and
* concepts that let a caller substitute its own resource behind the arena and
* pool faces. Every arithmetic kernel it exposes is the C one, called through.
*   The SOURCE wrappers live in mem_source.hpp, mirroring the C tier one for
* one: mem_common.h owns the vocabulary and mem_source.h owns the supply, so
* their C++ faces divide the same way.
*
*   WHY THIS IS A _common.hpp: mem_source.hpp, arena.hpp, pool.hpp,
* pool_allocator.hpp and the whole strategy tier need the status type, the
* block view and the alignment kernels. Two of them would otherwise each grow
* a private copy, which is the condition the framework reserves the _common
* suffix for.
*
* THE COST LAW (AGENT_README.md section 6, and it is a TEST here, not a
* convention): every wrapper in this subframework satisfies
*
*     sizeof(wrapper) == sizeof(wrapped)
*
* and is asserted to. The framework's own history is the argument: color_rgb's
* production wrapper was once silently replaced by a standalone struct that
* did not derive from its kernel, and nothing caught it. A static_assert
* catches it.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/core/memory/mem_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    VOCABULARY ALIASES
      ------------------
      a. mem_size / mem_status / mem_block / mem_stats
      b. status_is_ok / status_name
II.   ALIGNMENT KERNELS
      -----------------
      a. is_pow2 / align_up / align_padding / is_aligned
      b. align_of_v
III.  TRAITS
      ------
      a. has_allocate / has_release / has_reset
      b. is_memory_resource
IV.   CONCEPTS  (C++20, gated)
      -----------------------
      a. memory_resource_c / releasing_resource / resettable_resource
*/

#ifndef DJINTERP_MEMORY_MEM_COMMON_HPP
#define DJINTERP_MEMORY_MEM_COMMON_HPP 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../c/memory/mem_common.h"
#include "../../c/memory/mem_source.h"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                   I.   VOCABULARY ALIASES                               ///
///////////////////////////////////////////////////////////////////////////////

// mem_size
//   type: the counter every allocator in this subframework counts in. Whose
// width is D_CFG_MEM_SIZE_BITS -- the SAME type the C core uses, not a
// std::size_t that happens to agree, because a C++ face that quietly widened
// the counter would produce a different layout from the C one and break the
// parity law at the first struct that held it.
using mem_size = ::d_mem_size;

// mem_status
//   type: the outcome of a fallible memory operation.
using mem_status = ::d_mem_status;

// mem_block
//   type: a borrowed view of a contiguous byte range.
using mem_block = ::d_mem_block;

// mem_stats
//   type: byte and call accounting for one allocator.
using mem_stats = ::d_mem_stats;

// status_is_ok
//   function: true when a status names success.
D_CONSTEXPR_INLINE bool
status_is_ok(
    mem_status _status
)
{
    return (_status == D_MEM_OK);
}

// status_name
//   function: a stable, static name for a status. Never null.
D_INLINE const char*
status_name(
    mem_status _status
)
{
    return ::d_mem_status_string(_status);
}


///////////////////////////////////////////////////////////////////////////////
///                   II.   ALIGNMENT KERNELS                               ///
///////////////////////////////////////////////////////////////////////////////
//   These forward to the C kernels rather than reimplementing them. That is
// the point of the exercise: the same function, evaluated EARLIER in C++ than
// in C, never differently. Each is constexpr because the C kernel is declared
// D_MEM_FN, which resolves to constexpr under a C++ compiler.

// is_pow2
//   function: true when the value is a power of two. Zero is not.
D_CONSTEXPR_INLINE bool
is_pow2(
    mem_size _value
)
{
    return ::d_mem_is_pow2(_value);
}

// align_up
//   function: the value rounded up to the next multiple of a power-of-two
// alignment.
D_CONSTEXPR_INLINE mem_size
align_up(
    mem_size _value,
    mem_size _align
)
{
    return ::d_mem_align_up(_value, _align);
}

// align_padding
//   function: the bytes to skip from a value to reach the next multiple of an
// alignment.
D_CONSTEXPR_INLINE mem_size
align_padding(
    mem_size _value,
    mem_size _align
)
{
    return ::d_mem_align_padding(_value, _align);
}

// is_aligned
//   function: true when an address is a multiple of an alignment.
D_INLINE bool
is_aligned(
    const void* _ptr,
    mem_size    _align
)
{
    return ::d_mem_is_aligned(_ptr, _align);
}

// align_of_v
//   trait: alignof(_Type) as a mem_size, so a caller may hand a type's
// alignment straight to a C entry point without a cast at every call site.
template<typename _Type>
struct align_of
{
    static D_CONSTEXPR const mem_size value =
        static_cast<mem_size>(alignof(_Type));
};

// size_of_v
//   trait: sizeof(_Type) as a mem_size, for the same reason.
template<typename _Type>
struct size_of
{
    static D_CONSTEXPR const mem_size value =
        static_cast<mem_size>(sizeof(_Type));
};


///////////////////////////////////////////////////////////////////////////////
///                       III.   TRAITS                                     ///
///////////////////////////////////////////////////////////////////////////////
//   Purely structural: they duck-type the protocol rather than looking for a
// tag or a base class, so a caller's own resource satisfies them without
// including anything from here. Compiled only when D_CFG_MEM_TRAIT_DETECTORS
// is on, since a build that never substitutes a resource pays nothing for the
// ability.

#if (D_INTERNAL_MEM_TRAIT_DETECTORS == 1)

NS_INTERNAL

    // void_t
    //   type: the detection-idiom sink. Spelled here rather than taken from
    // <type_traits> because std::void_t is C++17 and this face's floor is
    // C++11.
    template<typename...>
    struct make_void
    {
        using type = void;
    };

    template<typename... _Args>
    using void_t = typename make_void<_Args...>::type;

NS_END  // internal

// has_allocate
//   trait: detects a resource exposing allocate(mem_size, mem_size).
template<typename _Type,
         typename _Enable = void>
struct has_allocate : std::false_type
{};

template<typename _Type>
struct has_allocate<
    _Type,
    internal::void_t<decltype(std::declval<_Type&>().allocate(
        std::declval<mem_size>(),
        std::declval<mem_size>()))>> : std::true_type
{};

// has_release
//   trait: detects a resource exposing release(void*, mem_size, mem_size).
template<typename _Type,
         typename _Enable = void>
struct has_release : std::false_type
{};

template<typename _Type>
struct has_release<
    _Type,
    internal::void_t<decltype(std::declval<_Type&>().release(
        std::declval<void*>(),
        std::declval<mem_size>(),
        std::declval<mem_size>()))>> : std::true_type
{};

// has_reset
//   trait: detects a resource exposing reset().
template<typename _Type,
         typename _Enable = void>
struct has_reset : std::false_type
{};

template<typename _Type>
struct has_reset<
    _Type,
    internal::void_t<decltype(std::declval<_Type&>().reset())>>
    : std::true_type
{};

// is_memory_resource
//   trait: detects the minimum resource protocol -- something that can vend
// bytes at an alignment.
template<typename _Type>
struct is_memory_resource
{
    static D_CONSTEXPR const bool value = has_allocate<_Type>::value;
};

#endif  // D_INTERNAL_MEM_TRAIT_DETECTORS


///////////////////////////////////////////////////////////////////////////////
///                      IV.   CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////
//   A thin face over the traits above, and nothing more. Dropping them where
// concepts are unavailable changes what is CHECKED, never what is computed --
// which is why D_CFG_MEM_USE_CONCEPTS can be turned off on a tier that has
// them without any behaviour moving.

#if (D_INTERNAL_MEM_USE_CONCEPTS == 1) &&                                     \
    (D_INTERNAL_MEM_TRAIT_DETECTORS == 1) &&                                  \
    defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// memory_resource_c
//   concept: constrains types satisfying the minimum resource protocol.
template<typename _Type>
concept memory_resource_c = is_memory_resource<_Type>::value;

// releasing_resource
//   concept: constrains resources that can take bytes back individually.
template<typename _Type>
concept releasing_resource = has_release<_Type>::value;

// resettable_resource
//   concept: constrains resources that can reclaim everything at once.
template<typename _Type>
concept resettable_resource = has_reset<_Type>::value;

#endif  // concepts


///////////////////////////////////////////////////////////////////////////////
///                      V.   THE COST LAW                                  ///
///////////////////////////////////////////////////////////////////////////////
//   Asserted rather than asserted-to-be-true-in-a-comment. See the header
// banner for why this is a test.

D_STATIC_ASSERT(sizeof(mem_block) == sizeof(::d_mem_block),
                "the C++ block view must be the C one, not a copy of it");

D_STATIC_ASSERT(sizeof(mem_stats) == sizeof(::d_mem_stats),
                "the C++ accounting block must be the C one");


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_MEM_COMMON_HPP
