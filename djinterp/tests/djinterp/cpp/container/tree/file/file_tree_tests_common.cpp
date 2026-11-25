/*******************************************************************************
* djinterp [cpp][container]                           file_tree_tests_common.cpp
*
*   file_tree_common.hpp and fs_os.hpp: the core over the null scanner, built
* by hand with add_child, so no file system is touched.
*
*
* path:      /tests/djinterp/cpp/container/tree/file/file_tree_tests_common.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "file_tree_tests.hpp"  // D_FT_CHECK, the declarations
#include <djinterp/cpp/container/tree/file/file_tree_common.hpp>  // under test
#include <djinterp/cpp/container/tree/file/fs_os.hpp>             // under test

// std
#include <cctype>  // std::toupper


NS_DJINTERP
NS_TESTING

namespace
{
    using DFtCore = DFileTreeCore<DNullScanner>;

    // DFtBuilt
    //   fixture: proj/{src/main.cpp (10 bytes), src/util.hpp (4), README (4)}.
    struct DFtBuilt
    {
        DFtCore     tree;
        DFileNodeId root;
        DFileNodeId src;
        DFileNodeId main;
        DFileNodeId util;
        DFileNodeId readme;

        DFtBuilt()
            : tree(),
              root(tree.scan("proj")),
              src(tree.add_child(root, "src", file_type_directory)),
              main(tree.add_child(src, "main.cpp", file_type_regular, 10)),
              util(tree.add_child(src, "util.hpp", file_type_regular, 4)),
              readme(tree.add_child(root, "README", file_type_regular, 4))
        {}
    };
}

/*
tests_file_tree_common_values
  the documented values
  Tests the following:
  - DFileType: unknown 0 to other 4
  - a default DFileEntry is all zero, of unknown type
  - null_file_node is the null pointer
  - DOperatingSystem: automatic 0, windows11 8, none 255
*/
bool tests_file_tree_common_values()
{
    const DFileEntry entry;

    D_FT_CHECK(file_type_unknown == 0);
    D_FT_CHECK(file_type_regular == 1);
    D_FT_CHECK(file_type_directory == 2);
    D_FT_CHECK(file_type_symlink == 3);
    D_FT_CHECK(file_type_other == 4);

    D_FT_CHECK(entry.name_offset == 0);
    D_FT_CHECK(entry.name_length == 0);
    D_FT_CHECK(entry.type == file_type_unknown);
    D_FT_CHECK(entry.size == 0);

    D_FT_CHECK(null_file_node == nullptr);

    D_FT_CHECK(static_cast<int>(DOperatingSystem::automatic) == 0);
    D_FT_CHECK(static_cast<int>(DOperatingSystem::windows11) == 8);
    D_FT_CHECK(static_cast<int>(DOperatingSystem::none) == 255);

    return true;
}

/*
tests_file_tree_common_build_and_address
  a tree built by hand, addressed both ways
  Tests the following:
  - the scan's root, and each add_child, count in size()
  - resolve finds each node by its address, and address(n) resolves to n
  - the root contributes no label; full_path keeps the scanned path
  - level, height, parent, is_ancestor, lca, name_str and the payload
*/
bool tests_file_tree_common_build_and_address()
{
    DFtBuilt b;
    bool     all_round = true;

    D_FT_CHECK(b.root != null_file_node);
    D_FT_CHECK(b.tree.size() == 5);
    D_FT_CHECK(!b.tree.empty());
    D_FT_CHECK(b.tree.root() == b.root);
    D_FT_CHECK(b.tree.root_path() == "proj");

    D_FT_CHECK(b.tree.resolve("src/main.cpp") == b.main);
    D_FT_CHECK(b.tree.resolve("README") == b.readme);
    D_FT_CHECK(b.tree.resolve("src/missing") == null_file_node);
    D_FT_CHECK(b.tree.address(b.main) == "src/main.cpp");
    D_FT_CHECK(b.tree.address(b.root).empty());
    D_FT_CHECK(b.tree.full_path(b.main) == "proj/src/main.cpp");

    b.tree.visit_depth_first(
        b.root,
        [&](DFileNodeId _id, std::size_t)
        {
            all_round = all_round && b.tree.round_trips(_id);
        });
    D_FT_CHECK(all_round);
    D_FT_CHECK(b.tree.is_addressable());

    D_FT_CHECK(b.tree.level(b.main) == 2);
    D_FT_CHECK(b.tree.height() == 2);
    D_FT_CHECK(b.tree.height(b.src) == 1);
    D_FT_CHECK(b.tree.parent(b.main) == b.src);
    D_FT_CHECK(b.tree.parent(b.root) == null_file_node);
    D_FT_CHECK(b.tree.is_ancestor(b.root, b.util));
    D_FT_CHECK(!b.tree.is_ancestor(b.readme, b.util));
    D_FT_CHECK(b.tree.lca(b.main, b.readme) == b.root);
    D_FT_CHECK(b.tree.lca(b.main, b.util) == b.src);
    D_FT_CHECK(b.tree.name_str(b.util) == "util.hpp");
    D_FT_CHECK(b.tree[b.main].data.size == 10);
    D_FT_CHECK(b.tree[b.src].data.type == file_type_directory);

    b.tree.clear();
    D_FT_CHECK(b.tree.empty());
    D_FT_CHECK(b.tree.size() == 0);

    return true;
}

