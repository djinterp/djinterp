/*******************************************************************************
* bitwisedom [app]                                                host_win32.cpp
*
* bitwisedom's main on Windows: Win32 and Direct3D 11, with nothing to install.
*   The device and swap-chain code follows Dear ImGui's own
* example_win32_directx11, the reference for this backend pair, with the
* app's frame in the middle of the loop. The window is DPI-aware and the
* interface is scaled to the monitor it opens on.
*   The window keeps the user's settings file, at preferences_path() -- the
* layout of its windows, the tables' column widths and the user's own
* preferences last from one run to the next -- and its session, at
* session_path(): its workspaces and every sheet in them, unsaved changes
* too, so it opens where it was left. A file dropped on the window is
* opened as File > Open opens one.
*   --warp on the command line draws with Microsoft's software rasterizer,
* WARP, whatever the GPU, and says so in the title: what looks wrong on the
* GPU and right with WARP is the GPU's driver, not the drawing.
*
* path:      /src/djinterp/tools/bitwisedom/app/host_win32.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.19
*******************************************************************************/
// windows
#ifndef NOMINMAX
    #define NOMINMAX 1
#endif  // NOMINMAX
#include <windows.h>   // the window and its message loop
#include <shellapi.h>  // DragAcceptFiles, DragQueryFileW
#include <d3d11.h>     // the device, context and swap chain
// std
#include <cstddef>  // std::size_t
#include <cstring>  // std::strstr
#include <string>   // std::string, std::wstring
#include <vector>   // std::vector
// imgui
#include <imgui.h>              // ImGui
#include <imgui_impl_dx11.h>    // the Direct3D 11 renderer backend
#include <imgui_impl_win32.h>   // the Win32 platform backend
// bitwisedom
#include "./app.hpp"    // app
#include "./theme.hpp"  // dpi_scale


// the backend's message handler, which its header declares commented out
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND   _window,
                                                             UINT   _message,
                                                             WPARAM _w,
                                                             LPARAM _l);


namespace
{

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

bool
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

    HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr,
                                                   _warp
                                                       ? D3D_DRIVER_TYPE_WARP
                                                       : D3D_DRIVER_TYPE_HARDWARE,
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

    // no hardware device: Microsoft's software rasterizer will do
    if (result == DXGI_ERROR_UNSUPPORTED)
    {
        result = D3D11CreateDeviceAndSwapChain(nullptr,
                                               D3D_DRIVER_TYPE_WARP,
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
    }

    if (result != S_OK)
    {
        return false;
    }

    create_target();

    return true;
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
        {
            // each file's path, for the loop to open
            HDROP      drop  = reinterpret_cast<HDROP>(_w);
            const UINT count = ::DragQueryFileW(drop, 0xFFFFFFFFu, nullptr,
                                                0);

            for (UINT i = 0; i < count; ++i)
            {
                const UINT   length = ::DragQueryFileW(drop, i, nullptr, 0);
                std::wstring wide(static_cast<std::size_t>(length) + 1u,
                                  L'\0');

                ::DragQueryFileW(drop, i, &wide[0], length + 1u);
                wide.resize(length);
                dropped().push_back(utf8_of(wide));
            }

            ::DragFinish(drop);

            return 0;
        }

        case WM_DESTROY:
            ::PostQuitMessage(0);

            return 0;

        default:
            break;
    }

    return ::DefWindowProcW(_window, _message, _w, _l);
}

}  // anonymous namespace


int WINAPI
WinMain(
    HINSTANCE _instance,
    HINSTANCE /*_previous*/,
    LPSTR     _command,
    int       /*_show*/
)
{
    // --warp: Microsoft's software rasterizer, whatever the GPU -- to tell
    // a fault of the GPU's driver from one of the drawing
    const bool warp = ( (_command != nullptr) &&
                        (std::strstr(_command, "--warp") != nullptr) );

    ImGui_ImplWin32_EnableDpiAwareness();

    const POINT   origin = { 0, 0 };
    const float   scale  = ImGui_ImplWin32_GetDpiScaleForMonitor(
        ::MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY));
    WNDCLASSEXW   type   = { sizeof(type),
                             CS_CLASSDC,
                             window_procedure,
                             0L,
                             0L,
                             _instance,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             L"bitwisedom",
                             nullptr };

    ::RegisterClassExW(&type);

    HWND window = ::CreateWindowW(type.lpszClassName,
                                  warp ? L"bitwis(e)dom -- software rendering"
                                       : L"bitwis(e)dom",
                                  WS_OVERLAPPEDWINDOW,
                                  100,
                                  80,
                                  static_cast<int>(1600 * scale),
                                  static_cast<int>(960 * scale),
                                  nullptr,
                                  nullptr,
                                  type.hInstance,
                                  nullptr);

    if (!create_device(window, warp))
    {
        destroy_device();
        ::UnregisterClassW(type.lpszClassName, type.hInstance);

        return 1;
    }

    ::DragAcceptFiles(window, TRUE);
    ::ShowWindow(window, SW_SHOWDEFAULT);
    ::UpdateWindow(window);

    bitwisedom::gui::dpi_scale() = scale;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // the window keeps its own settings file
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(the_device().d3d, the_device().context);
    bitwisedom::gui::app::load_fonts(16.0f * scale);

    {
        bitwisedom::gui::app bitwisedom(bitwisedom::gui::preferences_path(),
                                        bitwisedom::gui::session_path());
        bool                 running = true;

        while (running)
        {
            MSG message;

            while (::PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&message);
                ::DispatchMessage(&message);

                if (message.message == WM_QUIT)
                {
                    running = false;
                }
            }

            if (!running)
            {
                break;
            }

            // a file dropped on the window, opened
            for (const std::string& each : dropped())
            {
                uxoxo::event made;
                made.kind = "file.drop";
                made.payload.set("value", each);
                bitwisedom.post(made);
            }

            dropped().clear();

            device& d = the_device();

            // an occluded window draws nothing, and sleeps a little
            if ( (d.occluded) &&
                 (d.chain->Present(0, DXGI_PRESENT_TEST) ==
                  DXGI_STATUS_OCCLUDED) )
            {
                ::Sleep(10);

                continue;
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
            bitwisedom.frame();
            ImGui::Render();

            const ImVec4 clear = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            const float  rgba[4] = { clear.x, clear.y, clear.z, 1.0f };

            d.context->OMSetRenderTargets(1, &d.target, nullptr);
            d.context->ClearRenderTargetView(d.target, rgba);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            d.occluded = (d.chain->Present(1, 0) == DXGI_STATUS_OCCLUDED);

            if (bitwisedom.quitting())
            {
                ::PostMessage(window, WM_CLOSE, 0, 0);
            }
        }
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    destroy_device();
    ::DestroyWindow(window);
    ::UnregisterClassW(type.lpszClassName, type.hInstance);

    return 0;
}
