#include "../../../../../../inc/djinterp/c/container/tree/file/file_tree.h"


// D_FILE_TREE_DEFAULT_CHILD_CAPACITY
//   constant: default number of child slots allocated for a new
// directory node.
#define D_FILE_TREE_DEFAULT_CHILD_CAPACITY 8


/*******************************************************************************
* Internal helpers
*******************************************************************************/

/*
d_file_tree_internal_node_alloc
  Allocates and zero-initializes a d_file_tree_node.

Parameter(s):
  none.
Return:
  A pointer to the newly allocated node, or NULL on allocation failure.
*/
static struct d_file_tree_node*
d_file_tree_internal_node_alloc
(
    void
)
{
    struct d_file_tree_node* node;

    node = (struct d_file_tree_node*)calloc(1, sizeof(struct d_file_tree_node));

    return node;
}


/*
d_file_tree_internal_dir_init_children
  Initializes the children array of a directory node to the given capacity.

Parameter(s):
  _node:     the directory node whose children array to initialize.
  _capacity: the initial number of child slots to allocate.
Return:
  0 on success, -1 on allocation failure.
*/
static int
d_file_tree_internal_dir_init_children
(
    struct d_file_tree_node* _node,
    size_t                   _capacity
)
{
    if (!_node)
    {
        return -1;
    }

    if (_capacity == 0)
    {
        _capacity = D_FILE_TREE_DEFAULT_CHILD_CAPACITY;
    }

    _node->children = (struct d_file_tree_node**)calloc(
                          _capacity,
                          sizeof(struct d_file_tree_node*));

    if (!_node->children)
    {
        return -1;
    }

    _node->count    = 0;
    _node->capacity = _capacity;

    return 0;
}


/*
d_file_tree_internal_dir_grow
  Doubles the children array capacity of a directory node when full.

Parameter(s):
  _node: the directory node whose children array to grow.
Return:
  0 on success, -1 on failure.
*/
static int
d_file_tree_internal_dir_grow
(
    struct d_file_tree_node* _node
)
{
    struct d_file_tree_node** new_children;
    size_t                    new_capacity;

    if (!_node)
    {
        return -1;
    }

    new_capacity = _node->capacity * 2;
    if (new_capacity == 0)
    {
        new_capacity = D_FILE_TREE_DEFAULT_CHILD_CAPACITY;
    }

    new_children = (struct d_file_tree_node**)realloc(
                       _node->children,
                       new_capacity * sizeof(struct d_file_tree_node*));

    if (!new_children)
    {
        return -1;
    }

    _node->children = new_children;
    _node->capacity = new_capacity;

    return 0;
}


/*
d_file_tree_internal_dir_add_child
  Appends a child node to a directory node, growing the array if necessary.
Sets the child's parent pointer.

Parameter(s):
  _parent: the directory node to add the child to.
  _child:  the child node to add.
Return:
  0 on success, -1 on failure.
*/
static int
d_file_tree_internal_dir_add_child
(
    struct d_file_tree_node* _parent,
    struct d_file_tree_node* _child
)
{
    if ( (!_parent) ||
         (!_child)  ||
         (_parent->type != D_FILE_NODE_TYPE_DIR) )
    {
        return -1;
    }

    // grow if at capacity
    if (_parent->count >= _parent->capacity)
    {
        if (d_file_tree_internal_dir_grow(_parent) != 0)
        {
            return -1;
        }
    }

    _parent->children[_parent->count] = _child;
    _parent->count++;
    _child->parent = _parent;

    return 0;
}


/*
d_file_tree_internal_dir_remove_child
  Removes a child from a directory node by pointer identity. Does not free
the removed child.

Parameter(s):
  _parent: the directory node to remove from.
  _child:  the child node to detach.
Return:
  0 on success, -1 if the child was not found.
*/
static int
d_file_tree_internal_dir_remove_child
(
    struct d_file_tree_node* _parent,
    struct d_file_tree_node* _child
)
{
    size_t i;

    if ( (!_parent) ||
         (!_child)  ||
         (_parent->type != D_FILE_NODE_TYPE_DIR) )
    {
        return -1;
    }

    i = 0;
    while (i < _parent->count)
    {
        if (_parent->children[i] == _child)
        {
            // shift remaining children down
            _parent->count--;
            while (i < _parent->count)
            {
                _parent->children[i] = _parent->children[i + 1];
                i++;
            }

            _parent->children[_parent->count] = NULL;
            _child->parent = NULL;

            return 0;
        }

        i++;
    }

    return -1;
}


/*
d_file_tree_internal_resolve_path
  Walks from the root of the tree along the given slash-separated path,
returning the node at the final component, or NULL if any component is
missing.

Parameter(s):
  _tree: the file tree to search.
  _path: the slash-separated path string (e.g. "src/core/main.c").
Return:
  A pointer to the resolved node, or NULL if the path does not exist.
*/
static struct d_file_tree_node*
d_file_tree_internal_resolve_path
(
    const struct d_file_tree* _tree,
    const char*               _path
)
{
    struct d_file_tree_node* current;
    char*                    buf;
    char*                    saveptr;
    char*                    token;
    size_t                   path_len;
    char                     sep[2];
    bool                     found;
    size_t                   i;

    if ( (!_tree)       ||
         (!_tree->root) ||
         (!_path) )
    {
        return NULL;
    }

    // empty path means root
    path_len = strlen(_path);
    if (path_len == 0)
    {
        return _tree->root;
    }

    buf = d_strdup(_path);
    if (!buf)
    {
        return NULL;
    }

    sep[0] = _tree->separator;
    sep[1] = '\0';

    current = _tree->root;
    saveptr = NULL;
    token   = d_strtok_r(buf, sep, &saveptr);

    while (token)
    {
        if (current->type != D_FILE_NODE_TYPE_DIR)
        {
            // cannot descend into a non-directory
            free(buf);

            return NULL;
        }

        found = false;
        for (i = 0; i < current->count; i++)
        {
            if ( (current->children[i])       &&
                 (current->children[i]->name) &&
                 (d_string_equals_cstr(current->children[i]->name,
                                       token)) )
            {
                current = current->children[i];
                found   = true;
                break;
            }
        }

        if (!found)
        {
            free(buf);

            return NULL;
        }

        token = d_strtok_r(NULL, sep, &saveptr);
    }

    free(buf);

    return current;
}


