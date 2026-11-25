/*******************************************************************************
* djinterp [cpp][container]                           file_tree_tests_traits.cpp
*
*   file_tree_traits.hpp and file_tree_concepts.hpp: the method probes, the
* payload traits and the concepts, on the file tree's core and on types that
* are not a file tree.
*
*
* path:      /tests/djinterp/cpp/container/tree/file/file_tree_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "file_tree_tests.hpp"  // D_FT_CHECK, the declarations
#include <djinterp/cpp/container/tree/file/file_tree_traits.hpp>  // under test
#include <djinterp/cpp/container/tree/file/file_tree_concepts.hpp>  // under test

// std
#include <type_traits>  // std::is_same
#include <vector>       // std::vector


NS_DJINTERP
NS_TESTING

namespace
{
    using DFrCore = DFileTreeCore<DNullScanner>;

    // DFrScanOnly
    //   fixture: a scan method and nothing else of a file tree.
    struct DFrScanOnly
    {
        DFileNodeId scan(const char*) { return null_file_node; }
    };
}

/*
tests_file_tree_traits_detection
  the probes and the payload traits
  Tests the following:
  - the core has scan, resolve, name_str, full_path, both visits and clear
  - a type with only scan has scan and nothing else
  - std::vector and int have none of them
  - DFileEntry is a file-entry payload, int is not; the core's payload type
*/
bool tests_file_tree_traits_detection()
{
    D_FT_CHECK(has_scan_method_v<DFrCore>);
    D_FT_CHECK(has_resolve_method_v<DFrCore>);
    D_FT_CHECK(has_name_str_method_v<DFrCore>);
    D_FT_CHECK(has_full_path_method_v<DFrCore>);
    D_FT_CHECK(has_depth_first_method_v<DFrCore>);
    D_FT_CHECK(has_breadth_first_method_v<DFrCore>);
    D_FT_CHECK(has_clear_method_v<DFrCore>);

    D_FT_CHECK(has_scan_method_v<DFrScanOnly>);
    D_FT_CHECK(!has_resolve_method_v<DFrScanOnly>);
    D_FT_CHECK(!has_full_path_method_v<DFrScanOnly>);

    D_FT_CHECK(!has_scan_method_v<std::vector<int>>);
    D_FT_CHECK(!has_resolve_method_v<int>);
    D_FT_CHECK(has_clear_method_v<std::vector<int>>);

    D_FT_CHECK(is_file_entry_payload_v<DFileEntry>);
    D_FT_CHECK(!is_file_entry_payload_v<int>);
    D_FT_CHECK(std::is_same<file_payload_type_of_t<DFrCore>,
                            DFileEntry>::value);

    return true;
}

/*
tests_file_tree_traits_concepts
  the concepts agree with the traits (C++20)
  Tests the following:
  - the core is a file tree: named, path-addressable, traversable both
    ways, mutable, clearable, carrying file entries
  - a scan-only type is scannable and not a file tree
  - int is none of them
*/
bool tests_file_tree_traits_concepts()
{
#if defined(__cpp_concepts)
    D_FT_CHECK(file_tree_type<DFrCore>);
    D_FT_CHECK(named_file_tree<DFrCore>);
    D_FT_CHECK(path_addressable_file_tree<DFrCore>);
    D_FT_CHECK(dfs_traversable_file_tree<DFrCore>);
    D_FT_CHECK(bfs_traversable_file_tree<DFrCore>);
    D_FT_CHECK(mutable_file_tree<DFrCore>);
    D_FT_CHECK(clearable_file_tree<DFrCore>);
    D_FT_CHECK(file_entry_carrying_tree<DFrCore>);
    D_FT_CHECK(file_entry_payload<DFileEntry>);

    D_FT_CHECK(scannable_file_tree<DFrScanOnly>);
    D_FT_CHECK(!file_tree_type<DFrScanOnly>);
    D_FT_CHECK(!file_tree_type<int>);
    D_FT_CHECK(!file_entry_payload<int>);
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
