/*******************************************************************************
* djinterp [parse]                                                cli_io_tests.c
*
* The suite of the command-line kit's streams and files, declared in
* cli_io_tests.h.
*   The file sections work in a scratch directory of their own, which is the
* working directory while they run (arg_tests_files.h); what the kit makes
* there that the scratch directory did not is removed by the section. A
* standard stream is redirected for a moment (d_tests_cli_redirect), so
* that what the kit reads from standard input and writes to standard
* output is a file the section reads.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_io_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "./cli_io_tests.h"  // corresponding header
// std
#include <errno.h>    // EINVAL, EISDIR, ELOOP, ENOENT
#include <fcntl.h>    // open, O_RDWR, O_NONBLOCK
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // remove, snprintf
#include <stdlib.h>   // getenv, malloc, free
#include <string.h>   // memcmp, strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/parse/cli/c/cli_io.h"  // the kit's streams and files
#include "../arg_parser/c/arg_tests_files.h"  // scratch directories
#include "../../c/tests_section.h"  // d_tests_section
#include "./cli_tests_support.h"  // checks, the environment

#if (D_TESTS_ARG_FILES_POSIX == 1)
    #include <dirent.h>     // opendir, readdir, closedir
    #include <sys/stat.h>   // stat, lstat, umask, S_IS*
    #include <unistd.h>     // close, read
#endif


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_TESTS_CLI_IO_BIG
//   constant: the size of the file the round trips write, past every buffer
// the kit or the runtime keeps.
#define D_TESTS_CLI_IO_BIG     (1024u * 1024u + 7u)

// BYTES
//   constant: text a file holds that no conversion may touch: a NUL, a CR
// before an LF, a lone CR, a byte that is no UTF-8, and UTF-8 that is.
static const char BYTES[] = "a\0b\r\nc\rd\xff" "e\xc3\xa9\n";

/**
 * @brief Makes the bytes of the big round trip: every byte value, over and
 *        over, so that a byte moved or converted shows.
 *
 * @return the bytes, D_TESTS_CLI_IO_BIG of them, to be freed with free;
 *         NULL when there is no room.
 */
static char*
d_tests_cli_io_big(void)
{
    char* const bytes = (char*)malloc(D_TESTS_CLI_IO_BIG);

    for (size_t index = 0u;
         (bytes != NULL) && (index < D_TESTS_CLI_IO_BIG);
         ++index)
    {
        bytes[index] = (char)(unsigned char)((index * 7u) & 0xFFu);
    }

    return bytes;
}


//==============================================================================
// 1.  STREAMS
//==============================================================================

/*
d_tests_cli_io_streams
  The names that are a standard stream, the two sinks, and the streams that
are no terminal whatever the runner's are.
*/
bool
d_tests_cli_io_streams(void)
{
    const struct d_pack_sink out  = d_cli_sink_output();
    const struct d_pack_sink err  = d_cli_sink_error();
    bool                     held = true;

    held = d_tests_cli_check( (d_cli_is_standard("-"))                &&
                              (!d_cli_is_standard("--"))              &&
                              (!d_cli_is_standard(""))                &&
                              (!d_cli_is_standard("-x"))              &&
                              (!d_cli_is_standard(" -"))              &&
                              (!d_cli_is_standard("./-")),
                              "S01",
                              "only `-` is a standard stream") && held;
    held = d_tests_cli_check( (out.write != NULL)                     &&
                              (err.write != NULL)                     &&
                              (out.context == NULL)                   &&
                              (err.context != NULL),
                              "S02",
                              "the sinks of standard output and error") &&
           held;
    held = d_tests_cli_check( (!d_cli_is_terminal(-1))                &&
                              (!d_cli_is_terminal(3))                 &&
                              (!d_cli_is_terminal(99)),
                              "S03",
                              "no terminal past the standard streams") &&
           held;

    return held;
}


//==============================================================================
// 2.  COLOUR
//==============================================================================