/*
d_file_tree_internal_split_parent_child
  Splits a path into the parent directory path and the final component
name. The caller must free the returned strings.

Parameter(s):
  _path:        the full path to split.
  _separator:   the path separator character.
  _parent_out:  receives a newly allocated string for the parent portion
                (empty string if the path has no separator).
  _child_out:   receives a newly allocated string for the final component.
Return:
  0 on success, -1 on failure.
*/
static int
d_file_tree_internal_split_parent_child
(
    const char* _path,
    char        _separator,
    char**      _parent_out,
    char**      _child_out
)
{
    const char* last_sep;
    size_t      parent_len;
    size_t      path_len;

    if ( (!_path)       ||
         (!_parent_out) ||
         (!_child_out) )
    {
        return -1;
    }

    path_len = strlen(_path);
    if (path_len == 0)
    {
        return -1;
    }

    // find last separator
    last_sep = NULL;
    for (size_t i = 0; i < path_len; i++)
    {
        if (_path[i] == _separator)
        {
            last_sep = &_path[i];
        }
    }

    if (!last_sep)
    {
        // no separator: parent is empty, child is the whole path
        *_parent_out = d_strdup("");
        *_child_out  = d_strdup(_path);
    }
    else
    {
        parent_len   = (size_t)(last_sep - _path);
        *_parent_out = d_strndup(_path, parent_len);
        *_child_out  = d_strdup(last_sep + 1);
    }

    if ( (!*_parent_out) ||
         (!*_child_out) )
    {
        free(*_parent_out);
        free(*_child_out);
        *_parent_out = NULL;
        *_child_out  = NULL;

        return -1;
    }

    return 0;
}


/*
d_file_tree_internal_node_free_recursive
  Recursively frees a node and all of its descendants.

Parameter(s):
  _node: the node to free; may be NULL.
Return:
  none.
*/
static void
d_file_tree_internal_node_free_recursive
(
    struct d_file_tree_node* _node
)
{
    size_t i;

    if (!_node)
    {
        return;
    }

    // free children recursively for directory nodes
    if (_node->type == D_FILE_NODE_TYPE_DIR)
    {
        for (i = 0; i < _node->count; i++)
        {
            d_file_tree_internal_node_free_recursive(_node->children[i]);
        }

        free(_node->children);
    }

    if (_node->name)
    {
        d_string_free(_node->name);
    }

    if (_node->link_target)
    {
        d_string_free(_node->link_target);
    }

    free(_node);

    return;
}


/*
d_file_tree_internal_copy_node
  Recursively deep-copies a node and all of its descendants.

Parameter(s):
  _node: the node to copy.
Return:
  A pointer to the newly allocated copy, or NULL on failure.
*/
static struct d_file_tree_node*
d_file_tree_internal_copy_node
(
    const struct d_file_tree_node* _node
)
{
    struct d_file_tree_node* copy;
    struct d_file_tree_node* child_copy;
    size_t                   i;

    if (!_node)
    {
        return NULL;
    }

    copy = d_file_tree_internal_node_alloc();
    if (!copy)
    {
        return NULL;
    }

    // copy common fields
    copy->type      = _node->type;
    copy->file_size = _node->file_size;
    copy->mode      = _node->mode;
    copy->mtime     = _node->mtime;
    copy->user_data = _node->user_data;

    if (_node->name)
    {
        copy->name = d_string_new_copy(_node->name);
        if (!copy->name)
        {
            free(copy);

            return NULL;
        }
    }

    // copy symlink target
    if ( (_node->type == D_FILE_NODE_TYPE_SYMLINK) &&
         (_node->link_target) )
    {
        copy->link_target = d_string_new_copy(_node->link_target);
        if (!copy->link_target)
        {
            d_string_free(copy->name);
            free(copy);

            return NULL;
        }
    }

    // recursively copy children for directory nodes
    if (_node->type == D_FILE_NODE_TYPE_DIR)
    {
        if (d_file_tree_internal_dir_init_children(copy,
                                                   _node->capacity) != 0)
        {
            d_string_free(copy->name);
            free(copy);

            return NULL;
        }

        for (i = 0; i < _node->count; i++)
        {
            child_copy = d_file_tree_internal_copy_node(_node->children[i]);
            if (!child_copy)
            {
                d_file_tree_internal_node_free_recursive(copy);

                return NULL;
            }

            child_copy->parent        = copy;
            copy->children[copy->count] = child_copy;
            copy->count++;
        }
    }

    return copy;
}


/*
d_file_tree_internal_count_nodes
  Recursively counts a node and all of its descendants.

Parameter(s):
  _node: the subtree root to count from.
Return:
  The total number of nodes in the subtree (including the root).
*/
static size_t
d_file_tree_internal_count_nodes
(
    const struct d_file_tree_node* _node
)
{
    size_t total;
    size_t i;

    if (!_node)
    {
        return 0;
    }

    total = 1;

    if (_node->type == D_FILE_NODE_TYPE_DIR)
    {
        for (i = 0; i < _node->count; i++)
        {
            total += d_file_tree_internal_count_nodes(_node->children[i]);
        }
    }

    return total;
}


/*
d_file_tree_internal_traverse_preorder
  Recursive pre-order traversal helper.

Parameter(s):
  _node:    the current node.
  _depth:   the current depth.
  _visitor: the callback to invoke.
  _context: opaque context passed to the visitor.
Return:
  none.
*/
static void
d_file_tree_internal_traverse_preorder
(
    struct d_file_tree_node* _node,
    size_t                   _depth,
    fn_file_tree_visitor     _visitor,
    void*                    _context
)
{
    size_t i;

    if (!_node)
    {
        return;
    }

    _visitor(_node, _depth, _context);

    if (_node->type == D_FILE_NODE_TYPE_DIR)
    {
        for (i = 0; i < _node->count; i++)
        {
            d_file_tree_internal_traverse_preorder(_node->children[i],
                                                   _depth + 1,
                                                   _visitor,
                                                   _context);
        }
    }

    return;
}


