/*******************************************************************************
* djinterp [djinterp]                                                  state.hpp
*
*   The UI-framework-agnostic state / value model (roadmap phase 1a).  Describes
* the n-dimensional discrete state space (its `axis`es), addresses a cell of the
* multi-resolution grid (`cell_address`), and carries the optimum `path` through
* the space at each frame.  `state_source` is the abstract provider every
* frontend and lens reads from; `in_memory_state_source` is a simple concrete
* implementation.
*
*
* path:      /inc/djinterp/ui/state.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_UI_STATE_HPP
#define DJINTERP_UI_STATE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
// djinterp
#include "../core/functional/maybe.hpp"  // maybe<T>, just, nothing<T>
#include "./types.hpp"
// re_std
#include "../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


NS_DJINTERP
NS_UI

// axis_kind
//   enum: whether an axis spans a continuous range or a discrete set of
// ordinal values.
enum class axis_kind
{
    continuous,
    discrete
};

// axis
//   struct: one dimension of the state space.  For a `continuous` axis the
// span is the closed interval [min, max]; for a `discrete` axis the values are
// the entries of `labels` (and `min` / `max` are unused).  `name` labels the
// axis in the UI.
struct axis
{
    std::string              name;
    axis_kind                kind = axis_kind::continuous;
    scalar                   min  = 0.0;
    scalar                   max  = 1.0;
    std::vector<std::string> labels;

    D_NODISCARD std::size_t
    count() const
    {
        return labels.size();
    }
};

// cell_address
//   struct: identifies one cell of the multi-resolution grid: the LOD `level`
// (0 = the single root cell) and the per-axis `indices` of the cell within
// that level.  `indices.size()` equals the number of axes.
struct cell_address
{
    re_std::uint32_t            level = 0;
    std::vector<std::size_t> indices;
};

// path_node
//   struct: a single vertex of an optimum path — its position in the
// n-dimensional state space (`coords`, one per axis) and an optional scalar
// metric carried at that vertex.
struct path_node
{
    std::vector<scalar> coords;
    maybe<scalar>       value;
};

// path
//   struct: an ordered sequence of vertices describing one optimum path
// through the state space at a single frame.
struct path
{
    std::vector<path_node> nodes;
};

// state_source
//   class: the abstract, UI-framework-agnostic provider of everything a
// visualization reads — the axes that define the space, the number of frames
// over time, the optimum path at each frame, and the scalar metric at any
// cell.  Frontends and lenses depend only on this interface, never on a
// concrete data store.
class state_source
{
public:
    virtual ~state_source() = default;

    D_NODISCARD virtual std::size_t axis_count()  const = 0;
    D_NODISCARD virtual std::size_t frame_count() const = 0;

    D_NODISCARD virtual const axis&
    axis_at(
        std::size_t _index
    ) const = 0;

    D_NODISCARD virtual maybe<scalar>
    value_at(
        const cell_address& _cell,
        std::size_t         _frame
    ) const = 0;

    D_NODISCARD virtual const path&
    current_path(
        std::size_t _frame
    ) const = 0;
};

// in_memory_state_source
//   class: a simple `state_source` backed by stored axes and per-frame paths,
// with cell values supplied on demand by a caller-provided function.  Suitable
// for small problems and for driving the module from already-materialised
// data; large or streaming sources should implement `state_source` directly.
// (Per the style guide these bodies would normally move to a .cpp; they are
// inlined here to keep the phase 1 core header-only.)
class in_memory_state_source : public state_source
{
public:
    // value_fn
    //   type: the on-demand value provider — maps a cell address and frame
    // index to an optional scalar metric.
    using value_fn =
        std::function<maybe<scalar>(const cell_address&, std::size_t)>;

    in_memory_state_source(
        std::vector<axis> _axes,
        std::vector<path> _frames,
        value_fn          _values
    )
        : m_axes(static_cast<std::vector<axis>&&>(_axes)),
          m_frames(static_cast<std::vector<path>&&>(_frames)),
          m_values(static_cast<value_fn&&>(_values))
    {}

    D_NODISCARD std::size_t
    axis_count() const override
    {
        return m_axes.size();
    }

    D_NODISCARD std::size_t
    frame_count() const override
    {
        return m_frames.size();
    }

    D_NODISCARD const axis&
    axis_at(
        std::size_t _index
    ) const override
    {
        return m_axes[_index];
    }

    D_NODISCARD maybe<scalar>
    value_at(
        const cell_address& _cell,
        std::size_t         _frame
    ) const override
    {
        // with no provider there is no value to report
        if (!m_values)
        {
            return nothing<scalar>();
        }

        return m_values(_cell, _frame);
    }

    D_NODISCARD const path&
    current_path(
        std::size_t _frame
    ) const override
    {
        return m_frames[_frame];
    }

private:
    std::vector<axis> m_axes;
    std::vector<path> m_frames;
    value_fn          m_values;
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_STATE_HPP
