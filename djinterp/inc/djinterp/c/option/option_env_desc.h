/*******************************************************************************
* djinterp [c]                                                 option_env_desc.h
*
*   A frame's settings as facts: each variable a frame can see, its value in
* force, where that came from, its default, the knob behind the default, its
* help and the sources it refuses, written into a desc sink, so any machine
* -- text, JSON, a log record, a test -- reads them (option_env's decision
* 73). option_env keeps the provenance, so this is the one reading of it:
* describe, help and `get --show-origin` cannot disagree.
*   Each setting is a level of its own, keyed by its name, so a JSON reader
* finds "depth": {"value": 5, "source": "default", ...}. The facts sit on
* desc's layers -- the value on state, its source and knob on provenance, its
* help on intent, what it accepts and refuses on policy -- so a reader's
* layers and importance (desc.layers, a sink's filters) choose how much is
* shown. Flags add a setting's layers, its range or names, and each scope's
* settings in force.
*   A unit and a header of their own, above desc: the option core includes
* nothing of desc, and a program that never describes a frame links none of
* this.
*
*
* path:      /inc/djinterp/c/option/option_env_desc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DESCRIBING A FRAME
    ------------------
    1.  D_OPTION_ENV_DESC_*
    2.  d_option_env_describe
    3.  d_option_env_describer
*/

#ifndef DJINTERP_C_OPTION_ENV_DESC_H
#define DJINTERP_C_OPTION_ENV_DESC_H 1

// djinterp
#include "../djinterp.h"         // framework root
#include "../desc/desc.h"        // struct dj_desc_sink, the facts
#include "./option_env.h"        // struct dj_option_env


#if RE_STD_HAS_INT64

D_EXTERN_C_BEGIN


//==============================================================================
// 1.  DESCRIBING A FRAME
//==============================================================================

// 1.1
// D_OPTION_ENV_DESC_*
//   constants: what d_option_env_describe adds to each setting's facts,
// joined by '|'; 0 for the facts alone.
#define D_OPTION_ENV_DESC_LAYERS   0x1u  // every binding that bears on it,
                                         // in force first, the default last
#define D_OPTION_ENV_DESC_ACCEPTS  0x2u  // an int's range, a choice's or
                                         // flags' names, those not built
#define D_OPTION_ENV_DESC_SCOPES   0x4u  // each scope's settings in force,
                                         // under the scope's name
#define D_OPTION_ENV_DESC_ALL      0x7u

// 1.2
// D_OPTION_ENV_DESC_LAYERS_MAX
//   constant: the layers a setting lists at most, D_OPTION_ENV_DESC_LAYERS
// asked; the default's is the last kept.
#define D_OPTION_ENV_DESC_LAYERS_MAX  16u

/**
 * @brief Writes a frame's settings into a sink, a level a setting: name,
 *        kind, the value in force, its source, its default, the default's
 *        knob, its help, and the sources it refuses; flags add its layers,
 *        what it accepts, and each scope's settings.
 *
 * @note Every text is borrowed from the rows and the frame's arena, valid
 *       until a binding changes, so a collecting machine's facts are read
 *       before the frame is changed again.
 *
 * @param[in]     _env    the frame; its ancestors' variables are described
 *                        too.
 * @param[in,out] _sink   the sink.
 * @param[in]     _flags  D_OPTION_ENV_DESC_*, or 0.
 * @return 0 when every fact was given; 1 when the sink stopped; -1 for a
 *         null argument.
 */
int d_option_env_describe(const struct dj_option_env* _env,
                          struct dj_desc_sink*        _sink,
                          uint32_t                    _flags);

/**
 * @brief d_option_env_describe with no flags, as a fact describer: its
 *        object the frame, for d_desc_run and d_desc_write_with.
 *
 * @param[in]     _object  the frame, a const struct dj_option_env*.
 * @param[in,out] _sink    the sink.
 * @return as d_option_env_describe.
 */
int d_option_env_describer(const void*          _object,
                           struct dj_desc_sink* _sink);


D_EXTERN_C_END

#endif  // RE_STD_HAS_INT64


#endif  // DJINTERP_C_OPTION_ENV_DESC_H