/*
d_file_tree_internal_traverse_postorder
  Recursive post-order traversal helper.

Parameter(s):
  _node:    the current node.
  _depth:   the current depth.
  _visitor: the callback to invoke.
  _context: opaque context passed to the visitor.
Return:
  none.
*/
static void
d_file_tree_internal_traverse_postorder
(
    struct d_file_tree_node* _node,
    size_t                   _depth,
    fn_file_tree_visitor     _visitor,
    void*                    _context
)
{
    size_t i;

    if (!_node)
    {
        return;
    }

    if (_node->type == D_FILE_NODE_TYPE_DIR)
    {
        for (i = 0; i < _node->count; i++)
        {
            d_file_tree_internal_traverse_postorder(_node->children[i],
                                                    _depth + 1,
                                                    _visitor,
                                                    _context);
        }
    }

    _visitor(_node, _depth, _context);

    return;
}


/*
d_file_tree_internal_print_node
  Recursive helper for d_file_tree_print. Prints the tree with
indentation to visualize the hierarchy.

Parameter(s):
  _node:   the current node.
  _depth:  the current depth (for indentation).
  _stream: the output stream.
Return:
  none.
*/
static void
d_file_tree_internal_print_node
(
    const struct d_file_tree_node* _node,
    size_t                         _depth,
    FILE*                          _stream
)
{
    size_t i;

    if (!_node)
    {
        return;
    }

    // indent
    for (i = 0; i < _depth; i++)
    {
        fprintf(_stream, "  ");
    }

    // print name and type indicator
    if (_node->name)
    {
        switch (_node->type)
        {
            case D_FILE_NODE_TYPE_DIR:
                fprintf(_stream,
                        "%s/\n",
                        d_string_cstr(_node->name));
                break;

            case D_FILE_NODE_TYPE_SYMLINK:
                fprintf(_stream,
                        "%s -> %s\n",
                        d_string_cstr(_node->name),
                        _node->link_target
                            ? d_string_cstr(_node->link_target)
                            : "?");
                break;

            case D_FILE_NODE_TYPE_FILE:
            default:
                fprintf(_stream,
                        "%s\n",
                        d_string_cstr(_node->name));
                break;
        }
    }

    // recurse into children
    if (_node->type == D_FILE_NODE_TYPE_DIR)
    {
        for (i = 0; i < _node->count; i++)
        {
            d_file_tree_internal_print_node(_node->children[i],
                                            _depth + 1,
                                            _stream);
        }
    }

    return;
}


/*******************************************************************************
* II.   Node creation and destruction
*******************************************************************************/

/*
d_file_tree_node_new_file
  Creates a new file node with the given name.

Parameter(s):
  _name: the filename for this node; must not be NULL.
Return:
  A pointer to the newly allocated file node, or NULL on failure.
*/
struct d_file_tree_node*
d_file_tree_node_new_file
(
    const char* _name
)
{
    struct d_file_tree_node* node;

    if (!_name)
    {
        return NULL;
    }

    node = d_file_tree_internal_node_alloc();
    if (!node)
    {
        return NULL;
    }

    node->name = d_string_new_from_cstr(_name);
    if (!node->name)
    {
        free(node);

        return NULL;
    }

    node->type = D_FILE_NODE_TYPE_FILE;

    return node;
}


/*
d_file_tree_node_new_dir
  Creates a new directory node with the given name and default child
capacity.

Parameter(s):
  _name: the directory name for this node; must not be NULL.
Return:
  A pointer to the newly allocated directory node, or NULL on failure.
*/
struct d_file_tree_node*
d_file_tree_node_new_dir
(
    const char* _name
)
{
    return d_file_tree_node_new_dir_with_capacity(
               _name,
               D_FILE_TREE_DEFAULT_CHILD_CAPACITY);
}


/*
d_file_tree_node_new_dir_with_capacity
  Creates a new directory node with the given name and specified initial
child capacity.

Parameter(s):
  _name:     the directory name; must not be NULL.
  _capacity: the initial number of child slots.
Return:
  A pointer to the newly allocated directory node, or NULL on failure.
*/
struct d_file_tree_node*
d_file_tree_node_new_dir_with_capacity
(
    const char* _name,
    size_t      _capacity
)
{
    struct d_file_tree_node* node;

    if (!_name)
    {
        return NULL;
    }

    node = d_file_tree_internal_node_alloc();
    if (!node)
    {
        return NULL;
    }

    node->name = d_string_new_from_cstr(_name);
    if (!node->name)
    {
        free(node);

        return NULL;
    }

    node->type = D_FILE_NODE_TYPE_DIR;

    if (d_file_tree_internal_dir_init_children(node, _capacity) != 0)
    {
        d_string_free(node->name);
        free(node);

        return NULL;
    }

    return node;
}


/*
d_file_tree_node_new_symlink
  Creates a new symbolic link node with the given name and target path.

Parameter(s):
  _name:   the link name; must not be NULL.
  _target: the target path the symlink points to; must not be NULL.
Return:
  A pointer to the newly allocated symlink node, or NULL on failure.
*/
struct d_file_tree_node*
d_file_tree_node_new_symlink
(
    const char* _name,
    const char* _target
)
{
    struct d_file_tree_node* node;

    if ( (!_name) ||
         (!_target) )
    {
        return NULL;
    }

    node = d_file_tree_internal_node_alloc();
    if (!node)
    {
        return NULL;
    }

    node->name = d_string_new_from_cstr(_name);
    if (!node->name)
    {
        free(node);

        return NULL;
    }

    node->link_target = d_string_new_from_cstr(_target);
    if (!node->link_target)
    {
        d_string_free(node->name);
        free(node);

        return NULL;
    }

    node->type = D_FILE_NODE_TYPE_SYMLINK;

    return node;
}


