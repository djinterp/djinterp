/*******************************************************************************
* djinterp [test]                                  archive_kernel_tests_staged.c
*
*   The staged create and tar.gz's read path.  tar.gz is written through a
* caller's stage -- by the archive.h macros from named arrays, and by the
* calls -- and read with nothing staged; ZIP through a stage is byte for byte
* ZIP without one.  The stage and the output are separate regions with
* separate answers: a short stage is INVALID_ARGUMENT, a short output
* BUFFER_TOO_SMALL.
*
*
* path:      /tests/djinterp/c/util/archive/archive_kernel_tests_staged.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "archive_kernel_tests.h"  // the suite's helpers
// std
#include <stdlib.h>  // free, malloc
#include <string.h>  // memcmp, memset


// the members every test here archives: a file, a directory, a file in it
static const char d_tests_ak_body[] = "a payload long enough to compress, "
                                      "a payload long enough to compress";

static struct dj_archive_entry d_tests_ak_list[3];


/*
d_tests_ak_discard
  A sink write that accepts and drops everything.
*/
static size_t
d_tests_ak_discard(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_context;
    (void)_data;

    return _size;
}


/*
d_tests_ak_list_init
  Fill the shared member list.
*/
static void
d_tests_ak_list_init(void)
{
    d_tests_ak_list[0] = d_tests_ak_file("notes.txt", d_tests_ak_body,
                                         sizeof(d_tests_ak_body) - 1u);
    d_tests_ak_list[1] = d_tests_ak_file("dir/", NULL, 0u);
    d_tests_ak_list[1].is_directory = 1;
    d_tests_ak_list[2] = d_tests_ak_file("dir/inner", "xyz", 3u);

    return;
}

