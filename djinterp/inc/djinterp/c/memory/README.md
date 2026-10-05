# memory — zero-overhead C core with derived C++ faces

A replacement for the previous header-only C++ `pool.hpp` / `monotonic_byte_arena.hpp`.
Every algorithm now lives in C; the C++ faces derive from the C structs and add
no bytes.

---

## Installing

The archive is rooted at the **repo root** — the directory holding `inc/` and
`src/`. Unzip there; the four trees merge into the existing ones and nothing is
overwritten.

```
<repo root>/
├── inc/djinterp/c/memory/            .h          (rule a)
├── inc/djinterp/core/memory/         .hpp        (rule b)
├── inc/djinterp/config/core/memory/  cfg_*.h
└── src/djinterp/c/memory/            .c          (rule c)
    └── test/                         harnesses
```

**No `-I` is required.** Every include is repo-root relative, the same way
`color` does it, so this builds from the repo root as-is:

```
cc src/djinterp/c/memory/mem_common.c \
   src/djinterp/c/memory/mem_source.c \
   src/djinterp/c/memory/arena.c      \
   src/djinterp/c/memory/pool.c       \
   src/djinterp/c/memory/test/t_pool.c -o t_pool
```

### How the includes resolve

| from | to | spelling |
|---|---|---|
| `c/memory/*.h` | `djinterp.h` | `../djinterp.h` |
| `c/memory/mem_common.h` | `djinterp.hpp` | `../../core/djinterp.hpp` |
| `c/memory/*.h` | config | `../../config/core/memory/cfg_*.h` |
| `core/memory/*.hpp` | `djinterp.hpp` | `../djinterp.hpp` |
| `core/memory/*.hpp` | the C kernel | `../../c/memory/*.h` |
| `src/.../memory/*.c` | its header | `../../../../inc/djinterp/c/memory/*.h` |

The `.c` climb is four levels (`memory → c → djinterp → src → root`), one
shallower than `color`'s five because `color` sits under an extra `util/`.

### Two judgment calls

**Config stays at `inc/djinterp/config/core/memory/`.** Read literally, rule (a)
would put `cfg_*.h` under `inc/djinterp/c/` since they are `.h` files. But every
existing module reaches its config as `../../config/core/<module>/cfg_*.h` from
a header at `<tier>/<module>/`, which resolves to `inc/djinterp/config/` from
both the `c/` and `core/` tiers. Moving config under `c/` would break that
convention for one module only. Rule (a) reads as being about code headers; say
the word and this moves.

**Harnesses live at `src/djinterp/c/memory/test/`.** They are programs, not
headers, so rules (a) and (b) do not reach them, and keeping the `.c` and
`.cpp` harnesses together beside the sources they exercise keeps the build line
short. The `test/` module at `inc/djinterp/test/c/` is the test *framework* and
is untouched.

---

## Layout

```
inc/djinterp/c/memory/          — the C core (rule a)
    memory.h            umbrella
    mem_common.h        vocabulary: counter type, status set, block view,
                        alignment arithmetic, accounting, poison, guard bands
    mem_source.h        upstream byte-source protocol + built-in sources
    arena.h             monotonic bump arena, mark/rewind, chained regions
    pool.h              fixed-slot pool, three release policies, handles

inc/djinterp/core/memory/       — the C++ faces (rule b)
    memory.hpp          umbrella
    mem_common.hpp      C++ vocabulary, resource traits, concepts
    mem_source.hpp      memory_source, buffer_source, counting_source
    arena.hpp           arena + arena_scope
    pool.hpp            raw_pool + pool<T, Policy> + pool_handle<T>
    pool_allocator.hpp  node_pool, pool_allocator<T>, arena_allocator<T>
    memory_strategy_common.hpp   storage_kind, detection traits,
                                 classification, concepts
    memory_strategy.hpp          arena_ / pool_ / buffer_ /
                                 allocator_memory_strategy

inc/djinterp/config/core/memory/
    cfg_mem_common.h    subframework root: aggregate, presets, vocabulary knobs
    cfg_mem_source.h    which sources exist, vtable slots, over-alignment API
    cfg_arena.h         chaining, growth policy, region geometry, marks
    cfg_pool.h          release policies, index/generation width, block table

src/djinterp/c/memory/          — the implementation units (rule c)
    mem_common.c  mem_source.c  arena.c  pool.c
    test/  t_arena.c  t_pool.c  t_cpp.cpp  t_strategy.cpp
```

Compile and link the four `.c` units.

---

## Dependency graph

```
mem_common ──> mem_source ──┬──> arena ──┐
                            └──> pool    │
                                   ▲     │
                                   └─────┘   d_arena_as_source()
```

`mem_common` is the only module named `_common`: four modules read it, and
without it `arena` and `pool` would each carry a private copy of the same
rounding and the same overflow guard. `mem_source` is a foundation for three
modules but is a module in its own right, not a `_common`.

