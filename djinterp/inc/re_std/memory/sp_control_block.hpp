/*******************************************************************************
* djinterp [re_std]                                         sp_control_block.hpp
*
* sp_control_block class header:
* shared_ptr / weak_ptr control block hierarchy and atomic counters.
*
* this is an INTERNAL header. The control block is an implementation
* detail of shared_ptr and weak_ptr; user code should never name these
* types directly.
*
* layout invariant:
*   m_use_count    strong references (shared_ptr's holding the object).
*                  When this reaches 0, dispose() runs on the managed
*                  object.
*   m_weak_count   weak references plus 1 if m_use_count > 0. Holding
*                  +1 from "use_count > 0" prevents the cb from being
*                  destroyed while strong refs exist. When this reaches
*                  0, destroy() runs and the cb itself is deallocated.
*
* concrete cb variants:
*   sp_cb_pointer<U, D>          allocated separately from the object,
*                                  holds the U* and a deleter D.
*   sp_cb_inplace<U>              single-allocation cb that holds U
*                                  inline. Used by make_shared.
*   sp_cb_alloc_inplace<U, _A>    single-allocation cb with an
*                                  allocator copy. Used by
*                                  allocate_shared. destroy() rebinds
*                                  and deallocates self via _A.
*
* NOT implemented in this phase (will ship in 4b):
*   sp_cb_pointer_alloc            for shared_ptr(p, d, alloc) ctors.
*
* atomic refcounts:
*   RE_STD_HAS_SP_ATOMICS         1 when compiler supports __atomic_*
*                                  builtins. GCC, Clang, Intel are
*                                  detected. MSVC support TODO.
*
* When atomics are unavailable, the cb falls back to plain int ops.
* This is single-thread-correct; multi-thread usage of shared_ptr on
* such a configuration is UNSAFE. Document accordingly.
*
*
* path:      /inc/re_std/memory/sp_control_block.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_SP_CONTROL_BLOCK_HPP
#define RE_STD_MEMORY_SP_CONTROL_BLOCK_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>

    #if RE_STD_HAS_RTTI
        // std
        #include <typeinfo>
    #endif

    #if RE_STD_HAS_HEADER_NEW
        // std
        #include <new>
    #endif

    #include "re_std/memory/allocator_traits.hpp"
    #include "re_std/utility/forward.hpp"
    #include "re_std/utility/move.hpp"


// =============================================================================
// RE_STD_HAS_SP_ATOMICS
// =============================================================================

#ifndef RE_STD_HAS_SP_ATOMICS
    #if defined(__has_builtin)
        #if __has_builtin(__atomic_fetch_add) && __has_builtin(__atomic_load_n)
            #define RE_STD_HAS_SP_ATOMICS 1
        #else
            #define RE_STD_HAS_SP_ATOMICS 0
        #endif
    #elif defined(RE_STD_COMPILER_GCC) || defined(RE_STD_COMPILER_INTEL)
        #define RE_STD_HAS_SP_ATOMICS 1
    #else
        // MSVC requires _Interlocked* — different surface, deferred.
        #define RE_STD_HAS_SP_ATOMICS 0
    #endif
#endif


namespace re_std
{
namespace internal
{

// =============================================================================
// atomic counter helpers
// =============================================================================

typedef long sp_count_t;

#if RE_STD_HAS_SP_ATOMICS

    inline sp_count_t sp_atomic_load(const sp_count_t* _p) RE_STD_NOEXCEPT
    {
        return __atomic_load_n(_p, __ATOMIC_ACQUIRE);
    }

    // Returns the PREVIOUS value (not the new one).
    inline sp_count_t sp_atomic_inc(sp_count_t* _p) RE_STD_NOEXCEPT
    {
        return __atomic_fetch_add(_p, 1, __ATOMIC_ACQ_REL);
    }

    // Returns the PREVIOUS value (not the new one).
    inline sp_count_t sp_atomic_dec(sp_count_t* _p) RE_STD_NOEXCEPT
    {
        return __atomic_fetch_sub(_p, 1, __ATOMIC_ACQ_REL);
    }

    // CAS-loop incrementer. Returns true if successfully incremented
    // (i.e. counter was nonzero), false if counter was 0.
    inline bool sp_atomic_inc_if_nonzero(sp_count_t* _p) RE_STD_NOEXCEPT
    {
        sp_count_t _expected = __atomic_load_n(_p, __ATOMIC_RELAXED);
        for (;;)
        {
            if (_expected == 0)
            {
                return false;
            }
            if (__atomic_compare_exchange_n(
                    _p, &_expected, _expected + 1,
                    true /* weak */,
                    __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
            {
                return true;
            }
        }
    }

