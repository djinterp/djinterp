/*******************************************************************************
* djinterp [parse]                                                  cli_plugin.c
*
* A program's commands that live in modules of their own, declared in
* cli_plugin.h.
*   Finding a module is string work over three kinds of directory; loading
* one is cli_system.c's, and checking it is reading its record. The host's
* command takes everything after its name as the parser leaves it -- a prefix
* command with no options of its own, its help flag removed and the root's
* options not falling through -- so the module reads every argument itself,
* `--help` among them.
*
*
* path:      /src/djinterp/parse/cli/c/cli_plugin.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "../../../../../inc/djinterp/parse/cli/c/cli_plugin.h"  // corresponding header
// std
#include <errno.h>    // EINVAL
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdlib.h>   // the hooks' malloc, realloc and free
#include <string.h>   // memcpy, memset, strchr, strcmp, strlen, strncmp, strrchr
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/filesys/file_stat.h"  // d_file_is_regular
#include "../../../../../inc/djinterp/config/parse/cli/cfg_cli.h"  // D_INTERNAL_CLI_*
#include "../../../../../inc/djinterp/env/parse/cli/env_cli.h"  // D_ENV_CLI_*
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // d_arg_buffer
#include "../../../../../inc/djinterp/parse/cli/c/cli_io.h"  // d_cli_colour_wanted, d_cli_free
#include "../../../../../inc/djinterp/parse/cli/c/cli_program.h"  // d_cli_program, d_cli_report
#include "./cli_internal.h"  // the system, copies
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  THE PROGRAM'S FILE
//==============================================================================

/*
d_cli_executable_path
  The system's answer, through cli_system.c.
*/
int
d_cli_executable_path(
    const char* _argv0,
    char**      _path
)
{
    // parameter validation
    if (!_path)
    {
        return EINVAL;
    }

    return d_cli_internal_executable(_argv0,
                                     _path);
}


//==============================================================================
// 2.  FINDING
//==============================================================================

/**
 * @brief Where the directory part of a path ends: its last `/` -- or, on
 *        Windows, its last `\\`, whichever is later.
 *
 * @param[in] _path  the path.
 * @return the separator, or NULL when the path has none.
 */
static const char*
d_cli_internal_directory_end(
    const char* _path
)
{
    const char* slash = strrchr(_path,
                                '/');
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    const char* const back = strrchr(_path,
                                     '\\');

    slash = ( (back != NULL) &&
              ( (slash == NULL) ||
                (back > slash) ) )
            ? back
            : slash;
#endif

    return slash;
}

/**
 * @brief How much of a path names its directory: up to its last separator,
 *        and through it where the directory is a root -- `/`, or `C:\\` on
 *        Windows -- which without it would be another directory or none.
 *
 * @param[in] _path  the path.
 * @return the directory's length; 0 when the path names none.
 */
static size_t
d_cli_internal_directory_length(
    const char* _path
)
{
    const char* const slash  = d_cli_internal_directory_end(_path);
    const size_t      length = (slash != NULL)
                               ? (size_t)(slash - _path)
                               : 0u;
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    const bool        root   = ( (length == 0u) ||
                                 (_path[length - 1u] == ':') );
#else
    const bool        root   = (length == 0u);
#endif

    return ( (slash != NULL) &&
             (root) )
           ? (length + 1u)
           : length;
}

/**
 * @brief Appends a name in a directory to a path: the directory, a `/`
 *        unless it ends in a separator already, and the name.
 *
 * @param[in,out] _path       the path.
 * @param[in]     _directory  the directory.
 * @param[in]     _length     its length.
 * @param[in]     _name       the name.
 * @return D_ARG_OK, or the buffer's status when it could not grow.
 */
static int
d_cli_internal_join(
    struct d_arg_buffer* _path,
    const char*          _directory,
    size_t               _length,
    const char*          _name
)
{
    const char last   = (_length > 0u)
                        ? _directory[_length - 1u]
                        : '\0';
    int        status = d_arg_buffer_append(_path,
                                            _directory,
                                            _length);
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    const bool ended  = ( (last == '/') ||
                          (last == '\\') );
#else
    const bool ended  = (last == '/');
#endif

    status = ( (status == D_ARG_OK) &&
               (!ended) )
             ? d_arg_buffer_append_char(_path,
                                        '/')
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_cstr(_path,
                                      _name)
           : status;
}

/**
 * @brief Tries one file name in a directory.
 *
 * @param[in]     _directory  the directory.
 * @param[in]     _length     its length.
 * @param[in]     _file       the module's file name.
 * @param[in,out] _found      receives the path when the file is there.
 * @return D_ARG_OK when found; D_ARG_ERROR_FILE when not; the buffers'
 *         status when they could not grow.
 */
