/*******************************************************************************
* djinterp [test]                                         archive_kernel_tests.h
*
* The archive kernel's suite in DTest's C face (c/modules/test/test_suite.h).
*   One entry point builds the module "archive_kernel": the staged create
* (tar.gz and ZIP through a caller's stage), tar.gz read with nothing staged,
* the pax extended headers, the tar reader's extensions and damage, and the
* ZIP reader's checks.  The section files share the helpers declared here.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
* 2.  SHARED HELPERS
* 3.  SECTIONS
*
*
* path:      /tests/djinterp/c/util/archive/archive_kernel_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_UTIL_ARCHIVE_ARCHIVE_KERNEL_TESTS_H
#define DJINTERP_TESTS_C_UTIL_ARCHIVE_ARCHIVE_KERNEL_TESTS_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
#include "../../../../../inc/djinterp/c/util/archive/archive.h"
                                            // the kernel and its macros


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "archive_kernel" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_archive_kernel_all(struct dj_test_suite* _suite);


//==============================================================================
// 2.  SHARED HELPERS
//==============================================================================

// D_TESTS_AK_CHECK
//   macro: one check: counts it, and prints the expression, file and line on
// failure.
#define D_TESTS_AK_CHECK(expr)                                                 \
    d_tests_ak_check(((expr) ? 1 : 0), #expr, __FILE__, __LINE__)

void d_tests_ak_check(int _passed, const char* _expression,
                      const char* _file, int _line);
void d_tests_ak_open(const char* _name, const char* _descriptor);
struct dj_pack_text d_tests_ak_text(const char* _s);
struct dj_archive_entry d_tests_ak_file(const char* _name, const void* _data,
                                        size_t _size);
void d_tests_ak_tar_header(unsigned char* _block, const char* _name,
                           size_t _size, char _type);
unsigned char* d_tests_ak_create(enum dj_format_id _format,
                                 const struct dj_archive_entry* _entries,
                                 size_t _count,
                                 const struct dj_archive_options* _options,
                                 size_t* _out_size,
                                 enum dj_pack_status* _out_status);
int d_tests_ak_read(enum dj_format_id _format, const void* _in,
                    size_t _in_size, struct dj_archive_entry** _out_entries,
                    struct dj_archive_layout* _out_layout,
                    enum dj_pack_status* _out_status);
int d_tests_ak_has(const struct dj_archive_entry* _entries, size_t _count,
                   const char* _name, size_t _name_len, const void* _data,
                   size_t _size);


//==============================================================================
// 3.  SECTIONS
//==============================================================================

void d_tests_ak_staged(void);
void d_tests_ak_pax(void);
void d_tests_ak_reader(void);
void d_tests_ak_zip(void);


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_UTIL_ARCHIVE_ARCHIVE_KERNEL_TESTS_H
