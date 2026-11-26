/*******************************************************************************
* djinterp [cpp][desc]                                          filesys_desc.hpp
*
*   The C++ face of desc's describers of filesys's types
* (desc/of/filesys_desc.h): units<T> and type_name<T> for a file's status and
* a directory entry at each width filesys builds, and for a filesys notice, so
* describe(status) is the status's description. Built where the C describers
* are (D_CFG_DESC_OF_FILESYS); from C++98.
*
* HOW TO USE
* ----------
*   djinterp::desc::describe(status) is a struct dj_stat_t's description at
* desc.ring; describe(notice, D_DESC_RING_SENTENCE) is a notice's sentence.
*
* OPTIONS
* -------
*   desc's own, as for the C describers.
*
* CONFIGURATION
* -------------
*   D_CFG_DESC_OF and D_CFG_DESC_OF_FILESYS (cfg_desc_of.h).
*
*
* path:      /inc/djinterp/cpp/desc/of/filesys_desc.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  UNITS
    -----
    1.  dj_stat64_t, dj_stat32_t, dj_dirent64_t, dj_dirent32_t,
        dj_file_notice
*/

#ifndef DJINTERP_DESC_OF_FILESYS_DESC_HPP
#define DJINTERP_DESC_OF_FILESYS_DESC_HPP 1

// djinterp
#include "./desc_of.hpp"                       // D_DESC_OF_UNITS
#include "../../../c/desc/of/filesys_desc.h" // the describers


#if D_INTERNAL_DESC_OF_FILESYS

NS_DJINTERP
D_NAMESPACE(desc)


//==============================================================================
// 1.  UNITS
//==============================================================================

// 1.1
// dj_stat64_t, dj_stat32_t, dj_dirent64_t, dj_dirent32_t, dj_file_notice
//   traits: each type's units and name, from its C description.
#if D_FILE_64_IS_AVAILABLE
D_DESC_OF_UNITS(dj_stat64_t);
D_DESC_OF_UNITS(dj_dirent64_t);
#endif
D_DESC_OF_UNITS(dj_stat32_t);
D_DESC_OF_UNITS(dj_dirent32_t);
D_DESC_OF_UNITS(dj_file_notice);


NS_END  // desc
NS_END  // djinterp

#endif  // D_INTERNAL_DESC_OF_FILESYS


#endif  // DJINTERP_DESC_OF_FILESYS_DESC_HPP
