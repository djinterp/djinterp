/*******************************************************************************
* djinterp [core]                                                sync_common.hpp
*
* Shared foundations for the thread-safe sync module.
*   Container-agnostic utilities that several sync submodules were each
* hand-rolling: ownership base classes, an adaptive spin backoff, and a
* scope-exit guard. Extracting them here removes the duplication and gives
* the whole module one spelling of each idiom.
*
* TYPES / UTILITIES:
*   noncopyable       - base that deletes copy, keeps move
*   nonmovable        - base that deletes copy AND move (pinned in place)
*   D_NONCOPYABLE(T)  - in-class macro form of the above (copy)
*   D_NONMOVABLE(T)   - in-class macro form (copy + move)
*   cpu_relax()       - single CPU pause hint for spin loops
*   backoff           - adaptive exponential spin, escalating to yield
*   scope_guard<Fn>   - runs a callable on scope exit unless dismissed
*   make_scope_guard  - deduces scope_guard<Fn> from a callable (pre-C++17)
*
* NOTE (thread_safety_level): the synchronization-guarantee enum currently
*   lives twice - as thread_safety_level in lock_policy.hpp (C++) and as
*   d_thread_safety_levels in threadsafe_common.h (C). Unifying those onto
*   one definition is a separate change that touches both of those files, so
*   it is deliberately NOT redefined here - a third copy would only make the
*   drift worse.
*
* VERSIONING:
*   C++98/03:  unavailable (requires deleted members / <thread>)
*   C++11:     all utilities available
*
*
* path:      /inc/djinterp/core/sync/sync_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    OWNERSHIP BASES
      ---------------

II.   SPIN BACKOFF
      ------------

III.  SCOPE GUARD
      -----------
*/

#ifndef DJINTERP_SYNC_SYNC_COMMON_HPP
#define DJINTERP_SYNC_SYNC_COMMON_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <thread>
#include <utility>
#if defined(_MSC_VER)
    // msvc
    #include <intrin.h>
#endif
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP

// I.    Ownership bases
// Two tiny bases that state, once, what a type's copy/move story is - so the
// sync types stop hand-writing four deleted/defaulted members apiece. Both
// have protected special members, so they are usable only as a base.

// noncopyable
//   base: deletes the copy operations while leaving move intact. A type
// that owns a unique resource but may still be relocated (moved into a
// container, returned from a factory) inherits this privately to say so.
class noncopyable
{
protected:
    noncopyable()  noexcept        = default;
    ~noncopyable()                 = default;

    noncopyable(noncopyable&&)            noexcept = default;
    noncopyable& operator=(noncopyable&&) noexcept = default;

    noncopyable(const noncopyable&)            = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

// nonmovable
//   base: deletes copy AND move - the type is pinned at its address for its
// whole lifetime. The right choice for anything others hold a pointer or
// reference INTO (mutexes, epoch slots, registries): relocating it would
// leave those aliases dangling.
class nonmovable
{
protected:
    nonmovable()  noexcept = default;
    ~nonmovable()          = default;

