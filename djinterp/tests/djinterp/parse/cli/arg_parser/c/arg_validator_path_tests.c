/*******************************************************************************
* djinterp [parse]                                    arg_validator_path_tests.c
*
* The suite of the validators that ask the file system, declared in
* arg_validator_path_tests.h.
*   Each section makes, in a scratch directory it then works in, the tree
* CLI11's validators were run over (probes/xval/setup.sh): files empty and
* not, a directory and a file in it, an executable, a read-only file, a
* file whose name is UTF-8, and, on POSIX, symbolic links -- to a file, to
* the directory, to nothing -- a FIFO and a file of mode 000. The names are
* relative, so every transcript is the same text in every run.
*   The transcripts are CLI11 2.7.2's: on POSIX, built with libstdc++ on
* Linux; on Windows, built with llvm-mingw's libc++ and run under Wine. The
* names whose answers are the same on both systems share one transcript;
* the execute permission, which Windows grants to every file, and the names
* only one system has, have one for each. A UTF-8 name departs from CLI11
* on Windows, as its constant says: here every validator reads it as the
* file's name, on every system (ISSUES 26).
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_path_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_validator_path_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knob
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // D_ARG_BUILTIN_STRING
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // running, copies
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_path.h"  // the path validators
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_files.h"  // the scratch directory
#include "./arg_tests_support.h"  // transcripts, parses, kinds


#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// PORTABLE
//   constant: the names the validators are run over on every system, as
// CLI11's were: each kind of thing in the tree, a name nothing has, the
// empty name, a name found only under the default path, a directory with a
// final separator, and a doubled separator.
static const char* const PORTABLE[] =
{
    "tree/file0", "tree/file10", "tree/dir", "tree/exec", "tree/ro",
    "tree/missing", "", "inner.txt", "tree/dir/inner.txt", "tree/dir/",
    "tree//file10", "missing.txt"
};

// PORTABLE_COUNT
//   constant: how many names PORTABLE holds.
static const size_t PORTABLE_COUNT = sizeof(PORTABLE) / sizeof(PORTABLE[0]);

// POSIX_NAMES
//   constant: the names only POSIX has -- the links, the FIFO, the file of
// mode 000, the null device -- as CLI11's validators were run over them,
// then names with a backslash, which POSIX reads as part of the name.
static const char* const POSIX_NAMES[] =
{
    "tree/link_file", "tree/link_dir", "tree/broken", "tree/fifo",
    "tree/noperm", "/dev/null", "tree\\file10", "tree\\dir",
    "tree\\dir\\inner.txt", "tree\\dir\\", "tree\\missing"
};

// WINDOWS_NAMES
//   constant: the names whose answer is Windows' own: the same names with a
// backslash, which separates there, and names in another case, which
// Windows matches without regard to it.
static const char* const WINDOWS_NAMES[] =
{
    "tree\\file10", "tree\\dir", "tree\\dir\\inner.txt", "tree\\dir\\",
    "tree\\missing", "TREE/FILE10", "tree/DIR", "Tree/Dir/Inner.TXT"
};

// UTF8_NAMES
//   constant: the name of the tree's file whose name is UTF-8, "été.txt".
static const char* const UTF8_NAMES[] =
{
    "tree/\xc3\xa9t\xc3\xa9.txt"
};

// E_FILE
//   constant: CLI11's ExistingFile over PORTABLE.
static const char E_FILE[] =
    "desc <FILE>\n"
    "[tree/file0] -> pass <tree/file0>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> fail <File is actually a directory: tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> fail <File does not exist: tree/missing>\n"
    "[] -> fail <File name is missing.>\n"
    "[inner.txt] -> fail <File does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> fail <File is actually a directory: tree/dir/>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";

// E_DIR
//   constant: CLI11's ExistingDirectory over PORTABLE.
static const char E_DIR[] =
    "desc <DIR>\n"
    "[tree/file0] -> fail <Directory is actually a file: tree/file0>\n"
    "[tree/file10] -> fail <Directory is actually a file: tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> fail <Directory is actually a file: tree/exec>\n"
    "[tree/ro] -> fail <Directory is actually a file: tree/ro>\n"
    "[tree/missing] -> fail <Directory does not exist: tree/missing>\n"
    "[] -> fail <Directory does not exist: >\n"
    "[inner.txt] -> fail <Directory does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> fail <Directory is actually a file: "
    "tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> fail <Directory is actually a file: tree//file10>\n"
    "[missing.txt] -> fail <Directory does not exist: missing.txt>\n";

// E_PATH
//   constant: CLI11's ExistingPath over PORTABLE.
static const char E_PATH[] =
    "desc <PATH(existing)>\n"
    "[tree/file0] -> pass <tree/file0>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> fail <Path does not exist: tree/missing>\n"
    "[] -> fail <Path does not exist: >\n"
    "[inner.txt] -> fail <Path does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n";

// E_NONE
//   constant: CLI11's NonexistentPath over PORTABLE.
static const char E_NONE[] =
    "desc <PATH(non-existing)>\n"
    "[tree/file0] -> fail <Path already exists: tree/file0>\n"
    "[tree/file10] -> fail <Path already exists: tree/file10>\n"
    "[tree/dir] -> fail <Path already exists: tree/dir>\n"
    "[tree/exec] -> fail <Path already exists: tree/exec>\n"
    "[tree/ro] -> fail <Path already exists: tree/ro>\n"
    "[tree/missing] -> pass <tree/missing>\n"
    "[] -> pass <>\n"
    "[inner.txt] -> pass <inner.txt>\n"
    "[tree/dir/inner.txt] -> fail <Path already exists: tree/dir/inner.txt>\n"
    "[tree/dir/] -> fail <Path already exists: tree/dir/>\n"
    "[tree//file10] -> fail <Path already exists: tree//file10>\n"
    "[missing.txt] -> pass <missing.txt>\n";

// S_DEF
//   constant: CLI11's FileOnDefaultPath("tree/dir") over PORTABLE.
static const char S_DEF[] =
    "desc <FILE>\n"
    "[tree/file0] -> pass <tree/file0>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> fail <File does not exist: tree/missing>\n"
    "[] -> fail <File does not exist: >\n"
    "[inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";

// S_DEF2
//   constant: CLI11's FileOnDefaultPath("tree/dir/", false) over PORTABLE.
static const char S_DEF2[] =
    "desc <FILE>\n"
    "[tree/file0] -> pass <tree/file0>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> pass <tree/missing>\n"
    "[] -> pass <>\n"
    "[inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> pass <missing.txt>\n";

// S_DEF3
//   constant: CLI11's FileOnDefaultPath("") over PORTABLE.
static const char S_DEF3[] =
    "desc <FILE>\n"
    "[tree/file0] -> pass <tree/file0>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> fail <File does not exist: tree/missing>\n"
    "[] -> fail <File does not exist: >\n"
    "[inner.txt] -> fail <File does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";

// S_DEF4
//   constant: CLI11's FileOnDefaultPath("tree\\") over PORTABLE.
static const char S_DEF4[] =
    "desc <FILE>\n"
    "[tree/file0] -> pass <tree/file0>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> fail <File does not exist: tree/missing>\n"
    "[] -> fail <File does not exist: >\n"
    "[inner.txt] -> fail <File does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";

// R_READ
//   constant: CLI11's ReadPermissions over PORTABLE.
static const char R_READ[] =
    "desc <Path with read permission>\n"
    "[tree/file0] -> pass <tree/file0>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> fail <Path does not exist: tree/missing>\n"
    "[] -> fail <Path does not exist: >\n"
    "[inner.txt] -> fail <Path does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n";

// R_WRITE
//   constant: CLI11's WritePermissions over PORTABLE.
static const char R_WRITE[] =
    "desc <Path with write permission>\n"
    "[tree/file0] -> pass <tree/file0>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> fail <Path does not have required permissions: tree/ro>\n"
    "[tree/missing] -> fail <Path does not exist: tree/missing>\n"
    "[] -> fail <Path does not exist: >\n"
    "[inner.txt] -> fail <Path does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n";

// R_EXEC
//   constant: CLI11's ExecPermissions over PORTABLE, on POSIX.
static const char R_EXEC[] =
    "desc <Path with exec permission>\n"
    "[tree/file0] -> fail <Path does not have required permissions: "
    "tree/file0>\n"
    "[tree/file10] -> fail <Path does not have required permissions: "
    "tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> fail <Path does not have required permissions: tree/ro>\n"
    "[tree/missing] -> fail <Path does not exist: tree/missing>\n"
    "[] -> fail <Path does not exist: >\n"
    "[inner.txt] -> fail <Path does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> fail <Path does not have required permissions: "
    "tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> fail <Path does not have required permissions: "
    "tree//file10>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n";

// R_EXEC_WINDOWS
//   constant: CLI11's ExecPermissions over PORTABLE, on Windows, which
// grants execution to everything that exists.
static const char R_EXEC_WINDOWS[] =
    "desc <Path with exec permission>\n"
    "[tree/file0] -> pass <tree/file0>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> pass <tree/dir>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> fail <Path does not exist: tree/missing>\n"
    "[] -> fail <Path does not exist: >\n"
    "[inner.txt] -> fail <Path does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> pass <tree/dir/>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n";

// Z_SIZE
//   constant: CLI11's FileSizeValidator(5) over PORTABLE.
static const char Z_SIZE[] =
    "desc <File size at least 5 bytes>\n"
    "[tree/file0] -> fail <File size 0 bytes is less than minimum 5 bytes>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> fail <Error getting file size: Is a directory>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> fail <File size 2 bytes is less than minimum 5 bytes>\n"
    "[tree/missing] -> fail <File does not exist: tree/missing>\n"
    "[] -> fail <File does not exist: >\n"
    "[inner.txt] -> fail <File does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> fail <File size 1 bytes is less than minimum 5 "
    "bytes>\n"
    "[tree/dir/] -> fail <Error getting file size: Is a directory>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";

// Z_SIZE2
//   constant: CLI11's FileSizeValidator(1, 9) over PORTABLE.
static const char Z_SIZE2[] =
    "desc <File size between 1 and 9 bytes>\n"
    "[tree/file0] -> fail <File size 0 bytes is less than minimum 1 bytes>\n"
    "[tree/file10] -> fail <File size 10 bytes exceeds maximum 9 bytes>\n"
    "[tree/dir] -> fail <Error getting file size: Is a directory>\n"
    "[tree/exec] -> fail <File size 10 bytes exceeds maximum 9 bytes>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> fail <File does not exist: tree/missing>\n"
    "[] -> fail <File does not exist: >\n"
    "[inner.txt] -> fail <File does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> fail <Error getting file size: Is a directory>\n"
    "[tree//file10] -> fail <File size 10 bytes exceeds maximum 9 bytes>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";

// Z_NONEMPTY
//   constant: CLI11's NonEmptyFile over PORTABLE.
static const char Z_NONEMPTY[] =
    "desc <File size at least 1 bytes>\n"
    "[tree/file0] -> fail <File size 0 bytes is less than minimum 1 bytes>\n"
    "[tree/file10] -> pass <tree/file10>\n"
    "[tree/dir] -> fail <Error getting file size: Is a directory>\n"
    "[tree/exec] -> pass <tree/exec>\n"
    "[tree/ro] -> pass <tree/ro>\n"
    "[tree/missing] -> fail <File does not exist: tree/missing>\n"
    "[] -> fail <File does not exist: >\n"
    "[inner.txt] -> fail <File does not exist: inner.txt>\n"
    "[tree/dir/inner.txt] -> pass <tree/dir/inner.txt>\n"
    "[tree/dir/] -> fail <Error getting file size: Is a directory>\n"
    "[tree//file10] -> pass <tree//file10>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";

// POSIX_FILE
//   constant: CLI11's ExistingFile over POSIX_NAMES, on POSIX.
static const char POSIX_FILE[] =
    "desc <FILE>\n"
    "[tree/link_file] -> pass <tree/link_file>\n"
    "[tree/link_dir] -> fail <File is actually a directory: tree/link_dir>\n"
    "[tree/broken] -> fail <File does not exist: tree/broken>\n"
    "[tree/fifo] -> pass <tree/fifo>\n"
    "[tree/noperm] -> pass <tree/noperm>\n"
    "[/dev/null] -> pass </dev/null>\n"
    "[tree\\file10] -> fail <File does not exist: tree\\file10>\n"
    "[tree\\dir] -> fail <File does not exist: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <File does not exist: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <File does not exist: tree\\dir\\>\n"
    "[tree\\missing] -> fail <File does not exist: tree\\missing>\n";

// POSIX_DIR
//   constant: CLI11's ExistingDirectory over POSIX_NAMES, on POSIX.
static const char POSIX_DIR[] =
    "desc <DIR>\n"
    "[tree/link_file] -> fail <Directory is actually a file: tree/link_file>\n"
    "[tree/link_dir] -> pass <tree/link_dir>\n"
    "[tree/broken] -> fail <Directory does not exist: tree/broken>\n"
    "[tree/fifo] -> fail <Directory is actually a file: tree/fifo>\n"
    "[tree/noperm] -> fail <Directory is actually a file: tree/noperm>\n"
    "[/dev/null] -> fail <Directory is actually a file: /dev/null>\n"
    "[tree\\file10] -> fail <Directory does not exist: tree\\file10>\n"
    "[tree\\dir] -> fail <Directory does not exist: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <Directory does not exist: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <Directory does not exist: tree\\dir\\>\n"
    "[tree\\missing] -> fail <Directory does not exist: tree\\missing>\n";

// POSIX_PATH
//   constant: CLI11's ExistingPath over POSIX_NAMES, on POSIX.
static const char POSIX_PATH[] =
    "desc <PATH(existing)>\n"
    "[tree/link_file] -> pass <tree/link_file>\n"
    "[tree/link_dir] -> pass <tree/link_dir>\n"
    "[tree/broken] -> fail <Path does not exist: tree/broken>\n"
    "[tree/fifo] -> pass <tree/fifo>\n"
    "[tree/noperm] -> pass <tree/noperm>\n"
    "[/dev/null] -> pass </dev/null>\n"
    "[tree\\file10] -> fail <Path does not exist: tree\\file10>\n"
    "[tree\\dir] -> fail <Path does not exist: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <Path does not exist: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <Path does not exist: tree\\dir\\>\n"
    "[tree\\missing] -> fail <Path does not exist: tree\\missing>\n";

// POSIX_NONE
//   constant: CLI11's NonexistentPath over POSIX_NAMES, on POSIX.
static const char POSIX_NONE[] =
    "desc <PATH(non-existing)>\n"
    "[tree/link_file] -> fail <Path already exists: tree/link_file>\n"
    "[tree/link_dir] -> fail <Path already exists: tree/link_dir>\n"
    "[tree/broken] -> pass <tree/broken>\n"
    "[tree/fifo] -> fail <Path already exists: tree/fifo>\n"
    "[tree/noperm] -> fail <Path already exists: tree/noperm>\n"
    "[/dev/null] -> fail <Path already exists: /dev/null>\n"
    "[tree\\file10] -> pass <tree\\file10>\n"
    "[tree\\dir] -> pass <tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> pass <tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> pass <tree\\dir\\>\n"
    "[tree\\missing] -> pass <tree\\missing>\n";

// POSIX_DEF
//   constant: CLI11's FileOnDefaultPath("tree/dir") over POSIX_NAMES, on POSIX.
static const char POSIX_DEF[] =
    "desc <FILE>\n"
    "[tree/link_file] -> pass <tree/link_file>\n"
    "[tree/link_dir] -> pass <tree/link_dir>\n"
    "[tree/broken] -> fail <File does not exist: tree/broken>\n"
    "[tree/fifo] -> pass <tree/fifo>\n"
    "[tree/noperm] -> pass <tree/noperm>\n"
    "[/dev/null] -> pass </dev/null>\n"
    "[tree\\file10] -> fail <File does not exist: tree\\file10>\n"
    "[tree\\dir] -> fail <File does not exist: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <File does not exist: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <File does not exist: tree\\dir\\>\n"
    "[tree\\missing] -> fail <File does not exist: tree\\missing>\n";

// POSIX_READ
//   constant: CLI11's ReadPermissions over POSIX_NAMES, on POSIX.
static const char POSIX_READ[] =
    "desc <Path with read permission>\n"
    "[tree/link_file] -> pass <tree/link_file>\n"
    "[tree/link_dir] -> pass <tree/link_dir>\n"
    "[tree/broken] -> fail <Path does not exist: tree/broken>\n"
    "[tree/fifo] -> pass <tree/fifo>\n"
    "[tree/noperm] -> fail <Path does not have required permissions: "
    "tree/noperm>\n"
    "[/dev/null] -> pass </dev/null>\n"
    "[tree\\file10] -> fail <Path does not exist: tree\\file10>\n"
    "[tree\\dir] -> fail <Path does not exist: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <Path does not exist: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <Path does not exist: tree\\dir\\>\n"
    "[tree\\missing] -> fail <Path does not exist: tree\\missing>\n";

// POSIX_WRITE
//   constant: CLI11's WritePermissions over POSIX_NAMES, on POSIX.
static const char POSIX_WRITE[] =
    "desc <Path with write permission>\n"
    "[tree/link_file] -> pass <tree/link_file>\n"
    "[tree/link_dir] -> pass <tree/link_dir>\n"
    "[tree/broken] -> fail <Path does not exist: tree/broken>\n"
    "[tree/fifo] -> pass <tree/fifo>\n"
    "[tree/noperm] -> fail <Path does not have required permissions: "
    "tree/noperm>\n"
    "[/dev/null] -> pass </dev/null>\n"
    "[tree\\file10] -> fail <Path does not exist: tree\\file10>\n"
    "[tree\\dir] -> fail <Path does not exist: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <Path does not exist: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <Path does not exist: tree\\dir\\>\n"
    "[tree\\missing] -> fail <Path does not exist: tree\\missing>\n";

// POSIX_EXEC
//   constant: CLI11's ExecPermissions over POSIX_NAMES, on POSIX.
static const char POSIX_EXEC[] =
    "desc <Path with exec permission>\n"
    "[tree/link_file] -> fail <Path does not have required permissions: "
    "tree/link_file>\n"
    "[tree/link_dir] -> pass <tree/link_dir>\n"
    "[tree/broken] -> fail <Path does not exist: tree/broken>\n"
    "[tree/fifo] -> fail <Path does not have required permissions: tree/fifo>\n"
    "[tree/noperm] -> fail <Path does not have required permissions: "
    "tree/noperm>\n"
    "[/dev/null] -> fail <Path does not have required permissions: /dev/null>\n"
    "[tree\\file10] -> fail <Path does not exist: tree\\file10>\n"
    "[tree\\dir] -> fail <Path does not exist: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <Path does not exist: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <Path does not exist: tree\\dir\\>\n"
    "[tree\\missing] -> fail <Path does not exist: tree\\missing>\n";

// POSIX_SIZE
//   constant: CLI11's FileSizeValidator(5) over POSIX_NAMES, on POSIX.
static const char POSIX_SIZE[] =
    "desc <File size at least 5 bytes>\n"
    "[tree/link_file] -> pass <tree/link_file>\n"
    "[tree/link_dir] -> fail <Error getting file size: Is a directory>\n"
    "[tree/broken] -> fail <File does not exist: tree/broken>\n"
    "[tree/fifo] -> fail <Error getting file size: Operation not supported>\n"
    "[tree/noperm] -> pass <tree/noperm>\n"
    "[/dev/null] -> fail <Error getting file size: Operation not supported>\n"
    "[tree\\file10] -> fail <File does not exist: tree\\file10>\n"
    "[tree\\dir] -> fail <File does not exist: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <File does not exist: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <File does not exist: tree\\dir\\>\n"
    "[tree\\missing] -> fail <File does not exist: tree\\missing>\n";

// WINDOWS_FILE
//   constant: CLI11's ExistingFile over WINDOWS_NAMES, on Windows.
static const char WINDOWS_FILE[] =
    "desc <FILE>\n"
    "[tree\\file10] -> pass <tree\\file10>\n"
    "[tree\\dir] -> fail <File is actually a directory: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> pass <tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <File is actually a directory: tree\\dir\\>\n"
    "[tree\\missing] -> fail <File does not exist: tree\\missing>\n"
    "[TREE/FILE10] -> pass <TREE/FILE10>\n"
    "[tree/DIR] -> fail <File is actually a directory: tree/DIR>\n"
    "[Tree/Dir/Inner.TXT] -> pass <Tree/Dir/Inner.TXT>\n";

// WINDOWS_DIR
//   constant: CLI11's ExistingDirectory over WINDOWS_NAMES, on Windows.
static const char WINDOWS_DIR[] =
    "desc <DIR>\n"
    "[tree\\file10] -> fail <Directory is actually a file: tree\\file10>\n"
    "[tree\\dir] -> pass <tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <Directory is actually a file: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> pass <tree\\dir\\>\n"
    "[tree\\missing] -> fail <Directory does not exist: tree\\missing>\n"
    "[TREE/FILE10] -> fail <Directory is actually a file: TREE/FILE10>\n"
    "[tree/DIR] -> pass <tree/DIR>\n"
    "[Tree/Dir/Inner.TXT] -> fail <Directory is actually a file: "
    "Tree/Dir/Inner.TXT>\n";

// WINDOWS_PATH
//   constant: CLI11's ExistingPath over WINDOWS_NAMES, on Windows.
static const char WINDOWS_PATH[] =
    "desc <PATH(existing)>\n"
    "[tree\\file10] -> pass <tree\\file10>\n"
    "[tree\\dir] -> pass <tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> pass <tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> pass <tree\\dir\\>\n"
    "[tree\\missing] -> fail <Path does not exist: tree\\missing>\n"
    "[TREE/FILE10] -> pass <TREE/FILE10>\n"
    "[tree/DIR] -> pass <tree/DIR>\n"
    "[Tree/Dir/Inner.TXT] -> pass <Tree/Dir/Inner.TXT>\n";

// WINDOWS_NONE
//   constant: CLI11's NonexistentPath over WINDOWS_NAMES, on Windows.
static const char WINDOWS_NONE[] =
    "desc <PATH(non-existing)>\n"
    "[tree\\file10] -> fail <Path already exists: tree\\file10>\n"
    "[tree\\dir] -> fail <Path already exists: tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> fail <Path already exists: "
    "tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> fail <Path already exists: tree\\dir\\>\n"
    "[tree\\missing] -> pass <tree\\missing>\n"
    "[TREE/FILE10] -> fail <Path already exists: TREE/FILE10>\n"
    "[tree/DIR] -> fail <Path already exists: tree/DIR>\n"
    "[Tree/Dir/Inner.TXT] -> fail <Path already exists: Tree/Dir/Inner.TXT>\n";

// WINDOWS_DEF
//   constant: CLI11's FileOnDefaultPath("tree/dir") over WINDOWS_NAMES, on Windows.
static const char WINDOWS_DEF[] =
    "desc <FILE>\n"
    "[tree\\file10] -> pass <tree\\file10>\n"
    "[tree\\dir] -> pass <tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> pass <tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> pass <tree\\dir\\>\n"
    "[tree\\missing] -> fail <File does not exist: tree\\missing>\n"
    "[TREE/FILE10] -> pass <TREE/FILE10>\n"
    "[tree/DIR] -> pass <tree/DIR>\n"
    "[Tree/Dir/Inner.TXT] -> pass <Tree/Dir/Inner.TXT>\n";

// WINDOWS_READ
//   constant: CLI11's ReadPermissions over WINDOWS_NAMES, on Windows.
static const char WINDOWS_READ[] =
    "desc <Path with read permission>\n"
    "[tree\\file10] -> pass <tree\\file10>\n"
    "[tree\\dir] -> pass <tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> pass <tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> pass <tree\\dir\\>\n"
    "[tree\\missing] -> fail <Path does not exist: tree\\missing>\n"
    "[TREE/FILE10] -> pass <TREE/FILE10>\n"
    "[tree/DIR] -> pass <tree/DIR>\n"
    "[Tree/Dir/Inner.TXT] -> pass <Tree/Dir/Inner.TXT>\n";

// WINDOWS_WRITE
//   constant: CLI11's WritePermissions over WINDOWS_NAMES, on Windows.
static const char WINDOWS_WRITE[] =
    "desc <Path with write permission>\n"
    "[tree\\file10] -> pass <tree\\file10>\n"
    "[tree\\dir] -> pass <tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> pass <tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> pass <tree\\dir\\>\n"
    "[tree\\missing] -> fail <Path does not exist: tree\\missing>\n"
    "[TREE/FILE10] -> pass <TREE/FILE10>\n"
    "[tree/DIR] -> pass <tree/DIR>\n"
    "[Tree/Dir/Inner.TXT] -> pass <Tree/Dir/Inner.TXT>\n";

// WINDOWS_EXEC
//   constant: CLI11's ExecPermissions over WINDOWS_NAMES, on Windows.
static const char WINDOWS_EXEC[] =
    "desc <Path with exec permission>\n"
    "[tree\\file10] -> pass <tree\\file10>\n"
    "[tree\\dir] -> pass <tree\\dir>\n"
    "[tree\\dir\\inner.txt] -> pass <tree\\dir\\inner.txt>\n"
    "[tree\\dir\\] -> pass <tree\\dir\\>\n"
    "[tree\\missing] -> fail <Path does not exist: tree\\missing>\n"
    "[TREE/FILE10] -> pass <TREE/FILE10>\n"
    "[tree/DIR] -> pass <tree/DIR>\n"
    "[Tree/Dir/Inner.TXT] -> pass <Tree/Dir/Inner.TXT>\n";

// WINDOWS_SIZE
//   constant: CLI11's FileSizeValidator(5) over WINDOWS_NAMES, on Windows.
static const char WINDOWS_SIZE[] =
    "desc <File size at least 5 bytes>\n"
    "[tree\\file10] -> pass <tree\\file10>\n"
    "[tree\\dir] -> fail <Error getting file size: Is a directory>\n"
    "[tree\\dir\\inner.txt] -> fail <File size 1 bytes is less than minimum 5 "
    "bytes>\n"
    "[tree\\dir\\] -> fail <Error getting file size: Is a directory>\n"
    "[tree\\missing] -> fail <File does not exist: tree\\missing>\n"
    "[TREE/FILE10] -> pass <TREE/FILE10>\n"
    "[tree/DIR] -> fail <Error getting file size: Is a directory>\n"
    "[Tree/Dir/Inner.TXT] -> fail <File size 1 bytes is less than minimum 5 "
    "bytes>\n";

// NAMED_FILE
//   constant: CLI11's ExistingFile over UTF8_NAMES, on POSIX.
static const char NAMED_FILE[] =
    "desc <FILE>\n"
    "[tree/\xc3\xa9t\xc3\xa9.txt] -> pass <tree/\xc3\xa9t\xc3\xa9.txt>\n";

// NAMED_DIR
//   constant: CLI11's ExistingDirectory over UTF8_NAMES, on POSIX.
static const char NAMED_DIR[] =
    "desc <DIR>\n"
    "[tree/\xc3\xa9t\xc3\xa9.txt] -> fail <Directory is actually a file: "
    "tree/\xc3\xa9t\xc3\xa9.txt>\n";

// NAMED_PATH
//   constant: CLI11's ExistingPath over UTF8_NAMES, on POSIX.
static const char NAMED_PATH[] =
    "desc <PATH(existing)>\n"
    "[tree/\xc3\xa9t\xc3\xa9.txt] -> pass <tree/\xc3\xa9t\xc3\xa9.txt>\n";

// NAMED_NONE
//   constant: CLI11's NonexistentPath over UTF8_NAMES, on POSIX.
static const char NAMED_NONE[] =
    "desc <PATH(non-existing)>\n"
    "[tree/\xc3\xa9t\xc3\xa9.txt] -> fail <Path already exists: "
    "tree/\xc3\xa9t\xc3\xa9.txt>\n";

// NAMED_DEF
//   constant: CLI11's FileOnDefaultPath("tree/dir") over UTF8_NAMES, on POSIX.
static const char NAMED_DEF[] =
    "desc <FILE>\n"
    "[tree/\xc3\xa9t\xc3\xa9.txt] -> pass <tree/\xc3\xa9t\xc3\xa9.txt>\n";

// NAMED_READ
//   constant: CLI11's ReadPermissions over UTF8_NAMES, on POSIX.
static const char NAMED_READ[] =
    "desc <Path with read permission>\n"
    "[tree/\xc3\xa9t\xc3\xa9.txt] -> pass <tree/\xc3\xa9t\xc3\xa9.txt>\n";

// NAMED_WRITE
//   constant: CLI11's WritePermissions over UTF8_NAMES, on POSIX.
static const char NAMED_WRITE[] =
    "desc <Path with write permission>\n"
    "[tree/\xc3\xa9t\xc3\xa9.txt] -> pass <tree/\xc3\xa9t\xc3\xa9.txt>\n";

// NAMED_EXEC
//   constant: CLI11's ExecPermissions over UTF8_NAMES, on POSIX.
static const char NAMED_EXEC[] =
    "desc <Path with exec permission>\n"
    "[tree/\xc3\xa9t\xc3\xa9.txt] -> pass <tree/\xc3\xa9t\xc3\xa9.txt>\n";

// NAMED_SIZE
//   constant: CLI11's FileSizeValidator(5) over UTF8_NAMES, on POSIX.
static const char NAMED_SIZE[] =
    "desc <File size at least 5 bytes>\n"
    "[tree/\xc3\xa9t\xc3\xa9.txt] -> pass <tree/\xc3\xa9t\xc3\xa9.txt>\n";

// d_tests_arg_path_expected
//   struct: one validator's transcript expected over a set of names, and
// the name a mismatch is printed under.
struct d_tests_arg_path_expected
{
    const char* name;
    const char* text;
};

// SYSTEM_POSIX
//   constant: the transcripts over POSIX_NAMES, in the order
// d_tests_arg_path_every runs the validators: a link followed to what it
// names, a broken one nothing, a FIFO and a device files without a size, a
// file of mode 000 without any permission, and a backslash part of a name.
static const struct d_tests_arg_path_expected SYSTEM_POSIX[] =
{
    { "POSIX_FILE",  POSIX_FILE  },
    { "POSIX_DIR",   POSIX_DIR   },
    { "POSIX_PATH",  POSIX_PATH  },
    { "POSIX_NONE",  POSIX_NONE  },
    { "POSIX_DEF",   POSIX_DEF   },
    { "POSIX_READ",  POSIX_READ  },
    { "POSIX_WRITE", POSIX_WRITE },
    { "POSIX_EXEC",  POSIX_EXEC  },
    { "POSIX_SIZE",  POSIX_SIZE  }
};

// SYSTEM_WINDOWS
//   constant: the transcripts over WINDOWS_NAMES, in the same order, as
// CLI11 for Windows answers: a backslash separates, and case is ignored.
static const struct d_tests_arg_path_expected SYSTEM_WINDOWS[] =
{
    { "WINDOWS_FILE",  WINDOWS_FILE  },
    { "WINDOWS_DIR",   WINDOWS_DIR   },
    { "WINDOWS_PATH",  WINDOWS_PATH  },
    { "WINDOWS_NONE",  WINDOWS_NONE  },
    { "WINDOWS_DEF",   WINDOWS_DEF   },
    { "WINDOWS_READ",  WINDOWS_READ  },
    { "WINDOWS_WRITE", WINDOWS_WRITE },
    { "WINDOWS_EXEC",  WINDOWS_EXEC  },
    { "WINDOWS_SIZE",  WINDOWS_SIZE  }
};

// NAMED
//   constant: the transcripts over UTF8_NAMES, in the same order, as CLI11
// answers on POSIX, and as every validator here answers on every system.
// CLI11 built for Windows finds the file in its existence checks, which
// widen the name from UTF-8, but not in its permission and size checks,
// which hand std::filesystem the name narrow, to be read in the ANSI code
// page: there those four say "Path does not exist: tree/été.txt" and "File
// does not exist: tree/été.txt" (ISSUES 26).
static const struct d_tests_arg_path_expected NAMED[] =
{
    { "NAMED_FILE",  NAMED_FILE  },
    { "NAMED_DIR",   NAMED_DIR   },
    { "NAMED_PATH",  NAMED_PATH  },
    { "NAMED_NONE",  NAMED_NONE  },
    { "NAMED_DEF",   NAMED_DEF   },
    { "NAMED_READ",  NAMED_READ  },
    { "NAMED_WRITE", NAMED_WRITE },
    { "NAMED_EXEC",  NAMED_EXEC  },
    { "NAMED_SIZE",  NAMED_SIZE  }
};

// P_FILE_DIR
//   constant: CLI11's `--f tree/dir`, the option bound to text and checked
// by ExistingFile.
static const char P_FILE_DIR[] =
    "ERR ValidationError 105: --f: File is actually a directory: tree/dir\n"
    "cmd app n=1 miss=[] subs=[] order=[--f]\n"
    "opt --f n=1 res=[tree/dir]\n"
    "value []\n";

// P_FILE
//   constant: CLI11's `--f tree/file10`, to the same option.
static const char P_FILE[] =
    "cmd app n=1 miss=[] subs=[] order=[--f]\n"
    "opt --f n=1 res=[tree/file10]\n"
    "value [tree/file10]\n";

// P_DEFAULT
//   constant: CLI11's `--d inner.txt`, the option bound to text and
// transformed by FileOnDefaultPath("tree/dir").
static const char P_DEFAULT[] =
    "cmd app n=1 miss=[] subs=[] order=[--d]\n"
    "opt --d n=1 res=[tree/dir/inner.txt]\n"
    "value [tree/dir/inner.txt]\n";

// P_DEFAULT_MISSING
//   constant: CLI11's `--d nope.txt`, to the same option.
static const char P_DEFAULT_MISSING[] =
    "ERR ValidationError 105: --d: File does not exist: nope.txt\n"
    "cmd app n=1 miss=[] subs=[] order=[--d]\n"
    "opt --d n=1 res=[nope.txt]\n"
    "value []\n";

// P_DIRECTORY
//   constant: CLI11's `--o tree/dir`, the option bound to text and checked
// by ExistingDirectory.
static const char P_DIRECTORY[] =
    "cmd app n=1 miss=[] subs=[] order=[--o]\n"
    "opt --o n=1 res=[tree/dir]\n"
    "value [tree/dir]\n";

// P_DIRECTORY_FILE
//   constant: CLI11's `--o tree/file10`, to the same option.
static const char P_DIRECTORY_FILE[] =
    "ERR ValidationError 105: --o: Directory is actually a file: tree/file10\n"
    "cmd app n=1 miss=[] subs=[] order=[--o]\n"
    "opt --o n=1 res=[tree/file10]\n"
    "value []\n";

// FILE_OPTION
//   constant: CLI11's `add_option("--f", text)`, checked.
static const struct d_tests_arg_checked FILE_OPTION =
{
    "--f",
    D_ARG_BUILTIN_STRING,
    false
};

// DEFAULT_OPTION
//   constant: CLI11's `add_option("--d", text)`, transformed.
static const struct d_tests_arg_checked DEFAULT_OPTION =
{
    "--d",
    D_ARG_BUILTIN_STRING,
    true
};

// DIRECTORY_OPTION
//   constant: CLI11's `add_option("--o", text)`, checked.
static const struct d_tests_arg_checked DIRECTORY_OPTION =
{
    "--o",
    D_ARG_BUILTIN_STRING,
    false
};

#if (D_TESTS_ARG_FILES_POSIX == 1)

/**
 * @brief Makes the part of the tree only POSIX has: links to a file, to a
 *        directory and to nothing, a FIFO, and a file of mode 000.
 *
 * @param[in,out] _files  the scratch directory, which holds the rest of the
 *                        tree.
 * @return whether everything was made.
 */
