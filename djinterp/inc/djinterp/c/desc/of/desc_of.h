/*******************************************************************************
* djinterp [c][desc]                                                   desc_of.h
*
*   What desc's describers of other modules' types share (desc's decisions
* 68 and 82): the layout row by which desc's suite checks a describer
* against the type it describes. Each describer reads the type's fields
* itself, so each records, beside it, the type as the describer knows it: a
* record struct with the type's fields in the type's order, under the same
* knobs. A layout row sets the record's size, offsets and widths beside the
* type's own; where a row's two disagree, the type has changed and its
* describer has not followed, and desc's suite says which type and field.
*   A record checks the describer against its type; it cannot check the
* library against the program. A knob that changes a type's layout (a
* test build's D_CFG_MEM_STATS, say) lays the type out one way in a
* library built without it and another in a program built with it, and a
* describer then reads the wrong bytes. So each module's header carries its
* table as an inline, which a program compiles with its own knobs, and the
* library gives the same table as it was built: d_desc_of_agree compares
* the two, and names the first row they differ in.
*   Each module's describers have a unit and a header of their own
* (mem_desc.c and mem_desc.h for memory), in the library
* djinterp_c_desc_of, each built only where its knob keeps it
* (cfg_desc_of.h).
*
* HOW TO USE
* ----------
*   A module's describers lay out a record per type and a table of rows,
* D_DESC_OF_LAYOUT_SIZE and D_DESC_OF_LAYOUT_FIELD, in an inline of their
* header; d_desc_of_holds checks a table, and d_desc_of_agree checks the
* library's against the program's.
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   D_CFG_DESC_OF and each module's D_CFG_DESC_OF_<MODULE> (cfg_desc_of.h).
*
*
* path:      /inc/djinterp/c/desc/of/desc_of.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LAYOUTS
    -------
    1.  dj_desc_of_layout
    2.  D_DESC_OF_LAYOUT_SIZE
    3.  D_DESC_OF_LAYOUT_FIELD
    4.  fn_desc_of_layout
2.  CHECKING
    --------
    1.  d_desc_of_holds
    2.  d_desc_of_agree
*/

#ifndef DJINTERP_C_DESC_OF_H
#define DJINTERP_C_DESC_OF_H 1

// std
#include <stddef.h>  // offsetof, size_t
#include <string.h>  // strcmp
// djinterp
#include "../../djinterp.h"                       // framework root
#include "../../../config/c/desc/cfg_desc_of.h"   // the knobs
#include "../desc_units.h"                        // struct dj_desc_type


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  LAYOUTS
//==============================================================================

// 1.1
// dj_desc_of_layout
//   struct: one row of a describer's layout check: a type's size, or the
// offset and width of one field its describer reads, as the describer's
// record has it and as the type has it. A row holds when each recorded
// number equals the type's.
struct dj_desc_of_layout
{
    const char* type;            // the type's name: "dj_arena"
    const char* field;           // the field read; null: the type's size
    size_t      recorded;        // the record's offset, or its size
    size_t      actual;          // the type's
    size_t      recorded_width;  // the record's field size; 0: a size row
    size_t      actual_width;    // the type's; 0: a size row
};

// 1.2
// D_DESC_OF_LAYOUT_SIZE
//   macro: the row of a layout table that checks a type's size against its
// record's. Both are struct tags.
#define D_DESC_OF_LAYOUT_SIZE(type_, record_)                                  \
    { #type_, NULL, sizeof(struct record_), sizeof(struct type_), 0u, 0u }

// 1.3
// D_DESC_OF_LAYOUT_FIELD
//   macro: the row of a layout table that checks one field a describer
// reads: its offset and its width, in the record and in the type. The field
// has the same name in both.
#define D_DESC_OF_LAYOUT_FIELD(type_, record_, field_)                         \
    { #type_, #field_,                                                         \
      offsetof(struct record_, field_), offsetof(struct type_, field_),        \
      sizeof(((const struct record_*)0)->field_),                              \
      sizeof(((const struct type_*)0)->field_) }

// 1.4
// fn_desc_of_layout
//   function pointer: a module's layout table, d_desc_of_mem_layout's kind:
// it sets *_rows to the module's rows and returns their count; 0 and null
// where the module's describers are not built.
typedef size_t (*fn_desc_of_layout)(const struct dj_desc_of_layout** _rows);


//==============================================================================
// 2.  CHECKING
//==============================================================================

/**
 * @brief Whether every row of a layout table holds: each recorded size,
 *        offset and width is the type's.
 *
 * @param[in]  _rows   the rows; may be null when _count is 0.
 * @param[in]  _count  their count.
 * @param[out] _index  set to the first row that does not hold, or to
 *                     _count; may be null.
 * @return 1 when every row holds; 0 otherwise.
 */
D_INLINE int
d_desc_of_holds(
    const struct dj_desc_of_layout* _rows,
    size_t                          _count,
    size_t*                         _index
)
{
    size_t index;

    for (index = 0u; index < _count; ++index)
    {
        if ( (_rows[index].recorded != _rows[index].actual) ||
             (_rows[index].recorded_width != _rows[index].actual_width) )
        {
            break;
        }
    }

    if (_index != NULL)
    {
        *_index = index;
    }

    return (index == _count) ? 1 : 0;
}

/**
 * @brief Whether two builds of one layout table agree: the same rows, and
 *        each type laid out alike: a library's table against a program's.
 *
 * @param[in]  _built        the library's rows.
 * @param[in]  _built_count  their count.
 * @param[in]  _here         the program's rows, from the header's inline.
 * @param[in]  _here_count   their count.
 * @param[out] _index        set to the first row they differ in, or to the
 *                           shorter count when only the counts differ; may
 *                           be null.
 * @return 1 when they agree; 0 otherwise.
 */
D_INLINE int
d_desc_of_agree(
    const struct dj_desc_of_layout* _built,
    size_t                          _built_count,
    const struct dj_desc_of_layout* _here,
    size_t                          _here_count,
    size_t*                         _index
)
{
    size_t count = (_built_count < _here_count) ? _built_count : _here_count;
    size_t index;

    for (index = 0u; index < count; ++index)
    {
        if ( (strcmp(_built[index].type, _here[index].type) != 0)       ||
             ((_built[index].field == NULL) !=
              (_here[index].field == NULL))                            ||
             ( (_built[index].field != NULL) &&
               (strcmp(_built[index].field, _here[index].field) != 0) ) ||
             (_built[index].actual != _here[index].actual)              ||
             (_built[index].actual_width != _here[index].actual_width) )
        {
            break;
        }
    }

    if (_index != NULL)
    {
        *_index = index;
    }

    return ( (index == count) && (_built_count == _here_count) ) ? 1 : 0;
}


D_EXTERN_C_END


#endif  // DJINTERP_C_DESC_OF_H
