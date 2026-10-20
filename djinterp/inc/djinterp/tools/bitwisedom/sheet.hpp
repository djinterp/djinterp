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
*                                                            revised: 2026.10.19
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
    3.  What the grid shows
    4.  Special rows
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
// value_form
//   enum: how the symbols column writes a row's value when it lists rows
// by value: as the header writes it, in the row's own radix; or every row
// alike -- in hex, decimal or binary, as the run from its lowest set bit
// shifted there, or as the bits it sets.
enum class value_form : int
{
    written = 0,
    hex,
    dec,
    bin,
    shift,
    bits
};

// 1.1.12
// VALUE_FORM_COUNT
//   constant: how many value forms there are.
const int VALUE_FORM_COUNT = 6;

// 1.1.13
// colour_mode
//   enum: where a set bit's colour comes from.
enum class colour_mode : int
{
    row = 0,
    group,
    kind,
    accent
};

// 1.1.14
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

// 1.1.15
// DATA_MODEL_COUNT
//   constant: how many data models there are.
const int DATA_MODEL_COUNT = 3;

// 1.1.16
// sort_key
//   enum: the order the grid and the tree show each group's rows in: the
// sheet's own, or sorted -- by symbol name, the macro the header defines;
// by name, the row's own; by short name; by value; by lowest bit; by width
// in bits; by kind; or by note. Groups come first among their siblings,
// sorted by name where the key is a name, in the sheet's order where it is
// not. The sheet's order is what its header follows, sorted or not.
enum class sort_key : int
{
    sheet = 0,
    symbol,
    name,
    short_name,
    value,
    lowest,
    width,
    kind,
    note
};

// 1.1.17
// SORT_KEY_COUNT
//   constant: how many sort keys there are.
const int SORT_KEY_COUNT = 9;

// 1.1.18
// special_kind
//   enum: a row of the grid's own, not the sheet's: the probe, a scratch
// value; or the usage -- how many symbols allocate each bit.
enum class special_kind : int
{
    probe = 0,
    usage
};

// 1.1.19
// SPECIAL_KIND_COUNT
//   constant: how many kinds of special row there are.
const int SPECIAL_KIND_COUNT = 2;

// 1.1.20
// special_place
//   enum: where a special row is shown: pinned below every row -- at the
// grid's foot while the rows leave room -- or above every row; or among the
// rows, in the tree, where a usage row counts only its group's rows.
enum class special_place : int
{
    bottom = 0,
    top,
    tree
};

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
// display_rule
//   struct: how a region of the grid shows its cells, against the sheet's
// own settings: its digits, a range's name across its cells, and its
// colour. The region is rows -- those named, by id, and every row within
// them; none for every row -- by bits, from lsb to msb; an msb below the
// lsb for every bit. Each of the three is -1 as the sheet says, 0 not
// shown -- the colour, its bits drawn plain -- or 1 shown. Rules apply in
// order; a later one over a cell wins. A rule that names rows no longer in
// the sheet covers nothing -- never every row.
struct display_rule
{
    std::vector<int> rows;
    int              lsb    = 0;
    int              msb    = -1;
    int              digits = -1;
    int              names  = -1;
    int              colour = -1;
};

// 3.1.4
// special_row
//   struct: one of the grid's special rows, and where it is shown. Among
// the rows, it comes just before the row `before`; with none -- or that row
// gone -- at the end of the rows of `parent`, a group or range, or of the
// sheet's, 0. Rows of a kind are as many as the sheet places, every one
// showing the same thing: the sheet's one probe, or its usage -- a usage
// row in a group counting the group's rows, a subtotal. The grid's
// show_probe and show_usage show or hide every row of their kind at once.
// `id` is the row's own among the specials, from 1.
struct special_row
{
    int           id     = 0;
    special_kind  kind   = special_kind::probe;
    special_place place  = special_place::bottom;
    int           parent = 0;
    int           before = 0;
};

