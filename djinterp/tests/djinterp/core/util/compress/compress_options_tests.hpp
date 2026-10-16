/*******************************************************************************
* djinterp [test]                                     compress_options_tests.hpp
*
*   Unit-test header for compress_options.hpp - the codec-aware compression
* option vocabulary (the expanded, authoritative form of the minimal
* compress_options that compress.hpp ships).  It declares every test function
* (all flat in djinterp::testing) and the shared check macro and helpers the
* section translation units use; it defines no test bodies itself.
*
*   compress_options.hpp is pure, header-only data: eight categorical enums,
* six per-codec option structs, and the compress_options aggregate, with all
* defaults set in constructors.  There is no behavior to drive - so this suite
* verifies the DATA CONTRACT the rest of the library relies on:
*   - every field's default matches its documented sentinel (the whole
*     "leave it to the backend" design rests on these, and several are
*     deliberately non-zero: lzma.check is crc64, lzma.lc/lp/pb and
*     brotli.quality are -1, zstd.content_size_flag / dict_id_flag are true,
*     compress_options.level is -1);
*   - each categorical enum is a PLAIN enum (int-convertible, version-portable)
*     whose neutral first enumerator is 0 and whose ordering is stable;
*   - each struct is trivially copyable and standard-layout (the header's
*     "POD-ish, C++98-portable, ABI-stable" claim - though not is_trivial,
*     since the constructors are user-provided);
*   - the aggregate default-constructs each sub-block, and compress_options has
*     value semantics (independent, deep copies).
*
*   The suite is header-only and does not link the compressor: facade-level
* consumption of options (the generic `level` mapped onto each codec) is
* covered by the compress suite's options section; compress.cpp currently reads
* only `level`, so the advanced per-codec fields are forward-looking vocabulary
* whose contract - defaults and layout - is what this suite pins.
*
*   The sections mirror the header's own table of contents, one .cpp per region:
*   - compress_options_tests_enums.cpp       the eight categorical enums
*   - compress_options_tests_deflate.cpp     deflate_options
*   - compress_options_tests_bzip2.cpp       bzip2_options
*   - compress_options_tests_lzma.cpp        lzma_options
*   - compress_options_tests_zstd.cpp        zstd_options
*   - compress_options_tests_lz4.cpp         lz4_options
*   - compress_options_tests_brotli.cpp      brotli_options
*   - compress_options_tests_aggregate.cpp   compress_options
*
*
* path:      /tests/djinterp/core/util/compress/compress_options_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COMPRESS_OPTIONS_
#define DJINTERP_TESTS_COMPRESS_OPTIONS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../../../inc/djinterp/djinterp.hpp"            // NS_DJINTERP etc. - MUST precede the header
#include "../../../../../inc/djinterp/core/util/compress/compress_options.hpp"    // the vocabulary under test


// D_CO_CHECK
//   macro: the section suites' assertion primitive.  Evaluates _cond and, if it
// is false, returns false from the enclosing bool() test function immediately.
// Unique two-letter prefix (CO == Compress Options) so it never collides with a
// co-compiled suite's macro (e.g. the compress suite's D_CP_CHECK).
#define D_CO_CHECK(_cond)                                                       \
    do                                                                         \
    {                                                                         \
        if (!(_cond))                                                         \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (false)


NS_DJINTERP
NS_TESTING


// -- shared helpers -----------------------------------------------------------
//   Small compile-time-property probes used across the section suites.  They
// are function templates (implicitly inline), so every translation unit that
// includes this header may instantiate them without an ODR conflict.

// pod_ish_layout
//   helper: true iff _T is safe to treat as portable, ABI-stable plain data -
// trivially copyable (memcpy-safe), standard-layout (predictable field order,
// no reordering), trivially destructible, and non-polymorphic (no vtable).
// This is exactly the header's "POD-ish" claim; note it does NOT require
// is_trivial, because the structs have user-provided constructors that install
// their sentinels.
template<typename _T>
inline bool
pod_ish_layout()
{
    return ( std::is_trivially_copyable<_T>::value    &&
             std::is_standard_layout<_T>::value       &&
             std::is_trivially_destructible<_T>::value &&
             (!std::is_polymorphic<_T>::value) );
}

// plain_enum
//   helper: true iff _E is a PLAIN (unscoped) enum - one that converts
// implicitly to int (an enum class would not).  Confirms the header's choice of
// version-portable plain enums over enum class.
template<typename _E>
inline bool
plain_enum()
{
    return ( std::is_enum<_E>::value &&
             std::is_convertible<_E, int>::value );
}


// -- compress_options_tests_enums.cpp -----------------------------------------
bool tests_compress_options_enum_deflate_strategy();
bool tests_compress_options_enum_lzma_check();
bool tests_compress_options_enum_lzma_mode();
bool tests_compress_options_enum_lzma_match_finder();
bool tests_compress_options_enum_zstd_strategy();
bool tests_compress_options_enum_lz4_block_size();
bool tests_compress_options_enum_lz4_block_mode();
bool tests_compress_options_enum_brotli_mode();
bool tests_compress_options_enum_neutral_defaults_are_zero();
bool tests_compress_options_enum_are_plain();

// -- compress_options_tests_deflate.cpp ---------------------------------------
bool tests_compress_options_deflate_defaults();
bool tests_compress_options_deflate_fields();
bool tests_compress_options_deflate_layout();

// -- compress_options_tests_bzip2.cpp -----------------------------------------
bool tests_compress_options_bzip2_defaults();
bool tests_compress_options_bzip2_fields();
bool tests_compress_options_bzip2_layout();

// -- compress_options_tests_lzma.cpp ------------------------------------------
bool tests_compress_options_lzma_defaults();
bool tests_compress_options_lzma_sentinels();
bool tests_compress_options_lzma_fields();
bool tests_compress_options_lzma_layout();

// -- compress_options_tests_zstd.cpp ------------------------------------------
bool tests_compress_options_zstd_defaults();
bool tests_compress_options_zstd_flag_defaults();
bool tests_compress_options_zstd_fields();
bool tests_compress_options_zstd_layout();

// -- compress_options_tests_lz4.cpp -------------------------------------------
bool tests_compress_options_lz4_defaults();
bool tests_compress_options_lz4_fields();
bool tests_compress_options_lz4_layout();

// -- compress_options_tests_brotli.cpp ----------------------------------------
bool tests_compress_options_brotli_defaults();
bool tests_compress_options_brotli_fields();
bool tests_compress_options_brotli_layout();

// -- compress_options_tests_aggregate.cpp -------------------------------------
bool tests_compress_options_aggregate_level_default();
bool tests_compress_options_aggregate_subobjects_defaulted();
bool tests_compress_options_aggregate_independence();
bool tests_compress_options_aggregate_value_semantics();
bool tests_compress_options_aggregate_layout();
bool tests_compress_options_aggregate_minus_one_sentinels();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_COMPRESS_OPTIONS_
