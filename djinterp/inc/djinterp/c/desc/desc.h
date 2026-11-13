/*******************************************************************************
* djinterp [c]                                                            desc.h
*
*   Describing anything, facts first and text last. A describer never writes a
* sentence: it emits facts -- a key, a label for prose, a value, the layer the
* fact is on, how important it is, and what kind of truth it is -- into a
* sink, and the sink's machine turns them into text, JSON, markdown or
* key=value lines. One source then serves a log line, --help and a debugger's
* view (docs/djinterp/desc/desc_design.md).
*
*   This header is the core every describer and every machine shares: the
* fact, the machine, the sink and the two describer forms. It allocates
* nothing and needs no unit to link: a describer lives beside its type, and a
* program that never describes anything pays for nothing.
*
*   The work is a machine's (struct dj_desc_machine): a table of operations
* -- begin, fact, enter, leave, text, end -- each of which may be null. desc's
* own text, JSON and collecting machines are built on it, and a program
* writes any other. A sink filters before a fact is built: d_desc_wants lets
* a describer skip the work of a fact nobody asked for, by layer and
* importance; what the sink wants then goes to its machine.
*
*   A type describes itself in either form: a fact describer emits facts, a
* text describer writes words, and d_desc_run_text gives those words to any
* machine -- to its text operation, or as one fact. Nesting is the
* describer's, one level per call, as Common Lisp's describe-object has it:
* d_desc_enter names the level and d_desc_leave closes it, and the sink
* refuses past its limit, so a cycle ends.
*
*
* path:      /inc/djinterp/c/desc/desc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FACTS
    -----
    1.  dj_desc_layer
    2.  dj_desc_truth
    3.  dj_desc_fact
2.  MACHINES
    --------
    1.  fn_desc_begin
    2.  fn_desc_fact
    3.  fn_desc_enter
    4.  fn_desc_leave
    5.  fn_desc_text
    6.  fn_desc_end
    7.  dj_desc_machine
3.  SINKS AND DESCRIBERS
    --------------------
    1.  dj_desc_sink
    2.  fn_desc_describer
    3.  fn_desc_put
    4.  fn_desc_text_describer
    5.  d_desc_sink_init
    6.  d_desc_wants
    7.  d_desc_emit
    8.  d_desc_text
    9.  d_desc_enter
    10. d_desc_leave
    11. d_desc_run
    12. d_desc_run_text
*/

#ifndef DJINTERP_C_DESC_H
#define DJINTERP_C_DESC_H 1

// std
#include <stddef.h>  // NULL
// djinterp
#include "../djinterp.h"                // framework root
#include "../text/ostream_template.h"  // struct dj_ostream_value
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // uint8_t, uint16_t, uint32_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FACTS
//==============================================================================

// 1.1
// dj_desc_layer
//   enum: what a fact is about. A reader selects layers; a module adds keys to
// them, not layers of its own.
enum dj_desc_layer
{
    D_DESC_LAYER_IDENTITY   = 0,  // what it is called, and which one it is
    D_DESC_LAYER_TYPE       = 1,  // its type: "a tree map of text to int64_t"
    D_DESC_LAYER_STATE      = 2,  // how it is now: "42 elements"
    D_DESC_LAYER_INTENT     = 3,  // what it is for: the hand-written part
    D_DESC_LAYER_POLICY     = 4,  // its allocator, comparator, locking
    D_DESC_LAYER_HEALTH     = 5,  // whether it is well: errors, warnings
    D_DESC_LAYER_PROVENANCE = 6,  // where a value came from: a source, a knob
    D_DESC_LAYER_CONTENTS   = 7,  // what it holds, nested
    D_DESC_LAYER_COST       = 8,  // its size and its time
    D_DESC_LAYER_COUNT      = 9
};

// D_DESC_LAYERS_ALL
//   constant: the layers mask with every layer.
#define D_DESC_LAYERS_ALL                                                      \
    ( ((uint32_t)1u << (uint32_t)D_DESC_LAYER_COUNT) - 1u )

// 1.2
// dj_desc_truth
//   enum: what kind of truth a fact is, so a careful description can say so.
enum dj_desc_truth
{
    D_DESC_TRUTH_GUARANTEED     = 0,  // the standard requires it
    D_DESC_TRUTH_IMPLEMENTATION = 1,  // this library's or compiler's choice
    D_DESC_TRUTH_BUILD          = 2,  // fixed by configuration: a knob
    D_DESC_TRUTH_RUN_TIME       = 3,  // an option, with its source
    D_DESC_TRUTH_OBSERVED       = 4   // the state now
};

