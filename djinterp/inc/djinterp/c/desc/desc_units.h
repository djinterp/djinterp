/*******************************************************************************
* djinterp [c]                                                      desc_units.h
*
*   desc's light default: units of fill-in-the-blank text, chosen when a type
* is compiled and by a plan read from the object's values at run time, then
* written once, front to back. Nothing written is ever rewritten, and nothing
* allocates.
*
*   A TYPE IS AUTHORED ONCE. A type gives desc a field list -- each value's
* key, label, layer, importance and truth -- one reader for those fields, a
* plan, and units that name the fields. Prose comes from the units
* (d_desc_say); facts come from the field list (d_desc_facts), through
* desc.h's sink to any machine. Both read the same reader, so they cannot
* disagree, and a field nobody asked for is never read.
*
*   A DESCRIPTION IS CONCENTRIC. A ring says how much is said, each including
* the ones inside it: a name, a line, a sentence, the detail. A layer says
* what a unit is about (desc.h's layers), and a reader hides the layers it
* does not want; the noun, which names the type, is always written. A unit
* goes in a slot, and the slots are in the order a person says the parts:
* the adjectives by kind, the noun, what completes it, what the object has,
* then remarks, each a sentence.
*
*   Knobs fix what a build has (cfg_desc.h); desc's djin, "desc", lets a run
* choose among it: desc.ring, desc.layers, and desc.<type>.ring for a type
* declared to desc (desc_options.h).
*
*
* path:      /inc/djinterp/c/desc/desc_units.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RINGS, SLOTS AND ARTICLES
    -------------------------
    1.  D_DESC_RING_*
    2.  D_DESC_BUILT
    3.  D_DESC_SLOT_*
    4.  D_DESC_ARTICLE_*
2.  A TYPE'S TABLES
    ---------------
    1.  dj_desc_op
    2.  D_DESC_T, D_DESC_F, D_DESC_PL, D_DESC_WORDS, D_DESC_END
    3.  dj_desc_unit
    4.  D_DESC_UNIT, D_DESC_UNIT_ARTICLE
    5.  dj_desc_field
    6.  D_DESC_FIELD
    7.  fn_desc_plan
    8.  fn_desc_read
    9.  dj_desc_type
    10. D_DESC_OF, D_DESC_UNITS_TABLE, D_DESC_FIELDS_TABLE
3.  DESCRIBING
    ----------
    1.  D_DESC_CAPITAL, D_DESC_LAYERS_OPTION
    2.  d_desc_say
    3.  dj_desc_buffer
    4.  d_desc_buffer_init
    5.  d_desc_put_buffer
    6.  d_desc_put_file
    7.  d_desc_facts
    8.  d_desc_run_facts
4.  OPTIONS
    -------
    1.  d_desc_declare
    2.  d_desc_option_ring
    3.  d_desc_option_layers
*/

#ifndef DJINTERP_C_DESC_UNITS_H
#define DJINTERP_C_DESC_UNITS_H 1

// std
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // FILE
#include <string.h>  // memcpy
// djinterp
#include "./desc.h"                                // facts, sinks, puts
#include "../../config/c/desc/cfg_desc.h"          // D_CFG_DESC_*


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  RINGS, SLOTS AND ARTICLES
//==============================================================================

// 1.1
// D_DESC_RING_*
//   constants: how much is said. Each ring includes the ones inside it.
#define D_DESC_RING_NAME      0u     // "file tree"
#define D_DESC_RING_LINE      1u     // "a file tree with 6 nodes"
#define D_DESC_RING_SENTENCE  2u     // its policies, and a sentence more
#define D_DESC_RING_DETAILED  3u     // what costs a walk to know
#define D_DESC_RING_COUNT     4u
#define D_DESC_RING_OPTION    0xFEu  // d_desc_say: what desc.ring says
#define D_DESC_RING_NEVER     0xFFu  // a unit no ring shows

