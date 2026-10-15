/*******************************************************************************
* bitwisedom [app]                                                       app.hpp
*
* The bitwisedom window: one sheet, edited through uxoxo over Dear ImGui.
*   Each frame draws the realized interface and collects the events it
* posts; after the frame the events are applied, in order, by apply(). The
* interface is realized again only when the model has changed since it was
* last realized -- the model's version says when -- so an idle frame costs
* a draw and nothing more.
*   An edit to the sheet goes through edit(): the sheet as it was is kept for
* undo under the edit's merge key, the change is made, and the sheet is
* resolved again. Edits to how the window is used -- the selection, the
* panel sizes, the front tab -- change the model without touching history.
*   The host -- a GLFW or a Win32 main -- owns the ImGui context and the
* backend, and calls frame() between ImGui's NewFrame and Render. It gives
* the window preferences_path(), the settings file where ImGui keeps the
* widths of the tables' columns and the window the sizes of its panels; the
* window reads it before the first frame and writes it when ImGui's
* settings change and when it closes -- itself, rather than through ImGui's
* IniFilename, so that a value ImGui could not read back is never kept.
*
* path:      /src/djinterp/tools/bitwisedom/app/app.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

#ifndef BITWISEDOM_APP_APP_HPP
#define BITWISEDOM_APP_APP_HPP 1

// std
#include <string>  // std::string
#include <vector>  // std::vector
// uxoxo
#include <uxoxo/platform/imgui_platform.hpp>  // imgui_blueprint, event
// bitwisedom
#include "./model.hpp"  // model


namespace bitwisedom
{
namespace gui
{

// app
//   class: the window's state, and the loop step that draws and edits it.
class app
{
public:
    /**
     * @brief Makes the window, which keeps what it remembers between runs
     *        in the settings file named.
     *
     * @pre    ImGui's context exists, with IniFilename null: the window
     *         reads and writes the file itself.
     *
     * @param[in] _preferences  the settings file -- preferences_path() for
     *                          a user's window; empty, for a screenshot or
     *                          a test, keeps nothing.
     */
    explicit app(std::string _preferences = std::string());
    ~app();

    app(const app&)            = delete;
    app& operator=(const app&) = delete;

    /**
     * @brief Draws one frame and applies what it posted.
     *
     * @pre    ImGui::NewFrame has been called for this frame, and the fonts
     *         have been loaded with load_fonts().
     * @post   The model reflects every event posted during the frame.
     */
    void frame();

    // queueing -- an event applied before the next frame, as if posted
    void post(const uxoxo::event& _event);

    D_NODISCARD bool          quitting() const;
    D_NODISCARD const model&  state() const;

    // fonts -- loads the interface and monospace fonts into ImGui's atlas;
    // call once, after the context is created and before the first frame
    static void load_fonts(float _size);

private:
    model                            m_model;
    uxoxo::platform::imgui_blueprint m_blueprint;
    int                              m_built  = -1;
    int                              m_styled = -1;
    bool                             m_remembered = false;
    std::vector<uxoxo::event>        m_queue;
    std::string                      m_preferences;

    void keep_preferences();

    void apply(const uxoxo::event& _event);
    void changed();
    void status(const std::string& _text,
                bool               _error);
    void panel_to(panel_mode _mode);

    template<typename Change>
    void edit(const std::string& _key,
              Change             _change);

    template<typename Change>
    void quietly(Change _change);

    void shortcuts(uxoxo::platform::imgui_frame& _frame);

    // actions
    void select(int _id);
    void open_all(int  _group,
                  bool _open);
    void add_row(int _kind);
    void delete_rows();
    void duplicate_rows();
    void move_row_by(int _step);
    void drop_row(int _dragged,
                  int _onto);
    void grid_toggle(int _id,
                     int _bit);
    void grid_span(int _id,
                   int _low,
                   int _high,
                   bool _set);
    void collapse_segment(const uxoxo::option_set& _payload);
    void row_edit(const uxoxo::event& _event);
    void segment_edit(const uxoxo::event& _event);
    void template_edit(const uxoxo::event& _event);
    void open_file();
    void save_file();
    void export_header();
    void export_json(bool _everything);
    void import_json_text(const std::string& _path,
                          const std::string& _text);
    void load_sheet_into(sheet _sheet,
                         const std::string& _path);
    void apply_operation();

    // the context menu, renames, and code in and out
    void open_menu(const std::string&       _kind,
                   const std::string&       _where,
                   const uxoxo::option_set& _payload);
    void begin_rename(int                _id,
                      const std::string& _where);
    void finish_rename(int                _id,
                       const std::string& _text);
    void begin_segment_rename(const uxoxo::option_set& _payload);
    void finish_segment_rename(const std::string& _text);
    void copy_code(int _id);
    void paste_code(int _id);
    void import_code_text(const std::string& _path,
                          const std::string& _text);
    void grid_width(const uxoxo::option_set& _payload);
};

/**
 * @brief Where the window keeps what it remembers between runs: the
 *        widths of its tables' columns and the sizes of its panels.
 *
 * @note %APPDATA%\bitwisedom on Windows; elsewhere $XDG_CONFIG_HOME/
 *       bitwisedom, or ~/.config/bitwisedom. The directory is made if it
 *       is not there.
 *
 * @return the settings file's path, or the empty string when there is
 *         nowhere to keep one -- and then nothing is kept.
 */
std::string preferences_path();

// unique_name
//   a name no symbol in the sheet has: _base, or _base_2, _base_3, ...
std::string unique_name(const sheet&       _sheet,
                        const std::string& _base);

// range_choices
//   the range symbols a field_value may be placed in, in tree order.
std::vector<int> range_choices(const sheet& _sheet);

// op_targets
//   what an operation acts on: the selected rows.
std::vector<int> op_targets(const model& _model);

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_APP_HPP
