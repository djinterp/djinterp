/*******************************************************************************
* djinterp [c][desc]                                              desc_options.h
*
*   desc's djin, "desc": the variables a run sets to choose among what the
* knobs built. `ring` (name, line, sentence, detailed) says how much a
* description says, and `layers` (identity, type, state, intent, policy,
* health, provenance, contents, cost; names joined by '|', and '+' or '-' to
* edit the value in force) what it is about. A type declared to desc
* (d_desc_declare) is a scope of the djin, so desc.file_tree.ring sets its
* ring alone. `style` (units, facts), `form` (facts, text), `format` (text,
* json) and `text` (report, line) say how describe writes it
* (desc_machines.h), each per type as well. Defaults come from the knobs:
* D_CFG_DESC_RING, _LAYERS, _STYLE, _FORM, _FORMAT and _TEXT_PRESET; a
* choice the knobs did not build is refused by name.
*
*   The djin is static and ready before the program runs. A program
* registers it to read settings into it with the registry's calls:
*     d_desc_register(NULL);                       // the program's registry
*     d_option_env_registry_import(d_option_env_registry(), "DJINTERP_");
* With D_CFG_DESC_OPTIONS 0, or below the option core's 64-bit floor, there
* is no djin, and the knobs decide alone.
*
*
* path:      /inc/djinterp/c/desc/desc_options.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE DJIN
    --------
    1.  D_DESC_OPTIONS_BUILT
    2.  D_DESC_VAR_*
    3.  d_desc_djin
    4.  d_desc_register
*/

#ifndef DJINTERP_C_DESC_OPTIONS_H
#define DJINTERP_C_DESC_OPTIONS_H 1

// djinterp
#include "./desc_units.h"                                // the engine
#include "../option/option_env.h"                        // frames
#include "../option/option_env_registry.h"               // the registry


//==============================================================================
// 1.  THE DJIN
//==============================================================================

// 1.1
// D_DESC_OPTIONS_BUILT
//   constant: 1 when this build has desc's djin.
#if D_CFG_DESC_ENABLED && D_CFG_DESC_OPTIONS && RE_STD_HAS_INT64
    #define D_DESC_OPTIONS_BUILT 1
#else
    #define D_DESC_OPTIONS_BUILT 0
#endif

#if D_DESC_OPTIONS_BUILT

D_EXTERN_C_BEGIN

// 1.2
// D_DESC_VAR_*
//   constants: the djin's variables, by their place among its declarations;
// D_OPTION_ENV_ID(d_desc_djin()->depth, place) is a variable's id.
#define D_DESC_VAR_RING    0u
#define D_DESC_VAR_LAYERS  1u
#define D_DESC_VAR_STYLE   2u
#define D_DESC_VAR_FORM    3u
#define D_DESC_VAR_FORMAT  4u
#define D_DESC_VAR_TEXT    5u
#define D_DESC_VAR_JSON_INTEGERS   6u
#define D_DESC_VAR_JSON_NONFINITE  7u
#define D_DESC_VARS        8u

/**
 * @brief desc's djin: its frame, static, ready before the program runs.
 *
 * @return the frame; never null.
 */
struct dj_option_env* d_desc_djin(void);

/**
 * @brief Registers desc's djin, so the registry's settings reach it.
 *
 * @param[in,out] _registry  the registry; null for the program's.
 * @return as d_option_env_registry_add.
 */
struct dj_option_result d_desc_register(
                            struct dj_option_env_registry* _registry);

D_EXTERN_C_END

#endif  // D_DESC_OPTIONS_BUILT


#endif  // DJINTERP_C_DESC_OPTIONS_H
