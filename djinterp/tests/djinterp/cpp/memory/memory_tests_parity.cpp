/*******************************************************************************
* djinterp [cpp][memory]                                 memory_tests_parity.cpp
*
* The memory suite, block "parity": the cost law at run time (every face is
* its kernel, at the same address) and the faces computing what the C
* kernels compute, from the same inputs.
*
*
* path:      /tests/djinterp/cpp/memory/memory_tests_parity.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./memory_tests.hpp"  // corresponding header


NS_DJINTERP
NS_TESTING


/*
tests_memory_parity_layout
  The cost law, at run time.
  Tests the following:
  - each face's base subobject is at offset zero: a face pointer and its
    kernel pointer are the same address
  - each face is its kernel's size, and each allocator one pointer
*/
bool
tests_memory_parity_layout()
{
    arena                     a;
    raw_pool                  raw(32,
                                  8);
    pool<DMemoryTestsThing>   typed;
    DMemorySource             source;

    D_TEST_REQUIRE(static_cast<void*>(&a) ==
                   static_cast<void*>(static_cast< ::dj_arena* >(&a)));
    D_TEST_REQUIRE(static_cast<void*>(&raw) ==
                   static_cast<void*>(static_cast< ::dj_pool* >(&raw)));
    D_TEST_REQUIRE(static_cast<void*>(&typed) ==
                   static_cast<void*>(static_cast< ::dj_pool* >(&typed)));
    D_TEST_REQUIRE(static_cast<void*>(&source) ==
                   static_cast<void*>(
                       static_cast< ::dj_mem_source* >(&source)));

    D_TEST_REQUIRE(sizeof(arena) == sizeof(::dj_arena));
    D_TEST_REQUIRE(sizeof(raw_pool) == sizeof(::dj_pool));
    D_TEST_REQUIRE(sizeof(pool<DMemoryTestsThing>) == sizeof(::dj_pool));
    D_TEST_REQUIRE(sizeof(DMemorySource) == sizeof(::dj_mem_source));
    D_TEST_REQUIRE(sizeof(pool_allocator<double>) == sizeof(void*));
    D_TEST_REQUIRE(sizeof(arena_allocator<double>) == sizeof(void*));

    return true;
}

/*
tests_memory_parity_arena
  One arena, driven through both faces.
  Tests the following:
  - the same buffer size gives the same capacity
  - the same allocation sequence gives the same offsets from each buffer,
    and leaves the same bytes used
*/
bool
tests_memory_parity_arena()
{
    alignas(64) static unsigned char c_buffer[16384];
    alignas(64) static unsigned char cpp_buffer[16384];
    ::dj_arena                       c_arena;
    arena                            cpp_arena(
                                         cpp_buffer,
                                         static_cast<DMemSize>(
                                             sizeof(cpp_buffer)));

    D_TEST_REQUIRE(::d_arena_init_buffer(&c_arena,
                                         c_buffer,
                                         static_cast<DMemSize>(
                                             sizeof(c_buffer))) == D_MEM_OK);
    D_TEST_REQUIRE(::d_arena_capacity(&c_arena) == cpp_arena.capacity());

    // fifty allocations of mixed sizes and alignments
    for (int i = 0; i < 50; ++i)
    {
        const DMemSize bytes = static_cast<DMemSize>((i * 7) % 61) + 1;
        const DMemSize align = static_cast<DMemSize>(1u << (i % 5));
        unsigned char* from_c   =
            static_cast<unsigned char*>(::d_arena_allocate(&c_arena,
                                                           bytes,
                                                           align));
        unsigned char* from_cpp =
            static_cast<unsigned char*>(cpp_arena.allocate(bytes,
                                                           align));

        D_TEST_REQUIRE( (from_c != nullptr) && (from_cpp != nullptr) );
        D_TEST_REQUIRE((from_c - c_buffer) == (from_cpp - cpp_buffer));
    }

    D_TEST_REQUIRE(::d_arena_used(&c_arena) == cpp_arena.used());
    ::d_arena_release(&c_arena);

    return true;
}

/*
tests_memory_parity_pool
  Pool geometry, computed early and late.
  Tests the following:
  - pool<T>, told sizeof(T) and alignof(T) at compile time, arrives at the
    slot size and alignment the C kernel computes from them at run time
  - so does raw_pool, told them at run time
*/
bool
tests_memory_parity_pool()
{
    ::dj_pool_config        config =
        ::d_pool_config_default(sizeof(DMemoryTestsThing),
                                alignof(DMemoryTestsThing));
    ::dj_pool               c_pool;
    pool<DMemoryTestsThing> typed;
    raw_pool                raw(sizeof(DMemoryTestsThing),
                                alignof(DMemoryTestsThing));

    D_TEST_REQUIRE(::d_pool_init(&c_pool,
                                 &config) == D_MEM_OK);
    D_TEST_REQUIRE(typed.slot_size() == ::d_pool_slot_size(&c_pool));
    D_TEST_REQUIRE(typed.slot_align() == ::d_pool_slot_align(&c_pool));
    D_TEST_REQUIRE(raw.slot_size() == ::d_pool_slot_size(&c_pool));
    D_TEST_REQUIRE(raw.slot_align() == ::d_pool_slot_align(&c_pool));
    ::d_pool_release(&c_pool);

    return true;
}


NS_END  // testing
NS_END  // djinterp
