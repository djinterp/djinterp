/*******************************************************************************
* djinterp [c]                                                        sequence.h
*
* Sequence operations over contiguous arrays, and the distinct and flat_map
* transducer stages.
*   An array here is three facts: a base pointer, an element count, and the byte
* stride between elements. Every operation works on caller-owned storage of that
* shape and none allocates: `d_sorted` is a heapsort, which needs neither a
* scratch buffer nor recursion, and `d_distinct` remembers what it has seen in a
* buffer the caller supplies.
*   An operation that writes into a destination array is told its capacity and
* returns how many elements it wrote, so a short destination truncates the
* result rather than being overrun. The exception is `d_reverse_into`, whose
* destination must hold every element.
*   Ordering is a strict `fn_binary_predicate`, read as "left strictly precedes
* right". `d_precedes_from_comparator` reads a three-way comparator that way,
* through a `d_comparator_binding`.
*   The two stages plug into transducer.h chains. Each borrows a caller-owned
* binding, and the distinct stage keeps its run state there, out of reach of
* `d_transducer_reset`: a chain driven twice needs its `d_distinct_state`
* re-initialised as well as the chain reset. `flat_map` is the one stage that
* can make a chain longer than its input, by up to `capacity` outputs per value.
*
*
* path:      /inc/djinterp/c/functional/sequence.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Expansion
         1.  fn_expander
         2.  d_flat_map_state
    2.  Distinctness
         1.  d_distinct_state
    3.  Ordering
         1.  d_comparator_binding
2.  ARRAY OPERATIONS
    ----------------
    1.  Reversal
    2.  Ordering
    3.  Zipping
    4.  Expansion and slicing
    5.  Distinctness
    6.  Searching
3.  TRANSDUCER STAGES
    -----------------
    1.  Flat-map binding
    2.  Stage factories
4.  LAYOUT ASSERTIONS
    -----------------
    1.  Binding layouts
*/

#ifndef DJINTERP_C_FUNCTIONAL_SEQUENCE_H
#define DJINTERP_C_FUNCTIONAL_SEQUENCE_H 1

// std
#include <stddef.h>  // size_t, offsetof
// djinterp
#include "../djinterp.h"          // framework root
#include "./functional_common.h"  // fn_predicate, fn_binary_predicate,
                                  // fn_function_comparator
#include "./reducer.h"            // fn_zipper
#include "./transducer.h"         // d_transducer_stage
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


//==============================================================================
// 1.  TYPES
//==============================================================================
// The expander callback and the three bindings the operations below take. None
// of them owns storage: a buffer a binding points at stays the caller's, and
// must outlive the binding.


// 1.1    Expansion
//------------------------------------------------------------------------------
// 1.1.1
// fn_expander
//   function pointer: expands one element into zero or more outputs.
// Writes its outputs end to end from `_out_array`, at the output stride given
// to `d_flat_map` or `d_flat_map_state_init` -- which it is not told, so it
// must already agree with it -- and returns how many it wrote. It has room for
// `_capacity` outputs and must not write more; a larger return is clamped to
// `_capacity`, which bounds what is read back but cannot undo an overrun.
// Note: `_context` may be NULL.
typedef size_t (*fn_expander)(const void* _element,
                              void*       _out_array,
                              size_t      _capacity,
                              void*       _context);

// 1.1.2
// d_flat_map_state
//   struct: the binding behind a flat_map stage.
// The stage expands each value into `scratch` and emits the outputs downstream
// from there, so no stage downstream of it may share its binding. Caller-owned;
// must outlive every stage built from it.
struct d_flat_map_state
{
    fn_expander expander;  // the one-to-many arrow
    void*       context;   // context for `expander`; may be NULL
    void*       scratch;   // caller-owned buffer of `capacity` outputs
    size_t      capacity;  // outputs `scratch` holds; the most per value
    size_t      out_size;  // stride between outputs
};

