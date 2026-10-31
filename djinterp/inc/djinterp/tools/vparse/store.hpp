/*******************************************************************************
* vparse [vparse]                                                      store.hpp
*
* Saving runs to files, and loading them back.
*   Each run is saved to its own folder inside a session folder:
*
*     <root>/session-YYYYMMDD-HHMMSS/run-007/
*         run.txt         id, time, front end, trace and step-limit settings,
*                         and the result sentence
*         grammar.txt     the grammar, byte for byte
*         input.txt       the input, byte for byte
*         <part>.txt      each other part of the run as an aligned listing
*     <root>/session-YYYYMMDD-HHMMSS/index.tsv   one line per saved run
*
*   The part files are for reading and for diffing between runs outside the
* workbench. Loading reads only run.txt, grammar.txt and input.txt and runs
* them again: a run is deterministic, so every other part comes back as it was.
*   Both programs also read the files a command line names -- a grammar, an
* input -- and name them in UTF-8 whatever the system, as the command line
* and a Dear ImGui text field both give them; section 2.4 turns such a name
* into a path, and reads the file whole.
*
* path:      /inc/djinterp/tools/vparse/store.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SAVED RUNS
    ----------
    1.  saved_run
2.  OPERATIONS
    ----------
    1.  Saving
         1.  session_name
         2.  save_run
    2.  Loading
         1.  list_saved
         2.  load_request
    3.  Formatting
         1.  listing_text
    4.  Files given by name
         1.  path_from_utf8 / utf8_from_path
         2.  read_whole_file
*/

#ifndef DJINTERP_TOOLS_VPARSE_STORE_HPP
#define DJINTERP_TOOLS_VPARSE_STORE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as the
// rest of the workbench is, and std::filesystem is C++17's.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <filesystem>  // std::filesystem::path
#include <string>      // std::string
#include <vector>      // std::vector
// vparse
#include "./run.hpp"  // run_record, run_request, listing


namespace vparse
{
namespace workbench
{


//==============================================================================
// 1.  SAVED RUNS
//==============================================================================


// 1.1    saved_run
//------------------------------------------------------------------------------
// 1.1.1
// saved_run
//   struct: a run folder found on disk, described from its run.txt.
struct saved_run
{
    std::filesystem::path folder;
    std::string           session;
    int                   id = 0;
    std::string           frontend;
    std::string           when;
    std::string           result;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Saving
//------------------------------------------------------------------------------
// 2.1.1
// session_name
//   function: "session-YYYYMMDD-HHMMSS" for the current local time.
std::string session_name();

// 2.1.2
// save_run
//   function: writes _run's folder into _session, creating folders as needed,
// and adds or replaces its line in the session's index.tsv.
//   @return the run folder, or an empty path with *_error set on failure.
std::filesystem::path save_run(
    const run_record&            _run,
    const std::filesystem::path& _session,
    std::string*                 _error
);


// 2.2    Loading
//------------------------------------------------------------------------------
// 2.2.1
// list_saved
//   function: every saved run under _root (sessions and runs), newest first.
// A folder is a saved run if it holds a run.txt.
std::vector<saved_run> list_saved(
    const std::filesystem::path& _root
);

// 2.2.2
// load_request
//   function: the request a saved run was made from, ready to run again.
//   @return false with *_error set if the folder is not a readable run.
bool load_request(
    const std::filesystem::path& _folder,
    run_request&                 _request,
    std::string*                 _error
);


// 2.3    Formatting
//------------------------------------------------------------------------------
// 2.3.1
// listing_text
//   function: a listing as a text table -- a header, a rule, one line per row,
// columns aligned, no trailing spaces. An empty listing is its note.
std::string listing_text(
    const listing& _listing
);


// 2.4    Files given by name
//------------------------------------------------------------------------------
// 2.4.1
// path_from_utf8 / utf8_from_path
//   functions: the path a UTF-8 name names, and a path's name in UTF-8. A
// path made from a std::string reads it in the system's narrow encoding,
// which on Windows is the code page and not UTF-8; these read and write
// UTF-8 everywhere. Neither fails.
std::filesystem::path path_from_utf8(
    const std::string& _name
);
std::string utf8_from_path(
    const std::filesystem::path& _path
);

// 2.4.2
// read_whole_file
/**
 * @brief Reads a file whole, byte for byte, line endings included.
 *
 * @param[in]  _path   the file.
 * @param[out] _bytes  receives the file's contents; unchanged on failure.
 * @param[out] _error  receives why the file could not be read -- the
 *                     system's words, such as "No such file or directory"
 *                     -- on failure; may be NULL.
 * @return whether the file was read: `false` for a path that names nothing,
 *         a directory, or a file that cannot be opened or read.
 */
bool read_whole_file(
    const std::filesystem::path& _path,
    std::string&                 _bytes,
    std::string*                 _error
);

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now

#endif  // DJINTERP_TOOLS_VPARSE_STORE_HPP
