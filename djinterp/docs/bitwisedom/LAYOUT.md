# bitwisedom in the djinterp repository

This package is an overlay for the root of the **djinterp** repository.
Extract it there while preserving the directory structure. It does not
include the rest of djinterp or any Dear ImGui or uxoxo sources.

The window is made with uxoxo, which the djinterp tree does not hold, so it
lives beside djinterp's directories rather than in them -- `ui/` where
djinterp's are `djinterp/` -- and has a build of its own (2026.10.23).
`bitwisedom gui` loads it as a module; the command line carries none of it.

| Repository-relative path | Purpose |
|---|---|
| `inc/djinterp/tools/bitwisedom/` | Public headers: the model's and the command line's |
| `src/djinterp/tools/bitwisedom/` | The model, the command line (`cli/`) and the programs' mains (`tools/`) |
| `tests/djinterp/tools/bitwisedom/` | Tests and test fixtures; the command line's transcripts in `cli/` |
| `build/cmake/djinterp/tools/bitwisedom/` | The command line's CMake files and presets; generated build intermediates in `out/` |
| `inc/ui/bitwisedom/` | The window's headers |
| `src/ui/bitwisedom/` | The window, its two hosts, and the module's run function |
| `tests/ui/bitwisedom/` | The window's smoke test |
| `build/ui/bitwisedom/` | The window's CMake files and presets, which build the command line's within them; intermediates in `out/` |
| `bin/tools/bitwisedom/` | Release executables/libraries named `<target>-<platform><bits>.<ext>` |
| `bin/tools/bitwisedom/debug/` | Debug executables, libraries and PDB files |
| `res/djinterp/tools/bitwisedom/examples/` | JSON examples and large fixtures |
| `docs/bitwisedom/` | Documentation, schema and screenshots |

Use `cmake --preset msvc` from `build/ui/bitwisedom/` for everything, or
from `build/cmake/djinterp/tools/bitwisedom/` for the command line alone
(on Windows, in a Developer Command Prompt). The window's build finds uxoxo
two directories above the djinterp root -- `uxoxo/uxoxo-project` there,
else `uxoxo` -- unless `UXOXO_ROOT` names it. A previous standalone
bitfield_manager build should not be reused. The public header paths are now
`#include <djinterp/tools/bitwisedom/sheet.hpp>`, etc. The library's C++
namespace remains `bitwisedom`. The target names are unchanged.

This overlay does **not** change djinterp's main build registration.
bitwisedom currently builds from its own CMake entrypoint under `build/`.

## Binary filenames

On a Windows x64 build, outputs include:

```text
bin/tools/bitwisedom/bitwisedom-win64.exe
bin/tools/bitwisedom/bitwisedom-gui-win64.dll
bin/tools/bitwisedom/bitwisedom-gen-win64.exe
bin/tools/bitwisedom/bitwisedom_core-win64.lib
bin/tools/bitwisedom/debug/bitwisedom-win64.exe
bin/tools/bitwisedom/debug/bitwisedom-win64.pdb
```

All bitwisedom-owned test binaries and libraries use the same convention.
Dependencies such as uxoxo keep their own output layout. Build intermediate
files remain under the build's own `out/`:
`build/cmake/djinterp/tools/bitwisedom/out/`, or `build/ui/bitwisedom/out/`.
The tag is selected from the target OS/architecture, and can be overridden
using the CMake cache variable `BITWISEDOM_PLATFORM_TAG`.

**Migration:** The ZIP overlays the new paths; extracting a ZIP cannot
delete your old `build/djinterp/tools/bitwisedom/` directory. Remove that
obsolete configuration manually when the new build is working. Since
2026.10.23 the same holds for what moved out of the djinterp tree: delete
`src/djinterp/tools/bitwisedom/app/` (now `inc/ui/bitwisedom/` and
`src/ui/bitwisedom/`), `tests/djinterp/tools/bitwisedom/ui_smoke.cpp` (now
`tests/ui/bitwisedom/`) and
`build/cmake/djinterp/tools/bitwisedom/cmake/Uxoxo.cmake` (now
`build/ui/bitwisedom/cmake/`).

**uxoxo's Dear ImGui, patched.** Since 2026.10.20 the Options can set a
sheet's tabs over one of its columns, at the column's left or its right,
which Dear ImGui's dock nodes do not do by themselves: uxoxo's
`inc/imgui/imgui.cpp` and `imgui_internal.h` carry a small patch -- a dock
node's tab bar may begin and end inset, and keep to its right, marked
`[uxoxo]` and announced by `IMGUI_DOCK_TAB_BAR_INSET 2` (2026.10.19's
first form, `1`, began inset only) -- delivered as an overlay of uxoxo's
own, to extract over uxoxo's root. Without it the window still builds,
configure says so, and the tabs run across the whole width from the left,
as they do by default anyway.

**After unzipping an overlay, configure before building**
(`cmake --preset msvc`): extraction keeps the archive's file times, which
can be older than your last build's objects, and configure marks every
source whose content changed to compile again. [BUILD.md](BUILD.md) says
how.
