/*******************************************************************************
* djinterp [container]                                       array_iterator.hpp
*
* The `array_iterator` class is an iterator designed to be used with any
* array-based contiguous container, fulfilling both `iterator` and 
* `const_iterator` requirements based on if the template parameter is const or 
* not (respectively).  `array_iterator` fulfills all of the requirements of 
* `std::contiguous_iterator`.
* 
* 
* author(s): Samuel 'teer' Neal-Blim
* link:   TBA
* file:   \inc\container\array\array_iterator.hpp              date: 2024.06.10
*******************************************************************************/

#ifndef DJINTERP_ARRAY_ITERATOR_
#define	DJINTERP_ARRAY_ITERATOR_ 1

#include <memory>
#include <type_traits>
#include <utility>
#include "..\..\djinterp"


NS_DJINTERP

    // array_iterator
    //   
    template <typename _Type>
    class array_iterator
    {
        using internal_type = std::conditional_t<std::is_const_v<_Type>, const _Type*, _Type*>;

    public:
        using iterator_category = std::contiguous_iterator_tag;
        using iterator_concept  = std::contiguous_iterator_tag;
        using value_type        = std::remove_cv_t<_Type>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<std::is_const_v<_Type>, const value_type*, value_type*>;
        using const_pointer     = const pointer;
        using reference         = std::conditional_t<std::is_const_v<_Type>, const value_type&, value_type&>;
        using const_reference   = const reference;
        using size_type         = std::size_t;
        using element_type      = _Type;
        using iterator_type     = array_iterator<_Type>;


        array_iterator() noexcept = default;

        constexpr explicit array_iterator(pointer _ptr) noexcept : m_ptr(_ptr) 
        {}

        array_iterator(pointer _ptr, size_type _index) noexcept : m_ptr(_ptr + _index) 
        {}

        array_iterator(const array_iterator& _other) noexcept = default;

        array_iterator(array_iterator&& _other) noexcept = default;

        template <typename _T,
                  std::enable_if_t<std::is_same_v<_T, std::remove_const_t<_Type>>&& std::is_const_v<_Type>, int> = 0>
        constexpr array_iterator(const array_iterator<_T>& other) noexcept : m_ptr(other.base()) 
        {}

        constexpr auto operator<=>(const array_iterator&) const = default;

        ~array_iterator() noexcept
        {}

        // copy assignment
        iterator_type& operator=(const array_iterator& _other) noexcept = default;

        // move assignment
        iterator_type& operator=(array_iterator&& _other) noexcept = default;

        constexpr reference operator*() const noexcept
        {
            return *m_ptr;
        }

        constexpr pointer operator->() const noexcept
        {
            return m_ptr;
        }

        // prefix increment
        constexpr iterator_type& operator++() noexcept
        {
            ++m_ptr; 
            
            return *this;
        }

        // postfix increment
        constexpr iterator_type operator++(int) noexcept
        {
            iterator_type temp = *this;
            ++(*this);

            return temp;
        }

        // prefix decrement
        constexpr iterator_type& operator--() noexcept
        {
            --m_ptr; 
            
            return *this;
        }

        // postfix decrement
        constexpr iterator_type operator--(int) noexcept
        {
            iterator_type temp = *this;
            --(*this);

            return temp;
        }

        constexpr iterator_type& operator+=(difference_type _offset) noexcept
        {
            m_ptr += _offset;

            return *this;
        }

        constexpr iterator_type& operator-=(difference_type _offset) noexcept
        {
            m_ptr -= _offset;

            return *this;
        }

        constexpr reference operator[](difference_type _diff) noexcept
        {
            return m_ptr[_diff];
        }

        constexpr const_reference operator[](difference_type _diff) const noexcept
        {
            return m_ptr[_diff];
        }

        friend constexpr iterator_type operator+(const iterator_type& _iter, difference_type _n)
        {
            iterator_type result = _iter;
            result += _n;

            return result;
        }

        friend constexpr iterator_type operator+(difference_type _n, const iterator_type& _iter)
        {
            return _iter + _n;
        }

        friend constexpr iterator_type operator-(const iterator_type& _iterator, difference_type _n)
        {
            iterator_type result = _iterator;
            result -= _n;

            return result;
        }

        friend constexpr difference_type operator-(const iterator_type& _lhs, const iterator_type& _rhs)
        {
            return _lhs.m_ptr - _rhs.m_ptr;
        }
    
        // comparison operators
        bool operator==(const array_iterator& _other) const 
        {
            return m_ptr == _other.m_ptr;
        }

        bool operator!=(const array_iterator& _other) const 
        {
            return m_ptr != _other.m_ptr;
        }

        bool operator<(const array_iterator& _other) const
        {
            return m_ptr < _other.m_ptr;
        }

        bool operator<=(const array_iterator& _other) const
        {
            return m_ptr <= _other.m_ptr;
        }

        bool operator>(const array_iterator& _other) const
        {
            return m_ptr > _other.m_ptr;
        }

        bool operator>=(const array_iterator& _other) const
        {
            return m_ptr >= _other.m_ptr;
        }

        operator pointer() const noexcept
        {
            return this->m_ptr;
        }

        reference operator=(const value_type& _value)
        {
            *m_ptr = _value;
            return *m_ptr;
        }

        reference operator=(value_type&& _value)
        {
            *m_ptr = std::move(_value);
            return *m_ptr;
        }

    private:
        pointer m_ptr;
    };

    template <typename _Type>
    constexpr typename array_iterator<_Type>::pointer to_address(const array_iterator<_Type>& it) noexcept
    {
        return it.operator->();
    }

NS_END	// djinterp

#endif	// DJINTERP_ARRAY_ITERATOR_