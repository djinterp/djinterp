/*******************************************************************************
* bitwisedom [app]                                                       app.hpp
*
* The bitwisedom window: sheets, each in a window of its own, edited through
* uxoxo over Dear ImGui.
*   Each frame draws the realized interface and collects the events it
* posts; after the frame the events are applied, in order, by apply(). The
* interface is realized again only when the document in front, or the
* window around the documents, has changed since it was last realized --
* their versions say when -- so an idle frame costs a draw and nothing more.
*   An edit to a sheet goes through edit(): the sheet as it was is kept for
* undo under the edit's merge key, the change is made, and the sheet is
* resolved again. Edits to how a sheet is looked at -- the selection, the
* operand -- change its document without touching history, and edits to
* the window -- its workspaces, its tool windows, a window's zoom -- change
* the shell.
*   Every document belongs to one workspace, and the window shows the
* workspace in front: its documents and its tool windows, docked as the
* user left them. An event posted from a document's window carries that
* document's id, and brings it to the front before it is applied; the tool
* windows show, and act on, the document in front.
*   The host -- a GLFW or a Win32 main -- owns the ImGui context and the
* backend, and calls frame() between ImGui's NewFrame and Render. It gives
* the window preferences_path(), the settings file where ImGui keeps the
* layout of the windows and the widths of the tables' columns, and the
* window keeps its own preferences there beside them; and session_path(),
* where the window keeps its workspaces and documents -- unsaved changes
* too -- so that it opens where it was left. The window reads both before
* the first frame and writes them as they change and when it closes --
* itself, rather than through ImGui's IniFilename, so that a value ImGui
* could not read back is never kept.
*   The dialogs -- Save As, the definition files to import, closing a sheet
* with changes, naming a workspace, and Help's keyboard shortcuts and about
* box -- are the window's own ImGui popups, drawn after the interface.
*
* path:      /src/djinterp/tools/bitwisedom/app/app.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.19
*******************************************************************************/

#ifndef BITWISEDOM_APP_APP_HPP
#define BITWISEDOM_APP_APP_HPP 1

// std
#include <memory>  // std::unique_ptr
#include <string>  // std::string
#include <vector>  // std::vector
// uxoxo
#include <uxoxo/platform/imgui_platform.hpp>  // imgui_blueprint, event
// bitwisedom
#include "./model.hpp"  // model
#include "./shell.hpp"  // shell_state, workspace, tool


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
     *        in the settings file and the session file named.
     *
     * @pre    ImGui's context exists, with IniFilename null: the window
     *         reads and writes the file itself.
     *
     * @param[in] _preferences  the settings file -- preferences_path() for
     *                          a user's window; empty, for a screenshot or
     *                          a test, keeps nothing.
     * @param[in] _session      the session file -- session_path() for a
     *                          user's window; empty keeps no session, and
     *                          the window opens on an example.
     */
    explicit app(std::string _preferences = std::string(),
                 std::string _session     = std::string());
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

    // the document in front, and the window around the documents
    D_NODISCARD bool                quitting() const;
    D_NODISCARD const model&        state() const;
    D_NODISCARD const shell_state&  shell() const;

    // documents -- every one open, in every workspace, in the order opened;
    // one by its id, or nullptr
    D_NODISCARD std::vector<const model*>  documents() const;
    D_NODISCARD const model*               document(int _id) const;

    // fonts -- loads the interface and monospace fonts into ImGui's atlas;
    // call once, after the context is created and before the first frame
    static void load_fonts(float _size);

