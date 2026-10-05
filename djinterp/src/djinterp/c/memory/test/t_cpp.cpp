/*******************************************************************************
* djinterp [c]                                                         t_cpp.cpp
*
*   C++ parity and cost-law harness. Checks that every wrapper's base subobject
* sits at offset zero, and drives the SAME operation through both faces to
* confirm the C++ tier computes what the C core computes -- earlier, not
* differently.
*
*   Build (from the repo root):
*     cc  src/djinterp/c/memory/mem_common.c \
*         src/djinterp/c/memory/mem_source.c \
*         src/djinterp/c/memory/arena.c \
*         src/djinterp/c/memory/pool.c \
*         src/djinterp/c/memory/test/t_cpp.cpp -o t_cpp
*   No -I is required: every include is repo-root relative.
*
*
* path:      /src/djinterp/c/memory/test/t_cpp.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

/*   C++ parity harness.
 *   Two questions, and only two:
 *
 *     1. THE COST LAW. Does every wrapper add zero bytes? Most of this is
 *        answered by static_assert inside the headers -- compiling this file
 *        at all proves those. The runtime checks here cover what a
 *        static_assert cannot: that the base subobject sits at offset zero,
 *        so a wrapper pointer and a kernel pointer are the same address.
 *
 *     2. PARITY. Does the C++ face compute the same thing as the C core, or
 *        merely something that looks right? Each section below drives the
 *        SAME operation through both faces and compares the observable
 *        results -- addresses, counts, geometry -- rather than trusting that
 *        a forwarding call must agree.
 */
// FLOOR, FOR NOW: below C++11 this test is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
// (it includes core/memory, whose headers need C++11, so the gate comes
// before every include)
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <vector>

#include "../../../../../inc/djinterp/core/memory/arena.hpp"
#include "../../../../../inc/djinterp/core/memory/pool.hpp"
#include "../../../../../inc/djinterp/core/memory/pool_allocator.hpp"

using namespace djinterp;

static int fails   = 0;
static int skipped = 0;

#define CHECK(c)                                                              \
    do                                                                        \
    {                                                                         \
        if (!(c))                                                             \
        {                                                                     \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);        \
            ++fails;                                                          \
        }                                                                     \
    } while (0)

struct thing
{
    double   d;
    void*    p;
    unsigned tag;

    thing()
        : d(0.0), p(nullptr), tag(0)
    {}

    explicit thing(unsigned _t)
        : d(static_cast<double>(_t)), p(nullptr), tag(_t)
    {}
};

static int live_things = 0;

struct counted
{
    int value;

    explicit counted(int _v)
        : value(_v)
    {
        ++live_things;
    }

    ~counted()
    {
        --live_things;
    }
};


/* ------------------------------------------------------------------------ */
/*  1.  the cost law, at run time                                           */
/* ------------------------------------------------------------------------ */
static void
test_cost_law()
{
    arena          a;
    raw_pool       rp(32, 8);
    pool<thing>    tp;
    memory_source  ms;

    /* the base subobject must be at offset zero: a wrapper pointer and a
       kernel pointer are then literally the same address, which is what makes
       passing a wrapper to a C entry point free rather than a conversion */
    CHECK(static_cast<void*>(&a)  == static_cast<void*>(
              static_cast< ::d_arena* >(&a)));
    CHECK(static_cast<void*>(&rp) == static_cast<void*>(
              static_cast< ::d_pool* >(&rp)));
    CHECK(static_cast<void*>(&tp) == static_cast<void*>(
              static_cast< ::d_pool* >(&tp)));
    CHECK(static_cast<void*>(&ms) == static_cast<void*>(
              static_cast< ::d_mem_source* >(&ms)));

    /* and the sizes agree, which the headers also assert statically */
    CHECK(sizeof(arena)       == sizeof(::d_arena));
    CHECK(sizeof(raw_pool)    == sizeof(::d_pool));
    CHECK(sizeof(pool<thing>) == sizeof(::d_pool));

    std::printf("  sizeof: d_arena=%zu arena=%zu | d_pool=%zu pool<thing>=%zu"
                " | d_mem_source=%zu\n",
                sizeof(::d_arena), sizeof(arena),
                sizeof(::d_pool), sizeof(pool<thing>),
                sizeof(::d_mem_source));

    return;
}