/*
d_tests_ak_staged_macros
  Tests the following:
  - D_ARCHIVE_MEASURE_TAR_GZ reports the tar's exact size, the size
    d_archive_create gives for the same members as a tar
  - D_ARCHIVE_CREATE_TAR_GZ writes a gzip stream from named arrays, and it
    reads back to the same members
*/
static void
d_tests_ak_staged_macros(void)
{
    static unsigned char      stage[8192];
    static unsigned char      out[8192];
    size_t                    tar_size = 0u;
    size_t                    measured = 0u;
    size_t                    written  = 0u;
    struct dj_archive_entry*  got      = NULL;
    struct dj_archive_layout  layout;
    enum dj_pack_status        status;

    d_tests_ak_open("staged_macros",
                    "the tar.gz macros: measure the stage, create into "
                    "arrays");

    D_TESTS_AK_CHECK(D_ARCHIVE_MEASURE_TAR_GZ(d_tests_ak_list, &measured) ==
                     D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(D_ARCHIVE_MEASURE(D_FORMAT_ID_TAR, d_tests_ak_list, 3u,
                                       &tar_size) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK( (measured == tar_size) &&
                      (measured <= sizeof(stage)) );
    D_TESTS_AK_CHECK(D_ARCHIVE_CREATE_TAR_GZ(d_tests_ak_list, stage, out,
                                             &written) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK( (written > 10u) &&
                      (out[0] == 0x1Fu) &&
                      (out[1] == 0x8Bu) &&
                      (out[2] == 0x08u) );
    D_TESTS_AK_CHECK(d_tests_ak_read(D_FORMAT_ID_TAR_GZ, out, written, &got,
                                     &layout, &status));
    D_TESTS_AK_CHECK(layout.entry_count == 3u);
    D_TESTS_AK_CHECK( (got != NULL) &&
                      d_tests_ak_has(got, layout.entry_count, "notes.txt",
                                     9u, d_tests_ak_body,
                                     sizeof(d_tests_ak_body) - 1u) &&
                      d_tests_ak_has(got, layout.entry_count, "dir/inner",
                                     9u, "xyz", 3u) );

    free(got);

    return;
}

/*
d_tests_ak_staged_regions
  Tests the following:
  - a stage shorter than d_archive_stage_size reports, and a null one, are
    D_PACK_STATUS_INVALID_ARGUMENT
  - a null output measures; a short one is D_PACK_STATUS_BUFFER_TOO_SMALL
    with the requirement
  - the unstaged create calls report tar.gz as D_PACK_STATUS_UNSUPPORTED
  - tar and gz need no stage: 0, and a null stage is accepted
*/
static void
d_tests_ak_staged_regions(void)
{
    static unsigned char stage[8192];
    unsigned char        small[16];
    size_t               need = 0u;
    size_t               size = 0u;
    size_t               full = 0u;
    struct dj_pack_sink  none;

    d_tests_ak_open("staged_regions",
                    "the stage and the output answer separately");

    D_TESTS_AK_CHECK(d_archive_stage_size(D_FORMAT_ID_TAR_GZ,
                                          d_tests_ak_list, 3u, NULL,
                                          &need) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_archive_create_staged(
                         D_FORMAT_ID_TAR_GZ, d_tests_ak_list, 3u, NULL,
                         stage, need - 1u, NULL, 0u, &size) ==
                     D_PACK_STATUS_INVALID_ARGUMENT);
    D_TESTS_AK_CHECK(d_archive_create_staged(
                         D_FORMAT_ID_TAR_GZ, d_tests_ak_list, 3u, NULL,
                         NULL, 0u, NULL, 0u, &size) ==
                     D_PACK_STATUS_INVALID_ARGUMENT);
    D_TESTS_AK_CHECK(d_archive_create_staged(
                         D_FORMAT_ID_TAR_GZ, d_tests_ak_list, 3u, NULL,
                         stage, need, NULL, 0u, &full) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_archive_create_staged(
                         D_FORMAT_ID_TAR_GZ, d_tests_ak_list, 3u, NULL,
                         stage, need, small, sizeof(small), &size) ==
                     D_PACK_STATUS_BUFFER_TOO_SMALL);
    D_TESTS_AK_CHECK( (size == full) &&
                      (full > sizeof(small)) );

    // no stage: the unstaged calls
    none.write   = &d_tests_ak_discard;
    none.context = NULL;
    D_TESTS_AK_CHECK(d_archive_create(D_FORMAT_ID_TAR_GZ, d_tests_ak_list,
                                      3u, NULL, NULL, 0u, &size) ==
                     D_PACK_STATUS_UNSUPPORTED);
    D_TESTS_AK_CHECK(d_archive_create_to_sink(D_FORMAT_ID_TAR_GZ,
                                              d_tests_ak_list, 3u, NULL,
                                              none, NULL) ==
                     D_PACK_STATUS_UNSUPPORTED);

    // formats with no stage
    D_TESTS_AK_CHECK( (d_archive_stage_size(D_FORMAT_ID_TAR,
                                            d_tests_ak_list, 3u, NULL,
                                            &need) == D_PACK_STATUS_OK) &&
                      (need == 0u) );
    D_TESTS_AK_CHECK( (d_archive_stage_size(D_FORMAT_ID_GZ,
                                            d_tests_ak_list, 1u, NULL,
                                            &need) == D_PACK_STATUS_OK) &&
                      (need == 0u) );
    D_TESTS_AK_CHECK(d_archive_create_staged(D_FORMAT_ID_TAR,
                                             d_tests_ak_list, 3u, NULL,
                                             NULL, 0u, NULL, 0u, &size) ==
                     D_PACK_STATUS_OK);

    return;
}

/*
d_tests_ak_staged_zip
  Tests the following:
  - a ZIP's stage is at least a kept plan per member
  - through a stage, the ZIP is byte for byte the ZIP written without one
*/
static void
d_tests_ak_staged_zip(void)
{
    unsigned char*      staged;
    unsigned char*      plain;
    size_t              staged_size = 0u;
    size_t              plain_size  = 0u;
    size_t              need        = 0u;
    enum dj_pack_status  status;

    d_tests_ak_open("staged_zip",
                    "a ZIP through a stage is the same ZIP");

    D_TESTS_AK_CHECK( (d_archive_stage_size(D_FORMAT_ID_ZIP,
                                            d_tests_ak_list, 3u, NULL,
                                            &need) == D_PACK_STATUS_OK) &&
                      (need >= 3u * sizeof(size_t)) );

    staged = d_tests_ak_create(D_FORMAT_ID_ZIP, d_tests_ak_list, 3u, NULL,
                               &staged_size, &status);

    D_TESTS_AK_CHECK(d_archive_create(D_FORMAT_ID_ZIP, d_tests_ak_list, 3u,
                                      NULL, NULL, 0u, &plain_size) ==
                     D_PACK_STATUS_OK);

    plain = (unsigned char*)malloc(plain_size);

    D_TESTS_AK_CHECK(d_archive_create(D_FORMAT_ID_ZIP, d_tests_ak_list, 3u,
                                      NULL, plain, plain_size,
                                      &plain_size) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK( (staged != NULL) &&
                      (staged_size == plain_size) &&
                      (memcmp(staged, plain, plain_size) == 0) );

    free(staged);
    free(plain);

    return;
}

/*
d_tests_ak_tar_gz_read
  Tests the following:
  - d_archive_extract_entry reads one member of a tar.gz, and measures it
  - a flipped byte in the stream is a formal failure, not a success
  - a tar.gz cut short is a formal failure
  - d_archive_bound bounds the tar.gz of an incompressible payload
*/
static void
d_tests_ak_tar_gz_read(void)
{
    unsigned char           noise[3000];
    unsigned char           one[64];
    struct dj_archive_entry e;
    struct dj_archive_layout layout;
    unsigned char*          gz;
    size_t                  size  = 0u;
    size_t                  got   = 0u;
    size_t                  bound = 0u;
    size_t                  i;
    enum dj_pack_status      status;

    d_tests_ak_open("tar_gz_read",
                    "tar.gz read with nothing staged, and its failures");

    gz = d_tests_ak_create(D_FORMAT_ID_TAR_GZ, d_tests_ak_list, 3u, NULL,
                           &size, &status);
    D_TESTS_AK_CHECK(gz != NULL);

    if (gz == NULL)
    {
        return;
    }

    D_TESTS_AK_CHECK( (d_archive_extract_entry(
                           D_FORMAT_ID_TAR_GZ, gz, size,
                           d_tests_ak_text("dir/inner"), NULL,
                           NULL, 0u, &got) == D_PACK_STATUS_OK) &&
                      (got == 3u) );
    D_TESTS_AK_CHECK( (d_archive_extract_entry(
                           D_FORMAT_ID_TAR_GZ, gz, size,
                           d_tests_ak_text("dir/inner"), NULL,
                           one, sizeof(one), &got) == D_PACK_STATUS_OK) &&
                      (memcmp(one, "xyz", 3) == 0) );

    gz[size / 2u] ^= 0x5Au;
    D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_TAR_GZ, gz, size, NULL,
                                       &layout) != D_PACK_STATUS_OK);
    gz[size / 2u] ^= 0x5Au;
    D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_TAR_GZ, gz, size - 9u,
                                       NULL, &layout) != D_PACK_STATUS_OK);
    free(gz);

    // the bound covers the stream around an incompressible tar
    for (i = 0; i < sizeof(noise); ++i)
    {
        noise[i] = (unsigned char)((i * 2654435761u) >> 13);
    }

    e  = d_tests_ak_file("noise.bin", noise, sizeof(noise));
    gz = d_tests_ak_create(D_FORMAT_ID_TAR_GZ, &e, 1u, NULL, &size, &status);
    D_TESTS_AK_CHECK( (d_archive_bound(D_FORMAT_ID_TAR_GZ, &e, 1u, NULL,
                                       &bound) == D_PACK_STATUS_OK) &&
                      (gz != NULL) &&
                      (size <= bound) );
    free(gz);

    return;
}


/*
d_tests_ak_staged
  The section: the staged create, and tar.gz's read path.
*/
void
d_tests_ak_staged(void)
{
    d_tests_ak_list_init();
    d_tests_ak_staged_macros();
    d_tests_ak_staged_regions();
    d_tests_ak_staged_zip();
    d_tests_ak_tar_gz_read();

    return;
}