static bool
d_tests_arg_path_posix_tree(
    struct d_tests_arg_files* _files
)
{
    (void)d_tests_arg_files_link(_files,
                                 "tree/link_file",
                                 "file10");
    (void)d_tests_arg_files_link(_files,
                                 "tree/link_dir",
                                 "dir");
    (void)d_tests_arg_files_link(_files,
                                 "tree/broken",
                                 "nowhere");
    (void)d_tests_arg_files_fifo(_files,
                                 "tree/fifo");
    (void)d_tests_arg_files_file(_files,
                                 "tree/noperm",
                                 "12345");
    (void)d_tests_arg_files_mode(_files,
                                 "tree/noperm",
                                 0000u);

    return !_files->failed;
}

#else

/**
 * @brief Makes nothing: the system has no part of the tree only POSIX has.
 *
 * @param[in,out] _files  the scratch directory.
 * @return whether the rest of the tree was made.
 */
static bool
d_tests_arg_path_posix_tree(
    struct d_tests_arg_files* _files
)
{
    return !_files->failed;
}

#endif  // D_TESTS_ARG_FILES_POSIX

/**
 * @brief Enters a scratch directory and makes the tree in it.
 *
 * @param[out] _files  the scratch directory, to be left with
 *                     d_tests_arg_files_leave whatever this returns.
 * @return whether the whole tree was made.
 *
 * The modes are set whatever the process's umask would have made them, so
 * that every run asks about the same bits.
 */
