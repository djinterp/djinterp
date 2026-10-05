/*******************************************************************************
* djinterp [re_std]                                                  variant.hpp
*
* variant class header:
*   re_std's back-port of std::variant<Ts...> — type-safe sum type
* (discriminated union). C++17 in std; re_std targets C++11+.
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
* would not be a narrowing conversion. re_std's back-port simplifies:
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
* path:      /inc/re_std/variant/variant.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_VARIANT_VARIANT_HPP
#define RE_STD_VARIANT_VARIANT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <initializer_list>
#include <new>
#include <utility>          // std::declval, std::move/forward equivalents

#include "./bad_variant_access.hpp"
#include "./variant_npos.hpp"

#include "../optional/in_place.hpp"
#include "../utility/in_place_type.hpp"    // in_place_type_t
#include "../utility/in_place_index.hpp"   // in_place_index_t
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/is_constructible.hpp"
#include "../type_traits/is_nothrow_move_constructible.hpp"
#include "../type_traits/decay.hpp"


namespace re_std
{


// ===========================================================================
// 0.   INTERNAL HELPERS
// ===========================================================================

namespace internal
{

    // ------------------------------------------------------------------
    // variant_storage<Types...> — recursive union of alternatives.
    // ------------------------------------------------------------------

    template<typename... Ts> union variant_storage;

    template<>
    union variant_storage<>
    {
        // Empty terminator. ctor/dtor body empty.
        variant_storage() {}
        ~variant_storage() {}
    };

    template<typename Head, typename... Tail>
    union variant_storage<Head, Tail...>
    {
        Head                       m_head;
        variant_storage<Tail...>   m_tail;

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

    template<std::size_t I>
    struct storage_at
    {
        template<typename Head, typename... Tail>
        static auto get(variant_storage<Head, Tail...>& _s)
            -> decltype(storage_at<I - 1>::get(_s.m_tail))
        {
            return storage_at<I - 1>::get(_s.m_tail);
        }

        template<typename Head, typename... Tail>
        static auto get(variant_storage<Head, Tail...> const& _s)
            -> decltype(storage_at<I - 1>::get(_s.m_tail))
        {
            return storage_at<I - 1>::get(_s.m_tail);
        }
    };

    template<>
    struct storage_at<0>
    {
        template<typename Head, typename... Tail>
        static Head& get(variant_storage<Head, Tail...>& _s)
        {
            return _s.m_head;
        }

        template<typename Head, typename... Tail>
        static Head const& get(variant_storage<Head, Tail...> const& _s)
        {
            return _s.m_head;
        }
    };


    // ------------------------------------------------------------------
    // type_at<I, Types...> — picks the I-th type from a pack.
    // (Duplicates the trait in variant_alternative.hpp so variant.hpp
    // is self-contained for this commonly-used helper.)
    // ------------------------------------------------------------------

    template<std::size_t I, typename Head, typename... Tail>
    struct va_type_at
    {
        typedef typename va_type_at<I - 1, Tail...>::type type;
    };

    template<typename Head, typename... Tail>
    struct va_type_at<0, Head, Tail...>
    {
        typedef Head type;
    };


    // ------------------------------------------------------------------
    // index_of<T, Types...> — finds the index of T in the pack.
    // Returns sizeof...(Types) if not found (out-of-range).
    // ------------------------------------------------------------------

    template<typename T, typename... Types>
    struct index_of;

    template<typename T>
    struct index_of<T>
    {
        static const std::size_t value = 0;
    };

    template<typename T, typename Head, typename... Tail>
    struct index_of<T, Head, Tail...>
    {
        static const std::size_t value =
            re_std::is_same<T, Head>::value
                ? 0
                : 1 + index_of<T, Tail...>::value;
    };


    // ------------------------------------------------------------------
    // first_constructible<U, Types...> — finds the FIRST index k such
    // that is_constructible<T_k, U>::value is true. Returns sizeof...(Types)
    // if none match. Used as the fallback layer of best_match below.
    // ------------------------------------------------------------------

    template<typename U, typename... Types>
    struct first_constructible;

    template<typename U>
    struct first_constructible<U>
    {
        static const std::size_t value = 0;
    };

    template<typename U, typename Head, typename... Tail>
    struct first_constructible<U, Head, Tail...>
    {
        static const std::size_t value =
            re_std::is_constructible<Head, U>::value
                ? 0
                : 1 + first_constructible<U, Tail...>::value;
    };


    // ------------------------------------------------------------------
    // exact_match<U, Types...> — finds the index k such that
    // is_same<decay_t<U>, T_k>::value is true. Returns sizeof...(Types)
    // if no exact match exists (caller falls through to first_constructible).
    // ------------------------------------------------------------------

    template<typename U, typename... Types>
    struct exact_match;

    template<typename U>
    struct exact_match<U>
    {
        static const std::size_t value = 0;
    };

    template<typename U, typename Head, typename... Tail>
    struct exact_match<U, Head, Tail...>
    {
        static const std::size_t value =
            re_std::is_same<
                typename re_std::decay<U>::type, Head
            >::value
                ? 0
                : 1 + exact_match<U, Tail...>::value;
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

    template<typename U, typename... Types>
    struct best_match
    {
        static const std::size_t exact = exact_match<U, Types...>::value;
        static const std::size_t fallback = first_constructible<U, Types...>::value;
        static const std::size_t value =
            (exact < sizeof...(Types)) ? exact : fallback;
    };

}  // internal


// ===========================================================================
// I.   VARIANT<Types...>
// ===========================================================================

template<typename... Types>
class variant
{
    static_assert(sizeof...(Types) > 0,
                  "re_std::variant must have at least one alternative");

public:
    // =================================================================
    // CTORS
    // =================================================================

    // (1) default ctor — value-initialises the FIRST alternative.
    //   Requires the first alternative to be default-constructible.
    template<typename T0 = typename internal::va_type_at<0, Types...>::type,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<T0>::value
             >::type>
    variant()
        : m_storage(), m_index(0)
    {
        typedef T0 _FirstT;
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
    template<typename U,
             typename = typename re_std::enable_if<
                 !re_std::is_same<typename re_std::decay<U>::type, variant>::value &&
                 (internal::best_match<U, Types...>::value
                    < sizeof...(Types))
             >::type>
    variant(U&& _u)
        : m_storage(),
          m_index(internal::best_match<U, Types...>::value)
    {
        static const std::size_t _idx =
            internal::best_match<U, Types...>::value;
        typedef typename internal::va_type_at<_idx, Types...>::type T;
        new (static_cast<void*>(&internal::storage_at<_idx>::get(m_storage)))
            T(static_cast<U&&>(_u));
    }

    // (5) in_place_type ctor
    template<typename T,
             typename... Args,
             typename = typename re_std::enable_if<
                 (internal::index_of<T, Types...>::value < sizeof...(Types)) &&
                 re_std::is_constructible<T, Args...>::value
             >::type>
    explicit variant(in_place_type_t<T>, Args&&... _args)
        : m_storage(),
          m_index(internal::index_of<T, Types...>::value)
    {
        static const std::size_t _idx = internal::index_of<T, Types...>::value;
        new (static_cast<void*>(&internal::storage_at<_idx>::get(m_storage)))
            T(static_cast<Args&&>(_args)...);
    }

    // (6) in_place_index ctor
    template<std::size_t I,
             typename... Args,
             typename T = typename internal::va_type_at<I, Types...>::type,
             typename = typename re_std::enable_if<
                 (I < sizeof...(Types)) &&
                 re_std::is_constructible<T, Args...>::value
             >::type>
    explicit variant(in_place_index_t<I>, Args&&... _args)
        : m_storage(), m_index(I)
    {
        typedef typename internal::va_type_at<I, Types...>::type _Type;
        new (static_cast<void*>(&internal::storage_at<I>::get(m_storage)))
            _Type(static_cast<Args&&>(_args)...);
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
        RE_STD_NOEXCEPT_IF(false /* simplified: not promising the noexcept */)
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
    template<typename U,
             typename = typename re_std::enable_if<
                 !re_std::is_same<typename re_std::decay<U>::type, variant>::value &&
                 (internal::best_match<U, Types...>::value
                    < sizeof...(Types))
             >::type>
    variant& operator=(U&& _u)
    {
        _destroy();
        static const std::size_t _idx =
            internal::best_match<U, Types...>::value;
        typedef typename internal::va_type_at<_idx, Types...>::type T;
        new (static_cast<void*>(&internal::storage_at<_idx>::get(m_storage)))
            T(static_cast<U&&>(_u));
        m_index = _idx;
        return *this;
    }

    // =================================================================
    // EMPLACE
    // =================================================================

    // emplace<T>(args...) — replaces with T constructed from args.
    template<typename T, typename... Args>
    T& emplace(Args&&... _args)
    {
        _destroy();
        static const std::size_t _idx = internal::index_of<T, Types...>::value;
        new (static_cast<void*>(&internal::storage_at<_idx>::get(m_storage)))
            T(static_cast<Args&&>(_args)...);
        m_index = _idx;
        return internal::storage_at<_idx>::get(m_storage);
    }

    // emplace<I>(args...) — replaces with the I-th alternative.
    template<std::size_t I, typename... Args>
    typename internal::va_type_at<I, Types...>::type&
    emplace(Args&&... _args)
    {
        _destroy();
        typedef typename internal::va_type_at<I, Types...>::type T;
        new (static_cast<void*>(&internal::storage_at<I>::get(m_storage)))
            T(static_cast<Args&&>(_args)...);
        m_index = I;
        return internal::storage_at<I>::get(m_storage);
    }

    // =================================================================
    // OBSERVERS
    // =================================================================

    std::size_t index() const RE_STD_NOEXCEPT { return m_index; }

    bool valueless_by_exception() const RE_STD_NOEXCEPT
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

    template<std::size_t I>
    typename internal::va_type_at<I, Types...>::type&
    _ref()
    {
        return internal::storage_at<I>::get(m_storage);
    }

    template<std::size_t I>
    typename internal::va_type_at<I, Types...>::type const&
    _ref() const
    {
        return internal::storage_at<I>::get(m_storage);
    }

