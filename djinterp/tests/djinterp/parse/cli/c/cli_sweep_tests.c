/*******************************************************************************
* djinterp [parse]                                             cli_sweep_tests.c
*
* The command-line kit's allocation sweep, declared in cli_sweep_tests.h.
*   malloc, calloc, realloc and free are wrapped at link time (--wrap), and
* the wrappers keep a ledger while a run's measured part runs: how many
* allocations were asked for, which one to refuse, and every block made and
* not yet freed, by address, with the allocation that made it. A block the
* C library made for itself -- realpath's answer, freed by its caller -- is
* not the ledger's, and freeing it changes nothing; a block a run made and
* left live is named by the allocation that made it.
*   A case is run once as it is, counting; then once for each allocation it
* asked for, that one refused. Each run is held to the clean run's result
* (cli_sweep_tests.h): the same status with the same output, the same
* result -- a module's report, a file's bytes, a path -- and diagnostics no
* more than the clean run's; or trouble, with no output the clean run would
* not have begun. A case also checks what only it can see: a file left
* neither old nor new, a module left loaded after its load failed, a
* command's context not as it was given.
*   Every program here is made as a program's main makes one -- the parser,
* then the fixture's program, a definition refused ending the run in
* trouble -- and writes into the run's own buffers, which never allocate.
* Each command's work is given a context of its own, made with the wrapped
* malloc and freed with the program, so that a context lost is a block left
* live.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_sweep_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "./cli_sweep_tests.h"  // corresponding header
// std
#include <errno.h>    // ENOENT, ENOMEM
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf, snprintf, fopen, fwrite, fread, fclose, remove
#include <stdlib.h>   // malloc, free, getenv
#include <string.h>   // memcmp, memcpy, memset, strchr, strlen, strrchr, strstr
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/filesys/file_dir.h"  // d_path_resolve
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // D_INTERNAL_ARG_PARSER_COMPLETION
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_COMPILER_CLANG
#include "../../../../../inc/djinterp/env/parse/cli/env_cli.h"  // D_ENV_CLI_*
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h"  // the line width
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser
#include "../../../../../inc/djinterp/parse/cli/c/cli_io.h"  // files
#include "../../../../../inc/djinterp/parse/cli/c/cli_plugin.h"  // modules
#include "../../../../../inc/djinterp/parse/cli/c/cli_program.h"  // the program
#include "../arg_parser/c/arg_tests_files.h"  // scratch directories
#include "../../c/tests_section.h"  // d_tests_section
#include "./cli_tests_support.h"  // words, printing, values, the environment
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, uintptr_t
// platform
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #include <io.h>         // _findfirst, _findnext, _findclose
#else
    #include <dirent.h>     // opendir, readdir, closedir
#endif
#if (D_TESTS_ARG_FILES_POSIX == 1)
    #include <sys/stat.h>   // lstat, S_ISLNK
#endif


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_TESTS_CLI_SWEEP_COUNT
//   macro: how many entries a table has.
#define D_TESTS_CLI_SWEEP_COUNT(_table)  (sizeof(_table) / sizeof((_table)[0]))

// D_TESTS_CLI_SWEEP_SLOTS
//   constant: how many blocks the ledger can hold live at once: a power of
// two, as a block's slot is its hash masked.
#define D_TESTS_CLI_SWEEP_SLOTS     65536u

// D_TESTS_CLI_SWEEP_TEXT_MAX
//   constant: the most one of a run's texts holds, its NUL among them.
#define D_TESTS_CLI_SWEEP_TEXT_MAX  65536u

// D_TESTS_CLI_SWEEP_PATH_MAX
//   constant: the longest path a case builds.
#define D_TESTS_CLI_SWEEP_PATH_MAX  1024u

// D_TESTS_CLI_SWEEP_SHOWN
//   constant: how many runs of a case that fell short are described; the
// rest are counted.
#define D_TESTS_CLI_SWEEP_SHOWN     3u

// D_TESTS_CLI_SWEEP_LOST
//   constant: the status of a command's work that found its context not as
// it was given -- a status no clean run gives.
#define D_TESTS_CLI_SWEEP_LOST      99

// d_tests_cli_sweep_block
//   struct: a block the ledger holds: where it is, and the allocation,
// counted from 1, that made it.
struct d_tests_cli_sweep_block
{
    void*  memory;
    size_t made;
};

// d_tests_cli_sweep_ledger
//   struct: what the wrappers keep. `armed` while a run's measured part
// runs: only then is an allocation counted, refused or held. `refuse_at`
// is the allocation to refuse, counted from 1, 0 for none; `asked` how many
// have been asked for; `live` how many blocks are held; `full` whether a
// block went unheld for want of a slot.
struct d_tests_cli_sweep_ledger
{
    bool   armed;
    size_t refuse_at;
    size_t asked;
    size_t live;
    bool   full;
};

// d_tests_cli_sweep_book / d_tests_cli_sweep_blocks
//   variables: the ledger, and the blocks it holds, an open-addressed table
// probed in order.
static struct d_tests_cli_sweep_ledger d_tests_cli_sweep_book;
static struct d_tests_cli_sweep_block
              d_tests_cli_sweep_blocks[D_TESTS_CLI_SWEEP_SLOTS];

// --wrap names a symbol's wrapper __wrap_<symbol> and the symbol itself
// __real_<symbol>, names C reserves to the implementation -- the linker, here
// -- and no other spelling reaches them; so clang's -Wreserved-identifier,
// where it has it, is off for these eight declarations and nothing else
#if defined(D_ENV_COMPILER_CLANG)
    #if __has_warning("-Wreserved-identifier")
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wreserved-identifier"
    #endif
#endif

// the allocator the wrappers stand in front of, as the linker names it
void* __real_malloc(size_t _size);
void* __real_calloc(size_t _count,
                    size_t _size);
void* __real_realloc(void*  _memory,
                     size_t _size);
void  __real_free(void* _memory);

// the wrappers, which every call of the kit, the parser and this suite
// reaches
void* __wrap_malloc(size_t _size);
void* __wrap_calloc(size_t _count,
                    size_t _size);
void* __wrap_realloc(void*  _memory,
                     size_t _size);
void  __wrap_free(void* _memory);

#if defined(D_ENV_COMPILER_CLANG)
    #if __has_warning("-Wreserved-identifier")
        #pragma clang diagnostic pop
    #endif
#endif

/**
 * @brief Where a block's search starts in the table: its address hashed.
 *
 * @param[in] _memory  the block.
 * @return its first slot.
 */
static size_t
d_tests_cli_sweep_home(
    const void* _memory
)
{
    const uintptr_t address = (uintptr_t)_memory;

    // the address's low bits, which an allocator's alignment leaves still,
    // shifted off; the rest spread by Knuth's multiplier
    return (size_t)((address >> 4) * (uintptr_t)2654435761u) &
           (size_t)(D_TESTS_CLI_SWEEP_SLOTS - 1u);
}

/**
 * @brief Holds a block, made by the allocation given.
 *
 * @param[in] _memory  the block.
 * @param[in] _made    the allocation that made it.
 */
static void
d_tests_cli_sweep_hold(
    void*  _memory,
    size_t _made
)
{
    size_t slot = d_tests_cli_sweep_home(_memory);

    // no slot left: the run is judged short of slots, not of memory
    if (d_tests_cli_sweep_book.live >= (D_TESTS_CLI_SWEEP_SLOTS - 1u))
    {
        d_tests_cli_sweep_book.full = true;

        return;
    }

    // the first empty slot from the block's home
    while (d_tests_cli_sweep_blocks[slot].memory != NULL)
    {
        slot = (slot + 1u) & (D_TESTS_CLI_SWEEP_SLOTS - 1u);
    }

    d_tests_cli_sweep_blocks[slot].memory = _memory;
    d_tests_cli_sweep_blocks[slot].made   = _made;
    ++d_tests_cli_sweep_book.live;

    return;
}

/**
 * @brief Finds a block the ledger holds.
 *
 * @param[in] _memory  the block.
 * @return its slot; D_TESTS_CLI_SWEEP_SLOTS when it is not held.
 */
static size_t
d_tests_cli_sweep_find(
    const void* _memory
)
{
    size_t slot = d_tests_cli_sweep_home(_memory);

    // the run of full slots from its home
    while (d_tests_cli_sweep_blocks[slot].memory != NULL)
    {
        if (d_tests_cli_sweep_blocks[slot].memory == _memory)
        {
            return slot;
        }

        slot = (slot + 1u) & (D_TESTS_CLI_SWEEP_SLOTS - 1u);
    }

    return D_TESTS_CLI_SWEEP_SLOTS;
}

/**
 * @brief Lets a block go, when the ledger holds it.
 *
 * @param[in]  _memory  the block.
 * @param[out] _made    receives the allocation that made it; may be NULL.
 * @return whether it was held.
 *
 * The table is probed in order, so a hole would cut a later block off from
 * its home: each block after the hole, up to the next empty slot, moves
 * back into it unless its home lies between the hole and itself.
 */
