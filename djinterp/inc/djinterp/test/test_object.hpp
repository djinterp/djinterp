/******************************************************************************
* djinterp [test]                                              test_object.hpp
*
*   Unified test object for the DTest framework.  A single flat struct
* holding the per-instance state of one test element: type identity,
* result, status, and a user-supplied metadata container.
*
*   TYPE IDENTITY:
*   Every test_object carries a test_type_id - a signed 32-bit integer
* that identifies the kind of test it represents.  Resolution of that
* id to a kind classification is handled outside test_object, via the
* test_kind set wrapper held elsewhere (typically by the tree).
*
*   IDENTITY AND DEPTH (NOT STORED):
*   A test_object carries neither a unique id nor a depth.  Both are
* facts of a node's position within the owning tree, not properties of
* the node itself: structural identity is the node's address (its path
* from the root) and depth is that address's length.  The owning test
* handler computes and tracks them during the tree walk; test_object is
* ignorant of either.  This mirrors the formal definition, in which a
* node is anonymous and its address - conferred by the owning queue -
* is the identity that masking and reporting key on.
*
*   DEFERRED-CALLABLE HANDLE:
*   A test_object carries a test_callable_id - a 1-based index into an
* out-of-line test_callable_table (test_callable.hpp) holding the node's
* deferred boolean work as a `() -> bool` thunk.  The node stays flat,
* trivially copyable, and constexpr-friendly precisely BECAUSE the closure
* does not live inline: the node references a table row by id only.  The
* reserved id k_no_callable (0) means "no deferred work" - such a node is
* already fully described by its m_result / m_status.  The table's owner
* (the builder in test_builder.hpp, or a handler) binds the thunk via the
* table, stores the issued id on the node with set_callable_id(), and
* later runs it with table.invoke(callable_id()) before writing the
* verdict onto the node via evaluate().
*
*   METADATA CONTAINER:
*   In the previous revision the test_object held two non-owning
* pointers - a const char* name and a const dtest_option_set* options
* pointer - inline.  Both have been removed.  In their place is a
* user-supplied _MetadataContainer member that adheres to the
* metadata_traits / metadata_concepts protocol:
*     - metadata_container_type   nested alias
*     - metadata_type             nested alias
*     - metadata()                accessor returning the container
*
*   The user chooses whatever shape of metadata best fits their
* suite: a flat string for names, a small struct holding name +
* options, a flat_map keyed by string tag, a set of integer tags,
* etc.  test_object stores it directly and exposes it through the
* canonical metadata protocol.
*
*   No assumptions about metadata content are baked in.  The
* default `std::vector<std::int32_t>` is a generic placeholder;
* callers that need name + options should substitute a metadata
* container tailored to that shape.
*
*   TEST OBJECT PROTOCOL:
*   test_object satisfies the test object protocol detected by
* test_object_traits.hpp:
*     - operator bool()    boolean conversion (result)
*     - status()           status accessor
*     - result()           raw result accessor
*     - type_id()          test_type_id accessor
*     - callable_id()      deferred-callable id accessor
*     - metadata()         metadata container accessor
*
*   Naming, messaging, and option access - if needed - live on the
* metadata container the user supplied, not on test_object itself.
*
*   CONSTEXPR:
*   Accessors and simple mutators are constexpr where the metadata
* container permits.  Evaluation is D_TEST_CONSTEXPR (constexpr on
* C++14+).
*
*   PORTABILITY:
*   C++11 minimum.
*
*
* TABLE OF CONTENTS
* =================
* I.    test object
* II.   convenience aliases
* III.  factory functions
*
*
* path:      /inc/djinterp/test/test_object.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.14
******************************************************************************/

#ifndef DJINTERP_TEST_OBJECT_
#define DJINTERP_TEST_OBJECT_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/meta/kv_pair.hpp"
#include "./test_common.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST OBJECT                                         ///
///////////////////////////////////////////////////////////////////////////////

