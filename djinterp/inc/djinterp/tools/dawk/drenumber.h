/*******************************************************************************
* djinterp [c][tools]                                                drenumber.h
*
* Headings made decimal, as the style guide numbers them, and the tables of
* contents that agree with them (dawk's decision 45: the third of the tools
* folded into dcheck).
*   SECTIONS. A section's heading in a box - between rules of `=` with or
* without a space after the `//`, or of slashes - numbered in Roman or in
* decimal, becomes the guide's box: `//` and 78 `=`, `// N.  NAME`, the rule
* again. A Roman numeral keeps its value, so a gap stays a gap. A box with no
* number is left as it is.
*   SUBSECTIONS. Under a numbered section, headings of one line counted in
* Roman, in one case and rising, become `// N.k    name` with the guide's
* rule of `-` after them, and no empty line. A list counted in letters (one
* with a letter that is no numeral) is left as written; so are headings with
* no section above them.
*   TABLES OF CONTENTS. A table is converted with its body: each numbered
* line with its section, the lines under it moved with its name, Roman
* children laid out as the guide's, empty lines taken out. A header with
* numbered sections and no table gets one, written after its first comment,
* to the fourth level - unless a section has no number or the numbers skip
* one, when the table would leave something out or invent it.
*   LEFT WHOLE. A file is left as it is, all of it, when converting part of it
* would number it two ways: a boxed heading indented (the guide's box starts
* in the first column), a heading of several lines in its box, a numeral with
* a second part (`IV.b`), a Roman subsection in a box of rules, or a table
* that disagrees with the body before anything changes.
*
* HOW TO USE
* ----------
*   dcheck --renumber --base DIR
*     converts every C and C++ file under DIR/inc and DIR/src that a
*     generator didn't write, and prints `converted: N files`, then a line
*     for each file converted or left whole, and why
*
* OPTIONS
* -------
*   None.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/tools/dawk/drenumber.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  Converting
*/

#ifndef DJINTERP_TOOLS_DAWK_DRENUMBER_H
#define DJINTERP_TOOLS_DAWK_DRENUMBER_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // FILE


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Converting
//------------------------------------------------------------------------------
/**
 * @brief Converts one file's text.
 *
 * @param[in]  _text     the file's bytes; its line ends are kept.
 * @param[in]  _length   how many.
 * @param[in]  _header   whether the file is a header, which a table of
 *                       contents may be written into.
 * @param[out] _out      the text converted, allocated, or `NULL` when
 *                       nothing changes or the file is left whole.
 * @param[out] _length_out  its length.
 * @param[out] _why      why the file is left whole, or `NULL`; a static
 *                       string, good until the next call.
 * @return `false` only when memory runs out.
 */
bool d_renumber_text(
    const char*  _text,
    size_t       _length,
    bool         _header,
    char**       _out,
    size_t*      _length_out,
    const char** _why
);

/**
 * @brief Converts every C and C++ file under `_base`'s `inc` and `src`.
 *
 * @param[in] _base    the tree's root.
 * @param[in] _out     where the summary and a line a file go.
 * @param[in] _report  where a failure to read or write is said.
 * @return 0 when every file was read and every conversion written, 2
 *         otherwise.
 */
int d_renumber_tree(
    const char* _base,
    FILE*       _out,
    FILE*       _report
);


#endif  // DJINTERP_TOOLS_DAWK_DRENUMBER_H
