/*******************************************************************************
* djinterp [core]                                            tuple_partition.hpp
*
* djinterp tuple partitioning module:
*   Slicing a flat parameter pack or tuple into a sequence of sub-tuples
* of specified sizes, with optional remainder.  Three orthogonal axes:
*
*   1. SIZING
*        sized        — uniform chunk size N.
*        unsuffixed   — variable sizes via `sizes<N1, N2, ...>` wrapper.
*
*   2. LEVEL
*        type-level   — traits producing tuple types from type packs.
*        value-level  — constexpr functions producing tuple instances.
*
*   3. VALUE-LEVEL OWNERSHIP
*        copy_*       — non-destructive.  Reads from `const T&` input;
*                       result is a fresh tuple of (copies of) the
*                       sliced elements.  Original is untouched.
*        partition_*  — destructive.  Reads from `T&&` input; elements
*                       are MOVED into the result.  Original is left
*                       in a moved-from state.
*
*   At the type level there is only one direction (a trait can't
* mutate a type), so the type-level surface is just `partition_tuples`
* and `partition_tuples_sized`.  The `copy_*` naming applies only to
* the value-level functions.
*
*   SOURCE FORM:
*   Every trait and every function accepts the source as either a
* `std::tuple<Ts...>` or a bare `Ts...` variadic pack.  Partial
* specializations on `std::tuple` route to the underlying pack form.
*
*   REMAINDER:
*   When the sizes consume fewer elements than the source, the leftover
* elements form a final remainder tuple appended to the result.  The
* remainder tuple is ALWAYS emitted, including as an empty `std::tuple<>`
* when sizes consume the source exactly.  This guarantees a uniform
* output shape: `partition_tuples_sized_t<2, A,B,C,D>` produces
* `tuple<tuple<A,B>, tuple<C,D>, tuple<>>` — three tuples regardless of
* whether the partition is "exact" or "leaves leftover."
*
*   STATIC ASSERTS:
*   Every form static_asserts that the sum of requested sizes does not
* exceed the source size, so over-partitioning fails at the declaration
* site rather than producing nonsense.
*
*   HISTORY:
*   This header supersedes `dtuple_chunks.hpp`.  The replacements are:
*       tuple_chunks<N, Ts...>       -> partition_tuples_sized<N, Ts...>
*       chunks<N>(args...)            -> copy_tuples_sized<N>(args...)
*       pack_chunks<N, Vs...>         -> partition_pack_sized<N, Vs...>
*       nttp_chunk                     -> nttp_chunk (unchanged)
*
*   For variable chunk sizes, the new `sizes<...>` and corresponding
* `partition_tuples`, `copy_tuples`, `partition_tuples` (value-level),
* and `partition_pack` traits/functions have no prior equivalent.
*
*
* path:      /inc/djinterp/core/meta/tuple_partition.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.24
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    sizes wrapper
      -------------

II.   Type-level partitioning — variable sizes
      ----------------------------------------
      1.    partition_tuples<Sizes, Source...>
      2.    partition_tuples_t

III.  Type-level partitioning — uniform size
      --------------------------------------
      1.    partition_tuples_sized<N, Source...>
      2.    partition_tuples_sized_t

IV.   Value-level copy — uniform size
      -------------------------------
      1.    copy_tuples_sized<N>(args...)            (pack)
      2.    copy_tuples_sized<N>(tuple)              (tuple)

V.    Value-level copy — variable sizes
      ---------------------------------
      1.    copy_tuples<sizes>(args...)               (pack)
      2.    copy_tuples<sizes>(tuple)                 (tuple)

VI.   Value-level partition — uniform size
      ------------------------------------
      1.    partition_tuples_sized<N>(args&&...)     (pack)
      2.    partition_tuples_sized<N>(tuple&&)       (tuple)

VII.  Value-level partition — variable sizes
      --------------------------------------
      1.    partition_tuples<sizes>(args&&...)        (pack)
      2.    partition_tuples<sizes>(tuple&&)          (tuple)

VIII. Auto-NTTP forms (C++20)
      -----------------------
      1.    nttp_chunk<Vs...>
      2.    partition_pack_sized<N, Vs...>
      3.    partition_pack<Sizes, Vs...>
*/

#ifndef DJINTERP_META_TUPLE_PARTITION_HPP
#define DJINTERP_META_TUPLE_PARTITION_HPP 1

// THE MODULE FLOOR is C++14 (std::index_sequence and variable templates),
// and below it this header is EMPTY rather than an error (README rule 5:
// a facility is absent from a level it cannot express). env comes first,
// as the floor reads it.
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP14_OR_HIGHER
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP


// ===========================================================================
// I.   sizes wrapper
// ===========================================================================

// sizes
//   alias: explicit boundary between chunk sizes and types in
// `partition_tuples<...>` and value-level analogues.  Alias for
// `std::index_sequence` so callers can use stdlib utilities to
// construct or manipulate size lists.
template<std::size_t... Ns>
using sizes = std::index_sequence<Ns...>;


// ===========================================================================
// II.  Type-level partitioning — variable sizes
// ===========================================================================

NS_INTERNAL

    // type_pack_split
    //   helper: peels the first Take types from a tuple-of-types
    // and exposes the remainder as a tail tuple.
    //
    // Implementation: rather than recursive specializations (which
    // have a partial-ordering ambiguity when Take == 0), this uses
    // std::tuple_element + std::index_sequence to project the head
    // and tail tuples in one shot.

    // tail_indices
    //   helper: produces the index_sequence [Take, Take+1, ...].
    template<std::size_t Offset,
             typename    Seq>
    struct shift_seq;

    template<std::size_t Offset,
             std::size_t... Is>
    struct shift_seq<Offset, std::index_sequence<Is...>>
    {
        using type = std::index_sequence<(Is + Offset)...>;
    };

    template<std::size_t Offset,
             typename    Seq>
    using shift_seq_t = typename shift_seq<Offset, Seq>::type;

    // project_tuple
    //   helper: given a source `std::tuple<Ts...>` and an
    // index_sequence, produces a `std::tuple<Ts[Is]...>`.
    template<typename SourceTuple,
             typename Seq>
    struct project_tuple;

    template<typename... Ts,
             std::size_t... Is>
    struct project_tuple<std::tuple<Ts...>,
                         std::index_sequence<Is...>>
    {
        using type = std::tuple<
            typename std::tuple_element<Is,
                std::tuple<Ts...>>::type...>;
    };

    template<typename SourceTuple,
             typename Seq>
    using project_tuple_t =
        typename project_tuple<SourceTuple, Seq>::type;

    // type_pack_split
    //   helper: head/tail split of a tuple-of-types at offset Take.
    template<std::size_t Take,
             typename    Ignored,    // kept for back-compat in callers
             typename... Source>
    struct type_pack_split
    {
    private:
        using source_tuple = std::tuple<Source...>;

        using head_seq = std::make_index_sequence<Take>;
        using tail_seq = shift_seq_t<
            Take,
            std::make_index_sequence<sizeof...(Source) - Take>>;

    public:
        using head = project_tuple_t<source_tuple, head_seq>;
        using tail = project_tuple_t<source_tuple, tail_seq>;
    };

    // partition_var_recurse
    //   helper: recursive driver for variable-sizes partitioning.
    // Walks the sizes pack, peeling one chunk of the requested
    // length each step and accumulating into the result.
    //
    // When the sizes pack is exhausted, the remaining (possibly
    // empty) tail tuple is appended as the final remainder slot.
    template<typename    SizesRemaining,
             typename    Acc,
             typename... SourceRemaining>
    struct partition_var_recurse;

    // base case: no sizes left, append remainder tuple
    template<typename... AccTuples,
             typename... Remainder>
    struct partition_var_recurse<
        std::index_sequence<>,
        std::tuple<AccTuples...>,
        Remainder...>
    {
        using type = std::tuple<AccTuples...,
                                std::tuple<Remainder...>>;
    };

    // step: peel one size off, slice source, recurse
    template<std::size_t N,
             std::size_t... RestSizes,
             typename...    AccTuples,
             typename...    SourceRemaining>
    struct partition_var_recurse<
        std::index_sequence<N, RestSizes...>,
        std::tuple<AccTuples...>,
        SourceRemaining...>
    {
    private:
        // dispatch the head/tail split through a helper that
        // gracefully handles N == 0 (empty chunk).
        template<std::size_t Take,
                 typename... Pack>
        struct split_at
        {
            using inner = type_pack_split<Take,
                                          std::tuple<>,
                                          Pack...>;
            using head  = typename inner::head;
            using tail  = typename inner::tail;
        };

        using s = split_at<N, SourceRemaining...>;

        template<typename Tail>
        struct continue_with;

        template<typename... TailTypes>
        struct continue_with<std::tuple<TailTypes...>>
        {
            using type = typename partition_var_recurse<
                std::index_sequence<RestSizes...>,
                std::tuple<AccTuples..., typename s::head>,
                TailTypes...>::type;
        };

    public:
        using type =
            typename continue_with<typename s::tail>::type;
    };

    // sum_sizes
    //   helper: compile-time sum of a size pack, for the
    // static_assert that catches over-partitioning early.
    template<std::size_t... Ns>
    struct sum_sizes
        : std::integral_constant<std::size_t, 0>
    {};

    template<std::size_t First,
             std::size_t... Rest>
    struct sum_sizes<First, Rest...>
        : std::integral_constant<std::size_t,
            First + sum_sizes<Rest...>::value>
    {};