// 1.3
// dj_desc_fact
//   struct: one fact. The key is a path part ("element_count"); the label is
// prose's ("elements") and may be null. Every string is borrowed, for the
// emit's call only. Importance orders facts within a layer: 0 is the
// headline, and 3 the finest detail.
struct dj_desc_fact
{
    const char*             key;         // a path part
    const char*             label;       // for prose; may be null
    struct dj_ostream_value value;       // the stream's tagged value
    uint32_t                depth;       // nesting, from the sink
    uint16_t                layer;       // enum dj_desc_layer
    uint8_t                 importance;  // 0 the headline .. 3 the detail
    uint8_t                 truth;       // enum dj_desc_truth
};


//==============================================================================
// 2.  MACHINES
//==============================================================================
//   A machine is what a description is written to: a table of operations and
// the context they share. Every operation may be null, and is then skipped;
// each that returns int returns 0 to go on, and anything else to stop the
// sink. desc's own machines (text, JSON, collecting, a log record's) are
// built on this table, and a program writes any other on it.

// 2.1
// fn_desc_begin
//   function pointer: a description starts, before its first fact.
typedef int (*fn_desc_begin)(void* _context);

// 2.2
// fn_desc_fact
//   function pointer: one fact the sink wanted, at the sink's depth.
typedef int (*fn_desc_fact)(void*                      _context,
                            const struct dj_desc_fact* _fact);

// 2.3
// fn_desc_enter
//   function pointer: a level opens, named by a key and a label, at the depth
// of the facts that name it: a JSON machine writes "key": { here.
typedef int (*fn_desc_enter)(void*       _context,
                             const char* _key,
                             const char* _label,
                             uint32_t    _depth);

// 2.4
// fn_desc_leave
//   function pointer: the level opened last closes; _depth is the depth
// returned to.
typedef int (*fn_desc_leave)(void*    _context,
                             uint32_t _depth);

// 2.5
// fn_desc_text
//   function pointer: words from a text describer, as written. A machine
// without this operation receives them as a fact instead (d_desc_text).
typedef int (*fn_desc_text)(void*       _context,
                            const char* _text,
                            size_t      _size);

// 2.6
// fn_desc_end
//   function pointer: the description is over, stopped or not.
typedef void (*fn_desc_end)(void* _context);

// 2.7
// dj_desc_machine
//   struct: a machine's operations. Usually a static const table, shared by
// every sink that writes to that kind of machine.
struct dj_desc_machine
{
    fn_desc_begin begin;  // may be null
    fn_desc_fact  fact;   // may be null: facts are then dropped
    fn_desc_enter enter;  // may be null
    fn_desc_leave leave;  // may be null
    fn_desc_text  text;   // may be null: text then arrives as a fact
    fn_desc_end   end;    // may be null
};


//==============================================================================
// 3.  SINKS AND DESCRIBERS
//==============================================================================

// 3.1
// dj_desc_sink
//   struct: where a describer puts its facts, and what the reader wants. The
// caller owns it, usually on the stack; d_desc_sink_init sets it up to want
// everything, eight levels deep.
struct dj_desc_sink
{
    const struct dj_desc_machine* machine;     // the machine; null: none
    void*                         context;     // the machine's own
    uint32_t                      layers;      // wanted: bit 1 << layer
    uint32_t                      depth;       // the level being described
    uint32_t                      max_depth;   // levels d_desc_enter allows
    uint8_t                       importance;  // least important wanted, 0-3
    uint8_t                       stopped;     // 1 once the machine stopped
    uint16_t                      reserved;    // 0
};

// 3.2
// fn_desc_describer
//   function pointer: a type's fact describer, one level of it. It emits the
// object's facts into the sink, nests through d_desc_enter and d_desc_leave,
// and returns nonzero once the sink has stopped, so its caller stops too.
typedef int (*fn_desc_describer)(const void*          _object,
                                 struct dj_desc_sink* _sink);

// 3.3
// fn_desc_put
//   function pointer: where a text describer's words go. It returns 0 to go
// on, and anything else to stop.
typedef int (*fn_desc_put)(void*       _context,
                           const char* _text,
                           size_t      _size);