static int
d_cli_internal_try_file(
    const char*          _directory,
    size_t               _length,
    const char*          _file,
    struct d_arg_buffer* _found
)
{
    struct d_arg_buffer path   = D_ARG_BUFFER_INIT;
    int                 status = d_cli_internal_join(&path,
                                                     _directory,
                                                     _length,
                                                     _file);

    // a regular file of that name
    if ( (status == D_ARG_OK) &&
         (d_file_is_regular(path.data) != 0) )
    {
        status = d_arg_buffer_append(_found,
                                     path.data,
                                     path.length);
    }
    else if (status == D_ARG_OK)
    {
        status = D_ARG_ERROR_FILE;
    }

    d_arg_buffer_release(&path);

    return status;
}

/**
 * @brief Tries one directory: appends it to the searched list, and the
 *        module's path to `_found` when the directory holds it -- by the
 *        tagged name first, where there is one, then the plain name.
 *
 * @param[in]     _directory  the directory.
 * @param[in]     _length     its length.
 * @param[in]     _tagged     the module's name with the program's tag, or
 *                            NULL when the program's file has none.
 * @param[in]     _plain      the module's name without a tag.
 * @param[in,out] _found      receives the path when found.
 * @param[in,out] _searched   the directories searched; may be NULL.
 * @return D_ARG_OK when found; D_ARG_ERROR_FILE when not; the buffers'
 *         status when they could not grow.
 */
static int
d_cli_internal_try_directory(
    const char*          _directory,
    size_t               _length,
    const char*          _tagged,
    const char*          _plain,
    struct d_arg_buffer* _found,
    struct d_arg_buffer* _searched
)
{
    // an empty entry of a list names no directory
    if (_length == 0u)
    {
        return D_ARG_ERROR_FILE;
    }

    int status = D_ARG_OK;

    if (_searched != NULL)
    {
        status = d_arg_buffer_append(_searched,
                                     _directory,
                                     _length);
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_char(_searched,
                                            '\n')
                 : status;
    }

    // the tagged name, then the plain one
    status = ( (status == D_ARG_OK) &&
               (_tagged != NULL) )
             ? d_cli_internal_try_file(_directory,
                                       _length,
                                       _tagged,
                                       _found)
             : (status == D_ARG_OK) ? D_ARG_ERROR_FILE : status;

    return (status == D_ARG_ERROR_FILE)
           ? d_cli_internal_try_file(_directory,
                                     _length,
                                     _plain,
                                     _found)
           : status;
}

/**
 * @brief The module's file names: `PROGRAM-NAME` with the program's tag and
 *        the platform's suffix, and without the tag.
 *
 * @note A program's file may carry a tag after the program's name -- a
 *       build's bin/ directory names bitwisedom `bitwisedom-linux64` and
 *       `bitwisedom-win64.exe`, so every platform's builds can share it --
 *       and its modules carry the same tag: `bitwisedom-gui-linux64.so`. The
 *       tag is what follows the program's name and a `-` in the file's
 *       name, less `.exe` on Windows; a file named as the program has none.
 *
 * @param[in]     _program     the program's name.
 * @param[in]     _name        the command's.
 * @param[in]     _executable  the program's file, or NULL.
 * @param[in,out] _tagged      receives the tagged name; left empty when the
 *                             program's file has no tag.
 * @param[in,out] _plain       receives the plain name.
 * @return D_ARG_OK, or the buffers' status when they could not grow.
 */
static int
d_cli_internal_module_names(
    const char*          _program,
    const char*          _name,
    const char*          _executable,
    struct d_arg_buffer* _tagged,
    struct d_arg_buffer* _plain
)
{
    int status = d_arg_buffer_printf(_plain,
                                     "%s-%s%s",
                                     _program,
                                     _name,
                                     D_ENV_CLI_MODULE_SUFFIX);

    // no file to take a tag from
    if ( (status != D_ARG_OK) ||
         (_executable == NULL) )
    {
        return status;
    }

    const char* const slash  = d_cli_internal_directory_end(_executable);
    const char* const file   = (slash != NULL)
                               ? (slash + 1)
                               : _executable;
    const size_t      own    = strlen(_program);
    size_t            length = strlen(file);

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    // `.exe`, in any case, is not part of the tag
    if ( (length > 4u)                          &&
         (file[length - 4u] == '.')             &&
         ( (file[length - 3u] | 0x20) == 'e' )  &&
         ( (file[length - 2u] | 0x20) == 'x' )  &&
         ( (file[length - 1u] | 0x20) == 'e' ) )
    {
        length -= 4u;
    }
#endif

    // the program's name, a `-`, and something after it
    if ( (length > own + 1u)          &&
         (strncmp(file,
                  _program,
                  own) == 0)          &&
         (file[own] == '-') )
    {
        status = d_arg_buffer_printf(_tagged,
                                     "%s-%s%.*s%s",
                                     _program,
                                     _name,
                                     (int)(length - own),
                                     file + own,
                                     D_ENV_CLI_MODULE_SUFFIX);
    }

    return status;
}

