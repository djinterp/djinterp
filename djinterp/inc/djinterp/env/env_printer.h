/*******************************************************************************
* djinterp [env]                                                   env_printer.h
*
* djinterp environment printer.
*   The build environment env.h detected -- language, compiler, operating
* system, architecture, POSIX, preprocessor limits, build type, C runtime
* features and, in C++, the C++ feature-test results -- as a report, printed
* onto any output stream in any form a program wants, showing what it wants.
* Compiled by BOTH C and C++; env_printer.hpp is its C++ face.
*
*   THE REPORT IS THE CALLER'S UNIT'S. What env.h detects depends on how a
* unit is compiled: a C unit and a C++ unit of one program see different
* languages, and two C++ units may see different standards. So the report is
* collected where it is asked for, by d_env_report_here, which every unit
* compiles for itself (it has internal linkage), into constant tables of its
* own -- nothing is computed at run time. Printing it is the same for every
* report, and is env_printer.c's, compiled once.
*
*   THE PRINTER IS AN ABSTRACT MACHINE. Its program is a set of templates
* (c/util/ostream_template.h), one for each part of a report: its beginning
* and end, a section's, an entry, a feature, a note, and the separators. Its
* environment is a frame of variables (c/option/option_env.h): the style the
* templates default to, which sections and items are shown, column widths,
* the title, the words for true and false, and the templates themselves. Its
* input is a report, its output a stream. Every variable has a default pinned
* in the printer's schema, from config/env/cfg_env_printer.h; a program
* changes what it wants, at run time:
*     d_env_printer_put(&printer, "style=json");
*     d_env_printer_load(&printer, "select=-cpp_features.*; key_width=30");
*     d_env_printer_import(&printer, D_ENV_PRINTER_ENV_PREFIX);   // getenv
*     d_env_printer_put(&printer, "entry=\"{name}: {value}\\n\"");
* and a binding may be local to one section: "cpp_features.show_title=0".
*
*   SHOW ANYTHING, HIDE ANYTHING. `sections` turns whole sections on and off.
* `select` is a list of rules over "section.name" paths -- "-cpp_features.*",
* "+compiler", "-*.has_*" -- of which the last that matches an item decides;
* a list that starts with an exclusion starts from everything, one that
* starts with an inclusion from nothing. `features` keeps the supported or
* unsupported C++ features only; the show_* switches and the templates' own
* {?field} blocks hide parts of an item; a C filter decides anything else.
*
*   FORMAT IT ANY WAY. text, json, markdown, csv and keyvalue are presets: each
* is nine templates in the language every template is written in, and any of
* them can be replaced -- one, or all -- by setting its variable. A template
* names the item's fields ({label}, {value!json}, {width}) and the printer's
* variables ({title}, {true_text}, a program's own), and is compiled, with
* every name checked, once, then run for each part.
*
*   NOTHING IS LOST SILENTLY. Printing writes onto a struct dj_ostream, whose
* status is sticky, so a full disk or a closed pipe is reported, not dropped.
*
*   ONE DJIN, ANY NUMBER OF PRINTERS. The variables and the sections' scopes
* are declared once more in a djin, the frame "env_printer"
* (d_env_printer_djin). Registered in the program's registry
* (c/option/option_env_registry.h), its variables are the program's
* settings by full name -- "env_printer.style=json" in a file,
* DJINTERP_ENV_PRINTER_STYLE in the environment, --env_printer.style=json on
* the command line -- each bound from its source, so the order they arrive
* in cannot change the outcome. A printer made in it (d_env_printer_init_in)
* reads them through its own frame, where it binds what is its alone; one
* made by d_env_printer_init stands alone.
*
*   Linking needs env_printer.c; the stream's units it uses, c/util/ostream's
* ostream.c and ostream_file.c (with c/filesys's file_open.c and
* file_common.c, for d_env_print_file) and c/text's ostream_template.c and
* ostream_format.c; and the option core's (option_env.c,
* option_env_registry.c, option_set_common.c, option_common.c).
*   The printer stands on the option core, and so on its 64-bit floor: below
* it, ISO strict C++98 where int64_t is `long long`, the report is collected
* as everywhere and the printer is absent.
*
*
* path:      /inc/djinterp/env/env_printer.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE REPORT
    ----------
    1.  Values
         1.  dj_env_value_kind
         2.  dj_env_entry
         3.  dj_env_feature
    2.  Sections
         1.  dj_env_section_id
         2.  D_ENV_SECTIONS_CORE
         3.  D_ENV_SECTIONS_CPP
         4.  D_ENV_SECTIONS_ALL
         5.  dj_env_section
         6.  dj_env_report
2.  THE PRINTER
    -----------
    1.  Its variables
         1.  dj_env_style
         2.  dj_env_features
         3.  dj_env_template
         4.  dj_env_printer_var
         5.  D_ENV_PRINTER_ENV_PREFIX
    2.  Its fields
         1.  dj_env_field
         2.  dj_env_item_kind
         3.  dj_env_item
         4.  dj_env_filter_fn
         5.  dj_env_field_fn
    3.  The machine
         1.  dj_env_printer_storage
         2.  dj_env_printer_error
         3.  dj_env_printer
    4.  Setting up
    5.  The djin
    6.  Changing variables
    7.  Printing
3.  THE CALLER'S ENVIRONMENT
    ------------------------
    1.  Table entries
         1.  D_INTERNAL_ENV_PRINTER_TEXT
         2.  D_INTERNAL_ENV_PRINTER_INT
         3.  D_INTERNAL_ENV_PRINTER_BOOL
         4.  D_INTERNAL_ENV_PRINTER_FEATURE
    2.  Collection
    3.  Printing it
*/

#ifndef DJINTERP_ENV_ENV_PRINTER_H
#define DJINTERP_ENV_ENV_PRINTER_H 1

// std
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // FILE
// djinterp
#include "../c/djinterp.h"                    // framework root
#include "./env.h"                            // D_ENV_*
#include "../c/util/ostream/ostream.h"        // struct dj_ostream
#include "../c/util/ostream/ostream_file.h"   // d_ostream_init_file
#include "../c/text/ostream_template.h"       // the program's templates
#include "../c/option/option_env.h"           // the machine's variables
#include "../c/option/option_env_registry.h"  // the djin's registry
#include "../config/env/cfg_env_printer.h"    // D_CFG_ENV_PRINTER_*
#if D_ENV_LANG_USING_CPP
    #include "./cpp/env_cpp_features.h"       // D_ENV_CPP_FEATURE_*
#endif
// re_std
#include "../../re_std/c/cstdint/dstdint.h"  // int32_t, uint32_t;
                                             // RE_STD_HAS_INT64

#if !defined(D_STATIC_INLINE)
    #error "env_printer.h needs D_STATIC_INLINE; set D_CFG_DEFINE_INLINE to 1"
#endif  // D_STATIC_INLINE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  THE REPORT
//==============================================================================


// 1.1    Values
//------------------------------------------------------------------------------
// 1.1.1
// dj_env_value_kind
//   enum: how an entry's value reads: text, a number, or true and false.
enum dj_env_value_kind
{
    D_ENV_VALUE_TEXT    = 0,
    D_ENV_VALUE_INTEGER = 1,
    D_ENV_VALUE_BOOLEAN = 2
};

// 1.1.2
// dj_env_entry
//   struct: one detected property: its name, for paths and machines; its
// label, for people; and its value as `text` (TEXT) or `integer` (INTEGER,
// and BOOLEAN as 0 or 1).
struct dj_env_entry
{
    const char* name;      // "cpp_standard"; unique in its section
    const char* label;     // "C++ Standard"
    const char* text;      // TEXT: the value; null otherwise
    long        integer;   // INTEGER and BOOLEAN: the value
    int32_t     kind;      // enum dj_env_value_kind
    int32_t     reserved;  // pad; 0
};

