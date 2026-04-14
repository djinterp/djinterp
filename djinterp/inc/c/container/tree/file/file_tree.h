/*******************************************************************************
* djinterp [container]                                              file_tree.h
*
*   A file-system tree container built on the N-ary tree infrastructure.
* Represents hierarchical directory structures with typed nodes (files,
* directories, symlinks) and supports path-based navigation using string
* paths (e.g. "src/core/main.c").
*   Provides operations for building trees manually, populating from an
* actual filesystem directory, querying by path, and depth-first or
* breadth-first traversal.
*
*
* path:      \inc\djinterp\c\container\tree\file\file_tree.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TYPE DEFINITIONS
      ----------------
      1.  d_file_node_type    (file, directory, symlink)
      2.  d_file_tree_node    (node in the file tree)
      3.  d_file_tree         (the tree itself)
      4.  fn_file_tree_visitor (visitor callback)

II.   NODE CREATION AND DESTRUCTION
      -----------------------------
      1.  d_file_tree_node_new_file
      2.  d_file_tree_node_new_dir
      3.  d_file_tree_node_new_dir_with_capacity
      4.  d_file_tree_node_new_symlink
      5.  d_file_tree_node_free

III.  TREE CREATION AND DESTRUCTION
      -----------------------------
      1.  d_file_tree_new
      2.  d_file_tree_new_from_path
      3.  d_file_tree_new_copy
      4.  d_file_tree_free

IV.   NAVIGATION
      ----------
      1.  d_file_tree_get_root
      2.  d_file_tree_get_node
      3.  d_file_tree_get_parent
      4.  d_file_tree_get_child
      5.  d_file_tree_get_child_by_name

V.    MODIFICATION
      ------------
      1.  d_file_tree_add_file
      2.  d_file_tree_add_dir
      3.  d_file_tree_add_symlink
      4.  d_file_tree_mkdir_p
      5.  d_file_tree_remove
      6.  d_file_tree_move
      7.  d_file_tree_clear

VI.   QUERY
      -----
      1.  d_file_tree_is_empty
      2.  d_file_tree_size
      3.  d_file_tree_contains
      4.  d_file_tree_is_file
      5.  d_file_tree_is_dir
      6.  d_file_tree_child_count
      7.  d_file_tree_depth

VII.  TRAVERSAL
      ---------
      1.  d_file_tree_traverse_preorder
      2.  d_file_tree_traverse_postorder
      3.  d_file_tree_traverse_breadth_first

VIII. UTILITY
      -------
      1.  d_file_tree_node_path
      2.  d_file_tree_node_name
      3.  d_file_tree_print
*/

#ifndef DJINTERP_C_CONTAINER_TREE_FILE_
#define DJINTERP_C_CONTAINER_TREE_FILE_ 1

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "../../../djinterp.h"
#include "../../container.h"
#include "../../../dstring.h"
#include "../../../dfile.h"


// d_file_node_type
//   enum: classifies a file tree node as a regular file,
// directory, or symbolic link.
enum d_file_node_type
{
    D_FILE_NODE_TYPE_FILE    = 0x00,
    D_FILE_NODE_TYPE_DIR,
    D_FILE_NODE_TYPE_SYMLINK
};

// d_file_tree_node
//   struct: a single node in a file tree. Every node stores its
// name, type, and optional metadata. Directory nodes additionally
// maintain an array of child pointers.
struct d_file_tree_node
{
    struct d_string*           name;       // filename component
    enum d_file_node_type      type;       // file, dir, or symlink
    struct d_file_tree_node*   parent;     // NULL for root
    uint64_t                   file_size;  // size in bytes (files)
    uint32_t                   mode;       // permissions
    uint64_t                   mtime;      // modification time
    void*                      user_data;  // arbitrary payload

    // directory-only fields
    struct d_file_tree_node**  children;   // child node array
    size_t                     count;      // number of children
    size_t                     capacity;   // allocated slots

    // symlink-only field
    struct d_string*           link_target;
};

// d_file_tree
//   struct: a file-system tree rooted at a single directory node.
// Tracks total node count and the separator character used when
// parsing or formatting string paths.
struct d_file_tree
{
    struct d_file_tree_node* root;
    size_t                   size;       // total node count
    char                     separator;  // path separator ('/' or '\\')
};