/**
 * @brief The variable a program's modules are looked for along first: the
 *        program's name upper-cased, `-` as `_`, and `_PLUGIN_PATH`.
 *
 * @param[in]     _program   the program's name.
 * @param[in,out] _variable  receives the variable's name.
 * @return D_ARG_OK, or the buffer's status when it could not grow.
 */
static int
d_cli_internal_plugin_variable(
    const char*          _program,
    struct d_arg_buffer* _variable
)
{
    int status = D_ARG_OK;

    for (const char* at = _program;
         (status == D_ARG_OK) && (*at != '\0');
         ++at)
    {
        const char each = ( (*at >= 'a') &&
                            (*at <= 'z') )
                          ? (char)(*at - 'a' + 'A')
                          : (*at == '-') ? '_' : *at;

        status = d_arg_buffer_append_char(_variable,
                                          each);
    }

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_cstr(_variable,
                                      "_PLUGIN_PATH")
           : status;
}

/**
 * @brief Tries each directory a list holds, separated as PATH separates
 *        them, until one holds the module.
 *
 * @param[in]     _list      the list; may be NULL, for none.
 * @param[in]     _tagged    the module's tagged name, or NULL.
 * @param[in]     _plain     its plain name.
 * @param[in,out] _found     receives the path when found.
 * @param[in,out] _searched  the directories searched; may be NULL.
 * @return as d_cli_internal_try_directory; D_ARG_ERROR_FILE for no list.
 */
static int
d_cli_internal_try_list(
    const char*          _list,
    const char*          _tagged,
    const char*          _plain,
    struct d_arg_buffer* _found,
    struct d_arg_buffer* _searched
)
{
    int result = D_ARG_ERROR_FILE;

    for (const char* at = _list; at != NULL; )
    {
        const char* const end    = strchr(at,
                                          D_ENV_CLI_PATH_LIST_SEPARATOR);
        const size_t      length = (end != NULL)
                                   ? (size_t)(end - at)
                                   : strlen(at);

        result = d_cli_internal_try_directory(at,
                                              length,
                                              _tagged,
                                              _plain,
                                              _found,
                                              _searched);
        at     = ( (result == D_ARG_ERROR_FILE) &&
                   (end != NULL) )
                 ? (end + 1)
                 : NULL;
    }

    return result;
}

/**
 * @brief Tries the program's own directory, then `../lib/PROGRAM` from it.
 *
 * @param[in]     _program     the program's name.
 * @param[in]     _executable  the program's file.
 * @param[in]     _tagged      the module's tagged name, or NULL.
 * @param[in]     _plain       its plain name.
 * @param[in,out] _found       receives the path when found.
 * @param[in,out] _searched    the directories searched; may be NULL.
 * @return as d_cli_internal_try_directory; D_ARG_ERROR_FILE for a file
 *         named without a directory.
 */
static int
d_cli_internal_try_beside(
    const char*          _program,
    const char*          _executable,
    const char*          _tagged,
    const char*          _plain,
    struct d_arg_buffer* _found,
    struct d_arg_buffer* _searched
)
{
    const size_t        length = d_cli_internal_directory_length(
                                     _executable);
    struct d_arg_buffer lib    = D_ARG_BUFFER_INIT;
    int                 result = d_cli_internal_try_directory(_executable,
                                                              length,
                                                              _tagged,
                                                              _plain,
                                                              _found,
                                                              _searched);

    // ../lib/PROGRAM, where a POSIX installation puts the modules
    if ( (result == D_ARG_ERROR_FILE) &&
         (length > 0u) )
    {
        result = d_cli_internal_join(&lib,
                                     _executable,
                                     length,
                                     "../lib/");
        result = (result == D_ARG_OK)
                 ? d_arg_buffer_append_cstr(&lib,
                                            _program)
                 : result;
        result = (result == D_ARG_OK)
                 ? d_cli_internal_try_directory(lib.data,
                                                lib.length,
                                                _tagged,
                                                _plain,
                                                _found,
                                                _searched)
                 : result;
    }

    d_arg_buffer_release(&lib);

    return result;
}

