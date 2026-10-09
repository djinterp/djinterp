/*******************************************************************************
* djinterp [config]                                                 cfg_env_qt.h
*
* Configuration for env_qt.h, the Qt detection header under env/ui/.
*   Resolves D_CFG_ENV_QT_PROBE_VERSION, which lets env_qt.h include Qt 6.5's
* small version header itself, so a C++ unit need not include a Qt header
* first.
*   targets:  env/ui/env_qt.h -> D_CFG_ENV_QT_PROBE_VERSION
*   requires: cfg_common.h (D_CFG_IS_BOOL)
*
*
* path:      /inc/djinterp/config/core/env/ui/cfg_env_qt.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_UI_CFG_ENV_QT_H
#define DJINTERP_CONFIG_CORE_ENV_UI_CFG_ENV_QT_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


// D_CFG_ENV_QT_PROBE_VERSION
//   configuration: 1, the default, has env_qt.h include
// <QtCore/qtversionchecks.h> in a C++ unit that has no QT_VERSION yet, where
// the header exists (Qt 6.5 and later), so Qt's version is found whatever
// the unit included first; 0 keeps detection from touching any Qt header, as
// D_CFG_ENV_CURL_PROBE_VERSION does for curl (decision 47 of the register).
#ifndef D_CFG_ENV_QT_PROBE_VERSION
    #define D_CFG_ENV_QT_PROBE_VERSION 1
#endif  // D_CFG_ENV_QT_PROBE_VERSION

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_QT_PROBE_VERSION)
    #error "D_CFG_ENV_QT_PROBE_VERSION must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_UI_CFG_ENV_QT_H
