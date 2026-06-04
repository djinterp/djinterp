# OS Modules — `env_linux/windows/apple/bsd/ios.h`

OS detection happens in two layers:

1. **Classification** in `env.h` assigns the host an 8-bit `D_ENV_OS_FLAG_*` id
   and exposes block/range helper macros (documented in `env.md`, §VI).
2. **Per-OS detail** lives in one of the headers below, pulled in automatically by
   `env.h`'s callers once the OS is known. These are deep, version-gated probes for
   that platform's kernel, runtime, frameworks, and headers.

All per-OS headers require `env.h` first and follow the convention
`D_ENV_<PREFIX>_<CATEGORY>_<FEATURE>` — `1` if available, `0` otherwise — plus
string/version constants and a handful of `D_ENV_<PREFIX>_IS_*()` convenience
macros. The tables below summarize **categories**, not every leaf macro (each
module defines dozens to ~150).

## Selecting the right header

| Detected OS | Condition (`env.h`) | Header | Prefix(es) |
| --- | --- | --- | --- |
| Linux | `D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX` | `env_linux.h` | `D_ENV_LINUX_` |
| Windows | `D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)` | `env_windows.h` | `D_ENV_WIN_` |
| macOS / Apple desktop | `D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)` | `env_apple.h` | `D_ENV_APPLE_`, `D_ENV_MACOS_` |
| iOS / tvOS / watchOS / visionOS | `D_ENV_OS_ID == D_ENV_OS_FLAG_IOS` | `env_ios.h` (includes `env_apple.h`) | `D_ENV_IOS_`, `D_ENV_TVOS_`, `D_ENV_WATCHOS_`, `D_ENV_VISOS_`, `D_ENV_MOBILE_` |
| BSD family | `D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)` | `env_bsd.h` | `D_ENV_BSD_`, `D_ENV_FBSD_`, `D_ENV_OBSD_`, `D_ENV_NBSD_`, `D_ENV_DBSD_` |

---

## `env_linux.h` — prefix `D_ENV_LINUX_`

Detects glibc/musl/uClibc/Bionic, kernel-version targeting, GNU feature-test
macros, and a broad sweep of Linux kernel APIs.

| Section | Category | Representative symbols |
| --- | --- | --- |
| I | Kernel version | version constants + detected `LINUX_VERSION_CODE` |
| II | C library | glibc / musl / uClibc / Bionic detection + classification |
| III | GNU feature-test macros | `_GNU_SOURCE`, `_DEFAULT_SOURCE` configuration |
| IV | glibc feature availability | version-gated glibc API flags |
| V | Kernel APIs | epoll, `io_uring`, inotify, eventfd, memory/process/file ops |
| VI | Security frameworks | seccomp, capabilities, namespaces, Landlock |
| VII | Filesystem / VFS | `/proc`, `/sys`, cgroups, fanotify, statx |
| VIII | Networking | Linux-specific socket/network features |
| IX | Display server | X11, Wayland indicators |
| X | Init system | systemd detection |
| XI | Headers | Linux-specific header availability |
| XII | Toolchain | compiler/toolchain integration |
| XIII | Runtime functions | runtime detection helpers |
| XIV | Convenience | `D_ENV_LINUX_IS_MODERN()`, `_IS_EMBEDDED()`, `_IS_HARDENED()`, `_HAS_MODERN_IO()`, `_HAS_SANDBOXING()` |

---

## `env_windows.h` — prefix `D_ENV_WIN_`

Detects Windows version/SDK targeting, bitness, Unicode config, CRT/UCRT, COM/WinRT,
and security APIs.

| Section | Category | Representative symbols |
| --- | --- | --- |
| I | Version targeting | `_WIN32_WINNT` and NTDDI version constants + detected target |
| II | SDK detection | Windows SDK version |
| III | Platform / bitness | Win32/Win64, WoW64 |
| IV | Unicode / ANSI | `D_ENV_WIN_CHAR_MODE` and friends |
| V | Subsystem | console / GUI / driver |
| VI | CRT and runtime | `_HAS_UCRT`, `_DYNAMIC_CRT`, `_HAS_CRT_SECURE`, `_HAS_CRT_DBG` |
| VII | COM / OLE / WinRT | `_HAS_COM`, `_HAS_DCOM`, `_HAS_OLE`, `_HAS_CPPWINRT` |
| VIII | Security APIs | `_HAS_CNG`, `_HAS_DPAPI`, `_HAS_SSPI`, `_HAS_CRYPTOAPI`, `_HAS_BCRYPTRNG`, `_HAS_RTLGENRANDOM` |
| IX | Windows API features | fibers, thread pool, SRW locks, condition variables, SEH, intrinsics (`_HAS_POPCNT`, `_HAS_LZCNT`, `_HAS_BYTESWAP`, …) |
| X | Headers | `_HAS_INTRIN_H`, `_HAS_IO_H`, `_HAS_PROCESS_H`, `_HAS_PSAPI_H`, `_HAS_SHELLAPI_H`, … |
| XI | Compiler-specific | `_HAS_DECLSPEC`, `_HAS_SAL` |
| XII | MinGW | MinGW detection |
| XIII | Lean-and-mean | exclusion macros (`WIN32_LEAN_AND_MEAN`-style) |

