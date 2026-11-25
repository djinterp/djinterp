/*******************************************************************************
* djinterp [cpp][desc]                                               desc_of.hpp
*
*   The C++ face of desc's describers of other modules' types: a described C
* type's units<T> and type_name<T>, made from its struct dj_desc_type, so
* desc's say, describe and facts take the object itself. Each module's
* describers have a header of their own here (mem_desc.hpp for memory),
* built where the C header's are. From C++98.
*
* HOW TO USE
* ----------
*   At namespace djinterp::desc, D_DESC_OF_UNITS(dj_arena); gives dj_arena
* its units and its name; describe(arena) then describes it.
*
* OPTIONS
* -------
*   desc's own, as for the C describers.
*
* CONFIGURATION
* -------------
*   D_CFG_DESC_OF and each module's D_CFG_DESC_OF_<MODULE> (cfg_desc_of.h).
*
*
* path:      /inc/djinterp/cpp/desc/of/desc_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  UNITS
    -----
    1.  D_DESC_OF_UNITS
*/

#ifndef DJINTERP_DESC_OF_HPP
#define DJINTERP_DESC_OF_HPP 1

// djinterp
#include "../desc_units.hpp"               // units, type_name
#include "../../../c/desc/of/desc_of.h"    // the layout row, the knobs


//==============================================================================
// 1.  UNITS
//==============================================================================

// 1.1
// D_DESC_OF_UNITS
//   macro: a described C type's units<T>, whose type() is its
// d_desc_type_<type_>, and its type_name<T>, its noun; at namespace
// djinterp::desc, followed by a semicolon.
#define D_DESC_OF_UNITS(type_)                                                 \
    template<>                                                                 \
    struct units< type_ >                                                      \
    {                                                                          \
        static const struct dj_desc_type& type()                               \
        {                                                                      \
            return d_desc_type_##type_;                                        \
        }                                                                      \
    };                                                                         \
    D_DESC_TYPE_NAME(type_, d_desc_type_##type_.noun)


#endif  // DJINTERP_DESC_OF_HPP
