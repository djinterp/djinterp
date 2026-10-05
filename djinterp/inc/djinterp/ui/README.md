# djinterp::ui — multidimensional visualization framework

A UI-framework-agnostic, multi-resolution visualization stack built on the
djinterp math and render3d subframeworks. Its first consumer was a best-path
optimizer over an n-dimensional discrete state space, rendered as nested
wireframe (or translucent) cubes converging on a heat-coloured optimum path;
the vparse workbench now uses it too, to map where a parser spends its work.

Everything here is **header-only**: `#include <djinterp/ui/ui.hpp>`.

---

## Layout

```
inc/djinterp/ui/
  ui.hpp                  umbrella (include this)
  types.hpp               scalar, rgb/rgba, enums, and the geometry aliases
                          (vec, aabb3, camera, viewport, ray) onto math and 3d
  colormap.hpp            colour ramps through any number of stops, with presets
  state.hpp               axes, cells, paths, state_source
  config.hpp              colours / layout / shape / gridline / axis / lens /
                          interaction config
  scene.hpp               the render-space scene IR (cells, path, field,
                          decorations)
  lens.hpp                abstract lens interface
  visualization.hpp       config + state_source + lens -> scene
  lod.hpp                 the level-of-detail nesting scheme
  coordinate_mapper.hpp   cartesian / polar / spherical placement
  draw.hpp                the framework-agnostic 2D draw layer: scene
                          projection and picking, and field panels (projection,
                          picking, labelled layout with an overlay)
  lenses/
    common.hpp            shared lens helpers
    slice.hpp             the slice lens (flat + nested LOD)
    heatmap.hpp           the heatmap lens (2D scalar field)
  sources/
    visit_source.hpp      a state_source that counts visits per cell over time
  export/
    svg.hpp               draw lists and field panels as SVG documents
  frontends/
    imgui.hpp             the interactive ImGui frontend (opt-in; needs ImGui)
  roadmap.md              the original dependency-ordered plan
```

---

## Dependencies

All in the tree:

- `djinterp.hpp` — the root (namespace macros, including `NS_UI`).
- `core/functional/maybe.hpp`, `result.hpp` — `maybe<T>` / `result<T,E>`.
- `core/util/color/color_rgb.hpp` — `rgb`, `rgba`, and their `lerp`.
- `math/linear_algebra/` — vectors, matrices, transforms, quaternions.
- `math/geometry/` — `aabb`, `ray` and `intersect_aabb`.
- `math/coordinate/` — the cylindrical and spherical systems the mapper uses.
- `3d/camera.hpp`, `3d/box.hpp` — the orbit camera (with framing) and box
  projection.
- **ImGui** — only for `frontends/imgui.hpp`, which is deliberately left out of
  the umbrella. Include it where ImGui is linked.

---

## What works

- One shared pipeline, one swappable lens stage:
  `state_source -> lens -> scene IR -> renderer + camera -> controls`.
- Two lenses (slice, heatmap), LOD nesting, the coordinate bridge.
- A 2D draw layer that emits backend-neutral geometry, so frontends stay
  thin. Field panels are laid out there too — captions, tick labels thinned
  so they never collide, marked cells and a trail through cell centres — given
  the backend's text metrics, so the live view and an exported file are the
  same drawing.
- `visit_source`: record `(cell, frame)` visits from anything that moves
  through a discrete space — a search expanding nodes, a parser trying rules at
  positions — and the heatmap lens shows the counts as of any frame.
- SVG export of any draw list, or of a whole field panel in one call.
- Interactive ImGui views that draw into the current window: `draw_scene`
  (orbit / zoom / pan, hover-pick with tooltip, click-to-select, the camera
  framed on the scene at first draw) and `draw_field` (reports the hovered and
  clicked cell, so the host can add its own tooltip or act on a click).
  `show_scene` / `show_field` wrap them in a window, as before.

---

## Example

