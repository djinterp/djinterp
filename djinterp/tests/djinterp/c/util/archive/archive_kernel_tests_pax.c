/*******************************************************************************
* djinterp [test]                                     archive_kernel_tests_pax.c
*
*   The tar writer's pax extended headers.  Under the default format (pax) a
* member whose name, size or timestamp ustar cannot hold is written with a pax
* record, and the rest exactly as ustar writes them; strict ustar refuses such
* a member; gnu and v7 are not written.
*
*
* path:      /tests/djinterp/c/util/archive/archive_kernel_tests_pax.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "archive_kernel_tests.h"  // the suite's helpers
// std
#include <stdlib.h>  // free
#include <string.h>  // memcmp, memcpy, memset


/*
d_tests_ak_pax_names
  Tests the following:
  - a 300-byte path, a single 120-byte component and a 150-byte directory
    round-trip in full, through tar and tar.gz
  - the archive carries a pax extended header ('x') named PaxHeader/...
  - extract_entry finds a member by its long name
*/
static void
d_tests_ak_pax_names(void)
{
    char                     path300[301];
    char                     comp120[121];
    char                     dir150[151];
    struct dj_archive_entry  e[3];
    struct dj_archive_entry* got = NULL;
    struct dj_archive_layout layout;
    unsigned char*           tar;
    unsigned char            one[8];
    size_t                   size = 0u;
    size_t                   n    = 0u;
    enum dj_pack_status       status;

    d_tests_ak_open("pax_names",
                    "names ustar cannot hold, in pax records");

    memset(path300, 'p', 300);
    path300[120] = '/';
    path300[240] = '/';
    path300[300] = '\0';
    memset(comp120, 'c', 120);
    comp120[120] = '\0';
    memset(dir150, 'd', 150);
    dir150[150] = '\0';

    e[0] = d_tests_ak_file(path300, "one", 3u);
    e[1] = d_tests_ak_file(comp120, "two", 3u);
    e[2] = d_tests_ak_file(dir150, NULL, 0u);
    e[2].is_directory = 1;

    tar = d_tests_ak_create(D_FORMAT_ID_TAR, e, 3u, NULL, &size, &status);
    D_TESTS_AK_CHECK(status == D_PACK_STATUS_OK);

    if (tar == NULL)
    {
        return;
    }

    D_TESTS_AK_CHECK( (memcmp(tar, "PaxHeader/", 10) == 0) &&
                      (tar[156] == 'x') );
    D_TESTS_AK_CHECK(d_tests_ak_read(D_FORMAT_ID_TAR, tar, size, &got,
                                     &layout, &status));
    D_TESTS_AK_CHECK( (got != NULL) &&
                      (layout.entry_count == 3u) &&
                      d_tests_ak_has(got, 3u, path300, 300u, "one", 3u) &&
                      d_tests_ak_has(got, 3u, comp120, 120u, "two", 3u) &&
                      d_tests_ak_has(got, 3u, dir150, 150u, NULL, 0u) &&
                      got[2].is_directory );
    D_TESTS_AK_CHECK( (d_archive_extract_entry(
                           D_FORMAT_ID_TAR, tar, size,
                           d_tests_ak_text(comp120), NULL,
                           one, sizeof(one), &n) == D_PACK_STATUS_OK) &&
                      (n == 3u) &&
                      (memcmp(one, "two", 3) == 0) );
    free(got);
    free(tar);

    tar = d_tests_ak_create(D_FORMAT_ID_TAR_GZ, e, 3u, NULL, &size,
                            &status);
    D_TESTS_AK_CHECK( (tar != NULL) &&
                      d_tests_ak_read(D_FORMAT_ID_TAR_GZ, tar, size, &got,
                                      &layout, &status) &&
                      d_tests_ak_has(got, 3u, path300, 300u, "one", 3u) );
    free(got);
    free(tar);

    return;
}

