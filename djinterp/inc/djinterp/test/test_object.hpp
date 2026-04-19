/******************************************************************************
* djinterp [test]                                             test_object.hpp
*
*   Unified test object for the DTest framework.  A single flat struct
* holding the per-instance state of one test element: type identity,
* result, status, name, message, and optional per-instance option
* overrides.
*
*   TYPE IDENTITY:
*   Every test_object carries a test_type_id — a signed 32-bit integer
* that identifies the kind of test it represents.  The type id is the
* object's only link to the test_type registry held by the tree.
*
*   In isolation (no registry), the id acts as a numeric rank: a
* child's id must be <= its parent's.  Unregistered objects are
* treated as leaves with no default options.
*
*   When the tree holds a test_type registry, a matching id resolves
* to its test_kind, which supplies rank, leaf/interior classification,
* and default options.  The test_object itself does not cache these
* derived properties — they are resolved at query time through the
* registry.
*
*   OPTIONS:
*   A test_object may hold a non-owning pointer to a dtest_option_set
* for per-instance option overrides.  When non-null, this set takes
* precedence over the test_kind's defaults, which in turn take
* precedence over the global defaults.  When null, the test_kind's
* defaults (if any) apply directly.
*
*   NO BUILT-IN KINDS:
*   This header defines no kind constants or kind-specific factory
* functions.  The vocabulary of test classifications is entirely
* user-defined.  See test_defaults.hpp for the framework's default
* kind set and convenience factories.
*
*   TEST OBJECT PROTOCOL:
*   test_object satisfies the test object protocol detected by
* test_object_traits.hpp:
*     - operator bool()    boolean conversion (result)
*     - status()           status accessor
*     - name()             name accessor (const char*)
*     - message()          message accessor (const char*)
*     - result()           raw result accessor
*
*   CONSTEXPR:
*   All accessors and simple mutators are constexpr.  Evaluation
* (which may fire events in a future extension) is D_TEST_CONSTEXPR
* (constexpr on C++14+).
*
*   PORTABILITY:
*   C++11 minimum.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST OBJECT
* II.   CONVENIENCE ALIASES
* III.  FACTORY FUNCTIONS
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
#include <type_traits>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"
#include "./test_options.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST OBJECT                                         ///
///////////////////////////////////////////////////////////////////////////////

// test_object
//   struct: unified test element.  Holds type identity,
// result, status, name, message, unique id, depth cache,
// and an optional per-instance options pointer.
//
// Structural classification (rank, leaf/interior) is NOT
// stored here — it is resolved through the test_type
// registry at the tree level.  In isolation, the
// test_type_id itself acts as the rank.
//
// Usage:
//   test_object<> t(42);
//   t.set_name("basic assertion");
//   t.evaluate(1 + 1 == 2);
//   if (t.passed()) { ... }
template<typename _StatusType = std::uint8_t,
         typename _IdType     = std::uint32_t>
struct test_object
{
    static_assert(std::is_arithmetic<_StatusType>::value,
                  "`_StatusType` must be an arithmetic type.");
    static_assert(std::is_arithmetic<_IdType>::value,
                  "`_IdType` must be an arithmetic type.");

    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------
    using status_type     = _StatusType;
    using id_type         = _IdType;
    using size_type       = std::size_t;
    using option_set_type = dtest_option_set;

    // status constants
    static D_CONSTEXPR status_type status_passed  = static_cast<status_type>(0);
    static D_CONSTEXPR status_type status_failed  = static_cast<status_type>(1);
    static D_CONSTEXPR status_type status_skipped = static_cast<status_type>(2);
    static D_CONSTEXPR status_type status_pending = static_cast<status_type>(3);
    static D_CONSTEXPR status_type status_error   = static_cast<status_type>(4);

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default: pending, type id 0, unnamed
    D_CONSTEXPR test_object() D_NOEXCEPT
        : m_result(false),
          m_status(status_pending),
          m_type_id(0),
          m_id(static_cast<id_type>(0)),
          m_name(nullptr),
          m_message(nullptr),
          m_options(nullptr),
          m_depth(0)
    {}

    // from type id
    D_CONSTEXPR explicit test_object(
        test_type_id _type_id
    ) D_NOEXCEPT
        : m_result(false),
          m_status(status_pending),
          m_type_id(_type_id),
          m_id(static_cast<id_type>(0)),
          m_name(nullptr),
          m_message(nullptr),
          m_options(nullptr),
          m_depth(0)
    {}

    // from type id and result
    D_CONSTEXPR test_object(
        test_type_id _type_id,
        bool         _result
    ) D_NOEXCEPT
        : m_result(_result),
          m_status(_result ? status_passed : status_failed),
          m_type_id(_type_id),
          m_id(static_cast<id_type>(0)),
          m_name(nullptr),
          m_message(nullptr),
          m_options(nullptr),
          m_depth(0)
    {}

    // from type id, result, and name
    D_CONSTEXPR test_object(
        test_type_id _type_id,
        bool         _result,
        const char*  _name
    ) D_NOEXCEPT
        : m_result(_result),
          m_status(_result ? status_passed : status_failed),
          m_type_id(_type_id),
          m_id(static_cast<id_type>(0)),
          m_name(_name),
          m_message(nullptr),
          m_options(nullptr),
          m_depth(0)
    {}

    // from type id, result, name, and message
    D_CONSTEXPR test_object(
        test_type_id _type_id,
        bool         _result,
        const char*  _name,
        const char*  _message
    ) D_NOEXCEPT
        : m_result(_result),
          m_status(_result ? status_passed : status_failed),
          m_type_id(_type_id),
          m_id(static_cast<id_type>(0)),
          m_name(_name),
          m_message(_message),
          m_options(nullptr),
          m_depth(0)
    {}

    // from type id, result, name, pass message, fail message
    D_CONSTEXPR test_object(
        test_type_id _type_id,
        bool         _result,
        const char*  _name,
        const char*  _message_pass,
        const char*  _message_fail
    ) D_NOEXCEPT
        : m_result(_result),
          m_status(_result ? status_passed : status_failed),
          m_type_id(_type_id),
          m_id(static_cast<id_type>(0)),
          m_name(_name),
          m_message(_result ? _message_pass : _message_fail),
          m_options(nullptr),
          m_depth(0)
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
    D_CONSTEXPR void
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
    //   returns the test_type_id for this object.  When no
    // test_type registry is present, this also serves as
    // the numeric rank for tree ordering.
    D_CONSTEXPR test_type_id
    type_id() const D_NOEXCEPT
    {
        return m_type_id;
    }

    // set_type_id
    D_CONSTEXPR void
    set_type_id(
        test_type_id _type_id
    ) D_NOEXCEPT
    {
        m_type_id = _type_id;

        return;
    }

    // -----------------------------------------------------------------
    //  naming and identity
    // -----------------------------------------------------------------

    D_CONSTEXPR const char*
    name() const D_NOEXCEPT
    {
        return m_name;
    }

    D_CONSTEXPR void
    set_name(
        const char* _name
    ) D_NOEXCEPT
    {
        m_name = _name;

        return;
    }

    D_CONSTEXPR const char*
    message() const D_NOEXCEPT
    {
        return m_message;
    }

    D_CONSTEXPR void
    set_message(
        const char* _message
    ) D_NOEXCEPT
    {
        m_message = _message;

        return;
    }

    D_CONSTEXPR id_type
    id() const D_NOEXCEPT
    {
        return m_id;
    }

    D_CONSTEXPR void
    set_id(
        id_type _id
    ) D_NOEXCEPT
    {
        m_id = _id;

        return;
    }

    // -----------------------------------------------------------------
    //  options (per-instance overrides)
    // -----------------------------------------------------------------

    // options
    //   returns the non-owning pointer to per-instance option
    // overrides, or nullptr if no overrides are set.
    D_CONSTEXPR const dtest_option_set*
    options() const D_NOEXCEPT
    {
        return m_options;
    }

    // set_options
    //   attaches or detaches per-instance option overrides.
    // Pass nullptr to clear overrides.  The caller is
    // responsible for the lifetime of the pointed-to set.
    D_CONSTEXPR void
    set_options(
        const dtest_option_set* _options
    ) D_NOEXCEPT
    {
        m_options = _options;

        return;
    }

    // has_options
    D_CONSTEXPR bool
    has_options() const D_NOEXCEPT
    {
        return (m_options != nullptr);
    }

    // -----------------------------------------------------------------
    //  depth (computed by tree, stored as external cache)
    // -----------------------------------------------------------------

    D_CONSTEXPR size_type
    depth() const D_NOEXCEPT
    {
        return m_depth;
    }

    D_CONSTEXPR void
    set_depth(
        size_type _depth
    ) D_NOEXCEPT
    {
        m_depth = _depth;

        return;
    }

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    bool                    m_result;
    status_type             m_status;
    test_type_id            m_type_id;
    id_type                 m_id;
    const char*             m_name;
    const char*             m_message;
    const dtest_option_set* m_options;
    size_type               m_depth;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  CONVENIENCE ALIASES                                  ///
///////////////////////////////////////////////////////////////////////////////

// basic_test
//   type: default test object with uint8 status, uint32 id.
using basic_test = test_object<>;

// compact_test
//   type: test object with minimal storage — uint8 status,
// uint16 id.
using compact_test = test_object<std::uint8_t,
                                  std::uint16_t>;

// wide_test
//   type: test object with large id space — uint8 status,
// uint64 id.
using wide_test = test_object<std::uint8_t,
                               std::uint64_t>;


///////////////////////////////////////////////////////////////////////////////
///                III. FACTORY FUNCTIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// make_test
//   function: creates a basic_test from a type id and result.
D_CONSTEXPR_INLINE basic_test
make_test(
    test_type_id _type_id,
    bool         _result
) D_NOEXCEPT
{
    return basic_test(_type_id, _result);
}

// make_named_test
//   function: creates a named test from type id, result,
// and name.
D_CONSTEXPR_INLINE basic_test
make_named_test(
    test_type_id _type_id,
    bool         _result,
    const char*  _name
) D_NOEXCEPT
{
    return basic_test(_type_id, _result, _name);
}

// make_test_with_message
//   function: creates a basic_test with conditional messages.
D_CONSTEXPR_INLINE basic_test
make_test_with_message(
    test_type_id _type_id,
    bool         _result,
    const char*  _name,
    const char*  _message_pass,
    const char*  _message_fail
) D_NOEXCEPT
{
    return basic_test(
        _type_id,
        _result,
        _name,
        _message_pass,
        _message_fail);
}

// make_interior
//   function: creates a pending object of the given type.
// The caller is responsible for ensuring _type_id
// corresponds to an interior kind when a test_type
// registry is present.
D_CONSTEXPR_INLINE basic_test
make_interior(
    test_type_id _type_id,
    const char*  _name
) D_NOEXCEPT
{
    basic_test t(_type_id);
    t.set_name(_name);
    t.set_status(basic_test::status_pending);

    return t;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_OBJECT_
