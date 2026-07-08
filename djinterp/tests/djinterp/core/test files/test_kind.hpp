/******************************************************************************
* djinterp [test]                                                test_kind.hpp
*
*   The test_kind module: the kind record, the kind-set wrapper, and
* the resolved-query free functions that tie them together.  Storage
* and lookup of kinds, plus the per-kind structural data (rank,
* leaf/interior, defaults), live here in one place.
*
*   TEST KIND RECORD:
*   A test_kind is the structural definition of one test
* classification: the id it answers to, the rank it occupies in the
* tree hierarchy, whether it is a leaf or an interior node, an
* optional human-readable name, and a non-owning pointer to default
* options.  It is a plain aggregate - the canonical value type a user
* stores in a kind set.
*
*   TEST KIND SET:
*   A test_kind_set is a minimalist wrapper around any user-supplied
* set-classified container (as recognised by set_traits.hpp).  The
* container holds whatever represents kinds - test_kind records,
* integral ids, string tags, enum values - and all set work (lookup,
* insertion, erasure, iteration) is delegated to it.  test_kind_set
* stores no kind data of its own.  The previous test_type<_Container>
* registry is retired; this wrapper plus the record above replace it.
*
*   RESOLVED QUERIES:
*   The rank / leaf-interior / default-option resolution the old
* registry performed as member functions is provided as free
* functions over a kind set:
*     - find_kind(kinds, id)         -> const test_kind*
*     - rank_of(kinds, id)           -> std::uint16_t
*     - is_leaf(kinds, id)           -> bool
*     - is_interior(kinds, id)       -> bool
*     - name_of(kinds, id)           -> const char*
*     - default_options(kinds, id)   -> const test_option_set*
*     - can_be_child_of(kinds, c, p) -> bool
*   Each composes with a test_kind_set or any range of test_kind
* records, and each preserves the old registry fallback for an
* unregistered id: the raw id acts as the rank, the node is treated
* as a leaf, and it has no default options.
*
*   TREE INTEGRATION:
*   A kind set is metadata held alongside the tree (by whatever owns
* the walk), never a node in it.  When present, a test_object's
* test_type_id is resolved through it: a matched kind's rank governs
* insertion ordering and its is_leaf flag governs whether the node
* may have children, with default options cascading from the kind.
*
*   NO BUILT-IN KINDS:
*   This module defines no built-in kind constants.  The vocabulary
* of test classifications is entirely user-defined.  See
* test_defaults.hpp for the framework's default kind set.
*
*   CONSTRAINTS:
*   test_kind_set's _SetContainer must be structurally classified as
* a set-like container by set_traits.hpp.  Under C++20 this is
* enforced through a concept; under earlier standards through
* static_assert.
*
*   PORTABILITY:
*   C++11 minimum.  C++20 concepts are used when available; pre-C++20
* falls back to static_assert.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST KIND RECORD
* II.   FACTORY FUNCTION
* III.  TEST KIND SET
* IV.   RESOLVED QUERIES (FREE FUNCTIONS)
* V.    STRUCTURAL DETECTION
*
*
* path:      /inc/djinterp/test/test_kind.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.14
******************************************************************************/

#ifndef DJINTERP_TEST_KIND_
#define DJINTERP_TEST_KIND_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/meta/type_traits.hpp"
#include "../core/container/set/set_traits.hpp"
#include "./test_common.hpp"
#include "./test_options.hpp"

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #include "../core/container/set/set_concepts.hpp"
#endif


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST KIND RECORD                                    ///
///////////////////////////////////////////////////////////////////////////////

// test_kind
//   struct: the structural definition of one test classification.
// Aggregate-initializable.  Describes one class of test_object: the
// id it answers to, its rank in the tree hierarchy, whether it is a
// leaf or interior node, an optional name, and a non-owning pointer
// to default options.
//
//   This is the canonical value type stored in a kind set
// (test_kind_set); it carries no behavior of its own.  No built-in
// kind constants are provided - the vocabulary of test
// classifications is entirely user-defined.
//
// Usage:
//   test_kind k = {
//       10,                // id
//       "my_test_kind",    // name
//       2,                 // rank
//       true,              // is_leaf
//       nullptr            // default_options
//   };
struct test_kind
{
    test_type_id           id;
    const char*            name;
    std::uint16_t          rank;
    bool                   is_leaf;
    const test_option_set* default_options;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  FACTORY FUNCTION                                    ///
///////////////////////////////////////////////////////////////////////////////

// make_test_kind
//   function: convenience constructor for a test_kind record.  A
// single-return aggregate initialization, so it stays a constant
// expression on C++11 as well.
D_CONSTEXPR_INLINE test_kind
make_test_kind(
    test_type_id           _id,
    const char*            _name,
    std::uint16_t          _rank,
    bool                   _is_leaf,
    const test_option_set* _default_options = nullptr
) D_NOEXCEPT
{
    return test_kind{ _id, _name, _rank, _is_leaf, _default_options };
}


///////////////////////////////////////////////////////////////////////////////
///                III. TEST KIND SET                                       ///
///////////////////////////////////////////////////////////////////////////////

// test_kind_set
//   class: minimalist set wrapper.  Templated on any container
// satisfying the set protocol as recognised by set_traits.hpp.
// Stores no kind data of its own - all storage, lookup,
// insertion, erasure, and iteration are forwarded to the
// underlying container.
//
//   Template parameters:
//   _SetContainer - any set-classified container (std::set,
//                   djinterp::container::set, unordered variants,
//                   flat sets, etc.).  Must expose key_type,
//                   value_type, size_type, iterator, begin/end,
//                   and a structural set operation surface.
//
// Usage:
//   using kind_id_set = djinterp::container::set<std::int32_t>;
//   test_kind_set<kind_id_set> kinds;
//
//   kinds.insert(D_TEST_KIND_ASSERT);
//   if (kinds.contains(some_id)) { ... }

NS_INTERNAL
    // has_set_contains
    //   trait: ::type is std::true_type when _Container exposes a
    // contains(key_type) member (native-contains dispatch), else
    // std::false_type (the find()-fallback dispatch is selected).
    template<typename _Container,
             typename = void>
    struct has_set_contains : std::false_type
    {};

