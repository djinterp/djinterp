/*******************************************************************************
* djinterp [c][parse]                                                 settings.h
*
* Parse's settings group: the djin `parse`, whose rows are the settings an
* option of 0 stands for (lane E's knob-and-option pattern, knob_and_option.md
* section 2; the design's section 11).
*   Six rows, flat, each defaulting to its knob:
*     expr_max_depth    1 to 65535, or `unbounded`  D_CFG_PARSE_EXPR_MAX_DEPTH
*     expr_max_errors   1 to 65535                  D_CFG_PARSE_EXPR_MAX_ERRORS
*     expr_error_order  leftmost or structure_first D_CFG_PARSE_EXPR_ERROR_ORDER
*     diag_text         on or off                   D_CFG_PARSE_DIAG_TEXT
*     diag_facts        on or off                   D_CFG_PARSE_DIAG_FACTS
*     log               on or off                   D_CFG_PARSE_LOG
* so `--parse.expr_max_depth=unbounded`, DJINTERP_PARSE_EXPR_MAX_ERRORS=1 and
* DJINTERP_PARSE_DIAG_TEXT=off work once d_parse_register has run. It
* installs the group's readers into expression core v2
* (d_parse_expr_settings_from), which reads the first three rows through
* their caches as a parse starts, and into the diagnostics
* (d_parse_diag_settings_from), which read the last three as a sink is made
* and as parse's log unit is attached; until then, and in a build without
* the group (D_CFG_PARSE_SETTINGS 0), each means its knob.
*   The last three choose among what a build has: diag_text on in a build
* without text (D_CFG_PARSE_DIAG_TEXT 0) gives none, and the sink says so
* (d_parse_diag_sink_carry, d_parse_diag_sink_stream). A record reaches the
* log only where log is on and the log wants its code: log.level,
* log.parse_expr.level and its grade's level decide the rest, so log off
* saves even gathering its facts.
*   The group is registered, and its values bound, before the program starts
* its threads: a parse reads the rows, and nothing in it writes them.
*
*
* path:      /inc/djinterp/c/modules/parse/settings.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
*            /docs/djinterp/option/knob_and_option.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.23
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
    D_PARSE_SETTING_DIAG_TEXT        = 3,
    D_PARSE_SETTING_DIAG_FACTS       = 4,
    D_PARSE_SETTING_LOG              = 5,
    D_PARSE_SETTINGS                 = 6
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Registering
//------------------------------------------------------------------------------
/**
 * @brief Registers the djin `parse` in a registry, and installs its readers
 *        into expression core v2, so that an option of 0 reads its row, and
 *        into the diagnostics, so that their switches read theirs.
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
 *        v2 and the diagnostics back on their knobs.
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