---

## `env_apple.h` — prefixes `D_ENV_APPLE_` (shared), `D_ENV_MACOS_` (macOS-only)

Shared Apple-platform detection plus macOS specifics. Included directly for macOS,
and transitively by `env_ios.h` for the mobile platforms.

| Section | Category | Representative symbols |
| --- | --- | --- |
| I | Platform identification | platform flags + name (macOS/iOS/tvOS/watchOS/visionOS) |
| II | macOS SDK / deployment | version constants + `__MAC_OS_X_VERSION_MIN_REQUIRED` target |
| III | Darwin kernel / XNU | Mach subsystem availability |
| IV | Toolchain | Apple Clang / Xcode identification |
| V | ObjC / Swift interop | Objective-C runtime, ARC, Swift interop |
| VI | Frameworks (shared) | core, security, networking, graphics/media, UI frameworks |
| VII | macOS-specific | App Sandbox, Hardened Runtime, system APIs (GCD, Keychain) |
| VIII | Hardware | Apple Silicon, Rosetta 2, Hypervisor.framework |
| IX | BSD/POSIX extensions | Darwin POSIX/BSD layer characteristics |
| X | Filesystem | macOS filesystem features |
| XI | Runtime functions | runtime detection helpers |
| XII | Convenience | `D_ENV_APPLE_IS_DESKTOP()`, `_IS_MOBILE()`, `_IS_MODERN()`, `_HAS_SECURE_RANDOM()`, `_HAS_GPU_API()`, `D_ENV_MACOS_IS_HARDENED()` |

---

## `env_bsd.h` — prefixes `D_ENV_BSD_` (common) + per-variant

Covers FreeBSD, OpenBSD, NetBSD, DragonFly BSD, and historical BSD/OS. Per-variant
prefixes: `D_ENV_FBSD_`, `D_ENV_OBSD_`, `D_ENV_NBSD_`, `D_ENV_DBSD_`.

| Section | Category | Representative symbols |
| --- | --- | --- |
| I | Variant identification | variant detection + name |
| II–V | Per-variant versions | FreeBSD / OpenBSD / NetBSD / DragonFly version constants + detected version |
| VI | BSD-common features | `_HAS_KQUEUE`, `_HAS_ARC4RANDOM`, `_HAS_ARC4RANDOM_BUF`, `_HAS_GETENTROPY`, `_HAS_EXPLICIT_BZERO`, `_HAS_CLOSEFROM`, sysctl |
| VII | Security frameworks | OpenBSD pledge/unveil, FreeBSD Capsicum (`_HAS_CAPSICUM`, `_HAS_CAP_ENTER`, `_HAS_CAP_RIGHTS`), jails (`_HAS_JAIL`), securelevel, pf |
| VIII | Networking | pf, CARP (`_HAS_CARP`), routing sockets, BPF (`_HAS_BPF`), sendfile, accept filters (`_HAS_ACCEPT_FILTER`), IPFW |
| IX | Filesystem | ZFS, HAMMER/HAMMER2 (`_HAS_HAMMER2`), FFS/UFS (`_HAS_FFS`), extattr, nullfs, tmpfs |
| X | Process / threading | `rfork`, `kinfo_proc` (`_HAS_KINFO_PROC`), cpuset (`_HAS_CPUSET`) |
| XI | Display server | X11, Wayland indicators |
| XII | Headers | BSD-specific header availability (`_HAS_DLFCN_H`, …) |

Also exposes `D_ENV_BSD_DEFAULT_COMPILER_NAME` / `_DEFAULT_IS_CLANG` /
`_DEFAULT_IS_GCC` for the variant's default toolchain.

---

## `env_ios.h` — prefixes `D_ENV_IOS_`, `_TVOS_`, `_WATCHOS_`, `_VISOS_`, `_MOBILE_`

Covers iOS/iPadOS, tvOS, watchOS, and visionOS. Includes `env_apple.h` for shared
Apple features.

| Section | Category | Representative symbols |
| --- | --- | --- |
| I–IV | SDK / deployment targets | iOS / tvOS / watchOS / visionOS version constants + detected targets |
| V | iOS/iPadOS frameworks | ARKit, CoreML, HealthKit/CoreMotion, notifications, MapKit |
| VI | Widgets / App Clips / extensions | WidgetKit, App Clip, extension support |
| VII | StoreKit | in-app purchase support |
| VIII | Device capabilities | compile-time heuristics (camera, GPS, haptics, LiDAR) |
| IX | Multitasking / windowing | multitasking and windowing capabilities |
| X | visionOS-specific | spatial-computing features |
| XI | App Store / distribution | distribution constraints |
| XII | Runtime functions | runtime detection helpers |
| XIII | Convenience | `D_ENV_IOS_IS_MODERN()`, `_HAS_SWIFTUI_FULL()`, `_HAS_MODERN_CONCURRENCY()`, `_HAS_SPATIAL_COMPUTING()`, `D_ENV_MOBILE_IS_SANDBOXED()`, `_IS_CONSTRAINED()` |
