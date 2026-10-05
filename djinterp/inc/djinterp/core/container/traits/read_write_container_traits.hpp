/*******************************************************************************
* djinterp [core]                                read_write_container_traits.hpp
*
*   The ACCESS axis (capability), container-side classifier - and the shared
* core of the access-trait trio.  Given a type, it reports which access
* CAPABILITY a handle to it grants: observation, mutation, both, or neither.
* Where the access tags read_write / read_only / write_only
* (container/access/)
* own the capability VOCABULARY - each a tag carrying two bits, can_read and
* can_write - this header is the one place that reads a TYPE and reports its
* capability AS one of those tags.  read_write heads the small lattice
*
*               read_write            (can_read && can_write)
*               / \
*       read_only        write_only   (read only)   (write only)
*
* and its two corners are the restrictions that read_only_container_traits.hpp
* and write_only_container_traits.hpp classify; so this baseline owns the
* enum,
* the structural probes, access_capability_of, the read_write predicate, the
* tag
* re-emission access_tag_of, and the aggregate snapshot, and the two corner
* headers add only their own predicate on top.
*
*   TWO SIGNALS, in priority order:
*     1. an explicit `capability` member tag carrying can_read / can_write -
*   an
*        access wrapper stamps its own capability, and this reads it back for
*        posterity (highest priority, the opt-in);
*     2. else the STRUCTURAL surface, by SFINAE -
*          read surface   - a value observer on a CONST instance: const
*                           operator[], front, data, or begin (so an
*                         associative,
*                           having none of the first three, still counts
*                         through
*                           its const iteration);
*          write surface  - a mutator on a NON-CONST instance: push_back,
*                           emplace_back, push_front, clear, or an assignable
*                           operator[] (so a fixed-size std::array - mutable
*                         in
*                           place but not growable - still reads writable).
*        Value-free metadata (size / empty) is deliberately NOT a read
*      surface:
*        it reveals no element value.  That is exactly what lets a write_only
*        sink - which exposes only size / empty plus the append surface - read
*      as
*        write-only rather than read_write.
*
*   The pair (can_read, can_write) then selects the capability: both is
* read_write, read-only is read_only, write-only is write_only, neither is
* none.
* The structural surface is a HEURISTIC (a by-value operator[], say, can read
* as
* writable); the opt-in tag of signal 1 is the exact escape hatch.
*
*   ORTHOGONALITY:
*   An access capability is an OVERLAY on a handle, orthogonal to the
* intrinsic
* mutability grade (mutable_container_traits.hpp): the grade says what the
* underlying type CAN do, the capability says what THIS handle is permitted to
* do. A read_only handle to a fully mutable container reads as read_only here.
*
*   PORTABILITY:
*   C++11 baseline.  Every `_v` companion is emitted through the canonical
* trait_detect macros (inline variable on C++17+, variable template on C++14,
* absent on C++11 - the `::value` / `::type` members are always present).
*
*            read_write_container_traits.hpp
*
*
* path:      /inc/djinterp/core/container/traits/read_write_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.01
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRAITS_READ_WRITE_CONTAINER_TRAITS_HPP
#define DJINTERP_CONTAINER_TRAITS_READ_WRITE_CONTAINER_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"           // clean_t, NS_*, D_ENV_* feature macros
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_VALUE_BOOL, D_VOID_T
#include "../../meta/read_write.hpp"     // read_write capability tag
#include "../../meta/read_only.hpp"      // read_only  capability tag
#include "../../meta/write_only.hpp"     // write_only capability tag


NS_DJINTERP


// ===========================================================================
// I.   Capability signals (structural SFINAE + the opt-in tag)
// ===========================================================================

NS_INTERNAL

    // --- the explicit tag (opt-in, highest priority) ---

    // has_capability_bits_helper
    //   helper: detects a `capability` member tag that carries BOTH can_read
    // and can_write - the self-describing access tag an access wrapper stamps
    // on itself. Only such a tag is trusted as the opt-in; a `capability`
    // alias lacking the bits is ignored, and the structural surface decides
    // instead.
    template<typename Type,
             typename = void>
    struct has_capability_bits_helper : std::false_type
    {};

    template<typename Type>
    struct has_capability_bits_helper<Type,
        D_VOID_T<decltype(clean_t<Type>::capability::can_read),
                 decltype(clean_t<Type>::capability::can_write)>>
        : std::true_type
    {};

    // --- read surface: value observation on a CONST instance ---

    // has_const_index_helper
    //   helper: detects const operator[](size_t) - readable random access.
    template<typename Type, typename = void>
    struct has_const_index_helper : std::false_type
    {};
    // has_const_index_helper<Type, D_VOID_T<decltype(std::declval<const
    // clean_t<Type>&>()[ std::declval<std::size_t>()])>>
    //   trait: the `D_VOID_T<decltype(std::declval<const clean_t<Type>&>()[
    // std::declval<std::size_t>()])>` case; it reports true.
    template<typename Type>
    struct has_const_index_helper<Type,
        D_VOID_T<decltype(std::declval<const clean_t<Type>&>()[
                     std::declval<std::size_t>()])>>
        : std::true_type
    {};

    // has_const_front_helper
    //   helper: detects const front() - readable first element.
    template<typename Type, typename = void>
    struct has_const_front_helper : std::false_type
    {};
    // has_const_front_helper specialization
    //   helper: the detected case -- selected when the type exposes the member
    // function `front()`.
    template<typename Type>
    struct has_const_front_helper<Type,
        D_VOID_T<decltype(std::declval<const clean_t<Type>&>().front())>>
        : std::true_type
    {};

    // has_const_data_helper
    //   helper: detects const data() - a readable contiguous block.
    template<typename Type, typename = void>
    struct has_const_data_helper : std::false_type
    {};
    // has_const_data_helper specialization
    //   helper: the detected case -- selected when the type exposes the member
    // function `data()`.
    template<typename Type>
    struct has_const_data_helper<Type,
        D_VOID_T<decltype(std::declval<const clean_t<Type>&>().data())>>
        : std::true_type
    {};

    // has_const_begin_helper
    //   helper: detects const begin() - readable iteration; the tell that lets
    // an associative (no index / front / data) still count as observable.
    template<typename Type, typename = void>
    struct has_const_begin_helper : std::false_type
    {};
    template<typename Type>
    struct has_const_begin_helper<Type,
        D_VOID_T<decltype(std::declval<const clean_t<Type>&>().begin())>>
        : std::true_type
    {};

    // has_read_surface
    //   helper: the type exposes SOME value observer (any of the four above).
    template<typename Type>
    struct has_read_surface
        : std::integral_constant<bool,
                has_const_index_helper<Type>::value
             || has_const_front_helper<Type>::value
             || has_const_data_helper<Type>::value
             || has_const_begin_helper<Type>::value>
    {};

    // --- write surface: mutation on a NON-CONST instance ---

    // has_push_back_helper / has_emplace_back_helper / has_push_front_helper
    //   helper: detects an append mutator callable with a value_type.
    template<typename Type, typename = void>
    struct has_push_back_helper : std::false_type
    {};
    template<typename Type>
    struct has_push_back_helper<Type,
        D_VOID_T<decltype(std::declval<clean_t<Type>&>().push_back(
                     std::declval<typename clean_t<Type>::value_type>()))>>
        : std::true_type
    {};

    template<typename Type, typename = void>
    struct has_emplace_back_helper : std::false_type
    {};
    template<typename Type>
    struct has_emplace_back_helper<Type,
        D_VOID_T<decltype(std::declval<clean_t<Type>&>().emplace_back(
                     std::declval<typename clean_t<Type>::value_type>()))>>
        : std::true_type
    {};

    template<typename Type, typename = void>
    struct has_push_front_helper : std::false_type
    {};
    template<typename Type>
    struct has_push_front_helper<Type,
        D_VOID_T<decltype(std::declval<clean_t<Type>&>().push_front(
                     std::declval<typename clean_t<Type>::value_type>()))>>
        : std::true_type
    {};

    // has_clear_helper
    //   helper: detects clear() - a no-arg mutator present on every growable
    // standard container (sequence and associative alike), absent on a fixed
    // array.
    template<typename Type, typename = void>
    struct has_clear_helper : std::false_type
    {};
    // has_clear_helper specialization
    //   helper: the detected case -- selected when the type exposes the member
    // function `clear()`.
    template<typename Type>
    struct has_clear_helper<Type,
        D_VOID_T<decltype(std::declval<clean_t<Type>&>().clear())>>
        : std::true_type
    {};

    // has_assignable_index_helper
    //   helper: detects that non-const operator[] yields an ASSIGNABLE lvalue
    // - in-place element mutation. This is what marks a fixed-size std::array
    // (mutable elements, no growth) as writable, and is NOT satisfied by a
    // read_only handle, whose operator[] returns a const reference.
    template<typename Type, typename = void>
    struct has_assignable_index_helper : std::false_type
    {};
    // has_assignable_index_helper<Type,
    // D_VOID_T<decltype(std::declval<clean_t<Type>&>()[
    // std::declval<std::size_t>()] = std::declval<typename clean_t<Type>
    //   trait: the `D_VOID_T<decltype(std::declval<clean_t<Type>&>()[
    // std::declval<std::size_t>()] = std::declval<typename clean_t<Type`
    // case; it reports true.
    template<typename Type>
    struct has_assignable_index_helper<Type,
        D_VOID_T<decltype(std::declval<clean_t<Type>&>()[
                     std::declval<std::size_t>()]
                 = std::declval<typename clean_t<Type>::value_type&>())>>
        : std::true_type
    {};

    // has_write_surface
    //   helper: the type exposes SOME mutator (append or in-place assignment).
    template<typename Type>
    struct has_write_surface
        : std::integral_constant<bool,
                has_push_back_helper<Type>::value
             || has_emplace_back_helper<Type>::value
             || has_push_front_helper<Type>::value
             || has_clear_helper<Type>::value
             || has_assignable_index_helper<Type>::value>
    {};

    // --- (can_read, can_write) resolution: tag first, else structure ---

    template<typename Type, bool HasTag>
    struct access_bits_impl  // HasTag == false: read the structural surface
    {
        static constexpr bool can_read  = has_read_surface<Type>::value;
        static constexpr bool can_write = has_write_surface<Type>::value;
    };

    template<typename Type>
    struct access_bits_impl<Type, true>  // trust the stamped capability tag
    {
        static constexpr bool can_read  = clean_t<Type>::capability::can_read;
        static constexpr bool can_write = clean_t<Type>::capability::can_write;
    };

    template<typename Type>
    struct access_bits
        : access_bits_impl<clean_t<Type>,
              has_capability_bits_helper<clean_t<Type>>::value>
    {};

NS_END  // internal


// ===========================================================================
// II.  Capability classification
// ===========================================================================

// access_capability
//   enum: a type's position on the access axis - the capability a handle
// grants.
enum class access_capability
{
    none,        // neither observable nor mutable (not access-classifiable)
    read_only,   // observation only
    write_only,  // mutation only, no value observation (an append-only sink)
    read_write   // both observation and mutation
};

// access_capability_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
access_capability_name(access_capability _c) noexcept
{
    return ( _c == access_capability::none       ? "none"
           : _c == access_capability::read_only  ? "read_only"
           : _c == access_capability::write_only ? "write_only"
           :                                       "read_write" );
}

// access_capability_from_bits
//   function: the (can_read, can_write) -> capability mapping, shared by the
// classifier and the aggregate.
constexpr access_capability
access_capability_from_bits(bool _can_read, bool _can_write) noexcept
{
    return ( _can_read && _can_write ? access_capability::read_write
           : _can_read               ? access_capability::read_only
           : _can_write              ? access_capability::write_only
           :                           access_capability::none );
}

// access_capability_of
//   trait: classifies a type. A `capability` tag carrying can_read / can_write
// is honoured first (the wrapper's own stamp); otherwise the structural read
// and write surfaces decide.
template<typename Type>
struct access_capability_of
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr bool can_read  =
        internal::access_bits<clean_type>::can_read;
    static constexpr bool can_write =
        internal::access_bits<clean_type>::can_write;

    static constexpr bool has_capability_tag =
        internal::has_capability_bits_helper<clean_type>::value;

    static constexpr access_capability value =
        access_capability_from_bits(can_read, can_write);

    using type = std::integral_constant<access_capability, value>;
};

// access_capability_of_t / access_capability_of_v
//   type / value: the carrier and, where the language permits, the value
// companion of access_capability_of.
template<typename Type>
using access_capability_of_t = typename access_capability_of<Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Type>
    inline constexpr access_capability access_capability_of_v =
        access_capability_of<Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr access_capability access_capability_of_v =
        access_capability_of<Type>::value;
#endif


// ===========================================================================
// III. read_write predicate + tag re-emission
// ===========================================================================

// is_read_write_container
//   trait: true iff a handle to the type grants BOTH observation and mutation
// - the unrestricted baseline of the access lattice.
template<typename Type>
struct is_read_write_container
    : std::integral_constant<bool,
          access_capability_of<clean_t<Type>>::value
              == access_capability::read_write>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_read_write_container)

// access_tag_of
//   trait: re-emits the classification AS the corresponding capability tag
// TYPE (read_write / read_only / write_only), or void for `none` - so a
// classified type can be tag-DISPATCHED downstream, the capability preserved
// for posterity.
NS_INTERNAL

    template<typename Type,
             access_capability K = access_capability_of<clean_t<Type>>::value>
    struct access_tag_dispatch
    {
        using type = void;
    };

    template<typename Type>
    struct access_tag_dispatch<Type, access_capability::read_only>
    {
        using type = read_only;
    };

    template<typename Type>
    struct access_tag_dispatch<Type, access_capability::write_only>
    {
        using type = write_only;
    };

    template<typename Type>
    struct access_tag_dispatch<Type, access_capability::read_write>
    {
        using type = read_write;
    };

NS_END  // internal

template<typename Type>
struct access_tag_of
{
    using type = typename internal::access_tag_dispatch<clean_t<Type>>::type;
};

// access_tag_of_t
//   type: the carrier of access_tag_of -- its `::type`, for use where a type
// rather than a value is wanted.
template<typename Type>
using access_tag_of_t = typename access_tag_of<Type>::type;


// ===========================================================================
// IV.  Aggregate snapshot
// ===========================================================================

template<typename Type>
struct access_container_class
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr bool can_read =
        access_capability_of<clean_type>::can_read;
    static constexpr bool can_write =
        access_capability_of<clean_type>::can_write;
    static constexpr bool has_capability_tag =
        access_capability_of<clean_type>::has_capability_tag;

    static constexpr access_capability kind =
        access_capability_of<clean_type>::value;
    static constexpr const char* kind_name =
        access_capability_name(kind);

    static constexpr bool is_read_only =
        kind == access_capability::read_only;
    static constexpr bool is_write_only =
        kind == access_capability::write_only;
    static constexpr bool is_read_write =
        kind == access_capability::read_write;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TRAITS_READ_WRITE_CONTAINER_TRAITS_HPP
