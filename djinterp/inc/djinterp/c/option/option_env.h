/*******************************************************************************
* djinterp [c]                                                      option_env.h
*
*   An abstract machine's environment: the variables a machine -- a printer,
* a parser, a test runner -- is configured by. They are named and typed, and
* their defaults are pinned in the machine's schema, from its D_CFG_* knobs.
* Frames hold bindings, which can be set at run time, and frames nest. Built
* on the option core: a frame's bindings are dj_option cells in a
* dj_option_set.
*
*   A MACHINE DECLARES, A PROGRAM BINDS. A machine declares its variables once,
* in a constant table of struct dj_option_env_var -- name, kind, default, the
* values it accepts, one line of help -- and reads them through this header.
* A program changes only what it wants changed:
*     d_option_env_put(&env, "key_width=30");                   // by text
*     d_option_env_load(&env, "style=json; select=-cpp_*", NULL);  // a script
*     d_option_env_import(&env, "DJINTERP_ENV_PRINTER_");      // from getenv
*     d_option_env_set_int(&env, 0u, var, 30);                 // by value
* Nothing is copied at start: a variable with no binding reads its schema's
* default, so a frame holds what was set and nothing else.
*
*   LOCAL MEANS TWO THINGS, AND BOTH ARE HERE.
*   A FRAME is local to the machine that owns it. A frame may name a parent,
* whose bindings and declarations it sees through its own, so one shared
* configuration can sit under any number of machines, each overriding what it
* likes. Any frame may declare variables and scopes of its own; they are
* visible to it and to the frames beneath it, and a name is found innermost
* first. The program's root frame, the djin registry, is the top of every
* chain that wants one (option_env_registry.h).
*   A SCOPE is local to a part of a machine: a printer's section, a parser's
* phase. A frame names its scopes, and a binding made in one -- by text,
* "cpp_features.key_width=40" -- is seen only there, ahead of the frame's
* unscoped binding.
*
*   EVERY BINDING SAYS WHERE IT CAME FROM. A binding is made from a source --
* the program, a file, the process's environment, the command line, or an
* override -- and a frame keeps one binding per source. The value in force is
* the one with the highest standing; among equals, the innermost frame's,
* and in it the scoped binding before the unscoped one; with none, the
* default, which names the knob it came from. So the order in which sources
* are applied cannot change the outcome, and removing one source's binding
* gives back the next one's.
*
*   EVERY CHANGE BUMPS A GENERATION. A machine that compiled something from
* its variables, or caches one on a hot path (dj_option_env_cache), keeps
* d_option_env_generation and reads again when it moves, which covers the
* frame's parents too.
*
*   A frame belongs to one thread at a time while it is written; frames that
* are only read may be shared. Nothing here allocates.
*
*   The option core's 64-bit floor holds here: below it this header compiles
* to nothing (option_common.h).
*
*
* path:      /inc/djinterp/c/option/option_env.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DECLARATIONS
    ------------
    1.  Variables
         1.  dj_option_env_kind
         2.  dj_option_env_name
         3.  dj_option_env_var
    2.  Declaring a variable
         1.  D_OPTION_ENV_ROW
         2.  D_OPTION_ENV_BOOL
         3.  D_OPTION_ENV_INT
         4.  D_OPTION_ENV_INT_RANGE
         5.  D_OPTION_ENV_REAL
         6.  D_OPTION_ENV_TEXT
         7.  D_OPTION_ENV_CHOICE
         8.  D_OPTION_ENV_FLAGS
         9.  D_OPTION_ENV_INT_RANGE_NAMED
    3.  Declaring a variable from a knob
         1.  D_OPTION_ENV_BOOL_KNOB
         2.  D_OPTION_ENV_INT_KNOB
         3.  D_OPTION_ENV_INT_RANGE_KNOB
         4.  D_OPTION_ENV_CHOICE_KNOB
         5.  D_OPTION_ENV_FLAGS_KNOB
         6.  D_OPTION_ENV_INT_RANGE_KNOB_NAMED
         7.  D_OPTION_ENV_TEXT_KNOB
2.  FRAMES
    ------
    1.  The frame
         1.  dj_option_env
         2.  D_OPTION_ENV_GLOBAL
         3.  D_OPTION_ENV_NONE
         4.  D_OPTION_ENV_DEPTH_MAX
         5.  D_OPTION_ENV_ID
         6.  D_OPTION_ENV_ID_DEPTH
         7.  D_OPTION_ENV_ID_INDEX
    2.  Where a value came from
         1.  dj_option_env_source
         2.  dj_option_env_value
         3.  dj_option_env_layer
    3.  Storage
         1.  D_OPTION_ENV_DECLARE
         2.  D_OPTION_ENV_STORAGE_INIT
         3.  D_OPTION_ENV_VIEW
    4.  Setting up
3.  VARIABLES AT RUN TIME
    ---------------------
    1.  Finding
    2.  Reading
    3.  Binding by value
    4.  Binding by text
    5.  Writing as text
    6.  Reading on a hot path
         1.  dj_option_env_cache
         2.  D_OPTION_ENV_CACHE_INIT
*/

#ifndef DJINTERP_C_OPTION_ENV_H
#define DJINTERP_C_OPTION_ENV_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../djinterp.h"           // framework root
#include "./option_common.h"       // struct dj_option, dj_option_result
#include "./option_set_common.h"   // struct dj_option_set
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64: the floor;
                                                // int64_t, uint32_t, int32_t,
                                                // INT64_MIN, INT64_MAX

#if RE_STD_HAS_INT64


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  DECLARATIONS
//==============================================================================


// 1.1    Variables
//------------------------------------------------------------------------------
// 1.1.1
// dj_option_env_kind
//   enum: what a variable holds.
enum dj_option_env_kind
{
    D_OPTION_ENV_KIND_BOOL   = 1,  // bool: 1/0, true/false, yes/no, on/off
    D_OPTION_ENV_KIND_INT    = 2,  // int64_t: decimal, 0x, 0o or 0b
    D_OPTION_ENV_KIND_REAL   = 3,  // double, read in the C locale's form
    D_OPTION_ENV_KIND_TEXT   = 4,  // const char*, NUL-terminated; may be null
    D_OPTION_ENV_KIND_CHOICE = 5,  // int64_t: one of the variable's names
    D_OPTION_ENV_KIND_FLAGS  = 6   // int64_t: an OR of the variable's names
};

