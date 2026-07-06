/******************************************************************************
* djinterp [container]                                   container_iterator.hpp
*
* 
* 
* author(s): Samuel 'teer' Neal-Blim
* link:   TBA
* file:   \inc\container\container_iterator.hpp             created: 2024.04.19
******************************************************************************/

#ifndef DJINTERP_CONTAINER_ITERATOR_
#define	DJINTERP_CONTAINER_ITERATOR_ 1

#include <memory>
#include <type_traits>
#include "../djinterp"
#include "../iterator.hpp"
#include "../container.hpp"
#include "./container_concepts.hpp"


NS_DJINTERP
    // container_input_iterator
    //   Iterator class containing the minimum functionality necessary to satisfy the requirements
    // of `std::input_iterator` concept.
    template <typename _ContainerType>
    class container_input_iterator : public input_iterator_base<typename _ContainerType::value_type>
    {
	private:
		using base_iterator_type = input_iterator_base<typename _ContainerType::value_type>;

	public:
		usunderlying container_type     = _ContainerType;
		using value_type         = typename container_type::value_type;
	    using pointer            = value_type*;
	    using reference          = value_type&;
	    using difference_type    = typename container_type::difference_type;
	    using iterator_category  = std::input_iterator_tag;
	    using iterator_type      = container_input_iterator<value_type>;

	    container_input_iterator(const iterator_type& _other) : base_iterator_type(_other)
	    {}
	};

	// container_input_iterator
    //   Iterator class containing the minimum functionality necessary to satisfy the requirements
    // of `std::input_iterator` concept.
    template <typename _ContainerType>
	class container_output_iterator : public output_iterator_base<typename _ContainerType::value_type>
    {
	private:
		using base_iterator_type = output_iterator_base<typename _ContainerType::value_type>;

	public:
		usunderlying container_type     = _ContainerType;
	    using value_type         = typename _ContainerType::value_type;
	    using pointer            = value_type*;
	    using reference          = value_type&;
	    using difference_type    = typename container_type::difference_type;
	    using iterator_category  = std::output_iterator_tag;
	    using iterator_type      = container_input_iterator<value_type>;
        using base_iterator_type = output_iterator_base<value_type>;


		container_output_iterator(const iterator_type& _other) : base_iterator_type(_other)
	    {}
	};

	template <typename _ContainerType>
	class container_forward_iterator : public forward_iterator_base<typename _ContainerType::value_type>
	{
	private:
		using base_iterator_type = forward_iterator_base<typename _ContainerType::value_type>;

	public:
		usunderlying container_type     = _ContainerType;
		using value_type         = typename _ContainerType::value_type;
	    using pointer            = typename _ContainerType::pointer;
	    using reference          = typename _ContainerType::reference;
	    using difference_type    = typename container_type::difference_type;
	    using iterator_category  = std::forward_iterator_tag;
	    using iterator_type      = forward_iterator_base<value_type>;
	

		container_forward_iterator(const iterator_type& _other) : base_iterator_type(_other)
	    {}
	};
	
	template <typename _ContainerType>
	class container_bidirectional_iterator : public container_forward_iterator<typename _ContainerType::value_type>
	{

	};

	template <typename _ContainerType>
	class container_random_access_iterator : public container_bidirectional_iterator<typename _ContainerType::value_type>
	{

	};

	template <typename _ContainerType>
	class container_contiguous_iterator : public container_random_access_iterator<typename _ContainerType::value_type>
	{

	};
	
NS_END	// djinterp

#endif	// DJINTERP_CONTAINER_ITERATOR_