/******************************************************************************
* djinterp [core]                                                    dfile.hpp
*
* C++ file I/O for the djinterp framework.
*   This is the umbrella header for the C++ file layer: it pulls in the shared
* foundation and every use-case module, so a translation unit can include
* "dfile.hpp" and get the complete C++ API. To depend on only part of the
* API, include the specific module(s) instead (see the mapping below).
*   The C++ layer is a thin, zero-overhead veneer over the C API: RAII handle
* classes (stream, descriptor, pipe, directory) and namespaced free functions
* whose inline bodies forward to the extern "C" implementation declared by the
* matching C headers. Using it costs nothing over the C calls while adding
* ownership safety, namespaces, and type-safe flags.
*
* modules:
*   dfile_common.hpp     - namespace, aliases, and strongly-typed flags
*   dfile_stream.hpp     - class stream + whole-file / temp helpers
*   dfile_descriptor.hpp - class descriptor, class pipe
*   dfile_metadata.hpp   - class status + existence / size / access queries
*   dfile_fs.hpp         - class directory + directory / file / symlink ops
*   dfile_path.hpp       - djinterp::file::path free functions
*
* 
* path:      /inc/djinterp/cpp/io/file/dfile.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/

#ifndef DJINTERP_FILE_
#define DJINTERP_FILE_ 1

#include "./dfile_common.hpp"
#include "./dfile_stream.hpp"
#include "./dfile_descriptor.hpp"
#include "./dfile_metadata.hpp"
#include "./dfile_fs.hpp"
#include "./dfile_path.hpp"


#endif  // DJINTERP_FILE_
