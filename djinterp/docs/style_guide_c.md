# C Coding Style Guide
 
---
 
## General Formatting
 
### Spacing
 
#### Line Length
 
A line is measured in **characters**: the count of Unicode code points on the
line, not bytes and not display columns. The line terminator is not one of them,
so a line of exactly 80 characters is 81 bytes on disk with `LF` and 82 with
`CRLF`, and both are within the limit.
 
Three rules keep characters, columns, and the count a tool reports in agreement,
and all three are mandatory rather than stylistic:
 
-   **No tab characters** anywhere in a source file, for indentation or for
    alignment. A tab's width depends on the reader's settings, so a file
    containing one has no single well-defined line length.
-   **UTF-8 encoding, no byte-order mark.** Every source file must decode as
    valid UTF-8.
-   **Single-width characters only.** Every character must occupy one display
    column. Box-drawing characters, arrows, em dashes, the section sign, and
    Greek letters used in mathematical commentary are all fine. Characters with
    an East Asian Width of `Wide` or `Fullwidth`, and emoji, are not; represent
    them by code point rather than as literals.
Within that, the limit itself is a target rather than an absolute:
 
-   Code and comments should not exceed 80 characters whenever a reasonable and
    readable formatting exists within that limit.
-   Do not distort natural C or C++ syntax, introduce excessive indentation, or
    split an otherwise clear expression solely to satisfy the limit.
-   When a line can be broken naturally at function arguments, template
    arguments, operators, initializer elements, or other syntactic boundaries,
    prefer doing so rather than running long.
-   Indivisible or externally defined text — long identifiers, paths, URLs,
    string literals, or generated names — may exceed 80 characters when
    splitting it would reduce readability or alter its meaning.
-   Indentation depth is not indivisible text. A line that runs long because it
    sits five levels deep is a signal to extract a function, not an exemption.
Machine-generated files are exempt from this section. The generator conforms to
this guide; its output need not.
 
#### Whitespace
 
-   Use spaces, not tabs, without exception; see **Line Length** above
-   No trailing whitespace at the end of any line
-   Every file ends with exactly one newline
-   `LF` line endings; do not commit `CRLF`
-   Align related elements vertically where appropriate
### Include Guards
 
Every header carries an include guard derived mechanically from its path. The
derivation has **no exceptions**: the guard is a function of the file's location
and extension and nothing else. That is what makes a collision impossible rather
than merely unlikely, and it is what lets tooling verify and repair guards.
 
**Derivation**
 
1.  Start with `DJINTERP_`.
2.  Append the subsystem prefix for the top-level directory, from the table
    below. `/core` contributes nothing.
3.  Append every remaining directory segment, in order.
4.  Append the file's stem.
5.  Append `_H` for a `.h` file, `_HPP` for a `.hpp` file.
Uppercase the result and collapse every run of non-alphanumeric characters to a
single underscore. Do not merge repeated segments, do not drop segments, and do
not abbreviate. A guard that looks redundant is still correct.

**re_std is the exception.** It includes nothing from djinterp and builds on
its own, so its guards are derived from its own path: `RE_STD_`, then the
file's path under `inc/re_std/` with its extension, uppercased, every run of
non-alphanumeric characters collapsed to one underscore. `cstdint/cstdint.hpp`
is `RE_STD_CSTDINT_CSTDINT_HPP`, the extensionless umbrella `cstdint/cstdint`
is `RE_STD_CSTDINT_CSTDINT`, and `cstdint/dstdint.h` is
`RE_STD_CSTDINT_DSTDINT_H`. Its macros are spelled `RE_STD_`, never `D_`, and
its configuration is its own, `inc/re_std/config.hpp`.
 
**Subsystem table**
 
| Directory   | Guard prefix | Guard suffix | Macro prefix | Skippable | Namespace  |
|-------------|--------------|--------------|--------------|-----------|------------|
| `/djinterp` | `DJINTERP_`  | `_H`/`_HPP`  | `D_`         | no        | `djinterp` |
| `/3d`       | `3D_`        | `_H`/`_HPP`  | default      | no        | `render3d` |
| `/c`        | `C_`         | `_H`         | none         | yes       | none       |
| `/config`   | `CONFIG_`    | `_H`         | `CFG_`       | no        | none       |
| `/core`     | *(empty)*    | `_HPP`       | none         | yes       | none       |
| `/env`      | `ENV_`       | `_H`         | default      | no        | none       |
| `/jit`      | `JIT_`       | `_H`         | default      | no        | none       |
| `/math`     | `MATH_`      | `_H`/`_HPP`  | default      | no        | `math`     |
| `/net`      | `NET_`       | `_H`/`_HPP`  | default      | yes       | `net`      |
| `/parse`    | `PARSE_`     | `_H`/`_HPP`  | default      | yes       | `parse`    |
| `/parsegen` | `PARSEGEN_`  | `_H`/`_HPP`  | default      | yes       | `parsegen` |
| `/re_std`   | see above    | see above    | `RE_STD_`    | no        | `re_std`   |
| `/test`     | `TEST_`      | `_H`/`_HPP`  | default      | no        | `test`     |
 
The namespace column applies to the C++ tree; C code has no namespaces.
 
**Format**
 
```c
#ifndef DJINTERP_C_CONTAINER_MAP_HASH_MAP_INT_H
#define DJINTERP_C_CONTAINER_MAP_HASH_MAP_INT_H 1
 
// contents
 
#endif  // DJINTERP_C_CONTAINER_MAP_HASH_MAP_INT_H
```
 
**Rules**
 
-   No two headers may share a guard. Because the guard is derived from the
    path, this cannot happen unless the derivation is broken.
-   `/core` contributing an empty prefix means no subdirectory of `/core` may
    share a name with a top-level subsystem. `/core/net/` would collide with
    `/net/`.
-   The `#define` carries the value `1`.
-   The closing `#endif` names the guard in a trailing comment.
**Why there are no exceptions.** Shortening a guard by merging a repeated
segment looks deterministic and safe. It is not: across this project it produces
eight collisions, because an umbrella header and a same-named file inside its
own subdirectory shorten to the same symbol — `net/curl.hpp` against
`net/curl/curl.hpp`, `c/text/symbol/emoji.h` against
`c/text/symbol/emoji/emoji.h`. The saving is 1.5 characters on average. Guards
are read by the preprocessor, not for pleasure.
 
### Macro Names
 
Macros begin with `D_`, followed by the subsystem's macro prefix from the table
above, followed by enough of the path and name to be unambiguous.
 