/*
d_file_tree_node_free
  Frees a single file tree node and its owned strings. Does NOT free
children; use d_file_tree_free or the internal recursive free for that.

Parameter(s):
  _node: the node to free; may be NULL.
Return:
  none.
*/
void
d_file_tree_node_free
(
    struct d_file_tree_node* _node
)
{
    if (!_node)
    {
        return;
    }

    if (_node->name)
    {
        d_string_free(_node->name);
    }

    if (_node->link_target)
    {
        d_string_free(_node->link_target);
    }

    // free children array (not children themselves)
    if (_node->children)
    {
        free(_node->children);
    }

    free(_node);

    return;
}


/*******************************************************************************
* III.  Tree creation and destruction
*******************************************************************************/

/*
d_file_tree_new
  Creates a new file tree with a root directory node of the given name,
using the platform default path separator.

Parameter(s):
  _root_name: the name for the root directory; must not be NULL.
Return:
  A pointer to the newly allocated file tree, or NULL on failure.
*/
struct d_file_tree*
d_file_tree_new
(
    const char* _root_name
)
{
    return d_file_tree_new_with_separator(_root_name,
                                          D_FILE_PATH_SEP);
}


/*
d_file_tree_new_with_separator
  Creates a new file tree with a root directory node and a specific path
separator character.

Parameter(s):
  _root_name: the name for the root directory; must not be NULL.
  _separator: the character used to delimit path components.
Return:
  A pointer to the newly allocated file tree, or NULL on failure.
*/
struct d_file_tree*
d_file_tree_new_with_separator
(
    const char* _root_name,
    char        _separator
)
{
    struct d_file_tree* tree;

    if (!_root_name)
    {
        return NULL;
    }

    tree = (struct d_file_tree*)calloc(1, sizeof(struct d_file_tree));
    if (!tree)
    {
        return NULL;
    }

    tree->root = d_file_tree_node_new_dir(_root_name);
    if (!tree->root)
    {
        free(tree);

        return NULL;
    }

    tree->size      = 1;
    tree->separator = _separator;

    return tree;
}


/*
d_file_tree_new_from_path
  Creates a file tree by scanning an actual filesystem directory. When
_recursive is true, descends into all subdirectories.

Parameter(s):
  _filesystem_path: the path to the directory to scan; must not be NULL.
  _recursive:       if true, scan subdirectories recursively.
Return:
  A pointer to the newly allocated file tree, or NULL on failure.
*/
struct d_file_tree*
d_file_tree_new_from_path
(
    const char* _filesystem_path,
    bool        _recursive
)
{
    struct d_file_tree*      tree;
    struct d_dir_t*          dir;
    struct d_dirent_t*       entry;
    char                     child_path[D_FILE_PATH_MAX];
    char                     name_buf[D_FILE_NAME_MAX];
    const char*              root_name;
    struct d_file_tree_node* child_node;
    struct d_file_tree*      subtree;

    if (!_filesystem_path)
    {
        return NULL;
    }

    // verify the path is a directory
    if (!d_is_dir(_filesystem_path))
    {
        return NULL;
    }

    // extract the directory's own name for the tree root
    root_name = d_basename(_filesystem_path,
                           name_buf,
                           sizeof(name_buf));

    if ( (!root_name) ||
         (root_name[0] == '\0') )
    {
        root_name = _filesystem_path;
    }

    tree = d_file_tree_new_with_separator(root_name,
                                          D_FILE_PATH_SEP);

    if (!tree)
    {
        return NULL;
    }

    dir = d_opendir(_filesystem_path);
    if (!dir)
    {
        d_file_tree_free(tree);

        return NULL;
    }

    entry = d_readdir(dir);
    while (entry)
    {
        // skip . and ..
        if ( (strcmp(entry->d_name, ".") == 0) ||
             (strcmp(entry->d_name, "..") == 0) )
        {
            entry = d_readdir(dir);
            continue;
        }

        // build full path to child
        d_path_join(child_path,
                    D_FILE_PATH_MAX,
                    _filesystem_path,
                    entry->d_name);

        if ( (entry->d_type == DT_DIR) &&
             (_recursive) )
        {
            // recursively scan subdirectory
            subtree = d_file_tree_new_from_path(child_path, true);
            if (subtree)
            {
                d_file_tree_internal_dir_add_child(tree->root,
                                                   subtree->root);
                tree->size += subtree->size;

                // detach root from subtree so free does not destroy it
                subtree->root = NULL;
                subtree->size = 0;
                d_file_tree_free(subtree);
            }
        }
        else if (entry->d_type == DT_DIR)
        {
            child_node = d_file_tree_node_new_dir(entry->d_name);
            if (child_node)
            {
                d_file_tree_internal_dir_add_child(tree->root,
                                                   child_node);
                tree->size++;
            }
        }
        else if (entry->d_type == DT_LNK)
        {
            // read symlink target
            char target_buf[D_FILE_PATH_MAX];
            ssize_t link_len;

            target_buf[0] = '\0';
#if D_FILE_HAS_SYMLINKS
            link_len = d_readlink(child_path,
                                  target_buf,
                                  D_FILE_PATH_MAX);
            if ( (link_len > 0) &&
                 (link_len < (ssize_t)D_FILE_PATH_MAX) )
            {
                target_buf[link_len] = '\0';
            }
#endif

            child_node = d_file_tree_node_new_symlink(
                             entry->d_name,
                             target_buf[0] ? target_buf : "?");
            if (child_node)
            {
                d_file_tree_internal_dir_add_child(tree->root,
                                                   child_node);
                tree->size++;
            }
        }
        else
        {
            // regular file or other type
            child_node = d_file_tree_node_new_file(entry->d_name);
            if (child_node)
            {
                // populate file metadata
                struct d_stat_t st;
                if (d_stat(child_path, &st) == 0)
                {
                    child_node->file_size = st.st_size;
                    child_node->mode      = st.st_mode;
                    child_node->mtime     = st.st_mtime;
                }

                d_file_tree_internal_dir_add_child(tree->root,
                                                   child_node);
                tree->size++;
            }
        }

        entry = d_readdir(dir);
    }

    d_closedir(dir);

    return tree;
}


