/*******************************************************************************
* djinterp [cpp][memory]                                 memory_tests_source.cpp
*
* The memory suite, block "source": mem_source.hpp's DMemorySource and its
* three named sources, DBufferSource and DCountingSource.
*
*
* path:      /tests/djinterp/cpp/memory/memory_tests_source.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./memory_tests.hpp"  // corresponding header

// std
#include <cstring>


NS_DJINTERP
NS_TESTING


/*
tests_memory_source_default
  An empty source, and the sources that give.
  Tests the following:
  - a default-constructed DMemorySource is no source
  - the default source is valid, named, and gives aligned bytes it takes back
  - the system source, where built, does the same
*/
bool
tests_memory_source_default()
{
    DMemorySource none;
    DMemorySource fallback = DMemorySource::default_source();
    void*         bytes    = nullptr;

    D_TEST_REQUIRE(!none.is_valid());
    D_TEST_REQUIRE(fallback.is_valid());
    D_TEST_REQUIRE(fallback.name() != nullptr);

    bytes = fallback.allocate(100,
                              32);
    D_TEST_REQUIRE(bytes != nullptr);
    D_TEST_REQUIRE(is_aligned(bytes,
                              32));
    std::memset(bytes,
                0x5a,
                100);
    fallback.release(bytes,
                     100,
                     32);

#if (D_INTERNAL_MEM_SOURCE_SYSTEM == 1)
    {
        DMemorySource system = DMemorySource::system();

        D_TEST_REQUIRE(system.is_valid());
        bytes = system.allocate(64);
        D_TEST_REQUIRE(bytes != nullptr);
        system.release(bytes,
                       64);
    }
#endif  // D_INTERNAL_MEM_SOURCE_SYSTEM

    return true;
}

/*
tests_memory_source_refusing
  The source that gives nothing.
  Tests the following:
  - it is a valid source whose every allocation fails
  - releasing to it is harmless
*/
bool
tests_memory_source_refusing()
{
#if (D_INTERNAL_MEM_SOURCE_NULL == 1)
    DMemorySource refusing = DMemorySource::refusing();

    D_TEST_REQUIRE(refusing.is_valid());
    D_TEST_REQUIRE(refusing.allocate(1) == nullptr);
    D_TEST_REQUIRE(refusing.allocate(4096,
                                     64) == nullptr);
    refusing.release(nullptr,
                     0);
#else
    D_TEST_SKIP("built without D_CFG_MEM_SOURCE_NULL");
#endif  // D_INTERNAL_MEM_SOURCE_NULL

    return true;
}

/*
tests_memory_source_buffer
  A source over a caller's buffer.
  Tests the following:
  - it gives aligned bytes from inside the buffer, and counts them used
  - once spent it refuses
  - reset gives the whole buffer back
*/
bool
tests_memory_source_buffer()
{
#if (D_INTERNAL_MEM_SOURCE_BUFFER == 1)
    alignas(16) static unsigned char storage[256];
    DBufferSource                    buffer(storage);
    DMemorySource                    source = buffer.source();
    unsigned char*                   first  = nullptr;
    unsigned char*                   second = nullptr;

    D_TEST_REQUIRE(source.is_valid());
    D_TEST_REQUIRE(buffer.used() == 0);
    D_TEST_REQUIRE(buffer.remaining() == sizeof(storage));

    first  = static_cast<unsigned char*>(source.allocate(40,
                                                         8));
    second = static_cast<unsigned char*>(source.allocate(40,
                                                         16));
    D_TEST_REQUIRE(first != nullptr);
    D_TEST_REQUIRE(second != nullptr);
    D_TEST_REQUIRE( (first >= storage) && (first < storage + sizeof(storage)) );
    D_TEST_REQUIRE( (second >= first + 40) &&
                    (second < storage + sizeof(storage)) );
    D_TEST_REQUIRE(is_aligned(second,
                              16));
    D_TEST_REQUIRE(buffer.used() >= 80);
    D_TEST_REQUIRE(buffer.used() + buffer.remaining() == sizeof(storage));

    D_TEST_REQUIRE(source.allocate(sizeof(storage)) == nullptr);

    buffer.reset();
    D_TEST_REQUIRE(buffer.used() == 0);
    D_TEST_REQUIRE(source.allocate(sizeof(storage) / 2) != nullptr);
#else
    D_TEST_SKIP("built without D_CFG_MEM_SOURCE_BUFFER");
#endif  // D_INTERNAL_MEM_SOURCE_BUFFER

    return true;
}

/*
tests_memory_source_counting
  A source that counts what it forwards.
  Tests the following:
  - its allocations come from its upstream, and work
  - its stats count each allocation and release, and the live bytes
*/
bool
tests_memory_source_counting()
{
#if (D_INTERNAL_MEM_SOURCE_COUNTER == 1)
    DCountingSource counting;
    DMemorySource   source = counting.source();
    DMemStats       stats  = counting.stats();
    void*           a      = nullptr;
    void*           b      = nullptr;

    D_TEST_REQUIRE(source.is_valid());
    D_TEST_REQUIRE(stats.acquire_count == 0);

    a = source.allocate(24);
    b = source.allocate(100,
                        16);
    D_TEST_REQUIRE( (a != nullptr) && (b != nullptr) );
    stats = counting.stats();
    D_TEST_REQUIRE(stats.acquire_count == 2);
    D_TEST_REQUIRE(stats.bytes_live >= 124);

    source.release(a,
                   24);
    source.release(b,
                   100,
                   16);
    stats = counting.stats();
    D_TEST_REQUIRE(stats.release_count == 2);
    D_TEST_REQUIRE(stats.bytes_live == 0);
#else
    D_TEST_SKIP("built without D_CFG_MEM_SOURCE_COUNTER");
#endif  // D_INTERNAL_MEM_SOURCE_COUNTER

    return true;
}


NS_END  // testing
NS_END  // djinterp
