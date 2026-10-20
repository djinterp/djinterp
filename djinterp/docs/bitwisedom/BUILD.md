## Building from the djinterp tree

Run the CMake presets from the bitwisedom build configuration directory
(not the djinterp repository root):

```cmd
cd build\cmake\djinterp\tools\bitwisedom
cmake --preset msvc
cmake --build --preset msvc-release
ctest --preset msvc-release
```

Build intermediates remain in `out/msvc/` under this directory.
Release binaries go to `<djinterp>/bin/tools/bitwisedom/`, and Debug
binaries to `<djinterp>/bin/tools/bitwisedom/debug/`, each named with
a platform tag (e.g. `bitwisedom-win64.exe`).
`DJINTERP_ROOT` defaults to the containing djinterp checkout; `UXOXO_ROOT`
still points to the separate uxoxo checkout and may be overridden.

# Building bitwis(e)dom

A superbuild in vparse's shape: one CMake project at bitwis(e)dom's root that
takes djinterp and uxoxo from their own roots by absolute path, and builds
the model, the generator, the window and every test.

It is written for the consolidated djinterp of 2026.10.07, with the owner's
edits since, and the uxoxo of 2026.10.14 with **Dear ImGui's docking
branch** in its `inc/imgui` -- the uxoxo overlay of 2026.10.18, which
replaces that directory and nothing else. The window's sheets and tool
windows dock, float and tab as Visual Studio's do, and those are the
docking branch's: ImGui 1.92.8 WIP, its `docking` branch at `dee5bf3e`.
Before 2026.10.18 it was written for the uxoxo of 2026.10.11 --
`uxoxo_2026_10_11b.zip`, whose blueprints share their drawing, so a tree of
a thousand rows is realized in linear time.

---

## Before the first build

1. **uxoxo's Dear ImGui is the docking branch** (2026.10.18): its
   `inc/imgui/imgui.h` defines `IMGUI_HAS_DOCK`, or configure refuses it by
   name. The uxoxo overlay of 2026.10.18 puts it there; nothing else of
   uxoxo changes, and uxoxo's own elements build on it as they did.
   **uxoxo is otherwise the updated one, of 2026.10.11 or later.**
   bitwis(e)dom's window uses
   uxoxo's number, combo, colour, tree, menu, section, table and splitter
   elements, element keys and targets, and uxoxo's own build at
   `build/cmake/CMakeLists.txt` -- since 2026.10.09, its popup element (the
   context menus), a tree node's rename in place and its right-click and
   double-click actions, and a text field that reports only when its edit
   is done -- and, since 2026.10.11, its window element (the bottom panel
   floating, or opened over the grid), the buttons at a tab bar's end, line
   splitters and flush columns. The uxoxo of 2026.10.09 lacks the window,
   and configure refuses it by name (`element_window.hpp is missing`), as
   it does the uxoxo of 2026.10.07, which lacks the popup as well, and the
   uxoxo uploaded before 2026.10.04, which lacks everything.
