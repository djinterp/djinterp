/*******************************************************************************
* djinterp [djinterp]                                           visit_source.hpp
*
* A state source that counts visits.
*   Many searches are best seen as something moving through a discrete space
* over time: a pathfinder expanding cells, a parser trying rules at positions.
* visit_source records each visit as a cell and a frame, and reports, for any
* cell and frame, how many visits the cell had received by then -- the heat a
* heatmap lens draws. The path at each frame comes from a function the caller
* supplies (the current best path, a call stack), or is empty.
*   Every axis must be discrete; its labels are its cells. Visits may arrive in
* any order, though recording them in frame order is cheapest. A count is one
* binary search, so a lens samples any frame at the cost of one search per
* cell. (Per the style guide these bodies would normally move to a .cpp; they
* are inline here to keep djinterp::ui header-only.)
*
*
* path:      /inc/djinterp/ui/sources/visit_source.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UI_SOURCES_VISIT_SOURCE_HPP
#define DJINTERP_UI_SOURCES_VISIT_SOURCE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>   // std::upper_bound
#include <cstddef>     // std::size_t
#include <functional>  // std::function
#include <utility>     // std::move
#include <vector>      // std::vector
// djinterp [ui]
#include "../state.hpp"  // state_source, axis, cell_address, path, maybe


NS_DJINTERP
NS_UI

NS_INTERNAL

    // no_index
    //   constant: a cell outside a visit_source's space.
    constexpr std::size_t no_index = static_cast<std::size_t>(-1);

NS_END  // internal

// visit_source
//   class: a state_source whose cell values are visit counts over time. A
// cell's value at a frame is the number of visits it had received up to and
// including that frame, or nothing if it had received none.
class visit_source final : public state_source
{
public:
    // path_fn
    //   type: the path at a frame.
    using path_fn = std::function<path(std::size_t)>;

    visit_source(
        std::vector<axis> _axes,
        std::size_t       _frame_count,
        path_fn           _paths = path_fn()
    )
        : m_axes(std::move(_axes)),
          m_frames(_frame_count),
          m_paths(std::move(_paths))
    {
        std::size_t cells = 1;

        // row-major strides over the axes' label counts
        m_strides.assign(m_axes.size(), 0);

        for (std::size_t i = m_axes.size(); i-- > 0; )
        {
            m_strides[i] = cells;
            cells       *= m_axes[i].count();
        }

        m_visits.resize(m_axes.empty() ? 0 : cells);
    }

    // visit
    //   records a visit to the cell at _indices (one per axis) in _frame.
    // Returns false, recording nothing, when the cell is outside the space.
    bool
    visit(
        const std::vector<std::size_t>& _indices,
        std::size_t                     _frame
    )
    {
        const std::size_t at = m_flat(_indices);

        // a cell outside the space cannot be visited
        if (at == internal::no_index)
        {
            return false;
        }

        std::vector<std::size_t>& frames = m_visits[at];

        // appending is the usual case; an earlier frame is inserted in order
        frames.insert(std::upper_bound(frames.begin(), frames.end(), _frame),
                      _frame);

        return true;
    }

    // count
    //   visits to the cell at _indices in frames up to and including _frame.
    D_NODISCARD std::size_t
    count(
        const std::vector<std::size_t>& _indices,
        std::size_t                     _frame
    ) const
    {
        const std::size_t at = m_flat(_indices);

        if (at == internal::no_index)
        {
            return 0;
        }

        const std::vector<std::size_t>& frames = m_visits[at];

        return static_cast<std::size_t>(
            std::upper_bound(frames.begin(), frames.end(), _frame) -
            frames.begin());
    }

    // total
    //   all visits to the cell at _indices.
    D_NODISCARD std::size_t
    total(
        const std::vector<std::size_t>& _indices
    ) const
    {
        const std::size_t at = m_flat(_indices);

        return (at == internal::no_index) ? 0 : m_visits[at].size();
    }

    // first_visit
    //   the frame of the first visit to the cell at _indices, or nothing if
    // it was never visited.
    D_NODISCARD maybe<std::size_t>
    first_visit(
        const std::vector<std::size_t>& _indices
    ) const
    {
        const std::size_t at = m_flat(_indices);

        if ( (at == internal::no_index) ||
             (m_visits[at].empty()) )
        {
            return nothing<std::size_t>();
        }

        return just(m_visits[at].front());
    }

    // cell_count
    //   the number of cells in the space.
    D_NODISCARD std::size_t
    cell_count() const
    {
        return m_visits.size();
    }

    D_NODISCARD std::size_t
    axis_count() const override
    {
        return m_axes.size();
    }

    D_NODISCARD std::size_t
    frame_count() const override
    {
        return m_frames;
    }

    D_NODISCARD const axis&
    axis_at(
        std::size_t _index
    ) const override
    {
        return m_axes[_index];
    }

    // value_at
    //   the visit count at _frame, as a scalar; nothing for an unvisited cell
    // and for any level but 0, since visits are recorded at full resolution.
    D_NODISCARD maybe<scalar>
    value_at(
        const cell_address& _cell,
        std::size_t         _frame
    ) const override
    {
        if (_cell.level != 0)
        {
            return nothing<scalar>();
        }

        const std::size_t n = count(_cell.indices, _frame);

        if (n == 0)
        {
            return nothing<scalar>();
        }

        return just(static_cast<scalar>(n));
    }

    // current_path
    //   the caller's path at _frame. The last path asked for is kept, so the
    // reference stays valid until a different frame is asked for.
    D_NODISCARD const path&
    current_path(
        std::size_t _frame
    ) const override
    {
        // no path function: every frame's path is empty
        if (!m_paths)
        {
            return m_path;
        }

        if (_frame != m_path_frame)
        {
            m_path       = m_paths(_frame);
            m_path_frame = _frame;
        }

        return m_path;
    }

private:
    // m_flat
    //   the index of the cell at _indices in m_visits, or internal::no_index.
    D_NODISCARD std::size_t
    m_flat(
        const std::vector<std::size_t>& _indices
    ) const
    {
        if ( (_indices.size() != m_axes.size()) ||
             (m_axes.empty()) )
        {
            return internal::no_index;
        }

        std::size_t at = 0;

        for (std::size_t i = 0; i < _indices.size(); ++i)
        {
            // an index past its axis' labels is outside the space
            if (_indices[i] >= m_axes[i].count())
            {
                return internal::no_index;
            }

            at += _indices[i] * m_strides[i];
        }

        return at;
    }

    std::vector<axis>                     m_axes;
    std::size_t                           m_frames;
    std::vector<std::size_t>              m_strides;
    std::vector<std::vector<std::size_t>> m_visits;
    path_fn                               m_paths;
    mutable path                          m_path;
    mutable std::size_t                   m_path_frame = internal::no_index;
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_SOURCES_VISIT_SOURCE_HPP
