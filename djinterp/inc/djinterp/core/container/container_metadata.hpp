/******************************************************************************
* djinterp [container]                                  container_metadata.hpp
*
*   The foundational metadata facility for containers.  Metadata is, at bottom,
* nothing more than a COLLECTION OF KEY-VALUE PAIRS whose key and value types are
* open: a container may carry any optional metadata -- a title, a name, a
* description, a date, a provenance record -- and this module imposes no fixed
* metadata type, following the framework's metadata_traits protocol (which asks
* only whether a type carries metadata, what type it has, and what holds it).
*
*   TWO PIECES:
*   1. container_metadata<_Key, _Value, _Store> -- the store: an ordered
*      collection of _Key -> _Value entries over an open backing _Store.  Keys and
*      values are ANY types the caller chooses; for heterogeneous values, a
*      variant or std::any as _Value keeps the pairs fully generic
*      (container_metadata<std::string, std::any> is the maximally open form).
*   2. metadata_carrier<_Metadata> -- the attachment: a mixin a container inherits
*      to CARRY a metadata object and expose it through the metadata_traits
*      protocol names (metadata(), metadata_type, metadata_container_type), so the
*      framework's has_metadata / metadata_type_t / metadata_container_type_t
*      detect and extract it with no further wiring.
*
*   OPENNESS.  Nothing here fixes the key type, the value type, or the backing.
* The default backing is a flat vector of pairs with linear lookup -- right for
* the handful of entries metadata usually holds, and imposing only equality on
* the key; a caller wanting ordered or hashed lookup, or non-equality keys, passes
* a different _Store.  A metadata collection is not dimensionally constrained (it
* is just pairs); the table's dimensional headers are the derived table_metadata
* module's concern.
*
*   PORTABILITY:
*   C++11 baseline (runtime store; the `_v` companions degrade with the language).
*
*
* path:      /inc/djinterp/core/container/metadata/container_metadata.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    container_metadata (class)
II.   is_container_metadata (detection trait)
III.  equality
IV.   metadata_carrier (protocol-exposing mixin)
*/

#ifndef DJINTERP_CONTAINER_METADATA_
#define DJINTERP_CONTAINER_METADATA_ 1

// std
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"   // NS_*, D_NODISCARD, clean_t


NS_DJINTERP

// ===========================================================================
// I.   container_metadata (class)
// ===========================================================================

// container_metadata
//   class: an ordered collection of _Key -> _Value metadata entries over an open
// backing _Store.  The key and value types are unconstrained; the default store
// is a flat vector of pairs with linear lookup.  Insertion overwrites an existing
// key (a metadata name holds one value), preserving first-seen order otherwise.
template<typename _Key   = std::string,
         typename _Value = std::string,
         typename _Store = std::vector<std::pair<_Key, _Value>>>
class container_metadata
{
public:
    // --- member types ---

    using key_type       = _Key;
    using mapped_type    = _Value;
    using store_type     = _Store;
    using value_type     = typename _Store::value_type;   // the (key, value) pair
    using size_type      = typename _Store::size_type;
    using iterator       = typename _Store::iterator;
    using const_iterator = typename _Store::const_iterator;

    // --- construction ---

    container_metadata() = default;

    // from a list of entries (later duplicates overwrite earlier ones on set,
    // but the initializer keeps them verbatim; use set() to enforce uniqueness).
    container_metadata(std::initializer_list<value_type> _init)
        : m_entries(_init.begin(), _init.end())
    {}

    // --- key-value surface ---

    // set -- bind _key to _value, overwriting any existing entry for that key.
    void set(const _Key& _key, _Value _value)
    {
        // overwrite in place when the key is already present
        for (value_type& _entry : m_entries)
        {
            if (key_equal(get_key(_entry), _key))
            {
                get_value(_entry) = static_cast<_Value&&>(_value);

                return;
            }
        }

        // otherwise append a new entry
        m_entries.push_back(value_type(_key, static_cast<_Value&&>(_value)));

        return;
    }

    // find -- pointer to the value bound to _key, or nullptr when absent.
    D_NODISCARD const _Value* find(const _Key& _key) const
    {
        for (const value_type& _entry : m_entries)
        {
            if (key_equal(get_key(_entry), _key))
            {
                return &get_value(_entry);
            }
        }

        return nullptr;
    }

    D_NODISCARD _Value* find(const _Key& _key)
    {
        for (value_type& _entry : m_entries)
        {
            if (key_equal(get_key(_entry), _key))
            {
                return &get_value(_entry);
            }
        }

        return nullptr;
    }

    // at -- the value bound to _key; throws std::out_of_range when absent.
    D_NODISCARD const _Value& at(const _Key& _key) const
    {
        const _Value* _p = find(_key);

        // an absent metadata key is a lookup error, not a blank value
        if (_p == nullptr)
        {
            throw std::out_of_range("container_metadata::at: no such key.");
        }

        return *_p;
    }

    // contains -- whether _key is bound.
    D_NODISCARD bool contains(const _Key& _key) const
    {
        return (find(_key) != nullptr);
    }

