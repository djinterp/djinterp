/*******************************************************************************
* djinterp [cpp][memory]                                        memory_tests.hpp
*
* The C++ memory subframework's suite (cpp/memory/memory.hpp and every face
* it includes), in DTest's C face: mem_common.hpp, mem_source.hpp, arena.hpp,
* pool.hpp, pool_allocator.hpp and the strategy headers, a block each, and a
* block for the cost law and the faces' parity with the C kernels.
*   It replaces the suites written against the core/memory headers that
* came before (pool/pool.hpp, memory_strategy_traits.hpp, ...), which no
* longer exist. A test whose feature a knob leaves out skips, saying which.
*
*
* path:      /tests/djinterp/cpp/memory/memory_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FIXTURES
    --------
    1.  Element types
         1.  DMemoryTestsThing
         2.  DMemoryTestsCounted
2.  TESTS
    -----
    1.  Common
    2.  Sources
    3.  The arena
    4.  The pool
    5.  The allocators
    6.  Strategies
    7.  The cost law and parity
3.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_CPP_MEMORY_MEMORY_TESTS_HPP
#define DJINTERP_TESTS_CPP_MEMORY_MEMORY_TESTS_HPP 1

// djinterp
#include "djinterp/cpp/djinterp.hpp"                 // framework root
#include "djinterp/c/modules/test/test_suite.h"      // D_TEST_*
#ifndef DTEST_SPEC_MODE
    #include "djinterp/cpp/memory/memory.hpp"        // under test: every face
#endif  // !DTEST_SPEC_MODE


NS_DJINTERP
NS_TESTING


//==============================================================================
// 1.  FIXTURES
//==============================================================================
#ifndef DTEST_SPEC_MODE


// 1.1    Element types
//------------------------------------------------------------------------------
// 1.1.1
// DMemoryTestsThing
//   struct: an element with a double's alignment and a tag to tell copies
// apart.
struct DMemoryTestsThing
{
    double   m_value;
    void*    m_pointer;
    unsigned m_tag;

    DMemoryTestsThing()
        : m_value(0.0),
          m_pointer(nullptr),
          m_tag(0)
    {}

    explicit DMemoryTestsThing(
        unsigned _tag
    )
        : m_value(static_cast<double>(_tag)),
          m_pointer(nullptr),
          m_tag(_tag)
    {}
};

// 1.1.2
// DMemoryTestsCounted
//   struct: an element that counts how many of it are alive, so a test can
// see a constructor or a destructor run.
struct DMemoryTestsCounted
{
    static int& live()
    {
        static int count = 0;

        return count;
    }

    int m_value;

    explicit DMemoryTestsCounted(
        int _value
    )
        : m_value(_value)
    {
        ++live();
    }

    DMemoryTestsCounted()
        : m_value(0)
    {
        ++live();
    }

    ~DMemoryTestsCounted()
    {
        --live();
    }
};


#endif  // !DTEST_SPEC_MODE


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Common
//------------------------------------------------------------------------------
bool
tests_memory_common_status();

bool
tests_memory_common_alignment();

bool
tests_memory_common_traits();


// 2.2    Sources
//------------------------------------------------------------------------------
bool
tests_memory_source_default();

bool
tests_memory_source_refusing();

bool
tests_memory_source_buffer();

bool
tests_memory_source_counting();


// 2.3    The arena
//------------------------------------------------------------------------------
bool
tests_memory_arena_buffer();

bool
tests_memory_arena_growth();

bool
tests_memory_arena_objects();

bool
tests_memory_arena_marks();

bool
tests_memory_arena_scope();

bool
tests_memory_arena_lifetime();


// 2.4    The pool
//------------------------------------------------------------------------------
bool
tests_memory_pool_raw();

bool
tests_memory_pool_refusals();

bool
tests_memory_pool_objects();

bool
tests_memory_pool_handles();

bool
tests_memory_pool_lifetime();


// 2.5    The allocators
//------------------------------------------------------------------------------
bool
tests_memory_allocator_node_pool();

bool
tests_memory_allocator_pool();

bool
tests_memory_allocator_pool_containers();

bool
tests_memory_allocator_arena();


// 2.6    Strategies
//------------------------------------------------------------------------------
bool
tests_memory_strategy_classification();

bool
tests_memory_strategy_arena();

bool
tests_memory_strategy_pool();

bool
tests_memory_strategy_buffer();

bool
tests_memory_strategy_allocator();


// 2.7    The cost law and parity
//------------------------------------------------------------------------------
bool
tests_memory_parity_layout();

bool
tests_memory_parity_arena();

bool
tests_memory_parity_pool();


//==============================================================================
// 3.  SPEC
//==============================================================================


// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// memory_tests_common_rows
//   constant: the common block's rows.
static const struct dj_test_case_spec memory_tests_common_rows[] =
{
    { "tests_memory_common_status",
      "status_is_ok and status_name read the C statuses",
      &tests_memory_common_status },
    { "tests_memory_common_alignment",
      "is_pow2, align_up, align_padding, is_aligned, align_of and size_of",
      &tests_memory_common_alignment },
    { "tests_memory_common_traits",
      "has_allocate, has_release, has_reset and is_memory_resource",
      &tests_memory_common_traits }
};

// memory_tests_source_rows
//   constant: the sources block's rows.
static const struct dj_test_case_spec memory_tests_source_rows[] =
{
    { "tests_memory_source_default",
      "an empty source, and the default and system sources, which give",
      &tests_memory_source_default },
    { "tests_memory_source_refusing",
      "the refusing source is valid and gives nothing",
      &tests_memory_source_refusing },
    { "tests_memory_source_buffer",
      "a buffer source gives from its buffer until it is spent; reset",
      &tests_memory_source_buffer },
    { "tests_memory_source_counting",
      "a counting source forwards, and counts what it forwards",
      &tests_memory_source_counting }
};

// memory_tests_arena_rows
//   constant: the arena block's rows.
static const struct dj_test_case_spec memory_tests_arena_rows[] =
{
    { "tests_memory_arena_buffer",
      "an arena over a buffer gives aligned bytes from it, then refuses",
      &tests_memory_arena_buffer },
    { "tests_memory_arena_growth",
      "an arena over a source grows by regions; reset keeps them, trim and "
      "release give them back",
      &tests_memory_arena_growth },
    { "tests_memory_arena_objects",
      "allocate_object, allocate_array, create, create_array, duplicate",
      &tests_memory_arena_objects },
    { "tests_memory_arena_marks",
      "a mark taken and rewound to restores the cursor exactly",
      &tests_memory_arena_marks },
    { "tests_memory_arena_scope",
      "DArenaScope rewinds on every exit, unless dismissed",
      &tests_memory_arena_scope },
    { "tests_memory_arena_lifetime",
      "a move takes the regions and leaves the source empty",
      &tests_memory_arena_lifetime }
};

// memory_tests_pool_rows
//   constant: the pool block's rows.
static const struct dj_test_case_spec memory_tests_pool_rows[] =
{
    { "tests_memory_pool_raw",
      "raw_pool: geometry, acquire, release_slot, reserve, reset, queries",
      &tests_memory_pool_raw },
    { "tests_memory_pool_refusals",
      "a foreign pointer, a second release and a full pool are refused",
      &tests_memory_pool_refusals },
    { "tests_memory_pool_objects",
      "pool<T>: the geometry of T, and create and destroy run T's lifetime",
      &tests_memory_pool_objects },
    { "tests_memory_pool_handles",
      "a generational pool's handles resolve until released, then go stale",
      &tests_memory_pool_handles },
    { "tests_memory_pool_lifetime",
      "a move takes the slots and leaves the source empty and usable",
      &tests_memory_pool_lifetime }
};

// memory_tests_allocator_rows
//   constant: the allocators block's rows.
static const struct dj_test_case_spec memory_tests_allocator_rows[] =
{
    { "tests_memory_allocator_node_pool",
      "node_pool configures on first use; other sizes go to its source",
      &tests_memory_allocator_node_pool },
    { "tests_memory_allocator_pool",
      "pool_allocator: its typedefs, rebinding, allocation and equality",
      &tests_memory_allocator_pool },
    { "tests_memory_allocator_pool_containers",
      "std::list and std::map run on a pool_allocator",
      &tests_memory_allocator_pool_containers },
    { "tests_memory_allocator_arena",
      "arena_allocator gives from its arena, frees nothing, runs a vector",
      &tests_memory_allocator_arena }
};

// memory_tests_strategy_rows
//   constant: the strategies block's rows.
static const struct dj_test_case_spec memory_tests_strategy_rows[] =
{
    { "tests_memory_strategy_classification",
      "detection and classification, on the four strategies and non-ones",
      &tests_memory_strategy_classification },
    { "tests_memory_strategy_arena",
      "DArenaMemoryStrategy gives aligned bytes, frees none, resets",
      &tests_memory_strategy_arena },
    { "tests_memory_strategy_pool",
      "DPoolMemoryStrategy gives one slot at a time and releases it",
      &tests_memory_strategy_pool },
    { "tests_memory_strategy_buffer",
      "DBufferMemoryStrategy: a stack in a static extent",
      &tests_memory_strategy_buffer },
    { "tests_memory_strategy_allocator",
      "DAllocatorMemoryStrategy forwards to its allocator",
      &tests_memory_strategy_allocator }
};

// memory_tests_parity_rows
//   constant: the cost law and parity block's rows.
static const struct dj_test_case_spec memory_tests_parity_rows[] =
{
    { "tests_memory_parity_layout",
      "every face is its kernel: the same size, the same address",
      &tests_memory_parity_layout },
    { "tests_memory_parity_arena",
      "one allocation sequence gives the same offsets on both faces",
      &tests_memory_parity_arena },
    { "tests_memory_parity_pool",
      "pool<T>'s geometry, computed early, is the C kernel's",
      &tests_memory_parity_pool }
};

// memory_tests_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec memory_tests_blocks[] =
{
    { "common", "mem_common.hpp: the vocabulary, alignment and traits",
      memory_tests_common_rows,
      sizeof(memory_tests_common_rows) / sizeof(memory_tests_common_rows[0]) },
    { "source", "mem_source.hpp: where the bytes come from",
      memory_tests_source_rows,
      sizeof(memory_tests_source_rows) / sizeof(memory_tests_source_rows[0]) },
    { "arena", "arena.hpp: the arena and DArenaScope",
      memory_tests_arena_rows,
      sizeof(memory_tests_arena_rows) / sizeof(memory_tests_arena_rows[0]) },
    { "pool", "pool.hpp: raw_pool, pool<T> and DPoolHandle<T>",
      memory_tests_pool_rows,
      sizeof(memory_tests_pool_rows) / sizeof(memory_tests_pool_rows[0]) },
    { "allocator", "pool_allocator.hpp: the Allocator faces",
      memory_tests_allocator_rows,
      sizeof(memory_tests_allocator_rows) /
          sizeof(memory_tests_allocator_rows[0]) },
    { "strategy", "memory_strategy_common.hpp and memory_strategy.hpp",
      memory_tests_strategy_rows,
      sizeof(memory_tests_strategy_rows) /
          sizeof(memory_tests_strategy_rows[0]) },
    { "parity", "the cost law, and parity with the C kernels",
      memory_tests_parity_rows,
      sizeof(memory_tests_parity_rows) / sizeof(memory_tests_parity_rows[0]) }
};

// memory_tests_module
//   constant: the module spec.
static const struct dj_test_module_spec memory_tests_module =
{
    "memory",
    "cpp/memory: the C++ faces over the memory kernels",
    memory_tests_blocks,
    sizeof(memory_tests_blocks) / sizeof(memory_tests_blocks[0])
};

// memory_tests_spec
//   function: the suite's provider.
inline const struct dj_test_module_spec*
memory_tests_spec()
{
    return &memory_tests_module;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CPP_MEMORY_MEMORY_TESTS_HPP