// 3.4
// fn_desc_text_describer
//   function pointer: a type's text describer: it writes words through put,
// and returns nonzero once put has asked to stop.
typedef int (*fn_desc_text_describer)(const void* _object,
                                      fn_desc_put _put,
                                      void*       _context);

/**
 * @brief Sets a sink up to want every layer and importance, eight levels
 *        deep, writing to a machine.
 *
 * @param[out] _sink     the sink.
 * @param[in]  _machine  the machine; null makes a sink that wants nothing.
 * @param[in]  _context  the machine's own.
 */
D_INLINE void
d_desc_sink_init(
    struct dj_desc_sink*          _sink,
    const struct dj_desc_machine* _machine,
    void*                         _context
)
{
    _sink->machine    = _machine;
    _sink->context    = _context;
    _sink->layers     = (_machine != NULL) ? D_DESC_LAYERS_ALL : 0u;
    _sink->depth      = 0u;
    _sink->max_depth  = 8u;
    _sink->importance = 3u;
    _sink->stopped    = 0u;
    _sink->reserved   = 0u;

    return;
}

/**
 * @brief Whether a sink wants a fact of a layer and an importance, so a
 *        describer can skip the work of one nobody asked for.
 *
 * @param[in] _sink        the sink.
 * @param[in] _layer       enum dj_desc_layer.
 * @param[in] _importance  0 the headline .. 3 the detail.
 * @return 1 or 0; 0 once the sink has stopped.
 */
D_INLINE int
d_desc_wants(
    const struct dj_desc_sink* _sink,
    uint32_t                   _layer,
    uint32_t                   _importance
)
{
    return ( (_sink->stopped == 0u)                                 &&
             (_layer < (uint32_t)D_DESC_LAYER_COUNT)                &&
             ((_sink->layers & ((uint32_t)1u << _layer)) != 0u)     &&
             (_importance <= (uint32_t)_sink->importance) ) ? 1 : 0;
}

/**
 * @brief Emits one fact, at the sink's depth, if the sink wants it.
 *
 * @param[in,out] _sink        the sink.
 * @param[in]     _key         the fact's key, borrowed.
 * @param[in]     _label       its label for prose, borrowed; may be null.
 * @param[in]     _value       its value; its text is borrowed.
 * @param[in]     _layer       enum dj_desc_layer.
 * @param[in]     _importance  0 the headline .. 3 the detail.
 * @param[in]     _truth       enum dj_desc_truth.
 * @return 0 to go on, a fact unwanted included; 1 once the sink has stopped.
 */
D_INLINE int
d_desc_emit(
    struct dj_desc_sink*    _sink,
    const char*             _key,
    const char*             _label,
    struct dj_ostream_value _value,
    uint32_t                _layer,
    uint32_t                _importance,
    uint32_t                _truth
)
{
    struct dj_desc_fact fact;

    if (d_desc_wants(_sink, _layer, _importance) == 0)
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    if ( (_sink->machine == NULL) ||
         (_sink->machine->fact == NULL) )
    {
        return 0;
    }

    fact.key        = _key;
    fact.label      = _label;
    fact.value      = _value;
    fact.depth      = _sink->depth;
    fact.layer      = (uint16_t)_layer;
    fact.importance = (uint8_t)_importance;
    fact.truth      = (uint8_t)_truth;

    if (_sink->machine->fact(_sink->context, &fact) != 0)
    {
        _sink->stopped = 1u;
    }

    return (_sink->stopped != 0u) ? 1 : 0;
}

/**
 * @brief Gives a text describer's words to the sink: to the machine's text
 *        operation when it has one, and otherwise as one fact, "text", on
 *        the intent layer, as its headline.
 *
 * @param[in,out] _sink  the sink.
 * @param[in]     _text  the words, borrowed; need not end in a NUL.
 * @param[in]     _size  their length.
 * @return 0 to go on; 1 once the sink has stopped.
 */
D_INLINE int
d_desc_text(
    struct dj_desc_sink* _sink,
    const char*          _text,
    size_t               _size
)
{
    if ( (_sink->machine == NULL) ||
         (_sink->machine->text == NULL) )
    {
        return d_desc_emit(_sink, "text", NULL,
                           d_ostream_value_text_n(_text, _size),
                           (uint32_t)D_DESC_LAYER_INTENT, 0u,
                           (uint32_t)D_DESC_TRUTH_OBSERVED);
    }

    if ( (d_desc_wants(_sink, (uint32_t)D_DESC_LAYER_INTENT, 0u) != 0) &&
         (_sink->machine->text(_sink->context, _text, _size) != 0) )
    {
        _sink->stopped = 1u;
    }

    return (_sink->stopped != 0u) ? 1 : 0;
}

