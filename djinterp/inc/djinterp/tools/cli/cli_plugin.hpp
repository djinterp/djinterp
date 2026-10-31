/*******************************************************************************
* djinterp [tools]                                                cli_plugin.hpp
*
* The command-line kit's C++ face: modules.
*   A program's side of a plugin is program::add_plugin (cli_program.hpp),
* and a module's side is a program made from its host, program(host); what
* is here is the rest of cli_plugin.h for C++: the program's own file, the
* search for a module, and DPluginModule, which holds one loaded. A module
* written in C++ defines its entry point with D_CLI_PLUGIN, as a C module
* does: the macro is C and C++ alike, and the module's run function must
* not let an exception out into the C frame that calls it -- running a
* cli::program, whose work is guarded, is the usual way to be sure.
*
*
* path:      /inc/djinterp/tools/cli/cli_plugin.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FILES
    -----
    1.  The program's file
         1.  executable_path
    2.  Finding a module
         1.  find_plugin
2.  MODULES
    -------
    1.  The class
         1.  DPluginModule
3.  DEFINITIONS
    -----------
    1.  Files
    2.  DPluginModule
*/

#ifndef DJINTERP_TOOLS_CLI_CLI_PLUGIN_HPP
#define DJINTERP_TOOLS_CLI_CLI_PLUGIN_HPP 1

// std
#include <string>  // std::string
// djinterp
#include "../../cpp/djinterp.hpp"                         // framework root
#include "../../cpp/modules/parse/cli/arg_parser/arg_common.hpp"  // DCString, status,
                                                      // scratch
#include "./c/cli_io.h"                               // d_cli_free
#include "./c/cli_plugin.h"                           // the C calls,
                                                      // dj_cli_module


namespace djinterp
{
namespace cli
{

// the argument parser's names, which a program makes its definitions
// with, carried by the kit's namespace, so that `kit::command` and
// `kit::program` are spelled alike
using namespace ::djinterp::parse::cli;

//==============================================================================
// 1.  FILES
//==============================================================================


// 1.1    The program's file
//------------------------------------------------------------------------------
// 1.1.1
// executable_path
/**
 * @brief Finds the file the running program was started from;
 *        d_cli_executable_path.
 *
 * @param[in]  _argv0  the program's `argv[0]`, for the fallback.
 * @param[out] _path   receives the absolute path in UTF-8 when found; left
 *                     as it was otherwise.
 * @return 0 when found, or the C call's errno value; std::bad_alloc, as
 *         std::string throws it, when the string cannot hold the path.
 */
int executable_path(
    DCString     _argv0,
    std::string& _path
);

// 1.2    Finding a module
//------------------------------------------------------------------------------
// 1.2.1
// find_plugin
/**
 * @brief Finds the module of a program's command; d_cli_plugin_find.
 *
 * @param[in]  _program     the program's name.
 * @param[in]  _name        the command's.
 * @param[in]  _executable  the program's own file; empty leaves only
 *                          PROGRAM_PLUGIN_PATH to search.
 * @param[out] _found       receives the module's path when found.
 * @param[out] _searched    receives the directories searched, a line each.
 * @return the C call's status: ok when found, D_ARG_ERROR_FILE when no
 *         directory held it; std::bad_alloc, as std::string throws it,
 *         when a string cannot be made.
 */
status find_plugin(
    DCString     _program,
    DCString     _name,
    DCString     _executable,
    std::string& _found,
    std::string& _searched
);


//==============================================================================
// 2.  MODULES
//==============================================================================


// 2.1    The class
//------------------------------------------------------------------------------
// 2.1.1
// DPluginModule
//   class: a dj_cli_module: a module loaded, or none. Its destructor unloads
// it, as unload() does -- for a loader's tests and tools; a program that
// runs a window keeps the module for the process's life, as add_plugin's
// command does. Neither copied nor moved.
class DPluginModule
{
public:
    DPluginModule() D_NOEXCEPT;
    ~DPluginModule();

    /**
     * @brief Loads a module, and checks it belongs to a program; the one
     *        held before is unloaded first; d_cli_module_load.
     *
     * @param[in]  _path     the module's file, in UTF-8.
     * @param[in]  _program  the program it must belong to.
     * @param[in]  _version  the version it must belong to; NULL accepts any.
     * @param[out] _why      receives why, when it was refused.
     * @return the C call's status: ok when loaded; std::bad_alloc, as
     *         std::string throws it, when `_why` cannot be made.
     */
    status load(
        DCString     _path,
        DCString     _program,
        DCString     _version,
        std::string& _why
    );