// 3.1.5
// grid_settings
//   struct: how the grid is drawn: its lines, cells, columns and labels,
// the order its rows are shown in, the regions that show their cells
// otherwise, and where its special rows are.
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
    bool            show_probe    = false;  // every probe row
    bool            show_usage    = false;  // every usage row
    bool            lines_past_rows = false;  // lines down the room below
    bool            show_labels   = true;   // a range's name across its cells
    bool            show_colours  = true;   // its rows' colours in its cells
    bool            show_names    = true;
    bool            show_values   = false;  // opt in to the value column
    bool            show_bits     = true;
    bool            values_first  = false;  // swap the two text columns
    bool            show_cell_boxes = false;  // draw each cell as its own box
    bool            square_cells  = true;   // rows follow the cells' width
    float           cell_ratio    = 0.8f;   // ... as this share of it
    bool            full_names    = false;  // macro names, not local ones
    bool            names_by_value = false;  // the symbols column: values
    value_form      names_value_form = value_form::written;  // ... as
    bool            abbreviate_unused = false;
    int             unused_run    = 4;
    collapsed_style collapsed     = collapsed_style::hex;
    grid_fill       fill          = grid_fill::cells;
    sort_key        sort_by       = sort_key::sheet;
    bool            sort_descending = false;
    std::vector<display_rule> rules;
    std::vector<special_row>  specials;   // settle_specials keeps one of
                                          // each kind at least
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
// definition_source
//   An imported preprocessor file. Text is embedded in the JSON document,
//   so the sheet remains reproducible if the original file moves.
struct definition_source
{
    std::string path;
    std::string text;
    bool        show_other = false;
};

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
    std::vector<definition_source> definitions; // imported preprocessor files
    std::string     environment;   // #defines its macros are evaluated in,
                                   // which the header does not write
    int             next_id = 1;
};

// 4.2    Tree operations
//------------------------------------------------------------------------------
// construction -- a sheet with its root group, default lines and templates;
// and File > New's document: that sheet holding an editable full-width
// range named root, with no fill, and in it one bit, FLAG, at bit 0
D_NODISCARD sheet  make_sheet(const std::string& _title,
                              int                _width);
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


// 4.3    What the grid shows
//------------------------------------------------------------------------------
// 4.3.1
// shown_part
//   enum: a part of what a cell shows, which a display rule sets: its
// digit; the name of a range, or a value's number, written across it; and
// its row's colour.
enum class shown_part : int
{
    digits = 0,
    names,
    colour
};

// 4.3.2
// SHOWN_PART_COUNT
//   constant: how many parts a display rule sets.
const int SHOWN_PART_COUNT = 3;

// 4.3.3
// cell_shows
//   struct: what one cell of the grid shows.
struct cell_shows
{
    bool digits = false;
    bool names  = true;
    bool colour = true;
};

// parts -- a rule's setting of one part, and the grid's own: -1 as the
// sheet says, 0 hidden, 1 shown; and what a cell_shows says of it
D_NODISCARD int   rule_part(const display_rule& _rule,
                            shown_part          _part);
int&              rule_part(display_rule& _rule,
                            shown_part    _part);
D_NODISCARD bool  grid_shows(const grid_settings& _grid,
                             shown_part           _part);
bool&             grid_shows(grid_settings& _grid,
                             shown_part     _part);
D_NODISCARD bool  shows(const cell_shows& _cell,
                        shown_part        _part);

// lineage
//   a row's id, then those of the groups and ranges it is in, innermost
// first -- the root's excluded: what a rule over rows is matched against,
// so one over a group covers every row in it.
D_NODISCARD std::vector<int>  lineage(const sheet& _sheet,
                                      int          _id);

/**
 * @brief The rules over any of a row's cells, in order: for shown_at, which
 *        a row's cells are each asked of.
 *
 * @param[in] _grid     the grid's settings.
 * @param[in] _lineage  the row's lineage.
 * @return the rules, pointing into _grid.
 */
D_NODISCARD std::vector<const display_rule*>  rules_over(
    const grid_settings&    _grid,
    const std::vector<int>& _lineage);

/**
 * @brief What a cell of a row shows -- one bit's, or a column's of several:
 *        the grid's own settings, as the rules over the row change them
 *        where they cover any of the cell's bits; the last to set a part
 *        wins.
 *
 * @param[in] _grid  the grid's settings.
 * @param[in] _over  the rules over the row, from rules_over.
 * @param[in] _lsb   the cell's lowest bit.
 * @param[in] _msb   its highest.
 * @return what it shows.
 */