static bool
d_tests_arg_path_enter(
    struct d_tests_arg_files* _files
)
{
    static const char UTF8_FILE[] = "tree/\xc3\xa9t\xc3\xa9.txt";

    // a directory to work in
    if (!d_tests_arg_files_enter(_files))
    {
        return false;
    }

    (void)d_tests_arg_files_folder(_files,
                                   "tree");
    (void)d_tests_arg_files_folder(_files,
                                   "tree/dir");
    (void)d_tests_arg_files_file(_files,
                                 "tree/file0",
                                 "");
    (void)d_tests_arg_files_file(_files,
                                 "tree/file10",
                                 "0123456789");
    (void)d_tests_arg_files_file(_files,
                                 "tree/dir/inner.txt",
                                 "x");
    (void)d_tests_arg_files_file(_files,
                                 "tree/exec",
                                 "#!/bin/sh\n");
    (void)d_tests_arg_files_file(_files,
                                 "tree/ro",
                                 "ro");
    (void)d_tests_arg_files_file(_files,
                                 UTF8_FILE,
                                 "abcdef");
    (void)d_tests_arg_files_mode(_files,
                                 "tree/file0",
                                 0644u);
    (void)d_tests_arg_files_mode(_files,
                                 "tree/file10",
                                 0644u);
    (void)d_tests_arg_files_mode(_files,
                                 "tree/dir/inner.txt",
                                 0644u);
    (void)d_tests_arg_files_mode(_files,
                                 "tree/exec",
                                 0755u);
    (void)d_tests_arg_files_mode(_files,
                                 "tree/ro",
                                 0444u);
    (void)d_tests_arg_files_mode(_files,
                                 UTF8_FILE,
                                 0755u);

    return d_tests_arg_path_posix_tree(_files);
}

