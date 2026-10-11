/* includes.c: an include of the root says what the file takes from it */
// djinterp
#include "../djinterp.h"         // framework root
#include "../../c/djinterp.h"    // D_INLINE, D_STATIC_ASSERT
#include "djinterp.h"
#include "./not_djinterp.h"      // framework root
#include "../djinterp.hpp"       // framework root
#include "../djinterp.h"         // framework root: and what it brings


int d_includes;
