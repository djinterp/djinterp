/******************************************************************************
* djinterp [test]                                      test_suite_buffer.hpp
*
*   Example test suite exercising the buffer.hpp module.  Demonstrates
* how the DTest framework types compose: test_type registry for kind
* resolution, flat-vector test construction with hierarchical depth,
* and printer-driven output.
*
*   CONCRETE TEST BUFFER:
*   buffer_base is CRTP; it requires a derived class that owns memory
* and provides storage()/capacity()/grow().  This file defines
* heap_test_buffer — a minimal heap-backed derivation used
* exclusively for testing.
*
*   SUITE BUILDER:
*   suite_builder is a small helper that manages a flat
* std::vector<basic_test> with manual depth tracking and
* parent-status propagation.  Interior nodes receive
* status_passed when all children pass, status_failed
* otherwise.  This emulates tree-like structure without
* requiring test_tree.
*
*   TEST COVERAGE:
*     I.   Growth policies      — constexpr properties, compute()
*     II.  Cursor policies       — init, advance, readable, compact
*     III. Policy selection      — enum-to-type mapping
*     IV.  Buffer operations     — write, read, peek, compact, reset,
*                                  clear, reserve, predicates
*
*   ENTRY POINT:
*   run_buffer_suite() builds the suite, walks it through a
* test_printer, and returns true if all assertions passed.
* An optional suite_result out-parameter receives leaf-only
* assertion tallies for use by a host master-suite renderer;
* the printer's own print_context counts every walked node
* (interior + leaf) and is therefore unsuitable for host-side
* assertion summaries.
*
*
* path:      /inc/djinterp/test/suites/test_suite_buffer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.14
******************************************************************************/

#ifndef DJINTERP_TEST_SUITE_BUFFER_
#define DJINTERP_TEST_SUITE_BUFFER_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/core/djinterp.hpp"
#include "../../../../../inc/djinterp/core/container/buffer/buffer.hpp"
#include "../../../../../inc/djinterp/test/test_common.hpp"
#include "../../../../../inc/djinterp/test/test_defaults.hpp"
#include "../../../../../inc/djinterp/test/test_object.hpp"
#include "../../../../../inc/djinterp/test/test_type.hpp"
#include "../../../../../inc/djinterp/test/test_printer.hpp"


NS_DJINTERP
NS_TESTING

using namespace djinterp::test;

///////////////////////////////////////////////////////////////////////////////
///                I.   CONCRETE TEST BUFFER                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

// heap_test_buffer
//   class: minimal heap-backed buffer derived from
// buffer_base.  Provides the four CRTP contract methods
// (storage, capacity, grow) over a raw new[]/delete[]
// allocation.  Used exclusively for testing.
template<typename _Growth,
         typename _Cursor>
class heap_test_buffer :
    public container::buffer_base<heap_test_buffer<_Growth, _Cursor>,
                                  _Growth,
                                  _Cursor>
{
    using base_type = container::buffer_base<
        heap_test_buffer<_Growth, _Cursor>,
        _Growth,
        _Cursor>;

    friend base_type;

public:
    // default (zero capacity)
    heap_test_buffer()
        : base_type(),
          m_data(nullptr),
          m_cap(0)
    {}

    // from initial capacity
    explicit heap_test_buffer(
        std::size_t _capacity
    )
        : base_type(),
          m_data(nullptr),
          m_cap(0)
    {
        if (_capacity > 0)
        {
            m_data = new (std::nothrow) char[_capacity];

            if (m_data)
            {
                m_cap = _capacity;
                std::memset(m_data, 0, m_cap);
            }
        }
    }

    ~heap_test_buffer()
    {
        delete[] m_data;
    }

    // move
    heap_test_buffer(
        heap_test_buffer&& _other
    ) D_NOEXCEPT
        : base_type(
              static_cast<base_type&&>(_other)),
          m_data(_other.m_data),
          m_cap(_other.m_cap)
    {
        _other.m_data = nullptr;
        _other.m_cap  = 0;
    }

    // non-copyable
    heap_test_buffer(const heap_test_buffer&)            = delete;
    heap_test_buffer& operator=(const heap_test_buffer&) = delete;

    // raw data access
    const char*
    data() const D_NOEXCEPT
    {
        return m_data;
    }

private:
    // CRTP contract: storage
    char*
    storage() D_NOEXCEPT
    {
        return m_data;
    }

    const char*
    storage() const D_NOEXCEPT
    {
        return m_data;
    }

    // CRTP contract: capacity
    std::size_t
    capacity() const D_NOEXCEPT
    {
        return m_cap;
    }

    // CRTP contract: grow
    bool
    grow(
        std::size_t _new_cap
    ) D_NOEXCEPT
    {
        if (_new_cap <= m_cap)
        {
            return true;
        }

        char* p = new (std::nothrow) char[_new_cap];

        if (!p)
        {
            return false;
        }

        // copy existing data
        if (m_data)
        {
            std::memcpy(p, m_data, m_cap);
            delete[] m_data;
        }

        // zero the new region
        std::memset(p + m_cap, 0, _new_cap - m_cap);

        m_data = p;
        m_cap  = _new_cap;

        return true;
    }

    char*       m_data;
    std::size_t m_cap;
};