#else  // !RE_STD_HAS_SP_ATOMICS

    // Fallback: plain int ops. Single-thread only.

    inline sp_count_t sp_atomic_load(const sp_count_t* _p) RE_STD_NOEXCEPT
    {
        return *_p;
    }

    inline sp_count_t sp_atomic_inc(sp_count_t* _p) RE_STD_NOEXCEPT
    {
        sp_count_t _old = *_p;
        ++*_p;
        return _old;
    }

    inline sp_count_t sp_atomic_dec(sp_count_t* _p) RE_STD_NOEXCEPT
    {
        sp_count_t _old = *_p;
        --*_p;
        return _old;
    }

    inline bool sp_atomic_inc_if_nonzero(sp_count_t* _p) RE_STD_NOEXCEPT
    {
        if (*_p == 0)
        {
            return false;
        }
        ++*_p;
        return true;
    }

#endif  // RE_STD_HAS_SP_ATOMICS


// =============================================================================
// sp_control_block_base
// =============================================================================

class sp_control_block_base
{
public:
    sp_count_t m_use_count;
    sp_count_t m_weak_count;

    sp_control_block_base() RE_STD_NOEXCEPT
        : m_use_count(1)
        , m_weak_count(1)
    {
    }

    virtual ~sp_control_block_base() RE_STD_NOEXCEPT
    {
    }

    // Polymorphic destruction strategies.
    virtual void dispose() RE_STD_NOEXCEPT = 0;   // destroy managed object
    virtual void destroy() RE_STD_NOEXCEPT = 0;   // destroy this cb

    #if RE_STD_HAS_RTTI
        virtual void* get_deleter(const std::type_info&) RE_STD_NOEXCEPT
        {
            return 0;
        }
    #endif

    // ---- ref-count operations ----

    void add_ref() RE_STD_NOEXCEPT
    {
        sp_atomic_inc(&m_use_count);
    }

    bool add_ref_if_nonzero() RE_STD_NOEXCEPT
    {
        return sp_atomic_inc_if_nonzero(&m_use_count);
    }

    void release() RE_STD_NOEXCEPT
    {
        if (sp_atomic_dec(&m_use_count) == 1)
        {
            // strong count went 1 -> 0
            dispose();
            weak_release();
        }
    }

    void weak_add_ref() RE_STD_NOEXCEPT
    {
        sp_atomic_inc(&m_weak_count);
    }

    void weak_release() RE_STD_NOEXCEPT
    {
        if (sp_atomic_dec(&m_weak_count) == 1)
        {
            destroy();
        }
    }

    sp_count_t use_count() const RE_STD_NOEXCEPT
    {
        return sp_atomic_load(&m_use_count);
    }

private:
    // copying a control block would duplicate its counts; it is not
    // copyable at any tier (decision 3.3: private, so C++98's undefined
    // declaration is as inaccessible as C++11's deleted one).
    RE_STD_DELETED_FN(sp_control_block_base(const sp_control_block_base&))
    RE_STD_DELETED_FN(sp_control_block_base& operator=(
                     const sp_control_block_base&))
};


// =============================================================================
// sp_cb_pointer  -  for shared_ptr(ptr) and shared_ptr(ptr, deleter)
// =============================================================================

// Allocated separately from the managed object. The cb holds a pointer
// to the object (so dispose can reach it) and the deleter.
template<typename U, typename D>
class sp_cb_pointer : public sp_control_block_base
{
    U* m_ptr;
    D  m_del;

public:
    sp_cb_pointer(U* _p, D _d)
        : m_ptr(_p)
        , m_del(re_std::move(_d))
    {
    }

    void dispose() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        m_del(m_ptr);
    }

    void destroy() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        delete this;
    }

    #if RE_STD_HAS_RTTI
        void* get_deleter(const std::type_info& _ti) RE_STD_NOEXCEPT RE_STD_OVERRIDE
        {
            if (_ti == typeid(D))
            {
                return &m_del;
            }
            return 0;
        }
    #endif
};


// =============================================================================
// sp_cb_inplace  -  for make_shared
// =============================================================================

