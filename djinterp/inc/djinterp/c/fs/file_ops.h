/******************************************************************************
* djinterp [c]                                                      file_ops.h
*
* Whole-file operations -- removing, renaming, copying.
*   d_rename is atomic; d_copy_file is not, and cannot be. That asymmetry is
* the module's main hazard and is documented per function rather than assumed.
*
* path:      /inc/djinterp/c/fs/file_ops.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    REMOVAL
      -------
      1.  d_remove      (file or empty directory)
      2.  d_unlink      (file only)

II.   MOVEMENT
      --------
      1.  d_rename      (atomic within a filesystem)

III.  DUPLICATION
      -----------
      1.  d_copy_file   (native engine where available)
*/

#ifndef DJINTERP_FILE_OPS_
#define DJINTERP_FILE_OPS_ 1

// djinterp
#include "./file_common.h"
#include "../../config/c/fs/cfg_file_ops.h"


D_EXTERN_C_BEGIN


// I.    Removal
int d_remove(const char* _path);
int d_unlink(const char* _path);

// II.   Movement
int d_rename(const char* _old,
             const char* _new,
             int         _overwrite);

// III.  Duplication
int d_copy_file(const char* _src,
                const char* _dst);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_OPS_