// convenience aliases for common buffer configurations
using fixed_wo_buffer = heap_test_buffer<
    container::fixed_growth_policy,
    container::write_only_cursor_policy>;

using fixed_dual_buffer = heap_test_buffer<
    container::fixed_growth_policy,
    container::dual_cursor_policy>;

using exp_wo_buffer = heap_test_buffer<
    container::exponential_growth_policy,
    container::write_only_cursor_policy>;

using exp_dual_buffer = heap_test_buffer<
    container::exponential_growth_policy,
    container::dual_cursor_policy>;

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                II.  SUITE BUILDER                                        ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

// suite_builder
//   struct: manages a flat vector of basic_test with
// manual depth tracking and parent-status propagation.
// Interior nodes are pushed pending; when popped, their
// status is set to passed if all children passed, or
// failed if any child failed.
struct suite_builder
{
    std::vector<basic_test>  nodes;
    std::vector<std::size_t> parent_indices;
    std::size_t              current_depth;

    suite_builder()
        : nodes(),
          parent_indices(),
          current_depth(0)
    {}

    // begin_group
    //   pushes an interior node (module, block, or test)
    // and increments depth.
    void
    begin_group(
        test_type_id _kind,
        const char*  _name
    )
    {
        basic_test t(_kind);
        t.set_name(_name);
        t.set_status(basic_test::status_pending);
        t.set_depth(current_depth);

        parent_indices.push_back(nodes.size());
        nodes.push_back(t);

        ++current_depth;

        return;
    }

    // end_group
    //   pops the current interior node and propagates
    // child status.  All children between the parent
    // index and the current end are scanned.
    void
    end_group()
    {
        if (parent_indices.empty())
        {
            return;
        }

        std::size_t parent_idx = parent_indices.back();
        parent_indices.pop_back();
        --current_depth;

        // scan children for failures
        bool any_failed = false;

        for (std::size_t i = parent_idx + 1;
             i < nodes.size();
             ++i)
        {
            if (nodes[i].failed())
            {
                any_failed = true;
                break;
            }
        }

        nodes[parent_idx].set_status(
            any_failed
                ? basic_test::status_failed
                : basic_test::status_passed);

        return;
    }

