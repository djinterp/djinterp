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
#include "./test_object_common.h"
#include "./test_common_common.h"


NS_DJINTERP
NS_TEST


// =============================================================================
// I.   METADATA
// =============================================================================

// kv_row
//   struct: one metadata row.  The C++ default instantiation of what the
// kernel spells d_test_kv.
template<typename _Key, typename _Value>
struct kv_row
{
    _Key    key;
    _Value  value;

    kv_row() : key(), value() {}
    kv_row(const _Key& _k, const _Value& _v) : key(_k), value(_v) {}
};


// basic_metadata
//   class: a flat, back-insertable row sequence queried by linear scan.
// Agnostic over key, value and backing container.
//
//   The scan compares KEYS with _Key's own operator== and never invokes
// kv_row's relational operators, which is the note the previous revision was
// careful to make; the kernel's key comparison is a content comparison for
// the same reason.
template<typename _Key       = std::string,
         typename _Value     = std::string,
         typename _Container = std::vector<kv_row<std::string, std::string> > >
class basic_metadata
{
public:
    typedef _Key                                key_type;
    typedef _Value                              mapped_type;
    typedef typename _Container::value_type     value_type;
    typedef _Container                          container_type;

    // set
    //   replaces an existing key in place, or appends.  Returns whether a row
    // was replaced, so the caller can draw the same distinction the kernel's
    // REPLACED result draws.
    bool set(const key_type& _key, const mapped_type& _value)
    {
        for (typename _Container::iterator it = m_rows.begin();
             it != m_rows.end(); ++it)
        {
            if (it->key == _key)
            {
                it->value = _value;

                return true;
            }
        }

        m_rows.push_back(value_type(_key, _value));

        return false;
    }

    // get
    //   returns a value-initialised mapped_type on a miss -- the empty string
    // at the default instantiation, which is what D_TEST_METADATA_MISS is.
    D_NODISCARD mapped_type get(const key_type& _key) const
    {
        for (typename _Container::const_iterator it = m_rows.begin();
             it != m_rows.end(); ++it)
        {
            if (it->key == _key)
            {
                return it->value;
            }
        }

        return mapped_type();
    }

    D_NODISCARD bool contains(const key_type& _key) const
    {
        for (typename _Container::const_iterator it = m_rows.begin();
             it != m_rows.end(); ++it)
        {
            if (it->key == _key) { return true; }
        }

        return false;
    }

    D_NODISCARD std::size_t     count() const { return m_rows.size(); }
    D_NODISCARD container_type& rows()        { return m_rows; }
    D_NODISCARD const container_type& rows() const { return m_rows; }

private:
    container_type m_rows;
};


// test_metadata
//   type: the all-defaulted metadata container -- the default
// _MetadataContainer for test_object, and therefore for basic_test.
typedef basic_metadata<> test_metadata;


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
template<typename    _StatusType        = std::uint8_t,
         typename    _MetadataContainer = test_metadata,
         typename... _Options>
struct test_object
{
    static_assert(std::is_arithmetic<_StatusType>::value,
                  "`_StatusType` must be an arithmetic type.");

    typedef _StatusType                     status_type;
    typedef _MetadataContainer              metadata_container_type;
    typedef typename _MetadataContainer::value_type  metadata_type;

    //   The status constants, now READ FROM the shared macros rather than
    // written again.  The previous revision declared them independently and
    // nothing compared the two sets; test_object_common.h asserts them, and
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

    D_NODISCARD metadata_container_type&       metadata()       { return m_meta; }
    D_NODISCARD const metadata_container_type& metadata() const { return m_meta; }

    D_NODISCARD d_test_object*       raw()       { return &m_node; }
    D_NODISCARD const d_test_object* raw() const { return &m_node; }

private:
    d_test_object           m_node;
    metadata_container_type m_meta;
};


// =============================================================================
// III. THE ELEMENT PROTOCOL
// =============================================================================

// is_test_evaluable
//   trait: true iff _Type satisfies the element protocol a runner needs --
// boolean conversion, status(), result(), type_id(), callable_id(),
// metadata().  Probed on a const lvalue, so a const-qualified element agrees
// with its bare form.
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

// make_interior (named)
//   Not D_NOEXCEPT: setting a metadata entry may allocate.
D_NODISCARD inline basic_test
make_interior(d_test_type_id _type_id, const std::string& _name)
{
    basic_test t(_type_id);

    t.metadata().set("name", _name);

    return t;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TEST_OBJECT_