/**
 * @brief Runs a validator over PORTABLE and compares its transcript with
 *        the one expected.
 *
 * @param[in] _what       what the transcript is, for a printed mismatch.
 * @param[in] _validator  the validator, consumed.
 * @param[in] _expected   the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_path_portable(
    const char*            _what,
    struct d_arg_validator _validator,
    const char*            _expected
)
{
    return d_tests_arg_holds(_what,
                             _validator,
                             PORTABLE,
                             PORTABLE_COUNT,
                             _expected);
}

/**
 * @brief Runs every validator over names, in turn, and compares each
 *        transcript with the one expected of it.
 *
 * @param[in] _names     the names.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcripts expected, one for each validator,
 *                       in the order the validators run.
 * @return whether every transcript was as expected.
 *
 * The validators are ExistingFile, ExistingDirectory, ExistingPath,
 * NonexistentPath, FileOnDefaultPath("tree/dir"), the three permission
 * checks and FileSizeValidator(5), as the reference ran them; each is
 * compared on its own, so that every mismatch is printed.
 */
static bool
d_tests_arg_path_every(
    const char* const*                      _names,
    size_t                                  _count,
    const struct d_tests_arg_path_expected* _expected
)
{
    struct d_arg_validator validators[] =
    {
        d_arg_validator_existing_file(),
        d_arg_validator_existing_directory(),
        d_arg_validator_existing_path(),
        d_arg_validator_nonexistent_path(),
        d_arg_validator_file_on_default_path("tree/dir",
                                             true),
        d_arg_validator_read_permissions(),
        d_arg_validator_write_permissions(),
        d_arg_validator_exec_permissions(),
        d_arg_validator_file_size(5u,
                                  0u)
    };
    const size_t           count      = sizeof(validators) /
                                        sizeof(validators[0]);
    bool                   all        = true;

    // each validator against its own transcript
    for (size_t index = 0u; index < count; ++index)
    {
        all = ( (d_tests_arg_holds(_expected[index].name,
                                   validators[index],
                                   _names,
                                   _count,
                                   _expected[index].text)) &&
                (all) );
    }

    return all;
}

