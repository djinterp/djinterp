#include "..\..\inc\c\datomic.h"


///////////////////////////////////////////////////////////////////////////////
///             BACKEND-SPECIFIC HELPER MACROS                              ///
///////////////////////////////////////////////////////////////////////////////
//
// Each helper performs a single atomic primitive for the active backend.
// The stamp macros in the next section call these helpers to generate
// complete function definitions for every type in D_ATOMIC_INTEGER_TYPES.
//
// Naming: D_INTERNAL_ATOMIC_DO_<OP>(obj, arg..., vtype)
//
// For Windows, sizeof(vtype) selects the 32-bit or 64-bit Interlocked
// intrinsic. Both branches are syntactically valid; the compiler
// eliminates the dead branch at compile time.

#if D_ATOMIC_USE_STDATOMIC

    #define D_INTERNAL_ATOMIC_DO_INIT(obj, val, vtype)                 \
        atomic_init((obj), (val))

    #define D_INTERNAL_ATOMIC_DO_LOAD(obj, order, vtype)               \
        atomic_load_explicit((obj), (order))

    #define D_INTERNAL_ATOMIC_DO_STORE(obj, val, order, vtype)         \
        atomic_store_explicit((obj), (val), (order))

    #define D_INTERNAL_ATOMIC_DO_EXCHANGE(obj, val, order, vtype)      \
        atomic_exchange_explicit((obj), (val), (order))

    #define D_INTERNAL_ATOMIC_DO_FETCH_ADD(obj, arg, order, vtype)     \
        atomic_fetch_add_explicit((obj), (arg), (order))

    #define D_INTERNAL_ATOMIC_DO_FETCH_SUB(obj, arg, order, vtype)     \
        atomic_fetch_sub_explicit((obj), (arg), (order))

    #define D_INTERNAL_ATOMIC_DO_FETCH_OR(obj, arg, order, vtype)      \
        atomic_fetch_or_explicit((obj), (arg), (order))

    #define D_INTERNAL_ATOMIC_DO_FETCH_XOR(obj, arg, order, vtype)     \
        atomic_fetch_xor_explicit((obj), (arg), (order))

    #define D_INTERNAL_ATOMIC_DO_FETCH_AND(obj, arg, order, vtype)     \
        atomic_fetch_and_explicit((obj), (arg), (order))

#elif D_ATOMIC_USE_WINDOWS

    #define D_INTERNAL_ATOMIC_DO_INIT(obj, val, vtype)                 \
        (*(obj) = (val))

    #define D_INTERNAL_ATOMIC_DO_LOAD(obj, order, vtype)               \
        ( (sizeof(vtype) <= 4)                                         \
          ? (vtype)InterlockedCompareExchange(                         \
                (long volatile*)(obj), 0, 0)                           \
          : (vtype)InterlockedCompareExchange64(                       \
                (long long volatile*)(obj), 0, 0) )

    #define D_INTERNAL_ATOMIC_DO_STORE(obj, val, order, vtype)         \
        do {                                                           \
            if (sizeof(vtype) <= 4)                                    \
            {                                                          \
                InterlockedExchange(                                    \
                    (long volatile*)(obj), (long)(val));                \
            }                                                          \
            else                                                       \
            {                                                          \
                InterlockedExchange64(                                  \
                    (long long volatile*)(obj),                         \
                    (long long)(val));                                  \
            }                                                          \
        } while (0)

    #define D_INTERNAL_ATOMIC_DO_EXCHANGE(obj, val, order, vtype)      \
        ( (sizeof(vtype) <= 4)                                         \
          ? (vtype)InterlockedExchange(                                \
                (long volatile*)(obj), (long)(val))                    \
          : (vtype)InterlockedExchange64(                              \
                (long long volatile*)(obj),                            \
                (long long)(val)) )

    #define D_INTERNAL_ATOMIC_DO_FETCH_ADD(obj, arg, order, vtype)     \
        ( (sizeof(vtype) <= 4)                                         \
          ? (vtype)InterlockedExchangeAdd(                             \
                (long volatile*)(obj), (long)(arg))                    \
          : (vtype)InterlockedExchangeAdd64(                           \
                (long long volatile*)(obj),                            \
                (long long)(arg)) )

    #define D_INTERNAL_ATOMIC_DO_FETCH_SUB(obj, arg, order, vtype)     \
        ( (sizeof(vtype) <= 4)                                         \
          ? (vtype)InterlockedExchangeAdd(                             \
                (long volatile*)(obj), -(long)(arg))                   \
          : (vtype)InterlockedExchangeAdd64(                           \
                (long long volatile*)(obj),                            \
                -(long long)(arg)) )

    #define D_INTERNAL_ATOMIC_DO_FETCH_OR(obj, arg, order, vtype)      \
        ( (sizeof(vtype) <= 4)                                         \
          ? (vtype)InterlockedOr(                                      \
                (long volatile*)(obj), (long)(arg))                    \
          : (vtype)InterlockedOr64(                                    \
                (long long volatile*)(obj),                            \
                (long long)(arg)) )

    #define D_INTERNAL_ATOMIC_DO_FETCH_XOR(obj, arg, order, vtype)     \
        ( (sizeof(vtype) <= 4)                                         \
          ? (vtype)InterlockedXor(                                     \
                (long volatile*)(obj), (long)(arg))                    \
          : (vtype)InterlockedXor64(                                   \
                (long long volatile*)(obj),                            \
                (long long)(arg)) )

    #define D_INTERNAL_ATOMIC_DO_FETCH_AND(obj, arg, order, vtype)     \
        ( (sizeof(vtype) <= 4)                                         \
          ? (vtype)InterlockedAnd(                                     \
                (long volatile*)(obj), (long)(arg))                    \
          : (vtype)InterlockedAnd64(                                   \
                (long long volatile*)(obj),                            \
                (long long)(arg)) )