Unlike include guards, macro names are **not** mechanically derived, and should
be shortened wherever the full path adds length without adding clarity:
 
-   Omit path segments a reader would supply anyway. `D_CFG_DMACRO_VARG_MAX`
    is better than `D_CONFIG_C_UTIL_DMACRO_VARG_MAX`.
-   Omit repeated segments.
-   Where the table marks a subsystem skippable, part or all of the path may be
    dropped for a macro that is widely used and unambiguous.
-   A macro prefix of `none` means the macro may follow `D_` directly with no
    path at all. This is for the universal kit — `D_STATIC`, `D_INLINE`,
    `D_EXTERN_C` — that appears in every file.
Judgement replaces derivation here, so the safety net differs. The rule is not
about the shape of the name: **no two macros anywhere in the project may share a
name with different definitions.** That is checkable without constraining the
name, and it is the only property that actually matters.
 
### Include Directives
 
**Include what you use.** Every file includes exactly what its own contents
need, and nothing more. A header includes what its declarations require; a
source file includes what its definitions require, and does not rely on its
corresponding header to supply those transitively.
 
Group include directives by origin, using short category comments such as
`// std` and `// djinterp`.
 
Every `#include` directive should include a very brief trailing `//` comment
summarizing why that dependency is needed whenever reasonably possible.
Standard-library headers should always identify the principal types, constants,
or functions for which they are included.
 
Keep the complete `#include` directive and its summary on one line whenever
doing so is readable. Additional comment lines may be used when they improve
readability. When a summary continues onto another line, align the continuation
with the first summary when practical.
 
Align trailing include comments vertically within an include group whenever
practical. There should be at least two spaces between the end of the include
directive and the beginning of its trailing `//` comment whenever possible.
 
An include of `djinterp.h` or `djinterp.hpp`, regardless of its relative path,
must use the exact summary `// framework root`, with one exception: a source
file's include of its own corresponding header says `// corresponding header`,
as every source's does, so `djinterp.c` includes `djinterp.h` with that one.
 
**The fixed-width integers come from re_std.** A C file includes re_std's
`cstdint/dstdint.h` and a C++ file `cstdint/cstdint.hpp`, never `<stdint.h>`,
`<cstdint>`, `<inttypes.h>` or `<cinttypes>`. Between them they give the types,
the limits, the constants and the formats at every language level and on every
platform, and declare no family the build cannot spell. C++ spells the names
`re_std::uint32_t`, never `std::uint32_t`; a name written without a namespace
stays as it is, since `dstdint.h` declares the global names. The include is
written like any other, a path relative to the file and a comment naming what
the file uses, in a `// re_std` group of its own right after the `// djinterp`
group: djinterp's configuration, which the framework root brings, is then read
before re_std's, so that the switches djinterp carries into re_std arrive
(`config/cfg_re_std.h` stops the build where they cannot).
`tools/check_stdint_includes.py` enforces the rule,
in CI as the `stdint-lint` workflow; `dstdint.h` itself and its conformance
harness are the only files exempt.
 
**Format:**
 
```c
// std
#include <errno.h>        // EINVAL, ERANGE
#include <stddef.h>       // size_t
#include <stdlib.h>       // malloc
#include <string.h>       // memcpy, memset
// djinterp
#include "../djinterp.h"  // framework root
```
 
**Additional-line example:**
 
```c
#include "../env/env.h"  // compiler and platform environment detection
                         // used by conditional portability definitions
```
 
### Brackets
 
-   All opening braces `{` on their own line
-   No bracketless `if`-statements (always use brackets) e.g.
```c
if (<condition>)
{
    ...
}
```
 
### Boolean Expressions
 
Boolean expressions with multiple conditions should be broken up into individual
lines, parenthesized, and enclosed in one set of top-level parentheses. Each
condition should be on its own line with the boolean operators aligned for
consecutive terms of similar length.
 
**Format:**
 
```c
if ( (!_array)                                        ||
     (!_elements)                                     ||
     ( (some_condition) &&
       (other_condition) )                            ||
     (!d_array_common_validate_params(_element_size)) ||
     (_count == 0) )
{
    // handle condition
}
```
 
**Rules:**
 
-   Enclose the entire expression in one set of top-level parentheses
-   Each condition on its own line
-   Line up the opening parentheses of conditions
-   Align boolean operators (`||`, `&&`) for readability
-   Add space between the operator and the next condition
-   Use judgment for what is easiest to read when aligning operators
-   Always parenthesize multi-condition boolean expressions
### Function Calls and Returns
 
Function calls and return statements should be broken across multiple lines
when:
 
1.  The function has more than 1 parameter, OR
2.  The full statement exceeds the normal 80-character target and a natural,
    readable break is available
**Format:**
 
```c
    return d_array_common_resize_amount(_ptr_array->elements,
                                        _ptr_array->count,
                                        sizeof(void*),
                                        _amount);
```
 
**Rules:**
 
-   Each parameter on its own line
-   All parameters should line up/start in the same column
-   Align parameters with the first parameter after the opening parenthesis
### Variable Declarations and Initialization
 
Validate parameters first. Then declare each variable at its point of first use
and give it a value in the same statement.
 
**Format:**
 
```c
struct some_struct*
d_some_struct_new_copy(
    const struct some_struct* _other
)
{
    // parameter validation first
    if (!_other)
    {
        return NULL;
    }
 
    const size_t count    = _other->count;
    const size_t capacity = _other->capacity;
 
    struct some_struct* result = malloc(sizeof(*result));
 
    // check if memory allocation was successful
    if (!result)
    {
        return NULL;
    }
 
    void* elements = calloc(capacity, sizeof(void*));
 
    // rest of function logic...
}
```
 
**Rules:**
 
-   Perform parameter validation before declaring or initializing anything
-   Declare each variable at its point of first use, not at the top of the
    function
-   Initialize a variable in its declaration; do not declare it uninitialized
    and assign to it later
-   Prefer `const` for any variable that is never reassigned
-   Align variable names vertically within a run of consecutive declarations
    when doing so aids scanning
**Exception:** in a C function that uses a single `goto` cleanup label, any
resource released at that label may be declared and initialized before the
first `goto`, so that every jump crosses a fully initialized declaration.
 
### Control Flow and Spacing
 
#### if-statements and loops
 
-   Most `if`-statements and loops should have a comment explaining their
    purpose.
