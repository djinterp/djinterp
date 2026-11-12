/*******************************************************************************
* djinterp [c/parse]                                                  settings.h
*
* Parse's settings group: the djin `parse`, whose rows are the settings an
* option of 0 stands for (lane E's knob-and-option pattern, knob_and_option.md
* section 2; the design's section 11).
*   Three rows, flat, each defaulting to its knob:
*     expr_max_depth    1 to 65535, or `unbounded`  D_CFG_PARSE_EXPR_MAX_DEPTH
*     expr_max_errors   1 to 65535                  D_CFG_PARSE_EXPR_MAX_ERRORS
*     expr_error_order  leftmost or structure_first D_CFG_PARSE_EXPR_ERROR_ORDER
* so `--parse.expr_max_depth=unbounded` and DJINTERP_PARSE_EXPR_MAX_ERRORS=1
* work once d_parse_register has run. It installs the group's reader into
* expression core v2 (d_parse_expr_settings_from), which reads the rows
* through their caches as a parse starts; until then, and in a build without
* the group (D_CFG_PARSE_SETTINGS 0), an option of 0 means its knob.
*   The group is registered, and its values bound, before the program starts
* its threads: a parse reads the rows, and nothing in it writes them.
*
*
* path:      /inc/djinterp/c/modules/parse/settings.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
*            /docs/djinterp/option/knob_and_option.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  dj_parse_setting
2.  OPERATIONS
    ----------
    1.  Registering
*/

#ifndef DJINTERP_C_PARSE_SETTINGS_H
#define DJINTERP_C_PARSE_SETTINGS_H 1

// djinterp
#include "../../djinterp.h"                              // framework root
#include "../../../config/c/modules/parse/cfg_parse.h"   // D_INTERNAL_PARSE_SETTINGS
#include "../../option/option_env.h"                     // dj_option_env
#include "../../option/option_env_registry.h"            // dj_option_env_registry
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_PARSE_SETTINGS == 1)


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    dj_parse_setting
//------------------------------------------------------------------------------
// dj_parse_setting
//   enum: the group's rows, in its schema's order: a row's id in the djin is
// the first row's plus its place.
enum dj_parse_setting
{
    D_PARSE_SETTING_EXPR_MAX_DEPTH   = 0,
    D_PARSE_SETTING_EXPR_MAX_ERRORS  = 1,
    D_PARSE_SETTING_EXPR_ERROR_ORDER = 2,
    D_PARSE_SETTINGS                 = 3
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Registering
//------------------------------------------------------------------------------
/**
 * @brief Registers the djin `parse` in a registry, and installs its reader
 *        into expression core v2, so that an option of 0 reads its row.
 *
 * @param[in,out] _registry  the registry; NULL for the program's
 *                           (d_option_env_registry()).
 * @return the djin's place in the registry; KEY_DUPLICATE where a djin of
 *         the name is there already; the registry's failure otherwise, the
 *         reader then not installed.
 * @note Called before the program starts its threads. A djin is in one
 *       registry at a time: d_parse_unregister takes it off.
 */
struct dj_option_result d_parse_register(
                            struct dj_option_env_registry* _registry);

/**
 * @brief Takes the djin off the registry it is in, and puts expression core
 *        v2 back on its knobs.
 *
 * @return D_OPTION_STATUS_OK, or the registry's failure where the djin is in
 *         none.
 */
struct dj_option_result d_parse_unregister(void);

/**
 * @brief The djin's frame, for reading or binding a row by its id: the
 *        first row's id is d_parse_setting_id(D_PARSE_SETTING_EXPR_MAX_DEPTH).
 *
 * @return the frame; never NULL.
 */
struct dj_option_env* d_parse_djin(void);

/**
 * @brief A row's id in the djin as it stands.
 *
 * @param[in] _setting  the row, an enum dj_parse_setting.
 * @return the id, or D_OPTION_ENV_NONE for a row past the last.
 */
uint32_t d_parse_setting_id(int _setting);

D_EXTERN_C_END


#endif  // D_INTERNAL_PARSE_SETTINGS


#endif  // DJINTERP_C_PARSE_SETTINGS_H
