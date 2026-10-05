/*******************************************************************************
* djinterp [re_std]                                               owner_less.hpp
*
* owner_less class header:
* control-block-based strict-weak ordering predicate for shared_ptr
* and weak_ptr. Use as the comparator in associative containers when
* you want pointers that share ownership to compare equal even if
* their element pointers differ (e.g. via the aliasing constructor).
*
* layout:
*   The primary template `owner_less<T>` is intentionally undefined
*   (matches std). Specialisations are provided for:
*     owner_less<shared_ptr<T>>
*     owner_less<weak_ptr<T>>
*     owner_less<void>           (heterogeneous; std added in C++17,
*                                 re_std back-ports unconditionally)
*
*   Each specialisation has comparison operators for both the matching
*   pointer kind and the cross kind, so that std::set<weak_ptr<T>,
*   owner_less<weak_ptr<T>>>::find() can be called with a shared_ptr
*   key.
*
* the void specialisation:
*   `is_transparent` typedef is the C++14 std-container convention
*   that lets find / count / equal_range accept any key type, not
*   just the container's. Templating operator() on both arguments
*   does the rest.
*
*
* path:      /inc/re_std/memory/owner_less.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_OWNER_LESS_HPP
#define RE_STD_MEMORY_OWNER_LESS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/memory/weak_ptr.hpp"


namespace re_std
{

// Primary template: intentionally undefined. Matches std.
template<typename T = void>
struct owner_less;


// owner_less<shared_ptr<T>>
template<typename T>
struct owner_less<shared_ptr<T> >
{
    bool operator()(const shared_ptr<T>& _a,
                    const shared_ptr<T>& _b) const RE_STD_NOEXCEPT
    {
        return _a.owner_before(_b);
    }

    bool operator()(const shared_ptr<T>& _a,
                    const weak_ptr<T>&   _b) const RE_STD_NOEXCEPT
    {
        return _a.owner_before(_b);
    }

    bool operator()(const weak_ptr<T>&   _a,
                    const shared_ptr<T>& _b) const RE_STD_NOEXCEPT
    {
        return _a.owner_before(_b);
    }
};


// owner_less<weak_ptr<T>>
template<typename T>
struct owner_less<weak_ptr<T> >
{
    bool operator()(const weak_ptr<T>&   _a,
                    const weak_ptr<T>&   _b) const RE_STD_NOEXCEPT
    {
        return _a.owner_before(_b);
    }

    bool operator()(const shared_ptr<T>& _a,
                    const weak_ptr<T>&   _b) const RE_STD_NOEXCEPT
    {
        return _a.owner_before(_b);
    }

    bool operator()(const weak_ptr<T>&   _a,
                    const shared_ptr<T>& _b) const RE_STD_NOEXCEPT
    {
        return _a.owner_before(_b);
    }
};


// owner_less<void>  -  heterogeneous (transparent)
template<>
struct owner_less<void>
{
    typedef void is_transparent;

    template<typename A, typename B>
    bool operator()(const A& _a, const B& _b) const RE_STD_NOEXCEPT
    {
        return _a.owner_before(_b);
    }
};


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_OWNER_LESS_HPP
