/*******************************************************************************
* djinterp [cpp][container]                            tree_floor_tests_path.cpp
*
*   tree_path.hpp: the path helper over a node arena whose payloads are
* names: root/{src/{main, util}, docs}.
*
*
* path:      /tests/djinterp/cpp/container/tree/tree_floor_tests_path.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "tree_floor_tests.hpp"  // D_TF_CHECK, the declarations
#include <djinterp/cpp/container/tree/tree_path.hpp>  // under test

// std
#include <string>  // std::string


NS_DJINTERP
NS_TESTING

namespace
{
    // name_of
    //   fixture: the accessor, as the header documents it: a node's
    // component, here a view of its payload, a name.
    struct name_of
    {
        DComponentView operator()(
            const DNodeArena<std::string>& _arena,
            node_id                        _id
        ) const
        {
            const std::string& name = _arena.data(_id);

            return DComponentView{ name.data(), name.size() };
        }
    };

    // DTfArena
    //   fixture: root/{src/{main, util}, docs}, built with append_child.
    struct DTfArena
    {
        DNodeArena<std::string> arena;
        node_id                 root;
        node_id                 src;
        node_id                 main;
        node_id                 util;
        node_id                 docs;

        DTfArena()
            : arena(),
              root(arena.allocate(std::string("root"))),
              src(arena.allocate(std::string("src"))),
              main(arena.allocate(std::string("main"))),
              util(arena.allocate(std::string("util"))),
              docs(arena.allocate(std::string("docs")))
        {
            arena.append_child(root, src);
            arena.append_child(src, main);
            arena.append_child(src, util);
            arena.append_child(root, docs);
        }
    };
}

/*
tests_tree_floor_path_resolve_and_build
  resolving names from a node, and building a node's path
  Tests the following:
  - resolve from the root finds each node by the names below it
  - a missing name resolves to null_node
  - build joins the names from the root, with the separator asked for
*/
bool tests_tree_floor_path_resolve_and_build()
{
    DTfArena t;
    auto     path = make_tree_path<DComponentView>(t.arena, name_of());

    D_TF_CHECK(path.resolve(t.root, std::string("src/util")) == t.util);
    D_TF_CHECK(path.resolve(t.root, std::string("docs")) == t.docs);
    D_TF_CHECK(path.resolve(t.src, std::string("main")) == t.main);
    D_TF_CHECK(path.resolve(t.root, std::string("src/none")) == null_node);

    const std::string built = path.build(t.util);

    D_TF_CHECK(built.find("src/util") != std::string::npos);
    D_TF_CHECK(path.build(t.util, '\\').find("src\\util") !=
               std::string::npos);

    return true;
}

/*
tests_tree_floor_path_relations
  where nodes stand to one another
  Tests the following:
  - depth counts the parent links up to the root
  - ancestors lists them; lca and is_ancestor
  - the relative path climbs with ".." and descends by name
*/
bool tests_tree_floor_path_relations()
{
    DTfArena t;
    auto     path = make_tree_path<DComponentView>(t.arena, name_of());

    D_TF_CHECK(path.depth(t.root) == 0);
    D_TF_CHECK(path.depth(t.util) == 2);
    D_TF_CHECK(path.ancestors(t.util).size() == 2);
    D_TF_CHECK(path.lca(t.main, t.util) == t.src);
    D_TF_CHECK(path.lca(t.util, t.docs) == t.root);
    D_TF_CHECK(path.is_ancestor(t.src, t.main));
    D_TF_CHECK(!path.is_ancestor(t.docs, t.main));

    const std::string rel = path.relative_string(t.main, t.docs);

    D_TF_CHECK(rel.find("..") != std::string::npos);
    D_TF_CHECK(rel.find("docs") != std::string::npos);

    return true;
}

NS_END  // testing
NS_END  // djinterp
