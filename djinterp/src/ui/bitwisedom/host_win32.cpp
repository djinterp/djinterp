/*******************************************************************************
* bitwisedom [ui]                                                 host_win32.cpp
*
* The window's host on Windows: Win32 and Direct3D 11, with nothing to
* install.
*   The device and swap-chain code follows Dear ImGui's own
* example_win32_directx11, the reference for this backend pair, with the
* app's frame in the middle of the loop. The window is DPI-aware and the
* interface is scaled to the monitor it opens on, unless `bitwisedom gui`
* was given a scale; its size is the one asked, in points at that scale.
* The events `bitwisedom gui` was given -- the sheets named, each --do --
* are posted before the first frame.
*   The window keeps the user's settings file, at preferences_path() -- the
* layout of its windows, the tables' column widths and the user's own
* preferences last from one run to the next -- and its session, at
* session_path(): its workspaces and every sheet in them, unsaved changes
* too, so it opens where it was left. A file dropped on the window is
* opened as File > Open opens one.
*   --warp draws with Microsoft's software rasterizer, WARP, whatever the
* GPU, and says so in the title: what looks wrong on the GPU and right with
* WARP is the GPU's driver, not the drawing.
*   The window's class is registered under the module's own handle, since
* the module, not the program, holds its procedure; and a window that
* cannot be opened is said through the module's program, as an error that
* ends the run in trouble.
*
* path:      /src/ui/bitwisedom/host_win32.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "../../../inc/ui/bitwisedom/host.hpp"  // corresponding header
// windows
#ifndef NOMINMAX
    #define NOMINMAX 1
#endif  // NOMINMAX
#include <windows.h>   // the window and its message loop
#include <shellapi.h>  // DragAcceptFiles, DragQueryFileW
#include <d3d11.h>     // the device, context and swap chain
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::snprintf
#include <string>   // std::string, std::wstring
#include <vector>   // std::vector
// imgui
#include <imgui.h>             // ImGui
#include <imgui_impl_dx11.h>   // the Direct3D 11 renderer backend
#include <imgui_impl_win32.h>  // the Win32 platform backend
// djinterp
#include <djinterp/parse/cli/cli.hpp>  // program, severity, exit statuses
// bitwisedom
#include "../../../inc/ui/bitwisedom/app.hpp"    // app, preferences_path
#include "../../../inc/ui/bitwisedom/theme.hpp"  // dpi_scale


// the backend's message handler, which its header declares commented out
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND   _window,
                                                             UINT   _message,
                                                             WPARAM _w,
                                                             LPARAM _l);


namespace
{

namespace kit = ::djinterp::parse::cli;

using bitwisedom::gui::app;
using bitwisedom::gui::window_options;

// CLASS_NAME
//   constant: the window class's name.
const wchar_t* const CLASS_NAME = L"bitwisedom";

// device
//   struct: the Direct3D objects the window draws with.
struct device
{
    ID3D11Device*           d3d       = nullptr;
    ID3D11DeviceContext*    context   = nullptr;
    IDXGISwapChain*         chain     = nullptr;
    ID3D11RenderTargetView* target    = nullptr;
    bool                    occluded  = false;
    UINT                    resize_w  = 0;
    UINT                    resize_h  = 0;
};

device&
the_device()
{
    static device state;

    return state;
}

void
create_target()
{
    device&          d    = the_device();
    ID3D11Texture2D* back = nullptr;

    d.chain->GetBuffer(0, IID_PPV_ARGS(&back));

    if (back != nullptr)
    {
        d.d3d->CreateRenderTargetView(back, nullptr, &d.target);
        back->Release();
    }

    return;
}

void
release_target()
{
    device& d = the_device();

    if (d.target != nullptr)
    {
        d.target->Release();
        d.target = nullptr;
    }

    return;
}

/*
create_device
  The device and its swap chain on the GPU -- or, asked for or with no
hardware device, on WARP: S_OK, or the HRESULT that stopped it.
*/
HRESULT
create_device(
    HWND _window,
    bool _warp
)
{
    DXGI_SWAP_CHAIN_DESC description = {};

    description.BufferCount                        = 2;
    description.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.BufferDesc.RefreshRate.Numerator   = 60;
    description.BufferDesc.RefreshRate.Denominator = 1;
    description.Flags       = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.OutputWindow       = _window;
    description.SampleDesc.Count   = 1;
    description.SampleDesc.Quality = 0;
    description.Windowed           = TRUE;
    description.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0,
                                         D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL       chosen   = D3D_FEATURE_LEVEL_11_0;
    device&                 d        = the_device();
    D3D_DRIVER_TYPE         driver   = _warp ? D3D_DRIVER_TYPE_WARP
                                             : D3D_DRIVER_TYPE_HARDWARE;
    HRESULT                 result   = DXGI_ERROR_UNSUPPORTED;

    // no hardware device: Microsoft's software rasterizer will do
    for (int attempt = 0;
         ( (attempt < 2) && (result == DXGI_ERROR_UNSUPPORTED) );
         ++attempt)
    {
        result = D3D11CreateDeviceAndSwapChain(nullptr,
                                               driver,
                                               nullptr,
                                               0,
                                               levels,
                                               2,
                                               D3D11_SDK_VERSION,
                                               &description,
                                               &d.chain,
                                               &d.d3d,
                                               &chosen,
                                               &d.context);
        driver = D3D_DRIVER_TYPE_WARP;
    }

    if (result == S_OK)
    {
        create_target();
    }

    return result;
}