/**
 * @brief Whether a path validator completes as the kind expected.
 *
 * @param[in] _validator  the validator, consumed.
 * @param[in] _kind       the kind: D_ARG_VALIDATOR_PATH, alone or with
 *                        D_ARG_VALIDATOR_DIRECTORY.
 * @return whether its completion flags are the kind.
 */
static bool
d_tests_arg_path_kind(
    struct d_arg_validator _validator,
    unsigned int           _kind
)
{
    return (d_tests_arg_kind_of(_validator) == _kind);
}

/**
 * @brief Whether running a validator over a value stops with the status
 *        expected.
 *
 * @param[in] _validator  the validator, consumed.
 * @param[in] _value      the value.
 * @param[in] _status     the status expected.
 * @return whether d_arg_validator_run returned it.
 */
static bool
d_tests_arg_path_runs_as(
    struct d_arg_validator _validator,
    const char*            _value,
    int                    _status
)
{
    struct d_arg_validation validation =
    {
        { NULL, 0u },
        D_ARG_BUFFER_INIT,
        D_ARG_BUFFER_INIT,
        NULL,
        0
    };

    d_arg_validation_init(&validation,
                          d_pack_text_from_cstr(_value));

    const int status = d_arg_validator_run(&_validator,
                                           &validation);

    d_arg_validation_release(&validation);
    d_arg_validator_release(&_validator);

    return (status == _status);
}

