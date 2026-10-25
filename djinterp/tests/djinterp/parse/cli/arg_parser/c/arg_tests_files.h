/*******************************************************************************
* djinterp [parse]                                             arg_tests_files.h
*
* A scratch directory for the argument parser's suites: files and
* directories a section makes, by name, in a fresh directory that is the
* working directory while the section runs, and that is removed with
* everything in it when the section is done. It is C, and its calls have C
* linkage in C++, so that the C++ suites -- and the CLI11 programs their
* transcripts come from -- make their files with it too.
*   The suites that ask the file system -- the path validators, and the
* configuration and response files read from disk -- name what they made by
* relative paths, so that their transcripts are the same text on every
* system and in every run. Names are UTF-8 everywhere: on Windows they are
* widened for the system, as the parser's own system layer widens a path.
*   What a system cannot make is not declared there: symbolic links and
* FIFOs exist where the environment layer finds POSIX's file status
* (D_TESTS_ARG_FILES_POSIX), and a file's mode is the read-only attribute on
* Windows.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_tests_files.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.19
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RECORDS
    -------
    1.  The system
         1.  D_TESTS_ARG_FILES_POSIX
         2.  D_TESTS_ARG_FILES_WINDOWS
    2.  Limits
         1.  D_TESTS_ARG_FILES_MAX
    3.  The directory
         1.  d_tests_arg_files_entry
         2.  d_tests_arg_files
2.  OPERATIONS
    ----------
    1.  Entering and leaving
         1.  d_tests_arg_files_enter
         2.  d_tests_arg_files_leave
    2.  Making
         1.  d_tests_arg_files_folder
         2.  d_tests_arg_files_write / d_tests_arg_files_file
         3.  d_tests_arg_files_mode
         4.  d_tests_arg_files_link / d_tests_arg_files_fifo
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TESTS_FILES_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TESTS_FILES_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/env/parse/cli/arg_parser/env_arg_parser.h"  // D_ENV_ARG_PARSER_STAT


//==============================================================================
// 1.  RECORDS
//==============================================================================


// 1.1    The system
//------------------------------------------------------------------------------
// 1.1.1
// D_TESTS_ARG_FILES_POSIX
//   constant: 1 where the system asks POSIX for a file's status, and so has
// symbolic links, FIFOs and permission bits; 0 elsewhere.
#if (D_ENV_ARG_PARSER_STAT == D_ENV_ARG_PARSER_STAT_POSIX)
    #define D_TESTS_ARG_FILES_POSIX 1
#else
    #define D_TESTS_ARG_FILES_POSIX 0
#endif

// 1.1.2
// D_TESTS_ARG_FILES_WINDOWS
//   constant: 1 where the system asks the Microsoft runtime, and a file's
// mode is its read-only attribute; 0 elsewhere.
#if (D_ENV_ARG_PARSER_STAT == D_ENV_ARG_PARSER_STAT_WINDOWS)
    #define D_TESTS_ARG_FILES_WINDOWS 1
#else
    #define D_TESTS_ARG_FILES_WINDOWS 0
#endif

// 1.2    Limits
//------------------------------------------------------------------------------
// 1.2.1
// D_TESTS_ARG_FILES_MAX
//   constant: how many files and directories one scratch directory holds; a
// section that makes more is a section to split.
#define D_TESTS_ARG_FILES_MAX 32

// 1.3    The directory
//------------------------------------------------------------------------------
// 1.3.1
// d_tests_arg_files_entry
//   struct: one thing made in a scratch directory: its name relative to the
// directory, an owned copy, and whether it is a directory.
struct d_tests_arg_files_entry
{
    char* name;
    bool  folder;
};

// 1.3.2
// d_tests_arg_files
//   struct: a scratch directory: the working directory it replaced and its
// own path, each as the system spells a path (owned); what was made in it,
// in order, so that it is removed in the reverse order; and whether
// anything asked of it could not be done.
struct d_tests_arg_files
{
    void*                          home;
    void*                          path;
    struct d_tests_arg_files_entry entries[D_TESTS_ARG_FILES_MAX];
    size_t                         count;
    bool                           failed;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// Each making call takes a name relative to the scratch directory, in UTF-8,
// with `/` between its parts, and records what it made; each returns whether
// it was made, and a failure also sets `failed`, so that a section can make
// everything and look once.


D_EXTERN_C_BEGIN

// 2.1    Entering and leaving
//------------------------------------------------------------------------------
// 2.1.1
// d_tests_arg_files_enter
/**
 * @brief Makes a fresh scratch directory under the system's temporary
 *        directory and makes it the working directory.
 *
 * @param[out] _files  the scratch directory; whatever it held is ignored.
 * @return whether the directory was made and entered; on failure nothing is
 *         left behind, `_files` holds nothing, and the working directory is
 *         unchanged.
 */
