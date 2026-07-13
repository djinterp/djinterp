/******************************************************************************
* djinterp [meta]                                     dtuple_wrap_partition.hpp
*
*   RECONSTRUCTION NOTICE
*   =====================
*   The original dtuple_wrap_partition.hpp is no longer present in the tree, but
* option_builder.hpp still depends on the `partition_wrap_except` /
* `partition_wrap_except_t` engine it exported.  This header REBUILDS that
* engine from option_builder.hpp's own documented contract and its worked _N=3
* example; it is not the original source.  Its observable behavior matches the
* documented contract:
*
*     partition_wrap_except_t<_Wrap, _N, _IsPassthrough, _Schema...>
*       - groups the schema into chunks of exactly _N consecutive
*         NON-passthrough types;
*       - wraps each chunk via the _Wrap template-template
*         (_Wrap<slot0, slot1, ..., slot_{N-1}>);
*       - leaves passthroughs (types satisfying the unary _IsPassthrough trait)
*         UNWRAPPED at their original positions, which - per the contract - sit
*         at chunk boundaries, never inside a chunk;
*       - accepts the schema as a bare typename pack OR a single std::tuple<...>;
*       - yields a std::tuple<...> of wrapped chunks interleaved with the
*         surviving passthroughs.
*
*   Two malformed-schema conditions are hard errors (a passthrough inside a
* chunk; a trailing run of non-passthroughs that is not a whole multiple of _N),
* mirroring the "fail at the declaration site" discipline of the sized
* partitioner.  If the genuine dtuple_wrap_partition.hpp resurfaces, drop it in
* over this file - the option_builder suite depends only on the surface above.
*
*
* path:      /inc/djinterp/core/meta/dtuple_wrap_partition.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_META_DTUPLE_WRAP_PARTITION_
#define DJINTERP_META_DTUPLE_WRAP_PARTITION_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP

NS_INTERNAL

    // pwe_cat
    //   helper: concatenate two std::tuples at the type level.
    template<typename _L,
             typename _R>
    struct pwe_cat;

    template<typename... _Ls,
             typename... _Rs>
    struct pwe_cat<std::tuple<_Ls...>, std::tuple<_Rs...>>
    {
        using type = std::tuple<_Ls..., _Rs...>;
    };


    // pwe_walk
    //   helper: the left-to-right partition walk.  _Chunk is the std::tuple of
    // non-passthrough slots accumulated for the chunk in progress (always fewer
    // than _N elements between steps).
    template<template<typename...> class _Wrap,
             std::size_t              _N,
             template<typename> class _IsPassthrough,
             typename                 _Chunk,
             typename...              _Schema>
    struct pwe_walk;


    // pwe_dispatch
    //   helper: one step of the walk, keyed on whether the head is a passthrough
    // and whether appending it completes a chunk.
    template<bool                     _HeadIsPassthrough,
             bool                     _ChunkCompletes,
             template<typename...> class _Wrap,
             std::size_t              _N,
             template<typename> class _IsPassthrough,
             typename                 _Chunk,
             typename                 _Head,
             typename...              _Tail>
    struct pwe_dispatch;

    // (1) head is a passthrough: it must be at a chunk boundary (chunk empty).
    //     Emit it unwrapped and continue with a fresh (empty) chunk.
    template<bool                     _ChunkCompletes,
             template<typename...> class _Wrap,
             std::size_t              _N,
             template<typename> class _IsPassthrough,
             typename...              _ChunkTs,
             typename                 _Head,
             typename...              _Tail>
    struct pwe_dispatch<true, _ChunkCompletes, _Wrap, _N, _IsPassthrough,
                        std::tuple<_ChunkTs...>, _Head, _Tail...>
    {
        static_assert(sizeof...(_ChunkTs) == 0,
            "partition_wrap_except: a passthrough must sit at a chunk boundary, "
            "not inside a chunk of _N consecutive non-passthrough slots.");

        using type = typename pwe_cat<
            std::tuple<_Head>,
            typename pwe_walk<_Wrap, _N, _IsPassthrough, std::tuple<>, _Tail...>::type
        >::type;
    };

    // (2) head is a non-passthrough that does NOT complete the chunk: accumulate.
    template<template<typename...> class _Wrap,
             std::size_t              _N,
             template<typename> class _IsPassthrough,
             typename...              _ChunkTs,
             typename                 _Head,
             typename...              _Tail>
    struct pwe_dispatch<false, false, _Wrap, _N, _IsPassthrough,
                        std::tuple<_ChunkTs...>, _Head, _Tail...>
    {
        using type = typename pwe_walk<_Wrap, _N, _IsPassthrough,
                                       std::tuple<_ChunkTs..., _Head>, _Tail...>::type;
    };

    // (3) head is a non-passthrough that COMPLETES the chunk: wrap + emit, reset.
    template<template<typename...> class _Wrap,
             std::size_t              _N,
             template<typename> class _IsPassthrough,
             typename...              _ChunkTs,
             typename                 _Head,
             typename...              _Tail>
    struct pwe_dispatch<false, true, _Wrap, _N, _IsPassthrough,
                        std::tuple<_ChunkTs...>, _Head, _Tail...>
    {
        using type = typename pwe_cat<
            std::tuple< _Wrap<_ChunkTs..., _Head> >,
            typename pwe_walk<_Wrap, _N, _IsPassthrough, std::tuple<>, _Tail...>::type
        >::type;
    };


    // pwe_walk: schema exhausted - the chunk in progress must be empty.
    template<template<typename...> class _Wrap,
             std::size_t              _N,
             template<typename> class _IsPassthrough,
             typename...              _ChunkTs>
    struct pwe_walk<_Wrap, _N, _IsPassthrough, std::tuple<_ChunkTs...>>
    {
        static_assert(sizeof...(_ChunkTs) == 0,
            "partition_wrap_except: the schema ended mid-chunk - the run of "
            "non-passthrough slots is not a whole multiple of _N.");

        using type = std::tuple<>;
    };

    // pwe_walk: at least one schema entry remains - take one step.
    template<template<typename...> class _Wrap,
             std::size_t              _N,
             template<typename> class _IsPassthrough,
             typename...              _ChunkTs,
             typename                 _Head,
             typename...              _Tail>
    struct pwe_walk<_Wrap, _N, _IsPassthrough, std::tuple<_ChunkTs...>, _Head, _Tail...>
        : pwe_dispatch<
              _IsPassthrough<_Head>::value,
              (sizeof...(_ChunkTs) + 1 == _N),
              _Wrap, _N, _IsPassthrough,
              std::tuple<_ChunkTs...>, _Head, _Tail...>
    {};

NS_END  // internal


// partition_wrap_except
//   trait: chunk-and-wrap partition with passthrough survival.  See the file
// header for the full contract.  Accepts a bare typename pack of schema slots.
template<template<typename...> class _Wrap,
         std::size_t              _N,
         template<typename> class _IsPassthrough,
         typename...              _Schema>
struct partition_wrap_except
{
    using type = typename internal::pwe_walk<
        _Wrap, _N, _IsPassthrough, std::tuple<>, _Schema...>::type;
};

// partition_wrap_except  (single std::tuple schema)
//   trait: the source may also be a single std::tuple<...>; it is unwrapped to
// the pack form.  More specialized than the primary, so a lone tuple routes
// here while a pack (even a pack that happens to contain tuples) uses the
// primary.
template<template<typename...> class _Wrap,
         std::size_t              _N,
         template<typename> class _IsPassthrough,
         typename...              _Inner>
struct partition_wrap_except<_Wrap, _N, _IsPassthrough, std::tuple<_Inner...>>
{
    using type = typename internal::pwe_walk<
        _Wrap, _N, _IsPassthrough, std::tuple<>, _Inner...>::type;
};

// partition_wrap_except_t
//   alias: convenience for partition_wrap_except<...>::type.
template<template<typename...> class _Wrap,
         std::size_t              _N,
         template<typename> class _IsPassthrough,
         typename...              _Schema>
using partition_wrap_except_t =
    typename partition_wrap_except<_Wrap, _N, _IsPassthrough, _Schema...>::type;


NS_END  // djinterp


#endif  // DJINTERP_META_DTUPLE_WRAP_PARTITION_