// 1.1.2
// dj_option_env_name
//   struct: one named value of a CHOICE or FLAGS variable: a choice's value,
// or the bits a flag name stands for -- one bit, or several, as "all" does.
// An INT may name values too, as "unbounded" for a depth guard: text may give
// one by name, it is written back by name, and it is taken even outside the
// INT's range.
struct dj_option_env_name
{
    const char* name;
    int64_t     value;
};

// 1.1.3
// dj_option_env_var
//   struct: a variable's declaration: one row of a machine's schema, and of
// the djin registry. Rows are constant and borrowed; section 1.2 builds them.
//
//   The default is pinned here and nowhere else (option.h: "DEFAULTS ARE
// PINNED IN THE SCHEMA"): `integer` for BOOL, INT, CHOICE and FLAGS, `real`
// for REAL, `text` for TEXT; `knob` names the D_CFG_* it is, when it is one.
// A name, and an alias, must be an identifier -- letters, digits,
// underscores -- so it can be written in text and in a template.
//   What a row accepts is checked on every binding: an INT within [minimum,
// maximum], or one of its named values; a CHOICE's names, a FLAGS's bits; and
// a default must be one of these. A name in `unbuilt` is ruled
// but not built: refused by name, with D_OPTION_STATUS_POLICY_REJECTED,
// never accepted and ignored.
struct dj_option_env_var
{
    const char*                      name;           // "key_width"
    const char*                      help;           // one line; may be null
    const char*                      alias;          // an older name, or null
    const char*                      knob;           // the default's D_CFG_*,
                                                     // or null
    const struct dj_option_env_name* names;          // CHOICE and FLAGS; an
                                                     // INT's named values
    const struct dj_option_env_name* unbuilt;        // ruled, not built; or
                                                     // null
    const char*                      text;           // TEXT's default
    int64_t                          integer;        // the others' default
    int64_t                          minimum;        // INT: the least taken
    int64_t                          maximum;        // INT: the most taken
    double                           real;           // REAL's default
    uint32_t                         name_count;     // entries in names
    uint32_t                         unbuilt_count;  // entries in unbuilt
    int32_t                          kind;           // enum dj_option_env_kind
    uint32_t                         reserved;       // 0
};

// 1.2    Declaring a variable
//------------------------------------------------------------------------------
//   Brace initialisers for a constant schema table, which C++98 cannot build
// member by member. Each argument is evaluated once; a knob's is also
// stringised, which evaluates nothing.

// 1.2.1
// D_OPTION_ENV_ROW
//   macro: a whole row, every member given: what the macros below build
// from, and what a row with an alias, an unbuilt list or a range of its own
// is written with.
#define D_OPTION_ENV_ROW(kind,                                                 \
                         name,                                                 \
                         names,                                                \
                         name_count,                                           \
                         unbuilt,                                              \
                         unbuilt_count,                                        \
                         text,                                                 \
                         integer,                                              \
                         minimum,                                              \
                         maximum,                                              \
                         real,                                                 \
                         alias,                                                \
                         knob,                                                 \
                         help)                                                 \
    { (name), (help), (alias), (knob), (names), (unbuilt), (text),             \
      (int64_t)(integer), (int64_t)(minimum), (int64_t)(maximum),              \
      (double)(real), (uint32_t)(name_count), (uint32_t)(unbuilt_count),       \
      (int32_t)(kind), 0u }

// 1.2.2
// D_OPTION_ENV_BOOL
//   macro: a bool variable.
#define D_OPTION_ENV_BOOL(name, dflt, help)                                    \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_BOOL, name, NULL, 0u, NULL, 0u, NULL,   \
                     ((dflt) != 0) ? 1 : 0, INT64_MIN, INT64_MAX, 0.0, NULL,   \
                     NULL, help)

// 1.2.3
// D_OPTION_ENV_INT
//   macro: an int variable, any value accepted.
#define D_OPTION_ENV_INT(name, dflt, help)                                     \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, name, NULL, 0u, NULL, 0u, NULL,    \
                     dflt, INT64_MIN, INT64_MAX, 0.0, NULL, NULL, help)

// 1.2.4
// D_OPTION_ENV_INT_RANGE
//   macro: an int variable that accepts [minimum, maximum] only.
#define D_OPTION_ENV_INT_RANGE(name, dflt, minimum, maximum, help)             \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, name, NULL, 0u, NULL, 0u, NULL,    \
                     dflt, minimum, maximum, 0.0, NULL, NULL, help)

// 1.2.5
// D_OPTION_ENV_REAL
//   macro: a real variable.
#define D_OPTION_ENV_REAL(name, dflt, help)                                    \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_REAL, name, NULL, 0u, NULL, 0u, NULL,   \
                     0, INT64_MIN, INT64_MAX, dflt, NULL, NULL, help)

// 1.2.6
// D_OPTION_ENV_TEXT
//   macro: a text variable; its default may be NULL.
#define D_OPTION_ENV_TEXT(name, dflt, help)                                    \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_TEXT, name, NULL, 0u, NULL, 0u, dflt,   \
                     0, INT64_MIN, INT64_MAX, 0.0, NULL, NULL, help)

// 1.2.7
// D_OPTION_ENV_CHOICE
//   macro: a choice variable over an array of struct dj_option_env_name.
#define D_OPTION_ENV_CHOICE(name, names, dflt, help)                           \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_CHOICE, name, names,                    \
                     sizeof(names) / sizeof((names)[0]), NULL, 0u, NULL, dflt, \
                     INT64_MIN, INT64_MAX, 0.0, NULL, NULL, help)

