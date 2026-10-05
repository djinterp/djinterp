/*******************************************************************************
* djinterp [c]                                            event_table_internal.h
*
* Private to the event core's sources: how the registry brackets a dispatch.
*   Dispatch folds the word in force when the occurrence began, so a letter a
* handler unbinds mid-dispatch must still be folded -- and its state must not
* be released before it is. While a table is dispatching, unbind therefore
* leaves the letter in its chain as a ZOMBIE: gone from every query and count,
* still linked, state intact. The outermost dispatch reaps the zombies when it
* ends, running their state_free hooks then. The markers live in bits the
* public header leaves unassigned (the high bits of the table's flags and of
* an entry's mask byte) and exist only while a dispatch is running.
*
* path:      /src/djinterp/c/event/event_table_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_EVENT_EVENT_TABLE_INTERNAL_H
#define DJINTERP_C_EVENT_EVENT_TABLE_INTERNAL_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../../../inc/djinterp/c/event/event_table_common.h"  // the table
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint32_t


// D_INTERNAL_EVENT_TABLE_FLAG_DISPATCHING
//   constant: set on a table while at least one dispatch over it is running.
#define D_INTERNAL_EVENT_TABLE_FLAG_DISPATCHING ((uint32_t)0x80000000u)

// D_INTERNAL_EVENT_TABLE_FLAG_ZOMBIES
//   constant: set when a dispatch left zombies for the outermost one to reap.
#define D_INTERNAL_EVENT_TABLE_FLAG_ZOMBIES     ((uint32_t)0x40000000u)

// D_INTERNAL_EVENT_TABLE_FLAG_DISPOSE
//   constant: set when a handler disposed the table it was dispatched from;
// the outermost dispatch disposes it on the way out.
#define D_INTERNAL_EVENT_TABLE_FLAG_DISPOSE     ((uint32_t)0x20000000u)

// D_INTERNAL_EVENT_ENTRY_ON
//   constant: the mask bit of an entry's `enabled` byte.
#define D_INTERNAL_EVENT_ENTRY_ON               ((uint8_t)0x01u)

// D_INTERNAL_EVENT_ENTRY_ZOMBIE
//   constant: marks an entry unbound during a dispatch and not yet reaped.
#define D_INTERNAL_EVENT_ENTRY_ZOMBIE           ((uint8_t)0x80u)

// d_internal_event_table_dispatch_begin
//   function: marks the table as dispatching; returns true when this is the
// outermost dispatch, the one that must end it.
bool d_internal_event_table_dispatch_begin(struct d_event_table* _table);

// d_internal_event_table_dispatch_end
//   function: when `_outermost`, clears the mark, reaps the zombies (running
// their state_free hooks) and performs a disposal a handler requested.
void d_internal_event_table_dispatch_end(struct d_event_table* _table,
                                         bool                  _outermost);


#endif  // DJINTERP_C_EVENT_EVENT_TABLE_INTERNAL_H