private:
    std::vector<std::unique_ptr<model>> m_docs;
    model*                           m_doc     = nullptr;  // in front
    int                              m_next    = 1;        // the next id
    int                              m_next_ws = 1;
    shell_state                      m_shell;
    uxoxo::platform::imgui_blueprint m_blueprint;
    int                              m_built       = -1;
    int                              m_built_doc   = -1;
    int                              m_built_shell = -1;
    int                              m_styled      = -1;
    std::vector<uxoxo::event>        m_queue;
    std::string                      m_preferences;
    std::string                      m_session;
    long                             m_session_mark = -1;   // last kept
    double                           m_session_time = 0.0;
    bool                             m_save_as_open = false;
    bool                             m_save_as_focus = false;
    std::string                      m_save_as_path;
    bool                             m_open_open  = false;
    bool                             m_open_focus = false;
    std::string                      m_open_path;
    int                              m_save_as_extension = 0; // h=0, hpp=1
    bool                             m_import_open = false;
    std::string                      m_import_paths;
    bool                             m_help_open = false;  // for one frame
    int                              m_help      = 0;      // 0 keys, 1 about
    int                              m_closing   = 0;      // asked to close
    bool                             m_closing_open = false;
    bool                             m_naming_open  = false; // a workspace
    std::string                      m_naming_text;
    std::vector<std::string>         m_forget;      // windows' settings

    void keep_preferences();
    void keep_session(bool _now);
    bool restore_session();

    // the documents and workspaces
    model&       doc();
    const model& doc() const;
    model&      open_document();
    model&      fresh_document();
    model*      find_document(int _id);
    void        activate(int  _id,
                         bool _raise = true);
    void        close_document(int  _id,
                               bool _discard);
    void        close_dialog();
    void        naming_dialog();
    workspace&  current();
    workspace*  holding(int _document);
    workspace&  add_workspace();
    void        to_workspace(std::size_t _index);
    void        close_workspace(std::size_t _index);
    void        move_document(int         _document,
                              std::size_t _index);
    void        tool_to(tool _tool,
                        bool _open,
                        bool _focus);
    void        shell_changed();
    void        restyle();
    std::string zoom_key() const;
    void        zoom(const std::string& _key,
                     float              _step);
    void        set_environment(const uxoxo::option_set& _payload);
    void        settings_file(const std::string& _kind,
                              const uxoxo::option_set& _payload);

    void apply(const uxoxo::event& _event);
    bool apply_shell(const uxoxo::event& _event);
    void changed();
    void status(const std::string& _text,
                bool               _error);

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
    void add_child(int                _parent,
                   const std::string& _text);
    void delete_rows();
    void duplicate_rows();
    void move_row_by(int _step);
    void sorted_note();
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
    void display_edit(const uxoxo::event& _event);
    void special_edit(const uxoxo::event& _event);
    void node_edit(const uxoxo::event& _event);
    void note_action(const uxoxo::event& _event);
    void open_node(int _parent);
    void refresh_node();
    void add_node(bool _close);
    void keep_sorted();
    void template_edit(const uxoxo::event& _event);
    void open_file(const std::string& _path);
    void save_file(int _extension = -1);
    void save_as_dialog();
    void open_dialog();
    void rename_file(const std::string& _text);
    void import_definitions_dialog();
    void help_dialog();
    std::string macro_header() const;
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

/**
 * @brief Where the window keeps its session -- its workspaces, its
 *        documents and their unsaved changes -- between runs: beside the
 *        settings file, as session.json.
 *
 * @return the session file's path, or the empty string when there is
 *         nowhere to keep one.
 */
std::string session_path();

// unique_name
//   a name no symbol in the sheet has: _base, or _base_2, _base_3, ...
std::string unique_name(const sheet&       _sheet,
                        const std::string& _base);

// range_choices
//   the range symbols a field_value may be placed in, in tree order.
std::vector<int> range_choices(const sheet& _sheet);

// container_choices
//   where a row may be added: 0, the sheet's own rows, then every group
// and range, in tree order.
std::vector<int> container_choices(const sheet& _sheet);

// NODE_KINDS / NODE_KIND_COUNT
//   the kinds of row the Add node window makes, as its Kind lists them:
// each definition, in def_kind's order, then a group.
extern const char* const NODE_KINDS[];
const int NODE_KIND_COUNT = 7;

// op_targets
//   what an operation acts on: the selected rows.
std::vector<int> op_targets(const model& _model);

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_APP_HPP
