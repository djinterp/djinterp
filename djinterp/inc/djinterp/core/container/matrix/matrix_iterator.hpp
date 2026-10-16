/*******************************************************************************
* djinterp [core]                                            matrix_iterator.hpp
*
* The `matrix_iterator` class provides multi-dimensional iteration over matrix
* containers.  Unlike a simple linear iterator, `matrix_iterator` maintains
* awareness of the matrix's dimensional structure, allowing:
*
*   - Access to current multi-dimensional indices at any point during
* iteration
*   - Iteration in row-major order (default) with dimensional awareness
*   - Efficient traversal using pre-computed strides
*
* `matrix_iterator` fulfills all requirements of `std::random_access_iterator`
* and is compatible with standard algorithms.  The iterator stores both the
* linear position and can compute multi-dimensional indices on demand.
*
* Template Parameters:
*   _Type          - The element type (const-qualified for const_iterator)
*   _SizeType - The type used for sizes and indices (default: std::size_t)
*
* Example Usage:
*   matrix<int> m({3, 4, 5});
*   for (auto it = m.mbegin(); it != m.mend(); ++it)
*   {
*       auto indices = it.indices();  // get current (i, j, k)
*       *it = some_function(indices[0], indices[1], indices[2]);
*   }
*
*
* link(s):   TBA
*
*
* path:      /inc/djinterp/core/container/matrix/matrix_iterator.hpp
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.01
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_MATRIX_MATRIX_ITERATOR_HPP
#define DJINTERP_CONTAINER_MATRIX_MATRIX_ITERATOR_HPP 1

// std
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp


NS_DJINTERP

    // matrix_iterator
    //   class: a random-access iterator for multi-dimensional matrix
    // containers that maintains dimensional awareness during iteration. The
    // iterator operates in row-major order, meaning the last dimension varies
    // fastest. For a 3D matrix with dimensions [2, 3, 4]:
    //   - Indices progress: (0,0,0), (0,0,1), ..., (0,0,3), (0,1,0), ...
    //   - Linear positions: 0, 1, 2, ..., 23
    //
    // Key Design Decisions:
    //   1. Stores pointer to data and dimensional metadata (not copies)
    //   2. Computes multi-dimensional indices lazily (only when requested)
    //   3. Uses pre-computed strides for O(rank) index computation
    //   4. Supports const/non-const through template parameter qualification
    //
    template<typename _Type,
             typename _SizeType = std::size_t>
    class matrix_iterator
    {
    private:
        // internal_pointer
        //   type: pointer type adjusted for const-correctness based on _Type.
        using internal_pointer = std::conditional_t<std::is_const_v<_Type>,
                                                    const _Type*,
                                                    _Type*>;

    public:
        /***  I. TYPE ALIASES  ***/
        // Standard iterator type aliases required for STL compatibility. These
        // enable use with standard algorithms and range-based for loops.
        //

        // iterator_category
        //   type: declares this as a random access iterator, supporting all
        // iterator operations including arbitrary offset jumps.
        using iterator_category = std::random_access_iterator_tag;

        // iterator_concept (C++20)
        //   type: concept tag for C++20 iterator concepts.
        using iterator_concept = std::random_access_iterator_tag;

        // value_type
        //   type: the underlying element type with cv-qualifiers removed.
        // Required by the iterator concept for element type deduction.
        using value_type = std::remove_cv_t<_Type>;

        // difference_type
        //   type: signed integer type for iterator arithmetic and distances.
        using difference_type = std::ptrdiff_t;

        // pointer
        //   type: pointer to element, const-qualified if _Type is const.
        using pointer = std::conditional_t<std::is_const_v<_Type>,
                                           const value_type*,
                                           value_type*>;

        // const_pointer
        //   type: always-const pointer to element.
        using const_pointer = const value_type*;

        // reference
        //   type: reference to element, const-qualified if _Type is const.
        using reference = std::conditional_t<std::is_const_v<_Type>,
                                             const value_type&,
                                             value_type&>;

        // const_reference
        //   type: always-const reference to element.
        using const_reference = const value_type&;

        // size_type
        //   type: unsigned integer type for sizes and indices.
        using size_type = _SizeType;

        // element_type
        //   type: the template parameter type, preserving cv-qualifiers.
        using element_type = _Type;

        // iterator_type
        //   type: self-referential type alias for convenience.
        using iterator_type = matrix_iterator<_Type, _SizeType>;

        // indices_type
        //   type: container type for multi-dimensional index tuples.
        using indices_type = std::vector<size_type>;


        /***  II. CONSTRUCTORS / DESTRUCTOR  ***/

        // default constructor
        //   Creates an iterator in an invalid/singular state.
        //   Must be assigned before use.
        matrix_iterator() noexcept
            : m_ptr(nullptr),
              m_position(0),
              m_dimensions(nullptr),
              m_strides(nullptr)
        {}

        // parameterized constructor
        //   Creates an iterator pointing to a specific position in the matrix.
        //
        // Parameters:
        //   _ptr - pointer to the matrix's data array
        //   _position - linear index position (0 = begin, size = end)
        //   _dimensions - pointer to the matrix's dimension vector
        //   _strides - pointer to the matrix's stride vector
        //
        // Note: The iterator stores pointers to dimensions and strides rather
        // than copies, so the matrix must outlive the iterator.
        constexpr matrix_iterator(pointer                    _ptr,
                                  size_type                  _position,
                                  const indices_type* const  _dimensions,
                                  const indices_type* const  _strides) noexcept
            : m_ptr(_ptr),
              m_position(_position),
              m_dimensions(_dimensions),
              m_strides(_strides)
        {}

        // copy constructor
        //   Default copy semantics - iterators are lightweight and copyable.
        matrix_iterator(const matrix_iterator& _other) noexcept = default;

        // move constructor
        //   Default move semantics.
        matrix_iterator(matrix_iterator&& _other) noexcept = default;

        // const conversion constructor
        //   Enables implicit conversion from iterator to const_iterator.
        //   This is a standard pattern allowing non-const iterators to be
        //   used where const_iterators are expected.
        //
        // Template Parameters:
        //   _T - must be the non-const version of _Type, and _Type must be
        // const
        //
        // Example:
        //   matrix<int>::iterator it = m.begin();
        //   matrix<int>::const_iterator cit = it; // implicit conversion
        //
        template<typename _T,
                 std::enable_if_t<( std::is_same_v<_T, std::remove_const_t<_Type>> &&
                                    std::is_const_v<_Type> ),
                                  int> = 0>
        constexpr matrix_iterator(const matrix_iterator<_T, _SizeType>& _other) noexcept
            : m_ptr(_other.base()),
              m_position(_other.position()),
              m_dimensions(_other.dimensions_ptr()),
              m_strides(_other.strides_ptr())
        {}

        // destructor
        //   Default destructor - no owned resources to release.
        ~matrix_iterator() noexcept = default;


        /***  III. ASSIGNMENT OPERATORS  ***/

        // copy assignment
        //   Default copy assignment semantics.
        iterator_type& operator=(const matrix_iterator& _other) noexcept = default;

        // move assignment
        //   Default move assignment semantics.
        iterator_type& operator=(matrix_iterator&& _other) noexcept = default;


        /***  IV. ELEMENT ACCESS  ***/

        // operator*
        //   Dereferences the iterator to access the current element.
        //
        // Returns:
        //   Reference to the element at the current position.
        //
        // Preconditions:
        //   - Iterator must be dereferenceable (not at end position)
        //   - Iterator must be properly initialized
        //
        constexpr reference operator*() const noexcept
        {
            return m_ptr[m_position];
        }

        // operator->
        //   Provides pointer access to the current element's members.
        //
        // Returns:
        //   Pointer to the element at the current position.
        //
        constexpr pointer operator->() const noexcept
        {
            return m_ptr + m_position;
        }

        // operator[]
        //   Provides random access to elements relative to current position.
        //
        // Parameters:
        //   _offset - signed offset from current position
        //
        // Returns:
        //   Reference to element at (current position + offset). Note: No
        // bounds checking is performed.
        //
        constexpr reference operator[](difference_type _offset) const noexcept
        {
            return m_ptr[m_position + _offset];
        }


        /***  V. INCREMENT / DECREMENT OPERATORS  ***/

        // prefix increment (++it)
        //   Advances the iterator to the next element in row-major order.
        //
        // Returns:
        //   Reference to this iterator after incrementing.
        //
        constexpr iterator_type& operator++() noexcept
        {
            ++m_position;

            return *this;
        }

        // postfix increment (it++)
        //   Advances the iterator, returning the previous position.
        //
        // Returns:
        //   Copy of iterator at position before incrementing.
        //
        // Note: Prefer prefix increment when return value is unused, as it
        // avoids the copy.
        //
        constexpr iterator_type operator++(int) noexcept
        {
            iterator_type temp = *this;
            ++(*this);

            return temp;
        }

        // prefix decrement (--it)
        //   Moves the iterator to the previous element in row-major order.
        //
        // Returns:
        //   Reference to this iterator after decrementing.
        //
        constexpr iterator_type& operator--() noexcept
        {
            --m_position;

            return *this;
        }

        // postfix decrement (it--)
        //   Decrements the iterator, returning the previous position.
        //
        // Returns:
        //   Copy of iterator at position before decrementing.
        //
        constexpr iterator_type operator--(int) noexcept
        {
            iterator_type temp = *this;
            --(*this);

            return temp;
        }


        /***  VI. ARITHMETIC OPERATORS  ***/

        // operator+=
        //   Advances the iterator by a specified offset.
        //
        // Parameters:
        //   _offset - number of positions to advance (may be negative)
        //
        // Returns:
        //   Reference to this iterator after advancing.
        //
        constexpr iterator_type& operator+=(difference_type _offset) noexcept
        {
            m_position += _offset;

            return *this;
        }

        // operator-=
        //   Moves the iterator ward by a specified offset.
        //
        // Parameters:
        //   _offset - number of positions to move (may be negative)
        //
        // Returns:
        //   Reference to this iterator after moving.
        //
        constexpr iterator_type& operator-=(difference_type _offset) noexcept
        {
            m_position -= _offset;

            return *this;
        }

        // operator+ (iterator + offset)
        //   Creates a new iterator advanced by the specified offset.
        //
        // Parameters:
        //   _iter - the source iterator
        //   _offset - number of positions to advance
        //
        // Returns:
        //   New iterator at (original position + offset).
        //
        friend constexpr iterator_type operator+(const iterator_type& _iter,
                                                 difference_type      _offset)
        {
            iterator_type result = _iter;
            result += _offset;

            return result;
        }

        // operator+ (offset + iterator)
        //   Commutative version of iterator addition.
        //
        friend constexpr iterator_type operator+(difference_type      _offset,
                                                 const iterator_type& _iter)
        {
            return _iter + _offset;
        }

        // operator- (iterator - offset)
        //   Creates a new iterator moved by the specified offset.
        //
        // Parameters:
        //   _iter - the source iterator
        //   _offset - number of positions to move
        //
        // Returns:
        //   New iterator at (original position - offset).
        //
        friend constexpr iterator_type operator-(const iterator_type& _iter,
                                                 difference_type      _offset)
        {
            iterator_type result = _iter;
            result -= _offset;

            return result;
        }

        // operator- (iterator - iterator)
        //   Computes the distance between two iterators.
        //
        // Parameters:
        //   _lhs - the first iterator (minuend)
        //   _rhs - the second iterator (subtrahend)
        //
        // Returns:
        //   Signed distance such that (_rhs + result == _lhs).
        //
        // Preconditions:
        //   Both iterators must refer to the same matrix.
        //
        friend constexpr difference_type operator-(const iterator_type& _lhs,
                                                   const iterator_type& _rhs)
        {
            return static_cast<difference_type>(_lhs.m_position) -
                   static_cast<difference_type>(_rhs.m_position);
        }


        /***  VII. COMPARISON OPERATORS  ***/

        // operator<=>
        //   Three-way comparison operator (C++20 spaceship operator).
        //   Provides all comparison operations through a single
        // implementation.
        //
        // Returns:
        //   std::strong_ordering indicating relative positions.
        //
        constexpr auto operator<=>(const matrix_iterator& _other) const noexcept
        {
            return m_position <=> _other.m_position;
        }

        // operator==
        //   Equality comparison.
        //
        // Returns:
        //   true if both iterators point to the same position.
        //
        constexpr bool operator==(const matrix_iterator& _other) const noexcept
        {
            return m_position == _other.m_position;
        }

        // operator!=
        //   Inequality comparison.
        //
        // Returns:
        //   true if iterators point to different positions.
        //
        constexpr bool operator!=(const matrix_iterator& _other) const noexcept
        {
            return m_position != _other.m_position;
        }

        // operator<
        //   Less-than comparison based on linear position.
        //
        constexpr bool operator<(const matrix_iterator& _other) const noexcept
        {
            return m_position < _other.m_position;
        }

        // operator<=
        //   Less-than-or-equal comparison.
        //
        constexpr bool operator<=(const matrix_iterator& _other) const noexcept
        {
            return m_position <= _other.m_position;
        }

        // operator>
        //   Greater-than comparison.
        //
        constexpr bool operator>(const matrix_iterator& _other) const noexcept
        {
            return m_position > _other.m_position;
        }

        // operator>=
        //   Greater-than-or-equal comparison.
        //
        constexpr bool operator>=(const matrix_iterator& _other) const noexcept
        {
            return m_position >= _other.m_position;
        }


        /***  VIII. MULTI-DIMENSIONAL ACCESS  ***/

        // indices
        //   Computes the multi-dimensional index tuple for the current
        // position.
        //
        // This is the key feature distinguishing matrix_iterator from a simple
        // linear iterator. Given the current linear position, this method
        // calculates the corresponding index in each dimension. Algorithm (for
        // row-major order):
        //   For position P with strides S[0..n-1]:
        //     indices[0] = P / S[0]
        //     indices[1] = (P % S[0]) / S[1]
        //     indices[2] = (P % S[1]) / S[2]
        //     ... and so on
        //
        // Returns:
        //   Vector of indices, one per dimension, such that
        //   matrix.at(indices) == *this.
        //
        // Example:
        //   For a 3x4 matrix at linear position 7:
        //     indices() returns {1, 3} (row 1, column 3) Complexity: O(rank)
        // where rank is the number of dimensions. Note: This computation is
        // performed on-demand rather than cached to minimize iterator size and
        // avoid update overhead during iteration.
        //
        indices_type indices() const
        {
            // handle edge cases
            if ( (!m_dimensions) ||
                 (!m_strides)    ||
                 (m_dimensions->empty()) )
            {
                return indices_type();
            }

            indices_type result(m_dimensions->size());
            size_type    remaining = m_position;

            // compute index for each dimension using strides
            for (size_type i = 0; i < m_dimensions->size(); ++i)
            {
                result[i] = remaining / (*m_strides)[i];
                remaining = remaining % (*m_strides)[i];
            }

            return result;
        }

        // index
        //   Returns the index in a specific dimension for the current
        // position.
        //
        // Parameters:
        //   _dim - the dimension to query (0 = first dimension)
        //
        // Returns:
        //   The index in the specified dimension.
        //
        // Preconditions:
        //   _dim must be less than the matrix rank.
        //
        // Example:
        //   For a 3x4 matrix at linear position 7:
        //     index(0) returns 1 (row index)
        //     index(1) returns 3 (column index)
        //
        size_type index(size_type _dim) const
        {
            if ( (!m_strides) ||
                 (_dim >= m_strides->size()) )
            {
                return 0;
            }

            size_type remaining = m_position;

            // compute indices up to and including the requested dimension
            for (size_type i = 0; i < _dim; ++i)
            {
                remaining = remaining % (*m_strides)[i];
            }

            return remaining / (*m_strides)[_dim];
        }

        // position
        //   Returns the current linear position in the matrix.
        //
        // Returns:
        //   Zero-based linear index in row-major order.
        //
        // This is useful for:
        //   - Computing progress through iteration
        //   - Debugging and logging
        //   - Direct array access when needed
        //
        constexpr size_type position() const noexcept
        {
            return m_position;
        }

        // rank
        //   Returns the number of dimensions in the associated matrix.
        //
        // Returns:
        //   The dimensionality (1 for vector, 2 for matrix, etc.)
        //
        size_type rank() const noexcept
        {
            if (m_dimensions)
            {
                return m_dimensions->size();
            }

            return 0;
        }


        /***  IX. ACCESSOR METHODS  ***/

        // base
        //   Returns the underlying data pointer.
        //
        // Returns:
        //   Pointer to the beginning of the matrix's data array.
        // Note: This is the base pointer, not a pointer to the current
        // element. For current element pointer, use operator->().
        //
        constexpr pointer base() const noexcept
        {
            return m_ptr;
        }

        // dimensions_ptr
        //   Returns pointer to the dimensions vector (for internal use).
        //
        // This is primarily used by the const conversion constructor to
        // propagate dimensional information.
        //
        const indices_type* dimensions_ptr() const noexcept
        {
            return m_dimensions;
        }

        // strides_ptr
        //   Returns pointer to the strides vector (for internal use).
        //
        const indices_type* strides_ptr() const noexcept
        {
            return m_strides;
        }


        /***  X. CONVERSION OPERATORS  ***/

        // operator pointer
        //   Implicit conversion to pointer type. Enables using the iterator
        // where a raw pointer is expected. Returns pointer to the current
        // element (not the base pointer). Use with caution: this loses
        // dimensional information.
        //
        operator pointer() const noexcept
        {
            return m_ptr + m_position;
        }


    private:
        /***  PRIVATE MEMBERS  ***/

        // m_ptr
        //   Pointer to the beginning of the matrix's contiguous data array.
        //   Combined with m_position, gives access to any element.
        pointer m_ptr;

        // m_position
        //   Current linear position in row-major order.
        //   Range: [0, total_size] where total_size is the end position.
        size_type m_position;

        // m_dimensions
        //   Pointer to the matrix's dimension vector.
        //   Not owned by the iterator; matrix must outlive iterator.
        //   May be nullptr for singular/invalid iterators.
        const indices_type* m_dimensions;

        // m_strides
        //   Pointer to the matrix's pre-computed stride vector.
        //   Strides enable O(rank) conversion between linear and multi-dim
        // indices.
        //   May be nullptr for singular/invalid iterators.
        const indices_type* m_strides;
    };


    // to_address
    //   Obtains the address represented by the iterator. This function is
    // required for `std::contiguous_iterator` concept compliance and is used
    // by `std::to_address`.
    //
    // Parameters:
    //   _it - the matrix iterator
    //
    // Returns:
    //   Raw pointer to the current element.
    //
    template<typename   _Type,
             typename   _SizeType>
    constexpr typename matrix_iterator<_Type, _SizeType>::pointer
    to_address(const matrix_iterator<_Type, _SizeType>& _it) noexcept
    {
        return _it.operator->();
    }


    // matrix_index_iterator
    //   class: an iterator that yields multi-dimensional index tuples rather
    // than element values.
    //
    // This iterator is useful for algorithms that need to know the position
    // of elements without accessing the elements themselves, such as:
    //   - Initializing matrices with position-dependent values
    //   - Parallel algorithms that distribute work by index ranges
    //   - Debugging and visualization
    //
    // Example:
    //   for (auto idx : matrix_index_range({3, 4, 5}))
    //   {
    //       // idx is {i, j, k} for each valid combination
    //       m(idx[0], idx[1], idx[2]) = f(idx);
    //   }
    //
    template<typename _SizeType = std::size_t>
    class matrix_index_iterator
    {
    public:
        using size_type       = _SizeType;
        using indices_type    = std::vector<size_type>;
        using value_type      = indices_type;
        using reference       = const indices_type&;
        using pointer         = const indices_type*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
        using iterator_type   = matrix_index_iterator<_SizeType>;


        /***  CONSTRUCTORS  ***/

        // default constructor
        //   Creates an end iterator (position at total size).
        matrix_index_iterator() noexcept
            : m_dimensions(),
              m_current_indices(),
              m_position(0),
              m_total_size(0)
        {}

        // parameterized constructor
        //   Creates an iterator for the given dimensions.
        //
        // Parameters:
        //   _dimensions - vector of dimension sizes
        //   _at_end - if true, creates an end iterator
        //
        explicit matrix_index_iterator(const indices_type& _dimensions,
                                       bool                _at_end = false)
            : m_dimensions(_dimensions),
              m_current_indices(_dimensions.size(), 0),
              m_position(0),
              m_total_size(compute_total_size(_dimensions))
        {
            if (_at_end)
            {
                m_position = m_total_size;
            }
        }

        // parameterized constructor (initializer list)
        //   Convenience constructor accepting an initializer list.
        //
        explicit matrix_index_iterator(std::initializer_list<size_type> _dimensions,
                                       bool                             _at_end = false)
            : matrix_index_iterator(indices_type(_dimensions), _at_end)
        {}


        /***  ELEMENT ACCESS  ***/

        // operator*
        //   Returns the current multi-dimensional index tuple.
        //
        reference operator*() const noexcept
        {
            return m_current_indices;
        }

        // operator->
        //   Returns pointer to the current index tuple.
        //
        pointer operator->() const noexcept
        {
            return &m_current_indices;
        }


        /***  INCREMENT OPERATORS  ***/

        // prefix increment
        //   Advances to the next index combination in row-major order.
        //
        iterator_type& operator++()
        {
            ++m_position;

            if (m_position < m_total_size)
            {
                increment_indices();
            }

            return *this;
        }

        // postfix increment
        //   Advances iterator, returning copy of previous state.
        //
        iterator_type operator++(int)
        {
            iterator_type temp = *this;
            ++(*this);

            return temp;
        }


        /***  COMPARISON OPERATORS  ***/

        bool operator==(const matrix_index_iterator& _other) const noexcept
        {
            return m_position == _other.m_position;
        }

        bool operator!=(const matrix_index_iterator& _other) const noexcept
        {
            return m_position != _other.m_position;
        }


        /***  ACCESSORS  ***/

        // position
        //   Returns the current linear position.
        //
        size_type position() const noexcept
        {
            return m_position;
        }

        // dimensions
        //   Returns the dimension sizes.
        //
        const indices_type& dimensions() const noexcept
        {
            return m_dimensions;
        }


    private:
        indices_type m_dimensions;
        indices_type m_current_indices;
        size_type    m_position;
        size_type    m_total_size;

        // compute_total_size
        //   Computes product of all dimensions.
        //
        static size_type compute_total_size(const indices_type& _dims)
        {
            if (_dims.empty())
            {
                return 0;
            }

            size_type total = 1;

            for (const auto& dim : _dims)
            {
                total *= dim;
            }

            return total;
        }

        // increment_indices
        //   Advances the multi-dimensional index tuple by one step.
        //   Uses row-major ordering (last index varies fastest).
        //
        void increment_indices()
        {
            // increment from the last dimension (row-major order)
            for (size_type i = m_dimensions.size(); i > 0; --i)
            {
                size_type dim_idx = i - 1;
                ++m_current_indices[dim_idx];

                // check if we need to carry to the next dimension
                if (m_current_indices[dim_idx] < m_dimensions[dim_idx])
                {
                    // no carry needed
                    break;
                }

                // carry: reset this dimension and continue to next
                m_current_indices[dim_idx] = 0;
            }

            return;
        }
    };


    // matrix_index_range
    //   class: a range adapter for matrix_index_iterator, enabling range-based
    // for loops over index combinations.
    //
    // Example:
    //   for (const auto& idx : matrix_index_range({3, 4}))
    //   {
    //       std::cout << "(" << idx[0] << ", " << idx[1] << ")\n";
    //   }
    //   // Output: (0,0), (0,1), (0,2), (0,3), (1,0), ...
    //
    template<typename _SizeType = std::size_t>
    class matrix_index_range
    {
    public:
        using size_type    = _SizeType;
        using indices_type = std::vector<size_type>;
        using iterator     = matrix_index_iterator<size_type>;


        // constructor (vector)
        //   Creates a range over all index combinations for the given
        // dimensions.
        //
        explicit matrix_index_range(const indices_type& _dimensions)
            : m_dimensions(_dimensions)
        {}

        // constructor (initializer list)
        //   Convenience constructor for inline dimension specification.
        //
        matrix_index_range(std::initializer_list<size_type> _dimensions)
            : m_dimensions(_dimensions)
        {}

        // begin
        //   Returns iterator to the first index combination (all zeros).
        //
        iterator begin() const
        {
            return iterator(m_dimensions, false);
        }

        // end
        //   Returns past-the-end iterator.
        //
        iterator end() const
        {
            return iterator(m_dimensions, true);
        }


    private:
        indices_type m_dimensions;
    };

NS_END  // djinterp

#endif  // DJINTERP_CONTAINER_MATRIX_MATRIX_ITERATOR_HPP
