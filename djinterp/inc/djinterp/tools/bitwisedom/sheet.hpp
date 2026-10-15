/*******************************************************************************
* bitwisedom [bitwisedom]                                              sheet.hpp
*
* The sheet: a tree of symbols, each row one symbol mapped to a value.
*   A sheet is the document bitwisedom edits and saves. It has a width -- the
* bits a value may use: up to 64, an integer's, or wider, up to MAX_WIDTH,
* when its values are sets of bits that C holds as arrays of words -- and a
* tree of rows. A row is a group or
* a symbol; groups hold rows and collapse like the nodes of a tree. A symbol
* is one C macro to be: a name, mapped to a value, and the definition the
* value comes from --
*
*     bit          one bit:                          (1u << 3)
*     range        a run of bits, as a mask:         (0x7u << 4)
*     field_value  a number placed in a range,       (0x2u << 4)
*                  in one of five encodings
*     literal      a number, positive or negative:   0x1234u, (-3)
*     expression   an expression over other symbols: (D_READ | D_WRITE)
*     macro        a macro, verbatim:                D_IS_SET(x) (...)
*                  function-like, or object-like and
*                  valued by evaluating it as C
*
*   NAMES are built from affixes: the sheet's prefix, then each enclosing
* group's prefix, outermost first, then the row's own; the suffixes close in
* the reverse order. A group may isolate itself from the groups around it.
* The same cascade carries the literal format -- radix and integer suffix --
* down from the sheet to the row.
*   The sheet also carries how it is looked at and how it is written out:
* the grid's lines, cells and columns, its colours, and the C/C++ generator's
* settings, so a saved sheet opens looking as it was left. Its colours may
* each be left automatic, and most are: the window's theme -- dark or light
* -- and its colour scheme then choose them, and a sheet that sets one
* keeps it whichever theme it is opened in.
*   Rows have ids, unique within a sheet and never reused; everything that
* refers to a row (a field_value's range, the selection, a drag) holds an
* id, so rows can move without breaking references.
*
* path:      /inc/djinterp/tools/bitwisedom/sheet.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Enumerations
    2.  Colours
2.  ROWS
    ----
    1.  row
3.  SETTINGS
    --------
    1.  Grid
    2.  Colours, themes and schemes
    3.  Generator
4.  THE SHEET
    ---------
    1.  sheet
    2.  Tree operations
*/

#ifndef BITWISEDOM_SHEET_HPP
#define BITWISEDOM_SHEET_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <functional>  // std::function
#include <string>      // std::string
#include <vector>      // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./bits.hpp"  // u64, i64, encoding
// re_std
#include "re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


