# Visualization framework — roadmap

A UI-framework-agnostic core for multidimensional, multi-resolution visualizations,
with three frontends on top: an interactive ImGui binding, a file exporter
(PNG / SVG / GIF), and a DSL. First real consumer: the best-path optimizer over
the djinterp virtual computer.

The shape of the system (from the design conversation): a framework-agnostic core
(template + state + geometry + scene IR) feeds a swappable **lens** stage, which
emits one shared render scene that any frontend draws. Project / slice / flatten /
heatmap / parametric are not separate visualizations — they are one pipeline with
one stage that swaps.

Lives under `djinterp::ui`. Status legend: `[ ]` todo, `[~]` in progress, `[x]` done.

---

## Open decisions (gates)

These don't block the agnostic core, but they gate later phases — resolve before
the phase that needs them.

- [ ] **Rendering backend** (gates Phase 4 + 5). ImGui draw-list (software projection,
  no dependency) vs. GPU-to-texture (OpenGL/Vulkan/etc.) vs. headless software/GL.
  Note: SVG/GIF export pushes toward a software/vector path that can run without a window.
- [ ] **Optimizer data contract** (shapes Phase 1a). What the optimizer can emit per
  frame — axis definitions + time-indexed current-best path at minimum; per-cell scalar(s)
  ideally.
- [ ] **C++ baseline for `djinterp::ui`.** Concepts usage suggests C++20; decide whether
  to match the library's dual-standard fallback pattern or require C++20.
- [ ] **Confirm the lens set.** `parametric` is newly added; confirm whether `slice`
  stays (it is the most native to the axis-aligned grid and keeps the octree exact).

---

## Guiding principles

- The agnostic core knows nothing about ImGui, OpenGL, or files. Frontends are thin.
- The declarative **configuration model** is the single source of truth. ImGui, the
  exporter, and the DSL all drive the *same* config + state.
- Everything visual is configurable (see Configuration model below).
- Follow djinterp house style: undecorated `CamelCase` template params (a
  leading underscore before a capital is reserved to the implementation),
  `D_`/`NS_` macros, the trait-triple convention, version gating.

---

## Phases

### Phase 0 — Foundations
- [ ] Resolve the open decisions above (or enough to unblock Phase 1).
- [ ] Namespace + file layout under `djinterp::ui`; build wiring.
- [ ] Pin the C++ baseline and feature gates.

### Phase 1 — Agnostic core (no UI, no rendering) — *first buildable deliverable*
Backend-independent. This is the piece that unblocks everything and needs no decision.
- [ ] **1a. State / value model** *(your item 3)* — axes (continuous + discrete/ordinal),
  cells, time-indexed path(s), per-cell scalar metrics. Time is a first-class axis.
- [ ] **1b. Configuration model** — the declarative schema of a visualization
  (full knob list under Configuration model below). The spine of the whole framework.
- [ ] **1c. Scene IR** — positioned cells + styled path + scalar field, in render space.
  The lens → renderer contract.
- [ ] **1d. Top-level template type** *(your item 1)* — bundles config + state + lens;
  renderable by any frontend. The agnostic "graph/chart/infographic" handle.

### Phase 2 — 3D & geometry core (agnostic) — *your item 2*
- [ ] Coordinate systems: cartesian (first), then polar & spherical
  *(stretch — LOD subdivision in curved coordinates is non-trivial)*.
- [ ] Shape primitives: cube, tetrahedron, octahedron.
- [ ] LOD hierarchy: branching factor (shapes per level), focus+context fan-out
  (levels shown per level), discrete zoom snapping.
- [ ] Camera: orbit rotate / zoom / pan.
- [ ] Projection math (3D → 2D) — feeds the software renderer and export.

### Phase 3 — Lens layer
- [ ] Lens interface (runtime-polymorphic, since mode is user-selectable; lenses may be
  templated internally).
- [ ] `slice` — pick 3 of n axes, fix the rest; cells are the real axis-aligned grid
  clipped to the slab; path = its intersection. Keeps the octree exact.
