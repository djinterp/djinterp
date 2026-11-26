/*******************************************************************************
* djinterp [c][desc]                                     desc_of_filesys_tests.c
*
*   The suite of desc's describers of filesys's types
* (desc/of/filesys_desc.h), in DTest's C face: each type against its
* describer's record of it; then a file's status, a directory entry and a
* filesys notice, each in facts and in each ring. Expectations follow the
* knobs a build has.
*
*
* path:      /tests/djinterp/c/desc/desc_of_filesys_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#include "./desc_of_tests.h"  // the describers' tests' helpers

// std
#include <string.h>  // memcpy, memset
// djinterp
#include "../../../../inc/djinterp/c/desc/of/filesys_desc.h"  // describers


//==============================================================================
// THE TESTS
//==============================================================================

/*
d_tests_desc_of_filesys_layout
  Checks the following:
  - each type against its record, and the describers' build against this
    program's, as d_tests_of_layout does
*/
static void
d_tests_desc_of_filesys_layout(
    struct dj_test_suite* _suite
)
{
    const struct dj_desc_of_layout* rows  = NULL;
    size_t count = d_desc_of_filesys_layout_here(&rows);
    size_t index = count;
    int    agree = d_desc_of_filesys_agrees(&index);

    d_tests_of_layout(_suite, "filesys", D_INTERNAL_DESC_OF_FILESYS, rows,
                      count, agree, index);

    return;
}

#if D_INTERNAL_DESC_OF_FILESYS

/*
d_tests_desc_of_filesys_status
  Checks the following:
  - a status's kind and permissions come from its mode; a change or
    creation time of 0 is none
  - a regular file's size is its line; an empty one says so; a
    directory's line is its kind alone
  - D_DESC_OF(dj_stat_t) is the default width's description
*/
static void
d_tests_desc_of_filesys_status(
    struct dj_test_suite* _suite
)
{
    struct dj_stat32_t      status;
    struct dj_tests_of_keep keep;
    char                    text[320];

    (void)d_test_suite_test(_suite, "a file's status");
    memset(&status, 0, sizeof(status));
    status.st_size     = 1234u;
    status.st_mode     = 0100644u;
    status.st_nlink    = 1u;
    status.st_uid      = 1000u;
    status.st_gid      = 100u;
    status.st_modified = 1700000000;
    status.st_dev      = 2049u;
    status.st_ino      = 42u;
    D_TESTS_FACT(_suite, d_tests_of_facts(D_DESC_OF(dj_stat32_t), &status,
                                          &keep) == 0);
    D_TESTS_FACT(_suite, d_tests_of_text(&keep, "kind", "regular") &&
                         d_tests_of_is(&keep, "size", 0u, 1234u) &&
                         d_tests_of_text(&keep, "owner", "rw-") &&
                         d_tests_of_text(&keep, "group", "r--") &&
                         d_tests_of_text(&keep, "other", "r--"));
    D_TESTS_FACT(_suite, d_tests_of_is(&keep, "modified", 0u, 1700000000u) &&
                         d_tests_of_text(&keep, "changed", NULL) &&
                         d_tests_of_text(&keep, "created", NULL) &&
                         d_tests_of_is(&keep, "inode", 0u, 42u));
#if (D_FILE_WIDTH == 64)
    D_TEST_CHECK(_suite, D_DESC_OF(dj_stat_t) == &d_desc_type_dj_stat64_t);
#else
    D_TEST_CHECK(_suite, D_DESC_OF(dj_stat_t) == &d_desc_type_dj_stat32_t);
#endif

#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_stat32_t), &status, D_DESC_RING_NAME,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "the name", text, "regular file");
    (void)d_tests_of_say(D_DESC_OF(dj_stat32_t), &status,
                         D_DESC_RING_DETAILED, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "the detail", text,
                    D_TESTS_OF_A "regular file with 1234 bytes. Its "
                    "permissions are rw-r--r--. It belongs to user 1000 and "
                    "group 100. It has 1 hard link. Its inode is 42 on "
                    "device 2049.");
    status.st_size = 0u;
    (void)d_tests_of_say(D_DESC_OF(dj_stat32_t), &status, D_DESC_RING_LINE,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "an empty file", text,
                    D_TESTS_OF_AN "empty regular file");
    status.st_mode  = 040755u;
    status.st_nlink = 3u;
    (void)d_tests_of_say(D_DESC_OF(dj_stat32_t), &status,
                         D_DESC_RING_SENTENCE, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a directory", text,
                    D_TESTS_OF_A "directory. Its permissions are "
                    "rwxr-xr-x.");
#else
    (void)text;
#endif

    return;
}

#if D_FILE_64_IS_AVAILABLE
/*
d_tests_desc_of_filesys_wide
  Checks the following:
  - a 64-bit status keeps a size and a time past 32 bits, and a time
    before 1970, signed
*/
static void
d_tests_desc_of_filesys_wide(
    struct dj_test_suite* _suite
)
{
    struct dj_stat64_t      status;
    struct dj_tests_of_keep keep;
    const struct dj_desc_fact* fact;

    (void)d_test_suite_test(_suite, "a 64-bit file's status");
    memset(&status, 0, sizeof(status));
    status.st_size     = 5000000000u;
    status.st_mode     = 0100600u;
    status.st_modified = 4102444800;   // 2100-01-01
    status.st_accessed = -86400;       // 1969-12-31
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_stat64_t), &status,
                                           &keep) == 0) &&
                         d_tests_of_is(&keep, "size", 0u, 5000000000u) &&
                         d_tests_of_is(&keep, "modified", 0u, 4102444800u) &&
                         d_tests_of_text(&keep, "group", "---"));
    fact = d_tests_of_find(&keep, "accessed", 0u);
    D_TESTS_FACT(_suite, (fact != NULL) && (fact->value.negative == 1) &&
                         (fact->value.low == 86400u));

    return;
}
#endif  // D_FILE_64_IS_AVAILABLE

