/*******************************************************************************
* djinterp [core]                                       read_write_container.hpp
*
*   The read_write access wrapper: the full-capability member of the access
* trio. It HOLDS a container privately and forwards both the const observation
* surface AND the mutating surface - whichever members the underlying
* container
* actually exposes. It is the unrestricted baseline against which read_only
* and
* write_only are the restrictions; on its own it adds uniformity (a single
* access surface across container types) rather than constraint.
*
*   COMPOSITION, NOT INHERITANCE:
*   The container is a private member, never a public base. Public inheritance
* would expose the underlying type's entire interface and make any restriction
* a
* fiction; holding it privately and forwarding a CHOSEN subset is what lets
* the
* sibling wrappers actually seal capabilities away.
*
*   SFINAE-GUARDED, CONDITIONALLY CONSTEXPR FORWARDERS:
*   Every forwarder is a member template whose trailing return type is the
* very
* call it forwards, so a method materialises ONLY when the underlying
* container
* supports it (a std::array wrapper has no push_back; a std::list wrapper has
* no
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
* path:      /inc/djinterp/core/container/read_write_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_READ_WRITE_CONTAINER_HPP
#define DJINTERP_CONTAINER_READ_WRITE_CONTAINER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"         // clean_t, D_CONSTEXPR, NS_*, D_ENV_* feature macros
#include "../meta/read_write.hpp"  // read_write capability tag


// DJINTERP_ACCESS_MUT_CONSTEXPR
//   constexpr on a MUTATING member only where C++14 relaxed constexpr allows a
// non-const member to be constexpr; empty in C++11. Undefined at end of file.
#if ( D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL >= 201304L )
    #define DJINTERP_ACCESS_MUT_CONSTEXPR  D_CONSTEXPR
#else
    #define DJINTERP_ACCESS_MUT_CONSTEXPR
#endif


NS_DJINTERP


// read_write_container
//   class: holds a Container privately and forwards its full surface under
// the read_write capability.
template<typename Container>
class read_write_container
{
public:
    using container_type = Container;
    using capability     = read_write;
    using value_type     = typename Container::value_type;

    // --- construction ---

    // default: present (and instantiable) only when Container is default-
    // constructible, by the usual lazy-instantiation rule.
    D_CONSTEXPR read_write_container()
        : m_container()
    {}

    // wrap / emplace: constructs the held container from the arguments - one
    // container argument copy/moves it; several arguments emplace it. Excluded
    // for a single read_write_container argument so the copy/move constructor
    // is used instead.
    template<typename     First,
             typename...  Rest,
             typename = typename std::enable_if<
                 !std::is_same<clean_t<First>, read_write_container>::value>::type>
    explicit D_CONSTEXPR read_write_container(First&& _first, Rest&&... _rest)
        : m_container(static_cast<First&&>(_first), static_cast<Rest&&>(_rest)...)
    {}

    // --- const observation surface ---

    template<typename C = Container>
    D_CONSTEXPR auto size() const
        -> decltype(std::declval<const C&>().size())
    { return m_container.size(); }

    template<typename C = Container>
    D_CONSTEXPR auto empty() const
        -> decltype(std::declval<const C&>().empty())
    { return m_container.empty(); }

    template<typename C = Container>
    D_CONSTEXPR auto operator[](std::size_t _i) const
        -> decltype(std::declval<const C&>()[_i])
    { return m_container[_i]; }

    template<typename C = Container>
    D_CONSTEXPR auto front() const
        -> decltype(std::declval<const C&>().front())
    { return m_container.front(); }

    template<typename C = Container>
    D_CONSTEXPR auto back() const
        -> decltype(std::declval<const C&>().back())
    { return m_container.back(); }

    template<typename C = Container>
    D_CONSTEXPR auto data() const
        -> decltype(std::declval<const C&>().data())
    { return m_container.data(); }

    template<typename C = Container>
    D_CONSTEXPR auto begin() const
        -> decltype(std::declval<const C&>().begin())
    { return m_container.begin(); }

    template<typename C = Container>
    D_CONSTEXPR auto end() const
        -> decltype(std::declval<const C&>().end())
    { return m_container.end(); }

    template<typename C = Container>
    D_CONSTEXPR auto cbegin() const
        -> decltype(std::declval<const C&>().cbegin())
    { return m_container.cbegin(); }

    template<typename C = Container>
    D_CONSTEXPR auto cend() const
        -> decltype(std::declval<const C&>().cend())
    { return m_container.cend(); }

    // --- non-const element access (the write side of read_write) ---

    template<typename C = Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto operator[](std::size_t _i)
        -> decltype(std::declval<C&>()[_i])
    { return m_container[_i]; }

    template<typename C = Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto front()
        -> decltype(std::declval<C&>().front())
    { return m_container.front(); }

    template<typename C = Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto back()
        -> decltype(std::declval<C&>().back())
    { return m_container.back(); }

    template<typename C = Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto data()
        -> decltype(std::declval<C&>().data())
    { return m_container.data(); }

    template<typename C = Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto begin()
        -> decltype(std::declval<C&>().begin())
    { return m_container.begin(); }

    template<typename C = Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto end()
        -> decltype(std::declval<C&>().end())
    { return m_container.end(); }

    // --- mutating surface (forwarded with perfect argument forwarding) ---

    template<typename C = Container, typename... Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto push_back(Args&&... _args)
        -> decltype(std::declval<C&>().push_back(std::declval<Args>()...))
    { return m_container.push_back(static_cast<Args&&>(_args)...); }

    template<typename C = Container, typename... Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto push_front(Args&&... _args)
        -> decltype(std::declval<C&>().push_front(std::declval<Args>()...))
    { return m_container.push_front(static_cast<Args&&>(_args)...); }

    template<typename C = Container, typename... Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto emplace_back(Args&&... _args)
        -> decltype(std::declval<C&>().emplace_back(std::declval<Args>()...))
    { return m_container.emplace_back(static_cast<Args&&>(_args)...); }

    template<typename C = Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto pop_back()
        -> decltype(std::declval<C&>().pop_back())
    { return m_container.pop_back(); }

    template<typename C = Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto pop_front()
        -> decltype(std::declval<C&>().pop_front())
    { return m_container.pop_front(); }

    template<typename C = Container, typename... Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto insert(Args&&... _args)
        -> decltype(std::declval<C&>().insert(std::declval<Args>()...))
    { return m_container.insert(static_cast<Args&&>(_args)...); }

    template<typename C = Container, typename... Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto erase(Args&&... _args)
        -> decltype(std::declval<C&>().erase(std::declval<Args>()...))
    { return m_container.erase(static_cast<Args&&>(_args)...); }

    template<typename C = Container>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto clear()
        -> decltype(std::declval<C&>().clear())
    { return m_container.clear(); }

    template<typename C = Container, typename... Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto resize(Args&&... _args)
        -> decltype(std::declval<C&>().resize(std::declval<Args>()...))
    { return m_container.resize(static_cast<Args&&>(_args)...); }

    template<typename C = Container, typename... Args>
    DJINTERP_ACCESS_MUT_CONSTEXPR auto reserve(Args&&... _args)
        -> decltype(std::declval<C&>().reserve(std::declval<Args>()...))
    { return m_container.reserve(static_cast<Args&&>(_args)...); }

private:
    Container m_container;
};


// make_read_write
//   function: wraps a container value under the read_write capability,
// deducing the container type from the argument.
template<typename Container>
D_CONSTEXPR read_write_container<clean_t<Container>>
make_read_write(Container&& _c)
{
    return read_write_container<clean_t<Container>>(
        static_cast<Container&&>(_c));
}


NS_END  // djinterp


#undef DJINTERP_ACCESS_MUT_CONSTEXPR

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_READ_WRITE_CONTAINER_HPP
