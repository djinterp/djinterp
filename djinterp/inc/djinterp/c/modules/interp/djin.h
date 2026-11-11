/*******************************************************************************
* djinterp [c]                                                            djin.h
*
*   Djins: the named, versioned units of djinterp a program can list,
* describe, configure and run (docs/djinterp/interp/DESIGN.md, section 6). A
* djin is a record of constant data -- its name, version and summary, the
* commands it registers, and its settings, which are its frame in the
* program's option_env registry -- so that a djin can be written in C or C++
* and, later, carried by a module the command-line kit loads.
*   A program keeps its djins in a set over its registry. Registering a djin
* checks its record, refuses a name another djin has or one the program keeps
* for its own commands, and registers the djin's frame in the registry, so
* that its settings are reached by full name ("awk.fs") from a settings
* file, the environment and the command line. The set then lists the djins
* and describes one, as text, for the program's `list` and `describe`.
*     static const struct dj_djin* room[16];
*     struct dj_djin_set           djins;
*     d_djin_set_init(&djins, room, 16, d_option_env_registry(), reserved);
*     d_djin_register(&djins, &my_djin);
*     d_djin_write_list(&djins, sink);
*   A djin's command runs through a call record, dj_djin_call, whose shape
* follows the tagged value's header; it is declared here and defined there.
*
*   The set is written by one thread at a time: register before the program
* starts its threads. On the option core's 64-bit floor, as option_env.h is.
*
*
* path:      /inc/djinterp/c/modules/interp/djin.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE RECORDS
    -----------
    1.  dj_djin_command
    2.  dj_djin
    3.  D_DJIN_ABI, D_DJIN_COMMAND, D_DJIN
2.  THE SET
    -------
    1.  dj_djin_status
    2.  dj_djin_set
    3.  Setting up and registering
    4.  Finding
3.  AS TEXT
    -------
    1.  The list
    2.  A description
*/

#ifndef DJINTERP_C_INTERP_DJIN_H
#define DJINTERP_C_INTERP_DJIN_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"                     // framework root
#include "../../option/option_env.h"            // struct dj_option_env
#include "../../option/option_env_registry.h"   // the program's registry
#include "../../util/sink_common.h"             // struct dj_pack_sink
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64

#if RE_STD_HAS_INT64


D_EXTERN_C_BEGIN

// the kit's program, the argument parser's command, and a command's call,
// which the tagged value's header defines
struct dj_cli_program;
struct dj_arg_command;
struct dj_djin_call;


//==============================================================================
// 1.  THE RECORDS
//==============================================================================


// 1.1
// dj_djin_command
//   struct: a command a djin registers: its verb, a lowercase word (R1,
// cli-taken 66) of letters, digits and '-', starting with a letter; the
// line the command list shows; the function that defines its options and
// operands through the kit; and the function that runs it. `define` and
// `run` may be null while a djin has no command to run yet. The record grows
// by `size`, as the kit's records do.
struct dj_djin_command
{
    uint32_t    size;     // sizeof(struct dj_djin_command), as built
    const char* name;     // "run"
    const char* summary;  // one line
    int       (*define)(struct dj_cli_program* _program,
                        struct dj_arg_command* _command);
    int       (*run)(struct dj_djin_call* _call);
};

// 1.2
// dj_djin
//   struct: a djin: its name, a lowercase identifier (letters, digits and
// '_', starting with a letter), which its settings' full names begin with;
// its version and summary; its commands; its settings, as its frame in the
// program's registry, named as the djin, or null for a djin with none; and
// the function that gives its description's facts to desc's sink, whose
// type desc settles (null until then, or for a djin with nothing to add).
struct dj_djin
{
    uint32_t                      abi;            // D_DJIN_ABI
    uint32_t                      size;           // sizeof(struct dj_djin)
    const char*                   name;           // "awk"
    const char*                   version;        // "0.9.0"
    const char*                   summary;        // one line
    const struct dj_djin_command* commands;       // or null
    size_t                        command_count;  // entries in commands
    struct dj_option_env*         settings;       // or null
    int                         (*describe)(const struct dj_djin* _djin,
                                            void*                 _facts);
};

// 1.3
// D_DJIN_ABI, D_DJIN_COMMAND, D_DJIN
//   the records' layout version, which d_djin_register checks, and brace
// initialisers for the two records, for a constant table (C++98 cannot
// build them member by member).
#define D_DJIN_ABI 1u

#define D_DJIN_COMMAND(_name, _summary, _define, _run)                       \
    { (uint32_t)sizeof(struct dj_djin_command), (_name), (_summary),         \
      (_define), (_run) }

#define D_DJIN(_name, _version, _summary, _commands, _count, _settings)      \
    { D_DJIN_ABI, (uint32_t)sizeof(struct dj_djin), (_name), (_version),     \
      (_summary), (_commands), (_count), (_settings), NULL }


//==============================================================================
// 2.  THE SET
//==============================================================================