NS_END  // internal


// partition_tuples
//   trait: partitions a type pack (or tuple) into N sub-tuples of
// the sizes specified in `Sizes`, plus a final remainder tuple.
//
// Source forms:
//   partition_tuples<sizes<1, 4, 2>, A, B, C, D, E, F, G, H>
//   partition_tuples<sizes<1, 4, 2>, std::tuple<A,B,C,D,E,F,G,H>>
//
// Result (both forms):
//   std::tuple<std::tuple<A>,
//              std::tuple<B, C, D, E>,
//              std::tuple<F, G>,
//              std::tuple<H>>            // remainder
//
// Static-asserts the sum of sizes does not exceed the source length.
template<typename Sizes,
         typename... Source>
struct partition_tuples_trait;

// primary: variadic pack form
template<std::size_t... Ns,
         typename...    Source>
struct partition_tuples_trait<sizes<Ns...>, Source...>
{
    static_assert(
        internal::sum_sizes<Ns...>::value <= sizeof...(Source),
        "partition_tuples: sum of requested sizes exceeds source "
        "length.");

    using type = typename internal::partition_var_recurse<
        std::index_sequence<Ns...>,
        std::tuple<>,
        Source...>::type;
};

// tuple form: routes to the pack form by unwrapping std::tuple
template<std::size_t... Ns,
         typename... Inner>
struct partition_tuples_trait<sizes<Ns...>, std::tuple<Inner...>>
    : partition_tuples_trait<sizes<Ns...>, Inner...>
{};

// partition_tuples_t
//   alias: shorthand for partition_tuples<...>::type.
template<typename Sizes,
         typename... Source>
using partition_tuples_t =
    typename partition_tuples_trait<Sizes, Source...>::type;


// ===========================================================================
// III. Type-level partitioning — uniform size
// ===========================================================================

NS_INTERNAL

    // make_uniform_sizes
    //   helper: produces an index_sequence of `Count` copies of
    // `N`.  Used to lower `partition_tuples_sized<N, src>` onto
    // the variable-sizes machinery.

    template<std::size_t    N,
             typename       Acc,
             std::size_t    Remaining>
    struct make_uniform_sizes_impl;

    template<std::size_t N,
             std::size_t... Acc>
    struct make_uniform_sizes_impl<N,
                                   std::index_sequence<Acc...>,
                                   0>
    {
        using type = std::index_sequence<Acc...>;
    };

    template<std::size_t N,
             std::size_t... Acc,
             std::size_t    Remaining>
    struct make_uniform_sizes_impl<N,
                                   std::index_sequence<Acc...>,
                                   Remaining>
        : make_uniform_sizes_impl<N,
                                  std::index_sequence<Acc..., N>,
                                  Remaining - 1>
    {};

    // make_uniform_sizes_t
    //   alias: index_sequence of `Count` copies of `N`.
    template<std::size_t N,
             std::size_t Count>
    using make_uniform_sizes_t = typename make_uniform_sizes_impl<
        N, std::index_sequence<>, Count>::type;

NS_END  // internal


// partition_tuples_sized
//   trait: like `partition_tuples` but every chunk is the same size
// `N`.  Lowers onto `partition_tuples<sizes<N, N, ..., N>, ...>`
// internally.
//
// Source forms:
//   partition_tuples_sized<3, A, B, C, D, E, F, G>
//   partition_tuples_sized<3, std::tuple<A,B,C,D,E,F,G>>
//
// Result (both forms):
//   std::tuple<std::tuple<A, B, C>,
//              std::tuple<D, E, F>,
//              std::tuple<G>>            // remainder
//
// Note: if `N == 0` the partition would be infinite; static_assert
// rejects this.
template<std::size_t N,
         typename... Source>
struct partition_tuples_sized_trait;

// primary: variadic pack form
template<std::size_t N,
         typename... Source>
