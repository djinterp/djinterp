/*******************************************************************************
* djinterp [core]                                                     matrix.hpp
*
* A `matrix` is a multidimensional array container suitable for when the
* number
* of dimensions and their sizes are not known at compile-time, but will remain
* constant once initialized.
*
* Design Overview:
*   - Data is stored contiguously in row-major order (C-style), meaning the
*     last index varies fastest in memory layout
*   - Supports arbitrary dimensionality (1D vectors, 2D matrices, 3D tensors,
*     or higher-dimensional arrays)
*   - Inherits from array_container_base for efficient contiguous storage
*   - Pre-computes strides for O(rank) index calculations
*
* Memory Layout Example (2x3 matrix):
*   Logical view:     Memory layout:
*   [0,0] [0,1] [0,2]   [0,0] -> [0,1] -> [0,2] -> [1,0] -> [1,1] -> [1,2]
*   [1,0] [1,1] [1,2]   index:  0       1       2       3       4       5
*
* Stride Calculation:
*   For dimensions [d0, d1, d2, ..., dn-1], strides are:
*     stride[i] = product of dimensions[i+1] through dimensions[n-1]
*     stride[n-1] = 1
*
*   Example for 2x3x4 matrix:
*     dimensions = [2, 3, 4]
*     strides    = [12, 4, 1]  (3*4=12, 4*1=4, 1)
*     element(i,j,k) is at position: i*12 + j*4 + k*1
*
* Template Parameters:
*   _Type           - Element type stored in the matrix
*   _Iterator       - Iterator type (default: array_iterator<_Type>)
*   _ConstIterator - Const iterator type (default: array_iterator<const
* _Type>)
*   _DifferenceType - Signed integer type for differences (default: ptrdiff_t)
*   _SizeType       - Unsigned integer type for sizes (default: size_t)
*
* Usage Examples:
*
*   // Create a 3x4 matrix of doubles, initialized to 0.0
*   matrix<double> m1({3, 4}, 0.0);
*
*   // Create a 2x3x4 tensor (3D array)
*   matrix<int> tensor({2, 3, 4});
*
*   // Access elements using operator() - no bounds checking
*   m1(1, 2) = 3.14159;
*   tensor(0, 1, 2) = 42;
*
*   // Access with bounds checking using at()
*   try {
*       auto val = m1.at(5, 0);  // throws std::out_of_range
*   } catch (const std::out_of_range& e) { ... }
*
*   // Linear access (row-major order)
*   for (size_t i = 0; i < m1.size(); ++i) {
*       m1[i] = static_cast<double>(i);
*   }
*
*   // Iterator access
*   for (auto& elem : m1) {
*       elem *= 2.0;
*   }
*
*   // Multi-dimensional aware iteration
*   for (auto it = m1.mbegin(); it != m1.mend(); ++it) {
*       auto idx = it.indices();  // get {row, col}
*       *it = idx[0] * 10 + idx[1];
*   }
*
*   // Reshape without copying (must preserve total size)
*   m1.reshape({4, 3});   // 3x4 -> 4x3
*   m1.reshape({12});     // flatten to 1D
*   m1.reshape({2, 6});   // reshape to 2x6
*
*
* link(s):   TBA
*
*
* path:      /inc/djinterp/core/container/matrix/matrix.hpp
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.01
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_MATRIX_MATRIX_HPP
#define DJINTERP_CONTAINER_MATRIX_MATRIX_HPP 1

// std
#include <algorithm>
#include <initializer_list>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../array/array.hpp"
#include "../array/array_iterator.hpp"
#include "./matrix_iterator.hpp"


NS_DJINTERP

    NS_INTERNAL

        // matrix_index_calculator
        //   helper: provides static utility functions for converting between
        // linear indices and multi-dimensional indices using row-major
        // ordering. This helper class encapsulates the mathematics of
        // multi-dimensional
        // array indexing, keeping the matrix class focused on container
        // semantics. Row-Major Ordering:
        //   In row-major order (also called lexicographic or C-style order),
        //   elements are stored such that the last index varies fastest.
        //   For a 2D array, this means elements are stored row-by-row.
        //
        // Template Parameters:
        //   _SizeType - unsigned integer type for sizes and indices
        //
        template<typename _SizeType = std::size_t>
        struct matrix_index_calculator
        {
            using size_type = _SizeType;

            // compute_linear_index
            //   Converts a tuple of multi-dimensional indices to a single
            // linear index suitable for accessing a contiguous array.
            //
            // Parameters:
            //   _dimensions - container of dimension sizes [d0, d1, ..., dn-1]
            //   _indices - container of indices [i0, i1, ..., in-1]
            //
            // Returns:
            //   Linear index = sum(indices[k] * stride[k]) for k in [0, n-1]
            //
            // Algorithm:
            //   Computes strides on-the-fly by iterating in reverse order.
            //   This avoids needing pre-computed strides but is O(n) per call.
            //   For frequent access, prefer using pre-computed strides.
            //
            // Example:
            //   dimensions = {2, 3, 4}
            //   indices = {1, 2, 3}
            //   result = 1*12 + 2*4 + 3*1 = 23
            //
            template<typename _DimContainer,
                     typename _IndexContainer>
            static constexpr size_type compute_linear_index(
                const _DimContainer&   _dimensions,
                const _IndexContainer& _indices)
            {
                size_type linear_index = 0;
                size_type stride       = 1;

                // iterate in reverse for row-major ordering stride accumulates
                // as: 1, dim[n-1], dim[n-1]*dim[n-2], ...
                for (size_type i = _dimensions.size(); i > 0; --i)
                {
                    linear_index += _indices[i - 1] * stride;
                    stride       *= _dimensions[i - 1];
                }

                return linear_index;
            }

            // compute_strides
            //   Pre-computes stride values for each dimension.
            //
            // Strides represent the number of elements to skip in the linear
            // array when incrementing an index in that dimension by 1.
            //
            // Parameters:
            //   _dimensions - container of dimension sizes [d0, d1, ..., dn-1]
            //
            // Returns:
            //   Vector of strides where:
            //     strides[n-1] = 1
            //     strides[i] = strides[i+1] * dimensions[i+1]
            //
            // Example:
            //   dimensions = {2, 3, 4}
            //   strides = {12, 4, 1}
            //
            // Usage:
            //   Pre-computing strides enables O(n) index computation via
            //   dot product rather than O(n) with running products.
            //
            template<typename _DimContainer>
            static std::vector<size_type> compute_strides(
                const _DimContainer& _dimensions)
            {
                std::vector<size_type> strides(_dimensions.size());

                // handle empty dimensions
                if (_dimensions.empty())
                {
                    return strides;
                }

                // last stride is always 1 (single element step)
                strides.() = 1;

                // compute remaining strides in reverse order
                for (size_type i = _dimensions.size() - 1; i > 0; --i)
                {
                    strides[i - 1] = strides[i] * _dimensions[i];
                }

                return strides;
            }

            // compute_total_size
            //   Computes the total number of elements from dimension sizes.
            //
            // Parameters:
            //   _dimensions - container of dimension sizes
            //
            // Returns:
            //   Product of all dimensions, or 0 if dimensions is empty.
            //
            // Note:
            //   For very large matrices, consider overflow checking.
            //   A 64-bit size_t can hold products up to ~1.8e19.
            //
            template<typename _DimContainer>
            static constexpr size_type compute_total_size(
                const _DimContainer& _dimensions)
            {
                // empty matrix has size 0
                if (_dimensions.empty())
                {
                    return 0;
                }

                // use std::accumulate with multiplication
                return std::accumulate(_dimensions.begin(),
                                       _dimensions.end(),
                                       size_type(1),
                                       std::multiplies<size_type>());
            }
        };

    NS_END  // internal


    // matrix (forward declaration)
    //   class: multidimensional array container with runtime-determined
    // dimensions. Data is stored contiguously in row-major order. The primary
    // template provides default types for all template parameters. A partial
    // specialization handles the actual implementation.
    //
    template<typename _Type,
             typename _Iterator       = array_iterator<_Type>,
             typename _ConstIterator  = array_iterator<const _Type>,
             typename _DifferenceType = std::ptrdiff_t,
             typename _SizeType       = std::size_t>
    class matrix;

    // matrix (specialization)
    //   class: partial specialization that provides the full implementation.
    //
    // This specialization explicitly specifies the iterator types to enable
    // the class definition while still allowing the primary template to serve
    // as the public interface with defaulted parameters.
    //
    template<typename _Type>
    class matrix<_Type,
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
    private:
        /***  PRIVATE TYPE ALIASES  ***/

        // array_base_type
        //   type: the base class providing contiguous storage management.
        using array_base_type = internal::array_container_base<_Type,
                                                               array_iterator<_Type>,
                                                               array_iterator<const _Type>,
                                                               std::ptrdiff_t,
                                                               std::size_t>;

        // index_calc
        //   type: helper for index calculations.
        using index_calc = internal::matrix_index_calculator<std::size_t>;

    public:
        /***  PUBLIC TYPE ALIASES  ***/

        // value_type
        //   type: the element type stored in the matrix.
        using value_type = _Type;

        // pointer
        //   type: pointer to element type.
        using pointer = value_type*;

        // const_pointer
        //   type: pointer to const element type.
        using const_pointer = value_type const*;

        // reference
        //   type: reference to element type.
        using reference = value_type&;

        // const_reference
        //   type: reference to const element type.
        using const_reference = value_type const&;

        // iterator
        //   type: random access iterator for linear traversal.
        using iterator = array_iterator<_Type>;

        // const_iterator
        //   type: random access const iterator for linear traversal.
        using const_iterator = array_iterator<const _Type>;

        // matrix_iterator_type
        //   type: multi-dimensional aware iterator type.
        using matrix_iterator_type = matrix_iterator<_Type, std::size_t>;

        // const_matrix_iterator_type
        //   type: const version of multi-dimensional aware iterator.
        using const_matrix_iterator_type = matrix_iterator<const _Type, std::size_t>;

        // difference_type
        //   type: signed integer type for iterator arithmetic.
        using difference_type = std::ptrdiff_t;

        // size_type
        //   type: unsigned integer type for sizes and indices.
        using size_type = std::size_t;

        // container_type
        //   type: self-referential type for this container.
        using container_type = matrix<value_type,
                                      iterator,
                                      const_iterator,
                                      difference_type,
                                      size_type>;


        /***  I. CONSTRUCTORS / DESTRUCTOR  ***/

        // default constructor
        //   Creates an empty matrix with no dimensions and no elements.
        //
        // Postconditions:
        //   - size() == 0
        //   - rank() == 0
        //   - empty() == true
        //
        matrix()
            : array_base_type(),
              m_dimensions(),
              m_strides()
        {}

        // dimension constructor (initializer list)
        //   Creates a matrix with the specified dimensions.
        //   Elements are default-initialized (uninitialized for POD types).
        //
        // Parameters:
        //   _dims - initializer list of dimension sizes, e.g., {3, 4, 5}
        //
        // Postconditions:
        //   - size() == product of all dimensions
        //   - rank() == number of dimensions
        //   - dimensions() == _dims
        //
        // Example:
        //   matrix<int> m({3, 4}); // 3x4 matrix (12 elements)
        //   matrix<int> t({2, 3, 4}); // 2x3x4 tensor (24 elements)
        //
        explicit matrix(std::initializer_list<size_type> _dims)
            : array_base_type(index_calc::compute_total_size(_dims)),
              m_dimensions(_dims),
              m_strides(index_calc::compute_strides(m_dimensions))
        {}

        // dimension constructor (vector)
        //   Creates a matrix with dimensions specified by a vector.
        //   Useful when dimensions are computed at runtime.
        //
        // Parameters:
        //   _dims - vector of dimension sizes
        //
        // Example:
        //   std::vector<size_t> dims = compute_required_dims();
        //   matrix<float> m(dims);
        //
        explicit matrix(const std::vector<size_type>& _dims)
            : array_base_type(index_calc::compute_total_size(_dims)),
              m_dimensions(_dims),
              m_strides(index_calc::compute_strides(m_dimensions))
        {}

        // dimension + value constructor (initializer list)
        //   Creates a matrix with specified dimensions, all elements
        //   initialized to a given value.
        //
        // Parameters:
        //   _dims - initializer list of dimension sizes
        //   _value - initial value for all elements
        //
        // Example:
        //   matrix<double> m({3, 3}, 0.0); // 3x3 matrix of zeros
        //   matrix<int> ones({2, 4}, 1); // 2x4 matrix of ones
        //
        matrix(std::initializer_list<size_type> _dims,
               const value_type&                _value)
            : array_base_type(index_calc::compute_total_size(_dims)),
              m_dimensions(_dims),
              m_strides(index_calc::compute_strides(m_dimensions))
        {
            std::fill_n(this->m_arr, this->m_size, _value);
        }

        // dimension + value constructor (vector)
        //   Creates a matrix with vector-specified dimensions and initial
        // value.
        //
        // Parameters:
        //   _dims - vector of dimension sizes
        //   _value - initial value for all elements
        //
        matrix(const std::vector<size_type>& _dims,
               const value_type&             _value)
            : array_base_type(index_calc::compute_total_size(_dims)),
              m_dimensions(_dims),
              m_strides(index_calc::compute_strides(m_dimensions))
        {
            std::fill_n(this->m_arr, this->m_size, _value);
        }

        // copy constructor
        //   Creates a deep copy of another matrix.
        //
        // Parameters:
        //   _other - matrix to copy
        //
        // Postconditions:
        //   - this->size() == _other.size()
        //   - this->dimensions() == _other.dimensions()
        //   - All elements are copied Complexity: O(n) where n is the number
        // of elements.
        //
        matrix(const matrix& _other)
            : array_base_type(_other),
              m_dimensions(_other.m_dimensions),
              m_strides(_other.m_strides)
        {}

        // move constructor
        //   Takes ownership of another matrix's resources.
        //
        // Parameters:
        //   _other - matrix to move from (will be left in valid but
        //            unspecified state)
        //
        // Postconditions:
        //   - this acquires _other's data, dimensions, and strides
        //   - _other is left empty Complexity: O(1) - no element copying.
        //
        matrix(matrix&& _other) noexcept
            : array_base_type(std::move(_other)),
              m_dimensions(std::move(_other.m_dimensions)),
              m_strides(std::move(_other.m_strides))
        {}

        // destructor
        //   Releases all resources.
        //   Base class handles deallocation of element array.
        //
        ~matrix() = default;


        /***  II. ASSIGNMENT OPERATORS  ***/

        // copy assignment
        //   Replaces contents with a deep copy of another matrix.
        //
        // Parameters:
        //   _other - matrix to copy
        //
        // Returns:
        //   Reference to this matrix.
        //
        // Notes:
        //   - Self-assignment is handled correctly
        //   - Previous contents are deallocated Complexity: O(n) where n is
        // the number of elements.
        //
        matrix& operator=(const matrix& _other)
        {
            // check for self-assignment
            if (this != &_other)
            {
                array_base_type::operator=(_other);
                m_dimensions = _other.m_dimensions;
                m_strides    = _other.m_strides;
            }

            return *this;
        }

        // move assignment
        //   Replaces contents by taking ownership of another matrix's
        // resources.
        //
        // Parameters:
        //   _other - matrix to move from
        //
        // Returns:
        //   Reference to this matrix. Complexity: O(1) - no element copying.
        //
        matrix& operator=(matrix&& _other) noexcept
        {
            // check for self-assignment
            if (this != &_other)
            {
                array_base_type::operator=(std::move(_other));
                m_dimensions = std::move(_other.m_dimensions);
                m_strides    = std::move(_other.m_strides);
            }

            return *this;
        }


        /***  III. ELEMENT ACCESS  ***/

        // at (variadic indices with bounds checking)
        //   Accesses an element with full bounds validation.
        //
        // Template Parameters:
        //   _Indices - types of index arguments (must be convertible to
        // size_type)
        //
        // Parameters:
        //   _indices - one index per dimension
        //
        // Returns:
        //   Reference to the element at the specified position.
        //
        // Throws:
        //   std::invalid_argument - if number of indices != number of
        // dimensions
        //   std::out_of_range - if any index is out of bounds
        //
        // Example:
        //   matrix<int> m({3, 4, 5});
        //   m.at(1, 2, 3) = 42; // safe access
        //   m.at(10, 0, 0); // throws std::out_of_range
        //   m.at(1, 2); // throws std::invalid_argument (wrong count)
        // Complexity: O(rank) for bounds checking and index computation.
        //
        template<typename... _Indices>
        requires(std::convertible_to<_Indices, size_type> && ...)
        constexpr reference at(_Indices... _indices)
        {
            static_assert(sizeof...(_Indices) > 0,
                          "at() requires at least one index");

            std::vector<size_type> idx_vec{ static_cast<size_type>(_indices)... };

            // validate dimension count matches
            if (idx_vec.size() != m_dimensions.size())
            {
                throw std::invalid_argument(
                    "matrix::at: index count does not match dimensions");
            }

            // validate each index is within bounds
            for (size_type i = 0; i < idx_vec.size(); ++i)
            {
                if (idx_vec[i] >= m_dimensions[i])
                {
                    throw std::out_of_range("matrix::at: index out of bounds");
                }
            }

            return this->m_arr[compute_linear_index(idx_vec)];
        }

        // at (variadic, const version)
        //   Const-qualified version of variadic at().
        //
        template<typename... _Indices>
        requires(std::convertible_to<_Indices, size_type> && ...)
        constexpr const_reference at(_Indices... _indices) const
        {
            static_assert(sizeof...(_Indices) > 0,
                          "at() requires at least one index");

            std::vector<size_type> idx_vec{ static_cast<size_type>(_indices)... };

            // validate dimension count
            if (idx_vec.size() != m_dimensions.size())
            {
                throw std::invalid_argument(
                    "matrix::at: index count does not match dimensions");
            }

            // validate each index
            for (size_type i = 0; i < idx_vec.size(); ++i)
            {
                if (idx_vec[i] >= m_dimensions[i])
                {
                    throw std::out_of_range("matrix::at: index out of bounds");
                }
            }

            return this->m_arr[compute_linear_index(idx_vec)];
        }

        // at (vector of indices)
        //   Accesses an element using a vector of indices.
        //   Useful when indices are computed at runtime.
        //
        // Parameters:
        //   _indices - vector containing one index per dimension
        //
        // Returns:
        //   Reference to the element at the specified position.
        //
        // Throws:
        //   std::invalid_argument - if indices.size() != rank()
        //   std::out_of_range - if any index is out of bounds
        //
        // Example:
        //   std::vector<size_t> pos = {1, 2, 3};
        //   auto val = m.at(pos);
        //
        constexpr reference at(const std::vector<size_type>& _indices)
        {
            // validate dimension count
            if (_indices.size() != m_dimensions.size())
            {
                throw std::invalid_argument(
                    "matrix::at: index count does not match dimensions");
            }

            // validate each index is within bounds
            for (size_type i = 0; i < _indices.size(); ++i)
            {
                if (_indices[i] >= m_dimensions[i])
                {
                    throw std::out_of_range("matrix::at: index out of bounds");
                }
            }

            return this->m_arr[compute_linear_index(_indices)];
        }

        // at (vector, const version)
        //   Const-qualified version of vector at().
        //
        constexpr const_reference at(const std::vector<size_type>& _indices) const
        {
            // validate dimension count
            if (_indices.size() != m_dimensions.size())
            {
                throw std::invalid_argument(
                    "matrix::at: index count does not match dimensions");
            }

            // validate each index
            for (size_type i = 0; i < _indices.size(); ++i)
            {
                if (_indices[i] >= m_dimensions[i])
                {
                    throw std::out_of_range("matrix::at: index out of bounds");
                }
            }

            return this->m_arr[compute_linear_index(_indices)];
        }

        // operator() (variadic, no bounds checking)
        //   Fast element access without bounds validation. This operator
        // provides the most natural syntax for multi-dimensional access: m(i,
        // j, k) rather than m.at(i, j, k) or m[i][j][k].
        //
        // Template Parameters:
        //   _Indices - types of index arguments
        //
        // Parameters:
        //   _indices - one index per dimension
        //
        // Returns:
        //   Reference to the element at the specified position.
        //
        // Warning:
        //   No bounds checking is performed. Passing invalid indices
        //   results in undefined behavior.
        //
        // Example:
        //   matrix<double> m({3, 4});
        //   m(1, 2) = 3.14; // fast, unchecked access
        //   double v = m(0, 0); // read access Complexity: O(rank) for index
        // computation.
        //
        template<typename... _Indices>
        requires(std::convertible_to<_Indices, size_type> && ...)
        constexpr reference operator()(_Indices... _indices)
        {
            std::vector<size_type> idx_vec{ static_cast<size_type>(_indices)... };

            return this->m_arr[compute_linear_index(idx_vec)];
        }

        // operator() (variadic, const version)
        //   Const-qualified version of operator().
        //
        template<typename... _Indices>
        requires(std::convertible_to<_Indices, size_type> && ...)
        constexpr const_reference operator()(_Indices... _indices) const
        {
            std::vector<size_type> idx_vec{ static_cast<size_type>(_indices)... };

            return this->m_arr[compute_linear_index(idx_vec)];
        }

        // operator[] (linear access)
        //   Accesses elements by linear index in row-major order.
        //
        // Parameters:
        //   _pos - linear position in the range [0, size())
        //
        // Returns:
        //   Reference to the element at the linear position.
        //
        // Note:
        //   This ignores dimensional structure and treats the matrix
        //   as a flat array. Useful for algorithms that process all
        //   elements without regard to structure.
        //
        // Example:
        //   for (size_t i = 0; i < m.size(); ++i) {
        //       m[i] = compute(i);
        //   }
        //
        constexpr reference operator[](size_type _pos)
        {
            return this->m_arr[_pos];
        }

        // operator[] (linear, const version)
        //   Const-qualified version of linear operator[].
        //
        constexpr const_reference operator[](size_type _pos) const
        {
            return this->m_arr[_pos];
        }

        // front
        //   Returns reference to the first element in linear order.
        //
        // Returns:
        //   Reference to element at position (0, 0, ..., 0).
        //
        // Preconditions:
        //   Matrix must not be empty.
        //
        constexpr reference front() noexcept
        {
            return this->m_arr[0];
        }

        // front (const version)
        //   Const-qualified version of front().
        //
        constexpr const_reference front() const noexcept
        {
            return this->m_arr[0];
        }

        //
        //   Returns reference to the last element in linear order.
        //
        // Returns:
        //   Reference to element at position (d0-1, d1-1, ..., dn-1).
        //
        // Preconditions:
        //   Matrix must not be empty.
        //
        constexpr reference () noexcept
        {
            return this->m_arr[this->m_size - 1];
        }

        //  (const version)
        //   Const-qualified version of ().
        //
        constexpr const_reference () const noexcept
        {
            return this->m_arr[this->m_size - 1];
        }


        /***  IV. ITERATORS  ***/
        //
        // The matrix provides two iterator categories:
        //
        // 1. Standard iterators (begin/end): Simple linear traversal using
        //    array_iterator. Compatible with all STL algorithms but unaware
        //    of dimensional structure.
        //
        // 2. Matrix iterators (mbegin/mend): Multi-dimensional aware iterators
        //    that track position in each dimension. Use when you need to know
        //    the indices during iteration.
        //

        // begin
        //   Returns iterator to the first element.
        //
        iterator begin() noexcept
        {
            return iterator(this->m_arr);
        }

        // begin (const version)
        //   Returns const iterator to the first element.
        //
        const_iterator begin() const noexcept
        {
            return const_iterator(this->m_arr);
        }

        // cbegin
        //   Returns const iterator to the first element.
        //   Explicit const version, useful when begin() might return
        //   non-const iterator.
        //
        const_iterator cbegin() const noexcept
        {
            return const_iterator(this->m_arr);
        }

        // end
        //   Returns iterator to one past the last element.
        //
        iterator end() noexcept
        {
            return iterator(this->m_arr, this->m_size);
        }

        // end (const version)
        //   Returns const iterator to one past the last element.
        //
        const_iterator end() const noexcept
        {
            return const_iterator(this->m_arr, this->m_size);
        }

        // cend
        //   Returns const iterator to one past the last element.
        //
        const_iterator cend() const noexcept
        {
            return const_iterator(this->m_arr, this->m_size);
        }

        // mbegin
        //   Returns matrix-aware iterator to the first element. The matrix
        // iterator provides access to multi-dimensional indices during
        // iteration via the indices() method.
        //
        // Example:
        //   for (auto it = m.mbegin(); it != m.mend(); ++it) {
        //       auto idx = it.indices(); // {i, j, k, ...}
        //       *it = f(idx);
        //   }
        //
        matrix_iterator_type mbegin() noexcept
        {
            return matrix_iterator_type(this->m_arr,
                                        0,
                                        &m_dimensions,
                                        &m_strides);
        }

        // mbegin (const version)
        //   Returns const matrix-aware iterator to the first element.
        //
        const_matrix_iterator_type mbegin() const noexcept
        {
            return const_matrix_iterator_type(this->m_arr,
                                              0,
                                              &m_dimensions,
                                              &m_strides);
        }

        // mcbegin
        //   Returns const matrix-aware iterator to the first element.
        //
        const_matrix_iterator_type mcbegin() const noexcept
        {
            return const_matrix_iterator_type(this->m_arr,
                                              0,
                                              &m_dimensions,
                                              &m_strides);
        }

        // mend
        //   Returns matrix-aware iterator to one past the last element.
        //
        matrix_iterator_type mend() noexcept
        {
            return matrix_iterator_type(this->m_arr,
                                        this->m_size,
                                        &m_dimensions,
                                        &m_strides);
        }

        // mend (const version)
        //   Returns const matrix-aware iterator to one past the last element.
        //
        const_matrix_iterator_type mend() const noexcept
        {
            return const_matrix_iterator_type(this->m_arr,
                                              this->m_size,
                                              &m_dimensions,
                                              &m_strides);
        }

        // mcend
        //   Returns const matrix-aware iterator to one past the last element.
        //
        const_matrix_iterator_type mcend() const noexcept
        {
            return const_matrix_iterator_type(this->m_arr,
                                              this->m_size,
                                              &m_dimensions,
                                              &m_strides);
        }


        /***  V. CAPACITY  ***/

        // size
        //   Returns the total number of elements in the matrix.
        //
        // Returns:
        //   Product of all dimensions, or 0 for an empty matrix. Complexity:
        // O(1) - stored value.
        //
        constexpr size_type size() const noexcept
        {
            return this->m_size;
        }

        // rank
        //   Returns the number of dimensions (dimensionality).
        //
        // Returns:
        //   0 for empty matrix
        //   1 for vector (1D)
        //   2 for matrix (2D)
        //   3 for tensor (3D)
        //   n for n-dimensional array Note: "rank" here refers to tensor rank
        // (number of indices), not matrix rank (dimension of image).
        //
        constexpr size_type rank() const noexcept
        {
            return m_dimensions.size();
        }

        // ndim
        //   Alias for rank(), following NumPy naming convention.
        //
        constexpr size_type ndim() const noexcept
        {
            return rank();
        }

        // dimension
        //   Returns the size of a specific dimension.
        //
        // Parameters:
        //   _dim - dimension index (0-based)
        //
        // Returns:
        //   Size of the specified dimension.
        //
        // Throws:
        //   std::out_of_range if _dim >= rank()
        //
        // Example:
        //   matrix<int> m({3, 4, 5});
        //   m.dimension(0); // returns 3
        //   m.dimension(1); // returns 4
        //   m.dimension(2); // returns 5
        //
        constexpr size_type dimension(size_type _dim) const
        {
            if (_dim >= m_dimensions.size())
            {
                throw std::out_of_range("matrix::dimension: invalid dimension");
            }

            return m_dimensions[_dim];
        }

        // extent
        //   Alias for dimension(), following std::mdspan naming.
        //
        constexpr size_type extent(size_type _dim) const
        {
            return dimension(_dim);
        }

        // dimensions
        //   Returns a reference to the vector of all dimension sizes.
        //
        // Returns:
        //   Const reference to internal dimensions vector.
        //
        // Example:
        //   for (auto d : m.dimensions()) {
        //       std::cout << d << " ";
        //   }
        //
        const std::vector<size_type>& dimensions() const noexcept
        {
            return m_dimensions;
        }

        // shape
        //   Alias for dimensions(), following NumPy naming convention.
        //
        const std::vector<size_type>& shape() const noexcept
        {
            return dimensions();
        }

        // stride
        //   Returns the stride for a specific dimension.
        //
        // The stride is the number of elements to skip in linear memory when
        // incrementing the index for that dimension by 1.
        //
        // Parameters:
        //   _dim - dimension index (0-based)
        //
        // Returns:
        //   Stride value for the specified dimension.
        //
        // Throws:
        //   std::out_of_range if _dim >= rank()
        //
        // Example:
        //   matrix<int> m({2, 3, 4});
        //   m.stride(0); // returns 12 (skip 3*4 elements)
        //   m.stride(1); // returns 4 (skip 4 elements)
        //   m.stride(2); // returns 1 (adjacent elements)
        //
        constexpr size_type stride(size_type _dim) const
        {
            if (_dim >= m_strides.size())
            {
                throw std::out_of_range("matrix::stride: invalid dimension");
            }

            return m_strides[_dim];
        }

        // strides
        //   Returns a reference to the vector of all stride values.
        //
        const std::vector<size_type>& strides() const noexcept
        {
            return m_strides;
        }

        // empty
        //   Checks if the matrix contains no elements.
        //
        // Returns:
        //   true if size() == 0, false otherwise.
        //
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return this->m_size == 0;
        }


        /***  VI. OPERATIONS  ***/

        // fill
        //   Assigns a value to all elements in the matrix.
        //
        // Parameters:
        //   _value - value to assign to every element Complexity: O(n) where n
        // is size().
        //
        // Example:
        //   m.fill(0); // zero out all elements
        //   m.fill(-1); // set all to -1
        //
        void fill(const value_type& _value)
        {
            std::fill_n(this->m_arr, this->m_size, _value);

            return;
        }

        // swap
        //   Exchanges contents with another matrix.
        //
        // Parameters:
        //   _other - matrix to swap with
        //
        // Postconditions:
        //   - this contains _other's previous contents
        //   - _other contains this's previous contents Complexity: O(1) -
        // swaps pointers and metadata only. Note: Invalidates all iterators
        // and references to both matrices.
        //
        void swap(matrix& _other) noexcept
        {
            using std::swap;
            swap(this->m_size, _other.m_size);
            swap(this->m_arr, _other.m_arr);
            swap(m_dimensions, _other.m_dimensions);
            swap(m_strides, _other.m_strides);

            return;
        }

        // reshape (initializer list)
        //   Changes the dimensional structure without copying data.
        //
        // Parameters:
        //   _new_dims - new dimension sizes
        //
        // Preconditions:
        //   Product of new dimensions must equal current size().
        //
        // Throws:
        //   std::invalid_argument if total size would change.
        //
        // Note:
        //   This is a zero-copy operation that only updates metadata.
        //   The linear order of elements is unchanged.
        //
        // Example:
        //   matrix<int> m({3, 4}, 0); // 3x4 matrix (12 elements)
        //   m.reshape({4, 3}); // now 4x3
        //   m.reshape({2, 6}); // now 2x6
        //   m.reshape({12}); // flatten to 1D
        //   m.reshape({2, 3, 2}); // reshape to 3D
        //   m.reshape({5, 5}); // throws! 25 != 12 Complexity: O(rank) for
        // stride computation.
        //
        void reshape(std::initializer_list<size_type> _new_dims)
        {
            size_type new_size = index_calc::compute_total_size(_new_dims);

            // verify total element count is preserved
            if (new_size != this->m_size)
            {
                throw std::invalid_argument(
                    "matrix::reshape: new dimensions must have same total size");
            }

            m_dimensions = _new_dims;
            m_strides    = index_calc::compute_strides(m_dimensions);

            return;
        }

        // reshape (vector version)
        //   Changes dimensional structure using a vector of dimensions.
        //
        void reshape(const std::vector<size_type>& _new_dims)
        {
            size_type new_size = index_calc::compute_total_size(_new_dims);

            // verify total element count is preserved
            if (new_size != this->m_size)
            {
                throw std::invalid_argument(
                    "matrix::reshape: new dimensions must have same total size");
            }

            m_dimensions = _new_dims;
            m_strides    = index_calc::compute_strides(m_dimensions);

            return;
        }


    private:
        /***  PRIVATE MEMBERS  ***/

        // m_dimensions
        //   Vector storing the size of each dimension.
        //   For a 3x4x5 matrix: {3, 4, 5}
        std::vector<size_type> m_dimensions;

        // m_strides
        //   Pre-computed stride values for each dimension.
        //   For a 3x4x5 matrix: {20, 5, 1}
        //   Used for O(rank) index computation.
        std::vector<size_type> m_strides;


        /***  PRIVATE METHODS  ***/

        // compute_linear_index
        //   Converts multi-dimensional indices to a linear index using
        // pre-computed strides.
        //
        // Parameters:
        //   _indices - vector of indices, one per dimension
        //
        // Returns:
        //   Linear index = sum of (indices[i] * strides[i])
        //
        // Preconditions:
        //   _indices.size() must equal m_dimensions.size()
        //   Each index must be valid (not checked here) Complexity: O(rank)
        //
        constexpr size_type compute_linear_index(
            const std::vector<size_type>& _indices) const noexcept
        {
            size_type linear_index = 0;

            // dot product of indices and strides
            for (size_type i = 0; i < _indices.size(); ++i)
            {
                linear_index += _indices[i] * m_strides[i];
            }

            return linear_index;
        }
    };


    // matrix_view
    //   class: a non-owning view into a matrix or any contiguous data with
    // matrix-like multi-dimensional access semantics.
    //
    // matrix_view provides a lightweight way to interpret existing data as a
    // multi-dimensional array without copying. It's similar to std::span but
    // with multi-dimensional indexing.
    //
    // Use Cases:
    //   - Viewing a sub-region of a larger matrix (future feature)
    //   - Interpreting raw data as multi-dimensional
    //   - Passing matrix data to functions without ownership transfer
    //   - Interfacing with C APIs that use raw pointers
    //
    // Important Notes:
    //   - Does NOT own the data - user must ensure data lifetime
    //   - Modifications through the view affect the underlying data
    //   - View becomes invalid if underlying data is deallocated/moved
    //
    // Template Parameters:
    //   _Type - element type (may be const-qualified for read-only views)
    //   _SizeType - unsigned integer type for sizes
    //
    // Example:
    //   std::vector<int> data(24);
    //   matrix_view<int> view(data.data(), {2, 3, 4});
    //   view(1, 2, 3) = 42; // modifies data[23]
    //
    //   // Read-only view
    //   matrix_view<const int> const_view(data.data(), {2, 3, 4});
    //
    template<typename _Type,
             typename _SizeType = std::size_t>
    class matrix_view
    {
    private:
        // index_calc
        //   type: helper for index calculations.
        using index_calc = internal::matrix_index_calculator<_SizeType>;

    public:
        /***  TYPE ALIASES  ***/

        using value_type      = _Type;
        using pointer         = value_type*;
        using const_pointer   = value_type const*;
        using reference       = value_type&;
        using const_reference = value_type const&;
        using size_type       = _SizeType;


        /***  CONSTRUCTORS  ***/

        // default constructor
        //   Creates an empty view pointing to nullptr.
        //
        matrix_view() noexcept
            : m_data(nullptr),
              m_dimensions(),
              m_strides()
        {}

        // constructor (pointer + dimensions initializer list)
        //   Creates a view over existing data with specified dimensions.
        //
        // Parameters:
        //   _data - pointer to the first element of the data
        //   _dims - initializer list of dimension sizes
        //
        // Preconditions:
        //   _data must point to at least product(_dims) valid elements.
        //
        matrix_view(pointer                          _data,
                    std::initializer_list<size_type> _dims)
            : m_data(_data),
              m_dimensions(_dims),
              m_strides(index_calc::compute_strides(m_dimensions))
        {}

        // constructor (pointer + dimensions vector)
        //   Creates a view with dimensions specified by vector.
        //
        matrix_view(pointer                       _data,
                    const std::vector<size_type>& _dims)
            : m_data(_data),
              m_dimensions(_dims),
              m_strides(index_calc::compute_strides(m_dimensions))
        {}

        // constructor (from matrix)
        //   Creates a view of an existing matrix.
        //
        // Template Parameters:
        //   _Iterator, _ConstIterator, _DifferenceType - matrix template
        // params
        //
        // Parameters:
        //   _mat - matrix to view Note: The matrix must outlive the view.
        //
        template<typename _Iterator,
                 typename _ConstIterator,
                 typename _DifferenceType>
        matrix_view(matrix<_Type,
                           _Iterator,
                           _ConstIterator,
                           _DifferenceType,
                           _SizeType>& _mat)
            : m_data(_mat.data()),
              m_dimensions(_mat.dimensions()),
              m_strides(_mat.strides())
        {}

        // default copy/move operations
        //   Views are lightweight and trivially copyable.
        //
        matrix_view(const matrix_view&)            = default;
        matrix_view(matrix_view&&)                 = default;
        matrix_view& operator=(const matrix_view&) = default;
        matrix_view& operator=(matrix_view&&)      = default;
        ~matrix_view()                             = default;


        /***  ELEMENT ACCESS  ***/

        // operator() (variadic)
        //   Multi-dimensional element access.
        //
        template<typename... _Indices>
        requires(std::convertible_to<_Indices, size_type> && ...)
        constexpr reference operator()(_Indices... _indices)
        {
            std::vector<size_type> idx_vec{ static_cast<size_type>(_indices)... };

            return m_data[compute_linear_index(idx_vec)];
        }

        // operator() (variadic, const)
        //
        template<typename... _Indices>
        requires(std::convertible_to<_Indices, size_type> && ...)
        constexpr const_reference operator()(_Indices... _indices) const
        {
            std::vector<size_type> idx_vec{ static_cast<size_type>(_indices)... };

            return m_data[compute_linear_index(idx_vec)];
        }

        // operator[] (linear)
        //   Linear element access.
        //
        constexpr reference operator[](size_type _pos)
        {
            return m_data[_pos];
        }

        // operator[] (linear, const)
        //
        constexpr const_reference operator[](size_type _pos) const
        {
            return m_data[_pos];
        }

        // data
        //   Returns the underlying data pointer.
        //
        pointer data() noexcept
        {
            return m_data;
        }

        // data (const)
        //
        const_pointer data() const noexcept
        {
            return m_data;
        }


        /***  CAPACITY  ***/

        // size
        //   Returns total number of elements in the view.
        //
        constexpr size_type size() const noexcept
        {
            return index_calc::compute_total_size(m_dimensions);
        }

        // rank
        //   Returns number of dimensions.
        //
        constexpr size_type rank() const noexcept
        {
            return m_dimensions.size();
        }

        // dimension
        //   Returns size of a specific dimension.
        //
        constexpr size_type dimension(size_type _dim) const
        {
            return m_dimensions[_dim];
        }

        // dimensions
        //   Returns all dimension sizes.
        //
        const std::vector<size_type>& dimensions() const noexcept
        {
            return m_dimensions;
        }

        // shape
        //   Alias for dimensions().
        //
        const std::vector<size_type>& shape() const noexcept
        {
            return m_dimensions;
        }

        // empty
        //   Checks if the view is empty.
        //
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return m_data == nullptr || m_dimensions.empty();
        }


    private:
        pointer                m_data;
        std::vector<size_type> m_dimensions;
        std::vector<size_type> m_strides;

        // compute_linear_index
        //   Converts multi-dimensional indices to linear index.
        //
        constexpr size_type compute_linear_index(
            const std::vector<size_type>& _indices) const noexcept
        {
            size_type linear_index = 0;

            for (size_type i = 0; i < _indices.size(); ++i)
            {
                linear_index += _indices[i] * m_strides[i];
            }

            return linear_index;
        }
    };


    /***  DEDUCTION GUIDES  ***/

    // deduction guide: from initializer list + value
    //   Enables: matrix m({3, 4}, 0.0);
    //
    template<typename _Type>
    matrix(std::initializer_list<std::size_t>, const _Type&)
        -> matrix<_Type,
                  array_iterator<_Type>,
                  array_iterator<const _Type>,
                  std::ptrdiff_t,
                  std::size_t>;

    // deduction guide: from vector + value
    //   Enables: matrix m(dim_vec, 0.0);
    //
    template<typename _Type>
    matrix(const std::vector<std::size_t>&, const _Type&)
        -> matrix<_Type,
                  array_iterator<_Type>,
                  array_iterator<const _Type>,
                  std::ptrdiff_t,
                  std::size_t>;


    /***  NON-MEMBER FUNCTIONS  ***/

    // swap
    //   Specialization of std::swap for matrix.
    //
    // Parameters:
    //   _lhs, _rhs - matrices to swap Complexity: O(1)
    //
    template<typename _Type,
             typename _Iterator,
             typename _ConstIterator,
             typename _DifferenceType,
             typename _SizeType>
    void swap(matrix<_Type, _Iterator, _ConstIterator, _DifferenceType, _SizeType>& _lhs,
              matrix<_Type, _Iterator, _ConstIterator, _DifferenceType, _SizeType>& _rhs) noexcept
    {
        _lhs.swap(_rhs);

        return;
    }

NS_END  // djinterp

#endif  // DJINTERP_CONTAINER_MATRIX_MATRIX_HPP