The back edge is `d_arena_as_source()`, which presents an arena *as* a
`d_mem_source`. One upstream request at startup, an arena over it, every other
allocator carved out of that — with no allocator in the chain aware of the
arrangement, and the whole graph freed by one `d_arena_release`.

---

## Control over every byte

Knobs are documented as **field** knobs (remove struct members) or **branch**
knobs (remove only code), because conflating the two is how a config file stops
meaning anything. Measured on LP64:

| struct | default | MINIMAL | MINIMAL + 32-bit counters + 16-bit index | DEBUG |
|---|---|---|---|---|
| `d_arena` | 88 | 64 | **48** | 168 |
| `d_arena_region` | 24 | 16 | **8** | 24 |
| `d_pool` | 128 | 120 | **80** | 192 |
| `d_pool_handle` | 8 | 8 | **4** | 8 |
| `d_mem_stats` | 48 | — | 24 | 64 |

The largest single lever is `D_CFG_MEM_SIZE_BITS`, which sets the width of
`d_mem_size`: an arena carries five counters and a pool seven.
`D_CFG_POOL_INDEX_BITS` is second — it is simultaneously the slot index, the
free-list link, and therefore the pool's **minimum slot size**.

---

## Design decisions worth knowing

**The free list is threaded by index, not by pointer.** Smaller (4 bytes rather
than 8 on LP64), and *relocatable* — a pool whose internal links are offsets can
be `memcpy`'d, written out, and read back at a different address. This is the
framework's standing offsets-not-pointers decision applied to the one data
structure here that has a choice.

**Generation counters sit after the payload, not before it.** Before it, the
counter would have to be padded out to the slot alignment to keep the payload
aligned — 16 wasted bytes on a 16-aligned slot for a 2-byte counter. After it,
it lands in padding the stride was going to contain anyway.

**A full fixed arena reports `EXHAUSTED`, not `UNSUPPORTED`.** The caller asked
for an allocation, not for growth. `UNSUPPORTED` is reserved for an *operation*
the configuration forbids, such as releasing an individual block back to an
arena.

**Ownership is validated before any memory around a pointer is read.**
Verifying a guard band reads the bytes on either side of the payload, and a
foreign pointer is precisely the case where those bytes are not ours.

**A non-power-of-two alignment is refused, never raised to the floor.** With
`D_CFG_MEM_MIN_ALIGN` at 16, a request for alignment 3 would otherwise be
silently given 16 and never learn that 3 was meaningless.

---

## The C++ tiers

Every C module has a `.hpp` face, one for one:

| C | C++ |
|---|---|
| `mem_common.h` | `mem_common.hpp` |
| `mem_source.h` | `mem_source.hpp` |
| `arena.h` | `arena.hpp` |
| `pool.h` | `pool.hpp`, `pool_allocator.hpp` |
| — | `memory_strategy_common.hpp`, `memory_strategy.hpp` |

The strategy tier has no C counterpart **by design**. It is structural
detection over a duck-typed surface, resolved entirely at translation time, and
C has no notion of any of that — so per the phase rule it is C++-only *notation*
rather than a second implementation. It allocates nothing, computes no offset,
and decides no policy.

What it does do is **project the C configuration**. Every descriptive constant
is read off the kernel rather than asserted independently:

```cpp
arena_memory_strategy::pointer_stable          <- D_INTERNAL_ARENA_CHAIN
pool_memory_strategy::supports_individual_release
                                               <- pool's d_pool_policy
pool_memory_strategy::supports_generational_sweep
                                               <- D_INTERNAL_POOL_GENERATIONAL
```

That is what "derived from the C" means at this tier: the C config is the single
source of truth and the constants are a projection of it.

This required lifting the pool's release policy from a runtime field to a
template parameter — `pool<T, Policy>`. The kernel still carries the field
(one compiled implementation has to serve every policy), but the *facts that
follow from* the policy have to be `static constexpr` for a container to read
them, and a runtime field cannot produce a constant. The parameter is not a
member; `sizeof(pool<char, D_POOL_POLICY_MONOTONIC>) == sizeof(d_pool)` is
asserted.

`memory_strategy_common.hpp` earns its `_common` suffix: four modules read it.

### Strategy cost (LP64)

| type | bytes |
|---|---|
| `arena_memory_strategy<double>` | 8 |
| `pool_memory_strategy<pool<double>>` | 8 |
| `allocator_memory_strategy<std::allocator<int>>` | 1 |
| `buffer_memory_strategy<double, 64>` | 520 (512 payload + one index) |

---

## The allocator faces

Every wrapper derives from its C kernel and adds no data members, asserted:

```cpp
static_assert(sizeof(arena)       == sizeof(::d_arena));
static_assert(sizeof(raw_pool)    == sizeof(::d_pool));
static_assert(sizeof(pool<char>)  == sizeof(::d_pool));   // and double, void*
static_assert(sizeof(pool_handle<char>) == sizeof(::d_pool_handle));
```