struct partition_tuples_sized_trait
{
    static_assert(N > 0,
                  "partition_tuples_sized: chunk size N must be > 0.");

private:
    static constexpr std::size_t total = sizeof...(Source);
    static constexpr std::size_t count = total / N;

public:
    using type = partition_tuples_t<
        internal::make_uniform_sizes_t<N, count>,
        Source...>;
};

// tuple form: routes to the pack form
template<std::size_t N,
         typename... Inner>
struct partition_tuples_sized_trait<N, std::tuple<Inner...>>
    : partition_tuples_sized_trait<N, Inner...>
{};

// partition_tuples_sized_t
//   alias: shorthand for partition_tuples_sized<...>::type.
template<std::size_t N,
         typename... Source>
using partition_tuples_sized_t =
    typename partition_tuples_sized_trait<N, Source...>::type;


// ===========================================================================
// IV.  Value-level copy — uniform size
// ===========================================================================
//   Reads from a const-reference source, produces a fresh tuple-of-
// tuples.  The source is untouched.  Decayed types are used in the
// result so that lifetime issues at the call site become obvious.

NS_INTERNAL

    // slice_copy
    //   helper: builds one inner tuple by COPYING elements from
    // the source tuple at offset `Base + 0 ... Base + N - 1`.
    template<std::size_t    Base,
             typename       SourceTuple,
             std::size_t... Is>
    D_CONSTEXPR auto
    slice_copy_at(
        const SourceTuple& _src,
        std::index_sequence<Is...>
    )
    {
        return std::make_tuple(std::get<Base + Is>(_src)...);
    }

    // ---------------------------------------------------------------
    //  uniform-size copy driver
    // ---------------------------------------------------------------

    template<std::size_t    N,
             std::size_t    Total,
             typename       SourceTuple,
             std::size_t... OuterIs>
    D_CONSTEXPR auto
    copy_uniform_outer(
        const SourceTuple& _src,
        std::index_sequence<OuterIs...>
    )
    {
        constexpr std::size_t count     = Total / N;
        constexpr std::size_t remainder = Total - (count * N);
        // Append the remainder tuple at the end.  The remainder
        // is sliced from offset (count * N) for `remainder` elements.
        return std::tuple_cat(
            std::make_tuple(
                slice_copy_at<OuterIs * N>(
                    _src,
                    std::make_index_sequence<N>{})...),
            std::make_tuple(
                slice_copy_at<count * N>(
                    _src,
                    std::make_index_sequence<remainder>{})));
    }

NS_END  // internal


// copy_tuples_sized
//   function (pack form): copies a runtime parameter pack into a
// std::tuple-of-tuples of uniform inner size `N`, appending a
// (possibly empty) remainder tuple.
//
// Example:
//   copy_tuples_sized<2>(a, b, c, d, e)
//   -> tuple<tuple<A,B>, tuple<C,D>, tuple<E>>
template<std::size_t N,
         typename... Args>
D_CONSTEXPR auto
copy_tuples_sized(
    const Args&... _args
)
{
    static_assert(N > 0,
                  "copy_tuples_sized: chunk size N must be > 0.");

    constexpr std::size_t total = sizeof...(Args);
    constexpr std::size_t count = total / N;

    // Build a source tuple by copying the arguments, then dispatch
    // through the index-driven slicer.
    auto src = std::make_tuple(_args...);

    return internal::copy_uniform_outer<N, total>(
        src,
        std::make_index_sequence<count>{});
}

// copy_tuples_sized (tuple form)
//   function: copies elements from an existing tuple into a
// tuple-of-tuples of uniform inner size `N`.
template<std::size_t N,
         typename... Inner>
D_CONSTEXPR auto
copy_tuples_sized(
    const std::tuple<Inner...>& _tuple
)
{
    static_assert(N > 0,
                  "copy_tuples_sized: chunk size N must be > 0.");

    constexpr std::size_t total = sizeof...(Inner);
    constexpr std::size_t count = total / N;

    return internal::copy_uniform_outer<N, total>(
        _tuple,
        std::make_index_sequence<count>{});
}


// ===========================================================================
// V.   Value-level copy — variable sizes
// ===========================================================================

