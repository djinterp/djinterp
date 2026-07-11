/******************************************************************************
* djinterp [container]                                   constrained_table.hpp
*
*   constrained_table -- the BAG-LEVEL conjunction overlay over a table backing
* (Overlays: containers as restriction bundles).  It bundles the two bag-level
* restrictions of the vocabulary, both blind to order and construction:
*
*     capacity  gamma_kappa (bag-level)   at most kappa rows: |c| is capped, and
*                                          an insertion past the cap is refused,
*                                          not admitted (the overlay is preserved,
*                                          not merely tested).
*     domain    delta_I (bag-level)        every cell lies in a closed interval
*                                          [lo, hi]: a value outside the domain is
*                                          refused.  The domain is optional -- a
*                                          constrained_table may carry only the
*                                          capacity restriction.
*
*   The bundle is {gamma_kappa} or {gamma_kappa, delta_I}, and adding delta_I
* strictly strengthens it (a smaller extension) exactly when some value would lie
* outside I.  Both being bag-level, the overlay is order-blind: it observes a
* container only through its bag of cells and treats permutations alike.
*
*   PRESERVATION.  push_row admits a row only when it keeps the container within
* the cap AND (when a domain is set) every cell lies in the interval; otherwise it
* refuses -- throwing, or, in the try_ form, reporting failure.  Removal is
* unconstrained (dropping rows keeps both restrictions), and neither ceiling nor
* domain is ever silently broken.  This is the shape of a BOUNDED source under an
* operation that must respect the ceiling.
*
*   CLASSIFICATION.  A fixed cap with no growth accessor makes the framework read
* the container as BOUNDED (capacity present, no reserve).  Cf. a fixed-capacity
* unique buffer {mu_1^E, gamma_kappa} or an interval set {mu_1^E, delta_I}: those
* add a multiplicity/order restriction on top of these same bag-level bounds.
*
*   PORTABILITY:
*   C++17 (inherits the table backing and the options surface).  The domain
* restriction requires _Type to be ordered (operator<).
*
*
* path:      /inc/djinterp/core/container/table/constrained_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.05
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_constrained_table (detection trait)
II.   constrained_table (class)
      1. member types and overlay / axis markers
      2. construction
      3. read surface (delegated, const)
      4. restriction queries (capacity / domain)
      5. structural mutation (restriction-preserving)
III.  make_constrained_table
*/

#ifndef DJINTERP_CONTAINER_CONSTRAINED_TABLE_
#define DJINTERP_CONTAINER_CONSTRAINED_TABLE_ 1

// std
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"                     // NS_*, D_CONSTEXPR, clean_t
#include "./table.hpp"                             // table backing (+ hierarchical tag)
#include "../container_options.hpp"                // axis enums, options base


NS_DJINTERP


// ===========================================================================
// I.   is_constrained_table (detection trait)
// ===========================================================================

// constrained_table (fwd)
template<typename    _Type,
         std::size_t _MaxRows,
         typename    _SizeType,
         typename    _DifferenceType,
         typename... _Options>
class constrained_table;

// is_constrained_table
//   trait: true when _Type (after stripping cv/ref) is a specialization of
// constrained_table.
NS_INTERNAL

    template<typename _Type>
    struct is_constrained_table_impl : std::false_type
    {};

    template<typename    _T,
             std::size_t _M,
             typename    _S,
             typename    _D,
             typename... _O>
    struct is_constrained_table_impl<constrained_table<_T, _M, _S, _D, _O...>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_constrained_table : internal::is_constrained_table_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Type>
inline constexpr bool is_constrained_table_v =
    is_constrained_table<_Type>::value;
#endif


// ===========================================================================
// II.  constrained_table (class)
// ===========================================================================

// constrained_table
//   class: a table capped at _MaxRows rows and, optionally, with cells confined
// to a closed interval.  Wraps a `table` backing, delegates the read surface,
// and admits rows only when they keep both restrictions.
template<typename    _Type,
         std::size_t _MaxRows,
         typename    _SizeType       = std::size_t,
         typename    _DifferenceType = std::ptrdiff_t,
         typename... _Options>