/**
 * @brief Whether a copy of FileOnDefaultPath("tree/dir") judges as the
 *        original once the original is gone.
 *
 * @return whether the copy was made and its transcript over PORTABLE is
 *         S_DEF.
 */
static bool
d_tests_arg_path_copied(void)
{
    struct d_arg_validator       original =
        d_arg_validator_file_on_default_path("tree/dir",
                                             true);
    const struct d_arg_validator copy     =
        d_arg_validator_copy(&original);
    const bool                   made     = (copy.status == D_ARG_OK);

    d_arg_validator_release(&original);

    return ( (d_tests_arg_path_portable("S_DEF",
                                        copy,
                                        S_DEF)) &&
             (made) );
}

/**
 * @brief Runs checks in a scratch directory holding the tree, which is
 *        removed afterwards whatever they found.
 *
 * @param[in] _checks  the checks, run with the tree as the working
 *                     directory's.
 * @return whether the tree was made and the checks held.
 */
static bool
d_tests_arg_path_in_tree(
    bool (*_checks)(void)
)
{
    struct d_tests_arg_files files = { 0 };
    const bool               held  = ( (d_tests_arg_path_enter(&files)) &&
                                       (_checks()) );

    d_tests_arg_files_leave(&files);

    return held;
}

/**
 * @brief The existence checks over PORTABLE.
 *
 * @return whether each transcript was CLI11's.
 */
