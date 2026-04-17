/******************************************************************************
* djinterp [container]                                             container.h
*
*   The module 
*
* 
* path:      \inc\container\container.h                           
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.31
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_
#define DJINTERP_C_CONTAINER_ 1

#include "..\djinterp.h"
#include "..\dmemory.h"


// DMergeConflictFlag
//   enum: 
enum DMergeConflictFlag
{
	D_MERGE_CONFLICT_FLAG_IGNORE    = 0x00,
	D_MERGE_CONFLICT_FLAG_OVERWRITE,
	D_MERGE_CONFLICT_FLAG_KEEP_BOTH
};


#endif	// DJINTERP_C_CONTAINER_