// 1.2
// D_DESC_BUILT
//   macro: whether this build has the units of a ring and a layer, each
// named without its prefix -- D_DESC_BUILT(LINE, STATE) -- for #if around a
// unit table's row. desc.h's layers are enum constants, which #if cannot
// read, so the names map to numbers here; a misspelt name is an undefined
// macro, which -Wundef reports.
#define D_DESC_BUILT(ring, layer)                                              \
    D_INTERNAL_DESC_BUILT(D_INTERNAL_DESC_RING_##ring,                         \
                          D_INTERNAL_DESC_LAYER_##layer)
#define D_INTERNAL_DESC_BUILT(ring, layer)                                     \
    ( (D_CFG_DESC_ENABLED != 0) && (D_CFG_DESC_UNITS != 0)        &&           \
      ((ring) <= D_CFG_DESC_RING_MAX)                             &&           \
      ((((D_CFG_DESC_LAYERS_BUILT) >> (layer)) & 1) != 0) )
#define D_INTERNAL_DESC_RING_NAME         0
#define D_INTERNAL_DESC_RING_LINE         1
#define D_INTERNAL_DESC_RING_SENTENCE     2
#define D_INTERNAL_DESC_RING_DETAILED     3
#define D_INTERNAL_DESC_LAYER_IDENTITY    0
#define D_INTERNAL_DESC_LAYER_TYPE        1
#define D_INTERNAL_DESC_LAYER_STATE       2
#define D_INTERNAL_DESC_LAYER_INTENT      3
#define D_INTERNAL_DESC_LAYER_POLICY      4
#define D_INTERNAL_DESC_LAYER_HEALTH      5
#define D_INTERNAL_DESC_LAYER_PROVENANCE  6
#define D_INTERNAL_DESC_LAYER_CONTENTS    7
#define D_INTERNAL_DESC_LAYER_COST        8

// 1.3
// D_DESC_SLOT_*
//   constants: where a unit goes, in the order a person says the parts.
// Adjectives of one kind take commas between them; of different kinds, a
// space. A row whose slot is NONE shows nowhere.
#define D_DESC_SLOT_NONE        0u
#define D_DESC_SLOT_CONDITION   1u   // how it is now: "empty", "full"
#define D_DESC_SLOT_SIZE        2u   // "fixed-size"
#define D_DESC_SLOT_QUALITY     3u   // "immutable"
#define D_DESC_SLOT_SHAPE       4u   // "ordered"
#define D_DESC_SLOT_ORIGIN      5u   // "Windows-style"
#define D_DESC_SLOT_NOUN        6u   // "file tree": always written
#define D_DESC_SLOT_COMPLEMENT  7u   // "mapping uint32_t to text"
#define D_DESC_SLOT_WITH        8u   // "with 6 nodes and a custom allocator"
#define D_DESC_SLOT_REMARK      9u   // "Its root is src."

// 1.4
// D_DESC_ARTICLE_*
//   constants: a unit's own article, when its first letter or number would
// choose wrongly ("a unique", "an hour").
#define D_DESC_ARTICLE_AUTO   0u
#define D_DESC_ARTICLE_A      1u
#define D_DESC_ARTICLE_AN     2u


//==============================================================================
// 2.  A TYPE'S TABLES
//==============================================================================

// 2.1
// dj_desc_op
//   struct: one step of a unit's text, as the compiler lays it out. A frame's
// blanks are fields, read by their index in the type's field list; a
// misspelt field name does not compile.
#define D_DESC_OP_END         0u
#define D_DESC_OP_TEXT        1u     // a literal
#define D_DESC_OP_FIELD       2u     // a field's value, from the reader
#define D_DESC_OP_PLURAL      3u     // "s", unless the last number was 1
#define D_DESC_OP_WORDS       4u     // the first text after a 1, else the 2nd

struct dj_desc_op
{
    uint8_t     kind;   // D_DESC_OP_*
    uint8_t     field;  // D_DESC_OP_FIELD: its index, 0 to 255
    const char* text;   // TEXT, and WORDS's singular
    const char* other;  // WORDS's plural
};

// 2.2
// D_DESC_T, D_DESC_F, D_DESC_PL, D_DESC_WORDS, D_DESC_END
//   macros: an op array's entries. An array ends with D_DESC_END, and units
// that say the same thing share one.
#define D_DESC_T(text_)                                                        \
    { (uint8_t)D_DESC_OP_TEXT, 0u, (text_), NULL }
#define D_DESC_F(field_)                                                       \
    { (uint8_t)D_DESC_OP_FIELD, (uint8_t)(field_), NULL, NULL }
#define D_DESC_PL                                                              \
    { (uint8_t)D_DESC_OP_PLURAL, 0u, NULL, NULL }
#define D_DESC_WORDS(one_, many_)                                              \
    { (uint8_t)D_DESC_OP_WORDS, 0u, (one_), (many_) }
#define D_DESC_END                                                             \
    { (uint8_t)D_DESC_OP_END, 0u, NULL, NULL }

// 2.3
// dj_desc_unit
//   struct: one unit: where it goes, when it shows, and its text. It shows
// when its ring is in force, its layer is wanted (the noun's always is), and
// the plan has every bit of `on` and none of `off`. 24 bytes on x86-64.
struct dj_desc_unit
{
    const struct dj_desc_op* ops;      // shared; ends with D_DESC_END
    uint32_t                 on;       // plan bits it needs
    uint32_t                 off;      // plan bits it must not have
    uint8_t                  ring;     // D_DESC_RING_*
    uint8_t                  layer;    // enum dj_desc_layer
    uint8_t                  slot;     // D_DESC_SLOT_*
    uint8_t                  article;  // D_DESC_ARTICLE_*
};

// 2.4
// D_DESC_UNIT, D_DESC_UNIT_ARTICLE
//   macros: a unit table's row, its article chosen by its first word, or
// given.
#define D_DESC_UNIT(ring, layer, slot, on, off, ops)                           \
    D_DESC_UNIT_ARTICLE(ring, layer, slot, on, off, ops, D_DESC_ARTICLE_AUTO)
#define D_DESC_UNIT_ARTICLE(ring, layer, slot, on, off, ops, article)          \
    { (ops), (uint32_t)(on), (uint32_t)(off), (uint8_t)(ring),                 \
      (uint8_t)(layer), (uint8_t)(slot), (uint8_t)(article) }

// 2.5
// dj_desc_field
//   struct: one field: what a fact of it carries besides its value. Its
// index in the list is what a unit's D_DESC_F names and the reader reads.
struct dj_desc_field
{
    const char* key;         // the fact's key: "node_count"
    const char* label;       // prose's: "nodes"; may be null
    uint8_t     layer;       // enum dj_desc_layer
    uint8_t     importance;  // 0 the headline .. 3 the detail
    uint8_t     truth;       // enum dj_desc_truth
    uint8_t     reserved;    // 0
};

// 2.6
// D_DESC_FIELD
//   macro: a field list's row.
#define D_DESC_FIELD(key, label, layer, importance, truth)                     \
    { (key), (label), (uint8_t)(layer), (uint8_t)(importance),                 \
      (uint8_t)(truth), 0u }

// 2.7
// fn_desc_plan
//   function pointer: the plan. It reads the values that choose units and
// grammar, for the ring in force, before anything is written, and returns
// bits the units' `on` and `off` test.
typedef uint32_t (*fn_desc_plan)(const void* _object,
                                 uint32_t    _ring);

// 2.8
// fn_desc_read
//   function pointer: reads one field, by its index, when a unit being
// written or a fact being emitted reaches it, and only then.
typedef struct dj_ostream_value (*fn_desc_read)(const void* _object,
                                                uint32_t    _field);

// 2.9
// dj_desc_type
//   struct: what desc knows of a type. Usually a const object beside the
// type's own unit, named d_desc_type_<name> for D_DESC_OF.
struct dj_desc_type
{
    const char*                 name;         // an identifier: the scope of
                                              // its options, desc.<name>.ring
    const char*                 noun;         // its name in prose: the "type"
                                              // fact's value
    const struct dj_desc_unit*  units;        // null when none are built
    size_t                      unit_count;
    const struct dj_desc_field* fields;       // null when facts are not built
    size_t                      field_count;
    fn_desc_plan                plan;         // may be null: no bits
    fn_desc_read                read;         // may be null: no fields
    fn_desc_describer           contents;     // nested contents, written by
                                              // hand; may be null
};

// 2.10
// D_DESC_OF, D_DESC_UNITS_TABLE, D_DESC_FIELDS_TABLE
//   macros: a described type's dj_desc_type; and a table's pointer and
// count for a dj_desc_type's initialiser, or null and 0 when the knobs leave
// that engine out, so the table may sit inside #if D_CFG_DESC_UNITS.
#define D_DESC_OF(name) (&d_desc_type_##name)

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
    #define D_DESC_UNITS_TABLE(table)                                          \
        (table), (sizeof(table) / sizeof((table)[0]))
#else
    #define D_DESC_UNITS_TABLE(table)  NULL, 0u
#endif

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
    #define D_DESC_FIELDS_TABLE(table)                                         \
        (table), (sizeof(table) / sizeof((table)[0]))
#else
    #define D_DESC_FIELDS_TABLE(table)  NULL, 0u
#endif


//==============================================================================
// 3.  DESCRIBING
//==============================================================================

// 3.1
// D_DESC_CAPITAL, D_DESC_LAYERS_OPTION
//   constants: d_desc_say's flag to begin with a capital letter; and its
// layers argument to take desc.layers.
#define D_DESC_CAPITAL        1u
#define D_DESC_LAYERS_OPTION  0xFFFFFFFFu

/**
 * @brief Describes an object in prose: decides from its values, then writes
 *        once, front to back, in a sentence's order.
 *
 * @param[in] _type     the type's description.
 * @param[in] _object   the object.
 * @param[in] _ring     D_DESC_RING_*; D_DESC_RING_OPTION for desc.ring, or
 *                      desc.<type>.ring when the type was declared. A ring
 *                      past D_CFG_DESC_RING_MAX is the last one built.
 * @param[in] _layers   the layers wanted, bits of enum dj_desc_layer; or
 *                      D_DESC_LAYERS_OPTION for desc.layers. Layers not built
 *                      are never shown.
 * @param[in] _flags    D_DESC_CAPITAL, or 0.
 * @param[in] _put      where the text goes; it is only ever appended to.
 * @param[in] _context  the put's.
 * @return 0 once written; 1 when the put asked to stop; -1 for a null type,
 *         object or put, or a build without units.
 */
int d_desc_say(const struct dj_desc_type* _type,
               const void*                _object,
               uint32_t                   _ring,
               uint32_t                   _layers,
               uint32_t                   _flags,
               fn_desc_put                _put,
               void*                      _context);

// 3.3
// dj_desc_buffer
//   struct: a caller's buffer as a put's context: text goes in as far as it
// fits, always followed by a NUL, and `truncated` says whether all of it did.
struct dj_desc_buffer
{
    char*    data;
    size_t   capacity;   // bytes, the NUL's included
    size_t   size;       // bytes written, the NUL's excluded
    uint32_t truncated;  // 1 once text did not fit
};

/**
 * @brief Makes a buffer empty, over the caller's storage.
 *
 * @param[out] _buffer    the buffer.
 * @param[in]  _data      the storage; may be null when _capacity is 0.
 * @param[in]  _capacity  its size.
 */
D_INLINE void
d_desc_buffer_init(
    struct dj_desc_buffer* _buffer,
    char*                  _data,
    size_t                 _capacity
)
{
    _buffer->data      = _data;
    _buffer->capacity  = _capacity;
    _buffer->size      = 0u;
    _buffer->truncated = 0u;

    if (_capacity != 0u)
    {
        _data[0] = '\0';
    }

    return;
}

/**
 * @brief A put into a struct dj_desc_buffer, its context.
 *
 * @return 0 when the text fit; 1 when it did not, so the writer stops.
 */
D_INLINE int
d_desc_put_buffer(
    void*       _context,
    const char* _text,
    size_t      _size
)
{
    struct dj_desc_buffer* buffer = (struct dj_desc_buffer*)_context;
    size_t                 room   = 0u;

    if (buffer->capacity > buffer->size + 1u)
    {
        room = buffer->capacity - buffer->size - 1u;
    }

    if (_size > room)
    {
        _size             = room;
        buffer->truncated = 1u;
    }

    if (_size != 0u)
    {
        memcpy(buffer->data + buffer->size, _text, _size);
        buffer->size += _size;
    }

    if (buffer->capacity != 0u)
    {
        buffer->data[buffer->size] = '\0';
    }

    return (buffer->truncated != 0u) ? 1 : 0;
}

/**
 * @brief A put into a FILE*, its context.
 *
 * @return 0 when written; 1 on a write error.
 */
int d_desc_put_file(void*       _context,
                    const char* _text,
                    size_t      _size);

/**
 * @brief The facts a type's field list derives, into a sink: first "type",
 *        the type's noun, then each field the sink wants, read only then,
 *        then the type's nested contents, if it describes them.
 *
 * @note A fact describer for one type: to give it a fn_desc_describer, wrap
 *       it in a function that names the type.
 *
 * @param[in]     _type    the type's description.
 * @param[in]     _object  the object.
 * @param[in,out] _sink    the sink.
 * @return 0 to go on; 1 once the sink has stopped; -1 for a null argument,
 *         or a build without facts.
 */
int d_desc_facts(const struct dj_desc_type* _type,
                 const void*                _object,
                 struct dj_desc_sink*       _sink);

/**
 * @brief Describes an object's facts to the sink's machine: its begin, the
 *        facts, then its end, as d_desc_run.
 *
 * @return as d_desc_facts.
 */
int d_desc_run_facts(struct dj_desc_sink*       _sink,
                     const struct dj_desc_type* _type,
                     const void*                _object);


//==============================================================================
// 4.  OPTIONS
//==============================================================================

/**
 * @brief Declares a type to desc, so desc.<name>.ring may be set for it: its
 *        name becomes a scope of desc's djin. Declare a type before reading
 *        the settings that name it.
 *
 * @param[in] _type  the type; its name must be an identifier.
 * @return 0 when declared, already declared, or options are not built; -1
 *         for a null type or name, or when D_CFG_DESC_SCOPES are taken.
 */
int d_desc_declare(const struct dj_desc_type* _type);

/**
 * @brief The ring a type is described at when nobody says: desc.<name>.ring
 *        when the type was declared and it is set, else desc.ring, else
 *        D_CFG_DESC_RING.
 *
 * @param[in] _type  the type, or null for desc.ring alone.
 * @return D_DESC_RING_*.
 */
uint32_t d_desc_option_ring(const struct dj_desc_type* _type);

/**
 * @brief The layers a description shows when nobody says: desc.layers, else
 *        D_CFG_DESC_LAYERS.
 *
 * @return bits of enum dj_desc_layer.
 */
uint32_t d_desc_option_layers(void);


D_EXTERN_C_END


#endif  // DJINTERP_C_DESC_UNITS_H
