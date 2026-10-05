/*******************************************************************************
* djinterp [c]                                                    t_strategy.cpp
*
*   Memory-strategy conformance harness. Writes ONE container against the
* strategy contract and instantiates it over every binding, checking that each
* strategy's declared constants match what it actually does. Draws every byte
* from a static array, so it runs identically on a heapless build.
*
*   Build (from the repo root):
*     cc  src/djinterp/c/memory/mem_common.c \
*         src/djinterp/c/memory/mem_source.c \
*         src/djinterp/c/memory/arena.c \
*         src/djinterp/c/memory/pool.c \
*         src/djinterp/c/memory/test/t_strategy.cpp -o t_strategy
*   No -I is required: every include is repo-root relative.
*
*
* path:      /src/djinterp/c/memory/test/t_strategy.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

/*   Memory-strategy conformance harness.
 *   The compile-time half is already done: memory_strategy.hpp asserts every
 * strategy against the contract at its point of definition, so this file
 * compiling at all proves the classification. What remains is behavioural --
 * that a container written ONCE against the contract really does work over
 * all four strategies, and that the descriptive constants tell the truth
 * about what each one does.
 */
// FLOOR, FOR NOW: below C++11 this test is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
// (it includes core/memory, whose headers need C++11, so the gate comes
// before every include)
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#include <cstdio>
#include <memory>
#include "../../../../../inc/djinterp/core/memory/memory.hpp"

using namespace djinterp;

static int fails = 0;

#define CHECK(c)                                                              \
    do                                                                        \
    {                                                                         \
        if (!(c))                                                             \
        {                                                                     \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);        \
            ++fails;                                                          \
        }                                                                     \
    } while (0)


/*   A container written ONCE, against the contract and nothing else. It never
 * names an arena, a pool, an allocator or a buffer -- it reads the constants
 * and adapts. This is the whole reason the strategy tier exists, so the test
 * has to be a real instance of it rather than a call to allocate(). */
template<typename Strategy>
class stack_of
{
public:

    using strategy_type = Strategy;
    using value_type    = typename Strategy::value_type;

    explicit stack_of(Strategy& _s)
        : m_s(&_s), m_count(0)
    {
        for (int i = 0; i < 32; ++i)
        {
            m_items[i] = nullptr;
        }
    }

    bool push(const value_type& _v)
    {
        if (m_count >= 32)
        {
            return false;
        }

        try
        {
            /* an element strategy vends one object; a byte strategy vends
               bytes, so ask for the object's worth of them */
            m_items[m_count] = allocate_one(
                std::integral_constant<bool,
                    is_byte_strategy<Strategy>::value>());
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }

        if (!m_items[m_count])
        {
            return false;
        }

        *m_items[m_count] = _v;
        ++m_count;

        return true;
    }

    /*   The container reads supports_individual_release and does the right
     * thing. Over an arena this is a no-op that keeps the slot; over a pool it
     * genuinely returns it. Same source line, two behaviours, chosen at
     * translation time. */
    void pop()
    {
        if (m_count == 0)
        {
            return;
        }

        --m_count;
        release_one(m_items[m_count],
                    std::integral_constant<bool,
                        is_releasing_strategy<Strategy>::value>());
        m_items[m_count] = nullptr;
    }

    std::size_t   size() const { return m_count; }
    value_type*   at(std::size_t _i) const { return m_items[_i]; }

private:

    value_type* allocate_one(std::true_type)   /* byte strategy */
    {
        return reinterpret_cast<value_type*>(
            m_s->allocate(sizeof(value_type)));
    }

    value_type* allocate_one(std::false_type)  /* element strategy */
    {
        return m_s->allocate(1);
    }

    void release_one(value_type* _p, std::true_type)
    {
        release_typed(_p,
                      std::integral_constant<bool,
                          is_byte_strategy<Strategy>::value>());
    }

    void release_one(value_type*, std::false_type) {}

    void release_typed(value_type* _p, std::true_type)
    {
        m_s->deallocate(reinterpret_cast<unsigned char*>(_p),
                        sizeof(value_type));
    }

    void release_typed(value_type* _p, std::false_type)
    {
        m_s->deallocate(_p, 1);
    }

    Strategy*  m_s;
    value_type* m_items[32];
    std::size_t m_count;
};