/**
 * @brief Sets NO_COLOR and TERM, each to a value or unset, and says whether
 *        a stream in a mode should be coloured then.
 *
 * @param[in] _no_colour  NO_COLOR's value, or NULL to unset it.
 * @param[in] _terminal   TERM's value, or NULL to unset it.
 * @param[in] _stream     the stream.
 * @param[in] _mode       the d_cli_colour mode.
 * @return d_cli_colour_wanted's answer.
 */
static bool
d_tests_cli_io_wanted(
    const char* _no_colour,
    const char* _terminal,
    int         _stream,
    int         _mode
)
{
    (void)d_tests_cli_setenv("NO_COLOR",
                             _no_colour);
    (void)d_tests_cli_setenv("TERM",
                             _terminal);

    return d_cli_colour_wanted(_stream,
                               _mode);
}

/**
 * @brief `auto` where nothing refuses colour: yes for a terminal that takes
 *        sequences, and nothing else -- which is what the runner's own
 *        streams decide.
 *
 * @return whether every check held.
 */
static bool
d_tests_cli_io_colour_auto(void)
{
    const bool out = d_tests_cli_io_wanted(NULL,
                                           "xterm",
                                           (int)D_CLI_STREAM_OUTPUT,
                                           (int)D_CLI_COLOUR_AUTO);
    const bool err = d_tests_cli_io_wanted(NULL,
                                           "xterm",
                                           (int)D_CLI_STREAM_ERROR,
                                           (int)D_CLI_COLOUR_AUTO);

#if (D_INTERNAL_CLI_COLOUR == 1)
    return d_tests_cli_check( (!out ||
                               d_cli_is_terminal((int)D_CLI_STREAM_OUTPUT)) &&
                              (!err ||
                               d_cli_is_terminal((int)D_CLI_STREAM_ERROR)),
                              "C04",
                              "auto colours a terminal, and nothing else");
#else
    return d_tests_cli_check( (!out) &&
                              (!err),
                              "C04",
                              "nothing is coloured without colour");
#endif
}

/**
 * @brief No colour for standard input, nor for a stream that is none; and
 *        a mode no word names is read as `auto`.
 *
 * @return whether every check held.
 */
static bool
d_tests_cli_io_colour_odd(void)
{
    return d_tests_cli_check(
               (!d_tests_cli_io_wanted(NULL,
                                       "xterm",
                                       (int)D_CLI_STREAM_INPUT,
                                       (int)D_CLI_COLOUR_AUTO))        &&
               (!d_tests_cli_io_wanted(NULL,
                                       "xterm",
                                       7,
                                       (int)D_CLI_COLOUR_AUTO))        &&
               (d_tests_cli_io_wanted(NULL,
                                      "xterm",
                                      (int)D_CLI_STREAM_ERROR,
                                      9) ==
                d_tests_cli_io_wanted(NULL,
                                      "xterm",
                                      (int)D_CLI_STREAM_ERROR,
                                      (int)D_CLI_COLOUR_AUTO)),
               "C03",
               "no colour for input or no stream; a mode no word names is "
               "auto");
}