-   `if`-statements should be followed by an empty line, unless the next line
    contains another closing bracket for an outer `if`-statement or `for`-loop.
**Example:**
 
```c
// check if memory allocation was successful
if (!result)
{
    return NULL;
}
 
// initialize the array elements
elements = malloc(size * sizeof(void*));
```
 
#### Return Statements
 
-   Return statements should be preceded by an empty line
-   Void functions must have an explicit empty `return;` statement at the end
**Example:**
 
```c
void
d_some_struct_free(
    struct some_struct* _some_struct
)
{
    if (_some_struct)
    {
        if (_some_struct->member)
        {
            free(_some_struct->member);
        }
 
        free(_some_struct);
    }
 
    return;
}
```
 
## Naming Conventions
 
### Prefixes
 
-   **Struct/union/enum types**: prefixed with `d_`
-   **Macros**: prefixed with `D_`, with the rest in ALL CAPS
-   **Named constants**: ALL CAPS, with no `D_` prefix, so a constant is
    distinguishable from a macro at the point of use
-   **Function parameters**: prefixed with a single leading underscore, e.g.
    `_param1`
C has no namespaces, so the `d_` prefix is the only collision protection
available to a type name. It is not decoration.
 
A **named constant** is a `const` object at file or block scope that stands in
for a literal — `MAX_PATH_LENGTH`, `DEFAULT_CAPACITY`. The rule is about that
role, not about the `const` qualifier: a `const` parameter, a `const` pointer,
or a `const` local holding a computed value is an ordinary variable and stays
`snake_case`.
 
```c
const size_t MAX_PATH_LENGTH = 4096;          // named constant
 
bool d_path_is_valid(const char* _path)       // const parameter, not a constant
{
    const size_t length = strlen(_path);      // const local, not a constant
    return length < MAX_PATH_LENGTH;
}
```
 
 
### Reserved Identifiers
 
Two forms are reserved to the implementation and must never appear in an
identifier this project defines, in any scope:
 
-   a leading underscore followed by an upper-case letter (`_Type`, `_Args`)
-   a double underscore anywhere (`FOO__BAR`, `__baz`)
C++ [lex.name]/3.1 and C17 §7.1.3 reserve these *for any use*, not merely at
file scope, because the implementation is free to define them as macros — and
macro substitution happens before scope exists. A template parameter is not
protected by being inside a template.
 
A single leading underscore followed by a lower-case letter is reserved only in
the global namespace and at file scope, which is why function parameters may use
`_param`. Do not extend it to file-scope names or struct tags.
 
Enable `-Wreserved-identifier` (Clang 13+) to catch violations.
 
## Comments
 
### Comment Style in Headers
 
-   Use `//` comments in header files by default.
-   Block comments (`/* ... */`) are permitted for:
    -   the initial file header comment block,
    -   the required table of contents, and
    -   a declaration's Doxygen `/** ... */` contract, as **Function
        Comments** describes.
-   Structural section, subsection, and item delineations use the
    `//` formats defined below.
-   Use `//` comments for ordinary documentation, brief entity comments,
    implementation notes, and other comments.
-   The first letter of ordinary comments should be lower-case unless the text
    begins with a proper noun, identifier, acronym, or other term whose
    capitalization should be preserved.
### Header File Header Comments
 
All C and C++ header files must begin with the standard banner comment block.
The banner is identical in every file: the same fields, in the same order, at
the same columns, with every line of fixed geometry ending on character 80.
 
**Format:**
 
```c
/*******************************************************************************
* djinterp [c]                                                        djinterp.h
*
* Brief description of the header file.
*   Extended description providing more details about the purpose and contents
* of this header.
*
* path:      /inc/djinterp/c/djinterp.h
* link(s):   TBA
* author(s): Author Name                                     created: YYYY.MM.DD
*                                                            revised: YYYY.MM.DD
*******************************************************************************/
```
 
**Rules:**
 
The banner is a fixed template. Every header and every source file carries the
same one, with the same fields in the same order at the same columns. There are
no optional fields and no local variants.
 
Geometry (counted in characters, as defined under **Line Length**):
 
-   The opening rule is `/` followed by 79 `*`, ending on character 80.
-   The closing rule is 79 `*` followed by `/`, ending on character 80.
-   On the title line, the filename's last character sits on character 80.
-   On the `author(s):` line, the `created:` value's last character sits on
    character 80. The `revised:` line aligns beneath it and ends there too.
-   `path:`, `link(s):`, and `author(s):` values all begin on character 14.
-   Description lines are prose and are not padded; they wrap at 80 characters like
    any other line.
Fields:
 
-   All five fields are mandatory: `path:`, `link(s):`, `author(s):`,
    `created:`, and `revised:`.
-   `link(s):` is spelled with the parenthesized plural. When there is no link
    yet, the value is the literal `TBA` rather than an omitted field.
-   `author(s):` uses one canonical spelling per person for the life of the
    project. Do not mix long and short forms of the same name.
-   `created:` records the original creation date and never changes afterward.
-   `revised:` records the most recent revision date.
-   Both dates use the format `YYYY.MM.DD`.
-   The title line's tag is the file's **top-level directory**, in square
    brackets, exactly as it appears in the subsystem table: `[c]`, `[core]`,
    `[config]`, `[env]`, `[math]`, `[net]`, `[parse]`, `[parsegen]`,
    `[re_std]`, `[test]`, `[jit]`, `[3d]`. It is derived from the path, not
    chosen, so a module name such as `[container]` or `[util]` is never a tag.
    This makes the tag checkable; a tag nobody can check drifts.
-   The filename is the file's own name, which must match the file on disk and
    the include guard derived from it.
### Header Structure, Numbering, and Tables of Contents
 
Header bodies that are divided into structural sections use decimal hierarchical
numbering at every level, the fourth included. Neither Roman numerals nor
letters are used.
 
Body hierarchy:
 
-   section: `1.`
-   subsection: `1.1`
-   item: `1.1.1`
-   fourth level: `1.1.1.1`
Top-level section numbers retain a period. Numbers at the second level and
below do not use a trailing period.
 
A fourth-level entry is a decimal marker in the body, `1.1.1.1`, and a local
ordinal in the table of contents, as **Fourth-Level Notation** shows.
 
A table of contents is required for every header containing multiple sections.
Any header that uses section/subsection/item delineations must also contain a
table of contents, even if it currently contains only one top-level section.
The table of contents must agree with the numbering and names used in the body.
 
#### Table of Contents Format
 
