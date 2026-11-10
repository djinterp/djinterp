/*******************************************************************************
* djinterp [test]                                               buffer_tests.hpp
*
*   Declarations and helper types for the buffer.hpp unit test suite.
* Each tests_<group> function records its assertions with
* record_assertion() and returns its verdict; DTest runs them through the spec
* provider (buffer_spec);
* one .cpp file backs each section listed below.
*
*   HELPER TYPE:
*   mock_buffer is a minimal CRTP-derived buffer that owns a heap-
* allocated char array and implements the storage()/capacity()/grow()
* contract that DBufferBase depends on.  It also exposes test-only
* hooks (force_next_grow_failure(), grow_call_count()) so test cases
* can drive failure-path coverage without relying on allocator
* exhaustion.
*
*
* path:      /tests/djinterp/cpp/container/buffer/buffer_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.11.09
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

V.    DBufferBase
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
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/buffer/buffer.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// record_assertion
//   function: one check of a converted body: on failure it prints what was
// expected and clears the body's verdict.  (The bodies used DTest's retired
// handler API, record_assertion(handler, condition, what).)
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what
)
{
    if (!_condition)
    {
        std::printf("    ! %s\n", _what);
        _ok = false;
    }

    return;
}

// record_assertion (with detail)
//   function: the five-argument form the bodies also used: the detail and
// the extra text are printed with a failure.
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what,
    const char* _detail,
    const char* _extra = nullptr
)
{
    if (!_condition)
    {
        std::printf("    ! %s (%s%s%s)\n",
                    _what,
                    _detail ? _detail : "",
                    _extra ? "; " : "",
                    _extra ? _extra : "");
        _ok = false;
    }

    return;
}

// =============================================================================
//  helper: mock_buffer
// =============================================================================

// mock_buffer
//   class: minimal concrete buffer derived from DBufferBase.
// Owns a heap-allocated char array and satisfies the derived-
// class interface (storage / capacity / grow) required by the
// CRTP base.  Exposes test hooks to inject grow() failures and
// observe the number of grow() invocations.
template<typename _GrowthPolicy,
         typename _CursorPolicy>
class mock_buffer
    : public DBufferBase<mock_buffer<_GrowthPolicy, _CursorPolicy>,
                         _GrowthPolicy,
                         _CursorPolicy>
{
private:
    using base_type =
        DBufferBase<mock_buffer<_GrowthPolicy, _CursorPolicy>,
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
    mock_buffer(
        const mock_buffer&
    ) = delete;
    mock_buffer& operator=(
        const mock_buffer&
    ) = delete;

    // --- DBufferBase derived-class interface ---

    // storage (mutable)
    char* storage() noexcept
    {
        return m_storage;
    }

    // storage (const)
    const char* storage() const noexcept
    {
        return m_storage;
    }

    // capacity
    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

    // grow
    //   reallocates storage to at least _new_capacity bytes.
    // Returns true on success.  Returns false if a failure has
    // been injected via force_next_grow_failure() or if std::
    // nothrow allocation returns null.
    bool grow(
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
    void force_next_grow_failure() noexcept
    {
        m_force_next_grow_failure = true;

        return;
    }

    // grow_call_count
    //   total number of grow() invocations observed.
    std::size_t grow_call_count() const noexcept
    {
        return m_grow_calls;
    }

private:
    char*       m_storage;
    std::size_t m_capacity;
    bool        m_force_next_grow_failure;
    std::size_t m_grow_calls;
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



// =============================================================================
//  test function declarations
// =============================================================================

// --- I.   growth strategy enum ---
bool tests_buffer_growth_strategy();

// --- II.  cursor model enum ---
bool tests_buffer_cursor_model();

// --- III. growth policies ---
bool tests_fixed_growth_policy();
bool tests_linear_growth_policy();
bool tests_exponential_growth_policy();
bool tests_page_growth_policy();

// --- IV.  cursor policies ---
bool tests_write_only_cursor_policy();
bool tests_dual_cursor_policy();

// --- V.   DBufferBase ---
//   aliases / constants / predicates
bool tests_buffer_base_aliases();
bool tests_buffer_base_constants();
bool tests_buffer_base_empty();
bool tests_buffer_base_full();
bool tests_buffer_base_size();
bool tests_buffer_base_writable_and_position();

//   write operations
bool tests_buffer_base_write();
bool tests_buffer_base_write_byte();
bool tests_buffer_base_write_fill();
bool tests_buffer_base_write_head_commit();

//   read operations (dual cursor only)
bool tests_buffer_base_read();
bool tests_buffer_base_peek();
bool tests_buffer_base_readable_and_read_position();
bool tests_buffer_base_advance();
bool tests_buffer_base_compact();

//   lifecycle / capacity / move
bool tests_buffer_base_reset();
bool tests_buffer_base_clear();
bool tests_buffer_base_reserve();
bool tests_buffer_base_move_construction();
bool tests_buffer_base_move_assignment();

// --- VI.  growth policy selection ---
bool tests_select_growth_policy();

// --- VII. cursor policy selection ---
bool tests_select_cursor_policy();

// --- VIII. default policy aliases ---
bool tests_default_policy_aliases();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// buffer_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
buffer_spec()
{
    return dt::module_spec{
        "buffer",
        "foundational buffer module for the djinterp container framework",
        {
            dt::block_spec{
                "enums",
                "Growth strategy enum",
                {
                    { "tests_buffer_growth_strategy",
                      "buffer_growth_strategy enum sanity coverage",
                      &tests_buffer_growth_strategy },
                }
            },
            dt::block_spec{
                "enums",
                "Cursor model enum",
                {
                    { "tests_buffer_cursor_model",
                      "buffer_cursor_model enum sanity coverage",
                      &tests_buffer_cursor_model },
                }
            },
            dt::block_spec{
                "growth_policies",
                "Growth policies",
                {
                    { "tests_fixed_growth_policy",
                      "Covers the fixed_growth_policy struct",
                      &tests_fixed_growth_policy },
                    { "tests_linear_growth_policy",
                      "Covers the DLinearGrowthPolicy template",
                      &tests_linear_growth_policy },
                    { "tests_exponential_growth_policy",
                      "Covers the exponential_growth_policy struct",
                      &tests_exponential_growth_policy },
                    { "tests_page_growth_policy",
                      "Covers the DPageGrowthPolicy template",
                      &tests_page_growth_policy },
                }
            },
            dt::block_spec{
                "cursor_policies",
                "Cursor policies",
                {
                    { "tests_write_only_cursor_policy",
                      "Covers the write_only_cursor_policy struct",
                      &tests_write_only_cursor_policy },
                    { "tests_dual_cursor_policy",
                      "Covers the dual_cursor_policy struct",
                      &tests_dual_cursor_policy },
                }
            },
            dt::block_spec{
                "dbufferbase",
                "DBufferBase",
                {
                    { "tests_buffer_base_aliases",
                      "Verifies DBufferBase's public type aliases",
                      &tests_buffer_base_aliases },
                    { "tests_buffer_base_constants",
                      "Verifies DBufferBase's static constant members",
                      &tests_buffer_base_constants },
                    { "tests_buffer_base_empty",
                      "Covers empty() across both cursor models",
                      &tests_buffer_base_empty },
                    { "tests_buffer_base_full",
                      "Covers full() across growable and fixed buffers",
                      &tests_buffer_base_full },
                    { "tests_buffer_base_size",
                      "Covers size() across both cursor models",
                      &tests_buffer_base_size },
                    { "tests_buffer_base_writable_and_position",
                      "Covers writable() and write_position()",
                      &tests_buffer_base_writable_and_position },
                    { "tests_buffer_base_write",
                      "buffer base write",
                      &tests_buffer_base_write },
                    { "tests_buffer_base_write_byte",
                      "Covers the single-byte write_byte() entry point",
                      &tests_buffer_base_write_byte },
                    { "tests_buffer_base_write_fill",
                      "Covers write_fill()",
                      &tests_buffer_base_write_fill },
                    { "tests_buffer_base_write_head_commit",
                      "Covers the direct-write workflow: write_head() + "
                      "commit()",
                      &tests_buffer_base_write_head_commit },
                    { "tests_buffer_base_read",
                      "buffer base read",
                      &tests_buffer_base_read },
                    { "tests_buffer_base_peek",
                      "Covers peek() on a dual-cursor buffer",
                      &tests_buffer_base_peek },
                    { "tests_buffer_base_readable_and_read_position",
                      "Covers readable() and read_position() on a dual-cursor "
                      "buffer",
                      &tests_buffer_base_readable_and_read_position },
                    { "tests_buffer_base_advance",
                      "Covers advance() on a dual-cursor buffer",
                      &tests_buffer_base_advance },
                    { "tests_buffer_base_compact",
                      "Covers compact() on a dual-cursor buffer",
                      &tests_buffer_base_compact },
                    { "tests_buffer_base_reset",
                      "Covers reset() across both cursor policies",
                      &tests_buffer_base_reset },
                    { "tests_buffer_base_clear",
                      "Covers clear() across both cursor policies and storage "
                      "states",
                      &tests_buffer_base_clear },
                    { "tests_buffer_base_reserve",
                      "Covers reserve() across growable and non-growable "
                      "buffers",
                      &tests_buffer_base_reserve },
                    { "tests_buffer_base_move_construction",
                      "Covers the implicit base_type move constructor via "
                      "mock_buffer's move constructor (which forwards through "
                      "base_type(base_type&&))",
                      &tests_buffer_base_move_construction },
                    { "tests_buffer_base_move_assignment",
                      "Covers move assignment on DBufferBase via "
                      "mock_buffer's forwarding move-assignment operator",
                      &tests_buffer_base_move_assignment },
                }
            },
            dt::block_spec{
                "policy_selection",
                "Growth policy selection",
                {
                    { "tests_select_growth_policy",
                      "Covers select_growth_policy and its alias "
                      "select_growth_policy_t",
                      &tests_select_growth_policy },
                }
            },
            dt::block_spec{
                "policy_selection",
                "Cursor policy selection",
                {
                    { "tests_select_cursor_policy",
                      "Covers select_cursor_policy and its alias "
                      "select_cursor_policy_t",
                      &tests_select_cursor_policy },
                }
            },
            dt::block_spec{
                "defaults",
                "Default policy aliases",
                {
                    { "tests_default_policy_aliases",
                      "Covers the default_growth_policy and "
                      "DDefaultCursorPolicy aliases",
                      &tests_default_policy_aliases },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_CONTAINER_BUFFER_