/*
d_tests_cli_io_colour
  The two modes that decide alone, then what refuses `auto`: NO_COLOR with
a value, TERM=dumb, a stream that is only read or none; then `auto` itself;
and a mode no word names, read as `auto`. NO_COLOR is set again after, as
the runner set it, and TERM put back.
*/
bool
d_tests_cli_io_colour(void)
{
    const char* const term  = getenv("TERM");
    char              saved[256];
    const bool        had   = ( (term != NULL) &&
                                (strlen(term) < sizeof(saved)) );
    bool              held  = true;
    const bool        on    = (D_INTERNAL_CLI_COLOUR == 1);

    (void)snprintf(saved,
                   sizeof(saved),
                   "%s",
                   (had) ? term : "");
    held = d_tests_cli_check(
               (d_tests_cli_io_wanted("1",
                                      "xterm",
                                      (int)D_CLI_STREAM_ERROR,
                                      (int)D_CLI_COLOUR_ALWAYS) == on) &&
               (d_tests_cli_io_wanted(NULL,
                                      "dumb",
                                      (int)D_CLI_STREAM_OUTPUT,
                                      (int)D_CLI_COLOUR_ALWAYS) == on) &&
               (!d_tests_cli_io_wanted(NULL,
                                       "xterm",
                                       (int)D_CLI_STREAM_ERROR,
                                       (int)D_CLI_COLOUR_NEVER)),
               "C01",
               "always and never, whatever else holds") && held;
    held = d_tests_cli_check(
               (!d_tests_cli_io_wanted("1",
                                       "xterm",
                                       (int)D_CLI_STREAM_ERROR,
                                       (int)D_CLI_COLOUR_AUTO))        &&
               (!d_tests_cli_io_wanted("",
                                       "dumb",
                                       (int)D_CLI_STREAM_ERROR,
                                       (int)D_CLI_COLOUR_AUTO))        &&
               (!d_tests_cli_io_wanted(NULL,
                                       "dumb",
                                       (int)D_CLI_STREAM_OUTPUT,
                                       (int)D_CLI_COLOUR_AUTO)),
               "C02",
               "NO_COLOR and TERM=dumb refuse auto") && held;
    held = d_tests_cli_io_colour_odd() && held;
    held = d_tests_cli_io_colour_auto() && held;

    (void)d_tests_cli_setenv("NO_COLOR",
                             "1");
    (void)d_tests_cli_setenv("TERM",
                             (had) ? saved : NULL);

    return held;
}


//==============================================================================
// 3.  COLUMNS
//==============================================================================

/**
 * @brief Sets COLUMNS and says how wide the kit then says the terminal is.
 *
 * @param[in] _columns  COLUMNS's value, or NULL to unset it.
 * @return d_cli_terminal_columns's answer.
 */
static size_t
d_tests_cli_io_columns_with(
    const char* _columns
)
{
    (void)d_tests_cli_setenv("COLUMNS",
                             _columns);

    return d_cli_terminal_columns();
}

/*
d_tests_cli_io_columns
  Off a terminal, COLUMNS alone says, when it is a whole number from 1 to
10000; on one, the terminal says, whatever COLUMNS holds. COLUMNS is put
back after.
*/
bool
d_tests_cli_io_columns(void)
{
    const char* const columns = getenv("COLUMNS");
    char              saved[64];
    const bool        had     = ( (columns != NULL) &&
                                  (strlen(columns) < sizeof(saved)) );
    bool              held    = true;

    (void)snprintf(saved,
                   sizeof(saved),
                   "%s",
                   (had) ? columns : "");

    // a terminal answers for itself
    if (d_cli_is_terminal((int)D_CLI_STREAM_OUTPUT))
    {
        held = d_tests_cli_check(d_tests_cli_io_columns_with("abc") > 0u,
                                 "T01",
                                 "a terminal says how wide it is");
    }
    else
    {
        held = d_tests_cli_check(
                   (d_tests_cli_io_columns_with("120") == 120u)     &&
                   (d_tests_cli_io_columns_with("1") == 1u)         &&
                   (d_tests_cli_io_columns_with("10000") == 10000u) &&
                   (d_tests_cli_io_columns_with("10001") == 0u)     &&
                   (d_tests_cli_io_columns_with("0") == 0u)         &&
                   (d_tests_cli_io_columns_with("-80") == 0u)       &&
                   (d_tests_cli_io_columns_with("80x") == 0u)       &&
                   (d_tests_cli_io_columns_with("abc") == 0u)       &&
                   (d_tests_cli_io_columns_with("") == 0u)          &&
                   (d_tests_cli_io_columns_with(NULL) == 0u),
                   "T01",
                   "COLUMNS, when it is a whole number from 1 to 10000");
    }

    (void)d_tests_cli_setenv("COLUMNS",
                             (had) ? saved : NULL);

    return held;
}


