/******************************************************************************
* djinterp [restd]                                                    variant.hpp
*
*   restd's back-port of std::variant<Ts...> — type-safe sum type
* (discriminated union). C++17 in std; restd targets C++11+.
*
*   STORAGE MODEL:
*   Recursive-union storage (same pattern as libc++ and Microsoft
* STL). Each instantiation holds a head element and a recursive
* tail-union of the remaining types. Access via internal::storage_at<I>,
* a recursive trait that walks the head/tail chain.
*
*   LIFETIME MANAGEMENT:
*   The union has user-provided ctor/dtor with empty bodies — the
* surrounding variant manages active-alternative lifetime via
* placement-new and explicit destructor calls. Same approach used
* by expected<T,E>.
*
*   VALUELESS-BY-EXCEPTION:
*   If an alternative's assignment throws and the variant cannot
* recover (no fallback construction succeeds), m_index becomes
* variant_npos. Subsequent get<I>/get<T>/visit throw bad_variant_access.
* This is rare in practice (requires throwing move ctors), but the
* state must be representable per the standard.
*
*   FORWARDING CTOR — SIMPLIFICATION:
*   The standard's "imaginary function overload set" (P0608) picks
* the alternative whose construction from the forwarded argument
* would not be a narrowing conversion. restd's back-port simplifies:
* selects the FIRST alternative T_i such that
* is_constructible<T_i, Arg> is true. Common cases (an integer
* constructs the int alternative, a string-literal constructs the
* string alternative when there's no overlap) work identically;
* edge cases where multiple alternatives are convertible from the
* same source diverge. Documented.
*
*   NOT IMPLEMENTED:
*   - operator<=> (C++20) — separate deferred phase
*   - visit<R> (C++20 explicit return) — deferred
*   - Multi-variant visit — deferred
*   - hash<variant> — blocked on <functional>
*   - Reference alternatives — deferred (rebinding semantics)
*   - Allocator-aware ctors — niche, skipped
*   - Conditional triviality — always emits user-defined SMFs
*     (correctness over optimisation, documented)
*   - constexpr — not constexpr at this phase (back-port simplification)
*
*
* path:      /inc/djinterp/restd/variant/variant.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_VARIANT_
#define DJINTERP_RESTD_VARIANT_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <new>
#include <cstddef>
#include <utility>          // std::declval, std::move/forward equivalents
#include <initializer_list>

#include "./bad_variant_access.hpp"
#include "./variant_npos.hpp"

#include "../utility/in_place.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/is_constructible.hpp"
#include "../type_traits/is_nothrow_move_constructible.hpp"
#include "../type_traits/decay.hpp"


NS_RESTD


// ===========================================================================
// 0.   INTERNAL HELPERS
// ===========================================================================

NS_INTERNAL

    // ------------------------------------------------------------------
    // variant_storage<Types...> — recursive union of alternatives.
    // ------------------------------------------------------------------

    template<typename... _Ts> union variant_storage;

    template<>
    union variant_storage<>
    {
        // Empty terminator. ctor/dtor body empty.
        variant_storage() {}
        ~variant_storage() {}
    };

    template<typename _Head, typename... _Tail>
    union variant_storage<_Head, _Tail...>
    {
        _Head                       m_head;
        variant_storage<_Tail...>   m_tail;

        // ctor body empty — the surrounding variant placement-news
        // the right member explicitly.
        variant_storage() {}
        // dtor body empty — variant destroys the active member
        // explicitly before this union's dtor runs.
        ~variant_storage() {}
    };


    // ------------------------------------------------------------------
    // storage_at<I> — walks the head/tail chain to find element I.
    // ------------------------------------------------------------------

    template<std::size_t _I>
    struct storage_at
    {
        template<typename _Head, typename... _Tail>
        static auto get(variant_storage<_Head, _Tail...>& _s)
            -> decltype(storage_at<_I - 1>::get(_s.m_tail))
        {
            return storage_at<_I - 1>::get(_s.m_tail);
        }

        template<typename _Head, typename... _Tail>
        static auto get(variant_storage<_Head, _Tail...> const& _s)
            -> decltype(storage_at<_I - 1>::get(_s.m_tail))
        {
            return storage_at<_I - 1>::get(_s.m_tail);
        }
    };

    template<>
    struct storage_at<0>
    {
        template<typename _Head, typename... _Tail>
        static _Head& get(variant_storage<_Head, _Tail...>& _s)
        {
            return _s.m_head;
        }

        template<typename _Head, typename... _Tail>
        static _Head const& get(variant_storage<_Head, _Tail...> const& _s)
        {
            return _s.m_head;
        }
    };


    // ------------------------------------------------------------------
    // type_at<I, Types...> — picks the I-th type from a pack.
    // (Duplicates the trait in variant_alternative.hpp so variant.hpp
    // is self-contained for this commonly-used helper.)
    // ------------------------------------------------------------------

    template<std::size_t _I, typename _Head, typename... _Tail>
    struct va_type_at
    {
        typedef typename va_type_at<_I - 1, _Tail...>::type type;
    };

    template<typename _Head, typename... _Tail>
    struct va_type_at<0, _Head, _Tail...>
    {
        typedef _Head type;
    };


    // ------------------------------------------------------------------
    // index_of<T, Types...> — finds the index of T in the pack.
    // Returns sizeof...(Types) if not found (out-of-range).
    // ------------------------------------------------------------------

    template<typename _T, typename... _Types>
    struct index_of;

    template<typename _T>
    struct index_of<_T>
    {
        static const std::size_t value = 0;
    };

    template<typename _T, typename _Head, typename... _Tail>
    struct index_of<_T, _Head, _Tail...>
    {
        static const std::size_t value =
            restd::is_same<_T, _Head>::value
                ? 0
                : 1 + index_of<_T, _Tail...>::value;
    };


    // ------------------------------------------------------------------
    // first_constructible<U, Types...> — finds the FIRST index k such
    // that is_constructible<T_k, U>::value is true. Returns sizeof...(Types)
    // if none match. Used as the fallback layer of best_match below.
    // ------------------------------------------------------------------

    template<typename _U, typename... _Types>
    struct first_constructible;

    template<typename _U>
    struct first_constructible<_U>
    {
        static const std::size_t value = 0;
    };

    template<typename _U, typename _Head, typename... _Tail>
    struct first_constructible<_U, _Head, _Tail...>
    {
        static const std::size_t value =
            restd::is_constructible<_Head, _U>::value
                ? 0
                : 1 + first_constructible<_U, _Tail...>::value;
    };


    // ------------------------------------------------------------------
    // exact_match<U, Types...> — finds the index k such that
    // is_same<decay_t<U>, T_k>::value is true. Returns sizeof...(Types)
    // if no exact match exists (caller falls through to first_constructible).
    // ------------------------------------------------------------------

    template<typename _U, typename... _Types>
    struct exact_match;

    template<typename _U>
    struct exact_match<_U>
    {
        static const std::size_t value = 0;
    };

    template<typename _U, typename _Head, typename... _Tail>
    struct exact_match<_U, _Head, _Tail...>
    {
        static const std::size_t value =
            restd::is_same<
                typename restd::decay<_U>::type, _Head
            >::value
                ? 0
                : 1 + exact_match<_U, _Tail...>::value;
    };


    // ------------------------------------------------------------------
    // best_match<U, Types...> — picks the alternative for a forwarding
    // ctor. Two-pass: exact-match preferred, falls through to
    // first-constructible. Documented divergence from std's full
    // "imaginary function" rule (P0608), but handles the common
    // narrowing-rejection cases correctly:
    //
    //   variant<int, double, string> v = 3.14;
    //     exact_match     -> double (index 1) -> selected.
    //
    //   variant<int, string> v = "literal";
    //     exact_match     -> none (decay<const char[N]> = const char*)
    //     first_constructible -> string (index 1) -> selected.
    //
    //   Edge cases that diverge from std (multiple convertible-from-
    //   the-same-source alternatives with no exact match) are
    //   documented in the variant header subtitle.
    // ------------------------------------------------------------------

    template<typename _U, typename... _Types>
    struct best_match
    {
        static const std::size_t exact = exact_match<_U, _Types...>::value;
        static const std::size_t fallback = first_constructible<_U, _Types...>::value;
        static const std::size_t value =
            (exact < sizeof...(_Types)) ? exact : fallback;
    };

NS_END  // internal


// ===========================================================================
// I.   VARIANT<Types...>
// ===========================================================================

template<typename... _Types>
class variant
{
    static_assert(sizeof...(_Types) > 0,
                  "restd::variant must have at least one alternative");

public:
    // =================================================================
    // CTORS
    // =================================================================

    // (1) default ctor — value-initialises the FIRST alternative.
    //   Requires the first alternative to be default-constructible.
    template<typename _T0 = typename internal::va_type_at<0, _Types...>::type,
             typename = typename restd::enable_if<
                 restd::is_constructible<_T0>::value
             >::type>
    variant()
        : m_storage(), m_index(0)
    {
        typedef _T0 _FirstT;
        new (static_cast<void*>(&internal::storage_at<0>::get(m_storage))) _FirstT();
    }

    // (2) copy ctor
    variant(variant const& _other)
        : m_storage(), m_index(_other.m_index)
    {
        if (_other.m_index != variant_npos)
        {
            _copy_construct_from(_other.m_index, _other.m_storage,
                                 _index_seq());
        }
    }

    // (3) move ctor
    variant(variant&& _other)
        : m_storage(), m_index(_other.m_index)
    {
        if (_other.m_index != variant_npos)
        {
            _move_construct_from(_other.m_index, _other.m_storage,
                                 _index_seq());
        }
    }

    // (4) forwarding-from-U ctor
    //   Selects the best alternative via best_match: exact match
    //   preferred, falls back to first-constructible. Documented
    //   divergence from std's full "imaginary function" rule.
    template<typename _U,
             typename = typename restd::enable_if<
                 !restd::is_same<typename restd::decay<_U>::type, variant>::value &&
                 (internal::best_match<_U, _Types...>::value
                    < sizeof...(_Types))
             >::type>
    variant(_U&& _u)
        : m_storage(),
          m_index(internal::best_match<_U, _Types...>::value)
    {
        static const std::size_t _idx =
            internal::best_match<_U, _Types...>::value;
        typedef typename internal::va_type_at<_idx, _Types...>::type _T;
        new (static_cast<void*>(&internal::storage_at<_idx>::get(m_storage)))
            _T(static_cast<_U&&>(_u));
    }

    // (5) in_place_type ctor
    template<typename _T,
             typename... _Args,
             typename = typename restd::enable_if<
                 (internal::index_of<_T, _Types...>::value < sizeof...(_Types)) &&
                 restd::is_constructible<_T, _Args...>::value
             >::type>
    explicit variant(in_place_type_t<_T>, _Args&&... _args)
        : m_storage(),
          m_index(internal::index_of<_T, _Types...>::value)
    {
        static const std::size_t _idx = internal::index_of<_T, _Types...>::value;
        new (static_cast<void*>(&internal::storage_at<_idx>::get(m_storage)))
            _T(static_cast<_Args&&>(_args)...);
    }

    // (6) in_place_index ctor
    template<std::size_t _I,
             typename... _Args,
             typename _T = typename internal::va_type_at<_I, _Types...>::type,
             typename = typename restd::enable_if<
                 (_I < sizeof...(_Types)) &&
                 restd::is_constructible<_T, _Args...>::value
             >::type>
    explicit variant(in_place_index_t<_I>, _Args&&... _args)
        : m_storage(), m_index(_I)
    {
        typedef typename internal::va_type_at<_I, _Types...>::type _Type;
        new (static_cast<void*>(&internal::storage_at<_I>::get(m_storage)))
            _Type(static_cast<_Args&&>(_args)...);
    }

    // =================================================================
    // DESTRUCTOR
    // =================================================================

    ~variant()
    {
        _destroy();
    }

    // =================================================================
    // ASSIGNMENT
    // =================================================================

    variant& operator=(variant const& _other)
    {
        if (this != &_other)
        {
            _destroy();
            m_index = _other.m_index;
            if (_other.m_index != variant_npos)
            {
                _copy_construct_from(_other.m_index, _other.m_storage,
                                     _index_seq());
            }
        }
        return *this;
    }

    variant& operator=(variant&& _other)
        D_NOEXCEPT_IF(false /* simplified: not promising the noexcept */)
    {
        if (this != &_other)
        {
            _destroy();
            m_index = _other.m_index;
            if (_other.m_index != variant_npos)
            {
                _move_construct_from(_other.m_index, _other.m_storage,
                                     _index_seq());
            }
        }
        return *this;
    }

    // forwarding-from-U assignment
    template<typename _U,
             typename = typename restd::enable_if<
                 !restd::is_same<typename restd::decay<_U>::type, variant>::value &&
                 (internal::best_match<_U, _Types...>::value
                    < sizeof...(_Types))
             >::type>
    variant& operator=(_U&& _u)
    {
        _destroy();
        static const std::size_t _idx =
            internal::best_match<_U, _Types...>::value;
        typedef typename internal::va_type_at<_idx, _Types...>::type _T;
        new (static_cast<void*>(&internal::storage_at<_idx>::get(m_storage)))
            _T(static_cast<_U&&>(_u));
        m_index = _idx;
        return *this;
    }

    // =================================================================
    // EMPLACE
    // =================================================================

    // emplace<T>(args...) — replaces with T constructed from args.
    template<typename _T, typename... _Args>
    _T& emplace(_Args&&... _args)
    {
        _destroy();
        static const std::size_t _idx = internal::index_of<_T, _Types...>::value;
        new (static_cast<void*>(&internal::storage_at<_idx>::get(m_storage)))
            _T(static_cast<_Args&&>(_args)...);
        m_index = _idx;
        return internal::storage_at<_idx>::get(m_storage);
    }

    // emplace<I>(args...) — replaces with the I-th alternative.
    template<std::size_t _I, typename... _Args>
    typename internal::va_type_at<_I, _Types...>::type&
    emplace(_Args&&... _args)
    {
        _destroy();
        typedef typename internal::va_type_at<_I, _Types...>::type _T;
        new (static_cast<void*>(&internal::storage_at<_I>::get(m_storage)))
            _T(static_cast<_Args&&>(_args)...);
        m_index = _I;
        return internal::storage_at<_I>::get(m_storage);
    }

    // =================================================================
    // OBSERVERS
    // =================================================================

    std::size_t index() const D_NOEXCEPT { return m_index; }

    bool valueless_by_exception() const D_NOEXCEPT
    {
        return m_index == variant_npos;
    }

    // =================================================================
    // SWAP
    // =================================================================

    void swap(variant& _other)
    {
        if (m_index == _other.m_index)
        {
            if (m_index != variant_npos)
            {
                _swap_same_index(_other.m_index, m_storage, _other.m_storage,
                                 _index_seq());
            }
        }
        else
        {
            // Different alternatives: copy through a temporary.
            variant _tmp(static_cast<variant&&>(*this));
            *this  = static_cast<variant&&>(_other);
            _other = static_cast<variant&&>(_tmp);
        }
    }

    // =================================================================
    // INTERNAL ACCESSORS (for get<I> / visit / etc.)
    // =================================================================
    // Not strictly "private", but conventionally treated as such — the
    // _ prefix marks them as implementation detail. Free functions in
    // sibling headers reach in through these.

    template<std::size_t _I>
    typename internal::va_type_at<_I, _Types...>::type&
    _ref()
    {
        return internal::storage_at<_I>::get(m_storage);
    }

    template<std::size_t _I>
    typename internal::va_type_at<_I, _Types...>::type const&
    _ref() const
    {
        return internal::storage_at<_I>::get(m_storage);
    }