/* ------------------------------------------------------------------------ */
/*  2.  parity: the same arena driven through both faces                    */
/* ------------------------------------------------------------------------ */
static void
test_arena_parity()
{
    static char buf_c[16384];
    static char buf_cpp[16384];

    ::d_arena c_arena;
    arena     cpp_arena(buf_cpp, static_cast<mem_size>(sizeof(buf_cpp)));

    CHECK(::d_arena_init_buffer(&c_arena, buf_c,
                                static_cast<mem_size>(sizeof(buf_c))) ==
          D_MEM_OK);

    /* identical geometry from identical inputs */
    CHECK(::d_arena_capacity(&c_arena) == cpp_arena.capacity());

    /* identical allocation sequence -> identical OFFSETS. Not identical
       addresses: the two buffers are elsewhere. Offsets are the observable
       that parity actually claims. */
    for (int i = 0; i < 50; ++i)
    {
        mem_size bytes = static_cast<mem_size>((i * 7) % 61) + 1;
        mem_size align = static_cast<mem_size>(1u << (i % 5));

        void* from_c   = ::d_arena_allocate(&c_arena, bytes, align);
        void* from_cpp = cpp_arena.allocate(bytes, align);

        CHECK(from_c != nullptr);
        CHECK(from_cpp != nullptr);

        if (from_c && from_cpp)
        {
            CHECK((static_cast<char*>(from_c) - buf_c) ==
                  (static_cast<char*>(from_cpp) - buf_cpp));
        }
    }

    CHECK(::d_arena_used(&c_arena) == cpp_arena.used());

    ::d_arena_release(&c_arena);

    return;
}


/* ------------------------------------------------------------------------ */
/*  3.  parity: pool geometry computed early vs computed late               */
/* ------------------------------------------------------------------------ */
static void
test_pool_geometry_parity()
{
    /*   pool<T> supplies sizeof(T) and alignof(T) at translation time; the C
     * kernel is told them at run time. The claim is that the two arrive at
     * the SAME stride and alignment -- earlier, not differently. */
    ::d_pool_config c_cfg = ::d_pool_config_default(
        static_cast<mem_size>(sizeof(thing)),
        static_cast<mem_size>(alignof(thing)));

    ::d_pool c_pool;
    CHECK(::d_pool_init(&c_pool, &c_cfg) == D_MEM_OK);

    pool<thing> cpp_pool;

    CHECK(::d_pool_slot_size(&c_pool)  == cpp_pool.slot_size());
    CHECK(::d_pool_slot_align(&c_pool) == cpp_pool.slot_align());

    /* and the geometry helpers agree with the pools they describe */
    CHECK(::d_pool_align_for(static_cast<mem_size>(alignof(thing)),
                             D_POOL_POLICY_FREE_LIST) ==
          cpp_pool.slot_align());

    /* identical acquire sequences yield identical INDICES */
    for (int i = 0; i < 40; ++i)
    {
        void*  from_c   = ::d_pool_acquire(&c_pool);
        thing* from_cpp = cpp_pool.acquire();

        CHECK(from_c != nullptr);
        CHECK(from_cpp != nullptr);

#if (D_INTERNAL_POOL_OWNS == 1)
        if (from_c && from_cpp)
        {
            CHECK(::d_pool_index_of(&c_pool, from_c) ==
                  ::d_pool_index_of(&cpp_pool, from_cpp));
        }
#endif
    }

    CHECK(::d_pool_size(&c_pool)     == cpp_pool.size());
    CHECK(::d_pool_capacity(&c_pool) == cpp_pool.capacity());

    ::d_pool_release(&c_pool);

    return;
}


/* ------------------------------------------------------------------------ */
/*  4.  the capability the kernel cannot have: object lifetime              */
/* ------------------------------------------------------------------------ */
static void
test_object_lifetime()
{
    pool<counted> p;

    live_things = 0;

    {
        counted* objects[32];

        for (int i = 0; i < 32; ++i)
        {
            objects[i] = p.create(i);
            CHECK(objects[i] != nullptr);
        }

        CHECK(live_things == 32);

        for (int i = 0; i < 32; ++i)
        {
            CHECK(objects[i]->value == i);
        }

        /* destroy() runs the destructor BEFORE recycling the slot; the C
           kernel cannot, because it does not know a slot holds an object */
        for (int i = 0; i < 32; ++i)
        {
            CHECK(p.destroy(objects[i]) == D_MEM_OK);
        }

        CHECK(live_things == 0);
        CHECK(p.size() == 0);
    }

    /*   release_slot on the raw face deliberately does NOT destruct, and the
     * count proves the difference is real rather than notional. */
    live_things = 0;
    {
        counted* leaked = p.create(1);

        CHECK(live_things == 1);
        CHECK(p.release_slot(static_cast<void*>(leaked)) == D_MEM_OK);
        CHECK(live_things == 1);   /* raw release skipped the destructor */

        live_things = 0;
    }

    return;
}


