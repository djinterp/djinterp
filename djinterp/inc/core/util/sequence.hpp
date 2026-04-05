/******************************************************************************
* djinterp [container]                                         sequence.hpp
*
* Ordered linear element sequence:
*   A sequence is the simplest composite type in the container
* framework: a contiguous, ordered collection of elements of a
* single type.  It is the backbone of path operations — a path
* is just a sequence of components where consecutive elements
* have a parent-child relationship.
*
*   Sequences are type-agnostic: string paths, integer keys,
* node indices, bit patterns, and arbitrary payloads are all
* expressible as sequence<T>.  The only requirement on T is
* copyability.
*
* Contents:
*   - sequence              ordered, contiguous collection of T
*   - make_sequence         factory functions
*
* Usage:
*   // string path components
*   sequence<component_view> sp = {cv("src"), cv("main.cpp")};
*
*   // radix tree keys
*   sequence<uint8_t> rp = {0x01, 0x0A, 0xFF};
*
*   // node index chain
*   sequence<node_id> np = {0, 3, 7, 12};
*
*   // variadic construction (compile-time type-checked)
*   auto s = sequence<int>(1, 2, 3, 4, 5);
*
*
* path:      /inc/cpp/container/sequence.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.06.01
******************************************************************************/

#ifndef DJINTERP_CONTAINER_SEQUENCE_
#define DJINTERP_CONTAINER_SEQUENCE_ 1

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <vector>

#include "../djinterp.hpp"


NS_DJINTERP
NS_CONTAINER


// ================================================================
//  sequence
// ================================================================

// sequence
//   class: an ordered, contiguous collection of elements of
// type _Type.  Provides value semantics with move support.
//
//   A sequence represents the abstract concept of a linear
// ordering of elements.  In the path context, consecutive
// elements are interpreted as parent→child relationships.
// Outside the path context, a sequence is simply an ordered
// collection.
//
//   The type parameter _Type must be copy-constructible.
// Equality comparison of sequences requires _Type to support
// operator==.
template<typename _Type>
class sequence
{
public:
    using value_type      = _Type;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = _Type&;
    using const_reference = const _Type&;
    using pointer         = _Type*;
    using const_pointer   = const _Type*;
    using iterator        = typename std::vector<_Type>::iterator;
    using const_iterator  = typename std::vector<_Type>::const_iterator;

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    // sequence (default)
    //   constructs an empty sequence.
    sequence()
        {}

    // sequence (variadic)
    //   constructs a sequence from a list of elements.
    // All arguments must be the element type _Type.  SFINAE
    // prevents this overload from matching pointer+count or
    // iterator pair calls.
    template<typename... _Args,
             typename = std::enable_if_t<
                 (sizeof...(_Args) > 0) &&
                 (std::is_same<_Args, _Type>::value && ...)>>
    explicit sequence(
            _Args... _args
        )
            : m_data{_args...}
        {}

    // sequence (initializer_list)
    //   constructs a sequence from a brace-enclosed list.
    sequence(
            std::initializer_list<_Type> _init
        )
            : m_data(_init)
        {}

    // sequence (iterator pair)
    //   constructs a sequence from a range [_begin, _end).
    template<typename _Iter>
    sequence(
            _Iter _begin,
            _Iter _end
        )
            : m_data(_begin, _end)
        {}

    // sequence (pointer + count)
    //   constructs a sequence from a contiguous array.
    sequence(
            const _Type* _data,
            size_type    _count
        )
            : m_data(_data, _data + _count)
        {}

    // sequence (vector move)
    //   constructs a sequence by taking ownership of a vector.
    explicit sequence(
            std::vector<_Type>&& _vec
        )
            : m_data(static_cast<std::vector<_Type>&&>(_vec))
        {}

    // sequence (vector copy)
    //   constructs a sequence by copying a vector.
    explicit sequence(
            const std::vector<_Type>& _vec
        )
            : m_data(_vec)
        {}

    // sequence (count + value fill)
    //   constructs a sequence with _count copies of _value.
    sequence(
            size_type _count,
            const _Type& _value
        )
            : m_data(_count, _value)
        {}

    // --------------------------------------------------------
    //  element access
    // --------------------------------------------------------

    // operator[]
    //   returns a reference to the element at _index.
    reference
    operator[]
    (
        size_type _index
    )
    {
        return m_data[_index];
    }

    // operator[] (const)
    //   returns a const reference to the element at _index.
    const_reference
    operator[]
    (
        size_type _index
    ) const
    {
        return m_data[_index];
    }

    // front
    //   returns a reference to the first element.
    reference
    front()
    {
        return m_data.front();
    }

    // front (const)
    //   returns a const reference to the first element.
    const_reference
    front() const
    {
        return m_data.front();
    }

    // back
    //   returns a reference to the last element.
    reference
    back()
    {
        return m_data.back();
    }

    // back (const)
    //   returns a const reference to the last element.
    const_reference
    back() const
    {
        return m_data.back();
    }

    // data
    //   returns a pointer to the underlying contiguous storage.
    const_pointer
    data() const
    {
        return m_data.data();
    }

