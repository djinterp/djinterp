/*******************************************************************************
* djinterp [cpp][container]                          file_tree_windows_probe.cpp
*
*   Not a test section: the unit the file tree leaf compiles with a MinGW-w64
* cross-compiler, where one is installed, so that the Windows scanner's and
* the Windows populator's native code (behind _WIN32) is compiled on a Linux
* runner. It names the native backends and calls each, so every template in
* that code is instantiated. Compile only: the leaf's check passes when it
* compiles with no warning, at C++17 and C++20.
*
*
* path:      /tests/djinterp/cpp/container/tree/file/file_tree_windows_probe.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include <djinterp/cpp/container/tree/file/file_tree.hpp>        // the scanners
#include <djinterp/cpp/container/tree/file/file_attributes.hpp>  // populators

#if !defined(_WIN32)
    #error "file_tree_windows_probe.cpp is compiled for Windows only"
#endif


int main()
{
    djinterp::DFileTree<djinterp::DOperatingSystem::windows>   plain;
    djinterp::DFileTree<djinterp::DOperatingSystem::windows10> large;
    djinterp::DFileTreeDefault                                 native;
    djinterp::DFileMetadata                                    meta;

    plain.scan("C:\\");
    large.scan("C:\\");
    native.scan("C:\\");

    return ( djinterp::DFileAttributesDefault::populate("C:\\", meta) &&
             (plain.size() <= large.size()) ) ? 0 : 1;
}
