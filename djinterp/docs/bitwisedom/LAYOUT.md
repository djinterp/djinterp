# bitwisedom in the djinterp repository

This package is an overlay for the root of the **djinterp** repository.
Extract it there while preserving the directory structure. It does not
include the rest of djinterp or any Dear ImGui or uxoxo sources.

| Repository-relative path | Purpose |
|---|---|
| `inc/djinterp/tools/bitwisedom/` | Public headers |
| `src/djinterp/tools/bitwisedom/` | Core, GUI and generator sources |
| `tests/djinterp/tools/bitwisedom/` | Tests and test fixtures |
| `build/djinterp/tools/bitwisedom/` | CMake files, presets and generated output in `out/` |
| `res/djinterp/tools/bitwisedom/examples/` | JSON examples and large fixtures |
| `docs/bitwisedom/` | Documentation, schema and screenshots |

Use `cmake --preset msvc` from `build/djinterp/tools/bitwisedom/`
(on Windows, in a Developer Command Prompt). A previous standalone
bitfield_manager build should not be reused. The public header paths are now
`#include <djinterp/tools/bitwisedom/sheet.hpp>`, etc. The library's C++
namespace remains `bitwisedom`. The target names are unchanged.

This overlay does **not** change djinterp's main build registration.
bitwisedom currently builds from its own CMake entrypoint under `build/`.