`t_cpp.cpp` also checks at run time that each base subobject is at offset zero,
so a wrapper pointer and a kernel pointer are the same address.

**The one genuine capability difference** is object lifetime. The kernel deals
in raw slots and cannot know one holds an object, so it can never run a
constructor or destructor. `pool<T>::create` / `destroy` can. `t_cpp.cpp` proves
the difference is real by counting live objects across both paths.

**`node_pool` is deliberately exempt from the cost law**, and says so in its own
`static_assert`. It is not a wrapper — it holds an unconfigured `d_pool` plus the
geometry it will configure it with. It exists because a container's node type has
no portable name: `std::list<int, A>` allocates its own private node, not an
`int`, so the size cannot arrive until the container rebinds the allocator. A C
caller always knows what it is allocating and has no use for any of it.

---

## Verification

All under ASan + UBSan.

| suite | result |
|---|---|
| `t_arena.c`, `t_pool.c` × 20 configurations | 40 / 40 |
| `t_cpp.cpp`, `t_strategy.cpp` × 20 configurations × {C++11, 17, 20} | 120 / 120 |
| C compile, `-Wall -Wextra -Wpedantic`, C99 / C11 / C17 | clean |
| C++ compile, `-Wall -Wextra -Wpedantic`, C++11…C++23 | clean |

`t_strategy.cpp` writes **one** container against the contract and instantiates
it over all five strategy bindings, checking that each one's declared constants
match what it actually does — that a monotonic-pool strategy really does decline
to release, that a stable strategy really does keep earlier pointers valid. It
draws every byte from a static array, so it runs identically on a heapless
build.

`memory_strategy.hpp` also carries 17 contract assertions at its point of
definition, including the negative case: a bare `std::allocator` must *not*
classify as a strategy, or the contract's descriptive half would be optional in
practice.

The harnesses run against *any* configuration: a policy the build did not
compile is skipped rather than failed, and the MINIMAL preset (no system source
at all) runs the whole pool suite over a buffer-backed arena instead. A knob that
made a harness fail would be a knob that broke the module.

Four real defects were found by the configuration sweep and fixed, each visible
only in a specific combination:

1. The pool handed out the slot base while the guard-band writer assumed a lead
   offset — writing *before* the first slot, into the block header. Fixed by
   publishing `d_mem_redzone_lead` in `mem_common` and separating slot base from
   payload pointer throughout the pool.
2. `release_slot` verified guard bands before checking ownership, so validating
   a foreign pointer read the memory around it.
3. The pointer-link free list mixed slot bases with payload pointers, silently
   truncating the free list to one entry — visible only with redzones *and*
   pointer links *and* narrow counters.
4. A non-power-of-two alignment was silently rounded up to a raised floor.

---

## Two findings outside this module

**`d_pool_release` was a parity hazard.** It was originally both an enum tag and
a function name. C keeps enum tags in a separate namespace from ordinary
identifiers, so the header compiled clean as C; C++ has no such separation, the
function hid the type, and every declaration using it failed. The enum is now
`d_pool_policy`. This is worth a lint rule — it is exactly the class of bug the
parity law exists to catch, and it is invisible to a C-only build.

**`djinterp.h`'s `D_STATIC_ASSERT` fallback depends on `dmacro.h`.** Below C11
and below C++11 it expands `D_CONCAT`, which lives in `dmacro.h` — so any
translation unit reaching that fallback without `dmacro.h` fails to compile.
Nothing here trips it, but the coupling is undocumented.

---

## What was missing on the first pass

The first delivery replaced the *allocator* tier (`pool_resource` → `d_pool` /
`pool<T>`, `monotonic_byte_arena` → `d_arena` / `arena`) but left the *strategy*
tier dangling — and its concrete adapters referenced types that no longer
existed. That would have broken roughly twenty container headers:
`pool_resource` appears 188 times across `container_cpp`, `storage_kind` 149,
`pointer_stable` 140, `supports_individual_release` 92. The strategy tier above
closes that gap.

`static_buffer_strategy` is replaced by `buffer_memory_strategy`, which keeps
the same contract including `extent`. It is deliberately *not* built on
`d_arena`: an arena over an inline buffer would carry a whole `d_arena` struct's
worth of counters to administer a bump cursor over a fixed array, and the point
of that strategy is that it costs the array plus one index.

---

## Not built

**`slab`** — a size-class segregated allocator over N pools, which would have
been the fourth module. Designed but not written; the shape is a
`d_slab_config` carrying an ascending class table (defaulting to powers of two),
an array of `d_pool` (one per class), a size→class map, and a fallback to the
upstream source for oversize requests. It composes entirely from what exists —
`d_pool` plus `d_mem_source` — and adds no new concepts, which is why it was the
right thing to cut rather than rush. `D_CFG_SLAB_CLASS_MAX`, `_DEFAULT_CLASSES`,
`_LARGE_FALLBACK`, and `_CLASS_SELECT` are the knobs it wants.