// 1.2.8
// D_OPTION_ENV_FLAGS
//   macro: a flags variable over an array of struct dj_option_env_name.
#define D_OPTION_ENV_FLAGS(name, names, dflt, help)                            \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_FLAGS, name, names,                     \
                     sizeof(names) / sizeof((names)[0]), NULL, 0u, NULL, dflt, \
                     INT64_MIN, INT64_MAX, 0.0, NULL, NULL, help)

// 1.2.9
// D_OPTION_ENV_INT_RANGE_NAMED
//   macro: an int variable that accepts [minimum, maximum], and the values an
// array of struct dj_option_env_name names, inside the range or out of it.
// Text gives a named value by its name, ignoring case, or by its number; it
// is written back by its name. The default may be a named value.
#define D_OPTION_ENV_INT_RANGE_NAMED(name, dflt, minimum, maximum, names,      \
                                     help)                                     \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, name, names,                       \
                     sizeof(names) / sizeof((names)[0]), NULL, 0u, NULL, dflt, \
                     minimum, maximum, 0.0, NULL, NULL, help)

// 1.3    Declaring a variable from a knob
//------------------------------------------------------------------------------
//   The knob-and-option pattern's first half: the default IS the knob, and
// the row records the knob's name, so a reader asked where a value came from
// can say "the default, D_CFG_ENV_PRINTER_STYLE". The knob's cascade (its
// cfg_*.h) decides its value at compile time; a binding overrides it at run
// time.

// 1.3.1
// D_OPTION_ENV_BOOL_KNOB
//   macro: a bool variable whose default is a knob.
#define D_OPTION_ENV_BOOL_KNOB(name, knob, help)                               \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_BOOL, name, NULL, 0u, NULL, 0u, NULL,   \
                     ((knob) != 0) ? 1 : 0, INT64_MIN, INT64_MAX, 0.0, NULL,   \
                     #knob, help)

// 1.3.2
// D_OPTION_ENV_INT_KNOB
//   macro: an int variable whose default is a knob.
#define D_OPTION_ENV_INT_KNOB(name, knob, help)                                \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, name, NULL, 0u, NULL, 0u, NULL,    \
                     knob, INT64_MIN, INT64_MAX, 0.0, NULL, #knob, help)

// 1.3.3
// D_OPTION_ENV_INT_RANGE_KNOB
//   macro: an int variable of a range, whose default is a knob.
#define D_OPTION_ENV_INT_RANGE_KNOB(name, knob, minimum, maximum, help)        \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, name, NULL, 0u, NULL, 0u, NULL,    \
                     knob, minimum, maximum, 0.0, NULL, #knob, help)

// 1.3.4
// D_OPTION_ENV_CHOICE_KNOB
//   macro: a choice variable whose default is a knob.
#define D_OPTION_ENV_CHOICE_KNOB(name, names, knob, help)                      \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_CHOICE, name, names,                    \
                     sizeof(names) / sizeof((names)[0]), NULL, 0u, NULL, knob, \
                     INT64_MIN, INT64_MAX, 0.0, NULL, #knob, help)

// 1.3.5
// D_OPTION_ENV_FLAGS_KNOB
//   macro: a flags variable whose default is a knob.
#define D_OPTION_ENV_FLAGS_KNOB(name, names, knob, help)                       \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_FLAGS, name, names,                     \
                     sizeof(names) / sizeof((names)[0]), NULL, 0u, NULL, knob, \
                     INT64_MIN, INT64_MAX, 0.0, NULL, #knob, help)

// 1.3.6
// D_OPTION_ENV_INT_RANGE_KNOB_NAMED
//   macro: an int variable of a range with named values (1.2.9), whose
// default is a knob; the knob may be one of the named values.
#define D_OPTION_ENV_INT_RANGE_KNOB_NAMED(name, knob, minimum, maximum, names, \
                                          help)                                \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, name, names,                       \
                     sizeof(names) / sizeof((names)[0]), NULL, 0u, NULL, knob, \
                     minimum, maximum, 0.0, NULL, #knob, help)

// 1.3.7
// D_OPTION_ENV_TEXT_KNOB
//   macro: a text variable whose default is a knob: a string literal, or
// NULL.
#define D_OPTION_ENV_TEXT_KNOB(name, knob, help)                               \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_TEXT, name, NULL, 0u, NULL, 0u, knob,   \
                     0, INT64_MIN, INT64_MAX, 0.0, NULL, #knob, help)


//==============================================================================
// 2.  FRAMES
//==============================================================================


// 2.1    The frame
//------------------------------------------------------------------------------
// 2.1.1
// dj_option_env
//   struct: one frame: its bindings, its parent, its own declarations and
// scopes, its text arena. Built over caller storage by D_OPTION_ENV_VIEW; it
// points into that storage, so it is not copied.
//
//   A variable's id is its declaring frame's depth -- its count of ancestors
// -- in bits 20 to 27, and its index among that frame's declarations below;
// a scope's id is the same with the index plus 1, and 0 is the global scope.
// So a root's ids are plain indices, ids from different frames never meet,
// and a frame may declare while frames beneath it exist.
struct dj_option_env
{
    struct dj_option_set              set;            // the bindings
    struct dj_option_env*             parent;         // or null: a root
    const struct dj_option_env_var**  vars;           // its declarations
    const char* const*                scopes;         // scope s's name is
                                                      // scopes[s - 1]
    const char*                       name;           // or null; a djin's
    char*                             text;           // the text arena
    size_t                            text_capacity;  // its size
    size_t                            text_used;      // its high-water mark
    uint32_t                          var_count;      // declared here
    uint32_t                          var_capacity;   // room for more
    uint32_t                          scope_count;    // named here
    uint32_t                          generation;     // bumped by each change
    uint32_t                          depth;          // ancestors: 0 at a root
    uint32_t                          child_count;    // frames it is parent of
};

// 2.1.2
// D_OPTION_ENV_GLOBAL
//   constant: the scope that is everywhere: a binding in it holds in every
// scope that has none of its own.
#define D_OPTION_ENV_GLOBAL     0u

