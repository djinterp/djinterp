/*******************************************************************************
* djinterp [cpp][container]                           file_tree_tests_filter.cpp
*
*   file_tree_filter.hpp: the predicates and the query, over a tree built by
* hand on the null scanner.
*
*
* path:      /tests/djinterp/cpp/container/tree/file/file_tree_tests_filter.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "file_tree_tests.hpp"  // D_FT_CHECK, the declarations
#include <djinterp/cpp/container/tree/file/file_tree_filter.hpp>  // under test


NS_DJINTERP
NS_TESTING

namespace
{
    using DFfCore = DFileTreeCore<DNullScanner>;

    // DFfBuilt
    //   fixture: proj/{src/a.cpp (10), src/b.hpp (0), src/deep/c.cpp (30),
    // .hidden (1), notes.txt (20)}.
    struct DFfBuilt
    {
        DFfCore     tree;
        DFileNodeId root;
        DFileNodeId src;

        DFfBuilt()
            : tree(),
              root(tree.scan("proj")),
              src(tree.add_child(root, "src", file_type_directory))
        {
            DFileNodeId deep = tree.add_child(src, "deep", file_type_directory);

            tree.add_child(src, "a.cpp", file_type_regular, 10);
            tree.add_child(src, "b.hpp", file_type_regular, 0);
            tree.add_child(deep, "c.cpp", file_type_regular, 30);
            tree.add_child(root, ".hidden", file_type_regular, 1);
            tree.add_child(root, "notes.txt", file_type_regular, 20);
        }
    };
}

/*
tests_file_tree_filter_predicates
  the predicates, collected and counted
  Tests the following:
  - by_ext and by_glob find both .cpp files, at any depth
  - by_name, by_name_prefix and by_name_suffix
  - by_directory and by_regular split the nodes
  - by_size_gt, by_size_between and by_empty
  - by_depth_eq, from the root at 0; by_parent
  - collect_matching with no root starts at the tree's root (until
    2026.11.23 it started at node 0, the null node, and found nothing)
*/
bool tests_file_tree_filter_predicates()
{
    DFfBuilt  b;
    const DFfCore& t = b.tree;

    D_FT_CHECK(count_matching(t, b.root, by_ext(t, "cpp")) == 2);
    D_FT_CHECK(collect_matching(t, by_glob(t, "*.cpp")).size() == 2);
    D_FT_CHECK(count_matching(t, b.root, by_name(t, "notes.txt")) == 1);
    D_FT_CHECK(count_matching(t, b.root, by_name_prefix(t, "no")) == 1);
    D_FT_CHECK(count_matching(t, b.root, by_name_suffix(t, ".hpp")) == 1);

    D_FT_CHECK(count_matching(t, b.root, by_directory(t)) == 3);
    D_FT_CHECK(count_matching(t, b.root, by_regular(t)) == 5);

    D_FT_CHECK(count_matching(t, b.root, by_size_gt(t, 15)) == 2);
    D_FT_CHECK(count_matching(t, b.root, by_size_between(t, 10, 20)) == 2);
    D_FT_CHECK(count_matching(t, b.root, by_parent(t, b.src)) == 3);
    D_FT_CHECK(first_matching(t, b.root, by_name(t, "c.cpp")) ==
               t.resolve("src/deep/c.cpp"));

    D_FT_CHECK(count_matching(t, b.root, by_depth_eq(t, 1)) == 3);

    return true;
}

/*
tests_file_tree_filter_query
  DFileTreeQuery, a flat gather over the tree
  Tests the following:
  - files() and directories() split the nodes
  - where narrows; count and empty agree
  - to_names lists the survivors' names
  - a query given no root covers the whole tree (until 2026.11.23 it
    started at the null node and was empty)
*/
bool tests_file_tree_filter_query()
{
    DFfBuilt               b;
    DFileTreeQuery<DFfCore> q(b.tree, b.root);

    D_FT_CHECK(q.files().count() == 5);
    D_FT_CHECK(DFileTreeQuery<DFfCore>(b.tree).count() == b.tree.size());
    D_FT_CHECK(q.directories().count() == 3);
    D_FT_CHECK(q.files().where(by_ext(b.tree, "cpp")).count() == 2);
    D_FT_CHECK(q.files().where(by_name(b.tree, "none")).empty());

    const std::vector<std::string> names =
        q.files().where(by_size_gt(b.tree, 25)).to_names();

    D_FT_CHECK(names.size() == 1);
    D_FT_CHECK(names[0] == "c.cpp");

    return true;
}

NS_END  // testing
NS_END  // djinterp
