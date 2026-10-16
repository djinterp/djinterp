/*******************************************************************************
* djinterp [core]                                                      stack.hpp
*
* link(s):   TBA
*
*
* path:      /inc/djinterp/core/container/stack/stack.hpp
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.03.10
*                                                            revised: 2026.09.21
*******************************************************************************/
#pragma once

#ifndef DJINTERP_CONTAINER_STACK_STACK_HPP
#define DJINTERP_CONTAINER_STACK_STACK_HPP 1

// std
#include <memory>
#include <type_traits>
// djinterp
#include "../traits/container_traits.hpp"


namespace djinterp
{
    template <typename _Key,
              typename _Traits,
              typename _Allocator = std::allocator<_Key>>
    class list
    {

    };

    template <typename T,
              typename U = void>
    struct is_list : std::false_type
    {};

    template <typename T>
    struct is_list <T,
        std::void_t<typename T::value_type,
                    typename T::size_type,
                    typename T::allocator_type,
                    typename T::iterator,
                    typename T::const_iterator,
                    decltype(std::declval<T>().size()),
                    decltype(std::declval<T>().begin()),
                    decltype(std::declval<T>().end()),
                    decltype(std::declval<T>().cbegin()),
                    decltype(std::declval<T>().cend())>>
                // to do?
                // rbegin
                // crbegin
                // rend
                // crend
    : std::true_type{};

    template <typename T,
              typename U = void>
    constexpr bool is_list_v = is_list<T, U>::value;

    template <typename T>
    concept list = requires
    {
        djinterp::is_list_v<T>;
    };
};  // djinterp


#endif  // DJINTERP_CONTAINER_STACK_STACK_HPP