#else  /* D_ATOMIC_USE_SYNC */

    #define D_INTERNAL_ATOMIC_DO_INIT(obj, val, vtype)                 \
        (*(obj) = (val))

    #define D_INTERNAL_ATOMIC_DO_LOAD(obj, order, vtype)               \
        __sync_val_compare_and_swap((vtype*)(obj), 0, 0)

    #define D_INTERNAL_ATOMIC_DO_STORE(obj, val, order, vtype)         \
        do {                                                           \
            (void)__sync_lock_test_and_set((obj), (val));              \
            __sync_synchronize();                                      \
        } while (0)

    #define D_INTERNAL_ATOMIC_DO_EXCHANGE(obj, val, order, vtype)      \
        (vtype)__sync_lock_test_and_set((obj), (val))

    #define D_INTERNAL_ATOMIC_DO_FETCH_ADD(obj, arg, order, vtype)     \
        __sync_fetch_and_add((obj), (arg))

    #define D_INTERNAL_ATOMIC_DO_FETCH_SUB(obj, arg, order, vtype)     \
        __sync_fetch_and_sub((obj), (arg))

    #define D_INTERNAL_ATOMIC_DO_FETCH_OR(obj, arg, order, vtype)      \
        __sync_fetch_and_or((obj), (arg))

    #define D_INTERNAL_ATOMIC_DO_FETCH_XOR(obj, arg, order, vtype)     \
        __sync_fetch_and_xor((obj), (arg))

    #define D_INTERNAL_ATOMIC_DO_FETCH_AND(obj, arg, order, vtype)     \
        __sync_fetch_and_and((obj), (arg))

#endif  /* backend selection */


///////////////////////////////////////////////////////////////////////////////
///             GENERIC STAMP MACROS                                        ///
///////////////////////////////////////////////////////////////////////////////
//
// Each stamp produces TWO function definitions (explicit + non-explicit)
// for one (suffix, value_type, atomic_type, zero) tuple. Expanding via
// D_ATOMIC_INTEGER_TYPES generates all seven type variants at once.

// I.    init
#define D_INTERNAL_ATOMIC_IMPL_INIT(sfx, vtype, atype, zero)          \
    void                                                               \
    d_atomic_init_##sfx                                                \
    (                                                                  \
        atype* _obj,                                                   \
        vtype  _value                                                  \
    )                                                                  \
    {                                                                  \
        if (_obj)                                                      \
        {                                                              \
            D_INTERNAL_ATOMIC_DO_INIT(_obj, _value, vtype);            \
        }                                                              \
                                                                       \
        return;                                                        \
    }

// II.   load
#define D_INTERNAL_ATOMIC_IMPL_LOAD(sfx, vtype, atype, zero)          \
    vtype                                                              \
    d_atomic_load_##sfx##_explicit                                     \
    (                                                                  \
        const atype*   _obj,                                           \
        d_memory_order _order                                          \
    )                                                                  \
    {                                                                  \
        if (!_obj)                                                     \
        {                                                              \
            return zero;                                               \
        }                                                              \
                                                                       \
        return (vtype)D_INTERNAL_ATOMIC_DO_LOAD(                       \
                          _obj, _order, vtype);                        \
    }                                                                  \
                                                                       \
    vtype                                                              \
    d_atomic_load_##sfx                                                \
    (                                                                  \
        const atype* _obj                                              \
    )                                                                  \
    {                                                                  \
        return d_atomic_load_##sfx##_explicit(                         \
                   _obj,                                               \
                   D_MEMORY_ORDER_SEQ_CST);                            \
    }

