/*******************************************************************************
* djinterp [cpp][memory]                                  memory_tests_arena.cpp
*
* The memory suite, block "arena": arena.hpp's arena, over a buffer and over
* a source, its typed allocation, its marks, and DArenaScope.
*
*
* path:      /tests/djinterp/cpp/memory/memory_tests_arena.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./memory_tests.hpp"  // corresponding header

// std
#include <cstring>
#include <utility>


NS_DJINTERP
NS_TESTING


namespace
{
#if (D_INTERNAL_ARENA_MARKS == 1)
    // memory_tests_scoped_work
    //   function: allocates inside a scope and leaves it early or late, as
    // _is_early says; either way the scope must rewind.
    bool
    memory_tests_scoped_work(
        arena& _arena,
        bool   _is_early
    )
    {
        DArenaScope scope(_arena);

        if (!_arena.allocate(64))
        {
            return false;
        }

        // the early exit
        if (_is_early)
        {
            return true;
        }

        return (_arena.allocate(64) != nullptr);
    }
#endif  // D_INTERNAL_ARENA_MARKS
}  // anonymous namespace


/*
tests_memory_arena_buffer
  An arena over a caller's buffer.
  Tests the following:
  - it gives aligned bytes from inside the buffer, in one region
  - used and remaining add up to the capacity, which fits in the buffer
  - past its capacity it refuses, and allocate_ex says it is exhausted
  - reset gives every byte back
*/
bool
tests_memory_arena_buffer()
{
    alignas(16) static unsigned char storage[1024];
    arena                            a(storage,
                                       static_cast<DMemSize>(sizeof(storage)));
    unsigned char*                   bytes = nullptr;
    DMemBlock                        block = {};

    D_TEST_REQUIRE(a.capacity() > 0);
    D_TEST_REQUIRE(a.capacity() <= sizeof(storage));

    bytes = static_cast<unsigned char*>(a.allocate(10,
                                                   16));
    D_TEST_REQUIRE(bytes != nullptr);
    D_TEST_REQUIRE( (bytes >= storage) && (bytes < storage + sizeof(storage)) );
    D_TEST_REQUIRE(is_aligned(bytes,
                              16));
    D_TEST_REQUIRE(a.used() >= 10);
    D_TEST_REQUIRE(a.used() + a.remaining() == a.capacity());
    D_TEST_REQUIRE(a.region_count() == 1);
#if (D_INTERNAL_ARENA_OWNS == 1)
    D_TEST_REQUIRE(a.owns(bytes));
    D_TEST_REQUIRE(!a.owns(&block));
#endif  // D_INTERNAL_ARENA_OWNS

    D_TEST_REQUIRE(a.allocate(a.capacity() + 1) == nullptr);
    D_TEST_REQUIRE(a.allocate_ex(sizeof(storage) * 2,
                                 8,
                                 block) == D_MEM_ERR_EXHAUSTED);

    a.reset();
    D_TEST_REQUIRE(a.used() == 0);
    D_TEST_REQUIRE(a.allocate(a.capacity() / 2) != nullptr);

    return true;
}

/*
tests_memory_arena_growth
  An arena over a source.
  Tests the following:
  - it obtains nothing until asked, then grows by regions
  - every byte it gave stays writable as it grows
  - reset keeps the regions; trim keeps one; release keeps none
  - a ceiling refuses what would pass it
*/
bool
tests_memory_arena_growth()
{
    arena          a(DMemorySource::default_source(),
                     256);
    arena          capped(DMemorySource::default_source(),
                          256,
                          1024);
    unsigned char* blocks[32] = {};
    DMemStats      stats      = {};

    D_TEST_REQUIRE(a.region_count() == 0);

    // enough to need more than one region
    for (int i = 0; i < 32; ++i)
    {
        blocks[i] = static_cast<unsigned char*>(a.allocate(200,
                                                           8));
        D_TEST_REQUIRE(blocks[i] != nullptr);
        std::memset(blocks[i],
                    i,
                    200);
    }

    D_TEST_REQUIRE(a.region_count() > 1);
    D_TEST_REQUIRE( (blocks[0][199] == 0) && (blocks[31][0] == 31) );
    stats = a.stats();
    D_TEST_REQUIRE(stats.bytes_reserved >= 32 * 200);
    D_TEST_REQUIRE(stats.acquire_count == 32);

    a.reset();
    D_TEST_REQUIRE(a.used() == 0);
    D_TEST_REQUIRE(a.region_count() > 1);

    a.trim();
    D_TEST_REQUIRE(a.region_count() <= 1);

    a.release();
    D_TEST_REQUIRE(a.region_count() == 0);
    D_TEST_REQUIRE(a.allocate(16) != nullptr);

    D_TEST_REQUIRE(capped.allocate(4096) == nullptr);
    D_TEST_REQUIRE(capped.allocate(100) != nullptr);

    return true;
}