// 1.1.3
// dj_env_feature
//   struct: one C++ feature-test result.
struct dj_env_feature
{
    const char* name;         // the feature-test macro, "__cpp_lambdas"
    const char* description;  // what it is, "Lambda expressions"
    const char* standard;     // the standard it came in, "(C++11)"
    long        value;        // the macro's value; 0 when it is absent
};

// 1.2    Sections
//------------------------------------------------------------------------------
// 1.2.1
// dj_env_section_id
//   enum: the report's sections, as bits, so the `sections` variable can
// select any set; each is also the scope its section's local variables are
// bound in, by its slug.
enum dj_env_section_id
{
    D_ENV_SECTION_LANGUAGE     = 0x0001,
    D_ENV_SECTION_COMPILER     = 0x0002,
    D_ENV_SECTION_OS           = 0x0004,
    D_ENV_SECTION_ARCH         = 0x0008,
    D_ENV_SECTION_POSIX        = 0x0010,
    D_ENV_SECTION_PP_LIMITS    = 0x0020,
    D_ENV_SECTION_BUILD        = 0x0040,
    D_ENV_SECTION_C_RUNTIME    = 0x0080,  // the D_ENV_C_HAS_* flags
    D_ENV_SECTION_CPP_SUMMARY  = 0x0100,  // C++ only: per-standard rollups
    D_ENV_SECTION_CPP_FEATURES = 0x0200   // C++ only: every feature-test macro
};

// 1.2.2
// D_ENV_SECTIONS_CORE
//   constant: the seven sections every unit reports, language to build.
#define D_ENV_SECTIONS_CORE  0x007F

// 1.2.3
// D_ENV_SECTIONS_CPP
//   constant: the two C++ sections.
#define D_ENV_SECTIONS_CPP   0x0300

// 1.2.4
// D_ENV_SECTIONS_ALL
//   constant: every section.
#define D_ENV_SECTIONS_ALL   0x03FF

// 1.2.5
// dj_env_section
//   struct: one section of a report: its entries or its features, or -- when
// it has neither -- a note saying why.
struct dj_env_section
{
    const char*                  slug;           // "language": paths, scopes
    const char*                  title;          // "Language"
    const char*                  note;           // or null
    const struct dj_env_entry*   entries;        // or null
    size_t                       entry_count;
    const struct dj_env_feature* features;       // or null
    size_t                       feature_count;
    int32_t                      id;             // enum dj_env_section_id
    int32_t                      reserved;       // pad; 0
};

// 1.2.6
// dj_env_report
//   struct: a whole report: the sections a unit can report, in order.
struct dj_env_report
{
    const struct dj_env_section* sections;
    size_t                       count;
};


#if RE_STD_HAS_INT64

//==============================================================================
// 2.  THE PRINTER
//==============================================================================


// 2.1    Its variables
//------------------------------------------------------------------------------
// 2.1.1
// dj_env_style
//   enum: the presets: the values of the `style` variable, by name.
enum dj_env_style
{
    D_ENV_STYLE_TEXT     = 0,  // "text": aligned lines, for people
    D_ENV_STYLE_JSON     = 1,  // "json": one object, a member per section
    D_ENV_STYLE_MARKDOWN = 2,  // "markdown": a table per section
    D_ENV_STYLE_CSV      = 3,  // "csv": a row per item, with a header
    D_ENV_STYLE_KEYVALUE = 4,  // "keyvalue": section.name=value lines
    D_ENV_STYLE_COUNT    = 5
};

// 2.1.2
// dj_env_features
//   enum: the values of the `features` variable: which C++ features show.
enum dj_env_features
{
    D_ENV_FEATURES_ALL         = 0,  // "all"
    D_ENV_FEATURES_SUPPORTED   = 1,  // "supported": a value that is not 0
    D_ENV_FEATURES_UNSUPPORTED = 2   // "unsupported"
};

// 2.1.3
// dj_env_template
//   enum: the parts of a report a template prints, in the order of their
// variables. A part's variable unset, or null, uses the style's template.
enum dj_env_template
{
    D_ENV_TEMPLATE_REPORT_BEGIN      = 0,  // once, first
    D_ENV_TEMPLATE_REPORT_END        = 1,  // once, last
    D_ENV_TEMPLATE_SECTION_BEGIN     = 2,  // before a shown section's items
    D_ENV_TEMPLATE_SECTION_END       = 3,  // after them
    D_ENV_TEMPLATE_SECTION_SEPARATOR = 4,  // between two shown sections
    D_ENV_TEMPLATE_ENTRY             = 5,  // an entry
    D_ENV_TEMPLATE_FEATURE           = 6,  // a C++ feature
    D_ENV_TEMPLATE_NOTE              = 7,  // a section's note
    D_ENV_TEMPLATE_ITEM_SEPARATOR    = 8,  // between two shown items
    D_ENV_TEMPLATE_COUNT             = 9
};

// 2.1.4
// dj_env_printer_var
//   enum: the printer's variables, by index, in its schema's order. Their
// names are the enumerators' lower-cased tails: D_ENV_PRINTER_KEY_WIDTH is
// "key_width". A program's own variables follow D_ENV_PRINTER_VARS.
enum dj_env_printer_var
{
    D_ENV_PRINTER_STYLE             = 0,   // choice: text, json, markdown,
                                           // csv, keyvalue
    D_ENV_PRINTER_SECTIONS          = 1,   // flags: the section slugs, core,
                                           // cpp, all
    D_ENV_PRINTER_SELECT            = 2,   // text: path rules
    D_ENV_PRINTER_FEATURES          = 3,   // choice: all, supported,
                                           // unsupported
    D_ENV_PRINTER_TITLE             = 4,   // text: the report's title
    D_ENV_PRINTER_SHOW_TITLE        = 5,   // bool
    D_ENV_PRINTER_SHOW_STANDARDS    = 6,   // bool: a feature's standard
    D_ENV_PRINTER_SHOW_DESCRIPTIONS = 7,   // bool: a feature's description
    D_ENV_PRINTER_KEY_WIDTH         = 8,   // int: an entry's label column;
                                           // 0 fits the section's longest
    D_ENV_PRINTER_NAME_WIDTH        = 9,   // int: a feature's name column;
                                           // 0 fits the longest
    D_ENV_PRINTER_PREFIX            = 10,  // text: at the start of each line
    D_ENV_PRINTER_TRUE_TEXT         = 11,  // text: how text styles say true
    D_ENV_PRINTER_FALSE_TEXT        = 12,  // text: and false
    D_ENV_PRINTER_REPORT_BEGIN      = 13,  // text: the templates, in
    D_ENV_PRINTER_REPORT_END        = 14,  // enum dj_env_template's order
    D_ENV_PRINTER_SECTION_BEGIN     = 15,
    D_ENV_PRINTER_SECTION_END       = 16,
    D_ENV_PRINTER_SECTION_SEPARATOR = 17,
    D_ENV_PRINTER_ENTRY             = 18,
    D_ENV_PRINTER_FEATURE           = 19,
    D_ENV_PRINTER_NOTE              = 20,
    D_ENV_PRINTER_ITEM_SEPARATOR    = 21,
    D_ENV_PRINTER_VARS              = 22
};

// 2.1.5
// D_ENV_PRINTER_ENV_PREFIX
//   constant: the prefix d_env_printer_import is usually given: the printer's
// variables then read from DJINTERP_ENV_PRINTER_STYLE and its kin.
#define D_ENV_PRINTER_ENV_PREFIX  "DJINTERP_ENV_PRINTER_"

