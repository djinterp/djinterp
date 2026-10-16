/*******************************************************************************
* djinterp [test]                                               buffer_tests.hpp
*
*   Declarations and helper types for the buffer.hpp unit test suite.
* Each test_<group> function records its assertions through a
* djinterp::test::test_handler reference using record_assertion();
* one .cpp file backs each section listed below.
*
*   HELPER TYPE:
*   mock_buffer is a minimal CRTP-derived buffer that owns a heap-
* allocated char array and implements the storage()/capacity()/grow()
* contract that buffer_base depends on.  It also exposes test-only
* hooks (force_next_grow_failure(), grow_call_count()) so test cases
* can drive failure-path coverage without relying on allocator
* exhaustion.
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    growth strategy enum               (buffer_tests_enums.cpp)
      -----------------------------------------------------------

II.   cursor model enum                  (buffer_tests_enums.cpp)
      -----------------------------------------------------------

III.  growth policies                    (buffer_tests_growth_policies.cpp)
      ---------------------------------------------------------------------

IV.   cursor policies                    (buffer_tests_cursor_policies.cpp)
      ---------------------------------------------------------------------

V.    buffer_base
      -----------

      - aliases / constants / predicates (buffer_tests_buffer_base_state.cpp)

      - write operations                 (buffer_tests_buffer_base_write.cpp)

      - read operations  (dual cursor)   (buffer_tests_buffer_base_read.cpp)

      - lifecycle / move / reserve       (buffer_tests_buffer_base_lifecycle.cpp)

VI.   growth policy selection            (buffer_tests_policy_selection.cpp)
      ----------------------------------------------------------------------

VII.  cursor policy selection            (buffer_tests_policy_selection.cpp)
      ----------------------------------------------------------------------

VIII. default policy aliases             (buffer_tests_defaults.cpp)
      --------------------------------------------------------------
*/

#ifndef DJINTERP_TEST_CONTAINER_BUFFER_
#define DJINTERP_TEST_CONTAINER_BUFFER_ 1

// std
#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../../../inc/djinterp/core/container/buffer/buffer.hpp"
#include "../../../../../inc/djinterp/test/test_handler.hpp"
#include "../../../../../inc/djinterp/test/test_defaults.hpp"


NS_DJINTERP
NS_TEST


// =============================================================================
//  helper: mock_buffer
// =============================================================================

// mock_buffer
//   class: minimal concrete buffer derived from buffer_base.
// Owns a heap-allocated char array and satisfies the derived-
// class interface (storage / capacity / grow) required by the
// CRTP base.  Exposes test hooks to inject grow() failures and
// observe the number of grow() invocations.
template<typename _GrowthPolicy,
         typename _CursorPolicy>