/* ------------------------------------------------------------------------ */
/*  5.  handles survive what pointers do not                                */
/* ------------------------------------------------------------------------ */
static void
test_handles()
{
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    pool<thing, D_POOL_POLICY_GENERATIONAL> p;

    pool<thing, D_POOL_POLICY_GENERATIONAL>::handle_type h = p.create_handle(42u);

    CHECK(!h.is_null());
    CHECK(p.is_live(h));

    thing* t = p.resolve(h);
    CHECK(t != nullptr);

    if (t)
    {
        CHECK(t->tag == 42u);
    }

    CHECK(p.destroy_handle(h) == D_MEM_OK);
    CHECK(!p.is_live(h));
    CHECK(p.resolve(h) == nullptr);

    /* the slot comes back under a new handle, at the same address */
    pool<thing, D_POOL_POLICY_GENERATIONAL>::handle_type h2 = p.create_handle(7u);

    CHECK(p.resolve(h2) == t);
    CHECK(p.resolve(h) == nullptr);
    CHECK(h != h2);

    /* destroying a spent handle is reported, not double-destroyed */
    CHECK(p.destroy_handle(h) == D_MEM_ERR_STALE);

    /* a default-constructed handle is null and never resolves */
    pool<thing, D_POOL_POLICY_GENERATIONAL>::handle_type fresh;
    CHECK(fresh.is_null());
    CHECK(p.resolve(fresh) == nullptr);
#else
    ++skipped;
#endif

    return;
}


/* ------------------------------------------------------------------------ */
/*  6.  the scope guard rewinds on every path                               */
/* ------------------------------------------------------------------------ */
static void
test_arena_scope()
{
#if (D_INTERNAL_ARENA_MARKS == 1)
    arena    a;
    mem_size before;
    void*    first;

    first  = a.allocate(64, 8);
    before = a.used();

    CHECK(first != nullptr);

    {
        arena_scope guard(a);

        a.allocate(4096, 16);
        a.allocate(4096, 16);
        CHECK(a.used() > before);
    }

    CHECK(a.used() == before);

    /* nesting */
    {
        arena_scope outer(a);
        a.allocate(128, 8);

        {
            arena_scope inner(a);
            a.allocate(256, 8);
        }

        CHECK(a.used() > before);
    }

    CHECK(a.used() == before);

    /* dismiss keeps the allocations */
    {
        arena_scope guard(a);
        a.allocate(512, 8);
        guard.dismiss();
    }

    CHECK(a.used() > before);
#else
    ++skipped;
#endif

    return;
}


/* ------------------------------------------------------------------------ */
/*  7.  standard containers over the adapters                               */
/* ------------------------------------------------------------------------ */
static void
test_allocators()
{
    /* a node-based container over a pool: one block per many nodes */
    {
        using list_t = std::list<int, pool_allocator<int>>;

        node_pool nodes(d_mem_source_default(), 64);
        list_t    items((pool_allocator<int>(nodes)));

        for (int i = 0; i < 500; ++i)
        {
            items.push_back(i);
        }

        CHECK(items.size() == 500);
        CHECK(nodes.is_configured());
        CHECK(nodes.size() == 500);        /* every node came from the pool */
        CHECK(nodes.slot_size() >= sizeof(int) + sizeof(void*) * 2);
        CHECK(nodes.block_count() <= 9);   /* 500 nodes, 64 per block */

        int expect = 0;

        for (list_t::const_iterator it = items.begin();
             it != items.end();
             ++it)
        {
            CHECK(*it == expect);
            ++expect;
        }

        /* erasing returns slots, and refilling reuses them without growing */
        mem_size blocks = nodes.block_count();

        items.clear();
        CHECK(nodes.size() == 0);

        for (int i = 0; i < 500; ++i)
        {
            items.push_back(i);
        }

        CHECK(nodes.block_count() == blocks);
    }

    /* a vector over an arena: allocations never come back individually */
    {
        arena                             a;
        std::vector<int, arena_allocator<int>> v((arena_allocator<int>(a)));

        v.reserve(1000);

        for (int i = 0; i < 1000; ++i)
        {
            v.push_back(i);
        }

        CHECK(v.size() == 1000);
        CHECK(v[999] == 999);
        CHECK(a.used() >= (1000 * sizeof(int)));

#if (D_INTERNAL_ARENA_OWNS == 1)
        CHECK(a.owns(&v[0]));
#endif
    }

    /* a map, to prove rebinding to an unnameable node type works */
    {
        using map_t = std::map<int, int, std::less<int>,
                               pool_allocator<std::pair<const int, int>>>;

        node_pool nodes(d_mem_source_default(), 32);
        map_t     m((std::less<int>()),
                    pool_allocator<std::pair<const int, int>>(nodes));

        for (int i = 0; i < 200; ++i)
        {
            m[i] = i * 2;
        }

        CHECK(m.size() == 200);
        CHECK(m[150] == 300);
        CHECK(nodes.size() == 200);
    }

    return;
}


