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
* path:      /tests/djinterp/cpp/util/compress/compress_tests_status.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.10
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
  - the message is the kernel's (d_pack_status_message), word for word
*/
bool tests_compress_status_messages()
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

    // the facade's text is the kernel's, so the two faces say the same thing
    D_CP_CHECK(std::strcmp(status_message(status_ok),
                           d_pack_status_message(D_PACK_STATUS_OK)) == 0);

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
bool tests_compress_status_unknown()
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
  Pins the numeric values of the status enum to the kernel's taxonomy
  (compress_common.h 1.1): formal statuses -- the request is not defined --
  below 0x100, mechanical ones -- the machinery ran short -- from 0x100. The
  dispatch leaves and callers compare against these, and status_ok == 0 is
  relied on throughout, so a renumbering must be caught here.
  Tests the following:
  - status_ok is zero
  - the formal codes are 1 to 6, in the kernel's order
  - the mechanical codes start at 0x100, and the facade's names map to them
*/
bool tests_compress_status_enum_values()
{
    D_CP_CHECK(static_cast<int>(status_ok)               == 0);

    D_CP_CHECK(static_cast<int>(status_unavailable)      == 0x001);
    D_CP_CHECK(static_cast<int>(status_unsupported)      == 0x002);
    D_CP_CHECK(static_cast<int>(status_invalid_argument) == 0x003);
    D_CP_CHECK(static_cast<int>(status_corrupt_input)    == 0x004);
    D_CP_CHECK(static_cast<int>(status_truncated_input)  == 0x005);
    D_CP_CHECK(static_cast<int>(status_wrong_password)   == 0x006);

    D_CP_CHECK(static_cast<int>(status_buffer_error)     == 0x100);
    D_CP_CHECK(static_cast<int>(status_no_memory)        == 0x101);
    D_CP_CHECK(static_cast<int>(status_backend_error)    == 0x103);

    return true;
}


NS_END  // testing
NS_END  // djinterp