/*
d_tests_desc_of_filesys_entry
  Checks the following:
  - an entry's name, kind and inode; an empty name is none
  - its prose: its name at the line, its kind at the sentence, and an
    unknown kind said as such
*/
static void
d_tests_desc_of_filesys_entry(
    struct dj_test_suite* _suite
)
{
    struct dj_dirent32_t    entry;
    struct dj_tests_of_keep keep;
    char                    text[256];

    (void)d_test_suite_test(_suite, "a directory entry");
    memset(&entry, 0, sizeof(entry));
    memcpy(entry.d_name, "src", sizeof("src"));
    entry.d_type = (uint8_t)DT_DIR;
    entry.d_ino  = 7u;
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_dirent32_t), &entry,
                                           &keep) == 0) &&
                         d_tests_of_text(&keep, "name", "src") &&
                         d_tests_of_text(&keep, "kind", "directory") &&
                         d_tests_of_is(&keep, "inode", 0u, 7u));
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_dirent32_t), &entry,
                         D_DESC_RING_DETAILED, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "the detail", text,
                    D_TESTS_OF_A "directory entry named src. It names a "
                    "directory. Its inode is 7.");
#endif

    entry.d_name[0] = '\0';
    entry.d_type    = (uint8_t)DT_UNKNOWN;
    entry.d_ino     = 0u;
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_dirent32_t), &entry,
                                           &keep) == 0) &&
                         d_tests_of_text(&keep, "name", NULL) &&
                         d_tests_of_text(&keep, "kind", "unknown"));
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_dirent32_t), &entry,
                         D_DESC_RING_DETAILED, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "an unknown kind", text,
                    D_TESTS_OF_A "directory entry. Its kind is not known.");
#else
    (void)text;
#endif

    return;
}

/*
d_tests_desc_of_filesys_notice
  Checks the following:
  - a notice's level is its noun; where it came from is its line; what it
    says, its path and its error code are its sentence
  - a level filesys does not have is none, and the noun plain
*/
static void
d_tests_desc_of_filesys_notice(
    struct dj_test_suite* _suite
)
{
    struct dj_file_notice   notice;
    struct dj_tests_of_keep keep;
    char                    text[256];

    (void)d_test_suite_test(_suite, "a filesys notice");
    notice.level    = D_FILE_NOTIFY_ERROR;
    notice.error    = 2;
    notice.function = "d_file_open_stream";
    notice.path     = "/tmp/x";
    notice.message  = "open failed";
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_file_notice), &notice,
                                           &keep) == 0) &&
                         d_tests_of_text(&keep, "level", "error") &&
                         d_tests_of_is(&keep, "error", 0u, 2u) &&
                         d_tests_of_text(&keep, "path", "/tmp/x"));
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_file_notice), &notice,
                         D_DESC_RING_SENTENCE, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "an error", text,
                    D_TESTS_OF_A "filesys error from d_file_open_stream. It "
                    "says \"open failed\". Its path is /tmp/x. Its error "
                    "code is 2.");
    notice.level = D_FILE_NOTIFY_WARN;
    notice.error = 0;
    notice.path  = NULL;
    (void)d_tests_of_say(D_DESC_OF(dj_file_notice), &notice,
                         D_DESC_RING_SENTENCE, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a warning", text,
                    D_TESTS_OF_A "filesys warning from d_file_open_stream. "
                    "It says \"open failed\".");
#endif

    notice.level = 9;
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_file_notice), &notice,
                                           &keep) == 0) &&
                         d_tests_of_text(&keep, "level", "none"));
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_file_notice), &notice,
                         D_DESC_RING_NAME, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "no level", text, "filesys notice");
#else
    (void)text;
#endif

    return;
}

/*
d_tests_desc_of_filesys_declare
  Checks the following:
  - each type's name is an identifier, so it declares to desc
*/
static void
d_tests_desc_of_filesys_declare(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "each type declares by its name");
    D_TEST_CHECK(_suite,
                 (d_desc_declare(D_DESC_OF(dj_stat32_t)) == 0)    &&
                 (d_desc_declare(D_DESC_OF(dj_dirent32_t)) == 0)  &&
#if D_FILE_64_IS_AVAILABLE
                 (d_desc_declare(D_DESC_OF(dj_stat64_t)) == 0)    &&
                 (d_desc_declare(D_DESC_OF(dj_dirent64_t)) == 0)  &&
#endif
                 (d_desc_declare(D_DESC_OF(dj_file_notice)) == 0));

    return;
}

#endif  // D_INTERNAL_DESC_OF_FILESYS


/*
d_tests_desc_of_filesys
  The suite's fifth section: the block of desc/of/filesys_desc.h, in the
module d_tests_desc opened.
*/
void
d_tests_desc_of_filesys(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_block(_suite, "desc_of_filesys");

    d_tests_desc_of_filesys_layout(_suite);
#if D_INTERNAL_DESC_OF_FILESYS
    d_tests_desc_of_filesys_status(_suite);
#if D_FILE_64_IS_AVAILABLE
    d_tests_desc_of_filesys_wide(_suite);
#endif
    d_tests_desc_of_filesys_entry(_suite);
    d_tests_desc_of_filesys_notice(_suite);
    d_tests_desc_of_filesys_declare(_suite);
#endif

    return;
}
