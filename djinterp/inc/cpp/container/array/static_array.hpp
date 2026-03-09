/*******************************************************************************
* djinterp [container]                                         static_array.hpp
*
* 
* 
* 
* author(s): Samuel 'teer' Neal-Blim
* link:   TBA
* file:   \inc\container\array\static_array.hpp                date: 2024.04.30
*******************************************************************************/
#pragma once

#ifndef DJINTERP_ARRAY_STATIC_
#define	DJINTERP_ARRAY_STATIC_ 1

#include <memory>
#include <type_traits>
#include <utility>
#include "..\..\djinterp"
#include ".\array.hpp"
#include ".\array_iterator.hpp"


NS_DJINTERP

	//
	//
	template <typename  _Type,
			  typename  _Iterator       = array_iterator<_Type>,
			  typename  _ConstIterator  = array_iterator<const _Type>,
			  typename  _DifferenceType = std::ptrdiff_t,
              typename  _SizeType       = std::size_t,
	          _SizeType _Size           = 0>
    class static_array;

	template <typename    _Type,
		      std::size_t _Size>
	class static_array<_Type, array_iterator<_Type>, array_iterator<const _Type>, ptrdiff_t, std::size_t, _Size> : public internal::array_container_base<_Type, array_iterator<_Type>, array_iterator<const _Type>, ptrdiff_t, std::size_t>
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
        using container_type  = static_array<value_type, iterator, const_iterator, difference_type, size_type>;
        static constexpr size_type max_size = _Size;


        static_array() : array_container_base_type(max_size)
        {}

        template <typename... _Elements>
        requires(std::constructible_from<value_type, _Elements> && ...)
        static_array(_Elements&&... _Es) : array_container_base_type(_Es...)
        {}

        static_array(const static_array& _other) : array_container_base_type(_other)
        {}

        static_array(static_array&& _other) : array_container_base_type(_other)
        {}

        static_array& operator=(const static_array& _other)
        {
            if (this != &_other)
            {
                this->m_size = _other.m_size;

                delete[] m_arr;
                this->m_arr = new value_type[this->m_size];
                std::copy_n(_other.m_arr, this->m_size, this->m_arr);
            }

            return *this;
        }

        static_array& operator=(static_array&& _other) noexcept
        {
            delete[] this->m_arr;
            m_size = std::exchange(_other.m_size, 0);
            m_arr = std::exchange(_other.m_arr, nullptr);

            return *this;
        }

        constexpr const_reference back() const noexcept
        {
            return this->m_arr[this->m_size - 1];
        }

        constexpr const_reference front() const noexcept
        {
            return this->m_arr[(size_type)0];
        }

        iterator begin()
        {
            return iterator(*this, 0);
        }

        iterator end()
        {
            return iterator(*this, this->m_size);
        }

        const_iterator begin() const
        {
            return const_iterator(*this, 0);
        }

        const_iterator end() const
        {
            return const_iterator(*this, this->m_size);
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

	
NS_END	// djinterp

#endif	// DJINTERP_ARRAY_STATIC_