// 2.2    Its fields
//------------------------------------------------------------------------------
// 2.2.1
// dj_env_field
//   enum: the fields a template can name for the part it prints, by index;
// a printer variable is named by its own name and is field D_ENV_FIELDS plus
// its index. A field a part does not have is NONE: empty, or null in JSON.
enum dj_env_field
{
    D_ENV_FIELD_SECTION       = 0,   // "section": the slug
    D_ENV_FIELD_SECTION_TITLE = 1,   // "section_title"
    D_ENV_FIELD_SECTION_INDEX = 2,   // "section_index": among those shown,
                                     // from 0
    D_ENV_FIELD_SECTION_COUNT = 3,   // "section_count": sections shown
    D_ENV_FIELD_COUNT         = 4,   // "count": items the section shows
    D_ENV_FIELD_HAS_FEATURES  = 5,   // "has_features": it lists features
    D_ENV_FIELD_PATH          = 6,   // "path": "section.name"
    D_ENV_FIELD_NAME          = 7,   // "name"
    D_ENV_FIELD_LABEL         = 8,   // "label": a feature's is its name
    D_ENV_FIELD_VALUE         = 9,   // "value": text, integer or boolean
    D_ENV_FIELD_DISPLAY       = 10,  // "display": the value, with booleans
                                     // as true_text and false_text
    D_ENV_FIELD_KIND          = 11,  // "kind": text, integer, boolean,
                                     // feature or note
    D_ENV_FIELD_INDEX         = 12,  // "index": among the section's shown
                                     // items, from 0
    D_ENV_FIELD_FIRST         = 13,  // "first": the first of them
    D_ENV_FIELD_LAST          = 14,  // "last": the last of them
    D_ENV_FIELD_STANDARD      = 15,  // "standard": a feature's, "C++11"
    D_ENV_FIELD_DESCRIPTION   = 16,  // "description": a feature's
    D_ENV_FIELD_SUPPORTED     = 17,  // "supported": a feature's value is not 0
    D_ENV_FIELD_NOTE          = 18,  // "note": a note's text
    D_ENV_FIELD_WIDTH         = 19,  // "width": the column the section's
                                     // labels (entries) or names (features)
                                     // align to
    D_ENV_FIELDS              = 20
};

// 2.2.2
// dj_env_item_kind
//   enum: what a template is printing.
enum dj_env_item_kind
{
    D_ENV_ITEM_REPORT  = 0,  // the report's beginning, end
    D_ENV_ITEM_SECTION = 1,  // a section's beginning, end, a separator
    D_ENV_ITEM_ENTRY   = 2,
    D_ENV_ITEM_FEATURE = 3,
    D_ENV_ITEM_NOTE    = 4
};

// 2.2.3
// dj_env_item
//   struct: the part being printed, as a filter or a field hook sees it.
struct dj_env_item
{
    const struct dj_env_report*  report;
    const struct dj_env_section* section;        // or null for the report
    const struct dj_env_entry*   entry;          // ENTRY; else null
    const struct dj_env_feature* feature;        // FEATURE; else null
    size_t                       index;          // among the shown items
    size_t                       count;          // the section's shown items
    size_t                       section_index;  // among the shown sections
    size_t                       section_count;  // sections shown
    size_t                       width;          // the section's column
    uint32_t                     scope;          // the section's, in the env
    int32_t                      kind;           // enum dj_env_item_kind
};

// 2.2.4
// dj_env_filter_fn
//   function pointer: the last word on whether an entry, a feature or a note
// shows, after `sections`, `select` and `features` let it: non-zero shows it.
// Asked before the item's place is known, so index, count, section_index,
// section_count and width are 0; may be asked more than once for one item,
// and must answer the same.
typedef int32_t (*dj_env_filter_fn)(void*                     _context,
                                    const struct dj_env_item* _item);

// 2.2.5
// dj_env_field_fn
//   function pointer: a field's value, ahead of the printer's own: returns 1
// when it wrote _value, 0 to leave the field to the printer, and a negative
// number to stop printing, which is then recorded as D_OSTREAM_INVALID. A
// program's own fields are variables it declares, D_ENV_FIELDS plus their
// index; this is how one computes them, or renames a label. A column's
// width is measured from the report's own labels and names, before any hook.
typedef int32_t (*dj_env_field_fn)(void*                     _context,
                                   const struct dj_env_item* _item,
                                   uint32_t                  _field,
                                   struct dj_ostream_value*  _value);

// 2.3    The machine
//------------------------------------------------------------------------------
// 2.3.1
// dj_env_printer_storage
//   struct: the printer's environment's storage: room for
// D_CFG_ENV_PRINTER_BINDINGS bindings, D_CFG_ENV_PRINTER_VARIABLES variables
// and D_CFG_ENV_PRINTER_TEXT bytes of text set as text.
D_OPTION_ENV_DECLARE(dj_env_printer_storage,
                     D_CFG_ENV_PRINTER_BINDINGS,
                     D_CFG_ENV_PRINTER_VARIABLES,
                     D_CFG_ENV_PRINTER_TEXT);

// 2.3.2
// dj_env_printer_error
//   struct: why the templates last failed to compile: which, where, and the
// template status; `status` is D_OSTREAM_TEMPLATE_OK when they compiled.
struct dj_env_printer_error
{
    size_t  offset;    // the byte in the template's text
    int32_t part;      // enum dj_env_template
    int32_t status;    // enum dj_ostream_template_status
};

// 2.3.3
// dj_env_printer
//   struct: a printer: its environment, its compiled program, and the
// program's hooks. A value the caller owns -- static, or on a stack with
// room for it (about 11 KB at the default sizes) -- made by
// d_env_printer_init or d_env_printer_init_in. It points into itself, so it
// is never copied: make another and configure it the same way.
//
//   The program is compiled when it is first needed and again after any
// variable changes, which the environment's generation tells; between, a
// print parses nothing.
//
//   A field past D_ENV_FIELDS names a variable: field D_ENV_FIELDS + i names
// the one whose id is vars[i] -- the printer's own, in enum
// dj_env_printer_var's order, then each d_env_printer_declare added, then
// any other a template names, as it is compiled. So a field's number stays
// what it was given out as, whichever frame declared its variable.
struct dj_env_printer
{
    struct dj_option_env          env;
    struct dj_env_printer_storage storage;
    struct dj_ostream_template    templates[D_ENV_TEMPLATE_COUNT];
    struct dj_ostream_template_op ops[D_CFG_ENV_PRINTER_TEMPLATE_OPS];
    struct dj_env_printer_error   error;
    dj_env_filter_fn              filter;
    void*                         filter_context;
    dj_env_field_fn               field;
    void*                         field_context;
    const struct dj_option_env*   var_parent;  // the frame's parent, when
                                               // var_base was found
    uint32_t                      vars[D_CFG_ENV_PRINTER_VARIABLES];
    uint32_t                      var_count;   // fields that name one
    uint32_t                      var_fixed;   // of them, the printer's and
                                               // those declared
    uint32_t                      var_base;    // the id of its first
    uint32_t                      generation;  // the env's, when compiled
    int32_t                       compiled;    // 1: the program is current
};

// 2.4    Setting up
//------------------------------------------------------------------------------
/**
 * @brief Makes a printer: its variables at their defaults, no hooks.
 *
 * @param[out] _printer  the printer.
 * @return 1, or 0 for a null printer.
 */
int32_t d_env_printer_init(struct dj_env_printer* _printer);

/**
 * @brief Makes a printer in a parent frame: it reads the parent's bindings
 *        through its own frame, where it binds what is its alone; no hooks.
 *
 * @note When a frame above declares the printer's variables -- the djin, or
 *       another printer's frame -- the printer reads those, so that frame's
 *       bindings are its defaults; otherwise its frame declares them.
 *
 * @param[out]    _printer  the printer.
 * @param[in,out] _parent   the parent: the djin (d_env_printer_djin), another
 *                          printer's frame, any frame; borrowed, so it must
 *                          outlive the printer; null makes a printer as
 *                          d_env_printer_init does.
 * @return 1, or 0 for a null printer or a chain D_OPTION_ENV_DEPTH_MAX deep.
 */
int32_t d_env_printer_init_in(struct dj_env_printer* _printer,
                              struct dj_option_env*  _parent);