// Tag for for_overwrite construction.
//   Used to disambiguate value-init (cb(...args)) from default-init
//   (cb(sp_for_overwrite_t{})).
struct sp_for_overwrite_t
{
};


// Holds the object inline. Single allocation: cb + object live together.
template<typename U>
class sp_cb_inplace : public sp_control_block_base
{
    alignas(U) char m_storage[sizeof(U)];

    U* obj_ptr() RE_STD_NOEXCEPT
    {
        return reinterpret_cast<U*>(&m_storage[0]);
    }

public:
    template<typename... Args>
    explicit sp_cb_inplace(Args&&... _args)
    {
        ::new (static_cast<void*>(&m_storage[0]))
            U(re_std::forward<Args>(_args)...);
    }

    // For make_shared_for_overwrite: default-initialise (no parens).
    explicit sp_cb_inplace(sp_for_overwrite_t)
    {
        ::new (static_cast<void*>(&m_storage[0])) U;
    }

    U* get() RE_STD_NOEXCEPT
    {
        return obj_ptr();
    }

    void dispose() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        obj_ptr()->~U();
    }

    void destroy() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        delete this;
    }
};


// =============================================================================
// sp_cb_alloc_inplace  -  for allocate_shared
// =============================================================================

// Holds the object inline AND a copy of the allocator. The allocator
// is used to deallocate the cb itself (after destructing this).
template<typename U, typename Alloc>
class sp_cb_alloc_inplace : public sp_control_block_base
{
    alignas(U) char m_storage[sizeof(U)];
    Alloc m_alloc;

    U* obj_ptr() RE_STD_NOEXCEPT
    {
        return reinterpret_cast<U*>(&m_storage[0]);
    }

public:
    template<typename... Args>
    sp_cb_alloc_inplace(const Alloc& _a, Args&&... _args)
        : m_alloc(_a)
    {
        ::new (static_cast<void*>(&m_storage[0]))
            U(re_std::forward<Args>(_args)...);
    }

    // For allocate_shared_for_overwrite: default-initialise (no parens).
    sp_cb_alloc_inplace(const Alloc& _a, sp_for_overwrite_t)
        : m_alloc(_a)
    {
        ::new (static_cast<void*>(&m_storage[0])) U;
    }

    U* get() RE_STD_NOEXCEPT
    {
        return obj_ptr();
    }

    void dispose() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        obj_ptr()->~U();
    }

    void destroy() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        // The cb was allocated via a rebound Alloc. To deallocate it,
        // we need a fresh rebind. Critical ordering: take a COPY of
        // the allocator BEFORE destructing self (which destructs
        // m_alloc), then deallocate using the copy.
        typedef typename allocator_traits<Alloc>
            ::template rebind_alloc<sp_cb_alloc_inplace> alloc_cb_t;

        alloc_cb_t _a(m_alloc);
        this->~sp_cb_alloc_inplace();
        allocator_traits<alloc_cb_t>::deallocate(_a, this, 1);
    }
};


// =============================================================================
// sp_cb_pointer_alloc  -  for shared_ptr(p, d, alloc)
// =============================================================================

// Allocator-aware pointer-with-deleter cb. Like sp_cb_pointer, but the
// cb itself is allocated via Alloc (rebound). dispose() invokes the
// stored deleter on the held pointer; destroy() rebinds and uses
// Alloc to deallocate self. Used by the (p, d, alloc) shared_ptr
// constructor.
template<typename U, typename D, typename Alloc>
class sp_cb_pointer_alloc : public sp_control_block_base
{
    U*     m_ptr;
    D      m_del;
    Alloc  m_alloc;

public:
    sp_cb_pointer_alloc(U* _p, D _d, const Alloc& _a)
        : m_ptr(_p)
        , m_del(re_std::move(_d))
        , m_alloc(_a)
    {
    }

    void dispose() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        m_del(m_ptr);
    }

    void destroy() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        // Same dance as sp_cb_alloc_inplace: take a copy of the
        // allocator before destructing self, then deallocate via the
        // copy.
        typedef typename allocator_traits<Alloc>
            ::template rebind_alloc<sp_cb_pointer_alloc> alloc_cb_t;

        alloc_cb_t _a(m_alloc);
        this->~sp_cb_pointer_alloc();
        allocator_traits<alloc_cb_t>::deallocate(_a, this, 1);
    }

    #if RE_STD_HAS_RTTI
        void* get_deleter(const std::type_info& _ti) RE_STD_NOEXCEPT RE_STD_OVERRIDE
        {
            if (_ti == typeid(D))
            {
                return &m_del;
            }
            return 0;
        }
    #endif
};


