/*******************************************************************************
* djinterp [c]                                                   desc_machines.h
*
*   desc's own machines (desc.h's struct dj_desc_machine), and what describe
* writes with. A machine turns facts into output; these three are desc's:
*     TEXT        for people: a report, a line per fact, the label padded and
*                 a level per indent; or a line, key=value pairs on one line,
*                 a nested key dotted -- what a log line carries.
*     JSON        for machines: a level opened is "key": {, every key and
*                 value written by the stream's one JSON escaper
*                 (d_ostream_write_value, !json), with 64-bit integers and
*                 non-finite reals as numbers or strings, by a flag.
*     COLLECTING  for later: facts kept in a caller's array, in order, then
*                 given to any machine once (d_desc_replay). A log record's
*                 facts are collected so; past the array's room they are
*                 counted, not kept.
* Each machine's state is the caller's, usually on the stack, and nothing
* here allocates.
*
*   DESCRIBE, AS THE OPTIONS SAY. desc's djin chooses, per run and per type
* declared to desc, what d_desc_write does with a type: its style (prose from
* the units, or facts), its form (a fact describer or a text describer, when
* there are both), its format (text or JSON) and the text machine's preset
* (report or line). Each option's default is its knob (cfg_desc.h), and a
* choice the knobs did not build is refused by name.
*
*
* path:      /inc/djinterp/c/desc/desc_machines.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CHOICES
    -------
    1.  D_DESC_STYLE_*
    2.  D_DESC_FORM_*
    3.  D_DESC_FORMAT_*
    4.  D_DESC_TEXT_*
    5.  D_DESC_CHOICE_OPTION
    6.  D_DESC_MACHINE_BUILT
2.  THE TEXT MACHINE
    ----------------
    1.  D_DESC_TEXT_SHOW_*
    2.  D_DESC_TEXT_PATH_MAX
    3.  dj_desc_text_state
    4.  The machine
3.  THE JSON MACHINE
    ----------------
    1.  D_DESC_JSON_*
    2.  dj_desc_json_state
    3.  The machine
4.  THE COLLECTING MACHINE
    ----------------------
    1.  D_DESC_ITEM_*
    2.  dj_desc_item
    3.  dj_desc_collect_state
    4.  The machine, and replaying
5.  DESCRIBING TO A STREAM
    ----------------------
    1.  dj_desc_output
    2.  Puts and outputs
    3.  The options
    4.  Describing
*/

#ifndef DJINTERP_C_DESC_MACHINES_H
#define DJINTERP_C_DESC_MACHINES_H 1

// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "./desc.h"                            // machines, sinks
#include "./desc_units.h"                      // types, d_desc_say
#include "../util/ostream/ostream.h"           // struct dj_ostream
#include "../../config/c/desc/cfg_desc.h"      // D_CFG_DESC_*
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // uint32_t, uint64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CHOICES
//==============================================================================

// 1.1
// D_DESC_STYLE_*
//   constants: which engine describes a type: prose from its units, or its
// facts, written by a machine (desc.style).
#define D_DESC_STYLE_UNITS  0u
#define D_DESC_STYLE_FACTS  1u

// 1.2
// D_DESC_FORM_*
//   constants: which describer runs when a thing has both (desc.form).
#define D_DESC_FORM_FACTS   0u
#define D_DESC_FORM_TEXT    1u

// 1.3
// D_DESC_FORMAT_*
//   constants: which machine facts are written by (desc.format); also the
// bits of D_CFG_DESC_MACHINES, 1 << format.
#define D_DESC_FORMAT_TEXT     0u
#define D_DESC_FORMAT_JSON     1u
#define D_DESC_FORMAT_COLLECT  2u  // a machine, but no format: not an option

// 1.4
// D_DESC_TEXT_*
//   constants: the text machine's presets (desc.text).
#define D_DESC_TEXT_REPORT  0u
#define D_DESC_TEXT_LINE    1u

// 1.5
// D_DESC_CHOICE_OPTION
//   constant: an argument that means "what the option says".
#define D_DESC_CHOICE_OPTION  0xFEu

// 1.6
// D_DESC_MACHINE_BUILT
//   macro: whether this build has one of desc's machines, by its
// D_DESC_FORMAT_* number.
#define D_DESC_MACHINE_BUILT(format)                                           \
    ( (D_CFG_DESC_ENABLED) &&                                                  \
      ((D_CFG_DESC_MACHINES >> (format)) & 1) )


//==============================================================================
// 2.  THE TEXT MACHINE
//==============================================================================

// 2.1
// D_DESC_TEXT_SHOW_*
//   constants: the text machine's switches, as the env printer's show.
#define D_DESC_TEXT_SHOW_KEYS   0x01u  // keys, not labels
#define D_DESC_TEXT_SHOW_TRUTH  0x02u  // the kind of truth after a value
#define D_DESC_TEXT_SHOW_LAYER  0x04u  // the layer before a fact (report)
#define D_DESC_TEXT_NEWLINE     0x08u  // line: end the line at the end

