/***********************************************************************
* restd                                              sp_control_block.hpp
*
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
*   sp_cb_pointer<_U, _D>          allocated separately from the object,
*                                  holds the U* and a deleter D.
*   sp_cb_inplace<_U>              single-allocation cb that holds U
*                                  inline. Used by make_shared.
*   sp_cb_alloc_inplace<_U, _A>    single-allocation cb with an
*                                  allocator copy. Used by
*                                  allocate_shared. destroy() rebinds
*                                  and deallocates self via _A.
*
* NOT implemented in this phase (will ship in 4b):
*   sp_cb_pointer_alloc            for shared_ptr(p, d, alloc) ctors.
*
* atomic refcounts:
*   D_RESTD_HAS_SP_ATOMICS         1 when compiler supports __atomic_*
*                                  builtins. GCC, Clang, Intel are
*                                  detected. MSVC support TODO.
*
* When atomics are unavailable, the cb falls back to plain int ops.
* This is single-thread-correct; multi-thread usage of shared_ptr on
* such a configuration is UNSAFE. Document accordingly.
*
*
* path:      /inc/djinterp/re_std/memory/sp_control_block.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_INTERNAL_SP_CONTROL_BLOCK_
#define RESTD_MEMORY_INTERNAL_SP_CONTROL_BLOCK_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>

    #if D_ENV_CPP98_HAS_TYPEINFO
        #include <typeinfo>
    #endif

    #if D_ENV_CPP98_HAS_NEW
        #include <new>
    #endif

    #include "restd/memory/allocator_traits.hpp"
    #include "restd/utility/forward.hpp"
    #include "restd/utility/move.hpp"


// =============================================================================
// D_RESTD_HAS_SP_ATOMICS
// =============================================================================

#ifndef D_RESTD_HAS_SP_ATOMICS
    #if defined(__has_builtin)
        #if __has_builtin(__atomic_fetch_add) && __has_builtin(__atomic_load_n)
            #define D_RESTD_HAS_SP_ATOMICS 1
        #else
            #define D_RESTD_HAS_SP_ATOMICS 0
        #endif
    #elif defined(D_ENV_COMPILER_GCC) || defined(D_ENV_COMPILER_INTEL)
        #define D_RESTD_HAS_SP_ATOMICS 1
    #else
        // MSVC requires _Interlocked* — different surface, deferred.
        #define D_RESTD_HAS_SP_ATOMICS 0
    #endif
#endif


namespace restd
{
namespace internal
{

// =============================================================================
// atomic counter helpers
// =============================================================================

typedef long sp_count_t;

#if D_RESTD_HAS_SP_ATOMICS

    inline sp_count_t sp_atomic_load(const sp_count_t* _p) D_NOEXCEPT
    {
        return __atomic_load_n(_p, __ATOMIC_ACQUIRE);
    }

    // Returns the PREVIOUS value (not the new one).
    inline sp_count_t sp_atomic_inc(sp_count_t* _p) D_NOEXCEPT
    {
        return __atomic_fetch_add(_p, 1, __ATOMIC_ACQ_REL);
    }

    // Returns the PREVIOUS value (not the new one).
    inline sp_count_t sp_atomic_dec(sp_count_t* _p) D_NOEXCEPT
    {
        return __atomic_fetch_sub(_p, 1, __ATOMIC_ACQ_REL);
    }

    // CAS-loop incrementer. Returns true if successfully incremented
    // (i.e. counter was nonzero), false if counter was 0.
    inline bool sp_atomic_inc_if_nonzero(sp_count_t* _p) D_NOEXCEPT
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

#else  // !D_RESTD_HAS_SP_ATOMICS

    // Fallback: plain int ops. Single-thread only.

    inline sp_count_t sp_atomic_load(const sp_count_t* _p) D_NOEXCEPT
    {
        return *_p;
    }

    inline sp_count_t sp_atomic_inc(sp_count_t* _p) D_NOEXCEPT
    {
        sp_count_t _old = *_p;
        ++*_p;
        return _old;
    }

    inline sp_count_t sp_atomic_dec(sp_count_t* _p) D_NOEXCEPT
    {
        sp_count_t _old = *_p;
        --*_p;
        return _old;
    }

    inline bool sp_atomic_inc_if_nonzero(sp_count_t* _p) D_NOEXCEPT
    {
        if (*_p == 0)
        {
            return false;
        }
        ++*_p;
        return true;
    }

#endif  // D_RESTD_HAS_SP_ATOMICS


// =============================================================================
// sp_control_block_base
// =============================================================================

class sp_control_block_base
{
public:
    sp_count_t m_use_count;
    sp_count_t m_weak_count;

    sp_control_block_base() D_NOEXCEPT
        : m_use_count(1)
        , m_weak_count(1)
    {
    }

    virtual ~sp_control_block_base() D_NOEXCEPT
    {
    }

    sp_control_block_base(const sp_control_block_base&)            D_DELETE_FN;
    sp_control_block_base& operator=(const sp_control_block_base&) D_DELETE_FN;

    // Polymorphic destruction strategies.
    virtual void dispose() D_NOEXCEPT = 0;   // destroy managed object
    virtual void destroy() D_NOEXCEPT = 0;   // destroy this cb

    #if D_ENV_CPP98_HAS_TYPEINFO
        virtual void* get_deleter(const std::type_info&) D_NOEXCEPT
        {
            return 0;
        }
    #endif

    // ---- ref-count operations ----

    void add_ref() D_NOEXCEPT
    {
        sp_atomic_inc(&m_use_count);
    }

    bool add_ref_if_nonzero() D_NOEXCEPT
    {
        return sp_atomic_inc_if_nonzero(&m_use_count);
    }

    void release() D_NOEXCEPT
    {
        if (sp_atomic_dec(&m_use_count) == 1)
        {
            // strong count went 1 -> 0
            dispose();
            weak_release();
        }
    }

    void weak_add_ref() D_NOEXCEPT
    {
        sp_atomic_inc(&m_weak_count);
    }

    void weak_release() D_NOEXCEPT
    {
        if (sp_atomic_dec(&m_weak_count) == 1)
        {
            destroy();
        }
    }

    sp_count_t use_count() const D_NOEXCEPT
    {
        return sp_atomic_load(&m_use_count);
    }
};


// =============================================================================
// sp_cb_pointer  -  for shared_ptr(ptr) and shared_ptr(ptr, deleter)
// =============================================================================

// Allocated separately from the managed object. The cb holds a pointer
// to the object (so dispose can reach it) and the deleter.
template<typename _U, typename _D>
class sp_cb_pointer : public sp_control_block_base
{
    _U* m_ptr;
    _D  m_del;

public:
    sp_cb_pointer(_U* _p, _D _d)
        : m_ptr(_p)
        , m_del(restd::move(_d))
    {
    }

    void dispose() D_NOEXCEPT D_OVERRIDE
    {
        m_del(m_ptr);
    }

    void destroy() D_NOEXCEPT D_OVERRIDE
    {
        delete this;
    }

    #if D_ENV_CPP98_HAS_TYPEINFO
        void* get_deleter(const std::type_info& _ti) D_NOEXCEPT D_OVERRIDE
        {
            if (_ti == typeid(_D))
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
template<typename _U>
class sp_cb_inplace : public sp_control_block_base
{
    alignas(_U) char m_storage[sizeof(_U)];

    _U* obj_ptr() D_NOEXCEPT
    {
        return reinterpret_cast<_U*>(&m_storage[0]);
    }

public:
    template<typename... _Args>
    explicit sp_cb_inplace(_Args&&... _args)
    {
        ::new (static_cast<void*>(&m_storage[0]))
            _U(restd::forward<_Args>(_args)...);
    }

    // For make_shared_for_overwrite: default-initialise (no parens).
    explicit sp_cb_inplace(sp_for_overwrite_t)
    {
        ::new (static_cast<void*>(&m_storage[0])) _U;
    }

    _U* get() D_NOEXCEPT
    {
        return obj_ptr();
    }

    void dispose() D_NOEXCEPT D_OVERRIDE
    {
        obj_ptr()->~_U();
    }

    void destroy() D_NOEXCEPT D_OVERRIDE
    {
        delete this;
    }
};


// =============================================================================
// sp_cb_alloc_inplace  -  for allocate_shared
// =============================================================================

// Holds the object inline AND a copy of the allocator. The allocator
// is used to deallocate the cb itself (after destructing this).
template<typename _U, typename _Alloc>
class sp_cb_alloc_inplace : public sp_control_block_base
{
    alignas(_U) char m_storage[sizeof(_U)];
    _Alloc m_alloc;

    _U* obj_ptr() D_NOEXCEPT
    {
        return reinterpret_cast<_U*>(&m_storage[0]);
    }

public:
    template<typename... _Args>
    sp_cb_alloc_inplace(const _Alloc& _a, _Args&&... _args)
        : m_alloc(_a)
    {
        ::new (static_cast<void*>(&m_storage[0]))
            _U(restd::forward<_Args>(_args)...);
    }

    // For allocate_shared_for_overwrite: default-initialise (no parens).
    sp_cb_alloc_inplace(const _Alloc& _a, sp_for_overwrite_t)
        : m_alloc(_a)
    {
        ::new (static_cast<void*>(&m_storage[0])) _U;
    }

    _U* get() D_NOEXCEPT
    {
        return obj_ptr();
    }

    void dispose() D_NOEXCEPT D_OVERRIDE
    {
        obj_ptr()->~_U();
    }

    void destroy() D_NOEXCEPT D_OVERRIDE
    {
        // The cb was allocated via a rebound _Alloc. To deallocate it,
        // we need a fresh rebind. Critical ordering: take a COPY of
        // the allocator BEFORE destructing self (which destructs
        // m_alloc), then deallocate using the copy.
        typedef typename allocator_traits<_Alloc>
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
// cb itself is allocated via _Alloc (rebound). dispose() invokes the
// stored deleter on the held pointer; destroy() rebinds and uses
// _Alloc to deallocate self. Used by the (p, d, alloc) shared_ptr
// constructor.
template<typename _U, typename _D, typename _Alloc>
class sp_cb_pointer_alloc : public sp_control_block_base
{
    _U*     m_ptr;
    _D      m_del;
    _Alloc  m_alloc;

public:
    sp_cb_pointer_alloc(_U* _p, _D _d, const _Alloc& _a)
        : m_ptr(_p)
        , m_del(restd::move(_d))
        , m_alloc(_a)
    {
    }

    void dispose() D_NOEXCEPT D_OVERRIDE
    {
        m_del(m_ptr);
    }

    void destroy() D_NOEXCEPT D_OVERRIDE
    {
        // Same dance as sp_cb_alloc_inplace: take a copy of the
        // allocator before destructing self, then deallocate via the
        // copy.
        typedef typename allocator_traits<_Alloc>
            ::template rebind_alloc<sp_cb_pointer_alloc> alloc_cb_t;

        alloc_cb_t _a(m_alloc);
        this->~sp_cb_pointer_alloc();
        allocator_traits<alloc_cb_t>::deallocate(_a, this, 1);
    }

    #if D_ENV_CPP98_HAS_TYPEINFO
        void* get_deleter(const std::type_info& _ti) D_NOEXCEPT D_OVERRIDE
        {
            if (_ti == typeid(_D))
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
template<typename _U>
class sp_cb_inplace_array : public sp_control_block_base
{
    std::size_t m_count;

public:
    // round_up sizeof(self) to alignof(_U).
    static std::size_t offset_to_array() D_NOEXCEPT
    {
        const std::size_t _s = sizeof(sp_cb_inplace_array);
        const std::size_t _a = alignof(_U);
        return (_s + _a - 1) / _a * _a;
    }

    static std::size_t total_bytes(std::size_t _count) D_NOEXCEPT
    {
        return offset_to_array() + sizeof(_U) * _count;
    }

    explicit sp_cb_inplace_array(std::size_t _count) D_NOEXCEPT
        : m_count(_count)
    {
    }

    _U* data() D_NOEXCEPT
    {
        return reinterpret_cast<_U*>(
            reinterpret_cast<unsigned char*>(this) + offset_to_array());
    }

    void dispose() D_NOEXCEPT D_OVERRIDE
    {
        // Destroy in reverse order, mirroring delete[].
        _U* _arr = data();
        for (std::size_t _i = m_count; _i > 0; --_i)
        {
            _arr[_i - 1].~_U();
        }
    }

    void destroy() D_NOEXCEPT D_OVERRIDE
    {
        // Destroy self, then free the entire allocation.
        this->~sp_cb_inplace_array();
        ::operator delete(static_cast<void*>(this));
    }
};


// =============================================================================
// sp_cb_alloc_inplace_array  -  for allocate_shared<T[]>(alloc, n)
// =============================================================================

// Like sp_cb_inplace_array, but the entire block is allocated via _Alloc
// rebound to unsigned char. m_alloc is held inline and used in destroy()
// to deallocate self.
template<typename _U, typename _Alloc>
class sp_cb_alloc_inplace_array : public sp_control_block_base
{
    std::size_t m_count;
    _Alloc      m_alloc;

public:
    static std::size_t offset_to_array() D_NOEXCEPT
    {
        const std::size_t _s = sizeof(sp_cb_alloc_inplace_array);
        const std::size_t _a = alignof(_U);
        return (_s + _a - 1) / _a * _a;
    }

    static std::size_t total_bytes(std::size_t _count) D_NOEXCEPT
    {
        return offset_to_array() + sizeof(_U) * _count;
    }

    sp_cb_alloc_inplace_array(const _Alloc& _a, std::size_t _count) D_NOEXCEPT
        : m_count(_count)
        , m_alloc(_a)
    {
    }

    _U* data() D_NOEXCEPT
    {
        return reinterpret_cast<_U*>(
            reinterpret_cast<unsigned char*>(this) + offset_to_array());
    }

    void dispose() D_NOEXCEPT D_OVERRIDE
    {
        _U* _arr = data();
        for (std::size_t _i = m_count; _i > 0; --_i)
        {
            _arr[_i - 1].~_U();
        }
    }

    void destroy() D_NOEXCEPT D_OVERRIDE
    {
        // Take a copy of the allocator BEFORE destructing self, then
        // rebind to unsigned char and deallocate the whole block.
        typedef typename allocator_traits<_Alloc>
            ::template rebind_alloc<unsigned char> byte_alloc_t;

        const std::size_t _bytes = total_bytes(m_count);
        byte_alloc_t _ba(m_alloc);
        unsigned char* _self_bytes = reinterpret_cast<unsigned char*>(this);
        this->~sp_cb_alloc_inplace_array();
        allocator_traits<byte_alloc_t>::deallocate(_ba, _self_bytes, _bytes);
    }
};


}  // namespace internal
}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_INTERNAL_SP_CONTROL_BLOCK_
