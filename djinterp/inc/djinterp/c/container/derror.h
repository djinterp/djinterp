#define D_UINT_CAST(                                                           \
    _bits,                                                                     \
    _value)                                                                    \
    ( (_bits) <= 8  ? (uint8_t)(_value)  :                                     \
      (_bits) <= 16 ? (uint16_t)(_value) :                                     \
      (_bits) <= 32 ? (uint32_t)(_value) :                                     \
                      (uint64_t)(_value) )

#define D_BIT_R(n)    (1u << (n))
#define D_BIT_L(i, n) (D_UINT_CAST(1) << ((n) - 1 - (i)))

#define D_BYTE_BIT_L(i, n)                                                     \
    (D_UINT_CAST((n) * 8, 1) << (i))

#define D_BYTE_BIT_R(i, n)                                                     \
    (D_UINT_CAST((n) * 8, 1) << (((n) * 8) - 1 - (i)))

// =============================================================================

#define D_INTERVAL_A_PRECEDES_B(                                               \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( (_a_end) < (_b_start) )
	
#define D_INTERVAL_A_PRECEDED_BY_B(                                            \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( (_b_end) < (_a_start) )

#define D_INTERVAL_A_MEETS_B(                                                  \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( (_a_end) == (_b_start) )
	
#define D_INTERVAL_A_MET_BY_B(                                                 \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( (_a_start) == (_b_end) )

#define D_INTERVAL_A_OVERLAPS_B(                                               \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( ((_a_start) < (_b_start)) &&                                             \
      ((_b_start) < (_a_end))   &&                                             \
      ((_a_end)   < (_b_end)) )
	 
#define D_INTERVAL_A_OVERLAPPED_BY_B(                                          \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( ((_b_start) < (_a_start)) &&                                             \
      ((_a_start) < (_b_end))   &&                                             \
      ((_b_end)   < (_a_end)) )

#define D_INTERVAL_A_BEGINS_B(                                                 \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( ((_a_start) == (_b_start)) &&                                            \
	  ((_a_end) < (_b_end)) )
	
	
#define D_INTERVAL_A_STARTED_BY_B(                                             \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( ((_a_start) == (_b_start)) &&                                            \
	  ((_b_end) < (_a_end)) )

#define D_INTERVAL_A_DURING_B(                                                 \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( ((_b_start) < (_a_start)) &&                                             \
	  ((_a_end) < (_b_end)) )
	  
#define D_INTERVAL_A_CONTAINS_B(                                               \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( ((_a_start) < (_b_start)) &&                                             \
	  ((_b_end) < (_a_end)) )

#define D_INTERVAL_A_FINISHES_B(                                               \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( ((_b_start) < (_a_start)) &&                                             \
	  ((_a_end) == (_b_end)) )

#define D_INTERVAL_A_FINISHED_BY_B(                                            \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( ((_a_start) < (_b_start)) &&                                             \
	  ((_a_end) == (_b_end)) )

#define D_INTERVAL_EQUAL(                                                      \
    _a_start,                                                                  \
	_a_end,                                                                    \
	_b_start,                                                                  \
	_b_end                                                                     \
)                                                                              \
    ( ((_a_start) == (_b_start)) &&                                            \
	  ((_a_end) == (_b_end)) )


// =============================================================================

#define D_CODE_NULL_PTR  0x0


// =============================================================================
#define D_CODE_BLOCK_ARRAY          0x0A00

#define D_CODE_ARRAY_IS_BOUNDED     D_CODE_BLOCK_ARRAY | D_BYTE_BIT_R(3, 0)


#define D_CODE_ARRAY_INDEX_VALID    D_CODE_BLOCK_ARRAY | D_BYTE_BIT_R(4, 0)
#define D_CODE_ARRAY_INDEX_HI       D_CODE_BLOCK_ARRAY | D_BYTE_BIT_R(4, 1)
#define D_CODE_ARRAY_INDEX_POS      D_CODE_BLOCK_ARRAY | D_BYTE_BIT_R(4, 2)


#define D_ERROR_ARRAY_INDEX_LO      D_CODE_ARRAY_IDX_IS_VALID |                 \
                                    D_CODE_ARRAY_IDX_IS_HI
