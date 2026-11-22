/*******************************************************************************
* djinterp [c][interp]                                               djin_desc.h
*
*   A djin as facts: its name, version and summary, its commands, its
* settings and what its own describe slot adds, given to a desc sink
* (desc/desc.h), so that any of desc's machines writes it -- JSON for
* `djinterp describe --format=json`, text, a log record, a test. A frame of
* the registry that no djin record names is described the same way, by its
* name and its settings.
*   A unit and a header of their own, above desc, as option_env_desc.h is:
* djin.h includes nothing of desc, and a program that never describes a
* djin links none of this.
*
* HOW TO USE
* ----------
*   Run a describer through a machine, as any of desc's describers is run:
*     struct dj_desc_json_state state;
*     struct dj_desc_sink       sink;
*     d_desc_json_init(&state, &os, D_DESC_JSON_PRETTY);
*     d_desc_json_shape(&state, D_DJIN_DESC_SHAPE);
*     d_desc_sink_init(&sink, d_desc_json_machine(), &state);
*     d_desc_run(&sink, d_djin_describer, djin);
*   The facts are, in order: "name", "version" and "summary" (identity and
* intent); "commands", a level keyed by each command's name holding its
* "summary"; "settings", a level holding each setting as
* d_option_env_describe gives it, with what it accepts; then whatever the
* djin's describe slot gives, at the top level.
*
* OPTIONS
* -------
*   None of its own. Which facts a reader sees is the sink's choice: its
* layers and importance, and its machine's settings (desc's djin).
*
* CONFIGURATION
* -------------
*   None of its own. It is built where the option core's 64-bit floor is
* (RE_STD_HAS_INT64), as djin.h and option_env_desc.h are.
*
*
* path:      /inc/djinterp/c/modules/interp/djin_desc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DESCRIBING A DJIN
    -----------------
    1.  D_DJIN_DESC_SHAPE
    2.  dj_djin_command_of
    3.  d_djin_describer
    4.  d_djin_frame_describer
    5.  d_djin_command_describer
*/

#ifndef DJINTERP_C_INTERP_DJIN_DESC_H
#define DJINTERP_C_INTERP_DJIN_DESC_H 1

// djinterp
#include "../../djinterp.h"           // framework root
#include "../../desc/desc.h"          // struct dj_desc_sink
#include "../../option/option_env.h"  // struct dj_option_env
#include "./djin.h"                   // struct dj_djin


#if RE_STD_HAS_INT64

D_EXTERN_C_BEGIN


//==============================================================================
// 1.  DESCRIBING A DJIN
//==============================================================================

// 1.1
// D_DJIN_DESC_SHAPE
//   constant: the shape `describe --format=json` names first, as "format":
// MAJOR moves at any change that is not an addition, MINOR at an addition.
#define D_DJIN_DESC_SHAPE  "djinterp.describe/1.0"

// 1.2
// dj_djin_command_of
//   struct: a command with the djin it is one of, for
// d_djin_command_describer.
struct dj_djin_command_of
{
    const struct dj_djin*         djin;
    const struct dj_djin_command* command;
};

/**
 * @brief A djin record's facts: name, version, summary, commands, settings
 *        and its describe slot's own, as a fact describer (desc.h).
 *
 * @param[in]     _object  the djin, a const struct dj_djin*.
 * @param[in,out] _sink    the sink.
 * @return 0 when every fact was given; 1 once the sink stopped; -1 for a
 *         null argument.
 */
int d_djin_describer(const void*          _object,
                     struct dj_desc_sink* _sink);

/**
 * @brief A frame of the registry that no djin names: its name, then its
 *        settings, as d_djin_describer gives a djin's.
 *
 * @param[in]     _object  the frame, a const struct dj_option_env*.
 * @param[in,out] _sink    the sink.
 * @return as d_djin_describer.
 */
int d_djin_frame_describer(const void*          _object,
                           struct dj_desc_sink* _sink);

/**
 * @brief One of a djin's commands: the djin's name, the command's name and
 *        its summary.
 *
 * @param[in]     _object  a const struct dj_djin_command_of*.
 * @param[in,out] _sink    the sink.
 * @return as d_djin_describer.
 */
int d_djin_command_describer(const void*          _object,
                             struct dj_desc_sink* _sink);

D_EXTERN_C_END


#endif  // RE_STD_HAS_INT64


#endif  // DJINTERP_C_INTERP_DJIN_DESC_H