// 2.2
// D_DESC_TEXT_PATH_MAX
//   constant: the levels whose keys a line's dotted keys carry; a deeper
// level's facts carry the first sixteen.
#define D_DESC_TEXT_PATH_MAX  16u

// 2.3
// dj_desc_text_state
//   struct: the text machine's state: where it writes, how, and where it is.
// Set up by d_desc_text_init.
struct dj_desc_text_state
{
    struct dj_ostream* os;                          // where
    uint32_t           preset;                      // D_DESC_TEXT_*
    uint32_t           flags;                       // D_DESC_TEXT_SHOW_*
    uint32_t           key_width;                   // report: the label's
                                                    // column; 0 for "label:"
    uint32_t           depth;                       // levels open
    uint32_t           written;                     // pairs on the line
    uint32_t           open;                        // 1 inside a line or a
                                                    // text value
    const char*        path[D_DESC_TEXT_PATH_MAX];  // line: the levels' keys
};

// 2.4    The machine
//------------------------------------------------------------------------------
/**
 * @brief Sets up a text machine's state.
 *
 * @param[out] _state   the state.
 * @param[in]  _os      where it writes; borrowed.
 * @param[in]  _preset  D_DESC_TEXT_REPORT or D_DESC_TEXT_LINE.
 * @param[in]  _flags   D_DESC_TEXT_SHOW_* and D_DESC_TEXT_NEWLINE, or 0.
 */
void d_desc_text_init(struct dj_desc_text_state* _state,
                      struct dj_ostream*         _os,
                      uint32_t                   _preset,
                      uint32_t                   _flags);

// the text machine's operations: a sink's machine, its context the state;
// null when D_CFG_DESC_MACHINES leaves it out
const struct dj_desc_machine* d_desc_text_machine(void);


//==============================================================================
// 3.  THE JSON MACHINE
//==============================================================================

// 3.1
// D_DESC_JSON_*
//   constants: the JSON machine's switches.
#define D_DESC_JSON_PRETTY             0x01u  // a member a line, indented
#define D_DESC_JSON_INTEGER_STRINGS    0x02u  // integers as "12": exact for
                                              // every reader (OTLP's rule)
#define D_DESC_JSON_NONFINITE_STRINGS  0x04u  // "NaN", "Infinity",
                                              // "-Infinity"; else null
#define D_DESC_JSON_NEWLINE            0x08u  // a line break after the end
#define D_DESC_JSON_BARE               0x10u  // members only: the caller
                                              // writes the braces around

// 3.2
// dj_desc_json_state
//   struct: the JSON machine's state. Set up by d_desc_json_init.
struct dj_desc_json_state
{
    struct dj_ostream* os;      // where
    uint32_t           flags;   // D_DESC_JSON_*
    uint32_t           depth;   // objects open, the outermost's excluded
    uint64_t           empty;   // bit per level: 1 while nothing is in it
    uint32_t           text;    // 1 inside a text value
    uint32_t           reserved;
};

// 3.3    The machine
//------------------------------------------------------------------------------
/**
 * @brief Sets up a JSON machine's state.
 *
 * @param[out] _state  the state.
 * @param[in]  _os     where it writes; borrowed.
 * @param[in]  _flags  D_DESC_JSON_*, or 0: compact, numbers as numbers.
 */
void d_desc_json_init(struct dj_desc_json_state* _state,
                      struct dj_ostream*         _os,
                      uint32_t                   _flags);

// the JSON machine's operations; null when D_CFG_DESC_MACHINES leaves it out
const struct dj_desc_machine* d_desc_json_machine(void);


//==============================================================================
// 4.  THE COLLECTING MACHINE
//==============================================================================

// 4.1
// D_DESC_ITEM_*
//   constants: what a collected item is.
#define D_DESC_ITEM_FACT   0u  // a fact
#define D_DESC_ITEM_ENTER  1u  // a level opened: its key, label and depth

// 4.2
// dj_desc_item
//   struct: one collected fact, or a level opened, whose key and label are
// the fact's and whose value is none. Every string stays borrowed, as the
// fact's were: replay before the describer's data goes.
struct dj_desc_item
{
    struct dj_desc_fact fact;
    uint32_t            kind;  // D_DESC_ITEM_*
};

// 4.3
// dj_desc_collect_state
//   struct: the collecting machine's state: the caller's array, and what
// did not fit.
struct dj_desc_collect_state
{
    struct dj_desc_item* items;     // the caller's array
    size_t               capacity;  // its room
    size_t               count;     // items kept
    size_t               dropped;   // facts that did not fit
};

// 4.4    The machine, and replaying
//------------------------------------------------------------------------------
/**
 * @brief Sets up a collecting machine's state over a caller's array.
 *
 * @param[out] _state     the state.
 * @param[in]  _items     the array; may be null when _capacity is 0.
 * @param[in]  _capacity  its room.
 */