#define D_ERROR_ARRAY_INDEX_HI      D_CODE_ARRAY_FIELD | 0x02
#define D_ERROR_ARRAY_INDEX_OOB     D_CODE_ARRAY_FIELD | (D_CODE_ARRAY_IDX_LO | \
                                                          D_CODE_ARRAY_IDX_HI)


// =============================================================================
#define D_CODE_BLOCK_ARRAY          0x0A00

#define D_CODE_ARRAY_IS_BOUNDED     D_CODE_BLOCK_ARRAY | D_BYTE_BIT_R(3, 0)


#define D_CODE_ARRAY_INDEX_VALID    D_CODE_BLOCK_ARRAY | D_BYTE_BIT_R(4, 0)
#define D_CODE_ARRAY_INDEX_HI       D_CODE_BLOCK_ARRAY | D_BYTE_BIT_R(4, 1)
#define D_CODE_ARRAY_INDEX_POS      D_CODE_BLOCK_ARRAY | D_BYTE_BIT_R(4, 2)


#define D_ERROR_ARRAY_INDEX_LO      D_CODE_ARRAY_IDX_IS_VALID |                \
                                    D_CODE_ARRAY_IDX_IS_HI
#define D_ERROR_ARRAY_INDEX_HI      D_CODE_ARRAY_FIELD | 0x02
#define D_ERROR_ARRAY_INDEX_OOB     D_CODE_ARRAY_FIELD    |                    \
                                    ( D_CODE_ARRAY_IDX_LO |                    \
					                  D_CODE_ARRAY_IDX_HI )
														  
// =============================================================================
**Array errors**

```text
Condition                                           Arises in                                  Code                                    Grade
--------------------------------------------------  -----------------------------------------  --------------------------------------  ---------------------------------------------------
Array pointer is NULL                               every call                                 argument.null                           rejected
Uninitialized or already-freed array                every call                                 lifecycle.uninitialized / handle.stale  broken; detectable only with a canary or generation
Freed twice                                         free, deep_free                            handle.released                         broken
Storage NULL while count > 0                        every access                               integrity.invariant                     broken
Count above capacity; ring head or tail outside it  vector, circular                           integrity.invariant                     broken
Element size is zero                                creation, every call                       quantity.size {B}                       rejected
Element size differs from the array's own           every d_array call (size passed per call)  type.mismatch                           rejected
Element sizes differ between two arrays             insert_array, prepend_array, merge         type.mismatch                           rejected
(count + k) × element size overflows size_t         creation, append, insert, resize           quantity.size {A}                       rejected
Capacity rounding overflows                         calc_capacity                              quantity.size {A}                       rejected
Capacity of zero                                    circular creation                          quantity.count {B}                      rejected
Empty                                               get, front, back, peek, pop, remove        quantity.count {B}                      soft for pop/peek, rejected for get
Full                                                circular push, fixed-capacity insert       quantity.count {A}                      soft
Oldest element overwritten                          push_overwrite                             quantity.count {U}                      note
Now full / now empty                                successful push / pop                      quantity.count {U} / {L}                note
New capacity below current count                    new_copy_resized, shrink                   quantity.count {A}                      rejected, or caveat if truncating
Resize amount takes count below zero                resize_amount                              quantity.count {B}                      rejected
Resize amount takes count past SIZE_MAX             resize_amount                              quantity.count {A}                      rejected
Resize factor negative                              resize_factor                              value.range {B}                         rejected
Resize factor NaN or infinite                       resize_factor                              value.non_finite                        rejected
Resize factor result rounded                        resize_factor                              value.precision                         caveat
Zero-count operation                                append 0, resize 0, shift 0                —                                       ok, E: no-op
Source points into the array's own storage          append, insert, insert_array(self)         span.overlapping                        rejected, or handled by copying first
Array mutated by its own callback                   foreach, filter callbacks                  handle.stale                            broken
Deep free without a free function                   deep_free                                  argument.null                           rejected
NULL array among merge arguments                    new_merge                                  argument.null                           rejected
Varargs count doesn't match the arguments           new_from_args, new_merge                   quantity.arity                          undetectable in C
```

**Indexing errors.** For a `d_index`, −1 is the last element and −n the first, so
the valid access indices are [−n, n−1].

