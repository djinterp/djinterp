/*******************************************************************************
* djinterp [c]                                                   smtp_tests_sa.h
*
* djinterp SMTP kernel tests -- standalone.
*   Declarations for the standalone tests of the C SMTP kernel: the protocol
* vocabulary (smtp_tests_sa_common.c); scripted client and server sessions,
* and live loopback sessions over plaintext, STARTTLS and implicit TLS
* (smtp_tests_sa_session.c); and the runner (smtp_tests_sa_main.c).
*   The loopback tests need D_ENV_SMTP_CAN_SERVE, and the TLS ones
* D_ENV_SMTP_CAN_TLS; where a capability is missing they pass as skipped.
*
*
* path:      /tests/djinterp/c/net/smtp/smtp_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

#ifndef DJINTERP_C_NET_SMTP_SMTP_TESTS_SA_H
#define DJINTERP_C_NET_SMTP_SMTP_TESTS_SA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../inc/djinterp/c/net/smtp/smtp.h"         // kernel
#include "../../../../../inc/djinterp/test/c/test_standalone.h"  // asserts


D_EXTERN_C_BEGIN

// protocol vocabulary (smtp_tests_sa_common.c)
bool d_tests_sa_smtp_buffer(struct d_test_counter* _counter);
bool d_tests_sa_smtp_reply(struct d_test_counter* _counter);
bool d_tests_sa_smtp_capabilities(struct d_test_counter* _counter);
bool d_tests_sa_smtp_validation(struct d_test_counter* _counter);
bool d_tests_sa_smtp_commands(struct d_test_counter* _counter);
bool d_tests_sa_smtp_base64(struct d_test_counter* _counter);
bool d_tests_sa_smtp_data(struct d_test_counter* _counter);

// sessions (smtp_tests_sa_session.c)
bool d_tests_sa_smtp_client_script(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_serial(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_guards(struct d_test_counter* _counter);
bool d_tests_sa_smtp_server_script(struct d_test_counter* _counter);
bool d_tests_sa_smtp_server_auth(struct d_test_counter* _counter);
bool d_tests_sa_smtp_loopback(struct d_test_counter* _counter);
bool d_tests_sa_smtp_loopback_tls(struct d_test_counter* _counter);

// fixtures and the suite, shared with the C++ tests: a self-signed P-256
// certificate for "localhost" and 127.0.0.1 written to two PEM files
bool d_tests_sa_smtp_certificate(char*  _certificate,
                                 char*  _key,
                                 size_t _capacity);
bool d_tests_sa_smtp_run_all(struct d_test_counter* _counter);

D_EXTERN_C_END


#endif  // DJINTERP_C_NET_SMTP_SMTP_TESTS_SA_H
