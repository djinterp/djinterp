/***********************************************************************
* restd                                                   type_index.hpp
*
* class type_index:
*   Copyable, assignable wrapper around a std::type_info reference, so a
* type's identity can be used as a key in associative and unordered
* containers. Mirrors std::type_index (C++11). Holds a const type_info*
* (never null — there is no default ctor, matching std), so the type is
* copyable and assignable where a bare type_info is not.
*
*   restd has no <typeinfo> re-export module; type_index uses std::type_info
* directly behind D_ENV_CPP98_HAS_TYPEINFO (the same RTTI gate
* any/bad_any_cast uses). The whole class is unavailable when RTTI /
* <typeinfo> is absent — type_index is meaningless without it.
*
*   Tiered surface: the six relational operators ship as explicit members
* on C++11–C++17; on C++20 they are replaced by operator== plus
* operator<=> (returning restd::strong_ordering, from the shipped
* restd::compare), with the compiler synthesising !=, <, <=, >, >= — the
* explicit legacy four are gated out there to avoid redundant/ambiguous
* candidates. Not constexpr on any tier: type_info::before / hash_code /
* name are not constant expressions (true of std::type_index too).
*
*
* path:      /inc/restd/typeindex/type_index.hpp
* link(s):   TBA
* author(s): restd contributors                        date: 2026.06.05
***********************************************************************/

#ifndef RESTD_TYPEINDEX_TYPE_INDEX_
#define RESTD_TYPEINDEX_TYPE_INDEX_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#if D_ENV_CPP98_HAS_TYPEINFO

#include <typeinfo>  // std::type_info
#include <cstddef>   // size_t

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
#include "restd/compare/strong_ordering.hpp"  // strong_ordering (C++20 op<=>)
#endif

namespace restd
{

    // type_index
    //   class: lightweight, copyable handle to a type's std::type_info.
    class type_index
    {
    public:

        // type_index(const type_info&)
        //   function: implicit ctor (matches std — allows
        //   `type_index ti = typeid(T);`). Stores the address of the
        //   referenced type_info, which has static storage duration.
        type_index(const std::type_info& _tinfo) noexcept
            : m_target(&_tinfo)
        {}

        // operator==
        //   function: equal iff the two type_infos denote the same type.
        bool
        operator==(const type_index& _rhs) const noexcept
        { return *m_target == *_rhs.m_target; }

#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
        // legacy relational operators (C++11–C++17). On C++20 these are
        // synthesised from operator<=> / operator==, so they are gated out
        // to avoid redundant explicit candidates.
        bool
        operator!=(const type_index& _rhs) const noexcept
        { return *m_target != *_rhs.m_target; }

        bool
        operator<(const type_index& _rhs) const noexcept
        { return m_target->before(*_rhs.m_target) != 0; }

        bool
        operator>(const type_index& _rhs) const noexcept
        { return _rhs.m_target->before(*m_target) != 0; }

        bool
        operator<=(const type_index& _rhs) const noexcept
        { return _rhs.m_target->before(*m_target) == 0; }

        bool
        operator>=(const type_index& _rhs) const noexcept
        { return m_target->before(*_rhs.m_target) == 0; }
#endif  // !C++20

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        // operator<=>
        //   function: three-way comparison (C++20). Returns
        //   restd::strong_ordering, built from type_info::before. The
        //   compiler rewrites <, <=, >, >= from this and != from ==.
        restd::strong_ordering
        operator<=>(const type_index& _rhs) const noexcept
        {
            if (*m_target == *_rhs.m_target)
                return restd::strong_ordering::equal;
            if (m_target->before(*_rhs.m_target) != 0)
                return restd::strong_ordering::less;
            return restd::strong_ordering::greater;
        }
#endif  // C++20

        // hash_code
        //   function: forwards to type_info::hash_code() (C++11). Equal for
        //   type_indexes denoting the same type; the key used by
        //   hash<type_index>.
        std::size_t
        hash_code() const noexcept
        { return m_target->hash_code(); }

        // name
        //   function: implementation-defined type name (forwards to
        //   type_info::name()).
        const char*
        name() const noexcept
        { return m_target->name(); }

    private:

        const std::type_info* m_target;
    };

}  // namespace restd

#endif  // D_ENV_CPP98_HAS_TYPEINFO
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_TYPEINDEX_TYPE_INDEX_
