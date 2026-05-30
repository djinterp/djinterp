/******************************************************************************
* djinterp [fs]                                               file_tree_ios.hpp
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
* path:      /inc/cpp/fs/file_tree_ios.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_IOS_
#define DJINTERP_FS_FILE_TREE_IOS_ 1

#include "./file_tree_common.hpp"
#include "./file_tree_apple.hpp"

#include <string>


NS_DJINTERP
NS_FS


// ================================================================
//  ios_scanner
// ================================================================

// ios_scanner
//   policy: Darwin enumeration confined to the app sandbox.  Shares
// apple_scanner's batch backend; the distinction is documentary and
// behavioral at the sandbox boundary, not in the per-entry path.
struct ios_scanner
{
    // scan
    //   delegates to apple_scanner.  An inaccessible directory simply
    // yields no children (apple_scanner / bsd_scanner already return
    // on open failure), which on iOS is the correct response to a
    // sandbox denial.
    template<typename _Ctx>
    static void
    scan(
        _Ctx&              _ctx,
        const std::string& _dir_path,
        node_id            _parent
    )
    {
        apple_scanner::scan(_ctx, _dir_path, _parent);

        return;
    }
};


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_IOS_
