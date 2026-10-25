/*******************************************************************************
* djinterp [parse]                                             arg_sweep_tests.c
*
* The argument parser's allocation sweep, declared in arg_sweep_tests.h.
*   malloc, calloc, realloc and free are wrapped at link time (--wrap), and
* the wrappers keep a ledger: how many allocations were asked for, which
* one to refuse, and how many blocks are live. A scenario is run once as it
* is, then once for each allocation it asked for, that one refused; every
* run must end with no block live once the parser is released -- every
* context the scenario gave away freed with what kept it, or at once by the
* call that refused it -- and with every definition that came back NULL
* absent from its command, as a refused definition leaves nothing, and
* recorded as the parser's definition error, as a refused definition is
* never silent. Output is discarded: what a refused allocation changes in
* the text is the transcripts' business, not the sweep's.
*   The scenarios use the C API alone, and the contexts they give away are
* allocated through the wrapped malloc, so that a context a refused call
* failed to free is a block left live. The names `__wrap_malloc` and kin
* are the linker's, reserved as they are: clang's -Wreserved-identifier is
* off around their declarations alone.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_sweep_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "./arg_sweep_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcmp, memcpy
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../../inc/djinterp/env/env.h"  // D_ENV_COMPILER_CLANG
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser
#include "../../../c/tests_section.h"  // d_tests_section
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // intmax_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_tests_arg_sweep_ledger
//   struct: what the wrappers keep: the allocation to refuse, counted from
// 1 -- 0 refuses none -- how many have been asked for, how many blocks are
// live; and what the scenarios find: refused definitions left behind,
// refused definitions the parser did not record, and texts reported written
// that were not written whole.
struct d_tests_arg_sweep_ledger
{
    size_t refuse_at;
    size_t asked;
    long   live;
    size_t left_behind;
    size_t unrecorded;
    size_t misreported;
};

// d_tests_arg_sweep_book
//   variable: the ledger, which the wrappers and the scenarios share.
static struct d_tests_arg_sweep_ledger d_tests_arg_sweep_book =
{
    0u,
    0u,
    0,
    0u,
    0u,
    0u
};

// d_tests_arg_sweep_digest
//   struct: what a sink took, without keeping it: how many bytes, and their
// FNV-1a hash.
struct d_tests_arg_sweep_digest
{
    size_t   length;
    uint32_t hash;
};

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

// the wrappers, which every call of the library and of this suite reaches
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
 * @brief Counts an allocation asked for, and tells whether it is the one
 *        to refuse.
 *
 * @return `true` for the allocation the ledger refuses.
 */
static bool
d_tests_arg_sweep_refused(void)
{
    ++d_tests_arg_sweep_book.asked;

    return ( (d_tests_arg_sweep_book.refuse_at != 0u) &&
             (d_tests_arg_sweep_book.asked ==
              d_tests_arg_sweep_book.refuse_at) );
}

/**
 * @brief malloc, counted, refused when the ledger says so.
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
    if (d_tests_arg_sweep_refused())
    {
        return NULL;
    }

    void* const memory = __real_malloc(_size);

    d_tests_arg_sweep_book.live += (memory != NULL) ? 1 : 0;

    return memory;
}

/**
 * @brief calloc, counted, refused when the ledger says so.
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
    if (d_tests_arg_sweep_refused())
    {
        return NULL;
    }

    void* const memory = __real_calloc(_count,
                                       _size);

    d_tests_arg_sweep_book.live += (memory != NULL) ? 1 : 0;

    return memory;
}

/**
 * @brief realloc, counted, refused when the ledger says so.
 *
 * @param[in] _memory  the block to grow; NULL for a new one.
 * @param[in] _size    the size asked for.
 * @return the block; NULL, the old one untouched, when it is refused or
 *         cannot be had.
 *
 * Only a new block, from NULL, adds to the live count: a block moved by
 * growing it is the same block to the ledger.
 */
void*
__wrap_realloc(
    void*  _memory,
    size_t _size
)
{
    // the refused allocation
    if (d_tests_arg_sweep_refused())
    {
        return NULL;
    }

    void* const memory = __real_realloc(_memory,
                                        _size);

    d_tests_arg_sweep_book.live += ( (memory != NULL) &&
                                     (_memory == NULL) ) ? 1 : 0;

    return memory;
}

/**
 * @brief free, counted.
 *
 * @param[in] _memory  the block; NULL frees nothing.
 */
void
__wrap_free(
    void* _memory
)
{
    d_tests_arg_sweep_book.live -= (_memory != NULL) ? 1 : 0;
    __real_free(_memory);

    return;
}

/**
 * @brief A sink's write that takes everything and keeps nothing.
 *
 * @param[in] _context  unused.
 * @param[in] _data     unused.
 * @param[in] _size     how many bytes.
 * @return `_size`: all of them taken.
 */
static size_t
d_tests_arg_sweep_discard(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_context;
    (void)_data;

    return _size;
}

// DISCARD
//   constant: the sink every scenario writes to.
static const struct d_pack_sink DISCARD =
{
    d_tests_arg_sweep_discard,
    NULL
};

/**
 * @brief A sink's write that takes everything into a digest.
 *
 * @param[in,out] _context  the digest.
 * @param[in]     _data     the bytes.
 * @param[in]     _size     how many.
 * @return `_size`: all of them taken.
 */
static size_t
d_tests_arg_sweep_digest_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_tests_arg_sweep_digest* const digest = _context;
    const unsigned char* const             bytes  = _data;

    // FNV-1a, a byte at a time
    for (size_t index = 0u; index < _size; ++index)
    {
        digest->hash ^= (uint32_t)bytes[index];
        digest->hash *= 16777619u;
    }

    digest->length += _size;

    return _size;
}

/**
 * @brief A sink into a digest, emptied.
 *
 * @param[out] _digest  the digest the sink writes into.
 * @return the sink.
 */
static struct d_pack_sink
d_tests_arg_sweep_digest_sink(
    struct d_tests_arg_sweep_digest* _digest
)
{
    const struct d_pack_sink sink = { d_tests_arg_sweep_digest_write,
                                      _digest };

    _digest->length = 0u;
    _digest->hash   = 2166136261u;

    return sink;
}

/**
 * @brief Holds two writings of the same text to "reported written, written
 *        whole": when both succeed, they wrote the same bytes -- so a
 *        writing the refused allocation cut short cannot pass for whole.
 *
 * @param[in] _first        the first writing's status.
 * @param[in] _first_text   what it wrote.
 * @param[in] _second       the second's.
 * @param[in] _second_text  what it wrote.
 */
static void
d_tests_arg_sweep_whole(
    int                                    _first,
    const struct d_tests_arg_sweep_digest* _first_text,
    int                                    _second,
    const struct d_tests_arg_sweep_digest* _second_text
)
{
    // two successes that wrote different texts
    if ( (_first == D_ARG_OK)                             &&
         (_second == D_ARG_OK)                            &&
         ( (_first_text->length != _second_text->length) ||
           (_first_text->hash != _second_text->hash) ) )
    {
        ++d_tests_arg_sweep_book.misreported;
    }

    return;
}

/**
 * @brief A context to give away: a block of the caller's, which whatever
 *        takes it frees with free.
 *
 * @return the block; NULL when its allocation is the one refused, and
 *         then there is no context to give.
 */
static void*
d_tests_arg_sweep_context(void)
{
    int* const context = malloc(sizeof(*context));

    // a context only when one could be made
    if (context != NULL)
    {
        *context = 7;
    }

    return context;
}

/**
 * @brief Copies a context, for a record that copies its own.
 *
 * @param[in] _context  the context.
 * @return the copy; NULL when it could not be made.
 */
static void*
d_tests_arg_sweep_copy(
    const void* _context
)
{
    int* const copy = malloc(sizeof(*copy));

    // the value, once there is room for it
    if (copy != NULL)
    {
        memcpy(copy,
               _context,
               sizeof(*copy));
    }

    return copy;
}