    nonmovable(const nonmovable&)            = delete;
    nonmovable& operator=(const nonmovable&) = delete;
    nonmovable(nonmovable&&)                 = delete;
    nonmovable& operator=(nonmovable&&)      = delete;
};

// D_NONCOPYABLE / D_NONMOVABLE
//   macro: the same intent placed directly in a class body, for a type that
// already has a base or would rather not inherit. Use inside the class:
//     class widget { D_NONCOPYABLE(widget) public: ... };
//   D_NONCOPYABLE mirrors the noncopyable base - it deletes copy but keeps
// move, by defaulting the move members explicitly (a bare deleted copy ctor
// would otherwise SUPPRESS the implicit move and leave the type immovable).
#define D_NONCOPYABLE(_Type)                    \
    _Type(const _Type&)            = delete;    \
    _Type& operator=(const _Type&) = delete;    \
    _Type(_Type&&)                 = default;   \
    _Type& operator=(_Type&&)      = default;

#define D_NONMOVABLE(_Type)                     \
    _Type(const _Type&)            = delete;    \
    _Type& operator=(const _Type&) = delete;    \
    _Type(_Type&&)                 = delete;    \
    _Type& operator=(_Type&&)      = delete;


// II.   Spin backoff
// The module has spin loops in several places (the C spinlock, the hazard
// scan, RCU) and none of them backed off - they either burned the core or
// (the C spinlock) spun raw. These two pieces give them one shared, gentle
// wait: a hardware pause hint, and an adaptive escalation to a yield.

// cpu_relax
//   function: a single hardware "pause"/"yield" hint that tells the CPU it
// is spin-waiting, so it can save power and dodge the memory-order pipeline
// flush a tight read loop otherwise causes. Not a scheduling yield - just
// one relax of the spin.
inline void
cpu_relax() noexcept
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    __builtin_ia32_pause();
#elif defined(__GNUC__) && (defined(__aarch64__) || defined(__arm__))
    __asm__ __volatile__("yield" ::: "memory");
#elif defined(_MSC_VER)
    _mm_pause();
#else
    // no hardware hint on this target; backoff still escalates to a yield
#endif
}

// backoff
//   type: adaptive spin-wait. Early pause() calls relax the CPU an
// exponentially growing number of times; past a cap they hand the core to
// the scheduler with std::this_thread::yield(). A spin loop keeps one
// backoff, calls pause() each failed turn, and reset()s on progress.
//   This bridges the two layers the module already has - the atomic pause
// (datomic) and the thread yield (dmutex / condvar) - which no spin site
// had combined.
class backoff
{
public:
    backoff() noexcept
        : m_step(0)
    {}

    // pause
    //   spins with cpu_relax() while the step is small, then yields.
    void pause() noexcept
    {
        if (m_step < k_spin_cap)
        {
            const unsigned spins = 1u << m_step;

            for (unsigned i = 0; i < spins; ++i)
            {
                cpu_relax();
            }

            ++m_step;
        }
        else
        {
            std::this_thread::yield();
        }
    }

    // reset
    //   returns to the shortest spin - call after making progress.
    void reset() noexcept
    {
        m_step = 0;
    }

private:
    unsigned m_step;

    static const unsigned k_spin_cap = 6;
};


// III.  Scope guard
// The general form of "release in the destructor" that the lock / hazard /
// epoch guards each special-case. Use it for ad hoc cleanup - unregister a
// slot, restore a flag - without writing a bespoke guard class.

// scope_guard
//   type: runs a stored callable when it leaves scope, unless dismiss() was
// called first. Non-copyable; movable so it can be returned from a factory
// (the moved-from guard is disarmed).
template<typename Fn>
class scope_guard
{
public:
    explicit scope_guard(Fn _fn)
        : m_fn(std::move(_fn))
        , m_armed(true)
    {}

    scope_guard(scope_guard&& _other)
        : m_fn(std::move(_other.m_fn))
        , m_armed(_other.m_armed)
    {
        _other.m_armed = false;
    }

    scope_guard(const scope_guard&)            = delete;
    scope_guard& operator=(const scope_guard&) = delete;
    scope_guard& operator=(scope_guard&&)      = delete;

    ~scope_guard()
    {
        if (m_armed)
        {
            m_fn();
        }
    }

    // dismiss
    //   cancels the pending action - the callable will NOT run.
    void dismiss() noexcept
    {
        m_armed = false;
    }

private:
    Fn  m_fn;
    bool m_armed;
};

// make_scope_guard
//   function: deduces scope_guard<Fn> from a callable, standing in for class
// template argument deduction on pre-C++17 compilers.
//     auto g = make_scope_guard([&]{ cleanup(); });
template<typename Fn>
scope_guard<Fn>
make_scope_guard(Fn _fn)
{
    return scope_guard<Fn>(std::move(_fn));
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_SYNC_SYNC_COMMON_HPP
