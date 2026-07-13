# djinterp `color` subframework (C) — agent reference

A token-dense map of the 9 C header modules tagged `djinterp [color]`, plus
their matching `.c` external-definition units. Part 1 summarizes each module.
Part 2 (appendix) gives per-module signature tables. The C++ wrapper layer
(`*.hpp`; see `readme_color_cpp.md`) and the framework header it depends on
(`djinterp.h`) are out of scope here.

---

## Conventions (read once, applies everywhere)

**Header / source layout.** Every function body lives in its `.h` as an
`inline` definition; these headers *are* the C library and double as the
shared kernel the C++ layer compiles against (so no math is written twice).
For a normal compiled C library each module also ships a `.c` that re-declares
its prototypes — under C99 inline rules that emits exactly one out-of-line
external definition per function, resolving non-inlined calls and taken
addresses. Build/link the `color_*.c` files for that. Define
`D_COLOR_HEADER_ONLY` to switch the kernel to `static inline` and drop the
`.c` files entirely (header-only, no link step). `color.h` is the umbrella
(pulls in every model + conversions + cross-model ops); `color_common.h` is
the shared foundation every other header includes.

**Naming.** All public symbols carry the `d_color_` prefix. POD types are
`d_color_<space>` (`d_color_rgb`, `d_color_hsl`, …), declared
`typedef struct d_color_X { … } d_color_X;` so the bare name is usable without
`struct`. Per-model helpers keep the model in the name
(`d_color_rgb_clamp`, `d_color_rgba_premultiply`); cross-space conversions are
uniformly `d_color_convert_<from>_to_<to>`. Construction/validation/clamping
follow the triple `d_color_<X>_make` / `d_color_<X>_is_valid` /
`d_color_<X>_clamp`.

**Types & calling convention.** Colors are small PODs of `float` channels and
are always passed and returned **by value** (no pointers, no heap); the
dropped `_new`/`_free` API is replaced by value `_make` factories. Out-params
appear only where a parse can fail (`const char*` → `d_color_* _out`, returns
`bool`). Hex forms are `uint32_t` typedefs (`d_color_rgb_hex` = `0x00RRGGBB`,
`d_color_rgba_hex` = `0xRRGGBBAA`).

**Color conventions.** `d_color_rgb` holds **linear** RGB in `[0,1]`; sRGB is
produced/consumed by the gamma routines, and the 8-bit/hex paths gamma-encode.
Luminance/contrast use linear values (Rec. 709 weights; WCAG-correct). Alpha is
carried **only** by `d_color_rgba`/`d_color_rgba_premul` — every color-space
model (rgb, hsl, hsv, cmyk, ycbcr, xyz, lab) is alpha-free. YCbCr is full-range
ITU-R BT.601; XYZ/LAB use the D65 white point. `D_COLOR_EPSILON` (`1e-6f`)
guards near-zero divisions and comparisons.

**`constexpr` tiering (shared with C++).** Two qualifier macros classify every
kernel function so the *same* bodies are `constexpr` on the C++ side where the
standard allows: `D_COLOR_FN` marks pure-arithmetic math (constant-expression
safe); `D_COLOR_FN_RT` marks math that calls `<math.h>` transcendentals
(`powf`, `cbrtf`, `sqrtf`, trig, …) and is therefore runtime-only. In C both
expand to the same linkage; the distinction is documented per function in the
tables (RT = transcendental/runtime).

---

# Part 1 — Module summaries

**`color_common.h`** — Shared foundation included by every other header.
Defines the portability layer (`D_COLOR_FN` / `D_COLOR_FN_RT` qualifier macros,
`D_COLOR_NS_OPEN`/`D_COLOR_NS_CLOSE` namespace bridge — empty in C, `djinterp`
in C++ — `D_COLOR_LITERAL` aggregate construction, and `D_COLOR_EPSILON`), plus
the scalar helpers used across all models: `d_color_max3`/`min3`,
`clamp_01`/`clamp_range`, and constexpr-friendly `fmodf`/`fabsf` (so hue
arithmetic stays usable in constant expressions on the C++ side).

**`color_rgb.h`** — The RGB family and everything operating purely within it.
PODs: linear `d_color_rgb`, straight-alpha `d_color_rgba`, premultiplied
`d_color_rgba_premul`, 8-bit `d_color_rgb_u8`/`d_color_rgba_u8`, and packed
`d_color_rgb_hex`/`d_color_rgba_hex`. Routines: construction, validation,
clamping, sRGB gamma transfer (component + whole-color, RT), alpha
(attach/drop, pre/un-multiply, Porter-Duff "over" in both premultiplied and
straight forms), 8-bit and hex (de)serialization (gamma-aware, RT, incl.
`"#RRGGBB"`/`"#RRGGBBAA"` string parsing), luminance, WCAG contrast ratio,
grayscale, inversion, lerp, and blackbody `from_temperature` (RT).

