/******************************************************************************
* djinterp [container]                                   read_only_container.hpp
*
*   The read_only access wrapper: a handle that observes its container but
* cannot modify it.  It HOLDS the container privately, is populated at
* construction, and thereafter forwards ONLY the const observation surface -
* size / empty / const operator[] / front / back / data / const iteration.  It
* exposes no mutator and, crucially, never returns a non-const handle to the
* underlying container, so the seal is genuine: there is no path through this
* type by which the held container can change.
*
*   COMPOSITION, NOT INHERITANCE:
*   Sealing by restriction is only possible because the container is a private
* member rather than a public base - public inheritance would re-expose every
* mutator the underlying type has.  (See read_write_container.hpp for the shared
* rationale and the forwarder mechanics.)
*
*   ORTHOGONAL TO THE INTRINSIC GRADE:
*   read_only is an access OVERLAY, not a mutability grade.  A read_only handle
* to a fully mutable container still forbids mutation through THIS handle; what
* the underlying type could do is irrelevant to what this handle permits.
*
*   PORTABILITY:
*   C++11 baseline.  All forwarders are D_CONSTEXPR; whether a given const access
* is usable in a constant expression is governed by the held container's own
* lifetime (the Lifetime axis).
*
*
* path:      /inc/djinterp/core/container/access/read_only_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_READ_ONLY_CONTAINER_
#define DJINTERP_READ_ONLY_CONTAINER_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"        // clean_t, D_CONSTEXPR, NS_*
#include "../meta/read_only.hpp"  // read_only capability tag


NS_DJINTERP


// read_only_container
//   class: holds a _Container privately and forwards only its const observation
// surface under the read_only capability.
template<typename _Container>
class read_only_container
{
public:
    using container_type = _Container;
    using capability     = read_only;
    using value_type     = typename _Container::value_type;

    // --- construction (populate at construction; sealed thereafter) ---

    D_CONSTEXPR read_only_container()
        : m_container()
    {}

    template<typename     _First,
             typename...  _Rest,
             typename = typename std::enable_if<
                 !std::is_same<clean_t<_First>, read_only_container>::value>::type>
    explicit D_CONSTEXPR read_only_container(_First&& _first, _Rest&&... _rest)
        : m_container(static_cast<_First&&>(_first), static_cast<_Rest&&>(_rest)...)
    {}

    // --- const observation surface (the ONLY surface) ---

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

private:
    _Container m_container;
};


// make_read_only
//   function: wraps a container value under the read_only capability, deducing
// the container type from the argument.
template<typename _Container>
D_CONSTEXPR read_only_container<clean_t<_Container>>
make_read_only(_Container&& _c)
{
    return read_only_container<clean_t<_Container>>(
        static_cast<_Container&&>(_c));
}


NS_END  // djinterp


#endif  // DJINTERP_READ_ONLY_CONTAINER_