```cpp
#include <djinterp/ui/ui.hpp>

namespace ui = djinterp::ui;

// a 3 x 2 space of positions and rules, over 10 frames
ui::axis x;
x.name   = "position";
x.kind   = ui::axis_kind::discrete;
x.labels = { "a", "b", "c" };

ui::axis y;
y.name   = "rule";
y.kind   = ui::axis_kind::discrete;
y.labels = { "S", "T" };

ui::visit_source visits({ x, y }, 10);
visits.visit({ 0, 1 }, 3);    // (a, T) was visited in frame 3

ui::config cfg;
cfg.lens.mode = ui::lens_mode::heatmap;
cfg.lens.axes = { 0, 1 };

const ui::scene s = ui::heatmap_lens().build(visits, cfg, 9).value();

// a file, no window needed
const std::string svg =
    ui::field_svg(s,
                  ui::vec<2>(600.0, 200.0),
                  ui::scale_of(s.field.value(),
                               ui::colormap::yellow_orange_red()));

// or live, inside an ImGui window (frontends/imgui.hpp)
const ui::field_hit hit = ui::draw_field(s, ui::scale_of(s.field.value()));
```

---

## What the port changed

The module arrived as a header-only drop that had never been compiled. Moving
it into the tree:

- **`render3d/` is gone.** Its vector, matrix, quaternion, ray, bounds, mesh
  and camera code is the math and render3d subframeworks' job, and `types.hpp`
  now aliases the ui's spellings onto theirs. What the ui had that they
  lacked moved there as general utilities: the quaternion
  (`math/linear_algebra/quaternion.hpp`), camera framing (`camera::frame`),
  box projection (`3d/box.hpp`), `aabb::is_empty`, and colour `lerp`.
- **Colormaps are general.** A colormap is a list of stops; `cool_warm` (the
  original ramp) is the default, so default output is unchanged.
  `config::colors` selects the ramp.
- **Field layout moved out of the ImGui frontend** into `draw.hpp`, so SVG
  export and any future backend reuse it.
- **The ImGui views draw into the current region** instead of opening their own
  windows, so they embed in any layout.
- **Axis names** sit just past the end of their axis rather than on top of the
  last tick label.
- `IsMouseDragPastThreshold`, an ImGui internal, is replaced by the public
  `GetMouseDragDelta`.
- The zip's `maths/geometry/platonic.hpp` was not carried over; the tree's
  `math/geometry/platonic.hpp` is the maintained one.

---

## Tests

`tests/djinterp/ui/` (leaf `build/cmake/config/testing/djinterp/ui/`) covers
colormaps, the visit source alone and under the heatmap lens, field picking and
layout, SVG export, and scene projection and picking through the render3d
camera. No window is needed. Raw build:

```bash
g++ -std=c++20 -DD_TESTING=1 -Wall -Wextra -Iinc -Itests/djinterp/ui \
  build/cmake/config/testing/djinterp/ui/ui_tests_runner.cpp \
  tests/djinterp/ui/ui_tests_*.cpp -o ui_tests && ./ui_tests
```

The render3d additions have their own suite in `tests/djinterp/render/`, built the
same way.

---

## Known caveats

- **Renderer approximations:** wireframe and face depth ordering is the
  painter's algorithm (per cell for edges, per face centre for faces) — correct
  for separated or nested-but-not-touching cubes, approximate for
  interpenetrating geometry. No near-plane clipping. Axis decorations are
  straight lines, which assumes the cartesian layout (they don't curve under
  the polar and spherical mappers).
- `visit_source` counts at full resolution (LOD level 0) only.
- SVG text is measured as a monospace font; a proportional font renders
  narrower than the layout allowed for.
- The slice lens and LOD nesting compile, but have no unit tests yet.
- Built and tested with GCC 13 on Linux; not yet tried on MSVC.
- The depth-fade curve, the `shape.filled` default (off), and the axis and
  label colours are deliberate starting points meant to be tuned.
