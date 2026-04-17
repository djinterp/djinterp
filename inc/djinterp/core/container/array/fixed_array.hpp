/******************************************************************************
* djinterp [container]                                         fixed_array.hpp
*
* A `fixed_array` is an array container suitable for when size is not known 
* at compile-time, but will remain constant once initialized.
* 
*
* author(s): Sam 'teer' Neal-Blim
* link:      TBA
* file:      \inc\container\array\fixed_array.hpp             date: 2023.09.16
******************************************************************************/

#ifndef DJINTERP_ARRAY_FIXED_
#define	DJINTERP_ARRAY_FIXED_ 1

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include "..\..\djinterp"
#include ".\array.hpp"
#include ".\array_iterator.hpp"


NS_DJINTERP

    // fixed_array
    //    
    template<typename  _Type,
			 typename  _Iterator       = array_iterator<_Type>,
			 typename  _ConstIterator  = array_iterator<const _Type>,
			 typename  _DifferenceType = std::ptrdiff_t,
             typename  _SizeType       = std::size_t>
    class fixed_array;

    template<typename _Type>
    class fixed_array<_Type, 
                      array_iterator<_Type>, 
                      array_iterator<const _Type>, 
                      std::ptrdiff_t, 
                      std::size_t> 
    : public internal::array_container_base<_Type, 
                                            array_iterator<_Type>,
                                            array_iterator<const _Type>, 
                                            std::ptrdiff_t, 
                                            std::size_t>
    {
        using array_container_base_type = internal::array_container_base<_Type, array_iterator<_Type>, array_iterator<const _Type>, ptrdiff_t, std::size_t>;

    public:
        using value_type      = _Type;
        using pointer         = value_type*;
        using const_pointer   = value_type const*;
        using reference       = value_type&;
        using const_reference = value_type const&;
        using iterator        = array_iterator<_Type>;
        using const_iterator  = array_iterator<const _Type>;
        using difference_type = std::ptrdiff_t;
        using size_type       = std::size_t;
        using container_type  = fixed_array<value_type, iterator, const_iterator, difference_type, size_type>;


        fixed_array() : array_container_base_type()
        {}

        fixed_array(size_type _size) : array_container_base_type(_size)
        {}

        template<typename... _Elements>
        requires(std::constructible_from<value_type, _Elements> && ...)
        fixed_array(_Elements&&... _Es) : array_container_base_type(_Es...)
        {}

        fixed_array(const fixed_array& _other) : array_container_base_type(_other)
        {}

        fixed_array(fixed_array&& _other) noexcept : array_container_base_type(_other)
        {}

        fixed_array& operator=(const fixed_array& _other)
        {
            if (this != &_other)
            {
                this->m_size = _other.m_size;

                delete[] this->m_arr;
                this->m_arr = new value_type[this->m_size];
                std::copy_n(_other.m_arr, this->m_size, this->m_arr);
            }

            return *this;
        }

        fixed_array& operator=(fixed_array&& _other) noexcept
        {
            delete[] m_arr;
            this->m_size = std::exchange(_other.m_size, 0);
            this->m_arr = std::exchange(_other.m_arr, nullptr);

            return *this;
        }

        constexpr const_reference front() const noexcept
        {
            return this->m_arr[(size_type)0];
        }

        constexpr const_reference back() const noexcept
        {
            return this->m_arr[this->m_size - 1];
        }

        iterator begin()
        {
            return iterator(this->m_arr);
        }

        iterator end()
        {
            return iterator(this->m_arr, this->m_size);
        }

        const_iterator begin() const
        {
            return const_iterator(this->m_arr);
        }

        const_iterator end() const
        {
            return const_iterator(this->m_arr, this->m_size);
        }

        /***  III. OPERATORS  ***/
        constexpr value_type& operator[](const size_type _pos)
        {
            return this->m_arr[_pos];
        }

        constexpr const value_type& operator[](const size_type _pos) const
        {
            return this->m_arr[_pos];
        }
    };

    // deduction guides
    template<typename _Type>
    fixed_array(_Type) -> fixed_array<_Type, array_iterator<_Type>, array_iterator<const _Type>, std::ptrdiff_t, std::size_t>;

    //for lvalue references
    template<typename _Type, 
              std::size_t _Size>
    fixed_array(const _Type(&)[_Size]) -> fixed_array<_Type, array_iterator<_Type>, array_iterator<const _Type>, std::ptrdiff_t, std::size_t>;

    // for rvalue references
    template<typename _Type, 
              std::size_t _Size>
    fixed_array(_Type(&&)[_Size]) -> fixed_array<_Type, array_iterator<_Type>, array_iterator<const _Type>, std::ptrdiff_t, std::size_t>;

    // for std::array
    template<typename _Type, 
              std::size_t _Size>
    fixed_array(const std::array<_Type, _Size>&) -> fixed_array<_Type, array_iterator<_Type>, array_iterator<const _Type>, std::ptrdiff_t, std::size_t>;

    // deduction guide for initializer list
    template<typename _Type, 
              typename... _Args>
    fixed_array(_Type, _Args...) -> fixed_array<_Type, array_iterator<_Type>, array_iterator<const _Type>, std::ptrdiff_t, std::size_t>;

NS_END  // fixed_array_iterator

#endif	// DJINTERP_ARRAY_FIXED_