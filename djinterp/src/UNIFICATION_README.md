# Component subframework unification — change set

Cycle: 2026.04 — Tier-0 prerequisite for the file-level build queue
(palette, register_component, style target, to_css, uxoxo-decl all
benefit from a unified trait surface).

This change set executes steps 1–5 of the unification plan:

1. Collapse per-component mixin namespaces into `component_mixin`.
2. Delete prefixed verbs that duplicate `component_common` generics.
3. Strip duplicate per-component SFINAE detectors.
4. Resolve the `__1_.hpp` ghost files.
5. Settle the enum-naming policy (document the bitflag exception).


## Files in this change set

### Modified — replace existing files with these

| File                            | What changed                                           |
| ------------------------------- | ------------------------------------------------------ |
| `component_types.hpp`           | `checked_state` → `DCheckState` (matches checkbox.hpp, fixes broken trunk) |
| `toggleable_common.hpp`         | Bug-fixed SFINAE in section 4; uses `DCheckState`; normalized to `NS_INTERNAL` and `[[nodiscard]]` |
| `input_control.hpp`             | `input_mixin` → `component_mixin`; deleted seven `ic_*` verbs; deleted `input_control_traits`; deleted redundant `is_enabled()` member |
| `output_control.hpp`            | `output_mixin` → `component_mixin`; deleted seven `oc_*` verbs; deleted `output_control_traits` |
| `component_common.hpp`          | Preamble updated — removed `ic_enable`/`oc_enable` examples that no longer exist; explanation of why the prefixed verbs were removed |
| `uxoxo_style_guide_cpp.md`      | Added the bitflag-enum exception under §C++-Specific Naming |


### Delete — ghost files now resolved

| File                          | Reason                                                 |
| ----------------------------- | ------------------------------------------------------ |
| `checkbox__1_.hpp`            | Byte-identical to `checkbox.hpp`. Stale export.        |
| `tab_control__1_.hpp`         | Only stylistic differences from `tab_control.hpp` (em-dash, "date:" label). Current is canonical. |
| `toggleable_common__1_.hpp`   | Was the bug-fixed companion to `toggleable_common.hpp`. The fix has been folded into the modified file above. |


## What was the bug

`checkbox.hpp` references `DCheckState` in field types, switch cases,
and callback signatures. `component_types.hpp` defined
`checked_state` (snake_case). These two facts contradict — trunk
does not compile any code that touches checkbox without first hand-
patching one of the two files.

Independently, `toggleable_common.hpp` section 4 (the DCheckState-
valued overloads) had every SFINAE constraint copy-pasted from
section 3 — gating on `has_boolean_value_v<_Type>` instead of
`has_check_state_value_v<_Type>`. Result: every overload in section 4
was unreachable. `is_on(my_checkbox)`, `turn_on(my_checkbox)`, and
`toggle(my_checkbox)` would all fail to find a matching overload at
the call site.

Both bugs are fixed by this change set. The `__1_.hpp` ghost was the
in-progress fix; it has been finalized and folded into the canonical
file.


## Verification

No external callers reference the deleted symbols:

- `ic_*` verbs: only referenced inside `input_control.hpp` itself.
- `oc_*` verbs: only referenced inside `output_control.hpp` itself.
- `input_mixin::*`: only used inside `input_control.hpp` itself.
- `output_mixin::*`: only used inside `output_control.hpp` itself.
- `input_control_traits::*`: only referenced inside `input_control.hpp`
  itself.
- `output_control_traits::*`: only referenced inside `output_control.hpp`
  itself.
- `checked_state`: referenced only by `component_types.hpp` and
  `toggleable_common.hpp` — both updated in this change set.

Per the roadmap: "no deprecation aliases (we are pre-production)" —
the prefixed verbs and per-component traits are removed outright.

`text_output_mixin` (in `text_output.hpp`) and `console_mixin` (in
`dev_console.hpp`) were intentionally left alone — they hold
component-specific mixins (line buffer, selection, filtering;
console output sub-aggregator) that have no analogue in
`component_mixin` and are not redundant with anything.

`cb_cycle`, `cb_sync_from_children`, `csl_*`, and other domain-
specific prefixed verbs in their respective headers are likewise
not redundant — they cover operations with no generic analogue —
and were left alone.


## Why this gates the next milestones

The current file-level build queue (roadmap §V) is:

  4. Tier-0 foundation
  5. Renamed tree headers
  6. component_palette.hpp
  7. register_component.hpp
  8. uxoxo_style_target.hpp
  9. apply_stylesheet.hpp
  10. to_css.hpp
  ...

Items 6–10 all introspect components to extract a uniform capability
profile (focusable? labelable? clearable? has on_commit?). Each
introspection point folds over the trait surface in
`component_traits.hpp`. Before this change set, that fold would have
to special-case `input_control` and `output_control` because they
expose a parallel trait surface (`input_control_traits::is_input_control`,
etc.) that is *almost* but not quite the same as the generic one.
After this change set, the fold is uniform.

Same argument for the unified mixin surface: the palette wants to
ask "does this component carry a label?" and get one answer.
Before, `input_control`'s label inherited from `input_mixin::label_data`
while a hypothetical `popover` component might carry a
`component_mixin::label_data` label — both have a `.label` member,
so the structural detector still finds them, but type-level
introspection (e.g., the DSL emitter wanting to template-specialize
on the mixin type itself) couldn't. After this change set, every
labeled component inherits from the same `component_mixin::label_data`
specialization.

This is why the unification is Tier-0, not Tier-0.5: it removes a
class of special-case branching from the four headers ahead of it
in the queue.