NS_INTERNAL

    // variable_offsets
    //   helper: computes the prefix-sum offsets for a sizes pack
    // ENTIRELY AT THE TYPE LEVEL so we don't depend on local
    // constexpr arrays being readable as template arguments
    // (which is permissive in C++20 but historically finicky).
    //
    //   Given sizes<N1, N2, N3>, produces the offset sequence
    // <0, N1, N1+N2>.  Note this is *not* the cumulative sum
    // including the final element — it gives us the START offset
    // for each chunk index 0..(count-1), which is what
    // copy_var_one / move_var_one need.

    // sum_first_n
    //   trait: sum of the first `N` values in a size pack.
    //
    // Implementation: a bool discriminator (N == 0) routes between
    // the base case and the recursive case so partial ordering is
    // unambiguous when N == 0 with a non-empty pack.

    template<bool IsZero,
             std::size_t N,
             std::size_t... Ns>
    struct sum_first_n_dispatch;

    // base case: take 0 elements -> sum is 0 regardless of pack
    template<std::size_t N,
             std::size_t... Ns>
    struct sum_first_n_dispatch<true, N, Ns...>
        : std::integral_constant<std::size_t, 0>
    {};

    // recursive case: take N > 0 elements
    template<std::size_t N,
             std::size_t First,
             std::size_t... Rest>
    struct sum_first_n_dispatch<false, N, First, Rest...>
        : std::integral_constant<std::size_t,
            First + sum_first_n_dispatch<(N - 1 == 0),
                                           N - 1,
                                           Rest...>::value>
    {};

    template<std::size_t N,
             std::size_t... Ns>
    using sum_first_n = sum_first_n_dispatch<(N == 0), N, Ns...>;

    // nth_size
    //   trait: the `I`-th element of a size pack.
    template<std::size_t I,
             std::size_t First,
             std::size_t... Rest>
    struct nth_size
        : nth_size<I - 1, Rest...>
    {};

    template<std::size_t First,
             std::size_t... Rest>
    struct nth_size<0, First, Rest...>
        : std::integral_constant<std::size_t, First>
    {};

    // copy_var_one
    //   helper: builds one inner tuple of length `N` starting at
    // a compile-time-known `Offset` in the source tuple.
    template<std::size_t    Offset,
             std::size_t    N,
             typename       SourceTuple,
             std::size_t... Is>
    D_CONSTEXPR auto
    copy_var_one(
        const SourceTuple& _src,
        std::index_sequence<Is...>
    )
    {
        return std::make_tuple(
            std::get<Offset + Is>(_src)...);
    }

    // copy_var_one_chunk
    //   helper: extracts the `ChunkIndex`-th chunk for the given
    // sizes pack.  Wraps copy_var_one with the offset/length
    // resolution done at the type level.
    template<std::size_t ChunkIndex,
             typename    SourceTuple,
             std::size_t... Ns>
    D_CONSTEXPR auto
    copy_var_one_chunk(
        const SourceTuple& _src,
        std::index_sequence<Ns...>
    )
    {
        constexpr std::size_t offset =
            sum_first_n<ChunkIndex, Ns...>::value;
        constexpr std::size_t length =
            nth_size<ChunkIndex, Ns...>::value;

        return copy_var_one<offset, length>(
            _src,
            std::make_index_sequence<length>{});
    }

    // copy_var_outer
    //   helper: produces the outer tuple of (one tuple per chunk
    // size + a remainder tuple).  Walks the sizes pack via the
    // outer index sequence.
    template<std::size_t    Total,
             typename       SourceTuple,
             std::size_t... Ns,
             std::size_t... Is>
    D_CONSTEXPR auto
    copy_var_outer(
        const SourceTuple& _src,
        std::index_sequence<Ns...> _sz,
        std::index_sequence<Is...>
    )
    {
        constexpr std::size_t consumed = sum_sizes<Ns...>::value;
        constexpr std::size_t remainder = Total - consumed;

        return std::tuple_cat(
            std::make_tuple(
                copy_var_one_chunk<Is>(_src, _sz)...),
            std::make_tuple(
                copy_var_one<consumed, remainder>(
                    _src,
                    std::make_index_sequence<remainder>{})));
    }

NS_END  // internal


// copy_tuples
//   function (pack form): copies a runtime parameter pack into a
// tuple-of-tuples with chunks of the requested variable sizes.
//
// Example:
//   copy_tuples(sizes<1, 4, 2>{}, a, b, c, d, e, f, g, h)
//   -> tuple<tuple<A>,
//            tuple<B, C, D, E>,
//            tuple<F, G>,
//            tuple<H>>                    // remainder
template<std::size_t... Ns,
         typename... Args>