/**
 * @brief Holds a call that makes a definition to "a refused definition is
 *        never silent": refused, the parser holds a definition error.
 *
 * @param[in] _command  the command the definition was made in.
 * @param[in] _refused  whether the call came back NULL.
 *
 * The definition error is cleared afterwards, so that each call is held
 * to its own: a run refuses one allocation, so no failure of an earlier
 * call stands in for a later one's.
 */
static void
d_tests_arg_sweep_recorded(
    const struct d_arg_command* _command,
    bool                        _refused
)
{
    const struct d_arg_error* const error =
        d_arg_parser_definition_error(_command->parser);

    // a refusal nothing recorded
    if ( (_refused) &&
         ( (!error) ||
           (error->status == (int)D_ARG_OK) ) )
    {
        ++d_tests_arg_sweep_book.unrecorded;
    }

    d_arg_parser_clear_definition_error(_command->parser);

    return;
}

/**
 * @brief Holds a call that makes an option to "a refused definition leaves
 *        nothing": made, the command has one more option; refused, it has
 *        as many as before, and the refusal is recorded.
 *
 * @param[in] _command  the command the option was made in.
 * @param[in] _before   how many options it had before the call.
 * @param[in] _made     what the call returned.
 */
static void
d_tests_arg_sweep_option_held(
    const struct d_arg_command* _command,
    size_t                      _before,
    const struct d_arg_option*  _made
)
{
    const size_t expected = (_made != NULL) ? (_before + 1u) : _before;

    // a refused option left behind, or a made one missing
    if (_command->options.count != expected)
    {
        ++d_tests_arg_sweep_book.left_behind;
    }

    d_tests_arg_sweep_recorded(_command,
                               (_made == NULL));

    return;
}

/**
 * @brief Holds a call that sets something on a definition to the rule its
 *        header states -- a setter that can fail records the failure as a
 *        definition error: a refused allocation it reports is recorded.
 *
 * @param[in] _command  a command of the parser.
 * @param[in] _status   what the call returned.
 *
 * Only D_ARG_ERROR_OUT_OF_MEMORY is held: a NULL definition, which a
 * refused maker leaves, has no parser to record its D_ARG_ERROR_BAD_ARGUMENT
 * in, and the refusals a definition makes by design are its own business.
 */
static void
d_tests_arg_sweep_set(
    const struct d_arg_command* _command,
    int                         _status
)
{
    d_tests_arg_sweep_recorded(_command,
                               (_status == (int)D_ARG_ERROR_OUT_OF_MEMORY));

    return;
}

/**
 * @brief Holds a call that makes a subcommand or an option group as
 *        d_tests_arg_sweep_option_held holds an option.
 *
 * @param[in] _command  the command it was made in.
 * @param[in] _before   how many subcommands it had before the call.
 * @param[in] _made     what the call returned.
 */
static void
d_tests_arg_sweep_command_held(
    const struct d_arg_command* _command,
    size_t                      _before,
    const struct d_arg_command* _made
)
{
    const size_t expected = (_made != NULL) ? (_before + 1u) : _before;

    // a refused subcommand left behind, or a made one missing
    if (_command->subcommands.count != expected)
    {
        ++d_tests_arg_sweep_book.left_behind;
    }

    d_tests_arg_sweep_recorded(_command,
                               (_made == NULL));

    return;
}

/**
 * @brief Holds a call that replaces a special option -- the help flags,
 *        the version flag, the configuration option -- to the same rule:
 *        the old one gone, and the new one there only when it was made.
 *
 * @param[in] _command  the command.
 * @param[in] _before   how many options it had, the old one left out.
 * @param[in] _made     what the call returned.
 * @param[in] _slot     the command's pointer to the option, after the call.
 */
static void
d_tests_arg_sweep_special_held(
    const struct d_arg_command* _command,
    size_t                      _before,
    const struct d_arg_option*  _made,
    const struct d_arg_option*  _slot
)
{
    d_tests_arg_sweep_option_held(_command,
                                  _before,
                                  _made);

    // the slot holds what was made, or nothing
    if (_slot != _made)
    {
        ++d_tests_arg_sweep_book.left_behind;
    }

    return;
}

/**
 * @brief How many options a command has once a special option is gone.
 *
 * @param[in] _command  the command.
 * @param[in] _special  the special option it points at; NULL for none.
 * @return the count, the special option left out when the command owns it.
 */
static size_t
d_tests_arg_sweep_without(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _special
)
{
    size_t owned = 0u;

    // the special option, when it is among the command's own
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        owned += (_command->options.items[index] == _special) ? 1u : 0u;
    }

    return _command->options.count - owned;
}

// d_tests_arg_sweep_fn
//   type: a scenario, which makes, uses and releases a parser.
typedef void (*d_tests_arg_sweep_fn)(void);

/**
 * @brief Starts a run of a scenario: the ledger emptied, and the allocation
 *        it refuses set.
 *
 * @param[in] _refuse_at  the allocation the run refuses, counting from 1; 0
 *                        for none.
 */
static void
d_tests_arg_sweep_reset(
    size_t _refuse_at
)
{
    d_tests_arg_sweep_book.refuse_at   = _refuse_at;
    d_tests_arg_sweep_book.asked       = 0u;
    d_tests_arg_sweep_book.live        = 0;
    d_tests_arg_sweep_book.left_behind = 0u;
    d_tests_arg_sweep_book.unrecorded  = 0u;
    d_tests_arg_sweep_book.misreported = 0u;

    return;
}

/**
 * @brief Judges a run by its ledger, writing a line for each way it fell
 *        short: a block left live, a refused definition left behind or
 *        unrecorded, a text reported written that was cut short.
 *
 * @param[in] _name   what the lines name the scenario by.
 * @param[in] _at     the allocation the run refused; 0 for none.
 * @param[in] _total  how many allocations the scenario makes.
 * @return whether the run held on every count.
 */
static bool
d_tests_arg_sweep_judge(
    const char* _name,
    size_t      _at,
    size_t      _total
)
{
    bool held = true;

    // a block nothing freed
    if (d_tests_arg_sweep_book.live != 0)
    {
        printf("    %s: refusing allocation %lu of %lu leaves %ld "
               "blocks live\n",
               _name,
               (unsigned long)_at,
               (unsigned long)_total,
               d_tests_arg_sweep_book.live);
        held = false;
    }

    // a refused definition kept, or a made one lost
    if (d_tests_arg_sweep_book.left_behind != 0u)
    {
        printf("    %s: refusing allocation %lu of %lu leaves %lu "
               "refused definitions behind\n",
               _name,
               (unsigned long)_at,
               (unsigned long)_total,
               (unsigned long)d_tests_arg_sweep_book.left_behind);
        held = false;
    }

    // a refused definition nothing recorded
    if (d_tests_arg_sweep_book.unrecorded != 0u)
    {
        printf("    %s: refusing allocation %lu of %lu leaves %lu "
               "refused definitions unrecorded\n",
               _name,
               (unsigned long)_at,
               (unsigned long)_total,
               (unsigned long)d_tests_arg_sweep_book.unrecorded);
        held = false;
    }

    // a text cut short, reported written
    if (d_tests_arg_sweep_book.misreported != 0u)
    {
        printf("    %s: refusing allocation %lu of %lu reports %lu "
               "texts written that were cut short\n",
               _name,
               (unsigned long)_at,
               (unsigned long)_total,
               (unsigned long)d_tests_arg_sweep_book.misreported);
        held = false;
    }

    return held;
}