/* ------------------------------------------------------------------------ */
/*  8.  composition: a pool carved out of an arena over a static array      */
/* ------------------------------------------------------------------------ */
static void
test_composition()
{
#if (D_INTERNAL_ARENA_AS_SOURCE == 1)
    static char storage[131072];

    arena       backing(storage, static_cast<mem_size>(sizeof(storage)));
    pool<thing> p(backing.as_source(), 32);

    for (int i = 0; i < 200; ++i)
    {
        thing* t = p.create(static_cast<unsigned>(i));

        CHECK(t != nullptr);

        if (t)
        {
            CHECK(t->tag == static_cast<unsigned>(i));
#if (D_INTERNAL_ARENA_OWNS == 1)
            CHECK(backing.owns(t));   /* every byte from the static array */
#endif
        }
    }

    CHECK(p.size() == 200);
    CHECK(backing.used() > 0);
#else
    ++skipped;
#endif

    return;
}


/* ------------------------------------------------------------------------ */
/*  9.  move semantics do not disturb the kernel's invariants               */
/* ------------------------------------------------------------------------ */
static void
test_moves()
{
    arena a;
    void* p = a.allocate(256, 16);

    CHECK(p != nullptr);
    std::memset(p, 0x3C, 256);

    arena moved(std::move(a));

    CHECK(static_cast<unsigned char*>(p)[0]   == 0x3C);
    CHECK(static_cast<unsigned char*>(p)[255] == 0x3C);
#if (D_INTERNAL_ARENA_OWNS == 1)
    CHECK(moved.owns(p));
    CHECK(!a.owns(p));       /* the moved-from arena kept nothing */
#endif
    CHECK(a.capacity() == 0);

    pool<thing> tp;
    thing*      t = tp.create(9u);

    CHECK(t != nullptr);

    pool<thing> tp2(std::move(tp));

    CHECK(t->tag == 9u);
    CHECK(tp2.size() == 1);
    CHECK(tp.capacity() == 0);

    return;
}


/* heap_available
 *   Reports whether this configuration's default source can vend anything.
 * The MINIMAL preset compiles no system source at all, and the sections that
 * construct a default-source allocator have nothing to run against there.
 */
static bool
heap_available()
{
    memory_source s = memory_source::default_source();
    void*         p = s.allocate(64, 0);

    if (!p)
    {
        return false;
    }

    s.release(p, 64, 0);

    return true;
}


int
main()
{
    const bool heap = heap_available();

    std::printf("c++ parity harness: heap=%s stats=%d redzone=%d "
                "generational=%d size_bits=%d\n",
                heap ? "yes" : "no",
                static_cast<int>(D_INTERNAL_MEM_STATS),
                static_cast<int>(D_INTERNAL_MEM_REDZONE),
                static_cast<int>(D_INTERNAL_POOL_GENERATIONAL),
                static_cast<int>(D_INTERNAL_MEM_SIZE_BITS));

    /* these two need no heap in any configuration */
    test_arena_parity();
    test_composition();

    if (heap)
    {
        test_cost_law();
        test_pool_geometry_parity();
        test_object_lifetime();
        test_handles();
        test_arena_scope();
        test_allocators();
        test_moves();
    }
    else
    {
        std::printf("c++ parity harness: no heap in this configuration; "
                    "heap sections skipped\n");
        skipped += 7;
    }

    std::printf("c++ parity harness: %d section(s) skipped by configuration\n",
                skipped);
    std::printf(fails ? "CPP: %d FAILURE(S)\n" : "CPP: all checks passed\n",
                fails);

    return fails ? 1 : 0;
}

#endif  // floor, for now
