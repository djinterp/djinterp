/******************************************************************************
* djinterp [parsegen]                                              registry.h
*
* The table of stages a pipeline can be built from, and how one is chosen.
*   A frontend reads a notation. A pass rewrites a grammar. A family plans a
* program. A backend turns one into something runnable. All four are the same
* kind of thing from the outside -- something with a name, a declared
* capability profile, and a function table -- so they live in one registry and
* differ by a kind field.
*
*   SELECTION IS SET ARITHMETIC, NOT A DECISION TREE. A stage declares three
* masks: what it needs present, what it cannot cope with, and what it can
* produce or handle. A stage accepts a grammar when the grammar uses nothing
* the stage rejects and supplies everything the stage needs. That is two
* instructions, and -- the point -- adding the LR family later is a row of
* declarations, not an edit to whatever was choosing between the families that
* already existed. This is the op_set idea one level up: behaviour is
* registered, never branched on.
*
*   WHY THIS IS NOT AN op_set. An opcode space is dense and private, so a
* registry over it is an array indexed by opcode. Stages are keyed by name,
* queried by capability, and enumerated for diagnostics; none of that is an
* indexed load. The two registries share an idea and no code, which is the
* right amount.
*
*   DIAGNOSTICS ARE THE PRODUCT. When no stage fits, the useful output is not
* NULL -- it is "no family accepts this grammar: peg rejects LEFT_RECURSION;
* lr needs TOKEN_STREAM". Every query here takes a sink and says exactly that.
*
*   Requires: parsegen/parsegen.h, parsegen/feature.h, parse/diagnostic.h.
*
* path:      /inc/djinterp/parsegen/registry.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STAGES
    ------
    1.  Kinds
         1.  d_parsegen_stage_kind
    2.  The stage
         1.  d_parsegen_stage
         2.  Stage flags
              a. D_PARSEGEN_STAGE_DEFAULT
              b. D_PARSEGEN_STAGE_EXPERIMENTAL
    3.  Acceptance
         1.  d_parsegen_stage_accepts
2.  THE REGISTRY
    ------------
    1.  The table
         1.  d_parsegen_registry
         2.  Registry flags
              a. D_PARSEGEN_REGISTRY_OWNS_STAGES
3.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Registration
    3.  Enumeration
    4.  Selection
*/

#ifndef DJINTERP_PARSEGEN_REGISTRY_
#define DJINTERP_PARSEGEN_REGISTRY_ 1

// std
#include <stddef.h>                     // size_t, NULL
#include <stdint.h>                     // uint8_t, uint16_t, uint32_t
// djinterp
#include "../parse/diagnostic.h"        // the channel a failed query explains
                                        // itself through
#include "./feature.h"                  // d_parsegen_features, and the
                                        // subsystem umbrella


//==============================================================================
// 1.  STAGES
//==============================================================================


// 1.1    Kinds
//------------------------------------------------------------------------------
// 1.1.1
// d_parsegen_stage_kind
//   enum: what part of the pipeline a stage occupies. The kind also decides
// how to read the stage's function table, since this level stores it as an
// opaque pointer -- a registry that knew every stage's signature would have to
// be edited to admit a new kind, which is the thing being avoided.
enum d_parsegen_stage_kind
{
    D_PARSEGEN_STAGE_FRONTEND = 0,
    D_PARSEGEN_STAGE_PASS     = 1,
    D_PARSEGEN_STAGE_FAMILY   = 2,
    D_PARSEGEN_STAGE_BACKEND  = 3,
    D_PARSEGEN_STAGE_KIND_COUNT = 4,
    D_PARSEGEN_STAGE_KIND_USER  = 64
};


// 1.2    The stage
//------------------------------------------------------------------------------
// 1.2.1
// d_parsegen_stage
//   struct: one registrable stage. Everything except `table` and `ctx` is
// declaration -- the profile a query matches against -- so a registry can be
// searched, filtered, and explained without any stage being invoked.
//   `provides` means different things per kind and the same thing in each
// case: what this stage can put into the pipeline. A frontend provides the
// features its notation can express; a pass provides what it introduces; a
// family provides what it supports; a backend provides the opcode space it
// can lower. `needs` and `rejects` are uniform: what must be present, and what
// must not.
struct d_parsegen_stage
{
    const char*         name;
    const char*         summary;
    uint32_t            kind;
    d_parsegen_features provides;
    d_parsegen_features needs;
    d_parsegen_features rejects;
    const void*         table;
    void*               ctx;
    uint16_t            family;
    uint16_t            flags;
};

// 1.2.2
// D_PARSEGEN_STAGE_DEFAULT
//   constant: prefer this stage when several of its kind accept. Selection is
// otherwise registration order, which makes a build's choice depend on link
// order -- this is how a build says which one it meant.
#define D_PARSEGEN_STAGE_DEFAULT        0x0001u
// D_PARSEGEN_STAGE_EXPERIMENTAL
//   constant: never selected automatically; usable only when named. A stage
// can then be registered and exercised without becoming anyone's default.
#define D_PARSEGEN_STAGE_EXPERIMENTAL   0x0002u