Tables of contents use local numbering at each indentation level. Parent numbers
are communicated by indentation and are not repeated on child entries.
 
Use this hierarchy:
 
```text
1.  TOP-LEVEL SECTION
    -----------------
    1.  Subsection
         1.  Item
              1.  Fourth-level entry
              2.  Fourth-level entry
```
 
Rules:
 
-   Do not use blank lines anywhere inside a table of contents.
-   Separate a top-level ordinal from its name with two spaces.
-   Align the top-level hyphen underline directly beneath the section name.
-   Indent subsection entries four spaces.
-   Indent item entries nine spaces.
-   Fourth-level entries are indented so that the ordinal begins **one column
    to the right of the first character of the parent item's name**.
-   Restart numbering at `1.` beneath each parent, at every level.
-   Fourth-level entries appear in the body exactly as the other levels do, as
    a `#.#.#.#` marker. Every ToC entry has a matching marker at its
    destination.
-   For single-digit item ordinals, retain the second spacing column before the
    item name, e.g. `1.  name`; double-digit ordinals use `10. name`.
Example:
 
```text
1.  DEFINED CONSTANTS
    -----------------
    1.  Architecture types
         1.  Architecture type identifiers
              1.  D_ENV_ARCH_TYPE_X86
              2.  D_ENV_ARCH_TYPE_X64
              3.  D_ENV_ARCH_TYPE_ARM
```
 
#### Section Delineation
 
Top-level section names are written in uppercase. Use two spaces between the
section ordinal and section name:
 
```c
//==============================================================================
// 1.  CONFIGURATION SYSTEM
//==============================================================================
// Controls variadic argument limits, macro variant selection, and provides
// user-overridable settings for maximum flexibility.
//
// CONFIGURATION HIERARCHY (highest to lowest priority):
//   1. D_CFG_DMACRO_OVERRIDE - if 1, use D_CFG_DMACRO_* values directly
//   2. D_CFG_DMACRO_VARG_MAX - user-specified max (if override enabled)
//   3. D_ENV_PP_MAX_MACRO_ARGS - environment-detected limit
//   4. D_CFG_DMACRO_VARG_DEFAULT (128) - fallback default
 
 
// 1.1    Configuration constants
//------------------------------------------------------------------------------
```
 
When a useful section-level description exists, place it immediately after the
closing `//==============================================================================`
line, with no empty line before the description. Omit the description when it
would merely repeat the section name.
 
Use **two empty lines** after the section-description block before the first
subsection. When a section has no description, use two empty lines after the
closing `//==============================================================================`
delimiter before its first subsection.
 
#### Subsection Delineation
 
Subsection names use their complete hierarchical number with no trailing
period, followed by a line of hyphens. Use at least two spaces between the
number and name; pad second-level numbers so subsection and item names align:
 
```c
// 1.1    Architecture types
//------------------------------------------------------------------------------
// 1.1.1
// Architecture type identifiers
```
 
Use **zero empty lines** after the
`//------------------------------------------------------------------------------`
delimiter. The subsection's first item, declaration group, or other content
begins on the immediately following line.
 
#### Item Delineation
 
Third-level item markers use their complete hierarchical number with no
trailing period and carry **no descriptive text** on the numbered line.
 
The item name or entity documentation begins on the immediately following line,
with **zero empty lines** after the item marker:
 
```c
// 1.1.1
// Architecture type identifiers
```
 
Concrete entities follow the same pattern:
 
```c
// 1.2.3
// D_KEYWORD_CONCURRENT
//   keyword: resolves to `concurrent`.
#define D_KEYWORD_CONCURRENT concurrent
```
 
#### Fourth-Level Notation
 
The fourth level uses decimal ordinals like every other level, producing a
`#.#.#.#` marker. In the ToC, place the ordinal one column to the right of the
first character of its parent item's name:
 
```text
    1.  Architecture types
         1.  Architecture type identifiers
              1.  D_ENV_ARCH_TYPE_X86
              2.  D_ENV_ARCH_TYPE_X64
              3.  D_ENV_ARCH_TYPE_ARM
```
 
The matching body markers are `1.1.1.1`, `1.1.1.2`, and `1.1.1.3`. Unlike the
lettered notation this replaces, a fourth-level ToC entry always has a
corresponding marker in the body, so every ToC line names a place a reader can
actually find.
 
#### Conditional-Block Closures
 
In header files, every `#endif` that closes an `#ifdef` or `#ifndef` block must
identify the controlling symbol in a trailing comment:
 
```c
#ifndef D_SOME_FEATURE
    ...
#endif  // D_SOME_FEATURE
```
 
This applies to the file include guard and to nested `#ifdef` / `#ifndef`
blocks. A closing `#endif` for an ordinary expression-based `#if` does not
require a comment unless one improves clarity.
 
### Source File Header Comments
 
All C and C++ definition/source files must begin with a descriptive metadata
header block. The corresponding header is included immediately after it.
 
**Format:**
 
```c
/*******************************************************************************
* djinterp [c]                                                        djinterp.c
*
* Brief description of the source file.
*   Extended description providing more details about the implementation
* contained in this file.
*
* path:      /src/djinterp/c/djinterp.c
* link(s):   TBA
* author(s): Author Name                                     created: YYYY.MM.DD
*                                                            revised: YYYY.MM.DD
*******************************************************************************/
#include "../../../inc/djinterp/c/djinterp.h"  // corresponding header
```
 
**Rules:**
 
-   Applies to both `.c` and `.cpp` definition/source files.
-   The first line identifies the framework/module and source filename.
-   Include a brief description of what the definition file implements,
    typically identifying the corresponding header.
-   The block includes `path:`, `link(s):`, `author(s):`, `created:`, and
    `revised:` metadata.
-   Align `revised:` vertically beneath `created:`.
-   Use the date format `YYYY.MM.DD`.
-   Include the corresponding `.h` or `.hpp` immediately after the banner.
-   The corresponding header is the first include. Any further headers the
    definitions require follow it, grouped by origin as described under
    **Include Directives**.
-   When a trailing include summary is used, write `// corresponding header`.
-   Do not place an include-category comment between the source banner and the
    corresponding include.
### Comment Formatting
 
-   Only indent the first line of each paragraph in a comment/function
    definition header, not every line.
-   Subsequent lines in the same paragraph align with the text rather than
    receiving the paragraph indentation again.
-   In ordinary header comments, separate genuinely distinct prose paragraphs
    when doing so improves readability.
-   Contract fields use Doxygen tags. See **Function Comments** below.
**Example:**
 
