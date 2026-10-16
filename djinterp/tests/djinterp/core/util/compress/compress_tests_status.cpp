/*******************************************************************************
* djinterp [test]                                      compress_tests_status.cpp
*
*   Tests for the shared status vocabulary of the compression facade: the
* status enum (compress.hpp section II) and the status_message() lookup
* (compress.cpp section I).  These lock the numeric values callers and the
* dispatch leaves depend on, and confirm every code - plus an out-of-range
* value - maps to a non-empty, distinct human-readable string.
*
*
* path:      /tests/djinterp/core/util/compress/compress_tests_status.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstring>
#include <string>
// djinterp
#include "./compress_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_status_messages
  Confirms status_message() returns a usable description for every defined
  status code.
  Tests the following:
  - each of the six status codes yields a non-null, non-empty string
  - no two codes share the same message (each is distinct)
*/
bool
tests_compress_status_messages()
{
    const status codes[] =
    {
        status_ok,
        status_unavailable,
        status_invalid_argument,
        status_buffer_error,
        status_backend_error,
        status_unsupported
    };
    const std::size_t n = (sizeof(codes) / sizeof(codes[0]));
    std::size_t       i;
    std::size_t       j;

    // every code has a non-null, non-empty message
    for (i = 0; i < n; ++i)
    {
        const char* msg = status_message(codes[i]);

        D_CP_CHECK(msg != 0);
        D_CP_CHECK(msg[0] != '\0');
    }

    // and the messages are pairwise distinct
    for (i = 0; i < n; ++i)
    {
        for (j = (i + 1); j < n; ++j)
        {
            D_CP_CHECK(std::strcmp(status_message(codes[i]),
                                   status_message(codes[j])) != 0);
        }
    }

    // spot-check the anchor value
    D_CP_CHECK(std::strcmp(status_message(status_ok), "ok") == 0);

    return true;
}

/*
tests_compress_status_unknown
  Confirms status_message() is total - an out-of-range value still yields a
  safe, non-empty fallback string rather than reading past the switch.
  Tests the following:
  - a value outside the enum maps to a non-null, non-empty message
  - that fallback differs from the "ok" message
*/
bool
tests_compress_status_unknown()
{
    // a value deliberately outside the defined enumerators exercises the
    // switch's default arm
    status      bogus = static_cast<status>(9999);
    const char* msg   = status_message(bogus);

    D_CP_CHECK(msg != 0);
    D_CP_CHECK(msg[0] != '\0');
    D_CP_CHECK(std::strcmp(msg, status_message(status_ok)) != 0);

    return true;
}

/*
tests_compress_status_enum_values
  Pins the numeric values of the status enum.  The dispatch leaves and callers
  compare against these, and status_ok == 0 is relied on throughout, so a
  reordering must be caught here.
  Tests the following:
  - status_ok is zero
  - the remaining codes hold their documented, contiguous ordering
*/
bool
tests_compress_status_enum_values()
{
    D_CP_CHECK(static_cast<int>(status_ok)               == 0);
    D_CP_CHECK(static_cast<int>(status_unavailable)      == 1);
    D_CP_CHECK(static_cast<int>(status_invalid_argument) == 2);
    D_CP_CHECK(static_cast<int>(status_buffer_error)     == 3);
    D_CP_CHECK(static_cast<int>(status_backend_error)    == 4);
    D_CP_CHECK(static_cast<int>(status_unsupported)      == 5);

    return true;
}


NS_END  // testing
NS_END  // djinterp
