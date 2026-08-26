/******************************************************************************
* djinterp [re_std]                                         ranges_access.hpp
*
* ranges access CPO header:
*   Provides the C++20 ranges::begin / end / cbegin / cend / rbegin /
* rend / crbegin / crend / size / ssize / empty / data / cdata
* customisation point objects (niebloids). All 13 live in the
* nested namespace re_std::ranges:: to avoid colliding with the
* existing flat-namespace re_std::begin et al. ordinary-template
* overloads shipped in <iterator>.
*
*   FILE ORGANISATION:
*   This is a single-file batch for all 13 CPOs — a deliberate
* deviation from re_std's usual one-file-per-symbol convention. The
* CPOs share design heavily (priority-based dispatch with poison-
* pill ADL discovery) and the cross-references between them (cbegin
* uses begin; ssize uses size; empty uses size and begin/end; data
* uses begin) would otherwise force ~13 small files to all
* #include one another.
*
*   DISPATCH PATTERN (per CPO):
*   Each ranges:: CPO is a function-object whose operator() routes
* through a chain of priority-tagged _impl overloads:
*   priority<3>  — array specialisation (when applicable)
*   priority<2>  — member function call (r.begin() / r.size() / ...)
*   priority<1>  — ADL-found free function (begin(r) / size(r) / ...)
*   priority<0>  — computed fallback (e.g. ranges::size from end - begin)
*
*   The dispatcher namespace declares a deleted poison-pill version
* of the looked-up name (e.g. void begin() = delete;) so that the
* unqualified call inside priority<1> uses ADL augmentation rather
* than recursing into the CPO itself.
*
*   PORTABILITY:
*   - C++11+; uses trailing-return-types throughout.
*   - C++17+ uses inline-constexpr CPO instances; C++11/14 use
*     static-constexpr instances (same pattern as iter_move in R22).
*
*
* path:      /inc/djinterp/re_std/ranges/ranges_access.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_RANGES_ACCESS_
#define DJINTERP_RE_STD_RANGES_RANGES_ACCESS_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>  // std::size_t, std::ptrdiff_t

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../iterator/reverse_iterator.hpp"
#include "../iterator/basic_const_iterator.hpp"


NS_RESTD


namespace ranges
{


// ===========================================================================
// I.   PRIORITY HIERARCHY (shared)
// ===========================================================================

namespace internal
{
    template<int _N>
    struct priority : priority<_N - 1>
    {};

    template<>
    struct priority<0>
    {};
}


// ===========================================================================
// II.  RANGES::BEGIN
// ===========================================================================

namespace _begin_fn
{
    void begin() = delete;  // poison pill — ensures the unqualified
                            // begin(r) call below uses ADL augmentation
                            // and does NOT pick up the ranges::begin
                            // CPO itself (which lives in an outer
                            // namespace).

    // priority<3>: array specialisation. Returns _T*.
    template<typename _T, std::size_t _N>
    D_CONSTEXPR_INLINE _T*
    _impl(
        _T (&_arr)[_N],
        internal::priority<3>
    )
    D_NOEXCEPT
    {
        return _arr + 0;
    }

    // priority<2>: member function.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<2>
    )
        -> decltype(static_cast<_R&&>(_r).begin())
    {
        return static_cast<_R&&>(_r).begin();
    }

    // priority<1>: ADL free function.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<1>
    )
        -> decltype(begin(static_cast<_R&&>(_r)))
    {
        return begin(static_cast<_R&&>(_r));
    }

    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> decltype(_impl(static_cast<_R&&>(_r), internal::priority<3>()))
        {
            return _impl(static_cast<_R&&>(_r), internal::priority<3>());
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _begin_fn::fn begin = _begin_fn::fn();
#else
static D_CONSTEXPR _begin_fn::fn begin = _begin_fn::fn();
#endif


// ===========================================================================
// III. RANGES::END
// ===========================================================================

namespace _end_fn
{
    void end() = delete;

    // priority<3>: array.
    template<typename _T, std::size_t _N>
    D_CONSTEXPR_INLINE _T*
    _impl(
        _T (&_arr)[_N],
        internal::priority<3>
    )
    D_NOEXCEPT
    {
        return _arr + _N;
    }

    // priority<2>: member function.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<2>
    )
        -> decltype(static_cast<_R&&>(_r).end())
    {
        return static_cast<_R&&>(_r).end();
    }

    // priority<1>: ADL.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<1>
    )
        -> decltype(end(static_cast<_R&&>(_r)))
    {
        return end(static_cast<_R&&>(_r));
    }

    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> decltype(_impl(static_cast<_R&&>(_r), internal::priority<3>()))
        {
            return _impl(static_cast<_R&&>(_r), internal::priority<3>());
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _end_fn::fn end = _end_fn::fn();
#else
static D_CONSTEXPR _end_fn::fn end = _end_fn::fn();
#endif


// ===========================================================================
// IV.  RANGES::SIZE
// ===========================================================================

namespace _size_fn
{
    void size() = delete;

    // priority<3>: array.
    template<typename _T, std::size_t _N>
    D_CONSTEXPR_INLINE std::size_t
    _impl(
        _T (&)[_N],
        internal::priority<3>
    )
    D_NOEXCEPT
    {
        return _N;
    }

    // priority<2>: member function.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<2>
    )
        -> decltype(static_cast<_R&&>(_r).size())
    {
        return static_cast<_R&&>(_r).size();
    }

    // priority<1>: ADL.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<1>
    )
        -> decltype(size(static_cast<_R&&>(_r)))
    {
        return size(static_cast<_R&&>(_r));
    }

    // priority<0>: derived from end - begin (random-access common range).
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<0>
    )
        -> decltype(ranges::end(_r) - ranges::begin(_r))
    {
        return ranges::end(_r) - ranges::begin(_r);
    }

    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> decltype(_impl(static_cast<_R&&>(_r), internal::priority<3>()))
        {
            return _impl(static_cast<_R&&>(_r), internal::priority<3>());
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _size_fn::fn size = _size_fn::fn();
#else
static D_CONSTEXPR _size_fn::fn size = _size_fn::fn();
#endif


// ===========================================================================
// V.   RANGES::SSIGNED-SIZE (ssize)
// ===========================================================================

namespace _ssize_fn
{
    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> typename make_signed<
                            typename decay<decltype(ranges::size(_r))>::type
                        >::type
        {
            typedef typename make_signed<
                                  typename decay<decltype(ranges::size(_r))>::type
                              >::type signed_size_t;
            return static_cast<signed_size_t>(ranges::size(_r));
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _ssize_fn::fn ssize = _ssize_fn::fn();
#else
static D_CONSTEXPR _ssize_fn::fn ssize = _ssize_fn::fn();
#endif


// ===========================================================================
// VI.  RANGES::EMPTY
// ===========================================================================

namespace _empty_fn
{
    // priority<3>: member function r.empty().
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<3>
    )
        -> decltype(static_cast<bool>(static_cast<_R&&>(_r).empty()))
    {
        return static_cast<bool>(static_cast<_R&&>(_r).empty());
    }

    // priority<2>: ranges::size(r) == 0.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<2>
    )
        -> decltype(ranges::size(_r) == 0)
    {
        return ranges::size(_r) == 0;
    }

    // priority<1>: ranges::begin(r) == ranges::end(r). For common
    // ranges (where begin and end share a type).
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<1>
    )
        -> decltype(static_cast<bool>(ranges::begin(_r) == ranges::end(_r)))
    {
        return static_cast<bool>(ranges::begin(_r) == ranges::end(_r));
    }

    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> decltype(_impl(static_cast<_R&&>(_r), internal::priority<3>()))
        {
            return _impl(static_cast<_R&&>(_r), internal::priority<3>());
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _empty_fn::fn empty = _empty_fn::fn();
#else
static D_CONSTEXPR _empty_fn::fn empty = _empty_fn::fn();
#endif


// ===========================================================================
// VII. RANGES::DATA
// ===========================================================================

namespace _data_fn
{
    // priority<3>: array.
    template<typename _T, std::size_t _N>
    D_CONSTEXPR_INLINE _T*
    _impl(
        _T (&_arr)[_N],
        internal::priority<3>
    )
    D_NOEXCEPT
    {
        return _arr + 0;
    }

    // priority<2>: member function r.data().
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<2>
    )
        -> decltype(static_cast<_R&&>(_r).data())
    {
        return static_cast<_R&&>(_r).data();
    }

    // priority<1>: address of *begin (contiguous range fallback).
    // Strictly speaking C++20 uses to_address; for re_std this works
    // when begin yields a pointer or pointer-like iterator.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<1>
    )
        -> decltype(&(*ranges::begin(_r)))
    {
        return &(*ranges::begin(_r));
    }

    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> decltype(_impl(static_cast<_R&&>(_r), internal::priority<3>()))
        {
            return _impl(static_cast<_R&&>(_r), internal::priority<3>());
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _data_fn::fn data = _data_fn::fn();
#else
static D_CONSTEXPR _data_fn::fn data = _data_fn::fn();
#endif


// ===========================================================================
// VIII. RANGES::CBEGIN  /  RANGES::CEND
// ===========================================================================

namespace _cbegin_fn
{
    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> basic_const_iterator<
                  typename decay<decltype(ranges::begin(_r))>::type
               >
        {
            typedef basic_const_iterator<
                        typename decay<decltype(ranges::begin(_r))>::type
                    > result_t;
            return result_t(ranges::begin(_r));
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _cbegin_fn::fn cbegin = _cbegin_fn::fn();
#else
static D_CONSTEXPR _cbegin_fn::fn cbegin = _cbegin_fn::fn();
#endif


namespace _cend_fn
{
    // For common ranges (begin and end share a type), wrap end in
    // basic_const_iterator. For non-common, return the underlying
    // sentinel unchanged — basic_const_iterator's cross-comparison
    // (R22) handles the asymmetric compare.

    // priority<2>: same type as begin → wrap in basic_const_iterator.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<2>
    )
        -> typename enable_if<
                is_same<
                    typename decay<decltype(ranges::begin(_r))>::type,
                    typename decay<decltype(ranges::end(_r))>::type
                >::value,
                basic_const_iterator<
                    typename decay<decltype(ranges::end(_r))>::type
                >
            >::type
    {
        typedef basic_const_iterator<
                    typename decay<decltype(ranges::end(_r))>::type
                > result_t;
        return result_t(ranges::end(_r));
    }

    // priority<1>: non-common → pass sentinel through.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<1>
    )
        -> typename enable_if<
                !is_same<
                    typename decay<decltype(ranges::begin(_r))>::type,
                    typename decay<decltype(ranges::end(_r))>::type
                >::value,
                typename decay<decltype(ranges::end(_r))>::type
            >::type
    {
        return ranges::end(_r);
    }

    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> decltype(_impl(static_cast<_R&&>(_r), internal::priority<2>()))
        {
            return _impl(static_cast<_R&&>(_r), internal::priority<2>());
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _cend_fn::fn cend = _cend_fn::fn();
#else
static D_CONSTEXPR _cend_fn::fn cend = _cend_fn::fn();
#endif


// ===========================================================================
// IX.  RANGES::RBEGIN  /  RANGES::REND
// ===========================================================================

namespace _rbegin_fn
{
    void rbegin() = delete;

    // priority<3>: member function.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<3>
    )
        -> decltype(static_cast<_R&&>(_r).rbegin())
    {
        return static_cast<_R&&>(_r).rbegin();
    }

    // priority<2>: ADL.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<2>
    )
        -> decltype(rbegin(static_cast<_R&&>(_r)))
    {
        return rbegin(static_cast<_R&&>(_r));
    }

    // priority<1>: make_reverse_iterator(ranges::end(r)).
    // Requires the underlying to be a common bidirectional range.
    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<1>
    )
        -> reverse_iterator<typename decay<decltype(ranges::end(_r))>::type>
    {
        typedef reverse_iterator<
                    typename decay<decltype(ranges::end(_r))>::type
                > rev_t;
        return rev_t(ranges::end(_r));
    }

    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> decltype(_impl(static_cast<_R&&>(_r), internal::priority<3>()))
        {
            return _impl(static_cast<_R&&>(_r), internal::priority<3>());
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _rbegin_fn::fn rbegin = _rbegin_fn::fn();
#else
static D_CONSTEXPR _rbegin_fn::fn rbegin = _rbegin_fn::fn();
#endif


namespace _rend_fn
{
    void rend() = delete;

    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<3>
    )
        -> decltype(static_cast<_R&&>(_r).rend())
    {
        return static_cast<_R&&>(_r).rend();
    }

    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<2>
    )
        -> decltype(rend(static_cast<_R&&>(_r)))
    {
        return rend(static_cast<_R&&>(_r));
    }

    template<typename _R>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _R&&             _r,
        internal::priority<1>
    )
        -> reverse_iterator<typename decay<decltype(ranges::begin(_r))>::type>
    {
        typedef reverse_iterator<
                    typename decay<decltype(ranges::begin(_r))>::type
                > rev_t;
        return rev_t(ranges::begin(_r));
    }

    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> decltype(_impl(static_cast<_R&&>(_r), internal::priority<3>()))
        {
            return _impl(static_cast<_R&&>(_r), internal::priority<3>());
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _rend_fn::fn rend = _rend_fn::fn();
#else
static D_CONSTEXPR _rend_fn::fn rend = _rend_fn::fn();
#endif


// ===========================================================================
// X.   RANGES::CRBEGIN  /  RANGES::CREND  /  RANGES::CDATA
// ===========================================================================

namespace _crbegin_fn
{
    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> basic_const_iterator<
                   typename decay<decltype(ranges::rbegin(_r))>::type
               >
        {
            typedef basic_const_iterator<
                        typename decay<decltype(ranges::rbegin(_r))>::type
                    > result_t;
            return result_t(ranges::rbegin(_r));
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _crbegin_fn::fn crbegin = _crbegin_fn::fn();
#else
static D_CONSTEXPR _crbegin_fn::fn crbegin = _crbegin_fn::fn();
#endif


namespace _crend_fn
{
    struct fn
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> basic_const_iterator<
                   typename decay<decltype(ranges::rend(_r))>::type
               >
        {
            typedef basic_const_iterator<
                        typename decay<decltype(ranges::rend(_r))>::type
                    > result_t;
            return result_t(ranges::rend(_r));
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _crend_fn::fn crend = _crend_fn::fn();
#else
static D_CONSTEXPR _crend_fn::fn crend = _crend_fn::fn();
#endif


namespace _cdata_fn
{
    struct fn
    {
        // ranges::cdata(r) yields a pointer-to-const-element.
        template<typename _R>
        D_CONSTEXPR_INLINE
        auto
        operator()(_R&& _r) const
            -> typename add_pointer<
                   typename add_const<
                       typename remove_pointer<
                           typename decay<decltype(ranges::data(_r))>::type
                       >::type
                   >::type
               >::type
        {
            typedef typename add_pointer<
                                  typename add_const<
                                      typename remove_pointer<
                                          typename decay<decltype(ranges::data(_r))>::type
                                      >::type
                                  >::type
                              >::type result_t;
            return static_cast<result_t>(ranges::data(_r));
        }
    };
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR _cdata_fn::fn cdata = _cdata_fn::fn();
#else
static D_CONSTEXPR _cdata_fn::fn cdata = _cdata_fn::fn();
#endif


}  // namespace ranges


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RANGES_RANGES_ACCESS_