**`color_cmyk.h`** — CMYK model. POD `d_color_cmyk` (c,m,y,k in `[0,1]`) with
`make`/`is_valid`/`clamp`. Conversions live in `color_convert.h`.

**`color_hsv.h`** — HSV model. POD `d_color_hsv` (h in `[0,360)`, s,v in
`[0,1]`); `make`/`is_valid`/`clamp` (clamp wraps hue into range).

**`color_hsl.h`** — HSL model. POD `d_color_hsl` (h in `[0,360)`, s,l in
`[0,1]`); `make`/`is_valid`/`clamp` (clamp wraps hue).

**`color_lab.h`** — CIE XYZ and CIE L\*a\*b\* grouped together, since XYZ is the
device-independent hub LAB routes through. PODs `d_color_xyz` and
`d_color_lab`; the D65 white-point macros (`D_COLOR_XYZ_WHITE_X/Y/Z`); the
shared LAB transfer nonlinearity `d_color_lab_f`/`d_color_lab_f_inv` (RT, cube
root); and `make`/`is_valid`/`clamp` for both (LAB clamps L to `[0,100]`, a/b
to `[-128,127]`).

**`color_ycbcr.h`** — YCbCr model (full-range BT.601). POD `d_color_ycbcr`
(Y in `[0,1]`, Cb/Cr in `[-0.5,0.5]`); `make`/`is_valid`/`clamp`.

**`color_convert.h`** — The cross-model conversion kernel: every color-space
conversion written exactly once, by value. RGB is the hub — HSL, HSV, CMYK,
YCbCr, and XYZ convert directly to/from linear RGB; HSL↔HSV route through RGB;
LAB routes through XYZ (so `rgb↔lab` composes `rgb↔xyz` with `xyz↔lab`). All
arithmetic conversions are constexpr-tier; the XYZ↔LAB and RGB↔LAB paths are
RT (cube root). Also exposes the `d_color_hsl_hue_to_rgb` sector helper.

**`color.h`** — Umbrella. Includes `color_common.h`, all model headers, and
`color_convert.h`, then adds the operations that span more than one space:
HSL-mediated `adjust_saturation`/`adjust_brightness`/`rotate_hue` of RGB
(constexpr-tier), and the CIEDE2000 perceptual difference `d_color_delta_e`
(on LAB) with its `d_color_rgb_delta_e` convenience (both RT). Defines
`D_COLOR_PI`. This is the single header a C consumer normally includes.

---

# Part 2 — Appendix: per-module signature tables

Signatures are compacted: `→` denotes the return type, by-value POD params are
shown by type, and **RT** marks a routine that calls `<math.h>` transcendentals
(runtime-only; constexpr on the C++ side only from C++23). Everything else is
constant-expression-safe on the C++ side. `D_COLOR_HEADER_ONLY` does not change
any signature, only linkage.

## `color_common.h`

| Entity | Signature | Notes |
|---|---|---|
| `D_COLOR_FN` / `D_COLOR_FN_RT` | qualifier macros | constexpr-tier vs transcendental/RT; C linkage per build mode |
| `D_COLOR_NS_OPEN` / `D_COLOR_NS_CLOSE` | namespace bridge | empty (C) / `namespace djinterp {` … `}` (C++) |
| `D_COLOR_LITERAL(T, …)` | aggregate ctor macro | `(T){…}` (C) / `T{…}` (C++) |
| `D_COLOR_EPSILON` | `1e-6f` | near-zero guard tolerance |
| `d_color_max3(a,b,c)` / `d_color_min3(a,b,c)` | → `float` | extremum of three |
| `d_color_clamp_01(v)` | → `float` | clamp to `[0,1]` |
| `d_color_clamp_range(v,min,max)` | → `float` | clamp to `[min,max]` |
| `d_color_fmodf(x,y)` | → `float` | truncated remainder, constexpr-friendly |
| `d_color_fabsf(x)` | → `float` | absolute value, constexpr-friendly |

## `color_rgb.h`

