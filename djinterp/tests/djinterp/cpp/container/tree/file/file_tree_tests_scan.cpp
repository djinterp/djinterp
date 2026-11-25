/*******************************************************************************
* djinterp [cpp][container]                             file_tree_tests_scan.cpp
*
*   file_tree.hpp and the POSIX scanners, run against the suite's directory.
* On a POSIX host this unit allows the POSIX family (posix, bsd, linux, and
* apple and ios above them) before including the umbrella, so each scanner a
* Linux build can run is run; elsewhere it includes the umbrella as it is.
*
*
* path:      /tests/djinterp/cpp/container/tree/file/file_tree_tests_scan.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include <djinterp/env/env.h>  // D_ENV_OS_ID, before the knobs it decides
#if !D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #define D_CFG_FILESYS_ALLOW_POSIX_FAMILY 1
#endif
#include "file_tree_tests.hpp"  // D_FT_CHECK, DFtScratch, the declarations
#include <djinterp/cpp/container/tree/file/file_tree.hpp>  // under test

// std
#include <set>  // std::set


NS_DJINTERP
NS_TESTING

namespace
{
    // addresses
    //   every node's address in a scanned tree, with its type and size.
    template<typename Tree>
    std::set<std::string> addresses(
        const Tree& _tree
    )
    {
        std::set<std::string> out;

        _tree.visit_depth_first(
            _tree.root(),
            [&](DFileNodeId _id, std::size_t)
            {
                out.insert(_tree.address(_id) + "|" +
                           std::to_string(_tree[_id].data.type) + "|" +
                           std::to_string(_tree[_id].data.size));
            });

        return out;
    }

    // scans_scratch
    //   true when a scan of the scratch directory holds every entry, with
    // its type and size, each resolving and naming its file.
    template<typename Tree>
    bool scans_scratch(
        const DFtScratch& _s
    )
    {
        Tree        tree;
        DFileNodeId root = tree.scan(_s.path());

        D_FT_CHECK(root != null_file_node);
        D_FT_CHECK(tree.size() == _s.nodes());

        DFileNodeId a = tree.resolve("a.txt");
        DFileNodeId b = tree.resolve("sub/b.bin");
        DFileNodeId c = tree.resolve("sub/deeper/c");
        DFileNodeId d = tree.resolve("sub/deeper");

        D_FT_CHECK(a != null_file_node);
        D_FT_CHECK(tree[a].data.type == file_type_regular);
        D_FT_CHECK(tree[a].data.size == 5);
        D_FT_CHECK(b != null_file_node);
        D_FT_CHECK(tree[b].data.size == 3);
        D_FT_CHECK(c != null_file_node);
        D_FT_CHECK(tree[c].data.size == 0);
        D_FT_CHECK(d != null_file_node);
        D_FT_CHECK(tree[d].data.type == file_type_directory);
        D_FT_CHECK(tree[d].data.size == 0);
        D_FT_CHECK(tree.full_path(b) == (_s.path() + "/sub/b.bin"));
        D_FT_CHECK(tree.height() == 3);

        if (_s.has_link)
        {
            DFileNodeId l = tree.resolve("link");

            D_FT_CHECK(l != null_file_node);
            D_FT_CHECK(tree[l].data.type == file_type_symlink);
        }

        return true;
    }
}

/*
tests_file_tree_scan_native
  the detected backend, on the suite's directory
  Tests the following:
  - every entry, with its type and size; the link as a link, not followed
  - a directory's size is 0: a size is a regular file's
  - each resolves by address, and full_path names its file
*/
bool tests_file_tree_scan_native()
{
    DFtScratch s("native");

    D_FT_CHECK(s.ok);

    return scans_scratch<DFileTreeDefault>(s);
}

/*
tests_file_tree_scan_posix_family
  each POSIX scanner, on a POSIX host
  Tests the following:
  - posix, bsd and linux_generic each scan the whole directory
  - each gives the native scan's tree: the same addresses, types and sizes
    (the posix scanner gave a directory its st_size, until 2026.11.23)
  - elsewhere there is nothing to run here, and the test passes
*/
bool tests_file_tree_scan_posix_family()
{
#if D_FILESYS_ENABLE_POSIX && D_FILESYS_ENABLE_BSD && D_FILESYS_ENABLE_LINUX
    DFtScratch s("posix");

    D_FT_CHECK(s.ok);
    D_FT_CHECK(scans_scratch<DFileTree<DOperatingSystem::posix>>(s));
    D_FT_CHECK(scans_scratch<DFileTree<DOperatingSystem::bsd>>(s));
    D_FT_CHECK(scans_scratch<DFileTree<DOperatingSystem::linux_generic>>(s));

    DFileTreeDefault                      native;
    DFileTree<DOperatingSystem::posix>    posix;
    DFileTree<DOperatingSystem::bsd>      bsd;

    native.scan(s.path());
    posix.scan(s.path());
    bsd.scan(s.path());

    D_FT_CHECK(addresses(posix) == addresses(native));
    D_FT_CHECK(addresses(bsd) == addresses(native));
#endif

    return true;
}

/*
tests_file_tree_scan_null_and_missing
  the none backend, and a path that is not there
  Tests the following:
  - DFileTree<none> keeps the root and touches nothing else
  - the detected backend, given a missing path, keeps the root alone
*/
bool tests_file_tree_scan_null_and_missing()
{
    DFtScratch                        s("null");
    DFileTree<DOperatingSystem::none> null_tree;
    DFileTreeDefault                  native;

    D_FT_CHECK(s.ok);
    D_FT_CHECK(null_tree.scan(s.path()) != null_file_node);
    D_FT_CHECK(null_tree.size() == 1);

    D_FT_CHECK(native.scan(s.path() + "/not_there") != null_file_node);
    D_FT_CHECK(native.size() == 1);
    D_FT_CHECK(native.height() == 0);

    return true;
}

NS_END  // testing
NS_END  // djinterp