/*
d_file_tree_new_copy
  Creates a deep copy of an existing file tree.

Parameter(s):
  _other: the file tree to copy; must not be NULL.
Return:
  A pointer to the newly allocated copy, or NULL on failure.
*/
struct d_file_tree*
d_file_tree_new_copy
(
    const struct d_file_tree* _other
)
{
    struct d_file_tree* copy;

    if (!_other)
    {
        return NULL;
    }

    copy = (struct d_file_tree*)calloc(1, sizeof(struct d_file_tree));
    if (!copy)
    {
        return NULL;
    }

    copy->separator = _other->separator;

    if (_other->root)
    {
        copy->root = d_file_tree_internal_copy_node(_other->root);
        if (!copy->root)
        {
            free(copy);

            return NULL;
        }
    }

    copy->size = _other->size;

    return copy;
}


/*
d_file_tree_free
  Frees a file tree and all of its nodes.

Parameter(s):
  _tree: the file tree to free; may be NULL.
Return:
  none.
*/
void
d_file_tree_free
(
    struct d_file_tree* _tree
)
{
    if (!_tree)
    {
        return;
    }

    if (_tree->root)
    {
        d_file_tree_internal_node_free_recursive(_tree->root);
    }

    free(_tree);

    return;
}


/*******************************************************************************
* IV.   Navigation
*******************************************************************************/

/*
d_file_tree_get_root
  Returns the root node of the file tree.

Parameter(s):
  _tree: the file tree; must not be NULL.
Return:
  A pointer to the root node, or NULL if the tree is NULL.
*/
struct d_file_tree_node*
d_file_tree_get_root
(
    const struct d_file_tree* _tree
)
{
    if (!_tree)
    {
        return NULL;
    }

    return _tree->root;
}


/*
d_file_tree_get_node
  Resolves a path string to its corresponding node in the tree.

Parameter(s):
  _tree: the file tree to search.
  _path: the path to resolve (e.g. "src/core/main.c").
Return:
  A pointer to the node at the given path, or NULL if not found.
*/
struct d_file_tree_node*
d_file_tree_get_node
(
    const struct d_file_tree* _tree,
    const char*               _path
)
{
    if ( (!_tree) ||
         (!_path) )
    {
        return NULL;
    }

    return d_file_tree_internal_resolve_path(_tree, _path);
}


/*
d_file_tree_get_parent
  Returns the parent of the given node.

Parameter(s):
  _node: the node whose parent to retrieve.
Return:
  A pointer to the parent node, or NULL if the node is the root or NULL.
*/
struct d_file_tree_node*
d_file_tree_get_parent
(
    const struct d_file_tree_node* _node
)
{
    if (!_node)
    {
        return NULL;
    }

    return _node->parent;
}


/*
d_file_tree_get_child
  Returns the child at the given index from a directory node.

Parameter(s):
  _node:  the directory node; must not be NULL.
  _index: the zero-based index of the child.
Return:
  A pointer to the child node, or NULL if out of bounds or not a
directory.
*/
struct d_file_tree_node*
d_file_tree_get_child
(
    const struct d_file_tree_node* _node,
    size_t                         _index
)
{
    if ( (!_node) ||
         (_node->type != D_FILE_NODE_TYPE_DIR) ||
         (_index >= _node->count) )
    {
        return NULL;
    }

    return _node->children[_index];
}


/*
d_file_tree_get_child_by_name
  Searches the immediate children of a directory node for one matching
the given name.

Parameter(s):
  _node: the directory node to search; must not be NULL.
  _name: the name to match; must not be NULL.
Return:
  A pointer to the matching child, or NULL if not found.
*/
struct d_file_tree_node*
d_file_tree_get_child_by_name
(
    const struct d_file_tree_node* _node,
    const char*                    _name
)
{
    size_t i;

    if ( (!_node) ||
         (!_name) ||
         (_node->type != D_FILE_NODE_TYPE_DIR) )
    {
        return NULL;
    }

    for (i = 0; i < _node->count; i++)
    {
        if ( (_node->children[i])       &&
             (_node->children[i]->name) &&
             (d_string_equals_cstr(_node->children[i]->name, _name)) )
        {
            return _node->children[i];
        }
    }

    return NULL;
}


/*******************************************************************************
* V.    Modification
*******************************************************************************/

/*
d_file_tree_add_file
  Adds a file node at the given path. All intermediate directories must
already exist.

Parameter(s):
  _tree: the file tree.
  _path: the path for the new file (e.g. "src/main.c").
Return:
  0 on success, -1 on failure (missing parent, duplicate, etc.).
*/
int
d_file_tree_add_file
(
    struct d_file_tree* _tree,
    const char*         _path
)
{
    char*                    parent_path;
    char*                    child_name;
    struct d_file_tree_node* parent_node;
    struct d_file_tree_node* new_node;

    if ( (!_tree) ||
         (!_path) )
    {
        return -1;
    }

    // split path into parent + child
    if (d_file_tree_internal_split_parent_child(_path,
                                                _tree->separator,
                                                &parent_path,
                                                &child_name) != 0)
    {
        return -1;
    }

    // resolve parent directory
    parent_node = d_file_tree_internal_resolve_path(_tree, parent_path);
    if ( (!parent_node) ||
         (parent_node->type != D_FILE_NODE_TYPE_DIR) )
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    // check for duplicate
    if (d_file_tree_get_child_by_name(parent_node, child_name))
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    new_node = d_file_tree_node_new_file(child_name);
    if (!new_node)
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    if (d_file_tree_internal_dir_add_child(parent_node, new_node) != 0)
    {
        d_file_tree_node_free(new_node);
        free(parent_path);
        free(child_name);

        return -1;
    }

    _tree->size++;
    free(parent_path);
    free(child_name);

    return 0;
}