// 1.2    Distinctness
//------------------------------------------------------------------------------
// 1.2.1
// d_distinct_state
//   struct: a seen-buffer, recording the distinct elements met so far in
// caller-owned storage.
// `count`, `overflow` and the contents of `seen` are mutable run state that
// only `d_distinct_state_init` clears. Each test is a linear scan of the
// recorded elements. Once `seen` is full a novel element is still admitted, but
// counted in `overflow` instead of being recorded, so a later repeat of it is
// admitted too: a non-zero `overflow` means the output may hold duplicates.
struct d_distinct_state
{
    void*               seen;          // caller-owned, `capacity` slots
    size_t              capacity;      // slots in `seen`
    size_t              count;         // mutable: slots filled so far
    size_t              element_size;  // stride of slots and of elements
    fn_binary_predicate equals;        // the equality test
    void*               context;       // context for `equals`; may be NULL
    size_t              overflow;      // mutable: admitted but not recorded
};

// 1.3    Ordering
//------------------------------------------------------------------------------
// 1.3.1
// d_comparator_binding
//   struct: binds a three-way comparator to its context, so that it can be read
// as a strict ordering.
// Pass the whole struct as the `void*` context of `d_precedes_from_comparator`,
// which is itself an `fn_binary_predicate`. Caller-owned; must outlive every
// use of it.
struct d_comparator_binding
{
    fn_function_comparator compare;  // three-way comparison
    void*                  context;  // context for `compare`; may be NULL
};


//==============================================================================
// 2.  ARRAY OPERATIONS
//==============================================================================
// Eager operations over contiguous arrays. None allocates. A call whose
// parameters are unusable writes nothing and returns `false`, 0 or NULL --
// values an ordinary call can also return, so they do not by themselves show
// misuse.


// 2.1    Reversal
//------------------------------------------------------------------------------
/**
 * @brief Reverses an array in place, a byte at a time, so no element-sized
 *        temporary is needed.
 *
 * @param[in,out] _elements      the array; may be NULL only if `_count` is 0.
 * @param[in]     _count         the number of elements.
 * @param[in]     _element_size  the stride between elements; must be non-zero.
 * @return `true` if the array was reversed, a count below 2 trivially; `false`
 *         if the parameters were unusable, in which case it is untouched.
 */
bool     d_reverse(void*  _elements,
                   size_t _count,
                   size_t _element_size);
/**
 * @brief Writes an array's elements in reverse order into a separate
 *        destination, leaving the source untouched.
 *
 * @param[in]  _elements      the source; may be NULL only if `_count` is 0.
 * @param[in]  _count         the number of elements.
 * @param[in]  _element_size  the stride between elements; must be non-zero.
 * @param[out] _out_array     a destination of at least `_count` elements; may
 *                            be NULL only if `_count` is 0.
 * @pre `_out_array` does not overlap `_elements`.
 * @return `true` if the reversal was written; `false` if the parameters were
 *         unusable, in which case nothing was written.
 */
bool     d_reverse_into(const void* _elements,
                        size_t      _count,
                        size_t      _element_size,
                        void*       _out_array);

// 2.2    Ordering
//------------------------------------------------------------------------------
/**
 * @brief Sorts an array in place under a strict ordering, by heapsort:
 *        O(n log n) comparisons, no scratch buffer, and no recursion.
 *
 * @note Heapsort is not stable. Where stability matters, carry an index in each
 *       element and break ties on it.
 * @note An ordering that is not a strict weak ordering still leaves a
 *       permutation of the input, in an unspecified order.
 *
 * @param[in,out] _elements      the array; may be NULL only if `_count` is 0.
 * @param[in]     _count         the number of elements.
 * @param[in]     _element_size  the stride between elements; must be non-zero.
 * @param[in]     _precedes      the strict ordering, read as "left strictly
 *                               precedes right"; must not be NULL.
 * @param[in]     _context       context forwarded to `_precedes`; may be NULL.
 * @post On success no element strictly precedes the one before it.
 * @return `true` if the array was sorted, a count below 2 trivially; `false`
 *         if the parameters were unusable, in which case it is untouched.
 */