/*
tests_file_tree_common_separation
  a directory cannot hold two entries of one name
  Tests the following:
  - a second add_child of a name returns the first node, and adds none
  - the same name under another parent is a different node
  - add_child under the null node adds nothing
*/
bool tests_file_tree_common_separation()
{
    DFtBuilt          b;
    const std::size_t before = b.tree.size();

    D_FT_CHECK(b.tree.add_child(b.src, "main.cpp", file_type_regular, 99) ==
               b.main);
    D_FT_CHECK(b.tree.size() == before);
    D_FT_CHECK(b.tree[b.main].data.size == 10);

    D_FT_CHECK(b.tree.add_child(b.root, "main.cpp", file_type_regular) !=
               b.main);
    D_FT_CHECK(b.tree.size() == (before + 1));

    D_FT_CHECK(b.tree.add_child(null_file_node, "x", file_type_regular) ==
               null_file_node);
    D_FT_CHECK(b.tree.size() == (before + 1));

    return true;
}

/*
tests_file_tree_common_filter_and_rename
  filtering keeps the matches' ancestors; renaming must keep siblings apart
  Tests the following:
  - filter on a name keeps the match and the directories leading to it,
    and the match still resolves in the result
  - a filter nothing passes gives an empty tree
  - select and count_if agree
  - rename_preserves_separation is true for upper-casing, false for a
    constant name; rename keeps every path, renamed
*/
bool tests_file_tree_common_filter_and_rename()
{
    DFtBuilt b;
    auto     is_cpp = [&](DFileNodeId _id)
    {
        const std::string n = b.tree.name_str(_id);

        return ( (n.size() > 4) && (n.compare(n.size() - 4, 4, ".cpp") == 0) );
    };
    auto upper = [](const std::string& _name)
    {
        std::string out = _name;

        for (char& c : out)
        {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }

        return out;
    };
    auto constant = [](const std::string&)
    {
        return std::string("same");
    };

    DFtCore kept = b.tree.filter(is_cpp);

    D_FT_CHECK(kept.size() == 3);
    D_FT_CHECK(kept.resolve("src/main.cpp") != null_file_node);
    D_FT_CHECK(kept.resolve("README") == null_file_node);

    DFtCore none = b.tree.filter([](DFileNodeId) { return false; });

    D_FT_CHECK(none.empty());

    D_FT_CHECK(b.tree.select(is_cpp).size() == 1);
    D_FT_CHECK(b.tree.count_if(is_cpp) == 1);

    D_FT_CHECK(b.tree.rename_preserves_separation(upper));
    D_FT_CHECK(!b.tree.rename_preserves_separation(constant));

    DFtCore renamed = b.tree.rename(upper);

    D_FT_CHECK(renamed.size() == b.tree.size());
    D_FT_CHECK(renamed.resolve("SRC/MAIN.CPP") != null_file_node);

    return true;
}

/*
tests_file_tree_common_axes
  the core's container axes
  Tests the following:
  - hierarchical by construction
  - ordered, unbounded, unique, dynamic storage and iterable by default
  - no thread safety
*/
bool tests_file_tree_common_axes()
{
    D_FT_CHECK(DFtCore::axis_structure == DContainerStructure::hierarchical);
    D_FT_CHECK(DFtCore::axis_ordering == DContainerOrdering::ordered);
    D_FT_CHECK(DFtCore::axis_bounds == DContainerBounds::unbounded);
    D_FT_CHECK(DFtCore::axis_multiplicity == DContainerMultiplicity::unique);
    D_FT_CHECK(DFtCore::axis_storage_kind ==
               DContainerStorageKind::dynamic_storage);
    D_FT_CHECK(DFtCore::axis_iterability == DContainerIterability::iterable);
    D_FT_CHECK(DFtCore::axis_thread_safety == DContainerThreadSafety::none);

    return true;
}

NS_END  // testing
NS_END  // djinterp