private:

    // =================================================================
    // STORAGE
    // =================================================================

    internal::variant_storage<Types...>    m_storage;
    std::size_t                             m_index;

    // =================================================================
    // INTERNAL HELPERS — index-dispatched lifetime ops
    // =================================================================
    // We need to destroy / copy / move / swap the alternative at a
    // RUNTIME index. The dispatch is a compile-time unrolled if-else
    // chain over the index range [0, sizeof...(Types)).

    // Index sequence helper (avoids depending on full integer_sequence).
    template<std::size_t...> struct _idx_seq {};

    template<std::size_t N, std::size_t... Acc>
    struct _make_idx_seq : _make_idx_seq<N - 1, N - 1, Acc...> {};

    template<std::size_t... Acc>
    struct _make_idx_seq<0, Acc...>
    {
        typedef _idx_seq<Acc...> type;
    };

    typedef typename _make_idx_seq<sizeof...(Types)>::type _index_seq_type;
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

    template<std::size_t... Is>
    void _destroy_dispatch(std::size_t _i, _idx_seq<Is...>)
    {
        // Evaluate left-to-right; calls _destroy_one<I>() exactly once
        // for the matching I via an initializer-list expansion.
        // Cast to void array to discard the result and ensure ordering.
        using _expander = int[];
        (void)_expander{ 0, (_destroy_one<Is>(_i), 0)... };
    }

    template<std::size_t I>
    void _destroy_one(std::size_t _active)
    {
        if (_active == I)
        {
            typedef typename internal::va_type_at<I, Types...>::type T;
            internal::storage_at<I>::get(m_storage).~T();
        }
    }

    // Copy-construct from another variant's storage at the active index.
    template<std::size_t... Is>
    void _copy_construct_from(std::size_t _i,
                              internal::variant_storage<Types...> const& _src,
                              _idx_seq<Is...>)
    {
        using _expander = int[];
        (void)_expander{ 0, (_copy_one<Is>(_i, _src), 0)... };
    }

    template<std::size_t I>
    void _copy_one(std::size_t _active,
                   internal::variant_storage<Types...> const& _src)
    {
        if (_active == I)
        {
            typedef typename internal::va_type_at<I, Types...>::type T;
            new (static_cast<void*>(&internal::storage_at<I>::get(m_storage)))
                T(internal::storage_at<I>::get(_src));
        }
    }

    // Move-construct from another variant's storage at the active index.
    template<std::size_t... Is>
    void _move_construct_from(std::size_t _i,
                              internal::variant_storage<Types...>& _src,
                              _idx_seq<Is...>)
    {
        using _expander = int[];
        (void)_expander{ 0, (_move_one<Is>(_i, _src), 0)... };
    }

    template<std::size_t I>
    void _move_one(std::size_t _active,
                   internal::variant_storage<Types...>& _src)
    {
        if (_active == I)
        {
            typedef typename internal::va_type_at<I, Types...>::type T;
            new (static_cast<void*>(&internal::storage_at<I>::get(m_storage)))
                T(static_cast<T&&>(internal::storage_at<I>::get(_src)));
        }
    }

    // Swap when both variants hold the SAME alternative — ADL swap on
    // the held alternative.
    template<std::size_t... Is>
    void _swap_same_index(std::size_t _i,
                          internal::variant_storage<Types...>& _a,
                          internal::variant_storage<Types...>& _b,
                          _idx_seq<Is...>)
    {
        using _expander = int[];
        (void)_expander{ 0, (_swap_one<Is>(_i, _a, _b), 0)... };
    }

    template<std::size_t I>
    void _swap_one(std::size_t _active,
                   internal::variant_storage<Types...>& _a,
                   internal::variant_storage<Types...>& _b)
    {
        if (_active == I)
        {
            using std::swap;
            swap(internal::storage_at<I>::get(_a),
                 internal::storage_at<I>::get(_b));
        }
    }
};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_VARIANT_HPP