// III.  store
#define D_INTERNAL_ATOMIC_IMPL_STORE(sfx, vtype, atype, zero)         \
    void                                                               \
    d_atomic_store_##sfx##_explicit                                    \
    (                                                                  \
        atype*         _obj,                                           \
        vtype          _value,                                         \
        d_memory_order _order                                          \
    )                                                                  \
    {                                                                  \
        if (_obj)                                                      \
        {                                                              \
            D_INTERNAL_ATOMIC_DO_STORE(                                \
                _obj, _value, _order, vtype);                          \
        }                                                              \
                                                                       \
        return;                                                        \
    }                                                                  \
                                                                       \
    void                                                               \
    d_atomic_store_##sfx                                               \
    (                                                                  \
        atype* _obj,                                                   \
        vtype  _value                                                  \
    )                                                                  \
    {                                                                  \
        d_atomic_store_##sfx##_explicit(                               \
            _obj,                                                      \
            _value,                                                    \
            D_MEMORY_ORDER_SEQ_CST);                                   \
                                                                       \
        return;                                                        \
    }

// IV.   exchange
#define D_INTERNAL_ATOMIC_IMPL_EXCHANGE(sfx, vtype, atype, zero)      \
    vtype                                                              \
    d_atomic_exchange_##sfx##_explicit                                 \
    (                                                                  \
        atype*         _obj,                                           \
        vtype          _value,                                         \
        d_memory_order _order                                          \
    )                                                                  \
    {                                                                  \
        if (!_obj)                                                     \
        {                                                              \
            return zero;                                               \
        }                                                              \
                                                                       \
        return (vtype)D_INTERNAL_ATOMIC_DO_EXCHANGE(                   \
                          _obj, _value, _order, vtype);                \
    }                                                                  \
                                                                       \
    vtype                                                              \
    d_atomic_exchange_##sfx                                            \
    (                                                                  \
        atype* _obj,                                                   \
        vtype  _value                                                  \
    )                                                                  \
    {                                                                  \
        return d_atomic_exchange_##sfx##_explicit(                     \
                   _obj,                                               \
                   _value,                                             \
                   D_MEMORY_ORDER_SEQ_CST);                            \
    }

// V.    fetch operations (one stamp per operation)
#define D_INTERNAL_ATOMIC_IMPL_FETCH(do_op, opname,                    \
                                      sfx, vtype, atype, zero)        \
    vtype                                                              \
    d_atomic_fetch_##opname##_##sfx##_explicit                         \
    (                                                                  \
        atype*         _obj,                                           \
        vtype          _arg,                                           \
        d_memory_order _order                                          \
    )                                                                  \
    {                                                                  \
        if (!_obj)                                                     \
        {                                                              \
            return zero;                                               \
        }                                                              \
                                                                       \
        return (vtype)do_op(_obj, _arg, _order, vtype);                \
    }                                                                  \
                                                                       \
    vtype                                                              \
    d_atomic_fetch_##opname##_##sfx                                    \
    (                                                                  \
        atype* _obj,                                                   \
        vtype  _arg                                                    \
    )                                                                  \
    {                                                                  \
        return d_atomic_fetch_##opname##_##sfx##_explicit(             \
                   _obj,                                               \
                   _arg,                                               \
                   D_MEMORY_ORDER_SEQ_CST);                            \
    }

// adapter stamps that bind D_INTERNAL_ATOMIC_IMPL_FETCH to each
// backend helper — these are what D_ATOMIC_INTEGER_TYPES expands.
#define D_INTERNAL_ATOMIC_IMPL_FETCH_ADD(sfx, vtype, atype, zero)     \
    D_INTERNAL_ATOMIC_IMPL_FETCH(                                      \
        D_INTERNAL_ATOMIC_DO_FETCH_ADD, add,                           \
        sfx, vtype, atype, zero)

#define D_INTERNAL_ATOMIC_IMPL_FETCH_SUB(sfx, vtype, atype, zero)     \
    D_INTERNAL_ATOMIC_IMPL_FETCH(                                      \
        D_INTERNAL_ATOMIC_DO_FETCH_SUB, sub,                           \
        sfx, vtype, atype, zero)

#define D_INTERNAL_ATOMIC_IMPL_FETCH_OR(sfx, vtype, atype, zero)      \
    D_INTERNAL_ATOMIC_IMPL_FETCH(                                      \
        D_INTERNAL_ATOMIC_DO_FETCH_OR, or,                             \
        sfx, vtype, atype, zero)

#define D_INTERNAL_ATOMIC_IMPL_FETCH_XOR(sfx, vtype, atype, zero)     \
    D_INTERNAL_ATOMIC_IMPL_FETCH(                                      \
        D_INTERNAL_ATOMIC_DO_FETCH_XOR, xor,                           \
        sfx, vtype, atype, zero)