//==============================================================================
// 4.  READING
//==============================================================================

/**
 * @brief Reads a file with the kit and checks it came to the bytes given,
 *        a NUL after them; and frees what was read.
 *
 * @param[in] _tag     the case's tag.
 * @param[in] _name    the file's name.
 * @param[in] _bytes   what it holds.
 * @param[in] _length  how many.
 * @return whether it did.
 */
static bool
d_tests_cli_io_read_back(
    const char* _tag,
    const char* _name,
    const char* _bytes,
    size_t      _length
)
{
    char*      data   = NULL;
    size_t     length = 99u;
    const int  status = d_cli_read_file(_name,
                                        &data,
                                        &length);
    const bool held   = ( (status == 0)          &&
                          (data != NULL)         &&
                          (length == _length)    &&
                          ( (_length == 0u) ||
                            (memcmp(data,
                                    _bytes,
                                    _length) == 0) ) &&
                          (data[length] == '\0') );

    d_cli_free(data);

    return d_tests_cli_check(held,
                             _tag,
                             "the file read whole, its bytes as they are, "
                             "and a NUL after them");
}

/**
 * @brief Makes the files the reading section reads: bytes no conversion
 *        may touch, an empty file, a big one, and a directory.
 *
 * @param[in,out] _files  the scratch directory.
 * @param[in]     _big    the big file's bytes.
 * @return whether everything was made.
 */
static bool
d_tests_cli_io_to_read(
    struct d_tests_arg_files* _files,
    const char*               _big
)
{
    return ( (d_tests_arg_files_file(_files,
                                     "empty.txt",
                                     ""))                      &&
             (d_tests_arg_files_folder(_files,
                                       "dir"))                 &&
             (d_tests_cli_file_write("bytes.bin",
                                     BYTES,
                                     sizeof(BYTES) - 1u))      &&
             (d_tests_cli_file_write("big.bin",
                                     _big,
                                     D_TESTS_CLI_IO_BIG)) );
}

/*
d_tests_cli_io_reading
  A file of bytes no conversion may touch, an empty one, a big one; then
one that is not there and a directory, each refused with nothing given.
*/
bool
d_tests_cli_io_reading(void)
{
    struct d_tests_arg_files files;
    char* const              big  = d_tests_cli_io_big();
    char*                    data = (char*)&files;
    size_t                   size = 99u;
    bool                     held = ( (big != NULL) &&
                                      (d_tests_arg_files_enter(&files)) );

    // a scratch directory to read in
    if (!d_tests_cli_check(held,
                           "R01",
                           "the scratch directory could not be made"))
    {
        free(big);

        return false;
    }

    held = d_tests_cli_check(d_tests_cli_io_to_read(&files,
                                                    big),
                             "R01",
                             "the files to read could not be made") &&
           d_tests_cli_io_read_back("R01",
                                    "bytes.bin",
                                    BYTES,
                                    sizeof(BYTES) - 1u);
    held = d_tests_cli_io_read_back("R02",
                                    "empty.txt",
                                    "",
                                    0u) && held;
    held = d_tests_cli_io_read_back("R03",
                                    "big.bin",
                                    big,
                                    D_TESTS_CLI_IO_BIG) && held;
    held = d_tests_cli_check( (d_cli_read_file("missing.txt",
                                               &data,
                                               &size) == ENOENT) &&
                              (data == NULL)                     &&
                              (size == 0u),
                              "R04",
                              "a file that is not there: ENOENT") && held;
    held = d_tests_cli_check( (d_cli_read_file("dir",
                                               &data,
                                               &size) == EISDIR) &&
                              (data == NULL)                     &&
                              (size == 0u),
                              "R05",
                              "a directory: EISDIR") && held;

    (void)remove("bytes.bin");
    (void)remove("big.bin");
    d_tests_arg_files_leave(&files);
    free(big);

    return held;
}