// 2.1.3
// D_OPTION_ENV_NONE
//   constant: no variable: what d_option_env_var_id gives past the last.
#define D_OPTION_ENV_NONE       0xFFFFFFFFu

// 2.1.4
// D_OPTION_ENV_DEPTH_MAX
//   constant: the deepest a frame may be: 255 ancestors.
#define D_OPTION_ENV_DEPTH_MAX  255u

// 2.1.5
// D_OPTION_ENV_ID
//   macro: the id of a frame's own variable, or scope less 1, from the frame's
// depth and its place there: what a machine that keeps a frame's ids needs
// after the frame's depth changes (d_option_env_set_parent).
#define D_OPTION_ENV_ID(depth,                                                 \
                        index)                                                 \
    ( (((uint32_t)(depth) & 0xFFu) << 20) |                                    \
      ((uint32_t)(index) & 0x000FFFFFu) )

// 2.1.6
// D_OPTION_ENV_ID_DEPTH
//   macro: the depth of the frame an id's variable or scope belongs to.
#define D_OPTION_ENV_ID_DEPTH(id)  ( ((uint32_t)(id) >> 20) & 0xFFu )

// 2.1.7
// D_OPTION_ENV_ID_INDEX
//   macro: an id's place among its frame's own variables, or scopes from 1.
#define D_OPTION_ENV_ID_INDEX(id)  ( (uint32_t)(id) & 0x000FFFFFu )

// 2.2    Where a value came from
//------------------------------------------------------------------------------
// 2.2.1
// dj_option_env_source
//   enum: where a binding came from, in ascending standing: a binding yields
// only to one of higher standing, so the order sources are applied in cannot
// change the value in force. DEFAULT is never a binding: it is what an
// unbound variable reads, the schema's default, which a knob may have set.
enum dj_option_env_source
{
    D_OPTION_ENV_SOURCE_DEFAULT      = 0,  // the schema's; its knob's
    D_OPTION_ENV_SOURCE_PROGRAM      = 1,  // the program, in code
    D_OPTION_ENV_SOURCE_FILE         = 2,  // a configuration file or sheet
    D_OPTION_ENV_SOURCE_ENVIRONMENT  = 3,  // the process's environment
    D_OPTION_ENV_SOURCE_COMMAND_LINE = 4,  // the command line
    D_OPTION_ENV_SOURCE_OVERRIDE     = 5,  // forced: a scoped override, a test
    D_OPTION_ENV_SOURCES             = 6
};

// 2.2.2
// dj_option_env_value
//   struct: a variable's value in force, as d_option_env_get reads it, and
// where it came from.
struct dj_option_env_value
{
    const char* text;     // TEXT: the text, or null; CHOICE: its name, or
                          // null for a value with none
    int64_t     integer;  // BOOL (0 or 1), INT, CHOICE, FLAGS
    double      real;     // REAL
    int32_t     kind;     // enum dj_option_env_kind
    int32_t     bound;    // 1 when a frame bound it; 0 for the default
    int32_t     source;   // enum dj_option_env_source; DEFAULT when unbound
    uint32_t    depth;    // the binding frame's depth; D_OPTION_ENV_NONE
                          // for the default
};

// 2.2.3
// dj_option_env_layer
//   struct: one binding that bears on a variable in a scope, as
// d_option_env_layers lists them: what `get --show-origin` shows.
struct dj_option_env_layer
{
    const struct dj_option_env* frame;  // the frame that holds it
    struct dj_option_env_value  value;  // its value, source and depth
    uint32_t                    scope;  // the scope asked, or the global one
    int32_t                     shown;  // 1 for the one in force
};

// 2.3    Storage
//------------------------------------------------------------------------------
// 2.3.1
// D_OPTION_ENV_DECLARE
//   macro: declares a frame's storage: room for `bindings` bindings -- one
// per source, per scope, per variable bound -- `vars` declarations (0 for a
// frame that declares none) and `text` bytes of arena. Every slot a binding
// needs is at most eight bytes and eight-aligned, so a uint64_t per binding
// is enough value block.
#define D_OPTION_ENV_DECLARE(name,                                             \
                             bindings,                                         \
                             vars,                                             \
                             text)                                             \
    struct name                                                                \
    {                                                                          \
        uint64_t                        value_block[(bindings) ? (bindings)    \
                                                               : 1u];          \
        struct dj_option                option_block[(bindings) ? (bindings)   \
                                                                : 1u];         \
        const struct dj_option_env_var* var_block[(vars) ? (vars) : 1u];       \
        char                            text_block[(text) ? (text) : 1u];      \
    }

// 2.3.2
// D_OPTION_ENV_STORAGE_INIT
//   macro: the zero initialiser for a D_OPTION_ENV_DECLARE storage object.
#define D_OPTION_ENV_STORAGE_INIT                                              \
    { { 0u }, { { 0u, 0u, 0u, 0u, 0u, 0u } }, { NULL }, { 0 } }

// 2.3.3
// D_OPTION_ENV_VIEW
//   macro: makes a frame over a D_OPTION_ENV_DECLARE storage object: every
// size is read from the storage, so the frame cannot disagree with it.
#define D_OPTION_ENV_VIEW(env,                                                 \
                          storage)                                             \
    d_option_env_init((env),                                                   \
                      (storage).option_block,                                  \
                      (unsigned char*)(storage).value_block,                   \
                      (uint32_t)(sizeof((storage).option_block) /              \
                                 sizeof((storage).option_block[0])),           \
                      (uint32_t)sizeof((storage).value_block),                 \
                      (storage).var_block,                                     \
                      (uint32_t)(sizeof((storage).var_block) /                 \
                                 sizeof((storage).var_block[0])),              \
                      (storage).text_block,                                    \
                      sizeof((storage).text_block))