// 1.3    Acceptance
//------------------------------------------------------------------------------
/*
d_parsegen_stage_accepts
  Whether a stage can handle a grammar with a given capability profile.
NOTE:
  The whole selection rule, in one expression: nothing used that the stage
rejects, and everything present that the stage needs.

Parameter(s):
  _stage:    the stage to test; may be NULL, which accepts nothing.
  _features: what the grammar uses.
Return:
  A boolean value corresponding to either:
  - 1, if the stage can handle it, or
  - 0, otherwise.
*/
D_INLINE int
d_parsegen_stage_accepts(
    const struct d_parsegen_stage* _stage,
    d_parsegen_features            _features
)
{
    if (!_stage)
    {
        return 0;
    }

    return ( (!d_parsegen_features_any(_features, _stage->rejects)) &&
             (d_parsegen_features_has(_features, _stage->needs)) )
           ? 1
           : 0;
}


//==============================================================================
// 2.  THE REGISTRY
//==============================================================================


// 2.1    The table
//------------------------------------------------------------------------------
// 2.1.1
// d_parsegen_registry
//   struct: the stages available to build a pipeline from. Storage may be
// caller-supplied or owned, as everywhere else here -- and a static array is
// the common case, since a build usually links in exactly the stages it wants
// and registers them once.
struct d_parsegen_registry
{
    struct d_parsegen_stage* stages;
    uint32_t                 count;
    uint32_t                 capacity;
    uint8_t                  flags;
    uint8_t                  reserved[3];
};

// 2.1.2
// D_PARSEGEN_REGISTRY_OWNS_STAGES
//   constant: the table was allocated by the registry and is freed by
// d_parsegen_registry_release.
#define D_PARSEGEN_REGISTRY_OWNS_STAGES 0x01u


//==============================================================================
// 3.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 3.1    Lifetime
//------------------------------------------------------------------------------
void            d_parsegen_registry_init(struct d_parsegen_registry* _registry,
                                         struct d_parsegen_stage*    _stages,
                                         uint32_t                    _capacity);
#if (D_INTERNAL_PARSEGEN_REGISTRY_HEAP == 1)
D_NODISCARD int d_parsegen_registry_init_heap(
                    struct d_parsegen_registry* _registry,
                    uint32_t                    _capacity);
#endif  // D_INTERNAL_PARSEGEN_REGISTRY_HEAP
void            d_parsegen_registry_release(
                    struct d_parsegen_registry* _registry);

// 3.2    Registration
//------------------------------------------------------------------------------
D_NODISCARD int d_parsegen_registry_add(
                    struct d_parsegen_registry*    _registry,
                    const struct d_parsegen_stage* _stage);

// 3.3    Enumeration
//------------------------------------------------------------------------------
/*
d_parsegen_registry_at
  The stage at an index.

Parameter(s):
  _registry: the registry to read; may be NULL.
  _index:    zero-based index in registration order.
Return:
  A pointer to the stage, or NULL when the index names none.
*/
D_INLINE const struct d_parsegen_stage*
d_parsegen_registry_at(
    const struct d_parsegen_registry* _registry,
    uint32_t                          _index
)
{
    // reject a missing registry, absent storage, and an index past the table
    if ( (!_registry)                   ||
         (!_registry->stages)           ||
         (_index >= _registry->count)   )
    {
        return NULL;
    }

    return &_registry->stages[_index];
}

uint32_t        d_parsegen_registry_count_of(
                    const struct d_parsegen_registry* _registry,
                    uint32_t                          _kind);
size_t          d_parsegen_registry_render(
                    const struct d_parsegen_registry* _registry,
                    uint32_t                          _kind,
                    char*                             _out,
                    size_t                            _size);

// 3.4    Selection
//------------------------------------------------------------------------------
//   find names a stage explicitly; select asks the registry to choose. Both
// explain a failure through the sink rather than merely returning NULL, since
// "which of my stages could have done this, and what stopped each of them" is
// the only question a caller has at that point.
const struct d_parsegen_stage* d_parsegen_registry_find(
                                   const struct d_parsegen_registry* _registry,
                                   uint32_t                          _kind,
                                   const char*                       _name,
                                   struct d_parse_diag_sink*         _diag);
const struct d_parsegen_stage* d_parsegen_registry_select(
                                   const struct d_parsegen_registry* _registry,
                                   uint32_t                          _kind,
                                   d_parsegen_features               _features,
                                   struct d_parse_diag_sink*         _diag);

D_EXTERN_C_END


#endif  // DJINTERP_PARSEGEN_REGISTRY_