bool     d_sorted(void*               _elements,
                  size_t              _count,
                  size_t              _element_size,
                  fn_binary_predicate _precedes,
                  void*               _context);
/**
 * @brief Reports whether an array is in order under a strict ordering: no
 *        element strictly precedes the one before it.
 *
 * @param[in] _elements      the array; may be NULL only if `_count` is 0.
 * @param[in] _count         the number of elements.
 * @param[in] _element_size  the stride between elements; must be non-zero.
 * @param[in] _precedes      the strict ordering; must not be NULL.
 * @param[in] _context       context forwarded to `_precedes`; may be NULL.
 * @return `true` if the array is in order, which an empty or single-element
 *         array vacuously is; `false` if it is not, or if the parameters were
 *         unusable.
 */
bool     d_is_sorted(const void*         _elements,
                     size_t              _count,
                     size_t              _element_size,
                     fn_binary_predicate _precedes,
                     void*               _context);
/**
 * @brief Reads a three-way comparator as a strict ordering: `_left` precedes
 *        `_right` when their comparison is negative.
 *
 * @note This is an `fn_binary_predicate`, so it can be passed as the
 *       `_precedes` of `d_sorted` or `d_is_sorted`, with the binding as their
 *       `_context`.
 *
 * @param[in] _left                the left operand, forwarded to `compare`.
 * @param[in] _right               the right operand, forwarded to `compare`.
 * @param[in] _comparator_binding  a `struct d_comparator_binding`; may be NULL.
 * @return `true` if the comparison is negative; `false` if it is not, or if the
 *         binding is NULL or has no `compare`, which leaves every pair
 *         unordered.
 */
bool     d_precedes_from_comparator(const void* _left,
                                    const void* _right,
                                    void*       _comparator_binding);

// 2.3    Zipping
//------------------------------------------------------------------------------
/**
 * @brief Pairs two arrays elementwise, writing each pair as the left element's
 *        bytes followed immediately by the right element's.
 *
 * @note The destination stride is therefore `_left_size + _right_size`, with
 *       no padding between or after the two halves.
 *
 * @param[in]  _left        the first array; must not be NULL.
 * @param[in]  _left_size   the stride of `_left`; must be non-zero.
 * @param[in]  _right       the second array; must not be NULL.
 * @param[in]  _right_size  the stride of `_right`; must be non-zero.
 * @param[in]  _count       the number of pairs to form; each array holds at
 *                          least this many elements.
 * @param[out] _out_array   the destination; must not be NULL.
 * @param[in]  _capacity    the number of pairs `_out_array` can hold.
 * @pre `_out_array` overlaps neither `_left` nor `_right`.
 * @return the number of pairs written, the smaller of `_count` and
 *         `_capacity`; 0 if the parameters were unusable.
 */
size_t   d_zip(const void* _left,
               size_t      _left_size,
               const void* _right,
               size_t      _right_size,
               size_t      _count,
               void*       _out_array,
               size_t      _capacity);
/**
 * @brief Combines two arrays elementwise through a zipper: `d_zip` followed by
 *        a map, without materialising the pairs.
 *
 * @note Only the first `_count` or `_capacity` positions, whichever is fewer,
 *       are visited. A zipper returning `false` drops its position instead of
 *       ending the run, and the slot it was offered goes to the next output,
 *       so the result may be shorter than the positions visited.
 *
 * @param[in]  _left        the first array; must not be NULL.
 * @param[in]  _left_size   the stride of `_left`; must be non-zero.
 * @param[in]  _right       the second array; must not be NULL.
 * @param[in]  _right_size  the stride of `_right`; must be non-zero.
 * @param[in]  _count       the number of positions to combine; each array
 *                          holds at least this many elements.
 * @param[out] _out_array   the destination; must not be NULL.
 * @param[in]  _out_size    the stride of `_out_array`; must be non-zero.
 * @param[in]  _capacity    the number of outputs `_out_array` can hold.
 * @param[in]  _zipper      the combining function; must not be NULL.
 * @param[in]  _context     context forwarded to `_zipper`; may be NULL.
 * @return the number of outputs written; 0 if the parameters were unusable.
 */
