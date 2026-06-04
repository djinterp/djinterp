# C-side Modules — `env_c_lib.h`, `env_attributes.h`, `env_vendor_attributes.h`

These three headers cover the C runtime that **both** C and C++ translation units
rely on: standard-library/runtime feature flags, portable `[[…]]` attribute
wrappers, and vendor-specific attribute wrappers. All require `env.h` to be
included first.

---

## `env_c_lib.h` — C runtime / standard-library features

Despite the `D_ENV_C_HAS_*` naming, these flags describe the **C runtime**, so the
whole header is gated on `__STDC_HOSTED__` (hosted vs. freestanding) rather than on
source language. Every macro is pre-definable to override detection. Each is `1` if
available, `0` otherwise.

### A. Threading and concurrency

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_C11_THREADS` | C11 `<threads.h>` (C11+ and not `__STDC_NO_THREADS__`) |
| `D_ENV_C_HAS_PTHREAD` | POSIX threads (POSIX-like or Android) |
| `D_ENV_C_HAS_WINDOWS_THREADS` | Windows threading API |
| `D_ENV_C_HAS_STDATOMIC` | C11 `<stdatomic.h>` (or C++11 atomics) |

### B. Standard headers

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_STDBOOL_H` | `<stdbool.h>` (C99+) |
| `D_ENV_C_HAS_STDINT_H` | `<stdint.h>` (C99+) |
| `D_ENV_C_HAS_INTTYPES_H` | `<inttypes.h>` (C99+) |
| `D_ENV_C_HAS_STDALIGN_H` | `<stdalign.h>` (C11+) |
| `D_ENV_C_HAS_UCHAR_H` | `<uchar.h>` (C11+) |

### C. POSIX headers

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_UNISTD_H` | `<unistd.h>` (POSIX-like or Android) |
| `D_ENV_C_HAS_SYS_TYPES_H` | `<sys/types.h>` (POSIX-like or Windows) |
| `D_ENV_C_HAS_SYS_STAT_H` | `<sys/stat.h>` (POSIX-like or Windows) |
| `D_ENV_C_HAS_DIRENT_H` | `<dirent.h>` (POSIX-like) |

### D. String and memory functions

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_STRTOK_R` | reentrant `strtok_r` (POSIX) |
| `D_ENV_C_HAS_STRTOK_S` | `strtok_s` (Annex K / MSVC) |
| `D_ENV_C_HAS_SNPRINTF` | `snprintf` or close equivalent (C99+ / Windows) |
| `D_ENV_C_HAS_STRDUP` | `strdup` (POSIX / C23) |
| `D_ENV_C_HAS_STRNDUP` | `strndup` (POSIX / C23) |
| `D_ENV_C_HAS_STRCASECMP` | `strcasecmp` (POSIX) |
| `D_ENV_C_HAS_STRICMP` | `_stricmp` (Windows) |
| `D_ENV_C_HAS_MEMCCPY` | `memccpy` (POSIX) |

### E. File system and I/O

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_FLOCK` | `flock` file locking (POSIX) |
| `D_ENV_C_HAS_FOPEN_S` | `fopen_s` (Annex K / MSVC) |
| `D_ENV_C_HAS_FSYNC` | `fsync` (POSIX) |
| `D_ENV_C_HAS_LOCKFILE` | `LockFile` API (Windows) |
| `D_ENV_C_HAS_MMAP` | `mmap` memory-mapped files (POSIX) |
| `D_ENV_C_HAS_SCANF_S` | `scanf_s` (Annex K / MSVC) |

### F. Time and date

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_TIMESPEC_GET` | `timespec_get` (C11) |
| `D_ENV_C_HAS_CLOCK_GETTIME` | `clock_gettime` (POSIX) |
| `D_ENV_C_HAS_GETTIMEOFDAY` | `gettimeofday` (POSIX) |
| `D_ENV_C_HAS_QUERYPERFORMANCECOUNTER` | `QueryPerformanceCounter` (Windows) |

### G. Math

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_TGMATH_H` | `<tgmath.h>` type-generic math (C99+) |
| `D_ENV_C_HAS_COMPLEX_H` | `<complex.h>` (C99+ and not `__STDC_NO_COMPLEX__`) |
| `D_ENV_C_HAS_FENV_H` | `<fenv.h>` floating-point environment (C99+) |

### H. Networking

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_WINSOCK` | Winsock (Windows) |
| `D_ENV_C_HAS_BSD_SOCKETS` | BSD sockets (POSIX-like) |
| `D_ENV_C_HAS_GETADDRINFO` | `getaddrinfo` (POSIX-like or Windows) |

### I. Process and system

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_FORK` | `fork` (POSIX) |
| `D_ENV_C_HAS_EXECVE` | `execve` (POSIX) |
| `D_ENV_C_HAS_GETPID` | `getpid` (POSIX-like or Windows) |
| `D_ENV_C_HAS_SIGNAL_H` | `<signal.h>` (POSIX-like or Windows) |

### J. Memory management

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_ALIGNED_ALLOC` | `aligned_alloc` (C11, excluding Apple) |
| `D_ENV_C_HAS_POSIX_MEMALIGN` | `posix_memalign` (POSIX) |
| `D_ENV_C_HAS_ALIGNED_MALLOC` | `_aligned_malloc` (Windows) |
| `D_ENV_C_HAS_ALLOCA` | `alloca` stack allocation (POSIX-like or Windows) |

