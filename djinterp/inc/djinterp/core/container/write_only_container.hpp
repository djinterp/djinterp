/******************************************************************************
* djinterp [container]                                  write_only_container.hpp
*
*   The write_only access wrapper: an append-only sink.  It HOLDS its container
* privately and forwards only the append surface - push_back / emplace_back /
* clear - together with the VALUE-FREE metadata size() and empty(), which reveal
* how many elements are present but never an element's value.  It exposes no way
* to read an element (no operator[], data, iteration, front or back) and no
* position-dependent edit (random insert, erase) - those would require observing
* the contents first - so a holder may put data in and reset, but never read it
* back out.
*
*   COMPOSITION, NOT INHERITANCE:
*   As with the sibling wrappers (see read_write_container.hpp), the container is
* a private member and only a chosen subset is forwarded; that is what makes the
* read side genuinely absent rather than merely undocumented.
*
*   ORTHOGONAL TO THE INTRINSIC GRADE:
*   write_only is an access OVERLAY, not a mutability grade - the underlying
* container is whatever it is; this is a restriction imposed on the handle.
*
*   PORTABILITY:
*   C++11 baseline.  size()/empty() are D_CONSTEXPR; the append operations are
* constexpr only where C++14 relaxed constexpr permits a non-const member to be
* constexpr (in C++11 a constexpr member is implicitly const).
*
*
* path:      /inc/djinterp/core/container/access/write_only_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_WRITE_ONLY_CONTAINER_
#define DJINTERP_WRITE_ONLY_CONTAINER_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"   // clean_t, D_CONSTEXPR, NS_*, D_ENV_* feature macros
#include "../meta/write_only.hpp"     // write_only capability tag


// DJINTERP_ACCESS_MUT_CONSTEXPR
//   constexpr on a MUTATING member only where C++14 relaxed constexpr allows a
// non-const member to be constexpr; empty in C++11.  Undefined at end of file.
#if ( D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL >= 201304L )
    #define DJINTERP_ACCESS_MUT_CONSTEXPR  D_CONSTEXPR
#else
    #define DJINTERP_ACCESS_MUT_CONSTEXPR
#endif


NS_DJINTERP


// write_only_container
//   class: holds a _Container privately and forwards only its append surface
// plus value-free metadata, under the write_only capability.
template<typename _Container>
class write_only_container
{
public:
    using container_type = _Container;
    using capability     = write_only;
    using value_type     = typename _Container::value_type;

    // --- construction ---

    D_CONSTEXPR write_only_container()
        : m_container()
    {}

    template<typename     _First,
             typename...  _Rest,
             typename = typename std::enable_if<
                 !std::is_same<clean_t<_First>, write_only_container>::value>::type>
    explicit D_CONSTEXPR write_only_container(_First&& _first, _Rest&&... _rest)
        : m_container(static_cast<_First&&>(_first), static_cast<_Rest&&>(_rest)...)
    {}

    // --- value-free metadata (no element value is revealed) ---

    template<typename _C = _Container>
    D_CONSTEXPR auto size() const
        -> decltype(std::declval<const _C&>().size())
    { return m_container.size(); }

    template<typename _C = _Container>
    D_CONSTEXPR auto empty() const
        -> decltype(std::declval<const _C&>().empty())
    { return m_container.empty(); }

    // --- append surface (the ONLY mutating surface) ---

    template<typename _C = _Container, typename... _Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto push_back(_Args&&... _args)
        -> decltype(std::declval<_C&>().push_back(std::declval<_Args>()...))
    { return m_container.push_back(static_cast<_Args&&>(_args)...); }

    template<typename _C = _Container, typename... _Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto emplace_back(_Args&&... _args)
        -> decltype(std::declval<_C&>().emplace_back(std::declval<_Args>()...))
    { return m_container.emplace_back(static_cast<_Args&&>(_args)...); }

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto clear()
        -> decltype(std::declval<_C&>().clear())
    { return m_container.clear(); }

private:
    _Container m_container;
};


// make_write_only
//   function: wraps a container value under the write_only capability, deducing
// the container type from the argument.
template<typename _Container>
D_CONSTEXPR write_only_container<clean_t<_Container>>
make_write_only(_Container&& _c)
{
    return write_only_container<clean_t<_Container>>(
        static_cast<_Container&&>(_c));
}


NS_END  // djinterp


#undef DJINTERP_ACCESS_MUT_CONSTEXPR


#endif  // DJINTERP_WRITE_ONLY_CONTAINER_