D_CONSTEXPR auto
copy_tuples(
    sizes<Ns...>,
    const Args&... _args
)
{
    static_assert(
        internal::sum_sizes<Ns...>::value <= sizeof...(Args),
        "copy_tuples: sum of requested sizes exceeds argument count.");

    auto src = std::make_tuple(_args...);

    return internal::copy_var_outer<sizeof...(Args)>(
        src,
        std::index_sequence<Ns...>{},
        std::make_index_sequence<sizeof...(Ns)>{});
}

// copy_tuples (tuple form)
template<std::size_t... Ns,
         typename... Inner>
D_CONSTEXPR auto
copy_tuples(
    sizes<Ns...>,
    const std::tuple<Inner...>& _tuple
)
{
    static_assert(
        internal::sum_sizes<Ns...>::value <= sizeof...(Inner),
        "copy_tuples: sum of requested sizes exceeds tuple size.");

    return internal::copy_var_outer<sizeof...(Inner)>(
        _tuple,
        std::index_sequence<Ns...>{},
        std::make_index_sequence<sizeof...(Ns)>{});
}


// ===========================================================================
// VI.  Value-level partition — uniform size
// ===========================================================================
//   Destructive overloads: rvalue-reference input, elements are MOVED
// into the result.  Same shapes as the copy_* counterparts.

NS_INTERNAL

    // slice_move_at
    //   helper: MOVES elements from the source tuple at compile-time
    // offsets into a new inner tuple.
    template<std::size_t    Base,
             typename       SourceTuple,
             std::size_t... Is>
    D_CONSTEXPR auto
    slice_move_at(
        SourceTuple&& _src,
        std::index_sequence<Is...>
    )
    {
        return std::make_tuple(
            std::move(
                std::get<Base + Is>(_src))...);
    }

    template<std::size_t    N,
             std::size_t    Total,
             typename       SourceTuple,
             std::size_t... OuterIs>
    D_CONSTEXPR auto
    move_uniform_outer(
        SourceTuple&& _src,
        std::index_sequence<OuterIs...>
    )
    {
        constexpr std::size_t count     = Total / N;
        constexpr std::size_t remainder = Total - (count * N);

        return std::tuple_cat(
            std::make_tuple(
                slice_move_at<OuterIs * N>(
                    static_cast<SourceTuple&&>(_src),
                    std::make_index_sequence<N>{})...),
            std::make_tuple(
                slice_move_at<count * N>(
                    static_cast<SourceTuple&&>(_src),
                    std::make_index_sequence<remainder>{})));
    }

NS_END  // internal


// partition_tuples_sized
//   function (pack form): MOVES a runtime parameter pack into a
// tuple-of-tuples of uniform inner size `N`.  The inputs are
// taken as rvalue references and consumed.
//
// Example:
//   auto result = partition_tuples_sized<2>(
//       std::move(a), std::move(b), std::move(c), std::move(d));
template<std::size_t N,
         typename... Args>
D_CONSTEXPR auto
partition_tuples_sized(
    Args&&... _args
)
{
    static_assert(N > 0,
                  "partition_tuples_sized: chunk size N must be > 0.");

    constexpr std::size_t total = sizeof...(Args);
    constexpr std::size_t count = total / N;

    auto src = std::forward_as_tuple(
        static_cast<Args&&>(_args)...);

    return internal::move_uniform_outer<N, total>(
        std::move(src),
        std::make_index_sequence<count>{});
}

// partition_tuples_sized (tuple form)
//   function: MOVES elements out of an rvalue tuple into a
// tuple-of-tuples of uniform inner size `N`.  The source tuple
// is left in a moved-from state.
template<std::size_t N,
         typename... Inner>
D_CONSTEXPR auto
partition_tuples_sized(
    std::tuple<Inner...>&& _tuple
)
{
    static_assert(N > 0,
                  "partition_tuples_sized: chunk size N must be > 0.");

    constexpr std::size_t total = sizeof...(Inner);
    constexpr std::size_t count = total / N;

    return internal::move_uniform_outer<N, total>(
        std::move(_tuple),
        std::make_index_sequence<count>{});
}


// ===========================================================================
// VII. Value-level partition — variable sizes
// ===========================================================================