static bool
d_tests_arg_path_check_existence(void)
{
    const bool file   = d_tests_arg_path_portable(
                            "E_FILE",
                            d_arg_validator_existing_file(),
                            E_FILE);
    const bool folder = d_tests_arg_path_portable(
                            "E_DIR",
                            d_arg_validator_existing_directory(),
                            E_DIR);
    const bool path   = d_tests_arg_path_portable(
                            "E_PATH",
                            d_arg_validator_existing_path(),
                            E_PATH);
    const bool none   = d_tests_arg_path_portable(
                            "E_NONE",
                            d_arg_validator_nonexistent_path(),
                            E_NONE);

    return ( (file)   &&
             (folder) &&
             (path)   &&
             (none) );
}

/**
 * @brief FileOnDefaultPath under its four default paths, over PORTABLE.
 *
 * @return whether each transcript was CLI11's.
 */
static bool
d_tests_arg_path_check_search(void)
{
    const bool plain = d_tests_arg_path_portable(
                           "S_DEF",
                           d_arg_validator_file_on_default_path("tree/dir",
                                                                true),
                           S_DEF);
    const bool quiet = d_tests_arg_path_portable(
                           "S_DEF2",
                           d_arg_validator_file_on_default_path("tree/dir/",
                                                                false),
                           S_DEF2);
    const bool empty = d_tests_arg_path_portable(
                           "S_DEF3",
                           d_arg_validator_file_on_default_path("",
                                                                true),
                           S_DEF3);
    const bool back  = d_tests_arg_path_portable(
                           "S_DEF4",
                           d_arg_validator_file_on_default_path("tree\\",
                                                                true),
                           S_DEF4);

    return ( (plain) &&
             (quiet) &&
             (empty) &&
             (back) );
}

/**
 * @brief The permission checks over PORTABLE, execution as the system
 *        grants it.
 *
 * @return whether each transcript was CLI11's on this system.
 */
static bool
d_tests_arg_path_check_permissions(void)
{
    const char* const exec  = (D_TESTS_ARG_FILES_WINDOWS == 1)
                              ? R_EXEC_WINDOWS
                              : R_EXEC;
    const bool        read  = d_tests_arg_path_portable(
                                  "R_READ",
                                  d_arg_validator_read_permissions(),
                                  R_READ);
    const bool        write = d_tests_arg_path_portable(
                                  "R_WRITE",
                                  d_arg_validator_write_permissions(),
                                  R_WRITE);
    const bool        run   = d_tests_arg_path_portable(
                                  "R_EXEC",
                                  d_arg_validator_exec_permissions(),
                                  exec);

    return ( (read)  &&
             (write) &&
             (run) );
}

/**
 * @brief The size checks over PORTABLE.
 *
 * @return whether each transcript was CLI11's.
 */
static bool
d_tests_arg_path_check_size(void)
{
    const bool least   = d_tests_arg_path_portable(
                             "Z_SIZE",
                             d_arg_validator_file_size(5u,
                                                       0u),
                             Z_SIZE);
    const bool between = d_tests_arg_path_portable(
                             "Z_SIZE2",
                             d_arg_validator_file_size(1u,
                                                       9u),
                             Z_SIZE2);
    const bool filled  = d_tests_arg_path_portable(
                             "Z_NONEMPTY",
                             d_arg_validator_non_empty_file(),
                             Z_NONEMPTY);

    return ( (least)   &&
             (between) &&
             (filled) );
}

/**
 * @brief Every validator over the names whose answer is this system's own.
 *
 * @return whether the transcript was CLI11's on this system.
 */
static bool
d_tests_arg_path_check_system(void)
{
    const bool               windows  = (D_TESTS_ARG_FILES_WINDOWS == 1);
    const char* const* const names    = (windows)
                                        ? WINDOWS_NAMES
                                        : POSIX_NAMES;
    const size_t             count    = (windows)
                                        ? (sizeof(WINDOWS_NAMES) /
                                           sizeof(WINDOWS_NAMES[0]))
                                        : (sizeof(POSIX_NAMES) /
                                           sizeof(POSIX_NAMES[0]));
    const struct d_tests_arg_path_expected* const expected =
        (windows)
        ? SYSTEM_WINDOWS
        : SYSTEM_POSIX;

    return d_tests_arg_path_every(names,
                                  count,
                                  expected);
}

/**
 * @brief Every validator over the name that is UTF-8.
 *
 * @return whether the transcript was the one every system gives.
 */
static bool
d_tests_arg_path_check_utf8(void)
{
    return d_tests_arg_path_every(UTF8_NAMES,
                                  sizeof(UTF8_NAMES) / sizeof(UTF8_NAMES[0]),
                                  NAMED);
}

/**
 * @brief The checks on options in a parse.
 *
 * @return whether each transcript was CLI11's.
 */
static bool
d_tests_arg_path_check_parses(void)
{
    const bool dir     = d_tests_arg_validates(
                             &FILE_OPTION,
                             d_arg_validator_existing_file(),
                             "tree/dir",
                             P_FILE_DIR);
    const bool file    = d_tests_arg_validates(
                             &FILE_OPTION,
                             d_arg_validator_existing_file(),
                             "tree/file10",
                             P_FILE);
    const bool found   = d_tests_arg_validates(
                             &DEFAULT_OPTION,
                             d_arg_validator_file_on_default_path("tree/dir",
                                                                  true),
                             "inner.txt",
                             P_DEFAULT);
    const bool missing = d_tests_arg_validates(
                             &DEFAULT_OPTION,
                             d_arg_validator_file_on_default_path("tree/dir",
                                                                  true),
                             "nope.txt",
                             P_DEFAULT_MISSING);
    const bool folder  = d_tests_arg_validates(
                             &DIRECTORY_OPTION,
                             d_arg_validator_existing_directory(),
                             "tree/dir",
                             P_DIRECTORY);
    const bool other   = d_tests_arg_validates(
                             &DIRECTORY_OPTION,
                             d_arg_validator_existing_directory(),
                             "tree/file10",
                             P_DIRECTORY_FILE);

    return ( (dir)     &&
             (file)    &&
             (found)   &&
             (missing) &&
             (folder)  &&
             (other) );
}