/*
d_file_tree_add_dir
  Adds a directory node at the given path. The immediate parent directory
must already exist.

Parameter(s):
  _tree: the file tree.
  _path: the path for the new directory (e.g. "src/core").
Return:
  0 on success, -1 on failure.
*/
int
d_file_tree_add_dir
(
    struct d_file_tree* _tree,
    const char*         _path
)
{
    char*                    parent_path;
    char*                    child_name;
    struct d_file_tree_node* parent_node;
    struct d_file_tree_node* new_node;

    if ( (!_tree) ||
         (!_path) )
    {
        return -1;
    }

    if (d_file_tree_internal_split_parent_child(_path,
                                                _tree->separator,
                                                &parent_path,
                                                &child_name) != 0)
    {
        return -1;
    }

    parent_node = d_file_tree_internal_resolve_path(_tree, parent_path);
    if ( (!parent_node) ||
         (parent_node->type != D_FILE_NODE_TYPE_DIR) )
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    // check for duplicate
    if (d_file_tree_get_child_by_name(parent_node, child_name))
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    new_node = d_file_tree_node_new_dir(child_name);
    if (!new_node)
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    if (d_file_tree_internal_dir_add_child(parent_node, new_node) != 0)
    {
        d_file_tree_node_free(new_node);
        free(parent_path);
        free(child_name);

        return -1;
    }

    _tree->size++;
    free(parent_path);
    free(child_name);

    return 0;
}


/*
d_file_tree_add_symlink
  Adds a symbolic link node at the given path.

Parameter(s):
  _tree:   the file tree.
  _path:   the path for the new symlink.
  _target: the target the symlink points to.
Return:
  0 on success, -1 on failure.
*/
int
d_file_tree_add_symlink
(
    struct d_file_tree* _tree,
    const char*         _path,
    const char*         _target
)
{
    char*                    parent_path;
    char*                    child_name;
    struct d_file_tree_node* parent_node;
    struct d_file_tree_node* new_node;

    if ( (!_tree)   ||
         (!_path)   ||
         (!_target) )
    {
        return -1;
    }

    if (d_file_tree_internal_split_parent_child(_path,
                                                _tree->separator,
                                                &parent_path,
                                                &child_name) != 0)
    {
        return -1;
    }

    parent_node = d_file_tree_internal_resolve_path(_tree, parent_path);
    if ( (!parent_node) ||
         (parent_node->type != D_FILE_NODE_TYPE_DIR) )
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    // check for duplicate
    if (d_file_tree_get_child_by_name(parent_node, child_name))
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    new_node = d_file_tree_node_new_symlink(child_name, _target);
    if (!new_node)
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    if (d_file_tree_internal_dir_add_child(parent_node, new_node) != 0)
    {
        d_file_tree_node_free(new_node);
        free(parent_path);
        free(child_name);

        return -1;
    }

    _tree->size++;
    free(parent_path);
    free(child_name);

    return 0;
}


/*
d_file_tree_mkdir_p
  Creates all intermediate directories along the given path, similar to
`mkdir -p`. Components that already exist (and are directories) are
silently skipped.

Parameter(s):
  _tree: the file tree.
  _path: the full directory path to create.
Return:
  0 on success, -1 on failure (a non-directory component blocks the path).
*/
int
d_file_tree_mkdir_p
(
    struct d_file_tree* _tree,
    const char*         _path
)
{
    char*                    buf;
    char*                    saveptr;
    char*                    token;
    char                     sep[2];
    struct d_file_tree_node* current;
    struct d_file_tree_node* child;
    struct d_file_tree_node* new_dir;

    if ( (!_tree) ||
         (!_path) )
    {
        return -1;
    }

    buf = d_strdup(_path);
    if (!buf)
    {
        return -1;
    }

    sep[0]  = _tree->separator;
    sep[1]  = '\0';
    current = _tree->root;
    saveptr = NULL;
    token   = d_strtok_r(buf, sep, &saveptr);

    while (token)
    {
        if (current->type != D_FILE_NODE_TYPE_DIR)
        {
            free(buf);

            return -1;
        }

        child = d_file_tree_get_child_by_name(current, token);

        if (child)
        {
            // component exists; must be a directory to continue
            if (child->type != D_FILE_NODE_TYPE_DIR)
            {
                free(buf);

                return -1;
            }

            current = child;
        }
        else
        {
            // create the missing directory
            new_dir = d_file_tree_node_new_dir(token);
            if (!new_dir)
            {
                free(buf);

                return -1;
            }

            if (d_file_tree_internal_dir_add_child(current,
                                                   new_dir) != 0)
            {
                d_file_tree_node_free(new_dir);
                free(buf);

                return -1;
            }

            _tree->size++;
            current = new_dir;
        }

        token = d_strtok_r(NULL, sep, &saveptr);
    }

    free(buf);

    return 0;
}


/*
d_file_tree_remove
  Removes the node at the given path and all of its descendants. The root
cannot be removed.

Parameter(s):
  _tree: the file tree.
  _path: the path of the node to remove.
Return:
  0 on success, -1 on failure.
*/
int
d_file_tree_remove
(
    struct d_file_tree* _tree,
    const char*         _path
)
{
    struct d_file_tree_node* node;
    size_t                   removed_count;

    if ( (!_tree) ||
         (!_path) )
    {
        return -1;
    }

    node = d_file_tree_internal_resolve_path(_tree, _path);
    if (!node)
    {
        return -1;
    }

    // cannot remove root
    if (node == _tree->root)
    {
        return -1;
    }

    removed_count = d_file_tree_internal_count_nodes(node);

    // detach from parent
    if (node->parent)
    {
        d_file_tree_internal_dir_remove_child(node->parent, node);
    }

    d_file_tree_internal_node_free_recursive(node);
    _tree->size -= removed_count;

    return 0;
}


/*
d_file_tree_move
  Moves a node from one path to another within the tree. The destination
parent must already exist and be a directory.

Parameter(s):
  _tree:     the file tree.
  _old_path: the current path of the node.
  _new_path: the desired new path.
Return:
  0 on success, -1 on failure.
*/
int
d_file_tree_move
(
    struct d_file_tree* _tree,
    const char*         _old_path,
    const char*         _new_path
)
{
    struct d_file_tree_node* node;
    struct d_file_tree_node* new_parent;
    char*                    parent_path;
    char*                    child_name;
    struct d_string*         new_name;

    if ( (!_tree)     ||
         (!_old_path) ||
         (!_new_path) )
    {
        return -1;
    }

    node = d_file_tree_internal_resolve_path(_tree, _old_path);
    if ( (!node) ||
         (node == _tree->root) )
    {
        return -1;
    }

    if (d_file_tree_internal_split_parent_child(_new_path,
                                                _tree->separator,
                                                &parent_path,
                                                &child_name) != 0)
    {
        return -1;
    }

    new_parent = d_file_tree_internal_resolve_path(_tree, parent_path);
    if ( (!new_parent) ||
         (new_parent->type != D_FILE_NODE_TYPE_DIR) )
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    // check for duplicate at destination
    if (d_file_tree_get_child_by_name(new_parent, child_name))
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    // rename the node
    new_name = d_string_new_from_cstr(child_name);
    if (!new_name)
    {
        free(parent_path);
        free(child_name);

        return -1;
    }

    // detach from old parent
    if (node->parent)
    {
        d_file_tree_internal_dir_remove_child(node->parent, node);
    }

    // update name
    d_string_free(node->name);
    node->name = new_name;

    // attach to new parent
    d_file_tree_internal_dir_add_child(new_parent, node);

    free(parent_path);
    free(child_name);

    return 0;
}