class constrained_table
    : public options_container_base<_Options...>
{
private:
    using backing_type = table<_Type, _DifferenceType, _SizeType>;

public:
    // --- 1. member types and overlay / axis markers ---

    using value_type       = _Type;
    using cell_type        = _Type;
    using size_type        = _SizeType;
    using difference_type  = _DifferenceType;
    using reference        = const _Type&;   // cells read-only; growth is checked
    using const_reference  = const _Type&;
    using pointer          = const _Type*;
    using const_pointer    = const _Type*;

    using const_iterator     = typename backing_type::const_iterator;
    using const_row_type     = typename backing_type::const_row_type;
    using const_row_iterator = typename backing_type::const_row_iterator;

    using element_type       = _Type;
    using structure_category = hierarchical;

    // overlay markers (the restriction bundle this container wears).
    static constexpr bool      bounded_overlay = true;                       // gamma_kappa
    static constexpr size_type max_row_capacity = static_cast<size_type>(_MaxRows);

    // axis positions.
    static constexpr container_lifetime      lifetime      =
        container_lifetime::mutable_storage;
    static constexpr container_storage_kind  storage_kind  =
        container_storage_kind::dynamic_storage;
    static constexpr container_ordering      ordering      =
        container_ordering::ordered;
    static constexpr container_bounds        bounds        =
        container_bounds::bounded;              // the capacity overlay caps |c|
    static constexpr container_iterability   iterability   =
        container_iterability::iterable;
    static constexpr container_multiplicity  multiplicity_grade =
        container_multiplicity::multi;
    static constexpr container_structure     structure     =
        container_structure::hierarchical;
    static constexpr size_type               rank  = static_cast<size_type>(2);
    static constexpr size_type               depth = static_cast<size_type>(2);

    // --- 2. construction ---

    // capacity only: {gamma_kappa}, no domain restriction.
    constrained_table()
        : m_base(),
          m_has_domain(false),
          m_lo(),
          m_hi()
    {}

    // capacity and domain: {gamma_kappa, delta_I} with I = [_lo, _hi].
    constrained_table(
        const _Type& _lo,
        const _Type& _hi
    )
        : m_base(),
          m_has_domain(true),
          m_lo(_lo),
          m_hi(_hi)
    {}

    constrained_table(const constrained_table&)            = default;
    constrained_table(constrained_table&&)                 = default;
    constrained_table& operator=(const constrained_table&) = default;
    constrained_table& operator=(constrained_table&&)      = default;
    ~constrained_table()                                   = default;

    // --- 3. read surface (delegated, const) ---

    D_NODISCARD size_type rows() const noexcept
    {
        return m_base.rows();
    }

    D_NODISCARD size_type cols() const noexcept
    {
        return m_base.cols();
    }

    D_NODISCARD size_type row_count() const noexcept
    {
        return m_base.row_count();
    }

    D_NODISCARD size_type column_count() const noexcept
    {
        return m_base.column_count();
    }

    D_NODISCARD size_type size() const noexcept
    {
        return m_base.size();
    }

    D_NODISCARD bool empty() const noexcept
    {
        return m_base.empty();
    }

    D_NODISCARD const_reference operator()(
        size_type _r,
        size_type _c
    ) const
    {
        return m_base(_r, _c);
    }

    D_NODISCARD const_reference at(
        size_type _r,
        size_type _c
    ) const
    {
        return m_base.at(_r, _c);
    }

    D_NODISCARD const_row_type row(size_type _r) const
    {
        return m_base.row(_r);
    }

    D_NODISCARD const_row_type operator[](size_type _r) const
    {
        return m_base[_r];
    }

    D_NODISCARD const_iterator begin() const noexcept
    {
        return m_base.begin();
    }

    D_NODISCARD const_iterator end() const noexcept
    {
        return m_base.end();
    }

    D_NODISCARD const_iterator cbegin() const noexcept
    {
        return m_base.cbegin();
    }

    D_NODISCARD const_iterator cend() const noexcept
    {
        return m_base.cend();
    }

    D_NODISCARD const_row_iterator row_begin() const noexcept
    {
        return m_base.row_begin();
    }

    D_NODISCARD const_row_iterator row_end() const noexcept
    {
        return m_base.row_end();
    }

    D_NODISCARD const backing_type& base() const noexcept
    {
        return m_base;
    }

    // --- 4. restriction queries (capacity / domain) ---

    // max_rows -- the capacity ceiling kappa, in rows.
    D_NODISCARD static constexpr size_type max_rows() noexcept
    {
        return max_row_capacity;
    }

    // full -- true when the row cap is reached (no further row is admissible).
    D_NODISCARD bool full() const noexcept
    {
        return (m_base.rows() >= max_row_capacity);
    }

    // capacity -- the ceiling as a cell count (kappa rows times the width).
    D_NODISCARD size_type capacity() const noexcept
    {
        return (max_row_capacity * m_base.cols());
    }

    // has_domain -- whether the delta_I restriction is active.
    D_NODISCARD bool has_domain() const noexcept
    {
        return m_has_domain;
    }

    D_NODISCARD const _Type& domain_low() const noexcept
    {
        return m_lo;
    }

    D_NODISCARD const _Type& domain_high() const noexcept
    {
        return m_hi;
    }

    // in_domain -- whether _value lies in the closed interval [lo, hi] (always
    // true when no domain is set).
    D_NODISCARD bool in_domain(const _Type& _value) const
    {
        // no domain restriction admits every value
        if (!m_has_domain)
        {
            return true;
        }

        // closed interval membership: lo <= value <= hi
        return ( !(_value < m_lo) &&
                 !(m_hi < _value) );
    }

    // --- 5. structural mutation (restriction-preserving) ---

    // try_push_row -- append _row iff it keeps the bundle: the cap is not yet
    // reached AND (when a domain is set) every cell lies in the interval.
    // Returns whether the row was admitted; never breaks a restriction.
    D_NODISCARD bool try_push_row(std::initializer_list<_Type> _row)
    {
        // gamma_kappa: refuse once the ceiling is reached
        if (full())
        {
            return false;
        }

        // delta_I: refuse a row carrying any out-of-domain cell
        if (m_has_domain)
        {
            for (const _Type& cell : _row)
            {
                if (!in_domain(cell))
                {
                    return false;
                }
            }
        }

        m_base.push_row(_row);

        return true;
    }

    // push_row -- append _row, throwing when a restriction would be broken
    // (std::length_error past the cap, std::out_of_range outside the domain).
    void push_row(std::initializer_list<_Type> _row)
    {
        // gamma_kappa
        if (full())
        {
            throw std::length_error(
                "constrained_table::push_row: capacity (max_rows) reached.");
        }

        // delta_I
        if (m_has_domain)
        {
            for (const _Type& cell : _row)
            {
                if (!in_domain(cell))
                {
                    throw std::out_of_range(
                        "constrained_table::push_row: cell outside the domain.");
                }
            }
        }

        m_base.push_row(_row);

        return;
    }

    // erase_row -- remove the row at index _r (removal keeps both restrictions).
    void erase_row(size_type _r)
    {
        m_base.erase_row(_r);

        return;
    }

    // pop_row -- remove the last row.
    void pop_row()
    {
        m_base.pop_row();

        return;
    }

    // clear -- drop all rows.
    void clear() noexcept
    {
        m_base.clear();

        return;
    }

private:
    backing_type m_base;        // the row store, held within the cap
    bool         m_has_domain;  // whether delta_I is active
    _Type        m_lo;          // domain lower bound (inclusive)
    _Type        m_hi;          // domain upper bound (inclusive)
};