// back to the defaults -- the parent's bindings, for a printer made in one:
// its frame's every binding removed, the hooks and the declarations kept
void    d_env_printer_reset(struct dj_env_printer* _printer);
// lets go of the parent, which then counts the printer a child no more: a
// printer made in one is released before it goes, and prints only its
// failure after; harmless on any other printer, and twice
void    d_env_printer_release(struct dj_env_printer* _printer);
// the printer's environment, for option_env.h's whole interface -- typed
// reads and writes, scopes, layers -- and the id one of its variables, enum
// dj_env_printer_var, has there; D_OPTION_ENV_NONE for one that is none
struct dj_option_env*
        d_env_printer_env(struct dj_env_printer* _printer);
uint32_t
        d_env_printer_var_id(const struct dj_env_printer* _printer,
                             int32_t                      _var);
// a filter and a field hook; null removes one
void    d_env_printer_set_filter(struct dj_env_printer* _printer,
                                 dj_env_filter_fn       _filter,
                                 void*                  _context);
void    d_env_printer_set_field(struct dj_env_printer* _printer,
                                dj_env_field_fn        _field,
                                void*                  _context);

/**
 * @brief Declares a variable of the program's own, which templates can then
 *        name, and statements set, like the printer's.
 *
 * @param[in,out] _printer  the printer.
 * @param[in]     _var      the declaration; borrowed, so it must outlive the
 *                          printer.
 * @return its field number, D_ENV_FIELDS plus its place among the
 *         variables templates name; KEY_DUPLICATE for a name a field or a
 *         variable of the printer's frame has, CAPACITY past
 *         D_CFG_ENV_PRINTER_VARIABLES of them; as d_option_env_declare
 *         otherwise.
 */
struct dj_option_result d_env_printer_declare(
                            struct dj_env_printer*          _printer,
                            const struct dj_option_env_var* _var);

// 2.5    The djin
//------------------------------------------------------------------------------
//   The frame "env_printer": the printer's variables and the sections' scopes,
// declared once for the whole program, and the bindings every printer made in
// it reads. In a registry its variables are settings by full name --
// "env_printer.style", "env_printer.cpp_features.key_width" -- that a file,
// the environment and the command line bind, each from its source. It is
// written by one thread at a time, as the registry is: configure it before
// the program starts its threads; then any number of printers made in it may
// print at once, each printer in one thread at a time.

/**
 * @brief The printer's djin: static, and ready before the program runs.
 *
 * @return its frame; never null.
 */
struct dj_option_env*
        d_env_printer_djin(void);

/**
 * @brief Registers the djin in a registry, so the registry reaches its
 *        settings by full name.
 *
 * @note Registering makes the djin the registry root's child, which a frame
 *       with children cannot become: register it before making a printer in
 *       it.
 *
 * @param[in,out] _registry  the registry; null for the program's.
 * @return as d_option_env_registry_add.
 */
struct dj_option_result d_env_printer_register(
                            struct dj_option_env_registry* _registry);

// 2.6    Changing variables
//------------------------------------------------------------------------------
//   Each is option_env.h's own, on the printer's frame, then the program's
// compilation: a change that leaves a template that does not compile is made,
// and reported as INVALID_ARGUMENT, with d_env_printer_last_error saying
// where, and printing refuses until a later change mends it. Each binds from
// the program but d_env_printer_import, which binds from the environment; a
// binding yields only to one of higher standing (option_env.h, 2.2), so what
// the environment says holds over what the program set, whichever was first.

/**
 * @brief Sets one variable from text: "key_width", "30".
 *
 * @param[in,out] _printer  the printer.
 * @param[in]     _name     the name, with an optional "section." scope.
 * @param[in]     _value    the value, as its kind reads text.
 * @return as d_option_env_assign; INVALID_ARGUMENT for a template that does
 *         not compile.
 */
struct dj_option_result d_env_printer_set(struct dj_env_printer* _printer,
                                          const char*            _name,
                                          const char*            _value);

/**
 * @brief Carries out one statement: "name=value", "name", "-name".
 *
 * @param[in,out] _printer    the printer.
 * @param[in]     _statement  the statement.
 * @return as d_option_env_put; INVALID_ARGUMENT for a template that does not
 *         compile.
 */
struct dj_option_result d_env_printer_put(struct dj_env_printer* _printer,
                                          const char*            _statement);

/**
 * @brief Carries out a script of statements.
 *
 * @param[in,out] _printer       the printer.
 * @param[in]     _script        the script.
 * @param[out]    _error_offset  where a failing statement starts; may be
 *                               null.
 * @return as d_option_env_load; INVALID_ARGUMENT for a template that does
 *         not compile.
 */
struct dj_option_result d_env_printer_load(
                            struct dj_env_printer* _printer,
                            const char*            _script,
                            size_t*                _error_offset);

/**
 * @brief Sets variables from the process's environment.
 *
 * @param[in,out] _printer  the printer.
 * @param[in]     _prefix   the prefix; D_ENV_PRINTER_ENV_PREFIX, usually.
 * @return as d_option_env_import; INVALID_ARGUMENT for a template that does
 *         not compile.
 */
struct dj_option_result d_env_printer_import(struct dj_env_printer* _printer,
                                             const char*            _prefix);

/**
 * @brief Sets a part's template, compiled first, so a template that does not
 *        compile is refused and the one before it kept.
 *
 * @param[in,out] _printer  the printer.
 * @param[in]     _part     the part, enum dj_env_template.
 * @param[in]     _source   the template, copied; null for the style's.
 * @return the variable's index; INVALID_ARGUMENT for a template that does
 *         not compile, with d_env_printer_last_error saying where; or as
 *         d_option_env_set_text_copy.
 */
struct dj_option_result d_env_printer_set_template(
                            struct dj_env_printer* _printer,
                            int32_t                _part,
                            const char*            _source);

// 2.7    Printing
//------------------------------------------------------------------------------
/**
 * @brief Prints a report onto a stream.
 *
 * @note Writes only: the stream is neither flushed nor closed. A section the
 *       report does not have -- a C++ section in a C unit's -- is left out.
 *
 * @param[in,out] _printer  the printer; its program is compiled if a variable
 *                          changed.
 * @param[in,out] _os       the stream.
 * @param[in]     _report   the report, as d_env_report_here gives it.
 * @return 1 when every byte was accepted, 0 otherwise; the stream's status
 *         says why, and a null argument, a template that does not compile or
 *         a hook that stopped is recorded as D_OSTREAM_INVALID.
 */
int32_t d_env_printer_print(struct dj_env_printer*      _printer,
                            struct dj_ostream*          _os,
                            const struct dj_env_report* _report);

/**
 * @brief Writes the printer's variables as a script that sets them again:
 *        each with its help as a comment, then "name = value", scoped
 *        bindings as "section.name = value".
 *
 * @param[in]     _printer   the printer.
 * @param[in,out] _os        the stream.
 * @param[in]     _defaults  non-zero to write unbound variables too,
 *                           commented out, at their defaults.
 * @return as d_ostream_write().
 */
int32_t d_env_printer_dump(const struct dj_env_printer* _printer,
                           struct dj_ostream*           _os,
                           int32_t                      _defaults);

// why the program last failed to compile; status OK when it did not. Not
// named d_env_printer_error: in C++ the function would hide the struct
const struct dj_env_printer_error*
            d_env_printer_last_error(const struct dj_env_printer* _printer);
// a style's template for a part, as the presets have it; null for a style
// or part that is none
const char* d_env_style_template(int32_t _style,
                                 int32_t _part);

#endif  // RE_STD_HAS_INT64


//==============================================================================
// 3.  THE CALLER'S ENVIRONMENT
//==============================================================================
//   Internal linkage, every one: each unit that calls them compiles its own,
// over what env.h detected for that unit.


// 3.1    Table entries
//------------------------------------------------------------------------------
//   Macros because each builds a brace initialiser for a constant table, which
// C++98 cannot initialise member by member, and because the feature entry
// pastes one name into the four macros env_cpp_features.h defines for it.
// Each argument is used once.

