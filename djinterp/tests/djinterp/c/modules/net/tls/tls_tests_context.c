/*******************************************************************************
* djinterp [c][net]                                          tls_tests_context.c
*
* Tests of the TLS kernel's contexts.
*   A context is refused in one of two places: the kernel refuses what a
* configuration gets wrong before any engine is asked, and the engine
* refuses what it cannot load. An engine that counts its context_create
* calls, and passes each on to the OpenSSL-family engine, tells the two
* apart.
*
*
* path:      /tests/djinterp/c/modules/net/tls/tls_tests_context.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.31
*******************************************************************************/
#include "./tls_tests.h"  // corresponding header
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
// std
#include <stdio.h>  // snprintf


// d_tests_tls_creates
//   variable: the context_create calls the counting engine has passed on.
static int d_tests_tls_creates = 0;

/*
d_tests_tls_counting_create
  Counts the call, then passes it to the OpenSSL-family engine.
*/
static enum dj_tls_error
d_tests_tls_counting_create(
    const struct dj_tls_config* _config,
    const struct dj_tls_plan*   _plan,
    void**                     _out_context
)
{
    ++d_tests_tls_creates;

    return d_tls_engine_openssl()->context_create(_config,
                                                  _plan,
                                                  _out_context);
}

/*
d_tests_tls_counting_engine
  The OpenSSL-family engine, with its context_create counted.
*/
static
struct dj_tls_engine d_tests_tls_counting_engine(void)
{
    struct dj_tls_engine engine = *d_tls_engine_openssl();

    engine.context_create = d_tests_tls_counting_create;

    return engine;
}

/*
d_tests_tls_refused_as
  Whether a context made by `_engine` from `_config` is refused with
`_expected`, after `_asked` calls to the engine's context_create, and left
empty. The context starts out holding values a refusal must clear.
*/
static bool
d_tests_tls_refused_as(
    const struct dj_tls_engine* _engine,
    const struct dj_tls_config* _config,
    enum dj_tls_error           _expected,
    int                        _asked
)
{
    struct dj_tls_context context = { .engine = d_tls_engine_openssl(),
                                     .handle = &d_tests_tls_creates };

    d_tests_tls_creates = 0;

    const enum dj_tls_error status = d_tls_context_init(&context,
                                                       _engine,
                                                       _config);
    const bool             empty  = ( (context.engine == NULL) &&
                                      (context.handle == NULL) );

    // a refusal that was not one keeps nothing alive past the test
    if (status == D_TLS_OK)
    {
        d_tls_context_destroy(&context);
    }

    return ( (status == _expected)           &&
             (d_tests_tls_creates == _asked) &&
             (empty) );
}

/*
d_tests_tls_context_arguments
  The argument half of d_tests_tls_contexts: NULLs, and an engine
without a required operation, are refused before anything is asked.
*/
static
bool d_tests_tls_context_arguments(
    const struct dj_tls_engine* _counting,
    struct dj_test_suite*       _suite
)
{
    struct dj_tls_config config     = { .verify_hostname = true };
    struct dj_tls_engine incomplete = *_counting;

    d_tests_tls_client_config(&config);
    incomplete.describe = NULL;

    const bool refused =
        ( (d_tls_context_init(NULL,
                              _counting,
                              &config) == D_TLS_ERROR_INVALID_ARGUMENT) &&
          (d_tests_tls_refused_as(NULL,
                                  &config,
                                  D_TLS_ERROR_INVALID_ARGUMENT,
                                  0))                                   &&
          (d_tests_tls_refused_as(_counting,
                                  NULL,
                                  D_TLS_ERROR_INVALID_ARGUMENT,
                                  0))                                   &&
          (d_tests_tls_refused_as(&incomplete,
                                  &config,
                                  D_TLS_ERROR_INVALID_ARGUMENT,
                                  0)) );

    return D_TESTS_NET_ASSERT(refused,
                              "d_tls_context_init",
                              "NULLs and incomplete engines are refused",
                              _suite);
}

