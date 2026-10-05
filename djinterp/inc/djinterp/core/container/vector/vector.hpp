/*******************************************************************************
* djinterp [core]                                                     vector.hpp
*
* djinterp                                                          vector.hpp
*
*
*
* link(s):   TBA
*
*
* path:      /inc/djinterp/core/container/vector/vector.hpp
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.03.08
*                                                            revised: 2026.10.01
*******************************************************************************/
#pragma once

#ifndef DJINTERP_CONTAINER_VECTOR_VECTOR_HPP
#define DJINTERP_CONTAINER_VECTOR_VECTOR_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <memory>


namespace djinterp {


template <typename T,
          typename Comparator = std::less<T>,
          typename Allocator  = std::allocator<T>>
class vector
{};



};  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_VECTOR_VECTOR_HPP