static bool
d_tests_cli_sweep_let_go(
    const void* _memory,
    size_t*     _made
)
{
    const size_t mask = (size_t)(D_TESTS_CLI_SWEEP_SLOTS - 1u);
    size_t       hole = d_tests_cli_sweep_find(_memory);

    // not the ledger's
    if (hole == D_TESTS_CLI_SWEEP_SLOTS)
    {
        return false;
    }

    if (_made != NULL)
    {
        *_made = d_tests_cli_sweep_blocks[hole].made;
    }

    for (size_t next = (hole + 1u) & mask;
         d_tests_cli_sweep_blocks[next].memory != NULL;
         next = (next + 1u) & mask)
    {
        const size_t home  = d_tests_cli_sweep_home(
                                 d_tests_cli_sweep_blocks[next].memory);
        const bool   stays = (hole <= next)
                             ? ( (hole < home) && (home <= next) )
                             : ( (hole < home) || (home <= next) );

        if (!stays)
        {
            d_tests_cli_sweep_blocks[hole] = d_tests_cli_sweep_blocks[next];
            hole                           = next;
        }
    }

    d_tests_cli_sweep_blocks[hole].memory = NULL;
    d_tests_cli_sweep_blocks[hole].made   = 0u;
    --d_tests_cli_sweep_book.live;

    return true;
}

/**
 * @brief Counts an allocation asked for while armed, and tells whether it
 *        is the one to refuse.
 *
 * @return `true` for the allocation the ledger refuses.
 */
static bool
d_tests_cli_sweep_refused(void)
{
    // nothing counted outside a run's measured part
    if (!d_tests_cli_sweep_book.armed)
    {
        return false;
    }

    ++d_tests_cli_sweep_book.asked;

    return ( (d_tests_cli_sweep_book.refuse_at != 0u) &&
             (d_tests_cli_sweep_book.asked ==
              d_tests_cli_sweep_book.refuse_at) );
}

/**
 * @brief malloc: counted and held while armed, refused when the ledger
 *        says so.
 *
 * @param[in] _size  the size asked for.
 * @return the block; NULL when it is refused or cannot be had.
 */
void*
__wrap_malloc(
    size_t _size
)
{
    // the refused allocation
    if (d_tests_cli_sweep_refused())
    {
        return NULL;
    }

    void* const memory = __real_malloc(_size);

    if ( (memory != NULL) &&
         (d_tests_cli_sweep_book.armed) )
    {
        d_tests_cli_sweep_hold(memory,
                               d_tests_cli_sweep_book.asked);
    }

    return memory;
}

/**
 * @brief calloc: counted and held while armed, refused when the ledger
 *        says so.
 *
 * @param[in] _count  how many elements.
 * @param[in] _size   the size of each.
 * @return the zeroed block; NULL when it is refused or cannot be had.
 */
void*
__wrap_calloc(
    size_t _count,
    size_t _size
)
{
    // the refused allocation
    if (d_tests_cli_sweep_refused())
    {
        return NULL;
    }

    void* const memory = __real_calloc(_count,
                                       _size);

    if ( (memory != NULL) &&
         (d_tests_cli_sweep_book.armed) )
    {
        d_tests_cli_sweep_hold(memory,
                               d_tests_cli_sweep_book.asked);
    }

    return memory;
}

/**
 * @brief realloc: counted while armed, refused when the ledger says so.
 *
 * @param[in] _memory  the block to grow; NULL for a new one.
 * @param[in] _size    the size asked for.
 * @return the block; NULL, the old one untouched, when it is refused or
 *         cannot be had.
 *
 * A held block, moved or not, stays held under the allocation that first
 * made it; a new block is held while armed; a block the ledger never held
 * -- the C library's own -- stays unheld wherever it moves.
 */
void*
__wrap_realloc(
    void*  _memory,
    size_t _size
)
{
    size_t made = 0u;

    // the refused allocation, the block as it was
    if (d_tests_cli_sweep_refused())
    {
        return NULL;
    }

    void* const memory = __real_realloc(_memory,
                                        _size);

    // a failure, the block as it was and still where it was
    if (!memory)
    {
        return NULL;
    }

    if ( (_memory != NULL) &&
         (d_tests_cli_sweep_let_go(_memory,
                                   &made)) )
    {
        d_tests_cli_sweep_hold(memory,
                               made);
    }
    else if ( (_memory == NULL) &&
              (d_tests_cli_sweep_book.armed) )
    {
        d_tests_cli_sweep_hold(memory,
                               d_tests_cli_sweep_book.asked);
    }

    return memory;
}

/**
 * @brief free: a held block let go.
 *
 * @param[in] _memory  the block; NULL frees nothing.
 */
void
__wrap_free(
    void* _memory
)
{
    if (_memory != NULL)
    {
        (void)d_tests_cli_sweep_let_go(_memory,
                                       NULL);
    }

    __real_free(_memory);

    return;
}

/**
 * @brief Starts a run: the ledger emptied, and the allocation it refuses
 *        set. Blocks an earlier run left live, already reported, are let
 *        go -- not freed, as nothing knows what is in them.
 *
 * @param[in] _refuse_at  the allocation the run refuses, counted from 1; 0
 *                        for none.
 */
static void
d_tests_cli_sweep_reset(
    size_t _refuse_at
)
{
    if (d_tests_cli_sweep_book.live != 0u)
    {
        memset(d_tests_cli_sweep_blocks,
               0,
               sizeof(d_tests_cli_sweep_blocks));
    }

    d_tests_cli_sweep_book.armed     = false;
    d_tests_cli_sweep_book.refuse_at = _refuse_at;
    d_tests_cli_sweep_book.asked     = 0u;
    d_tests_cli_sweep_book.live      = 0u;
    d_tests_cli_sweep_book.full      = false;

    return;
}

/**
 * @brief Arms the ledger, or disarms it: the measured part of a run is
 *        what lies between.
 *
 * @param[in] _armed  whether to count, refuse and hold.
 */
static void
d_tests_cli_sweep_arm(
    bool _armed
)
{
    d_tests_cli_sweep_book.armed = _armed;

    return;
}

// d_tests_cli_sweep_text
//   struct: one of a run's texts, kept whole up to its room: its bytes,
// NUL-terminated; how many; and whether more came than it holds.
struct d_tests_cli_sweep_text
{
    char   data[D_TESTS_CLI_SWEEP_TEXT_MAX];
    size_t length;
    bool   over;
};

/**
 * @brief A sink's write into a text: everything taken, what does not fit
 *        dropped and marked.
 *
 * @param[in,out] _context  the text.
 * @param[in]     _data     the bytes.
 * @param[in]     _size     how many.
 * @return `_size`: all of them taken, as a sink that never fails takes them.
 */
static size_t
d_tests_cli_sweep_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_tests_cli_sweep_text* const text  = _context;
    const size_t                         room  =
        (D_TESTS_CLI_SWEEP_TEXT_MAX - 1u) - text->length;
    const size_t                         taken = (_size <= room) ? _size
                                                                 : room;

    memcpy(text->data + text->length,
           _data,
           taken);
    text->length            += taken;
    text->data[text->length]  = '\0';
    text->over               = (text->over) || (taken < _size);

    return _size;
}

/**
 * @brief Empties a text.
 *
 * @param[out] _text  the text.
 */
static void
d_tests_cli_sweep_empty(
    struct d_tests_cli_sweep_text* _text
)
{
    _text->data[0] = '\0';
    _text->length  = 0u;
    _text->over    = false;

    return;
}

/**
 * @brief A sink into a text.
 *
 * @param[in,out] _text  the text the sink writes into.
 * @return the sink.
 */
static struct d_pack_sink
d_tests_cli_sweep_sink(
    struct d_tests_cli_sweep_text* _text
)
{
    const struct d_pack_sink sink = { d_tests_cli_sweep_write,
                                      _text };

    return sink;
}

/**
 * @brief Whether two texts hold the same bytes.
 *
 * @param[in] _one    one text.
 * @param[in] _other  the other.
 * @return whether they are the same.
 */
static bool
d_tests_cli_sweep_same(
    const struct d_tests_cli_sweep_text* _one,
    const struct d_tests_cli_sweep_text* _other
)
{
    return ( (_one->length == _other->length) &&
             (memcmp(_one->data,
                     _other->data,
                     _one->length) == 0) );
}

/**
 * @brief Whether a text is the start of another, or all of it.
 *
 * @param[in] _start  the text that may begin the other.
 * @param[in] _whole  the other.
 * @return whether it begins it.
 */
static bool
d_tests_cli_sweep_begins(
    const struct d_tests_cli_sweep_text* _start,
    const struct d_tests_cli_sweep_text* _whole
)
{
    return ( (_start->length <= _whole->length) &&
             (memcmp(_start->data,
                     _whole->data,
                     _start->length) == 0) );
}

/**
 * @brief The length of the line a text has at a place: up to its newline,
 *        the newline counted, or to the text's end.
 *
 * @param[in] _at  the line's start.
 * @return its length.
 */
static size_t
d_tests_cli_sweep_line(
    const char* _at
)
{
    const char* const end = strchr(_at,
                                   '\n');

    return (end != NULL) ? ((size_t)(end - _at) + 1u) : strlen(_at);
}

/**
 * @brief Whether every line of a text is a line of another, in the same
 *        order: the other's diagnostics, some of them perhaps dropped.
 *
 * @param[in] _some  the text whose lines are looked for.
 * @param[in] _all   the text they are looked for in.
 * @return whether each was found, after the one before it.
 */
static bool
d_tests_cli_sweep_lines_kept(
    const struct d_tests_cli_sweep_text* _some,
    const struct d_tests_cli_sweep_text* _all
)
{
    const char* at   = _all->data;
    const char* line = _some->data;

    // each line looked for after the last one found
    while (*line != '\0')
    {
        const size_t length = d_tests_cli_sweep_line(line);
        bool         found  = false;

        while ( (!found) &&
                (*at != '\0') )
        {
            const size_t span = d_tests_cli_sweep_line(at);

            found = ( (span == length) &&
                      (memcmp(at,
                              line,
                              length) == 0) );
            at   += span;
        }

        if (!found)
        {
            return false;
        }

        line += length;
    }

    return true;
}

