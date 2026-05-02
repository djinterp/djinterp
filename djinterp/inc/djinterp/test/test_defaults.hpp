/******************************************************************************
* djinterp [test]                                            test_defaults.hpp
*
*   Default test framework configuration: the option enumeration, the
* five built-in test kinds, and their default metadata.
*
*   This header is the single source of truth for the DTest framework's
* default vocabulary of test types.  All built-in kind constants, rank
* assignments, leaf/interior flags, and default option content are
* defined here and nowhere else.  The generic infrastructure in
* test_type.hpp and test_object.hpp is intentionally kind-agnostic.
*
*   OPTION ENUMERATION:
*   DTestOption enumerates the configurable parameters available
* in a dtest_option_set.  Currently defines test_metadata; additional
* option keys will be added as the framework grows.
*
*   TEST METADATA:
*   Every built-in kind carries a default test_metadata option whose
* value is a sorted std::vector<std::string>.  The metadata vector
* stores tags, labels, or descriptive strings associated with that
* kind.  Default metadata content is empty - the user populates it
* at registration or runtime.  The sorted invariant is maintained
* by insertion helpers.
*
*   BUILT-IN KINDS (ranked lowest to highest):
*     0  assert       - single boolean assertion         (leaf)
*     1  test_fn      - test function pointer wrapper    (leaf)
*     2  test         - individual test case             (interior)
*     3  test_block   - group of tests                   (interior)
*     4  module       - group of blocks                  (interior)
*
*   OPTION LIFETIME:
*   make_default_test_type() creates kinds with nullptr default
* options.  To wire up per-kind defaults, the caller creates
* dtest_option_set instances via make_kind_options(), owns their
* storage, and passes pointers into the test_kind entries.  This
* keeps lifetime management explicit and avoids hidden statics.
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h for version detection and djinterp.hpp
* for namespace macros and constexpr support.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST METADATA TYPE
* II.   TEST METADATA HELPERS
* III.  BUILT-IN KIND CONSTANTS
* IV.   DEFAULT TYPE FACTORY
* V.    DEFAULT OPTION FACTORIES
* VI.   CONVENIENCE OBJECT FACTORIES
* VII.  TEST-RECORDING HELPERS
* VIII. NUMBERED-LEAF NODE TEMPLATE
* IX.   VALUE-TAGGED EVENT TAGS
* X.    DEFAULT TEST HANDLER (THRESHOLD-FILTERED)
*
*
* path:      /inc/djinterp/test/test_defaults.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.26
******************************************************************************/

#ifndef DJINTERP_TEST_DEFAULTS_
#define DJINTERP_TEST_DEFAULTS_ 1

// std
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../restd/any/any.hpp"
#include "../core/options/option_pair.hpp"
#include "../core/options/option_set.hpp"
#include "./test_common.hpp"
#include "./test_event.hpp"
#include "./test_handler.hpp"
#include "./test_options.hpp"
#include "./test_printer.hpp"
#include "./test_type.hpp"
#include "./test_object.hpp"


NS_DJINTERP
NS_TEST

using restd::any;

///////////////////////////////////////////////////////////////////////////////
///                I.   TEST METADATA TYPE                                   ///
///////////////////////////////////////////////////////////////////////////////

// test_metadata_type
//   type: the concrete value type for the test_metadata
// option key.  A sorted vector of strings - the sorted
// invariant is maintained by the insertion helpers below.
using test_metadata_type = std::vector<std::string>;


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST METADATA HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

// metadata_insert
//   function: inserts a tag into a sorted metadata vector
// at its sorted position.  Duplicates are silently ignored.
inline void
metadata_insert(
    test_metadata_type& _meta,
    const std::string&  _tag
)
{
    auto pos = std::lower_bound(_meta.begin(),
                                _meta.end(),
                                _tag);

    // skip duplicate
    if ( (pos != _meta.end()) &&
         (*pos == _tag) )
    {
        return;
    }

    _meta.insert(pos, _tag);

    return;
}

// metadata_contains
//   function: returns true if _tag is present in the sorted
// metadata vector.  Uses binary search.
inline bool
metadata_contains(
    const test_metadata_type& _meta,
    const std::string&        _tag
)
{
    return std::binary_search(_meta.begin(),
                              _meta.end(),
                              _tag);
}