// =============================================================================
// sp_cb_inplace_array  -  for make_shared<T[]>(n)
// =============================================================================

// Single-allocation control block for a runtime-sized array. Storage
// layout:
//
//   +------------------+------+----------+----------+ ... +----------+
//   |   cb base+count  | pad? |    T[0]  |    T[1]  |     |  T[n-1]  |
//   +------------------+------+----------+----------+ ... +----------+
//   ^this              ^                 ^
//   |                  |                 +-- aligned for T
//   |                  +-- size is round_up(sizeof(self), alignof(T))
//
// Allocation: ::operator new(total_bytes(n)).
// Deallocation: ::operator delete(this) in destroy().
template<typename U>
class sp_cb_inplace_array : public sp_control_block_base
{
    std::size_t m_count;

public:
    // round_up sizeof(self) to alignof(U).
    static std::size_t offset_to_array() RE_STD_NOEXCEPT
    {
        const std::size_t _s = sizeof(sp_cb_inplace_array);
        const std::size_t _a = alignof(U);
        return (_s + _a - 1) / _a * _a;
    }

    static std::size_t total_bytes(std::size_t _count) RE_STD_NOEXCEPT
    {
        return offset_to_array() + sizeof(U) * _count;
    }

    explicit sp_cb_inplace_array(std::size_t _count) RE_STD_NOEXCEPT
        : m_count(_count)
    {
    }

    U* data() RE_STD_NOEXCEPT
    {
        return reinterpret_cast<U*>(
            reinterpret_cast<unsigned char*>(this) + offset_to_array());
    }

    void dispose() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        // Destroy in reverse order, mirroring delete[].
        U* _arr = data();
        for (std::size_t _i = m_count; _i > 0; --_i)
        {
            _arr[_i - 1].~U();
        }
    }

    void destroy() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        // Destroy self, then free the entire allocation.
        this->~sp_cb_inplace_array();
        ::operator delete(static_cast<void*>(this));
    }
};


// =============================================================================
// sp_cb_alloc_inplace_array  -  for allocate_shared<T[]>(alloc, n)
// =============================================================================

// Like sp_cb_inplace_array, but the entire block is allocated via Alloc
// rebound to unsigned char. m_alloc is held inline and used in destroy()
// to deallocate self.
template<typename U, typename Alloc>
class sp_cb_alloc_inplace_array : public sp_control_block_base
{
    std::size_t m_count;
    Alloc      m_alloc;

public:
    static std::size_t offset_to_array() RE_STD_NOEXCEPT
    {
        const std::size_t _s = sizeof(sp_cb_alloc_inplace_array);
        const std::size_t _a = alignof(U);
        return (_s + _a - 1) / _a * _a;
    }

    static std::size_t total_bytes(std::size_t _count) RE_STD_NOEXCEPT
    {
        return offset_to_array() + sizeof(U) * _count;
    }

    sp_cb_alloc_inplace_array(const Alloc& _a, std::size_t _count) RE_STD_NOEXCEPT
        : m_count(_count)
        , m_alloc(_a)
    {
    }

    U* data() RE_STD_NOEXCEPT
    {
        return reinterpret_cast<U*>(
            reinterpret_cast<unsigned char*>(this) + offset_to_array());
    }

    void dispose() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        U* _arr = data();
        for (std::size_t _i = m_count; _i > 0; --_i)
        {
            _arr[_i - 1].~U();
        }
    }

    void destroy() RE_STD_NOEXCEPT RE_STD_OVERRIDE
    {
        // Take a copy of the allocator BEFORE destructing self, then
        // rebind to unsigned char and deallocate the whole block.
        typedef typename allocator_traits<Alloc>
            ::template rebind_alloc<unsigned char> byte_alloc_t;

        const std::size_t _bytes = total_bytes(m_count);
        byte_alloc_t _ba(m_alloc);
        unsigned char* _self_bytes = reinterpret_cast<unsigned char*>(this);
        this->~sp_cb_alloc_inplace_array();
        allocator_traits<byte_alloc_t>::deallocate(_ba, _self_bytes, _bytes);
    }
};


}  // internal
}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_SP_CONTROL_BLOCK_HPP