    /**
     * @brief Unloads the module held, if any; d_cli_module_unload.
     *
     * @warning What the module started that outlives its run then runs code
     *          that is gone; see d_cli_module_unload.
     */
    void unload() D_NOEXCEPT;

    // what is held: whether a module is, its record -- NULL for none -- and
    // the file it came from, "" for none
    bool loaded() const D_NOEXCEPT;
    const ::dj_cli_plugin* plugin() const D_NOEXCEPT;
    const char* path() const D_NOEXCEPT;

    // the C record
    const ::dj_cli_module& get() const D_NOEXCEPT;

private:
    D_DELETED_FN(DPluginModule(const DPluginModule& _other))
    D_DELETED_FN(DPluginModule& operator=(const DPluginModule& _other))

    ::dj_cli_module m_module;
};


//==============================================================================
// 3.  DEFINITIONS
//==============================================================================


// 3.1    Files
//------------------------------------------------------------------------------
// executable_path / find_plugin
//   The C calls; what they allocate is freed before a string is touched,
// so that a std::bad_alloc leaks nothing.
inline
int executable_path(
    DCString     _argv0,
    std::string& _path
)
{
    char*     found  = D_NULLPTR;
    const int result = d_cli_executable_path(_argv0.get(),
                                             &found);

    // not found
    if (result != 0)
    {
        return result;
    }

    // the path, owned by a string before the C copy is freed
    struct owner
    {
        char* m_text;

        ~owner()
        {
            d_cli_free(m_text);
        }
    } const held = { found };

    std::string(held.m_text).swap(_path);

    return 0;
}

inline
status find_plugin(
    DCString     _program,
    DCString     _name,
    DCString     _executable,
    std::string& _found,
    std::string& _searched
)
{
    ::djinterp::parse::cli::internal::scratch found;
    ::djinterp::parse::cli::internal::scratch searched;
    const char* const executable = ( (_executable.get() != D_NULLPTR) &&
                                     (_executable.get()[0] != '\0') )
                                   ? _executable.get()
                                   : D_NULLPTR;
    const status      result(d_cli_plugin_find(_program.get(),
                                               _name.get(),
                                               executable,
                                               found.get(),
                                               searched.get()));

    _found    = found.str();
    _searched = searched.str();

    return result;
}

// 3.2    DPluginModule
//------------------------------------------------------------------------------
// DPluginModule::DPluginModule / ~DPluginModule / load / unload / loaded /
// plugin / path / get
//   The C record and calls; load() keeps the reason in a scratch buffer
// until it is copied out.
inline
DPluginModule::DPluginModule() D_NOEXCEPT
{
    const ::dj_cli_module none = D_CLI_MODULE_INIT;

    m_module = none;
}

inline
DPluginModule::~DPluginModule()
{
    d_cli_module_unload(&m_module);
}

inline
status DPluginModule::load(
    DCString     _path,
    DCString     _program,
    DCString     _version,
    std::string& _why
)
{
    ::djinterp::parse::cli::internal::scratch why;

    d_cli_module_unload(&m_module);

    const status result(d_cli_module_load(&m_module,
                                          _path.get(),
                                          _program.get(),
                                          _version.get(),
                                          why.get()));

    _why = why.str();

    return result;
}

inline
void DPluginModule::unload() D_NOEXCEPT
{
    d_cli_module_unload(&m_module);

    return;
}

inline
bool DPluginModule::loaded() const D_NOEXCEPT
{
    return (m_module.plugin != D_NULLPTR);
}

inline
const ::dj_cli_plugin* DPluginModule::plugin() const D_NOEXCEPT
{
    return m_module.plugin;
}

inline
const char* DPluginModule::path() const D_NOEXCEPT
{
    return (m_module.path != D_NULLPTR) ? m_module.path : "";
}

inline
const ::dj_cli_module& DPluginModule::get() const D_NOEXCEPT
{
    return m_module;
}

}  // namespace cli
}  // namespace djinterp


#endif  // DJINTERP_TOOLS_CLI_CLI_PLUGIN_HPP