/*
d_cli_plugin_find
  PROGRAM_PLUGIN_PATH's directories, the program's own, then ../lib/PROGRAM
from it; the first that holds the module wins, by its tagged name before its
plain one in each.
*/
int
d_cli_plugin_find(
    const char*          _program,
    const char*          _name,
    const char*          _executable,
    struct d_arg_buffer* _found,
    struct d_arg_buffer* _searched
)
{
    // parameter validation
    if ( (!_program) ||
         (!_name)    ||
         (!_found) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer tagged   = D_ARG_BUFFER_INIT;
    struct d_arg_buffer plain    = D_ARG_BUFFER_INIT;
    struct d_arg_buffer variable = D_ARG_BUFFER_INIT;
    int                 status   = d_cli_internal_module_names(_program,
                                                               _name,
                                                               _executable,
                                                               &tagged,
                                                               &plain);
    const char* const   by_tag   = (tagged.length > 0u) ? tagged.data : NULL;

    status = (status == D_ARG_OK)
             ? d_cli_internal_plugin_variable(_program,
                                              &variable)
             : status;

    char* list = NULL;

    // the variable, which there was no room to read is no variable unset
    status = ( (status == D_ARG_OK) &&
               (d_cli_internal_getenv(variable.data,
                                      &list) == ENOMEM) )
             ? D_ARG_ERROR_OUT_OF_MEMORY
             : status;

    int result = (status == D_ARG_OK)
                 ? d_cli_internal_try_list(list,
                                           by_tag,
                                           plain.data,
                                           _found,
                                           _searched)
                 : status;

    D_INTERNAL_CLI_FREE(list);

    // the program's directory, then ../lib/PROGRAM from it
    if ( (result == D_ARG_ERROR_FILE) &&
         (_executable != NULL) )
    {
        result = d_cli_internal_try_beside(_program,
                                           _executable,
                                           by_tag,
                                           plain.data,
                                           _found,
                                           _searched);
    }

    d_arg_buffer_release(&tagged);
    d_arg_buffer_release(&plain);
    d_arg_buffer_release(&variable);

    return result;
}


//==============================================================================
// 3.  LOADING
//==============================================================================

/**
 * @brief Checks a module's record is one of this interface: there, of its
 *        ABI, and as long as its interface makes it.
 *
 * @param[in]     _plugin  the record.
 * @param[in,out] _why     why not, when not.
 * @return `true` if it is.
 */
static bool
d_cli_internal_interface_fits(
    const struct d_cli_plugin* _plugin,
    struct d_arg_buffer*       _why
)
{
    // the record itself
    if (!_plugin)
    {
        (void)d_arg_buffer_append_cstr(_why,
                                       "its entry point returned no record");

        return false;
    }

    if (_plugin->abi != D_CLI_PLUGIN_ABI)
    {
        (void)d_arg_buffer_printf(_why,
                                  "it was built for plugin interface %lu, "
                                  "and this program takes %lu",
                                  (unsigned long)_plugin->abi,
                                  (unsigned long)D_CLI_PLUGIN_ABI);

        return false;
    }

    if (_plugin->size < (uint32_t)sizeof(struct d_cli_plugin))
    {
        (void)d_arg_buffer_append_cstr(_why,
                                       "its record is too short for its "
                                       "interface");

        return false;
    }

    return true;
}

/**
 * @brief Checks a module's record belongs to the program loading it: its
 *        program's, at its version, with work to run.
 *
 * @param[in]     _plugin   the record, of this interface.
 * @param[in]     _program  the program it must belong to.
 * @param[in]     _version  the version it must belong to, or NULL.
 * @param[in,out] _why      why not, when not.
 * @return `true` if it does.
 */
static bool
d_cli_internal_owner_fits(
    const struct d_cli_plugin* _plugin,
    const char*                _program,
    const char*                _version,
    struct d_arg_buffer*       _why
)
{
    if ( (_plugin->program == NULL) ||
         (strcmp(_plugin->program,
                 _program) != 0) )
    {
        (void)d_arg_buffer_printf(_why,
                                  "it belongs to %s, not %s",
                                  (_plugin->program != NULL)
                                  ? _plugin->program
                                  : "no program",
                                  _program);

        return false;
    }

    if ( (_version != NULL) &&
         ( (_plugin->version == NULL) ||
           (strcmp(_plugin->version,
                   _version) != 0) ) )
    {
        (void)d_arg_buffer_printf(_why,
                                  "it belongs to %s %s, and this is %s %s",
                                  _program,
                                  (_plugin->version != NULL)
                                  ? _plugin->version
                                  : "of no version",
                                  _program,
                                  _version);

        return false;
    }

    if (_plugin->run == NULL)
    {
        (void)d_arg_buffer_append_cstr(_why,
                                       "it has no work to run");

        return false;
    }

    return true;
}

/**
 * @brief Checks a module's record against the program loading it.
 *
 * @param[in]     _plugin   the record.
 * @param[in]     _program  the program it must belong to.
 * @param[in]     _version  the version it must belong to, or NULL.
 * @param[in,out] _why      why not, when not; may be NULL.
 * @return `true` if the module belongs.
 */
static bool
d_cli_internal_belongs(
    const struct d_cli_plugin* _plugin,
    const char*                _program,
    const char*                _version,
    struct d_arg_buffer*       _why
)
{
    struct d_arg_buffer        scratch = D_ARG_BUFFER_INIT;
    struct d_arg_buffer* const why     = (_why != NULL) ? _why : &scratch;
    const bool                 belongs =
        ( (d_cli_internal_interface_fits(_plugin,
                                         why)) &&
          (d_cli_internal_owner_fits(_plugin,
                                     _program,
                                     _version,
                                     why)) );

    d_arg_buffer_release(&scratch);

    return belongs;
}

#if (D_INTERNAL_CLI_PLUGINS == 1)

/**
 * @brief Loads a module the program's parameters have been checked for:
 *        open it, find its entry point, read its record, check it; any
 *        refusal closes it again.
 *
 * @param[out]    _module   the module, zeroed.
 * @param[in]     _path     its file.
 * @param[in]     _program  the program it must belong to.
 * @param[in]     _version  the version it must belong to, or NULL.
 * @param[in,out] _why      why it was refused; may be NULL.
 * @return as d_cli_module_load.
 */
static int
d_cli_internal_load(
    struct d_cli_module* _module,
    const char*          _path,
    const char*          _program,
    const char*          _version,
    struct d_arg_buffer* _why
)
{
    void*     handle = NULL;
    const int opened = d_cli_internal_module_open(_path,
                                                  &handle,
                                                  _why);

    // the system has said why, or there was no room to ask it
    if (opened != D_ARG_OK)
    {
        return opened;
    }

    const d_cli_plugin_entry_fn       entry  = d_cli_internal_module_entry(
                                                   handle);
    const struct d_cli_plugin* const plugin = (entry != NULL) ? entry()
                                                              : NULL;
    char* const                       path   = d_cli_internal_copy(_path);
    int                               status = D_ARG_OK;

    // not a module of this interface at all; another program's, another
    // version's or another interface's; or no room for its path
    if ( (!entry) &&
         (_why != NULL) )
    {
        (void)d_arg_buffer_append_cstr(_why,
                                       "it has no entry point "
                                       D_CLI_PLUGIN_ENTRY_NAME);
    }

    status = (!entry)                     ? D_ARG_ERROR_FILE
           : (!d_cli_internal_belongs(plugin,
                                      _program,
                                      _version,
                                      _why))  ? D_ARG_ERROR_INVALID
           : (!path)                      ? D_ARG_ERROR_OUT_OF_MEMORY
                                          : D_ARG_OK;

    if (status != D_ARG_OK)
    {
        D_INTERNAL_CLI_FREE(path);
        d_cli_internal_module_close(handle);

        return status;
    }

    _module->handle = handle;
    _module->plugin = plugin;
    _module->path   = path;

    return D_ARG_OK;
}

#else

/**
 * @brief Refuses every module: the build has no plugins.
 *
 * @param[out]    _module   the module, left zeroed.
 * @param[in]     _path     its file.
 * @param[in]     _program  the program.
 * @param[in]     _version  its version.
 * @param[in,out] _why      receives why; may be NULL.
 * @return D_ARG_ERROR_UNSUPPORTED.
 */
static int
d_cli_internal_load(
    struct d_cli_module* _module,
    const char*          _path,
    const char*          _program,
    const char*          _version,
    struct d_arg_buffer* _why
)
{
    (void)_module;
    (void)_path;
    (void)_program;
    (void)_version;

    if (_why != NULL)
    {
        (void)d_arg_buffer_append_cstr(_why,
                                       "this program was built without "
                                       "plugins");
    }

    return D_ARG_ERROR_UNSUPPORTED;
}

#endif  // D_INTERNAL_CLI_PLUGINS

/*
d_cli_module_load
  The parameters, the module zeroed, and the build's loading.
*/
int
d_cli_module_load(
    struct d_cli_module* _module,
    const char*          _path,
    const char*          _program,
    const char*          _version,
    struct d_arg_buffer* _why
)
{
    // parameter validation
    if ( (!_module) ||
         (!_path)   ||
         (!_program) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    memset(_module,
           0,
           sizeof(*_module));

    return d_cli_internal_load(_module,
                               _path,
                               _program,
                               _version,
                               _why);
}

/*
d_cli_module_unload
  Close, free, forget.
*/
void
d_cli_module_unload(
    struct d_cli_module* _module
)
{
    // nothing loaded
    if (!_module)
    {
        return;
    }

    d_cli_internal_module_close(_module->handle);
    D_INTERNAL_CLI_FREE(_module->path);
    memset(_module,
           0,
           sizeof(*_module));

    return;
}


//==============================================================================
// 4.  IN A PROGRAM
//==============================================================================

#if (D_INTERNAL_CLI_PLUGINS == 1)

// d_cli_internal_arguments
//   struct: the module's arguments as they are gathered: the command's path
// first, then each argument the command left, each a copy.
struct d_cli_internal_arguments
{
    char** items;
    size_t count;
    size_t capacity;
    int    status;
};

/**
 * @brief Keeps a copy of one argument the command left, a
 *        d_arg_extra_visit_fn.
 *
 * @param[in] _extra    the argument.
 * @param[in] _context  the d_cli_internal_arguments.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY.
 */
static int
d_cli_internal_keep_argument(
    const struct d_arg_extra* _extra,
    void*                     _context
)
{
    struct d_cli_internal_arguments* const arguments =
        (struct d_cli_internal_arguments*)_context;

    // room for it and the NULL after the last
    if (arguments->count + 2u > arguments->capacity)
    {
        const size_t room  = (arguments->capacity == 0u)
                             ? 16u
                             : (arguments->capacity * 2u);
        void* const  grown = D_INTERNAL_CLI_REALLOC(arguments->items,
                                                    room * sizeof(char*));

        if (!grown)
        {
            arguments->status = D_ARG_ERROR_OUT_OF_MEMORY;

            return D_ARG_ERROR_OUT_OF_MEMORY;
        }

        arguments->items    = (char**)grown;
        arguments->capacity = room;
    }

    char* const copy = (char*)D_INTERNAL_CLI_MALLOC(_extra->text.length + 1u);

    if (!copy)
    {
        arguments->status = D_ARG_ERROR_OUT_OF_MEMORY;

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    if (_extra->text.length > 0u)
    {
        memcpy(copy,
               _extra->text.data,
               _extra->text.length);
    }

    copy[_extra->text.length]           = '\0';
    arguments->items[arguments->count++] = copy;
    arguments->items[arguments->count]   = NULL;

    return D_ARG_OK;
}

/**
 * @brief Frees the gathered arguments.
 *
 * @param[in,out] _arguments  the arguments.
 */
static void
d_cli_internal_drop_arguments(
    struct d_cli_internal_arguments* _arguments
)
{
    for (size_t index = 0u; index < _arguments->count; ++index)
    {
        D_INTERNAL_CLI_FREE(_arguments->items[index]);
    }

    D_INTERNAL_CLI_FREE(_arguments->items);
    memset(_arguments,
           0,
           sizeof(*_arguments));

    return;
}

/**
 * @brief Says where a module was looked for: a note for each directory.
 *
 * @param[in] _program   the program.
 * @param[in] _searched  the directories, a line each.
 */
static void
d_cli_internal_looked_in(
    const struct d_cli_program* _program,
    const struct d_arg_buffer*  _searched
)
{
    for (const char* at = _searched->data;
         (at != NULL) && (*at != '\0'); )
    {
        const char* const end    = strchr(at,
                                          '\n');
        const int         length = (end != NULL)
                                   ? (int)(end - at)
                                   : (int)strlen(at);

        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_NOTE,
                     "looked in %.*s",
                     length,
                     at);
        at = (end != NULL) ? (end + 1) : NULL;
    }

    return;
}

/**
 * @brief Says where a module was looked for and not found: an error naming
 *        the files, then a note for each directory; or, when there was no
 *        memory to name them, that.
 *
 * @param[in] _program   the program.
 * @param[in] _command   the command's name, which ends the module's: `gui`.
 * @param[in] _exe       the program's file, or NULL.
 * @param[in] _searched  the directories, a line each.
 */
static void
d_cli_internal_not_found(
    const struct d_cli_program* _program,
    const char*                 _command,
    const char*                 _exe,
    const struct d_arg_buffer*  _searched
)
{
    struct d_arg_buffer tagged = D_ARG_BUFFER_INIT;
    struct d_arg_buffer plain  = D_ARG_BUFFER_INIT;
    const int           named  = d_cli_internal_module_names(_program->name,
                                                             _command,
                                                             _exe,
                                                             &tagged,
                                                             &plain);

    // no memory to name the files with; the files named, tagged or not
    if ( (named != D_ARG_OK) ||
         (plain.data == NULL) )
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "out of memory");
    }
    else if (tagged.length > 0u)
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "'%s' is not installed: neither %s nor %s was found",
                     _command,
                     tagged.data,
                     plain.data);
        d_cli_internal_looked_in(_program,
                                 _searched);
    }
    else
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "'%s' is not installed: %s was not found",
                     _command,
                     plain.data);
        d_cli_internal_looked_in(_program,
                                 _searched);
    }

    d_arg_buffer_release(&tagged);
    d_arg_buffer_release(&plain);

    return;
}