//==============================================================================
// 5.  WRITING
//==============================================================================

#if (D_TESTS_ARG_FILES_POSIX == 1)

/**
 * @brief A file's permission bits.
 *
 * @param[in] _name  the file's name.
 * @return the bits, or 0 when it has no status.
 */
static unsigned int
d_tests_cli_io_mode(
    const char* _name
)
{
    struct stat status;

    return (stat(_name,
                 &status) == 0)
           ? ((unsigned int)status.st_mode & 07777u)
           : 0u;
}

/**
 * @brief Whether the working directory holds exactly the names given: what
 *        a write made beside a file -- its replacement -- is gone.
 *
 * @param[in] _names  the names, NULL after the last.
 * @return `true` if it holds those and nothing else.
 */
static bool
d_tests_cli_io_only(
    const char* const* _names
)
{
    DIR* const directory = opendir(".");
    size_t     found     = 0u;
    size_t     wanted    = 0u;
    bool       only      = (directory != NULL);

    while (_names[wanted] != NULL)
    {
        ++wanted;
    }

    for (struct dirent* each = (only) ? readdir(directory) : NULL;
         each != NULL;
         each = readdir(directory))
    {
        bool known = ( (strcmp(each->d_name,
                               ".") == 0) ||
                       (strcmp(each->d_name,
                               "..") == 0) );

        for (size_t index = 0u; (!known) && (index < wanted); ++index)
        {
            known = (strcmp(each->d_name,
                            _names[index]) == 0);
            found += (known) ? 1u : 0u;
        }

        only = (only) && (known);
    }

    if (directory != NULL)
    {
        (void)closedir(directory);
    }

    return ( (only) &&
             (found == wanted) );
}

/**
 * @brief The permission bits a write gives: a new file's under the umask,
 *        a replaced file's own, whatever they were.
 *
 * @param[in,out] _files  the scratch directory.
 * @return whether every check held.
 */
static bool
d_tests_cli_io_modes(
    struct d_tests_arg_files* _files
)
{
    const mode_t mask = umask(022);
    bool         held = ( (d_tests_arg_files_file(_files,
                                                  "private.txt",
                                                  "old"))          &&
                          (d_tests_arg_files_mode(_files,
                                                  "private.txt",
                                                  0600u))           &&
                          (d_tests_arg_files_file(_files,
                                                  "tool.sh",
                                                  "old"))           &&
                          (d_tests_arg_files_mode(_files,
                                                  "tool.sh",
                                                  0755u)) );

    held = d_tests_cli_check(held,
                             "W03",
                             "the files to replace could not be made") &&
           d_tests_cli_check( (d_cli_write_file("fresh.txt",
                                                "x",
                                                1u) == 0)            &&
                              (d_tests_cli_io_mode("fresh.txt") == 0644u) &&
                              (d_cli_write_file("private.txt",
                                                "new",
                                                3u) == 0)            &&
                              (d_tests_cli_io_mode("private.txt") ==
                               0600u)                                 &&
                              (d_cli_write_file("tool.sh",
                                                "new",
                                                3u) == 0)            &&
                              (d_tests_cli_io_mode("tool.sh") == 0755u),
                              "W03",
                              "a new file's mode under the umask; a "
                              "replaced file's kept");

    (void)remove("fresh.txt");
    (void)umask(mask);

    return held;
}

/**
 * @brief Writing through symbolic links: the file a link leads to is
 *        replaced and the link kept; a link that leads nowhere is replaced
 *        as a file would be; a link that cannot be followed -- round a loop
 *        -- fails the write, and is kept.
 *
 * @param[in,out] _files  the scratch directory.
 * @return whether every check held.
 */