namespace bitwisedom
{


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// row_kind
//   enum: whether a row holds rows or defines a symbol.
enum class row_kind : int
{
    group = 0,
    symbol
};

// 1.1.2
// def_kind
//   enum: where a symbol's value comes from.
enum class def_kind : int
{
    bit = 0,
    range,
    field_value,
    literal,
    expression,
    macro
};

// 1.1.3
// DEF_KIND_COUNT
//   constant: how many definition kinds there are.
const int DEF_KIND_COUNT = 6;

// 1.1.4
// radix
//   enum: how a literal is written in the generated header. shift writes a
// bit as (1u << n) and a run as (mask << lsb).
enum class radix : int
{
    inherit = -1,
    hex     = 0,
    dec,
    oct,
    bin,
    shift
};

// 1.1.5
// int_suffix
//   enum: the integer suffix a generated literal carries.
enum class int_suffix : int
{
    inherit = -1,
    none    = 0,
    u,
    ul,
    ull
};

// 1.1.6
// numbering
//   enum: which bit is called bit 0 on screen. The generated header always
// counts from the least significant bit, as C does.
enum class numbering : int
{
    lsb0 = 0,
    msb0
};

// 1.1.7
// byte_order
//   enum: which byte is called byte 0, for byte ranges and labels.
enum class byte_order : int
{
    little = 0,
    big
};

// 1.1.8
// line_style
//   enum: how a grid line is drawn.
enum class line_style : int
{
    solid = 0,
    dashed,
    dotted
};

// 1.1.9
// collapsed_style
//   enum: what a collapsed column segment shows of each row.
enum class collapsed_style : int
{
    hex = 0,
    dots,
    count,
    blank
};

// 1.1.10
// grid_fill
//   enum: what takes the width the grid has beyond what it needs: the bits'
// cells, widened alike; the name column; or nothing, the grid ending where
// its columns do.
enum class grid_fill : int
{
    cells = 0,
    names,
    none
};

// 1.1.11
// colour_mode
//   enum: where a set bit's colour comes from.
enum class colour_mode : int
{
    row = 0,
    group,
    kind,
    accent
};

// 1.1.12
// data_model
//   enum: the widths C leaves to the target, as the evaluator of macros
// takes them. lp64 is Linux's and macOS's 64-bit model (a 64-bit long),
// llp64 Windows' (a 32-bit long), ilp32 any 32-bit target's.
enum class data_model : int
{
    lp64 = 0,
    llp64,
    ilp32
};

// 1.1.13
// DATA_MODEL_COUNT
//   constant: how many data models there are.
const int DATA_MODEL_COUNT = 3;

// 1.2    Colours
//------------------------------------------------------------------------------
// 1.2.1
// rgba
//   type: a colour packed as 0xRRGGBBAA. Zero, transparent black, is never a
// useful colour for a row, a role or a line, so a zero means "choose one for
// me": the palette chooses a row's, the theme and scheme everything else's.
using rgba = re_std::uint32_t;

// colour helpers -- packing, unpacking, and the automatic palette
D_NODISCARD rgba   make_rgba(float _r,
                             float _g,
                             float _b,
                             float _a);
D_NODISCARD float  channel(rgba _colour,
                           int  _index);
D_NODISCARD rgba   with_alpha(rgba  _colour,
                              float _alpha);
D_NODISCARD rgba   from_hsv(float _h,
                            float _s,
                            float _v,
                            float _a);
D_NODISCARD rgba   palette_colour(int   _palette,
                                  int   _index,
                                  float _saturation,
                                  float _value);
D_NODISCARD rgba   or_else(rgba _colour,
                           rgba _automatic);
D_NODISCARD std::string  rgba_text(rgba _colour);
D_NODISCARD bool         parse_rgba(const std::string& _text,
                                    rgba&              _colour);


//==============================================================================
// 2.  ROWS
//==============================================================================


// 2.1    row
//------------------------------------------------------------------------------
// 2.1.1
// row
//   struct: one row of the sheet -- a group, or a symbol and its definition.
// Every field is meaningful for the kinds its comment names; the others keep
// their values, so switching a symbol's kind back and forth loses nothing.
struct row
{
    // every row
    int          id    = 0;
    row_kind     kind  = row_kind::symbol;
    std::string  name;
    std::string  short_name;   // the abbreviation the grid may show
    std::string  note;         // the brief comment in the generated header
    rgba         colour    = 0;
    bool         transparent = false;  // no fill (distinct from automatic)
    bool         emit      = true;
    std::string  prefix;
    std::string  suffix;

    // groups
    bool         collapsed = false;
    bool         isolate   = false;   // ignore enclosing groups' affixes
    bool         name_children = false;  // use this container's name as a prefix
    bool         exclusive = true;    // warn when its allocations overlap

    // symbols
    def_kind     def      = def_kind::bit;
    int          bit      = 0;        // bit
    int          lsb      = 0;        // range; field_value without a field
    int          width    = 1;        // range; field_value without a field
    int          field    = 0;        // field_value: the range row's id
    encoding     enc      = encoding::unsigned_binary;
    i64          bias     = 0;        // field_value, offset binary
    i64          number   = 0;        // field_value: the number placed
    wide         literal;             // literal: the bits
    bool         negative = false;    // literal: written as negative
    int          digits   = 0;        // literal: at least this many digits
    std::string  expr;                // expression
    std::string  params;              // macro: "x, v", or "x, ..."
    std::string  body;                // macro: C, with {name} for symbols;
                                      // its lines keep their indentation
    bool         callable = false;    // macro: function-like, even with no
                                      // parameters -- NAME()
    radix        fmt      = radix::inherit;
    int_suffix   suf      = int_suffix::inherit;

