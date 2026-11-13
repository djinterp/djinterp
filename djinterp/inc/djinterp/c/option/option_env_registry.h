/*******************************************************************************
* djinterp [c]                                             option_env_registry.h
*
*   The djin registry: the program's root frame, and the list of the djins
* beneath it. A djin is a named frame -- a printer, a logger, desc, a tool's
* settings -- that declares its own variables; registered, it becomes the
* registry frame's child, so it sees the program-wide variables the root
* declares, and the registry can reach every setting of the program by its
* full name, "env_printer.style" or "env_printer.cpp_features.key_width".
*   It is dawk's dsettings folded into option_env: every setting in one
* place, with its type, its default and the knob it came from, the values it
* accepts, where its value came from, and a suggestion for a name that is
* not one. Settings arrive by full name from a file, the environment or the
* command line, each from its source, so the order they arrive in cannot
* change the outcome.
*     d_option_env_registry_add(d_option_env_registry(), &my_djin);
*     d_option_env_registry_import(d_option_env_registry(), "DJINTERP_");
*     d_option_env_registry_take_option(d_option_env_registry(), argc, argv,
*                                       &at, &result);   // --my_djin.x=1
*   A machine made after its djin is registered -- a second printer, say --
* makes its own frame a child of the djin's, and binds there what is its
* alone.
*
*   The program's registry is static, its room set by cfg_option_env.h, and
* initialised before the program runs, so no first call can race another.
* It is written by one thread at a time: register and configure before the
* program starts its threads, or keep them out of each other's way.
*
*   On the option core's 64-bit floor, as option_env.h is.
*
*
* path:      /inc/djinterp/c/option/option_env_registry.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE REGISTRY
    ------------
    1.  Storage
         1.  dj_option_env_registry_storage
         2.  dj_option_env_registry
    2.  Setting up
    3.  Djins
2.  SETTINGS BY FULL NAME
    ---------------------
    1.  Binding
    2.  The command line
    3.  Did you mean
*/

#ifndef DJINTERP_C_OPTION_ENV_REGISTRY_H
#define DJINTERP_C_OPTION_ENV_REGISTRY_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../djinterp.h"                           // framework root
#include "./option_common.h"                       // dj_option_result
#include "./option_env.h"                          // struct dj_option_env
#include "../../config/c/option/cfg_option_env.h"  // the registry's room
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64, uint32_t

#if RE_STD_HAS_INT64


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  THE REGISTRY
//==============================================================================


// 1.1    Storage
//------------------------------------------------------------------------------
// 1.1.1
// dj_option_env_registry_storage
//   struct: the root frame's storage, sized by cfg_option_env.h.
D_OPTION_ENV_DECLARE(dj_option_env_registry_storage,
                     D_CFG_OPTION_ENV_REGISTRY_BINDINGS,
                     D_CFG_OPTION_ENV_REGISTRY_VARIABLES,
                     D_CFG_OPTION_ENV_REGISTRY_TEXT);

// 1.1.2
// dj_option_env_registry
//   struct: a registry: its root frame, the djins beneath it, and the root's
// storage. It points into itself, so it is not copied.
struct dj_option_env_registry
{
    struct dj_option_env  frame;       // the root
    struct dj_option_env* djins[D_CFG_OPTION_ENV_REGISTRY_DJINS];
    uint32_t              djin_count;
    uint32_t              reserved;    // 0

    // the root's room
    struct dj_option_env_registry_storage storage;
};

// 1.2    Setting up
//------------------------------------------------------------------------------
/**
 * @brief The program's registry: static, ready before the program runs, and
 *        the one every module that registers a djin registers it in.
 *
 * @return the registry; never null.
 */
struct dj_option_env_registry*
        d_option_env_registry(void);