    template<typename _Container>
    struct has_set_contains<_Container, void_t<
        decltype(std::declval<const _Container&>().contains(
            std::declval<const typename _Container::key_type&>()))
    >> : std::true_type
    {};
NS_END  // internal

template<typename _SetContainer>
class test_kind_set
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(
        ::djinterp::container::traits::set_like_container<_SetContainer>,
        "`_SetContainer` must satisfy the set-like container "
        "protocol (set_traits.hpp / set_concepts.hpp).");
#else
    static_assert(
        ::djinterp::container::traits::is_set_like<_SetContainer>::value,
        "`_SetContainer` must satisfy the set-like container "
        "protocol (set_traits.hpp).");
#endif

public:
    // -----------------------------------------------------------------
    //  type aliases (forwarded from the underlying container)
    // -----------------------------------------------------------------

    using container_type   = _SetContainer;
    using key_type         = typename _SetContainer::key_type;
    using value_type       = typename _SetContainer::value_type;
    using size_type        = typename _SetContainer::size_type;
    using iterator         = typename _SetContainer::iterator;
    using const_iterator   = typename _SetContainer::const_iterator;


    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // test_kind_set
    //   constructor: default.  Wraps a default-constructed
    // underlying set container.
    test_kind_set() D_NOEXCEPT
        : m_set()
    {}

    // test_kind_set
    //   constructor: from container (copy).
    explicit test_kind_set(
        const container_type& _set
    )
        : m_set(_set)
    {}

    // test_kind_set
    //   constructor: from container (move).
    explicit test_kind_set(
        container_type&& _set
    ) D_NOEXCEPT
        : m_set(static_cast<container_type&&>(_set))
    {}


    // -----------------------------------------------------------------
    //  underlying access
    // -----------------------------------------------------------------

    // underlying
    //   returns a mutable reference to the wrapped container.
    container_type&
    underlying() D_NOEXCEPT
    {
        return m_set;
    }

    // underlying (const)
    const container_type&
    underlying() const D_NOEXCEPT
    {
        return m_set;
    }


    // -----------------------------------------------------------------
    //  forwarded set surface
    // -----------------------------------------------------------------

    // size
    size_type
    size() const D_NOEXCEPT
    {
        return m_set.size();
    }

    // empty
    bool
    empty() const D_NOEXCEPT
    {
        return m_set.empty();
    }

    // clear
    void
    clear()
    {
        m_set.clear();

        return;
    }

    // insert
    //   forwards a copy of _value into the container.
    auto
    insert(
        const value_type& _value
    )
        -> decltype(std::declval<container_type&>().insert(_value))
    {
        return m_set.insert(_value);
    }

    // insert
    //   forwards an rvalue of _value into the container.
    auto
    insert(
        value_type&& _value
    )
        -> decltype(std::declval<container_type&>().insert(
                std::move(_value)))
    {
        return m_set.insert(static_cast<value_type&&>(_value));
    }

    // erase
    //   forwards a key-based erase to the container.
    auto
    erase(
        const key_type& _key
    )
        -> decltype(std::declval<container_type&>().erase(_key))
    {
        return m_set.erase(_key);
    }

    // find
    //   forwards a key-based find to the container.
    iterator
    find(
        const key_type& _key
    )
    {
        return m_set.find(_key);
    }

    // find (const)
    const_iterator
    find(
        const key_type& _key
    ) const
    {
        return m_set.find(_key);
    }

    // contains
    //   returns true if _key is present in the container.
    // Uses the container's contains() when available; falls
    // back to find() != end() otherwise.
    bool
    contains(
        const key_type& _key
    ) const
    {
        return contains_dispatch(
            _key,
            typename internal::has_set_contains<container_type>::type{});
    }


    // -----------------------------------------------------------------
    //  iteration
    // -----------------------------------------------------------------

