/******************************************************************************
* djinterp [container]                                  read_write_container.hpp
*
*   The read_write access wrapper: the full-capability member of the access
* trio.  It HOLDS a container privately and forwards both the const observation
* surface AND the mutating surface - whichever members the underlying container
* actually exposes.  It is the unrestricted baseline against which read_only and
* write_only are the restrictions; on its own it adds uniformity (a single
* access surface across container types) rather than constraint.
*
*   COMPOSITION, NOT INHERITANCE:
*   The container is a private member, never a public base.  Public inheritance
* would expose the underlying type's entire interface and make any restriction a
* fiction; holding it privately and forwarding a CHOSEN subset is what lets the
* sibling wrappers actually seal capabilities away.
*
*   SFINAE-GUARDED, CONDITIONALLY CONSTEXPR FORWARDERS:
*   Every forwarder is a member template whose trailing return type is the very
* call it forwards, so a method materialises ONLY when the underlying container
* supports it (a std::array wrapper has no push_back; a std::list wrapper has no
* operator[]).  Const observers are D_CONSTEXPR.  Mutators are constexpr only
* where C++14 relaxed constexpr permits a non-const member to be constexpr (in
* C++11 a constexpr member is implicitly const, so a mutator cannot be one);
* whether a given call is actually USABLE in a constant expression is then
* governed by the underlying container's own lifetime (the Lifetime axis), not
* by this wrapper.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/access/read_write_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_READ_WRITE_CONTAINER_
#define DJINTERP_READ_WRITE_CONTAINER_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"         // clean_t, D_CONSTEXPR, NS_*, D_ENV_* feature macros
#include "../meta/read_write.hpp"  // read_write capability tag


// DJINTERP_ACCESS_MUT_CONSTEXPR
//   constexpr on a MUTATING member only where C++14 relaxed constexpr allows a
// non-const member to be constexpr; empty in C++11.  Undefined at end of file.
#if ( D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL >= 201304L )
    #define DJINTERP_ACCESS_MUT_CONSTEXPR  D_CONSTEXPR
#else
    #define DJINTERP_ACCESS_MUT_CONSTEXPR
#endif


NS_DJINTERP


// read_write_container
//   class: holds a _Container privately and forwards its full surface under the
// read_write capability.
template<typename _Container>
class read_write_container
{
public:
    using container_type = _Container;
    using capability     = read_write;
    using value_type     = typename _Container::value_type;

    // --- construction ---

    // default: present (and instantiable) only when _Container is default-
    // constructible, by the usual lazy-instantiation rule.
    D_CONSTEXPR read_write_container()
        : m_container()
    {}

    // wrap / emplace: constructs the held container from the arguments - one
    // container argument copy/moves it; several arguments emplace it.  Excluded
    // for a single read_write_container argument so the copy/move constructor is
    // used instead.
    template<typename     _First,
             typename...  _Rest,
             typename = typename std::enable_if<
                 !std::is_same<clean_t<_First>, read_write_container>::value>::type>
    explicit D_CONSTEXPR read_write_container(_First&& _first, _Rest&&... _rest)
        : m_container(static_cast<_First&&>(_first), static_cast<_Rest&&>(_rest)...)
    {}

    // --- const observation surface ---

    template<typename _C = _Container>
    D_CONSTEXPR auto size() const
        -> decltype(std::declval<const _C&>().size())
    { return m_container.size(); }

    template<typename _C = _Container>
    D_CONSTEXPR auto empty() const
        -> decltype(std::declval<const _C&>().empty())
    { return m_container.empty(); }

    template<typename _C = _Container>
    D_CONSTEXPR auto operator[](std::size_t _i) const
        -> decltype(std::declval<const _C&>()[_i])
    { return m_container[_i]; }

    template<typename _C = _Container>
    D_CONSTEXPR auto front() const
        -> decltype(std::declval<const _C&>().front())
    { return m_container.front(); }

    template<typename _C = _Container>
    D_CONSTEXPR auto back() const
        -> decltype(std::declval<const _C&>().back())
    { return m_container.back(); }

    template<typename _C = _Container>
    D_CONSTEXPR auto data() const
        -> decltype(std::declval<const _C&>().data())
    { return m_container.data(); }

    template<typename _C = _Container>
    D_CONSTEXPR auto begin() const
        -> decltype(std::declval<const _C&>().begin())
    { return m_container.begin(); }

    template<typename _C = _Container>
    D_CONSTEXPR auto end() const
        -> decltype(std::declval<const _C&>().end())
    { return m_container.end(); }

    template<typename _C = _Container>
    D_CONSTEXPR auto cbegin() const
        -> decltype(std::declval<const _C&>().cbegin())
    { return m_container.cbegin(); }

    template<typename _C = _Container>
    D_CONSTEXPR auto cend() const
        -> decltype(std::declval<const _C&>().cend())
    { return m_container.cend(); }

    // --- non-const element access (the write side of read_write) ---

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto operator[](std::size_t _i)
        -> decltype(std::declval<_C&>()[_i])
    { return m_container[_i]; }

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto front()
        -> decltype(std::declval<_C&>().front())
    { return m_container.front(); }

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto back()
        -> decltype(std::declval<_C&>().back())
    { return m_container.back(); }

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto data()
        -> decltype(std::declval<_C&>().data())
    { return m_container.data(); }

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto begin()
        -> decltype(std::declval<_C&>().begin())
    { return m_container.begin(); }

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto end()
        -> decltype(std::declval<_C&>().end())
    { return m_container.end(); }

    // --- mutating surface (forwarded with perfect argument forwarding) ---

    template<typename _C = _Container, typename... _Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto push_back(_Args&&... _args)
        -> decltype(std::declval<_C&>().push_back(std::declval<_Args>()...))
    { return m_container.push_back(static_cast<_Args&&>(_args)...); }

    template<typename _C = _Container, typename... _Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto push_front(_Args&&... _args)
        -> decltype(std::declval<_C&>().push_front(std::declval<_Args>()...))
    { return m_container.push_front(static_cast<_Args&&>(_args)...); }

    template<typename _C = _Container, typename... _Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto emplace_back(_Args&&... _args)
        -> decltype(std::declval<_C&>().emplace_back(std::declval<_Args>()...))
    { return m_container.emplace_back(static_cast<_Args&&>(_args)...); }

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto pop_back()
        -> decltype(std::declval<_C&>().pop_back())
    { return m_container.pop_back(); }

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto pop_front()
        -> decltype(std::declval<_C&>().pop_front())
    { return m_container.pop_front(); }

    template<typename _C = _Container, typename... _Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto insert(_Args&&... _args)
        -> decltype(std::declval<_C&>().insert(std::declval<_Args>()...))
    { return m_container.insert(static_cast<_Args&&>(_args)...); }

    template<typename _C = _Container, typename... _Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto erase(_Args&&... _args)
        -> decltype(std::declval<_C&>().erase(std::declval<_Args>()...))
    { return m_container.erase(static_cast<_Args&&>(_args)...); }

    template<typename _C = _Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto clear()
        -> decltype(std::declval<_C&>().clear())
    { return m_container.clear(); }

    template<typename _C = _Container, typename... _Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto resize(_Args&&... _args)
        -> decltype(std::declval<_C&>().resize(std::declval<_Args>()...))
    { return m_container.resize(static_cast<_Args&&>(_args)...); }

    template<typename _C = _Container, typename... _Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto reserve(_Args&&... _args)
        -> decltype(std::declval<_C&>().reserve(std::declval<_Args>()...))
    { return m_container.reserve(static_cast<_Args&&>(_args)...); }

private:
    _Container m_container;
};


// make_read_write
//   function: wraps a container value under the read_write capability, deducing
// the container type from the argument.
template<typename _Container>
D_CONSTEXPR read_write_container<clean_t<_Container>>
make_read_write(_Container&& _c)
{
    return read_write_container<clean_t<_Container>>(
        static_cast<_Container&&>(_c));
}


NS_END  // djinterp


#undef DJINTERP_ACCESS_MUT_CONSTEXPR


#endif  // DJINTERP_READ_WRITE_CONTAINER_