// 3.1.1
// D_INTERNAL_ENV_PRINTER_TEXT
//   macro: an entry whose value is text.
#define D_INTERNAL_ENV_PRINTER_TEXT(name, label, text)                         \
    { (name), (label), (text), 0L, D_ENV_VALUE_TEXT, 0 }

// 3.1.2
// D_INTERNAL_ENV_PRINTER_INT
//   macro: an entry whose value is an integer.
#define D_INTERNAL_ENV_PRINTER_INT(name, label, value)                         \
    { (name), (label), NULL, (long)(value), D_ENV_VALUE_INTEGER, 0 }

// 3.1.3
// D_INTERNAL_ENV_PRINTER_BOOL
//   macro: an entry whose value is true or false.
#define D_INTERNAL_ENV_PRINTER_BOOL(name, label, value)                        \
    { (name), (label), NULL, ((value) != 0) ? 1L : 0L, D_ENV_VALUE_BOOLEAN, 0 }

// 3.1.4
// D_INTERNAL_ENV_PRINTER_FEATURE
//   macro: a C++ feature's entry, from the four macros of its name.
#define D_INTERNAL_ENV_PRINTER_FEATURE(feature)                                \
    { D_ENV_CPP_FEATURE_##feature##_NAME,                                      \
      D_ENV_CPP_FEATURE_##feature##_DESC,                                      \
      D_ENV_CPP_FEATURE_##feature##_VERS,                                      \
      (long)(D_ENV_CPP_FEATURE_##feature##_VAL) }

// 3.2    Collection
//------------------------------------------------------------------------------
/**
 * @brief The environment of the unit that calls it, as a report.
 *
 * @note Every unit compiles its own, over what env.h detected for it. Every
 *       table is constant and initialised before the program runs, so a call
 *       costs nothing but its return, and any number of threads may call it.
 *
 * @return the report, constant, alive for the program's life.
 *
 * A table, not logic: its length is the number of properties env.h detects,
 * and splitting it would only scatter the list.
 */
D_STATIC_INLINE const struct dj_env_report*
d_env_report_here(void)
{
    static const struct dj_env_entry language[] =
    {
#ifdef D_ENV_LANG_CPP_STANDARD_NAME
        D_INTERNAL_ENV_PRINTER_TEXT("cpp_standard",
                                    "C++ Standard",
                                    D_ENV_LANG_CPP_STANDARD_NAME),
#endif  // D_ENV_LANG_CPP_STANDARD_NAME
#ifdef __cplusplus
        D_INTERNAL_ENV_PRINTER_INT("cplusplus",
                                   "__cplusplus",
                                   __cplusplus),
#endif  // __cplusplus
#ifdef D_ENV_LANG_C_STANDARD_NAME
        D_INTERNAL_ENV_PRINTER_TEXT("c_standard",
                                    "C Standard",
                                    D_ENV_LANG_C_STANDARD_NAME),
#endif  // D_ENV_LANG_C_STANDARD_NAME
#ifdef __STDC_VERSION__
        D_INTERNAL_ENV_PRINTER_INT("stdc_version",
                                   "__STDC_VERSION__",
                                   __STDC_VERSION__),
#endif  // __STDC_VERSION__
        D_INTERNAL_ENV_PRINTER_BOOL("using_cpp",
                                    "Using C++",
                                    D_ENV_LANG_USING_CPP),
        D_INTERNAL_ENV_PRINTER_BOOL("using_c",
                                    "Using C",
                                    D_ENV_LANG_USING_C),
        D_INTERNAL_ENV_PRINTER_BOOL("long_long",
                                    "long long",
                                    D_ENV_HAS_LONG_LONG),
    };
    static const struct dj_env_entry compiler[] =
    {
#ifdef D_ENV_COMPILER_NAME
        D_INTERNAL_ENV_PRINTER_TEXT("name",
                                    "Compiler",
                                    D_ENV_COMPILER_NAME),
#else
        D_INTERNAL_ENV_PRINTER_TEXT("name",
                                    "Compiler",
                                    "Unknown"),
#endif  // D_ENV_COMPILER_NAME
#ifdef D_ENV_COMPILER_FULL_NAME
        D_INTERNAL_ENV_PRINTER_TEXT("full_name",
                                    "Full Name",
                                    D_ENV_COMPILER_FULL_NAME),
#endif  // D_ENV_COMPILER_FULL_NAME
#ifdef D_ENV_COMPILER_VERSION_STRING
        D_INTERNAL_ENV_PRINTER_TEXT("version",
                                    "Version",
                                    D_ENV_COMPILER_VERSION_STRING),
#endif  // D_ENV_COMPILER_VERSION_STRING
#ifdef D_ENV_COMPILER_MAJOR
        D_INTERNAL_ENV_PRINTER_INT("major",
                                   "Major",
                                   D_ENV_COMPILER_MAJOR),
#endif  // D_ENV_COMPILER_MAJOR
#ifdef D_ENV_COMPILER_MINOR
        D_INTERNAL_ENV_PRINTER_INT("minor",
                                   "Minor",
                                   D_ENV_COMPILER_MINOR),
#endif  // D_ENV_COMPILER_MINOR
#ifdef D_ENV_COMPILER_PATCHLEVEL
        D_INTERNAL_ENV_PRINTER_INT("patch",
                                   "Patch",
                                   D_ENV_COMPILER_PATCHLEVEL),
#endif  // D_ENV_COMPILER_PATCHLEVEL
    };
    static const struct dj_env_entry os[] =
    {
#ifdef D_ENV_OS_NAME
        D_INTERNAL_ENV_PRINTER_TEXT("name",
                                    "OS",
                                    D_ENV_OS_NAME),
#else
        D_INTERNAL_ENV_PRINTER_TEXT("name",
                                    "OS",
                                    "Unknown"),
#endif  // D_ENV_OS_NAME
#ifdef D_ENV_PLATFORM_NAME
        D_INTERNAL_ENV_PRINTER_TEXT("platform",
                                    "Platform",
                                    D_ENV_PLATFORM_NAME),
#endif  // D_ENV_PLATFORM_NAME
#ifdef D_ENV_OS_ID
        D_INTERNAL_ENV_PRINTER_INT("id",
                                   "OS Flag",
                                   D_ENV_OS_ID),
        D_INTERNAL_ENV_PRINTER_BOOL("posix_like",
                                    "POSIX-like",
                                    D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)),
        D_INTERNAL_ENV_PRINTER_BOOL("windows",
                                    "Windows",
                                    D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)),
#endif  // D_ENV_OS_ID
#ifdef __STDC_HOSTED__
        D_INTERNAL_ENV_PRINTER_BOOL("hosted",
                                    "Hosted",
                                    __STDC_HOSTED__),
#endif  // __STDC_HOSTED__
    };
    static const struct dj_env_entry arch[] =
    {
#ifdef D_ENV_ARCH_NAME
        D_INTERNAL_ENV_PRINTER_TEXT("name",
                                    "Architecture",
                                    D_ENV_ARCH_NAME),
#else
        D_INTERNAL_ENV_PRINTER_TEXT("name",
                                    "Architecture",
                                    "Unknown"),
#endif  // D_ENV_ARCH_NAME
#ifdef D_ENV_ARCH_BITS
        D_INTERNAL_ENV_PRINTER_INT("bits",
                                   "Bit Width",
                                   D_ENV_ARCH_BITS),
#endif  // D_ENV_ARCH_BITS
        D_INTERNAL_ENV_PRINTER_INT("pointer_bits",
                                   "Pointer Bits",
                                   D_ENV_ARCH_POINTER_BITS),
        D_INTERNAL_ENV_PRINTER_TEXT("endianness",
                                    "Endianness",
                                    (D_ENV_ARCH_IS_LITTLE_ENDIAN)
                                        ? "Little"
                                        : ( (D_ENV_ARCH_IS_BIG_ENDIAN)
                                            ? "Big"
                                            : "Unknown" )),
    };
    static const struct dj_env_entry posix[] =
    {
#ifdef D_ENV_POSIX_NAME
        D_INTERNAL_ENV_PRINTER_TEXT("standard",
                                    "POSIX",
                                    D_ENV_POSIX_NAME),
#endif  // D_ENV_POSIX_NAME
        D_INTERNAL_ENV_PRINTER_BOOL("available",
                                    "Available",
                                    D_ENV_POSIX_IS_AVAILABLE),
#ifdef D_ENV_POSIX_XSI_NAME
        D_INTERNAL_ENV_PRINTER_TEXT("xsi",
                                    "XSI",
                                    D_ENV_POSIX_XSI_NAME),
#endif  // D_ENV_POSIX_XSI_NAME
        D_INTERNAL_ENV_PRINTER_BOOL("threads",
                                    "Threads",
                                    D_ENV_POSIX_FEATURE_THREADS),
        D_INTERNAL_ENV_PRINTER_BOOL("realtime",
                                    "Realtime",
                                    D_ENV_POSIX_FEATURE_REALTIME),
        D_INTERNAL_ENV_PRINTER_BOOL("sockets",
                                    "Sockets",
                                    D_ENV_POSIX_FEATURE_SOCKETS),
    };
    static const struct dj_env_entry pp_limits[] =
    {
#ifdef D_ENV_PP_LIMIT_SOURCE
        D_INTERNAL_ENV_PRINTER_TEXT("source",
                                    "Source",
                                    D_ENV_PP_LIMIT_SOURCE),
#endif  // D_ENV_PP_LIMIT_SOURCE
        D_INTERNAL_ENV_PRINTER_INT("max_macro_args",
                                   "Max Macro Args",
                                   D_ENV_PP_MAX_MACRO_ARGS),
        D_INTERNAL_ENV_PRINTER_INT("max_nesting_depth",
                                   "Max Nesting Depth",
                                   D_ENV_PP_MAX_NESTING_DEPTH),
        D_INTERNAL_ENV_PRINTER_BOOL("has_va_opt",
                                    "Has __VA_OPT__",
                                    D_ENV_PP_HAS_VA_OPT),
    };
    static const struct dj_env_entry build[] =
    {
#ifdef D_ENV_BUILD_TYPE
        D_INTERNAL_ENV_PRINTER_TEXT("type",
                                    "Build Type",
                                    D_ENV_BUILD_TYPE),
#else
        D_INTERNAL_ENV_PRINTER_TEXT("type",
                                    "Build Type",
                                    "Unknown"),
#endif  // D_ENV_BUILD_TYPE
#ifdef D_ENV_BUILD_DEBUG
        D_INTERNAL_ENV_PRINTER_BOOL("debug",
                                    "Debug",
                                    1),
#else
        D_INTERNAL_ENV_PRINTER_BOOL("debug",
                                    "Debug",
                                    0),
#endif  // D_ENV_BUILD_DEBUG
    };
    static const struct dj_env_entry c_runtime[] =
    {
        D_INTERNAL_ENV_PRINTER_BOOL("aligned_alloc",
                                    "aligned_alloc",
                                    D_ENV_C_HAS_ALIGNED_ALLOC),
        D_INTERNAL_ENV_PRINTER_BOOL("_aligned_malloc",
                                    "_aligned_malloc",
                                    D_ENV_C_HAS_ALIGNED_MALLOC),
        D_INTERNAL_ENV_PRINTER_BOOL("alloca",
                                    "alloca",
                                    D_ENV_C_HAS_ALLOCA),
        D_INTERNAL_ENV_PRINTER_BOOL("avx",
                                    "AVX",
                                    D_ENV_C_HAS_AVX),
        D_INTERNAL_ENV_PRINTER_BOOL("avx2",
                                    "AVX2",
                                    D_ENV_C_HAS_AVX2),
        D_INTERNAL_ENV_PRINTER_BOOL("bsd_sockets",
                                    "BSD sockets",
                                    D_ENV_C_HAS_BSD_SOCKETS),
        D_INTERNAL_ENV_PRINTER_BOOL("c11_threads",
                                    "C11 threads",
                                    D_ENV_C_HAS_C11_THREADS),
        D_INTERNAL_ENV_PRINTER_BOOL("clock_gettime",
                                    "clock_gettime",
                                    D_ENV_C_HAS_CLOCK_GETTIME),
        D_INTERNAL_ENV_PRINTER_BOOL("complex_h",
                                    "complex.h",
                                    D_ENV_C_HAS_COMPLEX_H),
        D_INTERNAL_ENV_PRINTER_BOOL("dirent_h",
                                    "dirent.h",
                                    D_ENV_C_HAS_DIRENT_H),
        D_INTERNAL_ENV_PRINTER_BOOL("execve",
                                    "execve",
                                    D_ENV_C_HAS_EXECVE),
        D_INTERNAL_ENV_PRINTER_BOOL("fenv_h",
                                    "fenv.h",
                                    D_ENV_C_HAS_FENV_H),
        D_INTERNAL_ENV_PRINTER_BOOL("flock",
                                    "flock",
                                    D_ENV_C_HAS_FLOCK),
        D_INTERNAL_ENV_PRINTER_BOOL("flockfile",
                                    "flockfile",
                                    D_ENV_C_HAS_LOCKFILE),
        D_INTERNAL_ENV_PRINTER_BOOL("fopen_s",
                                    "fopen_s",
                                    D_ENV_C_HAS_FOPEN_S),
        D_INTERNAL_ENV_PRINTER_BOOL("fork",
                                    "fork",
                                    D_ENV_C_HAS_FORK),
        D_INTERNAL_ENV_PRINTER_BOOL("fsync",
                                    "fsync",
                                    D_ENV_C_HAS_FSYNC),
        D_INTERNAL_ENV_PRINTER_BOOL("getaddrinfo",
                                    "getaddrinfo",
                                    D_ENV_C_HAS_GETADDRINFO),
        D_INTERNAL_ENV_PRINTER_BOOL("getentropy",
                                    "getentropy",
                                    D_ENV_C_HAS_GETENTROPY),
        D_INTERNAL_ENV_PRINTER_BOOL("getpid",
                                    "getpid",
                                    D_ENV_C_HAS_GETPID),
        D_INTERNAL_ENV_PRINTER_BOOL("gettimeofday",
                                    "gettimeofday",
                                    D_ENV_C_HAS_GETTIMEOFDAY),
        D_INTERNAL_ENV_PRINTER_BOOL("inttypes_h",
                                    "inttypes.h",
                                    D_ENV_C_HAS_INTTYPES_H),
        D_INTERNAL_ENV_PRINTER_BOOL("memccpy",
                                    "memccpy",
                                    D_ENV_C_HAS_MEMCCPY),
        D_INTERNAL_ENV_PRINTER_BOOL("mmap",
                                    "mmap",
                                    D_ENV_C_HAS_MMAP),
        D_INTERNAL_ENV_PRINTER_BOOL("neon",
                                    "NEON",
                                    D_ENV_C_HAS_NEON),
        D_INTERNAL_ENV_PRINTER_BOOL("posix_memalign",
                                    "posix_memalign",
                                    D_ENV_C_HAS_POSIX_MEMALIGN),
        D_INTERNAL_ENV_PRINTER_BOOL("pthread",
                                    "pthread",
                                    D_ENV_C_HAS_PTHREAD),
        D_INTERNAL_ENV_PRINTER_BOOL("queryperformancecounter",
                                    "QueryPerformanceCounter",
                                    D_ENV_C_HAS_QUERYPERFORMANCECOUNTER),
        D_INTERNAL_ENV_PRINTER_BOOL("scanf_s",
                                    "scanf_s",
                                    D_ENV_C_HAS_SCANF_S),
        D_INTERNAL_ENV_PRINTER_BOOL("annex_k_strings",
                                    "Annex K strings",
                                    D_ENV_C_HAS_SECURE_STRING_LIB),
        D_INTERNAL_ENV_PRINTER_BOOL("signal_h",
                                    "signal.h",
                                    D_ENV_C_HAS_SIGNAL_H),
        D_INTERNAL_ENV_PRINTER_BOOL("snprintf",
                                    "snprintf",
                                    D_ENV_C_HAS_SNPRINTF),
        D_INTERNAL_ENV_PRINTER_BOOL("sse",
                                    "SSE",
                                    D_ENV_C_HAS_SSE),
        D_INTERNAL_ENV_PRINTER_BOOL("sse2",
                                    "SSE2",
                                    D_ENV_C_HAS_SSE2),
        D_INTERNAL_ENV_PRINTER_BOOL("stdalign_h",
                                    "stdalign.h",
                                    D_ENV_C_HAS_STDALIGN_H),
        D_INTERNAL_ENV_PRINTER_BOOL("stdatomic_h",
                                    "stdatomic.h",
                                    D_ENV_C_HAS_STDATOMIC),
        D_INTERNAL_ENV_PRINTER_BOOL("stdbool_h",
                                    "stdbool.h",
                                    D_ENV_C_HAS_STDBOOL_H),
        D_INTERNAL_ENV_PRINTER_BOOL("stdint_h",
                                    "stdint.h",
                                    D_ENV_C_HAS_STDINT_H),
        D_INTERNAL_ENV_PRINTER_BOOL("strcasecmp",
                                    "strcasecmp",
                                    D_ENV_C_HAS_STRCASECMP),
        D_INTERNAL_ENV_PRINTER_BOOL("strdup",
                                    "strdup",
                                    D_ENV_C_HAS_STRDUP),
        D_INTERNAL_ENV_PRINTER_BOOL("_stricmp",
                                    "_stricmp",
                                    D_ENV_C_HAS_STRICMP),
        D_INTERNAL_ENV_PRINTER_BOOL("strndup",
                                    "strndup",
                                    D_ENV_C_HAS_STRNDUP),
        D_INTERNAL_ENV_PRINTER_BOOL("strtok_r",
                                    "strtok_r",
                                    D_ENV_C_HAS_STRTOK_R),
        D_INTERNAL_ENV_PRINTER_BOOL("strtok_s",
                                    "strtok_s",
                                    D_ENV_C_HAS_STRTOK_S),
        D_INTERNAL_ENV_PRINTER_BOOL("sys_stat_h",
                                    "sys/stat.h",
                                    D_ENV_C_HAS_SYS_STAT_H),
        D_INTERNAL_ENV_PRINTER_BOOL("sys_types_h",
                                    "sys/types.h",
                                    D_ENV_C_HAS_SYS_TYPES_H),
        D_INTERNAL_ENV_PRINTER_BOOL("tgmath_h",
                                    "tgmath.h",
                                    D_ENV_C_HAS_TGMATH_H),
        D_INTERNAL_ENV_PRINTER_BOOL("timespec_get",
                                    "timespec_get",
                                    D_ENV_C_HAS_TIMESPEC_GET),
        D_INTERNAL_ENV_PRINTER_BOOL("uchar_h",
                                    "uchar.h",
                                    D_ENV_C_HAS_UCHAR_H),
        D_INTERNAL_ENV_PRINTER_BOOL("unistd_h",
                                    "unistd.h",
                                    D_ENV_C_HAS_UNISTD_H),
        D_INTERNAL_ENV_PRINTER_BOOL("vla",
                                    "VLA",
                                    D_ENV_C_HAS_VLA),
        D_INTERNAL_ENV_PRINTER_BOOL("windows_threads",
                                    "Windows threads",
                                    D_ENV_C_HAS_WINDOWS_THREADS),
        D_INTERNAL_ENV_PRINTER_BOOL("winsock",
                                    "Winsock",
                                    D_ENV_C_HAS_WINSOCK),
    };
#if D_ENV_LANG_USING_CPP
    static const struct dj_env_entry cpp_summary[] =
    {
        D_INTERNAL_ENV_PRINTER_BOOL("cpp11_language",
                                    "C++11 language",
                                    D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP11),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp11",
                                    "C++11",
                                    D_ENV_CPP_FEATURE_HAS_ALL_CPP11),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp14_language",
                                    "C++14 language",
                                    D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp14_library",
                                    "C++14 library",
                                    D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp14",
                                    "C++14",
                                    D_ENV_CPP_FEATURE_HAS_ALL_CPP14),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp17_language",
                                    "C++17 language",
                                    D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp17_library",
                                    "C++17 library",
                                    D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp17",
                                    "C++17",
                                    D_ENV_CPP_FEATURE_HAS_ALL_CPP17),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp20_language",
                                    "C++20 language",
                                    D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp20_library",
                                    "C++20 library",
                                    D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp20",
                                    "C++20",
                                    D_ENV_CPP_FEATURE_HAS_ALL_CPP20),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp23_language",
                                    "C++23 language",
                                    D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp23_library",
                                    "C++23 library",
                                    D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp23",
                                    "C++23",
                                    D_ENV_CPP_FEATURE_HAS_ALL_CPP23),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp26_language",
                                    "C++26 language",
                                    D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP26),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp26_library",
                                    "C++26 library",
                                    D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP26),
        D_INTERNAL_ENV_PRINTER_BOOL("cpp26",
                                    "C++26",
                                    D_ENV_CPP_FEATURE_HAS_ALL_CPP26),
    };