static bool
d_tests_cli_io_links(
    struct d_tests_arg_files* _files
)
{
    struct stat status;
    bool        held = ( (d_tests_arg_files_file(_files,
                                                 "target.txt",
                                                 "old"))             &&
                         (d_tests_arg_files_link(_files,
                                                 "link.txt",
                                                 "target.txt"))      &&
                         (d_tests_arg_files_link(_files,
                                                 "dangling.txt",
                                                 "nowhere.txt"))     &&
                         (d_tests_arg_files_link(_files,
                                                 "loop.txt",
                                                 "loop.txt")) );

    held = d_tests_cli_check(held,
                             "W04",
                             "the links could not be made") &&
           d_tests_cli_check( (d_cli_write_file("link.txt",
                                                "through",
                                                7u) == 0)              &&
                              (d_tests_cli_file_holds("target.txt",
                                                      "through",
                                                      7u))             &&
                              (lstat("link.txt",
                                     &status) == 0)                   &&
                              (S_ISLNK(status.st_mode)),
                              "W04",
                              "a link's file replaced, and the link kept");
    held = d_tests_cli_check( (d_cli_write_file("dangling.txt",
                                                "file",
                                                4u) == 0)              &&
                              (lstat("dangling.txt",
                                     &status) == 0)                   &&
                              (S_ISREG(status.st_mode))               &&
                              (d_tests_cli_file_holds("dangling.txt",
                                                      "file",
                                                      4u)),
                              "W05",
                              "a link that leads nowhere replaced by a "
                              "file") && held;
    held = d_tests_cli_check( (d_cli_write_file("loop.txt",
                                                "x",
                                                1u) == ELOOP)          &&
                              (lstat("loop.txt",
                                     &status) == 0)                   &&
                              (S_ISLNK(status.st_mode)),
                              "W08",
                              "a link round a loop refused, and kept") &&
           held;

    return held;
}

#endif  // D_TESTS_ARG_FILES_POSIX

/**
 * @brief Writing files on every system: a new one, one replaced, an empty
 *        one, a big one; a directory and a missing directory refused, with
 *        nothing made.
 *
 * @param[in,out] _files  the scratch directory.
 * @param[in]     _big    the big round trip's bytes.
 * @return whether every check held.
 */
static bool
d_tests_cli_io_files(
    struct d_tests_arg_files* _files,
    const char*               _big
)
{
    bool held = ( (d_tests_arg_files_file(_files,
                                          "old.txt",
                                          "the old text, longer"))  &&
                  (d_tests_arg_files_folder(_files,
                                            "dir")) );

    held = d_tests_cli_check(held,
                             "W01",
                             "the files to write over could not be made") &&
           d_tests_cli_check( (d_cli_write_file("new.bin",
                                                BYTES,
                                                sizeof(BYTES) - 1u) == 0) &&
                              (d_tests_cli_file_holds("new.bin",
                                                      BYTES,
                                                      sizeof(BYTES) - 1u)),
                              "W01",
                              "a new file, its bytes as they are");
    held = d_tests_cli_check( (d_cli_write_file("old.txt",
                                                "new",
                                                3u) == 0)               &&
                              (d_tests_cli_file_holds("old.txt",
                                                      "new",
                                                      3u))              &&
                              (d_cli_write_file("empty.txt",
                                                NULL,
                                                0u) == 0)               &&
                              (d_tests_cli_file_holds("empty.txt",
                                                      "",
                                                      0u))              &&
                              (d_cli_write_file("big.bin",
                                                _big,
                                                D_TESTS_CLI_IO_BIG) == 0) &&
                              (d_tests_cli_file_holds("big.bin",
                                                      _big,
                                                      D_TESTS_CLI_IO_BIG)),
                              "W02",
                              "a file replaced, an empty one, a big one") &&
           held;
    held = d_tests_cli_check( (d_cli_write_file("dir",
                                                "x",
                                                1u) == EISDIR)          &&
                              (d_cli_write_file("nowhere/x.txt",
                                                "x",
                                                1u) == ENOENT),
                              "W06",
                              "a directory, and a directory that is not "
                              "there, refused") && held;

    return held;
}

