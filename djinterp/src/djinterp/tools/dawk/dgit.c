// SCRATCH STUB -- see dgit.h.  Reports no history.
#include "../../../../inc/djinterp/tools/dawk/dgit.h"  // corresponding header
// std
#include <stddef.h>  // NULL


struct dj_git_history* d_git_read(
    const char* _root
)
{
    (void)_root;

    return NULL;
}

bool d_git_build(
    const struct dj_git_history* _history,
    struct dj_node_tree*         _tree,
    uint32_t                     _file,
    const char*                  _relative
)
{
    (void)_history;
    (void)_tree;
    (void)_file;
    (void)_relative;

    return false;
}

void d_git_free(
    struct dj_git_history* _history
)
{
    (void)_history;

    return;
}
