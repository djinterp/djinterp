/*******************************************************************************
* djinterp [core]                                              file_tree_ios.hpp
*
* iOS file tree scanner:
*   iOS runs the Darwin kernel, so the directory-enumeration machinery
* is identical to macOS - this header reuses apple_scanner underneath.
* What differs is the *reachable* filesystem: an iOS app is confined to
* its container (Documents, Library, tmp, plus security-scoped bookmarks
* it has been granted).  Walking outside the sandbox fails with EPERM
* rather than returning entries.
*
*   ios_scanner therefore adds one sandbox-aware guard: it treats an
* opendir/open failure on the *root* path as a definitive "not
* permitted" result (producing a root-only tree) rather than a transient
* error, and otherwise delegates entirely to apple_scanner.  This keeps
* the iOS backend a thin, documented specialization instead of a copy.
*
*   getattrlistbulk is available on iOS 8+, so the batch path in
* apple_scanner applies here too.
*
*
* path:      /inc/djinterp/core/container/tree/file/file_tree_ios.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_IOS_HPP
#define DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_IOS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "./file_tree_common.hpp"

#if D_FILESYS_ENABLE_IOS

#include "./file_tree_apple.hpp"

// std
#include <string>


NS_DJINTERP


// ================================================================
//  ios_scanner
// ================================================================

// ios_scanner
//   policy: Darwin enumeration confined to the app sandbox. Shares
// apple_scanner's batch backend; the distinction is documentary and behavioral
// at the sandbox boundary, not in the per-entry path.
struct ios_scanner
{
    // scan
    //   delegates to apple_scanner. An inaccessible directory simply yields no
    // children (apple_scanner / bsd_scanner already return on open failure),
    // which on iOS is the correct response to a sandbox denial.
    template<typename Ctx>
    static void
    scan(
        Ctx&              _ctx,
        const std::string& _dir_path,
        file_node_id            _parent
    )
    {
        apple_scanner::scan(_ctx, _dir_path, _parent);

        return;
    }
};


NS_END  // djinterp

#else  // !D_FILESYS_ENABLE_IOS

// iOS backend not enabled for this build. Without it this header declares
// nothing, rather than stopping the build: a disabled backend is absent, and
// naming its scanner fails at the point of use (see os_scanner in
// file_tree.hpp). Set D_CFG_FILESYS_ALLOW_IOS (or
// D_CFG_FILESYS_ALLOW_APPLE_FAMILY, or D_CFG_FILESYS_ALLOW_POSIX_FAMILY, or
// D_CFG_FILESYS_ALLOW_FOREIGN) to 1 before including file_tree.hpp.

#endif  // D_FILESYS_ENABLE_IOS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_IOS_HPP
