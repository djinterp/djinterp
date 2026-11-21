/*******************************************************************************
* djinterp [cpp]                                                   db_common.hpp
*
* The namespace of djinterp's database layers: db, inside djinterp, with one
* namespace per vendor inside it -- db::sqlite, db::postgres, db::mariadb and
* the rest -- and the faces in db::face.
*   The C++17 layer reaches the two macros below through database.hpp and
* database_async.hpp; the faces, which gen_db_face.py writes, spell their
* namespaces out. C++98 or later; it declares nothing else.
*
* HOW TO USE
* ----------
*     NS_DJINTERP
*     NS_DB
*     namespace sqlite {
*     ...
*     }  // namespace sqlite
*     NS_END  // db
*     NS_END  // djinterp
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/modules/db/db_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NAMESPACE
    ---------
    1.  Keywords
         1.  D_KEYWORD_DB
         2.  NS_DB
*/

#ifndef DJINTERP_DB_COMMON_HPP
#define DJINTERP_DB_COMMON_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root


//==============================================================================
// 1.  NAMESPACE
//==============================================================================


// 1.1    Keywords
//------------------------------------------------------------------------------
// 1.1.1
// D_KEYWORD_DB
//   keyword: resolves to `db`, for identifiers, macros, and namespaces of the
// database layers. Guarded, so the core may adopt it.
#ifndef D_KEYWORD_DB
    #define D_KEYWORD_DB db
#endif  // D_KEYWORD_DB

// 1.1.2
// NS_DB
//   namespace: opens the `db` namespace, the root of the database layers.
// Nest it inside NS_DJINTERP and close it with NS_END; a vendor's namespace
// nests inside it.
#ifndef NS_DB
    #define NS_DB D_NAMESPACE(D_KEYWORD_DB)
#endif  // NS_DB


#endif  // DJINTERP_DB_COMMON_HPP
