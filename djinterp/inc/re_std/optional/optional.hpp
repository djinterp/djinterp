/******************************************************************************
* re_std [optional]                                               optional.hpp
*
*   optional<T> - a value that may or may not be there, with no allocation and
* no sentinel value stolen from T's range.
*
*   STD IS C++17; re_std IS C++11.
*   A six-year back-port, and constexpr six years early with it: because
* storage is a union rather than a byte buffer, construction and observation
* are constant expressions from C++11.
*
*   TRIVIALITY IS INHERITED, NOT DISCARDED.
*   This class declares NO copy constructor, move constructor, copy assignment
* or move assignment.  All four are implicitly defaulted so their triviality
* falls through from internal::optional_base - see optional_base.hpp for why
* that needs two layers of base class.  The observable result:
*
*     is_trivially_destructible<optional<int>>  ->  true
*     is_trivially_copyable<optional<int>>      ->  true
*     constexpr optional<int> b = a;            ->  compiles
*
*   Do not add a user-provided copy or move member to this class.  Doing so
* silently costs every T all three of the above.
*
*   CONSTEXPR TIERS.
*     C++11  default / nullopt / in_place / value construction; has_value,
*            operator bool, operator->, operator*, value(), value_or() on
*            const lvalues; copy of a trivially-copyable T.
*     C++14  the non-const observers, which need relaxed constexpr.
*     never  emplace(), reset() on an engaged optional, and swap(): these end
*            or begin a lifetime through placement new, which is not a constant
*            expression.  std reaches them at C++20 via construct_at; doing the
*            same here is a tracked follow-up rather than a silent difference.
*
*   THE value() / operator* DISTINCTION IS NOT COSMETIC.
*   value() throws bad_optional_access when disengaged; operator* and
* operator-> are UNDEFINED when disengaged and do not check.  That is std's
* design and re_std keeps it: operator* is the one used in loops where the
* engagement has already been tested, and a hidden branch there would be paid
* on every iteration.
*
*
* path:      /inc/djinterp/re_std/optional/optional.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_OPTIONAL_OPTIONAL_
#define DJINTERP_RE_STD_OPTIONAL_OPTIONAL_ 1

// re_std
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "./optional_base.hpp"
#include "./nullopt.hpp"
#include "./bad_optional_access.hpp"

NS_RESTD

// optional
//   class: holds either a value of _Type or nothing.
template<typename _Type>
class optional : private internal::optional_base<_Type>
{
    typedef internal::optional_base<_Type> _Base;

    //   Guard for the perfect-forwarding value constructor.  Without it that
    // template is a better match than the copy constructor for a non-const
    // optional lvalue, and optional<T> b(a) would try to build a T from an
    // optional<T>.  This is the classic greedy-forwarding-constructor trap.
    template<typename _Other>
    struct allows_value_ctor
    {
        typedef typename remove_cv<
            typename remove_reference<_Other>::type>::type _Bare;
        static const bool value =
               !is_same<_Bare, optional>::value
            && !is_same<_Bare, in_place_t>::value
            && !is_same<_Bare, nullopt_t>::value
            &&  is_constructible<_Type, _Other&&>::value;
    };

public:
    typedef _Type value_type;

    // ---- construction ------------------------------------------------
    D_CONSTEXPR optional() D_NOEXCEPT {}

    D_CONSTEXPR optional(nullopt_t) D_NOEXCEPT {}

    //   Copy / move constructors and assignments are DELIBERATELY not
    // declared; see the header note.

    template<typename... _Args>
    D_CONSTEXPR explicit optional(in_place_t, _Args&&... args)
        : _Base(in_place, static_cast<_Args&&>(args)...)
    {}

    template<typename _Other = _Type,
             typename enable_if<allows_value_ctor<_Other>::value, int>::type = 0>
    D_CONSTEXPR optional(_Other&& value)
        : _Base(in_place, static_cast<_Other&&>(value))
    {}

    // ---- assignment --------------------------------------------------
    optional& operator=(nullopt_t) D_NOEXCEPT
    {
        this->destroy();
        return *this;
    }

    template<typename _Other = _Type,
             typename enable_if<allows_value_ctor<_Other>::value, int>::type = 0>
    optional& operator=(_Other&& value)
    {
        if (this->m_engaged)
        {
            this->m_value = static_cast<_Other&&>(value);
        }
        else
        {
            this->construct(static_cast<_Other&&>(value));
        }
        return *this;
    }

    // ---- observers ---------------------------------------------------
    D_CONSTEXPR bool has_value() const D_NOEXCEPT { return this->m_engaged; }

    D_CONSTEXPR explicit operator bool() const D_NOEXCEPT
    { return this->m_engaged; }

    //   Unchecked. Undefined when disengaged - see the header note.
    D_CONSTEXPR const _Type* operator->() const
    { return re_std::addressof(this->m_value); }

    D_CONSTEXPR_CPP14 _Type* operator->()
    { return re_std::addressof(this->m_value); }

    D_CONSTEXPR const _Type&  operator*() const&  { return this->m_value; }
    D_CONSTEXPR_CPP14 _Type&  operator*() &       { return this->m_value; }

    D_CONSTEXPR_CPP14 _Type&& operator*() &&
    { return static_cast<_Type&&>(this->m_value); }

    D_CONSTEXPR const _Type&& operator*() const&&
    { return static_cast<const _Type&&>(this->m_value); }

    //   Checked. Throws bad_optional_access when disengaged.
    D_CONSTEXPR const _Type& value() const&
    {
        return this->m_engaged
                   ? this->m_value
                   : (internal::throw_bad_optional_access(), this->m_value);
    }

    D_CONSTEXPR_CPP14 _Type& value() &
    {
        if (!this->m_engaged) { internal::throw_bad_optional_access(); }
        return this->m_value;
    }

    D_CONSTEXPR_CPP14 _Type&& value() &&
    {
        if (!this->m_engaged) { internal::throw_bad_optional_access(); }
        return static_cast<_Type&&>(this->m_value);
    }

    template<typename _Other>
    D_CONSTEXPR _Type value_or(_Other&& fallback) const&
    {
        return this->m_engaged
                   ? this->m_value
                   : static_cast<_Type>(static_cast<_Other&&>(fallback));
    }

    template<typename _Other>
    D_CONSTEXPR_CPP14 _Type value_or(_Other&& fallback) &&
    {
        return this->m_engaged
                   ? static_cast<_Type&&>(this->m_value)
                   : static_cast<_Type>(static_cast<_Other&&>(fallback));
    }

    // ---- modifiers ---------------------------------------------------
    void reset() D_NOEXCEPT
    {
        this->destroy();
        return;
    }

    template<typename... _Args>
    _Type& emplace(_Args&&... args)
    {
        //   Destroy first: emplace is specified to replace whatever is there,
        // and constructing over a live object would leak it.
        this->destroy();
        this->construct(static_cast<_Args&&>(args)...);
        return this->m_value;
    }

    void swap(optional& other)
        D_NOEXCEPT_IF(   is_nothrow_move_constructible<_Type>::value
                      && is_nothrow_swappable<_Type>::value)
    {
        if (this->m_engaged && other.m_engaged)
        {
            re_std::swap(this->m_value, other.m_value);
        }
        else if (this->m_engaged)
        {
            other.construct(static_cast<_Type&&>(this->m_value));
            this->destroy();
        }
        else if (other.m_engaged)
        {
            this->construct(static_cast<_Type&&>(other.m_value));
            other.destroy();
        }
        return;
    }
};

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_OPTIONAL_OPTIONAL_
