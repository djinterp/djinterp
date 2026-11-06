# A stand-in for `cl`

> **Where this file is, and what its paths mean (lane F, 2026-11-05).**
> Restored after the restructure of 2026-11-03 left it out of the tree, as
> it stood on 2026.10.27, and kept as written. The folder above this one is
> now `build/cmake/config/djinterp/tools/vparse`; the window's build, which
> it calls `build/ui/vparse` (`../../../../ui/vparse` from there), is
> `build/cmake/config/ui/vparse` (`../../../ui/vparse`); and the tool's
> README is `docs/djinterp/tools/vparse/README.md`.

For a machine with no MSVC: Linux, WSL, CI. It is not part of the build, and
nothing in the build refers to it.

`cl` here is a script that takes `cl`'s command line and compiles with a
MinGW-w64 Clang for 64-bit Windows. In front of every unit it puts the
standard library, read under the real compiler's identity, and then
`identity.h`, which takes that identity away and puts `cl` 19.51's in its
place — `_MSC_VER`, `_MSC_FULL_VER`, `_MSVC_LANG` and the rest defined,
`__clang__` and `__GNUC__` gone.

Two things follow.

- **CMake identifies the compiler as MSVC 19.51**, and takes every branch of
  every list file that it takes for `cl`: the flags, the levels `cl` can and
  cannot select, uxoxo's own build, the suites' counts.
- **djinterp's `env` takes the branches it takes under `cl`**, in every unit,
  and what those branches select is compiled, linked against the MinGW-w64
  runtime, and run under Wine.

## Running it

It needs `clang` with `lld`, a MinGW-w64 GCC for 64-bit Windows (for its
headers and libraries: `x86_64-w64-mingw32-g++`), Ninja and Wine. From the
folder above this one, `build/cmake/djinterp/tools/vparse` in djinterp's tree,
vparse and its command line:

```bash
chmod +x cl_standin/cl cl_standin/link cl_standin/rc  # an archive drops the bits

export CL_STANDIN_LOG=$PWD/out/standin.units           # optional

cmake -S . -B out/standin -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=cl_standin/toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build out/standin
ctest --test-dir out/standin
```

and the window, from its own build, `build/ui/vparse`, which builds vparse
with it:

```bash
export CL_STANDIN_REAL_ROOTS=/path/to/uxoxo/inc/imgui  # Dear ImGui keeps its own

cmake -S ../../../../ui/vparse -B out/standin-ui -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=$PWD/cl_standin/toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DUXOXO_ROOT=/path/to/uxoxo
cmake --build out/standin-ui
ctest --test-dir out/standin-ui
```

Configure should begin `The C compiler identification is MSVC 19.51.36260.0`
and, in the window's build, report the window as Win32 + Direct3D 11. The
tests are the ones `cl` gets. vparse's build has 21: the tool's three (the
model's suite, the command line's, and `vparse run` over the example
grammar), djinterp's foundation suites at C++14, 17, 20 and 23 and its C
suites at C99, C11 and C17, vparse's at C++14 to 23, and the argument
parser's seven -- its C suites at C99, C11 and C17, and its C++ suites at
C++98, 14, 17 and 20; its allocation sweep is left out, as under `cl`,
whose linker has no `--wrap`. The window's has those and its own five, 26: the window's suite, its Dear ImGui suite, and three runs of
`vparse gui` -- its help, its version, and a grammar it cannot read --
which find the module, `vparse-gui-win64.dll`, and load it. `cl` has no
C++11 to select, so the leaves leave that level out.

The argument parser's C++ suites do not pass here (2026.10.27), and they
are not the tool's: they stop on their first instructions, at every level,
with a read of a low address that Wine reports as a page fault. Its C
suites pass in `C.UTF-8`. The window's
build with djinterp's suites left out (`-DVPARSE_BUILD_DJINTERP_TESTS=OFF`)
has the tool's eight, which pass.

`Ninja Multi-Config`, which the `msvc` preset uses, works the same way with
`--config` and `-C`.

The first program Wine runs creates its prefix (`~/.wine`). Tests started
together on a machine with none race to create it, and some fail; run
`wineboot -i` first, or run the tests once with `-j1`.

`CL_STANDIN_REAL_ROOTS` names directories, colon-separated, whose sources
keep the real identity. Dear ImGui's must: they include `<windows.h>`, which
the MinGW-w64 headers will not read as `cl`'s, and they include none of
djinterp, so nothing is lost. `CL_STANDIN_LOG` names a file that gets a line
for each unit: its language, its level, whether it was given
`/Zc:preprocessor`, and whether it kept the real identity. Every unit but
Dear ImGui's, CMake's own checks, and the kit's should say
`traditional=0 real=0`: the kit's module, `DjinterpCli.cmake`, builds
`djinterp::cli` and filesys under it with `/utf-8` alone, and its units say
`traditional=1 real=0`.

The toolchain confines what a list file finds to the MinGW-w64 sysroot, as
a cross toolchain does: uxoxo's build looks for GLFW, and the machine's own
GLFW is nothing a Windows unit can read.

## What a run shows, and what it cannot

It shows that the build is right for `cl` — that CMake, told it has `cl`,
configures what the tool's README (`src/djinterp/tools/vparse/README.md`)
says it does — and that the code `cl` is handed, along the branches `env`
takes for it, compiles and runs and passes.

It cannot show that `cl` accepts that code. The parser is Clang's, which is
stricter than `cl` in places and laxer in others; the warnings are Clang's,
not `/W4`'s; and the library is libstdc++ over the MinGW-w64 runtime, not
Microsoft's. That runtime is `msvcrt.dll`, where `cl`'s is the Universal CRT,
so a unit that takes `cl`'s branches may call a function `msvcrt.dll` lacks:
`ucrt.c` supplies each one met so far, to the Universal CRT's contract --
`_dupenv_s`, which djinterp's argument parser reads the environment with. A flag that tells `cl` how to read, check or emit is taken for
what it means to Clang or dropped. The first build with `cl` itself is still
the one that proves those.

One difference matters to the warnings in particular. CMake hands `cl` a
system include directory as `/external:I`, and `cl` judges a header external
by where it is; Clang judges one by how it was found. The workbench's model
headers sit in djinterp's `inc/`, which this build passes as a system
directory, and are included by relative path: under the stand-in they are
the workbench's own and are held to its warnings, under `cl` itself they are
read at `/external:W0`. GCC and Clang builds check them.

## The files

| file | what |
|---|---|
| `cl` | the driver: `cl`'s command line in, a MinGW-w64 Clang compile out. A flag it does not know stops it. It holds the lists of standard headers read before the swap; a header a unit includes and its list does not must be added, since the library does not compile as `cl`'s |
| `link` | links through `lld` against the MinGW-w64 runtime, statically, with `ucrt.c` compiled in; a system library named `foo.lib` becomes `-lfoo` |
| `ucrt.c` | what the Universal CRT has and `msvcrt.dll` does not, under the Universal CRT's names: `_dupenv_s` |
| `rc` | stands in for the resource compiler and the manifest tool, which CMake looks for and this build has no work for |
| `identity.h` | the change of identity |
| `toolchain.cmake` | puts the three scripts where `cl`, `link` and `rc` would be, confines what is found to the MinGW-w64 sysroot, has CMake's checks link through `link` as `cl`'s do, and tells ctest to run tests through Wine |
| `override.cmake` | the link and archive rules, in place of the ones CMake has for `link.exe` and `lib.exe` |
