/*******************************************************************************
* djinterp [core]                                      dtuple_wrap_partition.hpp
*
*   RECONSTRUCTION NOTICE
*   =====================
*   The original dtuple_wrap_partition.hpp is no longer present in the tree, but
* option_builder.hpp still depends on the `partition_wrap_except` /
* `partition_wrap_except_t` engine it exported.  This header REBUILDS that
* engine from option_builder.hpp's own documented contract and its worked N=3
* example; it is not the original source.  Its observable behavior matches the
* documented contract:
*
*     partition_wrap_except_t<Wrap, N, IsPassthrough, Schema...>
*       - groups the schema into chunks of exactly N consecutive
*         NON-passthrough types;
*       - wraps each chunk via the Wrap template-template
*         (Wrap<slot0, slot1, ..., slot_{N-1}>);
*       - leaves passthroughs (types satisfying the unary IsPassthrough trait)
*         UNWRAPPED at their original positions, which - per the contract - sit
*         at chunk boundaries, never inside a chunk;
*       - accepts the schema as a bare typename pack OR a single std::tuple<...>;
*       - yields a std::tuple<...> of wrapped chunks interleaved with the
*         surviving passthroughs.
*
*   Two malformed-schema conditions are hard errors (a passthrough inside a
* chunk; a trailing run of non-passthroughs that is not a whole multiple of N),
* mirroring the "fail at the declaration site" discipline of the sized
* partitioner.  If the genuine dtuple_wrap_partition.hpp resurfaces, drop it in
* over this file - the option_builder suite depends only on the surface above.
*
*
* path:      /inc/djinterp/core/meta/dtuple_wrap_partition.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_META_DTUPLE_WRAP_PARTITION_HPP
#define DJINTERP_META_DTUPLE_WRAP_PARTITION_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP

NS_INTERNAL

    // pwe_cat
    //   helper: concatenate two std::tuples at the type level.
    template<typename L,
             typename R>
    struct pwe_cat;

    template<typename... Ls,
             typename... Rs>
    struct pwe_cat<std::tuple<Ls...>, std::tuple<Rs...>>
    {
        using type = std::tuple<Ls..., Rs...>;
    };


    // pwe_walk
    //   helper: the left-to-right partition walk.  Chunk is the std::tuple of
    // non-passthrough slots accumulated for the chunk in progress (always fewer
    // than N elements between steps).
    template<template<typename...> class Wrap,
             std::size_t              N,
             template<typename> class IsPassthrough,
             typename                 Chunk,
             typename...              Schema>
    struct pwe_walk;


    // pwe_dispatch
    //   helper: one step of the walk, keyed on whether the head is a passthrough
    // and whether appending it completes a chunk.
    template<bool                     HeadIsPassthrough,
             bool                     ChunkCompletes,
             template<typename...> class Wrap,
             std::size_t              N,
             template<typename> class IsPassthrough,
             typename                 Chunk,
             typename                 Head,
             typename...              Tail>
    struct pwe_dispatch;

    // (1) head is a passthrough: it must be at a chunk boundary (chunk empty).
    //     Emit it unwrapped and continue with a fresh (empty) chunk.
    template<bool                     ChunkCompletes,
             template<typename...> class Wrap,
             std::size_t              N,
             template<typename> class IsPassthrough,
             typename...              ChunkTs,
             typename                 Head,
             typename...              Tail>
    struct pwe_dispatch<true, ChunkCompletes, Wrap, N, IsPassthrough,
                        std::tuple<ChunkTs...>, Head, Tail...>
    {
        static_assert(sizeof...(ChunkTs) == 0,
            "partition_wrap_except: a passthrough must sit at a chunk boundary, "
            "not inside a chunk of N consecutive non-passthrough slots.");

        using type = typename pwe_cat<
            std::tuple<Head>,
            typename pwe_walk<Wrap, N, IsPassthrough, std::tuple<>, Tail...>::type
        >::type;
    };

    // (2) head is a non-passthrough that does NOT complete the chunk: accumulate.
    template<template<typename...> class Wrap,
             std::size_t              N,
             template<typename> class IsPassthrough,
             typename...              ChunkTs,
             typename                 Head,
             typename...              Tail>
    struct pwe_dispatch<false, false, Wrap, N, IsPassthrough,
                        std::tuple<ChunkTs...>, Head, Tail...>
    {
        using type = typename pwe_walk<Wrap, N, IsPassthrough,
                                       std::tuple<ChunkTs..., Head>, Tail...>::type;
    };

    // (3) head is a non-passthrough that COMPLETES the chunk: wrap + emit, reset.
    template<template<typename...> class Wrap,
             std::size_t              N,
             template<typename> class IsPassthrough,
             typename...              ChunkTs,
             typename                 Head,
             typename...              Tail>
    struct pwe_dispatch<false, true, Wrap, N, IsPassthrough,
                        std::tuple<ChunkTs...>, Head, Tail...>
    {
        using type = typename pwe_cat<
            std::tuple< Wrap<ChunkTs..., Head> >,
            typename pwe_walk<Wrap, N, IsPassthrough, std::tuple<>, Tail...>::type
        >::type;
    };


    // pwe_walk: schema exhausted - the chunk in progress must be empty.
    template<template<typename...> class Wrap,
             std::size_t              N,
             template<typename> class IsPassthrough,
             typename...              ChunkTs>
    struct pwe_walk<Wrap, N, IsPassthrough, std::tuple<ChunkTs...>>
    {
        static_assert(sizeof...(ChunkTs) == 0,
            "partition_wrap_except: the schema ended mid-chunk - the run of "
            "non-passthrough slots is not a whole multiple of N.");

        using type = std::tuple<>;
    };

    // pwe_walk: at least one schema entry remains - take one step.
    template<template<typename...> class Wrap,
             std::size_t              N,
             template<typename> class IsPassthrough,
             typename...              ChunkTs,
             typename                 Head,
             typename...              Tail>
    struct pwe_walk<Wrap, N, IsPassthrough, std::tuple<ChunkTs...>, Head, Tail...>
        : pwe_dispatch<
              IsPassthrough<Head>::value,
              (sizeof...(ChunkTs) + 1 == N),
              Wrap, N, IsPassthrough,
              std::tuple<ChunkTs...>, Head, Tail...>
    {};

NS_END  // internal


// partition_wrap_except
//   trait: chunk-and-wrap partition with passthrough survival.  See the file
// header for the full contract.  Accepts a bare typename pack of schema slots.
template<template<typename...> class Wrap,
         std::size_t              N,
         template<typename> class IsPassthrough,
         typename...              Schema>
struct partition_wrap_except
{
    using type = typename internal::pwe_walk<
        Wrap, N, IsPassthrough, std::tuple<>, Schema...>::type;
};

// partition_wrap_except  (single std::tuple schema)
//   trait: the source may also be a single std::tuple<...>; it is unwrapped to
// the pack form.  More specialized than the primary, so a lone tuple routes
// here while a pack (even a pack that happens to contain tuples) uses the
// primary.
template<template<typename...> class Wrap,
         std::size_t              N,
         template<typename> class IsPassthrough,
         typename...              Inner>
struct partition_wrap_except<Wrap, N, IsPassthrough, std::tuple<Inner...>>
{
    using type = typename internal::pwe_walk<
        Wrap, N, IsPassthrough, std::tuple<>, Inner...>::type;
};

// partition_wrap_except_t
//   alias: convenience for partition_wrap_except<...>::type.
template<template<typename...> class Wrap,
         std::size_t              N,
         template<typename> class IsPassthrough,
         typename...              Schema>
using partition_wrap_except_t =
    typename partition_wrap_except<Wrap, N, IsPassthrough, Schema...>::type;


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_META_DTUPLE_WRAP_PARTITION_HPP