// metadata_remove
//   function: removes _tag from the sorted metadata vector.
// Returns true if the tag was found and removed.
inline bool
metadata_remove(
    test_metadata_type& _meta,
    const std::string&  _tag
)
{
    auto pos = std::lower_bound(_meta.begin(),
                                _meta.end(),
                                _tag);

    if ( (pos == _meta.end()) ||
         (*pos != _tag) )
    {
        return false;
    }

    _meta.erase(pos);

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                III. BUILT-IN KIND CONSTANTS                              ///
///////////////////////////////////////////////////////////////////////////////

// D_TEST_KIND_ASSERT
//   constant: test_type_id for assertion-level test objects.
// Rank 0, leaf.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_ASSERT     = 0;

// D_TEST_KIND_TEST_FN
//   constant: test_type_id for test function pointer wrappers.
// Rank 1, leaf.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST_FN    = 1;

// D_TEST_KIND_TEST
//   constant: test_type_id for individual test cases.
// Rank 2, interior.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST       = 2;

// D_TEST_KIND_TEST_BLOCK
//   constant: test_type_id for test blocks (groups of tests).
// Rank 3, interior.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST_BLOCK = 3;

// D_TEST_KIND_MODULE
//   constant: test_type_id for test modules (top-level grouping).
// Rank 4, interior.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_MODULE     = 4;

// D_TEST_KIND_COUNT
//   constant: number of built-in kind constants.
D_STATIC_CONSTEXPR std::size_t  D_TEST_KIND_COUNT      = 5;


///////////////////////////////////////////////////////////////////////////////
///                IV.  DEFAULT TYPE FACTORY                                 ///
///////////////////////////////////////////////////////////////////////////////

// make_default_test_type
//   function: constructs a test_type pre-populated with the
// five built-in kinds using their default ranks and
// leaf/interior flags.  All default_options pointers are
// nullptr - the caller is responsible for creating option
// sets and wiring them in.
template<typename _Container = std::vector<test_kind>>
inline test_type<_Container>
make_default_test_type()
{
    test_type<_Container> tt;

    tt.register_kind({ D_TEST_KIND_ASSERT,
                       "assert",
                       0,
                       true,
                       nullptr });

    tt.register_kind({ D_TEST_KIND_TEST_FN,
                       "test_fn",
                       1,
                       true,
                       nullptr });

    tt.register_kind({ D_TEST_KIND_TEST,
                       "test",
                       2,
                       false,
                       nullptr });

    tt.register_kind({ D_TEST_KIND_TEST_BLOCK,
                       "test_block",
                       3,
                       false,
                       nullptr });

    tt.register_kind({ D_TEST_KIND_MODULE,
                       "module",
                       4,
                       false,
                       nullptr });

    return tt;
}


///////////////////////////////////////////////////////////////////////////////
///                V.   DEFAULT OPTION FACTORIES                             ///
///////////////////////////////////////////////////////////////////////////////

// make_kind_options
//   function: constructs a dtest_option_set containing a
// single test_metadata entry initialized to an empty sorted
// vector.  The caller may populate the vector via
// metadata_insert() after construction.
inline dtest_option_set
make_kind_options()
{
    dtest_option_set opts;

    opts.insert(DTestOption::metadata, any(test_metadata_type{}));

    return opts;
}

// make_kind_options (with initial tags)
//   function: constructs a dtest_option_set whose
// test_metadata vector is pre-populated with the given
// tags.  Tags are sorted on construction.
inline dtest_option_set
make_kind_options(
    test_metadata_type _tags
)
{
    // ensure sorted invariant
    std::sort(_tags.begin(), _tags.end());

    // remove duplicates
    _tags.erase(std::unique(_tags.begin(),
                            _tags.end()),
                _tags.end());

    dtest_option_set opts;

    opts.insert(DTestOption::metadata,
                any(static_cast<test_metadata_type&&>(_tags)));

    return opts;
}


///////////////////////////////////////////////////////////////////////////////
///                VI.  CONVENIENCE OBJECT FACTORIES                         ///
///////////////////////////////////////////////////////////////////////////////

// make_assert
//   function: creates an assertion-level leaf test.
D_CONSTEXPR_INLINE basic_test
make_assert(
    bool        _result,
    const char* _name         = nullptr,
    const char* _message_pass = nullptr,
    const char* _message_fail = nullptr
) D_NOEXCEPT
{
    return basic_test(
        D_TEST_KIND_ASSERT,
        _result,
        _name,
        _message_pass,
        _message_fail);
}

// make_test_fn
//   function: creates a test_fn-level leaf test.
D_CONSTEXPR_INLINE basic_test
make_test_fn(
    bool        _result,
    const char* _name = nullptr
) D_NOEXCEPT
{
    return basic_test(D_TEST_KIND_TEST_FN,
                      _result,
                      _name);
}

// make_test_case
//   function: creates a test-level interior node.
D_CONSTEXPR_INLINE basic_test
make_test_case(
    const char* _name
) D_NOEXCEPT
{
    return make_interior(D_TEST_KIND_TEST, _name);
}

// make_test_block
//   function: creates a test_block-level interior node.
D_CONSTEXPR_INLINE basic_test
make_test_block(
    const char* _name
) D_NOEXCEPT
{
    return make_interior(D_TEST_KIND_TEST_BLOCK, _name);
}

// make_module
//   function: creates a module-level interior node.
D_CONSTEXPR_INLINE basic_test
make_module(
    const char* _name
) D_NOEXCEPT
{
    return make_interior(D_TEST_KIND_MODULE, _name);
}

// ---- the template string ----
D_STATIC const char* const D_TEST_TPL_MASTER_SUITE =
    "============================================"
    "====================================\n"
    "  TESTING:     %module_name%\n"
    "============================================"
    "====================================\n"
    "  description: %module_description%\n"
    "  path:        %module_path%\n"
    "  date/time:   %timestamp_start%\n"
    "============================================"
    "====================================\n"

    "\n"

    "--------------------------------------------"
    "------------------------------------\n"
    "  MODULE: %description_short%\n"
    "  %description_long%\n"
    "--------------------------------------------"
    "------------------------------------\n"

    "\n"

    "%test_modules%"

    "\n"

    "--------------------------------------------"
    "------------------------------------\n"
    "  COMPREHENSIVE TEST RESULTS\n"
    "--------------------------------------------"
    "------------------------------------\n"

    "  MODULE SUMMARY:\n"
    "    Modules Tested:       %modules_tested%\n"
    "    Modules Passed:       %modules_passed%\n"
    "    Module Success Rate:  %modules_percent%\n"

    "\n"

    "  ASSERTION SUMMARY:\n"
    "    Total Assertions:     %asserts_total%\n"
    "    Assertions Passed:    %asserts_passed%\n"
    "    Assertions Failed:    %asserts_failed%\n"
    "    Assertion Pass Rate:  %asserts_percent%\n"

    "\n"

    "  UNIT TEST SUMMARY:\n"
    "    Total Unit Tests:     %tests_total%\n"
    "    Unit Tests Passed:    %tests_passed%\n"
    "    Unit Tests Failed:    %tests_failed%\n"
    "    Unit Test Pass Rate:  %tests_percent%\n"

    "\n"

    "  EXECUTION TIME:\n"
    "    Total Time:           %time_total%\n"

    "--------------------------------------------"
    "------------------------------------\n"

    "\n"

    "============================================"
    "====================================\n"
    "  MODULE RESULTS: %module_name%\n"
    "============================================"
    "====================================\n"
    "  Assertions: %asserts_passed% / %asserts_total%"
    " (%asserts_percent%)\n"
    "  Unit Tests: %tests_passed% / %tests_total%"
    " (%tests_percent%)\n"
    "  Status:     %has_passed%\n"
    "============================================"
    "====================================\n";


///////////////////////////////////////////////////////////////////////////////
///                VII. TEST-RECORDING HELPERS                              ///
///////////////////////////////////////////////////////////////////////////////
//   These bridges run a leaf assertion through the handler so the
// session counters stay authoritative - no shadow accounting on
// the call site.  They replace ad-hoc per-translation-unit macros
// and give every test file a uniform, debuggable entry point.

// status_for
//   helper: maps a boolean result to test_status::passed or
// test_status::failed.  Other statuses are reachable via the
// explicit record_status() entry point.
D_CONSTEXPR_INLINE test_status
status_for(
    bool _ok
) D_NOEXCEPT
{
    return _ok ? test_status::passed
               : test_status::failed;
}

// record_assertion
//   helper: appends an assertion-level basic_test to the sink
// AND advances the handler's counters via test_handler::record.
// One source of truth for the "did this assertion pass?"
// signal - both the printed leaf and the counter tally come
// from the same expression.
template<typename _Handler,
         typename _Sink>
inline void
record_assertion(
    _Handler&   _handler,
    _Sink&      _sink,
    bool        _ok,
    const char* _name,
    const char* _msg_pass = nullptr,
    const char* _msg_fail = nullptr)
{
    _sink.push_back(make_assert(_ok, _name, _msg_pass, _msg_fail));
    _handler.record(status_for(_ok));
    return;
}

// record_status
//   helper: variant for non-boolean outcomes (skip / error /
// pending).  Appends a basic_test stamped with the requested
// status and forwards to the handler's counter.
template<typename _Handler,
         typename _Sink>
inline void
record_status(
    _Handler&   _handler,
    _Sink&      _sink,
    test_status _status,
    const char* _name)
{
    basic_test t = make_assert(false, _name);
    t.set_status(static_cast<basic_test::status_type>(_status));
    _sink.push_back(t);
    _handler.record(_status);
    return;
}


// unit_test_tally
//   struct: per-suite summary of "unit tests" - each unit test
// is one logical grouping of assertions, typically the body of
// one test_array_* function.  Distinct from the handler's
// session_result, which counts at assertion granularity.
struct unit_test_tally
{
    std::size_t total;
    std::size_t passed;
    std::size_t failed;

    unit_test_tally() D_NOEXCEPT
        : total(0),
          passed(0),
          failed(0)
    {}

    // pass_rate
    //   returns the pass percentage on [0.0, 100.0].
    double pass_rate() const D_NOEXCEPT
    {
        return (total == 0)
                   ? 0.0
                   : (100.0 *
                        static_cast<double>(passed) /
                        static_cast<double>(total));
    }
};


// run_unit_test
//   helper: runs a unit-test functor, observing the handler's
// fail/error counters across the call to decide whether the
// unit test as a whole passed (delta zero) or failed (any new
// failures or errors).  Records the unit test as a test_fn-kind
// leaf in the sink for printer rendering and updates the tally.
//
//   The functor runs ALL its `record_assertion` calls into the
// same sink - this keeps individual assertion-level rows in
// the output, while the wrapper adds a single roll-up entry per
// unit test.  Together they mirror the C-side framework's
// "Total Unit Tests / Total Assertions" split.
template<typename _Handler,
         typename _Sink,
         typename _Fn>
inline void
run_unit_test(
    _Handler&        _handler,
    _Sink&           _sink,
    unit_test_tally& _tally,
    const char*      _name,
    _Fn&&            _body)
{
    const std::size_t fails_before  = _handler.failed();
    const std::size_t errors_before = _handler.errors();

    _body();

    const bool ok =
          (_handler.failed() == fails_before)
       && (_handler.errors() == errors_before);

    // Append a test_fn-kind leaf so the printer (and any future
    // tree-shaped reporter) can show the unit-test row inline
    // with its assertion children.
    _sink.push_back(make_test_fn(ok, _name));

    ++_tally.total;
    if (ok)
    {
        ++_tally.passed;
    }
    else
    {
        ++_tally.failed;
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                VIII. NUMBERED-LEAF NODE TEMPLATE                        ///
///////////////////////////////////////////////////////////////////////////////
//   Used by suite printers that want left-aligned numbering on
// every leaf node, matching the "[1/97] name ... [PASS]"
// cadence of the C-side reference output.  The %number%
// specifier comes from the printer's print_context (leaf_number,
// pre-incremented at emission).

// D_TEST_TPL_NODE_NUMBERED
//   template: leaf-only node line with left-aligned counter,
// space-separated name, and right-aligned status tag.
D_STATIC const char* const D_TEST_TPL_NODE_NUMBERED =
    "  [%number%/%total%] %name% %status%\n";

// D_TEST_TPL_SECTION_HEADER_NUMBERED
//   template: numbered section header for grouping rows in the
// inner walk.  The printer fills %section_number% and %name%.
D_STATIC const char* const D_TEST_TPL_SECTION_HEADER_NUMBERED =
    "\n"
    "  [%section_number%] %name%\n"
    "  ----------------------------------------------"
    "----------------------------\n";

// D_TEST_TPL_SECTION_FOOTER
//   template: per-section footer with a passed/total tally
// and a status verdict.
D_STATIC const char* const D_TEST_TPL_SECTION_FOOTER =
    "  --- %name% Summary ---\n"
    "    Assertions: %asserts_passed%/%asserts_total% "
    "(%asserts_percent%)\n"
    "    Status:     %has_passed%\n";


///////////////////////////////////////////////////////////////////////////////
///                IX.  VALUE-TAGGED EVENT TAGS                              ///
///////////////////////////////////////////////////////////////////////////////
//
//   Value-tagged events carry a `const events::test_event<intN_t>&`
// payload - see test_event.hpp for the templated struct.  Each
// integer width gets its own D_EVENT-declared tag here so that
// users (and default_test_handler below) can bind listeners
// distinctly per width.
//
//   The naming pattern is `on_test_event_N` where N is the bit
// width (8, 16, 32, 64).  This mirrors the integer's numeric
// suffix and keeps fire/bind sites self-documenting.

namespace events {

    // on_test_event_8
    //   event: value-tagged event with an int8_t payload field.
    D_EVENT(on_test_event_8,
            const test_event<std::int8_t>&);

    // on_test_event_16
    //   event: value-tagged event with an int16_t payload field.
    D_EVENT(on_test_event_16,
            const test_event<std::int16_t>&);

    // on_test_event_32
    //   event: value-tagged event with an int32_t payload field.
    D_EVENT(on_test_event_32,
            const test_event<std::int32_t>&);

    // on_test_event_64
    //   event: value-tagged event with an int64_t payload field.
    D_EVENT(on_test_event_64,
            const test_event<std::int64_t>&);

}  // namespace events


///////////////////////////////////////////////////////////////////////////////
///                X.   DEFAULT TEST HANDLER (THRESHOLD-FILTERED)            ///
///////////////////////////////////////////////////////////////////////////////
//
//   default_test_handler extends test_handler with a printer
// listener bundle that:
//
//     - Lifecycle events (on_test_passed, on_test_failed, etc.)
//       ALWAYS forward to the printer; there is no value gate.
//
//     - Value-tagged events (on_test_event_N) are forwarded to
//       the printer only when the carried value is GREATER THAN
//       OR EQUAL TO the configured threshold.  Below-threshold
//       events still dispatch to other listeners - only the
//       printer's listener is gated.
//
//   The threshold is a single int64_t value used to compare
// against payloads of any narrower width.  Each value-tagged
// listener widens its int8_t / int16_t / int32_t / int64_t
// payload to int64_t before the comparison; the widening is
// always lossless because every signed narrow type's range is
// a subset of int64_t's range.
//
//   THRESHOLD DEFAULT:
//   The default threshold is INT64_MIN - i.e. every value-tagged
// event is printed.  Callers who want to silence below-warning
// events use set_threshold() with a higher value.

// default_test_handler
//   class: standard test_handler with the framework's default
// printer-listener bundle.  When a printer is attached via
// set_printer(), this class installs:
//
//     1. Lifecycle listeners - one per built-in lifecycle event
//        - that always render through the printer.
//     2. Value-tagged listeners - one per value-tagged event tag
//        in section IX - that gate the printer call on
//        `event.value >= threshold()`.
class default_test_handler : public test_handler
{
public:
    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------

    // threshold_type
    //   the integer type used for threshold comparisons.
    // Wide enough to losslessly accept any value-tagged
    // payload's value.
    using threshold_type = std::int64_t;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    //   constructs a default_test_handler with threshold set to
    // the minimum representable threshold_type value, so every
    // value-tagged event passes the gate and reaches the
    // printer.
    default_test_handler() D_NOEXCEPT
        : test_handler(),
          m_threshold(threshold_min())
    {}

    // from threshold
    //   constructs a default_test_handler with the supplied
    // threshold.  Any value-tagged event with `value < _t`
    // will be dropped from the printer's view.
    explicit default_test_handler(
        threshold_type _t
    ) D_NOEXCEPT
        : test_handler(),
          m_threshold(_t)
    {}

    // -----------------------------------------------------------------
    //  threshold accessors
    // -----------------------------------------------------------------

    // set_threshold
    //   updates the threshold.  Takes effect immediately for
    // subsequent dispatches; in-flight listener bodies that
    // already evaluated the threshold will complete with the
    // pre-update value.  Does not require re-installing the
    // listener bundle.
    void set_threshold(
        threshold_type _t
    ) D_NOEXCEPT
    {
        m_threshold = _t;

        return;
    }

    // threshold
    //   returns the current threshold value.
    D_CONSTEXPR threshold_type
    threshold() const D_NOEXCEPT
    {
        return m_threshold;
    }

    // threshold_min
    //   returns the minimum representable threshold_type value
    // - i.e. the threshold that admits every payload.  Provided
    // as a static helper so callers can write
    // `handler.set_threshold(default_test_handler::threshold_min())`
    // instead of pulling in <limits> at the call site.
    static D_CONSTEXPR threshold_type
    threshold_min() D_NOEXCEPT
    {
        // INT64_MIN is the most-negative representable int64_t.
        // We avoid <limits> here so this header stays light;
        // <cstdint> is already included for std::int64_t.
        return static_cast<threshold_type>(INT64_MIN);
    }

protected:
    // install_printer_listeners
    //   override: installs the framework's default printer
    // bundle on top of the base class's empty default.  The
    // base class's m_printer pointer is already set when this
    // is called (set_printer stored it before calling here).
    //
    //   Every binding's listener_id is appended to
    // m_printer_listener_ids so that the inherited
    // uninstall_printer_listeners() can tear the bundle down
    // on clear_printer() or destruction.
    virtual void install_printer_listeners()
    {
        // capture-by-value of the printer pointer keeps the
        // listener bodies independent of any later mutation
        // of m_printer.  The handler's destructor unbinds the
        // bundle before the pointer is invalidated; we never
        // outlive the printer the user attached.
        test_printer* const printer = m_printer;

        if (printer == nullptr)
        {
            return;
        }

        bind_lifecycle_listeners(printer);
        bind_value_tagged_listeners(printer);

        return;
    }

private:
    // bind_lifecycle_listeners
    //   helper: binds one listener per built-in lifecycle
    // event whose body forwards to the printer's per-node
    // rendering.  All lifecycle listeners are unconditional
    // - there is no value gate here.
    void bind_lifecycle_listeners(
        test_printer* _printer
    )
    {
        // on_test_passed: render the leaf with passed status.
        m_printer_listener_ids.push_back(
            events().bind<events::on_test_passed>(
                [_printer](event_context& /*_ctx*/,
                           const basic_test* _obj) D_NOEXCEPT
                {
                    if ( (_printer == nullptr) ||
                         (_obj     == nullptr) )
                    {
                        return;
                    }

                    _printer->print_node(
                        test_status::passed,
                        std::string(_obj->name()    ? _obj->name()    : ""),
                        std::string(_obj->message() ? _obj->message() : ""),
                        _obj->depth(),
                        static_cast<std::size_t>(0));

                    return;
                }));

        // on_test_failed: render the leaf with failed status.
        m_printer_listener_ids.push_back(
            events().bind<events::on_test_failed>(
                [_printer](event_context& /*_ctx*/,
                           const basic_test* _obj) D_NOEXCEPT
                {
                    if ( (_printer == nullptr) ||
                         (_obj     == nullptr) )
                    {
                        return;
                    }

                    _printer->print_node(
                        test_status::failed,
                        std::string(_obj->name()    ? _obj->name()    : ""),
                        std::string(_obj->message() ? _obj->message() : ""),
                        _obj->depth(),
                        static_cast<std::size_t>(0));

                    return;
                }));

        // on_test_skipped: render the leaf with skipped status.
        m_printer_listener_ids.push_back(
            events().bind<events::on_test_skipped>(
                [_printer](event_context& /*_ctx*/,
                           const basic_test* _obj) D_NOEXCEPT
                {
                    if ( (_printer == nullptr) ||
                         (_obj     == nullptr) )
                    {
                        return;
                    }

                    _printer->print_node(
                        test_status::skipped,
                        std::string(_obj->name()    ? _obj->name()    : ""),
                        std::string(_obj->message() ? _obj->message() : ""),
                        _obj->depth(),
                        static_cast<std::size_t>(0));

                    return;
                }));

        // on_test_error: render the leaf with error status and
        // the diagnostic message carried by the event.
        m_printer_listener_ids.push_back(
            events().bind<events::on_test_error>(
                [_printer](event_context& /*_ctx*/,
                           const basic_test* _obj,
                           const char*       _msg) D_NOEXCEPT
                {
                    if ( (_printer == nullptr) ||
                         (_obj     == nullptr) )
                    {
                        return;
                    }

                    _printer->print_node(
                        test_status::error,
                        std::string(_obj->name() ? _obj->name() : ""),
                        std::string(_msg         ? _msg         : ""),
                        _obj->depth(),
                        static_cast<std::size_t>(0));

                    return;
                }));

        return;
    }

    // bind_value_tagged_listeners
    //   helper: binds one listener per value-tagged event tag
    // (one per integer width) whose body widens the payload
    // value to threshold_type and forwards to the printer
    // ONLY IF the value is at or above the configured
    // threshold.  Listeners capture `this` so the threshold
    // is read at dispatch time, not at bind time - set_threshold
    // calls take effect immediately without re-binding.
    void bind_value_tagged_listeners(
        test_printer* _printer
    )
    {
        // 8-bit
        m_printer_listener_ids.push_back(
            events().bind<events::on_test_event_8>(
                [_printer, this](event_context& /*_ctx*/,
                                 const events::test_event<std::int8_t>& _evt) D_NOEXCEPT
                {
                    print_value_event_if_above_threshold(
                        _printer,
                        static_cast<threshold_type>(_evt.value),
                        _evt.name,
                        _evt.message);

                    return;
                }));

        // 16-bit
        m_printer_listener_ids.push_back(
            events().bind<events::on_test_event_16>(
                [_printer, this](event_context& /*_ctx*/,
                                 const events::test_event<std::int16_t>& _evt) D_NOEXCEPT
                {
                    print_value_event_if_above_threshold(
                        _printer,
                        static_cast<threshold_type>(_evt.value),
                        _evt.name,
                        _evt.message);

                    return;
                }));

        // 32-bit
        m_printer_listener_ids.push_back(
            events().bind<events::on_test_event_32>(
                [_printer, this](event_context& /*_ctx*/,
                                 const events::test_event<std::int32_t>& _evt) D_NOEXCEPT
                {
                    print_value_event_if_above_threshold(
                        _printer,
                        static_cast<threshold_type>(_evt.value),
                        _evt.name,
                        _evt.message);

                    return;
                }));

        // 64-bit
        m_printer_listener_ids.push_back(
            events().bind<events::on_test_event_64>(
                [_printer, this](event_context& /*_ctx*/,
                                 const events::test_event<std::int64_t>& _evt) D_NOEXCEPT
                {
                    print_value_event_if_above_threshold(
                        _printer,
                        _evt.value,
                        _evt.name,
                        _evt.message);

                    return;
                }));

        return;
    }

    // print_value_event_if_above_threshold
    //   helper: shared body for every value-tagged listener.
    // Compares the widened value against the current threshold
    // and forwards to the printer if the gate passes.  The
    // printed `number` field is set to the value cast to
    // size_t so the printer can render it as a decimal token.
    void print_value_event_if_above_threshold(
        test_printer*  _printer,
        threshold_type _value,
        const char*    _name,
        const char*    _message
    ) D_NOEXCEPT
    {
        if (_printer == nullptr)
        {
            return;
        }

        if (_value < m_threshold)
        {
            return;
        }

        _printer->print_node(
            test_status::passed,
            std::string(_name    ? _name    : ""),
            std::string(_message ? _message : ""),
            static_cast<std::size_t>(0),
            static_cast<std::size_t>(_value));

        return;
    }


    threshold_type m_threshold;
};


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_DEFAULTS_