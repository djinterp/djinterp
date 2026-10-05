/*******************************************************************************
* djinterp [re_std]                                            optional_base.hpp
*
* optional_base class header:
*   internal storage layers for re_std::optional.
*
*   Not a public header.  optional<T> is assembled from two base layers whose
* only job is to make the SPECIAL MEMBER FUNCTIONS of optional<T> inherit their
* triviality from T.  That cannot be done in one class: a special member is
* trivial only if it is not user-provided, so the class that provides a
* non-trivial one must be a DIFFERENT class from the one that defaults it.
* Hence the layering.
*
*     optional_storage<T, trivially_destructible>   union + engaged flag
*         layer 1: declares a destructor only when T needs one.
*
*     optional_base<T, trivially_copyable>          copy / move plumbing
*         layer 2: user-provides copy/move ctor and assignment only when T
*         needs them; otherwise defaults all four.
*
*     optional<T>                                   the public interface
*         declares NO copy/move ctor or assignment at all, so all four are
*         implicitly defaulted and their triviality falls straight through
*         from the layers below.
*
*   WHY THIS MATTERS, CONCRETELY.
*   With the layering, optional<int> is trivially copyable, trivially
* destructible, passable in registers, memcpy-able by a container reallocating
* its buffer, and USABLE IN A CONSTANT EXPRESSION - including copy-construction,
* because a defaulted copy constructor is constexpr when the underlying copy is.
* Without it, every one of those is lost for every T.
*
*   THE UNION IS NOT AN OPTIMISATION.
*   Storage is a union with a char alternative, not an aligned buffer plus a
* reinterpret_cast, because a union member can be initialised in a constexpr
* constructor and a placement-new into a byte buffer cannot.  The union is what
* makes constexpr optional possible at all.  The `char m_empty` alternative
* gives the disengaged state something to activate so the union is never
* wholly uninitialised.
*
*   STD IS C++17; re_std IS C++11 - a six-year back-port, and constexpr six
* years early with it.
*
*
* path:      /inc/re_std/optional/optional_base.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_OPTIONAL_OPTIONAL_BASE_HPP
#define RE_STD_OPTIONAL_OPTIONAL_BASE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../memory/addressof.hpp"
#include "./in_place.hpp"
#include "./nullopt.hpp"

namespace re_std
{

namespace internal
{

// =============================================================================
// LAYER 1 - storage, and destructor triviality
// =============================================================================

// optional_storage
//   struct: union storage plus the engaged flag.  Primary template is the
// NON-trivially-destructible case and declares a destructor.
template<typename Type,
         bool TriviallyDestructible = is_trivially_destructible<Type>::value>
struct optional_storage
{
    union
    {
        char  m_empty;
        Type m_value;
    };
    bool m_engaged;

    RE_STD_CONSTEXPR optional_storage() RE_STD_NOEXCEPT
        : m_empty(), m_engaged(false)
    {}

    template<typename... Args>
    RE_STD_CONSTEXPR explicit optional_storage(in_place_t, Args&&... args)
        : m_value(static_cast<Args&&>(args)...), m_engaged(true)
    {}

    ~optional_storage()
    {
        destroy();
        return;
    }

    optional_storage(const optional_storage&)            = default;
    optional_storage(optional_storage&&)                 = default;
    optional_storage& operator=(const optional_storage&) = default;
    optional_storage& operator=(optional_storage&&)      = default;

    // destroy
    //   function: end the contained object's lifetime if there is one.
    void destroy() RE_STD_NOEXCEPT
    {
        if (m_engaged)
        {
            m_value.~Type();
            m_engaged = false;
        }
        return;
    }

    // construct
    //   function: begin a contained object's lifetime.  Precondition: not
    // currently engaged.
    template<typename... Args>
    void construct(Args&&... args)
    {
        ::new (static_cast<void*>(re_std::addressof(m_value)))
            Type(static_cast<Args&&>(args)...);
        m_engaged = true;
        return;
    }
};

// optional_storage<Type, true>
//   struct: trivially-destructible Type.  Declares NO destructor, so
// optional<Type> keeps a trivial one and stays a literal type.
template<typename Type>
struct optional_storage<Type, true>
{
    union
    {
        char  m_empty;
        Type m_value;
    };
    bool m_engaged;

    RE_STD_CONSTEXPR optional_storage() RE_STD_NOEXCEPT
        : m_empty(), m_engaged(false)
    {}

    template<typename... Args>
    RE_STD_CONSTEXPR explicit optional_storage(in_place_t, Args&&... args)
        : m_value(static_cast<Args&&>(args)...), m_engaged(true)
    {}

    //   No ~optional_storage() here.  That omission is the entire point of the
    // specialisation - declaring even an empty one would make the destructor
    // non-trivial and cost optional<int> its literal-type status.

    void destroy() RE_STD_NOEXCEPT
    {
        m_engaged = false;
        return;
    }

    template<typename... Args>
    void construct(Args&&... args)
    {
        ::new (static_cast<void*>(re_std::addressof(m_value)))
            Type(static_cast<Args&&>(args)...);
        m_engaged = true;
        return;
    }
};


// =============================================================================
// LAYER 2 - copy / move triviality
// =============================================================================

// optional_base
//   struct: primary template, for types that need real copy/move plumbing.
// Every one of the four is user-provided here, so none is trivial - which is
// correct, because Type's own are not either.
template<typename Type,
         bool TriviallyCopyable = is_trivially_copyable<Type>::value>
struct optional_base : optional_storage<Type>
{
    typedef optional_storage<Type> _Storage;

    using _Storage::_Storage;

    RE_STD_CONSTEXPR optional_base() RE_STD_NOEXCEPT {}

    optional_base(const optional_base& other)
    {
        if (other.m_engaged)
        {
            this->construct(other.m_value);
        }
        return;
    }

    optional_base(optional_base&& other)
        RE_STD_NOEXCEPT_IF(is_nothrow_move_constructible<Type>::value)
    {
        if (other.m_engaged)
        {
            this->construct(static_cast<Type&&>(other.m_value));
        }
        return;
    }

    optional_base& operator=(const optional_base& other)
    {
        //   Four cases, and the engaged-to-engaged one must ASSIGN rather than
        // destroy-and-reconstruct: assignment is what Type's own operator=
        // exists for, and reconstructing would lose the strong guarantee a
        // throwing copy assignment might otherwise provide.
        if (this->m_engaged && other.m_engaged)
        {
            this->m_value = other.m_value;
        }
        else if (other.m_engaged)
        {
            this->construct(other.m_value);
        }
        else
        {
            this->destroy();
        }
        return *this;
    }

    optional_base& operator=(optional_base&& other)
        RE_STD_NOEXCEPT_IF(   is_nothrow_move_constructible<Type>::value
                      && is_nothrow_move_assignable<Type>::value)
    {
        if (this->m_engaged && other.m_engaged)
        {
            this->m_value = static_cast<Type&&>(other.m_value);
        }
        else if (other.m_engaged)
        {
            this->construct(static_cast<Type&&>(other.m_value));
        }
        else
        {
            this->destroy();
        }
        return *this;
    }
};

// optional_base<Type, true>
//   struct: trivially-copyable Type.  All four special members are defaulted,
// so optional<Type> is itself trivially copyable - and copy-construction
// works in a constant expression, which a user-provided copy never could
// before C++20.
template<typename Type>
struct optional_base<Type, true> : optional_storage<Type>
{
    typedef optional_storage<Type> _Storage;

    using _Storage::_Storage;

    RE_STD_CONSTEXPR optional_base() RE_STD_NOEXCEPT {}

    optional_base(const optional_base&)            = default;
    optional_base(optional_base&&)                 = default;
    optional_base& operator=(const optional_base&) = default;
    optional_base& operator=(optional_base&&)      = default;
};

}  // internal

}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_OPTIONAL_OPTIONAL_BASE_HPP