/**
 * @brief Runs a loaded module with the arguments its command left.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command.
 * @param[in] _module   the module.
 * @param[in] _exe      the program's file, or NULL.
 * @return the module's status, or D_CLI_EXIT_TROUBLE when its arguments
 *         could not be gathered.
 */
static int
d_cli_internal_run_module(
    struct d_cli_program*      _program,
    struct d_arg_command*      _command,
    const struct d_cli_module* _module,
    const char*                _exe
)
{
    struct d_arg_buffer             path      = D_ARG_BUFFER_INIT;
    struct d_cli_internal_arguments arguments;
    struct d_arg_extra              first;

    memset(&arguments,
           0,
           sizeof(arguments));

    int status = d_cli_command_path(_command,
                                    &path);

    first.text       = d_arg_buffer_text(&path);
    first.classifier = 0;
    status           = (status == D_ARG_OK)
                       ? d_cli_internal_keep_argument(&first,
                                                      &arguments)
                       : status;
    status           = (status == D_ARG_OK)
                       ? d_arg_command_each_remaining(
                             _command,
                             false,
                             d_cli_internal_keep_argument,
                             &arguments)
                       : status;

    int result = D_CLI_EXIT_TROUBLE;

    // the module's own run, as the program presents itself
    if (status == D_ARG_OK)
    {
        struct d_cli_plugin_host host;

        memset(&host,
               0,
               sizeof(host));
        host.abi        = D_CLI_PLUGIN_ABI;
        host.size       = (uint32_t)sizeof(host);
        host.program    = _program->name;
        host.version    = _program->version;
        host.command    = path.data;
        host.executable = _exe;
        host.module     = _module->path;
        host.verbosity  = _program->verbosity;
        host.colour     = _program->colour;

        result = _module->plugin->run(&host,
                                      (int)arguments.count,
                                      (const char* const*)arguments.items);
    }
    else
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "out of memory");
    }

    d_cli_internal_drop_arguments(&arguments);
    d_arg_buffer_release(&path);

    return result;
}