- [ ] `projection` — reduce n → 3 (PCA / UMAP / linear, or project just the trajectory);
  hierarchy rebuilt over the embedding; path projects whole.
- [ ] `flatten` — Morton/Hilbert-pack several axes per render axis; curve recursion = zoom
  levels; path follows the curve.
- [ ] `heatmap` — degenerate 2-D case of the cell renderer; cells colored by a metric,
  camera locked top-down, path overlaid.
- [ ] `parametric` — bind an axis to a parameter of a parametric function; sweep the
  parameter and render the resulting locus.

### Phase 4 — Interactive frontend (ImGui reference binding)
Needs the backend decision.
- [ ] Renderer: depth-sorted semi-transparent shapes; gridlines (thickness / type /
  opacity / color / fade distance); path lines (color / thickness / type); axis labels,
  lines, and scales; discrete-axis rendering.
- [ ] Camera controls + discrete zoom.
- [ ] Hover / picking on cells at each level *(your "mousing over cubes" item)*.
- [ ] Control panel: lens selector, per-lens params, fixed-axis sliders, time slider with
  multiple speeds, full config editing, legend / colorbar.
- [ ] First real consumer: wire the optimizer's output end-to-end.

### Phase 5 — Export pipeline — *your item 5*
Needs a headless render path.
- [ ] Headless render path (software renderer or headless GL) — required for files.
- [ ] Exporters: PNG (raster), SVG (vector), animated GIF (time-scrubbed frames).
- [ ] Python script / bindings driving the core from a spec + state.
- [ ] Decide the export route: C++ emits SVG directly vs. Python orchestrates frames vs.
  pybind11 over the core lib.

### Phase 6 — DSL — *your item 6*
Needs a stable configuration model.
- [ ] Grammar mapping 1:1 to the configuration model (coords, shapes, gridlines, axes,
  lenses, line styling, time/animation).
- [ ] Two surfaces: an embedded C++ fluent/compile-time builder (`fixed_string` +
  the functional subframework) and a textual DSL parsed at runtime (pairs with export
  and external tooling).
- [ ] Round-trip: DSL → config → render, and serialize config back to DSL.

---

## Configuration model (the "everything configurable" schema)

The Phase 1b spine. Every knob the frontends expose maps to one of these.

- **Space & layout** — coordinate system (cartesian / polar / spherical); shape primitive
  (cube / tetrahedron / octahedron); spacing between shapes; shapes per level (branching
  factor); levels shown per level (focus+context depth — how many cells render at full
  detail around the focus before falling back to coarse/transparent context).
- **Appearance — shapes** — fill color; opacity; per-shape and per-level overrides.
- **Appearance — gridlines** — thickness; type (solid / dashed / …); opacity; color;
  fade distance (distance-based opacity falloff).
- **Appearance — path / line** — color; thickness; type; per-node and per-segment coloring
  (explicit color or scalar → colormap).
- **Axes** — labels; axis lines; scales / ticks; discrete (ordinal/categorical) axis support.
- **Lens / mapping** — mode = slice | projection | flatten | heatmap | parametric; per-mode
  params (which axes, which reduction, which curve, which metric, parametric bindings).
- **Interaction** — hover / pick targets per level; selection highlight styling.
- **Time** — enabled; playback speeds; frame mapping.

---

## Cross-cutting

- [ ] Testing (`djinterp` test/testing conventions), per layer.
- [ ] Examples — the optimizer run as the canonical demo.
- [ ] Docs — agent reference in the style of the existing `functional.md`.
- [ ] Performance budget — cap on simultaneously visible cells; ties directly to the
  focus+context fan-out knob.

---

## Suggested first step

Build Phase 1 (1a–1d). It is backend-independent, unblocks everything, and needs none of
the open decisions. Then Phase 2 geometry. Then, rather than going wide, cut one vertical
slice end-to-end — cartesian + cube + `slice` lens + ImGui — to validate the contracts
before adding breadth (other coordinate systems, shapes, lenses, and the export/DSL frontends).