// 2.4    Setting up
//------------------------------------------------------------------------------
/**
 * @brief Makes an empty root frame over the caller's storage.
 *
 * @note D_OPTION_ENV_VIEW passes every argument from a storage object.
 *
 * @param[out] _env               the frame.
 * @param[in]  _options           binding storage: _binding_capacity cells.
 * @param[in]  _values            value storage: _value_capacity bytes,
 *                                eight-aligned.
 * @param[in]  _binding_capacity  the most bindings.
 * @param[in]  _value_capacity    the value storage's size.
 * @param[in]  _vars              declaration storage.
 * @param[in]  _var_capacity      the most declarations.
 * @param[in]  _text              the arena; may be null when _text_capacity
 *                                is 0.
 * @param[in]  _text_capacity     its size.
 * @return D_OPTION_STATUS_OK, or INVALID_ARGUMENT for a null pointer where
 *         storage was promised.
 */
struct dj_option_result d_option_env_init(
                            struct dj_option_env*             _env,
                            struct dj_option*                 _options,
                            unsigned char*                    _values,
                            uint32_t                          _binding_capacity,
                            uint32_t                          _value_capacity,
                            const struct dj_option_env_var**  _vars,
                            uint32_t                          _var_capacity,
                            char*                             _text,
                            size_t                            _text_capacity);

/**
 * @brief Declares one variable in a frame: visible to it and to every frame
 *        beneath it, ahead of an ancestor's of the same name.
 *
 * @param[in,out] _env  the frame.
 * @param[in]     _var  the declaration; borrowed, so it must outlive the
 *                      frame.
 * @return the variable's id; KEY_DUPLICATE for a name or alias this frame
 *         already declares, INVALID_ARGUMENT for a declaration that is
 *         malformed (no name; a name or alias that is not an identifier; an
 *         unknown kind; a CHOICE or FLAGS with no names; an INT range that is
 *         empty or leaves out its default), CAPACITY when the frame is full.
 */
struct dj_option_result d_option_env_declare(
                            struct dj_option_env*           _env,
                            const struct dj_option_env_var* _var);

/**
 * @brief Declares a schema: a table of variables, in order, so their ids are
 *        consecutive.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _schema  the table; borrowed.
 * @param[in]     _count   its rows.
 * @return the first variable's id; or the first failure, as
 *         d_option_env_declare, with the rows before it declared.
 */
struct dj_option_result d_option_env_declare_all(
                            struct dj_option_env*           _env,
                            const struct dj_option_env_var* _schema,
                            uint32_t                        _count);

/**
 * @brief Names a frame's scopes: its scope s, from 1, is _names[s - 1].
 *
 * @param[in,out] _env    the frame.
 * @param[in]     _names  the names, identifiers; borrowed.
 * @param[in]     _count  how many.
 * @return the first scope's id; INVALID_ARGUMENT for a null frame, or names
 *         missing.
 */
struct dj_option_result d_option_env_set_scopes(
                            struct dj_option_env* _env,
                            const char* const*    _names,
                            uint32_t              _count);

/**
 * @brief Gives a frame a parent, whose bindings and declarations it then
 *        sees through its own; null makes it a root again.
 *
 * @note The frame's own bindings are removed, since their ids may name the
 *       old chain's variables, and the ids of its own variables change with
 *       its depth.
 *
 * @param[in,out] _env     the frame; no frame may have it as parent.
 * @param[in,out] _parent  the parent; borrowed; it counts its children.
 * @return D_OPTION_STATUS_OK; INVALID_ARGUMENT for a frame with children of
 *         its own, or a parent that would make a cycle; CAPACITY past
 *         D_OPTION_ENV_DEPTH_MAX.
 * @post The generation in force is above any it had before, so a cache
 *       filled from the old chain reads again.
 */
struct dj_option_result d_option_env_set_parent(
                            struct dj_option_env* _env,
                            struct dj_option_env* _parent);

/**
 * @brief Names a frame: a djin's name, by which the registry finds it.
 *
 * @param[in,out] _env   the frame.
 * @param[in]     _name  an identifier, borrowed; or null.
 * @return D_OPTION_STATUS_OK, or INVALID_ARGUMENT for a name that is not an
 *         identifier.
 */
struct dj_option_result d_option_env_set_name(
                            struct dj_option_env* _env,
                            const char*           _name);

/**
 * @brief Removes every binding of a frame, from every source: each variable
 *        reads its parent's value, or its default, again.
 *
 * @param[in,out] _env  the frame.
 */
void        d_option_env_clear(struct dj_option_env* _env);

// the frame's parent -- d_option_env_set_parent(_env, NULL) -- let go of
void        d_option_env_detach(struct dj_option_env* _env);
// the generation in force: the sum of the frame's and its parents' -- a
// machine that compiled from the variables recompiles when it changes
uint32_t    d_option_env_generation(const struct dj_option_env* _env);
// the root frame: the one with no parent
const struct dj_option_env*
            d_option_env_root(const struct dj_option_env* _env);
// the kind's name, "int" say; "unknown" for a value that is none
const char* d_option_env_kind_name(int32_t _kind);
// the source's name, "command line" say; "unknown" for a value that is none
const char* d_option_env_source_name(int32_t _source);


//==============================================================================
// 3.  VARIABLES AT RUN TIME
//==============================================================================
//   Every function takes a scope -- D_OPTION_ENV_GLOBAL or a named one -- and a
// variable's id, from d_option_env_find, a schema's first id plus a row's
// index, or d_option_env_var_id. The status vocabulary is the option core's:
// KEY_ABSENT for an id or scope the frame cannot see, VALUE_TYPE_MISMATCH for
// a value of another kind, INVALID_ARGUMENT for one the variable does not
// accept, POLICY_REJECTED for a value ruled but not built, CAPACITY and
// VALUE_CAPACITY for a frame out of bindings, BUFFER_TOO_SMALL for one out of
// arena.


// 3.1    Finding
//------------------------------------------------------------------------------
//   The variables a frame can see are its ancestors' and its own, counted
// from the root down: d_option_env_var_id gives the id of the k-th of them,
// and d_option_env_var_index the k of an id, so a machine can keep a table
// indexed by k whatever frames declared them.

