/*******************************************************************************
* djinterp [cpp][container]                             file_tree_tests_path.cpp
*
*   file_tree_path.hpp: the path helper over a tree built by hand on the null
* scanner.
*
*
* path:      /tests/djinterp/cpp/container/tree/file/file_tree_tests_path.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "file_tree_tests.hpp"  // D_FT_CHECK, the declarations
#include <djinterp/cpp/container/tree/file/file_tree_path.hpp>  // under test


NS_DJINTERP
NS_TESTING

namespace
{
    using DFpCore = DFileTreeCore<DNullScanner>;

    // DFpBuilt
    //   fixture: proj/{src/lib/a.tar.gz, src/main.cpp, docs/guide.md}.
    struct DFpBuilt
    {
        DFpCore     tree;
        DFileNodeId root;
        DFileNodeId src;
        DFileNodeId lib;
        DFileNodeId archive;
        DFileNodeId main;
        DFileNodeId docs;
        DFileNodeId guide;

        DFpBuilt()
            : tree(),
              root(tree.scan("proj")),
              src(tree.add_child(root, "src", file_type_directory)),
              lib(tree.add_child(src, "lib", file_type_directory)),
              archive(tree.add_child(lib, "a.tar.gz", file_type_regular, 7)),
              main(tree.add_child(src, "main.cpp", file_type_regular, 9)),
              docs(tree.add_child(root, "docs", file_type_directory)),
              guide(tree.add_child(docs, "guide.md", file_type_regular, 2))
        {}
    };
}

/*
tests_file_tree_path_resolve_and_build
  resolving and building paths
  Tests the following:
  - resolve takes '/' and '\\' alike, and a missing component gives null
  - resolve_many answers each path in order
  - with no starting node, each starts at the tree's root (until
    2026.11.23 they started at node 0, the null node, and found nothing)
  - build_posix and build_windows differ only in the separator
  - extension and stem split at the last dot
*/
bool tests_file_tree_path_resolve_and_build()
{
    DFpBuilt                     b;
    DFileTreePath<DFpCore>       path(b.tree);
    const std::vector<DFileNodeId> many =
        path.resolve_many({ "src/main.cpp", "nope", "docs\\guide.md" });

    D_FT_CHECK(path.resolve("src/lib/a.tar.gz") == b.archive);
    D_FT_CHECK(path.resolve("src\\lib\\a.tar.gz") == b.archive);
    D_FT_CHECK(path.resolve("src/nope") == null_file_node);
    D_FT_CHECK(many.size() == 3);
    D_FT_CHECK(many[0] == b.main);
    D_FT_CHECK(many[1] == null_file_node);
    D_FT_CHECK(many[2] == b.guide);
    D_FT_CHECK(path.resolve("lib/a.tar.gz", b.src) == b.archive);
    D_FT_CHECK(path.all_paths().size() == b.tree.size());
    D_FT_CHECK(path.nodes_at_depth(1).size() == 2);

    std::string posix   = path.build_posix(b.archive);
    std::string windows = path.build_windows(b.archive);

    D_FT_CHECK(posix.size() == windows.size());
    D_FT_CHECK(posix.find("src/lib/a.tar.gz") != std::string::npos);
    D_FT_CHECK(windows.find("src\\lib\\a.tar.gz") != std::string::npos);

    D_FT_CHECK(path.extension(b.main) == "cpp" ||
               path.extension(b.main) == ".cpp");
    D_FT_CHECK(path.stem(b.main) == "main");
    D_FT_CHECK(path.stem(b.archive) == "a.tar");

    return true;
}

/*
tests_file_tree_path_relations
  where nodes stand to one another
  Tests the following:
  - level counts parent links up; depth is its retained spelling; height
    goes down to the deepest leaf
  - ancestors, lca, is_ancestor and is_descendant
  - the relative path climbs with ".." and descends by name
*/
bool tests_file_tree_path_relations()
{
    DFpBuilt               b;
    DFileTreePath<DFpCore> path(b.tree);

    D_FT_CHECK(path.level(b.archive) == 3);
    D_FT_CHECK(path.depth(b.archive) == path.level(b.archive));
    D_FT_CHECK(path.height(b.src) == 2);
    D_FT_CHECK(path.height(b.archive) == 0);

    D_FT_CHECK(path.ancestors(b.archive).size() == 3);
    D_FT_CHECK(path.lca(b.archive, b.main) == b.src);
    D_FT_CHECK(path.lca(b.archive, b.guide) == b.root);
    D_FT_CHECK(path.is_ancestor(b.src, b.archive));
    D_FT_CHECK(path.is_descendant(b.archive, b.src));
    D_FT_CHECK(!path.is_ancestor(b.docs, b.archive));

    const std::string rel = path.relative_string(b.archive, b.guide);

    D_FT_CHECK(rel.find("..") != std::string::npos);
    D_FT_CHECK(rel.find("docs") != std::string::npos);
    D_FT_CHECK(rel.find("guide.md") != std::string::npos);

    return true;
}

NS_END  // testing
NS_END  // djinterp