/**
 * @brief Finds a plugin command's module, saying why when it is not found:
 *        where it was looked for, or the want of memory to look with.
 *
 * @param[in]     _program  the program.
 * @param[in]     _name     the command's name.
 * @param[in]     _exe      the program's file, or NULL.
 * @param[in,out] _found    receives the module's path.
 * @return whether it was found.
 */
static bool
d_cli_internal_find_module(
    const struct d_cli_program* _program,
    const char*                 _name,
    const char*                 _exe,
    struct d_arg_buffer*        _found
)
{
    struct d_arg_buffer searched = D_ARG_BUFFER_INIT;
    const int           status   = d_cli_plugin_find(_program->name,
                                                     _name,
                                                     _exe,
                                                     _found,
                                                     &searched);

    // no memory to look with, or to say where it looked
    if ( (status == (int)D_ARG_ERROR_OUT_OF_MEMORY) ||
         ( (status != (int)D_ARG_OK) &&
           (searched.failed) ) )
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "out of memory");
    }
    else if (status != (int)D_ARG_OK)
    {
        d_cli_internal_not_found(_program,
                                 _name,
                                 _exe,
                                 &searched);
    }

    d_arg_buffer_release(&searched);

    return (status == (int)D_ARG_OK);
}

/**
 * @brief Loads a plugin command's module, saying why when it cannot: the
 *        module's refusal, or the want of memory to load it with.
 *
 * @param[in]  _program  the program.
 * @param[in]  _path     the module's file.
 * @param[out] _module   receives the module.
 * @return whether it was loaded.
 */