/*
tests_memory_arena_objects
  Typed allocation.
  Tests the following:
  - allocate_object and allocate_array align to the type
  - an array whose byte count would wrap is refused
  - create forwards its arguments; create_array value-initializes each
  - duplicate and duplicate_string copy into the arena
*/
bool
tests_memory_arena_objects()
{
    arena              a;
    DMemoryTestsThing* thing  = nullptr;
    int*               ints   = nullptr;
    DMemoryTestsThing* made   = nullptr;
    int*               zeros  = nullptr;
    const int          before = DMemoryTestsCounted::live();
    const char         text[] = "hello arena";
    char*              copy   = nullptr;
    void*              bytes  = nullptr;

    thing = a.allocate_object<DMemoryTestsThing>();
    ints  = a.allocate_array<int>(10);
    D_TEST_REQUIRE( (thing != nullptr) && (ints != nullptr) );
    D_TEST_REQUIRE(is_aligned(thing,
                              align_of<DMemoryTestsThing>::value));
    D_TEST_REQUIRE(is_aligned(ints,
                              align_of<int>::value));
    D_TEST_REQUIRE(a.allocate_array<int>(D_MEM_SIZE_MAX / 2) == nullptr);

    made = a.create<DMemoryTestsThing>(7u);
    D_TEST_REQUIRE(made != nullptr);
    D_TEST_REQUIRE( (made->m_tag == 7u) && (made->m_value == 7.0) );

    zeros = a.create_array<int>(5);
    D_TEST_REQUIRE(zeros != nullptr);
    D_TEST_REQUIRE( (zeros[0] == 0) && (zeros[4] == 0) );

    {
        DMemoryTestsCounted* counted =
            a.create_array<DMemoryTestsCounted>(3);

        D_TEST_REQUIRE(counted != nullptr);
        D_TEST_REQUIRE(DMemoryTestsCounted::live() == before + 3);

        // an arena frees bytes, not objects: the caller runs destructors
        for (int i = 0; i < 3; ++i)
        {
            counted[i].~DMemoryTestsCounted();
        }

        D_TEST_REQUIRE(DMemoryTestsCounted::live() == before);
    }

    bytes = a.duplicate(text,
                        sizeof(text));
    copy  = a.duplicate_string(text);
    D_TEST_REQUIRE( (bytes != nullptr) && (bytes != text) );
    D_TEST_REQUIRE(std::memcmp(bytes,
                               text,
                               sizeof(text)) == 0);
    D_TEST_REQUIRE(copy != nullptr);
    D_TEST_REQUIRE(std::strcmp(copy,
                               text) == 0);

    return true;
}