// ===========================================================================
// III. make_constrained_table
// ===========================================================================

// make_constrained_table
//   function: a capacity-only constrained_table<_Type, _MaxRows>.
template<std::size_t _MaxRows,
         typename    _Type>
D_NODISCARD constrained_table<_Type, _MaxRows>
make_constrained_table()
{
    return constrained_table<_Type, _MaxRows>();
}

// make_domain_table
//   function: a constrained_table<_Type, _MaxRows> with domain [_lo, _hi].
template<std::size_t _MaxRows,
         typename    _Type>
D_NODISCARD constrained_table<_Type, _MaxRows>
make_domain_table(
    const _Type& _lo,
    const _Type& _hi)
{
    return constrained_table<_Type, _MaxRows>(_lo, _hi);
}


// ---------------------------------------------------------------------------
// axis / overlay conformance -- a fixed cap with no growth accessor must read as
// BOUNDED (representative instantiation).
// ---------------------------------------------------------------------------
namespace table_axis_conformance
{
    using constrained_table_probe = constrained_table<int, 4>;

    static_assert(is_iterable_container_v<constrained_table_probe>,
                  "constrained_table must classify as iterable.");
    static_assert(is_bounded_container_v<constrained_table_probe>,
                  "constrained_table must classify as bounded (capacity overlay).");
    static_assert(!is_unbounded_container_v<constrained_table_probe>,
                  "constrained_table must not classify as unbounded.");
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONSTRAINED_TABLE_
