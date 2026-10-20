# bitwisedom in the djinterp repository

This package is an overlay for the root of the **djinterp** repository.
Extract it there while preserving the directory structure. It does not
include the rest of djinterp or any Dear ImGui or uxoxo sources.

| Repository-relative path | Purpose |
|---|---|
| `inc/djinterp/tools/bitwisedom/` | Public headers |
| `src/djinterp/tools/bitwisedom/` | Core, GUI and generator sources |
| `tests/djinterp/tools/bitwisedom/` | Tests and test fixtures |
| `build/cmake/djinterp/tools/bitwisedom/` | CMake files and presets; generated build intermediates in `out/` |
| `bin/tools/bitwisedom/` | Release executables/libraries named `<target>-<platform><bits>.<ext>` |
| `bin/tools/bitwisedom/debug/` | Debug executables, libraries and PDB files |
| `res/djinterp/tools/bitwisedom/examples/` | JSON examples and large fixtures |
| `docs/bitwisedom/` | Documentation, schema and screenshots |

Use `cmake --preset msvc` from `build/cmake/djinterp/tools/bitwisedom/`
(on Windows, in a Developer Command Prompt). A previous standalone
bitfield_manager build should not be reused. The public header paths are now
`#include <djinterp/tools/bitwisedom/sheet.hpp>`, etc. The library's C++
namespace remains `bitwisedom`. The target names are unchanged.

This overlay does **not** change djinterp's main build registration.
bitwisedom currently builds from its own CMake entrypoint under `build/`.

## Binary filenames

On a Windows x64 build, outputs include:

```text
bin/tools/bitwisedom/bitwisedom-win64.exe
bin/tools/bitwisedom/bitwisedom-gen-win64.exe
bin/tools/bitwisedom/bitwisedom_core-win64.lib
bin/tools/bitwisedom/debug/bitwisedom-win64.exe
bin/tools/bitwisedom/debug/bitwisedom-win64.pdb
```

All bitwisedom-owned test binaries and libraries use the same convention.
Dependencies such as uxoxo keep their own output layout. Source and build
intermediate files remain under `build/cmake/djinterp/tools/bitwisedom/out/`.
The tag is selected from the target OS/architecture, and can be overridden
using the CMake cache variable `BITWISEDOM_PLATFORM_TAG`.

**Migration:** The ZIP overlays the new paths; extracting a ZIP cannot
delete your old `build/djinterp/tools/bitwisedom/` directory. Remove that
obsolete configuration manually when the new build is working.

**uxoxo's Dear ImGui, patched.** Since 2026.10.19 a sheet's tabs begin
past its symbols column, which Dear ImGui's dock nodes do not do by
themselves: uxoxo's `inc/imgui/imgui.cpp` and `imgui_internal.h` carry a
small patch -- a dock node's tab bar may begin inset, marked `[uxoxo]` and
announced by `IMGUI_DOCK_TAB_BAR_INSET` -- delivered as an overlay of
uxoxo's own, to extract over uxoxo's root. Without it bitwisedom still
builds, configure says so, and the tabs run across the whole width.

**After unzipping an overlay, configure before building**
(`cmake --preset msvc`): extraction keeps the archive's file times, which
can be older than your last build's objects, and configure marks every
source whose content changed to compile again. [BUILD.md](BUILD.md) says
how.