NS_INTERNAL

    // move_var_one
    //   helper: MOVES `N` elements from the source tuple starting
    // at compile-time `Offset` into a new inner tuple.
    template<std::size_t    Offset,
             std::size_t    N,
             typename       SourceTuple,
             std::size_t... Is>
    D_CONSTEXPR auto
    move_var_one(
        SourceTuple&& _src,
        std::index_sequence<Is...>
    )
    {
        return std::make_tuple(
            std::move(
                std::get<Offset + Is>(_src))...);
    }

    // move_var_one_chunk
    //   helper: extracts the `ChunkIndex`-th chunk (destructively)
    // for the given sizes pack.  Uses the type-level offset helpers
    // defined above (sum_first_n, nth_size) shared with copy_*.
    template<std::size_t ChunkIndex,
             typename    SourceTuple,
             std::size_t... Ns>
    D_CONSTEXPR auto
    move_var_one_chunk(
        SourceTuple&& _src,
        std::index_sequence<Ns...>
    )
    {
        constexpr std::size_t offset =
            sum_first_n<ChunkIndex, Ns...>::value;
        constexpr std::size_t length =
            nth_size<ChunkIndex, Ns...>::value;

        return move_var_one<offset, length>(
            static_cast<SourceTuple&&>(_src),
            std::make_index_sequence<length>{});
    }

    template<std::size_t    Total,
             typename       SourceTuple,
             std::size_t... Ns,
             std::size_t... Is>
    D_CONSTEXPR auto
    move_var_outer(
        SourceTuple&& _src,
        std::index_sequence<Ns...> _sz,
        std::index_sequence<Is...>
    )
    {
        constexpr std::size_t consumed = sum_sizes<Ns...>::value;
        constexpr std::size_t remainder = Total - consumed;

        return std::tuple_cat(
            std::make_tuple(
                move_var_one_chunk<Is>(
                    static_cast<SourceTuple&&>(_src),
                    _sz)...),
            std::make_tuple(
                move_var_one<consumed, remainder>(
                    static_cast<SourceTuple&&>(_src),
                    std::make_index_sequence<remainder>{})));
    }

NS_END  // internal


// partition_tuples
//   function (pack form): MOVES a runtime parameter pack into a
// tuple-of-tuples with chunks of the requested variable sizes.
template<std::size_t... Ns,
         typename... Args>
D_CONSTEXPR auto
partition_tuples(
    sizes<Ns...>,
    Args&&... _args
)
{
    static_assert(
        internal::sum_sizes<Ns...>::value <= sizeof...(Args),
        "partition_tuples: sum of requested sizes exceeds argument "
        "count.");

    auto src = std::forward_as_tuple(
        static_cast<Args&&>(_args)...);

    return internal::move_var_outer<sizeof...(Args)>(
        std::move(src),
        std::index_sequence<Ns...>{},
        std::make_index_sequence<sizeof...(Ns)>{});
}

// partition_tuples (tuple form)
template<std::size_t... Ns,
         typename... Inner>
D_CONSTEXPR auto
partition_tuples(
    sizes<Ns...>,
    std::tuple<Inner...>&& _tuple
)
{
    static_assert(
        internal::sum_sizes<Ns...>::value <= sizeof...(Inner),
        "partition_tuples: sum of requested sizes exceeds tuple size.");

    return internal::move_var_outer<sizeof...(Inner)>(
        std::move(_tuple),
        std::index_sequence<Ns...>{},
        std::make_index_sequence<sizeof...(Ns)>{});
}


// ===========================================================================
// VIII. Auto-NTTP forms (C++20)
// ===========================================================================
//   The auto-NTTP analogue of the variable / uniform forms.  Source
// is a flat `auto... Vs` pack; each chunk becomes an `nttp_chunk<...>`
// preserving its values as non-type template parameters.

