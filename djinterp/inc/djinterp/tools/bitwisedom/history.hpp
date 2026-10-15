/*******************************************************************************
* bitwisedom [bitwisedom]                                            history.hpp
*
* Undo and redo, as whole-sheet snapshots.
*   A sheet is small, so each step keeps the sheet as it was before the
* edit. Edits that arrive as a stream -- a drag, typing into a field -- share
* a merge key, and a run of edits with one key is one step: dragging a
* colour across the picker is undone in one go, not frame by frame. seal()
* ends a run, so the next edit starts a step of its own even with the same
* key.
*
* path:      /inc/djinterp/tools/bitwisedom/history.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.10
*******************************************************************************/

#ifndef BITWISEDOM_HISTORY_HPP
#define BITWISEDOM_HISTORY_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./sheet.hpp"  // sheet


namespace bitwisedom
{

// history
//   class: the undo and redo stacks of one sheet.
class history
{
public:
    // HISTORY_LIMIT
    //   constant: how many steps are kept; the oldest go first.
    static const std::size_t HISTORY_LIMIT = 256;

    /**
     * @brief Records the sheet as it was before an edit.
     *
     * @param[in] _before  the sheet before the edit.
     * @param[in] _key     the edit's merge key; empty never merges.
     * @post   The redo stack is empty.
     */
    void record(const sheet&       _before,
                const std::string& _key);

    // stepping -- each returns false, leaving _current alone, when there is
    // nothing to step to
    bool undo(sheet& _current);
    bool redo(sheet& _current);

    void seal();
    void clear();

    D_NODISCARD bool        can_undo() const;
    D_NODISCARD bool        can_redo() const;
    D_NODISCARD std::size_t depth() const;

private:
    std::vector<sheet> m_undo;
    std::vector<sheet> m_redo;
    std::string        m_key;
};

}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_HISTORY_HPP