void
destroy_device()
{
    device& d = the_device();

    release_target();

    if (d.chain != nullptr)
    {
        d.chain->Release();
        d.chain = nullptr;
    }

    if (d.context != nullptr)
    {
        d.context->Release();
        d.context = nullptr;
    }

    if (d.d3d != nullptr)
    {
        d.d3d->Release();
        d.d3d = nullptr;
    }

    return;
}

// dropped
//   the files dropped on the window since the last frame, as UTF-8 paths.
std::vector<std::string>&
dropped()
{
    static std::vector<std::string> held;

    return held;
}

// utf8_of
//   a wide string as UTF-8.
std::string
utf8_of(
    const std::wstring& _wide
)
{
    if (_wide.empty())
    {
        return std::string();
    }

    const int   length = static_cast<int>(_wide.size());
    const int   size   = ::WideCharToMultiByte(CP_UTF8, 0, _wide.data(),
                                               length, nullptr, 0, nullptr,
                                               nullptr);
    std::string out(static_cast<std::size_t>((size > 0) ? size : 0), '\0');

    if (size > 0)
    {
        ::WideCharToMultiByte(CP_UTF8, 0, _wide.data(), length, &out[0],
                              size, nullptr, nullptr);
    }

    return out;
}

/*
take_drop
  Each file dropped, its path kept for the loop to open.
*/
void
take_drop(
    HDROP _drop
)
{
    const UINT count = ::DragQueryFileW(_drop, 0xFFFFFFFFu, nullptr, 0);

    for (UINT i = 0; i < count; ++i)
    {
        const UINT   length = ::DragQueryFileW(_drop, i, nullptr, 0);
        std::wstring wide(static_cast<std::size_t>(length) + 1u, L'\0');

        ::DragQueryFileW(_drop, i, &wide[0], length + 1u);
        wide.resize(length);
        dropped().push_back(utf8_of(wide));
    }

    ::DragFinish(_drop);

    return;
}