#if D_CFG_IS_ON(D_CFG_ENV_PRINTER_CPP_FEATURES)
    static const struct dj_env_feature cpp_features[] =
    {
        // BEGIN GENERATED: tools/gen_env_printer_features.py
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_ALIAS_TEMPLATES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_ATTRIBUTES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CONSTEXPR),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_DECLTYPE),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_DELEGATING_CONSTRUCTORS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_INHERITING_CONSTRUCTORS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_INITIALIZER_LISTS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_LAMBDAS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_NSDMI),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_RANGE_BASED_FOR),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_RAW_STRINGS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_REF_QUALIFIERS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_RVALUE_REFERENCES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_STATIC_ASSERT),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_THREADSAFE_STATIC_INIT),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_UNICODE_CHARACTERS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_UNICODE_LITERALS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_USER_DEFINED_LITERALS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_VARIADIC_TEMPLATES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_AGGREGATE_NSDMI),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_BINARY_LITERALS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_DECLTYPE_AUTO),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_ENUMERATOR_ATTRIBUTES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_GENERIC_LAMBDAS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_INIT_CAPTURES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_NAMESPACE_ATTRIBUTES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_NONTYPE_TEMPLATE_ARGS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_RETURN_TYPE_DEDUCTION),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_SIZED_DEALLOCATION),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_VARIABLE_TEMPLATES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_AGGREGATE_BASES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_ALIGNED_NEW),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CAPTURE_STAR_THIS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CONSTEXPR_IN_DECLTYPE),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_DEDUCTION_GUIDES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_FOLD_EXPRESSIONS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_GUARANTEED_COPY_ELISION),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_HEX_FLOAT),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_IF_CONSTEXPR),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_INLINE_VARIABLES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_NOEXCEPT_FUNCTION_TYPE),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_STRUCTURED_BINDINGS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_TEMPLATE_TEMPLATE_ARGS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_VARIADIC_USING),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_AGGREGATE_PAREN_INIT),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CHAR8_T),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CONCEPTS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CONDITIONAL_EXPLICIT),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CONSTEVAL),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CONSTEXPR_DYNAMIC_ALLOC),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CONSTINIT),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_DESIGNATED_INITIALIZERS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_IMPL_COROUTINE),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_IMPL_DESTROYING_DELETE),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_IMPL_THREE_WAY_COMPARISON),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_MODULES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_USING_ENUM),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_AUTO_CAST),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_EXPLICIT_THIS_PARAMETER),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_IF_CONSTEVAL),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_IMPLICIT_MOVE),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_MULTIDIMENSIONAL_SUBSCRIPT),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_NAMED_CHARACTER_ESCAPES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_SIZE_T_SUFFIX),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_STATIC_CALL_OPERATOR),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CONSTEXPR_EXCEPTIONS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_CONTRACTS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_DELETED_FUNCTION),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_PACK_INDEXING),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_PLACEHOLDER_VARIABLES),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_PP_EMBED),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_TEMPLATE_PARAMETERS),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_TRIVIAL_RELOCATABILITY),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_TRIVIAL_UNION),
        D_INTERNAL_ENV_PRINTER_FEATURE(LANG_VARIADIC_FRIEND),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CHRONO_UDLS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_COMPLEX_UDLS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ADDRESSOF_CONSTEXPR),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ANY),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_APPLY),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ARRAY_CONSTEXPR),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_AS_CONST),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BOOL_CONSTANT),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BOYER_MOORE_SEARCHER),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BYTE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CLAMP),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_FILESYSTEM),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_OPTIONAL),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_VARIANT),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ASSUME_ALIGNED),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ATOMIC_FLAG_TEST),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ATOMIC_FLOAT),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ATOMIC_REF),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ATOMIC_WAIT),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BARRIER),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BIND_FRONT),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BIT_CAST),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BITOPS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BOUNDED_ARRAY_TRAITS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CHAR8_T),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONCEPTS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_ALGORITHMS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_COMPLEX),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_DYNAMIC_ALLOC),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_STRING),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_VECTOR),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_COROUTINE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ENDIAN),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_FORMAT),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_JTHREAD),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_LATCH),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_MATH_CONSTANTS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_RANGES),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_SEMAPHORE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_SOURCE_LOCATION),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_SPAN),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_THREE_WAY_COMPARISON),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_TO_ARRAY),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_IS_CONSTANT_EVALUATED),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BIND_BACK),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_BYTESWAP),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_BITSET),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_CHARCONV),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_CMATH),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_EXPECTED),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_FLAT_MAP),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_FLAT_SET),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_GENERATOR),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_MDSPAN),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_MOVE_ONLY_FUNCTION),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_PRINT),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_RANGES_TO_CONTAINER),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_SPANSTREAM),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_STACKTRACE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_STDATOMIC_H),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_STRING_CONTAINS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_STRING_RESIZE_AND_OVERWRITE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_UNREACHABLE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ALGORITHM_DEFAULT_VALUE_TYPE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_ATOMIC_MIN_MAX),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_ATOMIC),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONSTEXPR_DEQUE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_CONTRACTS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_COPYABLE_FUNCTION),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_DEBUGGING),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_FORMAT_PATH),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_FUNCTION_REF),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_HAZARD_POINTER),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_HIVE),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_INPLACE_VECTOR),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_LINALG),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_POLYMORPHIC),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_RCU),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_SATURATION_ARITHMETIC),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_SENDERS),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_SIMD),
        D_INTERNAL_ENV_PRINTER_FEATURE(STL_TEXT_ENCODING)
        // END GENERATED: tools/gen_env_printer_features.py
    };