```c
/**
 * @brief Creates a `d_ptr_array` with the initial capacity specified.
 *
 * @note An initial size of 0 is valid and creates an empty array.
 *
 * @param[in] _initial_size  the initial capacity of the array, in pointer
 *                           elements.
 * @return the initialized array, or `NULL` if allocation failed.
 */
```
 
### Brief Comment Format
 
For structs, unions, classes, typedefs, and `#define` items:
 
```c
// <name>
//   <category>: <brief description>
```
 
e.g.
 
```c
// D_KEYWORD_FRAMEWORK_NAME
//   constant: keyword corresponding to the name of this framework.
#define D_KEYWORD_FRAMEWORK_NAME    djinterp
```
 
## Functions
 
### Function Declarations (Header Files)
 
Function declarations in header files must follow these rules:
 
1.  **Group functions by general functionality** — related functions should be
    placed together in logical sections or subsections.
2.  **Include parameter names, not just types.**
3.  **Pad between return type and function name** — all function names in a
    block should line up in the same column.
 
    *Exception:* where padding a long return type would carry a declaration
    past 80 characters, the return type may stand on its own line when that
    improves readability. Keep it as neat as the padded form: the return type
    starts where the block's return types start, and the function name
    starts on the next line in the block's name column, its parameters
    aligned as rule 4 describes.
 
    ```c
    int     d_registry_open(const char* _path);
    const struct d_registry_entry_descriptor_table*
            d_registry_descriptor_table(const struct d_registry* _registry);
    void    d_registry_close(struct d_registry* _registry);
    ```
 
4.  **Multi-line parameters** — when a declaration has more than one parameter,
    break across multiple lines. Align all parameters with the first parameter
    after the opening parenthesis. Align parameter names (the `_` character)
    vertically within each declaration.
5.  **No empty lines between functions** — functions within the same group have
    no blank lines separating them.
6.  **Use the common decimal header hierarchy** — section, subsection, and item
    delineations must follow the formats defined under **Header Structure,
    Numbering, and Tables of Contents**.
7.  **Keep the table of contents synchronized** — any header using structural
    delineations must contain matching ToC entries.
8.  **Use one blank line between distinct declaration groups when useful.**
9.  **Document declarations per the rules under Function Comments.** A group
    comment carries the shared contract for a run of related declarations; a
    function that allocates, invalidates, transfers ownership, or can fail
    carries its own block.
**Example (correct):**
 
```c
// 3.1    secure file opening
//------------------------------------------------------------------------------
FILE*   d_fopen(const char* _filename,
                const char* _mode);
int     d_fopen_s(FILE**      _stream,
                  const char* _filename,
                  const char* _mode);
FILE*   d_freopen(const char* _filename,
                  const char* _mode,
                  FILE*       _stream);
int     d_freopen_s(FILE**      _newstream,
                    const char* _filename,
                    const char* _mode,
                    FILE*       _stream);
FILE*   d_fdopen(int         _fd,
                 const char* _mode);
 
// 3.2    large file support
//------------------------------------------------------------------------------
int     d_fseeko(FILE*   _stream,
                 d_off_t _offset,
                 int     _whence);
d_off_t d_ftello(FILE* _stream);
int     d_ftruncate(int     _fd,
                    d_off_t _length);
int     d_ftruncate_stream(FILE*   _stream,
                           d_off_t _length);
 
// 3.3    file descriptor operations
//------------------------------------------------------------------------------
int     d_fileno(FILE* _stream);
int     d_dup(int _fd);
int     d_dup2(int _fd,
               int _fd2);
int     d_close(int _fd);
ssize_t d_read(int    _fd,
               void*  _buf,
               size_t _count);
ssize_t d_write(int         _fd,
                const void* _buf,
                size_t      _count);
int     d_open(const char* _path,
               int         _flags,
               ...);
```
 
**Example with longer return types:**
 
```c
// 3.4    directory operations
//------------------------------------------------------------------------------
int                d_mkdir(const char* _path,
                           uint32_t    _mode);
int                d_mkdir_p(const char* _path,
                             uint32_t    _mode);
int                d_rmdir(const char* _path);
struct d_dir_t*    d_opendir(const char* _path);
struct d_dirent_t* d_readdir(struct d_dir_t* _dir);
int                d_closedir(struct d_dir_t* _dir);
void               d_rewinddir(struct d_dir_t* _dir);
```
 
**Incorrect (do NOT do this):**
 
```c
/// d_arg_parser_init
///   function: initialize a parser with default settings.
/// Returns 0 on success, non-zero on failure.
int                  d_arg_parser_init(struct d_arg_parser* _parser);
 
int                  d_some_other_function(void);
```
 
This is wrong because:
-   Uses an obsolete decorative/unnumbered comment style instead of the
    decimal hierarchy
-   Documents a declaration in free text (`/// ... Returns 0 on success`)
    instead of a `/** */` contract with `@param` and `@return`, so
    `-Wdocumentation` has nothing to check
-   Has empty lines between functions in the same group
### Function Definitions
 
#### Standard Functions
 
For non-unit-test functions, use a compact structured block immediately above
the definition:
 
In the header, on the declaration -- the contract:
 
```c
/**
 * @brief Brief description of the function.
 *
 * @note    Optional note belonging to the description.
 * @warning Optional warning when misuse could be dangerous or destructive.
 *
 * @param[in]     _param1  description1.
 * @param[in,out] _param2  description2.
 * @pre    optional conditions that must already be true before the call.
 * @post   optional conditions guaranteed after a successful call, including
 *         any pointer or iterator this call invalidates.
 * @return `true` if the operation succeeds, `false` otherwise.
 */
bool             d_some_boolean_fn(const void*       _param1,
                                   const void* const _param2);
```
 
In the source, on the definition -- the mechanism:
 
```c
/*
d_some_boolean_fn
  Why this algorithm, what is subtle about it, and which invariant is easy to
break. No parameter list and no return description: those are in the header,
and a second copy of them would drift.
*/
bool
d_some_boolean_fn(
    const void*       _param1,
    const void* const _param2
)
{
    ...
}
```
 
**Function-comment rules:**
 
Contracts are written in Doxygen syntax. This is a format, not a build
dependency: clang parses these tags natively, so `-Wdocumentation` validates
them and `clangd` and Visual Studio render them in hover tooltips without any
generator being run. Running Doxygen itself remains possible and is nobody's
obligation.
 