/*
tests_memory_arena_marks
  Marks.
  Tests the following:
  - rewinding to a mark restores the cursor exactly: the next allocation is
    the same address
  - a mark taken before growth rewinds across regions
  - a mark from elsewhere is refused, where validation is built
*/
bool
tests_memory_arena_marks()
{
#if (D_INTERNAL_ARENA_MARKS == 1)
    arena          a(DMemorySource::default_source(),
                     256);
    ::dj_arena_mark mark;
    void*          first  = nullptr;
    void*          second = nullptr;
    DMemSize       used   = 0;

    D_TEST_REQUIRE(a.allocate(32) != nullptr);
    mark  = a.mark();
    used  = a.used();
    first = a.allocate(128,
                       8);
    D_TEST_REQUIRE(first != nullptr);
    D_TEST_REQUIRE(a.rewind(mark) == D_MEM_OK);
    D_TEST_REQUIRE(a.used() == used);
    second = a.allocate(128,
                        8);
    D_TEST_REQUIRE(second == first);

    D_TEST_REQUIRE(a.rewind(mark) == D_MEM_OK);

    // past one region and back
    for (int i = 0; i < 20; ++i)
    {
        D_TEST_REQUIRE(a.allocate(200) != nullptr);
    }

    D_TEST_REQUIRE(a.rewind(mark) == D_MEM_OK);
    D_TEST_REQUIRE(a.allocate(128,
                              8) == first);

#   if (D_INTERNAL_ARENA_VALIDATE_MARKS == 1)
    {
        ::dj_arena_mark bogus;
        char            elsewhere[64] = {};

        bogus.region = reinterpret_cast< ::dj_arena_region* >(
                           static_cast<void*>(elsewhere));
        bogus.used   = 0;
        D_TEST_REQUIRE(a.rewind(bogus) == D_MEM_ERR_FOREIGN);
    }
#   endif  // D_INTERNAL_ARENA_VALIDATE_MARKS
#else
    D_TEST_SKIP("built without D_CFG_ARENA_MARKS");
#endif  // D_INTERNAL_ARENA_MARKS

    return true;
}

/*
tests_memory_arena_scope
  DArenaScope.
  Tests the following:
  - leaving a scope, late or early, rewinds what it allocated
  - a dismissed scope keeps it
*/
bool
tests_memory_arena_scope()
{
#if (D_INTERNAL_ARENA_MARKS == 1)
    arena    a;
    DMemSize used = 0;

    D_TEST_REQUIRE(a.allocate(16) != nullptr);
    used = a.used();

    D_TEST_REQUIRE(memory_tests_scoped_work(a,
                                            false));
    D_TEST_REQUIRE(a.used() == used);
    D_TEST_REQUIRE(memory_tests_scoped_work(a,
                                            true));
    D_TEST_REQUIRE(a.used() == used);

    {
        DArenaScope scope(a);

        D_TEST_REQUIRE(a.allocate(64) != nullptr);
        scope.dismiss();
    }

    D_TEST_REQUIRE(a.used() > used);
#else
    D_TEST_SKIP("built without D_CFG_ARENA_MARKS");
#endif  // D_INTERNAL_ARENA_MARKS

    return true;
}

/*
tests_memory_arena_lifetime
  Moving an arena.
  Tests the following:
  - a moved-to arena holds the regions and the bytes in them
  - the moved-from arena is empty, and usable
  - move assignment releases the destination's own regions first
*/
bool
tests_memory_arena_lifetime()
{
    arena a;
    char* text = nullptr;

    text = a.duplicate_string("moved");
    D_TEST_REQUIRE(text != nullptr);

    {
        arena b(std::move(a));

        D_TEST_REQUIRE(b.region_count() == 1);
        D_TEST_REQUIRE(std::strcmp(text,
                                   "moved") == 0);
#if (D_INTERNAL_ARENA_OWNS == 1)
        D_TEST_REQUIRE(b.owns(text));
#endif  // D_INTERNAL_ARENA_OWNS
        D_TEST_REQUIRE(a.region_count() == 0);
        D_TEST_REQUIRE(a.used() == 0);
        D_TEST_REQUIRE(a.allocate(8) != nullptr);

        a = std::move(b);
        D_TEST_REQUIRE(b.region_count() == 0);
        D_TEST_REQUIRE(std::strcmp(text,
                                   "moved") == 0);
    }

    D_TEST_REQUIRE(a.region_count() == 1);

    return true;
}


NS_END  // testing
NS_END  // djinterp
