/******************************************************************************
* djinterp [test]                             option_builder_tests_pipeline.cpp
*
*   Section III of the option_builder.hpp suite: the end-to-end pipeline.
*
*     option_set_from_flat_t<_N, _Schema...>
*         - partition _Schema into chunks of _N consecutive non-passthrough
*           slots, wrap each chunk as an option (first slot's ::value = key),
*           leave passthroughs in place, and lift the result to an option_set.
*
*   This section drives option_builder's own composition: partition (via the
* wrap-except engine) -> option_partition_wrap per chunk -> tuple_to_option_set.
* pipeline_partition_stage pins the intermediate tuple (the engine using
* option_partition_wrap as its wrapper); the remaining tests pin the final
* option_set for the documented cases - a single chunk, several chunks (the
* header's worked _N=3 example), _N == 1 (each slot its own key-only option), a
* passthrough surviving the partition and then flattening in the set, a single
* std::tuple as the schema source, and the empty schema.
*
*   NB: the partition engine here is the reconstructed dtuple_wrap_partition.hpp
* (see the suite header); §I/§II are option_builder's own code, but these §III
* results depend on that reconstruction matching the documented contract.
*
*
* path:      /tests/djinterp/core/option/option_builder_tests_pipeline.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <tuple>
#include <type_traits>
// djinterp
#include "option_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


// pipeline_partition_stage
//   the partition/wrap stage in isolation: partition_wrap_except_t with
// option_partition_wrap as the wrapper yields the intermediate tuple of
// options, with a passthrough surviving unwrapped at a chunk boundary.
bool
pipeline_partition_stage()
{
    constexpr bool ok =
        std::is_same<
            partition_wrap_except_t<option_partition_wrap, 2, is_passthrough,
                                    ob_key<ob_enum::a>, ob_slot<5>,
                                    ob_key<ob_enum::b>, ob_slot<7>>,
            std::tuple<option<ob_enum::a, ob_slot<5>>,
                       option<ob_enum::b, ob_slot<7>>>>::value                  &&
        std::is_same<
            partition_wrap_except_t<option_partition_wrap, 2, is_passthrough,
                                    ob_key<ob_enum::a>, ob_slot<5>, ob_pass,
                                    ob_key<ob_enum::b>, ob_slot<7>>,
            std::tuple<option<ob_enum::a, ob_slot<5>>,
                       ob_pass,
                       option<ob_enum::b, ob_slot<7>>>>::value;

    static_assert(ok, "partition stage: chunks wrapped via option_partition_wrap; passthrough survives");
    return ok;
}

// pipeline_single_chunk
//   one chunk of _N slots becomes one option in the set.
bool
pipeline_single_chunk()
{
    constexpr bool ok =
        std::is_same<option_set_from_flat_t<2, ob_key<ob_enum::a>, ob_slot<5>>,
                     option_set<option<ob_enum::a, ob_slot<5>>>>::value         &&
        (option_set_from_flat_t<2, ob_key<ob_enum::a>, ob_slot<5>>::size == 1);

    static_assert(ok, "pipeline: one chunk -> one option");
    return ok;
}

// pipeline_multiple_chunks
//   the header's worked _N = 3 example: two chunks of three slots each become
// two options carrying their opaque args.
bool
pipeline_multiple_chunks()
{
    using result = option_set_from_flat_t<3,
        ob_key<ob_enum::a>, ob_slot<5>, ob_desc<1>,
        ob_key<ob_enum::b>, ob_slot<7>, ob_desc<2>>;

    constexpr bool ok =
        std::is_same<result,
            option_set<option<ob_enum::a, ob_slot<5>, ob_desc<1>>,
                       option<ob_enum::b, ob_slot<7>, ob_desc<2>>>>::value      &&
        (result::size == 2);

    static_assert(ok, "pipeline: several chunks -> several options (documented _N=3 example)");
    return ok;
}

// pipeline_n1_key_only
//   with _N == 1 every slot is its own chunk, so each becomes a unary
// (key-only) option.
bool
pipeline_n1_key_only()
{
    constexpr bool ok =
        std::is_same<
            option_set_from_flat_t<1, ob_key<ob_enum::a>, ob_key<ob_enum::b>, ob_key<ob_enum::c>>,
            option_set<option<ob_enum::a>, option<ob_enum::b>, option<ob_enum::c>>>::value;

    static_assert(ok, "pipeline: _N == 1 -> one key-only option per slot");
    return ok;
}

// pipeline_passthrough_survives
//   a passthrough at a chunk boundary survives the partition, then flattens
// away in the final option_set - so the set has only the real options, in
// order, and the passthrough contributes nothing.
bool
pipeline_passthrough_survives()
{
    using result = option_set_from_flat_t<2,
        ob_key<ob_enum::a>, ob_slot<5>, ob_pass, ob_key<ob_enum::b>, ob_slot<7>>;

    constexpr bool ok =
        (result::size == 2)                                                    &&
        std::is_same<result::flat_options_t,
                     std::tuple<option<ob_enum::a, ob_slot<5>>,
                                option<ob_enum::b, ob_slot<7>>>>::value;

    static_assert(ok, "pipeline: a boundary passthrough survives partition, then flattens in the set");
    return ok;
}

// pipeline_tuple_input
//   the schema may be given as a single std::tuple<...> rather than a bare
// pack; it is unwrapped and partitioned identically.
bool
pipeline_tuple_input()
{
    constexpr bool ok =
        std::is_same<
            option_set_from_flat_t<2, std::tuple<ob_key<ob_enum::a>, ob_slot<5>,
                                                 ob_key<ob_enum::b>, ob_slot<7>>>,
            option_set<option<ob_enum::a, ob_slot<5>>, option<ob_enum::b, ob_slot<7>>>>::value;

    static_assert(ok, "pipeline: a single std::tuple schema is accepted like a bare pack");
    return ok;
}

// pipeline_empty_schema
//   an empty schema yields the empty option_set.
bool
pipeline_empty_schema()
{
    constexpr bool ok =
        std::is_same<option_set_from_flat_t<2>, option_set<>>::value           &&
        option_set_from_flat_t<2>::empty;

    static_assert(ok, "pipeline: empty schema -> empty option_set");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_builder_pipeline_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "III. option_set_from_flat_t";
    b.descriptor = "end-to-end: partition + wrap + lift, with passthroughs and tuple/pack input";
    b.tests = {
        { "pipeline_partition_stage",
          "partition stage: chunks wrapped; passthrough survives unwrapped",
          &pipeline_partition_stage },
        { "pipeline_single_chunk",
          "one chunk -> one option",
          &pipeline_single_chunk },
        { "pipeline_multiple_chunks",
          "several chunks -> several options (documented _N=3 example)",
          &pipeline_multiple_chunks },
        { "pipeline_n1_key_only",
          "_N == 1 -> one key-only option per slot",
          &pipeline_n1_key_only },
        { "pipeline_passthrough_survives",
          "a boundary passthrough survives partition, then flattens in the set",
          &pipeline_passthrough_survives },
        { "pipeline_tuple_input",
          "a single std::tuple schema is accepted like a bare pack",
          &pipeline_tuple_input },
        { "pipeline_empty_schema",
          "empty schema -> empty option_set",
          &pipeline_empty_schema },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