| Tag | Use |
|---|---|
| `@brief` | one-sentence summary; the first sentence if `@brief` is omitted |
| `@param[in]` / `@param[out]` / `@param[in,out]` | one per parameter, direction always marked |
| `@pre` | precondition the caller must establish |
| `@post` | what holds after a successful call, including invalidation |
| `@return` | the value, and what it means on failure |
| `@retval` | a specific return value, when enumerating them is clearer |
| `@throws` | rare; see the exceptions policy in this guide |
| `@note` / `@warning` | description appendices, before the parameter block |
 
-   Every parameter gets a `@param` with an explicit direction. `-Wdocumentation`
    checks these names against the signature, which is the main reason to use
    the tags at all.
-   `@pre` and `@post` are optional but are the only place a caller learns about
    ownership transfer, invalidation, or required initialization. Prefer stating
    them over assuming they are obvious.
-   Parameter descriptions align vertically when practical.
-   Document the return value; omit `@return` entirely for `void` functions
    rather than writing `none`.
-   Use spaces, not tabs, for alignment.
**Where each comment goes.** The declaration comment describes how to use the
function; the definition comment describes how it works. Contract tags belong on
the declaration, in the header, because a consumer's translation unit contains
only the header — a contract on the definition is invisible to their editor and
absent from a binary distribution. The definition carries prose about mechanism:
why this algorithm, what is subtle, which invariant is easy to break. The two do
not duplicate each other.
 
**Not every declaration needs a block.** A group comment carries the shared
contract for a run of related declarations, which keeps aligned declaration
blocks intact:
 
```c
// basic accessors -- all O(1), none allocate, all require a non-NULL _string
size_t           d_string_length(const struct d_string* _string);
size_t           d_string_size(const struct d_string* _string);
const char*      d_string_cstr(const struct d_string* _string);
bool             d_string_is_empty(const struct d_string* _string);
```
 
A function carries its own block when it **allocates, invalidates, transfers
ownership, or can fail**. Those four are the cases a caller cannot infer from
the signature.
 
#### Void Function Requirements
 
Functions with a `void` return type must:
 
1.  Omit `@return` from the contract: Clang's `-Wdocumentation` reports a
    `@return` on a function returning `void`
2.  Include an explicit empty `return;` statement at the end of the function
**Example:**
 
Declaration, in the header:
 
```c
/**
 * @brief Frees a `d_some_struct` and all associated memory.
 *
 * @param[in] _some_struct  the struct to free; may be `NULL`.
 * @post Every pointer previously obtained from `_some_struct` is invalid.
 */
void             d_some_struct_free(struct some_struct* _some_struct);
```
 
Definition, in the source -- no `@return` for a `void` function:
 
```c
/*
d_some_struct_free
  Members are released before the struct itself so the free order stays
readable; the NULL check is first so callers need no guard of their own.
*/
void
d_some_struct_free(
    struct some_struct* _some_struct
)
{
    if (_some_struct)
    {
        if (_some_struct->member)
        {
            free(_some_struct->member);
        }
 
        free(_some_struct);
    }
 
    return;
}
```
 
#### Unit Test Functions
 
Simplified format for unit tests (both standalone AND DTest):
 
in the header (declarations) \*.h file:
 
```c
// group of tests name (corresponding one section of the module's header
// file, if applicable)
bool d_tests_some_feature(parameters);
bool d_tests_another_feature(parameters);
```
 
in the source (definitions) \*.c file:
 
```c
/*
d_tests_some_feature
  brief description
  Tests the following:
  - summary
  - of what
  - is
  - specifically tested
  - in this function
*/
bool
d_tests_some_feature(
    parameters
)
{
  ...
}
```
 
### Macros
 
-   Macros and definitions for very specific cases should be hidden with the
    `D_INTERNAL_` prefix.
-   All `#define` statements, including both constants and macros, should
    consist of the following format (excluding blocks of iterative macros, see
    below):
```c
// D_DEFINED_OR_MACRO_NAME
//   macro: a brief, concise description of the macro, using multiple
// lines if necessary to avoid going over 80 total characters.
#define D_DEFINED_OR_MACRO_NAME 42
 
// D_SOME_MACRO
//   macro: brief macros should be defined on the following line,
// as shown.
#define D_SOME_MACRO(param1)     MACRO_DEFINITION
 
// D_SOME_OTHER_MACRO
//   macro: a macro whose parameter list would go over 80
// characters of total line width should have its parameters
// spread out:
#define D_SOME_OTHER_MACRO(param1,                                \
                           param2,                                \
                           param3,                                \
                           param4,                                \
                           param5,                                \
                           param6)                                \
    SOME_OTHER_MACRO_DEFINITION
 
// D_INTERNAL_SOME_STRUCT
//   macro: a macro whose definition covers multiple lines,
// ESPECIALLY with macros that define blocks of code or entire
// functions, should be broken up into smaller lines, with the
// backslash characters ('\') lined up. Function definitions in
// macros should resemble normally-defined functions in formatting.
#define D_INTERNAL_SOME_STRUCT(ret_type,                          \
                               fn_name,                           \
                               param1,                            \
                               param2,                            \
                               other_fn_name,                     \
                               call_params)                       \
    ret_type                                                      \
    d_some_struct_hypothetical_example_fn(                        \
        struct d_some_struct* param1,                             \
        param2                                                    \
    )                                                             \
    {                                                             \
        if (param1)                                               \
        {                                                         \
            return other_fn_name(                                 \
                (struct d_some_struct*)param1,                    \
                param1->count,                                    \
                call_params);                                     \
        }                                                         \
                                                                  \
        return (ret_type){0}; /* or appropriate default */        \
    }
```
 
-   Iterative macros are macros that call one another in a chain, typically on
    adjacent lines with numerical suffixes. Frequently they are called by a
    root, or master function: e.g.