//==============================================================================
// 1.  EXISTENCE AND SEARCH
//==============================================================================

/*
d_tests_arg_path_existence
  CLI11's ExistingFile, ExistingDirectory, ExistingPath and NonexistentPath.
  Tests the following:
  - described as "FILE", "DIR", "PATH(existing)" and "PATH(non-existing)"
  - anything that exists and is not a directory counts as a file; a final
    separator names the directory, and a doubled one is one
  - each refusal with CLI11's message, "File name is missing." for the
    empty name among them
  - a passing name stands as it was given
*/
bool
d_tests_arg_path_existence(void)
{
    return d_tests_arg_path_in_tree(d_tests_arg_path_check_existence);
}

/*
d_tests_arg_path_search
  CLI11's FileOnDefaultPath, under four default paths.
  Tests the following:
  - a name that exists stands, directory or not
  - one that does not is tried again under the default path, joined with
    `/` unless the path is empty or ends in `/` or `\`, and replaced when
    that names a file
  - one found in neither place is refused with "File does not exist:
    <name>", or stands when errors are not returned
*/
bool
d_tests_arg_path_search(void)
{
    return d_tests_arg_path_in_tree(d_tests_arg_path_check_search);
}

//==============================================================================
// 2.  PERMISSIONS AND SIZE
//==============================================================================

/*
d_tests_arg_path_permissions
  CLI11's ReadPermissions, WritePermissions and ExecPermissions.
  Tests the following:
  - a name that does not exist refused with "Path does not exist: <name>"
  - one whose bits grant the access to no one refused with "Path does not
    have required permissions: <name>": a read-only file's write, and on
    POSIX a file's execution without an execute bit
  - on Windows everything that exists may be read and executed, and
    written unless read-only
*/
bool
d_tests_arg_path_permissions(void)
{
    return d_tests_arg_path_in_tree(d_tests_arg_path_check_permissions);
}

/*
d_tests_arg_path_size
  CLI11's FileSizeValidator with a minimum, with a minimum and a maximum,
and NonEmptyFile.
  Tests the following:
  - described as "File size at least <min> bytes", or "File size between
    <min> and <max> bytes"
  - a smaller file refused with "File size <size> bytes is less than
    minimum <min> bytes", a larger one with "File size <size> bytes exceeds
    maximum <max> bytes"
  - a directory refused with "Error getting file size: Is a directory",
    and a name that does not exist with "File does not exist: <name>"
*/
bool
d_tests_arg_path_size(void)
{
    return d_tests_arg_path_in_tree(d_tests_arg_path_check_size);
}

//==============================================================================
// 3.  NAMES
//==============================================================================

/*
d_tests_arg_path_system
  The names whose answer is the system's own, through every validator.
  Tests the following:
  - on POSIX: a link to a file is the file, one to a directory the
    directory, and a broken one nothing; a FIFO and a device are files
    without a size, "Error getting file size: Operation not supported"; a
    file of mode 000 has no permission, whoever asks; a backslash is part
    of a name
  - on Windows: a backslash separates, at the end too, and names are
    matched without regard to case
*/
bool
d_tests_arg_path_system(void)
{
    return d_tests_arg_path_in_tree(d_tests_arg_path_check_system);
}

/*
d_tests_arg_path_utf8
  A file whose name is UTF-8, through every validator.
  Tests the following:
  - each validator reads the name as the file's, the permission and size
    checks too, on every system: on Windows the name is widened from UTF-8
    for each of them
*/
bool
d_tests_arg_path_utf8(void)
{
    return d_tests_arg_path_in_tree(d_tests_arg_path_check_utf8);
}

//==============================================================================
// 4.  CONSTRUCTION AND COMPLETION
//==============================================================================

/*
d_tests_arg_path_construction
  What cannot be made, what each completes as, and a copy.
  Tests the following:
  - FileOnDefaultPath of no path is a failed validator holding
    D_ARG_ERROR_BAD_ARGUMENT, which runs as that error
  - every path validator completes as a path, ExistingDirectory as a
    directory
  - a copy of FileOnDefaultPath owns its default path: it judges as the
    original once the original is gone
*/
bool
d_tests_arg_path_construction(void)
{
    const unsigned int path   = D_ARG_VALIDATOR_PATH;
    const bool         copied =
        d_tests_arg_path_in_tree(d_tests_arg_path_copied);

    return ( (copied)                                                      &&
             (d_tests_arg_path_runs_as(
                  d_arg_validator_file_on_default_path(NULL,
                                                       true),
                  "x",
                  D_ARG_ERROR_BAD_ARGUMENT))                               &&
             (d_tests_arg_path_kind(d_arg_validator_existing_file(),
                                    path))                                 &&
             (d_tests_arg_path_kind(d_arg_validator_existing_directory(),
                                    path | D_ARG_VALIDATOR_DIRECTORY))     &&
             (d_tests_arg_path_kind(d_arg_validator_existing_path(),
                                    path))                                 &&
             (d_tests_arg_path_kind(d_arg_validator_nonexistent_path(),
                                    path))                                 &&
             (d_tests_arg_path_kind(
                  d_arg_validator_file_on_default_path("tree",
                                                       true),
                  path))                                                   &&
             (d_tests_arg_path_kind(d_arg_validator_read_permissions(),
                                    path))                                 &&
             (d_tests_arg_path_kind(d_arg_validator_write_permissions(),
                                    path))                                 &&
             (d_tests_arg_path_kind(d_arg_validator_exec_permissions(),
                                    path))                                 &&
             (d_tests_arg_path_kind(d_arg_validator_non_empty_file(),
                                    path)) );
}

//==============================================================================
// 5.  IN A PARSE
//==============================================================================

/*
d_tests_arg_path_in_a_parse
  The path checks on options, as CLI11 adds them.
  Tests the following:
  - a name a check refuses fails the parse with ValidationError, its
    message after the option's name, and the variable keeps its zero
  - a name a check passes reaches the variable as given
  - FileOnDefaultPath added by transform() replaces a name found under the
    default path, in the results and the variable
*/
bool
d_tests_arg_path_in_a_parse(void)
{
    return d_tests_arg_path_in_tree(d_tests_arg_path_check_parses);
}

//==============================================================================
// 6.  THE SUITE
//==============================================================================

// d_tests_arg_path
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_path[] =
{
    { "arg_path_existence",
      d_tests_arg_path_existence },
    { "arg_path_search",
      d_tests_arg_path_search },
    { "arg_path_permissions",
      d_tests_arg_path_permissions },
    { "arg_path_size",
      d_tests_arg_path_size },
    { "arg_path_system",
      d_tests_arg_path_system },
    { "arg_path_utf8",
      d_tests_arg_path_utf8 },
    { "arg_path_construction",
      d_tests_arg_path_construction },
    { "arg_path_in_a_parse",
      d_tests_arg_path_in_a_parse }
};

// d_tests_arg_path_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_path_count =
    sizeof(d_tests_arg_path) / sizeof(d_tests_arg_path[0]);

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