/*
d_tests_tls_context_kernel
  The kernel's half of d_tests_tls_contexts: configurations it refuses
before the engine is asked.
*/
static
bool d_tests_tls_context_kernel(
    const struct dj_tls_engine*      _counting,
    const struct dj_tests_tls_world* _world,
    struct dj_test_suite*            _suite
)
{
    struct dj_tls_config bare    = { .verify_hostname = false };
    struct dj_tls_config half    = { .verify_hostname = false };
    struct dj_tls_config role    = { .verify_hostname = true };
    struct dj_tls_config version = { .verify_hostname = true };
    struct dj_tls_config ssl3    = { .verify_hostname = true };

    d_tests_tls_server_config(&bare,
                              NULL,
                              NULL);
    d_tests_tls_server_config(&half,
                              _world->pki.server_cert,
                              NULL);
    d_tests_tls_client_config(&role);
    d_tests_tls_client_config(&version);
    d_tests_tls_client_config(&ssl3);
    role.role           = (enum dj_tls_role)7;
    version.min_version = (enum dj_tls_version)0x0305;
    ssl3.min_version    = D_TLS_VERSION_SSL3;

    const bool refused =
        ( (d_tests_tls_refused_as(_counting,
                                  &bare,
                                  D_TLS_ERROR_CONFIG,
                                  0)) &&
          (d_tests_tls_refused_as(_counting,
                                  &half,
                                  D_TLS_ERROR_CONFIG,
                                  0)) &&
          (d_tests_tls_refused_as(_counting,
                                  &role,
                                  D_TLS_ERROR_CONFIG,
                                  0)) &&
          (d_tests_tls_refused_as(_counting,
                                  &version,
                                  D_TLS_ERROR_CONFIG,
                                  0)) &&
          (d_tests_tls_refused_as(_counting,
                                  &ssl3,
                                  D_TLS_ERROR_INSECURE,
                                  0)) );

    return D_TESTS_NET_ASSERT(refused,
                              "d_tls_context_init",
                              "the kernel refuses bad configurations first",
                              _suite);
}

/*
d_tests_tls_context_engine
  The engine's half of d_tests_tls_contexts: files it cannot load, and a
sound configuration it makes once.
*/
static
bool d_tests_tls_context_engine(
    const struct dj_tls_engine*      _counting,
    const struct dj_tests_tls_world* _world,
    struct dj_test_suite*            _suite
)
{
    char                 absent[400] = { 0 };
    struct dj_tls_config  trust       = { .verify_hostname = true };
    struct dj_tls_config  mismatch    = { .verify_hostname = false };
    struct dj_tls_context context     = { .engine = NULL };

    (void)snprintf(absent,
                   sizeof(absent),
                   "%s/absent.pem",
                   _world->pki.directory);
    d_tests_tls_client_config(&trust);
    trust.ca_file = absent;
    d_tests_tls_server_config(&mismatch,
                              _world->pki.server_cert,
                              _world->pki.rogue_key);

    const bool refused = ( (d_tests_tls_refused_as(_counting,
                                                   &trust,
                                                   D_TLS_ERROR_CONFIG,
                                                   1)) &&
                           (d_tests_tls_refused_as(_counting,
                                                   &mismatch,
                                                   D_TLS_ERROR_CONFIG,
                                                   1)) );

    d_tests_tls_client_config(&trust);
    d_tests_tls_creates = 0;

    const bool made = ( (d_tls_context_init(&context,
                                            _counting,
                                            &trust) == D_TLS_OK) &&
                        (d_tests_tls_creates == 1)               &&
                        (context.engine == _counting)            &&
                        (context.plan.verify_hostname) );

    d_tls_context_destroy(&context);
    d_tls_context_destroy(&context);

    const bool result = D_TESTS_NET_ASSERT(refused,
                                           "d_tls_context_init",
                                           "the engine refuses files it "
                                           "cannot load",
                                           _suite);

    return D_TESTS_NET_ASSERT( ( (made) &&
                                 (context.engine == NULL) ),
                               "d_tls_context_init",
                               "a sound context is made, and destroyed",
                               _suite) && result;
}

/*
d_tests_tls_contexts
  Tests the following:
  - NULL arguments, and an engine missing a required operation, are
    INVALID_ARGUMENT, and leave the context empty
  - a server without a certificate, half a certificate-and-key pair, an
    unknown role, and an unknown version floor are CONFIG, and SSL 3.0 is
    INSECURE (UNSUPPORTED until 2026.10.31), each before the engine is asked
  - an unreadable trust store, and a certificate with another's key, are
    the engine's CONFIG
  - a sound configuration is made with one call to the engine, and a
    second destroy is harmless
*/
bool d_tests_tls_contexts(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_tls_world* const world    = d_tests_tls_world();
    const struct dj_tls_engine       counting = d_tests_tls_counting_engine();
    bool                            result   = true;

    // the run's certificates name the files these tests load
    if (world == NULL)
    {
        return false;
    }

    result = d_tests_tls_context_arguments(&counting,
                                           _suite) && result;
    result = d_tests_tls_context_kernel(&counting,
                                        world,
                                        _suite) && result;
    result = d_tests_tls_context_engine(&counting,
                                        world,
                                        _suite) && result;

    return result;
}