// d_tests_cli_sweep_result
//   struct: what a run came to: its status -- the program's, or a call's
// mapped to the program's statuses; its output and error output; its
// other result -- a module's report, a file's bytes, a path; and what the
// run itself found broken, or NULL.
struct d_tests_cli_sweep_result
{
    int                           status;
    struct d_tests_cli_sweep_text out;
    struct d_tests_cli_sweep_text err;
    struct d_tests_cli_sweep_text extra;
    const char*                   broken;
};

// d_tests_cli_sweep_clean / d_tests_cli_sweep_now
//   variables: what a case's clean run came to, and what its run under a
// refusal came to.
static struct d_tests_cli_sweep_result d_tests_cli_sweep_clean;
static struct d_tests_cli_sweep_result d_tests_cli_sweep_now;

/**
 * @brief Empties a result, before its run.
 *
 * @param[out] _result  the result.
 */
static void
d_tests_cli_sweep_start(
    struct d_tests_cli_sweep_result* _result
)
{
    _result->status = -1;
    d_tests_cli_sweep_empty(&_result->out);
    d_tests_cli_sweep_empty(&_result->err);
    d_tests_cli_sweep_empty(&_result->extra);
    _result->broken = NULL;

    return;
}

/**
 * @brief Maps a call's status to a program's: done, refused for a reason
 *        of its own, or trouble.
 *
 * @param[in] _status       the call's d_arg_status.
 * @param[in] _out_of_room  whether a buffer it wrote into failed to grow.
 * @return D_CLI_EXIT_OK, D_CLI_EXIT_NO or D_CLI_EXIT_TROUBLE.
 */
