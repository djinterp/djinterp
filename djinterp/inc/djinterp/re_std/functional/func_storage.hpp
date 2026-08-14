/******************************************************************************
* re_std [functional]                                          func_storage.hpp
*
*   internal storage core for move_only_function and copyable_function.
*
*   Not a public header.  Provides the small-buffer union, the lifetime
* operations table, and the manager that binds a concrete target type to them.
*
*   WHY SMALL-BUFFER AT ALL.
*   The overwhelmingly common target is a capture-light lambda or a function
* pointer - one or two words.  Heap-allocating those would make every
* construction an allocation and every call a second indirection.  The buffer
* is three pointers wide, which holds a function pointer, a member-pointer
* (two words on the Itanium ABI, more on MSVC's), or a lambda capturing a
* pointer and an index, without touching the allocator.
*
*   THE SBO ELIGIBILITY TEST INCLUDES is_nothrow_move_constructible, AND THAT
* IS NOT AN OPTIMISATION - IT IS REQUIRED FOR CORRECTNESS.  A wrapper's own
* move constructor is noexcept, so it must be able to relocate an in-buffer
* target without any chance of throwing.  A type whose move can throw is
* therefore forced onto the heap, where relocation is a pointer copy and
* cannot fail.  Dropping that condition would give a noexcept move that can
* terminate.
*
*   OPERATIONS ARE A STATIC TABLE, NOT VIRTUAL FUNCTIONS.
*   A vtable pointer would force every target onto a polymorphic base and cost
* an allocation for the base subobject.  One shared `const func_ops*` per
* target type, pointing at a namespace-scope constant, keeps the wrapper two
* words of state plus the buffer, and makes an empty wrapper exactly "ops is
* null" with no other bookkeeping.
*
*   `copy` IS NULL FOR MOVE-ONLY TARGETS and is simply never called by
* move_only_function.  Sharing one table shape between the copyable and
* move-only wrappers is what lets them share this entire file.
*
*
* path:      /inc/djinterp/re_std/functional/func_storage.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_FUNCTIONAL_FUNC_STORAGE_
#define RESTD_FUNCTIONAL_FUNC_STORAGE_ 1

// re_std
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../memory/addressof.hpp"
#include "./invoke.hpp"

NS_DJINTERP
NS_RESTD
NS_INTERNAL

// func_buffer
//   union: the small-buffer storage.  The alignment members are declared for
// their alignment only and are never read.
union func_buffer
{
    void* m_ptr;
    char  m_data[3 * sizeof(void*)];
    void (*m_align_func)();
    long double m_align_max;
};

// func_fits
//   trait: may _Target live inside the buffer?  All three conditions are
// load-bearing; see the header note on the noexcept one in particular.
template<typename _Target>
struct func_fits
{
    static const bool value =
        (   sizeof(_Target)  <= sizeof(func_buffer)
         && alignof(_Target) <= alignof(func_buffer)
         && is_nothrow_move_constructible<_Target>::value);
};

// func_ops
//   struct: the lifetime operations for one target type.  `copy` is null when
// the target is not copy-constructible.
struct func_ops
{
    void (*destroy)(func_buffer&);
    void (*move)(func_buffer&, func_buffer&);
    void (*copy)(func_buffer&, const func_buffer&);
};

// func_manager
//   struct: binds a target type to a func_ops table.  Primary template is the
// SBO case; the specialisation below is the heap case.
template<typename _Target, bool _Small = func_fits<_Target>::value>
struct func_manager
{
    static _Target& get(func_buffer& b)
    {
        return *static_cast<_Target*>(static_cast<void*>(b.m_data));
    }

    static const _Target& get(const func_buffer& b)
    {
        return *static_cast<const _Target*>(
            static_cast<const void*>(b.m_data));
    }

    template<typename... _Args>
    static void construct(func_buffer& b, _Args&&... args)
    {
        ::new (static_cast<void*>(b.m_data))
            _Target(static_cast<_Args&&>(args)...);
        return;
    }

    static void destroy(func_buffer& b)
    {
        get(b).~_Target();
        return;
    }

    static void move(func_buffer& to, func_buffer& from)
    {
        ::new (static_cast<void*>(to.m_data))
            _Target(static_cast<_Target&&>(get(from)));
        get(from).~_Target();
        return;
    }

    static void copy(func_buffer& to, const func_buffer& from)
    {
        ::new (static_cast<void*>(to.m_data)) _Target(get(from));
        return;
    }
};

// func_manager<_Target, false>
//   struct: heap case.  Relocation is a pointer copy, which is why a
// throwing-move target is routed here.
template<typename _Target>
struct func_manager<_Target, false>
{
    static _Target& get(func_buffer& b)
    { return *static_cast<_Target*>(b.m_ptr); }

    static const _Target& get(const func_buffer& b)
    { return *static_cast<const _Target*>(b.m_ptr); }

    template<typename... _Args>
    static void construct(func_buffer& b, _Args&&... args)
    {
        b.m_ptr = new _Target(static_cast<_Args&&>(args)...);
        return;
    }

    static void destroy(func_buffer& b)
    {
        delete static_cast<_Target*>(b.m_ptr);
        b.m_ptr = 0;
        return;
    }

    static void move(func_buffer& to, func_buffer& from)
    {
        to.m_ptr   = from.m_ptr;
        from.m_ptr = 0;
        return;
    }

    static void copy(func_buffer& to, const func_buffer& from)
    {
        to.m_ptr = new _Target(*static_cast<const _Target*>(from.m_ptr));
        return;
    }
};

// func_ops_holder
//   struct: the shared operations table for _Target, SPECIALISED on _Copyable
// rather than selecting the copy slot with a ternary.
//
//   That distinction is not stylistic and the difference is invisible until it
// bites.  Writing the table as
//
//       _Copyable ? &func_manager<_Target>::copy : 0
//
// looks like it omits copy for a move-only target, but `?:` is a RUN-TIME
// selection: naming func_manager<_Target>::copy in either branch odr-uses it
// and forces its instantiation, whose body copy-constructs _Target.  For a
// move-only target - a lambda capturing a unique_ptr, say, which is the whole
// reason move_only_function exists - that is a hard error inside a function
// nobody will ever call.
//
//   Specialising on the flag means the copy branch is never instantiated at
// all for the move-only case.  Found by compiling, not by reading.
template<typename _Target, bool _Copyable>
struct func_ops_holder;

template<typename _Target>
struct func_ops_holder<_Target, true>
{
    static const func_ops value;
};

template<typename _Target>
const func_ops func_ops_holder<_Target, true>::value =
{
    &func_manager<_Target>::destroy,
    &func_manager<_Target>::move,
    &func_manager<_Target>::copy
};

template<typename _Target>
struct func_ops_holder<_Target, false>
{
    static const func_ops value;
};

template<typename _Target>
const func_ops func_ops_holder<_Target, false>::value =
{
    &func_manager<_Target>::destroy,
    &func_manager<_Target>::move,
    0
};

NS_END  // internal
NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_FUNCTIONAL_FUNC_STORAGE_