// 2.1
// dj_djin_status
//   enum: what registering a djin came to.
enum dj_djin_status
{
    D_DJIN_STATUS_OK        = 0,  // registered
    D_DJIN_STATUS_INVALID   = 1,  // a null set or record, another ABI, a
                                  // record smaller than this build's, or a
                                  // name, command or summary out of shape
    D_DJIN_STATUS_DUPLICATE = 2,  // a djin of that name is registered, or
                                  // another frame of it is in the registry
    D_DJIN_STATUS_RESERVED  = 3,  // the name is one of the program's own
    D_DJIN_STATUS_FULL      = 4,  // the set's room is used up
    D_DJIN_STATUS_SETTINGS  = 5   // the registry refused the djin's frame
};

// 2.2
// dj_djin_set
//   struct: a program's djins, in the order they were registered, in room
// the caller gives; the registry their settings are in; and the names the
// program keeps for its own commands, which no djin may take.
struct dj_djin_set
{
    const struct dj_djin**         items;     // the caller's room
    uint32_t                       capacity;  // its size
    uint32_t                       count;     // registered
    struct dj_option_env_registry* registry;  // or null: no settings
    const char* const*             reserved;  // null-terminated, or null
};

// 2.3    Setting up and registering
//------------------------------------------------------------------------------
/**
 * @brief Makes an empty set over the caller's room.
 *
 * @param[out] _set        the set.
 * @param[in]  _room       room for `_capacity` djins; borrowed.
 * @param[in]  _capacity   its size.
 * @param[in]  _registry   the registry the djins' settings go in, usually
 *                         d_option_env_registry(); or null for none.
 * @param[in]  _reserved   the program's own command names, ending in a null;
 *                         borrowed; or null.
 * @return D_DJIN_STATUS_OK, or INVALID for a null set, or null room with a
 *         capacity.
 */
int32_t d_djin_set_init(struct dj_djin_set*            _set,
                        const struct dj_djin**         _room,
                        uint32_t                       _capacity,
                        struct dj_option_env_registry* _registry,
                        const char* const*             _reserved);

/**
 * @brief Registers a djin: checks its record and lists it, and registers its
 *        settings' frame in the set's registry, unless the frame is already
 *        there, as a module's static frame may be.
 *
 * @param[in,out] _set   the set.
 * @param[in]     _djin  the record; borrowed, so it must outlive the set.
 * @return D_DJIN_STATUS_OK, or the status that says why not; nothing is
 *         changed when it is not OK.
 * @pre The program has no threads yet, or none that reads the set.
 */
int32_t d_djin_register(struct dj_djin_set*   _set,
                        const struct dj_djin* _djin);

// a status's name, "ok" or "duplicate", say; "unknown" for any other value
const char* d_djin_status_name(int32_t _status);

// 2.4    Finding
//------------------------------------------------------------------------------
// how many djins are registered; the one at a place, null past the last; the
// one of a name, which need not end in a NUL, or null
uint32_t              d_djin_set_count(const struct dj_djin_set* _set);
const struct dj_djin* d_djin_set_at(const struct dj_djin_set* _set,
                                    uint32_t                  _index);
const struct dj_djin* d_djin_set_find(const struct dj_djin_set* _set,
                                      const char*               _name,
                                      size_t                    _size);

// a djin's command of a name, which need not end in a NUL, or null
const struct dj_djin_command*
                      d_djin_command_find(const struct dj_djin* _djin,
                                          const char*           _name,
                                          size_t                _size);


//==============================================================================
// 3.  AS TEXT
//==============================================================================


// 3.1    The list
//------------------------------------------------------------------------------
/**
 * @brief Writes the djins, a line each, in the order they were registered:
 *        the name, the version and the summary, each column as wide as its
 *        widest entry. A registry frame no djin record names -- a module's
 *        settings alone -- follows, with "-" for its version and "settings
 *        only" for its summary.
 *
 * @param[in] _set   the set.
 * @param[in] _sink  where the text goes.
 * @return 0, or -1 for a null set or a sink that took less than it was given.
 */
int d_djin_write_list(const struct dj_djin_set* _set,
                      struct dj_pack_sink       _sink);

// 3.2    A description
//------------------------------------------------------------------------------
/**
 * @brief Writes what a djin is: its name and version, its summary, its
 *        commands (each with its summary), and its settings, a line each:
 *        the full name, the kind, the default as text, the knob the default
 *        comes from, and the help. A section with nothing in it is left out.
 *
 * @param[in] _djin  the djin.
 * @param[in] _sink  where the text goes.
 * @return 0, or -1 for a null djin or a sink that took less than it was
 *         given.
 */
int d_djin_write_description(const struct dj_djin* _djin,
                             struct dj_pack_sink   _sink);

/**
 * @brief As d_djin_write_description, for a frame of the registry that no
 *        djin record names: its name, and its settings.
 *
 * @param[in] _frame  the frame.
 * @param[in] _sink   where the text goes.
 * @return 0, or -1 for a null frame or a sink that took less than it was
 *         given.
 */
int d_djin_write_frame(const struct dj_option_env* _frame,
                       struct dj_pack_sink         _sink);


D_EXTERN_C_END


#endif  // RE_STD_HAS_INT64


#endif  // DJINTERP_C_INTERP_DJIN_H