    // data (mutable)
    //   returns a mutable pointer to the underlying storage.
    pointer
    data()
    {
        return m_data.data();
    }

    // --------------------------------------------------------
    //  capacity
    // --------------------------------------------------------

    // size
    //   returns the number of elements.
    size_type
    size() const
    {
        return m_data.size();
    }

    // empty
    //   returns true if the sequence contains no elements.
    bool
    empty() const
    {
        return m_data.empty();
    }

    // --------------------------------------------------------
    //  iteration
    // --------------------------------------------------------

    // begin
    //   returns an iterator to the first element.
    iterator
    begin()
    {
        return m_data.begin();
    }

    // begin (const)
    //   returns a const iterator to the first element.
    const_iterator
    begin() const
    {
        return m_data.begin();
    }

    // end
    //   returns an iterator past the last element.
    iterator
    end()
    {
        return m_data.end();
    }

    // end (const)
    //   returns a const iterator past the last element.
    const_iterator
    end() const
    {
        return m_data.end();
    }

    // cbegin
    //   returns a const iterator to the first element.
    const_iterator
    cbegin() const
    {
        return m_data.cbegin();
    }

    // cend
    //   returns a const iterator past the last element.
    const_iterator
    cend() const
    {
        return m_data.cend();
    }

    // --------------------------------------------------------
    //  modification
    // --------------------------------------------------------

    // push_back
    //   appends an element to the end.
    void
    push_back
    (
        const _Type& _value
    )
    {
        m_data.push_back(_value);

        return;
    }

    // push_back (move)
    //   appends an element by move to the end.
    void
    push_back
    (
        _Type&& _value
    )
    {
        m_data.push_back(
            static_cast<_Type&&>(_value));

        return;
    }

    // append
    //   appends another sequence to the end.
    void
    append
    (
        const sequence& _other
    )
    {
        m_data.insert(
            m_data.end(),
            _other.m_data.begin(),
            _other.m_data.end());

        return;
    }

    // clear
    //   removes all elements.
    void
    clear()
    {
        m_data.clear();

        return;
    }

    // reserve
    //   pre-allocates storage for _count elements.
    void
    reserve
    (
        size_type _count
    )
    {
        m_data.reserve(_count);

        return;
    }

    // resize
    //   resizes the sequence to _count elements.
    void
    resize
    (
        size_type _count
    )
    {
        m_data.resize(_count);

        return;
    }

    // --------------------------------------------------------
    //  subsequence
    // --------------------------------------------------------

    // sub
    //   returns a new sequence from [_offset, _offset+_count).
    sequence
    sub
    (
        size_type _offset,
        size_type _count
    ) const
    {
        return sequence(
            m_data.data() + _offset,
            _count);
    }

    // head
    //   returns the first _count elements.
    sequence
    head
    (
        size_type _count
    ) const
    {
        return sub(0, _count);
    }

    // tail
    //   returns the last _count elements.
    sequence
    tail
    (
        size_type _count
    ) const
    {
        return sub(m_data.size() - _count, _count);
    }

    // --------------------------------------------------------
    //  comparison
    // --------------------------------------------------------

    // operator==
    //   returns true if both sequences have the same size and
    // all elements compare equal.
    friend bool
    operator==
    (
        const sequence& _a,
        const sequence& _b
    )
    {
        if (_a.m_data.size() != _b.m_data.size())
        {
            return false;
        }

        // element-wise comparison.
        for (size_type i = 0; i < _a.m_data.size(); ++i)
        {
            if (!(_a.m_data[i] == _b.m_data[i]))
            {
                return false;
            }
        }

        return true;
    }

    // operator!=
    //   returns true if the sequences differ.
    friend bool
    operator!=
    (
        const sequence& _a,
        const sequence& _b
    )
    {
        return !(_a == _b);
    }

    // --------------------------------------------------------
    //  conversion
    // --------------------------------------------------------

    // vec
    //   returns a const reference to the underlying vector.
    const std::vector<_Type>&
    vec() const
    {
        return m_data;
    }

    // release
    //   moves the underlying vector out, leaving the sequence
    // empty.
    std::vector<_Type>
    release()
    {
        return static_cast<std::vector<_Type>&&>(m_data);
    }

private:
    std::vector<_Type> m_data;
};


// ================================================================
//  make_sequence
// ================================================================

// make_sequence (variadic)
//   factory: creates a sequence from a list of elements.
template<typename _Type,
         typename... _Args>
sequence<_Type>
make_sequence
(
    _Args... _args
)
{
    return sequence<_Type>(_args...);
}

// make_sequence (pointer + count)
//   factory: creates a sequence from a contiguous array.
template<typename _Type>
sequence<_Type>
make_sequence
(
    const _Type* _data,
    std::size_t  _count
)
{
    return sequence<_Type>(_data, _count);
}

// make_sequence (C array)
//   factory: creates a sequence from a C array.
template<typename _Type,
         std::size_t _N>
sequence<_Type>
make_sequence
(
    const _Type (&_arr)[_N]
)
{
    return sequence<_Type>(_arr, _N);
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_SEQUENCE_