    iterator        begin()         D_NOEXCEPT { return m_set.begin();  }
    const_iterator  begin()  const  D_NOEXCEPT { return m_set.begin();  }
    iterator        end()           D_NOEXCEPT { return m_set.end();    }
    const_iterator  end()    const  D_NOEXCEPT { return m_set.end();    }
    const_iterator  cbegin() const  D_NOEXCEPT { return m_set.begin();  }
    const_iterator  cend()   const  D_NOEXCEPT { return m_set.end();    }


private:
    // -----------------------------------------------------------------
    //  internal: contains() dispatch
    // -----------------------------------------------------------------

    // contains_dispatch (native contains)
    bool
    contains_dispatch(
        const key_type& _key,
        std::true_type
    ) const
    {
        return m_set.contains(_key);
    }

    // contains_dispatch (find fallback)
    bool
    contains_dispatch(
        const key_type& _key,
        std::false_type
    ) const
    {
        return (m_set.find(_key) != m_set.end());
    }

    container_type m_set;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  RESOLVED QUERIES (FREE FUNCTIONS)                   ///
///////////////////////////////////////////////////////////////////////////////
//
//   These resolve a test_type_id against a kind set.  _Kinds is any
// range of test_kind records exposing const_iterator and begin() /
// end() (a test_kind_set, a std::vector<test_kind>, etc.); lookup is
// a linear scan by id - the discipline the retired registry used,
// first match winning.

// find_kind
//   returns a pointer to the test_kind whose id is _id within
// _kinds, or nullptr if none is registered.
template<typename _Kinds>
const test_kind*
find_kind(
    const _Kinds& _kinds,
    test_type_id  _id
)
{
    typename _Kinds::const_iterator it  = _kinds.begin();
    typename _Kinds::const_iterator end = _kinds.end();

    for (; it != end; ++it)
    {
        if (it->id == _id)
        {
            return &(*it);
        }
    }

    return nullptr;
}

// rank_of
//   returns the rank for _id.  If registered, the matched kind's
// rank; otherwise the raw id cast to uint16_t (the fallback when no
// kind set resolves the id).
template<typename _Kinds>
std::uint16_t
rank_of(
    const _Kinds& _kinds,
    test_type_id  _id
)
{
    const test_kind* k = find_kind(_kinds, _id);

    if (k)
    {
        return k->rank;
    }

    return static_cast<std::uint16_t>(_id);
}

// is_leaf
//   returns true if _id maps to a leaf kind.  Unregistered ids
// default to leaf.
template<typename _Kinds>
bool
is_leaf(
    const _Kinds& _kinds,
    test_type_id  _id
)
{
    const test_kind* k = find_kind(_kinds, _id);

    if (k)
    {
        return k->is_leaf;
    }

    return true;
}

// is_interior
//   complement of is_leaf.
template<typename _Kinds>
bool
is_interior(
    const _Kinds& _kinds,
    test_type_id  _id
)
{
    return !is_leaf(_kinds, _id);
}

// name_of
//   returns the name for _id, or nullptr if unregistered.
template<typename _Kinds>
const char*
name_of(
    const _Kinds& _kinds,
    test_type_id  _id
)
{
    const test_kind* k = find_kind(_kinds, _id);

    if (k)
    {
        return k->name;
    }

    return nullptr;
}

// default_options
//   returns the default options pointer for _id, or nullptr if
// unregistered or the kind has no defaults.
template<typename _Kinds>
const test_option_set*
default_options(
    const _Kinds& _kinds,
    test_type_id  _id
)
{
    const test_kind* k = find_kind(_kinds, _id);

    if (k)
    {
        return k->default_options;
    }

    return nullptr;
}

// can_be_child_of
//   returns true if an object of kind _child_id may be inserted as a
// child of an object of kind _parent_id.  The rule is rank
// monotonicity: child rank <= parent rank, both resolved through the
// kind set (unmatched ids fall back to the raw id as rank).
template<typename _Kinds>
bool
can_be_child_of(
    const _Kinds& _kinds,
    test_type_id  _child_id,
    test_type_id  _parent_id
)
{
    return (rank_of(_kinds, _child_id) <= rank_of(_kinds, _parent_id));
}


///////////////////////////////////////////////////////////////////////////////
///                V.   STRUCTURAL DETECTION                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_test_kind_set_instantiation
    //   trait: detects whether a type is an instantiation of
    // the test_kind_set class template.
    template<typename _Type>
    struct is_test_kind_set_instantiation : std::false_type
    {};

    template<typename _SetContainer>
    struct is_test_kind_set_instantiation<test_kind_set<_SetContainer>>
        : std::true_type
    {};

NS_END  // internal

// is_test_kind_set
//   trait: true if _Type is an instantiation of test_kind_set.
template<typename _Type>
struct is_test_kind_set
    : internal::is_test_kind_set_instantiation<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_test_kind_set_v =
        is_test_kind_set<_Type>::value;
#endif


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_KIND_