```c
#define D_EVAL(...)             D_INTERNAL_ARGS01(D_INTERNAL_ARGS01(D_INTERNAL_ARGS01(__VA_ARGS__)))
 
// D_INTERNAL_ARGS<01-10>
//   macro:
#define D_INTERNAL_ARGS01(...) D_INTERNAL_ARGS02(D_INTERNAL_ARGS02(D_INTERNAL_ARGS02(__VA_ARGS__)))
#define D_INTERNAL_ARGS02(...) D_INTERNAL_ARGS03(D_INTERNAL_ARGS03(D_INTERNAL_ARGS03(__VA_ARGS__)))
#define D_INTERNAL_ARGS03(...) D_INTERNAL_ARGS04(D_INTERNAL_ARGS04(D_INTERNAL_ARGS04(__VA_ARGS__)))
#define D_INTERNAL_ARGS04(...) D_INTERNAL_ARGS05(D_INTERNAL_ARGS05(D_INTERNAL_ARGS05(__VA_ARGS__)))
#define D_INTERNAL_ARGS05(...) D_INTERNAL_ARGS06(D_INTERNAL_ARGS06(D_INTERNAL_ARGS06(__VA_ARGS__)))
#define D_INTERNAL_ARGS06(...) D_INTERNAL_ARGS07(D_INTERNAL_ARGS07(D_INTERNAL_ARGS07(__VA_ARGS__)))
#define D_INTERNAL_ARGS07(...) D_INTERNAL_ARGS08(D_INTERNAL_ARGS08(D_INTERNAL_ARGS08(__VA_ARGS__)))
#define D_INTERNAL_ARGS08(...) D_INTERNAL_ARGS09(D_INTERNAL_ARGS09(D_INTERNAL_ARGS09(__VA_ARGS__)))
#define D_INTERNAL_ARGS09(...) D_INTERNAL_ARGS10(D_INTERNAL_ARGS10(D_INTERNAL_ARGS10(__VA_ARGS__)))
#define D_INTERNAL_ARGS10(...) __VA_ARGS__
```
 
Since all of the subfunctions called by `D_EVAL` are only called by `D_EVAL`,
and will probably only ever be used in that context, one comment for the entire
block is sufficient.
 
## Language Standard and Diagnostics
 
The framework's floor is **C99**, and MSVC's default C mode is supported.
Every C unit compiles with GCC and Clang at `-std=c99` and `-std=c11` under
`-pedantic-errors` and `-Werror=undef`, plain and in testing mode
(`ci/check_c_standards.sh`). Anything newer than C99 is used only behind the
environment layer's tests (`D_ENV_LANG_IS_C11_OR_HIGHER` and its kin, defined
at every level), with a C99 spelling below it.
Vendor extensions are not used outside the
environment-detection layer, which exists to isolate them.
 
Feature-test macros come from the build, never from a source file: the names
are reserved, and a source reaches its first system header through its
corresponding header, before a line of its own could define one. Where the C
library hides POSIX in a strict ISO mode, as glibc and musl do, every build
defines `_XOPEN_SOURCE=700` (POSIX.1-2008 with XSI), as the ladders do;
without it the `c/fs` sources, `dtime.c` and `dmutex.h`, a public header, do
not compile. Elsewhere leave it undefined: on macOS and the BSDs it hides
the system's own extensions, `timegm` among them. A file that needs more
than POSIX and XSI says so in its banner.
 
On Windows, the build defines `_WIN32_WINNT` to the oldest version it
targets, as Windows practice has it (`-D_WIN32_WINNT=0x0A00` for Windows 10).
`env_windows.h` reads it, with `NTDDI_VERSION`, and never sets one: including
`<sdkddkver.h>` would set a default, and a file that then defines its own
gets a redefinition. Without it, every version-gated Windows feature reads 0.
 
Builds are warning-clean. The baseline is `-Wall -Wextra -Werror`, plus:
 
| Flag | Catches |
|---|---|
| `-Wreserved-identifier` (Clang 13+) | identifiers reserved to the implementation; see **Reserved Identifiers** |
| `-Wdocumentation` (Clang) | a `@param` whose name no longer matches the signature |
| `-Wshadow` | a declaration hiding an outer name |
| `-Wconversion` | implicit narrowing |
| `-Wundef`, an error in the ladders (`-Werror=undef`) | an `#if` that reads an undefined macro as 0; test a flag that may be absent with `defined()` |
 
A warning that is genuinely not worth fixing is suppressed at the narrowest
possible scope with a comment explaining why — never project-wide.
 
## Assertions
 
Assert liberally, and prefer compile-time assertions to run-time ones wherever
the condition is knowable at compile time. Spell it
`D_STATIC_ASSERT(condition, message)`, from the root: `_Static_assert` from
C11, `static_assert` in C++11 and up, and below both a declaration that fails
to compile when the condition is false. It reads the same in both languages
and at every level, which neither standard spelling does from C99.
 
-   Use a static assertion for every assumption about type size, alignment,
    layout, or trait satisfaction that the code silently depends on.
-   Give every static assertion a message. The message is what the next person
    reads when the build breaks; the expression is rarely enough on its own.
-   Run-time assertions check preconditions the type system cannot. They are a
    statement about a bug, never a substitute for validating input.
## The Preprocessor
 
Object-like `#define` for constants is fine. Function-like macros are not the
default tool: prefer a `static inline` function or an `enum` constant wherever one will do. A macro has no
scope, no type checking, does not appear in a debugger or a stack trace, and
cannot be stepped through.
 
**Carve-out.** Preprocessor recursion, token pasting, and macro families that
emit declarations are permitted inside `c/util/macro/` and the `D_INTERNAL_`
argument-count machinery. Variadic argument handling in C genuinely requires
them and there is no alternative construct. Outside that directory, a
function-like macro needs a reason that survives review.
 
Where a macro is unavoidable:
 
-   Parenthesize every parameter use and the whole body.
-   Name it in ALL CAPS with the `D_` prefix so call sites show what it is.
-   Evaluate each parameter exactly once, or document that it does not.
### Conditional Compilation
 
Every `#if` doubles the number of configurations that exist and halves the
fraction of the code any single build tests. Minimize it.
 
-   Include guards are the one unremarkable use.
-   The configuration subframework under `config/` is built on conditional
    compilation by design; the rule does not constrain it.
-   Elsewhere, prefer a run-time branch on a compile-time constant, so both
    sides of the branch are always compiled and type-checked.
-   Never let an `#if` split a function body such that the two arms have
    different control flow. Select an implementation at the function level.
-   Every `#endif` that closes an `#ifdef` or `#ifndef` names its symbol; see
    **Conditional-Block Closures**.
## Control Transfer
 
`goto` is permitted where it **removes** duplication or nesting. It is not
permitted where it adds either. That is the whole rule; the cases below are
what it usually looks like in practice, not an exhaustive list to argue about.
 
-   Resource cleanup and error unwinding through a single exit label
-   Escaping several levels of loop at once, which C has no labelled `break` for
-   Dispatch in state machines, parsers, and interpreters
-   A restart or retry point
-   A shared continuation reached from several places
-   Generated code not intended to be read
Regardless of the reason:
 