LRESULT WINAPI
window_procedure(
    HWND   _window,
    UINT   _message,
    WPARAM _w,
    LPARAM _l
)
{
    if (ImGui_ImplWin32_WndProcHandler(_window, _message, _w, _l))
    {
        return 1;
    }

    switch (_message)
    {
        case WM_SIZE:
            // the resize happens in the loop, between frames
            if (_w != SIZE_MINIMIZED)
            {
                the_device().resize_w = static_cast<UINT>(LOWORD(_l));
                the_device().resize_h = static_cast<UINT>(HIWORD(_l));
            }

            return 0;

        case WM_SYSCOMMAND:
            // no Alt menu: Alt belongs to the shortcuts
            if ((_w & 0xFFF0) == SC_KEYMENU)
            {
                return 0;
            }

            break;

        case WM_DROPFILES:
            take_drop(reinterpret_cast<HDROP>(_w));

            return 0;

        case WM_DESTROY:
            ::PostQuitMessage(0);

            return 0;

        default:
            break;
    }

    return ::DefWindowProcW(_window, _message, _w, _l);
}

/*
this_module
  The module this code is in -- the window's, not the program's -- found by
an address inside it, as a window class whose procedure it holds is
registered under it.
*/
HINSTANCE
this_module()
{
    HMODULE found = nullptr;

    if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCWSTR>(&window_procedure),
                              &found))
    {
        found = ::GetModuleHandleW(nullptr);
    }

    return found;
}

/*
cannot_open
  The window could not be opened: said, with the code that says why;
trouble.
*/
int
cannot_open(
    const kit::program& _program,
    const char*         _what,
    unsigned long       _code
)
{
    char words[96];

    (void)std::snprintf(words,
                        sizeof(words),
                        "cannot open the window: %s (0x%08lx)",
                        _what,
                        _code);
    _program.report(kit::severity::error,
                    words);

    return D_CLI_EXIT_TROUBLE;
}

/*
open_window
  The window, its size the one asked in points at the display's scale, and
its device: shown, taking dropped files. NULL, everything let go, and
said, when either cannot be made.
*/
HWND
open_window(
    const kit::program&   _program,
    const window_options& _options,
    HINSTANCE             _module,
    float                 _scale
)
{
    HWND window = ::CreateWindowW(CLASS_NAME,
                                  _options.warp
                                      ? L"bitwis(e)dom -- software rendering"
                                      : L"bitwis(e)dom",
                                  WS_OVERLAPPEDWINDOW,
                                  100,
                                  80,
                                  static_cast<int>(
                                      static_cast<float>(_options.width) *
                                      _scale),
                                  static_cast<int>(
                                      static_cast<float>(_options.height) *
                                      _scale),
                                  nullptr,
                                  nullptr,
                                  _module,
                                  nullptr);

    if (window == nullptr)
    {
        (void)cannot_open(_program,
                          "no window",
                          ::GetLastError());

        return nullptr;
    }

    const HRESULT result = create_device(window,
                                         _options.warp);

    if (result != S_OK)
    {
        destroy_device();
        ::DestroyWindow(window);
        (void)cannot_open(_program,
                          "no Direct3D 11 device",
                          static_cast<unsigned long>(result));

        return nullptr;
    }

    ::DragAcceptFiles(window, TRUE);
    ::ShowWindow(window, SW_SHOWDEFAULT);
    ::UpdateWindow(window);

    return window;
}

/*
start_imgui
  ImGui's context and backends, and the fonts at the display's scale. The
window keeps its own settings file, so ImGui keeps none.
*/
void
start_imgui(
    HWND  _window,
    float _scale
)
{
    bitwisedom::gui::dpi_scale() = _scale;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplWin32_Init(_window);
    ImGui_ImplDX11_Init(the_device().d3d, the_device().context);
    bitwisedom::gui::app::load_fonts(16.0f * _scale);

    return;
}

/*
take_messages
  The messages waiting, dispatched: false once the window has gone.
*/
bool
take_messages()
{
    bool running = true;
    MSG  message;

    while (::PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&message);
        ::DispatchMessage(&message);

        if (message.message == WM_QUIT)
        {
            running = false;
        }
    }

    return running;
}