static bool
d_cli_internal_load_module(
    const struct d_cli_program* _program,
    const char*                 _path,
    struct d_cli_module*        _module
)
{
    struct d_arg_buffer why    = D_ARG_BUFFER_INIT;
    const int           status = d_cli_module_load(_module,
                                                   _path,
                                                   _program->name,
                                                   _program->version,
                                                   &why);

    // no memory to load it with, or to say why not
    if ( (status == (int)D_ARG_ERROR_OUT_OF_MEMORY) ||
         ( (status != (int)D_ARG_OK) &&
           (why.failed) ) )
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "out of memory");
    }
    // there, but not this program's
    else if (status != (int)D_ARG_OK)
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "cannot load '%s': %s",
                     _path,
                     (why.data != NULL) ? why.data : "it was refused");
    }

    d_arg_buffer_release(&why);

    return (status == (int)D_ARG_OK);
}

/**
 * @brief A plugin command's work: find, load and run its module. A
 *        d_cli_command_fn.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command.
 * @param[in] _context  unused.
 * @return the module's status, or D_CLI_EXIT_TROUBLE.
 */
static int
d_cli_internal_plugin_command(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    char*               exe    = NULL;
    struct d_arg_buffer found  = D_ARG_BUFFER_INIT;
    struct d_cli_module module = D_CLI_MODULE_INIT;
    int                 result = D_CLI_EXIT_TROUBLE;

    (void)_context;

    // the program's own file: one the system cannot name is searched
    // without; one there was no memory to name is trouble
    if (d_cli_executable_path(_program->parser->program,
                              &exe) == ENOMEM)
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "out of memory");

        return D_CLI_EXIT_TROUBLE;
    }

    // found, this program's, and run
    if ( (d_cli_internal_find_module(_program,
                                     d_arg_command_name(_command),
                                     exe,
                                     &found)) &&
         (d_cli_internal_load_module(_program,
                                     found.data,
                                     &module)) )
    {
        result = d_cli_internal_run_module(_program,
                                           _command,
                                           &module,
                                           exe);

        // the module stays loaded until the process ends, whatever it
        // started outliving its run; only the copy of its path goes
        D_INTERNAL_CLI_FREE(module.path);
    }

    d_arg_buffer_release(&found);
    d_cli_free(exe);

    return result;
}