// how many variables a frame can see; the k-th one's id, D_OPTION_ENV_NONE
// past the last; the k of an id, D_OPTION_ENV_NONE for one it cannot see
uint32_t    d_option_env_var_count(const struct dj_option_env* _env);
uint32_t    d_option_env_var_id(const struct dj_option_env* _env,
                                uint32_t                    _index);
uint32_t    d_option_env_var_index(const struct dj_option_env* _env,
                                   uint32_t                    _var);
// a variable's declaration, by id; null for one the frame cannot see
const struct dj_option_env_var*
            d_option_env_var_at(const struct dj_option_env* _env,
                                uint32_t                    _var);
// the frame that declared a variable, by id; null for one it cannot see
const struct dj_option_env*
            d_option_env_var_frame(const struct dj_option_env* _env,
                                   uint32_t                    _var);
// a scope's name, by id; null for the global scope or one it cannot see
const char* d_option_env_scope_name(const struct dj_option_env* _env,
                                    uint32_t                    _scope);

/**
 * @brief A variable's id from its name or alias, innermost frame first.
 *
 * @param[in] _env   any frame.
 * @param[in] _name  the name; need not end in a NUL.
 * @param[in] _size  its length.
 * @return the id, or KEY_ABSENT.
 */
struct dj_option_result d_option_env_find(const struct dj_option_env* _env,
                                          const char*                 _name,
                                          size_t                      _size);

/**
 * @brief A scope's id from its name, innermost frame first.
 *
 * @param[in] _env   any frame.
 * @param[in] _name  the name; need not end in a NUL.
 * @param[in] _size  its length.
 * @return the scope's id, or KEY_ABSENT.
 */
struct dj_option_result d_option_env_find_scope(
                            const struct dj_option_env* _env,
                            const char*                 _name,
                            size_t                      _size);

/**
 * @brief A variable and its scope from a name that may carry one,
 *        "cpp_features.key_width".
 *
 * @param[in]  _env    any frame.
 * @param[in]  _name   the name; need not end in a NUL.
 * @param[in]  _size   its length.
 * @param[out] _scope  the scope: D_OPTION_ENV_GLOBAL when it has none.
 * @return the variable's id, or KEY_ABSENT for an unknown name or scope.
 */
struct dj_option_result d_option_env_resolve(
                            const struct dj_option_env* _env,
                            const char*                 _name,
                            size_t                      _size,
                            uint32_t*                   _scope);

/**
 * @brief Did you mean: the visible name nearest a name nothing has -- a
 *        variable's, an alias, or, for a name's scope part, a scope's.
 *
 * @note Nearest by edit distance, a swap of two neighbours counting one; at
 *       most 1 for a name of three characters or fewer, and 2 for a longer
 *       one, so a suggestion is a likely typo, never a guess.
 *
 * @param[in] _env   any frame.
 * @param[in] _name  the name, as written; need not end in a NUL.
 * @param[in] _size  its length.
 * @return the name to suggest, borrowed from its declaration; or null.
 */
const char* d_option_env_suggest(const struct dj_option_env* _env,
                                 const char*                 _name,
                                 size_t                      _size);

// the edit distance d_option_env_suggest measures by: ASCII case ignored, a
// swap of two neighbours counting one; 9 when either is over 63 characters
uint32_t    d_option_env_distance(const char* _name,
                                  const char* _text,
                                  size_t      _size);

// 3.2    Reading
//------------------------------------------------------------------------------
/**
 * @brief The value in force for a variable in a scope, and where it came
 *        from: the binding of the highest standing; among equals, the
 *        innermost frame's, and in it the scoped one before the global; with
 *        none, the default.
 *
 * @param[in]  _env    the frame.
 * @param[in]  _scope  the scope.
 * @param[in]  _var    the variable.
 * @param[out] _value  the value.
 * @return the variable's id, or KEY_ABSENT.
 */
struct dj_option_result d_option_env_get(const struct dj_option_env* _env,
                                         uint32_t                    _scope,
                                         uint32_t                    _var,
                                         struct dj_option_env_value* _value);

// the value in force as one type, or 0 / null for a variable that does not
// exist; bool is BOOL's, int is INT's, CHOICE's and FLAGS's, real is REAL's,
// text is TEXT's, a CHOICE's name, and an INT's name when its value has one
bool        d_option_env_bool(const struct dj_option_env* _env,
                              uint32_t                    _scope,
                              uint32_t                    _var);
int64_t     d_option_env_int(const struct dj_option_env* _env,
                             uint32_t                    _scope,
                             uint32_t                    _var);
double      d_option_env_real(const struct dj_option_env* _env,
                              uint32_t                    _scope,
                              uint32_t                    _var);
const char* d_option_env_text(const struct dj_option_env* _env,
                              uint32_t                    _scope,
                              uint32_t                    _var);
// where the value in force came from: enum dj_option_env_source, DEFAULT for
// one unbound, and -1 for a variable that does not exist
int32_t     d_option_env_source(const struct dj_option_env* _env,
                                uint32_t                    _scope,
                                uint32_t                    _var);
// whether this frame -- not a parent -- binds the variable in the scope,
// from any source
bool        d_option_env_is_bound(const struct dj_option_env* _env,
                                  uint32_t                    _scope,
                                  uint32_t                    _var);
// whether any frame of the chain binds anything in the scope: when none
// does, every variable reads in it as it does globally
bool        d_option_env_scope_is_bound(const struct dj_option_env* _env,
                                        uint32_t                    _scope);

/**
 * @brief Every binding of the chain that bears on a variable in a scope, the
 *        one in force first and the rest in the order they would take over.
 *
 * @param[in]  _env       the frame.
 * @param[in]  _scope     the scope.
 * @param[in]  _var       the variable.
 * @param[out] _layers    where; may be null when _capacity is 0.
 * @param[in]  _capacity  its room.
 * @return how many there are, whether or not they fitted; (size_t)-1 for a
 *         variable or scope the frame cannot see.
 */
