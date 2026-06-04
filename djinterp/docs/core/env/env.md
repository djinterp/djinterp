# `env.h` — Environment Detection

`env.h` is the core of the djinterp environment layer. It performs **compile-time**
detection of the build environment and exposes a unified `D_ENV_*` macro interface
so portable code can adapt to the language standard, compiler, CPU, OS, and build
type with zero runtime overhead.

Everything is driven from `env.h`. It pulls in its configuration from
`env_config.h` and then `#include`s several internal sub-headers once detection is
complete. Those sub-headers are **not** meant to be included directly; see the
companion docs for their contents:

| Area | Header | Doc |
| --- | --- | --- |
| Core detection (this file) | `env.h` | `env.md` |
| `long long` availability | `env_long_long.h` *(internal)* | `env.md` |
| C runtime / stdlib features | `env_c_lib.h` *(internal)* | `env_c.md` |
| Portable `[[…]]` attributes | `env_attributes.h` | `env_c.md` |
| Vendor compiler attributes | `env_vendor_attributes.h` | `env_c.md` |
| C++98 stdlib headers | `env_cpp98.h` | `env_cpp.md` |
| C++11–C++26 feature tests | `env_cpp_features.h` | `env_cpp.md` |
| Per-OS detection | `env_linux/windows/apple/bsd/ios.h` | `env_os.md` |

## Configuration system

`env.h` supports simulating an environment instead of detecting it, via
`D_CFG_ENV_CUSTOM` (defined in `env_config.h`):

- `0` (default) — full automatic detection.
- `1` — skip all detection; you must pre-define the relevant `D_ENV_DETECTED_*`
  variables.
- bitfield — selectively skip sections (bit 0 = language, 1 = compiler, 2 = OS,
  3 = arch, 4 = build).

Pre-defining a `D_ENV_DETECTED_*` variable automatically enables its section's
"manual" branch, which is how alternate environments are exercised in tests. Each
detection block is guarded by a `D_CFG_ENV_*_ENABLED` switch.

## I. Language standard

Numeric version constants are provided for every standard, e.g.
`D_ENV_LANG_C_STANDARD_C99` (`199901L`) and `D_ENV_LANG_CPP_STANDARD_CPP20`
(`202002L`), spanning C95–C23 and C++98–C++23.

| Symbol | Meaning |
| --- | --- |
| `D_ENV_LANG_CPP_STANDARD` | Numeric value of the detected C++ standard |
| `D_ENV_LANG_CPP_STANDARD_NAME` | Detected C++ standard as a string (`"C++17"`) |
| `D_ENV_LANG_C_STANDARD` | Numeric value of the detected C standard |
| `D_ENV_LANG_C_STANDARD_NAME` | Detected C standard as a string (`"C11"`) |
| `D_ENV_LANG_DETECTED_CPP` | Defined when `__cplusplus` is present |
| `D_ENV_LANG_DETECTED_C` | Defined when `__STDC_VERSION__` is present |
| `D_ENV_LANG_USING_CPP` | `1` if compiling as C++, else `0` |
| `D_ENV_LANG_USING_C` | `1` if a C standard was detected, else `0` |
| `D_ENV_LANG_IS_CPP98_OR_HIGHER` … `_CPP23_OR_HIGHER` | `1` if C++ ≥ that standard |
| `D_ENV_LANG_IS_C95_OR_HIGHER` … `_C23_OR_HIGHER` | `1` if C ≥ that standard |
| `D_DELETE` | `= delete` on C++11+, empty on C++03 |
| `D_ENV_HAS_LONG_LONG` | `1` if `long long`/`unsigned long long` exist (from `env_long_long.h`) |

## II. POSIX standards

POSIX version constants (`D_ENV_POSIX_VERSION_1988` … `_2024`), `_POSIX_C_SOURCE`
levels, and XSI levels (`D_ENV_POSIX_XSI_VERSION_3` … `_700`) are defined, then
detection runs against `_POSIX_VERSION`, `_POSIX_C_SOURCE`, `_XOPEN_VERSION`, and
known Unix predefines.

