/******************************************************************************
* djinterp [test]                                            test_metadata.hpp
*
*   The C++ metadata container: kv_row<>, basic_metadata<> and the
* all-defaulted `test_metadata`.  Extracted from test_object.hpp
* (revision.md §6).
*
*   IT HAS NEVER BEEN THE SAME OBJECT AS THE C KERNEL'S CONTAINER, and that is
* the point of keeping them in separate files rather than one shared header:
* basic_metadata<> OWNS std::strings in a std::vector and grows; the C side's
* d_test_metadata BORROWS const char* rows into caller storage and refuses to
* grow.  Two representations, one protocol.  The parity oracle compares the
* protocol's ANSWERS and deliberately not the mechanism.
*
*   THE POINTER CHANGE IN revision.md §6 DID NOT TOUCH THIS FILE.  test_object<>
* still holds its container BY VALUE, as the default template argument
* _MetadataContainer = test_metadata.  Metadata is OPTIONAL IN C and MANDATORY
* IN C++, deliberately: a default template argument must name a COMPLETE type,
* so test_object.hpp includes this header unconditionally.  Gating it on
* D_CFG_TEST_METADATA would make what `basic_test` IS depend on a knob -- the
* one-layout hazard reappearing as template identity, which is worse than the
* member-layout version it replaced because it is invisible in a struct dump.
*
* path:      /inc/djinterp/test/test_metadata.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_TEST_METADATA
#define DJINTERP_TEST_METADATA 1

// c++
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"          // djinterp::test vocabulary


NS_DJINTERP
NS_TEST

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

NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_METADATA