static int
d_tests_cli_sweep_status(
    int  _status,
    bool _out_of_room
)
{
    // memory, wherever it ran out
    if ( (_out_of_room) ||
         (_status == (int)D_ARG_ERROR_OUT_OF_MEMORY) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    return (_status == (int)D_ARG_OK) ? D_CLI_EXIT_OK : D_CLI_EXIT_NO;
}

/**
 * @brief Copies bytes into a text, as a result.
 *
 * @param[out] _text    the text.
 * @param[in]  _data    the bytes; NULL for none.
 * @param[in]  _length  how many.
 */
static void
d_tests_cli_sweep_keep(
    struct d_tests_cli_sweep_text* _text,
    const char*                    _data,
    size_t                         _length
)
{
    d_tests_cli_sweep_empty(_text);

    if (_data != NULL)
    {
        (void)d_tests_cli_sweep_write(_text,
                                      _data,
                                      _length);
    }

    return;
}

// d_tests_cli_sweep_make_fn
//   type: a fixture: makes a program over a parser d_arg_parser_init made --
// d_cli_program_init, then the program's own definitions -- and returns
// whether everything was made.
typedef bool (*d_tests_cli_sweep_make_fn)(struct d_cli_program* _program,
                                          struct d_arg_parser*  _parser);

struct d_tests_cli_sweep_case;

// d_tests_cli_sweep_run_fn
//   type: what a case does: one run, measured, into a result.
typedef void (*d_tests_cli_sweep_run_fn)(
                  const struct d_tests_cli_sweep_case* _case,
                  struct d_tests_cli_sweep_result*     _result);

// d_tests_cli_sweep_case
//   struct: one case: its tag; its command line, or the name it works on;
// the fixture that makes its program, if it runs one; what it does; and the
// status its clean run must give, so that a case written wrong cannot pass
// as one that ends in trouble every time.
struct d_tests_cli_sweep_case
{
    const char*               tag;
    const char*               line;
    d_tests_cli_sweep_make_fn make;
    d_tests_cli_sweep_run_fn  run;
    int                       status;
};

/**
 * @brief Judges how a program's run that ended in trouble said so. Where
 *        the clean run did not end in trouble, the refusal is the one thing
 *        that changed, and the run must say it: the want of memory -- never
 *        another failure, such as a command not there or a module not
 *        installed, that the refusal would be passed off as. Where the
 *        clean run failed too, the failure may be said in fewer words, or
 *        in the parser's plainer ones, but must be said.
 *
 * @param[in] _clean  the clean run.
 * @param[in] _now    the run under a refusal, which ended in trouble.
 * @return whether it was said so.
 */
static bool
d_tests_cli_sweep_said(
    const struct d_tests_cli_sweep_result* _clean,
    const struct d_tests_cli_sweep_result* _now
)
{
    return (_clean->status == D_CLI_EXIT_TROUBLE)
           ? (_now->err.length > 0u)
           : (strstr(_now->err.data,
                     "out of memory") != NULL);
}

/**
 * @brief Judges a run under a refusal against the clean run.
 *
 * @param[in] _case  the case: a program's run, whose trouble must be said,
 *                   when it has a fixture.
 * @return what fell short, in words; NULL when nothing did.
 */
static const char*
d_tests_cli_sweep_fault(
    const struct d_tests_cli_sweep_case* _case
)
{
    const struct d_tests_cli_sweep_result* const clean =
        &d_tests_cli_sweep_clean;
    const struct d_tests_cli_sweep_result* const now   =
        &d_tests_cli_sweep_now;

    // the ledger's own limits, then what every run must keep
    if ( (d_tests_cli_sweep_book.full) ||
         (now->out.over)               ||
         (now->err.over)               ||
         (now->extra.over) )
    {
        return "more than the sweep holds";
    }

    if (d_tests_cli_sweep_book.live != 0u)
    {
        return "memory left live";
    }

    if (now->broken != NULL)
    {
        return now->broken;
    }

    // all the clean run did, or trouble
    if ( (now->status == clean->status) &&
         (now->status != D_CLI_EXIT_TROUBLE) )
    {
        return (!d_tests_cli_sweep_same(&now->out,
                                        &clean->out))
               ? "the clean run's status, another output"
               : (!d_tests_cli_sweep_same(&now->extra,
                                          &clean->extra))
               ? "the clean run's status, another result"
               : (!d_tests_cli_sweep_lines_kept(&now->err,
                                                &clean->err))
               ? "the clean run's status, other diagnostics"
               : NULL;
    }

    if (now->status != D_CLI_EXIT_TROUBLE)
    {
        return "a status neither the clean run's nor trouble";
    }

    return (!d_tests_cli_sweep_begins(&now->out,
                                      &clean->out))
           ? "trouble, with output the clean run would not have begun"
           : ( (_case->make != NULL) &&
               (!d_tests_cli_sweep_said(clean,
                                        now)) )
           ? "trouble, said as neither the want of memory nor the clean "
             "run's failure"
           : NULL;
}

/**
 * @brief Writes which allocations made the blocks a run left live, the
 *        first few of them.
 */
static void
d_tests_cli_sweep_name_live(void)
{
    size_t named = 0u;

    printf("        left live, made by allocation");

    for (size_t slot = 0u;
         (slot < D_TESTS_CLI_SWEEP_SLOTS) &&
         (named < D_TESTS_CLI_SWEEP_SHOWN);
         ++slot)
    {
        if (d_tests_cli_sweep_blocks[slot].memory != NULL)
        {
            printf(" %lu",
                   (unsigned long)d_tests_cli_sweep_blocks[slot].made);
            ++named;
        }
    }

    printf("%s\n",
           (d_tests_cli_sweep_book.live > named) ? " ..." : "");

    return;
}

/**
 * @brief Describes a run that fell short: the case, the allocation it
 *        refused, what fell short, and the run's status and first line of
 *        error output.
 *
 * @param[in] _case   the case.
 * @param[in] _at     the allocation the run refused.
 * @param[in] _total  how many the case makes.
 * @param[in] _fault  what fell short.
 */
static void
d_tests_cli_sweep_describe(
    const struct d_tests_cli_sweep_case* _case,
    size_t                               _at,
    size_t                               _total,
    const char*                          _fault
)
{
    const struct d_tests_cli_sweep_result* const now = &d_tests_cli_sweep_now;

    printf("    [%s] refusing allocation %lu of %lu: %s\n"
           "        status %d (clean %d); err: %.*s\n",
           _case->tag,
           (unsigned long)_at,
           (unsigned long)_total,
           _fault,
           now->status,
           d_tests_cli_sweep_clean.status,
           (int)d_tests_cli_sweep_line(now->err.data),
           now->err.data);

    if (d_tests_cli_sweep_book.live != 0u)
    {
        d_tests_cli_sweep_name_live();
    }

    // everything both runs wrote, asked for
    if (getenv("D_TESTS_CLI_SWEEP_SHOW") != NULL)
    {
        printf("        clean out:\n%s\n        clean err:\n%s\n"
               "        clean result:\n%s\n"
               "        out:\n%s\n        err:\n%s\n        result:\n%s\n",
               d_tests_cli_sweep_clean.out.data,
               d_tests_cli_sweep_clean.err.data,
               d_tests_cli_sweep_clean.extra.data,
               now->out.data,
               now->err.data,
               now->extra.data);
    }

    return;
}

/**
 * @brief Runs a case as it is, and holds the clean run to its case: the
 *        status the case gives, nothing left live, nothing broken.
 *
 * @param[in]  _case   the case.
 * @param[out] _total  receives how many allocations the run made.
 * @return whether the clean run held.
 */
static bool
d_tests_cli_sweep_clean_run(
    const struct d_tests_cli_sweep_case* _case,
    size_t*                              _total
)
{
    const struct d_tests_cli_sweep_result* const clean =
        &d_tests_cli_sweep_clean;

    d_tests_cli_sweep_reset(0u);
    _case->run(_case,
               &d_tests_cli_sweep_clean);
    *_total = d_tests_cli_sweep_book.asked;

    // what the case says, whole
    if ( (clean->status == _case->status)  &&
         (d_tests_cli_sweep_book.live == 0u) &&
         (!d_tests_cli_sweep_book.full)      &&
         (clean->broken == NULL)             &&
         (!clean->out.over)                  &&
         (!clean->err.over)                  &&
         (!clean->extra.over) )
    {
        return true;
    }

    printf("    [%s] the clean run: status %d, not %d; %lu blocks live; "
           "%s\n        err: %.*s\n",
           _case->tag,
           clean->status,
           _case->status,
           (unsigned long)d_tests_cli_sweep_book.live,
           (clean->broken != NULL) ? clean->broken : "nothing broken",
           (int)d_tests_cli_sweep_line(clean->err.data),
           clean->err.data);

    return false;
}

/**
 * @brief Sweeps a case: run as it is, then once for each allocation, that
 *        one refused; describes the first few runs that fell short, and
 *        writes a line for the case.
 *
 * @param[in] _case  the case.
 * @return whether every run held.
 */
static bool
d_tests_cli_sweep_case(
    const struct d_tests_cli_sweep_case* _case
)
{
    size_t total = 0u;
    size_t short_runs = 0u;

    // the clean run, which every other is held to
    if (!d_tests_cli_sweep_clean_run(_case,
                                     &total))
    {
        return false;
    }

    // each allocation refused in turn
    for (size_t at = 1u; at <= total; ++at)
    {
        d_tests_cli_sweep_reset(at);
        _case->run(_case,
                   &d_tests_cli_sweep_now);

        const char* const fault = d_tests_cli_sweep_fault(_case);

        if (fault != NULL)
        {
            if (short_runs < D_TESTS_CLI_SWEEP_SHOWN)
            {
                d_tests_cli_sweep_describe(_case,
                                           at,
                                           total,
                                           fault);
            }

            ++short_runs;
        }
    }

    d_tests_cli_sweep_reset(0u);
    printf("    %s: %lu allocations, each refused in turn; %lu fell short\n",
           _case->tag,
           (unsigned long)total,
           (unsigned long)short_runs);

    return (short_runs == 0u);
}

/**
 * @brief Sweeps each case of a table, every one whatever came before.
 *
 * @param[in] _cases  the table.
 * @param[in] _count  how many cases it has.
 * @return whether every case held.
 */
static bool
d_tests_cli_sweep_cases(
    const struct d_tests_cli_sweep_case* _cases,
    size_t                               _count
)
{
    bool held = true;

    for (size_t index = 0u; index < _count; ++index)
    {
        held = d_tests_cli_sweep_case(&_cases[index]) && held;
    }

    return held;
}


//==============================================================================
// 1.  MAKING
//==============================================================================

// d_tests_cli_sweep_left_behind
//   variable: how many commands a run's fixture asked for that came back
// NULL yet were left among their parent's, or came back made yet were not.
static size_t d_tests_cli_sweep_left_behind;

/**
 * @brief A context to give a command: a block, made with the wrapped
 *        malloc, holding 7.
 *
 * @return the block; NULL when its allocation is the one refused, and then
 *         the command has none.
 */
static void*
d_tests_cli_sweep_context(void)
{
    int* const context = malloc(sizeof(*context));

    if (context != NULL)
    {
        *context = 7;
    }

    return context;
}

/**
 * @brief Whether a command's work was given its context as it was made:
 *        none, or the 7 it holds.
 *
 * @param[in] _context  the context.
 * @return whether it is as given.
 */
static bool
d_tests_cli_sweep_given(
    const void* _context
)
{
    return ( (!_context) ||
             (*(const int*)_context == 7) );
}

/**
 * @brief Adds a command with a context of its own, released with the
 *        program, and holds the call to "a refused command leaves
 *        nothing": made, its parent has one more; refused, as many as
 *        before.
 *
 * @param[in,out] _program      the program.
 * @param[in,out] _parent       the parent; NULL for the root.
 * @param[in]     _name         the command's name.
 * @param[in]     _description  what it does.
 * @param[in]     _run          its work; NULL for none, and then no
 *                              context.
 * @return the command, or NULL.
 */
static struct d_arg_command*
d_tests_cli_sweep_command(
    struct d_cli_program* _program,
    struct d_arg_command* _parent,
    const char*           _name,
    const char*           _description,
    d_cli_command_fn      _run
)
{
    const struct d_arg_command* const parent  = (_parent != NULL)
                                                ? _parent
                                                : &_program->parser->command;
    const size_t                      before  = parent->subcommands.count;
    struct d_arg_command* const       command = d_cli_program_add_command(
        _program,
        _parent,
        _name,
        _description,
        _run,
        (_run != NULL) ? d_tests_cli_sweep_context() : NULL,
        (_run != NULL) ? free : NULL);

    // a refused command kept, or a made one missing
    if (parent->subcommands.count !=
        ((command != NULL) ? (before + 1u) : before))
    {
        ++d_tests_cli_sweep_left_behind;
    }

    return command;
}

/**
 * @brief Adds a required operand to a command.
 *
 * @param[in,out] _command      the command; NULL adds nothing.
 * @param[in]     _name         the operand's name.
 * @param[in]     _description  what it is.
 * @return whether it was added.
 */
static bool
d_tests_cli_sweep_operand(
    struct d_arg_command* _command,
    const char*           _name,
    const char*           _description
)
{
    struct d_arg_option* const operand = (_command != NULL)
                                         ? d_arg_command_add_option(
                                               _command,
                                               _name,
                                               _description)
                                         : NULL;

    if (operand != NULL)
    {
        d_arg_option_set_required(operand,
                                  true);
    }

    return (operand != NULL);
}

/**
 * @brief `show`'s work: what each of its options holds.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `show`.
 * @param[in] _context  its context.
 * @return D_CLI_EXIT_OK; D_TESTS_CLI_SWEEP_LOST when the context is not as
 *         given.
 */
static int
d_tests_cli_sweep_work_show(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    char sheet[64];
    char define[128];
    char level[32];
    char out[64];
    char width[32];

    if (!d_tests_cli_sweep_given(_context))
    {
        return D_TESTS_CLI_SWEEP_LOST;
    }

    d_tests_cli_print(_program,
                      "show %s all=%lu define=%s level=%s out=%s width=%s\n",
                      d_tests_cli_values(_command, "SHEET", sheet,
                                         sizeof(sheet)),
                      (unsigned long)d_arg_option_count(
                          d_arg_command_option(_command,
                                               "--all")),
                      d_tests_cli_values(_command, "--define", define,
                                         sizeof(define)),
                      d_tests_cli_values(_command, "--level", level,
                                         sizeof(level)),
                      d_tests_cli_values(_command, "--out", out,
                                         sizeof(out)),
                      d_tests_cli_values(_command, "--width", width,
                                         sizeof(width)));

    return D_CLI_EXIT_OK;
}

/**
 * @brief `check`'s work: a warning at a place in the sheet, a note at each
 *        of two levels, and the answer no.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `check`.
 * @param[in] _context  its context.
 * @return D_CLI_EXIT_NO; D_TESTS_CLI_SWEEP_LOST when the context is not as
 *         given.
 */
static int
d_tests_cli_sweep_work_check(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    char sheet[64];

    if (!d_tests_cli_sweep_given(_context))
    {
        return D_TESTS_CLI_SWEEP_LOST;
    }

    d_cli_report_at(_program,
                    (int)D_CLI_SEVERITY_WARNING,
                    d_tests_cli_values(_command,
                                       "SHEET",
                                       sheet,
                                       sizeof(sheet)),
                    3L,
                    7L,
                    "row %d shares a bit",
                    2);
    d_cli_note(_program,
               1,
               "checked %s",
               sheet);
    d_cli_note(_program,
               2,
               "every row read");

    return D_CLI_EXIT_NO;
}

/**
 * @brief The work of `group`'s subcommands: the command's path, which takes
 *        memory to make -- and, without it, trouble, said.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command.
 * @param[in] _context  its context.
 * @return D_CLI_EXIT_OK; D_CLI_EXIT_TROUBLE without memory for the path;
 *         D_TESTS_CLI_SWEEP_LOST when the context is not as given.
 */
static int
d_tests_cli_sweep_work_named(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    struct d_arg_buffer path   = D_ARG_BUFFER_INIT;
    int                 status = D_CLI_EXIT_OK;

    if (!d_tests_cli_sweep_given(_context))
    {
        return D_TESTS_CLI_SWEEP_LOST;
    }

    // the path, or the want of it said
    if ( (d_cli_command_path(_command,
                             &path) != D_ARG_OK) ||
         (path.data == NULL) )
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "out of memory");
        status = D_CLI_EXIT_TROUBLE;
    }
    else
    {
        d_tests_cli_print(_program,
                          "%s\n",
                          path.data);
    }

    d_arg_buffer_release(&path);

    return status;
}

/**
 * @brief `fail`'s work: a usage error of its own.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `fail`.
 * @param[in] _context  its context.
 * @return D_CLI_EXIT_TROUBLE.
 */
static int
d_tests_cli_sweep_work_fail(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    if (!d_tests_cli_sweep_given(_context))
    {
        return D_TESTS_CLI_SWEEP_LOST;
    }

    return d_cli_usage_error(_program,
                             _command,
                             "--x: '%s' is not a number",
                             "abc");
}

/**
 * @brief `open`'s work: the file named could not be read.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `open`.
 * @param[in] _context  its context.
 * @return D_CLI_EXIT_TROUBLE.
 */