-   **Jump forward only.** The one exception is a retry point, which is a
    backward jump by definition and should be obvious enough at the label that
    a reader sees the loop.
-   **Never jump into a block.** In C++ crossing an initialization is
    ill-formed; in C it compiles and leaves the variable uninitialized, which is
    worse.
-   **One label per function** for cleanup. Several unwind labels means the
    function is doing several things.
-   Name the label for what happens there — `cleanup`, `retry`, `done` — not
    where it is.
A `goto` that jumps past a declaration, or that exists because the function is
too long, is a signal to extract a function rather than a use of this rule.
 
## Function Size and Complexity
 
A function should do one thing, and the thing should be nameable without
"and". The prose is the rule; the numbers below are the tripwire that catches
what the prose misses.
 
| Measure | Limit | Escalation |
|---|---|---|
| Length | 60 lines, excluding the comment block | Extract a helper |
| Brace nesting | 4 levels inside the body | Early return, or a `goto` cleanup label where this guide permits one |
| Parameters | 5 | Group related parameters into a struct |
 
Exceeding a limit is not forbidden, it is a signal that needs a reason. A
dispatch table, a generated function, or a state machine's switch may legitimately
run long; a 90-line function that validates, transforms, and writes should be
three functions.
 
Indentation depth is not a licence to run past 80 characters. A line that is
long because it sits four levels deep is telling you about the nesting, not the
line.
 
## Typedefs
 
-   Do not use `typedef` to hide whether a type is a `struct`, `union`, or
    `enum`; that information should remain explicit at the point of use.
-   `typedef` is permitted for function-pointer types, portability aliases,
    and deliberate scalar abstraction types.
## File Structure
 
### Header Files (.h)
 
Header files should contain:
 
1.  A header comment block at the top of the file (copyright, description, etc.)
2.  A table of contents when required by the common header-structure rules
3.  Include guards
4.  All `#include` directives for dependencies, with brief trailing summaries
5.  Type definitions (structs, unions, enums) — with brief comments
6.  Macro definitions — with brief comments
7.  Function declarations — grouped by functionality, documented as
    **Function Comments** describes
**Header File Formatting Rules:**
 
-   Headers with multiple sections must contain a table of contents.
-   Any header using structural delineations must use the common decimal
    hierarchy and must contain a matching table of contents.
-   Top-level section names are uppercase.
-   Subsection and item names use ordinary comment capitalization.
-   Pad return types so all function names in a declaration block align,
    except where rule 3 of **Function Declarations** lets a return type too
    long to pad take its own line.
-   No empty lines between function declarations in the same group.
-   One empty line between different logical declaration groups when useful.
-   A declaration carries a comment of its own only as the `/** */`
    contract above it, for a function that allocates, invalidates,
    transfers ownership, or can fail; the rest share their group's
    comment, and nothing goes beside a declaration.
**Example header file structure:**
 
```c
/*******************************************************************************
* djinterp [c]                                                             bar.h
*
* Brief description of the header file.
*   Extended description providing more details about the purpose and contents
* of this header.
*
* path:      /inc/djinterp/c/foo/bar.h
* link(s):   TBA
* author(s): Author Name                                     created: YYYY.MM.DD
*                                                            revised: YYYY.MM.DD
*******************************************************************************/
 
/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Types
         1.  d_some_type
    2.  Constants
         1.  D_SOME_CONSTANT
2.  OPERATIONS
    ----------
    1.  Initialization
    2.  Core operations
    3.  Utilities
*/
 
#ifndef DJINTERP_C_FOO_BAR_H
#define DJINTERP_C_FOO_BAR_H 1
 
// std
#include <stddef.h>  // size_t
// djinterp
#include "../djinterp.h"   // framework root
#include "./dependency.h"  // module dependency
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t
 
 
//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================
// Defines the module's public data types and compile-time constants.
 
 
// 1.1    types
//------------------------------------------------------------------------------
// 1.1.1
// d_some_type
//   type: brief description of the type.
struct d_some_type
{
    int    member1;     // description
    char*  member2;     // description
};
 
// 1.2    constants
//------------------------------------------------------------------------------
// 1.2.1
// D_SOME_CONSTANT
//   constant: brief description.
#define D_SOME_CONSTANT 42
 
 
//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// Declares the operations exposed by the module.
 
 
// 2.1    initialization
//------------------------------------------------------------------------------
int  d_init(void);
void d_shutdown(void);
 
// 2.2    core operations
//------------------------------------------------------------------------------
int         d_create(const char* _name,
                     int         _flags);
int         d_destroy(int _handle);
const char* d_get_name(int _handle);
 
// 2.3    utilities
//------------------------------------------------------------------------------
int  d_validate(const void* _data,
                size_t      _size);
void d_debug_print(const char* _message);
 
 
#endif  // DJINTERP_C_FOO_BAR_H
```
 
### Source Files (.c)
 
Source files should contain:
 
1.  The source-file header comment block defined in this guide
2.  The `#include` directive for the corresponding header file, using a
    relative path, immediately after the source-file header
3.  Any further `#include` directives the definitions require, grouped by
    origin
4.  Function definitions
-   After the last `#include`, use exactly two empty
    lines before the first function comment.
-   No other location in a `.c` file should contain two consecutive empty
    lines. Elsewhere, use at most one empty line between logical units.
**A header must not carry includes on behalf of its source file.** Anything a
definition needs but a declaration does not — the allocator, `memcpy`, a
logging facility — belongs in the `.c` file. Pushing it into the header makes
every consumer pay for it and rebuilds them all when it changes.
 
**Example source file structure:**
 
```c
/*******************************************************************************
* djinterp [c]                                                             bar.c
*
* Brief description of the source file.
*   Extended description providing more details about the implementation
* contained in this file.
*
* path:      /src/djinterp/c/foo/bar.c
* link(s):   TBA
* author(s): Author Name                                     created: YYYY.MM.DD
*                                                            revised: YYYY.MM.DD
*******************************************************************************/
#include "../../../../inc/djinterp/c/foo/bar.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy
 
 
/*
d_some_function
  How the function works: the approach taken, anything subtle about it, and any
invariant a future maintainer could break without noticing. The contract --
parameters, preconditions, return value -- lives on the declaration in the
header and is not repeated here.
*/
int
d_some_function(
    int                        _param1,
    struct d_some_struct_type* _param2,
    const char*                _param3
)
{
    ...
}
```