### K. SIMD and hardware intrinsics

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_SSE` / `_SSE2` | SSE / SSE2 intrinsics (x86/x64) |
| `D_ENV_C_HAS_AVX` / `_AVX2` | AVX / AVX2 intrinsics (x86/x64) |
| `D_ENV_C_HAS_NEON` | ARM NEON intrinsics (ARM/ARM64) |

### L–M. Other

| Symbol | Meaning |
| --- | --- |
| `D_ENV_C_HAS_VLA` | Variable-length arrays (C99+ and not `__STDC_NO_VLA__`) |
| `D_ENV_C_HAS_SECURE_STRING_LIB` | Annex K secure string library |
| `D_ENV_C_HAS_GETENTROPY` | `getentropy` (Linux/Apple/BSD; deliberately narrower than POSIX-like) |

---

## `env_attributes.h` — portable standard attributes

Defines `D_*` macros that expand to the correct attribute spelling for the detected
language, compiler, and version, with graceful fallbacks (compiler `__attribute__`
/ `__declspec` / keyword, then no-op). The file splits into a C++ section and a C
section (`__cplusplus`); each macro is pre-definable. Resolution generally goes:
standard `[[…]]` → `__has_cpp_attribute`/`__has_c_attribute` probe → vendor
extension → no-op.

| Macro | Expands to | Introduced |
| --- | --- | --- |
| `D_DELETE` | `= delete` / empty | C++11 |
| `D_NORETURN` | `[[noreturn]]` / `_Noreturn` / vendor | C++11 / C11 / C23 |
| `D_CARRIES_DEPENDENCY` | `[[carries_dependency]]` (C++ only) | C++11 |
| `D_DEPRECATED` | `[[deprecated]]` / vendor | C++14 / C23 |
| `D_DEPRECATED_MSG(msg)` | `[[deprecated("…")]]` / vendor | C++14 / C23 |
| `D_FALLTHROUGH` | `[[fallthrough]]` / vendor | C++17 / C23 |
| `D_NODISCARD` | `[[nodiscard]]` / `warn_unused_result` | C++17 / C23 |
| `D_NODISCARD_MSG(msg)` | `[[nodiscard("…")]]` (falls back to `D_NODISCARD`) | C++20 / C23 |
| `D_MAYBE_UNUSED` | `[[maybe_unused]]` / `unused` | C++17 / C23 |
| `D_NO_UNIQUE_ADDRESS` | `[[no_unique_address]]` / `[[msvc::no_unique_address]]` | C++20 |
| `D_LIKELY` / `D_UNLIKELY` | `[[likely]]` / `[[unlikely]]` (statement attrs) | C++20 |
| `D_ASSUME(expr)` | `[[assume(…)]]` / `__assume` / `__builtin_assume` | C++23 |

In C mode, attributes with no C equivalent (`D_CARRIES_DEPENDENCY`,
`D_NO_UNIQUE_ADDRESS`, `D_LIKELY`, `D_UNLIKELY`) are no-ops.

---

## `env_vendor_attributes.h` — vendor-specific attributes

Portable wrappers for compiler attributes with **no** standard `[[…]]` equivalent
(or whose standard form is too recent to rely on). It deliberately avoids
redefining anything owned by `djinterp.h` (`D_INLINE`, `D_NOINLINE`, `D_RESTRICT`)
or `env_attributes.h`. Every macro is pre-definable.

### Function purity and optimization

| Macro | Meaning |
| --- | --- |
| `D_PURE` | Pure function (reads globals, no side effects) |
| `D_CONST` | Const function (depends only on its parameters) |
| `D_HOT` / `D_COLD` | Hot-/cold-path optimization hint |
| `D_FLATTEN` | Inline all calls within the function |

### Memory and allocation

| Macro | Meaning |
| --- | --- |
| `D_MALLOC` | Returns a pointer to unaliased memory |
| `D_ALLOC_SIZE(...)` | Which params describe the allocation size |
| `D_ALLOC_ALIGN(n)` | Which param gives the alignment |
| `D_ALIGNED(n)` | Minimum alignment for a type/variable |
| `D_PACKED` | Remove struct padding |

### Null and parameter contracts

| Macro | Meaning |
| --- | --- |
| `D_NONNULL(...)` | Listed pointer params must not be `NULL` |
| `D_NONNULL_ALL` | All pointer params must not be `NULL` |
| `D_RETURNS_NONNULL` | Return value is never `NULL` |

### Format-string checking

| Macro | Meaning |
| --- | --- |
| `D_FORMAT_PRINTF(fmt, va)` | `printf`-style format/args validation |
| `D_FORMAT_SCANF(fmt, va)` | `scanf`-style format/args validation |

### Visibility, linkage, and placement

| Macro | Meaning |
| --- | --- |
| `D_EXPORT` / `D_IMPORT` | Public / imported symbol (`dllexport` / `dllimport` / visibility) |
| `D_HIDDEN` | Hidden (non-exported) symbol |
| `D_WEAK` | Weak symbol |
| `D_USED` | Keep symbol even if seemingly unused |
| `D_SECTION` | Place symbol in a named section |
| `D_THREAD_LOCAL` | Thread-local storage |
| `D_CONSTRUCTOR` / `D_DESTRUCTOR` | Run before `main` / after `main` |
| `D_NAKED` | Naked function (no prologue/epilogue) |

### Branch and memory hints

| Macro | Meaning |
| --- | --- |
| `D_EXPECT(expr, val)` | `__builtin_expect` wrapper |
| `D_EXPECT_TRUE(expr)` / `D_EXPECT_FALSE(expr)` | Expression-level likely/unlikely |
| `D_PREFETCH` | Memory prefetch hint |
| `D_UNREACHABLE` | Mark code as unreachable |

(`D_INTERNAL_GCC_COMPAT_` is an internal implementation helper, not part of the
public surface.)