static int
d_tests_cli_sweep_work_open(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    char file[64];

    if (!d_tests_cli_sweep_given(_context))
    {
        return D_TESTS_CLI_SWEEP_LOST;
    }

    return d_cli_file_error(_program,
                            "read",
                            d_tests_cli_values(_command,
                                               "FILE",
                                               file,
                                               sizeof(file)),
                            ENOENT);
}

/**
 * @brief The root's own options: `-D NAME`, each use a name kept, and
 *        `--level=N`; and an option group, `Output`, with `--width=N`.
 *
 * @param[in,out] _root  the root.
 * @return whether they were made.
 */
static bool
d_tests_cli_sweep_root(
    struct d_arg_command* _root
)
{
    struct d_arg_option* const  define = d_arg_command_add_option(
                                             _root,
                                             "-D,--define",
                                             "define NAME for every command");
    struct d_arg_option* const  level  = d_arg_command_add_option(
                                             _root,
                                             "--level",
                                             "how far to go");
    struct d_arg_command* const output = d_arg_command_add_option_group(
                                             _root,
                                             "Output",
                                             "how the output looks");

    // made
    if ( (!define) ||
         (!level)  ||
         (!output) )
    {
        return false;
    }

    // a list: any number of names, each use one of them
    d_arg_option_set_expected_range(define,
                                    1,
                                    -1);
    d_arg_option_set_multi_option_policy(define,
                                         (int)D_ARG_MULTI_TAKE_ALL);

    return ( (d_arg_option_set_type_name(define,
                                         "NAME") == D_ARG_OK)         &&
             (d_arg_option_set_type_name(level,
                                         "N") == D_ARG_OK)            &&
             (d_arg_option_set_default_text(level,
                                            "3") == D_ARG_OK)         &&
             (d_arg_command_add_option(output,
                                       "--width",
                                       "the columns to fill") != NULL) );
}

/**
 * @brief `show`, its operand, `--all` and `-o FILE`; `check` and its
 *        operand.
 *
 * @param[in,out] _program  the program.
 * @return whether they were made.
 */
static bool
d_tests_cli_sweep_reading(
    struct d_cli_program* _program
)
{
    struct d_arg_command* const show = d_tests_cli_sweep_command(
                                           _program,
                                           NULL,
                                           "show",
                                           "list what a sheet holds",
                                           d_tests_cli_sweep_work_show);
    struct d_arg_option* const  out  = (show != NULL)
                                       ? d_arg_command_add_option(
                                             show,
                                             "-o,--out",
                                             "write the list to FILE")
                                       : NULL;

    return ( (d_tests_cli_sweep_operand(show,
                                        "SHEET",
                                        "the sheet to list"))           &&
             (d_arg_command_add_flag(show,
                                     "--all",
                                     "every row, hidden ones too") != NULL) &&
             (out != NULL)                                              &&
             (d_arg_option_set_type_name(out,
                                         "FILE") == D_ARG_OK)           &&
             (d_tests_cli_sweep_operand(d_tests_cli_sweep_command(
                                            _program,
                                            NULL,
                                            "check",
                                            "report a sheet's issues",
                                            d_tests_cli_sweep_work_check),
                                        "SHEET",
                                        "the sheet to check")) );
}

/**
 * @brief COMMANDS: a program with commands, `prog 1.2`: the root's options
 *        and option group; `show` and `check`; `group`, with no work and
 *        two subcommands; `fail`; and `open`.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static bool
d_tests_cli_sweep_make_commands(
    struct d_cli_program* _program,
    struct d_arg_parser*  _parser
)
{
    // the program, its root's options, and the reading commands
    if ( (d_cli_program_init(_program,
                             _parser,
                             "prog",
                             "1.2",
                             D_CLI_PROGRAM_COMMANDS) != D_ARG_OK) ||
         (!d_tests_cli_sweep_root(&_parser->command))             ||
         (!d_tests_cli_sweep_reading(_program)) )
    {
        return false;
    }

    struct d_arg_command* const group = d_tests_cli_sweep_command(
                                            _program,
                                            NULL,
                                            "group",
                                            "hold commands of its own",
                                            NULL);

    return ( (group != NULL)                                       &&
             (d_tests_cli_sweep_command(
                  _program,
                  group,
                  "one",
                  "the group's first command",
                  d_tests_cli_sweep_work_named) != NULL)           &&
             (d_tests_cli_sweep_command(
                  _program,
                  group,
                  "two",
                  "the group's second command",
                  d_tests_cli_sweep_work_named) != NULL)           &&
             (d_tests_cli_sweep_command(
                  _program,
                  NULL,
                  "fail",
                  "end with a usage error of its own",
                  d_tests_cli_sweep_work_fail) != NULL)            &&
             (d_tests_cli_sweep_operand(d_tests_cli_sweep_command(
                                            _program,
                                            NULL,
                                            "open",
                                            "read a file",
                                            d_tests_cli_sweep_work_open),
                                        "FILE",
                                        "the file to read")) );
}

/**
 * @brief SINGLE's work: its files, and how often `-n` was given.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the root.
 * @param[in] _context  its context.
 * @return D_CLI_EXIT_OK; D_TESTS_CLI_SWEEP_LOST when the context is not as
 *         given.
 */
static int
d_tests_cli_sweep_work_main(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    char files[128];

    if (!d_tests_cli_sweep_given(_context))
    {
        return D_TESTS_CLI_SWEEP_LOST;
    }

    d_tests_cli_print(_program,
                      "files=%s n=%lu\n",
                      d_tests_cli_values(_command,
                                         "FILE",
                                         files,
                                         sizeof(files)),
                      (unsigned long)d_arg_option_count(
                          d_arg_command_option(_command,
                                               "-n")));

    return D_CLI_EXIT_OK;
}

/**
 * @brief SINGLE: a program of one command, with no version: its root's
 *        work, any number of files, and `-n`.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static bool
d_tests_cli_sweep_make_single(
    struct d_cli_program* _program,
    struct d_arg_parser*  _parser
)
{
    // the program
    if (d_cli_program_init(_program,
                           _parser,
                           "prog",
                           NULL,
                           0u) != D_ARG_OK)
    {
        return false;
    }

    struct d_arg_option* const files = d_arg_command_add_option(
                                           &_parser->command,
                                           "FILE",
                                           "the files to read");

    if (!files)
    {
        return false;
    }

    d_arg_option_set_expected_range(files,
                                    0,
                                    -1);

    return ( (d_arg_command_add_flag(&_parser->command,
                                     "-n,--dry-run",
                                     "say what would be done") != NULL) &&
             (d_cli_program_set_main(_program,
                                     d_tests_cli_sweep_work_main,
                                     d_tests_cli_sweep_context(),
                                     free) == D_ARG_OK) );
}

// D_TESTS_CLI_SWEEP_HOST
//   constant: the host a module's program is made from: `prog 1.2`, its
// command `prog gui`, started at verbosity 1, colouring as `auto` does.
static const struct d_cli_plugin_host D_TESTS_CLI_SWEEP_HOST =
{
    D_CLI_PLUGIN_ABI,
    (uint32_t)sizeof(struct d_cli_plugin_host),
    "prog",
    "1.2",
    "prog gui",
    NULL,
    NULL,
    1,
    0
};

/**
 * @brief A window's work: its verbosity, and how often `--shout` was
 *        given.
 *
 * @param[in] _program  the module's program.
 * @param[in] _command  its root.
 * @param[in] _context  its context.
 * @return D_CLI_EXIT_OK; D_TESTS_CLI_SWEEP_LOST when the context is not as
 *         given.
 */
static int
d_tests_cli_sweep_work_window(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    if (!d_tests_cli_sweep_given(_context))
    {
        return D_TESTS_CLI_SWEEP_LOST;
    }

    d_tests_cli_print(_program,
                      "window verbosity=%d shout=%lu\n",
                      d_cli_program_verbosity(_program),
                      (unsigned long)d_arg_option_count(
                          d_arg_command_option(_command,
                                               "--shout")));

    return D_CLI_EXIT_OK;
}

/**
 * @brief MODULE: a module's program, made from D_TESTS_CLI_SWEEP_HOST, as
 *        a window makes its own: its root's work, and `--shout`.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser, named as the host's command.
 * @return whether it was made.
 */
static bool
d_tests_cli_sweep_make_module(
    struct d_cli_program* _program,
    struct d_arg_parser*  _parser
)
{
    return ( (d_cli_program_init_plugin(_program,
                                        _parser,
                                        &D_TESTS_CLI_SWEEP_HOST,
                                        0u) == D_ARG_OK)               &&
             (d_arg_command_add_flag(&_parser->command,
                                     "--shout",
                                     "say it louder") != NULL)        &&
             (d_cli_program_set_main(_program,
                                     d_tests_cli_sweep_work_window,
                                     d_tests_cli_sweep_context(),
                                     free) == D_ARG_OK) );
}

