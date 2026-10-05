/*******************************************************************************
* djinterp [c]                                                   option_common.h
*
* The option cell: one key, one typed value slot. Tier 0 -- the shared core
* both faces compile.
*
* WHAT AN OPTION IS. `option_registry.hpp` states the governing identity, and
* it is the whole design:
*
*     AN OPTION IS SHORTHAND FOR (lookup by key) + (set value in its column).
*
* So an option is not a container of a value. It is the COLUMN DESCRIPTOR --
* the key, the value's declared type, and where that value sits in the set's
* value block. The set owns the column; the option addresses it. This is why
* `struct d_option` carries an OFFSET and not a pointer, and why a set of ten
* options is ten cells plus one value block rather than ten cells each
* dragging storage behind it.
*
* WHY AN OFFSET. Per the decision log: payload bytes use offsets, not
* pointers, which makes a record relocatable, `memcpy`-able and `fwrite`-able.
* A set can therefore be written to disk and read back at a different address
* without a fixup pass, and goal 4's runtime `sizeof` is arithmetic rather
* than a traversal.
*
* THE C++ FACE ADDS NO MEMBERS. `option<_Key, _Args...>` derives from this
* struct and contributes only `static constexpr` members and member
* functions, so `sizeof(option<...>) == sizeof(struct d_option)` and the base
* sits at offset 0 -- the Cost law, asserted rather than assumed. The args
* pack stays where it belongs: it is C++ NOTATION for describing a column, and
* the column it describes is this struct.
*
* TWO RELATIONS, NOT ONE. `kv_pair::operator==` in the C++ tree compares key
* only, which contradicts body-options.tex's definition of identity as key AND
* value. The reason it was never a one-line fix is that `operator<` is key-only
* and correctly so, so repairing `==` in place breaks strict-weak-ordering
* consistency. The core therefore names the two relations separately from the
* start -- `d_option_key_less` / `d_option_key_eq` for the ordering, and
* `d_option_eq` for identity -- and neither face gets to conflate them.
*
* EQUALITY TAKES ITS COMPARATOR AS AN ARGUMENT. Byte comparison is not value
* comparison: floats and padding both break it. Where the per-type equality
* hook lives -- a global registry or a passed context -- is an OPEN question
* (AGENT_README section 9, item 2), so this header takes the pure branch and
* passes the comparator in. That is not a decision about the open question; it
* is a refusal to encode one, and it costs a parameter.
*
* STATUS RANGES, NOT STATUS CODES. Formal failures (the operation is undefined
* on these arguments) and mechanical ones (the buffer is full) occupy disjoint
* numeric ranges, so a caller can ask "is this retryable?" by range rather than
* by enumerating cases. This is the `d_pack_status` shape from the archive
* work, kept because conflating the two kinds is a conformance bug that reads
* as correct behaviour.
*
* CONSTANTS, NOT ENUMS. An enum has implementation-defined size and C and C++
* may resolve one declaration differently, which is a goal 4 determinacy break
* at the root of the layout. Status is `int32_t`; flags are `uint32_t`.
*
*
* path:      /inc/djinterp/c/option/option_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_OPTION_OPTION_COMMON_H
#define DJINTERP_C_OPTION_OPTION_COMMON_H 1

// std
#include <stddef.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "../meta/type_info.h"
#include "../functional/functional_common.h"
#include "../functional/result.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t, uint32_t, uint64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN

///////////////////////////////////////////////////////////////////////////////
///             I.    STATUS                                                ///
///////////////////////////////////////////////////////////////////////////////

// d_option_status
//   enum: the result of any option operation.
//   Two kinds of failure, kept apart on purpose. A FORMAL failure says the
// operation is not defined -- there is no such key, these sets are in conflict.
// A MECHANICAL failure says the operation is defined and the machinery ran
// short -- the buffer was small, the schema array is full. Reporting them
// identically reads as correct behaviour and is not, so they occupy disjoint
// numeric ranges: formal below 0x100, mechanical at or above it. A caller that
// only wants to know "can I retry with more room?" tests the range, not the
// enumerator.
//
//   THE SPELLING FOLLOWS d_pack_status EXACTLY -- same range split, same floor
// constant, same two range macros, same accessor pair. That is deliberate:
// these are two instances of one discipline, and a reader who has learned one
// should not have to learn the other. An earlier draft of this header used
// `#define` constants instead and justified it by claiming the framework
// avoids enums for byte determinacy. That misread the rule. The rule is that
// no struct MEMBER may be an enum or a bool, because a member's size is
// implementation-defined and C and C++ may resolve one declaration
// differently. A status VOCABULARY is not a member; `d_pack_status` is an enum
// and stores as int32_t where it is carried, which is exactly what happens
// below.
enum d_option_status
{
    // -- success ------------------------------------------------------------
    D_OPTION_STATUS_OK                  = 0,

    // -- formal: the operation is not defined -------------------------------
    D_OPTION_STATUS_KEY_ABSENT          = 0x001,
    D_OPTION_STATUS_KEY_DUPLICATE       = 0x002,
    D_OPTION_STATUS_VALUE_TYPE_MISMATCH = 0x003,
    D_OPTION_STATUS_KEY_TYPE_MISMATCH   = 0x004,
    D_OPTION_STATUS_NO_VALUE            = 0x005,
    D_OPTION_STATUS_POLICY_REJECTED     = 0x006,
    D_OPTION_STATUS_CONFLICT            = 0x007,
    D_OPTION_STATUS_INVALID_ARGUMENT    = 0x008,

    // -- mechanical: the operation is defined, the machinery ran short ------
    D_OPTION_STATUS_BUFFER_TOO_SMALL    = 0x100,
    D_OPTION_STATUS_CAPACITY            = 0x101,
    D_OPTION_STATUS_VALUE_CAPACITY      = 0x102,
    D_OPTION_STATUS_NO_MEMORY           = 0x103
};

//   D_OPTION_STATUS_CONFLICT is the one worth pausing on. body-options.tex
// makes the plain union PARTIAL -- valid "if and only if O1 ~ O2" -- so a
// union over conflicting sets is UNDEFINED, not merely awkward. It is formal
// for that reason: no amount of extra buffer makes a conflicting union exist,
// and a mechanical status would tell a caller to retry, which cannot help.

// D_OPTION_STATUS_MECHANICAL_FLOOR
//   constant: the first mechanical status. A status at or above this value
// describes the machinery; below it, the request itself.
#define D_OPTION_STATUS_MECHANICAL_FLOOR    0x100

// D_OPTION_STATUS_IS_FORMAL
//   macro: 1 when _s reports that the operation is not defined.
#define D_OPTION_STATUS_IS_FORMAL(_s)                                       \
    ( ((_s) != D_OPTION_STATUS_OK) &&                                       \
      ((int)(_s) < D_OPTION_STATUS_MECHANICAL_FLOOR) )

// D_OPTION_STATUS_IS_MECHANICAL
//   macro: 1 when _s reports that the machinery ran short.
#define D_OPTION_STATUS_IS_MECHANICAL(_s)                                   \
    ( (int)(_s) >= D_OPTION_STATUS_MECHANICAL_FLOOR )

const char* d_option_status_name(enum d_option_status _status);
const char* d_option_status_message(enum d_option_status _status);

// d_option_result
//   struct: the `result` carrier at this subframework's instantiation --
// `result<uint32_t, int32_t>`. Fallible operations return it BY VALUE; it is
// eight bytes and costs nothing to pass.
//   Declared through D_RESULT_DECLARE so it is the framework's own carrier
// rather than a lookalike, and D_RESULT_VIEW lifts it into `struct d_result`
// for `map` / `bind` / `bimap` when a caller wants them. This closes the gap
// left open by the archive work, whose facade returns a bare status because
// `result` did not exist in C yet. It does now.
D_RESULT_DECLARE(d_option_result, uint32_t, int32_t);

// d_option_ok / d_option_fail
//   function: the two introductions of d_option_result.
struct d_option_result d_option_ok(uint32_t _value);
struct d_option_result d_option_fail(int32_t _status);

// d_option_result_status
//   function: the error arm, or D_OPTION_STATUS_OK when the result is a
// success.
//   NOT named `d_option_status`, which would collide with the enum. In C it
// would not: tag names and ordinary identifiers live in separate namespaces, so
// `enum d_option_status` and a function of that name coexist happily. In C++
// they do not -- an enum introduces its name into the ordinary namespace, the
// function would hide it, and every use of the type would then require the
// elaborated `enum` keyword to compile. That is a C/C++ divergence in a SHARED
// header, which is the one place the framework cannot tolerate one, so the
// collision is avoided rather than worked around.
int32_t  d_option_result_status(const struct d_option_result* _result);


///////////////////////////////////////////////////////////////////////////////
///             II.   FLAGS                                                 ///
///////////////////////////////////////////////////////////////////////////////

// D_OPTION_FLAG_NONE
//   constant: no flag set.
#define D_OPTION_FLAG_NONE          ((uint32_t)0u)

// D_OPTION_FLAG_UNARY
//   constant: the option carries no runtime value -- the `unit` slot of the
// C++ face, a presence-only key. `value_size` is 0 and reading the slot is
// D_OPTION_STATUS_NO_VALUE; ask `d_option_set_contains` instead.
#define D_OPTION_FLAG_UNARY         ((uint32_t)(1u << 0))

// D_OPTION_FLAG_ASSIGNED
//   constant: the slot holds a value written since the set was initialised,
// as opposed to the default it was seeded with. This is what makes "is this
// set pristine?" a read rather than a comparison against a second set.
#define D_OPTION_FLAG_ASSIGNED      ((uint32_t)(1u << 1))

// D_OPTION_FLAG_INTERNED_KEY
//   constant: `key` holds an INTERNED IDENTIFIER, not the key's own value.
//   Integral and enumeration keys store their value directly. A class-type
// NTTP key (a fixed_string, say) has no integral value to store, so it must be
// interned -- and whether interned handles are session-local or content-
// derived is OPEN (AGENT_README section 9, item 3). This flag does not settle
// that. It records WHICH of the two a given cell holds, so that when the
// question is settled the seam is already named and nothing has to be
// re-derived from context.
#define D_OPTION_FLAG_INTERNED_KEY  ((uint32_t)(1u << 2))

// D_OPTION_FLAG_MASK
//   constant: every flag this version defines. Bits outside the mask are
// reserved and must be written zero, so a later reader can tell an old cell
// from a corrupt one.
#define D_OPTION_FLAG_MASK          ((uint32_t)0x00000007u)


///////////////////////////////////////////////////////////////////////////////
///             III.  THE CELL                                              ///
///////////////////////////////////////////////////////////////////////////////

// d_option
//   struct: one option -- a key and the description of its value column.
//   Field order is chosen so the struct is 24 bytes with no padding on every
// supported target, which is why `key` leads and the two 16-bit descriptors
// are adjacent. The layout assertions in section VII are the check; this
// comment is not.
struct d_option
{
    uint64_t      key;           // the key's value, or its interned id
    uint32_t      value_offset;  // byte offset of the slot in the value block
    uint32_t      value_size;    // byte width of the slot; 0 when unary
    d_type_info16 key_type;      // the key's declared type
    d_type_info16 value_type;    // the value's declared type
    uint32_t      flags;         // D_OPTION_FLAG_*
};

// D_OPTION_INIT
//   macro: a brace initialiser for a cell, for static tables and for the C++
// face's `constexpr lower()`. Both faces build a cell exactly one way.
#define D_OPTION_INIT(key, key_type, value_type, offset, size, flags)       \
    { (uint64_t)(key), (uint32_t)(offset), (uint32_t)(size),                \
      (d_type_info16)(key_type), (d_type_info16)(value_type),               \
      (uint32_t)(flags) }

// I.     construction
struct d_option d_option_make(uint64_t      _key,
                              d_type_info16 _key_type,
                              d_type_info16 _value_type,
                              uint32_t      _value_offset,
                              uint32_t      _value_size,
                              uint32_t      _flags);
struct d_option d_option_make_unary(uint64_t      _key,
                                    d_type_info16 _key_type);
bool            d_option_is_valid(const struct d_option* _option);

// II.    inspection
bool     d_option_is_unary(const struct d_option* _option);
bool     d_option_is_assigned(const struct d_option* _option);
bool     d_option_has_interned_key(const struct d_option* _option);
uint32_t d_option_value_size(const struct d_option* _option);


///////////////////////////////////////////////////////////////////////////////
///             IV.   ORDERING -- KEY ONLY                                  ///
///////////////////////////////////////////////////////////////////////////////
//
//   The ordering relation is key-only, and correctly so: it is what makes a
// set of options sortable and searchable, and a value has no place in it. The
// C++ face's `operator<` routes here.

// d_option_key_less
//   function: three-way comparison on the KEY alone. Returns <0, 0, >0 in the
// usual `fn_comparator` sense, so it can be handed to any sort in the
// framework without an adapter.
int      d_option_key_less(const void* _lhs,
                           const void* _rhs,
                           void*       _context);

// d_option_key_eq
//   function: key equality alone -- the ordering's induced equivalence, and
// the relation `find` and `contains` are defined over.
bool     d_option_key_eq(const struct d_option* _lhs,
                         const struct d_option* _rhs);


///////////////////////////////////////////////////////////////////////////////
///             V.    THE CARRIER AND THE PROJECTION                        ///
///////////////////////////////////////////////////////////////////////////////
//
//   body-options.tex, Difference: fix a VALUE PROJECTION pi mapping an option
// to the carrier being compared,
//
//       pi(o) in { _|_ } union { values }
//
// where `_|_` denotes "no value of the requested kind". pi selects WHICH
// aspect of an option is compared -- its declared value, a default, or an
// effective value-or-default. Carrier comparison is structural, with
// `_|_ != v` for any value v and `_|_ = _|_`.
//
//   THIS IS NOT A COMPARATOR, AND THE DIFFERENCE IS THE POINT. An earlier
// draft of this header passed an `fn_binary_predicate` and called it done. A
// bare equality test can only compare an option's OWN value; it cannot express
// "compare the defaults" or "compare the effective value-or-default", which is
// exactly what the document says pi is for. The projection is the parameter
// the .tex specifies, so it is the parameter the core takes.
//
//   The C++ face already had this right: `option_set_compare.hpp`'s
// `{value_absent | value_present<V>}` interface with `carrier_eq` is this
// carrier, and leaving `_Extract` a required parameter with no default matches
// the .tex's insistence that pi is fixed by the caller. The C core now says
// the same thing in the same shape.

// d_option_carrier
//   struct: the result of a projection -- `_|_`, or a value.
//   `has_value` false IS `_|_`; the other two fields are then meaningless and
// must be ignored rather than tested. The carrier BORROWS: `data` points into
// storage the caller owns, and the carrier must not outlive it.
struct d_option_carrier
{
    const void* data;       // the projected bytes; meaningless when absent
    uint32_t    size;       // their width; meaningless when absent
    uint32_t    has_value;  // 0 is `_|_`. uint32_t, not bool: this crosses
};                          // a boundary and bool has no fixed width

// D_OPTION_CARRIER_ABSENT
//   macro: the `_|_` carrier. Spelled once so no call site invents a second
// spelling of "no value of the requested kind".
#define D_OPTION_CARRIER_ABSENT     { NULL, 0u, 0u }

// fn_option_project
//   function pointer: a value projection pi. Maps an option -- and the value
// block it addresses -- to the carrier being compared.
//   Passing NULL selects `d_option_project_value`, the identity projection,
// which is the pi = id of the document's "Relation to agreement and equality"
// paragraph. That is a documented default rather than a silent one: with
// pi = id, `changed` empty is exactly agreement, and the whole difference
// section reduces to comparison of the options themselves.
// Note: `_context` may be NULL.
typedef struct d_option_carrier (*fn_option_project)(
    const struct d_option* _option,
    const unsigned char*   _values,
    void*                  _context);

// d_option_project_value
//   projection: pi = id -- an option's own value, or `_|_` when it is unary.
// The default projection, and the one every relation in this subframework is
// specified against unless a caller says otherwise.
struct d_option_carrier d_option_project_value(const struct d_option* _option,
                                               const unsigned char*   _values,
                                               void*                  _context);

// d_option_carrier_eq
//   function: STRUCTURAL carrier comparison, exactly as the .tex defines it --
// `_|_ = _|_`, `_|_ != v` for every value v, and two values equal iff their
// bytes agree under `_compare`.
//
//   `_compare` is the per-type value comparator, and it is separate from the
// projection because they answer different questions: pi decides WHAT is
// compared, `_compare` decides HOW two of those things are compared. NULL
// selects byte comparison, which is right for the integral and enumeration
// slots that dominate real option sets and WRONG for floats and for any type
// with padding -- so it is a choice the caller makes explicitly. Where the
// per-type equality hook should live is an open framework decision, and this
// header will not settle it by accident.
bool     d_option_carrier_eq(const struct d_option_carrier* _lhs,
                             const struct d_option_carrier* _rhs,
                             fn_binary_predicate            _compare,
                             void*                          _context);


///////////////////////////////////////////////////////////////////////////////
///             VI.   IDENTITY -- KEY AND VALUE                             ///
///////////////////////////////////////////////////////////////////////////////
//
//   "Two options are IDENTICAL if, and only if, they have the same key and the
// same value." A DIFFERENT relation from section IV, and deliberately not
// spelled with a similar name, so that no future maintainer can simplify one
// into the other: they disagree on exactly the pairs that matter, and the C++
// tree already shipped that bug once -- `kv_pair::operator==` compares key
// only, which is the uniqueness predicate wearing identity's name.
//
//   ON SEVERAL DATA. The .tex adds: "If an option carries several data rather
// than a single value, identity is componentwise over all of them." A cell
// here has one slot, so a multi-datum option lowers to a slot holding all of
// its components, and componentwise identity is `_compare` over that slot.
//   The boundary consequence is worth stating, because it corrects a natural
// misreading of the C++ face: an arg that is a `field<T>` is DATA and is
// therefore inside the formal object; an arg that is a verifier or a
// description is notation and is not. The arg pack is not uniformly one or the
// other, and "args are opaque" is true of the SUBFRAMEWORK's treatment of them,
// not of their formal status.

// d_option_eq
//   function: identity -- equal keys AND equal carriers under pi. A unary
// option projects to `_|_`, so two unary options with one key are identical,
// and a unary option is never identical to a valued one.
//   Pass NULL for `_project` to get pi = id.
bool     d_option_eq(const struct d_option* _lhs,
                     const unsigned char*   _lhs_values,
                     const struct d_option* _rhs,
                     const unsigned char*   _rhs_values,
                     fn_option_project      _project,
                     fn_binary_predicate    _compare,
                     void*                  _context);

// d_option_value_eq
//   function: the carrier half alone, for callers that have already
// established key equality and do not want to pay for it twice.
bool     d_option_value_eq(const struct d_option* _lhs,
                           const unsigned char*   _lhs_values,
                           const struct d_option* _rhs,
                           const unsigned char*   _rhs_values,
                           fn_option_project      _project,
                           fn_binary_predicate    _compare,
                           void*                  _context);


///////////////////////////////////////////////////////////////////////////////
///             VII.  SLOT ACCESS                                           ///
///////////////////////////////////////////////////////////////////////////////
//
//   The "set value in its column" half of the governing identity. These take
// the value block rather than a set so that a caller holding a bare cell and
// its storage -- the C++ face lowering one option, say -- can use them without
// materialising a set.

// d_option_slot / d_option_slot_const
//   function: the address of the option's slot within `_values`. NULL for a
// unary option, which is the same answer as "there is nothing to read".
void*        d_option_slot(const struct d_option* _option,
                           unsigned char*         _values);
const void*  d_option_slot_const(const struct d_option* _option,
                                 const unsigned char*   _values);

// d_option_read
//   function: copy the slot out. `_out_size` must equal the slot width; a
// mismatch is D_OPTION_STATUS_VALUE_TYPE_MISMATCH rather than a truncated read.
struct d_option_result d_option_read(const struct d_option* _option,
                                     const unsigned char*   _values,
                                     void*                  _out,
                                     size_t                 _out_size);

// d_option_write
//   function: copy a value into the slot and raise D_OPTION_FLAG_ASSIGNED.
// Width is checked, not assumed.
struct d_option_result d_option_write(struct d_option* _option,
                                      unsigned char*   _values,
                                      const void*      _value,
                                      size_t           _value_size);


///////////////////////////////////////////////////////////////////////////////
///             VIII. LAYOUT ASSERTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////
//
//   The Layout law: one declaration, `sizeof` / `offsetof` / `alignof`
// asserted in BOTH dialects. This header is compiled by both, so these fire in
// both, and drift becomes a compile error rather than a wire-format bug.

D_STATIC_ASSERT(sizeof(struct d_option) == 24,
                "d_option layout drift: expected 24 bytes");
D_STATIC_ASSERT(offsetof(struct d_option, key) == 0,
                "d_option layout drift: key must lead");
D_STATIC_ASSERT(offsetof(struct d_option, value_offset) == 8,
                "d_option layout drift: value_offset");
D_STATIC_ASSERT(offsetof(struct d_option, value_size) == 12,
                "d_option layout drift: value_size");
D_STATIC_ASSERT(offsetof(struct d_option, key_type) == 16,
                "d_option layout drift: key_type");
D_STATIC_ASSERT(offsetof(struct d_option, value_type) == 18,
                "d_option layout drift: value_type");
D_STATIC_ASSERT(offsetof(struct d_option, flags) == 20,
                "d_option layout drift: flags");

D_STATIC_ASSERT(sizeof(struct d_option_carrier) ==
                    (sizeof(const void*) + 8u),
                "d_option_carrier layout drift");
D_STATIC_ASSERT(offsetof(struct d_option_carrier, size) == sizeof(const void*),
                "d_option_carrier layout drift: size");
D_STATIC_ASSERT(offsetof(struct d_option_carrier, has_value) ==
                    (sizeof(const void*) + 4u),
                "d_option_carrier layout drift: has_value");

//   The status ranges are disjoint, and stay disjoint. Asserted because the
// range test in section I is only meaningful while this holds.
D_STATIC_ASSERT(D_OPTION_STATUS_INVALID_ARGUMENT <
                    D_OPTION_STATUS_MECHANICAL_FLOOR,
                "d_option status: a formal status has crossed the floor");
D_STATIC_ASSERT(D_OPTION_STATUS_BUFFER_TOO_SMALL >=
                    D_OPTION_STATUS_MECHANICAL_FLOOR,
                "d_option status: a mechanical status is below the floor");


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_OPTION_OPTION_COMMON_H
