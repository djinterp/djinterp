/***********************************************************************
* restd                                                       to_address.hpp
*
* obtain the raw address represented by a (possibly fancy) pointer.
*
* overloads:
*   to_address(_T* _p)         -> _T*       (raw pointer pass-through)
*   to_address(const _Ptr& _p) -> auto      (fancy pointer)
*
* the fancy-pointer overload defers to pointer_traits<_Ptr>::to_address
* when that member exists; otherwise falls back to to_address(p.operator->()).
* This matches the C++20 std::to_address contract on all tiers.
*
* design note:
*   to_address is unusual in that it is well-defined on a pointer that
*   does not point to a constructed object. It is the recommended way
*   to interoperate with allocator-traits returns (which may be fancy
*   pointers) without a dereference. Notably:
*
*       T* raw = restd::to_address(allocator_traits<A>::allocate(a, 1));
*
*   is well-formed even though the storage is uninitialised.
*
* added in std C++20; restd back-ports unconditionally to C++11+.
*
*
* path:      /inc/restd/memory/to_address.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_TO_ADDRESS_
#define RESTD_MEMORY_TO_ADDRESS_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/memory/pointer_traits.hpp"
    #include "restd/type_traits/is_function.hpp"


namespace restd
{
namespace internal
{

    // Detection: does pointer_traits<_Ptr> have a static to_address?
    // We do not implement the detection trait fully here — instead, a
    // 2-overload tag-style approach via overload resolution works on
    // any C++11+ compiler.

    // Fallback path: use _p.operator->() recursively.
    template<typename _Ptr>
    auto to_address_impl(const _Ptr& _p, long /*tag*/) D_NOEXCEPT
        -> decltype(restd::to_address(_p.operator->()));

    // Preferred path: use pointer_traits::to_address when present.
    // Detected via decltype substitution — if pointer_traits<_Ptr>
    // exposes to_address, this overload is viable; otherwise it
    // SFINAEs out and the fallback wins.
    template<typename _Ptr>
    auto to_address_impl(const _Ptr& _p, int /*tag*/) D_NOEXCEPT
        -> decltype(pointer_traits<_Ptr>::to_address(_p));

}  // namespace internal


// Raw pointer overload.
template<typename _T>
D_CONSTEXPR _T* to_address(_T* _p) D_NOEXCEPT
{
    // Function pointers are explicitly excluded by the standard.
    static_assert(!is_function<_T>::value,
                  "restd::to_address: function pointers are not allowed");
    return _p;
}


// Fancy pointer overload. Dispatches to pointer_traits::to_address
// when available, else to operator->.
//
// NOTE: pointer_traits::to_address is itself a C++20+ member; on
// pointer_traits implementations that lack it, the overload resolution
// falls through to the operator->() path, which is the C++17 std
// behaviour for fancy pointers.
template<typename _Ptr>
auto to_address(const _Ptr& _p) D_NOEXCEPT
    -> decltype(internal::to_address_impl(_p, 0))
{
    return internal::to_address_impl(_p, 0);
}


namespace internal
{

    // Definitions of the implementation overloads (after to_address is
    // declared, to permit recursion through the fallback path).
    template<typename _Ptr>
    auto to_address_impl(const _Ptr& _p, long) D_NOEXCEPT
        -> decltype(restd::to_address(_p.operator->()))
    {
        return restd::to_address(_p.operator->());
    }

    template<typename _Ptr>
    auto to_address_impl(const _Ptr& _p, int) D_NOEXCEPT
        -> decltype(pointer_traits<_Ptr>::to_address(_p))
    {
        return pointer_traits<_Ptr>::to_address(_p);
    }

}  // namespace internal


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_TO_ADDRESS_
