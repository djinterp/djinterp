/*******************************************************************************
* djinterp [c]                                                    table_common.c
*
*   The out-of-line half of table_common.h.  Every definition here is D_INLINE while its
* declaration in the header is not, so C11 6.7.4p7 makes each an EXTERNAL
* definition: one symbol, linkable from both languages, and free to be inlined
* within this translation unit.
*
*
* path:      /src/djinterp/c/container/table/table_common.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/

#include "../../../../../inc/djinterp/c/container/table/table_common.h"


/*
d_table_status_message
  Describes a status in a sentence, for a diagnostic a person will read.

Parameter(s):
  _status: the status to describe.
Return:
  A pointer to a static, null-terminated string; never null.
*/
const char*
d_table_status_message
(
    enum d_table_status _status
)
{
    switch (_status)
    {
        case D_TABLE_STATUS_OK:
            return "the operation succeeded.";
        case D_TABLE_STATUS_DOMAIN:
            return "the index lies outside I_T; it is undefined, not blank.";
        case D_TABLE_STATUS_SHAPE:
            return "the operation would leave the domain non-rectangular.";
        case D_TABLE_STATUS_INVALID_ARGUMENT:
            return "a required argument was null.";
        case D_TABLE_STATUS_CAPACITY:
            return "the buffer is full and the storage strategy cannot grow.";
        case D_TABLE_STATUS_NO_MEMORY:
            return "the storage strategy refused to supply memory.";
        case D_TABLE_STATUS_OVERFLOW:
            return "the extent or byte count is not representable.";
        default:
            return "unrecognised status.";
    }
}

/*
d_table_status_name
  Names a status in a short, stable form. Reporting only: a caller switches on
the enum, never on this text. The formal and the mechanical codes are named
distinctly so that a log does not blur a claim containers.tex makes with a fact
about a machine.

Parameter(s):
  _status: the status to name.
Return:
  A pointer to a static, null-terminated string; never null.
*/
const char*
d_table_status_name
(
    enum d_table_status _status
)
{
    switch (_status)
    {
        case D_TABLE_STATUS_OK:               return "ok";
        case D_TABLE_STATUS_DOMAIN:           return "domain";
        case D_TABLE_STATUS_SHAPE:            return "shape";
        case D_TABLE_STATUS_INVALID_ARGUMENT: return "invalid_argument";
        case D_TABLE_STATUS_CAPACITY:         return "capacity";
        case D_TABLE_STATUS_NO_MEMORY:        return "no_memory";
        case D_TABLE_STATUS_OVERFLOW:         return "overflow";
        default:                              return "unknown";
    }
}