#if D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO

    // nttp_chunk
    //   carrier: a chunk of auto NTTPs preserved as a type.
    // Element access via `nttp_chunk<...>::get<I>()`.
    template<auto... Vs>
    struct nttp_chunk
    {
        static constexpr std::size_t size = sizeof...(Vs);

        template<std::size_t I>
        static constexpr auto get() noexcept
        {
            return std::get<I>(std::tuple{Vs...});
        }
    };


    NS_INTERNAL

        // nttp_pack_split
        //   helper: peels the first `Take` NTTPs from an auto pack
        // into an nttp_chunk and exposes the tail via a continuation
        // template so further metaprogramming can keep the values
        // as auto NTTPs (not types).
        //
        // Implementation: bool discriminator (Take == 0) disambiguates
        // the base case from the recursive case.

        template<bool        IsZero,
                 std::size_t Take,
                 typename    TakenChunk,
                 auto...     Rest>
        struct nttp_pack_split_dispatch;

        // base case: done taking
        template<std::size_t Take,
                 auto...     Taken,
                 auto...     Rest>
        struct nttp_pack_split_dispatch<true, Take,
                                        nttp_chunk<Taken...>,
                                        Rest...>
        {
            using head = nttp_chunk<Taken...>;

            template<template<auto...> typename Cont>
            using tail_apply = Cont<Rest...>;
        };

        // recursive case: still taking
        template<std::size_t Take,
                 auto...     Taken,
                 auto        First,
                 auto...     Rest>
        struct nttp_pack_split_dispatch<false, Take,
                                        nttp_chunk<Taken...>,
                                        First, Rest...>
            : nttp_pack_split_dispatch<(Take - 1 == 0),
                                       Take - 1,
                                       nttp_chunk<Taken..., First>,
                                       Rest...>
        {};

        template<std::size_t Take,
                 typename    TakenChunk,
                 auto...     Rest>
        using nttp_pack_split =
            nttp_pack_split_dispatch<(Take == 0), Take,
                                     TakenChunk, Rest...>;

        // partition_pack_var_recurse
        //   helper: recursive driver for variable-sized NTTP
        // partitioning.  Mirrors `partition_var_recurse` but
        // operates on auto NTTPs throughout.
        template<typename SizesRemaining,
                 typename Acc,
                 auto...  PackRemaining>
        struct partition_pack_var_recurse;

        // base case: emit remainder
        template<typename... AccChunks,
                 auto...     Remainder>
        struct partition_pack_var_recurse<
            std::index_sequence<>,
            std::tuple<AccChunks...>,
            Remainder...>
        {
            using type = std::tuple<AccChunks...,
                                    nttp_chunk<Remainder...>>;
        };

        // step: peel one chunk
        template<std::size_t N,
                 std::size_t... RestSizes,
                 typename...    AccChunks,
                 auto...        PackRemaining>
        struct partition_pack_var_recurse<
            std::index_sequence<N, RestSizes...>,
            std::tuple<AccChunks...>,
            PackRemaining...>
        {
        private:
            using split = nttp_pack_split<N,
                                          nttp_chunk<>,
                                          PackRemaining...>;
            using one_chunk = typename split::head;

            template<auto... Tail>
            using cont = partition_pack_var_recurse<
                std::index_sequence<RestSizes...>,
                std::tuple<AccChunks..., one_chunk>,
                Tail...>;

        public:
            using type =
                typename split::template tail_apply<cont>::type;
        };

    NS_END  // internal


    // partition_pack
    //   trait: variable-sized NTTP partitioning.  Result is a
    // std::tuple of nttp_chunk types — one per requested size,
    // plus a final remainder nttp_chunk.
    //
    // Example:
    //   using r = partition_pack_t<sizes<1, 2>, 10, 20, 30, 40, 50>;
    //   // -> std::tuple<nttp_chunk<10>,
    //   //               nttp_chunk<20, 30>,
    //   //               nttp_chunk<40, 50>>   // remainder
    template<typename Sizes,
             auto...  Vs>
    struct partition_pack;

    template<std::size_t... Ns,
             auto...        Vs>
    struct partition_pack<sizes<Ns...>, Vs...>
    {
        static_assert(
            internal::sum_sizes<Ns...>::value <= sizeof...(Vs),
            "partition_pack: sum of requested sizes exceeds pack "
            "length.");

        using type = typename internal::partition_pack_var_recurse<
            std::index_sequence<Ns...>,
            std::tuple<>,
            Vs...>::type;
    };

    // partition_pack_t
    //   alias: shorthand for partition_pack<...>::type.
    template<typename Sizes,
             auto...  Vs>
    using partition_pack_t =
        typename partition_pack<Sizes, Vs...>::type;


    // partition_pack_sized
    //   trait: uniform-sized NTTP partitioning.  Same as
    // `partition_pack` but every chunk is the same size `N`.
    //
    // Replaces the previous `pack_chunks` from dtuple_chunks.hpp.
    template<std::size_t N,
             auto...     Vs>
    struct partition_pack_sized
    {
        static_assert(N > 0,
                      "partition_pack_sized: chunk size N must be > 0.");

    private:
        static constexpr std::size_t total = sizeof...(Vs);
        static constexpr std::size_t count = total / N;

    public:
        using type = partition_pack_t<
            internal::make_uniform_sizes_t<N, count>,
            Vs...>;
    };

    // partition_pack_sized_t
    //   alias: shorthand for partition_pack_sized<...>::type.
    template<std::size_t N,
             auto...     Vs>
    using partition_pack_sized_t =
        typename partition_pack_sized<N, Vs...>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


#endif  // DJINTERP_META_TUPLE_PARTITION_HPP