/*
d_tests_ak_pax_times
  Tests the following:
  - a timestamp before 1970 and one past the ustar field (the year 2242)
    round-trip exactly, in pax mtime records
*/
static void
d_tests_ak_pax_times(void)
{
    struct dj_archive_entry  e[2];
    struct dj_archive_entry* got = NULL;
    struct dj_archive_layout layout;
    unsigned char*           tar;
    size_t                   size = 0u;
    enum dj_pack_status       status;

    d_tests_ak_open("pax_times",
                    "timestamps outside the ustar field, in pax records");

    e[0]       = d_tests_ak_file("before", "b", 1u);
    e[0].mtime = -86400;
    e[1]       = d_tests_ak_file("after", "a", 1u);
    e[1].mtime = (int64_t)8589934592 + 100;

    tar = d_tests_ak_create(D_FORMAT_ID_TAR, e, 2u, NULL, &size, &status);
    D_TESTS_AK_CHECK( (tar != NULL) &&
                      d_tests_ak_read(D_FORMAT_ID_TAR, tar, size, &got,
                                      &layout, &status) &&
                      (layout.entry_count == 2u) );

    if (got != NULL)
    {
        D_TESTS_AK_CHECK(got[0].mtime == -86400);
        D_TESTS_AK_CHECK(got[1].mtime == (int64_t)8589934592 + 100);
    }

    free(got);
    free(tar);

    return;
}

/*
d_tests_ak_pax_formats
  Tests the following:
  - members that fit ustar are written byte for byte the same under pax and
    under ustar: pax adds nothing they do not need
  - strict ustar refuses a long name: D_PACK_STATUS_UNSUPPORTED
  - gnu and v7 are D_PACK_STATUS_UNSUPPORTED, naming the tar format knob;
    a value that is no format is D_PACK_STATUS_INVALID_ARGUMENT
*/
static void
d_tests_ak_pax_formats(void)
{
    char                      comp120[121];
    struct dj_archive_entry   e;
    struct dj_archive_options opt;
    enum dj_archive_knob      offender = D_ARCHIVE_KNOB_LEVEL;
    unsigned char*            pax;
    unsigned char*            ustar;
    size_t                    pax_size   = 0u;
    size_t                    ustar_size = 0u;
    enum dj_pack_status        status;

    d_tests_ak_open("pax_formats",
                    "pax by default, strict ustar on request, no gnu or v7");

    e = d_tests_ak_file("dir/short.txt", "short", 5u);
    d_archive_options_init(&opt);
    opt.tar.format = D_TAR_FORMAT_USTAR;

    pax   = d_tests_ak_create(D_FORMAT_ID_TAR, &e, 1u, NULL, &pax_size,
                              &status);
    ustar = d_tests_ak_create(D_FORMAT_ID_TAR, &e, 1u, &opt, &ustar_size,
                              &status);
    D_TESTS_AK_CHECK( (pax != NULL) &&
                      (ustar != NULL) &&
                      (pax_size == ustar_size) &&
                      (memcmp(pax, ustar, pax_size) == 0) );
    free(pax);
    free(ustar);

    memset(comp120, 'c', 120);
    comp120[120] = '\0';
    e     = d_tests_ak_file(comp120, "x", 1u);
    ustar = d_tests_ak_create(D_FORMAT_ID_TAR, &e, 1u, &opt, &ustar_size,
                              &status);
    D_TESTS_AK_CHECK( (ustar == NULL) &&
                      (status == D_PACK_STATUS_UNSUPPORTED) );

    opt.tar.format = D_TAR_FORMAT_GNU;
    D_TESTS_AK_CHECK( (d_archive_options_validate(D_FORMAT_ID_TAR, &opt,
                                                  &offender) ==
                       D_PACK_STATUS_UNSUPPORTED) &&
                      (offender == D_ARCHIVE_KNOB_TAR_FORMAT) );
    opt.tar.format = D_TAR_FORMAT_V7;
    D_TESTS_AK_CHECK(d_archive_options_validate(D_FORMAT_ID_TAR_GZ, &opt,
                                                NULL) ==
                     D_PACK_STATUS_UNSUPPORTED);
    opt.tar.format = 9;
    D_TESTS_AK_CHECK(d_archive_options_validate(D_FORMAT_ID_TAR, &opt,
                                                NULL) ==
                     D_PACK_STATUS_INVALID_ARGUMENT);

    // resolved, an unset format is pax
    d_archive_options_init(&opt);
    D_TESTS_AK_CHECK( (d_archive_options_resolve(D_FORMAT_ID_TAR, &opt,
                                                 &opt) ==
                       D_PACK_STATUS_OK) &&
                      (opt.tar.format == D_TAR_FORMAT_PAX) );

    return;
}


/*
d_tests_ak_pax
  The section: pax extended headers in the writer.
*/
void
d_tests_ak_pax(void)
{
    d_tests_ak_pax_names();
    d_tests_ak_pax_times();
    d_tests_ak_pax_formats();

    return;
}