/**
 * @brief Sweeps a scenario: run as it is, counting its allocations, then
 *        once for each, that one refused; writes a line for each run that
 *        leaks, leaves a refused definition behind or unrecorded, or reports
 *        a text written that was not written whole, then the scenario's.
 *
 * @param[in] _name      what the lines name the scenario by.
 * @param[in] _scenario  the scenario.
 * @return whether every run ended with nothing live, nothing left behind,
 *         every refusal recorded, and every text reported written written
 *         whole.
 */
static bool
d_tests_arg_sweep_run(
    const char*          _name,
    d_tests_arg_sweep_fn _scenario
)
{
    // the scenario as it is, counting
    d_tests_arg_sweep_reset(0u);
    _scenario();

    const size_t total = d_tests_arg_sweep_book.asked;
    bool         held  = true;

    // each allocation refused in turn; 0 is the run with none refused
    for (size_t at = 0u; at <= total; ++at)
    {
        d_tests_arg_sweep_reset(at);
        _scenario();
        held = ( (d_tests_arg_sweep_judge(_name,
                                          at,
                                          total)) &&
                 (held) );
    }

    d_tests_arg_sweep_book.refuse_at = 0u;
    printf("    %s: %lu allocations, each refused in turn\n",
           _name,
           (unsigned long)total);

    return held;
}

/**
 * @brief Makes a parser for a scenario, writing to nothing.
 *
 * @param[out] _parser  the parser; made even when its making fails, as the
 *                      parser can then still be used and released.
 */
static void
d_tests_arg_sweep_parser(
    struct d_arg_parser* _parser
)
{
    (void)d_arg_parser_init(_parser,
                            "a program swept",
                            "prog");
    d_arg_parser_set_output(_parser,
                            DISCARD,
                            DISCARD);

    return;
}

/**
 * @brief Parses arguments, then writes what the parse comes to, as a main
 *        that ends with d_arg_parser_exit does.
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _argc    how many arguments, the program's name among
 *                         them.
 * @param[in]     _argv    the arguments.
 */
static void
d_tests_arg_sweep_parse(
    struct d_arg_parser* _parser,
    int                  _argc,
    char* const*         _argv
)
{
    const int status = d_arg_parser_parse(_parser,
                                          _argc,
                                          _argv);

    (void)d_arg_parser_exit_to(_parser,
                               status,
                               DISCARD,
                               DISCARD);

    return;
}

/**
 * @brief Writes a command's help in each mode, twice, held to "written
 *        whole".
 *
 * @param[in] _command  the command.
 */
static void
d_tests_arg_sweep_help(
    const struct d_arg_command* _command
)
{
    static const int MODES[] =
    {
        (int)D_ARG_HELP_NORMAL,
        (int)D_ARG_HELP_ALL
    };

    // each mode
    for (size_t index = 0u;
         index < (sizeof(MODES) / sizeof(MODES[0]));
         ++index)
    {
        struct d_tests_arg_sweep_digest first;
        struct d_tests_arg_sweep_digest second;
        const int                       wrote  =
            d_arg_help(_command,
                       d_pack_text_from_cstr(""),
                       MODES[index],
                       d_tests_arg_sweep_digest_sink(&first));
        const int                       again  =
            d_arg_help(_command,
                       d_pack_text_from_cstr(""),
                       MODES[index],
                       d_tests_arg_sweep_digest_sink(&second));

        d_tests_arg_sweep_whole(wrote,
                                &first,
                                again,
                                &second);
    }

    return;
}

//==============================================================================
// 1.  CALLBACKS
//==============================================================================

/**
 * @brief An option's callback that takes its values and does nothing.
 *
 * @param[in] _option   unused.
 * @param[in] _results  unused.
 * @param[in] _count    unused.
 * @param[in] _context  unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_sweep_on_values(
    struct d_arg_option*      _option,
    const struct d_pack_text* _results,
    size_t                    _count,
    void*                     _context
)
{
    (void)_option;
    (void)_results;
    (void)_count;
    (void)_context;

    return D_ARG_OK;
}

/**
 * @brief An each() function that passes every value.
 *
 * @param[in] _option   unused.
 * @param[in] _value    unused.
 * @param[in] _context  unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_sweep_on_each(
    const struct d_arg_option* _option,
    struct d_pack_text         _value,
    void*                      _context
)
{
    (void)_option;
    (void)_value;
    (void)_context;

    return D_ARG_OK;
}

/**
 * @brief A flag's function that takes its count and does nothing.
 *
 * @param[in] _option   unused.
 * @param[in] _count    unused.
 * @param[in] _context  unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_sweep_on_flag(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_option;
    (void)_count;
    (void)_context;

    return D_ARG_OK;
}

/**
 * @brief A flag's function that takes its count as an intmax_t and does
 *        nothing.
 *
 * @param[in] _option   unused.
 * @param[in] _count    unused.
 * @param[in] _context  unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_sweep_on_wide(
    struct d_arg_option* _option,
    intmax_t             _count,
    void*                _context
)
{
    (void)_option;
    (void)_count;
    (void)_context;

    return D_ARG_OK;
}

/**
 * @brief A command's callback that does nothing.
 *
 * @param[in] _command  unused.
 * @param[in] _context  unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_sweep_on_command(
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_command;
    (void)_context;

    return D_ARG_OK;
}

/**
 * @brief A command's pre-parse callback that does nothing.
 *
 * @param[in] _command    unused.
 * @param[in] _remaining  unused.
 * @param[in] _context    unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_sweep_on_preparse(
    struct d_arg_command* _command,
    size_t                _remaining,
    void*                 _context
)
{
    (void)_command;
    (void)_remaining;
    (void)_context;

    return D_ARG_OK;
}

/**
 * @brief Writes a command's text -- a usage line, a footer, a version.
 *
 * @param[in] _command  unused.
 * @param[in] _sink     where the text goes.
 * @param[in] _context  unused.
 * @return the sink's status.
 */
static int
d_tests_arg_sweep_command_text(
    const struct d_arg_command* _command,
    struct d_pack_sink          _sink,
    void*                       _context
)
{
    (void)_command;
    (void)_context;

    return d_arg_sink_cstr(_sink,
                           "text of the command");
}

/**
 * @brief Writes an option's text -- a type name, a default.
 *
 * @param[in] _option   unused.
 * @param[in] _sink     where the text goes.
 * @param[in] _context  unused.
 * @return the sink's status.
 */
static int
d_tests_arg_sweep_option_text(
    const struct d_arg_option* _option,
    struct d_pack_sink         _sink,
    void*                      _context
)
{
    (void)_option;
    (void)_context;

    return d_arg_sink_cstr(_sink,
                           "TEXT");
}

/**
 * @brief A failure message of the caller's: the error's message, then the
 *        command's help.
 *
 * @param[in] _command  the command the error is reported from.
 * @param[in] _error    the error.
 * @param[in] _sink     where the message goes.
 * @param[in] _context  unused.
 * @return the first failure among the writes; D_ARG_OK.
 */
static int
d_tests_arg_sweep_failure(
    const struct d_arg_command* _command,
    const struct d_arg_error*   _error,
    struct d_pack_sink          _sink,
    void*                       _context
)
{
    (void)_context;

    const int status = d_arg_sink_cstr(_sink,
                                       d_arg_error_message(_error));

    return (status == D_ARG_OK)
           ? d_arg_help(_command,
                        d_pack_text_from_cstr(""),
                        (int)D_ARG_HELP_NORMAL,
                        _sink)
           : status;
}

/**
 * @brief A custom validator's function: rejects the value "bad", passes
 *        any other.
 *
 * @param[in]     _validator   unused.
 * @param[in,out] _validation  the value, and its verdict.
 * @return D_ARG_OK, or the status of keeping the reason.
 */
static int
d_tests_arg_sweep_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validator;

    return ( (_validation->value.length == 3u) &&
             (memcmp(_validation->value.data,
                     "bad",
                     3u) == 0) )
           ? d_arg_validation_reject(_validation,
                                     "bad is refused")
           : D_ARG_OK;
}