#define D_INTERNAL_ATOMIC_IMPL_FETCH_AND(sfx, vtype, atype, zero)     \
    D_INTERNAL_ATOMIC_IMPL_FETCH(                                      \
        D_INTERNAL_ATOMIC_DO_FETCH_AND, and,                           \
        sfx, vtype, atype, zero)


///////////////////////////////////////////////////////////////////////////////
///             COMPARE-AND-EXCHANGE STAMP MACROS                          ///
///////////////////////////////////////////////////////////////////////////////
//
// CAS requires a local variable and conditional *_expected update,
// so it cannot use a simple expression helper. The stamp is defined
// per backend inside #if blocks.

#if D_ATOMIC_USE_STDATOMIC

#define D_INTERNAL_ATOMIC_IMPL_CAS(sfx, vtype, atype, zero)           \
    bool                                                               \
    d_atomic_compare_exchange_strong_##sfx##_explicit                   \
    (                                                                  \
        atype*         _obj,                                           \
        vtype*         _expected,                                      \
        vtype          _desired,                                       \
        d_memory_order _success,                                       \
        d_memory_order _failure                                        \
    )                                                                  \
    {                                                                  \
        if ( (!_obj) || (!_expected) )                                 \
        {                                                              \
            return false;                                              \
        }                                                              \
                                                                       \
        return atomic_compare_exchange_strong_explicit(                 \
                   _obj, _expected, _desired,                          \
                   _success, _failure);                                \
    }                                                                  \
                                                                       \
    bool                                                               \
    d_atomic_compare_exchange_weak_##sfx##_explicit                     \
    (                                                                  \
        atype*         _obj,                                           \
        vtype*         _expected,                                      \
        vtype          _desired,                                       \
        d_memory_order _success,                                       \
        d_memory_order _failure                                        \
    )                                                                  \
    {                                                                  \
        if ( (!_obj) || (!_expected) )                                 \
        {                                                              \
            return false;                                              \
        }                                                              \
                                                                       \
        return atomic_compare_exchange_weak_explicit(                   \
                   _obj, _expected, _desired,                          \
                   _success, _failure);                                \
    }                                                                  \
                                                                       \
    bool                                                               \
    d_atomic_compare_exchange_strong_##sfx                              \
    (                                                                  \
        atype* _obj,                                                   \
        vtype* _expected,                                              \
        vtype  _desired                                                \
    )                                                                  \
    {                                                                  \
        return d_atomic_compare_exchange_strong_##sfx##_explicit(       \
                   _obj, _expected, _desired,                          \
                   D_MEMORY_ORDER_SEQ_CST,                             \
                   D_MEMORY_ORDER_SEQ_CST);                            \
    }                                                                  \
                                                                       \
    bool                                                               \
    d_atomic_compare_exchange_weak_##sfx                                \
    (                                                                  \
        atype* _obj,                                                   \
        vtype* _expected,                                              \
        vtype  _desired                                                \
    )                                                                  \
    {                                                                  \
        return d_atomic_compare_exchange_weak_##sfx##_explicit(         \
                   _obj, _expected, _desired,                          \
                   D_MEMORY_ORDER_SEQ_CST,                             \
                   D_MEMORY_ORDER_SEQ_CST);                            \
    }

#else  /* D_ATOMIC_USE_WINDOWS or D_ATOMIC_USE_SYNC — shared CAS */
       /* weak == strong on these backends (no spurious failure)   */

// D_INTERNAL_ATOMIC_DO_CAS
//   helper: performs a single CAS, writes old value into *old_out.
// Returns true on match, false otherwise. Defined per backend.
#if D_ATOMIC_USE_WINDOWS
    #define D_INTERNAL_ATOMIC_DO_CAS(obj, expected_ptr, desired,       \
                                      old_out, vtype)                  \
        do {                                                           \
            if (sizeof(vtype) <= 4)                                    \
            {                                                          \
                *(old_out) = (vtype)InterlockedCompareExchange(        \
                                 (long volatile*)(obj),                \
                                 (long)(desired),                      \
                                 (long)*(expected_ptr));               \
            }                                                          \
            else                                                       \
            {                                                          \
                *(old_out) = (vtype)InterlockedCompareExchange64(      \
                                 (long long volatile*)(obj),           \
                                 (long long)(desired),                 \
                                 (long long)*(expected_ptr));          \
            }                                                          \
        } while (0)
#else
    #define D_INTERNAL_ATOMIC_DO_CAS(obj, expected_ptr, desired,       \
                                      old_out, vtype)                  \
        do {                                                           \
            *(old_out) = (vtype)__sync_val_compare_and_swap(           \
                             (obj), *(expected_ptr), (desired));        \
        } while (0)
