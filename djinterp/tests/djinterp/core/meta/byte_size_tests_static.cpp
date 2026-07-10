/******************************************************************************
* djinterp [test]                                    byte_size_tests_static.cpp
*
*   Section I of byte_size.hpp: static_byte_size / static_byte_size_v.
*
*   The static footprint is EXACT: sizeof(clean_t<_Container>).  Pinned here:
*     - equality with sizeof for scalars, library containers (handle-holding
*       dynamic ones have a small fixed static size), and inline containers
*       (whose static size grows with their inline cell count);
*     - the clean_t stripping, so cv- and reference-qualified inputs report the
*       size of the bare object;
*     - the static_byte_size_v companion (gated to the standards that have it).
*
*
* path:      /inc/djinterp/test/byte_size_tests_static.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.03
******************************************************************************/

#include "byte_size_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace byte_size_test_types;


// =========================================================================
// I.   EXACT sizeof
// =========================================================================

static_assert(static_byte_size<int>::value    == sizeof(int),    "static: int");
static_assert(static_byte_size<double>::value == sizeof(double), "static: double");
static_assert(static_byte_size<wide_cell>::value == sizeof(wide_cell),
              "static: custom 32-byte cell");
static_assert(static_byte_size<wide_cell>::value == 32,
              "static: wide_cell is 32 bytes");

static_assert(static_byte_size<std::vector<int>>::value == sizeof(std::vector<int>),
              "static: vector object size");
static_assert(static_byte_size<std::list<int>>::value == sizeof(std::list<int>),
              "static: list object size");
static_assert(static_byte_size<std::string>::value == sizeof(std::string),
              "static: string object size");
static_assert(static_byte_size<std::array<int, 10>>::value == sizeof(std::array<int, 10>),
              "static: array object size");

// a handle-holding dynamic container's static size is independent of element
// COUNT (there is no instance in the trait) and small relative to its content
static_assert(static_byte_size<std::vector<char>>::value == sizeof(std::vector<char>),
              "static: vector<char> object size");

// an inline container's static size grows with its inline cell count
static_assert(static_byte_size<std::array<int, 100>>::value
            > static_byte_size<std::array<int, 10>>::value,
              "static: bigger inline array -> bigger static footprint");
static_assert(static_byte_size<inline_bag<int, 8>>::value == sizeof(inline_bag<int, 8>),
              "static: inline_bag object size");


// =========================================================================
// II.  clean_t STRIPPING
// =========================================================================

static_assert(static_byte_size<const std::vector<int>&>::value == sizeof(std::vector<int>),
              "static: const-ref strips to bare size");
static_assert(static_byte_size<std::vector<int>&&>::value == sizeof(std::vector<int>),
              "static: rvalue-ref strips to bare size");
static_assert(static_byte_size<volatile std::array<int, 4>>::value == sizeof(std::array<int, 4>),
              "static: volatile strips to bare size");
static_assert(static_byte_size<const volatile int&>::value == sizeof(int),
              "static: cv-ref scalar strips to bare size");


// =========================================================================
// III. static_byte_size_v  (gated)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(static_byte_size_v<std::vector<int>> == static_byte_size<std::vector<int>>::value,
                  "static_v: mirrors ::value (vector)");
    static_assert(static_byte_size_v<std::array<int, 10>> == sizeof(std::array<int, 10>),
                  "static_v: array");
    static_assert(static_byte_size_v<const std::string&> == sizeof(std::string),
                  "static_v: clean_t stripping under the alias");
#endif


// =========================================================================
// IV.  RUN-TIME MIRRORS
// =========================================================================

bool
tests_byte_size_static_exact()
{
    bool _ok = true;

    _ok = _ok && (static_byte_size<int>::value == sizeof(int));
    _ok = _ok && (static_byte_size<wide_cell>::value == 32);
    _ok = _ok && (static_byte_size<std::vector<int>>::value == sizeof(std::vector<int>));
    _ok = _ok && (static_byte_size<std::array<int, 10>>::value == sizeof(std::array<int, 10>));
    _ok = _ok && (static_byte_size<std::string>::value == sizeof(std::string));
    _ok = _ok && (static_byte_size<std::array<int, 100>>::value
                > static_byte_size<std::array<int, 10>>::value);

    return _ok;
}

bool
tests_byte_size_static_cleans()
{
    bool _ok = true;

    _ok = _ok && (static_byte_size<const std::vector<int>&>::value == sizeof(std::vector<int>));
    _ok = _ok && (static_byte_size<std::vector<int>&&>::value == sizeof(std::vector<int>));
    _ok = _ok && (static_byte_size<volatile std::array<int, 4>>::value == sizeof(std::array<int, 4>));
    _ok = _ok && (static_byte_size<const volatile int&>::value == sizeof(int));

    return _ok;
}

bool
tests_byte_size_static_v()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    bool _ok = true;

    _ok = _ok && (static_byte_size_v<std::vector<int>> == static_byte_size<std::vector<int>>::value);
    _ok = _ok && (static_byte_size_v<std::array<int, 10>> == sizeof(std::array<int, 10>));
    _ok = _ok && (static_byte_size_v<const std::string&> == sizeof(std::string));

    return _ok;
#else
    return true;   // no variable templates in this mode; nothing to check
#endif
}


NS_END  // testing
NS_END  // djinterp
