/*******************************************************************************
* djinterp [parse]                                          arg_validator_path.h
*
* Validators that ask the file system: CLI11's existing-file, -directory and
* -path checks, the nonexistent-path check, the file found on a default
* path, and the permission and size checks.
*   Each makes a `struct d_arg_validator` (arg_validator.h) that describes
* itself and rejects exactly as its CLI11 namesake does. A path is examined
* with one status query, which follows symbolic links as
* std::filesystem::status does: stat where the system has it, and on
* Windows the Microsoft runtime's status of the path read as UTF-8, as
* CLI11 reads it -- so the arguments d_arg_parser_ensure_utf8 recovers name
* their files -- or in the ANSI code page when it is not valid UTF-8. Every
* validator here reads a path so, the permission and size checks too, which
* CLI11 hands the narrow path to std::filesystem. A path is what the value
* holds up to a NUL, as CLI11's c_str() gives it. Anything that exists and
* is not a directory counts as a file, as in CLI11.
*   Like every such check, each answers for the moment it asks: the file
* can change before the program opens it, so a validator here improves an
* error message and decides nothing about security.
*   Everything here is compiled only with D_INTERNAL_ARG_PARSER_PATH_
* VALIDATORS; without it the header declares nothing.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_validator_path.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PATH VALIDATORS
    ---------------
    1.  Existence
         1.  d_arg_validator_existing_file
         2.  d_arg_validator_existing_directory
         3.  d_arg_validator_existing_path
         4.  d_arg_validator_nonexistent_path
    2.  Search
         1.  d_arg_validator_file_on_default_path
    3.  Permissions
         1.  d_arg_validator_read_permissions and kin
    4.  Size
         1.  d_arg_validator_file_size
         2.  d_arg_validator_non_empty_file
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_PATH_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_PATH_H 1

// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "./arg_common.h"                    // config knobs
#include "./arg_validator.h"                 // d_arg_validator
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint64_t, UINT64_MAX

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)


//==============================================================================
// 1.  PATH VALIDATORS
//==============================================================================
// None allocates but d_arg_validator_file_on_default_path, so none fails but
// for that one's copy of its path.


D_EXTERN_C_BEGIN

// 1.1    Existence
//------------------------------------------------------------------------------
// 1.1.1
// d_arg_validator_existing_file
//   constructor: CLI11's ExistingFile, described as "FILE": rejects an empty
// value with "File name is missing.", a path that does not exist with "File
// does not exist: <path>", and a directory with "File is actually a
// directory: <path>". Does not fail.
struct d_arg_validator d_arg_validator_existing_file(void);

// 1.1.2
// d_arg_validator_existing_directory
//   constructor: CLI11's ExistingDirectory, described as "DIR": rejects a
// path that does not exist with "Directory does not exist: <path>" and one
// that is not a directory with "Directory is actually a file: <path>". Does
// not fail.
struct d_arg_validator d_arg_validator_existing_directory(void);

// 1.1.3
// d_arg_validator_existing_path
//   constructor: CLI11's ExistingPath, described as "PATH(existing)": rejects
// a path that does not exist with "Path does not exist: <path>". Does not
// fail.
struct d_arg_validator d_arg_validator_existing_path(void);

// 1.1.4
// d_arg_validator_nonexistent_path
//   constructor: CLI11's NonexistentPath, described as "PATH(non-existing)":
// rejects a path that exists with "Path already exists: <path>". Does not
// fail.
struct d_arg_validator d_arg_validator_nonexistent_path(void);

// 1.2    Search
//------------------------------------------------------------------------------
// 1.2.1
// d_arg_validator_file_on_default_path
/**
 * @brief Makes CLI11's FileOnDefaultPath, a transformation that finds a file
 *        in a default directory.
 *
 * @note A value naming nothing that exists is tried again under the
 *       default path, joined with `/` unless the path already ends in `/`
 *       or `\`; when that names a file, it replaces the value. Otherwise
 *       the value is rejected with "File does not exist: <value>", or kept,
 *       as `_error_return` says. A value naming something that exists,
 *       directory or not, passes unchanged. Described as "FILE".
 *
 * @param[in] _default_path  the directory to search, which is copied.
 * @param[in] _error_return  whether a file found in neither place is
 *                           rejected (CLI11's enableErrorReturn).
 * @return the validator; on failure, a failed validator holding
 *         D_ARG_ERROR_OUT_OF_MEMORY, or D_ARG_ERROR_BAD_ARGUMENT for a NULL
 *         path.
 */
struct d_arg_validator
            d_arg_validator_file_on_default_path(const char* _default_path,
                                                 bool        _error_return);

// 1.3    Permissions
//------------------------------------------------------------------------------
// 1.3.1
// d_arg_validator_read_permissions and kin
//   constructors: CLI11's ReadPermissions, WritePermissions and
// ExecPermissions, described as "Path with read permission" and so on:
// each rejects a path that does not exist with "Path does not exist:
// <path>", and one whose permission bits grant the access to no one -- not
// the owner, the group or others -- with "Path does not have required
// permissions: <path>". The bits are the file's, not the caller's effective
// access, as in CLI11; on Windows read and execute are always granted and
// write unless the file is read-only, as Microsoft's std::filesystem
// reports them. None fails.
struct d_arg_validator d_arg_validator_read_permissions(void);
struct d_arg_validator d_arg_validator_write_permissions(void);
struct d_arg_validator d_arg_validator_exec_permissions(void);

// 1.4    Size
//------------------------------------------------------------------------------
#if defined(UINT64_MAX)

// 1.4.1
// d_arg_validator_file_size
/**
 * @brief Makes CLI11's FileSizeValidator: a file's size must lie between two
 *        limits.
 *
 * @note Rejects a path that does not exist with "File does not exist:
 *       <path>"; one that is not a regular file, which has no size to
 *       give, with "Error getting file size: Is a directory" for a
 *       directory and "Error getting file size: Operation not supported"
 *       for anything else, as libstdc++ and libc++ report it on every
 *       system; a smaller file with "File size <size> bytes is less than
 *       minimum <min> bytes", and a larger one with "File size <size> bytes
 *       exceeds maximum <max> bytes". Described as "File size at least <min>
 *       bytes", or "File size between <min> and <max> bytes" when there is
 *       a maximum. Declared where dstdint.h declares uint64_t, which is
 *       everywhere but ISO strict C++98 on a 32-bit target.
 *
 * @param[in] _minimum  the least size in bytes.
 * @param[in] _maximum  the greatest size in bytes; 0 for no maximum.
 * @return the validator. Does not fail.
 */
struct d_arg_validator d_arg_validator_file_size(uint64_t _minimum,
                                                 uint64_t _maximum);

#endif  // UINT64_MAX

// 1.4.2
// d_arg_validator_non_empty_file
//   constructor: CLI11's NonEmptyFile, the file-size validator with a minimum
// of one byte and no maximum, described as "File size at least 1 bytes".
// Does not fail.
struct d_arg_validator d_arg_validator_non_empty_file(void);

D_EXTERN_C_END


#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_PATH_H
