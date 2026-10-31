#include <curses.h>
#include <panel.h>
#include <string>
#include <vector>
#include <functional>

#define MENU_HEIGHT 1
#define FUNCTION_KEY_HEIGHT 2

    
struct TreeNode
{
    std::string name;
    bool checked;
    std::vector<TreeNode> children;

    TreeNode(const std::string& n) : name(n), checked(false) {}
    TreeNode(const std::string& n, const std::vector<TreeNode>& c)
        : name(n), checked(false), children(c) {}
};

class TreeView
{
private:
    WINDOW * win;
    std::vector<TreeNode>& nodes;
    int selected = 0;
    int scroll = 0;
    int max_display;

    void draw_node(
        const TreeNode& node,
        int             depth,
        int&            index
    )
    {
        if (index >= scroll && index < scroll + max_display)
        {
            int y = index - scroll;
            mvwprintw(win, y + 1, depth * 2 + 1, "%s[%c] %s",
                index == selected ? ">" : " ",
                node.checked ? 'X' : ' ',
                node.name.c_str());
        }
        index++;
        for (const auto& child : node.children)
        {
            draw_node(child, depth + 1, index);
        }
    }

    void toggle_check(
        TreeNode& node
    )
    {
        node.checked = !node.checked;
        for (auto& child : node.children)
        {
            toggle_check(child);
        }
    }

    int count_nodes(
        const TreeNode& node
    )
    {
        int count = 1;
        for (const auto& child : node.children)
        {
            count += count_nodes(child);
        }
        return count;
    }

public:
    TreeView(WINDOW * w, std::vector<TreeNode>& n) : win(w), nodes(n)
    {
        int height, width;
        getmaxyx(win, height, width);
        max_display = height - 2;
    }

    void draw()
    {
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 0, 2, " Select Components ");
        int index = 0;
        for (const auto& node : nodes)
        {
            draw_node(node, 0, index);
        }
        wrefresh(win);
    }

    void handle_input(
        int ch
    )
    {
        int total_nodes = 0;
        for (const auto& node : nodes)
        {
            total_nodes += count_nodes(node);
        }

        switch (ch)
        {
        case KEY_UP:
            if (selected > 0)
            {
                selected--;
                if (selected < scroll) scroll = selected;
            }
            break;
        case KEY_DOWN:
            if (selected < total_nodes - 1)
            {
                selected++;
                if (selected >= scroll + max_display) scroll = selected - max_display + 1;
            }
            break;
        case ' ':  // Toggle checkbox
        {
            int index = 0;
            std::function<bool(TreeNode&)> toggle_selected = [&](TreeNode& node) -> bool
                {
                    if (index == selected)
                    {
                        toggle_check(node);
                        return true;
                    }
                    index++;
                    for (auto& child : node.children)
                    {
                        if (toggle_selected(child)) return true;
                    }
                    return false;
                };
            for (auto& node : nodes)
            {
                if (toggle_selected(node)) break;
            }
        }
        break;
        }
    }
};

int max_y,
max_x;
WINDOW * menu_win,
       * left_panel,
       * right_panel, 
       * function_keys_win, 
       * popup_win;
PANEL * popup_panel;


void draw_menu(
    int                             selected_item,
    const std::vector<std::string>& menu_options
)
{
    int x = 2;
    wmove(menu_win, 0, 0);
    wclrtoeol(menu_win);
    for (size_t i = 0; i < menu_options.size(); ++i)
    {
        if (i == selected_item)
            wattron(menu_win, A_REVERSE);
        mvwprintw(menu_win, 0, x, menu_options[i].c_str());
        wattroff(menu_win, A_REVERSE);
        x += menu_options[i].length() + 2;
    }
    wrefresh(menu_win);
}

void draw_left_menu(
    int                             selected_item,
    const std::vector<std::string>& menu_options
)
{
    wclear(left_panel);
    box(left_panel, 0, 0);
    mvwprintw(left_panel, 0, 2, " Left Menu ");
    for (size_t i = 0; i < menu_options.size(); ++i)
    {
        if (i == selected_item)
        {
            wattron(left_panel, A_REVERSE);
        }
            
        mvwprintw(left_panel, i + 1, 2, menu_options[i].c_str());
        wattroff(left_panel, A_REVERSE);
    }
    wrefresh(left_panel);
}

void draw_right_panel()
{
    wclear(right_panel);
    box(right_panel, 0, 0);
    mvwprintw(right_panel, 0, 2, " Right Panel ");
    mvwprintw(right_panel, 1, 1, "This is the right panel content");
    wrefresh(right_panel);
}

