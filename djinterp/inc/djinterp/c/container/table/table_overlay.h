/*******************************************************************************
* djinterp [c]                                                   table_overlay.h
*
*   The restriction bundle a table wears -- the finite vocabulary
* containers.tex fixes (mu, gamma, delta, sigma, eta) and its composition
* law, in one struct rather than one container type per combination.
*
*   PORTABILITY:
*   C99 / C++11, the framework floors.
*
*
* path:      /inc/djinterp/c/container/table/table_overlay.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.02
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_TABLE_TABLE_OVERLAY_H
#define DJINTERP_C_CONTAINER_TABLE_TABLE_OVERLAY_H 1

// djinterp
#include "./table_layout.h"

D_EXTERN_C_BEGIN


// VI.   The overlay: a table as a restriction bundle
//   containers.tex, "Overlays: containers as restriction bundles", fixes a
// FINITE vocabulary of restrictions and a composition law over them, so the
// bundle is core and not one container type per combination:
//
//     mu_m^E     bounded multiplicity (bag)   #_E(c,x) <= m for every x
//     gamma_k    capacity             (bag)   |c| <= kappa
//     delta_I    domain               (bag)   every value lies in <I>
//     sigma      sorted           (sequence)  comp(e_i, e_i+1) in {-1, 0}
//     eta        keyed              (static)  the value is a Key x Val pair
//
//   Overlays compose by UNION of their restrictions, the extension being the
// intersection: O1 /\ O2 = R1 u R2.  The empty overlay is worn by everything,
// and adding a restriction can only shrink the extension -- which is why
// d_table_overlay_meet is a bitwise-or of the present sets and nothing more
// clever.
//
//   ONE STRUCT, NOT THREE CONTAINERS.  constrained_table wears
// {gamma_kappa, delta_I}, sorted_table wears {sigma}, keyed_table wears
// {eta, mu_1}.  In C they are the same carrier plus this bundle; the C++ face
// spells each as a type because C++ can, which is notation, not semantics.
//
//   TYPE ERASURE NEEDS COMPARATORS.  The carrier is bytes, so delta_I, sigma
// and mu_m cannot compare cells themselves; the bundle carries comp_tau and,
// under eta, the key projection.  A restriction whose comparator is absent
// simply is not checked -- the same discipline header_extent uses.
//
//   WHY NOT fn_comparator.  djinterp.h's fn_comparator is
// `int (*)(const void*, const void*)` -- no context pointer, so it cannot carry
// a locale, an epsilon, or a key table without global mutable state.  The hooks
// below take a context and are otherwise the same idea.

// restriction identifiers
//   macro: the bitmask positions of the five restrictions. A bundle is the set
// of restrictions it declares, so `present` is that set and nothing else.
#define D_TABLE_RESTRICTION_MULTIPLICITY    0x01u   // mu_m^E
#define D_TABLE_RESTRICTION_CAPACITY        0x02u   // gamma_kappa
#define D_TABLE_RESTRICTION_DOMAIN          0x04u   // delta_I
#define D_TABLE_RESTRICTION_SORTED          0x08u   // sigma
#define D_TABLE_RESTRICTION_KEYED           0x10u   // eta

// d_table_compare_fn
//   type: comp_tau over two cells, returning negative, zero or positive. The
// context is the caller's and is passed through untouched. (A function-pointer
// member, not an enum or a bool, so the struct's layout is the same in both
// dialects.)

// d_table_overlay
//   struct: the restriction bundle a table wears. Every field is inert unless
// its bit is set in `present`, so the empty bundle is a zeroed struct and is
// worn by everything.
struct d_table_overlay
{
    uint32_t                present;        // the set of declared restrictions
    D_INTERNAL_TABLE_EXTENT capacity;       // kappa, when CAPACITY is present
    D_INTERNAL_TABLE_EXTENT multiplicity;   // m, when MULTIPLICITY is present
    const void*             domain_low;     // <I> lower, when DOMAIN is present
    const void*             domain_high;    // <I> upper, when DOMAIN is present
    int  (*compare)(const void* _a,         // comp_tau; delta_I and sigma need
    // it
                    const void* _b,
                    void*       _context);
    const void* (*key_of)(const void* _cell,// eta's projection, when KEYED
                          void*       _context);
    void*                   context;        // passed to both hooks untouched
};

// d_table_overlay_none
//   function: the empty overlay -- the top of the strength order, worn by
// every table. A zeroed bundle, so a caller may also just memset one.
D_NODISCARD D_INLINE struct d_table_overlay
d_table_overlay_none(void)
{
    struct d_table_overlay result;

    result.present      = 0;
    result.capacity     = 0;
    result.multiplicity = 0;
    result.domain_low   = NULL;
    result.domain_high  = NULL;
    result.compare      = NULL;
    result.key_of       = NULL;
    result.context      = NULL;

    return result;
}

// d_table_overlay_wears
//   function: whether the bundle declares a given restriction.
int d_table_overlay_wears(const struct d_table_overlay* _overlay,
                          uint32_t                      _restriction);

// d_table_overlay_capacity_of
//   function: kappa, or the largest representable extent when the bundle
// declares no capacity -- an undeclared ceiling does not bind.
size_t d_table_overlay_capacity_of(const struct d_table_overlay* _overlay);

// d_table_overlay_in_domain
//   function: whether a cell lies in <I> -- delta_I applied to one value.
// Vacuously true when the restriction is absent, and also when the bundle
// carries no comparator, since an unreadable restriction is not a failed one.
int d_table_overlay_in_domain(const struct d_table_overlay* _overlay,
                              const void*                   _cell);

// d_table_overlay_key
//   function: eta's projection of a cell, or the cell itself when the bundle
// is not keyed -- an unkeyed duplicate-equivalence is over whole values.
const void* d_table_overlay_key(const struct d_table_overlay* _overlay,
                                const void*                   _cell);


// II.   operations (cont.)
enum d_table_status    d_table_overlay_admits_row(
    const struct d_table*         _table,
    const struct d_table_overlay* _overlay,
    const void*                   _row,
    size_t                        _width);
enum d_table_status    d_table_overlay_holds(
    const struct d_table*         _table,
    const struct d_table_overlay* _overlay);
struct d_table_overlay d_table_overlay_meet(const struct d_table_overlay* _a,
                                            const struct d_table_overlay* _b);
int                    d_table_overlay_stronger_than(
    const struct d_table_overlay* _a,
    const struct d_table_overlay* _b);
enum d_table_status    d_table_overlay_push_row(
    struct d_table*               _table,
    const struct d_table_overlay* _overlay,
    const void*                   _row,
    size_t                        _width,
    const struct d_table_alloc*   _alloc);

D_EXTERN_C_END


#endif  // DJINTERP_C_CONTAINER_TABLE_TABLE_OVERLAY_H
