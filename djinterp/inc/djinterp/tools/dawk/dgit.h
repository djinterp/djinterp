/*******************************************************************************
* djinterp [tools]                                                        dgit.h
*
* SCRATCH STUB -- not part of the delivered tree.
*   dcheck.c includes this header and calls three functions from it, but
* neither djinterp.zip nor djinterp_7_.zip carries dgit.h or dgit.c (R-26,
* "ext_git as a local hook", is NOT STARTED in the dawk decision register).
* This stub reports "no history" so dcheck links; no sheet reads git facts.
*
*
* path:      /inc/djinterp/tools/dawk/dgit.h
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                                   revised: TBA
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DGIT_H
#define DJINTERP_TOOLS_DAWK_DGIT_H 1

// std
#include <stdbool.h>  // bool
#include <stdint.h>   // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree

struct d_git_history;

struct d_git_history* d_git_read(const char* _root);
bool                  d_git_build(const struct d_git_history* _history,
                                  struct d_node_tree*         _tree,
                                  uint32_t                    _file,
                                  const char*                 _relative);
void                  d_git_free(struct d_git_history* _history);


#endif  // DJINTERP_TOOLS_DAWK_DGIT_H
