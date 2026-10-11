/* re_std_order.c: Include Directives -- the // re_std group */
#include "./re_std_order.h"  // corresponding header
// std
#include <stddef.h>  // size_t
// re_std
#include "../../re_std/cstdint/dstdint.h"  // uint32_t
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#   include "./face.hpp"  // negative: a C++ face, inside a conditional
#endif
// djinterp
#include "../djinterp.h"  // D_INLINE
#include "../../re_std/cstddef/dstddef.h"  // positive: no // re_std above
#include "./other.h"      // positive: after re_std's


int d_value = 0;