void draw_function_keys()
{
    wclear(function_keys_win);

    // Define a new color pair for black background and white text
    init_pair(3, COLOR_WHITE, COLOR_BLACK);

    const char* function_keys[] = 
    {
        "Help", "Menu", "View", "Edit", "Copy", "Move", "Mkdir", "Delete", "PullDn", "Quit"
    };

    int x = 0;
    for (int i = 0; i < 10; i++)
    {
        // Draw a space with black background
        wattron(function_keys_win, COLOR_PAIR(3));
        mvwprintw(function_keys_win, 0, x, " ");
        x++;

        // Draw the number, still with black background
        mvwprintw(function_keys_win, 0, x, "%d", i + 1);
        wattroff(function_keys_win, COLOR_PAIR(3));

        // Draw the function name with default background
        x += (i < 9) ? 1 : 2; // Adjust for two-digit number
        mvwprintw(function_keys_win, 0, x, "%s", function_keys[i]);

        x += strlen(function_keys[i]) + 1; // Move to the next function key
    }

    mvwprintw(function_keys_win, 1, 0, "a:Check All  u:Uncheck All  q:Quit");
    wrefresh(function_keys_win);
}

int main()
{
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    if (can_change_color() && COLORS >= 256)
    {
        // Define our custom teal color for the menu bar
        int r_teal = 0, g_teal = 128 * 1000 / 255, b_teal = 128 * 1000 / 255;
        init_color(COLOR_CYAN, r_teal, g_teal, b_teal);
        init_pair(1, COLOR_WHITE, COLOR_CYAN);  // Changed to white text

        // Define our custom deep blue color for the panels
        int r_blue = 0, g_blue = 0, b_blue = 128 * 1000 / 255;
        init_color(COLOR_BLUE, r_blue, g_blue, b_blue);
        init_pair(2, COLOR_WHITE, COLOR_BLUE);

        // Add a new color pair for the function key numbers
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
    }
    else
    {
        init_pair(1, COLOR_WHITE, COLOR_CYAN);
        init_pair(2, COLOR_WHITE, COLOR_BLUE);
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
    }

    getmaxyx(stdscr, max_y, max_x);
    int panel_height = max_y - MENU_HEIGHT - FUNCTION_KEY_HEIGHT;
    int panel_width = max_x / 2;

    menu_win = newwin(MENU_HEIGHT, max_x, 0, 0);
    left_panel = newwin(panel_height, panel_width, MENU_HEIGHT, 0);
    right_panel = newwin(panel_height, panel_width, MENU_HEIGHT, panel_width);
    function_keys_win = newwin(FUNCTION_KEY_HEIGHT, max_x, max_y - FUNCTION_KEY_HEIGHT, 0);

    wbkgd(menu_win, COLOR_PAIR(1));
    wbkgd(left_panel, COLOR_PAIR(2));
    wbkgd(right_panel, COLOR_PAIR(2));
    wbkgd(function_keys_win, COLOR_PAIR(1));

    std::vector<std::string> top_menu_options = 
    {
        "Left", "File", "Command", "Options", "Right"
    };

    std::vector<std::string> left_menu_options = 
    {
        "Option 1", "Option 2", "Select Components", "Option 4", "Option 5"
    };

    std::vector<TreeNode> nodes = 
    {
        TreeNode("Component 1", 
        {
            TreeNode("Subcomponent 1.1"),
            TreeNode("Subcomponent 1.2", 
            {
                TreeNode("Sub-subcomponent 1.2.1"),
                TreeNode("Sub-subcomponent 1.2.2")
            })
        }),
        TreeNode("Component 2", 
        {
            TreeNode("Subcomponent 2.1"),
            TreeNode("Subcomponent 2.2")
        })
    };

    int popup_height = 20;
    int popup_width = 40;
    popup_win = newwin(popup_height, popup_width, (max_y - popup_height) / 2, (max_x - popup_width) / 2);
    popup_panel = new_panel(popup_win);
    hide_panel(popup_panel);

    TreeView tree_view(popup_win, nodes);

    int selected_top_menu = 0;
    int selected_left_menu = 0;
    bool in_popup = false;
    int ch;
    while ((ch = getch()) != 'q')
    {
        if (in_popup)
        {
            if (ch == 27)  // ESC key
            {
                in_popup = false;
                hide_panel(popup_panel);
            }
            else
            {
                tree_view.handle_input(ch);
                tree_view.draw();
            }
        }
        else
        {
            switch (ch)
            {
            case KEY_LEFT:
                if (selected_top_menu > 0) selected_top_menu--;
                break;
            case KEY_RIGHT:
                if (selected_top_menu < top_menu_options.size() - 1) selected_top_menu++;
                break;
            case KEY_UP:
                if (selected_left_menu > 0) selected_left_menu--;
                break;
            case KEY_DOWN:
                if (selected_left_menu < left_menu_options.size() - 1) selected_left_menu++;
                break;
            case 10:  // Enter key
                if (selected_left_menu == 2)  // "Select Components" option
                {
                    in_popup = true;
                    show_panel(popup_panel);
                    tree_view.draw();
                }
                break;
            }
        }

        if (!in_popup)
        {
            draw_menu(selected_top_menu, top_menu_options);
            draw_left_menu(selected_left_menu, left_menu_options);
            draw_right_panel();
            draw_function_keys();  // Make sure this line is present
        }
        update_panels();
        doupdate();
    }

    endwin();
    return 0;
}