/*
draw_frame
  One frame: the files dropped since the last posted to the app; nothing
drawn while the window is hidden, but a short sleep; the swap chain resized
to the window first if it changed; then the app between NewFrame and
Render, drawn over the theme's background and presented.
*/
void
draw_frame(
    app& _app
)
{
    for (const std::string& each : dropped())
    {
        uxoxo::event made;

        made.kind = "file.drop";
        made.payload.set("value", each);
        _app.post(made);
    }

    dropped().clear();

    device& d = the_device();

    if ( (d.occluded) &&
         (d.chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) )
    {
        ::Sleep(10);

        return;
    }

    d.occluded = false;

    if ( (d.resize_w != 0) &&
         (d.resize_h != 0) )
    {
        release_target();
        d.chain->ResizeBuffers(0, d.resize_w, d.resize_h,
                               DXGI_FORMAT_UNKNOWN, 0);
        d.resize_w = 0;
        d.resize_h = 0;
        create_target();
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    _app.frame();
    ImGui::Render();

    const ImVec4 clear   = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    const float  rgba[4] = { clear.x, clear.y, clear.z, 1.0f };

    d.context->OMSetRenderTargets(1, &d.target, nullptr);
    d.context->ClearRenderTargetView(d.target, rgba);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    d.occluded = (d.chain->Present(1, 0) == DXGI_STATUS_OCCLUDED);

    return;
}

/*
run_frames
  The app, with its settings and its session, its events posted, and its
frames until the window has gone -- closed by the user, or by the app
quitting.
*/
void
run_frames(
    HWND                  _window,
    const window_options& _options
)
{
    app bitwisedom(bitwisedom::gui::preferences_path(),
                   bitwisedom::gui::session_path());

    for (const uxoxo::event& each : _options.events)
    {
        bitwisedom.post(each);
    }

    while (take_messages())
    {
        draw_frame(bitwisedom);

        if (bitwisedom.quitting())
        {
            ::PostMessage(_window, WM_CLOSE, 0, 0);
        }
    }

    return;
}

}  // anonymous namespace


/*
window_features
  WARP is Direct3D's; a screenshot would read Direct3D's back buffer, which
this host does not.
*/
int
bitwisedom::gui::window_features()
{
    return static_cast<int>(window_feature_warp);
}

/*
run_window
  The process made DPI-aware, the scale chosen -- the one asked, else the
primary monitor's -- the class registered, the window opened and run, and
everything let go in the order it was made.
*/
int
bitwisedom::gui::run_window(
    kit::program&         _program,
    const window_options& _options
)
{
    ImGui_ImplWin32_EnableDpiAwareness();

    const POINT     origin  = { 0, 0 };
    const HMONITOR  primary = ::MonitorFromPoint(origin,
                                                 MONITOR_DEFAULTTOPRIMARY);
    const float     scale   = (_options.scale > 0.0f)
                                  ? _options.scale
                                  : ImGui_ImplWin32_GetDpiScaleForMonitor(
                                        primary);
    const HINSTANCE module  = this_module();
    WNDCLASSEXW     type    = { sizeof(type),
                                CS_CLASSDC,
                                window_procedure,
                                0L,
                                0L,
                                module,
                                nullptr,
                                nullptr,
                                nullptr,
                                nullptr,
                                CLASS_NAME,
                                nullptr };

    ::RegisterClassExW(&type);

    HWND window = open_window(_program,
                              _options,
                              module,
                              scale);

    if (window == nullptr)
    {
        ::UnregisterClassW(CLASS_NAME,
                           module);

        return D_CLI_EXIT_TROUBLE;
    }

    start_imgui(window,
                scale);
    run_frames(window,
               _options);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    destroy_device();
    ::DestroyWindow(window);
    ::UnregisterClassW(CLASS_NAME,
                       module);

    return D_CLI_EXIT_OK;
}