| Symbol | Meaning |
| --- | --- |
| `D_ENV_POSIX_VERSION` | Detected POSIX version number (`0L` if none) |
| `D_ENV_POSIX_NAME` | POSIX standard as a string (`"POSIX.1-2008"`) |
| `D_ENV_POSIX_2024` … `_1988` | Defined for the specific detected revision |
| `D_ENV_POSIX_LIKELY` / `_NONE` / `_UNKNOWN` | Coarse detection outcome flags |
| `D_ENV_POSIX_XSI_VERSION` / `_NAME` | Detected XSI level number / string |
| `D_ENV_POSIX_FEATURE_THREADS` | `1` if pthreads are advertised |
| `D_ENV_POSIX_FEATURE_REALTIME` | `1` if real-time extensions are advertised |
| `D_ENV_POSIX_FEATURE_SOCKETS` | `1` if sockets/networking are advertised |
| `D_ENV_POSIX_FEATURE_SHARED_MEMORY` | `1` if POSIX shared memory is advertised |
| `D_ENV_POSIX_FEATURE_SEMAPHORES` | `1` if POSIX semaphores are advertised |
| `D_ENV_POSIX_FEATURE_MESSAGE_QUEUES` | `1` if POSIX message queues are advertised |
| `D_ENV_POSIX_FEATURE_MEMORY_MAPPING` | `1` if `mmap` is advertised |
| `D_ENV_POSIX_IS_AVAILABLE` | `1` if any POSIX is present |
| `D_ENV_POSIX_IS_MODERN` | `1` if POSIX.1-2001 or later |
| `D_ENV_POSIX_HAS_FEATURE(m)` | Passthrough test of a feature macro |
| `D_ENV_POSIX_VERSION_AT_LEAST(v)` | `1` if POSIX version ≥ `v` |
| `D_ENV_XSI_IS_AVAILABLE` | `1` if XSI extensions present |
| `D_ENV_XSI_VERSION_AT_LEAST(v)` | `1` if XSI version ≥ `v` |

## III. Compiler

Clang is checked first (it can masquerade as GCC); Apple Clang is distinguished via
`__apple_build_version__`. MSVC's `_MSC_VER` is mapped to a marketing major version.

| Symbol | Meaning |
| --- | --- |
| `D_ENV_COMPILER_CLANG` / `_APPLE_CLANG` / `_GCC` / `_MSVC` / `_INTEL` / `_BORLAND` / `_UNKNOWN` | Defined for the detected compiler |
| `D_ENV_COMPILER_NAME` | Short compiler name (`"GCC"`) |
| `D_ENV_COMPILER_FULL_NAME` | Full compiler name (`"GNU Compiler Collection"`) |
| `D_ENV_COMPILER_MAJOR` / `_MINOR` / `_PATCHLEVEL` | Version components |
| `D_ENV_COMPILER_VERSION_STRING` | Raw version string |
| `D_ENV_COMPILER_VERSION_AT_LEAST(maj,min,pat)` | `1` if compiler ≥ that version |
| `D_ENV_COMPILER_VERSION_AT_MOST(maj,min,pat)` | `1` if compiler ≤ that version |
| `D_ENV_PP_HAS_VA_OPT` | `1` if `__VA_OPT__` works (probed in C, `__cpp_va_opt` in C++) |
| `D_ENV_PP_HAS_VA_OPT_ENABLED` | Function-style alias for the above |

## IV-A. Preprocessor limits

Standard minimum limits (C89/C99/C++) and per-compiler practical maximums.

| Symbol | Meaning |
| --- | --- |
| `D_ENV_PP_MIN_MACRO_ARGS` / `_MIN_NESTING_DEPTH` / `_MIN_MACRO_IDS` / `_MIN_PARAMS` / `_MIN_STRING_LENGTH` | Standard-mandated minimums for the detected standard |
| `D_ENV_PP_MAX_MACRO_ARGS` / `_MAX_NESTING_DEPTH` / `_MAX_MACRO_IDS` / `_MAX_STRING_LENGTH` | Practical per-compiler maximums (`0` = memory-bound/unlimited) |
| `D_ENV_PP_LIMIT_SOURCE` | String describing where the limits came from |
| `D_ENV_PP_ARGS_WITHIN_LIMIT(n)` | `1` if `n` ≤ practical max args |
| `D_ENV_PP_ARGS_WITHIN_STANDARD(n)` | `1` if `n` ≤ standard min args (portable) |
| `D_ENV_PP_IS_UNLIMITED(l)` | `1` if limit `l` is `0` |
| `D_ENV_PP_EFFECTIVE_LIMIT(l)` | `l`, treating `0` as a large finite value |

(The `D_ENV_PP_LIMIT_C89_*`, `_C99_*`, and `_CPP_*` families hold the raw constants
these are selected from.)

## V. Architecture

`D_ENV_ARCH_TYPE_*` enumerates architecture families (`X86`, `X64`, `ARM`, `ARM64`,
`RISCV`, `POWERPC`, `MIPS`, `SPARC`, `S390`, `IA64`, `ALPHA`, `UNKNOWN`).
`D_ENV_ARCH_ENDIAN_*` enumerates endianness (`UNKNOWN`, `LITTLE`, `BIG`).