/**
 * @brief A custom validator that owns a context it copies with itself.
 *
 * @return the validator; a failed one when it could not be made, the
 *         context then freed here, as nothing took it.
 */
static struct d_arg_validator
d_tests_arg_sweep_custom(void)
{
    void* const            context   = d_tests_arg_sweep_context();
    struct d_arg_validator validator =
        d_arg_validator_custom(d_tests_arg_sweep_validate,
                               context,
                               "CUSTOM");

    // a validator that could not be made never took the context
    if (validator.status != D_ARG_OK)
    {
        free(context);

        return validator;
    }

    d_arg_validator_own_context(&validator,
                                (context != NULL) ? d_tests_arg_sweep_copy
                                                  : NULL,
                                (context != NULL) ? free : NULL);

    return validator;
}

//==============================================================================
// 2.  THE SCENARIOS
//==============================================================================

// 2.1    Options
//------------------------------------------------------------------------------

/**
 * @brief Gives the options scenario its options bound to variables, each
 *        with a default: a count's typed, a ratio's from the environment,
 *        and a name's as text.
 *
 * @param[in,out] _app    the root.
 * @param[in,out] _count  the count's variable, which the option is bound to.
 * @param[in,out] _ratio  the ratio's variable, which the option is bound to.
 * @param[in,out] _name   the name's variable, which the option is bound to.
 * @return the name's option; NULL when it was refused.
 */
static struct d_arg_option*
d_tests_arg_sweep_options_bound(
    struct d_arg_command* _app,
    int*                  _count,
    double*               _ratio,
    const char**          _name
)
{
    static const int SEVEN = 7;

    struct d_arg_option* made   = NULL;
    size_t               before = 0u;

    before = _app->options.count;
    made   = d_arg_command_add_option_bound(
                 _app,
                 "-c,--count",
                 _count,
                 d_arg_type_builtin((int)D_ARG_BUILTIN_INT),
                 "a count");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_tests_arg_sweep_set(_app,
                          d_arg_option_set_default_typed(
                              made,
                              d_arg_type_builtin((int)D_ARG_BUILTIN_INT),
                              &SEVEN));

    before = _app->options.count;
    made   = d_arg_command_add_option_bound(
                 _app,
                 "--ratio",
                 _ratio,
                 d_arg_type_builtin((int)D_ARG_BUILTIN_DOUBLE),
                 "a ratio");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_tests_arg_sweep_set(_app,
                          d_arg_option_set_env_name(
                              made,
                              "D_TESTS_ARG_SWEEP_RATIO"));

    before = _app->options.count;
    made   = d_arg_command_add_option_bound(
                 _app,
                 "-n,--name",
                 _name,
                 d_arg_type_builtin((int)D_ARG_BUILTIN_STRING),
                 "a name");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_tests_arg_sweep_set(_app,
                          d_arg_option_set_default_value(made,
                                                         "nobody"));

    return made;
}

/**
 * @brief Gives the options scenario its option taking several values,
 *        described at length.
 *
 * @param[in,out] _app  the root.
 * @return the option; NULL when it was refused.
 */
static struct d_arg_option*
d_tests_arg_sweep_options_plain(
    struct d_arg_command* _app
)
{
    const size_t               before = _app->options.count;
    struct d_arg_option* const plain  = d_arg_command_add_option(
                                            _app,
                                            "--plain",
                                            "values taken as they come");

    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  plain);
    d_arg_option_set_expected_range(plain,
                                    1,
                                    3);
    d_arg_option_set_multi_option_policy(plain,
                                         (int)D_ARG_MULTI_TAKE_ALL);
    d_tests_arg_sweep_set(_app,
                          d_arg_option_set_type_name(plain,
                                                     "WORD"));
    d_tests_arg_sweep_set(_app,
                          d_arg_option_set_description(
                              plain,
                              "values taken as they come, up to three of "
                              "them, each a word"));

    return plain;
}

/**
 * @brief Gives the options scenario its options that call functions, each
 *        owning its context: a callback, an each() function, and a type
 *        name's and a default's functions on an option whose case does not
 *        matter.
 *
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_sweep_options_called(
    struct d_arg_command* _app
)
{
    struct d_arg_option* made   = NULL;
    size_t               before = 0u;

    before = _app->options.count;
    made   = d_arg_command_add_option_callback(_app,
                                               "--cb",
                                               d_tests_arg_sweep_on_values,
                                               d_tests_arg_sweep_context(),
                                               free,
                                               "a callback");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "--each",
                                      "each value seen");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_tests_arg_sweep_set(_app,
                          d_arg_option_each(made,
                                            d_tests_arg_sweep_on_each,
                                            d_tests_arg_sweep_context(),
                                            free));

    // a type name and a default from functions owning their contexts
    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "--case",
                                      "case does not matter");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_tests_arg_sweep_set(_app,
                          d_arg_option_set_ignore_case(made,
                                                       true));
    d_arg_option_set_type_name_fn(made,
                                  d_tests_arg_sweep_option_text,
                                  d_tests_arg_sweep_context(),
                                  free);
    d_arg_option_set_default_fn(made,
                                d_tests_arg_sweep_option_text,
                                d_tests_arg_sweep_context(),
                                free);
    d_tests_arg_sweep_set(_app,
                          d_arg_option_capture_default(made));

    return;
}

/**
 * @brief Gives the options scenario its options linked to others or to the
 *        command's switches: a positional that needs the name, in a group
 *        of its own, and a short name longer than a character, where the
 *        command takes one.
 *
 * @param[in,out] _app    the root.
 * @param[in]     _named  the name's option, which the positional needs.
 */
static void
d_tests_arg_sweep_options_linked(
    struct d_arg_command* _app,
    struct d_arg_option*  _named
)
{
    struct d_arg_option* made   = NULL;
    size_t               before = 0u;

    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "pos",
                                      "a positional");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_tests_arg_sweep_set(_app,
                          d_arg_option_needs(made,
                                             _named));
    d_tests_arg_sweep_set(_app,
                          d_arg_option_set_group(made,
                                                 "Positionals"));

    // a short name longer than a character, where the command takes one
    d_arg_command_set_non_standard_names(_app,
                                         true);
    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "-vv,--very",
                                      "a non-standard short name");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    return;
}

/**
 * @brief Options of every kind the C API makes, with the defaults new ones
 *        take, their text, links and functions, and a non-standard short
 *        name, parsed and described.
 */
static void
d_tests_arg_sweep_options_run(void)
{
    static char* ARGV[] =
    {
        "prog", "--count", "3", "--ratio=0.5", "--name", "joe", "--each",
        "a", "--cb", "x", "--plain", "p", "q", "--CASE", "c", "first"
    };
    struct d_arg_parser   parser;
    struct d_arg_command* app     = &parser.command;
    int                   count   = 0;
    double                ratio   = 0.0;
    const char*           name    = NULL;
    size_t                results = 0u;

    d_tests_arg_sweep_parser(&parser);

    // the defaults the new options take
    (void)d_arg_option_defaults_set_group(d_arg_command_option_defaults(app),
                                          "Settings");
    d_arg_option_defaults_set_flag(d_arg_command_option_defaults(app),
                                   D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT,
                                   true);

    struct d_arg_option* const named = d_tests_arg_sweep_options_bound(app,
                                                                       &count,
                                                                       &ratio,
                                                                       &name);
    struct d_arg_option* const plain = d_tests_arg_sweep_options_plain(app);

    d_tests_arg_sweep_options_called(app);
    d_tests_arg_sweep_options_linked(app,
                                     named);
    d_tests_arg_sweep_parse(&parser,
                            (int)(sizeof(ARGV) / sizeof(ARGV[0])),
                            ARGV);
    (void)d_arg_option_results(plain,
                               &results);
    d_tests_arg_sweep_help(app);
    d_arg_parser_release(&parser);

    return;
}