#endif  // D_CFG_ENV_PRINTER_CPP_FEATURES
#endif  // D_ENV_LANG_USING_CPP
    static const struct dj_env_section sections[] =
    {
        { "language", "Language", NULL,
          language, sizeof(language) / sizeof(language[0]),
          NULL, 0, D_ENV_SECTION_LANGUAGE, 0 },
        { "compiler", "Compiler", NULL,
          compiler, sizeof(compiler) / sizeof(compiler[0]),
          NULL, 0, D_ENV_SECTION_COMPILER, 0 },
        { "os", "Operating System", NULL,
          os, sizeof(os) / sizeof(os[0]),
          NULL, 0, D_ENV_SECTION_OS, 0 },
        { "architecture", "Architecture", NULL,
          arch, sizeof(arch) / sizeof(arch[0]),
          NULL, 0, D_ENV_SECTION_ARCH, 0 },
        { "posix", "POSIX", NULL,
          posix, sizeof(posix) / sizeof(posix[0]),
          NULL, 0, D_ENV_SECTION_POSIX, 0 },
        { "preprocessor_limits", "Preprocessor Limits", NULL,
          pp_limits, sizeof(pp_limits) / sizeof(pp_limits[0]),
          NULL, 0, D_ENV_SECTION_PP_LIMITS, 0 },
        { "build", "Build", NULL,
          build, sizeof(build) / sizeof(build[0]),
          NULL, 0, D_ENV_SECTION_BUILD, 0 },
        { "c_runtime", "C Runtime Features", NULL,
          c_runtime, sizeof(c_runtime) / sizeof(c_runtime[0]),
          NULL, 0, D_ENV_SECTION_C_RUNTIME, 0 },
#if D_ENV_LANG_USING_CPP
        { "cpp_summary", "C++ Feature Summary", NULL,
          cpp_summary, sizeof(cpp_summary) / sizeof(cpp_summary[0]),
          NULL, 0, D_ENV_SECTION_CPP_SUMMARY, 0 },
#if D_CFG_IS_ON(D_CFG_ENV_PRINTER_CPP_FEATURES)
        { "cpp_features", "C++ Features", NULL,
          NULL, 0,
          cpp_features, sizeof(cpp_features) / sizeof(cpp_features[0]),
          D_ENV_SECTION_CPP_FEATURES, 0 },
#else
        { "cpp_features", "C++ Features",
          "not compiled in: build with D_CFG_ENV_PRINTER_CPP_FEATURES=1",
          NULL, 0, NULL, 0, D_ENV_SECTION_CPP_FEATURES, 0 },
#endif  // D_CFG_ENV_PRINTER_CPP_FEATURES
#endif  // D_ENV_LANG_USING_CPP
    };
    static const struct dj_env_report report =
    {
        sections,
        sizeof(sections) / sizeof(sections[0])
    };

    return &report;
}