#else

/**
 * @brief A plugin command's work where the kit has no loader: says so.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_TROUBLE.
 */
static int
d_cli_internal_plugin_command(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_context;

    d_cli_report(_program,
                 (int)D_CLI_SEVERITY_ERROR,
                 "'%s' is a plugin, and this program was built without "
                 "plugins",
                 d_arg_command_name(_command));

    return D_CLI_EXIT_TROUBLE;
}

#endif  // D_INTERNAL_CLI_PLUGINS

/*
d_cli_program_add_plugin
  A command whose arguments are all the module's: no options of its own --
its help flag removed -- none falling through from the root, and parsing
stopping at the first argument, which is kept with the rest.
*/
struct d_arg_command*
d_cli_program_add_plugin(
    struct d_cli_program* _program,
    struct d_arg_command* _parent,
    const char*           _name,
    const char*           _description
)
{
    struct d_arg_command* const command =
        d_cli_program_add_command(_program,
                                  _parent,
                                  _name,
                                  _description,
                                  d_cli_internal_plugin_command,
                                  NULL,
                                  NULL);

    // the parser has recorded why
    if (!command)
    {
        return NULL;
    }

    d_arg_command_set_fallthrough(command,
                                  false);
    (void)d_arg_command_set_help_flag(command,
                                      NULL,
                                      NULL);
    d_arg_command_set_prefix_command(command,
                                     (int)D_ARG_PREFIX_ON);

    return command;
}

/*
d_cli_program_init_plugin
  d_cli_program_init under the host's name and version, then the host's
verbosity as the base -v counts from, and its colour mode as the mode
--color changes.
*/
int
d_cli_program_init_plugin(
    struct d_cli_program*           _program,
    struct d_arg_parser*            _parser,
    const struct d_cli_plugin_host* _host,
    uint32_t                        _flags
)
{
    // parameter validation
    if ( (!_host)                              ||
         (_host->abi != D_CLI_PLUGIN_ABI)      ||
         (_host->size < (uint32_t)sizeof(*_host)) ||
         (!_host->program) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const int status = d_cli_program_init(_program,
                                          _parser,
                                          _host->program,
                                          _host->version,
                                          _flags);

    // the host's say on how much is said, and in what colour
    if (status == D_ARG_OK)
    {
        _program->verbosity_base = _host->verbosity;
        _program->verbosity      = _host->verbosity;
        _program->colour         = _host->colour;
        _program->colour_output  = d_cli_colour_wanted(
                                       (int)D_CLI_STREAM_OUTPUT,
                                       _program->colour);
        _program->colour_error   = d_cli_colour_wanted(
                                       (int)D_CLI_STREAM_ERROR,
                                       _program->colour);
    }

    return status;
}