// 2.2    Flags
//------------------------------------------------------------------------------

/**
 * @brief Gives the flags scenario its flags that take no variable: a plain
 *        one, one carrying a value, and one with a negated name.
 *
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_sweep_flags_plain(
    struct d_arg_command* _app
)
{
    struct d_arg_option* made   = NULL;
    size_t               before = 0u;

    before = _app->options.count;
    made   = d_arg_command_add_flag(_app,
                                    "-v,--verbose",
                                    "say more");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    before = _app->options.count;
    made   = d_arg_command_add_flag(_app,
                                    "--level{5}",
                                    "a level, given");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    before = _app->options.count;
    made   = d_arg_command_add_flag(_app,
                                    "-q,--quiet,!--no-quiet",
                                    "say less, or not");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    return;
}

/**
 * @brief Gives the flags scenario its flags bound to variables: a counting
 *        integer and a bool.
 *
 * @param[in,out] _app    the root.
 * @param[in,out] _count  the integer, which the flag is bound to.
 * @param[in,out] _on     the bool, which the flag is bound to.
 */
static void
d_tests_arg_sweep_flags_bound(
    struct d_arg_command* _app,
    int*                  _count,
    bool*                 _on
)
{
    struct d_arg_option* made   = NULL;
    size_t               before = 0u;

    before = _app->options.count;
    made   = d_arg_command_add_flag_bound(
                 _app,
                 "--count",
                 _count,
                 d_arg_type_builtin((int)D_ARG_BUILTIN_INT),
                 "counted");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    before = _app->options.count;
    made   = d_arg_command_add_flag_bound(
                 _app,
                 "--on",
                 _on,
                 d_arg_type_builtin((int)D_ARG_BUILTIN_BOOL),
                 "on or off");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    return;
}

/**
 * @brief Gives the flags scenario its flags that call functions, each
 *        owning its context: a callback, a function given the count, and
 *        one given it as an intmax_t.
 *
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_sweep_flags_called(
    struct d_arg_command* _app
)
{
    struct d_arg_option* made   = NULL;
    size_t               before = 0u;

    before = _app->options.count;
    made   = d_arg_command_add_flag_callback(_app,
                                             "--trigger",
                                             d_tests_arg_sweep_on_flag,
                                             d_tests_arg_sweep_context(),
                                             free,
                                             "calls back");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    before = _app->options.count;
    made   = d_arg_command_add_flag_function(_app,
                                             "--sum",
                                             d_tests_arg_sweep_on_flag,
                                             d_tests_arg_sweep_context(),
                                             free,
                                             "sums");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    before = _app->options.count;
    made   = d_arg_command_add_flag_function_intmax(
                 _app,
                 "--wide",
                 d_tests_arg_sweep_on_wide,
                 d_tests_arg_sweep_context(),
                 free,
                 "sums widely");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    return;
}

/**
 * @brief Gives the flags scenario a flag whose name another takes once case
 *        is ignored, which the option defaults then refuse.
 *
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_sweep_flags_refused(
    struct d_arg_command* _app
)
{
    struct d_arg_option* made   = NULL;
    size_t               before = 0u;

    before = _app->options.count;
    made   = d_arg_command_add_flag(_app,
                                    "--Mixed",
                                    "made before the defaults change");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_arg_option_defaults_set_flag(d_arg_command_option_defaults(_app),
                                   D_ARG_OPTION_IGNORE_CASE,
                                   true);
    before = _app->options.count;
    made   = d_arg_command_add_flag(_app,
                                    "--mixed",
                                    "refused: --Mixed takes the name");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    return;
}

/**
 * @brief Flags of every kind: plain, carrying values, negated, bound to a
 *        counting integer and a bool, and calling functions that own their
 *        contexts; then one whose name another takes once case is ignored,
 *        which the option defaults refuse.
 */
static void
d_tests_arg_sweep_flags_run(void)
{
    static char* ARGV[] =
    {
        "prog", "-vvv", "--level", "--no-quiet", "--count", "--count",
        "--on", "--trigger", "--sum", "--sum", "--wide"
    };
    struct d_arg_parser   parser;
    struct d_arg_command* app   = &parser.command;
    int                   count = 0;
    bool                  on    = false;

    d_tests_arg_sweep_parser(&parser);
    d_tests_arg_sweep_flags_plain(app);
    d_tests_arg_sweep_flags_bound(app,
                                  &count,
                                  &on);
    d_tests_arg_sweep_flags_called(app);
    d_tests_arg_sweep_flags_refused(app);

    // the refusal recorded, then cleared, so that the parse runs
    d_arg_parser_clear_definition_error(&parser);

    d_tests_arg_sweep_parse(&parser,
                            (int)(sizeof(ARGV) / sizeof(ARGV[0])),
                            ARGV);
    d_tests_arg_sweep_help(app);
    d_arg_parser_release(&parser);

    return;
}

// 2.3    Commands
//------------------------------------------------------------------------------

/**
 * @brief Makes the commands scenario's subcommand, with an alias, a footer
 *        and an option of its own.
 *
 * @param[in,out] _app  the root.
 * @return the subcommand; NULL when it was refused.
 */
static struct d_arg_command*
d_tests_arg_sweep_commands_sub(
    struct d_arg_command* _app
)
{
    struct d_arg_command* sub    = NULL;
    struct d_arg_option*  made   = NULL;
    size_t                before = 0u;

    before = _app->subcommands.count;
    sub    = d_arg_command_add_subcommand(_app,
                                          "sub",
                                          "a subcommand");
    d_tests_arg_sweep_command_held(_app,
                                   before,
                                   sub);
    d_tests_arg_sweep_set(_app,
                          d_arg_command_add_alias(sub,
                                                  "s"));
    d_tests_arg_sweep_set(_app,
                          d_arg_command_set_footer(
                              sub,
                              "the subcommand's footer"));

    // an option of the subcommand's
    if (sub != NULL)
    {
        before = sub->options.count;
        made   = d_arg_command_add_option(sub,
                                          "--inner",
                                          "an option of the subcommand");
        d_tests_arg_sweep_option_held(sub,
                                      before,
                                      made);
    }

    return sub;
}