class mock_buffer
    : public buffer_base<mock_buffer<_GrowthPolicy, _CursorPolicy>,
                         _GrowthPolicy,
                         _CursorPolicy>
{
private:
    using base_type =
        buffer_base<mock_buffer<_GrowthPolicy, _CursorPolicy>,
                    _GrowthPolicy,
                    _CursorPolicy>;

public:
    // constructor
    //   optionally pre-allocates _initial bytes of storage.
    // Zero-fills the region so tests can rely on a known
    // baseline.
    explicit mock_buffer(
        std::size_t _initial = 0
    ) noexcept
        : base_type(),
          m_storage(nullptr),
          m_capacity(0),
          m_force_next_grow_failure(false),
          m_grow_calls(0)
    {
        // allocate the requested initial capacity
        if (_initial > 0)
        {
            m_storage  = new char[_initial];
            m_capacity = _initial;
            std::memset(m_storage, 0, _initial);
        }

        return;
    }

    // destructor
    //   releases the owned storage.
    ~mock_buffer() noexcept
    {
        delete[] m_storage;
        m_storage  = nullptr;
        m_capacity = 0;

        return;
    }

    // move constructor
    mock_buffer(
        mock_buffer&& _other
    ) noexcept
        : base_type(static_cast<base_type&&>(_other)),
          m_storage(_other.m_storage),
          m_capacity(_other.m_capacity),
          m_force_next_grow_failure(_other.m_force_next_grow_failure),
          m_grow_calls(_other.m_grow_calls)
    {
        _other.m_storage                 = nullptr;
        _other.m_capacity                = 0;
        _other.m_force_next_grow_failure = false;
        _other.m_grow_calls              = 0;
    }

    // move assignment
    mock_buffer&
    operator=(
        mock_buffer&& _other
    ) noexcept
    {
        // self-assignment guard
        if (this != &_other)
        {
            delete[] m_storage;

            base_type::operator=(static_cast<base_type&&>(_other));

            m_storage                 = _other.m_storage;
            m_capacity                = _other.m_capacity;
            m_force_next_grow_failure = _other.m_force_next_grow_failure;
            m_grow_calls              = _other.m_grow_calls;

            _other.m_storage                 = nullptr;
            _other.m_capacity                = 0;
            _other.m_force_next_grow_failure = false;
            _other.m_grow_calls              = 0;
        }

        return *this;
    }

    // non-copyable
    mock_buffer(const mock_buffer&)            = delete;
    mock_buffer& operator=(const mock_buffer&) = delete;

    // --- buffer_base derived-class interface ---

    // storage (mutable)
    char*
    storage() noexcept
    {
        return m_storage;
    }

    // storage (const)
    const char*
    storage() const noexcept
    {
        return m_storage;
    }

    // capacity
    std::size_t
    capacity() const noexcept
    {
        return m_capacity;
    }

    // grow
    //   reallocates storage to at least _new_capacity bytes.
    // Returns true on success.  Returns false if a failure has
    // been injected via force_next_grow_failure() or if std::
    // nothrow allocation returns null.
    bool
    grow(
        std::size_t _new_capacity
    ) noexcept
    {
        ++m_grow_calls;

        // honor an injected failure
        if (m_force_next_grow_failure)
        {
            m_force_next_grow_failure = false;
            return false;
        }

        // already large enough - nothing to do
        if (_new_capacity <= m_capacity)
        {
            return true;
        }

        char* nb = new (std::nothrow) char[_new_capacity];

        // allocation failure
        if (!nb)
        {
            return false;
        }

        // copy existing payload
        if ( (m_storage)     &&
             (m_capacity > 0) )
        {
            std::memcpy(nb, m_storage, m_capacity);
        }

        // zero the newly-acquired region for test stability
        std::memset(nb + m_capacity,
                    0,
                    _new_capacity - m_capacity);

        delete[] m_storage;

        m_storage  = nb;
        m_capacity = _new_capacity;

        return true;
    }

    // --- test hooks ---

    // force_next_grow_failure
    //   arms a one-shot failure for the next grow() call.
    void
    force_next_grow_failure() noexcept
    {
        m_force_next_grow_failure = true;

        return;
    }

    // grow_call_count
    //   total number of grow() invocations observed.
    std::size_t
    grow_call_count() const noexcept
    {
        return m_grow_calls;
    }

private:
    char*       m_storage;
    std::size_t m_capacity;
    bool        m_force_next_grow_failure;
    std::size_t m_grow_calls;
};


// =============================================================================
//  test function declarations
// =============================================================================

// --- I.   growth strategy enum ---
void tests_buffer_growth_strategy(test_handler& _handler);

// --- II.  cursor model enum ---
void tests_buffer_cursor_model(test_handler& _handler);

// --- III. growth policies ---
void tests_fixed_growth_policy(test_handler& _handler);
void tests_linear_growth_policy(test_handler& _handler);
void tests_exponential_growth_policy(test_handler& _handler);
void tests_page_growth_policy(test_handler& _handler);

// --- IV.  cursor policies ---
void tests_write_only_cursor_policy(test_handler& _handler);
void tests_dual_cursor_policy(test_handler& _handler);

// --- V.   buffer_base ---
//   aliases / constants / predicates
void tests_buffer_base_aliases(test_handler& _handler);
void tests_buffer_base_constants(test_handler& _handler);
void tests_buffer_base_empty(test_handler& _handler);
void tests_buffer_base_full(test_handler& _handler);
void tests_buffer_base_size(test_handler& _handler);
void tests_buffer_base_writable_and_position(test_handler& _handler);

//   write operations
void tests_buffer_base_write(test_handler& _handler);
void tests_buffer_base_write_byte(test_handler& _handler);
void tests_buffer_base_write_fill(test_handler& _handler);
void tests_buffer_base_write_head_commit(test_handler& _handler);

//   read operations (dual cursor only)
void tests_buffer_base_read(test_handler& _handler);
void tests_buffer_base_peek(test_handler& _handler);
void tests_buffer_base_readable_and_read_position(test_handler& _handler);
void tests_buffer_base_advance(test_handler& _handler);
void tests_buffer_base_compact(test_handler& _handler);

//   lifecycle / capacity / move
void tests_buffer_base_reset(test_handler& _handler);
void tests_buffer_base_clear(test_handler& _handler);
void tests_buffer_base_reserve(test_handler& _handler);
void tests_buffer_base_move_construction(test_handler& _handler);
void tests_buffer_base_move_assignment(test_handler& _handler);

// --- VI.  growth policy selection ---
void tests_select_growth_policy(test_handler& _handler);

// --- VII. cursor policy selection ---
void tests_select_cursor_policy(test_handler& _handler);

// --- VIII. default policy aliases ---
void tests_default_policy_aliases(test_handler& _handler);


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_CONTAINER_BUFFER_
