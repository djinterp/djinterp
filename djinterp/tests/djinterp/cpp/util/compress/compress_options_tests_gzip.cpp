/*******************************************************************************
* djinterp [cpp][utility]                        compress_options_tests_gzip.cpp
*
*   Tests for the gzip block (DGzipOptions): the header's OS byte, its choices
* and its equality helper (compress_options.hpp section II).  The block is the
* core's struct; a pristine one has its knob UNSET, which the core resolves to
* the build's byte, 255 unless the build changes it.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_options_tests_gzip.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

// std
#include <cstring>  // std::strcmp
// djinterp
#include "./compress_options_tests.hpp"  // the suite


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_gzip_defaults
  Confirms the gzip block of a default-constructed compress_options is
  pristine, and that resolving for GZIP gives the build's byte.
  Tests the following:
  - os is D_COMPRESS_KNOB_UNSET
  - resolved for GZIP, os is D_INTERNAL_COMPRESS_GZIP_OS, which is
    gzip_os_unknown (255) in a build that doesn't change it
  - the choices are the core's constants
*/
bool tests_compress_options_gzip_defaults()
{
    compress_options           o;
    struct dj_compress_options r;

    D_CO_CHECK(o.gzip.os == D_COMPRESS_KNOB_UNSET);

    D_CO_CHECK(d_compress_options_resolve(D_CODEC_ID_GZIP, &o, &r) ==
               D_PACK_STATUS_OK);
    D_CO_CHECK(r.gzip.os == D_INTERNAL_COMPRESS_GZIP_OS);
#if (D_CFG_COMPRESS_GZIP_OS == 255)
    D_CO_CHECK(r.gzip.os == gzip_os_unknown);
#endif

    D_CO_CHECK(gzip_os_default == D_GZIP_OS_DEFAULT);
    D_CO_CHECK(gzip_os_zlib == D_GZIP_OS_ZLIB);
    D_CO_CHECK(gzip_os_unknown == 255);

    return true;
}

/*
tests_compress_options_gzip_fields
  Confirms the knob is writable, compares through gzip_options_equal, and is
  the 51st knob of the whole set.
  Tests the following:
  - a written os reads back, and leaves every other knob UNSET
  - gzip_options_equal tells two blocks apart by os alone
  - the set's diff names it "gzip.os"
*/
bool tests_compress_options_gzip_fields()
{
    compress_options a;
    compress_options b;

    b.gzip.os = 3;
    D_CO_CHECK(b.gzip.os == 3);
    D_CO_CHECK(b.brotli.large_window == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(b.level == D_COMPRESS_KNOB_UNSET);

    D_CO_CHECK(gzip_options_equal(a.gzip, a.gzip));
    D_CO_CHECK(!gzip_options_equal(a.gzip, b.gzip));

    enum dj_compress_knob moved[D_COMPRESS_KNOB_COUNT];

    D_CO_CHECK(d_compress_options_diff(&a, &b, moved,
                                       (size_t)D_COMPRESS_KNOB_COUNT) == 1u);
    D_CO_CHECK(moved[0] == D_COMPRESS_KNOB_GZIP_OS);
    D_CO_CHECK(std::strcmp(d_compress_knob_name(moved[0]), "gzip.os") == 0);

    return true;
}

/*
tests_compress_options_gzip_layout
  Confirms DGzipOptions is portable plain data of its one knob, and that it
  sits last in the set, so no knob before it moved when it was added.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible
    and non-polymorphic
  - its size is 1 int32_t, at offset 50 int32_t in the set
*/
bool tests_compress_options_gzip_layout()
{
    D_CO_CHECK(pod_ish_layout<DGzipOptions>());
    D_CO_CHECK(sizeof(DGzipOptions) == sizeof(int32_t));
    D_CO_CHECK(offsetof(struct dj_compress_options, gzip) ==
               (50 * sizeof(int32_t)));

    return true;
}


NS_END  // testing
NS_END  // djinterp
