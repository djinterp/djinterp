/*******************************************************************************
* djinterp [config]                                     cfg_container_iterator.h
*
*   Build-time configuration for the C++ container iterators
* (cpp/container/iterator). It defines no knobs yet.
*
*   targets:  cpp/container/iterator/container_iterator.hpp, which does not
*             include it yet
*   requires: cfg_common.h (user overrides); c/container/cfg_container.h
*             (the container family's filter gates)
*
*   RENAMED 2026.11.01 from container_iterator_config.h: every config header
* is cfg_<module>.h (dconfig_readme.md, 2.1).
*
*
* path:      /inc/djinterp/config/cpp/container/iterator/cfg_container_iterator.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.21
*                                                            revised: 2026.11.01
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CPP_CONTAINER_ITERATOR_CFG_CONTAINER_ITERATOR_H
#define DJINTERP_CONFIG_CPP_CONTAINER_ITERATOR_CFG_CONTAINER_ITERATOR_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_* helpers, user overrides
#include "../../../c/container/cfg_container.h"


#endif  // DJINTERP_CONFIG_CPP_CONTAINER_ITERATOR_CFG_CONTAINER_ITERATOR_H
