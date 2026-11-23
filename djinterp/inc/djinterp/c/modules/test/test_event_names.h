/*******************************************************************************
* djinterp [c][test]                                          test_event_names.h
*
*   The names of DTest's twelve built-in events, in the one table both faces
* read.  C's alphabet (test_event.c) returns them from d_test_event_kind_name,
* and C++'s tags (test_event.hpp) give them to D_EVENT, which hashes each into
* the event's key.  So an event prints, binds and hashes the same in C and in
* C++, and the parity test (test_event_tests_parity.cpp) holds it so.
*
*   A name is dotted lower-case words, in the form of a message code
* (module.name): every built-in is test.<summand>.  Once a name is hashed into
* a key it is a wire format, so it changes only with the alphabet.
*
* HOW TO USE
* ----------
*   D_TEST_EVENT_NAME_TEST_FAILED is the string literal "test.test_failed", and
* likewise for each summand.  D_TEST_EVENT_NAMES(X) expands X(KIND, NAME) once
* per built-in, in kind order, where KIND is the summand's suffix in
* enum dj_test_event_kind (D_TEST_EVENT_##KIND):
*
*     #define ROW(_kind, _name) _name,
*     static const char* const names[] = { D_TEST_EVENT_NAMES(ROW) };
*     #undef ROW
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None.  The header holds only string literals and one list macro, so it
* compiles at every C and C++ level.
*
*
* path:      /inc/djinterp/c/modules/test/test_event_names.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

#ifndef DJINTERP_C_TEST_EVENT_NAMES_H
#define DJINTERP_C_TEST_EVENT_NAMES_H 1


// D_TEST_EVENT_NAME_*
//   constant: each built-in event's name, a string literal.
#define D_TEST_EVENT_NAME_SESSION_START     "test.session_start"
#define D_TEST_EVENT_NAME_SESSION_END       "test.session_end"
#define D_TEST_EVENT_NAME_MODULE_START      "test.module_start"
#define D_TEST_EVENT_NAME_MODULE_END        "test.module_end"
#define D_TEST_EVENT_NAME_TEST_START        "test.test_start"
#define D_TEST_EVENT_NAME_TEST_END          "test.test_end"
#define D_TEST_EVENT_NAME_TEST_PASSED       "test.test_passed"
#define D_TEST_EVENT_NAME_TEST_FAILED       "test.test_failed"
#define D_TEST_EVENT_NAME_TEST_SKIPPED      "test.test_skipped"
#define D_TEST_EVENT_NAME_TEST_ERROR        "test.test_error"
#define D_TEST_EVENT_NAME_STATUS_CHANGE     "test.status_change"
#define D_TEST_EVENT_NAME_LISTENER_THREW    "test.listener_threw"

// D_TEST_EVENT_NAMES
//   macro: expands _x(KIND, NAME) for each built-in event, in kind order.
#define D_TEST_EVENT_NAMES(_x)                                                \
    _x(SESSION_START,  D_TEST_EVENT_NAME_SESSION_START)                       \
    _x(SESSION_END,    D_TEST_EVENT_NAME_SESSION_END)                         \
    _x(MODULE_START,   D_TEST_EVENT_NAME_MODULE_START)                        \
    _x(MODULE_END,     D_TEST_EVENT_NAME_MODULE_END)                          \
    _x(TEST_START,     D_TEST_EVENT_NAME_TEST_START)                          \
    _x(TEST_END,       D_TEST_EVENT_NAME_TEST_END)                            \
    _x(TEST_PASSED,    D_TEST_EVENT_NAME_TEST_PASSED)                         \
    _x(TEST_FAILED,    D_TEST_EVENT_NAME_TEST_FAILED)                         \
    _x(TEST_SKIPPED,   D_TEST_EVENT_NAME_TEST_SKIPPED)                        \
    _x(TEST_ERROR,     D_TEST_EVENT_NAME_TEST_ERROR)                          \
    _x(STATUS_CHANGE,  D_TEST_EVENT_NAME_STATUS_CHANGE)                       \
    _x(LISTENER_THREW, D_TEST_EVENT_NAME_LISTENER_THREW)


#endif  // DJINTERP_C_TEST_EVENT_NAMES_H