private:

    // =================================================================
    // STORAGE
    // =================================================================

    internal::variant_storage<_Types...>    m_storage;
    std::size_t                             m_index;

    // =================================================================
    // INTERNAL HELPERS — index-dispatched lifetime ops
    // =================================================================
    // We need to destroy / copy / move / swap the alternative at a
    // RUNTIME index. The dispatch is a compile-time unrolled if-else
    // chain over the index range [0, sizeof...(Types)).

    // Index sequence helper (avoids depending on full integer_sequence).
    template<std::size_t...> struct _idx_seq {};

    template<std::size_t _N, std::size_t... _Acc>
    struct _make_idx_seq : _make_idx_seq<_N - 1, _N - 1, _Acc...> {};

    template<std::size_t... _Acc>
    struct _make_idx_seq<0, _Acc...>
    {
        typedef _idx_seq<_Acc...> type;
    };

    typedef typename _make_idx_seq<sizeof...(_Types)>::type _index_seq_type;
    static _index_seq_type _index_seq() { return _index_seq_type(); }

    // Destroy the active alternative.
    void _destroy()
    {
        if (m_index != variant_npos)
        {
            _destroy_dispatch(m_index, _index_seq());
        }
        m_index = variant_npos;
    }

    template<std::size_t... _Is>
    void _destroy_dispatch(std::size_t _i, _idx_seq<_Is...>)
    {
        // Evaluate left-to-right; calls _destroy_one<I>() exactly once
        // for the matching I via an initializer-list expansion.
        // Cast to void array to discard the result and ensure ordering.
        using _expander = int[];
        (void)_expander{ 0, (_destroy_one<_Is>(_i), 0)... };
    }

    template<std::size_t _I>
    void _destroy_one(std::size_t _active)
    {
        if (_active == _I)
        {
            typedef typename internal::va_type_at<_I, _Types...>::type _T;
            internal::storage_at<_I>::get(m_storage).~_T();
        }
    }

    // Copy-construct from another variant's storage at the active index.
    template<std::size_t... _Is>
    void _copy_construct_from(std::size_t _i,
                              internal::variant_storage<_Types...> const& _src,
                              _idx_seq<_Is...>)
    {
        using _expander = int[];
        (void)_expander{ 0, (_copy_one<_Is>(_i, _src), 0)... };
    }

    template<std::size_t _I>
    void _copy_one(std::size_t _active,
                   internal::variant_storage<_Types...> const& _src)
    {
        if (_active == _I)
        {
            typedef typename internal::va_type_at<_I, _Types...>::type _T;
            new (static_cast<void*>(&internal::storage_at<_I>::get(m_storage)))
                _T(internal::storage_at<_I>::get(_src));
        }
    }

    // Move-construct from another variant's storage at the active index.
    template<std::size_t... _Is>
    void _move_construct_from(std::size_t _i,
                              internal::variant_storage<_Types...>& _src,
                              _idx_seq<_Is...>)
    {
        using _expander = int[];
        (void)_expander{ 0, (_move_one<_Is>(_i, _src), 0)... };
    }

    template<std::size_t _I>
    void _move_one(std::size_t _active,
                   internal::variant_storage<_Types...>& _src)
    {
        if (_active == _I)
        {
            typedef typename internal::va_type_at<_I, _Types...>::type _T;
            new (static_cast<void*>(&internal::storage_at<_I>::get(m_storage)))
                _T(static_cast<_T&&>(internal::storage_at<_I>::get(_src)));
        }
    }

    // Swap when both variants hold the SAME alternative — ADL swap on
    // the held alternative.
    template<std::size_t... _Is>
    void _swap_same_index(std::size_t _i,
                          internal::variant_storage<_Types...>& _a,
                          internal::variant_storage<_Types...>& _b,
                          _idx_seq<_Is...>)
    {
        using _expander = int[];
        (void)_expander{ 0, (_swap_one<_Is>(_i, _a, _b), 0)... };
    }

    template<std::size_t _I>
    void _swap_one(std::size_t _active,
                   internal::variant_storage<_Types...>& _a,
                   internal::variant_storage<_Types...>& _b)
    {
        if (_active == _I)
        {
            using std::swap;
            swap(internal::storage_at<_I>::get(_a),
                 internal::storage_at<_I>::get(_b));
        }
    }
};


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_VARIANT_