/**
 * @brief Makes the commands scenario's option groups, one inside another,
 *        with an option of the outer group's own and one moved into it.
 *
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_sweep_commands_groups(
    struct d_arg_command* _app
)
{
    struct d_arg_command* group  = NULL;
    struct d_arg_command* inner  = NULL;
    struct d_arg_option*  made   = NULL;
    size_t                before = 0u;

    before = _app->subcommands.count;
    group  = d_arg_command_add_option_group(_app,
                                            "Group",
                                            "a group");
    d_tests_arg_sweep_command_held(_app,
                                   before,
                                   group);

    // the group's own option, and a group inside it
    if (group != NULL)
    {
        before = group->options.count;
        made   = d_arg_command_add_option(group,
                                          "--grouped",
                                          "an option of the group");
        d_tests_arg_sweep_option_held(group,
                                      before,
                                      made);
        before = group->subcommands.count;
        inner  = d_arg_command_add_option_group(group,
                                                "Inner",
                                                "a group in the group");
        d_tests_arg_sweep_command_held(group,
                                       before,
                                       inner);
    }

    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "--moved",
                                      "moved into the group");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_tests_arg_sweep_set(_app,
                          d_arg_command_group_add_option(group,
                                                         made));

    return;
}

/**
 * @brief Gives the root of the commands scenario its special options: the
 *        help-all flag, the version flag made twice, by text and by
 *        function, and the configuration option.
 *
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_sweep_commands_special(
    struct d_arg_command* _app
)
{
    struct d_arg_option* made   = NULL;
    size_t               before = 0u;

    before = d_tests_arg_sweep_without(_app,
                                       _app->help_all);
    made   = d_arg_command_set_help_all_flag(_app,
                                             "--help-all",
                                             "all the help");
    d_tests_arg_sweep_special_held(_app,
                                   before,
                                   made,
                                   _app->help_all);

    before = d_tests_arg_sweep_without(_app,
                                       _app->version);
    made   = d_arg_command_set_version_flag(_app,
                                            "-V,--version",
                                            "1.2.3",
                                            NULL);
    d_tests_arg_sweep_special_held(_app,
                                   before,
                                   made,
                                   _app->version);

    before = d_tests_arg_sweep_without(_app,
                                       _app->version);
    made   = d_arg_command_set_version_fn(_app,
                                          "--version",
                                          d_tests_arg_sweep_command_text,
                                          d_tests_arg_sweep_context(),
                                          free,
                                          "the version, written");
    d_tests_arg_sweep_special_held(_app,
                                   before,
                                   made,
                                   _app->version);

    before = d_tests_arg_sweep_without(_app,
                                       _app->config);
    made   = d_arg_command_set_config(_app,
                                      "--config",
                                      NULL,
                                      "read a file",
                                      false);
    d_tests_arg_sweep_special_held(_app,
                                   before,
                                   made,
                                   _app->config);

    return;
}

/**
 * @brief Gives the root of the commands scenario options retired and
 *        deprecated: one retired by name, one retired from an option made,
 *        and one deprecated for another.
 *
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_sweep_commands_retired(
    struct d_arg_command* _app
)
{
    struct d_arg_option* made   = NULL;
    size_t               before = 0u;

    before = _app->options.count;
    made   = d_arg_command_retire_option_name(_app,
                                              "--gone");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "--old",
                                      "to be retired");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    // retired in place: one option out, one in; refused, the old one may
    // stay, untouched, so only what is made is held
    if (made != NULL)
    {
        before = _app->options.count - 1u;
        made   = d_arg_command_retire_option(_app,
                                             made);

        // the retired one in the old one's place
        if (made != NULL)
        {
            d_tests_arg_sweep_option_held(_app,
                                          before,
                                          made);
        }
    }

    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "--new",
                                      "the replacement");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "--dep",
                                      "deprecated");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_tests_arg_sweep_set(_app,
                          d_arg_command_deprecate_option(made,
                                                         "--new"));

    return;
}

/**
 * @brief Gives the commands scenario what its commands call, each function
 *        owning its context: the subcommand's callback, the root's final and
 *        pre-parse callbacks, its usage, footer and failure message; then
 *        requires a subcommand of the root.
 *
 * @param[in,out] _app  the root.
 * @param[in,out] _sub  the subcommand; NULL when it was refused.
 */
static void
d_tests_arg_sweep_commands_called(
    struct d_arg_command* _app,
    struct d_arg_command* _sub
)
{
    d_arg_command_set_callback(_sub,
                               d_tests_arg_sweep_on_command,
                               d_tests_arg_sweep_context(),
                               free);
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_sweep_on_command,
                                     d_tests_arg_sweep_context(),
                                     free);
    d_arg_command_set_preparse_callback(_app,
                                        d_tests_arg_sweep_on_preparse,
                                        d_tests_arg_sweep_context(),
                                        free);
    d_arg_command_set_usage_fn(_app,
                               d_tests_arg_sweep_command_text,
                               d_tests_arg_sweep_context(),
                               free);
    d_arg_command_set_footer_fn(_app,
                                d_tests_arg_sweep_command_text,
                                d_tests_arg_sweep_context(),
                                free);
    d_arg_command_set_failure_message(_app,
                                      d_tests_arg_sweep_failure,
                                      d_tests_arg_sweep_context(),
                                      free);
    d_arg_command_require_subcommand(_app,
                                     0u,
                                     1u);

    return;
}

/**
 * @brief Commands: a subcommand with an alias, a footer and an option;
 *        option groups, one inside another, an option moved in; the help-
 *        all, version and configuration options; options retired and
 *        deprecated; callbacks, text functions and a failure message, each
 *        owning its context; a subcommand required; a parse that runs the
 *        subcommand, and one that fails and is written as a main writes it.
 */
static void
d_tests_arg_sweep_commands_run(void)
{
    static char* GOOD[] =
    {
        "prog", "--moved", "m", "--gone", "g", "--dep", "d", "s",
        "--inner", "i"
    };
    static char* BAD[] =
    {
        "prog", "--nope", "sub"
    };
    struct d_arg_parser   parser;
    struct d_arg_command* app = &parser.command;

    d_tests_arg_sweep_parser(&parser);

    struct d_arg_command* const sub = d_tests_arg_sweep_commands_sub(app);

    d_tests_arg_sweep_commands_groups(app);
    d_tests_arg_sweep_commands_special(app);
    d_tests_arg_sweep_commands_retired(app);
    d_tests_arg_sweep_commands_called(app,
                                      sub);
    d_tests_arg_sweep_parse(&parser,
                            (int)(sizeof(GOOD) / sizeof(GOOD[0])),
                            GOOD);
    d_tests_arg_sweep_parse(&parser,
                            (int)(sizeof(BAD) / sizeof(BAD[0])),
                            BAD);
    d_tests_arg_sweep_help(app);

    // the subcommand's own help
    if (sub != NULL)
    {
        d_tests_arg_sweep_help(sub);
    }

    d_arg_parser_release(&parser);

    return;
}

// 2.4    Validators
//------------------------------------------------------------------------------

/**
 * @brief Gives an option a check, consumed whatever comes of it, and holds
 *        the option's making as the other scenarios do.
 *
 * @param[in,out] _app        the command.
 * @param[in]     _names      the option's names.
 * @param[in]     _validator  the check, consumed.
 * @param[in]     _transform  whether it is given as a transformation.
 */
static void
d_tests_arg_sweep_checked(
    struct d_arg_command*  _app,
    const char*            _names,
    struct d_arg_validator _validator,
    bool                   _transform
)
{
    const size_t               before = _app->options.count;
    struct d_arg_option* const made   = d_arg_command_add_option(
                                            _app,
                                            _names,
                                            "a checked option");

    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);

    // a transformation, or a check
    if (_transform)
    {
        d_tests_arg_sweep_set(_app,
                              d_arg_option_transform(made,
                                                     _validator,
                                                     NULL,
                                                     NULL));
    }
    else
    {
        d_tests_arg_sweep_set(_app,
                              d_arg_option_check(made,
                                                 _validator,
                                                 NULL));
    }

    return;
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/**
 * @brief A filter of the caller's: the value as it is.
 *
 * @param[in]     _value    the text.
 * @param[in,out] _out      the empty buffer that receives it.
 * @param[in]     _context  unused.
 * @return the buffer's status.
 */
static int
d_tests_arg_sweep_filter(
    struct d_pack_text   _value,
    struct d_arg_buffer* _out,
    void*                _context
)
{
    (void)_context;

    return d_arg_buffer_append_text(_out,
                                    _value);
}

/**
 * @brief Gives the validators scenario the extra checks of numbers: bounds,
 *        a number's composed with a type's, and sizes.
 *
 * @param[in,out] _app  the command.
 */
static void
d_tests_arg_sweep_extra_numbers(
    struct d_arg_command* _app
)
{
    d_tests_arg_sweep_checked(_app,
                              "--bound",
                              d_arg_validator_bound_long(1,
                                                         10),
                              true);
    d_tests_arg_sweep_checked(
        _app,
        "--number",
        d_arg_validator_and(d_arg_validator_number(),
                            d_arg_validator_type(
                                d_arg_type_builtin(
                                    (int)D_ARG_BUILTIN_DOUBLE))),
        false);
    d_tests_arg_sweep_checked(_app,
                              "--bytes",
                              d_arg_validator_as_size_value(false),
                              true);

    return;
}

/**
 * @brief Gives the validators scenario CLI11's sets and mappings, each
 *        through a filter.
 *
 * @param[in,out] _app     the command.
 * @param[in]     _filter  the filter, whose context each copies.
 */