// fn_file_tree_visitor
//   typedef: callback invoked during traversal. Receives the
// current node, its depth in the tree, and an opaque context
// pointer.
typedef void (*fn_file_tree_visitor)(struct d_file_tree_node* _node,
                                     size_t                   _depth,
                                     void*                    _context);


/*******************************************************************************
* II.   Node creation and destruction
*******************************************************************************/

// II.   node creation and destruction
struct d_file_tree_node* d_file_tree_node_new_file(const char* _name);
struct d_file_tree_node* d_file_tree_node_new_dir(const char* _name);
struct d_file_tree_node* d_file_tree_node_new_dir_with_capacity(const char* _name,
                                                                size_t      _capacity);
struct d_file_tree_node* d_file_tree_node_new_symlink(const char* _name,
                                                      const char* _target);
void                     d_file_tree_node_free(struct d_file_tree_node* _node);

// III.  tree creation and destruction
struct d_file_tree*      d_file_tree_new(const char* _root_name);
struct d_file_tree*      d_file_tree_new_with_separator(const char* _root_name,
                                                        char        _separator);
struct d_file_tree*      d_file_tree_new_from_path(const char* _filesystem_path,
                                                   bool        _recursive);
struct d_file_tree*      d_file_tree_new_copy(const struct d_file_tree* _other);
void                     d_file_tree_free(struct d_file_tree* _tree);

// IV.   navigation
struct d_file_tree_node* d_file_tree_get_root(const struct d_file_tree* _tree);
struct d_file_tree_node* d_file_tree_get_node(const struct d_file_tree* _tree,
                                              const char*               _path);
struct d_file_tree_node* d_file_tree_get_parent(const struct d_file_tree_node* _node);
struct d_file_tree_node* d_file_tree_get_child(const struct d_file_tree_node* _node,
                                               size_t                         _index);
struct d_file_tree_node* d_file_tree_get_child_by_name(const struct d_file_tree_node* _node,
                                                       const char*                    _name);

// V.    modification
int                      d_file_tree_add_file(struct d_file_tree* _tree,
                                              const char*         _path);
int                      d_file_tree_add_dir(struct d_file_tree* _tree,
                                             const char*         _path);
int                      d_file_tree_add_symlink(struct d_file_tree* _tree,
                                                 const char*         _path,
                                                 const char*         _target);
int                      d_file_tree_mkdir_p(struct d_file_tree* _tree,
                                             const char*         _path);
int                      d_file_tree_remove(struct d_file_tree* _tree,
                                            const char*         _path);
int                      d_file_tree_move(struct d_file_tree* _tree,
                                          const char*         _old_path,
                                          const char*         _new_path);
void                     d_file_tree_clear(struct d_file_tree* _tree);

// VI.   query
bool                     d_file_tree_is_empty(const struct d_file_tree* _tree);
size_t                   d_file_tree_size(const struct d_file_tree* _tree);
bool                     d_file_tree_contains(const struct d_file_tree* _tree,
                                              const char*               _path);
bool                     d_file_tree_is_file(const struct d_file_tree* _tree,
                                             const char*               _path);
bool                     d_file_tree_is_dir(const struct d_file_tree* _tree,
                                            const char*               _path);
size_t                   d_file_tree_child_count(const struct d_file_tree* _tree,
                                                 const char*               _path);
size_t                   d_file_tree_depth(const struct d_file_tree_node* _node);

// VII.  traversal
void                     d_file_tree_traverse_preorder(const struct d_file_tree* _tree,
                                                       fn_file_tree_visitor      _visitor,
                                                       void*                     _context);
void                     d_file_tree_traverse_postorder(const struct d_file_tree* _tree,
                                                        fn_file_tree_visitor      _visitor,
                                                        void*                     _context);
void                     d_file_tree_traverse_breadth_first(const struct d_file_tree* _tree,
                                                            fn_file_tree_visitor      _visitor,
                                                            void*                     _context);

// VIII. utility
struct d_string*         d_file_tree_node_path(const struct d_file_tree*      _tree,
                                               const struct d_file_tree_node* _node);
const struct d_string*   d_file_tree_node_name(const struct d_file_tree_node* _node);
void                     d_file_tree_print(const struct d_file_tree* _tree,
                                           FILE*                     _stream);


#endif  // DJINTERP_C_CONTAINER_TREE_FILE_
