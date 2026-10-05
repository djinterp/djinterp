/*******************************************************************************
* djinterp [config]                                                 cfg_option.h
*
* Configuration knobs for the option subframework.
*
*
* WHAT IS AND IS NOT CONFIGURABLE HERE, AND WHY
* =============================================
*   THERE IS NO VALUE-TYPE KNOB, AND THERE SHOULD NOT BE ONE. It is the
* obvious thing to ask for and it is the one thing this design cannot have.
* option_common.h states the governing identity: an option IS the column
* descriptor, and the point is that ten options in one set have ten different
* value types over one block. A build-wide D_CFG_OPTION_VALUE_TYPE collapses
* the set into a homogeneous map -- a different container -- and deletes the
* `_value_size` and `_value_align` parameters d_option_set_add needs to do its
* job. The value's TYPE stays per-cell, in `value_type`, where the design put
* it.
*
*   WHAT IS CONFIGURABLE IS THE ADDRESSING WIDTH: how wide a key the cell
* carries, how large a value block it can point into, and how large one slot
* may be. Those are three separate questions with three separate answers, and
* they are the three knobs below.
*
*
* NARROWING THESE IS NOT FREE, IN A WAY THE FIELDS DO NOT SHOW
* ============================================================
*   Each knob names its own cost at its own definition. The one worth reading
* before any of them is that d_option's LAYOUT ASSERTIONS WEAKEN when a knob
* moves off its default. option_common.h asserts sizeof == 24 and six exact
* offsets today; under three knobs those become expressions, and an assertion
* that computes both sides from the same arithmetic checks nothing.
*
*   The resolution is in option_common.h section VIII: the exact assertions
* SURVIVE FOR THE DEFAULT BUILD, guarded on D_INTERNAL_OPTION_LAYOUT_IS_DEFAULT
* below, and a narrowed build gets a derived set that checks the relations
* still expressible -- the key leads, and the struct is exactly the sum of its
* members. That is weaker, and it is weaker only where the strong form has
* stopped being expressible.
*
*
* path:      /inc/djinterp/config/core/option/cfg_option.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.06
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_OPTION_CFG_OPTION_H
#define DJINTERP_CONFIG_CORE_OPTION_CFG_OPTION_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_* helpers, djinterp's switches to re_std
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // uint32_t, uint64_t



// I.     the knobs

// D_CFG_OPTION_KEY_TYPE
//   knob: type of the option key carrier. Must be an UNSIGNED integral type of
// 1, 2, 4 or 8 bytes. Default uint64_t -- what the field was before it was
// configurable.
//
//   NARROWING THIS NARROWS THE INTERNING NAMESPACE, which is the cost the
// field does not show. An integral or enumeration key stores its own value and
// needs only to fit; a class-type NTTP key has no integral value and is stored
// as an INTERNED HANDLE (D_OPTION_FLAG_INTERNED_KEY). At uint16_t a build gets
// 65,535 distinct class-type keys for its whole lifetime, which is generous
// for a configuration surface and wrong for anything content-derived.
//   Whether handles are session-local or content-derived is still open --
// AGENT_README §9 item 3 -- so a build that narrows this is betting on the
// answer to a question the framework has not settled.
#ifndef D_CFG_OPTION_KEY_TYPE
#   define D_CFG_OPTION_KEY_TYPE        uint64_t
#endif

// D_CFG_OPTION_OFFSET_TYPE
//   knob: type of d_option's value_offset, and of the set's value_used and
// value_capacity. UNSIGNED, 2 or 4 bytes. Default uint32_t.
//
//   THIS BOUNDS THE VALUE BLOCK, NOT A VALUE. uint16_t caps a set's whole
// block at 64 KiB and takes the cell from 24 bytes to 16, which is the reason
// to want it: a configuration surface with a hundred small options fits inside
// that bound comfortably, and the saving is per-cell.
#ifndef D_CFG_OPTION_OFFSET_TYPE
#   define D_CFG_OPTION_OFFSET_TYPE     uint32_t
#endif

// D_CFG_OPTION_SIZE_TYPE
//   knob: type of d_option's value_size. UNSIGNED, 1, 2 or 4 bytes. Default
// uint32_t.
//
//   THIS BOUNDS ONE SLOT, and it is separate from the offset knob because they
// bound different things: a build that wants a 64 KiB block does not thereby
// want 64 KiB slots, and a build that stores nothing wider than a pointer can
// have a one-byte width field without capping the block at 255 bytes.
//
//   MIXING WIDTHS REINTRODUCES PADDING. Narrowing the offset to uint16_t while
// the size stays uint32_t puts two bytes of alignment filler back between
// them, and the saving the narrowing was for is spent on padding. Section II's
// assertion is what catches that; it is the one assertion in the option module
// that earns its place purely from these knobs existing.
#ifndef D_CFG_OPTION_SIZE_TYPE
#   define D_CFG_OPTION_SIZE_TYPE       uint32_t
#endif


// II.    normalisation

// D_INTERNAL_OPTION_LAYOUT_IS_DEFAULT
//   macro: whether all three knobs sit at their defaults, which is the
// condition under which option_common.h's exact layout assertions still mean
// something.
//   THE TEST IS ON WIDTHS, NOT ON TOKENS. A build that sets
// D_CFG_OPTION_KEY_TYPE to `unsigned long long` has not changed the layout on
// any target where that is eight bytes, and a token comparison would report it
// as a narrowed build and silently drop the strong assertions it still
// deserves.
//
//   WHICH IS WHY THIS IS NOT USABLE IN #if, AND MUST NOT BECOME SO. `sizeof`
// is a compile-time operator and the preprocessor cannot evaluate it, so
// `#if D_INTERNAL_OPTION_LAYOUT_IS_DEFAULT` is a hard error -- "missing binary
// operator before token '('". The guard belongs INSIDE the static assertion,
// as `(!IS_DEFAULT) || (the strong claim)`, which is how option_common.h
// section VIII spells it. Rewriting this in terms the preprocessor can read
// would mean comparing tokens, and that is the imprecision the paragraph above
// exists to reject.
#define D_INTERNAL_OPTION_LAYOUT_IS_DEFAULT                                    \
    ( (sizeof(D_CFG_OPTION_KEY_TYPE)    == 8u) &&                              \
      (sizeof(D_CFG_OPTION_OFFSET_TYPE) == 4u) &&                              \
      (sizeof(D_CFG_OPTION_SIZE_TYPE)   == 4u) )


#endif  // DJINTERP_CONFIG_CORE_OPTION_CFG_OPTION_H