    // check
    //   pushes a leaf assertion with the given result.
    void
    check(
        bool        _result,
        const char* _name,
        const char* _message_pass = nullptr,
        const char* _message_fail = nullptr
    )
    {
        basic_test t(
            D_TEST_KIND_ASSERT,
            _result,
            _name,
            _message_pass,
            _message_fail);

        t.set_depth(current_depth);

        nodes.push_back(t);

        return;
    }
};

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                III. TEST SECTIONS                                        ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // -----------------------------------------------------------------
    //  growth policy tests
    // -----------------------------------------------------------------

    D_INLINE void
    test_growth_policies(
        suite_builder& _b
    )
    {
        _b.begin_group(D_TEST_KIND_TEST_BLOCK,
                       "growth_policies");

        _b.begin_group(D_TEST_KIND_TEST,
                       "fixed_growth_policy");
        {
            using fp = container::fixed_growth_policy;

            _b.check(
                fp::strategy == container::DBufferGrowth::none,
                "strategy is none");

            _b.check(
                fp::can_grow == false,
                "can_grow is false");

            _b.check(
                fp::compute(256, 512) == 256,
                "compute returns current capacity unchanged");
        }
        _b.end_group();

        // --- linear ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "linear_growth_policy");
        {
            using lp = container::linear_growth_policy<>;

            _b.check(
                lp::strategy == container::DBufferGrowth::linear,
                "strategy is linear");

            _b.check(
                lp::can_grow == true,
                "can_grow is true");

            _b.check(
                lp::increment == 4096,
                "default increment is 4096");

            _b.check(
                lp::compute(0, 100) == 4096,
                "compute from 0 rounds up to one increment");

            _b.check(
                lp::compute(4096, 5000) == 8192,
                "compute rounds up to next increment boundary");

            // custom increment
            using lp2 = container::linear_growth_policy<64>;

            _b.check(
                lp2::compute(0, 100) == 128,
                "custom increment: 64-byte steps to reach 100");
        }
        _b.end_group();

        // --- exponential ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "exponential_growth_policy");
        {
            using ep = container::exponential_growth_policy;

            _b.check(
                ep::strategy == container::DBufferGrowth::exponential,
                "strategy is exponential");

            _b.check(
                ep::can_grow == true,
                "can_grow is true");

            _b.check(
                ep::min_capacity == 64,
                "min_capacity is 64");

            _b.check(
                ep::compute(0, 10) == 64,
                "compute from 0 uses min_capacity");

            _b.check(
                ep::compute(64, 100) == 128,
                "compute doubles 64 to 128 for required 100");

            _b.check(
                ep::compute(128, 500) == 512,
                "compute doubles until >= required");
        }
        _b.end_group();

        // --- page ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "page_growth_policy");
        {
            using pp = container::page_growth_policy<>;

            _b.check(
                pp::strategy == container::DBufferGrowth::page_aligned,
                "strategy is page_aligned");

            _b.check(
                pp::can_grow == true,
                "can_grow is true");

            _b.check(
                pp::page_size == 4096,
                "default page_size is 4096");

            _b.check(
                pp::compute(0, 1) == 4096,
                "compute rounds 1 byte up to one page");

            _b.check(
                pp::compute(0, 4096) == 4096,
                "compute exact page boundary");

            _b.check(
                pp::compute(0, 4097) == 8192,
                "compute one byte over rounds to two pages");
        }
        _b.end_group();

        _b.end_group();  // growth_policies

        return;
    }


    // -----------------------------------------------------------------
    //  cursor policy tests
    // -----------------------------------------------------------------

    D_INLINE void
    test_cursor_policies(
        suite_builder& _b
    )
    {
        _b.begin_group(D_TEST_KIND_TEST_BLOCK,
                       "cursor_policies");

        // --- write_only ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "write_only_cursor_policy");
        {
            using wo = container::write_only_cursor_policy;

            auto state = wo::init();

            _b.check(
                state.write_pos == 0,
                "init: write_pos is 0");

            _b.check(
                wo::writable(state, 256) == 256,
                "writable equals full capacity at start");

            _b.check(
                wo::written(state) == 0,
                "written is 0 at start");

            wo::advance_write(state, 100);

            _b.check(
                wo::written(state) == 100,
                "written is 100 after advance_write(100)");

            _b.check(
                wo::writable(state, 256) == 156,
                "writable is 156 after writing 100 of 256");

            wo::reset(state);

            _b.check(
                wo::written(state) == 0,
                "written is 0 after reset");
        }
        _b.end_group();

        // --- dual ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "dual_cursor_policy");
        {
            using dc = container::dual_cursor_policy;

            auto state = dc::init();

            _b.check(
                ( state.write_pos == 0 &&
                  state.read_pos  == 0 ),
                "init: both cursors at 0");

            dc::advance_write(state, 200);

            _b.check(
                dc::readable(state) == 200,
                "readable is 200 after writing 200");

            dc::advance_read(state, 50);

            _b.check(
                dc::readable(state) == 150,
                "readable is 150 after consuming 50");

            _b.check(
                dc::consumed(state) == 50,
                "consumed is 50");

            // compact
            char buf[256];
            std::memset(buf, 0, sizeof(buf));

            // fill [0..200) with 'A', simulate the written region
            std::memset(buf, 'A', 200);

            dc::compact(state, buf);

            _b.check(
                ( state.read_pos  == 0 &&
                  state.write_pos == 150 ),
                "compact: cursors shifted, 150 bytes remain");

            _b.check(
                buf[0] == 'A',
                "compact: unconsumed data shifted to front");

            dc::reset(state);

            _b.check(
                ( dc::written(state)  == 0 &&
                  dc::consumed(state) == 0 ),
                "reset: both cursors at 0");
        }
        _b.end_group();

        _b.end_group();  // cursor_policies

        return;
    }


    // -----------------------------------------------------------------
    //  policy selection tests
    // -----------------------------------------------------------------

    D_INLINE void
    test_policy_selection(
        suite_builder& _b
    )
    {
        _b.begin_group(D_TEST_KIND_TEST_BLOCK,
                       "policy_selection");

        // --- growth ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "select_growth_policy");
        {
            using namespace container;

            _b.check(
                (std::is_same<
                    select_growth_policy_t<DBufferGrowth::none>,
                    fixed_growth_policy>::value),
                "none maps to fixed_growth_policy");

            _b.check(
                (std::is_same<
                    select_growth_policy_t<DBufferGrowth::linear>,
                    linear_growth_policy<>>::value),
                "linear maps to linear_growth_policy<>");

            _b.check(
                (std::is_same<
                    select_growth_policy_t<DBufferGrowth::exponential>,
                    exponential_growth_policy>::value),
                "exponential maps to exponential_growth_policy");

            _b.check(
                (std::is_same<
                    select_growth_policy_t<DBufferGrowth::page_aligned>,
                    page_growth_policy<>>::value),
                "page_aligned maps to page_growth_policy<>");
        }
        _b.end_group();

        // --- cursor ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "select_cursor_policy");
        {
            using namespace container;

            _b.check(
                (std::is_same<
                    select_cursor_policy_t<DBufferCursorModel::write_only>,
                    write_only_cursor_policy>::value),
                "write_only maps to write_only_cursor_policy");

            _b.check(
                (std::is_same<
                    select_cursor_policy_t<DBufferCursorModel::dual>,
                    dual_cursor_policy>::value),
                "dual maps to dual_cursor_policy");
        }
        _b.end_group();

        _b.end_group();  // policy_selection

        return;
    }


    // -----------------------------------------------------------------
    //  buffer operation tests
    // -----------------------------------------------------------------

    D_INLINE void
    test_buffer_operations(
        suite_builder& _b
    )
    {
        _b.begin_group(D_TEST_KIND_TEST_BLOCK,
                       "buffer_operations");

        // --- predicates at construction ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "initial_state");
        {
            fixed_wo_buffer buf(64);

            _b.check(
                buf.empty(),
                "empty after construction");

            _b.check(
                buf.size() == 0,
                "size is 0 after construction");

            _b.check(
                buf.writable() == 64,
                "writable equals initial capacity");

            _b.check(
                !buf.full(),
                "not full with 64 bytes available");
        }
        _b.end_group();

        // --- write and size ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "write_and_size");
        {
            fixed_wo_buffer buf(64);

            const char data[] = "hello";
            std::size_t n = buf.write(data, 5);

            _b.check(
                n == 5,
                "write returns 5 for 5-byte payload");

            _b.check(
                buf.size() == 5,
                "size is 5 after writing 5 bytes");

            _b.check(
                !buf.empty(),
                "not empty after write");

            _b.check(
                buf.writable() == 59,
                "writable is 59 after writing 5 of 64");
        }
        _b.end_group();

        // --- write_byte ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "write_byte");
        {
            fixed_wo_buffer buf(8);

            _b.check(
                buf.write_byte('X'),
                "write_byte returns true on success");

            _b.check(
                buf.size() == 1,
                "size is 1 after one write_byte");

            _b.check(
                buf.data()[0] == 'X',
                "written byte is 'X'");
        }
        _b.end_group();

        // --- write_fill ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "write_fill");
        {
            fixed_wo_buffer buf(16);

            std::size_t n = buf.write_fill('.', 10);

            _b.check(
                n == 10,
                "write_fill returns 10");

            _b.check(
                buf.size() == 10,
                "size is 10 after fill");

            bool all_dots = true;

            for (std::size_t i = 0; i < 10; ++i)
            {
                if (buf.data()[i] != '.')
                {
                    all_dots = false;
                    break;
                }
            }

            _b.check(
                all_dots,
                "all 10 bytes are '.'");
        }
        _b.end_group();

        // --- fixed buffer overflow ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "fixed_overflow");
        {
            fixed_wo_buffer buf(4);

            std::size_t n = buf.write("abcdefgh", 8);

            _b.check(
                n == 4,
                "write clamps to capacity (4 of 8)");

            _b.check(
                buf.full(),
                "full after writing to capacity");

            _b.check(
                buf.write_byte('Z') == false,
                "write_byte returns false when full");
        }
        _b.end_group();

        // --- growable buffer ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "exponential_growth");
        {
            exp_wo_buffer buf(0);

            const char payload[] = "grow me";
            std::size_t n = buf.write(payload, 7);

            _b.check(
                n == 7,
                "write succeeds on zero-capacity growable buffer");

            _b.check(
                buf.size() == 7,
                "size is 7 after growth + write");

            _b.check(
                std::memcmp(buf.data(), payload, 7) == 0,
                "written content matches payload");
        }
        _b.end_group();

        // --- dual cursor read ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "dual_cursor_read");
        {
            fixed_dual_buffer buf(64);

            const char msg[] = "hello world";
            buf.write(msg, 11);

            char out[16];
            std::memset(out, 0, sizeof(out));

            std::size_t n = buf.read(out, 5);

            _b.check(
                n == 5,
                "read returns 5 for 5-byte request");

            _b.check(
                std::memcmp(out, "hello", 5) == 0,
                "read data matches first 5 bytes");

            _b.check(
                buf.readable() == 6,
                "readable is 6 after reading 5 of 11");
        }
        _b.end_group();

        // --- peek ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "peek");
        {
            fixed_dual_buffer buf(32);

            buf.write("ABCD", 4);

            const char* p = buf.peek();

            _b.check(
                p != nullptr,
                "peek returns non-null pointer");

            _b.check(
                p[0] == 'A',
                "peek sees first byte without advancing");

            _b.check(
                buf.readable() == 4,
                "readable unchanged after peek");
        }
        _b.end_group();

        // --- compact ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "compact");
        {
            fixed_dual_buffer buf(16);

            buf.write("0123456789", 10);

            // consume first 6 bytes
            char tmp[6];
            buf.read(tmp, 6);

            _b.check(
                buf.readable() == 4,
                "readable is 4 before compact");

            _b.check(
                buf.writable() == 6,
                "writable is 6 before compact");

            std::size_t remaining = buf.compact();

            _b.check(
                remaining == 4,
                "compact returns 4 unconsumed bytes");

            _b.check(
                buf.writable() == 12,
                "writable is 12 after compact (tail freed)");

            // verify the unconsumed data is intact
            _b.check(
                std::memcmp(buf.peek(), "6789", 4) == 0,
                "unconsumed data shifted to front intact");
        }
        _b.end_group();

        // --- reset ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "reset");
        {
            fixed_dual_buffer buf(32);

            buf.write("test", 4);

            _b.check(
                buf.size() > 0,
                "size > 0 before reset");

            buf.reset();

            _b.check(
                buf.empty(),
                "empty after reset");

            _b.check(
                buf.writable() == 32,
                "full capacity restored after reset");
        }
        _b.end_group();

        // --- clear ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "clear");
        {
            fixed_wo_buffer buf(16);

            buf.write("ZZZZ", 4);
            buf.clear();

            _b.check(
                buf.empty(),
                "empty after clear");

            // verify storage is zeroed
            bool all_zero = true;

            for (std::size_t i = 0; i < 16; ++i)
            {
                if (buf.data()[i] != 0)
                {
                    all_zero = false;
                    break;
                }
            }

            _b.check(
                all_zero,
                "storage is zeroed after clear");
        }
        _b.end_group();

        // --- reserve ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "reserve");
        {
            exp_wo_buffer buf(0);

            _b.check(
                buf.reserve(256),
                "reserve(256) succeeds on growable buffer");

            _b.check(
                buf.writable() >= 256,
                "writable >= 256 after reserve");

            // fixed buffer rejects growth
            fixed_wo_buffer fixed(16);

            _b.check(
                fixed.reserve(16),
                "reserve at current capacity succeeds");

            _b.check(
                !fixed.reserve(32),
                "reserve beyond capacity fails on fixed buffer");
        }
        _b.end_group();

        // --- write_head / commit ---
        _b.begin_group(D_TEST_KIND_TEST,
                       "write_head_commit");
        {
            fixed_wo_buffer buf(32);

            char* head = buf.write_head();

            // direct-write pattern
            head[0] = 'D';
            head[1] = 'I';
            head[2] = 'R';
            buf.commit(3);

            _b.check(
                buf.size() == 3,
                "size is 3 after commit(3)");

            _b.check(
                std::memcmp(buf.data(), "DIR", 3) == 0,
                "direct-written bytes match");
        }
        _b.end_group();

        _b.end_group();  // buffer_operations

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                IV.  ENTRY POINT                                          ///
///////////////////////////////////////////////////////////////////////////////

// suite_result
//   struct: leaf-only assertion tallies produced by a suite
// run.  The printer's print_context accumulates counts for
// every walked node (interior + leaf); this struct surfaces
// the leaf-only subset so a host master-suite renderer can
// bind assertion summaries that match what the suite's own
// inner footer reports.
struct suite_result
{
    bool        passed;
    std::size_t leaf_total;
    std::size_t leaf_passed;
    std::size_t leaf_failed;

    suite_result() D_NOEXCEPT
        : passed(false),
          leaf_total(0),
          leaf_passed(0),
          leaf_failed(0)
    {}
};


// D_TEST_FMT_SUITE_HEADER
//   constant: header banner format for suite output, matching
// the djinterp Master Test Suite visual style.
D_STATIC const char* const D_TEST_FMT_SUITE_HEADER =
    "========================================"
    "========================================\n"
    "  TESTING: {suite_name}\n"
    "========================================"
    "========================================\n"
    "  Description: {suite_description}\n"
    "========================================"
    "========================================\n\n"
    "  Starting test suite execution...\n\n";

// D_TEST_FMT_SUITE_FOOTER
//   constant: footer format for suite output. Contains the
// assertion summary and exit status line.  Specifiers are
// bound manually with leaf-only counts.
D_STATIC const char* const D_TEST_FMT_SUITE_FOOTER =
    "\n  ASSERTION SUMMARY:\n"
    "    Total Assertions:     {leaf_total}\n"
    "    Assertions Passed:    {leaf_passed}\n"
    "    Assertions Failed:    {leaf_failed}\n"
    "    Assertion Pass Rate:  {pass_rate}\n"
    "\n  exit: {exit_status}\n";

// D_TEST_FMT_NODE_WALK
//   constant: per-node format string.  The {number} specifier
// is re-bound by the node binder to include the trailing
// period and space for leaf nodes, or a bullet ". " for
// interior nodes.
D_STATIC const char* const D_TEST_FMT_NODE_WALK =
    "{indent}{number}{symbol} {name} [{status}]\n";


// run_buffer_suite
//   function: builds and runs the buffer.hpp test suite.
// Output is directed to the provided test_printer.  When
// _out is non-null, it is populated with leaf-only assertion
// tallies for use by a host master-suite renderer.  Returns
// true if all leaf assertions passed.
D_INLINE bool
run_buffer_suite(
    test_printer& _printer,
    suite_result* _out = nullptr
)
{
    // build the test type registry
    auto types = make_default_test_type();

    // build the suite
    internal::suite_builder builder;

    builder.begin_group(D_TEST_KIND_MODULE,
                        "buffer.hpp");

    internal::test_growth_policies(builder);
    internal::test_cursor_policies(builder);
    internal::test_policy_selection(builder);
    internal::test_buffer_operations(builder);

    builder.end_group();  // module

    const auto& nodes = builder.nodes;

    // ---- pre-compute leaf assertion counts ----

    std::size_t leaf_total  = 0;
    std::size_t leaf_passed = 0;
    std::size_t leaf_failed = 0;

    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        if (types.is_leaf(nodes[i].type_id()))
        {
            ++leaf_total;

            if (nodes[i].passed())
            {
                ++leaf_passed;
            }
            else
            {
                ++leaf_failed;
            }
        }
    }

    // surface leaf-only counts to the caller.  The host
    // master-suite renderer binds from these rather than
    // from print_context::total, which counts every walked
    // node (interior + leaf).
    if (_out)
    {
        _out->leaf_total  = leaf_total;
        _out->leaf_passed = leaf_passed;
        _out->leaf_failed = leaf_failed;
        _out->passed      = (leaf_failed == 0);
    }

    // compute the decimal width of the largest leaf number
    // so the binder can left-pad single-digit numbers and
    // keep the symbol column aligned across the 9/10, 99/100,
    // etc. boundaries.
    std::size_t num_width = 1;
    {
        std::size_t n = leaf_total;

        while (n >= 10)
        {
            n /= 10;
            ++num_width;
        }
    }

    // ---- walk state ----
    // shared between the leaf extraction function and the
    // node binder to coordinate number formatting.

    struct walk_state
    {
        bool        is_leaf;
        std::size_t leaf_num;
    };

    walk_state wstate;
    wstate.is_leaf  = false;
    wstate.leaf_num = 0;

    // ---- configure header ----

    _printer.set_header_format(D_TEST_FMT_SUITE_HEADER);

    _printer.header_template().bind(
        "suite_name",
        "buffer.hpp");

    _printer.header_template().bind(
        "suite_description",
        "Buffer module: growth policies, cursor policies, "
        "policy selection, buffer operations");

    // ---- configure node format ----
    // the format omits ". " after {number}; the binder
    // re-binds {number} to include it contextually.

    _printer.set_node_format(D_TEST_FMT_NODE_WALK);

    // use 'none' numbering — the binder manages numbers
    _printer.set_numbering_mode(numbering_mode::none);

    // node binder: re-binds {number} to a width-padded
    // "N. " for leaves (right-aligned so the period column
    // stays fixed across the 9/10, 99/100, etc. boundaries)
    // or an empty string for interior nodes (indent alone
    // carries the hierarchy).
    _printer.set_node_binder(
        [&wstate, num_width](text::text_template& _tmpl,
                             std::size_t          /*_depth*/)
        {
            if (wstate.is_leaf)
            {
                char buf[32];

                std::snprintf(buf, sizeof(buf),
                              "%*zu. ",
                              static_cast<int>(num_width),
                              wstate.leaf_num);

                _tmpl.bind("number",
                           std::string(buf));
            }
            else
            {
                _tmpl.bind("number",
                           std::string());
            }

            return;
        });

    // ---- suppress built-in summary ----
    // the printer's context counts all nodes; we want
    // leaf-only assertion counts in the summary.

    _printer.set_summary_format("");

    // ---- configure footer ----

    _printer.set_footer_format(D_TEST_FMT_SUITE_FOOTER);

    // ---- walk the suite ----

    _printer.walk(
        nodes,
        [](const basic_test& _t) -> std::string
        {
            return _t.name()
                ? std::string(_t.name())
                : std::string("(unnamed)");
        },
        [](const basic_test& _t) -> std::string
        {
            return _t.message()
                ? std::string(_t.message())
                : std::string();
        },
        [](const basic_test& _t) -> std::size_t
        {
            return _t.depth();
        },
        [](const basic_test& _t) -> test_status
        {
            return static_cast<test_status>(_t.status());
        },
        [&types, &wstate](const basic_test& _t) -> bool
        {
            bool leaf = types.is_leaf(_t.type_id());

            wstate.is_leaf = leaf;

            if (leaf)
            {
                ++(wstate.leaf_num);
            }

            return leaf;
        },
        true,    // with header
        false,   // without built-in summary
        false);  // without footer (bind + print below)

    // ---- bind and print footer ----

    char   rate_buf[16];
    double rate = 0.0;

    if (leaf_total > 0)
    {
        rate = (static_cast<double>(leaf_passed) /
                static_cast<double>(leaf_total)) * 100.0;
    }

    std::snprintf(rate_buf, sizeof(rate_buf),
                  "%.2f%%", rate);

    _printer.footer_template().bind(
        "leaf_total",
        print_context::size_to_string(leaf_total));

    _printer.footer_template().bind(
        "leaf_passed",
        print_context::size_to_string(leaf_passed));

    _printer.footer_template().bind(
        "leaf_failed",
        print_context::size_to_string(leaf_failed));

    _printer.footer_template().bind(
        "pass_rate",
        std::string(rate_buf));

    _printer.footer_template().bind(
        "exit_status",
        (leaf_failed == 0)
            ? "SUCCESS"
            : "FAILURE");

    _printer.print_footer();

    return (leaf_failed == 0);
}

// run_buffer_suite (default printer)
//   function: convenience overload that creates a default
// printer and delegates to the printer-accepting overload.
// All format configuration is handled within the main
// overload.
D_INLINE bool
run_buffer_suite(
    suite_result* _out = nullptr
)
{
    test_printer printer;

    return run_buffer_suite(printer, _out);
}


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_SUITE_BUFFER_