/**
 * @brief Makes a registry of the caller's own -- a test's, a sandbox's --
 *        empty, its root named `_name`.
 *
 * @param[out] _registry  the registry.
 * @param[in]  _name      the root's name, an identifier; borrowed; or null.
 * @return D_OPTION_STATUS_OK, or INVALID_ARGUMENT for a null registry or a
 *         name that is not an identifier.
 */
struct dj_option_result d_option_env_registry_init(
                            struct dj_option_env_registry* _registry,
                            const char*                    _name);

// the registry's root frame, for program-wide declarations and for a frame
// that wants it as parent without being a djin
struct dj_option_env*
        d_option_env_registry_frame(struct dj_option_env_registry* _registry);

// 1.3    Djins
//------------------------------------------------------------------------------
/**
 * @brief Registers a djin: lists it, and makes it the root's child.
 *
 * @param[in,out] _registry  the registry.
 * @param[in,out] _djin      the djin's frame: named, with no children yet;
 *                           borrowed, so it must outlive its registration.
 * @return its place in the list; KEY_DUPLICATE for a name a djin has,
 *         INVALID_ARGUMENT for a frame with no name or with children,
 *         CAPACITY when the list is full.
 * @post The djin's own bindings are gone (d_option_env_set_parent), and its
 *       own variables' ids carry its new depth, 1.
 */
struct dj_option_result d_option_env_registry_add(
                            struct dj_option_env_registry* _registry,
                            struct dj_option_env*          _djin);

/**
 * @brief Takes a djin off the list, and lets go of it as its parent.
 *
 * @param[in,out] _registry  the registry.
 * @param[in,out] _djin      the djin.
 * @return its place in the list; KEY_ABSENT for one not listed.
 */
struct dj_option_result d_option_env_registry_remove(
                            struct dj_option_env_registry* _registry,
                            struct dj_option_env*          _djin);

// the djins listed; the one at a place, or null past the last; the one of a
// name, or null
uint32_t    d_option_env_registry_count(
                const struct dj_option_env_registry* _registry);
struct dj_option_env*
            d_option_env_registry_at(
                const struct dj_option_env_registry* _registry,
                uint32_t                             _index);
struct dj_option_env*
            d_option_env_registry_find(
                const struct dj_option_env_registry* _registry,
                const char*                          _name,
                size_t                               _size);

/**
 * @brief The frame a full name addresses: the djin its first part names,
 *        or the root.
 *
 * @param[in] _registry  the registry.
 * @param[in] _name      the full name; need not end in a NUL.
 * @param[in] _size      its length.
 * @return the frame, which resolves the name as written; null for a null
 *         registry or name.
 */
struct dj_option_env*
            d_option_env_registry_target(
                struct dj_option_env_registry* _registry,
                const char*                    _name,
                size_t                         _size);


//==============================================================================
// 2.  SETTINGS BY FULL NAME
//==============================================================================
//   A full name is "djin.variable" or "djin.scope.variable" for a djin's
// variable, and "variable" or "scope.variable" for one the root declares.


// 2.1    Binding
//------------------------------------------------------------------------------
/**
 * @brief Binds a setting by full name from text, from a source.
 *
 * @param[in,out] _registry    the registry.
 * @param[in]     _name        the full name; need not end in a NUL.
 * @param[in]     _name_size   its length.
 * @param[in]     _value       the text; need not end in a NUL.
 * @param[in]     _value_size  its length.
 * @param[in]     _source      the source.
 * @return as d_option_env_assign_from, in the frame the name addresses.
 */
struct dj_option_result d_option_env_registry_assign(
                            struct dj_option_env_registry* _registry,
                            const char*                    _name,
                            size_t                         _name_size,
                            const char*                    _value,
                            size_t                         _value_size,
                            int32_t                        _source);

/**
 * @brief Carries out one statement whose name is a full name.
 *
 * @param[in,out] _registry   the registry.
 * @param[in]     _statement  the statement, NUL-terminated.
 * @param[in]     _source     the source.
 * @return as d_option_env_put_from.
 */
