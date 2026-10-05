/*******************************************************************************
* djinterp [re_std]                                        destroying_delete.hpp
*
* destroying_delete_t tag header:
*   C++20 destroying_delete_t is a tag type used to mark class-scope
* operator delete overloads as destroying-delete:
*
*     struct widget {
*       void operator delete(widget* p, std::destroying_delete_t);
*     };
*
*   When this overload is present, the compiler invokes it INSTEAD
* of the destructor — the operator is responsible for running the
* destructor itself. This enables advanced patterns like reverse-
* destruction in custom allocators.
*
*   STRATEGY:
*     C++20+: using-declaration from std::destroying_delete_t.
*     C++11 - C++17: back-port as struct + named instance. The
*                    operator-delete dispatch is COMPILER-DRIVEN,
*                    so the back-port type cannot trigger the
*                    actual destroying-delete behavior — but it
*                    lets user code write the signature so it
*                    compiles cleanly on older tiers.
*     C++98: not provided. struct definition needs constexpr default
*            ctor for the named instance pattern; constexpr is C++11.
*
*
* path:      /inc/re_std/new/destroying_delete.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_NEW_DESTROYING_DELETE_HPP
#define RE_STD_NEW_DESTROYING_DELETE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

// std
#if RE_STD_LANG_IS_CPP20_OR_HIGHER
    #include <new>  // std::destroying_delete_t, std::destroying_delete
#endif

// gate: requires C++11 minimum for the constexpr-instance pattern.
#if RE_STD_LANG_IS_CPP11_OR_HIGHER


namespace re_std
{
// ===========================================================================
// I.   DESTROYING_DELETE_T
// ===========================================================================

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

using std::destroying_delete_t;
using std::destroying_delete;

#else

// Back-port. The struct has an explicit constexpr default ctor —
// matches std's defensive design (prevents construction from {} in
// ambiguous contexts).
struct destroying_delete_t
{
    explicit constexpr destroying_delete_t() {}
};

#if RE_STD_LANG_HAS_INLINE_VARIABLES
inline constexpr destroying_delete_t destroying_delete{};
#else
// Pre-C++17: holder-template pattern. Same trick used for
// re_std::unexpect and re_std::in_place.
namespace internal
{

    template<typename Dummy>
    struct destroying_delete_holder
    {
        static const destroying_delete_t value;
    };
    template<typename Dummy>
    const destroying_delete_t destroying_delete_holder<Dummy>::value
        = destroying_delete_t();
}
static const destroying_delete_t& destroying_delete
    = internal::destroying_delete_holder<void>::value;
#endif

#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_NEW_DESTROYING_DELETE_HPP
