/*******************************************************************************
* bitwisedom [bitwisedom]                                            history.cpp
*
* Undo and redo, as whole-sheet snapshots.
*
* path:      /src/djinterp/tools/bitwisedom/history.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.10
*******************************************************************************/
#include <djinterp/tools/bitwisedom/history.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <utility>  // std::move


namespace bitwisedom
{

void
history::record(
    const sheet&       _before,
    const std::string& _key
)
{
    m_redo.clear();

    // a run of edits with one key is one step
    if ( (!_key.empty()) &&
         (_key == m_key) &&
         (!m_undo.empty()) )
    {
        return;
    }

    m_undo.push_back(_before);
    m_key = _key;

    if (m_undo.size() > HISTORY_LIMIT)
    {
        m_undo.erase(m_undo.begin());
    }

    return;
}

bool
history::undo(
    sheet& _current
)
{
    if (m_undo.empty())
    {
        return false;
    }

    m_redo.push_back(std::move(_current));
    _current = std::move(m_undo.back());
    m_undo.pop_back();
    m_key.clear();

    return true;
}

bool
history::redo(
    sheet& _current
)
{
    if (m_redo.empty())
    {
        return false;
    }

    m_undo.push_back(std::move(_current));
    _current = std::move(m_redo.back());
    m_redo.pop_back();
    m_key.clear();

    return true;
}

void
history::seal()
{
    m_key.clear();

    return;
}

void
history::clear()
{
    m_undo.clear();
    m_redo.clear();
    m_key.clear();

    return;
}

bool
history::can_undo() const
{
    return !m_undo.empty();
}

bool
history::can_redo() const
{
    return !m_redo.empty();
}

std::size_t
history::depth() const
{
    return m_undo.size();
}

}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