```text
Condition                                            Arises in               Code                                        Grade
---------------------------------------------------  ----------------------  ------------------------------------------  ---------------------------
Index ≥ count                                        get, set, remove, swap  position.index {A}                          rejected
Negative index beyond −count                         same                    position.from_end {B}                       rejected
Any index on an empty array                          same                    quantity.count {B}                          rejected
Checked against capacity instead of count            vector, circular        position.index {A}                          rejected (a classic bug)
Insertion at the append slot (index = count)         insert                  —                                           valid, must not be rejected
Insertion into an empty array                        insert                  —                                           valid, must not be rejected
Insertion index beyond the append slot               insert                  position.index {A}                          rejected
Swap with an invalid index                           swap                    position.index / from_end, ARG names which  rejected
Swap of an index with itself                         swap                    —                                           ok, E: no-op
Negative d_index cast to size_t                      filter calls            position.index {A}                          rejected
Index list entry out of bounds                       filter at_indices       position.index {A}                          rejected
Index list NULL with a nonzero count                 filter at_indices       argument.null                               rejected
Typed read at an unaligned byte offset               byte buffers            position.misaligned                         rejected
Count beyond what d_index can address (> SSIZE_MAX)  32-bit byte arrays      position.index {A}                          rejected
Not-found sentinel used as an index                  find → get              lookup.not_found {B}                        soft
Index held across a mutation                         callers                 handle.stale                                undetectable
Resolved from a negative index                       any                     position.from_end {W}                       note
```

**Range errors.** `d_array` ranges are inclusive `d_index` pairs; filter ranges
are half-open `size_t`.

```text
Condition                                     Arises in                                Code                              Grade
--------------------------------------------  ---------------------------------------  --------------------------------  -----------------------------
Start after end, once negatives are resolved  copy_range, slice_range                  span.inverted                     rejected
Range entirely before / after the array       copy_range, slice_range                  position.range {B} / {A}          rejected
Range overlaps the start / the end            same                                     position.range {B,L,W} / {W,U,A}  rejected
Range covers past both ends                   same                                     position.range {B,L,W,U,A}        rejected
Outside, side not reported                    cheap checks                             position.range {B,A}              rejected
Any range on an empty array                   all range calls                          quantity.count {B}                rejected
Empty range where one is required             half-open ranges                         span.empty                        rejected, or ok with E: no-op
End clamped to the count                      filter range, slice, take, skip          position.range {W,U,A}            caveat
Step of zero                                  filter slice                             argument.invalid                  rejected
start + count overflows                       (start, count) forms                     quantity.count {A}                rejected
Range length × element size overflows         copy, slice                              quantity.size {A}                 rejected
Source and destination overlap in one array   copy within, insert_array(self)          span.overlapping                  rejected, or memmove
Start after end on a reverse call             copy_range_reverse, slice_range_reverse  span.inverted                     define: error or descending
Shift amount ≥ count                          shift_left, shift_right                  position.offset {A}               rejected
Rotate amount ≥ count                         rotate, shift_*_circular                 position.offset {A}               note, reduced modulo count
```

**Other errors** (not array-specific)

```text
Condition                                     Arises in                                    Code                                    Grade
--------------------------------------------  -------------------------------------------  --------------------------------------  ---------------------------------
Allocation failed, array unchanged            creation, append, insert, resize             resource.memory {A}                     failed
Allocation failed partway, some effects kept  merge, multi-element insert                  resource.memory {A}                     failed, E: partial
Comparator NULL                               sort, find, contains, count_value, distinct  argument.null                           rejected
Comparator not a strict weak order            sort, distinct                               argument.unmet                          undetectable outside debug builds
Predicate or apply function NULL              filter, foreach                              argument.null                           rejected
Callback reported failure                     foreach, filter, deep_free                   the callee's code, else generic.callee  failed, E: partial if mid-pass
Value not found                               find, find_last, contains                    lookup.not_found {B}                    soft
No elements matched                           filter where, distinct                       quantity.count {L}                      ok: an empty result
Closest match on an empty array               find_closest                                 quantity.count {B}                      soft
Out-parameter NULL                            pop_to, index conversion                     argument.null                           rejected
Fill value NULL                               fill                                         argument.null                           rejected
Lock not acquired                             circular_array_mtx                           sync.busy / would_block / deadlock      soft or failed, R
Debug invariant check failed                  debug builds                                 integrity.invariant                     broken
```