bool d_tests_arg_files_enter(struct d_tests_arg_files* _files);

// 2.1.2
// d_tests_arg_files_leave
/**
 * @brief Returns to the working directory the scratch directory replaced,
 *        and removes the scratch directory with everything made in it.
 *
 * @param[in,out] _files  the scratch directory; one that was never entered
 *                        holds nothing, and is left as it is.
 * @post   `_files` holds nothing.
 */
void d_tests_arg_files_leave(struct d_tests_arg_files* _files);

// 2.2    Making
//------------------------------------------------------------------------------
// 2.2.1
// d_tests_arg_files_folder
/**
 * @brief Makes a directory.
 *
 * @param[in,out] _files  the scratch directory.
 * @param[in]     _name   the directory's name.
 * @return whether it was made.
 */
bool d_tests_arg_files_folder(struct d_tests_arg_files* _files,
                              const char*               _name);

// 2.2.2
// d_tests_arg_files_write / d_tests_arg_files_file
/**
 * @brief Makes a file holding the bytes given.
 *
 * @param[in,out] _files  the scratch directory.
 * @param[in]     _name   the file's name.
 * @param[in]     _data   the bytes; may be NULL when `_size` is 0.
 * @param[in]     _size   how many.
 * @return whether the file was made and holds every byte.
 */
bool d_tests_arg_files_write(struct d_tests_arg_files* _files,
                             const char*               _name,
                             const void*               _data,
                             size_t                    _size);
/**
 * @brief Makes a file holding a text, without its terminating NUL.
 *
 * @param[in,out] _files    the scratch directory.
 * @param[in]     _name     the file's name.
 * @param[in]     _content  the text.
 * @return whether the file was made and holds the text.
 */
bool d_tests_arg_files_file(struct d_tests_arg_files* _files,
                            const char*               _name,
                            const char*               _content);

// 2.2.3
// d_tests_arg_files_mode
/**
 * @brief Sets the permission bits of something made.
 *
 * @note On Windows only the owner's write bit counts: without it the file is
 *       read-only, and with it it is not.
 *
 * @param[in,out] _files  the scratch directory.
 * @param[in]     _name   what was made.
 * @param[in]     _mode   the bits, as octal 0755 is written.
 * @return whether they were set.
 */
bool d_tests_arg_files_mode(struct d_tests_arg_files* _files,
                            const char*               _name,
                            unsigned int              _mode);

#if (D_TESTS_ARG_FILES_POSIX == 1)

// 2.2.4
// d_tests_arg_files_link / d_tests_arg_files_fifo
/**
 * @brief Makes a symbolic link.
 *
 * @param[in,out] _files   the scratch directory.
 * @param[in]     _name    the link's name.
 * @param[in]     _target  what it points to, as written into the link.
 * @return whether it was made.
 */
bool d_tests_arg_files_link(struct d_tests_arg_files* _files,
                            const char*               _name,
                            const char*               _target);
/**
 * @brief Makes a FIFO.
 *
 * @param[in,out] _files  the scratch directory.
 * @param[in]     _name   the FIFO's name.
 * @return whether it was made.
 */
bool d_tests_arg_files_fifo(struct d_tests_arg_files* _files,
                            const char*               _name);

#endif  // D_TESTS_ARG_FILES_POSIX

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TESTS_FILES_H
