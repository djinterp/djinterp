/******************************************************************************
* djinterp [test]                                              test_type.hpp
*
*   Test kind definitions and the parameterized test_type registry.
*
*   A test_kind is the structural definition of a test classification:
* what rank it occupies in the tree hierarchy, whether it is a leaf or
* interior node, an optional human-readable name, and a pointer to
* default options that apply to all test_objects of that kind.
*
*   A test_type<_Container> is a parameterized collection of test_kind
* entries.  The _Container template parameter controls the storage
* strategy (std::vector for dynamic registration, a fixed_array for
* compile-time-known sets, etc.).  The test_type provides lookup by
* test_type_id and satisfies the structural detection probes in
* lookup_table_traits.hpp (key_type, contains, find).
*
*   TREE INTEGRATION:
*   A test_tree may optionally hold a test_type instance.  When it
* does, each test_object's test_type_id is resolved against the
* registry:
*
*   - MATCHED: the test_kind's rank governs tree insertion ordering
*     and its is_leaf flag determines whether the node may have
*     children.  Default options cascade from the test_kind.
*
*   - UNMATCHED: the raw test_type_id acts as the rank (the original
*     behavior).  The node is treated as a leaf with no default
*     options.
*
*   test_type instances are NOT nodes in the tree.  They are metadata
* held alongside the tree to enrich its insertion and query logic.
*
*   NO BUILT-IN KINDS:
*   This header defines no built-in kind constants.  The vocabulary
* of test classifications is entirely user-defined.  See
* test_defaults.hpp for the framework's default kind set.
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h for version detection and djinterp.hpp
* for namespace macros and constexpr support.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST KIND
* II.   TEST TYPE (PARAMETERIZED REGISTRY)
* III.  FACTORY FUNCTION
* IV.   TRAIT DETECTION
*
*
* path:      /inc/djinterp/test/test_type.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.14
******************************************************************************/

#ifndef DJINTERP_TEST_TYPE_
#define DJINTERP_TEST_TYPE_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"
#include "./test_options.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST KIND                                            ///
///////////////////////////////////////////////////////////////////////////////

// test_kind
//   struct: the structural definition of a test classification.
// Aggregate-initializable.  Each test_kind describes one class
// of test_object: its id, rank in the tree hierarchy, whether
// it is a leaf or interior node, an optional name, and a
// non-owning pointer to default options.
//
// No built-in kind constants are provided - the vocabulary
// of test classifications is entirely user-defined.
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
    test_type_id            id;
    const char*             name;
    std::uint16_t           rank;
    bool                    is_leaf;
    const dtest_option_set* default_options;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST TYPE (PARAMETERIZED REGISTRY)                   ///
///////////////////////////////////////////////////////////////////////////////

// test_type
//   class: a parameterized registry of test_kind entries.
// Provides lookup by test_type_id: find() returns a pointer
// to the matching test_kind, contains() checks existence.
//
// The _Container parameter controls the storage model.
// std::vector<test_kind> gives dynamic registration;
// a fixed_array or std::array gives compile-time-known
// sets.
//
// Usage:
//   test_type<> types;
//   types.register_kind({ 0, "leaf",     0, true,  nullptr });
//   types.register_kind({ 1, "interior", 1, false, nullptr });
//
//   const test_kind* k = types.find(0);
//   bool leaf = k->is_leaf;   // true
template<typename _Container = std::vector<test_kind>>
class test_type
{
public:
    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------
    using key_type               = test_type_id;
    using value_type             = test_kind;
    using container_type         = _Container;
    using backing_container_type = _Container;
    using size_type              = std::size_t;
    using iterator               = typename _Container::iterator;
    using const_iterator         = typename _Container::const_iterator;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------
    test_type() = default;

    explicit test_type(
        _Container _kinds
    )
        : m_kinds(static_cast<_Container&&>(_kinds))
    {}

    // -----------------------------------------------------------------
    //  registration
    // -----------------------------------------------------------------

    // register_kind
    //   inserts a test_kind entry.  Returns false if the id
    // already exists (first wins).
    bool
    register_kind(
        const test_kind& _kind
    )
    {
        // reject duplicate id
        if (contains(_kind.id))
        {
            return false;
        }

        m_kinds.push_back(_kind);

        return true;
    }

    // register_kind (move)
    bool
    register_kind(
        test_kind&& _kind
    )
    {
        // reject duplicate id
        if (contains(_kind.id))
        {
            return false;
        }

        m_kinds.push_back(static_cast<test_kind&&>(_kind));

        return true;
    }

    // -----------------------------------------------------------------
    //  lookup
    // -----------------------------------------------------------------

    // find
    //   returns a pointer to the test_kind with the given id,
    // or nullptr if absent.
    const test_kind*
    find(
        test_type_id _id
    ) const
    {
        for (size_type i = 0; i < m_kinds.size(); ++i)
        {
            if (m_kinds[i].id == _id)
            {
                return &m_kinds[i];
            }
        }

        return nullptr;
    }

    // find (mutable)
    test_kind*
    find(
        test_type_id _id
    )
    {
        for (size_type i = 0; i < m_kinds.size(); ++i)
        {
            if (m_kinds[i].id == _id)
            {
                return &m_kinds[i];
            }
        }

        return nullptr;
    }

    // contains
    //   returns true if a test_kind with _id is registered.
    bool
    contains(
        test_type_id _id
    ) const
    {
        return (find(_id) != nullptr);
    }

    // -----------------------------------------------------------------
    //  resolved queries
    // -----------------------------------------------------------------