#endif

#define D_INTERNAL_ATOMIC_IMPL_CAS(sfx, vtype, atype, zero)           \
    bool                                                               \
    d_atomic_compare_exchange_strong_##sfx##_explicit                   \
    (                                                                  \
        atype*         _obj,                                           \
        vtype*         _expected,                                      \
        vtype          _desired,                                       \
        d_memory_order _success,                                       \
        d_memory_order _failure                                        \
    )                                                                  \
    {                                                                  \
        vtype old_val;                                                 \
                                                                       \
        (void)_success;                                                \
        (void)_failure;                                                \
                                                                       \
        if ( (!_obj) || (!_expected) )                                 \
        {                                                              \
            return false;                                              \
        }                                                              \
                                                                       \
        D_INTERNAL_ATOMIC_DO_CAS(_obj, _expected, _desired,            \
                                  &old_val, vtype);                    \
                                                                       \
        if (old_val == *_expected)                                     \
        {                                                              \
            return true;                                               \
        }                                                              \
                                                                       \
        *_expected = old_val;                                          \
                                                                       \
        return false;                                                  \
    }                                                                  \
                                                                       \
    bool                                                               \
    d_atomic_compare_exchange_weak_##sfx##_explicit                     \
    (                                                                  \
        atype*         _obj,                                           \
        vtype*         _expected,                                      \
        vtype          _desired,                                       \
        d_memory_order _success,                                       \
        d_memory_order _failure                                        \
    )                                                                  \
    {                                                                  \
        return d_atomic_compare_exchange_strong_##sfx##_explicit(       \
                   _obj, _expected, _desired,                          \
                   _success, _failure);                                \
    }                                                                  \
                                                                       \
    bool                                                               \
    d_atomic_compare_exchange_strong_##sfx                              \
    (                                                                  \
        atype* _obj,                                                   \
        vtype* _expected,                                              \
        vtype  _desired                                                \
    )                                                                  \
    {                                                                  \
        return d_atomic_compare_exchange_strong_##sfx##_explicit(       \
                   _obj, _expected, _desired,                          \
                   D_MEMORY_ORDER_SEQ_CST,                             \
                   D_MEMORY_ORDER_SEQ_CST);                            \
    }                                                                  \
                                                                       \
    bool                                                               \
    d_atomic_compare_exchange_weak_##sfx                                \
    (                                                                  \
        atype* _obj,                                                   \
        vtype* _expected,                                              \
        vtype  _desired                                                \
    )                                                                  \
    {                                                                  \
        return d_atomic_compare_exchange_strong_##sfx##_explicit(       \
                   _obj, _expected, _desired,                          \
                   D_MEMORY_ORDER_SEQ_CST,                             \
                   D_MEMORY_ORDER_SEQ_CST);                            \
    }

#endif  /* CAS backend selection */


///////////////////////////////////////////////////////////////////////////////
///             STAMP EXPANSIONS — INTEGER TYPES                            ///
///////////////////////////////////////////////////////////////////////////////

D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_INIT)
D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_LOAD)
D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_STORE)
D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_EXCHANGE)
D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_CAS)
D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_FETCH_ADD)
D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_FETCH_SUB)
D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_FETCH_OR)
D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_FETCH_XOR)
D_ATOMIC_INTEGER_TYPES(D_INTERNAL_ATOMIC_IMPL_FETCH_AND)


///////////////////////////////////////////////////////////////////////////////
///             POINTER ATOMIC OPERATIONS                                   ///
///////////////////////////////////////////////////////////////////////////////

/*
d_atomic_init_ptr
  Initializes an atomic pointer (non-atomic — call before sharing).

Parameter(s):
  _obj:   the atomic pointer to initialize.
  _value: the initial value.
Return:
  none.
*/
void
d_atomic_init_ptr
(
    d_atomic_ptr* _obj,
    void*         _value
)
{
    if (_obj)
    {
#if D_ATOMIC_USE_STDATOMIC
        atomic_init(_obj, _value);
#else
        *_obj = _value;
#endif
    }

    return;
}

/*
d_atomic_load_ptr_explicit
  Atomically loads a pointer value with the specified memory order.

Parameter(s):
  _obj:   the atomic pointer to read.
  _order: the memory ordering constraint.
Return:
  the current value, or NULL if _obj is NULL.
*/
void*
d_atomic_load_ptr_explicit
(
    const d_atomic_ptr* _obj,
    d_memory_order      _order
)
{
    if (!_obj)
    {
        return NULL;
    }

#if D_ATOMIC_USE_STDATOMIC
    return atomic_load_explicit((d_atomic_ptr*)_obj, _order);
#elif D_ATOMIC_USE_WINDOWS
    #if (defined(_WIN64))
        return (void*)InterlockedCompareExchange64(
                   (long long volatile*)_obj, 0, 0);
    #else
        return (void*)(size_t)InterlockedCompareExchange(
                   (long volatile*)_obj, 0, 0);
    #endif
#else
    return (void*)__sync_val_compare_and_swap(
               (void* volatile*)_obj, NULL, NULL);
#endif
}

