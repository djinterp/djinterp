/*******************************************************************************
* djinterp [container]                                                array.hpp
*
* 
* 
* 
* author(s): Samuel 'teer' Neal-Blim
* link:   TBA
* file:   \inc\container\array\array.hpp                       date: 2024.04.30
*******************************************************************************/

#ifndef DJINTERP_ARRAY_
#define	DJINTERP_ARRAY_ 1

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "..\..\djinterp.h"
#include "..\container.hpp"
#include ".\array_iterator.hpp"


NS_DJINTERP
NS_CONTAINER

    NS_INTERNAL
        // array_container_base
        //
        template<typename  _Type,
			    typename  _Iterator       = array_iterator<_Type>,
			    typename  _ConstIterator  = array_iterator<const _Type>,
			    typename  _DifferenceType = std::ptrdiff_t,
                typename  _SizeType       = std::size_t>
        class array_container_base;

        template<typename _Type>
        class array_container_base<_Type,
                                   array_iterator<_Type>, 
                                   array_iterator<const _Type>,
                                   ptrdiff_t, 
                                   std::size_t> 
        : public container_base<_Type, 
                                ptrdiff_t, 
                                std::size_t>
        {
            using container_base_type = container_base<_Type, ptrdiff_t, std::size_t>;

        public:
            using type            = array_container_base<_Type,
                                                         array_iterator<_Type>, 
                                                         array_iterator<const _Type>,
                                                         ptrdiff_t, 
                                                         std::size_t> ;
            using value_type      = _Type;
            using pointer         = _Type*;
            using const_pointer   = _Type const*;
            using reference       = _Type&;
            using const_reference = _Type const&;
            using iterator        = array_iterator<_Type>;
            using const_iterator  = array_iterator<const _Type>;
            using difference_type = std::ptrdiff_t;
            using size_type       = std::size_t;
            using container_type  = array_container_base<value_type, 
                                                         iterator, 
                                                         const_iterator, 
                                                         difference_type, 
                                                         size_type>;


            array_container_base() = default;

            array_container_base(size_type _size)
            {
                if (_size > 0)
                {
                    m_size = _size;
                    m_arr  = new value_type[m_size];
                }
                else
                {
                    m_arr = nullptr;
                }
            }

            template<typename... _Elements>
            requires(std::constructible_from<value_type, _Elements> && ...)
            array_container_base(_Elements&&... _Es) : m_size{ sizeof...(_Es) }, m_arr{ new value_type[sizeof...(_Es)] { _Es... } }
            {}

            array_container_base(const array_container_base& _other) : container_base_type(_other)
            {
                if (this != &_other)
                {
                    m_arr = new value_type[m_size];
                    std::copy_n(_other.m_arr, m_size, m_arr);
                }
            }

            array_container_base(array_container_base&& _other) noexcept : container_base_type(_other), m_arr(std::exchange(_other.m_arr, nullptr))
            {}

            array_container_base& operator=(const array_container_base& _other)
            {
                if (this != &_other)
                {
                    m_size = _other.m_size;

                    delete[] m_arr;
                    m_arr = new value_type[m_size];
                    std::copy_n(_other.m_arr, m_size, m_arr);
                }

                return *this;
            }

            array_container_base& operator=(array_container_base&& _other) noexcept
            {
                delete[] m_arr;
                m_size = std::exchange(_other.m_size, 0);
                m_arr = std::exchange(_other.m_arr, nullptr);

                return *this;
            }

            constexpr value_type& at(const size_type _index)
            {
                if (_index < m_size)
                {
                    return m_arr[_index];
                }
                else
                {
                    throw std::out_of_range("TO DO: index out of bounds.");
                }
            }

            pointer data() noexcept
            {
                return m_arr;
            }

            const_pointer data() const noexcept
            {
                return m_arr;
            }

        protected:
            size_type m_size;
            pointer   m_arr;
        };

    NS_END  // internal

    // array_end
    // 
    template<typename _Type>
    struct array_end
    {};

    // sized_array
    //   Simply an `std::array` clone, only with a compile-time member `array_size` equal to the 
    // value of the second `std::size_t` parameter.
    // As to why a struct, whose size is a parameter known at compile-time, does not have a public
    // `std::size_t` constexpr member: that is beyond the scope of this section.
	template<typename    _Type,
              std::size_t _Size>
    class sized_array : std::array<_Type, _Size>
    {
    public:
        using value_type      = _Type;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using pointer         = value_type*;
        using const_pointer   = const value_type*;

        using iterator               = pointer;
        using const_iterator         = const_pointer;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_pointer>;

        static constexpr size_type array_size = _Size;
    };

NS_END	// djinterp


#endif	// DJINTERP_ARRAY_