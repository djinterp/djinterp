/******************************************************************************
* djinterp [utility]                                  document_render_algebra.c
*
*   The helpers of document_render_algebra.h.  Two of them exist to make rules
* checkable rather than merely written down: d_doc_render_status_is_formal
* separates a violation of ch-documents.tex from an exhausted sink, and
* d_doc_render_algebra_reads exposes the `reads(phi)` set that the hint
* tolerance law is stated over.
*
*
* path:      /src/djinterp/c/util/document/document_render_algebra.c
* link(s):   TBA
* author(s): Agent B (structure)                           created: 2026.07.31
******************************************************************************/

#include "./document_render_algebra.h"


/*
d_doc_render_algebra_is_valid
  Whether an algebra can be folded with at all.

Parameter(s):
  _algebra: the dialect; may be NULL.
Return:
  true when the algebra exists, supplies the one primitive, and has its
reserved padding zeroed.
*/
int32_t
d_doc_render_algebra_is_valid(
    const struct d_doc_render_algebra* _algebra
)
{
    if (!_algebra)
    {
        return false;
    }

    // reserved is checked because a non-zero pad means the struct was built
    // by something that did not know its layout -- a wire-format bug caught
    // one call earlier than it would otherwise be
    return ( (_algebra->write_line != NULL) &&
             (_algebra->reserved == 0u) );
}

/*
d_doc_render_algebra_reads
  Whether a dialect declares that it consults a hint key.

Parameter(s):
  _algebra: the dialect.
  _key:     the hint key.
Return:
  true when the key is in the declared reads set.  false when the algebra
declares no set at all, since "unknown" is not "reads everything" -- a caller
testing the tolerance law must be able to tell an undeclared dialect from an
omnivorous one.
*/
int32_t
d_doc_render_algebra_reads(
    const struct d_doc_render_algebra* _algebra,
    const char*                        _key
)
{
    uint32_t index;

    if ( (!_algebra)         ||
         (!_algebra->reads)  ||
         (!_key) )
    {
        return false;
    }

    for (index = 0u; index < _algebra->read_count; ++index)
    {
        if (d_doc_attr_key_compare(_algebra->reads[index], _key) == 0)
        {
            return true;
        }
    }

    return false;
}