| Entity | Signature | Notes |
|---|---|---|
| `d_color_rgb` / `d_color_rgba` / `d_color_rgba_premul` | PODs | linear; straight alpha; premultiplied alpha |
| `d_color_rgb_u8` / `d_color_rgba_u8` | PODs | 8-bit sRGB channels |
| `d_color_rgb_hex` / `d_color_rgba_hex` | `uint32_t` | `0x00RRGGBB` / `0xRRGGBBAA` |
| `d_color_rgb_make(r,g,b)` | → `d_color_rgb` | raw, no clamp |
| `d_color_rgba_make(r,g,b,a)` / `d_color_rgba_premul_make(r,g,b,a)` | → resp. POD | raw |
| `d_color_rgb_u8_make(r,g,b)` / `d_color_rgba_u8_make(r,g,b,a)` | → resp. POD | `uint8_t` channels |
| `d_color_rgb_is_valid(c)` / `d_color_rgba_is_valid(c)` | → `bool` | all channels in `[0,1]` |
| `d_color_rgb_clamp(c)` / `d_color_rgba_clamp(c)` | → resp. POD | per-channel clamp to `[0,1]` |
| `d_color_srgb_to_linear_component(x)` / `d_color_linear_to_srgb_component(x)` | → `float` **RT** | sRGB transfer, one component |
| `d_color_rgb_from_srgb(c)` / `d_color_rgb_to_srgb(c)` | → `d_color_rgb` **RT** | whole-color gamma |
| `d_color_rgba_from_rgb(c, alpha)` | → `d_color_rgba` | attach straight alpha |
| `d_color_rgb_from_rgba(c)` | → `d_color_rgb` | drop alpha |
| `d_color_rgba_premultiply(c)` | → `d_color_rgba_premul` | straight → premultiplied |
| `d_color_rgba_unpremultiply(c)` | → `d_color_rgba` | premultiplied → straight (0 at α≈0) |
| `d_color_rgba_blend_over(src,dst)` | → `d_color_rgba_premul` | Porter-Duff "over", premultiplied |
| `d_color_rgba_blend_over_straight(src,dst)` | → `d_color_rgba` | "over" via pre/un-multiply |
| `d_color_rgb_from_u8(c)` / `d_color_rgb_to_u8(c)` | → resp. **RT** | gamma-aware 8-bit (de)code |
| `d_color_rgba_from_u8(c)` / `d_color_rgba_to_u8(c)` | → resp. **RT** | color gamma-coded, alpha linear |
| `d_color_rgb_from_hex(h)` / `d_color_rgb_to_hex(c)` | → resp. **RT** | `0x00RRGGBB` (de)code |
| `d_color_rgba_from_hex(h)` / `d_color_rgba_to_hex(c)` | → resp. **RT** | `0xRRGGBBAA` (de)code |
| `d_color_rgb_from_hex_string(str, out)` | → `bool` **RT** | parse `"#RRGGBB"`; writes on success |
| `d_color_rgba_from_hex_string(str, out)` | → `bool` **RT** | parse `"#RRGGBBAA"` |
| `d_color_rgb_hex_from_string(str)` | → `d_color_rgb_hex` **RT** | parse to packed; `0` on failure |
| `d_color_rgb_luminance(c)` / `d_color_rgba_luminance(c)` | → `float` | Rec. 709 (alpha ignored) |
| `d_color_rgb_contrast_ratio(a,b)` | → `float` | WCAG ratio `[1,21]` |
| `d_color_rgb_to_grayscale(c)` | → `d_color_rgb` | luminance in all channels |
| `d_color_rgb_invert(c)` | → `d_color_rgb` | per-channel `1 - x` |
| `d_color_rgb_lerp(a,b,t)` / `d_color_rgba_lerp(a,b,t)` | → resp. POD | linear interpolation |
| `d_color_rgb_from_temperature(kelvin)` | → `d_color_rgb` **RT** | blackbody approximation |

## `color_cmyk.h`

| Entity | Signature | Notes |
|---|---|---|
| `d_color_cmyk` | POD | c,m,y,k in `[0,1]` |
| `d_color_cmyk_make(c,m,y,k)` | → `d_color_cmyk` | raw |
| `d_color_cmyk_is_valid(c)` | → `bool` | all in `[0,1]` |
| `d_color_cmyk_clamp(c)` | → `d_color_cmyk` | per-channel clamp |

## `color_hsv.h`