/*
d_atomic_load_ptr
  Atomically loads a pointer value with sequential consistency.

Parameter(s):
  _obj: the atomic pointer to read.
Return:
  the current value, or NULL if _obj is NULL.
*/
void*
d_atomic_load_ptr
(
    const d_atomic_ptr* _obj
)
{
    return d_atomic_load_ptr_explicit(_obj,
                                      D_MEMORY_ORDER_SEQ_CST);
}

/*
d_atomic_store_ptr_explicit
  Atomically stores a pointer value with the specified memory order.

Parameter(s):
  _obj:   the atomic pointer to write.
  _value: the value to store.
  _order: the memory ordering constraint.
Return:
  none.
*/
void
d_atomic_store_ptr_explicit
(
    d_atomic_ptr*  _obj,
    void*          _value,
    d_memory_order _order
)
{
    if (!_obj)
    {
        return;
    }

#if D_ATOMIC_USE_STDATOMIC
    atomic_store_explicit(_obj, _value, _order);
#elif D_ATOMIC_USE_WINDOWS
    #if (defined(_WIN64))
        InterlockedExchange64(
            (long long volatile*)_obj,
            (long long)(size_t)_value);
    #else
        InterlockedExchange(
            (long volatile*)_obj,
            (long)(size_t)_value);
    #endif
#else
    (void)__sync_lock_test_and_set(
        (void* volatile*)_obj, _value);
    __sync_synchronize();
#endif

    return;
}

/*
d_atomic_store_ptr
  Atomically stores a pointer with sequential consistency.

Parameter(s):
  _obj:   the atomic pointer to write.
  _value: the value to store.
Return:
  none.
*/
void
d_atomic_store_ptr
(
    d_atomic_ptr* _obj,
    void*         _value
)
{
    d_atomic_store_ptr_explicit(_obj,
                                _value,
                                D_MEMORY_ORDER_SEQ_CST);

    return;
}

/*
d_atomic_exchange_ptr_explicit
  Atomically exchanges a pointer value.

Parameter(s):
  _obj:   the atomic pointer to modify.
  _value: the new value.
  _order: the memory ordering constraint.
Return:
  the previous value, or NULL if _obj is NULL.
*/
void*
d_atomic_exchange_ptr_explicit
(
    d_atomic_ptr*  _obj,
    void*          _value,
    d_memory_order _order
)
{
    if (!_obj)
    {
        return NULL;
    }

#if D_ATOMIC_USE_STDATOMIC
    return atomic_exchange_explicit(_obj, _value, _order);
#elif D_ATOMIC_USE_WINDOWS
    #if (defined(_WIN64))
        return (void*)InterlockedExchange64(
                   (long long volatile*)_obj,
                   (long long)(size_t)_value);
    #else
        return (void*)(size_t)InterlockedExchange(
                   (long volatile*)_obj,
                   (long)(size_t)_value);
    #endif
#else
    return (void*)__sync_lock_test_and_set(
               (void* volatile*)_obj, _value);
#endif
}

/*
d_atomic_exchange_ptr
  Atomically exchanges a pointer with sequential consistency.

Parameter(s):
  _obj:   the atomic pointer to modify.
  _value: the new value.
Return:
  the previous value, or NULL if _obj is NULL.
*/
void*
d_atomic_exchange_ptr
(
    d_atomic_ptr* _obj,
    void*         _value
)
{
    return d_atomic_exchange_ptr_explicit(
               _obj,
               _value,
               D_MEMORY_ORDER_SEQ_CST);
}

