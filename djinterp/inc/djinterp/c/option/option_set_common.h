/*******************************************************************************
* djinterp [c]                                               option_set_common.h
*
* The option set: a flat, key-unique, declaration-ordered sequence of option
* cells over one value block. Tier 0 -- the shared core both faces compile.
*
* WHAT containers.tex SAYS IT IS. `option_set` is placed on the container axes
* as: flat structure, unordered, comparator, multiplicity 1. Each of those is a
* commitment this header keeps and can be asked about:
*   - FLAT means there is no descent. One array of cells, no children, so
*     addressability is an index and every traversal is a loop.
*   - MULTIPLICITY 1 means keys are UNIQUE. It is the set's central invariant,
*     it is checked on insert rather than assumed, and `d_option_set_is_valid`
*     is the predicate that says so.
*   - UNORDERED means membership does not depend on position, and
*     body-options.tex is stronger still: an option set is a SET, defined
*     entirely by K(O) and O[k]. Order is not part of the formal object at all.
*
*       CORRECTION, and it matters. An earlier draft of this header claimed
*     declaration order was "part of the answer" because the parity oracle
*     diffs line by line. That was wrong, and wrong in the expensive direction:
*     it would have made a permutation a parity failure when the .tex says the
*     two sets are EQUAL. Order is a property of the REPRESENTATION, not of the
*     set. The oracle's need for a stable line sequence is what `canon` is for
*     -- and `canon` is already named in the parity law,
*     `canon(lower(A (x) B))`, precisely so that representation choices cannot
*     leak into semantic ones. `d_option_set_canon` (section VII) sorts by key;
*     every relation below is order-insensitive.
*   - COMPARATOR means the ordering is available but not imposed;
*     `d_option_key_less` is it.
*
*   The C++ face currently participates in NO container axis -- no
* structure_category, no multiplicity_category, no comparator -- which is a
* recorded conformance defect. Placing the axes on the core rather than on the
* face fixes it for both languages at once, and is the reason this header
* names them at all.
*
* THE SET BORROWS. IT NEVER OWNS.
*   `struct d_option_set` is a VIEW: two pointers and four counts over storage
* the caller supplies. This is the same shape as `d_maybe` and `d_result` in
* the functional core, and it is deliberate on three counts.
*   First, goal 3: no hidden allocation; storage strategy is the caller's.
*   Second, goal 5: a fixed-buffer set works on every tier, including the ones
* with no allocator at all.
*   Third, and least obvious: it takes this subframework OFF the C container
* substrate's critical path. `d_string` and `d_vector` gate roughly 24,000
* lines elsewhere in the framework; they gate nothing here. An option set is a
* flat array of 24-byte cells over a byte block, and both can be automatic
* storage. The options pilot can therefore be finished before the substrate is
* written, which is not true of any other module of comparable size.
*
* CONSTRUCTION IS TWO DECLARATIONS AND A VIEW.
*     D_OPTION_SET_DECLARE(my_storage, 8, 64);   // 8 cells, 64 value bytes
*     struct my_storage        raw = D_OPTION_SET_STORAGE_INIT;
*     struct d_option_set      set = D_OPTION_SET_VIEW(raw);
*   The widths travel with the storage, so a view can never disagree with what
* it describes -- the same guarantee D_MAYBE_VIEW and D_RESULT_VIEW give, for
* the same reason.
*
* THE VALUE BLOCK IS PACKED BY THE SET, NOT BY THE CALLER.
*   `d_option_set_add` assigns each new option's `value_offset` from the
* running high-water mark, honouring the slot's alignment. A caller therefore
* never computes an offset, which is the only way the offsets-not-pointers
* decision stays true under editing.
*
* WHAT IS NOT HERE, AND WHY.
*   No expansion, no `::expanded_t`, no flattening, no uniqueness STATIC
* assert, no key_type uniformity assert. Those are the C++ face's
* construction-time machinery -- notation, in the framework's sense -- and
* what they enforce at translation time this header enforces at run time
* through `d_option_set_add` and `d_option_set_is_valid`. Same invariant,
* different enforcement tier, which goal 2 permits explicitly: enforcement may
* differ between the languages, behaviour may not.
*
*
* path:      /inc/djinterp/c/option/option_set_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_OPTION_OPTION_SET_COMMON_H
#define DJINTERP_C_OPTION_OPTION_SET_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "../functional/functional_common.h"
#include "./option_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN

///////////////////////////////////////////////////////////////////////////////
///             I.    THE SET                                               ///
///////////////////////////////////////////////////////////////////////////////

// d_option_set
//   struct: a borrowed view onto a flat option set.
//   `options` is the schema -- `count` cells, key-unique, in declaration
// order. `values` is the block those cells address by offset. The view owns
// neither; both must outlive it.
struct d_option_set
{
    struct d_option* options;         // the schema cells
    unsigned char*   values;          // the value block the cells address
    uint32_t         count;           // cells in use
    uint32_t         capacity;        // cells the schema array can hold
    uint32_t         value_used;      // bytes of the value block in use
    uint32_t         value_capacity;  // bytes the value block can hold
};

// D_OPTION_SET_DECLARE
//   macro: declares a storage type for a set of at most `n` options over
// `bytes` of value block. Use it at file or block scope:
//     D_OPTION_SET_DECLARE(window_storage, 8, 64);
//   The value block leads and is `uint64_t`-aligned so that every slot the set
// packs into it can meet its own alignment without the set having to over-
// allocate. Storing the widths as members is what lets D_OPTION_SET_VIEW read
// them back rather than being told them a second time.
#define D_OPTION_SET_DECLARE(name,                                          \
                             n,                                             \
                             bytes)                                         \
    struct name                                                             \
    {                                                                       \
        uint64_t        value_block[(((bytes) + 7u) / 8u) ? (((bytes) + 7u) / 8u) : 1u]; \
        struct d_option option_block[(n) ? (n) : 1u];                       \
    }

// D_OPTION_SET_STORAGE_INIT
//   macro: the zero initialiser for a D_OPTION_SET_DECLARE storage object.
// Spelled once so no call site invents a different one.
#define D_OPTION_SET_STORAGE_INIT   { { 0 }, { { 0, 0, 0, 0, 0, 0 } } }

// D_OPTION_SET_VIEW
//   macro: builds a view onto a D_OPTION_SET_DECLARE storage object. Every
// width is read from the storage's own members, so the view cannot disagree
// with what it describes.
#define D_OPTION_SET_VIEW(storage)                                          \
    d_option_set_view((storage).option_block,                               \
                      (unsigned char*)((storage).value_block),              \
                      (uint32_t)(sizeof((storage).option_block) /           \
                                 sizeof(struct d_option)),                  \
                      (uint32_t)sizeof((storage).value_block))

// I.     construction
struct d_option_set d_option_set_view(struct d_option* _options,
                                      unsigned char*   _values,
                                      uint32_t         _capacity,
                                      uint32_t         _value_capacity);
void     d_option_set_clear(struct d_option_set* _set);

// d_option_set_is_valid
//   function: the set's invariants, checked rather than trusted -- widths
// consistent, offsets in range, and (the one that matters) keys UNIQUE and of
// one key type. This is multiplicity 1 as a predicate.
bool     d_option_set_is_valid(const struct d_option_set* _set);


///////////////////////////////////////////////////////////////////////////////
///             II.   POPULATION                                            ///
///////////////////////////////////////////////////////////////////////////////

// d_option_set_add
//   function: append an option column and pack its slot into the value block.
// The offset is assigned here from the running high-water mark; the caller
// never computes one. A duplicate key is D_OPTION_STATUS_KEY_DUPLICATE (formal, the
// multiplicity invariant); running out of either block is D_OPTION_STATUS_CAPACITY or
// D_OPTION_STATUS_VALUE_CAPACITY (mechanical). On success the result carries the new
// option's index.
struct d_option_result d_option_set_add(struct d_option_set* _set,
                                        uint64_t             _key,
                                        d_type_info16        _key_type,
                                        d_type_info16        _value_type,
                                        uint32_t             _value_size,
                                        uint32_t             _value_align);

// d_option_set_add_unary
//   function: append a presence-only key -- the `unit` slot of the C++ face.
// Consumes no value bytes.
struct d_option_result d_option_set_add_unary(struct d_option_set* _set,
                                              uint64_t             _key,
                                              d_type_info16        _key_type);

// d_option_set_add_cell
//   function: append an already-formed cell, re-packing its slot. This is the
// path the C++ face's `lower()` takes: it hands over the cell it built at
// translation time and the set decides where the bytes sit, so a lowered
// option and a natively-added one differ in nothing.
struct d_option_result d_option_set_add_cell(struct d_option_set*   _set,
                                             const struct d_option* _option,
                                             const void*            _value);


///////////////////////////////////////////////////////////////////////////////
///             III.  QUERIES -- THE LOOKUP HALF                            ///
///////////////////////////////////////////////////////////////////////////////
//
//   "An option is shorthand for (lookup by key) + (set value in its column)."
// This section is the first half and section IV is the second, which is why
// they are adjacent and why nothing sits between them.

uint32_t d_option_set_size(const struct d_option_set* _set);
bool     d_option_set_empty(const struct d_option_set* _set);
bool     d_option_set_contains(const struct d_option_set* _set,
                               uint64_t                   _key);

// d_option_set_find
//   function: index of the option with `_key`, written through `_out_index`.
// Returns false and leaves `_out_index` untouched when the key is absent, so a
// miss cannot be mistaken for index 0. `_out_index` may be NULL when only
// presence is wanted, which makes this the one primitive `contains` is built
// on rather than a second scan.
bool     d_option_set_find(const struct d_option_set* _set,
                           uint64_t                   _key,
                           uint32_t*                  _out_index);

// d_option_set_at / d_option_set_at_const
//   function: the cell at an index, or NULL when out of range.
struct d_option*       d_option_set_at(struct d_option_set* _set,
                                       uint32_t             _index);
const struct d_option* d_option_set_at_const(const struct d_option_set* _set,
                                             uint32_t                   _index);

// d_option_set_key_at
//   function: the key at an index, written through `_out_key`. The keys in
// order are what the parity oracle diffs, so this is the accessor the report
// projection uses.
bool     d_option_set_key_at(const struct d_option_set* _set,
                             uint32_t                   _index,
                             uint64_t*                  _out_key);

// d_option_set_key_type
//   function: the set's key type. Uniform across the set by invariant; zero
// for an empty set.
d_type_info16 d_option_set_key_type(const struct d_option_set* _set);


///////////////////////////////////////////////////////////////////////////////
///             IV.   ACCESS -- THE COLUMN HALF                             ///
///////////////////////////////////////////////////////////////////////////////

// d_option_set_get
//   function: copy the value at `_key` out. D_OPTION_STATUS_KEY_ABSENT when there is
// no such key, D_OPTION_STATUS_NO_VALUE when the key is unary, and
// D_OPTION_STATUS_VALUE_TYPE_MISMATCH when `_out_size` disagrees with the slot --
// three distinct formal failures, because collapsing them is what makes a
// misconfigured set look like an empty one.
struct d_option_result d_option_set_get(const struct d_option_set* _set,
                                        uint64_t                   _key,
                                        void*                      _out,
                                        size_t                     _out_size);

// d_option_set_set
//   function: assign the value at `_key` and raise D_OPTION_FLAG_ASSIGNED.
struct d_option_result d_option_set_set(struct d_option_set* _set,
                                        uint64_t             _key,
                                        const void*          _value,
                                        size_t               _value_size);

// d_option_set_is_assigned
//   function: whether the key's slot has been written since initialisation.
// "Is this set pristine?" is a fold of this over the set -- one loop over a
// flat table, not a comparison against a second set.
bool     d_option_set_is_assigned(const struct d_option_set* _set,
                                  uint64_t                   _key);


///////////////////////////////////////////////////////////////////////////////
///             V.    THE RELATION ALGEBRA                                  ///
///////////////////////////////////////////////////////////////////////////////
//
//   Four relations from body-options.tex, and they are FOUR, not one with
// variations. Getting them confused is the defect this section exists to
// prevent, so each states its own quantifier.
//
//       equal      O1 = O2   <=>  K(O1) = K(O2)  and  forall k, O1[k] = O2[k]
//       agreement  O1 ~ O2   <=>  forall k in K(O1) ^ K(O2), O1[k] = O2[k]
//       conflict   O1 !~ O2  <=>  exists k in K(O1) ^ K(O2), O1[k] != O2[k]
//       key-equal            <=>  K(O1) = K(O2)                (keys only)
//
//   The quantifier is the whole difference. EQUALITY ranges over the union of
// the key sets and demands they coincide; AGREEMENT ranges only over the
// OVERLAP and says nothing about the rest. Two sets with disjoint keys agree
// vacuously and are not equal.
//
//   Every relation takes the projection pi, because the .tex specifies each of
// them in terms of option identity and identity is defined through the
// carrier. Pass NULL for pi = id.

// d_option_set_equal
//   function: O1 = O2 -- same key set, and identical options at every key.
bool     d_option_set_equal(const struct d_option_set* _lhs,
                            const struct d_option_set* _rhs,
                            fn_option_project          _project,
                            fn_binary_predicate        _compare,
                            void*                      _context);

// d_option_set_key_equal
//   function: K(O1) = K(O2), order-insensitive, values not consulted.
//   NOT one of the .tex's named relations -- it is the key-set half of
// equality, useful on its own and cheap. Named for what it compares so that it
// cannot be mistaken for equality or for agreement.
bool     d_option_set_key_equal(const struct d_option_set* _lhs,
                                const struct d_option_set* _rhs);

// d_option_set_agrees
//   function: O1 ~ O2 -- for every key occurring in BOTH, the options are
// identical. Disjoint sets agree vacuously.
//
//   AGREEMENT IS A TOLERANCE, NOT AN EQUIVALENCE. Reflexive and symmetric, and
// NOT TRANSITIVE. The .tex spends its longest passage and an entire table on
// this, and states the consequence in as many words: agreement "does not
// partition option sets into equivalence classes and must not be used to group
// or deduplicate them."
//
//   The counterexample, kept here because a maintainer who finds this function
// and reaches for it to deduplicate will not go and read the .tex first:
//
//       O1 = { (foo, 1) }
//       O2 = { (bar, 2) }
//       O3 = { (foo, 99) }
//
//       O1 ~ O2   (no shared key -- vacuous)
//       O2 ~ O3   (no shared key -- vacuous)
//       O1 !~ O3  (shared key foo, values differ)
//
//   Framework goal 1 requires a test for every stated non-property, so that a
// future maintainer cannot quietly "fix" a tolerance into an equivalence. The
// conformance suite must assert NON-transitivity on exactly the triple above.
// That test is not optional and it is not a formality: it is the single
// clearest instance in the corpus of the rule goal 1 was written for.
bool     d_option_set_agrees(const struct d_option_set* _lhs,
                             const struct d_option_set* _rhs,
                             fn_option_project          _project,
                             fn_binary_predicate        _compare,
                             void*                      _context);

// d_option_set_conflicts
//   function: O1 !~ O2 -- there is at least one shared key whose options are
// not identical. Exactly the negation of agreement, so the two are
// complementary and one is implemented as the other.
//
//   Note the vocabulary this repairs. `merge_mode` has always been described
// as "conflict-resolution vocabulary", but nothing in the framework could
// DETECT a conflict: a mode is a resolution policy, and the predicate the .tex
// names is a different thing that did not exist. This is it.
//
//   `_out_key` receives the first conflicting key, in the left set's order, or
// is left untouched when the sets agree. It may be NULL. A conflict a caller
// cannot locate is a diagnostic they cannot act on, which is why the predicate
// reports the witness rather than just the verdict.
bool     d_option_set_conflicts(const struct d_option_set* _lhs,
                                const struct d_option_set* _rhs,
                                fn_option_project          _project,
                                fn_binary_predicate        _compare,
                                void*                      _context,
                                uint64_t*                  _out_key);


///////////////////////////////////////////////////////////////////////////////
///             VI.   UNION AND INTERSECTION                                ///
///////////////////////////////////////////////////////////////////////////////

// d_option_set_intersection
//   function: O1 ^ O2 := { o in O1 | k(o) in K(O2) and O2[k(o)] = o }.
//
//   AN IDENTITY-FILTERED INTERSECTION, not a key intersection. A key present
// in both sets carrying DIFFERENT options belongs to neither. The distinction
// is not pedantic: `d_option_set_key_intersection` below is the weaker
// operation, the two differ on exactly the conflicting keys, and the C++ tree
// shipped the weaker one under a name that suggested the stronger.
struct d_option_result d_option_set_intersection(struct d_option_set*       _out,
                                                 const struct d_option_set* _lhs,
                                                 const struct d_option_set* _rhs,
                                                 fn_option_project          _project,
                                                 fn_binary_predicate        _compare,
                                                 void*                      _context);

// d_option_set_key_intersection
//   function: K(O1) ^ K(O2) -- the shared KEYS, values not consulted.
//   Weaker than the operation above and named so. It is what the definitions
// quantify over ("for every key occurring in both"), so it is genuinely
// wanted; it is simply not the .tex's `^`.
struct d_option_result d_option_set_key_intersection(const struct d_option_set* _lhs,
                                                     const struct d_option_set* _rhs,
                                                     uint64_t*                  _out,
                                                     uint32_t                   _capacity);

// d_option_set_union
//   function: O1 u O2 -- the plain union. PARTIAL.
//
//   "is a valid option set if, and only if, O1 ~ O2; otherwise a shared key
// would carry two distinct options, violating uniqueness." So this operation
// is UNDEFINED on conflicting sets, and returns D_OPTION_STATUS_CONFLICT -- a FORMAL
// status, because no amount of extra buffer makes a conflicting union exist.
// Conflating that with a mechanical failure is precisely the anti-pattern the
// agent guide names: reporting "these sets are in conflict" and "the output
// buffer is full" identically is a conformance bug that reads as correct
// behaviour.
//
//   The relation to precedence, which the .tex states and which nothing in the
// code previously expressed: `u` is the AGREEMENT-RESTRICTED TOTAL of `(+)`,
// and `(+)` extends it to conflicting sets by electing a winner. On agreeing
// sets the two coincide in either order. So a caller who wants a union and
// does not care which side wins should say so by calling `(+)`; one who calls
// `u` is asserting there is nothing to elect, and gets told when there is.
struct d_option_result d_option_set_union(struct d_option_set*       _out,
                                          const struct d_option_set* _lhs,
                                          const struct d_option_set* _rhs,
                                          fn_option_project          _project,
                                          fn_binary_predicate        _compare,
                                          void*                      _context);


///////////////////////////////////////////////////////////////////////////////
///             VII.  TRAVERSAL                                             ///
///////////////////////////////////////////////////////////////////////////////
//
//   A set is a flat sequence, so its Foldable instance is the tier 1 spine and
// not new machinery. Every projection in the framework -- printing a set,
// encoding it, diffing it, emitting the parity oracle's parseable row -- is
// this fold at a different algebra.

// fn_option_visit
//   function pointer: the per-cell step of a fold over a set. Returning false
// stops the walk, which is what gives quantifiers and early-exit searches the
// same shape as a full traversal.
// Note: `_context` may be NULL.
typedef bool (*fn_option_visit)(const struct d_option* _option,
                                const unsigned char*   _values,
                                uint32_t               _index,
                                void*                  _context);

// d_option_set_for_each
//   function: walk the set in declaration order. Returns the number of cells
// visited, which equals `count` unless a step stopped the walk.
uint32_t d_option_set_for_each(const struct d_option_set* _set,
                               fn_option_visit            _visit,
                               void*                      _context);

// d_option_set_fold
//   function: the catamorphism. `_step` is an ordinary `fn_accumulator` over
// cells, so any fold already written against the functional core works here
// unchanged.
bool     d_option_set_fold(const struct d_option_set* _set,
                           void*                      _accumulator,
                           fn_accumulator             _step,
                           void*                      _context);

// d_option_set_canon
//   function: reorder the set into CANONICAL form -- ascending by key, the
// value block repacked to match.
//
//   This is the `canon` of the parity law,
// `canon(lower(A (x)_cpp B)) == canon(lower(A) (x)_c lower(B))`, and it exists
// because an option set is a SET: two representations differing only in order
// denote the same object, so a differential test that compared representations
// directly would report a divergence the .tex says is not one. Canonicalising
// both sides removes the representation from the comparison entirely.
//
//   It is also what makes the parity oracle's parseable table diffable line by
// line, which is the practical reason to have it and the wrong reason to
// believe order is semantic.
//
//   In place, and it needs no scratch: the schema array is permuted and the
// value block repacked in one pass.
struct d_option_result d_option_set_canon(struct d_option_set* _set);

// d_option_set_is_canon
//   function: whether the set is already in canonical order. Cheap, and lets a
// caller skip the repack.
bool     d_option_set_is_canon(const struct d_option_set* _set);

// d_option_set_byte_size
//   function: the set's size on disk -- cells plus value block, no traversal.
// Goal 4 requires every type to report this; for a flat set it is arithmetic,
// which is the whole benefit of offsets over pointers.
size_t   d_option_set_byte_size(const struct d_option_set* _set);


///////////////////////////////////////////////////////////////////////////////
///             VIII. LAYOUT ASSERTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////
//
//   Written against `sizeof(void*)` rather than against a constant, because
// the set carries two pointers and the framework supports 32-bit targets. The
// four counts are fixed width and their offsets are exact.

D_STATIC_ASSERT(offsetof(struct d_option_set, values) == sizeof(void*),
                "d_option_set layout drift: values");
D_STATIC_ASSERT(offsetof(struct d_option_set, count) == (2u * sizeof(void*)),
                "d_option_set layout drift: count");
D_STATIC_ASSERT(offsetof(struct d_option_set, capacity) ==
                    ((2u * sizeof(void*)) + 4u),
                "d_option_set layout drift: capacity");
D_STATIC_ASSERT(offsetof(struct d_option_set, value_used) ==
                    ((2u * sizeof(void*)) + 8u),
                "d_option_set layout drift: value_used");
D_STATIC_ASSERT(offsetof(struct d_option_set, value_capacity) ==
                    ((2u * sizeof(void*)) + 12u),
                "d_option_set layout drift: value_capacity");
D_STATIC_ASSERT(sizeof(struct d_option_set) == ((2u * sizeof(void*)) + 16u),
                "d_option_set layout drift: trailing padding");


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_OPTION_OPTION_SET_COMMON_H