/**
 * @brief HOST: a program with a plugin command for PROBE, one for OTHER,
 *        which belongs to another program, and one for a module never
 *        built.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static bool
d_tests_cli_sweep_make_host(
    struct d_cli_program* _program,
    struct d_arg_parser*  _parser
)
{
    return ( (d_cli_program_init(_program,
                                 _parser,
                                 "prog",
                                 "1.2",
                                 D_CLI_PROGRAM_COMMANDS) == D_ARG_OK) &&
             (d_cli_program_add_plugin(_program,
                                       NULL,
                                       "probe",
                                       "a probe") != NULL)            &&
             (d_cli_program_add_plugin(_program,
                                       NULL,
                                       "other",
                                       "another program's") != NULL)  &&
             (d_cli_program_add_plugin(_program,
                                       NULL,
                                       "missing",
                                       "never built") != NULL) );
}

/**
 * @brief What a program's main does with a fixture, as the kit means a
 *        main to: the parser, named as the line's first word; the program,
 *        whatever of it could be made; its run on the line -- which, where
 *        the making failed, ends in trouble and says why; and both
 *        released.
 *
 * @param[in]     _make    the fixture.
 * @param[in]     _words   the line, split.
 * @param[in]     _run     whether to run a program made whole, or only
 *                         make it.
 * @param[in,out] _result  receives what the run wrote.
 * @return the status the program exits with; D_CLI_EXIT_OK for a program
 *         made whole and not run.
 */
static int
d_tests_cli_sweep_main(
    d_tests_cli_sweep_make_fn        _make,
    const struct d_tests_cli_words*  _words,
    bool                             _run,
    struct d_tests_cli_sweep_result* _result
)
{
    struct d_cli_program program;
    struct d_arg_parser  parser;
    int                  status = D_CLI_EXIT_OK;

    memset(&program,
           0,
           sizeof(program));
    // the parser, which can be released, and parsed, even when its making
    // fails; then the program
    (void)d_arg_parser_init(&parser,
                            "a program swept",
                            _words->items[0]);

    const bool made = _make(&program,
                            &parser);

    d_cli_program_set_output(&program,
                             d_tests_cli_sweep_sink(&_result->out),
                             d_tests_cli_sweep_sink(&_result->err));
    d_arg_formatter_set_line_width(&parser.formatter,
                                   (size_t)D_TESTS_CLI_WIDTH);

    // the run, which says why where the making failed
    if ( (_run) ||
         (!made) )
    {
        status = d_cli_program_run(&program,
                                   _words->count,
                                   _words->items);
    }

    d_cli_program_release(&program);
    d_arg_parser_release(&parser);

    return status;
}

/**
 * @brief Runs a program the way a case says: made, and run on the case's
 *        line unless the case only makes it; each command its fixture asked
 *        for held to "a refused command leaves nothing".
 *
 * @param[in]  _case    the case.
 * @param[in]  _run     whether to run the program.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_program(
    const struct d_tests_cli_sweep_case* _case,
    bool                                 _run,
    struct d_tests_cli_sweep_result*     _result
)
{
    struct d_tests_cli_words words;

    d_tests_cli_sweep_start(_result);

    // a line the table spelled wrong is no case at all
    if (!d_tests_cli_split(_case->line,
                           &words))
    {
        _result->broken = "the case's line does not split";

        return;
    }

    d_tests_cli_sweep_left_behind = 0u;
    d_tests_cli_sweep_arm(true);
    _result->status = d_tests_cli_sweep_main(_case->make,
                                             &words,
                                             _run,
                                             _result);
    d_tests_cli_sweep_arm(false);

    if (d_tests_cli_sweep_left_behind != 0u)
    {
        _result->broken = "a refused command left behind";
    }

    return;
}

/**
 * @brief A case's run that makes its program and releases it, unrun.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_run_made(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    d_tests_cli_sweep_program(_case,
                              false,
                              _result);

    return;
}

/**
 * @brief A case's run that makes its program and runs it on its line.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_run_line(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    d_tests_cli_sweep_program(_case,
                              true,
                              _result);

    return;
}

// MAKING
//   constant: each fixture made and released, never run.
static const struct d_tests_cli_sweep_case MAKING[] =
{
    { "M01", "prog",       d_tests_cli_sweep_make_commands,
      d_tests_cli_sweep_run_made, D_CLI_EXIT_OK },
    { "M02", "prog",       d_tests_cli_sweep_make_single,
      d_tests_cli_sweep_run_made, D_CLI_EXIT_OK },
    { "M03", "'prog gui'", d_tests_cli_sweep_make_module,
      d_tests_cli_sweep_run_made, D_CLI_EXIT_OK },
    { "M04", "prog",       d_tests_cli_sweep_make_host,
      d_tests_cli_sweep_run_made, D_CLI_EXIT_OK }
};

/*
d_tests_cli_sweep_making
  MAKING: every fixture made and released, every allocation refused in turn.
*/
bool
d_tests_cli_sweep_making(void)
{
    return d_tests_cli_sweep_cases(MAKING,
                                   D_TESTS_CLI_SWEEP_COUNT(MAKING));
}


//==============================================================================
// 2.  ENDINGS
//==============================================================================

// D_TESTS_CLI_SWEEP_LINE
//   macro: a case that runs a fixture on a line, and the status its clean
// run gives.
#define D_TESTS_CLI_SWEEP_LINE(_tag, _line, _make, _status)                    \
    { (_tag), (_line), d_tests_cli_sweep_make_##_make,                         \
      d_tests_cli_sweep_run_line, (_status) }