/*
d_file_tree_clear
  Removes all children from the root, leaving an empty tree with just
the root directory.

Parameter(s):
  _tree: the file tree; may be NULL.
Return:
  none.
*/
void
d_file_tree_clear
(
    struct d_file_tree* _tree
)
{
    size_t i;

    if ( (!_tree) ||
         (!_tree->root) )
    {
        return;
    }

    if (_tree->root->type == D_FILE_NODE_TYPE_DIR)
    {
        for (i = 0; i < _tree->root->count; i++)
        {
            d_file_tree_internal_node_free_recursive(
                _tree->root->children[i]);
            _tree->root->children[i] = NULL;
        }

        _tree->root->count = 0;
    }

    _tree->size = 1;

    return;
}


/*******************************************************************************
* VI.   Query
*******************************************************************************/

/*
d_file_tree_is_empty
  Returns true if the tree contains only the root node with no children.

Parameter(s):
  _tree: the file tree.
Return:
  true if the tree has no children under the root, false otherwise.
*/
bool
d_file_tree_is_empty
(
    const struct d_file_tree* _tree
)
{
    if (!_tree)
    {
        return true;
    }

    return (_tree->size <= 1);
}


/*
d_file_tree_size
  Returns the total number of nodes in the tree, including the root.

Parameter(s):
  _tree: the file tree.
Return:
  The total node count, or 0 if the tree is NULL.
*/
size_t
d_file_tree_size
(
    const struct d_file_tree* _tree
)
{
    if (!_tree)
    {
        return 0;
    }

    return _tree->size;
}


/*
d_file_tree_contains
  Returns true if a node exists at the given path.

Parameter(s):
  _tree: the file tree.
  _path: the path to check.
Return:
  true if the path resolves to a node, false otherwise.
*/
bool
d_file_tree_contains
(
    const struct d_file_tree* _tree,
    const char*               _path
)
{
    if ( (!_tree) ||
         (!_path) )
    {
        return false;
    }

    return (d_file_tree_internal_resolve_path(_tree, _path) != NULL);
}


/*
d_file_tree_is_file
  Returns true if the node at the given path is a regular file.

Parameter(s):
  _tree: the file tree.
  _path: the path to check.
Return:
  true if the node exists and is a file, false otherwise.
*/
bool
d_file_tree_is_file
(
    const struct d_file_tree* _tree,
    const char*               _path
)
{
    struct d_file_tree_node* node;

    if ( (!_tree) ||
         (!_path) )
    {
        return false;
    }

    node = d_file_tree_internal_resolve_path(_tree, _path);

    return (node && node->type == D_FILE_NODE_TYPE_FILE);
}


/*
d_file_tree_is_dir
  Returns true if the node at the given path is a directory.

Parameter(s):
  _tree: the file tree.
  _path: the path to check.
Return:
  true if the node exists and is a directory, false otherwise.
*/
bool
d_file_tree_is_dir
(
    const struct d_file_tree* _tree,
    const char*               _path
)
{
    struct d_file_tree_node* node;

    if ( (!_tree) ||
         (!_path) )
    {
        return false;
    }

    node = d_file_tree_internal_resolve_path(_tree, _path);

    return (node && node->type == D_FILE_NODE_TYPE_DIR);
}


/*
d_file_tree_child_count
  Returns the number of immediate children of the node at the given path.

Parameter(s):
  _tree: the file tree.
  _path: the path to the directory node.
Return:
  The number of children, or 0 if the path is invalid or not a directory.
*/
size_t
d_file_tree_child_count
(
    const struct d_file_tree* _tree,
    const char*               _path
)
{
    struct d_file_tree_node* node;

    if ( (!_tree) ||
         (!_path) )
    {
        return 0;
    }

    node = d_file_tree_internal_resolve_path(_tree, _path);
    if ( (!node) ||
         (node->type != D_FILE_NODE_TYPE_DIR) )
    {
        return 0;
    }

    return node->count;
}


/*
d_file_tree_depth
  Returns the depth of a node in the tree (root is depth 0).

Parameter(s):
  _node: the node whose depth to compute.
Return:
  The depth of the node, or 0 if the node is NULL.
*/
size_t
d_file_tree_depth
(
    const struct d_file_tree_node* _node
)
{
    size_t                         depth;
    const struct d_file_tree_node* current;

    if (!_node)
    {
        return 0;
    }

    depth   = 0;
    current = _node;

    while (current->parent)
    {
        depth++;
        current = current->parent;
    }

    return depth;
}


/*******************************************************************************
* VII.  Traversal
*******************************************************************************/

/*
d_file_tree_traverse_preorder
  Visits every node in the tree in pre-order (parent before children).

Parameter(s):
  _tree:    the file tree.
  _visitor: the callback to invoke on each node.
  _context: opaque pointer forwarded to the visitor.
Return:
  none.
*/
void
d_file_tree_traverse_preorder
(
    const struct d_file_tree* _tree,
    fn_file_tree_visitor      _visitor,
    void*                     _context
)
{
    if ( (!_tree) ||
         (!_visitor) )
    {
        return;
    }

    d_file_tree_internal_traverse_preorder(_tree->root,
                                           0,
                                           _visitor,
                                           _context);

    return;
}