template<typename Strategy>
static void
exercise(const char* _name, Strategy& _s)
{
    stack_of<Strategy> stack(_s);
    using v = typename Strategy::value_type;

    for (int i = 0; i < 20; ++i)
    {
        CHECK(stack.push(static_cast<v>(i + 1)));
    }

    CHECK(stack.size() == 20);

    /* every slot is distinct and holds what was written */
    for (int i = 0; i < 20; ++i)
    {
        CHECK(*stack.at(i) == static_cast<v>(i + 1));

        for (int j = i + 1; j < 20; ++j)
        {
            CHECK(stack.at(i) != stack.at(j));
        }
    }

    /* stability: if the strategy claims it, every earlier pointer must still
       be valid after more allocation */
    if (is_stable_strategy<Strategy>::value)
    {
        v* first = stack.at(0);

        for (int i = 0; i < 10; ++i)
        {
            stack.push(static_cast<v>(99));
        }

        CHECK(stack.at(0) == first);
        CHECK(*first == static_cast<v>(1));
    }

    while (stack.size() > 0)
    {
        stack.pop();
    }

    CHECK(stack.size() == 0);

    std::printf("    %-26s kind=%d stable=%d release=%d sweep=%d\n",
                _name,
                static_cast<int>(strategy_storage_kind_of<Strategy>::value),
                static_cast<int>(is_stable_strategy<Strategy>::value),
                static_cast<int>(is_releasing_strategy<Strategy>::value),
                static_cast<int>(is_sweeping_strategy<Strategy>::value));
}


int main()
{
    /*   Every section below draws from a STATIC ARRAY rather than the default
     * source, so the harness runs identically on a build with no heap at all
     * -- which the MINIMAL preset is. That is not a workaround: heapless is a
     * first-class target, and a strategy tier that only worked over malloc
     * would be a strategy tier that missed the point. */
    static char storage[262144];
    buffer_source bytes(storage);

    std::printf("strategy harness: one container, four strategies "
                "(heapless: all storage from a static array)\n");

    {
        arena a(bytes.source());
        arena_memory_strategy<double> s(a);
        exercise("arena_memory_strategy", s);
        /* the arena kept everything: individual release is a no-op */
        CHECK(a.used() > 0);
    }

    {
        pool<double> p(bytes.source());
        pool_memory_strategy<pool<double> > s(p);
        exercise("pool_memory_strategy", s);
        /* the pool genuinely took every slot back */
        CHECK(p.size() == 0);
        CHECK(p.capacity() > 0);
    }

    {
        pool<double, D_POOL_POLICY_MONOTONIC> p(bytes.source());
        pool_memory_strategy<pool<double, D_POOL_POLICY_MONOTONIC> > s(p);
        exercise("pool_memory_strategy(mono)", s);
        /* a monotonic pool declares no individual release, so the container
           never tried -- and the slots are still held */
        CHECK(!is_releasing_strategy<decltype(s)>::value);
        CHECK(p.size() == 30);
    }

    {
        buffer_memory_strategy<double, 64> s;
        exercise("buffer_memory_strategy", s);
        CHECK(s.capacity() == 64);
        CHECK(strategy_extent_of<decltype(s)>::value == 64);
    }

    {
        allocator_memory_strategy<std::allocator<double> > s;
        exercise("allocator_memory_strategy", s);
    }

    /* exhaustion of a static strategy is permanent and reported */
    {
        buffer_memory_strategy<int, 4> s;
        bool threw = false;

        s.allocate(4);

        try
        {
            s.allocate(1);
        }
        catch (const std::bad_alloc&)
        {
            threw = true;
        }

        CHECK(threw);
        s.reset();
        CHECK(s.remaining() == 4);
    }

    /* a generational pool projects its sweep capability */
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    {
        using gp = pool<double, D_POOL_POLICY_GENERATIONAL>;
        CHECK((is_sweeping_strategy<pool_memory_strategy<gp> >::value));
        CHECK((!is_sweeping_strategy<
                   pool_memory_strategy<pool<double> > >::value));
    }
#endif

    std::printf(fails ? "STRATEGY: %d FAILURE(S)\n"
                      : "STRATEGY: all checks passed\n", fails);

    return fails ? 1 : 0;
}

#endif  // floor, for now