| Entity | Signature | Notes |
|---|---|---|
| `d_color_hsv` | POD | h `[0,360)`, s,v `[0,1]` |
| `d_color_hsv_make(h,s,v)` | → `d_color_hsv` | raw |
| `d_color_hsv_is_valid(c)` | → `bool` | canonical ranges |
| `d_color_hsv_clamp(c)` | → `d_color_hsv` | wrap hue, clamp s,v |

## `color_hsl.h`

| Entity | Signature | Notes |
|---|---|---|
| `d_color_hsl` | POD | h `[0,360)`, s,l `[0,1]` |
| `d_color_hsl_make(h,s,l)` | → `d_color_hsl` | raw |
| `d_color_hsl_is_valid(c)` | → `bool` | canonical ranges |
| `d_color_hsl_clamp(c)` | → `d_color_hsl` | wrap hue, clamp s,l |

## `color_lab.h`

| Entity | Signature | Notes |
|---|---|---|
| `d_color_xyz` / `d_color_lab` | PODs | tristimulus / L\*a\*b\* |
| `D_COLOR_XYZ_WHITE_X/Y/Z` | macros | D65 reference white |
| `d_color_xyz_make(x,y,z)` / `d_color_lab_make(l,a,b)` | → resp. POD | raw |
| `d_color_lab_f(t)` / `d_color_lab_f_inv(t)` | → `float` **RT** | LAB nonlinearity + inverse |
| `d_color_xyz_is_valid(c)` / `d_color_lab_is_valid(c)` | → `bool` | xyz ≥ 0 / L in `[0,100]` |
| `d_color_xyz_clamp(c)` | → `d_color_xyz` | tristimulus made ≥ 0 |
| `d_color_lab_clamp(c)` | → `d_color_lab` | L `[0,100]`, a/b `[-128,127]` |

## `color_ycbcr.h`

| Entity | Signature | Notes |
|---|---|---|
| `d_color_ycbcr` | POD | Y `[0,1]`, Cb/Cr `[-0.5,0.5]` |
| `d_color_ycbcr_make(y,cb,cr)` | → `d_color_ycbcr` | raw |
| `d_color_ycbcr_is_valid(c)` | → `bool` | canonical ranges |
| `d_color_ycbcr_clamp(c)` | → `d_color_ycbcr` | clamp Y and Cb/Cr |

## `color_convert.h`

| Entity | Signature | Notes |
|---|---|---|
| `d_color_hsl_hue_to_rgb(p,q,t)` | → `float` | HSL hue-sector helper |
| `d_color_convert_rgb_to_hsl(c)` / `d_color_convert_hsl_to_rgb(c)` | → resp. | RGB ↔ HSL |
| `d_color_convert_rgb_to_hsv(c)` / `d_color_convert_hsv_to_rgb(c)` | → resp. | RGB ↔ HSV |
| `d_color_convert_hsl_to_hsv(c)` / `d_color_convert_hsv_to_hsl(c)` | → resp. | HSL ↔ HSV (via RGB) |
| `d_color_convert_rgb_to_cmyk(c)` / `d_color_convert_cmyk_to_rgb(c)` | → resp. | RGB ↔ CMYK |
| `d_color_convert_rgb_to_ycbcr(c)` / `d_color_convert_ycbcr_to_rgb(c)` | → resp. | RGB ↔ YCbCr (BT.601 full) |
| `d_color_convert_rgb_to_xyz(c)` / `d_color_convert_xyz_to_rgb(c)` | → resp. | RGB ↔ XYZ (sRGB/D65) |
| `d_color_convert_xyz_to_lab(c)` / `d_color_convert_lab_to_xyz(c)` | → resp. **RT** | XYZ ↔ LAB |
| `d_color_convert_rgb_to_lab(c)` / `d_color_convert_lab_to_rgb(c)` | → resp. **RT** | RGB ↔ LAB (via XYZ) |

## `color.h`

| Entity | Signature | Notes |
|---|---|---|
| `D_COLOR_PI` | `3.14159265f` | CIEDE2000 trigonometry |
| `d_color_rgb_adjust_saturation(c, amount)` | → `d_color_rgb` | scale HSL saturation, clamp |
| `d_color_rgb_adjust_brightness(c, amount)` | → `d_color_rgb` | scale HSL lightness, clamp |
| `d_color_rgb_rotate_hue(c, degrees)` | → `d_color_rgb` | rotate HSL hue, wrap |
| `d_color_delta_e(lab1, lab2)` | → `float` **RT** | CIEDE2000 on L\*a\*b\* |
| `d_color_rgb_delta_e(rgb1, rgb2)` | → `float` **RT** | CIEDE2000 via LAB conversion |