    // erase -- remove the entry for _key; returns whether one was removed.
    bool erase(const _Key& _key)
    {
        for (iterator _it = m_entries.begin(); _it != m_entries.end(); ++_it)
        {
            if (key_equal(get_key(*_it), _key))
            {
                m_entries.erase(_it);

                return true;
            }
        }

        return false;
    }

    // --- size / iteration / access to the backing ---

    D_NODISCARD size_type size() const noexcept
    {
        return m_entries.size();
    }

    D_NODISCARD bool empty() const noexcept
    {
        return m_entries.empty();
    }

    void clear() noexcept
    {
        m_entries.clear();

        return;
    }

    D_NODISCARD iterator       begin()        noexcept { return m_entries.begin(); }
    D_NODISCARD iterator       end()          noexcept { return m_entries.end();   }
    D_NODISCARD const_iterator begin()  const noexcept { return m_entries.begin(); }
    D_NODISCARD const_iterator end()    const noexcept { return m_entries.end();   }
    D_NODISCARD const_iterator cbegin() const noexcept { return m_entries.begin(); }
    D_NODISCARD const_iterator cend()   const noexcept { return m_entries.end();   }

    // entries -- the underlying store (read-only), for interop.
    D_NODISCARD const store_type& entries() const noexcept
    {
        return m_entries;
    }

private:
    // key_equal -- equality on keys; the only relation the default store imposes
    // on _Key.  A store keyed by a non-equality type supplies its own lookup.
    static bool key_equal(const _Key& _a, const _Key& _b)
    {
        return (_a == _b);
    }

    // get_key / get_value -- read a pair-like entry's members generically, so the
    // store's value_type may be std::pair or any {first, second} aggregate.
    static const _Key&   get_key(const value_type& _e)   { return _e.first;  }
    static const _Value& get_value(const value_type& _e) { return _e.second; }
    static _Value&       get_value(value_type& _e)       { return _e.second; }

    _Store m_entries;
};


// ===========================================================================
// II.  is_container_metadata (detection trait)
// ===========================================================================

// is_container_metadata
//   trait: true when _Type (after stripping cv/ref) is a specialization of
// container_metadata.
NS_INTERNAL

    template<typename _Type>
    struct is_container_metadata_impl : std::false_type
    {};

    template<typename _K,
             typename _V,
             typename _S>
    struct is_container_metadata_impl<container_metadata<_K, _V, _S>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_container_metadata
    : internal::is_container_metadata_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _Type>
inline constexpr bool is_container_metadata_v =
    is_container_metadata<_Type>::value;
#endif


// ===========================================================================
// III. equality
// ===========================================================================

// operator== / operator!=
//   two metadata collections are equal iff they bind the same keys to equal
// values -- an order-INSENSITIVE comparison, since metadata is a set of named
// entries, not a sequence.
template<typename _Key,
         typename _Value,
         typename _Store>
D_NODISCARD bool operator==(
    const container_metadata<_Key, _Value, _Store>& _a,
    const container_metadata<_Key, _Value, _Store>& _b)
{
    if (_a.size() != _b.size())
    {
        return false;
    }

    // every key of _a must be present in _b with an equal value
    for (const auto& _entry : _a)
    {
        const _Value* _rhs = _b.find(_entry.first);

        if ( (_rhs == nullptr) ||
             (!(*_rhs == _entry.second)) )
        {
            return false;
        }
    }

    return true;
}

template<typename _Key,
         typename _Value,
         typename _Store>
D_NODISCARD bool operator!=(
    const container_metadata<_Key, _Value, _Store>& _a,
    const container_metadata<_Key, _Value, _Store>& _b)
{
    return !(_a == _b);
}


// ===========================================================================
// IV.  metadata_carrier (protocol-exposing mixin)
// ===========================================================================

// metadata_carrier
//   class: a mixin a container inherits to CARRY a metadata object and expose it
// through the metadata_traits protocol.  It provides metadata() (the accessor the
// has_metadata trait detects) and the metadata_type / metadata_container_type
// aliases the extractors read.  _Metadata is any metadata type -- a
// container_metadata, a table_metadata, or a user's own; when the metadata is a
// collection it is its own container, so both aliases name it.
//
// Usage:
//   class my_container
//       : public metadata_carrier<container_metadata<std::string, std::any>>
//   { ... };
//   c.metadata().set("title", std::string("Q3 results"));
template<typename _Metadata>
class metadata_carrier
{
public:
    // the metadata_traits protocol names
    using metadata_type           = _Metadata;
    using metadata_container_type = _Metadata;

    // metadata -- the carried metadata object (the has_metadata accessor).
    D_NODISCARD const _Metadata& metadata() const noexcept
    {
        return m_metadata;
    }

    D_NODISCARD _Metadata& metadata() noexcept
    {
        return m_metadata;
    }

protected:
    metadata_carrier() = default;

    explicit metadata_carrier(_Metadata _metadata)
        : m_metadata(static_cast<_Metadata&&>(_metadata))
    {}

    ~metadata_carrier() = default;

    _Metadata m_metadata;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_METADATA_