/**
 * @brief Opens a level, to describe what an object holds, unless the sink
 *        does not want contents, its limit is reached, or it has stopped.
 *
 * @param[in,out] _sink   the sink.
 * @param[in]     _key    the level's key ("children"), borrowed.
 * @param[in]     _label  its label for prose, borrowed; may be null.
 * @return 1 when the caller may describe the next level, then call
 *         d_desc_leave; 0 when it may not, and must not leave.
 */
D_INLINE int
d_desc_enter(
    struct dj_desc_sink* _sink,
    const char*          _key,
    const char*          _label
)
{
    if ( (d_desc_wants(_sink, (uint32_t)D_DESC_LAYER_CONTENTS,
                       0u) == 0)                    ||
         (_sink->depth >= _sink->max_depth) )
    {
        return 0;
    }

    if ( (_sink->machine != NULL)        &&
         (_sink->machine->enter != NULL) &&
         (_sink->machine->enter(_sink->context, _key, _label,
                                _sink->depth) != 0) )
    {
        _sink->stopped = 1u;

        return 0;
    }

    ++_sink->depth;

    return 1;
}

/**
 * @brief Closes the level the last d_desc_enter that returned 1 opened.
 *
 * @param[in,out] _sink  the sink.
 */
D_INLINE void
d_desc_leave(
    struct dj_desc_sink* _sink
)
{
    if (_sink->depth == 0u)
    {
        return;
    }

    --_sink->depth;

    if ( (_sink->machine != NULL)        &&
         (_sink->machine->leave != NULL) &&
         (_sink->machine->leave(_sink->context, _sink->depth) != 0) )
    {
        _sink->stopped = 1u;
    }

    return;
}

/**
 * @brief Describes an object with a fact describer: the machine's begin, the
 *        describer, then its end, which is called whenever begin was.
 *
 * @param[in,out] _sink       the sink.
 * @param[in]     _describer  the type's fact describer.
 * @param[in]     _object     the object.
 * @return 0 when the description ran to its end; 1 when the sink stopped.
 */
D_INLINE int
d_desc_run(
    struct dj_desc_sink* _sink,
    fn_desc_describer    _describer,
    const void*          _object
)
{
    if (_sink->machine == NULL)
    {
        return 0;
    }

    if ( (_sink->machine->begin != NULL) &&
         (_sink->machine->begin(_sink->context) != 0) )
    {
        _sink->stopped = 1u;
    }

    if (_sink->stopped == 0u)
    {
        (void)_describer(_object, _sink);
    }

    if (_sink->machine->end != NULL)
    {
        _sink->machine->end(_sink->context);
    }

    return (_sink->stopped != 0u) ? 1 : 0;
}

/**
 * @brief A text describer's put, writing to a sink: d_desc_run_text's
 *        adapter, so a text describer reaches every machine.
 */
D_INLINE int
d_internal_desc_put_text(
    void*       _context,
    const char* _text,
    size_t      _size
)
{
    return d_desc_text((struct dj_desc_sink*)_context, _text, _size);
}

/**
 * @brief Describes an object with a text describer: as d_desc_run, its words
 *        going to the machine's text operation, or arriving as a fact.
 *
 * @param[in,out] _sink       the sink.
 * @param[in]     _describer  the type's text describer.
 * @param[in]     _object     the object.
 * @return 0 when the description ran to its end; 1 when the sink stopped.
 */
D_INLINE int
d_desc_run_text(
    struct dj_desc_sink*   _sink,
    fn_desc_text_describer _describer,
    const void*            _object
)
{
    if (_sink->machine == NULL)
    {
        return 0;
    }

    if ( (_sink->machine->begin != NULL) &&
         (_sink->machine->begin(_sink->context) != 0) )
    {
        _sink->stopped = 1u;
    }

    if (_sink->stopped == 0u)
    {
        (void)_describer(_object, d_internal_desc_put_text, _sink);
    }

    if (_sink->machine->end != NULL)
    {
        _sink->machine->end(_sink->context);
    }

    return (_sink->stopped != 0u) ? 1 : 0;
}


D_EXTERN_C_END


#endif  // DJINTERP_C_DESC_H
