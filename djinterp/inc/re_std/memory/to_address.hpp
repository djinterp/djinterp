/*******************************************************************************
* djinterp [re_std]                                               to_address.hpp
*
* obtain the raw address represented by a (possibly fancy) pointer.
*
* overloads:
*   to_address(T* _p)         -> T*       (raw pointer pass-through)
*   to_address(const Ptr& _p) -> auto      (fancy pointer)
*
* the fancy-pointer overload defers to pointer_traits<Ptr>::to_address
* when that member exists; otherwise falls back to to_address(p.operator->()).
* This matches the C++20 std::to_address contract on all tiers.
*
* design note:
*   to_address is unusual in that it is well-defined on a pointer that
*   does not point to a constructed object. It is the recommended way
*   to interoperate with allocator-traits returns (which may be fancy
*   pointers) without a dereference. Notably:
*
*       T* raw = re_std::to_address(allocator_traits<A>::allocate(a, 1));
*
*   is well-formed even though the storage is uninitialised.
*
* added in std C++20; re_std back-ports unconditionally to C++11+.
*
*
* path:      /inc/re_std/memory/to_address.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_TO_ADDRESS_HPP
#define RE_STD_MEMORY_TO_ADDRESS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/pointer_traits.hpp"
    #include "re_std/type_traits/is_function.hpp"


namespace re_std
{
// forward declaration -- internal::to_address_impl names re_std::to_address in
// a trailing return type, and that qualified-id is looked up where it is
// written rather than at instantiation, so it must be declared first.  Only
// the raw-pointer overload is needed: the impl applies it to p.operator->().
template<typename T>
RE_STD_CONSTEXPR T* to_address(T* _p) RE_STD_NOEXCEPT;

namespace internal
{

    // Detection: does pointer_traits<Ptr> have a static to_address?
    // We do not implement the detection trait fully here — instead, a
    // 2-overload tag-style approach via overload resolution works on
    // any C++11+ compiler.

    // Fallback path: use _p.operator->() recursively.
    template<typename Ptr>
    auto to_address_impl(const Ptr& _p, long /*tag*/) RE_STD_NOEXCEPT
        -> decltype(re_std::to_address(_p.operator->()));

    // Preferred path: use pointer_traits::to_address when present.
    // Detected via decltype substitution — if pointer_traits<Ptr>
    // exposes to_address, this overload is viable; otherwise it
    // SFINAEs out and the fallback wins.
    template<typename Ptr>
    auto to_address_impl(const Ptr& _p, int /*tag*/) RE_STD_NOEXCEPT
        -> decltype(pointer_traits<Ptr>::to_address(_p));

}  // internal
// Raw pointer overload.
template<typename T>
RE_STD_CONSTEXPR T* to_address(T* _p) RE_STD_NOEXCEPT
{
    // Function pointers are explicitly excluded by the standard.
    static_assert(!is_function<T>::value,
                  "re_std::to_address: function pointers are not allowed");
    return _p;
}


// Fancy pointer overload. Dispatches to pointer_traits::to_address
// when available, else to operator->.
//
// NOTE: pointer_traits::to_address is itself a C++20+ member; on
// pointer_traits implementations that lack it, the overload resolution
// falls through to the operator->() path, which is the C++17 std
// behaviour for fancy pointers.
template<typename Ptr>
auto to_address(const Ptr& _p) RE_STD_NOEXCEPT
    -> decltype(internal::to_address_impl(_p, 0))
{
    return internal::to_address_impl(_p, 0);
}


namespace internal
{

    // Definitions of the implementation overloads (after to_address is
    // declared, to permit recursion through the fallback path).
    template<typename Ptr>
    auto to_address_impl(const Ptr& _p, long) RE_STD_NOEXCEPT
        -> decltype(re_std::to_address(_p.operator->()))
    {
        return re_std::to_address(_p.operator->());
    }

    template<typename Ptr>
    auto to_address_impl(const Ptr& _p, int) RE_STD_NOEXCEPT
        -> decltype(pointer_traits<Ptr>::to_address(_p))
    {
        return pointer_traits<Ptr>::to_address(_p);
    }

}  // internal
}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_TO_ADDRESS_HPP