    std::vector<row> children;
};


// A group or range can contain nested rows; all other symbols are leaves.
inline bool
can_contain(const row& _row)
{
    return (_row.kind == row_kind::group) ||
           ((_row.kind == row_kind::symbol) &&
            (_row.def == def_kind::range));
}


//==============================================================================
// 3.  SETTINGS
//==============================================================================


// 3.1    Grid
//------------------------------------------------------------------------------
// 3.1.1
// line_spec
//   struct: one kind of grid line -- whether it shows, and how. Its colour is
// automatic while it is zero.
struct line_spec
{
    bool       show      = true;
    float      thickness = 1.0f;
    rgba       colour    = 0;
    line_style style     = line_style::solid;
};

// 3.1.2
// column_segment
//   struct: a named run of columns, collapsible to one narrow column.
struct column_segment
{
    std::string name;
    int         lsb       = 0;
    int         msb       = 7;
    rgba        colour    = 0;
    bool        collapsed = false;
};

// 3.1.3
// grid_settings
//   struct: how the grid is drawn: its lines, cells, columns and labels.
struct grid_settings
{
    bool            lines = true;     // every grid line at once
    line_spec       minor;            // between bits
    line_spec       major;            // every major_every bits
    line_spec       rows;             // between rows
    line_spec       groups;           // above each group
    line_spec       segments;         // between column segments
    line_spec       border;           // around the grid
    int             major_every   = 8;
    float           cell_width    = 20.0f;
    float           cell_height   = 22.0f;
    float           cell_gap      = 2.0f;
    float           rounding      = 3.0f;
    float           name_width    = 190.0f;
    float           value_width   = 104.0f;
    float           indent        = 14.0f;
    int             index_every   = 1;
    int             auto_segments = 8;      // 0, 4, 8 or 16 bits
    bool            show_indices  = true;
    bool            show_digits   = false;
    bool            show_probe    = true;
    bool            show_usage    = true;
    bool            show_labels   = true;   // a range's name across its cells
    bool            show_cell_boxes = false;  // draw each cell as its own box
    bool            full_names    = false;  // macro names, not local ones
    bool            abbreviate_unused = false;
    int             unused_run    = 4;
    collapsed_style collapsed     = collapsed_style::hex;
    grid_fill       fill          = grid_fill::cells;
};

// 3.2    Colours, themes and schemes
//------------------------------------------------------------------------------
// 3.2.1
// THEME_COUNT / SCHEME_COUNT
//   constants: the window's themes -- 0 dark, 1 light -- and its colour
// schemes -- 0 vivid, 1 pastel, 2 earth, 3 neon, 4 colour-blind safe. A
// scheme's palette, for automatic row colours, has the scheme's number.
const int THEME_COUNT  = 2;
const int SCHEME_COUNT = 5;

// theme_name / scheme_name
//   the name a menu shows.
D_NODISCARD const char*  theme_name(int _theme);
D_NODISCARD const char*  scheme_name(int _scheme);

// 3.2.2
// colour_settings
//   struct: how a sheet is coloured. A zero colour, and a negative palette,
// saturation or brightness, is automatic: the theme and scheme the sheet is
// shown with choose it. `theme` is the theme a look was made for -- it is
// the window's, and is not saved with the sheet.
struct colour_settings
{
    int         theme       = 0;
    int         palette     = -1;
    float       saturation  = -1.0f;
    float       value       = -1.0f;
    colour_mode mode        = colour_mode::row;
    float       set_alpha   = 0.92f;
    float       range_alpha = 0.30f;
    rgba        accent      = 0;
    rgba        empty_cell  = 0;
    rgba        collision   = 0;
    rgba        probe       = 0;
    rgba        selection   = 0;
    rgba        hover       = 0;
    rgba        ghost       = 0;
};

// 3.2.3
// scheme_colours
//   struct: what a colour scheme gives one theme: the window's surfaces --
// behind everything, behind the panels, behind fields, and the text -- and
// a value for everything a sheet may leave automatic.
struct scheme_colours
{
    rgba            window = 0;
    rgba            panel  = 0;
    rgba            frame  = 0;
    rgba            text   = 0;
    colour_settings colours;
    rgba            minor    = 0;
    rgba            major    = 0;
    rgba            rows     = 0;
    rgba            groups   = 0;
    rgba            segments = 0;
    rgba            border   = 0;
};

// scheme_for
//   a scheme's colours for a theme; out-of-range numbers are held to the
// nearest.
D_NODISCARD scheme_colours  scheme_for(int _theme,
                                       int _scheme);

// 3.2.4
// look
//   struct: a sheet's colours and grid as the window draws them, every
// automatic colour filled in.
struct look
{
    colour_settings colours;
    grid_settings   grid;
};

// make_look
//   colours and a grid with what they leave automatic filled in from a
// theme and a scheme.
D_NODISCARD look  make_look(const colour_settings& _colours,
                            const grid_settings&   _grid,
                            int                    _theme,
                            int                    _scheme);

// 3.3    Generator
//------------------------------------------------------------------------------
// 3.3.1
// language / gen_form / gen_style / standard
//   enums: what the generator writes, and for which language level.
enum class language : int { c = 0, cpp };
enum class gen_form : int { macros = 0, constants };
enum class gen_style : int { djinterp = 0, plain };
enum class standard : int
{
    c99 = 0, c11, c17, c23,
    cpp98, cpp11, cpp14, cpp17, cpp20, cpp23
};

// 3.3.2
// STANDARD_COUNT
//   constant: how many language levels the generator knows.
const int STANDARD_COUNT = 10;

// host_data_model
//   the data model of the machine bitwisedom was built for: a new sheet's.
D_NODISCARD data_model host_data_model();

// 3.3.3
// macro_template
//   struct: a macro written beside every symbol of the kinds it applies to.
// name, params and body take placeholders: {NAME} the symbol's macro name,
// {LOCAL} its local name, {VALUE} its value as a literal, {MASK} its mask,
// {SHIFT} its lowest bit, {WIDTH} its width, {BIT} its bit, {PREFIX} and
// {SUFFIX} its affixes. An empty params writes an object-like macro.
//   A template is for one kind of sheet: a `wide` one is written for a sheet
// wider than an integer, the rest for the others. A wide sheet's take more
// placeholders: {FIELD} a value's range's macro name; {WORD} the word a
// range or a bit begins in, {WORD_SHIFT} where in it, {WORD_MASK} its bits
// there, {WORD_BITS} a word's width; {BITSET} the prefix of the sheet's
// array macros; and {GET} and {PUT}, the expressions that read a range of
// an array and write one, across words.
struct macro_template
{
    bool        enabled = true;
    int         applies = 0;   // a bit per def_kind
    std::string name;
    std::string params;
    std::string body;
    std::string note;
    bool        wide    = false;
};

// 3.3.4
// gen_settings
//   struct: everything the C/C++ generator is told.
struct gen_settings
{
    language    lang       = language::c;
    gen_form    form       = gen_form::macros;
    gen_style   style      = gen_style::djinterp;
    standard    level      = standard::c99;
    radix       base       = radix::hex;
    int_suffix  suffix     = int_suffix::u;
    bool        upper_hex  = true;
    bool        upper_suffix = false;
    bool        pad        = true;
    bool        banner     = true;
    bool        guard      = true;
    bool        pragma_once = false;
    bool        comments   = true;
    bool        sections   = true;
    bool        align      = true;
    std::string file_name  = "bitfields.h";
    std::string guard_name;        // empty: derived from the file name
    std::string name_space;        // C++ constants: the namespace
    std::string author     = "bitwisedom";
    data_model  model      = host_data_model();
    int         word       = 32;   // a wide sheet's array words, in bits
    std::vector<macro_template> templates;
};


//==============================================================================
// 4.  THE SHEET
//==============================================================================


// 4.1    sheet
//------------------------------------------------------------------------------
// 4.1.1
// sheet
//   struct: the document.
struct sheet
{
    std::string     title  = "untitled";
    int             width  = 32;
    numbering       bits   = numbering::lsb0;
    bool            msb_left = true;
    byte_order      bytes  = byte_order::little;
    std::string     prefix;
    std::string     suffix;
    std::string     trim_both;   // comma-separated tokens at either end
    std::string     trim_left;   // tokens at the start of each local name
    std::string     trim_right;  // tokens at the end of each local name
    row             root;
    wide            probe;
    std::vector<column_segment> segments;
    grid_settings   grid;
    colour_settings colours;
    gen_settings    gen;
    std::string     environment;   // #defines its macros are evaluated in,
                                   // which the header does not write
    int             next_id = 1;
};

// 4.2    Tree operations
//------------------------------------------------------------------------------
// construction -- a sheet with its root group, default lines and templates
D_NODISCARD sheet  make_sheet(const std::string& _title,
                              int                _width);
// New blank document, with an editable full-width root range.
D_NODISCARD sheet  make_document(const std::string& _title,
                                 int                _width);
void               reset_view(sheet& _sheet);
void               automatic_colours(sheet& _sheet);
D_NODISCARD look   make_look(const sheet& _sheet,
                             int          _theme,
                             int          _scheme);
D_NODISCARD std::vector<macro_template>  default_templates();
D_NODISCARD std::vector<macro_template>  default_wide_templates();

// word_bits
//   a wide sheet's array words, in bits: 8, 16, 32 or 64.
D_NODISCARD int    word_bits(const sheet& _sheet);
D_NODISCARD int    applies_bit(def_kind _kind);

// lookup -- by id; nullptr when no row has it (the root has id 0)
D_NODISCARD row*        find_row(sheet& _sheet,
                                 int    _id);
D_NODISCARD const row*  find_row(const sheet& _sheet,
                                 int          _id);
D_NODISCARD row*        parent_of(sheet& _sheet,
                                  int    _id);

// row_index
//   class: every row of a sheet by its id, found at once. find_row walks
// the tree, which is quick for one row and slow for each of a thousand; an
// index walks it once. It is as good as the rows it points at: anything
// that invalidates pointers into the sheet's rows invalidates it.
class row_index
{
public:
    explicit row_index(const sheet& _sheet);