/*
d_tests_cli_io_writing
  The files every system writes; the modes and links of POSIX systems;
then the directory must hold what the section made and nothing else.
*/
bool
d_tests_cli_io_writing(void)
{
    struct d_tests_arg_files files;
    char* const              big  = d_tests_cli_io_big();
    bool                     held = ( (big != NULL) &&
                                      (d_tests_arg_files_enter(&files)) );

    // a scratch directory to write in
    if (!d_tests_cli_check(held,
                           "W01",
                           "the scratch directory could not be made"))
    {
        free(big);

        return false;
    }

    held = d_tests_cli_io_files(&files,
                                big);

#if (D_TESTS_ARG_FILES_POSIX == 1)
    static const char* const made[] =
    {
        "old.txt", "dir", "new.bin", "empty.txt", "big.bin",
        "private.txt", "tool.sh", "target.txt", "link.txt",
        "dangling.txt", "loop.txt", NULL
    };

    held = d_tests_cli_io_modes(&files) && held;
    held = d_tests_cli_io_links(&files) && held;
    held = d_tests_cli_check(d_tests_cli_io_only(made),
                             "W07",
                             "nothing left beside a file written") && held;
#endif

    (void)remove("new.bin");
    (void)remove("empty.txt");
    (void)remove("big.bin");
    d_tests_arg_files_leave(&files);
    free(big);

    return held;
}


//==============================================================================
// 6.  WHAT NO FILE STANDS IN FOR
//==============================================================================

#if (D_TESTS_ARG_FILES_POSIX == 1)

/**
 * @brief Writes a FIFO with a reader already on it, and reads back what was
 *        written: the FIFO written to, and still a FIFO.
 *
 * @param[in,out] _files  the scratch directory.
 * @return whether it held.
 */
static bool
d_tests_cli_io_fifo(
    struct d_tests_arg_files* _files
)
{
    struct stat status;
    char        got[16];
    const int   reader = d_tests_arg_files_fifo(_files,
                                                "fifo")
                         ? open("fifo",
                                O_RDWR | O_NONBLOCK)
                         : -1;
    const bool  wrote  = (reader >= 0) &&
                         (d_cli_write_file("fifo",
                                           "piped",
                                           5u) == 0);
    const long  read_  = (wrote) ? (long)read(reader,
                                              got,
                                              sizeof(got))
                                 : -1L;

    if (reader >= 0)
    {
        (void)close(reader);
    }

    return d_tests_cli_check( (read_ == 5L)                          &&
                              (memcmp(got,
                                      "piped",
                                      5u) == 0)                      &&
                              (lstat("fifo",
                                     &status) == 0)                  &&
                              (S_ISFIFO(status.st_mode)),
                              "D02",
                              "a FIFO written to, and still a FIFO");
}

#endif  // D_TESTS_ARG_FILES_POSIX

/*
d_tests_cli_io_special
  A device and a FIFO are written to, not replaced: /dev/null is still the
null device after, the FIFO still a FIFO, and its reader has the bytes.
Where the system has neither, there is nothing to write.
*/
bool
d_tests_cli_io_special(void)
{
#if (D_TESTS_ARG_FILES_POSIX == 1)
    struct d_tests_arg_files files;
    struct stat              status;
    bool                     held = d_tests_cli_check(
                                        (d_cli_write_file("/dev/null",
                                                          "nothing",
                                                          7u) == 0)  &&
                                        (stat("/dev/null",
                                              &status) == 0)         &&
                                        (S_ISCHR(status.st_mode)),
                                        "D01",
                                        "/dev/null written to, and still "
                                        "the null device");

    // a scratch directory for the FIFO
    if (!d_tests_cli_check(d_tests_arg_files_enter(&files),
                           "D02",
                           "the scratch directory could not be made"))
    {
        return false;
    }

    held = d_tests_cli_io_fifo(&files) && held;
    d_tests_arg_files_leave(&files);

    return held;
#else
    return true;
#endif
}


