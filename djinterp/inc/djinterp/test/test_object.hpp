/******************************************************************************
* djinterp [test]                                              test_object.hpp
*
*   The C++ face of the test node.  The template, the metadata container, the
* protocol trait and the make_* helpers -- everything that was notation --
* over the shared kernel that holds everything that was not.
*
*
* WHAT STAYED HERE
* ================
*   test_object<_StatusType, _MetadataContainer, _Options...>.  The template
* is the notation: _StatusType is a range restriction on the kernel's status
* (goals §11 -- a narrower status is the same object, not a different one),
* _MetadataContainer is the protocol point, and _Options is a reserved policy
* pack.  C gets one monomorphised node, which is what §11 says it should have.
*
*   basic_metadata<_Key, _Value, _Container>.  Agnostic over key, value and
* backing container, defaulting to std::string / std::string / std::vector.
* The kernel's d_test_metadata is the SAME protocol -- a back-insertable row
* sequence with linear-scan lookup -- with borrowed rows instead of owned
* ones, which is what lets the node precede d_string at step J.
*
*   is_test_evaluable.  A detection trait over the element protocol.  Pure
* notation; C expresses the same contract by the kernel's function set being
* the only way to touch a node.
*
*
* THE TWO METADATA SHAPES ARE ONE PROTOCOL, AND THE RECORD MUST PROVE IT
* ======================================================================
*   basic_metadata<> owns its strings; d_test_metadata borrows them.  On every
* question the protocol asks -- insertion order, replace-in-place, what a miss
* returns, what an empty container reports -- they must answer identically,
* and the step F parity body is where that stops being a claim.
*
*   The interesting one is the miss.  basic_metadata::get returns a
* value-initialised mapped_type, which for std::string is the empty string;
* the kernel returns D_TEST_METADATA_MISS, which is the empty string.  They
* agree, and they agree because both were written to -- not because one calls
* the other.  That is exactly the shape §3 warns about, so it is recorded
* rather than assumed.
*
*
* path:      /inc/djinterp/test/test_object.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.14
*                                                          coalesced: 2026.07.31
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_OBJECT_
#define DJINTERP_TEST_TEST_OBJECT_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <type_traits>
// djinterp
#include "../core/djinterp.hpp"
#include "../c/test/test_object.h"
#include "../c/test/test_common.h"
//   test_metadata.hpp IS DELIBERATELY NOT INCLUDED.  It had to be, because a
// default template argument _MetadataContainer = test_metadata must name a
// COMPLETE type -- which is the whole mechanism that made metadata optional in
// C and mandatory in C++. With the association moved to the tree there is no
// such argument, the include goes, and D_CFG_TEST_METADATA=0 now turns the
// module off in BOTH forks. Nothing here is conditional on it, so there is no
// knob-dependent template identity either.


NS_DJINTERP
NS_TEST


// =============================================================================
// II.  THE NODE
// =============================================================================

// test_object
//   struct: one test element.  Holds the kernel's node for the four scalars
// and a user-supplied metadata container beside it.
//
//   THE SCALARS LIVE IN THE KERNEL AND THE METADATA DOES NOT, and that split
// is the whole design: the scalars are what the layout law is stated over and
// what a parity record compares, while the container is a policy point whose
// representation is allowed to differ between the languages so long as its
// protocol does not.
template<typename    _StatusType = std::uint8_t,
         typename... _Options>
struct test_object
{
    static_assert(std::is_arithmetic<_StatusType>::value,
                  "`_StatusType` must be an arithmetic type.");

    typedef _StatusType                     status_type;

    //   The status constants, now READ FROM the shared macros rather than
    // written again.  The previous revision declared them independently and
    // nothing compared the two sets; test_object.h asserts them, and
    // taking them from the same macros here means there is no second place
    // for them to drift from.
    static D_CONSTEXPR status_type status_passed  =
        static_cast<status_type>(D_TEST_STATUS_PASSED);
    static D_CONSTEXPR status_type status_failed  =
        static_cast<status_type>(D_TEST_STATUS_FAILED);
    static D_CONSTEXPR status_type status_skipped =
        static_cast<status_type>(D_TEST_STATUS_SKIPPED);
    static D_CONSTEXPR status_type status_pending =
        static_cast<status_type>(D_TEST_STATUS_PENDING);
    static D_CONSTEXPR status_type status_error   =
        static_cast<status_type>(D_TEST_STATUS_ERROR);

    test_object() { d_test_object_init(&m_node); }

    explicit test_object(d_test_type_id _type_id)
    { d_test_object_init_typed(&m_node, _type_id); }

    test_object(d_test_type_id _type_id, bool _result)
    {
        d_test_object_init_typed(&m_node, _type_id);
        d_test_object_evaluate(&m_node, _result ? 1 : 0);
    }

    // evaluate
    //   writes the verdict and derives passed / failed from it.
    void evaluate(bool _result)
    { d_test_object_evaluate(&m_node, _result ? 1 : 0); }

    void set_status(status_type _status)
    { d_test_object_set_status(&m_node, static_cast<std::int32_t>(_status)); }

    void set_type_id(d_test_type_id _id)
    { d_test_object_set_type_id(&m_node, _id); }

    void set_callable_id(d_test_callable_id _id)
    { d_test_object_set_callable_id(&m_node, _id); }

    D_NODISCARD bool result() const
    { return d_test_object_result_of(&m_node) != 0; }

    D_NODISCARD status_type status() const
    { return static_cast<status_type>(d_test_object_status(&m_node)); }

    D_NODISCARD d_test_type_id type_id() const { return m_node.type_id; }

    D_NODISCARD d_test_callable_id callable_id() const
    { return m_node.callable_id; }

    D_NODISCARD bool passed() const
    { return d_test_object_passed(&m_node) != 0; }

    D_NODISCARD bool is_deferred() const
    { return d_test_object_is_deferred(&m_node) != 0; }

    explicit operator bool() const { return result(); }

    //   NO metadata() ACCESSOR.  The node no longer owns or refers to a
    // container; the owning tree keeps the association and hands it out with
    // whatever ownership it likes -- which, on this side, means RAII rather
    // than the borrowed pointer the C fork uses.

    D_NODISCARD d_test_object*       raw()       { return &m_node; }
    D_NODISCARD const d_test_object* raw() const { return &m_node; }

private:
    d_test_object           m_node;
};


// =============================================================================
// III. THE ELEMENT PROTOCOL
// =============================================================================

// is_test_evaluable
//   trait: true iff _Type is CONTEXTUALLY CONVERTIBLE TO BOOL on a const
// lvalue.  That is the whole of the check, and the whole of what this trait
// promises.
//
//   IT DOES NOT PROBE status(), result(), type_id(), callable_id() OR
// metadata(), and this comment used to say it did.  The consequence of the
// wider claim was concrete: `is_test_evaluable<int>` is true, so a container
// of ints satisfies `is_test_object_container`, a runner accepts it, and the
// call to status() fails somewhere else entirely -- with a comment here
// asserting that could not happen.
//
//   THE CHECK IS NOT VACUOUS.  A type with no boolean conversion is correctly
// rejected, which is what `test/contract/container_ladder.cpp` pins from both
// sides.  Narrowing it to the full six-member protocol would change what
// qualifies across every consumer of test_tree and needs a consumer audit
// first; until then this comment describes the code rather than the intent.
//
//   Probed on a const lvalue, so a const-qualified element agrees with its
// bare form.
template<typename _Type, typename = void>
struct is_test_evaluable : std::false_type {};

template<typename _Type>
struct is_test_evaluable<
    _Type,
    typename std::enable_if<
        std::is_same<
            decltype(static_cast<bool>(std::declval<const _Type&>())), bool
        >::value
    >::type
> : std::true_type {};


// basic_test
//   type: the default node -- uint8 status and the default key/value store.
typedef test_object<> basic_test;


// =============================================================================
// IV.  CONSTRUCTION HELPERS
// =============================================================================

D_NODISCARD inline basic_test
make_test(d_test_type_id _type_id, bool _result)
{
    return basic_test(_type_id, _result);
}

D_NODISCARD inline basic_test
make_interior(d_test_type_id _type_id)
{
    return basic_test(_type_id);
}

//   make_interior(type_id, name) IS GONE, not moved.  It built a node and
// then wrote the name into the node's own metadata container. With no such
// container there is nothing here for it to write to, and a version that
// reached into the tree would put a tree operation in an object header. The
// naming of an interior node belongs with whatever owns the association.


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TEST_OBJECT_