size_t   d_zip_with(const void* _left,
                    size_t      _left_size,
                    const void* _right,
                    size_t      _right_size,
                    size_t      _count,
                    void*       _out_array,
                    size_t      _out_size,
                    size_t      _capacity,
                    fn_zipper   _zipper,
                    void*       _context);

// 2.4    Expansion and slicing
//------------------------------------------------------------------------------
/**
 * @brief Expands every element into zero or more outputs, concatenated in a
 *        destination.
 *
 * @note Each element expands straight into the destination and is offered
 *       only the capacity still free, so nothing is buffered twice and the
 *       last element to fit may be cut short. Once the destination is full,
 *       the remaining elements are not expanded.
 *
 * @param[in]  _elements      the source; may be NULL only if `_count` is 0.
 * @param[in]  _count         the number of source elements.
 * @param[in]  _element_size  the stride of the source; must be non-zero.
 * @param[in]  _expander      the one-to-many arrow; must not be NULL.
 * @param[in]  _context       context forwarded to `_expander`; may be NULL.
 * @param[out] _out_array     the destination; must not be NULL.
 * @param[in]  _out_size      the stride of the destination, which `_expander`
 *                            writes at; must be non-zero.
 * @param[in]  _capacity      the number of outputs `_out_array` can hold.
 * @return the number of outputs written, at most `_capacity`, since a count an
 *         expander reports beyond what it was offered is clamped; 0 if the
 *         parameters were unusable.
 */
size_t   d_flat_map(const void* _elements,
                    size_t      _count,
                    size_t      _element_size,
                    fn_expander _expander,
                    void*       _context,
                    void*       _out_array,
                    size_t      _out_size,
                    size_t      _capacity);
/**
 * @brief Copies the half-open index range [`_begin`, `_end`) into a
 *        destination.
 *
 * @note An `_end` past `_count` is clipped to it, and the copy is then clipped
 *       to `_capacity`; the return value shows both. A request that is empty,
 *       inverted, or begins at or past `_count` copies nothing, rather than
 *       being clamped in a way the caller cannot detect.
 *
 * @param[in]  _elements      the source; must not be NULL.
 * @param[in]  _count         the number of source elements.
 * @param[in]  _element_size  the stride; must be non-zero.
 * @param[in]  _begin         the first index copied.
 * @param[in]  _end           one past the last index copied.
 * @param[out] _out_array     the destination; must not be NULL.
 * @param[in]  _capacity      the number of elements `_out_array` can hold.
 * @pre `_out_array` does not overlap `_elements`.
 * @return the number of elements written; 0 for a request that copies nothing
 *         or whose parameters were unusable.
 */
size_t   d_slice(const void* _elements,
                 size_t      _count,
                 size_t      _element_size,
                 size_t      _begin,
                 size_t      _end,
                 void*       _out_array,
                 size_t      _capacity);

// 2.5    Distinctness
//------------------------------------------------------------------------------
/**
 * @brief Initialises a seen-buffer over caller-owned storage, empty and with
 *        no overflow.
 *
 * @param[out] _state         the state to initialise; must not be NULL.
 * @param[in]  _seen          a buffer of `_capacity` slots of `_element_size`
 *                            bytes; must not be NULL.
 * @param[in]  _capacity      the number of distinct elements that can be
 *                            recorded; must be non-zero.
 * @param[in]  _element_size  the stride of a slot, and of every array the state
 *                            is used with; must be non-zero.
 * @param[in]  _equals        the equality test, called as
 *                            `_equals(recorded, candidate, _context)`; must not
 *                            be NULL.
 * @param[in]  _context       context forwarded to `_equals`; may be NULL.
 * @post On success the state borrows `_seen`, which must outlive it.
 * @return `true` if the state was initialised; `false` if the parameters were
 *         unusable, in which case `_state` is untouched.
 */