    // rank_of
    //   returns the rank for _id.  If the id is registered,
    // returns the test_kind's rank.  Otherwise returns the
    // raw id cast to uint16_t (the fallback behavior when
    // no type registry is present).
    std::uint16_t
    rank_of(
        test_type_id _id
    ) const
    {
        const test_kind* k = find(_id);

        if (k)
        {
            return k->rank;
        }

        return static_cast<std::uint16_t>(_id);
    }

    // is_leaf
    //   returns true if _id maps to a leaf test_kind.  If
    // the id is unregistered, returns true (unresolved
    // objects default to leaf).
    bool
    is_leaf(
        test_type_id _id
    ) const
    {
        const test_kind* k = find(_id);

        if (k)
        {
            return k->is_leaf;
        }

        return true;
    }

    // is_interior
    //   complement of is_leaf.
    bool
    is_interior(
        test_type_id _id
    ) const
    {
        return !is_leaf(_id);
    }

    // name_of
    //   returns the name for _id, or nullptr if unregistered.
    const char*
    name_of(
        test_type_id _id
    ) const
    {
        const test_kind* k = find(_id);

        if (k)
        {
            return k->name;
        }

        return nullptr;
    }

    // default_options
    //   returns the default options pointer for _id, or
    // nullptr if unregistered or the kind has no defaults.
    const dtest_option_set*
    default_options(
        test_type_id _id
    ) const
    {
        const test_kind* k = find(_id);

        if (k)
        {
            return k->default_options;
        }

        return nullptr;
    }

    // -----------------------------------------------------------------
    //  tree insertion query
    // -----------------------------------------------------------------

    // can_be_child_of
    //   returns true if an object with _child_id may be
    // inserted as a child of an object with _parent_id.
    // The rule: child rank <= parent rank.  Both ids are
    // resolved through the registry; unmatched ids fall
    // back to the raw id as rank.
    bool
    can_be_child_of(
        test_type_id _child_id,
        test_type_id _parent_id
    ) const
    {
        return (rank_of(_child_id) <= rank_of(_parent_id));
    }

    // -----------------------------------------------------------------
    //  capacity
    // -----------------------------------------------------------------

    size_type
    size() const D_NOEXCEPT
    {
        return m_kinds.size();
    }

    bool
    empty() const D_NOEXCEPT
    {
        return m_kinds.empty();
    }

    // -----------------------------------------------------------------
    //  iteration
    // -----------------------------------------------------------------

    iterator       begin()        D_NOEXCEPT { return m_kinds.begin();  }
    const_iterator begin()  const D_NOEXCEPT { return m_kinds.begin();  }
    iterator       end()          D_NOEXCEPT { return m_kinds.end();    }
    const_iterator end()    const D_NOEXCEPT { return m_kinds.end();    }
    const_iterator cbegin() const D_NOEXCEPT { return m_kinds.cbegin(); }
    const_iterator cend()   const D_NOEXCEPT { return m_kinds.cend();   }

    // -----------------------------------------------------------------
    //  underlying access
    // -----------------------------------------------------------------

    const _Container&
    kinds() const D_NOEXCEPT
    {
        return m_kinds;
    }

    _Container&
    kinds() D_NOEXCEPT
    {
        return m_kinds;
    }

private:
    _Container m_kinds;
};


///////////////////////////////////////////////////////////////////////////////
///                III. FACTORY FUNCTION                                     ///
///////////////////////////////////////////////////////////////////////////////

// make_test_kind
//   function: convenience constructor for a test_kind entry.
D_CONSTEXPR_INLINE test_kind
make_test_kind(
    test_type_id            _id,
    const char*             _name,
    std::uint16_t           _rank,
    bool                    _is_leaf,
    const dtest_option_set* _default_options = nullptr
) D_NOEXCEPT
{
    test_kind k;
    k.id              = _id;
    k.name            = _name;
    k.rank            = _rank;
    k.is_leaf         = _is_leaf;
    k.default_options = _default_options;

    return k;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  TRAIT DETECTION                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_TRAITS

NS_INTERNAL

    // detect_rank_of_method
    //   helper: probes for .rank_of(test_type_id).
    template<typename _Type>
    using detect_rank_of_method = decltype(
        std::declval<const _Type&>().rank_of(
            std::declval<test_type_id>()));

    // detect_is_leaf_id_method
    //   helper: probes for .is_leaf(test_type_id).
    template<typename _Type>
    using detect_is_leaf_id_method = decltype(
        std::declval<const _Type&>().is_leaf(
            std::declval<test_type_id>()));

    // detect_can_be_child_of_method
    //   helper: probes for .can_be_child_of(id, id).
    template<typename _Type>
    using detect_can_be_child_of_method = decltype(
        std::declval<const _Type&>().can_be_child_of(
            std::declval<test_type_id>(),
            std::declval<test_type_id>()));

NS_END  // internal

// is_test_type_registry
//   trait: true if _Type satisfies the test type registry
// structural contract: rank_of(id), is_leaf(id), and
// can_be_child_of(id, id).
template<typename _Type,
         typename = void>
struct is_test_type_registry : std::false_type
{};

template<typename _Type>
struct is_test_type_registry<_Type, void_t<
    internal::detect_rank_of_method<_Type>,
    internal::detect_is_leaf_id_method<_Type>,
    internal::detect_can_be_child_of_method<_Type>
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_test_type_registry_v =
        is_test_type_registry<_Type>::value;
#endif

NS_END  // traits


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TYPE_