D_NODISCARD cell_shows  shown_at(const grid_settings&                    _grid,
                                 const std::vector<const display_rule*>& _over,
                                 int                                     _lsb,
                                 int                                     _msb);

/**
 * @brief Sets whether a region of the grid shows a part: the last rule, if
 *        it is over just that region, takes it, else a new rule does --
 *        so it wins over every rule before it -- and the part is taken from
 *        any earlier rule over just that region. Rules left setting nothing
 *        are dropped.
 *
 * @param[in,out] _grid   the grid's settings.
 * @param[in]     _rows   the region's rows, by id; none for every row.
 * @param[in]     _lsb    its lowest bit.
 * @param[in]     _msb    its highest; below _lsb for every bit.
 * @param[in]     _part   what is set.
 * @param[in]     _value  -1 as the rest of the grid shows it -- no rule over
 *                        just the region sets it; 0 hidden; 1 shown.
 * @param[in]     _only   shown there and nowhere else: the grid's own
 *                        setting and every other rule's taken back first,
 *                        and _value taken as 1.
 */
void  set_shown(grid_settings&          _grid,
                const std::vector<int>& _rows,
                int                     _lsb,
                int                     _msb,
                shown_part              _part,
                int                     _value,
                bool                    _only);

/**
 * @brief Drops from the grid's rules the rows a sheet no longer has, and a
 *        rule left with none -- which would otherwise cover every row.
 *
 * @param[in,out] _sheet  the sheet.
 * @return how many rules were changed or dropped.
 */
int   prune_rules(sheet& _sheet);

// 4.4    Special rows
//------------------------------------------------------------------------------
// 4.4.1
// special_name
//   the name a kind of special row is listed by: "probe", "usage".
D_NODISCARD const char*  special_name(special_kind _kind);
D_NODISCARD const char*  place_name(special_place _place);

/**
 * @brief Keeps the grid's special rows whole: one of each kind at least --
 *        a kind with none gets one, pinned at the bottom, probe first --
 *        every one with an id of its own, and every one among the rows
 *        anchored only to rows the sheet has: a row gone is let go of, its
 *        parent then, and the sheet's own rows last.
 *
 * @param[in,out] _sheet  the sheet.
 * @return whether anything was changed.
 */
bool  settle_specials(sheet& _sheet);

// lookup -- by id; nullptr when no special row has it
D_NODISCARD special_row*        find_special(grid_settings& _grid,
                                             int            _id);
D_NODISCARD const special_row*  find_special(const grid_settings& _grid,
                                             int                  _id);

/**
 * @brief Adds a special row of a kind's, just after another, or at the
 *        bottom: a copy of where the other is.
 *
 * @param[in,out] _grid  the grid's settings.
 * @param[in]     _kind  its kind.
 * @param[in]     _like  the special row it is placed as, and after; 0 for
 *                       pinned at the bottom, last.
 * @return its id.
 */
int   add_special(grid_settings& _grid,
                  special_kind   _kind,
                  int            _like);

/**
 * @brief Removes a special row. The last of its kind stays, hidden with its
 *        kind -- it keeps where it was for when its kind is shown again.
 *
 * @param[in,out] _grid  the grid's settings.
 * @param[in]     _id    the special row.
 * @return whether it went, or its kind was hidden.
 */
bool  remove_special(grid_settings& _grid,
                     int            _id);

/**
 * @brief Moves a special row: pinned at the top or the bottom, or among the
 *        rows; and in the specials' order, just before another -- which
 *        orders rows pinned alike, or anchored alike -- or last.
 *
 * @param[in,out] _grid    the grid's settings.
 * @param[in]     _id      the special row.
 * @param[in]     _place   where it goes.
 * @param[in]     _parent  among the rows: the group or range it is in.
 * @param[in]     _before  among the rows: the row it comes before; 0, its
 *                         parent's end.
 * @param[in]     _ahead   the special row it is put before in their order;
 *                         0 for last.
 * @return whether it moved.
 */
bool  place_special(grid_settings& _grid,
                    int            _id,
                    special_place  _place,
                    int            _parent,
                    int            _before,
                    int            _ahead);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_SHEET_HPP