| Symbol | Meaning |
| --- | --- |
| `D_ENV_ARCH_X64` / `_X86` / `_ARM64` / `_ARM` / `_RISCV` / `_POWERPC` / `_MIPS` / `_SPARC` / `_S390` / `_IA64` / `_ALPHA` / `_UNKNOWN` | Defined for the detected architecture (with `*32`/`*64` sub-flags where relevant) |
| `D_ENV_ARCH_NAME` | Architecture as a string (`"x86-64"`) |
| `D_ENV_ARCH_TYPE` | One of the `D_ENV_ARCH_TYPE_*` constants |
| `D_ENV_ARCH_BITS` | `32`, `64`, or `0` if unknown |
| `D_ENV_ARCH_ENDIAN` | One of the `D_ENV_ARCH_ENDIAN_*` constants |
| `D_ENV_ARCH_IS_X86_FAMILY` / `_IS_ARM_FAMILY` | `1` for that family |
| `D_ENV_ARCH_IS_64BIT` / `_IS_32BIT` | `1` for that bit width |
| `D_ENV_ARCH_IS_LITTLE_ENDIAN` / `_IS_BIG_ENDIAN` | `1` for that byte order |

## VI. Operating system

OSes use a **block/flag** classification: each OS gets an 8-bit `D_ENV_OS_FLAG_*`
id whose high nibble is its "block" (e.g. block `0x1` = Unix family, `0x4` = BSD
family, `0x6`/`0x7`/`0x8` = Microsoft). `D_ENV_OS_BLOCK_SIZE` (`4`) is the nibble
shift. Range markers (`D_ENV_OS_FLAG_WIN_FIRST/LAST`, `_DISCONTINUED_FIRST/LAST`,
`_UNSUPPORTED_FIRST/LAST`, `_VENDOR_MS_FIRST/LAST`) bound the classification helpers.

| Symbol | Meaning |
| --- | --- |
| `D_ENV_OS_FLAG_*` | Per-OS id constants (`MACOS`, `LINUX`, `BSD_FREE`, `WIN_PC_10`, `IOS`, `ANDROID`, …) |
| `D_ENV_OS_ID` | The detected OS's flag value |
| `D_ENV_OS_NAME` | Detected OS as a string (`"Linux"`) |
| `D_ENV_OS_USING_WINDOWS64` | Defined on 64-bit Windows |
| `D_ENV_IS_OS_FLAG_IN_BLOCK(f,b)` | `1` if flag `f` is in block `b` |
| `D_ENV_IS_OS_FLAG_UNIX(f)` | `1` if `f` is in the Unix block (`0x1`) |
| `D_ENV_IS_OS_MOBILE(f)` | `1` for iOS/Android/Bada |
| `D_ENV_IS_OS_MSDOS(f)` / `D_ENV_IS_OS_WINDOWS(f)` | DOS / Windows-range tests |
| `D_ENV_IS_OS_DISCONTINUED(f)` / `_UNSUPPORTED(f)` | Legacy/unsupported-range tests |
| `D_ENV_IS_OS_POSIX_COMPLIANT(f)` | Likely POSIX-conformant OS |
| `D_ENV_IS_OS_POSIX_LIKE(f)` | Unix/Apple/BSD families (traditional POSIX headers) |
| `D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID(f)` | POSIX-like plus Android |
| `D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(f)` | POSIX-like plus Windows |
| `D_ENV_PLATFORM_ANDROID` / `_WINDOWS` / `_LINUX` / `_MACOS` / `_UNIX` / `_UNKNOWN` | Legacy back-compat platform flags |
| `D_ENV_PLATFORM_NAME` | Alias of `D_ENV_OS_NAME` |

When a specific OS is detected, `env.h`'s callers pull in the matching per-OS
header (`env_linux.h`, `env_windows.h`, `env_apple.h`, `env_bsd.h`, `env_ios.h`);
see `env_os.md`.

## VII. C/C++ feature includes

After all detection above is complete, `env.h` `#include`s `env_c_lib.h` (the
`D_ENV_C_HAS_*` C-runtime family, gated on `__STDC_HOSTED__`). C++ feature tests
live in `env_cpp_features.h` and the C++98 header probes in `env_cpp98.h`. See
`env_c.md` and `env_cpp.md`.

## VIII. Build configuration

| Symbol | Meaning |
| --- | --- |
| `D_ENV_BUILD_DEBUG` | Defined in a debug build (`DEBUG`/`_DEBUG`, or `!NDEBUG`) |
| `D_ENV_BUILD_RELEASE` | Defined in a release build |
| `D_ENV_BUILD_TYPE` | `"Debug"` or `"Release"` |

> Note: with automatic detection, a build that defines neither `NDEBUG` nor a debug
> macro is classified as **Debug**.

## IX. Debug utilities

When `D_DEBUG_` is defined, `env.h` declares `void print_compiler_info(void);` for
ad-hoc diagnostics. Higher-level reporting is provided by `env_printer.hpp`, whose
`print_env(target)` walks language, compiler, OS, architecture, POSIX, preprocessor
limits, and build sections (plus optional C and C++ feature dumps).