// ENDINGS
//   constant: the lines a parse ends: help, a version, a completion script,
// and the failures the kit words.
static const struct d_tests_cli_sweep_case ENDINGS[] =
{
    D_TESTS_CLI_SWEEP_LINE("E01", "prog --help",          commands, 0),
    D_TESTS_CLI_SWEEP_LINE("E02", "prog -h",              commands, 0),
    D_TESTS_CLI_SWEEP_LINE("E03", "prog help",            commands, 0),
    D_TESTS_CLI_SWEEP_LINE("E04", "prog help show",       commands, 0),
    D_TESTS_CLI_SWEEP_LINE("E05", "prog help group one",  commands, 0),
    D_TESTS_CLI_SWEEP_LINE("E06", "prog show --help",     commands, 0),
    D_TESTS_CLI_SWEEP_LINE("E07", "prog group --help",    commands, 0),
    D_TESTS_CLI_SWEEP_LINE("E08", "prog --version",       commands, 0),
#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
    D_TESTS_CLI_SWEEP_LINE("E09", "prog completion bash", commands, 0),
#endif
#if (D_ENV_CLI_HELP_SLASH == 1)
    D_TESTS_CLI_SWEEP_LINE("E10", "prog /?",              commands, 0),
#endif
    D_TESTS_CLI_SWEEP_LINE("E11", "prog",                 commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E12", "prog frob",            commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E13", "prog frob a b",        commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E14", "prog shwo s.json",     commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E15", "prog --frob",          commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E16", "prog show",            commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E17", "prog show a b",        commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E18", "prog --level",         commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E19", "prog --color=sometimes show a",
                           commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E20", "prog group",           commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E21", "prog -- show a",       commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E22", "prog help frob",       commands, 2),
    D_TESTS_CLI_SWEEP_LINE("E23", "prog --help",          single,   0),
    D_TESTS_CLI_SWEEP_LINE("E24", "prog -x",              single,   2),
    D_TESTS_CLI_SWEEP_LINE("E25", "'prog gui' --help",    module,   0),
    D_TESTS_CLI_SWEEP_LINE("E26", "'prog gui' --version", module,   0),
    D_TESTS_CLI_SWEEP_LINE("E27", "'prog gui' extra",     module,   2)
};

/*
d_tests_cli_sweep_endings
  ENDINGS: each line, every allocation refused in turn.
*/
bool
d_tests_cli_sweep_endings(void)
{
    return d_tests_cli_sweep_cases(ENDINGS,
                                   D_TESTS_CLI_SWEEP_COUNT(ENDINGS));
}


//==============================================================================
// 3.  WORK
//==============================================================================

// WORK
//   constant: the lines that reach a command's work: its output, its
// reports at each verbosity and in colour, its path, its own usage error and
// file error; a program of one command; a module's program.
static const struct d_tests_cli_sweep_case WORK[] =
{
    D_TESTS_CLI_SWEEP_LINE("W01", "prog show s.json -D a -D b --level 4 "
                                  "--all -o out.txt --width 9",
                           commands, 0),
    D_TESTS_CLI_SWEEP_LINE("W02", "prog check s.json",          commands, 1),
    D_TESTS_CLI_SWEEP_LINE("W03", "prog -v check s.json",       commands, 1),
    D_TESTS_CLI_SWEEP_LINE("W04", "prog -vv check s.json",      commands, 1),
    D_TESTS_CLI_SWEEP_LINE("W05", "prog -q check s.json",       commands, 1),
    D_TESTS_CLI_SWEEP_LINE("W06", "prog --color=always check s.json",
                           commands, 1),
    D_TESTS_CLI_SWEEP_LINE("W07", "prog group one",             commands, 0),
    D_TESTS_CLI_SWEEP_LINE("W08", "prog group two",             commands, 0),
    D_TESTS_CLI_SWEEP_LINE("W09", "prog fail",                  commands, 2),
    D_TESTS_CLI_SWEEP_LINE("W10", "prog open nothing.txt",      commands, 2),
    D_TESTS_CLI_SWEEP_LINE("W11", "prog a b -n",                single,   0),
    D_TESTS_CLI_SWEEP_LINE("W12", "'prog gui' --shout",         module,   0),
    D_TESTS_CLI_SWEEP_LINE("W13", "'prog gui' -v",              module,   0)
};

/*
d_tests_cli_sweep_work
  WORK: each line, every allocation refused in turn.
*/
bool
d_tests_cli_sweep_work(void)
{
    return d_tests_cli_sweep_cases(WORK,
                                   D_TESTS_CLI_SWEEP_COUNT(WORK));
}


//==============================================================================
// 4.  FILES
//==============================================================================

// D_TESTS_CLI_SWEEP_OLD / D_TESTS_CLI_SWEEP_NEW
//   constants: what a file holds before a write, and what the write gives
// it.
#define D_TESTS_CLI_SWEEP_OLD  "the old file\n"
#define D_TESTS_CLI_SWEEP_NEW  "the new file, longer than the old\n"

// D_TESTS_CLI_SWEEP_BYTES
//   constant: what a file read whole holds: a NUL, a CRLF and a byte that is
// no UTF-8 among its bytes.
static const char D_TESTS_CLI_SWEEP_BYTES[] = "one\r\ntwo\n\0three\xff\n";

/**
 * @brief Writes a file with the C library, outside any run's measured part.
 *
 * @param[in] _name    the file's name.
 * @param[in] _data    its bytes.
 * @param[in] _length  how many.
 * @return whether it was written whole.
 */
static bool
d_tests_cli_sweep_put(
    const char* _name,
    const char* _data,
    size_t      _length
)
{
    FILE* const file    = fopen(_name,
                                "wb");
    bool        written = false;

    if (file != NULL)
    {
        written = (fwrite(_data,
                          1u,
                          _length,
                          file) == _length);
        written = (fclose(file) == 0) && written;
    }

    return written;
}

/**
 * @brief Reads a file with the C library into a text, outside any run's
 *        measured part.
 *
 * @param[in]  _name  the file's name.
 * @param[out] _text  receives its bytes, as many as fit.
 * @return whether the file was there.
 */
static bool
d_tests_cli_sweep_get(
    const char*                    _name,
    struct d_tests_cli_sweep_text* _text
)
{
    char        block[512];
    FILE* const file = fopen(_name,
                             "rb");
    size_t      read = 0u;

    d_tests_cli_sweep_empty(_text);

    if (!file)
    {
        return false;
    }

    // block by block, to the end
    do
    {
        read = fread(block,
                     1u,
                     sizeof(block),
                     file);
        (void)d_tests_cli_sweep_write(_text,
                                      block,
                                      read);
    } while (read == sizeof(block));

    (void)fclose(file);

    return true;
}

/**
 * @brief Whether the working directory holds a file a write left beside
 *        the one it wrote: a name with `.~` in it, as d_cli_write_file
 *        names the file it writes before renaming it.
 *
 * @return whether such a file is there.
 */
static bool
d_tests_cli_sweep_leftover(void)
{
    bool found = false;

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    struct _finddata_t entry;
    const intptr_t     search = _findfirst("*",
                                           &entry);

    for (int more = (search != -1) ? 0 : -1;
         (more == 0) && (!found);
         more = _findnext(search,
                          &entry))
    {
        found = (strstr(entry.name,
                        ".~") != NULL);
    }

    if (search != -1)
    {
        (void)_findclose(search);
    }
#else
    DIR* const           directory = opendir(".");
    const struct dirent* entry     = NULL;

    while ( (directory != NULL)                     &&
            (!found)                                &&
            ((entry = readdir(directory)) != NULL) )
    {
        found = (strstr(entry->d_name,
                        ".~") != NULL);
    }

    if (directory != NULL)
    {
        (void)closedir(directory);
    }
#endif

    return found;
}

/**
 * @brief Holds what a write left: the file there, holding the new bytes if
 *        the write said so and the old ones if it did not -- or, for a file
 *        that was not there before, nothing -- and nothing beside it.
 *
 * @param[in]     _name     the file written.
 * @param[in]     _written  whether the write said it was written.
 * @param[in]     _old      what it held before; NULL for no file.
 * @param[in,out] _result   receives the file's bytes, and what is broken.
 */
static void
d_tests_cli_sweep_written(
    const char*                      _name,
    bool                             _written,
    const char*                      _old,
    struct d_tests_cli_sweep_result* _result
)
{
    const char* const want  = (_written) ? D_TESTS_CLI_SWEEP_NEW : _old;
    const bool        there = d_tests_cli_sweep_get(_name,
                                                    &_result->extra);

    // the new file or the old, whole; or no file, as there was none
    if ( (there != (want != NULL)) ||
         ( (want != NULL) &&
           ( (_result->extra.length != strlen(want)) ||
             (memcmp(_result->extra.data,
                     want,
                     _result->extra.length) != 0) ) ) )
    {
        _result->broken = "the file is neither the new one nor the old";
    }
    else if (d_tests_cli_sweep_leftover())
    {
        _result->broken = "a file left beside the one written";
    }

    return;
}

/**
 * @brief F01: a file written over: d_cli_write_file on `target.txt`, which
 *        holds D_TESTS_CLI_SWEEP_OLD.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_run_over(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    d_tests_cli_sweep_start(_result);
    (void)d_tests_cli_sweep_put(_case->line,
                                D_TESTS_CLI_SWEEP_OLD,
                                strlen(D_TESTS_CLI_SWEEP_OLD));

    d_tests_cli_sweep_arm(true);
    const int written = d_cli_write_file(_case->line,
                                         D_TESTS_CLI_SWEEP_NEW,
                                         strlen(D_TESTS_CLI_SWEEP_NEW));
    d_tests_cli_sweep_arm(false);

    _result->status = (written == 0) ? D_CLI_EXIT_OK : D_CLI_EXIT_TROUBLE;
    d_tests_cli_sweep_written(_case->line,
                              (written == 0),
                              D_TESTS_CLI_SWEEP_OLD,
                              _result);

    return;
}

/**
 * @brief F02: a file written that was not there: d_cli_write_file on
 *        `fresh.txt`, removed first, and removed again after.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_run_fresh(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    d_tests_cli_sweep_start(_result);
    (void)remove(_case->line);

    d_tests_cli_sweep_arm(true);
    const int written = d_cli_write_file(_case->line,
                                         D_TESTS_CLI_SWEEP_NEW,
                                         strlen(D_TESTS_CLI_SWEEP_NEW));
    d_tests_cli_sweep_arm(false);

    _result->status = (written == 0) ? D_CLI_EXIT_OK : D_CLI_EXIT_TROUBLE;
    d_tests_cli_sweep_written(_case->line,
                              (written == 0),
                              NULL,
                              _result);
    (void)remove(_case->line);

    return;
}

/**
 * @brief F03 and F04: a file read whole: d_cli_read_file on the case's
 *        name, `target.txt` holding D_TESTS_CLI_SWEEP_BYTES or a name that
 *        is not there; its bytes kept, then freed with d_cli_free.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to: the bytes as the result; a
 *                      failure that is neither the file's absence nor
 *                      ENOMEM, or that gave bytes, broken.
 */
static void
d_tests_cli_sweep_run_read(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    char*  data   = NULL;
    size_t length = 0u;

    d_tests_cli_sweep_start(_result);
    (void)d_tests_cli_sweep_put("target.txt",
                                D_TESTS_CLI_SWEEP_BYTES,
                                sizeof(D_TESTS_CLI_SWEEP_BYTES) - 1u);

    d_tests_cli_sweep_arm(true);
    const int read = d_cli_read_file(_case->line,
                                     &data,
                                     &length);

    d_tests_cli_sweep_keep(&_result->extra,
                           data,
                           length);
    d_cli_free(data);
    d_tests_cli_sweep_arm(false);

    _result->status = (read == 0) ? D_CLI_EXIT_OK : D_CLI_EXIT_TROUBLE;

    // a failure gives nothing, and is the file's absence or the memory's
    if ( (read != 0) &&
         ( (data != NULL)    ||
           (length != 0u)    ||
           ( (read != ENOMEM) &&
             (read != ENOENT) ) ) )
    {
        _result->broken = "a failed read that gave bytes, or no reason";
    }

    return;
}

#if (D_TESTS_ARG_FILES_POSIX == 1)

/**
 * @brief F05: a file written through a symbolic link: d_cli_write_file on
 *        `link.txt`, which points at `target.txt`, holding
 *        D_TESTS_CLI_SWEEP_OLD: the target written, the link still a link.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_run_link(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    struct stat status;

    d_tests_cli_sweep_start(_result);
    (void)d_tests_cli_sweep_put("target.txt",
                                D_TESTS_CLI_SWEEP_OLD,
                                strlen(D_TESTS_CLI_SWEEP_OLD));

    d_tests_cli_sweep_arm(true);
    const int written = d_cli_write_file(_case->line,
                                         D_TESTS_CLI_SWEEP_NEW,
                                         strlen(D_TESTS_CLI_SWEEP_NEW));
    d_tests_cli_sweep_arm(false);

    _result->status = (written == 0) ? D_CLI_EXIT_OK : D_CLI_EXIT_TROUBLE;
    d_tests_cli_sweep_written("target.txt",
                              (written == 0),
                              D_TESTS_CLI_SWEEP_OLD,
                              _result);

    // the link left a link
    if ( (lstat(_case->line,
                &status) != 0) ||
         (!S_ISLNK(status.st_mode)) )
    {
        _result->broken = "the link is a link no more";
    }

    return;
}

#endif  // D_TESTS_ARG_FILES_POSIX

// FILES
//   constant: a file written over, written new, read whole, not there to
// read, and written through a link.
static const struct d_tests_cli_sweep_case FILES[] =
{
    { "F01", "target.txt", NULL, d_tests_cli_sweep_run_over,  0 },
    { "F02", "fresh.txt",  NULL, d_tests_cli_sweep_run_fresh, 0 },
    { "F03", "target.txt", NULL, d_tests_cli_sweep_run_read,  0 },
    { "F04", "absent.txt", NULL, d_tests_cli_sweep_run_read,  2 }
#if (D_TESTS_ARG_FILES_POSIX == 1)
    ,
    { "F05", "link.txt",   NULL, d_tests_cli_sweep_run_link,  0 }
#endif
};

/*
d_tests_cli_sweep_files
  FILES in a scratch directory holding `target.txt` and, where there are
links, `link.txt` pointing at it.
*/
bool
d_tests_cli_sweep_files(void)
{
    struct d_tests_arg_files files;
    bool                     held = d_tests_arg_files_enter(&files);

    held = (held) &&
           (d_tests_arg_files_write(&files,
                                    "target.txt",
                                    D_TESTS_CLI_SWEEP_OLD,
                                    strlen(D_TESTS_CLI_SWEEP_OLD)));
#if (D_TESTS_ARG_FILES_POSIX == 1)
    held = (held) &&
           (d_tests_arg_files_link(&files,
                                   "link.txt",
                                   "target.txt"));
#endif

    if (!held)
    {
        printf("    the scratch directory could not be made\n");
        d_tests_arg_files_leave(&files);

        return false;
    }

    held = d_tests_cli_sweep_cases(FILES,
                                   D_TESTS_CLI_SWEEP_COUNT(FILES));
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 5.  PLUGINS
//==============================================================================

// d_tests_cli_sweep_modules_at / d_tests_cli_sweep_beside
//   variables: the directory the probe modules are in; and a program's file
// in it, `prog`, which finding looks beside.
static char d_tests_cli_sweep_modules_at[D_TESTS_CLI_SWEEP_PATH_MAX];
static char d_tests_cli_sweep_beside[D_TESTS_CLI_SWEEP_PATH_MAX + 8u];

/*
d_tests_cli_sweep_modules
  The directory given, made absolute while the working directory is still
the one it was given from; or the runner's own, its file's name cut off.
*/
void
d_tests_cli_sweep_modules(
    const char* _directory
)
{
    char* found = NULL;

    // the directory the runner was given, or the runner's own
    if (_directory != NULL)
    {
        found = d_path_resolve(_directory,
                               NULL,
                               0u);
    }
    else if (d_cli_executable_path(NULL,
                                   &found) == 0)
    {
        char* const slash = strrchr(found,
                                    '/');
        char* const back  = strrchr(found,
                                    '\\');
        char* const last  = ( (back != NULL) &&
                              ( (slash == NULL) ||
                                (back > slash) ) ) ? back : slash;

        if (last != NULL)
        {
            *last = '\0';
        }
    }

    (void)snprintf(d_tests_cli_sweep_modules_at,
                   sizeof(d_tests_cli_sweep_modules_at),
                   "%s",
                   (found != NULL) ? found
                                   : (_directory != NULL) ? _directory : ".");
    (void)snprintf(d_tests_cli_sweep_beside,
                   sizeof(d_tests_cli_sweep_beside),
                   "%s/prog",
                   d_tests_cli_sweep_modules_at);
    d_cli_free(found);

    return;
}

/**
 * @brief A case's run of HOST on its line, `report.txt` -- what PROBE
 *        writes -- removed before it and kept, as the result, after it.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_run_host(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    (void)remove("report.txt");
    d_tests_cli_sweep_program(_case,
                              true,
                              _result);
    (void)d_tests_cli_sweep_get("report.txt",
                                &_result->extra);

    return;
}

/**
 * @brief D01: finding PROBE's module, beside a program in the modules'
 *        directory: the path found as the result.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_run_find(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    struct d_arg_buffer found    = D_ARG_BUFFER_INIT;
    struct d_arg_buffer searched = D_ARG_BUFFER_INIT;

    d_tests_cli_sweep_start(_result);
    d_tests_cli_sweep_arm(true);

    const int status = d_cli_plugin_find("prog",
                                         _case->line,
                                         d_tests_cli_sweep_beside,
                                         &found,
                                         &searched);

    d_tests_cli_sweep_keep(&_result->extra,
                           found.data,
                           found.length);
    _result->status = d_tests_cli_sweep_status(status,
                                               (found.failed) ||
                                               (searched.failed));
    d_arg_buffer_release(&found);
    d_arg_buffer_release(&searched);
    d_tests_cli_sweep_arm(false);

    return;
}

/**
 * @brief D02: the program's own file: its path as the result.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_run_executable(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    char* path = NULL;

    (void)_case;
    d_tests_cli_sweep_start(_result);
    d_tests_cli_sweep_arm(true);

    const int status = d_cli_executable_path(NULL,
                                             &path);

    d_tests_cli_sweep_keep(&_result->extra,
                           path,
                           (path != NULL) ? strlen(path) : 0u);
    d_cli_free(path);
    d_tests_cli_sweep_arm(false);

    // found, or not for want of memory
    _result->status = (status == 0)      ? D_CLI_EXIT_OK
                    : (status == ENOMEM) ? D_CLI_EXIT_TROUBLE
                                         : D_CLI_EXIT_NO;

    return;
}

/**
 * @brief D03 and D04: a module loaded and unloaded: `prog-NAME` from the
 *        modules' directory, loaded for `prog 1.2`; its name and summary,
 *        or why it was refused, as the result. A load that failed must
 *        leave no module.
 *
 * @param[in]  _case    the case.
 * @param[out] _result  what the run came to.
 */
static void
d_tests_cli_sweep_run_load(
    const struct d_tests_cli_sweep_case* _case,
    struct d_tests_cli_sweep_result*     _result
)
{
    char                path[D_TESTS_CLI_SWEEP_PATH_MAX + 64u];
    struct d_cli_module module = D_CLI_MODULE_INIT;
    struct d_arg_buffer why    = D_ARG_BUFFER_INIT;

    d_tests_cli_sweep_start(_result);
    (void)snprintf(path,
                   sizeof(path),
                   "%s/prog-%s" D_ENV_CLI_MODULE_SUFFIX,
                   d_tests_cli_sweep_modules_at,
                   _case->line);
    d_tests_cli_sweep_arm(true);

    const int status = d_cli_module_load(&module,
                                         path,
                                         "prog",
                                         "1.2",
                                         &why);

    // a failed load leaves nothing loaded
    if ( (status != (int)D_ARG_OK) &&
         ( (module.handle != NULL) ||
           (module.plugin != NULL) ||
           (module.path != NULL) ) )
    {
        _result->broken = "a failed load left a module";
    }

    const char* const said = (module.plugin != NULL) ? module.plugin->name
                                                     : why.data;

    d_tests_cli_sweep_keep(&_result->extra,
                           said,
                           (said != NULL) ? strlen(said) : 0u);
    _result->status = d_tests_cli_sweep_status(status,
                                               why.failed);
    d_cli_module_unload(&module);
    d_arg_buffer_release(&why);
    d_tests_cli_sweep_arm(false);

    return;
}

// PLUGINS
//   constant: PROBE run through HOST two ways; a module refused and a module
// never built; and the calls underneath: finding, the program's own file,
// loading a module that belongs and one that does not.
static const struct d_tests_cli_sweep_case PLUGINS[] =
{
    { "U01", "prog probe report.txt 3 a --b",
      d_tests_cli_sweep_make_host, d_tests_cli_sweep_run_host,       3 },
    { "U02", "prog -vv --color=always probe report.txt",
      d_tests_cli_sweep_make_host, d_tests_cli_sweep_run_host,       0 },
    { "U03", "prog other",
      d_tests_cli_sweep_make_host, d_tests_cli_sweep_run_host,       2 },
    { "U04", "prog missing",
      d_tests_cli_sweep_make_host, d_tests_cli_sweep_run_host,       2 },
    { "D01", "probe",
      NULL,                        d_tests_cli_sweep_run_find,       0 },
    { "D02", "",
      NULL,                        d_tests_cli_sweep_run_executable, 0 },
    { "D03", "probe",
      NULL,                        d_tests_cli_sweep_run_load,       0 },
    { "D04", "other",
      NULL,                        d_tests_cli_sweep_run_load,       1 }
};

/*
d_tests_cli_sweep_plugins
  PLUGINS in a scratch directory, where PROBE writes its report, with
PROG_PLUGIN_PATH the modules' directory.
*/
bool
d_tests_cli_sweep_plugins(void)
{
    struct d_tests_arg_files files;
    bool                     held = d_tests_arg_files_enter(&files);

    // a scratch directory for the probe's reports
    if ( (!held) ||
         (!d_tests_arg_files_write(&files,
                                   "report.txt",
                                   "",
                                   0u)) )
    {
        printf("    the scratch directory could not be made\n");
        d_tests_arg_files_leave(&files);

        return false;
    }

    (void)d_tests_cli_setenv("PROG_PLUGIN_PATH",
                             d_tests_cli_sweep_modules_at);
    held = d_tests_cli_sweep_cases(PLUGINS,
                                   D_TESTS_CLI_SWEEP_COUNT(PLUGINS));
    (void)d_tests_cli_setenv("PROG_PLUGIN_PATH",
                             NULL);

    // the report put back, for the directory's removal to find
    (void)d_tests_cli_sweep_put("report.txt",
                                "",
                                0u);
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_sweep
//   constant: the suite's sections, in the order they run.
const struct d_tests_section d_tests_cli_sweep[] =
{
    { "cli_sweep_making",  d_tests_cli_sweep_making  },
    { "cli_sweep_endings", d_tests_cli_sweep_endings },
    { "cli_sweep_work",    d_tests_cli_sweep_work    },
    { "cli_sweep_files",   d_tests_cli_sweep_files   },
    { "cli_sweep_plugins", d_tests_cli_sweep_plugins }
};

// d_tests_cli_sweep_count
//   constant: how many sections d_tests_cli_sweep has.
const size_t d_tests_cli_sweep_count =
    sizeof(d_tests_cli_sweep) / sizeof(d_tests_cli_sweep[0]);