size_t      d_option_env_layers(const struct dj_option_env* _env,
                                uint32_t                    _scope,
                                uint32_t                    _var,
                                struct dj_option_env_layer* _layers,
                                size_t                      _capacity);

// 3.3    Binding by value
//------------------------------------------------------------------------------
//   Each binds in this frame, replacing the frame's own binding from the same
// source if it had one; the plain setters bind from the program. Text set by
// value is borrowed: it must outlive the binding.

struct dj_option_result d_option_env_set_bool(struct dj_option_env* _env,
                                              uint32_t              _scope,
                                              uint32_t              _var,
                                              bool                  _value);
struct dj_option_result d_option_env_set_int(struct dj_option_env* _env,
                                             uint32_t              _scope,
                                             uint32_t              _var,
                                             int64_t               _value);
struct dj_option_result d_option_env_set_real(struct dj_option_env* _env,
                                              uint32_t              _scope,
                                              uint32_t              _var,
                                              double                _value);
struct dj_option_result d_option_env_set_text(struct dj_option_env* _env,
                                              uint32_t              _scope,
                                              uint32_t              _var,
                                              const char*           _value);

/**
 * @brief Binds a value of any kind from any source.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _scope   the scope.
 * @param[in]     _var     the variable.
 * @param[in]     _source  the source, PROGRAM to OVERRIDE.
 * @param[in]     _value   its kind and the member that kind reads; INT
 *                         carries CHOICE and FLAGS; TEXT is borrowed.
 * @return the variable's id; INVALID_ARGUMENT for DEFAULT or a source that is
 *         none; or as the setters.
 */
struct dj_option_result d_option_env_set_value(
                            struct dj_option_env*             _env,
                            uint32_t                          _scope,
                            uint32_t                          _var,
                            int32_t                           _source,
                            const struct dj_option_env_value* _value);

/**
 * @brief Binds a TEXT variable to a copy of some text, kept in the frame's
 *        arena.
 *
 * @param[in,out] _env    the frame.
 * @param[in]     _scope  the scope.
 * @param[in]     _var    the variable.
 * @param[in]     _value  the text; need not end in a NUL.
 * @param[in]     _size   its length.
 * @return D_OPTION_STATUS_OK, or BUFFER_TOO_SMALL when even a compacted
 *         arena cannot hold it, or as d_option_env_set_text.
 * @post A compaction may move every copy the frame holds; it bumps the
 *       generation, so a machine holding pointers to them knows.
 */
struct dj_option_result d_option_env_set_text_copy(
                            struct dj_option_env* _env,
                            uint32_t              _scope,
                            uint32_t              _var,
                            const char*           _value,
                            size_t                _size);

/**
 * @brief Removes this frame's bindings of a variable in a scope, from every
 *        source: it then reads what the next binding out, or the default,
 *        says.
 *
 * @param[in,out] _env    the frame.
 * @param[in]     _scope  the scope.
 * @param[in]     _var    the variable.
 * @return the variable's id, bound or not; KEY_ABSENT for a variable or scope
 *         the frame cannot see.
 */
struct dj_option_result d_option_env_unset(struct dj_option_env* _env,
                                           uint32_t              _scope,
                                           uint32_t              _var);

/**
 * @brief Removes this frame's binding of a variable in a scope from one
 *        source: the next binding by standing, or the default, then holds.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _scope   the scope.
 * @param[in]     _var     the variable.
 * @param[in]     _source  the source.
 * @return as d_option_env_unset.
 */
struct dj_option_result d_option_env_unset_from(struct dj_option_env* _env,
                                                uint32_t              _scope,
                                                uint32_t              _var,
                                                int32_t               _source);

// 3.4    Binding by text
//------------------------------------------------------------------------------
//   The plain forms bind from the program; the _from forms name their source.
// d_option_env_import binds from the environment.

/**
 * @brief Binds a variable from text, read as its kind reads it.
 *
 * @note The name may carry a scope, "cpp_features.key_width". TEXT is copied
 *       as it is; FLAGS starting with '+' or '-' edits the value in force.
 *
 * @param[in,out] _env         the frame.
 * @param[in]     _name        the name; need not end in a NUL.
 * @param[in]     _name_size   its length.
 * @param[in]     _value       the text; need not end in a NUL.
 * @param[in]     _value_size  its length.
 * @param[in]     _source      the source.
 * @return the variable's id; KEY_ABSENT for an unknown name or scope (ask
 *         d_option_env_suggest), INVALID_ARGUMENT for text the kind does not
 *         read or a value it does not accept, POLICY_REJECTED for one ruled
 *         but not built, or as d_option_env_set_text_copy.
 */
struct dj_option_result d_option_env_assign_from(
                            struct dj_option_env* _env,
                            const char*           _name,
                            size_t                _name_size,
                            const char*           _value,
                            size_t                _value_size,
                            int32_t               _source);

// d_option_env_assign_from, from the program
struct dj_option_result d_option_env_assign(struct dj_option_env* _env,
                                            const char*           _name,
                                            size_t                _name_size,
                                            const char*           _value,
                                            size_t                _value_size);

/**
 * @brief Carries out one statement: "name=value", "name" (a bool, true),
 *        "-name" (this source's binding removed), with an optional scope in
 *        the name.
 *
 * @note Space around the name and the value is ignored. A value in double
 *       quotes is read with C's escapes -- a backslash before a quote, an
 *       apostrophe, a backslash, n, r, t, 0, or x and two hexadecimal
 *       digits -- and may hold anything; otherwise it runs to the end.
 *
 * @param[in,out] _env        the frame.
 * @param[in]     _statement  the statement, NUL-terminated.
 * @param[in]     _source     the source.
 * @return as d_option_env_assign_from; INVALID_ARGUMENT for a malformed
 *         statement.
 */
struct dj_option_result d_option_env_put_from(struct dj_option_env* _env,
                                              const char*           _statement,
                                              int32_t               _source);

// d_option_env_put_from, from the program
struct dj_option_result d_option_env_put(struct dj_option_env* _env,
                                         const char*           _statement);