    // the row with an id; nullptr when no row has it
    D_NODISCARD const row*  find(int _id) const;

private:
    std::vector<const row*> m_rows;  // by id
};

// walking -- depth first, parents before children; _depth starts at 0
void  walk(const sheet&                                          _sheet,
           const std::function<void(const row&, const row&, int)>& _visit);

/**
 * @brief Inserts a row into a group.
 *
 * @param[in,out] _sheet   the sheet; its next id is consumed.
 * @param[in]     _parent  the group's id; a symbol's id inserts beside it.
 * @param[in]     _index   the position among the group's children; -1 or
 *                         past the end appends.
 * @param[in]     _row     the row; its id, and its children's, are replaced.
 * @post   Pointers and references into _sheet's rows are invalid.
 * @return the new row's id, or 0 if _parent names no row.
 */
int   insert_row(sheet&      _sheet,
                 int         _parent,
                 int         _index,
                 const row&  _row);

/**
 * @brief Removes a row, and everything in it, from the sheet.
 *
 * @param[in,out] _sheet  the sheet.
 * @param[in]     _id     the row; the root cannot be removed.
 * @post   Pointers and references into _sheet's rows are invalid.
 * @return `true` if a row was removed.
 */
bool  remove_row(sheet& _sheet,
                 int    _id);

/**
 * @brief Moves a row to a new place in the tree.
 *
 * @param[in,out] _sheet   the sheet.
 * @param[in]     _id      the row to move.
 * @param[in]     _parent  the group to move it into.
 * @param[in]     _index   its position there; -1 appends.
 * @post   Pointers and references into _sheet's rows are invalid.
 * @return `true` if it moved: not into itself, nor into its own subtree.
 */
bool  move_row(sheet& _sheet,
               int    _id,
               int    _parent,
               int    _index);

/**
 * @brief Opens or closes a group and every group within it, as the tree
 *        and the grid show them.
 *
 * @param[in,out] _sheet  the sheet.
 * @param[in]     _id     the group; the root's id is every group the sheet
 *                        has. A symbol's id, or an id no row has, changes
 *                        nothing.
 * @param[in]     _open   true to open them, false to close them.
 * @return how many groups were changed: 0 when each already was so.
 */
int   open_groups(sheet& _sheet,
                  int    _id,
                  bool   _open);

// allocation -- the lowest bit a group's bit and range rows leave free
D_NODISCARD int  next_free_bit(const sheet& _sheet,
                               int          _group);

// macros -- whether a macro row takes arguments: it has parameters, or is
// marked callable
D_NODISCARD bool function_like(const row& _row);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_SHEET_HPP