//==============================================================================
// 7.  STANDARD STREAMS
//==============================================================================

/*
d_tests_cli_io_standard
  `-` read is standard input, to its end, its bytes as they are; `-`
written is standard output, through the kit's sink, its bytes as they are
-- an LF never a CRLF (R18).
*/
bool
d_tests_cli_io_standard(void)
{
    struct d_tests_arg_files files;
    char*                    data   = NULL;
    size_t                   length = 0u;
    bool                     held   = d_tests_arg_files_enter(&files);

    // a scratch directory for the streams' files
    if (!d_tests_cli_check(held,
                           "I01",
                           "the scratch directory could not be made"))
    {
        return false;
    }

    held = d_tests_cli_file_write("in.bin",
                                  BYTES,
                                  sizeof(BYTES) - 1u);

    int       saved = d_tests_cli_redirect(0,
                                           "in.bin");
    const int read_ = (saved >= 0) ? d_cli_read_file("-",
                                                     &data,
                                                     &length)
                                   : -1;

    d_tests_cli_restore(0,
                        saved);
    held = d_tests_cli_check( (held)                              &&
                              (read_ == 0)                        &&
                              (length == sizeof(BYTES) - 1u)      &&
                              (memcmp(data,
                                      BYTES,
                                      length) == 0),
                              "I01",
                              "standard input read whole, its bytes as "
                              "they are");
    d_cli_free(data);

    saved = d_tests_cli_redirect(1,
                                 "out.bin");

    const int wrote = (saved >= 0) ? d_cli_write_file("-",
                                                      BYTES,
                                                      sizeof(BYTES) - 1u)
                                   : -1;

    d_tests_cli_restore(1,
                        saved);
    held = d_tests_cli_check( (wrote == 0)                        &&
                              (d_tests_cli_file_holds("out.bin",
                                                      BYTES,
                                                      sizeof(BYTES) - 1u)),
                              "I02",
                              "standard output written, its bytes as they "
                              "are") && held;

    (void)remove("in.bin");
    (void)remove("out.bin");
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 8.  NULLS
//==============================================================================

/*
d_tests_cli_io_nulls
  EINVAL for every pointer the calls need and were not given; nothing
freed, nothing standard.
*/
bool
d_tests_cli_io_nulls(void)
{
    char*  data   = NULL;
    size_t length = 0u;

    d_cli_free(NULL);

    return d_tests_cli_check( (d_cli_read_file(NULL,
                                               &data,
                                               &length) == EINVAL)  &&
                              (d_cli_read_file("x",
                                               NULL,
                                               &length) == EINVAL)  &&
                              (d_cli_read_file("x",
                                               &data,
                                               NULL) == EINVAL)     &&
                              (d_cli_write_file(NULL,
                                                "x",
                                                1u) == EINVAL)      &&
                              (d_cli_write_file("x",
                                                NULL,
                                                1u) == EINVAL)      &&
                              (!d_cli_is_standard(NULL)),
                              "N01",
                              "EINVAL for what the calls were not given");
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_io
//   constant: the suite's sections, in the order they run.
const struct d_tests_section d_tests_cli_io[] =
{
    { "cli_io_streams",  d_tests_cli_io_streams  },
    { "cli_io_colour",   d_tests_cli_io_colour   },
    { "cli_io_columns",  d_tests_cli_io_columns  },
    { "cli_io_reading",  d_tests_cli_io_reading  },
    { "cli_io_writing",  d_tests_cli_io_writing  },
    { "cli_io_special",  d_tests_cli_io_special  },
    { "cli_io_standard", d_tests_cli_io_standard },
    { "cli_io_nulls",    d_tests_cli_io_nulls    }
};

// d_tests_cli_io_count
//   constant: how many sections d_tests_cli_io has.
const size_t d_tests_cli_io_count =
    sizeof(d_tests_cli_io) / sizeof(d_tests_cli_io[0]);