/*
d_file_tree_traverse_postorder
  Visits every node in the tree in post-order (children before parent).

Parameter(s):
  _tree:    the file tree.
  _visitor: the callback to invoke on each node.
  _context: opaque pointer forwarded to the visitor.
Return:
  none.
*/
void
d_file_tree_traverse_postorder
(
    const struct d_file_tree* _tree,
    fn_file_tree_visitor      _visitor,
    void*                     _context
)
{
    if ( (!_tree) ||
         (!_visitor) )
    {
        return;
    }

    d_file_tree_internal_traverse_postorder(_tree->root,
                                            0,
                                            _visitor,
                                            _context);

    return;
}


/*
d_file_tree_traverse_breadth_first
  Visits every node in the tree in breadth-first (level) order using an
internal queue allocated on the heap.

Parameter(s):
  _tree:    the file tree.
  _visitor: the callback to invoke on each node.
  _context: opaque pointer forwarded to the visitor.
Return:
  none.
*/
void
d_file_tree_traverse_breadth_first
(
    const struct d_file_tree* _tree,
    fn_file_tree_visitor      _visitor,
    void*                     _context
)
{
    struct d_file_tree_node** queue;
    size_t*                   depths;
    size_t                    queue_capacity;
    size_t                    head;
    size_t                    tail;
    struct d_file_tree_node*  current;
    size_t                    current_depth;
    size_t                    i;
    struct d_file_tree_node** new_queue;
    size_t*                   new_depths;

    if ( (!_tree)       ||
         (!_tree->root) ||
         (!_visitor) )
    {
        return;
    }

    queue_capacity = 16;
    queue  = (struct d_file_tree_node**)malloc(
                 queue_capacity * sizeof(struct d_file_tree_node*));
    depths = (size_t*)malloc(queue_capacity * sizeof(size_t));

    if ( (!queue) ||
         (!depths) )
    {
        free(queue);
        free(depths);

        return;
    }

    head = 0;
    tail = 0;

    // enqueue root
    queue[tail]  = _tree->root;
    depths[tail] = 0;
    tail++;

    while (head < tail)
    {
        current       = queue[head];
        current_depth = depths[head];
        head++;

        _visitor(current, current_depth, _context);

        // enqueue children
        if (current->type == D_FILE_NODE_TYPE_DIR)
        {
            for (i = 0; i < current->count; i++)
            {
                // grow queue if necessary
                if (tail >= queue_capacity)
                {
                    queue_capacity *= 2;
                    new_queue = (struct d_file_tree_node**)realloc(
                                    queue,
                                    queue_capacity *
                                        sizeof(struct d_file_tree_node*));
                    new_depths = (size_t*)realloc(
                                     depths,
                                     queue_capacity * sizeof(size_t));

                    if ( (!new_queue) ||
                         (!new_depths) )
                    {
                        free(new_queue ? new_queue : queue);
                        free(new_depths ? new_depths : depths);

                        return;
                    }

                    queue  = new_queue;
                    depths = new_depths;
                }

                queue[tail]  = current->children[i];
                depths[tail] = current_depth + 1;
                tail++;
            }
        }
    }

    free(queue);
    free(depths);

    return;
}


/*******************************************************************************
* VIII. Utility
*******************************************************************************/

/*
d_file_tree_node_path
  Builds the full path string from the root to the given node by walking
up the parent chain.

Parameter(s):
  _tree: the file tree (used for the separator character).
  _node: the node whose path to construct.
Return:
  A newly allocated d_string containing the full path, or NULL on failure.
The caller is responsible for freeing the returned string.
*/
struct d_string*
d_file_tree_node_path
(
    const struct d_file_tree*      _tree,
    const struct d_file_tree_node* _node
)
{
    const struct d_file_tree_node** stack;
    size_t                          stack_size;
    size_t                          stack_capacity;
    const struct d_file_tree_node*  current;
    struct d_string*                result;
    char                            sep[2];
    size_t                          i;

    if ( (!_tree) ||
         (!_node) )
    {
        return NULL;
    }

    // collect ancestors into a stack
    stack_capacity = 16;
    stack_size     = 0;
    stack = (const struct d_file_tree_node**)malloc(
                stack_capacity * sizeof(const struct d_file_tree_node*));

    if (!stack)
    {
        return NULL;
    }

    current = _node;
    while (current)
    {
        if (stack_size >= stack_capacity)
        {
            stack_capacity *= 2;
            stack = (const struct d_file_tree_node**)realloc(
                        (void*)stack,
                        stack_capacity *
                            sizeof(const struct d_file_tree_node*));

            if (!stack)
            {
                return NULL;
            }
        }

        stack[stack_size] = current;
        stack_size++;
        current = current->parent;
    }

    // build path from root to node
    sep[0] = _tree->separator;
    sep[1] = '\0';

    result = d_string_new();
    if (!result)
    {
        free((void*)stack);

        return NULL;
    }

    // walk stack in reverse (root first), skipping the root itself
    for (i = stack_size; i > 0; i--)
    {
        if (stack[i - 1] == _tree->root)
        {
            continue;
        }

        if (!d_string_is_empty(result))
        {
            d_string_append_cstr(result, sep);
        }

        if (stack[i - 1]->name)
        {
            d_string_append(result, stack[i - 1]->name);
        }
    }

    free((void*)stack);

    return result;
}


/*
d_file_tree_node_name
  Returns the name of the given node.

Parameter(s):
  _node: the node whose name to retrieve.
Return:
  A pointer to the node's name string, or NULL if the node is NULL.
*/
const struct d_string*
d_file_tree_node_name
(
    const struct d_file_tree_node* _node
)
{
    if (!_node)
    {
        return NULL;
    }

    return _node->name;
}


/*
d_file_tree_print
  Prints an indented textual representation of the tree to the given
output stream.

Parameter(s):
  _tree:   the file tree to print.
  _stream: the output stream (e.g. stdout).
Return:
  none.
*/
void
d_file_tree_print
(
    const struct d_file_tree* _tree,
    FILE*                     _stream
)
{
    if ( (!_tree) ||
         (!_stream) )
    {
        return;
    }

    d_file_tree_internal_print_node(_tree->root,
                                    0,
                                    _stream);

    return;
}