static void
d_tests_arg_sweep_extra_sets(
    struct d_arg_command*      _app,
    const struct d_arg_filter* _filter
)
{
    static const char* const          MEMBERS[] =
    {
        "alpha", "Beta", "gamma_ray"
    };
    static const struct d_arg_mapping PAIRS[]   =
    {
        { "one", "1" },
        { "two", "2" }
    };

    d_tests_arg_sweep_checked(_app,
                              "--member",
                              d_arg_validator_is_member(NULL,
                                                        MEMBERS,
                                                        3u,
                                                        _filter),
                              false);
    d_tests_arg_sweep_checked(_app,
                              "--mapped",
                              d_arg_validator_transformer(NULL,
                                                          PAIRS,
                                                          2u,
                                                          _filter),
                              true);
    d_tests_arg_sweep_checked(_app,
                              "--checked",
                              d_arg_validator_checked_transformer(NULL,
                                                                  PAIRS,
                                                                  2u,
                                                                  _filter),
                              true);

    return;
}

/**
 * @brief Gives the validators scenario the extra checks -- bounds, a
 *        number's, a type's, sizes -- CLI11's sets and mappings, each
 *        through built-in filters and a function of the caller's whose
 *        context each copies, and a number with units.
 *
 * @param[in,out] _app  the command.
 */
