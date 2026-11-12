/*******************************************************************************
* djinterp [test]                                         archive_kernel_tests.c
*
*   The archive kernel's suite: the module "archive_kernel", its helpers, and
* the entry point that runs the section files (archive_kernel_tests_*.c).
* Each section is a group of tests; each check is recorded against the open
* test.  Buffers come from malloc: the kernel allocates nothing, and a test is
* a caller like any other.
*
*
* path:      /tests/djinterp/c/util/archive/archive_kernel_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "archive_kernel_tests.h"  // the suite's entry point and helpers
// std
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcmp, memcpy, memset, strlen


// d_tests_ak_suite
//   the suite being built, for the checks; set by d_tests_archive_kernel_all.
static struct dj_test_suite* d_tests_ak_suite;


/*
d_tests_ak_check
  Records one check against the open test.
*/
void
d_tests_ak_check(
    int         _passed,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    (void)d_test_suite_check(d_tests_ak_suite,
                             (_passed != 0),
                             _expression,
                             _file,
                             _line);

    return;
}

/*
d_tests_ak_open
  Opens a group's test, with its descriptor.
*/
void
d_tests_ak_open(
    const char* _name,
    const char* _descriptor
)
{
    (void)d_test_suite_test(d_tests_ak_suite, _name);
    (void)d_test_suite_describe(d_tests_ak_suite, _descriptor);

    return;
}

/*
d_tests_ak_text
  A span over a NUL-terminated string.
*/
struct dj_pack_text
d_tests_ak_text(
    const char* _s
)
{
    struct dj_pack_text t;

    t.data   = _s;
    t.length = strlen(_s);

    return t;
}

/*
d_tests_ak_file
  A file member with a fixed timestamp, so archives are comparable.
*/
struct dj_archive_entry
d_tests_ak_file(
    const char* _name,
    const void* _data,
    size_t      _size
)
{
    struct dj_archive_entry e;

    memset(&e, 0, sizeof(e));
    e.name      = d_tests_ak_text(_name);
    e.data.data = _data;
    e.data.size = _size;
    e.mtime     = 1700000000;

    return e;
}

/*
d_tests_ak_tar_header
  A valid ustar header, built by hand, for archives the kernel's writer
would not make: GNU long names, global headers, damage.  Mode 0644, owner 0,
mtime 0; the checksum is the unsigned sum with its field as spaces.
*/
void
d_tests_ak_tar_header(
    unsigned char* _block,
    const char*    _name,
    size_t         _size,
    char           _type
)
{
    unsigned long sum = 0u;
    size_t        i;

    memset(_block, 0, 512);
    memcpy(_block, _name, strlen(_name));
    memcpy(_block + 100, "0000644", 7);
    memcpy(_block + 108, "0000000", 7);
    memcpy(_block + 116, "0000000", 7);

    for (i = 0; i < 11u; ++i)
    {
        _block[124 + 10 - i] =
            (unsigned char)('0' + ((_size >> (3u * i)) & 7u));
    }

    memcpy(_block + 136, "00000000000", 11);
    _block[156] = (unsigned char)_type;
    memcpy(_block + 257, "ustar", 5);
    memcpy(_block + 263, "00", 2);
    memset(_block + 148, ' ', 8);

    for (i = 0; i < 512u; ++i)
    {
        sum += _block[i];
    }

    for (i = 0; i < 6u; ++i)
    {
        _block[148 + 5 - i] = (unsigned char)('0' + ((sum >> (3u * i)) & 7u));
    }

    _block[154] = 0u;

    return;
}

/*
d_tests_ak_create
  Create a container through the staged call, with a stage of the size
d_archive_stage_size reports: measure, then write.  The result is malloc'd;
null on failure, with the status in *_out_status.
*/
unsigned char*
d_tests_ak_create(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                           _count,
    const struct dj_archive_options* _options,
    size_t*                          _out_size,
    enum dj_pack_status*             _out_status
)
{
    unsigned char* stage = NULL;
    unsigned char* out   = NULL;
    size_t         need  = 0u;
    size_t         size  = 0u;

    *_out_status = d_archive_stage_size(_format, _entries, _count, _options,
                                        &need);

    if ( (*_out_status == D_PACK_STATUS_OK) &&
         (need != 0u) )
    {
        stage = (unsigned char*)malloc(need);
    }
    if (*_out_status == D_PACK_STATUS_OK)
    {
        *_out_status = d_archive_create_staged(_format, _entries, _count,
                                               _options, stage, need,
                                               NULL, 0u, &size);
    }
    if (*_out_status == D_PACK_STATUS_OK)
    {
        out          = (unsigned char*)malloc(size ? size : 1u);
        *_out_status = d_archive_create_staged(_format, _entries, _count,
                                               _options, stage, need,
                                               out, size, &size);
    }

    free(stage);

    if (*_out_status != D_PACK_STATUS_OK)
    {
        free(out);

        return NULL;
    }

    *_out_size = size;

    return out;
}

/*
d_tests_ak_read
  Measure and extract a container.  The entries and the arena they point
into are one malloc'd block, freed by freeing *_out_entries.  Returns 1 when
the extract succeeded.
*/
int
d_tests_ak_read(
    enum dj_format_id         _format,
    const void*               _in,
    size_t                    _in_size,
    struct dj_archive_entry** _out_entries,
    struct dj_archive_layout* _out_layout,
    enum dj_pack_status*      _out_status
)
{
    struct dj_archive_entry* entries;
    size_t                   head;

    *_out_entries = NULL;
    *_out_status  = d_archive_measure(_format, _in, _in_size, NULL,
                                      _out_layout);

    if (*_out_status != D_PACK_STATUS_OK)
    {
        return 0;
    }

    head    = (_out_layout->entry_count + 1u) * sizeof(*entries);
    entries = (struct dj_archive_entry*)malloc(head +
                                               _out_layout->arena_size + 1u);
    *_out_status = d_archive_extract(_format, _in, _in_size, NULL,
                                     entries, _out_layout->entry_count,
                                     (unsigned char*)entries + head,
                                     _out_layout->arena_size, _out_layout);

    if (*_out_status != D_PACK_STATUS_OK)
    {
        free(entries);

        return 0;
    }

    *_out_entries = entries;

    return 1;
}

/*
d_tests_ak_has
  Whether a member of that name holds exactly those bytes.
*/
int
d_tests_ak_has(
    const struct dj_archive_entry* _entries,
    size_t                         _count,
    const char*                    _name,
    size_t                         _name_len,
    const void*                    _data,
    size_t                         _size
)
{
    size_t i;

    for (i = 0; i < _count; ++i)
    {
        if ( (_entries[i].name.length == _name_len) &&
             (memcmp(_entries[i].name.data, _name, _name_len) == 0) )
        {
            return (_entries[i].data.size == _size) &&
                   ( (_size == 0u) ||
                     (memcmp(_entries[i].data.data, _data, _size) == 0) );
        }
    }

    return 0;
}


/*
d_tests_archive_kernel_all
  Opens the module and its block, then runs every section.
*/
void
d_tests_archive_kernel_all(
    struct dj_test_suite* _suite
)
{
    d_tests_ak_suite = _suite;

    (void)d_test_suite_module(_suite, "archive_kernel");
    (void)d_test_suite_describe(_suite,
                                "the archive kernel: staged create, tar.gz "
                                "read, pax, the readers' checks");
    (void)d_test_suite_block(_suite, "archive_common.h, archive.h");

    d_tests_ak_staged();
    d_tests_ak_pax();
    d_tests_ak_reader();
    d_tests_ak_zip();

    d_tests_ak_suite = NULL;

    return;
}
