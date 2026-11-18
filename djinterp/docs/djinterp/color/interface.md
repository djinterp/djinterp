# Color

The color module's headers: what each promises, who uses what, and what
stays fixed until the freeze of Friday 2026-12-11. The headers hold the full
contracts; this is the map.

## The rule until the freeze

From the agreement of 2026-11-13 until 12-11:

- **Nothing named here changes in a way that breaks a caller** without the
  agreement of those who use it (the table under "Who uses what").
- **Additions are allowed**: a new function, a new model, a new conversion.
  No member is added to a model's struct, in C or C++, since that changes
  its layout.
- **Every change is announced** with the delivery that makes it, to those
  who use what changed.

## The headers

| Header | What it is |
|---|---|
| `c/util/color/color_common.h` | the C kernel's shared pieces: how its functions are emitted, `D_COLOR_EPSILON`, `d_color_fabsf` |
| `c/util/color/color_rgb.h`, `color_cmyk.h`, `color_hsv.h`, `color_hsl.h`, `color_lab.h`, `color_ycbcr.h` | one model each, in C: its struct, making, validating, clamping; `color_rgb.h` also has `rgba` and interpolation, and `color_lab.h` has CIE XYZ as well as L\*a\*b\* |
| `c/util/color/color_convert.h` | conversions between the models, through RGB |
| `c/util/color/color.h` | everything above, plus the operations across models: saturation, brightness and hue through HSL, and the CIEDE2000 difference |
| `cpp/util/color/color_common.hpp` | `channel_t`, the model tags and `is_color_model` |
| `cpp/util/color/color_rgb.hpp`, `color_cmyk.hpp`, `color_hsv.hpp`, `color_hsl.hpp`, `color_ycbcr.hpp`, `color_cie_lab.hpp` | one model each, in C++, each deriving from its C struct |
| `cpp/util/color/color_convert.hpp` | every model, plus `color_cast`, `is_convertible_color` and, from C++20, the concepts `color_model_type` and `convertible_color_type` |

`cpp/util/color/color.hpp` is a reduced header that the PDF and CSS layers
include. It can't yet be included beside `color_cmyk.hpp` or
`color_convert.hpp`; a program that needs the conversions includes
`color_convert.hpp`.

**Levels.** The C face needs C99. The C++ face needs C++14 (C++11 for
`color_common.hpp`); below that its headers are empty rather than an error.

**Linking.** The C kernel's bodies are in the headers. A normal build links
`src/djinterp/c/util/color/*.c`, which emit one out-of-line definition of
each. Defined before inclusion, `D_COLOR_HEADER_ONLY` makes every function
`static inline` instead, and needs no `.c` file.

## What each promises

### Channels

- **A channel is a `float`**, in C and in C++ (`channel_t`).
- RGB channels run from 0 to 1. Linear RGB may leave that range; nothing
  clamps unless asked (`clamp`, `d_color_rgb_clamp`), and `is_valid` says
  whether every channel is inside it.

### `rgb` and `rgba`

- `rgb` is `r`, `g` and `b`; `rgba` adds `a`, straight (not premultiplied)
  alpha. Each derives from its C struct (`dj_color_rgb`, `dj_color_rgba`)
  and adds no state.
- **Constructors**: from channels; `rgba(rgb, alpha)`; and from the C struct,
  implicitly, so a kernel's result becomes a model. A default `rgb` is
  black, and a default `rgba` is transparent black: every channel 0.
- `rgba::to_rgb()` drops alpha, without premultiplying.
- `rgba` is a carrier, not a model: it has no model tag and takes no part
  in conversion.
- **`==` and `!=`** compare every channel exactly, alpha included. Either
  side may be the C struct, at every level from C++14, and both work in a
  constant expression. Two different models don't compare.

### Interpolation

- `lerp(a, b, t)`, for two `rgb` or two `rgba`, is `a * (1 - t) + b * t` in
  each channel, alpha included. It returns `a` at `t = 0` and `b` at
  `t = 1`, and clamps nothing: a `t` outside 0 to 1 extrapolates, and a
  channel may leave its range. It keeps its overload's type.
- In C, `d_color_rgb_lerp` and `d_color_rgba_lerp` give the same values.

### Conversion

- `color_cast<To>(from)` converts between any two models, through RGB.
  `is_convertible_color<From, To>` says whether a pair converts. A type
  that isn't a model doesn't compile.

### The CIEDE2000 difference

- `d_color_delta_e(lab1, lab2)` is CIEDE2000 with `k_L = k_C = k_H = 1`. It
  follows Sharma, Wu and Dalal's implementation notes (Color Research and
  Application 30(1), 2005) and reproduces their 34 test pairs to within
  10⁻⁴, in either order.
- It is 0 for a color and itself, and the same either way round.
- `d_color_rgb_delta_e` converts both RGB colors to L\*a\*b\* first.

## Who uses what

On the tree of 2026-11-17:

| Where | What it uses |
|---|---|
| the UI kit's `types.hpp`, and the front ends that include it (`ui/frontends/imgui.hpp`) | `rgb`, `rgba` (channel constructors, `rgba(rgb, alpha)`, `to_rgb()`, members `r`, `g`, `b`, `a`, the transparent default), `channel_t`, `lerp` |
| fonts (`cpp/text/font.hpp`) | `rgb`, the default font color |
| the PDF layer (`pdf_primitives.hpp`), and DTest's PDF reports through it | `rgb`, `rgba`, `cmyk`, `channel_t`, `is_color_model`, `color_cast`, through `color.hpp` |
| CSS (`cpp/text/css/css.hpp`) | `rgb`, `rgba`, `channel_t`, `is_color_model`, `color_cast`, through `color.hpp` |