static void
d_tests_arg_sweep_extra(
    struct d_arg_command* _app
)
{
    static const struct d_arg_mapping UNITS[] =
    {
        { "k", "1000" },
        { "m", "1000000" }
    };
    void* const               context = d_tests_arg_sweep_context();
    const struct d_arg_filter filter  =
    {
        D_ARG_FILTER_IGNORE_CASE | D_ARG_FILTER_IGNORE_UNDERSCORE,
        d_tests_arg_sweep_filter,
        context,
        (context != NULL) ? d_tests_arg_sweep_copy : NULL,
        (context != NULL) ? free : NULL
    };

    d_tests_arg_sweep_extra_numbers(_app);
    d_tests_arg_sweep_extra_sets(_app,
                                 &filter);
    d_tests_arg_sweep_checked(
        _app,
        "--size",
        d_arg_validator_as_number_with_unit(
            d_arg_type_builtin((int)D_ARG_BUILTIN_DOUBLE),
            UNITS,
            2u,
            D_ARG_UNIT_DEFAULT,
            "SIZE"),
        true);

    // each set kept a copy of its own
    free(context);

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

/**
 * @brief Gives the validators scenario the checks the parser always has: a
 *        range, a custom one owning its context, a composite of them, and
 *        an escaped string's.
 *
 * @param[in,out] _app  the command.
 */
static void
d_tests_arg_sweep_validators_core(
    struct d_arg_command* _app
)
{
    d_tests_arg_sweep_checked(_app,
                              "--range",
                              d_arg_validator_range_long(1,
                                                         10),
                              false);
    d_tests_arg_sweep_checked(_app,
                              "--custom",
                              d_tests_arg_sweep_custom(),
                              false);
    d_tests_arg_sweep_checked(
        _app,
        "--either",
        d_arg_validator_or(d_arg_validator_range_long(1,
                                                      5),
                           d_arg_validator_not(
                               d_arg_validator_range_long(0,
                                                          100))),
        false);
    d_tests_arg_sweep_checked(_app,
                              "--escaped",
                              d_arg_validator_escaped_string(),
                              true);

    return;
}

/**
 * @brief Validators: a range, a custom one owning its context, composites
 *        of them, an escaped string's; bounds, a number's, a type's, sizes,
 *        CLI11's sets and mappings with their filters, and numbers with
 *        units; the file system's; then values that pass, and values that
 *        do not.
 */
static void
d_tests_arg_sweep_validators_run(void)
{
    static char* GOOD[] =
    {
        "prog", "--range", "5", "--custom", "fine", "--either", "3",
        "--escaped", "a\\tb"
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
        , "--bound", "99", "--number", "2.5", "--bytes", "2kb", "--member",
        "ALPHA", "--mapped", "one", "--checked", "2", "--size", "3k"
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
        , "--dir", "/", "--absent", "/no/such/place/at/all"
#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
    };
    static char* BAD[] =
    {
        "prog", "--range", "50", "--custom", "bad"
    };
    struct d_arg_parser   parser;
    struct d_arg_command* app = &parser.command;

    d_tests_arg_sweep_parser(&parser);
    d_tests_arg_sweep_validators_core(app);
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    d_tests_arg_sweep_extra(app);
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
    d_tests_arg_sweep_checked(app,
                              "--dir",
                              d_arg_validator_existing_directory(),
                              false);
    d_tests_arg_sweep_checked(app,
                              "--absent",
                              d_arg_validator_nonexistent_path(),
                              false);
#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

    d_tests_arg_sweep_parse(&parser,
                            (int)(sizeof(GOOD) / sizeof(GOOD[0])),
                            GOOD);
    d_tests_arg_sweep_parse(&parser,
                            (int)(sizeof(BAD) / sizeof(BAD[0])),
                            BAD);
    d_tests_arg_sweep_help(app);
    d_arg_parser_release(&parser);

    return;
}

// 2.5    Text
//------------------------------------------------------------------------------

/**
 * @brief Splits a line by each set of rules, keeping nothing.
 *
 * @param[in] _line  the line.
 */
static void
d_tests_arg_sweep_split(
    const char* _line
)
{
    static const int RULES[] =
    {
        (int)D_ARG_QUOTING_POSIX,
        (int)D_ARG_QUOTING_WINDOWS,
        (int)D_ARG_QUOTING_GNU,
        (int)D_ARG_QUOTING_NATIVE
    };

    // each set of rules, with the program's name, then without it
    for (size_t index = 0u;
         index < (sizeof(RULES) / sizeof(RULES[0]));
         ++index)
    {
        struct d_arg_string_list words = D_ARG_LIST_INIT;
        struct d_arg_buffer      error = D_ARG_BUFFER_INIT;

        (void)d_arg_split_line(d_pack_text_from_cstr(_line),
                               RULES[index],
                               D_ARG_SPLIT_PROGRAM_NAME,
                               &words,
                               &error);
        d_arg_string_list_release(&words);
        d_arg_buffer_release(&error);
        (void)d_arg_split_line(d_pack_text_from_cstr(_line),
                               RULES[index],
                               D_ARG_SPLIT_LINE_BREAKS,
                               &words,
                               &error);
        d_arg_string_list_release(&words);
        d_arg_buffer_release(&error);
    }

    return;
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief Writes a command's configuration in a mode, twice, held to
 *        "written whole".
 *
 * @param[in] _command            the command.
 * @param[in] _mode               a d_arg_config_output_mode value.
 * @param[in] _write_description  whether descriptions are written.
 */
static void
d_tests_arg_sweep_config(
    const struct d_arg_command* _command,
    int                         _mode,
    bool                        _write_description
)
{
    struct d_tests_arg_sweep_digest first;
    struct d_tests_arg_sweep_digest second;
    const int                       wrote  =
        d_arg_command_config_to_sink(_command,
                                     _mode,
                                     _write_description,
                                     d_tests_arg_sweep_digest_sink(&first));
    const int                       again  =
        d_arg_command_config_to_sink(_command,
                                     _mode,
                                     _write_description,
                                     d_tests_arg_sweep_digest_sink(&second));

    d_tests_arg_sweep_whole(wrote,
                            &first,
                            again,
                            &second);

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

/**
 * @brief Gives the text scenario's root what its lines and configuration
 *        give values to: a flag, a name, a list taking any number of values,
 *        and a subcommand.
 *
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_sweep_text_defined(
    struct d_arg_command* _app
)
{
    struct d_arg_command* sub    = NULL;
    struct d_arg_option*  made   = NULL;
    size_t                before = 0u;

    before = _app->options.count;
    made   = d_arg_command_add_flag(_app,
                                    "-v,--verbose",
                                    "say more");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "--name",
                                      "a name");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    before = _app->options.count;
    made   = d_arg_command_add_option(_app,
                                      "--list",
                                      "a list");
    d_tests_arg_sweep_option_held(_app,
                                  before,
                                  made);
    d_arg_option_set_expected_range(made,
                                    1,
                                    D_ARG_EXPECTED_UNLIMITED);
    before = _app->subcommands.count;
    sub    = d_arg_command_add_subcommand(_app,
                                          "sub",
                                          "a subcommand");
    d_tests_arg_sweep_command_held(_app,
                                   before,
                                   sub);

    return;
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief Reads the text scenario's configuration from text, then writes it
 *        in each mode.
 *
 * @param[in,out] _parser  the parser.
 */
static void
d_tests_arg_sweep_text_config(
    struct d_arg_parser* _parser
)
{
    (void)d_arg_parser_parse_config(_parser,
                                    d_pack_text_from_cstr(
                                        "name = \"from a file\"\n"
                                        "list = [1, 2, 3]\n"
                                        "verbose = true\n"
                                        "[sub]\n"
                                        "unknown = 4\n"));
    d_tests_arg_sweep_config(&_parser->command,
                             (int)D_ARG_CONFIG_OUTPUT_ACTIVE,
                             true);
    d_tests_arg_sweep_config(&_parser->command,
                             (int)D_ARG_CONFIG_OUTPUT_ALL_DEFAULTS,
                             false);

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

/**
 * @brief Gives the text scenario's parser the completion option, then
 *        writes every shell's script, each twice, held to "written whole".
 *
 * @param[in,out] _parser  the parser.
 */
static void
d_tests_arg_sweep_text_completion(
    struct d_arg_parser* _parser
)
{
    struct d_arg_command* const app    = &_parser->command;
    const size_t                before = app->options.count;
    struct d_arg_option* const  made   = d_arg_parser_set_completion_flag(
                                             _parser,
                                             "--completion",
                                             NULL);

    d_tests_arg_sweep_option_held(app,
                                  before,
                                  made);

    // each shell, twice
    for (int shell = 0; shell < (int)D_ARG_SHELL_COUNT; ++shell)
    {
        struct d_tests_arg_sweep_digest first;
        struct d_tests_arg_sweep_digest second;
        const int                       wrote  =
            d_arg_completion_script(app,
                                    shell,
                                    d_pack_text_from_cstr(""),
                                    d_tests_arg_sweep_digest_sink(&first));
        const int                       again  =
            d_arg_completion_script(app,
                                    shell,
                                    d_pack_text_from_cstr(""),
                                    d_tests_arg_sweep_digest_sink(&second));

        d_tests_arg_sweep_whole(wrote,
                                &first,
                                again,
                                &second);
    }

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

/**
 * @brief Text in and out: lines split by each set of rules and parsed;
 *        configuration read from text and written in each mode; the
 *        completion option and every shell's script; and a name given
 *        wrong, which the parse answers with the one it most likely meant.
 */
static void
d_tests_arg_sweep_text_run(void)
{
    static char* MISSPELT[] =
    {
        "prog", "--verbos"
    };
    struct d_arg_parser parser;

    d_tests_arg_sweep_split("prog --name \"two words\" 'quoted' a\\ b "
                            "--list=1,2 \"unclosed");
    d_tests_arg_sweep_parser(&parser);
    d_tests_arg_sweep_text_defined(&parser.command);

    // a line, parsed whole
    (void)d_arg_parser_parse_line(&parser,
                                  d_pack_text_from_cstr(
                                      "prog --name \"two words\" -v"),
                                  true);
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    d_tests_arg_sweep_text_config(&parser);
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
    d_tests_arg_sweep_text_completion(&parser);
#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

    // a name given wrong
    d_tests_arg_sweep_parse(&parser,
                            (int)(sizeof(MISSPELT) / sizeof(MISSPELT[0])),
                            MISSPELT);
    d_arg_parser_release(&parser);

    return;
}

//==============================================================================
// 3.  THE SWEEPS
//==============================================================================

/*
d_tests_arg_sweep_options
  Options, every allocation refused in turn.
  Tests the following:
  - options bound to an int, a double and a string, with typed and text
    defaults, an environment name, the option defaults' group and capture
  - a callback, an each() function, a type name's and a default's
    functions, each owning its context, which goes with a refused option
  - several values, case ignored, a positional needing another
  - a parse, the results, help in each mode; nothing live after release,
    no refused option kept
*/
bool
d_tests_arg_sweep_options(void)
{
    return d_tests_arg_sweep_run("options",
                                 d_tests_arg_sweep_options_run);
}

/*
d_tests_arg_sweep_flags
  Flags, every allocation refused in turn.
  Tests the following:
  - plain flags, flags carrying values and negated names, flags bound to a
    counting int and a bool, flags calling functions that own contexts
  - a flag the option defaults refuse once case is ignored, removed whole
  - a parse and help; nothing live after release, no refused flag kept
*/
bool
d_tests_arg_sweep_flags(void)
{
    return d_tests_arg_sweep_run("flags",
                                 d_tests_arg_sweep_flags_run);
}

/*
d_tests_arg_sweep_commands
  Commands, every allocation refused in turn.
  Tests the following:
  - a subcommand with an alias, a footer and an option; option groups, one
    in another; an option moved into a group
  - the help-all flag, the version flag by text and by function, the
    configuration option, each refused whole
  - options retired by name and in place, one deprecated
  - callbacks, text functions and a failure message owning contexts
  - a parse running the subcommand, a failed one written as main writes it,
    help; nothing live after release, no refused definition kept
*/
bool
d_tests_arg_sweep_commands(void)
{
    return d_tests_arg_sweep_run("commands",
                                 d_tests_arg_sweep_commands_run);
}

/*
d_tests_arg_sweep_validators
  Validators, every allocation refused in turn.
  Tests the following:
  - a range, a custom validator owning its context, composites, an escaped
    string's
  - bounds, a number's, a type's, sizes, CLI11's sets and mappings through
    filters whose contexts they copy, and numbers with units, where
    EXTRA_VALIDATORS builds them
  - the file system's, where PATH_VALIDATORS builds them
  - values that pass and values that do not; nothing live after release
*/
bool
d_tests_arg_sweep_validators(void)
{
    return d_tests_arg_sweep_run("validators",
                                 d_tests_arg_sweep_validators_run);
}

/*
d_tests_arg_sweep_text
  Text in and out, every allocation refused in turn.
  Tests the following:
  - a line split by each set of rules, with the program's name and with
    line breaks, and a line parsed whole
  - configuration read from text and written in two modes, where
    CONFIG_FILES builds it
  - the completion option and each shell's script, where COMPLETION builds
    them
  - a name given wrong, answered with a suggestion where SUGGESTIONS
    builds them; nothing live after release
*/
bool
d_tests_arg_sweep_text(void)
{
    return d_tests_arg_sweep_run("text",
                                 d_tests_arg_sweep_text_run);
}

//==============================================================================
// 4.  THE SUITE
//==============================================================================

// d_tests_arg_sweep
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_sweep[] =
{
    { "arg_sweep_options",
      d_tests_arg_sweep_options },
    { "arg_sweep_flags",
      d_tests_arg_sweep_flags },
    { "arg_sweep_commands",
      d_tests_arg_sweep_commands },
    { "arg_sweep_validators",
      d_tests_arg_sweep_validators },
    { "arg_sweep_text",
      d_tests_arg_sweep_text }
};

// d_tests_arg_sweep_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_sweep_count =
    sizeof(d_tests_arg_sweep) / sizeof(d_tests_arg_sweep[0]);