// test_metadata
//   class: minimal runtime key/value metadata store for a test node -
// a flat list of kv_pairs queried and updated by string key.  Just
// enough to carry name / message strings on a test_object until the
// fuller key-value metadata design lands.  It is the default metadata
// container for test_object (and therefore for basic_test), so it must
// be a complete type before the test_object template below.  Satisfies
// test_object's container requirement: it exposes a `value_type` (the
// row), from which test_object derives metadata_type.
//
//   NOTE: lookup is a linear scan over the entry list comparing the
// key strings directly; it deliberately never invokes kv_pair's
// relational operators (which are presently ill-formed when
// instantiated), constructing rows through the value constructor only.
class test_metadata
{
public:
    // value_type
    //   type: one metadata row, a string key / string value pair.
    using value_type = ::djinterp::kv_pair<std::string, std::string>;

    test_metadata() = default;

    // set
    //   inserts or overwrites the value stored under _key.
    void
    set(
        const std::string& _key,
        const std::string& _value
    )
    {
        std::size_t i;

        for (i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].m_key == _key)
            {
                m_entries[i].m_value = _value;

                return;
            }
        }

        m_entries.push_back(value_type(_key, _value));

        return;
    }

    // get
    //   returns the value stored under _key, or an empty string if the
    // key is absent.
    std::string
    get(
        const std::string& _key
    ) const
    {
        std::size_t i;

        for (i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].m_key == _key)
            {
                return m_entries[i].m_value;
            }
        }

        return std::string();
    }

    // contains
    //   true iff an entry is stored under _key.
    bool
    contains(
        const std::string& _key
    ) const
    {
        std::size_t i;

        for (i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].m_key == _key)
            {
                return true;
            }
        }

        return false;
    }

    // size
    //   number of stored entries.
    std::size_t
    size() const D_NOEXCEPT
    {
        return m_entries.size();
    }

private:
    std::vector<value_type> m_entries;
};


// test_object
//   struct: unified test element.  Holds type identity, result,
// status, and a user-supplied metadata container.
//
//   Naming, message, options, tags - any classification not
// covered by the explicit members - live on the metadata
// container the user picks via _MetadataContainer.
//
//   Structural classification (rank, leaf/interior) is NOT
// stored here.  It is resolved through a test_kind set held by
// the tree; in isolation, the test_type_id acts as the rank.
//
//   Structural POSITION (address, depth) is NOT stored here
// either.  A node is anonymous: it carries no unique id and no
// depth.  Identity is the node's address and depth is that
// address's length, both conferred and tracked by the owning
// test handler as it walks the tree.  test_object is ignorant
// of either.
//
//   DEFERRED WORK: a test_object carries a test_callable_id
// referencing its deferred thunk in an out-of-line
// test_callable_table; k_no_callable means none.  The node
// stores only the id, never the closure, so it stays flat and
// trivially copyable.
//
// Template parameters:
//   _StatusType        - arithmetic type for status codes.
//                        Default: std::uint8_t.
//   _MetadataContainer - any type adhering to metadata_traits /
//                        metadata_concepts.  Stored in-line and
//                        exposed through metadata().
//                        Default: test_metadata.
//   _Options...        - tail option-policy pack (reserved for
//                        per-instantiation policy mix-in).
//
// Usage:
//   test_object<> t(42);
//   t.evaluate(1 + 1 == 2);
//   t.metadata().set("name", "my test");
//   if (t.passed()) { ... }
template<typename    _StatusType        = std::uint8_t,
         typename    _MetadataContainer = test_metadata,
         typename... _Options>
struct test_object
{
    static_assert(std::is_arithmetic<_StatusType>::value,
                  "`_StatusType` must be an arithmetic type.");

    //  type aliases
    using status_type             = _StatusType;

    // metadata protocol exposure
    //   These aliases satisfy metadata_traits and the
    // metadata_concepts protocol: any consumer querying for
    // metadata_container_type_t / metadata_type_t against a
    // test_object instantiation will find them here.
    using metadata_container_type = _MetadataContainer;
    using metadata_type           = typename _MetadataContainer::value_type;