bool     d_distinct_state_init(struct d_distinct_state* _state,
                               void*                    _seen,
                               size_t                   _capacity,
                               size_t                   _element_size,
                               fn_binary_predicate      _equals,
                               void*                    _context);
/**
 * @brief Copies the first occurrence of each distinct element into a
 *        destination, in source order.
 *
 * @note The state accumulates across calls, so a later call also drops
 *       elements an earlier one recorded; re-initialise it to start afresh.
 *       Elements are read and written at the state's `element_size`. Once the
 *       destination is full, the remaining elements are neither copied nor
 *       recorded.
 * @warning Once the state's buffer is full, a novel element is still copied but
 *          is counted in its `overflow` instead of being recorded, so a later
 *          repeat of it is copied too.
 *
 * @param[in]     _elements      the source; may be NULL only if `_count` is 0.
 * @param[in]     _count         the number of source elements.
 * @param[in,out] _state         a seen-buffer initialised by
 *                               `d_distinct_state_init`; must not be NULL.
 * @param[out]    _out_array     the destination; must not be NULL.
 * @param[in]     _out_capacity  the number of elements `_out_array` can hold.
 * @pre `_out_array` does not overlap `_elements`.
 * @return the number of elements written; 0 if the parameters were unusable.
 */
size_t   d_distinct(const void*              _elements,
                    size_t                   _count,
                    struct d_distinct_state* _state,
                    void*                    _out_array,
                    size_t                   _out_capacity);

// 2.6    Searching
//------------------------------------------------------------------------------
/**
 * @brief Finds the last element satisfying a predicate, searching from the end
 *        so the search stops at the first match met.
 *
 * @param[in] _elements      the array; may be NULL only if `_count` is 0.
 * @param[in] _count         the number of elements.
 * @param[in] _element_size  the stride; must be non-zero.
 * @param[in] _predicate     the test; must not be NULL.
 * @param[in] _context       context forwarded to `_predicate`; may be NULL.
 * @return a pointer into `_elements` to the last satisfying element; NULL if
 *         there is none, or if the parameters were unusable.
 */
void*    d_find_last(const void*  _elements,
                     size_t       _count,
                     size_t       _element_size,
                     fn_predicate _predicate,
                     void*        _context);
/**
 * @brief Reports the index of the first element satisfying a predicate.
 *
 * @note The index comes back through `_out_index` rather than as a sentinel,
 *       because every candidate sentinel is a legitimate index.
 *
 * @param[in]  _elements      the array; may be NULL only if `_count` is 0.
 * @param[in]  _count         the number of elements.
 * @param[in]  _element_size  the stride; must be non-zero.
 * @param[in]  _predicate     the test; must not be NULL.
 * @param[in]  _context       context forwarded to `_predicate`; may be NULL.
 * @param[out] _out_index     receives the index; must not be NULL.
 * @return `true` if a satisfying element was found; `false` if none was, or if
 *         the parameters were unusable, in which case `_out_index` is
 *         untouched.
 */
bool     d_index_of(const void*  _elements,
                    size_t       _count,
                    size_t       _element_size,
                    fn_predicate _predicate,
                    void*        _context,
                    size_t*      _out_index);
/**
 * @brief Reports the index of the last element satisfying a predicate,
 *        searching from the end.
 *
 * @param[in]  _elements      the array; may be NULL only if `_count` is 0.
 * @param[in]  _count         the number of elements.
 * @param[in]  _element_size  the stride; must be non-zero.
 * @param[in]  _predicate     the test; must not be NULL.
 * @param[in]  _context       context forwarded to `_predicate`; may be NULL.
 * @param[out] _out_index     receives the index; must not be NULL.
 * @return `true` if a satisfying element was found; `false` if none was, or if
 *         the parameters were unusable, in which case `_out_index` is
 *         untouched.
 */