/*
d_atomic_compare_exchange_strong_ptr_explicit
  Atomically performs a strong compare-and-exchange on a pointer.

Parameter(s):
  _obj:      the atomic pointer to modify.
  _expected: pointer to the expected value; updated on failure.
  _desired:  the value to store on match.
  _success:  memory order on success.
  _failure:  memory order on failure.
Return:
  true if the exchange occurred, false otherwise.
*/
bool
d_atomic_compare_exchange_strong_ptr_explicit
(
    d_atomic_ptr*  _obj,
    void**         _expected,
    void*          _desired,
    d_memory_order _success,
    d_memory_order _failure
)
{
#if !D_ATOMIC_USE_STDATOMIC
    void* old_val;
#endif

    if ( (!_obj) || (!_expected) )
    {
        return false;
    }

#if D_ATOMIC_USE_STDATOMIC
    return atomic_compare_exchange_strong_explicit(
               _obj, _expected, _desired,
               _success, _failure);
#elif D_ATOMIC_USE_WINDOWS
    (void)_success;
    (void)_failure;

    #if (defined(_WIN64))
        old_val = (void*)InterlockedCompareExchange64(
                      (long long volatile*)_obj,
                      (long long)(size_t)_desired,
                      (long long)(size_t)*_expected);
    #else
        old_val = (void*)(size_t)InterlockedCompareExchange(
                      (long volatile*)_obj,
                      (long)(size_t)_desired,
                      (long)(size_t)*_expected);
    #endif

    if (old_val == *_expected)
    {
        return true;
    }

    *_expected = old_val;

    return false;
#else
    (void)_success;
    (void)_failure;

    old_val = (void*)__sync_val_compare_and_swap(
                  (void* volatile*)_obj,
                  *_expected,
                  _desired);

    if (old_val == *_expected)
    {
        return true;
    }

    *_expected = old_val;

    return false;
#endif
}

/*
d_atomic_compare_exchange_strong_ptr
  Strong compare-and-exchange on a pointer (sequential consistency).

Parameter(s):
  _obj:      the atomic pointer to modify.
  _expected: pointer to the expected value; updated on failure.
  _desired:  the value to store on match.
Return:
  true if the exchange occurred, false otherwise.
*/
bool
d_atomic_compare_exchange_strong_ptr
(
    d_atomic_ptr* _obj,
    void**        _expected,
    void*         _desired
)
{
    return d_atomic_compare_exchange_strong_ptr_explicit(
               _obj, _expected, _desired,
               D_MEMORY_ORDER_SEQ_CST,
               D_MEMORY_ORDER_SEQ_CST);
}

/*
d_atomic_compare_exchange_weak_ptr_explicit
  Weak compare-and-exchange on a pointer (may spuriously fail on
C11; identical to strong on Windows/__sync_*).

Parameter(s):
  _obj:      the atomic pointer to modify.
  _expected: pointer to the expected value; updated on failure.
  _desired:  the value to store on match.
  _success:  memory order on success.
  _failure:  memory order on failure.
Return:
  true if the exchange occurred, false otherwise.
*/
bool
d_atomic_compare_exchange_weak_ptr_explicit
(
    d_atomic_ptr*  _obj,
    void**         _expected,
    void*          _desired,
    d_memory_order _success,
    d_memory_order _failure
)
{
#if D_ATOMIC_USE_STDATOMIC
    if ( (!_obj) || (!_expected) )
    {
        return false;
    }

    return atomic_compare_exchange_weak_explicit(
               _obj, _expected, _desired,
               _success, _failure);
#else
    return d_atomic_compare_exchange_strong_ptr_explicit(
               _obj, _expected, _desired,
               _success, _failure);
#endif
}

/*
d_atomic_compare_exchange_weak_ptr
  Weak compare-and-exchange on a pointer (sequential consistency).

Parameter(s):
  _obj:      the atomic pointer to modify.
  _expected: pointer to the expected value; updated on failure.
  _desired:  the value to store on match.
Return:
  true if the exchange occurred, false otherwise.
*/
bool
d_atomic_compare_exchange_weak_ptr
(
    d_atomic_ptr* _obj,
    void**        _expected,
    void*         _desired
)
{
    return d_atomic_compare_exchange_weak_ptr_explicit(
               _obj, _expected, _desired,
               D_MEMORY_ORDER_SEQ_CST,
               D_MEMORY_ORDER_SEQ_CST);
}


///////////////////////////////////////////////////////////////////////////////
///             ATOMIC FLAG OPERATIONS                                      ///
///////////////////////////////////////////////////////////////////////////////

/*
d_atomic_flag_test_and_set_explicit
  Atomically sets the flag and returns the previous value.

Parameter(s):
  _flag:  the atomic flag.
  _order: the memory ordering constraint.
Return:
  the previous value of the flag.
*/
bool
d_atomic_flag_test_and_set_explicit
(
    d_atomic_flag* _flag,
    d_memory_order _order
)
{
    if (!_flag)
    {
        return false;
    }

#if D_ATOMIC_USE_STDATOMIC
    return atomic_flag_test_and_set_explicit(_flag, _order);
#elif D_ATOMIC_USE_WINDOWS
    (void)_order;
    return (bool)InterlockedExchange(&_flag->value, 1L);
#else
    (void)_order;
    return (bool)__sync_lock_test_and_set(&_flag->value, 1);
#endif
}

