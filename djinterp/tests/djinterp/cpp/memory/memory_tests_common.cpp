/*******************************************************************************
* djinterp [cpp][memory]                                 memory_tests_common.cpp
*
* The memory suite, block "common": mem_common.hpp's vocabulary, its
* alignment kernels and its resource traits.
*
*
* path:      /tests/djinterp/cpp/memory/memory_tests_common.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./memory_tests.hpp"  // corresponding header

// std
#include <cstring>
#include <type_traits>


NS_DJINTERP
NS_TESTING


namespace
{
    // DMemoryTestsResource
    //   class: a resource with all three operations.
    struct DMemoryTestsResource
    {
        void*
        allocate(
            DMemSize,
            DMemSize
        )
        {
            return nullptr;
        }

        void
        release(
            void*,
            DMemSize,
            DMemSize
        )
        {}

        void
        reset()
        {}
    };

    // DMemoryTestsAllocateOnly
    //   class: a resource that only allocates.
    struct DMemoryTestsAllocateOnly
    {
        void*
        allocate(
            DMemSize,
            DMemSize
        )
        {
            return nullptr;
        }
    };
}  // anonymous namespace


/*
tests_memory_common_status
  The status vocabulary.
  Tests the following:
  - status_is_ok holds for D_MEM_OK only
  - status_name names every status, as the C core's d_mem_status_string
*/
bool
tests_memory_common_status()
{
    D_TEST_REQUIRE(status_is_ok(D_MEM_OK));
    D_TEST_REQUIRE(!status_is_ok(D_MEM_ERR_NULL));
    D_TEST_REQUIRE(!status_is_ok(D_MEM_ERR_EXHAUSTED));
    D_TEST_REQUIRE(!status_is_ok(D_MEM_ERR_STALE));

    D_TEST_REQUIRE(status_name(D_MEM_OK) != nullptr);
    D_TEST_REQUIRE(std::strcmp(status_name(D_MEM_ERR_FOREIGN),
                               ::d_mem_status_string(D_MEM_ERR_FOREIGN)) ==
                   0);
    D_TEST_REQUIRE(std::strcmp(status_name(D_MEM_OK),
                               status_name(D_MEM_ERR_INVALID)) != 0);

    return true;
}

/*
tests_memory_common_alignment
  The alignment kernels, at run time and in constant expressions.
  Tests the following:
  - is_pow2 holds for powers of two, not for 0 or others
  - align_up rounds up to a power of two; align_padding is the difference
  - is_aligned reads a pointer's alignment
  - align_of and size_of are alignof and sizeof as DMemSize
*/
bool
tests_memory_common_alignment()
{
    alignas(16) unsigned char block[32];

    static_assert(is_pow2(64),
                  "is_pow2 is a constant expression");
    static_assert(align_up(13,
                           8) == 16,
                  "align_up is a constant expression");

    D_TEST_REQUIRE(is_pow2(1));
    D_TEST_REQUIRE(is_pow2(4096));
    D_TEST_REQUIRE(!is_pow2(0));
    D_TEST_REQUIRE(!is_pow2(12));

    D_TEST_REQUIRE(align_up(0,
                            8) == 0);
    D_TEST_REQUIRE(align_up(1,
                            8) == 8);
    D_TEST_REQUIRE(align_up(8,
                            8) == 8);
    D_TEST_REQUIRE(align_up(17,
                            16) == 32);
    D_TEST_REQUIRE(align_padding(13,
                                 8) == 3);
    D_TEST_REQUIRE(align_padding(16,
                                 8) == 0);

    D_TEST_REQUIRE(is_aligned(block,
                              16));
    D_TEST_REQUIRE(!is_aligned(block + 1,
                               2));

    D_TEST_REQUIRE(align_of<double>::value ==
                   static_cast<DMemSize>(alignof(double)));
    D_TEST_REQUIRE(size_of<DMemoryTestsThing>::value ==
                   static_cast<DMemSize>(sizeof(DMemoryTestsThing)));

    return true;
}

/*
tests_memory_common_traits
  The resource traits.
  Tests the following:
  - each detects its operation, and only its own
  - is_memory_resource needs allocate alone
  - an unrelated type is none of them
*/
bool
tests_memory_common_traits()
{
#if (D_INTERNAL_MEM_TRAIT_DETECTORS == 1)
    D_TEST_REQUIRE(has_allocate<DMemoryTestsResource>::value);
    D_TEST_REQUIRE(has_release<DMemoryTestsResource>::value);
    D_TEST_REQUIRE(has_reset<DMemoryTestsResource>::value);
    D_TEST_REQUIRE(is_memory_resource<DMemoryTestsResource>::value);

    D_TEST_REQUIRE(has_allocate<DMemoryTestsAllocateOnly>::value);
    D_TEST_REQUIRE(!has_release<DMemoryTestsAllocateOnly>::value);
    D_TEST_REQUIRE(!has_reset<DMemoryTestsAllocateOnly>::value);
    D_TEST_REQUIRE(is_memory_resource<DMemoryTestsAllocateOnly>::value);

    D_TEST_REQUIRE(!is_memory_resource<int>::value);
    D_TEST_REQUIRE(!has_reset<DMemoryTestsThing>::value);

    D_TEST_REQUIRE(is_memory_resource<arena>::value);
    D_TEST_REQUIRE(has_reset<arena>::value);
#else
    D_TEST_SKIP("built without D_CFG_MEM_TRAIT_DETECTORS");
#endif  // D_INTERNAL_MEM_TRAIT_DETECTORS

    return true;
}


NS_END  // testing
NS_END  // djinterp