2. **djinterp is the consolidated tree of 2026.10.07.** bitwis(e)dom reads
   its headers -- the environment layer, the root's kit, and `re_std`'s
   integers and bit functions -- and, since 2026.10.09, compiles nine C
   sources of its parse layer: the C and C++ lexer, its literal decoder and
   the diagnostic sink they report into, with which it reads macro bodies,
   imported headers and environments. A djinterp without them is refused by
   name, with the files it is missing. Nothing else of djinterp is
   compiled, so the leftovers an unzip leaves in djinterp's tree do not
   concern it (vparse's `BUILD.md` has the two scripts that retire them).
3. **Off Windows, GLFW 3.3 or later** (`libglfw3-dev`, `brew install glfw`,
   `vcpkg install glfw3`). On Windows the window is Win32 + Direct3D 11 and
   needs nothing beyond the Windows SDK. Without GLFW off Windows,
   `-DBITWISEDOM_BUILD_APP=OFF` builds everything else.

---

## Quick start

On Windows, from a Developer Command Prompt (so `cl` is on `PATH`):

```
cmake --preset msvc
cmake --build --preset msvc-debug
ctest --preset msvc-debug
..\..\..\..\..\bin\tools\bitwisedom\debug\bitwisedom-win64.exe
```

`vs2022` makes a solution for the IDE instead; `clang-cl` and `strict`
(clang-cl, warnings as errors) are there too.

Elsewhere -- the defaults are Windows paths, so name the roots:

```
cmake --preset clang -DDJINTERP_ROOT=/path/to/djinterp \
                     -DUXOXO_ROOT=/path/to/uxoxo
cmake --build --preset clang-debug
ctest --preset clang-debug
../../../../../bin/tools/bitwisedom/debug/bitwisedom-linux64
```

`gcc` is the same with GCC; `clang-strict` is the loudest configuration;
`mingw-cross` builds a Windows `bitwisedom.exe` on Linux (see below).

---

## The three roots

| variable | default | must contain |
|---|---|---|
| `DJINTERP_ROOT` | detected from the containing djinterp checkout | `inc/djinterp/djinterp.hpp` |
| `UXOXO_ROOT` | `D:/sam/code/projects/frameworks/uxoxo/uxoxo-project` | `inc/uxoxo/element.hpp`, `build/cmake/CMakeLists.txt` |
| `BITWISEDOM_ROOT` | djinterp repository root | |

The djinterp root is detected from the relocated CMake directory; the uxoxo
path is the existing project default. Each is checked at configure time, and a wrong
one is one message naming the variable to set.

---

## Language levels

bitwis(e)dom's floor is **C++17**, the floor of the uxoxo and djinterp headers
it stands on (djinterp's free monad and option record). `BITWISEDOM_CXX_STANDARD`
picks 17, 20 or 23 for the build, default 20, and passes it to uxoxo as
`UXOXO_CXX_STANDARD`. Below the floor every bitwis(e)dom header is empty rather
than an error, as djinterp's are below theirs.

The model's suite is built at every level from the floor the compiler can
select, whatever the build's own level is.

---

## Flags

bitwis(e)dom's own sources -- the model, the generator, the window, the tests --
are held to the style guide's whole baseline: `-Wall -Wextra -Wshadow
-Wcast-qual -Wconversion` under `-pedantic-errors` and `-Werror=undef`, and
under Clang `-Wreserved-identifier` and `-Wdocumentation` as well (under `cl`,
`/W4`). `BITWISEDOM_WARNINGS_AS_ERRORS=ON` makes them errors.

The dependencies' headers -- djinterp's, uxoxo's, Dear ImGui's, GLFW's -- are
system headers to bitwis(e)dom's targets, so their warnings are not counted
against it. uxoxo builds itself with only its level and `/utf-8`. djinterp's
lexer is compiled as C99, djinterp's floor, with no extensions and no
warnings asked for -- under `cl`, with `/utf-8` and the conforming
preprocessor.

djinterp's lexer headers have no `extern "C"` guards of their own, so
bitwis(e)dom includes them inside one (`src/clex.cpp`), and they are read
nowhere else. That is a gap in djinterp worth closing there: a C++ unit that
includes them bare links against names it cannot find.

Under a `cl`-style driver: `/utf-8` (the trees are UTF-8 without a
byte-order mark), `/Zc:__cplusplus`, `/Zc:preprocessor`, `/permissive-`,
`/EHsc`, `/bigobj` (the interface's units hold enough lambdas to run a Debug
object past `cl`'s section limit), and `_CRT_SECURE_NO_WARNINGS` for
`std::localtime`.

---

## Targets

| target | is |
|---|---|
| `bitwisedom::core` | the model: sheets, resolution, expressions, operations, the C/C++ generator, the define environment (expansion and typed evaluation of C), the importer of C and C++, renames, the file format, undo -- no GUI |
| `djinterp::lex` | djinterp's C and C++ lexer, compiled as C99 for the core |
| `bitwisedom-gen` | the generator on the command line |
| `bitwisedom` | the window: Win32 + Direct3D 11 on Windows, GLFW + OpenGL 3 elsewhere |

`BITWISEDOM_HOST` (`auto`, `win32`, `glfw`) chooses the window's backend;
`auto` is `win32` on Windows. GLFW on Windows works too, where uxoxo's build
finds it.

The two fonts -- Roboto Medium for the interface, Cousine for values and code,
both Apache 2.0 -- are compiled in, from the copies uxoxo's ImGui carries, as
byte arrays: ImGui's base85 form is one string literal, past `cl`'s 64 KB
limit (C2026).

---

## Tests

| test | checks |
|---|---|
| `model_c++17`, `model_c++20`, `model_c++23` | the model's suite: 36 sections, from bit masks to undo, every group opened and closed at once, values wider than an integer and the headers that hold them in arrays, the colour schemes, the JSON a bitfield is read from and every example written as one and as an export and read back, and the C environment, the importer and renames -- the code attached on 2026.10.09 read, evaluated against GCC's own answers, exported in the style guide's layout and read back the same -- and File > New's document, square cells, and what the symbols column lists rows by and how it writes their values, kept in the file; and the order a sheet's rows are shown in -- the sheet's own, or each group's sorted by a key, either way, the header in the sheet's order still -- and what each cell shows, as the display rules over its rows and bits say, kept in the file, and a rule whose rows are all deleted gone with them; since 2026.10.19, the rows' share of the cells' width, the probe and usage rows -- one of each, another added, moved, pinned, removed and the last hidden, anchors to rows gone let go of, kept in the file -- and a definition file's lines the preprocessor cannot read, named by their lines |
| `header_<example>_<level>_<form>` | each of the twenty-one examples' headers, written by `bitwisedom-gen` with a self-test, compiled at that level with `-Wall -Wextra -pedantic-errors -Werror`; the self-test does not compile unless every macro has the sheet's value and every `_GET`/`_SET` agrees with the named values. A sheet wider than an integer -- `pci_config_header`, `x86_cpuid`, and `tests/sheets/wide_mix`, a bitfield in JSON made to be hard: fields across every word size's boundaries, values in every encoding, a range no integer carries -- has its self-test linked and run as well, which checks its arrays and the macros that read and write them; and again, as `header_<sheet>_w<bits>_...`, with arrays of 8-, 16- and 64-bit words at C11 and C++17 |
| `bitfield_<file>` | each large example's file in `res/djinterp/tools/bitwisedom/examples/large`, and `wide_mix`, read by `bitwisedom-gen` and written back as a bitfield: the same bytes, line ends aside -- nothing of it lost on the way in, and the script that made it laying it out as the writer does |
| `platform_<example>` | the examples of the platform the build targets -- Windows, macOS, Linux or FreeBSD -- each generated and compiled beside the system's own headers (`tests/platform/<example>.c`), every value the system also defines compared at compile time; on Windows, `win32_hresults` too, its 1,376 codes and 41 facilities against `winerror.h`; on Linux, `pci_config_header`, against `linux/pci_regs.h`; and where GCC or Clang targets x86, `x86_cpuid`, each flag's mask against `<cpuid.h>` under every name GCC's and Clang's give it |
| `ui_smoke` | the window through ImGui's null backend: every tab of every example, each arriving unchanged with its columns fitted to its names and values, every row in the inspector, every setting changed and undone, an operation applied and undone, a file saved and opened, and 600 frames of a mouse that wanders, clicks and drags; then, with the mouse and keyboard where the grid says it drew things, the context menus, renames in the grid, the tree, the inspector and a segment's heading, the columns' edges and the ruler's boundaries dragged and double-clicked, and code pasted, copied, imported and dropped; the grid filling its panel, the bottom panel shown every way, the theme and colour scheme, what a resting mouse shows, a sheet wider than 64 bits, the 1,424 rows of the HRESULT example with every group opened, rows selected across it and one renamed, its groups opened and closed all at once -- by the events the buttons and menus post, by both shortcuts and from a group's own menu -- and the JSON shapes written beside the sheet and read back, dropped and pasted; a new sheet and its add row's "Add new...", children named in the tree's "Add child..." fields, square cells, the menu bar's Help at its right end and its two pages, the buttons over the symbols column -- pressed, undone, fitted to, rested on, dragged from, narrowed and hidden -- the values written every way a press on value takes, lined up, the list a right-click on a button opens, and a macro with no bits written over its cells, and text that reads on every colour in every theme and scheme; and since 2026.10.18 the window around the sheets: the tool windows opened, closed by their x and docked, several sheets at once, workspaces added, switched, renamed and closed and a sheet moved between them, the sheets arranged tabbed, in columns, rows and a grid, each window zoomed, the session kept and opened again as it was -- in a second ImGui, as the next run -- the Options, every page, their search and the settings files exported, applied, forgotten and deleted; the grid's symbols column as the tree; the add control as a button and its menu and as a split button, driven by the mouse, and the colour new rows take; the rows sorted from the heading's menu and the sort made the sheet's own; and digits, names and colours shown in parts of the grid only, by rows, bits and a block, and the Display window; and since 2026.10.19 cells sized by hand and proportional rows, the probe and usage rows dragged by the mouse among the rows and into a group -- whose usage row then counts the group's -- pinned from their menu, added and removed, the Add child field fitted and filling, alone or with its +, the + opening the Add node window as a tab or a window and a range made and added there, the status bar and the console a click on its message opens, a definition file edited, and the sheet's tab over its bits, renamed by a double-click -- 44 sections, every frame checked for ImGui assertions, for ImGui's own errors, for clashing widget IDs, and for vertices a GPU may draw as stray pixels |

With GCC or Clang there are 379 header tests -- twenty-one examples and
`wide_mix` at C99, C11, C17, C23, and C++98 to C++23 as macros and as
constants, and the three wide sheets' 27 with other words -- three
bitfields read and written back, and two to five platform checks: 391
tests in all on x86 Linux, 389 on x86 FreeBSD and an Intel Mac, 388 on an
Apple-silicon one. Where `long` has 32 bits -- Windows, and every 32-bit
target -- C++98, which has no `long long`, cannot hold the 64-bit constants
of two examples and of `wide_mix`, and those six tests are left out:
MinGW-w64 runs 384. Under `cl`, which cannot select C99, C23, C++98 or
C++11, there are 247 header tests and no CPUID check, and 257 in all.

The platform checks are compiled without `-Werror` or `/WX`, since the
system's headers are not this project's to keep clean, and at `-std=gnu11`
(or `/std:c11`) so that the BSD and Linux headers show what they hide from
strict C. A check fails on a value that disagrees; the example's own header
is held to every warning by the header tests.

`ui_smoke` compiles its own ImGui with every ID clash recorded
(`IMGUI_DEBUG_HIGHLIGHT_ALL_ID_CONFLICTS`); without that, ImGui notices a
clash only under the mouse. ImGui 1.92.8 in that mode flags its own
`InputTextMultiline` every frame -- the widget submits its ID as an item and
again when its child window ends -- so a clash whose ID is a child window's is
passed over. A real one is still caught.

---

## After unzipping an overlay

An overlay unzipped over the trees keeps the times its archive holds, and
those can be older than the objects of the last build. A build that went
by the times alone would take the old objects for the new sources, and the
tests would run the code of the overlay before: `model_c++17`, `_20`,
`_23` and `ui_smoke` then fail, since each overlay that adds sections
raises the count they must print (`passed: 36`, `passed: 44`) while the
old programs print the old one.

So **configure after unzipping** -- `cmake --preset msvc`, as the quick
start does -- and then build. Each configure keeps a hash of every source
it watches, bitwisedom's and uxoxo's with its Dear ImGui, in the build tree
(`bitwisedom_sources.sha1`), and touches each whose content changed since
the last configure, so the build compiles it again
(`cmake/Freshness.cmake`); it says how many it marked. A build tree
configured before it kept any hashes has every source marked once, and
compiles them all. Only the times of changed files are changed. Where a
file cannot be touched, configure names it: build with `--clean-first`
(`cmake --build --preset msvc-debug --clean-first`), which a build tree in
any doubt can do as well.

---

## If the window draws stray pixels

`bitwisedom-win64.exe --warp` draws with WARP, Microsoft's software
rasterizer, instead of the GPU, and says so in its title. Stray pixels on
the GPU that WARP does not draw are the GPU driver's; pixels WARP draws too
are the drawing's. `ui_smoke` checks every frame for what a GPU may draw as
stray pixels -- a vertex that is not a number, one far past any window, or
one outside the font atlas -- and the grid's lines are cut to what the
window shows: a line across a sheet of 65,536 bits once ran a million
pixels past it.

---

## How uxoxo is found

`cmake/Uxoxo.cmake` adds `${UXOXO_ROOT}/build/cmake` as a subproject. uxoxo's
build takes djinterp from the `djinterp::headers` target `cmake/Djinterp.cmake`
defines, and gives back `uxoxo::uxoxo`, `uxoxo::imgui`, and whichever backends
it can build: `uxoxo::imgui_win32_dx11` on Windows, and
`uxoxo::imgui_glfw_opengl3` where it finds GLFW. uxoxo's own tests stay off
unless `BITWISEDOM_BUILD_UXOXO_TESTS=ON`.

uxoxo's Dear ImGui must be the docking branch (`IMGUI_HAS_DOCK`), or
configure stops. Its `imgui.cpp` and `imgui_internal.h` carry, since
2026.10.19, a patch of uxoxo's -- `ImGuiDockNode::TabBarInsetLeft`, a dock
node's tab bar begun inset, marked `[uxoxo]` and announced by
`IMGUI_DOCK_TAB_BAR_INSET` -- so a sheet's tabs can begin past its symbols
column; without it, configure warns, and the tabs run across the whole
width, as Dear ImGui's own do.

---

## Cross-compiling for Windows

`cmake/toolchains/mingw-w64-x86_64.cmake` builds a self-contained Windows
`bitwisedom.exe` and `bitwisedom-gen.exe` on Linux with MinGW-w64 (the posix
thread model):

```
cmake --preset mingw-cross -DDJINTERP_ROOT=... -DUXOXO_ROOT=...
cmake --build --preset mingw-cross-release
```

The toolchain keeps CMake's package searches and pkg-config inside the
MinGW sysroot; without that, the build machine's GLFW is found and offered for
a Windows target.

---

## What is here

```
CMakeLists.txt          the superbuild
CMakePresets.json       the presets above
cmake/                  ProjectRoots, CompilerSetup, Djinterp, Uxoxo,
                        EmbedExamples (res/djinterp/tools/bitwisedom/examples/large, built in),
                        toolchains/mingw-w64-x86_64
inc/bitwisedom/         the model's headers, in inc/ as djinterp's and
                        uxoxo's are
src/                    the model's sources
app/                    the window: the reducer (app), the interface as uxoxo
                        templates (view), the shell of workspaces and tool
                        windows (shell), the grid, flow and code views,
                        the controls uxoxo has no element for -- the add
                        control, the tree's "Add child..." fields, the
                        menu bar's right end, the dock space and its
                        windows (controls) -- the settings registries and
                        settings files, the themes, the two hosts, and
                        the compiled-in fonts
tools/                  bitwisedom-gen; the scripts that made the large
                        examples and their platform checks
                        (winerror_to_bitfield.py, x86_cpuid_bitfield.py),
                        and bitfield_json.py, the writer's layout for them
tests/                  the suites, the header, bitfield and platform
                        checks (platform/), and the sheet the wide header
                        tests try the generator with (sheets/)
examples/               the nineteen example sheets, and large/, the two
                        large examples as bitfields
docs/bitwisedom/         json.md, the JSON shapes key by key, and
                        bitfield.schema.json; screenshots/, the README's
                        pictures
```

---

## What was verified, and what was not

Later still on 2026.10.19, after the cells were sized by hand and the
rows made four fifths of a cell's width, the probe and usage rows moved
among the rows, the Add child field given its + and the Add node window,
the status bar and its console, a definition file made editable, the
sheet's tabs set over its bits and renamed in place, and the inspector made
one form; against the djinterp of 2026.10.18 and uxoxo's Dear ImGui of the
docking branch with its tab bar patch:

| | result |
|---|---|
| GCC 13.3, Linux, Debug, C++20, warnings as errors | every target built; 391 of 391 tests, the model's 36 sections and the smoke test's 44 among them |
| Clang 18.1, Release, warnings as errors | every target built, no warning; 391 of 391 tests |
| GCC 13.3, Debug, libstdc++'s checked containers and iterators (`_GLIBCXX_DEBUG`) | the model's suite at every level, 36 of 36 sections; the smoke test, 44 of 44, no ImGui error, ID clash or stray vertex |
| fresh trees: the djinterp of 2026.10.18 with the overlay of 2026.10.19 that cut the grid's lines, and a fresh uxoxo with the docking overlay, built; then this overlay and the ImGui patch unzipped over them, configured and built, GCC 13.3, Debug | configure marked the 20 sources that changed, Dear ImGui's two among them; the build compiled them and what includes them; 391 of 391 tests |
| the window's sources against a Dear ImGui without the tab bar patch, GCC 13.3, warnings as errors | every unit compiled clean (`-fsyntax-only`): without the patch the tabs run across the whole width |
| MinGW-w64 GCC 13, cross, Release, warnings as errors | every target built: `bitwisedom-win64.exe`, with its Win32 host, the Open dialog's Explorer picker and the patched docking branch, and `bitwisedom-gen-win64.exe`; not run, with no Wine here |

The smoke test drives the new parts as a user would: the probe and usage
rows dragged by the mouse onto a group's name and a symbol's, the + under a
group's rows clicked, the status bar's message clicked, and a sheet's tab
double-clicked and a name typed over it. The window was photographed on a
virtual display: the rows a fifth shorter than the cells are wide, cells
sized by hand, the lines stopping at the rows and running on, the probe
pinned at the top and a usage row counting a group's bits, the Add node
window as a tab and as a window, the console, a definition file underlined
as a view and as an editor, and the tabs over the bits; the README's first
picture was taken again, and two added.

On 2026.10.19, a build under MSVC reported four of its 257 tests failed:
the model's suite at its three levels, and `ui_smoke` -- the four whose
counts the overlay of 2026.10.18 raised -- and every other passed. That is
the old programs, run against the new counts: the overlay's files kept
their archive's times, older than the objects of the build before, and
the build compiled none of them. The same was done here on fresh trees --
the overlay of the afternoon of 2026.10.18 built and tested, then the
overlay of the evening unzipped over it, configured and built -- and the
same four failed, the build having compiled nothing it changed. Configure
now marks every source whose content changed since the last configure to
compile again ([After unzipping an overlay](#after-unzipping-an-overlay)),
and with it the same trees built and passed every test:

| | result |
|---|---|
| the overlay of 2026.10.18's afternoon, built, then the evening's unzipped over it, configured and built, GCC 13.3, Debug | the build compiled only the one file the overlay added; 387 of 391 tests -- `model_c++17`, `_20`, `_23` and `ui_smoke` failed, printing `passed: 33` and `passed: 30`, the counts of the overlay before |
| then this overlay unzipped over that, configured and built | configure marked all 168 sources; the build compiled every one, Dear ImGui's too; 391 of 391 tests |
| GCC 13.3, Linux, Debug, an old-dated change to one source: built, configured, built | the first build compiled nothing; configure marked the one source; the second build compiled it |

Later that day the window was reported drawing stray white pixels on
Windows. On Linux, through OpenGL on a virtual display, at scales of 1, 1.25
and 1.5 and zoomed, no frame differed from the next and none had a stray
pixel. Every frame of the smoke test was then searched for what a GPU may
draw as stray pixels, and frames of a sheet of 65,536 bits had them: its
grid lines ran 1.3 million pixels past the window. The lines are now cut to
what the window shows, the smoke test checks every frame for such
vertices, and was seen to fail with the lines uncut; and the Windows host
takes `--warp`, to tell a GPU driver's stray pixels from the drawing's.
GCC Debug and Clang Release: 391 of 391 tests; MinGW-w64: every target
built.

In the evening of 2026.10.18, after the window took Visual Studio's shape
-- each sheet in a window of its own, tool windows that dock, float, tab
and close, workspaces and the Window menu's arrangements, a zoom for each
window, the session kept, the Options and settings files -- and after the
symbols tree became the grid's symbols column, one add control took the
add buttons' place, new rows took the colour the user chooses, and the
rows were sorted and the display rules added; against the djinterp of that
day and the uxoxo of 2026.10.14 with Dear ImGui's docking branch:

| | result |
|---|---|
| GCC 13.3, Linux, Debug, C++20, warnings as errors | every target built; 391 of 391 tests, the model's 34 sections and the smoke test's 39 among them |
| Clang 18.1, Release, warnings as errors | every target built, no warning under `-Wdocumentation` and `-Wreserved-identifier`; 391 of 391 tests |
| GCC 13.3, Debug, libstdc++'s checked containers and iterators (`_GLIBCXX_DEBUG`), as MSVC's debug library checks them | the model's suite at every level, 34 of 34 sections; the smoke test, 39 of 39 |
| GCC 13.3, the window's sources at C++17, warnings as errors | every unit of the window compiled clean (`-fsyntax-only`) |
| the overlay unpacked over a fresh djinterp, and the ImGui overlay over a fresh uxoxo of 2026.10.14, GCC 13.3, Debug, warnings as errors | every target built; 391 of 391 tests |
| MinGW-w64 GCC 13, cross, Release, warnings as errors | every target built: `bitwisedom-win64.exe`, its Win32 host and Direct3D 11 backend on the docking branch, and `bitwisedom-gen-win64.exe`; not run, with no Wine here |

The smoke test drives the new window as a user would: the add control's
button, its split face and arrow, and its menu's colours by the mouse, at
the places the control says it drew them; the heading's menu, a row's and
a band's by right-clicks; a block selected; and a session written by one
window and opened by another, in an ImGui of its own. Its sections on the
sort and the display rules were seen to fail with what they check taken
away -- the rows shown in the sheet's order whatever the sort, and the
grid's digits drawn as its own setting says whatever the rules over a row
say. Rows added with no colour showed nothing where their bits were set,
with the digits off, as a row with no fill always had: their set bits are
now drawn plain, in the theme's text, faintly -- but a range's, which its
name across its cells already says. The window was photographed on a
virtual display: two sheets in columns, a sheet sorted with its display
rules and the Display window, rows added with no colour in both themes,
the heading's menu and a row's; the README's first picture was taken
again, and two new ones added. The rest of its pictures show the window
before it docked. `cl` and a real Windows machine were not tried.

Later on 2026.10.18, after the buttons over the symbols column -- symbol
name, name, value -- were added, then a press on value again writing the
values every way, lined up, a right-click listing every way, and a row
with no bits given its cells and its definition over them, with the same
djinterp and uxoxo:

| | result |
|---|---|
| GCC 13.3, Linux, Debug, C++20, warnings as errors | every target built; 391 of 391 tests, the model's 33 sections and the smoke test's 30 among them |
| Clang 18.1, Release, warnings as errors | every target built, no warning under `-Wdocumentation` and `-Wreserved-identifier`; 391 of 391 tests |
| GCC 13.3, the window's sources at C++17, warnings as errors | the grid, the view, the settings and the app compiled clean (`-fsyntax-only`) |
| the overlay unpacked over a fresh djinterp, GCC 13.3, Debug | every target built; 391 of 391 tests |

Clang alone found the buttons' tooltip text, two literals joined in an
array, suspicious (`-Wstring-concatenation`); it is parenthesized. The two
new smoke sections were seen to fail with what they check taken away -- a
column fitted to its names whatever it lists, a press on a button that
also drags the heading, a heading that leaves no line for the label below
the buttons, values that follow the tree in every form, a wide sheet that
does not pass over decimal and binary. Presses on the value button one
after another are one step of undo, as every run of one setting's changes
is. The window was photographed on a virtual display listing `uart_cr` by
each of the three, by value in every form at one width, in both themes,
with the value column shown and first, with no bands and no ruler, at
three widths of the column, with the list a right-click opens, and with
`IS_ENABLED`, a macro that takes an argument, selected. MinGW-w64, `cl`
and a real Windows machine were not tried, and the README's pictures were
not taken again.

On 2026.10.18, after File > New's root lost its fill and gained a bit, the
add buttons' "Add new...", the tree's "Add child..." fields, the menu
bar's indent, Help and View > Theme, square cells, and text that reads on
every fill, against the djinterp of that day and the uxoxo of 2026.10.14:

| | result |
|---|---|
| GCC 13.3, Linux, Debug, C++20, warnings as errors | every target built; 391 of 391 tests, the model's 32 sections and the smoke test's 28 among them |
| Clang 18.1, Release, warnings as errors | every target built, no warning under `-Wdocumentation` and `-Wreserved-identifier`; 391 of 391 tests |
| GCC 13.3, the window's sources at C++17, warnings as errors | every unit the change touched compiled clean (`-fsyntax-only`) |

The tree that came with the request did not build with warnings as errors:
a local `last` in the grid's rectangle drag shadowed the band click's, and
the toolbar the menu bar replaced was no longer called; and Clang found an
`int` given to `format_dec` in the main editor's tab names. Each is
mended. Each new smoke section was seen to fail with what it checks taken
away -- a field that does not take the keyboard back, an add row that
answers its "+" alone. The window was run on a virtual display and the
new sheet, every fill and colour in both themes with and without cell
boxes, the Macros tab, the View and Help menus and Help's two pages were
looked at; names were typed into an "Add child..." field one after
another. MinGW-w64, `cl` and a real Windows machine were not tried this
time, and the README's pictures were not taken again.

The record of later still on 2026.10.11 follows.

Later still on 2026.10.11, after Expand all and Collapse all were added,
with the same djinterp and uxoxo:

| | result |
|---|---|
| GCC 13.3, Linux, Debug, C++20, warnings as errors | every target built; 391 of 391 tests, the model's 30 sections and the smoke test's 23 among them |
| Clang 18.1, Release | every target built; 391 of 391 tests |
| GCC 13.3, the window and the model at C++17, Release | every target built; 391 of 391 tests |
| MinGW-w64 GCC 13, cross, warnings as errors | every target built; 384 of 384 run under Wine 9.0 -- one, `header_x86_cpuid_cpp11_macros`, after a second run: in the first, Wine's client lost its server (`recvmsg: Connection reset by peer`) before `bitwisedom-gen` started |
| the whole build configured for `cl` 19.51 through vparse's `tools/cl_standin/` | every unit compiled, no warning; both executables linked; 257 of 257 tests run under Wine |

The smoke test's new section takes under three seconds of its forty in a
debug build. The record of earlier on 2026.10.11 follows.

Late on 2026.10.11, after the two large examples, the writer's new layout
and the work for a thousand rows were added, with the same djinterp and the
uxoxo of `uxoxo_2026_10_11b.zip`:

| | result |
|---|---|
| GCC 13.3, Linux, Debug, C++20, warnings as errors | every target built; 391 of 391 tests, `x86_cpuid` checked against GCC's `<cpuid.h>` among them |
| Clang 18.1, Release | every target built; 391 of 391 tests, `x86_cpuid` against Clang's `<cpuid.h>` |
| GCC 13.3, the window and the model at C++17, Release | every target built; 391 of 391 tests |
| MinGW-w64 GCC 13, cross, warnings as errors | every target built; 384 of 384 run under Wine 9.0, `win32_hresults` against MinGW-w64's `winerror.h` and `x86_cpuid` against its `<cpuid.h>` among them |
| the whole build configured for `cl` 19.51 through vparse's `tools/cl_standin/` | every unit compiled, no warning; both executables linked; 257 of 257 tests run under Wine, `platform_win32_hresults` compiled `cl`-style with the real identity |
| uxoxo's own smoke test, after its blueprints were shared | `passed: 20   failed: 0` with GCC 13.3 at C++20 and Clang 18.1 at C++17 |

**The HRESULTs** are MinGW-w64's `winerror.h`, read by
`tools/winerror_to_bitfield.py`. Its 1,376 codes were compared with the
Windows SDK's as the `windows-sys` crate 0.59 carries them -- generated from
the SDK's own metadata: the 1,339 it has agree, every one, and so do its
facilities; the 37 it lacks -- DXGI's errors, four of DirectWrite's and
`TBS_E_DEACTIVATED` -- are as Microsoft documents them. The SDK's
`winerror.h` itself has not been compiled against here: on Windows,
`platform_win32_hresults` does that, with `cl`. A facility the SDK names
and MinGW-w64 does not -- NAP, TPM_SERVICES, FWP, and the DirectX and
imaging ones -- is named as the SDK names it, its value compared where the
header defines it.

**The CPUID flags** are the SDM's and the APM's, from the table in
`tools/x86_cpuid_bitfield.py`. Each was compared with Linux's
`arch/x86/include/asm/cpufeatures.h` (the 195 of them it has in these ten
registers), the kernel's `tools/arch/x86/kcpuid/cpuid.csv` (204, and
MAWAU's five bits) and KVM's tables (leaf 7.1's EDX): every one agrees.
`USER_MSR`, leaf 7.1 EDX bit 15, is the SDM's alone. GCC 13's and Clang
18's `<cpuid.h>` agree with each other on every mask but `bit_PKU`, which
Clang's has at bit 2, UMIP's, where the SDM, Linux and GCC have bit 3; the
check passes it over under Clang. The example's header was compiled and
its self-test run at every level, and with arrays of 8-, 16- and 64-bit
words.

**The writer's layout**: every example's sheet file was written again, and
each reads back as the same JSON, value for value. Each large example's file
is the bitfield `bitwisedom-gen --bitfield` writes back of it, byte for
byte, as is `wide_mix` rewritten; all three are valid against
`docs/bitwisedom/bitfield.schema.json` (`jsonschema` 4.26).

**A thousand rows.** The window was timed headless, through ImGui's null
backend, on the HRESULT example -- 1,424 rows -- a click that selects a row
being the frame after it, in which the window's elements are built again:

| | a click, release | a frame, release | a click, debug | a frame, debug |
|---|---|---|---|---|
| before, every group open | 95 ms | 2.4 ms | 569 ms | 15 ms |
| uxoxo's blueprints shared | 30 ms | 2.4 ms | 225 ms | 14 ms |
| and the tree as it is now, every group open | 23 ms | 2.5 ms | 209 ms | 13 ms |
| as it arrives, its facilities closed | 2.8 ms | 0.5 ms | 28 ms | 3.1 ms |
| `uart_cr`, for scale | 1.9 ms | 0.3 ms | 14 ms | 2.1 ms |

Opening the example takes 28 ms (release) and 142 ms (debug); opening its
groups one by one, 22 ms and 154 ms each. Before the grid
found its rows by an index, each frame walked the tree for each row --
with 1,424 rows, its largest cost. Debug is GCC's `-O0`; MSVC's debug
library checks its iterators, and will be slower. `ui_smoke` takes 26
seconds in a debug build with its new section, from 43 with the large
examples and none of this.

The record of the evening of 2026.10.11 follows.

In the evening of 2026.10.11, after the JSON shapes -- the bitfield and the
export -- were added, with the same djinterp and uxoxo:

| | result |
|---|---|
| GCC 13.3, Linux, Debug, C++20, warnings as errors | every target built; 346 of 346 tests |
| Clang 18.1, Release | every target built; 346 of 346 tests |
| GCC 13.3, the window and the model at C++17, Release | every target built; 346 of 346 tests |
| MinGW-w64 GCC 13, cross, warnings as errors | every target built; 338 of 338 run under Wine 9.0 |
| the whole build configured for `cl` 19.51 through vparse's `tools/cl_standin/` | every unit compiled, no warning; both executables linked; 224 of 224 tests run under Wine |

Every example was written by `bitwisedom-gen` as a bitfield and as an
export, at C99 and C++17, as macros and as constants, and each was read back
and generated again: the bitfield wrote the same header, byte for byte, and
the export read back as the same sheet -- 76 headers each way. The model's
tests do the same for every example, at C99. The wide header tests read
`wide_mix` from a bitfield now, so the generator reads one at every level
and word size, and each of those 50 self-tests ran.

Every bitfield written -- the nineteen examples' and `wide_mix` -- was
checked against `docs/bitwisedom/bitfield.schema.json` with Python's `jsonschema` 4.26,
and the schema was seen to refuse a misspelt key and a row with two
definitions. The examples in `docs/bitwisedom/json.md` and the README were read by
`bitwisedom-gen`, and the messages they quote are the program's.

The record of the afternoon of 2026.10.11 follows.

Later on 2026.10.11, after sheets wider than 64 bits were added, with the
same djinterp and uxoxo:

| | result |
|---|---|
| GCC 13.3, Linux, Debug, C++20, warnings as errors | every target built; 346 of 346 tests, `pci_config_header` checked against `linux/pci_regs.h` among them |
| Clang 18.1, Release | every target built; 346 of 346 tests |
| GCC 13.3, the window and the model at C++17, Release | every target built; 346 of 346 tests |
| MinGW-w64 GCC 13, cross, warnings as errors | every target built; 338 of 338 run under Wine 9.0, the wide sheets' self-tests run as Windows programs |
| the whole build configured for `cl` 19.51 through vparse's `tools/cl_standin/` | every unit compiled, no warning; both executables linked; 224 of 224 tests run under Wine, the 38 wide self-tests linked `cl`-style and run |

A wide sheet's self-test checks, for `pci_config_header` at C99, 294 things
as it compiles and 434 as it runs; for `wide_mix`, 71 and 118. Before they
were trusted they were tried against headers with faults put in by hand --
a word of an initializer, the array macros' modulus, a field's `_SET` shift
and its `_GET` mask, a value, a bit's place -- and each fault failed them,
as they compiled or as they ran. The `_GET` mask's at first did not: the
one pattern a field was written with left its top bit clear, so a mask a
bit short read it back the same. A field is now written with a pattern and
its complement. A self-test whose run failed also said "the self-test
compiled", which is how CTest knew a header test had passed; it now says
only that the run failed.

The generator's narrow output was compared with that of the generator
before this change: 576 headers -- the eighteen narrow examples at four C
and four C++ levels, as macros and as constants, in both styles -- byte for
byte the same, but for one line of `win32_hresult` that ran to 81 columns.
A body too long for its line is now broken inside nested parentheses as
well as at its outermost operators, which keeps that line within 80.

A sheet of 65,536 bits -- which `ui_smoke` makes when it sets every setting
to its highest -- took that section 573 seconds in a debug build: the grid
fitted each value's 16,384 hex digits to its column a character at a time.
It now halves to the fit and names a wide sheet's values by their runs; the
section takes 10. And the operations' preview coloured red every bit two
symbols claimed, in whatever groups; it now colours the collisions it
counts, two claims in an exclusive group, as the usage row does -- which
the PCI example, whose registers lie under their own flags, made plain.

The record of the morning of 2026.10.11 follows.

On 2026.10.11, after the window's layout was redone -- the panels flush and
divided by hairlines, a grid that fills its panel, the bottom panel's five
ways of being shown, the Theme menu with its five colour schemes, and what
a resting mouse shows -- against djinterp of 2026.10.07 and uxoxo of
2026.10.11:

| | result |
|---|---|
| GCC 13.3, Linux, Release, C++20, warnings as errors | every target built; 295 of 295 tests |
| Clang 18.1, the same | every target built; 295 of 295 tests |
| GCC 13.3, the window and the model at C++17, warnings as errors | every target built; 295 of 295 tests |
| MinGW-w64 GCC 13, cross, warnings as errors | every target built; 290 of 290 run under Wine 9.0 |
| the whole build configured for `cl` 19.51 through vparse's `tools/cl_standin/` | every unit compiled, no warning; both executables linked; 186 of 186 tests run under Wine |

`ui_smoke` has four more sections, 19 in all. One opens `win32_hresult`
and checks that its cells widen to the grid panel's edge; that with the
room given to the name column instead, that column widens to the edge; that
with the room given to nothing, the grid ends where its columns do; and
that the choice undoes. One takes the bottom panel through its ways of
being shown: minimized, and docked again at a tab pressed on its strip;
opened over the grid by the mouse resting on its strip, kept open while the
mouse is on the page, and closed once the mouse is away from both; floating
in its own window, and hidden by that window's close button; brought back
as it was by Ctrl+J, and by a page asked for. One changes the theme and the
scheme and checks that the window and the grid change and the sheet does
not, that both go to the settings file, and that a palette the sheet
chooses stays in either theme until it is given back. One rests the mouse
on a name, a band and the ruler, and finds a tooltip under each. The
model's tests check that a sheet's colours are automatic until it sets one,
that rows take their colours from the scheme's palette, that a colour the
sheet sets stays in every theme and scheme and is the only colour its file
names, and that a file saved with every colour, as before, reads with its
defaults automatic and its own choices kept.

The README's pictures were taken again from this build, on a virtual
display, the File menu's by synthetic mouse input.

The record of 2026.10.10 follows.

On 2026.10.10, after fifteen examples were added, with the same djinterp and
uxoxo:

| | result |
|---|---|
| GCC 13.3, Linux, Release, C++20, warnings as errors | every target built; 295 of 295 tests, the three Linux platform checks among them |
| Clang 18.1, the same | every target built; 295 of 295 tests |
| GCC 13.3, the window and the model at C++17, warnings as errors | every target built; 295 of 295 tests |
| MinGW-w64 GCC 13, cross, warnings as errors | every target built; 290 of 290 run under Wine 9.0, the two Windows platform checks among them, compiled against MinGW-w64's `windows.h` |
| the whole build configured for `cl` 19.51 through vparse's `tools/cl_standin/` | every unit compiled, no warning; both executables linked; 186 of 186 tests run under Wine -- the Windows platform checks compiled `cl`-style, with the real identity, as every unit that includes `<windows.h>` is there |

The macOS and FreeBSD checks could not be run on a Mac or a FreeBSD
machine. They were compiled against stand-ins for the system headers made
from XNU's own `mach-o/loader.h` and `mach/machine.h`, and from FreeBSD's
`sys/stat.h` and the `sys/event.h` of FreeBSD 14 and of 15 -- the first
without the jail filters, so the checks' guards were taken both ways. Then
every literal in the nine generated headers was changed, one at a time:
each change to a value the system also defines failed its check, so the
checks compare what they claim to. (What no system header defines -- a
field's `_SHIFT` and `_WIDTH`, HRESULT's reserved bits -- the header tests
hold to the sheet.)

Two things changed in the course of it. The self-test wrote a 64-bit value
as `0x...ULL` at C++98, which has no `long long`, and failed under
`-pedantic-errors`; it now writes it as the header does, as an `unsigned
long`. And a sheet taken from the examples, or imported, has its name and
value columns fitted to what they hold -- a 64-bit example's values no
longer arrive cut off -- without being marked changed.

The record of 2026.10.09 follows.

On 2026.10.09, after the context menus, the renames, the resizable columns
and the reading of C, against the consolidated djinterp of 2026.10.07 and
the uxoxo of 2026.10.09:

| | result |
|---|---|
| GCC 13.3, Linux, Release, C++20, warnings as errors | every target built; 52 of 52 tests |
| Clang 18.1, the same | every target built; 52 of 52 tests |
| GCC 13.3, the window and the model at C++17, warnings as errors | every target built; 52 of 52 tests |
| MinGW-w64 GCC 13, cross, warnings as errors | every target built, the tests too; 52 of 52 run under Wine 9.0, `ui_smoke`'s clipboard and dropped file included |
| the whole build configured for `cl` 19.51 through vparse's `tools/cl_standin/`, `Ninja Multi-Config`, Release, as on 2026.10.07 | CMake reports MSVC 19.51.36260.0 and takes `cl`'s branches; every unit compiled, djinterp's nine lexer sources among them as C under `cl`'s identity with the conforming preprocessor; `bitwisedom.exe` and `bitwisedom-gen.exe` linked; 34 of 34 tests run under Wine |

The stand-in was configured with CMake's searches for GLFW and pkg-config
turned off (`-DCMAKE_DISABLE_FIND_PACKAGE_glfw3=ON
-DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=ON`): its toolchain does not keep
package searches inside a Windows sysroot, as `mingw-cross`'s does, and
uxoxo otherwise offers the build machine's GLFW to a Windows target. The
window's link, which it could not make on 2026.10.07, it now makes against
MinGW-w64's import libraries; that `bitwisedom.exe` was not run.

`ui_smoke` checks every frame for Dear ImGui's own errors as well as for
clashing IDs. That check found one, in uxoxo: a tree node renamed in place
put ImGui's cursor back with nothing drawn after it, which ImGui 1.92 calls
an error when the node is the last item of its window -- an assertion in a
debug build. uxoxo of 2026.10.09 draws the field without moving the cursor
back; the run has no error in any frame.

The window was run on a virtual display and photographed: the context menu,
a rename typed over a name in the grid, and the attached code imported
(`screenshots/import-define-environment.png`). Its settings file was
written, read back on the next run with the panels' sizes it held, and a
table weight ImGui had saved as `nan` before its first layout was left out
on reading and writing both.

The record of 2026.10.07 follows.

On 2026.10.07, against the consolidated djinterp and uxoxo, laid out as on
the owner's machine (each the owner's tree with its archive unzipped over
it):

| | result |
|---|---|
| GCC 13.3 and Clang 18.1, Linux, Release, warnings as errors | every target built; 52 of 52 tests |
| the `mingw-cross` preset, warnings as errors | `bitwisedom.exe` and `bitwisedom-gen.exe` built |
| **the whole build as CMake configures it for `cl` 19.51**, through vparse's `tools/cl_standin/`, `Ninja Multi-Config` as the `msvc` preset has it, Release | CMake reports MSVC 19.51.36260.0 and takes `cl`'s branches; every unit compiled, no warning; 34 of 34 tests -- the thirty generated headers' self-tests compiled `cl`-style, `/W4 /WX` -- run under Wine |

In the stand-in, `app/`'s units, which include `<windows.h>`, kept the real
identity, as Dear ImGui's do: the MinGW-w64 headers will not read it as
`cl`'s. It cannot link `bitwisedom.exe` either, since it has no Windows SDK
import libraries; the MinGW-w64 build links the same window. So `cl` itself,
the window's link under it, and a real Windows machine remain unverified.

That run is why two things changed on 2026.10.07: under `cl`, uxoxo's
library and the UI smoke test, which read djinterp's headers, now take
`/Zc:preprocessor` as every other unit here does -- before, they were left
to `cl`'s traditional preprocessor; and the header tests run
`bitwisedom-gen` through `CMAKE_CROSSCOMPILING_EMULATOR` where a cross build
has one, which changes nothing in a native build.

The rest of this section is the record of 2026.10.05.

On Linux (Ubuntu 24.04), every target built and all 52 tests passed:

* with GCC 13.3 and with Clang 18.1, at C++20, the model's suite also at C++17
  and C++23;
* with Clang and `BITWISEDOM_WARNINGS_AS_ERRORS=ON`: no warning from any of
  bitwis(e)dom's own sources under the whole baseline;
* through the `clang-strict` preset (Ninja Multi-Config), configure to tests.

The window was run on a virtual display (Xvfb, Mesa's software renderer) and
photographed: the screenshots in `docs/` are those runs.

The Windows build was cross-compiled with MinGW-w64 GCC 13 with warnings as
errors, and both executables were run under Wine 9.0: `bitwisedom-gen.exe`
wrote headers byte-identical to the Linux build's -- all three examples, in
both styles -- and `bitwisedom.exe` drew the same window through Wine's
Direct3D 11.

**Not verified:** `cl` and `clang-cl`, and so the `msvc`, `vs2022`,
`clang-cl` and `strict` presets; a real Windows machine and a real GPU;
macOS.