    // status constants
    static D_CONSTEXPR status_type status_passed  = static_cast<status_type>(0);
    static D_CONSTEXPR status_type status_failed  = static_cast<status_type>(1);
    static D_CONSTEXPR status_type status_skipped = static_cast<status_type>(2);
    static D_CONSTEXPR status_type status_pending = static_cast<status_type>(3);
    static D_CONSTEXPR status_type status_error   = static_cast<status_type>(4);

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default: pending, type id 0, default-constructed metadata
    D_CONSTEXPR test_object() D_NOEXCEPT_IF(
        std::is_nothrow_default_constructible<metadata_container_type>::value)
        : m_result(false),
          m_status(status_pending),
          m_type_id(0),
          m_callable_id(k_no_callable),
          m_metadata()
    {}

    // from type id
    D_CONSTEXPR explicit test_object(
        test_type_id _type_id
    ) D_NOEXCEPT_IF(
        std::is_nothrow_default_constructible<
            metadata_container_type>::value)
        : m_result(false),
          m_status(status_pending),
          m_type_id(_type_id),
          m_callable_id(k_no_callable),
          m_metadata()
    {}

    // from type id and result
    D_CONSTEXPR test_object(
        test_type_id _type_id,
        bool         _result
    ) D_NOEXCEPT_IF(std::is_nothrow_default_constructible<metadata_container_type>::value)
        : m_result(_result),
          m_status(_result ? status_passed : status_failed),
          m_type_id(_type_id),
          m_callable_id(k_no_callable),
          m_metadata()
    {}

    // from type id, result, and metadata (copy)
    D_CONSTEXPR test_object(
        test_type_id                   _type_id,
        bool                           _result,
        const metadata_container_type& _metadata
    )
        : m_result(_result),
          m_status(_result ? status_passed : status_failed),
          m_type_id(_type_id),
          m_callable_id(k_no_callable),
          m_metadata(_metadata)
    {}

    // from type id, result, and metadata (move)
    D_CONSTEXPR test_object(
        test_type_id              _type_id,
        bool                      _result,
        metadata_container_type&& _metadata
    ) D_NOEXCEPT_IF(
        std::is_nothrow_move_constructible<
            metadata_container_type>::value)
        : m_result(_result),
          m_status(_result ? status_passed : status_failed),
          m_type_id(_type_id),
          m_callable_id(k_no_callable),
          m_metadata(static_cast<metadata_container_type&&>(_metadata))
    {}


    // -----------------------------------------------------------------
    //  test object protocol
    // -----------------------------------------------------------------

    D_CONSTEXPR
    operator bool() const D_NOEXCEPT
    {
        return m_result;
    }

    D_CONSTEXPR status_type
    status() const D_NOEXCEPT
    {
        return m_status;
    }

    D_CONSTEXPR bool
    result() const D_NOEXCEPT
    {
        return m_result;
    }

    D_CONSTEXPR bool
    passed() const D_NOEXCEPT
    {
        return (m_status == status_passed);
    }

    D_CONSTEXPR bool
    failed() const D_NOEXCEPT
    {
        return (m_status == status_failed);
    }


    // -----------------------------------------------------------------
    //  evaluation
    // -----------------------------------------------------------------

    // evaluate
    //   sets the result and derives status from it.
    D_TEST_CONSTEXPR void
    evaluate(
        bool _result
    ) D_NOEXCEPT
    {
        m_result = _result;
        m_status = _result ? status_passed : status_failed;

        return;
    }

    // skip
    //   marks this test as intentionally skipped.
    D_TEST_CONSTEXPR void
    skip() D_NOEXCEPT
    {
        m_result = false;
        m_status = status_skipped;

        return;
    }

    // set_status
    D_TEST_CONSTEXPR void
    set_status(
        status_type _status
    ) D_NOEXCEPT
    {
        m_status = _status;

        return;
    }


    // -----------------------------------------------------------------
    //  type identity
    // -----------------------------------------------------------------

    // type_id
    //   returns the test_type_id for this object.  In isolation
    // this doubles as a numeric rank for tree ordering; when a
    // test_kind set is attached to the tree the id resolves
    // through it.
    D_CONSTEXPR test_type_id
    type_id() const D_NOEXCEPT
    {
        return m_type_id;
    }

    // set_type_id
    D_TEST_CONSTEXPR void
    set_type_id(
        test_type_id _type_id
    ) D_NOEXCEPT
    {
        m_type_id = _type_id;

        return;
    }