struct dj_option_result d_option_env_registry_put(
                            struct dj_option_env_registry* _registry,
                            const char*                    _statement,
                            int32_t                        _source);

/**
 * @brief Carries out a script of statements whose names are full names, as
 *        d_option_env_load_from splits one; each goes to the frame its name
 *        addresses.
 *
 * @param[in,out] _registry      the registry.
 * @param[in]     _script        the script, NUL-terminated.
 * @param[in]     _source        the source: FILE for a file's text.
 * @param[out]    _error_offset  where a failing statement starts; may be
 *                               null.
 * @return the number of statements done; or the first failure.
 */
struct dj_option_result d_option_env_registry_load(
                            struct dj_option_env_registry* _registry,
                            const char*                    _script,
                            int32_t                        _source,
                            size_t*                        _error_offset);

/**
 * @brief Binds settings from the process's environment: the root's under
 *        `_prefix` -- "DJINTERP_COLOR" -- and each djin's under the prefix
 *        and its name -- "DJINTERP_ENV_PRINTER_STYLE".
 *
 * @note A djin sees the root's variables too, so the djin's prefix sets one
 *       of them for that djin alone: "DJINTERP_ENV_PRINTER_COLOR".
 *
 * @param[in,out] _registry  the registry.
 * @param[in]     _prefix    the prefix; null or empty for none.
 * @return the number bound; or the first failure, the rest still read.
 */
struct dj_option_result d_option_env_registry_import(
                            struct dj_option_env_registry* _registry,
                            const char*                    _prefix);

// 2.2    The command line
//------------------------------------------------------------------------------
/**
 * @brief Takes one setting off a command line: "--set NAME=VALUE",
 *        "--NAME=VALUE", "--NAME" for a bool's true, "--no-NAME" for its
 *        false; NAME a full name, bound from the command line.
 *
 * @note "--NAME" and "--no-NAME" are a bool's alone: on a setting of any
 *       other kind they are -1, INVALID_ARGUMENT, and nothing is bound.
 *
 * @param[in,out] _registry  the registry.
 * @param[in]     _argc      the arguments' count.
 * @param[in]     _argv      the arguments.
 * @param[in,out] _at        the one to look at; on return, the last one
 *                           read: "--set"'s value after "--set NAME=VALUE",
 *                           and unchanged otherwise, whatever the answer. The
 *                           caller steps past it, as a for loop's ++ does.
 * @param[out]    _result    the binding's result when one was tried; may be
 *                           null.
 * @return 1 when it took a setting, 0 when the argument is not one -- an
 *         operand, "--", or no "--" at all -- and -1 when it named a
 *         setting badly: the result says why, and
 *         d_option_env_registry_suggest what was meant.
 */
int32_t d_option_env_registry_take_option(
            struct dj_option_env_registry* _registry,
            int                            _argc,
            char* const*                   _argv,
            int*                           _at,
            struct dj_option_result*       _result);

// 2.3    Did you mean
//------------------------------------------------------------------------------
/**
 * @brief The full name nearest one that names nothing: its failing part --
 *        the djin, the scope, the variable -- replaced by the nearest that
 *        exists, the rest kept.
 *
 * @param[in]  _registry  the registry.
 * @param[in]  _name      the full name, as written; need not end in a NUL.
 * @param[in]  _size      its length.
 * @param[out] _buffer    where; may be null when _capacity is 0.
 * @param[in]  _capacity  its size.
 * @return the suggestion's length, written as snprintf writes; 0 when
 *         nothing is near.
 */
size_t d_option_env_registry_suggest(
           struct dj_option_env_registry* _registry,
           const char*                    _name,
           size_t                         _size,
           char*                          _buffer,
           size_t                         _capacity);


D_EXTERN_C_END


#endif  // RE_STD_HAS_INT64

#endif  // DJINTERP_C_OPTION_ENV_REGISTRY_H
