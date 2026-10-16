/*******************************************************************************
* djinterp [test]                                 test_handler_tests_session.cpp
*
*   Section IV (session / printer / sink).  start_session zeroes the counters
* and fires on_session_start; end_session fires on_session_end carrying the
* current pass/fail counts (and leaves the counters intact).  The printer is a
* non-owning attachment: set_printer stores the pointer and calls the (virtual)
* install hook; clear_printer calls uninstall and forgets the pointer; both are
* idempotent, re-attaching swaps the bundle, and the destructor clears an
* attached printer.  The base install hook is a no-op, so a small subclass
* (probe_handler) overrides it to bind a probe-event listener and record its id
* on m_printer_listener_ids - letting has_listeners_for witness the bundle being
* installed on set_printer and torn down on clear_printer.  The sink is a plain
* vector: push_assertion appends, both sink() overloads enumerate, and
* clear_sink empties it without touching the counters.
*
*
* path:      /tests/djinterp/test/test_handler_tests_session.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_handler_tests.hpp"

NS_DJINTERP
NS_TESTING

// probe_evt
//   a custom empty event used only to witness the printer listener bundle
// (kept off the built-in lifecycle events).
D_EVENT_EMPTY(probe_evt);

// probe_handler
//   a test_handler subclass whose install hook binds one probe_evt listener and
// records its id on the (protected) id vector, so the base uninstall unbinds it.
// This stands in for a real default_test_handler's printer bundle without
// pulling in test_printer.hpp.
struct probe_handler : public dt::test_handler
{
    void install_printer_listeners() override
    {
        handler_id _id = this->on<probe_evt>([]() { return verdict::pass; });
        this->m_printer_listener_ids.push_back(_id);
    }
};


// tests_session_start_resets_and_fires
//   start_session zeroes the counters and fires on_session_start.
bool
tests_session_start_resets_and_fires()
{
    bool ok = true;

    lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
    h.record(dt::test_status::failed);
    h.record(dt::test_status::passed);

    h.start_session();

    ok = D_TH_CHECK(h.total() == 0 && h.failed() == 0 && h.passed() == 0) && ok;   // reset
    ok = D_TH_CHECK(rec.session_start == 1) && ok;                                 // fired

    return ok;
}

// tests_session_end_fires_with_counts
//   end_session fires on_session_end with the current pass/fail counts and does
// not clear the counters.
bool
tests_session_end_fires_with_counts()
{
    bool ok = true;

    lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
    h.record(dt::test_status::passed);
    h.record(dt::test_status::passed);
    h.record(dt::test_status::failed);

    h.end_session();

    ok = D_TH_CHECK(rec.session_end == 1) && ok;
    ok = D_TH_CHECK(rec.end_passed == 2 && rec.end_failed == 1) && ok;   // payload = (passed, failed)
    ok = D_TH_CHECK(h.passed() == 2 && h.failed() == 1) && ok;           // counters left intact

    return ok;
}

// tests_printer_set_get_clear
//   set_printer stores the pointer; clear_printer (and set_printer(nullptr))
// forgets it.
bool
tests_printer_set_get_clear()
{
    bool ok = true;

    dt::test_handler h;
    char sentinel;
    dt::test_printer* p = reinterpret_cast<dt::test_printer*>(&sentinel);

    ok = D_TH_CHECK(h.printer() == nullptr) && ok;   // none initially
    h.set_printer(p);
    ok = D_TH_CHECK(h.printer() == p) && ok;
    h.clear_printer();
    ok = D_TH_CHECK(h.printer() == nullptr) && ok;

    h.set_printer(p);
    h.set_printer(nullptr);                          // equivalent to clear
    ok = D_TH_CHECK(h.printer() == nullptr) && ok;

    return ok;
}

// tests_printer_replace
//   re-attaching a printer swaps the pointer (the old bundle is cleared first).
bool
tests_printer_replace()
{
    bool ok = true;

    dt::test_handler h;
    char s1, s2;
    dt::test_printer* p1 = reinterpret_cast<dt::test_printer*>(&s1);
    dt::test_printer* p2 = reinterpret_cast<dt::test_printer*>(&s2);

    h.set_printer(p1);
    ok = D_TH_CHECK(h.printer() == p1) && ok;
    h.set_printer(p2);
    ok = D_TH_CHECK(h.printer() == p2) && ok;

    return ok;
}

// tests_printer_clear_idempotent
//   clear_printer with no printer attached is a safe no-op.
bool
tests_printer_clear_idempotent()
{
    bool ok = true;

    dt::test_handler h;
    h.clear_printer();
    ok = D_TH_CHECK(h.printer() == nullptr) && ok;
    h.clear_printer();
    ok = D_TH_CHECK(h.printer() == nullptr) && ok;

    return ok;
}

// tests_printer_install_uninstall_hooks
//   set_printer runs the install hook (the bundle appears); clear_printer runs
// the base uninstall (the bundle is torn down).
bool
tests_printer_install_uninstall_hooks()
{
    bool ok = true;

    probe_handler h;
    char sentinel;
    dt::test_printer* p = reinterpret_cast<dt::test_printer*>(&sentinel);

    ok = D_TH_CHECK(!h.has_listeners_for<probe_evt>()) && ok;   // no bundle yet
    h.set_printer(p);
    ok = D_TH_CHECK(h.has_listeners_for<probe_evt>()) && ok;    // install bound it
    ok = D_TH_CHECK(h.printer() == p) && ok;
    h.clear_printer();
    ok = D_TH_CHECK(!h.has_listeners_for<probe_evt>()) && ok;   // uninstall unbound it
    ok = D_TH_CHECK(h.printer() == nullptr) && ok;

    return ok;
}

// tests_printer_dtor_unbinds
//   destruction runs clear_printer for an attached printer (exercises the dtor
// teardown path); a fresh handler is independent and clean.
bool
tests_printer_dtor_unbinds()
{
    bool ok = true;

    bool bound_before = false;
    {
        probe_handler h;
        char sentinel;
        dt::test_printer* p = reinterpret_cast<dt::test_printer*>(&sentinel);
        h.set_printer(p);
        bound_before = h.has_listeners_for<probe_evt>();
    }   // ~test_handler -> clear_printer -> uninstall_printer_listeners

    ok = D_TH_CHECK(bound_before) && ok;   // the bundle was present before teardown

    probe_handler fresh;
    ok = D_TH_CHECK(!fresh.has_listeners_for<probe_evt>()) && ok;
    ok = D_TH_CHECK(fresh.printer() == nullptr) && ok;

    return ok;
}

// tests_sink_push_and_enumerate
//   push_assertion appends nodes; both sink() overloads enumerate them; the
// mutable overload permits direct manipulation.
bool
tests_sink_push_and_enumerate()
{
    bool ok = true;

    dt::test_handler h;
    ok = D_TH_CHECK(h.sink().empty()) && ok;

    dt::basic_test n1(dt::test_type_id{0}, true);    // passed
    dt::basic_test n2(dt::test_type_id{0}, false);   // failed
    h.push_assertion(n1);
    h.push_assertion(n2);
    ok = D_TH_CHECK(h.sink().size() == 2) && ok;

    const dt::test_handler& ch = h;
    ok = D_TH_CHECK(ch.sink().size() == 2) && ok;    // const overload

    ok = D_TH_CHECK(h.sink()[0].status() == dt::basic_test::status_passed) && ok;
    ok = D_TH_CHECK(h.sink()[1].status() == dt::basic_test::status_failed) && ok;

    h.sink().push_back(n1);                          // mutable overload
    ok = D_TH_CHECK(h.sink().size() == 3) && ok;

    return ok;
}

// tests_sink_clear
//   clear_sink empties the sink and leaves the counters untouched.
bool
tests_sink_clear()
{
    bool ok = true;

    dt::test_handler h;
    dt::basic_test n(dt::test_type_id{0}, true);
    h.push_assertion(n);
    h.push_assertion(n);
    h.record(dt::test_status::passed);   // counter state

    h.clear_sink();
    ok = D_TH_CHECK(h.sink().empty()) && ok;
    ok = D_TH_CHECK(h.passed() == 1) && ok;   // counters unaffected

    return ok;
}


NS_END  // testing
NS_END  // djinterp