/*
d_atomic_flag_test_and_set
  Atomically sets the flag (sequential consistency).

Parameter(s):
  _flag: the atomic flag.
Return:
  the previous value of the flag.
*/
bool
d_atomic_flag_test_and_set
(
    d_atomic_flag* _flag
)
{
    return d_atomic_flag_test_and_set_explicit(
               _flag,
               D_MEMORY_ORDER_SEQ_CST);
}

/*
d_atomic_flag_clear_explicit
  Atomically clears the flag.

Parameter(s):
  _flag:  the atomic flag.
  _order: the memory ordering constraint.
Return:
  none.
*/
void
d_atomic_flag_clear_explicit
(
    d_atomic_flag* _flag,
    d_memory_order _order
)
{
    if (!_flag)
    {
        return;
    }

#if D_ATOMIC_USE_STDATOMIC
    atomic_flag_clear_explicit(_flag, _order);
#elif D_ATOMIC_USE_WINDOWS
    (void)_order;
    InterlockedExchange(&_flag->value, 0L);
#else
    (void)_order;
    __sync_lock_release(&_flag->value);
#endif

    return;
}

/*
d_atomic_flag_clear
  Atomically clears the flag (sequential consistency).

Parameter(s):
  _flag: the atomic flag.
Return:
  none.
*/
void
d_atomic_flag_clear
(
    d_atomic_flag* _flag
)
{
    d_atomic_flag_clear_explicit(_flag,
                                 D_MEMORY_ORDER_SEQ_CST);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             MEMORY ORDERING AND FENCES                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_atomic_thread_fence
  Issues a thread fence with the specified memory order.

Parameter(s):
  _order: the memory ordering constraint.
Return:
  none.
*/
void
d_atomic_thread_fence
(
    d_memory_order _order
)
{
#if D_ATOMIC_USE_STDATOMIC
    atomic_thread_fence(_order);
#elif D_ATOMIC_USE_WINDOWS
    (void)_order;
    MemoryBarrier();
#else
    (void)_order;
    __sync_synchronize();
#endif

    return;
}

/*
d_atomic_signal_fence
  Issues a signal fence with the specified memory order.

Parameter(s):
  _order: the memory ordering constraint.
Return:
  none.
*/
void
d_atomic_signal_fence
(
    d_memory_order _order
)
{
#if D_ATOMIC_USE_STDATOMIC
    atomic_signal_fence(_order);
#elif D_ATOMIC_USE_WINDOWS
    (void)_order;
    _ReadWriteBarrier();
#else
    (void)_order;
    __asm__ __volatile__("" : : : "memory");
#endif

    return;
}

/*
d_atomic_is_lock_free_1
  Reports whether 1-byte atomics are lock-free.

Parameter(s):
  none.
Return:
  true if 1-byte atomics are lock-free on this platform.
*/
bool
d_atomic_is_lock_free_1
(
    void
)
{
#if D_ATOMIC_USE_STDATOMIC
    #ifdef ATOMIC_CHAR_LOCK_FREE
        return (ATOMIC_CHAR_LOCK_FREE == 2);
    #else
        return false;
    #endif
#else
    return true;
#endif
}

/*
d_atomic_is_lock_free_2
  Reports whether 2-byte atomics are lock-free.

Parameter(s):
  none.
Return:
  true if 2-byte atomics are lock-free on this platform.
*/
bool
d_atomic_is_lock_free_2
(
    void
)
{
#if D_ATOMIC_USE_STDATOMIC
    #ifdef ATOMIC_SHORT_LOCK_FREE
        return (ATOMIC_SHORT_LOCK_FREE == 2);
    #else
        return false;
    #endif
#else
    return true;
#endif
}

/*
d_atomic_is_lock_free_4
  Reports whether 4-byte atomics are lock-free.

Parameter(s):
  none.
Return:
  true if 4-byte atomics are lock-free on this platform.
*/
bool
d_atomic_is_lock_free_4
(
    void
)
{
#if D_ATOMIC_USE_STDATOMIC
    #ifdef ATOMIC_INT_LOCK_FREE
        return (ATOMIC_INT_LOCK_FREE == 2);
    #else
        return false;
    #endif
#else
    return true;
#endif
}

/*
d_atomic_is_lock_free_8
  Reports whether 8-byte atomics are lock-free.

Parameter(s):
  none.
Return:
  true if 8-byte atomics are lock-free on this platform.
*/
bool
d_atomic_is_lock_free_8
(
    void
)
{
#if D_ATOMIC_USE_STDATOMIC
    #ifdef ATOMIC_LLONG_LOCK_FREE
        return (ATOMIC_LLONG_LOCK_FREE == 2);
    #else
        return false;
    #endif
#elif D_ATOMIC_USE_WINDOWS
    #if (defined(_WIN64))
        return true;
    #else
        return false;
    #endif
#else
    return (sizeof(long long) <= sizeof(void*));
#endif
}

