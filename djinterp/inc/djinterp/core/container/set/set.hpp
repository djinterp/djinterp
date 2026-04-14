/******************************************************************************
* djinterp [container]                                                 set.hpp
*
* djinterp set container header:
*   This header provides the djinterp set container family — sorted,
* unique-key associative containers that are structurally detected by the
* trait system. The containers expose the required type aliases and methods
* to be auto-classified on all twelve axes by container_class<T> and
* set_class<T>.
*
*   Provided containers:
*   - set<K, Compare, Alloc>           sorted unique set
*   - multiset<K, Compare, Alloc>      sorted multi set
*   - unordered_set<K, Hash, Eq, Alloc>  hashed unique set
*   - unordered_multiset<K, Hash, Eq, Alloc>  hashed multi set
*
*   All containers delegate to the corresponding STL container while
* exposing the structural interface required for djinterp classification.
* They may be extended with interval bounds, thread safety, serialization,
* and option support by composing the appropriate aliases and methods.
*
*
* path:      /inc/djinterp/container/set/set.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_CONTAINER_SET_
#define DJINTERP_CONTAINER_SET_ 1

#include <functional>
#include <memory>
#include <set>
#include <unordered_set>
#include "..\core\djinterp.hpp"
#include "..\core\type_traits.hpp"
#include "..\meta\container_traits.hpp"
#include ".\set_traits.hpp"


NS_DJINTERP
NS_CONTAINER

///////////////////////////////////////////////////////////////////////////////
///                    I.   SORTED UNIQUE SET                               ///
///////////////////////////////////////////////////////////////////////////////

// set
//   class: sorted unique-key associative container. Elements are the keys.
// Structurally detected as: set-like, ordered, unique, flat, dynamic,
// bidirectional iteration, sorted invariant.
//
//   Template parameters:
//   _Key     — element/key type
//   _Compare — comparison function object (default: std::less<_Key>)
//   _Alloc   — allocator type (default: std::allocator<_Key>)
template<typename _Key,
         typename _Compare   = std::less<_Key>,
         typename _Allocator = std::allocator<_Key>>
class set
{
private:
    using underlying_type = std::set<_Key, _Compare, _Allocator>;

    underlying_type m_data;

public:
    // --- type aliases (structural detection surface) ---
    using key_type               = _Key;
    using value_type             = _Key;
    using key_compare            = _Compare;
    using value_compare          = _Compare;
    using allocator_type         = _Allocator;
    using size_type              = typename underlying_type::size_type;
    using difference_type        = typename underlying_type::difference_type;
    using reference              = typename underlying_type::reference;
    using const_reference        = typename underlying_type::const_reference;
    using iterator               = typename underlying_type::iterator;
    using const_iterator         = typename underlying_type::const_iterator;
    using reverse_iterator       = typename underlying_type::reverse_iterator;
    using const_reverse_iterator = typename underlying_type::const_reverse_iterator;
    using node_type              = typename underlying_type::node_type;

    // --- constructors ---

    set() = default;

    explicit set(const _Compare& _comp,
                 const _Allocator& _allocator = _Allocator())
        : m_data(_comp, _allocator)
    {}

    template<typename _InputIt>
    set(_InputIt         _first,
        _InputIt         _last,
        const _Compare&  _comp  = _Compare(),
        const _Allocator& _allocator = _Allocator())
        : m_data(_first, _last, _comp, _allocator)
    {}

    set(
		std::initializer_list<_Key> _init,
        const _Compare&             _comp  = _Compare(),
        const _Allocator&           _allocator = _Allocator())
        : m_data(_init, _comp, _allocator)
    {
    }

    set(const set&) = default;
    set(set&&)      = default;

    set& operator=(const set&) = default;
    set& operator=(set&&)      = default;

    // --- iteration (structural: activates iterable, const_iterable,
    //     reverse_iterable, bidirectional) ---

    iterator               begin()         noexcept { return m_data.begin();   }
    const_iterator         begin()   const noexcept { return m_data.begin();   }
    iterator               end()           noexcept { return m_data.end();     }
    const_iterator         end()     const noexcept { return m_data.end();     }
    const_iterator         cbegin()  const noexcept { return m_data.cbegin();  }
    const_iterator         cend()    const noexcept { return m_data.cend();    }
    reverse_iterator       rbegin()        noexcept { return m_data.rbegin();  }
    const_reverse_iterator rbegin()  const noexcept { return m_data.rbegin();  }
    reverse_iterator       rend()          noexcept { return m_data.rend();    }
    const_reverse_iterator rend()    const noexcept { return m_data.rend();    }

    // --- capacity (structural: activates sized) ---

    bool      empty()    const noexcept { return m_data.empty();    }
    size_type size()     const noexcept { return m_data.size();     }
    size_type max_size() const noexcept { return m_data.max_size(); }

    // --- modifiers ---

    void clear() noexcept { m_data.clear(); }

    std::pair<iterator, bool> insert(const value_type& _value)
    {
        return m_data.insert(_value);
    }

    std::pair<iterator, bool> insert(value_type&& _value)
    {
        return m_data.insert(std::move(_value));
    }

    iterator insert(const_iterator _hint,
                    const value_type& _value)
    {
        return m_data.insert(_hint, _value);
    }

    template<typename _InputIt>
    void insert(_InputIt _first,
                _InputIt _last)
    {
        m_data.insert(_first, _last);
    }

    void insert(std::initializer_list<value_type> _init)
    {
        m_data.insert(_init);
    }

    template<typename... _Args>
    std::pair<iterator, bool> emplace(_Args&&... _args)
    {
        return m_data.emplace(std::forward<_Args>(_args)...);
    }

    template<typename... _Args>
    iterator emplace_hint(const_iterator _hint,
                          _Args&&...     _args)
    {
        return m_data.emplace_hint(_hint, std::forward<_Args>(_args)...);
    }

    iterator  erase(const_iterator _pos)              { return m_data.erase(_pos);          }
    iterator  erase(const_iterator _first,
                    const_iterator _last)              { return m_data.erase(_first, _last); }
    size_type erase(const key_type& _key)              { return m_data.erase(_key);          }

    void swap(set& _other) noexcept { m_data.swap(_other.m_data); }

    node_type extract(const_iterator _pos)             { return m_data.extract(_pos);  }
    node_type extract(const key_type& _key)            { return m_data.extract(_key);  }

    template<typename _Comp2>
    void merge(std::set<_Key, _Comp2, _Allocator>& _source)
    {
        m_data.merge(_source);
    }

    void merge(set& _source) { m_data.merge(_source.m_data); }

    // --- lookup (structural: activates set_class find/count/contains) ---

    size_type                                count(const key_type& _key) const { return m_data.count(_key);       }
    iterator                                 find(const key_type& _key)        { return m_data.find(_key);        }
    const_iterator                           find(const key_type& _key)  const { return m_data.find(_key);        }
    bool                                     contains(const key_type& _key) const { return m_data.count(_key) > 0; }
    std::pair<iterator, iterator>            equal_range(const key_type& _key)        { return m_data.equal_range(_key); }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& _key) const { return m_data.equal_range(_key); }
    iterator                                 lower_bound(const key_type& _key)        { return m_data.lower_bound(_key); }
    const_iterator                           lower_bound(const key_type& _key) const  { return m_data.lower_bound(_key); }
    iterator                                 upper_bound(const key_type& _key)        { return m_data.upper_bound(_key); }
    const_iterator                           upper_bound(const key_type& _key) const  { return m_data.upper_bound(_key); }

    // --- observers ---

    key_compare   key_comp()   const { return m_data.key_comp();   }
    value_compare value_comp() const { return m_data.value_comp(); }

    // --- comparison operators ---

    friend bool operator==(const set& _lhs,
                           const set& _rhs) { return _lhs.m_data == _rhs.m_data; }
    friend bool operator!=(const set& _lhs,
                           const set& _rhs) { return _lhs.m_data != _rhs.m_data; }
    friend bool operator<(const set& _lhs,
                          const set& _rhs)  { return _lhs.m_data < _rhs.m_data;  }
    friend bool operator<=(const set& _lhs,
                           const set& _rhs) { return _lhs.m_data <= _rhs.m_data; }
    friend bool operator>(const set& _lhs,
                          const set& _rhs)  { return _lhs.m_data > _rhs.m_data;  }
    friend bool operator>=(const set& _lhs,
                           const set& _rhs) { return _lhs.m_data >= _rhs.m_data; }
};


///////////////////////////////////////////////////////////////////////////////
///                   II.   SORTED MULTI SET                                ///
///////////////////////////////////////////////////////////////////////////////

// multiset
//   class: sorted multi-key associative container. Allows duplicate keys.
// Structurally detected as: set-like, ordered, multi, flat, dynamic,
// bidirectional iteration, sorted invariant.
template<typename _Key,
         typename _Compare = std::less<_Key>,
         typename _Allocator   = std::allocator<_Key>>
class multiset
{
private:
    using underlying_type = std::multiset<_Key, _Compare, _Allocator>;

    underlying_type m_data;

public:
    // --- type aliases (structural detection surface) ---
    using key_type               = _Key;
    using value_type             = _Key;
    using key_compare            = _Compare;
    using value_compare          = _Compare;
    using allocator_type         = _Allocator;
    using size_type              = typename underlying_type::size_type;
    using difference_type        = typename underlying_type::difference_type;
    using reference              = typename underlying_type::reference;
    using const_reference        = typename underlying_type::const_reference;
    using iterator               = typename underlying_type::iterator;
    using const_iterator         = typename underlying_type::const_iterator;
    using reverse_iterator       = typename underlying_type::reverse_iterator;
    using const_reverse_iterator = typename underlying_type::const_reverse_iterator;
    using node_type              = typename underlying_type::node_type;

    // --- constructors ---

    multiset() = default;

    explicit multiset(const _Compare& _comp,
                      const _Allocator&   _allocator = _Allocator())
        : m_data(_comp, _Allocator)
    {
    }

    template<typename _InputIt>
    multiset(_InputIt         _first,
             _InputIt         _last,
             const _Compare&  _comp  = _Compare(),
             const _Allocator&    _allocator = _Allocator())
        : m_data(_first, _last, _comp, _Allocator)
    {
    }

    multiset(std::initializer_list<_Key> _init,
             const _Compare&             _comp  = _Compare(),
             const _Allocator&           _allocator = _Allocator())
        : m_data(_init, _comp, _Allocator)
    {
    }

    multiset(const multiset&) = default;
    multiset(multiset&&)      = default;

    multiset& operator=(const multiset&) = default;
    multiset& operator=(multiset&&)      = default;

    // --- iteration ---

    iterator               begin()         noexcept { return m_data.begin();   }
    const_iterator         begin()   const noexcept { return m_data.begin();   }
    iterator               end()           noexcept { return m_data.end();     }
    const_iterator         end()     const noexcept { return m_data.end();     }
    const_iterator         cbegin()  const noexcept { return m_data.cbegin();  }
    const_iterator         cend()    const noexcept { return m_data.cend();    }
    reverse_iterator       rbegin()        noexcept { return m_data.rbegin();  }
    const_reverse_iterator rbegin()  const noexcept { return m_data.rbegin();  }
    reverse_iterator       rend()          noexcept { return m_data.rend();    }
    const_reverse_iterator rend()    const noexcept { return m_data.rend();    }

    // --- capacity ---

    bool      empty()    const noexcept { return m_data.empty();    }
    size_type size()     const noexcept { return m_data.size();     }
    size_type max_size() const noexcept { return m_data.max_size(); }

    // --- modifiers ---

    void clear() noexcept { m_data.clear(); }

    iterator insert(const value_type& _value)         { return m_data.insert(_value);            }
    iterator insert(value_type&& _value)              { return m_data.insert(std::move(_value));  }
    iterator insert(const_iterator _hint,
                    const value_type& _value)          { return m_data.insert(_hint, _value);     }

    template<typename _InputIt>
    void insert(_InputIt _first,
                _InputIt _last)                        { m_data.insert(_first, _last);            }

    void insert(std::initializer_list<value_type> _init) { m_data.insert(_init); }

    template<typename... _Args>
    iterator emplace(_Args&&... _args)
    {
        return m_data.emplace(std::forward<_Args>(_args)...);
    }

    template<typename... _Args>
    iterator emplace_hint(const_iterator _hint,
                          _Args&&...     _args)
    {
        return m_data.emplace_hint(_hint, std::forward<_Args>(_args)...);
    }

    iterator  erase(const_iterator _pos)               { return m_data.erase(_pos);          }
    iterator  erase(const_iterator _first,
                    const_iterator _last)               { return m_data.erase(_first, _last); }
    size_type erase(const key_type& _key)               { return m_data.erase(_key);          }

    void swap(multiset& _other) noexcept { m_data.swap(_other.m_data); }

    node_type extract(const_iterator _pos)              { return m_data.extract(_pos);  }
    node_type extract(const key_type& _key)             { return m_data.extract(_key);  }

    void merge(multiset& _source) { m_data.merge(_source.m_data); }

    // --- lookup ---

    size_type                                count(const key_type& _key) const { return m_data.count(_key);       }
    iterator                                 find(const key_type& _key)        { return m_data.find(_key);        }
    const_iterator                           find(const key_type& _key)  const { return m_data.find(_key);        }
    bool                                     contains(const key_type& _key) const { return m_data.count(_key) > 0; }
    std::pair<iterator, iterator>            equal_range(const key_type& _key)        { return m_data.equal_range(_key); }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& _key) const { return m_data.equal_range(_key); }
    iterator                                 lower_bound(const key_type& _key)        { return m_data.lower_bound(_key); }
    const_iterator                           lower_bound(const key_type& _key) const  { return m_data.lower_bound(_key); }
    iterator                                 upper_bound(const key_type& _key)        { return m_data.upper_bound(_key); }
    const_iterator                           upper_bound(const key_type& _key) const  { return m_data.upper_bound(_key); }

    // --- observers ---

    key_compare   key_comp()   const { return m_data.key_comp();   }
    value_compare value_comp() const { return m_data.value_comp(); }

    // --- comparison operators ---

    friend bool operator==(const multiset& _lhs,
                           const multiset& _rhs) { return _lhs.m_data == _rhs.m_data; }
    friend bool operator!=(const multiset& _lhs,
                           const multiset& _rhs) { return _lhs.m_data != _rhs.m_data; }
    friend bool operator<(const multiset& _lhs,
                          const multiset& _rhs)  { return _lhs.m_data < _rhs.m_data;  }
    friend bool operator<=(const multiset& _lhs,
                           const multiset& _rhs) { return _lhs.m_data <= _rhs.m_data; }
    friend bool operator>(const multiset& _lhs,
                          const multiset& _rhs)  { return _lhs.m_data > _rhs.m_data;  }
    friend bool operator>=(const multiset& _lhs,
                           const multiset& _rhs) { return _lhs.m_data >= _rhs.m_data; }
};


///////////////////////////////////////////////////////////////////////////////
///                  III.   HASHED UNIQUE SET                               ///
///////////////////////////////////////////////////////////////////////////////

// unordered_set
//   class: hash-based unique-key associative container.
// Structurally detected as: set-like, unordered, unique, flat, dynamic,
// forward iteration.
template<typename _Key,
         typename _Hash      = std::hash<_Key>,
         typename _KeyEqual  = std::equal_to<_Key>,
         typename _Allocator = std::allocator<_Key>>
class unordered_set
{
private:
    using underlying_type = std::unordered_set<_Key, _Hash, _KeyEqual, _Allocator>;

    underlying_type m_data;

public:
    // --- type aliases (structural detection surface) ---
    using key_type        = _Key;
    using value_type      = _Key;
    using hasher          = _Hash;
    using key_equal       = _KeyEqual;
    using allocator_type  = _Allocator;
    using size_type       = typename underlying_type::size_type;
    using difference_type = typename underlying_type::difference_type;
    using reference       = typename underlying_type::reference;
    using const_reference = typename underlying_type::const_reference;
    using iterator        = typename underlying_type::iterator;
    using const_iterator  = typename underlying_type::const_iterator;
    using node_type       = typename underlying_type::node_type;


    // --- constructors ---
    unordered_set() = default;

    explicit unordered_set(
		size_type         _bucket_count,
        const _Hash&      _hash      = _Hash(),
        const _KeyEqual&  _equal     = _KeyEqual(),
        const _Allocator& _allocator = _Allocator())
	: m_data(_bucket_count, _hash, _equal, _allocator)
    {}

    template<typename _InputIt>
    unordered_set(_InputIt          _first,
                  _InputIt          _last,
                  size_type         _bucket_count = 0,
                  const _Hash&      _hash  = _Hash(),
                  const _KeyEqual&  _equal = _KeyEqual(),
                  const _Allocator& _allocator = _Allocator())
        : m_data(_first, _last, _bucket_count, _hash, _equal, _allocator)
    {}

    unordered_set(
		std::initializer_list<_Key> _init,
		size_type                   _bucket_count = 0,
		const _Hash&                _hash         = _Hash(),
		const _KeyEqual&            _equal        = _KeyEqual(),
		const _Allocator&           _allocator    = _Allocator())
        : m_data(_init, _bucket_count, _hash, _equal, _allocator)
    {}

    unordered_set(const unordered_set&) = default;
    unordered_set(unordered_set&&)      = default;

    unordered_set& operator=(const unordered_set&) = default;
    unordered_set& operator=(unordered_set&&)      = default;

    // --- iteration ---
    iterator       begin()        noexcept { return m_data.begin();  }
    const_iterator begin()  const noexcept { return m_data.begin();  }
    iterator       end()          noexcept { return m_data.end();    }
    const_iterator end()    const noexcept { return m_data.end();    }
    const_iterator cbegin() const noexcept { return m_data.cbegin(); }
    const_iterator cend()   const noexcept { return m_data.cend();   }

    // --- capacity ---
    bool      empty()    const noexcept { return m_data.empty();    }
    size_type size()     const noexcept { return m_data.size();     }
    size_type max_size() const noexcept { return m_data.max_size(); }

    // --- modifiers ---
    void clear() noexcept { m_data.clear(); }

    std::pair<iterator, bool> insert(const value_type& _value)
    {
        return m_data.insert(_value);
    }

    std::pair<iterator, bool> insert(value_type&& _value)
    {
        return m_data.insert(std::move(_value));
    }

    iterator insert(const_iterator _hint,
                    const value_type& _value)
    {
        return m_data.insert(_hint, _value);
    }

    template<typename _InputIt>
    void insert(_InputIt _first,
                _InputIt _last)
    {
        m_data.insert(_first, _last);
    }

    void insert(std::initializer_list<value_type> _init) { m_data.insert(_init); }

    template<typename... _Args>
    std::pair<iterator, bool> emplace(_Args&&... _args)
    {
        return m_data.emplace(std::forward<_Args>(_args)...);
    }

    template<typename... _Args>
    iterator emplace_hint(const_iterator _hint,
                          _Args&&...     _args)
    {
        return m_data.emplace_hint(_hint, std::forward<_Args>(_args)...);
    }

    iterator  erase(const_iterator _pos)               { return m_data.erase(_pos);          }
    iterator  erase(const_iterator _first,
                    const_iterator _last)               { return m_data.erase(_first, _last); }
    size_type erase(const key_type& _key)               { return m_data.erase(_key);          }

    void swap(unordered_set& _other) noexcept { m_data.swap(_other.m_data); }

    node_type extract(const_iterator _pos)              { return m_data.extract(_pos); }
    node_type extract(const key_type& _key)             { return m_data.extract(_key); }

    void merge(unordered_set& _source) { m_data.merge(_source.m_data); }

    // --- lookup ---

    size_type      count(const key_type& _key)    const { return m_data.count(_key);    }
    iterator       find(const key_type& _key)           { return m_data.find(_key);     }
    const_iterator find(const key_type& _key)     const { return m_data.find(_key);     }
    bool           contains(const key_type& _key) const { return m_data.count(_key) > 0; }

    std::pair<iterator, iterator>            equal_range(const key_type& _key)        { return m_data.equal_range(_key); }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& _key) const { return m_data.equal_range(_key); }

    // --- bucket interface ---

    size_type bucket_count()                       const { return m_data.bucket_count();     }
    size_type max_bucket_count()                   const { return m_data.max_bucket_count(); }
    size_type bucket_size(size_type _n)            const { return m_data.bucket_size(_n);    }
    size_type bucket(const key_type& _key)         const { return m_data.bucket(_key);       }

    // --- hash policy ---

    float load_factor()                            const { return m_data.load_factor();     }
    float max_load_factor()                        const { return m_data.max_load_factor(); }
    void  max_load_factor(float _mlf)                    { m_data.max_load_factor(_mlf);    }
    void  rehash(size_type _count)                       { m_data.rehash(_count);           }
    void  reserve(size_type _count)                      { m_data.reserve(_count);          }

    // --- observers ---

    hasher    hash_function() const { return m_data.hash_function(); }
    key_equal key_eq()        const { return m_data.key_eq();        }

    // --- comparison operators ---

    friend bool operator==(const unordered_set& _lhs,
                           const unordered_set& _rhs) { return _lhs.m_data == _rhs.m_data; }
    friend bool operator!=(const unordered_set& _lhs,
                           const unordered_set& _rhs) { return _lhs.m_data != _rhs.m_data; }
};


///////////////////////////////////////////////////////////////////////////////
///                  IV.   HASHED MULTI SET                                 ///
///////////////////////////////////////////////////////////////////////////////
`
// unordered_multiset
//   class: hash-based multi-key associative container. Allows duplicate keys.
// Structurally detected as: set-like, unordered, multi, flat, dynamic,
// forward iteration.
template<typename _Key,
         typename _Hash      = std::hash<_Key>,
         typename _KeyEqual  = std::equal_to<_Key>,
         typename _Allocator = std::allocator<_Key>>
class unordered_multiset
{
private:
    using underlying_type = std::unordered_multiset<_Key, _Hash, _KeyEqual, _Allocator>;

    underlying_type m_data;

public:
    // --- type aliases (structural detection surface) ---
    using key_type        = _Key;
    using value_type      = _Key;
    using hasher          = _Hash;
    using key_equal       = _KeyEqual;
    using allocator_type  = _Allocator;
    using size_type       = typename underlying_type::size_type;
    using difference_type = typename underlying_type::difference_type;
    using reference       = typename underlying_type::reference;
    using const_reference = typename underlying_type::const_reference;
    using iterator        = typename underlying_type::iterator;
    using const_iterator  = typename underlying_type::const_iterator;
    using node_type       = typename underlying_type::node_type;

    // --- constructors ---

    unordered_multiset() = default;

    explicit unordered_multiset(
		size_type         _bucket_count,
		const _Hash&      _hash      = _Hash(),
		const _KeyEqual&  _equal     = _KeyEqual(),
		const _Allocator& _allocator = _Allocator()
	) 
		: m_data(_bucket_count, _hash, _equal, _allocator)
    {}

    template<typename _InputIt>
    unordered_multiset(
		_InputIt          _first,
		_InputIt          _last,
		size_type         _bucket_count = 0,
		const _Hash&      _hash  = _Hash(),
		const _KeyEqual&  _equal = _KeyEqual(),
		const _Allocator& _allocator = _Allocator()
	)
		: m_data(_first, _last, _bucket_count, _hash, _equal, _allocator)
    {}

    unordered_multiset(
		std::initializer_list<_Key> _init,
		size_type                   _bucket_count = 0,
		const _Hash&                _hash  = _Hash(),
		const _KeyEqual&            _equal = _KeyEqual(),
		const _Allocator&           _allocator = _Allocator()
	)
		: m_data(_init, _bucket_count, _hash, _equal, _allocator)
    {}

    unordered_multiset(const unordered_multiset&) = default;
    unordered_multiset(unordered_multiset&&)      = default;

    unordered_multiset& operator=(const unordered_multiset&) = default;
    unordered_multiset& operator=(unordered_multiset&&)      = default;

    // --- iteration ---

    iterator       begin()        noexcept { return m_data.begin();  }
    const_iterator begin()  const noexcept { return m_data.begin();  }
    iterator       end()          noexcept { return m_data.end();    }
    const_iterator end()    const noexcept { return m_data.end();    }
    const_iterator cbegin() const noexcept { return m_data.cbegin(); }
    const_iterator cend()   const noexcept { return m_data.cend();   }

    // --- capacity ---

    bool      empty()    const noexcept { return m_data.empty();    }
    size_type size()     const noexcept { return m_data.size();     }
    size_type max_size() const noexcept { return m_data.max_size(); }

    // --- modifiers ---

    void clear() noexcept { m_data.clear(); }

    iterator insert(const value_type& _value)          { return m_data.insert(_value);            }
    iterator insert(value_type&& _value)               { return m_data.insert(std::move(_value)); }
    iterator insert(const_iterator _hint,
                    const value_type& _value)           { return m_data.insert(_hint, _value);     }

    template<typename _InputIt>
    void insert(_InputIt _first,
                _InputIt _last)                         { m_data.insert(_first, _last);            }

    void insert(std::initializer_list<value_type> _init) { m_data.insert(_init); }

    template<typename... _Args>
    iterator emplace(_Args&&... _args)
    {
        return m_data.emplace(std::forward<_Args>(_args)...);
    }

    template<typename... _Args>
    iterator emplace_hint(const_iterator _hint,
                          _Args&&...     _args)
    {
        return m_data.emplace_hint(_hint, std::forward<_Args>(_args)...);
    }

    iterator  erase(const_iterator _pos)                { return m_data.erase(_pos);          }
    iterator  erase(const_iterator _first,
                    const_iterator _last)                { return m_data.erase(_first, _last); }
    size_type erase(const key_type& _key)                { return m_data.erase(_key);          }

    void swap(unordered_multiset& _other) noexcept { m_data.swap(_other.m_data); }

    node_type extract(const_iterator _pos)               { return m_data.extract(_pos); }
    node_type extract(const key_type& _key)              { return m_data.extract(_key); }

    void merge(unordered_multiset& _source) { m_data.merge(_source.m_data); }

    // --- lookup ---

    size_type      count(const key_type& _key)    const { return m_data.count(_key);    }
    iterator       find(const key_type& _key)           { return m_data.find(_key);     }
    const_iterator find(const key_type& _key)     const { return m_data.find(_key);     }
    bool           contains(const key_type& _key) const { return m_data.count(_key) > 0; }

    std::pair<iterator, iterator>            equal_range(const key_type& _key)        { return m_data.equal_range(_key); }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& _key) const { return m_data.equal_range(_key); }

    // --- bucket interface ---

    size_type bucket_count()                        const { return m_data.bucket_count();     }
    size_type max_bucket_count()                    const { return m_data.max_bucket_count(); }
    size_type bucket_size(size_type _n)             const { return m_data.bucket_size(_n);    }
    size_type bucket(const key_type& _key)          const { return m_data.bucket(_key);       }

    // --- hash policy ---

    float load_factor()                             const { return m_data.load_factor();     }
    float max_load_factor()                         const { return m_data.max_load_factor(); }
    void  max_load_factor(float _mlf)                     { m_data.max_load_factor(_mlf);    }
    void  rehash(size_type _count)                        { m_data.rehash(_count);           }
    void  reserve(size_type _count)                       { m_data.reserve(_count);          }

    // --- observers ---

    hasher    hash_function() const { return m_data.hash_function(); }
    key_equal key_eq()        const { return m_data.key_eq();        }

    // --- comparison operators ---

    friend bool operator==(const unordered_multiset& _lhs,
                           const unordered_multiset& _rhs) { return _lhs.m_data == _rhs.m_data; }
    friend bool operator!=(const unordered_multiset& _lhs,
                           const unordered_multiset& _rhs) { return _lhs.m_data != _rhs.m_data; }
};


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_SET_