/**
 * @brief Carries out one statement given as a counted text, which need not
 *        end in a NUL.
 *
 * @param[in,out] _env        the frame.
 * @param[in]     _statement  the statement.
 * @param[in]     _size       its length.
 * @param[in]     _source     the source.
 * @return as d_option_env_put_from.
 */
struct dj_option_result d_option_env_put_n_from(
                            struct dj_option_env* _env,
                            const char*           _statement,
                            size_t                _size,
                            int32_t               _source);

// where a script's statement that starts at _at ends: at ';', a line break
// or '#' outside double quotes, or at the script's NUL -- how
// d_option_env_load splits a script, for a caller that routes statements
// to several frames
size_t      d_option_env_statement_end(const char* _script,
                                       size_t      _at);

/**
 * @brief Carries out a script: statements as d_option_env_put reads them,
 *        separated by ';' or line breaks, with '#' starting a comment that
 *        runs to the end of its line.
 *
 * @note An unquoted value ends at ';', a line break or '#'. The script stops
 *       at its first failure; the statements before it stay done.
 *
 * @param[in,out] _env           the frame.
 * @param[in]     _script        the script, NUL-terminated.
 * @param[in]     _source        the source: FILE for a file's text.
 * @param[out]    _error_offset  where the failing statement starts; may be
 *                               null.
 * @return the number of statements done; or the first failure.
 */
struct dj_option_result d_option_env_load_from(
                            struct dj_option_env* _env,
                            const char*           _script,
                            int32_t               _source,
                            size_t*               _error_offset);

// d_option_env_load_from, from the program
struct dj_option_result d_option_env_load(struct dj_option_env* _env,
                                          const char*           _script,
                                          size_t*               _error_offset);

/**
 * @brief Binds variables from the process's environment: each variable the
 *        frame can see whose name, upper-cased after a prefix, is set there
 *        -- "DJINTERP_ENV_PRINTER_KEY_WIDTH" for "key_width" -- is assigned
 *        its text, globally, from the environment; an alias is tried when the
 *        name is not set.
 *
 * @note Reads getenv, which is not safe against a thread calling setenv. A
 *       value a variable does not accept is skipped and reported; the rest
 *       are still read.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _prefix  the prefix; null or empty for none.
 * @return the number bound; or the first failure.
 */
struct dj_option_result d_option_env_import(struct dj_option_env* _env,
                                            const char*           _prefix);

// 3.5    Writing as text
//------------------------------------------------------------------------------
/**
 * @brief Writes the value in force as text d_option_env_assign reads back:
 *        "true", "-12", "0.5", the text, a choice's name, flags' names joined
 *        by '|', and an int's name for a value that has one ("unbounded").
 *
 * @note Writes as much as fits, always NUL-terminated when _capacity is not
 *       0, as snprintf does.
 *
 * @param[in]  _env       the frame.
 * @param[in]  _scope     the scope.
 * @param[in]  _var       the variable.
 * @param[out] _buffer    where; may be null when _capacity is 0.
 * @param[in]  _capacity  its size.
 * @return the length of the whole text, without its NUL, or (size_t)-1 for a
 *         variable that does not exist.
 */
size_t d_option_env_format(const struct dj_option_env* _env,
                           uint32_t                    _scope,
                           uint32_t                    _var,
                           char*                       _buffer,
                           size_t                      _capacity);

/**
 * @brief Writes the value in force as d_option_env_format does, but a TEXT
 *        value in double quotes, with d_option_env_load's escapes, where it
 *        must be: empty, with space at either end, or holding ';', '#', a
 *        line break, a quote, a backslash or another control character. A
 *        statement of the name, '=' and this text loads back the same value,
 *        for every kind: what a settings file, or a "get" a user pastes into
 *        one, is written with.
 *
 * @note A null TEXT writes nothing, as d_option_env_format does: no
 *       statement sets a TEXT to null, and "-name" unbinds instead.
 *
 * @param[in]  _env       the frame.
 * @param[in]  _scope     the scope.
 * @param[in]  _var       the variable.
 * @param[out] _buffer    where; may be null when _capacity is 0.
 * @param[in]  _capacity  its size.
 * @return as d_option_env_format.
 */
size_t d_option_env_format_quoted(const struct dj_option_env* _env,
                                  uint32_t                    _scope,
                                  uint32_t                    _var,
                                  char*                       _buffer,
                                  size_t                      _capacity);

// 3.6    Reading on a hot path
//------------------------------------------------------------------------------
//   The knob-and-option pattern's last part. A module that reads a variable
// on every call keeps a cache beside it, and d_option_env_cached reads the
// frame again only when its generation moved: one call and a walk of the
// chain's counters, then the cached value.

// 3.6.1
// dj_option_env_cache
//   struct: one variable's value, and the generation it was read at.
struct dj_option_env_cache
{
    struct dj_option_env_value value;       // the value read
    uint32_t                   generation;  // the chain's, when read
    int32_t                    filled;      // 0 until the first read
};

// 3.6.2
// D_OPTION_ENV_CACHE_INIT
//   macro: an empty cache, to be filled by the first read.
#define D_OPTION_ENV_CACHE_INIT                                                \
    { { NULL, 0, 0.0, 0, 0, 0, 0u }, 0u, 0 }

/**
 * @brief The value in force, read again only when the chain's generation
 *        moved since the cache was filled.
 *
 * @param[in]     _env    the frame.
 * @param[in]     _scope  the scope.
 * @param[in]     _var    the variable.
 * @param[in,out] _cache  the cache; one per (frame, scope, variable).
 * @return the cached value; a variable that does not exist reads as kind 0.
 */
const struct dj_option_env_value*
       d_option_env_cached(const struct dj_option_env* _env,
                           uint32_t                    _scope,
                           uint32_t                    _var,
                           struct dj_option_env_cache* _cache);


D_EXTERN_C_END


#endif  // RE_STD_HAS_INT64

#endif  // DJINTERP_C_OPTION_ENV_H