#if RE_STD_HAS_INT64

// 3.3    Printing it
//------------------------------------------------------------------------------
/**
 * @brief Prints the calling unit's environment onto a stream, with a printer
 *        made for the call and configured by a script.
 *
 * @note Writes only: the stream is neither flushed nor closed. The printer
 *       lives on the stack for the call, about 11 KB; a program printing
 *       often keeps one of its own.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _script  statements, as d_env_printer_load reads them --
 *                         "style=json; sections=all" -- or null for the
 *                         defaults.
 * @return as d_env_printer_print(); 0 as well for a script that fails, which
 *         is recorded as D_OSTREAM_INVALID and prints nothing.
 */
D_STATIC_INLINE int32_t
d_env_print(
    struct dj_ostream* _os,
    const char*        _script
)
{
    struct dj_env_printer printer;

    // a script that fails prints nothing, rather than something unasked for
    if ( (d_env_printer_init(&printer) == 0) ||
         ( (_script != NULL) &&
           (!d_env_printer_load(&printer, _script, NULL).is_ok) ) )
    {
        return d_ostream_fail(_os, D_OSTREAM_INVALID, 0);
    }

    return d_env_printer_print(&printer, _os, d_env_report_here());
}

/**
 * @brief Prints the calling unit's environment to a FILE*, and flushes it.
 *
 * @param[in,out] _file    the file; stdout for the console.
 * @param[in]     _script  as d_env_print(); null for the defaults.
 * @return 1 when the whole report reached the file, 0 otherwise, with errno
 *         set where the file said why.
 */
D_STATIC_INLINE int32_t
d_env_print_file(
    FILE*       _file,
    const char* _script
)
{
    struct dj_ostream os;
    unsigned char     buffer[1024];

    if ( (d_ostream_init_file(&os, _file, 0) == 0) ||
         (d_ostream_set_buffer(&os, buffer, sizeof(buffer)) == 0) )
    {
        return 0;
    }

    (void)d_env_print(&os, _script);

    return d_ostream_close(&os);
}

#endif  // RE_STD_HAS_INT64


D_EXTERN_C_END


#endif  // DJINTERP_ENV_ENV_PRINTER_H