void d_desc_collect_init(struct dj_desc_collect_state* _state,
                         struct dj_desc_item*          _items,
                         size_t                        _capacity);

// the collecting machine's operations; null when D_CFG_DESC_MACHINES leaves
// it out
const struct dj_desc_machine* d_desc_collect_machine(void);

/**
 * @brief Gives what was collected to a machine, in order: each fact, each
 *        level opened, and each level closed where the depth falls back. No
 *        begin and no end: the caller writes around it.
 *
 * @param[in]     _state    the collected items.
 * @param[in]     _machine  the machine.
 * @param[in,out] _context  its own.
 * @return 0 when all was given; 1 when the machine asked to stop.
 */
int d_desc_replay(const struct dj_desc_collect_state* _state,
                  const struct dj_desc_machine*       _machine,
                  void*                               _context);


//==============================================================================
// 5.  DESCRIBING TO A STREAM
//==============================================================================

// 5.1
// dj_desc_output
//   struct: a sink and the state of the machine it writes to, set up for a
// stream by d_desc_output_init.
struct dj_desc_output
{
    struct dj_desc_sink sink;    // what a describer is given
    uint32_t            format;  // D_DESC_FORMAT_TEXT or _JSON
    uint32_t            reserved;
    union
    {
        struct dj_desc_text_state text;
        struct dj_desc_json_state json;
    } as;
};

// 5.2    Puts and outputs
//------------------------------------------------------------------------------
/**
 * @brief A put onto a struct dj_ostream, its context: text describers and
 *        d_desc_say write to a stream through it.
 *
 * @return 0 while the stream is well; 1 once it has failed.
 */
int d_desc_put_ostream(void*       _context,
                       const char* _text,
                       size_t      _size);

/**
 * @brief Sets up a sink writing to a stream, its machine and layers chosen
 *        as desc's options say for a type.
 *
 * @param[out] _output  the output.
 * @param[in]  _os      the stream; borrowed.
 * @param[in]  _type    the type, whose options apply; null for desc's own.
 * @param[in]  _format  D_DESC_FORMAT_TEXT or _JSON, or D_DESC_CHOICE_OPTION
 *                      for desc.format.
 * @param[in]  _flags   the machine's switches: D_DESC_TEXT_* for text, with
 *                      desc.text's preset; D_DESC_JSON_* for JSON.
 * @return 0; -1 for a null argument or a machine not built.
 */
int d_desc_output_init(struct dj_desc_output*     _output,
                       struct dj_ostream*         _os,
                       const struct dj_desc_type* _type,
                       uint32_t                   _format,
                       uint32_t                   _flags);

// 5.3    The options
//------------------------------------------------------------------------------
//   Each is desc.<type>.<name> when the type was declared and it is set, else
// desc.<name>, else its knob. A null type reads desc.<name> alone.

// desc.style: D_DESC_STYLE_*
uint32_t d_desc_option_style(const struct dj_desc_type* _type);
// desc.form: D_DESC_FORM_*
uint32_t d_desc_option_form(const struct dj_desc_type* _type);
// desc.format: D_DESC_FORMAT_TEXT or _JSON
uint32_t d_desc_option_format(const struct dj_desc_type* _type);
// desc.text: D_DESC_TEXT_*
uint32_t d_desc_option_text(const struct dj_desc_type* _type);

// 5.4    Describing
//------------------------------------------------------------------------------
/**
 * @brief Describes an object to a stream as the options say for its type:
 *        in prose from its units, at desc.ring, or as facts written by
 *        desc.format's machine.
 *
 * @note Prose needs the type's units, and facts its field list or its
 *       contents; when the style chosen has nothing to say, the other is
 *       written.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _type    the type's description.
 * @param[in]     _object  the object.
 * @param[in]     _style   D_DESC_STYLE_*, or D_DESC_CHOICE_OPTION.
 * @return 0 when written; 1 when the stream failed; -1 for a null argument.
 */
int d_desc_write(struct dj_ostream*         _os,
                 const struct dj_desc_type* _type,
                 const void*                _object,
                 uint32_t                   _style);

/**
 * @brief Describes an object to a stream with a fact describer or a text
 *        describer, whichever desc.form chooses, through desc.format's
 *        machine: what a djin's describe slot is written with.
 *
 * @note A null describer is never chosen: the other runs.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _scope   the type whose options apply; may be null.
 * @param[in]     _facts   the fact describer; may be null.
 * @param[in]     _text    the text describer; may be null.
 * @param[in]     _object  the object.
 * @return as d_desc_write.
 */
int d_desc_write_with(struct dj_ostream*         _os,
                      const struct dj_desc_type* _scope,
                      fn_desc_describer          _facts,
                      fn_desc_text_describer     _text,
                      const void*                _object);


D_EXTERN_C_END


#endif  // DJINTERP_C_DESC_MACHINES_H