bool     d_last_index_of(const void*  _elements,
                         size_t       _count,
                         size_t       _element_size,
                         fn_predicate _predicate,
                         void*        _context,
                         size_t*      _out_index);


//==============================================================================
// 3.  TRANSDUCER STAGES
//==============================================================================
// Stages for transducer.h chains. Each borrows a caller-owned binding, which
// must outlive every drive of a chain holding the stage.


// 3.1    Flat-map binding
//------------------------------------------------------------------------------
/**
 * @brief Initialises the binding behind a flat_map stage.
 *
 * @param[out] _state     the binding to initialise; must not be NULL.
 * @param[in]  _expander  the one-to-many arrow; must not be NULL.
 * @param[in]  _context   context forwarded to `_expander`; may be NULL.
 * @param[in]  _scratch   a buffer of `_capacity` outputs of `_out_size` bytes;
 *                        must not be NULL.
 * @param[in]  _capacity  the outputs `_scratch` holds, and so the most a value
 *                        expands into; must be non-zero.
 * @param[in]  _out_size  the stride between outputs, which `_expander` writes
 *                        at; must be non-zero.
 * @post On success the binding borrows `_scratch`, which must outlive it.
 * @return `true` if the binding was initialised; `false` if the parameters were
 *         unusable, in which case `_state` is untouched.
 */
bool     d_flat_map_state_init(struct d_flat_map_state* _state,
                               fn_expander              _expander,
                               void*                    _context,
                               void*                    _scratch,
                               size_t                   _capacity,
                               size_t                   _out_size);

// 3.2    Stage factories
//------------------------------------------------------------------------------
/**
 * @brief Builds a stage that passes a value on only the first time it is seen.
 *
 * @note The stage keeps its run state in `_state`, where `d_transducer_reset`
 *       does not reach it: a chain driven again needs `_state` re-initialised
 *       as well as the chain reset. The seen-buffer's overflow rule applies as
 *       it does to `d_distinct`. A NULL or unusable state makes a stage that
 *       drops every value.
 *
 * @param[in] _state  a seen-buffer initialised by `d_distinct_state_init`.
 * @pre `_state` outlives every drive of a chain holding the stage.
 * @return the stage, by value.
 */
struct d_transducer_stage d_transducer_distinct(
                              struct d_distinct_state* _state);
/**
 * @brief Builds a stage that expands each value into zero or more outputs and
 *        passes them on in turn.
 *
 * @note This is the one stage that can make a chain longer than its input. A
 *       count the expander reports beyond the binding's `capacity` is clamped
 *       to it, and emission stops early once a downstream stage latches the
 *       reducing state. A NULL or unusable binding makes a stage that drops
 *       every value.
 *
 * @param[in] _state  a binding initialised by `d_flat_map_state_init`.
 * @pre `_state` outlives every drive of a chain holding the stage.
 * @return the stage, by value.
 */
struct d_transducer_stage d_transducer_flat_map(
                              struct d_flat_map_state* _state);


//==============================================================================
// 4.  LAYOUT ASSERTIONS
//==============================================================================
// The Layout law: one declaration of each type, asserted in both of the
// languages that compile this header.


// 4.1    Binding layouts
//------------------------------------------------------------------------------
D_STATIC_ASSERT(offsetof(struct d_flat_map_state, context) ==
                    sizeof(fn_expander),
                "d_flat_map_state layout drift");
D_STATIC_ASSERT(offsetof(struct d_distinct_state, capacity) == sizeof(void*),
                "d_distinct_state layout drift");
D_STATIC_ASSERT(offsetof(struct d_comparator_binding, context) ==
                    sizeof(fn_function_comparator),
                "d_comparator_binding layout drift");


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FUNCTIONAL_SEQUENCE_H