    // -----------------------------------------------------------------
    //  deferred-callable handle
    // -----------------------------------------------------------------

    // callable_id
    //   returns the id of this node's deferred thunk in the
    // out-of-line test_callable_table, or k_no_callable when the
    // node carries no deferred work.
    D_CONSTEXPR test_callable_id
    callable_id() const D_NOEXCEPT
    {
        return m_callable_id;
    }

    // has_callable
    //   true iff this node references a deferred thunk (its id is
    // not the k_no_callable sentinel).
    D_CONSTEXPR bool
    has_callable() const D_NOEXCEPT
    {
        return (m_callable_id != k_no_callable);
    }

    // set_callable_id
    //   binds this node to a table row by id.  The id is the value
    // issued by test_callable_table::add(); pass k_no_callable to
    // detach the node from any deferred work.
    D_TEST_CONSTEXPR void
    set_callable_id(
        test_callable_id _id
    ) D_NOEXCEPT
    {
        m_callable_id = _id;

        return;
    }


    // -----------------------------------------------------------------
    //  metadata access (metadata_traits / metadata_concepts protocol)
    // -----------------------------------------------------------------

    // metadata
    //   returns a mutable reference to the metadata container.
    // The container's shape is entirely user-controlled; this
    // type makes no assumptions about its contents.
    D_TEST_CONSTEXPR metadata_container_type&
    metadata() D_NOEXCEPT
    {
        return m_metadata;
    }

    // metadata (const)
    D_CONSTEXPR const metadata_container_type&
    metadata() const D_NOEXCEPT
    {
        return m_metadata;
    }

    // set_metadata
    //   assigns a new metadata container by copy.
    void
    set_metadata(
        const metadata_container_type& _metadata
    )
    {
        m_metadata = _metadata;

        return;
    }

    // set_metadata
    //   assigns a new metadata container by move.
    void
    set_metadata(
        metadata_container_type&& _metadata
    ) D_NOEXCEPT_IF(
        std::is_nothrow_move_assignable<
            metadata_container_type>::value)
    {
        m_metadata = static_cast<metadata_container_type&&>(_metadata);

        return;
    }


    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    bool                    m_result;
    status_type             m_status;
    test_type_id            m_type_id;
    test_callable_id        m_callable_id;
    metadata_container_type m_metadata;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  CONVENIENCE ALIASES                                  ///
///////////////////////////////////////////////////////////////////////////////

// basic_test
//   type: default test object - uint8 status and the default
// test_metadata key/value container carrying name / message strings.
// Equivalent to test_object<>, so it stays the same type as any
// test_object<uint8> signature elsewhere in the framework.
using basic_test = test_object<>;

// tagged_test
//   type: test object using a plain integer-tag list as its metadata
// container instead of the default key/value store - for suites that
// classify nodes by numeric tag rather than by string key.
using tagged_test = test_object<std::uint8_t,
                                std::vector<std::int32_t>>;


///////////////////////////////////////////////////////////////////////////////
///                III. FACTORY FUNCTIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// make_test
//   function: creates a basic_test from a type id and result.
D_INLINE basic_test
make_test(
    test_type_id _type_id,
    bool         _result
) D_NOEXCEPT
{
    return basic_test(_type_id, _result);
}

// make_interior
//   function: creates a pending object of the given type.
// The caller is responsible for ensuring _type_id corresponds
// to an interior kind in their test_kind set.
D_INLINE basic_test
make_interior(
    test_type_id _type_id
) D_NOEXCEPT
{
    basic_test t(_type_id);
    t.set_status(basic_test::status_pending);

    return t;
}

// make_interior (named)
//   function: creates a pending object of the given type and stores
// _name under the "name" metadata key.  Not D_NOEXCEPT: setting a
// metadata entry may allocate.
D_INLINE basic_test
make_interior(
    test_type_id _type_id,
    const char*  _name
)
{
    basic_test t(_type_id);
    t.set_status(basic_test::status_pending);

    if (_name != nullptr)
    {
        t.metadata().set("name", _name);
    }

    return t;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_OBJECT_