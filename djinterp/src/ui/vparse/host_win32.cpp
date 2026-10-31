/*******************************************************************************
* vparse [ui]                                                     host_win32.cpp
*
* The workbench's window on Win32 and Direct3D 11: the host on Windows, which
* needs nothing the system does not have.
*   It follows Dear ImGui's own example for this pair. The process is made
* DPI-aware and the window sized by the primary monitor's scale; Direct3D 11
* is asked for a hardware device and falls back to WARP, the software one,
* where there is none. A resize is queued by the window procedure and applied
* between frames, and a window that is minimized or covered is waited out
* rather than drawn.
*   A capture copies the back buffer into a staging texture the processor can
* read, after the frame is drawn and before it is presented.
*   The program is a console program, so that its help and its errors reach
* a terminal. Started from Explorer it would stand an empty console behind
* its window; the host closes that one, and only that one, when the window
* opens.
*   The window's class is registered under the module's own handle, since
* the module `vparse gui` loads, not the program, holds its procedure.
*   <windows.h> is included here with NOMINMAX, which keeps its min and max
* macros off std::min and std::max, and WIN32_LEAN_AND_MEAN; no djinterp
* header comes after it, so wingdi.h's ERROR meets no opcode of that name.
*
* path:      /src/ui/vparse/host_win32.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.08
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "../../../inc/ui/vparse/host.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::fopen, std::fprintf, std::fwrite, std::fclose
#include <string>   // std::string
#include <vector>   // std::vector
// windows
#ifndef NOMINMAX
    #define NOMINMAX
#endif  // NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <windows.h>  // windows, messages, MultiByteToWideChar
#include <d3d11.h>    // the device, the swap chain, textures
// imgui
#include "imgui.h"             // the context, frames, draw data
#include "imgui_impl_dx11.h"   // the Direct3D 11 renderer backend
#include "imgui_impl_win32.h"  // the Win32 platform backend


// ImGui_ImplWin32_WndProcHandler
//   function: the Win32 backend's message handler, which its header leaves to
// the application to declare so as not to include <windows.h> itself.
extern
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND   _window,
    UINT   _message,
    WPARAM _wide,
    LPARAM _long
);


namespace vparse
{
namespace workbench
{

namespace
{

// CLASS_NAME
//   constant: the window class the host registers, named for the program.
const wchar_t* const CLASS_NAME = L"vparse_workbench";

// window
//   struct: everything that exists while a window is open.
struct window
{
    HWND                    handle      = nullptr;
    HINSTANCE               instance    = nullptr;
    ID3D11Device*           device      = nullptr;
    ID3D11DeviceContext*    context     = nullptr;
    IDXGISwapChain*         swap_chain  = nullptr;
    ID3D11RenderTargetView* target      = nullptr;
    UINT                    resize_w    = 0u;
    UINT                    resize_h    = 0u;
    bool                    occluded    = false;
    bool                    quit        = false;
    float                   scale       = 1.0f;
};

window g_window;

// fail
//   records why there is no window, and returns false for the caller to.
bool fail(
    std::string*       _error,
    const std::string& _why
)
{
    if (_error != nullptr)
    {
        *_error = _why;
    }

    return false;
}

// release
//   releases a COM object, if there is one, and forgets it.
template<typename Com>
void release(
    Com*& _object
)
{
    if (_object != nullptr)
    {
        _object->Release();
        _object = nullptr;
    }

    return;
}

// make_target
//   the render target view of the swap chain's back buffer.
bool make_target()
{
    ID3D11Texture2D* back = nullptr;

    if (FAILED(g_window.swap_chain->GetBuffer(0, IID_PPV_ARGS(&back))))
    {
        return false;
    }

    const HRESULT made =
        g_window.device->CreateRenderTargetView(back, nullptr,
                                                &g_window.target);
    release(back);

    return SUCCEEDED(made);
}

// make_device
//   the device, its context and the swap chain: hardware if there is any,
// else WARP.
bool make_device(
    HWND _handle
)
{
    DXGI_SWAP_CHAIN_DESC chain = {};

    chain.BufferCount                        = 2;
    chain.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    chain.BufferDesc.RefreshRate.Numerator   = 60;
    chain.BufferDesc.RefreshRate.Denominator = 1;
    chain.Flags      = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    chain.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    chain.OutputWindow       = _handle;
    chain.SampleDesc.Count   = 1;
    chain.SampleDesc.Quality = 0;
    chain.Windowed           = TRUE;
    chain.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0,
                                          D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL       level     = D3D_FEATURE_LEVEL_11_0;

    HRESULT made = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0u, levels, 2u,
        D3D11_SDK_VERSION, &chain, &g_window.swap_chain, &g_window.device,
        &level, &g_window.context);

    // no hardware device: the software rasterizer draws the same frames
    if (made == DXGI_ERROR_UNSUPPORTED)
    {
        made = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0u, levels, 2u,
            D3D11_SDK_VERSION, &chain, &g_window.swap_chain, &g_window.device,
            &level, &g_window.context);
    }

    if (made != S_OK)
    {
        return false;
    }

    // Alt+Enter would switch to full screen behind Dear ImGui's back
    IDXGIFactory* factory = nullptr;

    if (SUCCEEDED(g_window.swap_chain->GetParent(IID_PPV_ARGS(&factory))))
    {
        factory->MakeWindowAssociation(_handle, DXGI_MWA_NO_ALT_ENTER);
        release(factory);
    }

    return make_target();
}

// drop_device
//   releases what make_device made.
void drop_device()
{
    release(g_window.target);
    release(g_window.swap_chain);
    release(g_window.context);
    release(g_window.device);

    return;
}

// procedure
//   the window procedure: Dear ImGui sees every message first; a resize is
// queued for the next frame, and the window menu's Alt key is kept from
// stealing focus.
LRESULT WINAPI procedure(
    HWND   _handle,
    UINT   _message,
    WPARAM _wide,
    LPARAM _long
)
{
    if (ImGui_ImplWin32_WndProcHandler(_handle, _message, _wide, _long))
    {
        return 1;
    }

    switch (_message)
    {
        case WM_SIZE:
            if (_wide != SIZE_MINIMIZED)
            {
                g_window.resize_w = static_cast<UINT>(LOWORD(_long));
                g_window.resize_h = static_cast<UINT>(HIWORD(_long));
            }

            return 0;

        case WM_SYSCOMMAND:
            if ((_wide & 0xfff0u) == SC_KEYMENU)
            {
                return 0;
            }

            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);

            return 0;

        default:
            break;
    }

    return ::DefWindowProcW(_handle, _message, _wide, _long);
}

// widen
//   UTF-8 text as Windows' UTF-16.
std::wstring widen(
    const std::string& _text
)
{
    if (_text.empty())
    {
        return std::wstring();
    }

    const int size = static_cast<int>(_text.size());
    const int wide = ::MultiByteToWideChar(CP_UTF8, 0, _text.data(), size,
                                           nullptr, 0);
    std::wstring out(static_cast<std::size_t>(wide), L'\0');

    ::MultiByteToWideChar(CP_UTF8, 0, _text.data(), size, &out[0], wide);

    return out;
}

// capture
//   the back buffer as a binary PPM: copied to a staging texture, mapped, and
// written row by row without its alpha.
bool capture(
    const std::string& _path
)
{
    ID3D11Texture2D* back    = nullptr;
    ID3D11Texture2D* staging = nullptr;

    if (FAILED(g_window.swap_chain->GetBuffer(0, IID_PPV_ARGS(&back))))
    {
        return false;
    }

    D3D11_TEXTURE2D_DESC desc = {};

    back->GetDesc(&desc);
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.BindFlags      = 0u;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags      = 0u;

    bool written = false;

    if (SUCCEEDED(g_window.device->CreateTexture2D(&desc, nullptr, &staging)))
    {
        g_window.context->CopyResource(staging, back);

        D3D11_MAPPED_SUBRESOURCE mapped = {};

        if (SUCCEEDED(g_window.context->Map(staging, 0u, D3D11_MAP_READ, 0u,
                                            &mapped)))
        {
            std::FILE* file = std::fopen(_path.c_str(), "wb");

            if (file != nullptr)
            {
                const std::size_t          width = desc.Width;
                std::vector<unsigned char> row(width * 3u);

                std::fprintf(file, "P6\n%u %u\n255\n", desc.Width,
                             desc.Height);

                for (UINT y = 0u; y < desc.Height; ++y)
                {
                    const unsigned char* source =
                        static_cast<const unsigned char*>(mapped.pData) +
                        (static_cast<std::size_t>(y) * mapped.RowPitch);

                    // RGBA in, RGB out
                    for (std::size_t x = 0u; x < width; ++x)
                    {
                        row[(x * 3u) + 0u] = source[(x * 4u) + 0u];
                        row[(x * 3u) + 1u] = source[(x * 4u) + 1u];
                        row[(x * 3u) + 2u] = source[(x * 4u) + 2u];
                    }

                    std::fwrite(row.data(), 1u, row.size(), file);
                }

                written = (std::fclose(file) == 0);
            }

            g_window.context->Unmap(staging, 0u);
        }
    }

    release(staging);
    release(back);

    return written;
}

// this_module
//   the module this code is in -- the window's, which `vparse gui` loads,
// not the program's -- found by an address inside it, as a window class
// whose procedure it holds is registered under it; the program's where it
// cannot be found.
HINSTANCE
this_module()
{
    HMODULE found = nullptr;

    if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCWSTR>(&procedure),
                              &found))
    {
        found = ::GetModuleHandleW(nullptr);
    }

    return found;
}

}  // anonymous namespace


/*
host_name
  Fixed: the build compiles one host.
*/
const char* host_name()
{
    return "Win32 + Direct3D 11";
}

/*
host_scale
  The primary monitor's, read when the window opened.
*/
float host_scale()
{
    return g_window.scale;
}

/*
host_open
  The DPI awareness is set before anything measures the screen, or Windows
answers in scaled pixels. The window is shown only once the device exists,
so a failure leaves nothing on the screen.
*/
bool host_open(
    const window_options& _options,
    std::string*          _error
)
{
    if (g_window.handle != nullptr)
    {
        return fail(_error, "a window is already open");
    }

    ImGui_ImplWin32_EnableDpiAwareness();

    g_window.scale    = ImGui_ImplWin32_GetDpiScaleForMonitor(
        ::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
    g_window.instance = this_module();

    WNDCLASSEXW type   = {};
    type.cbSize        = sizeof(type);
    type.style         = CS_CLASSDC;
    type.lpfnWndProc   = &procedure;
    type.hInstance     = g_window.instance;
    // LoadCursor, not LoadCursorW: IDC_ARROW is narrow or wide as UNICODE
    // is, and the macro follows it
    type.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    type.lpszClassName = CLASS_NAME;

    if (::RegisterClassExW(&type) == 0)
    {
        return fail(_error, "the window class could not be registered");
    }

    const std::wstring title = widen(_options.title);

    g_window.handle = ::CreateWindowExW(
        0u, CLASS_NAME, title.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<int>(static_cast<float>(_options.width) * g_window.scale),
        static_cast<int>(static_cast<float>(_options.height) * g_window.scale),
        nullptr, nullptr, g_window.instance, nullptr);

    if (g_window.handle == nullptr)
    {
        ::UnregisterClassW(CLASS_NAME, g_window.instance);

        return fail(_error, "the window could not be made");
    }

    if (!make_device(g_window.handle))
    {
        drop_device();
        ::DestroyWindow(g_window.handle);
        ::UnregisterClassW(CLASS_NAME, g_window.instance);
        g_window = window();

        return fail(_error, "Direct3D 11 could not make a device");
    }

    ::ShowWindow(g_window.handle, SW_SHOWDEFAULT);
    ::UpdateWindow(g_window.handle);

    // Started from Explorer, a console program is given a console of its
    // own, which here would only stand empty behind the window: it goes. One
    // started from a terminal shares the terminal's, which stays.
    DWORD attached = 0u;

    if (::GetConsoleProcessList(&attached, 1u) == 1u)
    {
        ::FreeConsole();
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(g_window.handle);
    ImGui_ImplDX11_Init(g_window.device, g_window.context);

    return true;
}

/*
host_close
  The reverse of host_open: the backends, the context, the device, then the
window and its class.
*/
void host_close()
{
    if (g_window.handle == nullptr)
    {
        return;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    drop_device();
    ::DestroyWindow(g_window.handle);
    ::UnregisterClassW(CLASS_NAME, g_window.instance);

    g_window = window();

    return;
}

/*
host_begin_frame
  Messages are drained first, since WM_QUIT arrives only through the queue. A
swap chain that reported itself occluded is tested, not presented, until it
is visible again. A queued resize rebuilds the back buffers between frames,
never inside the window procedure.
*/
bool host_begin_frame()
{
    for (;;)
    {
        MSG message = {};

        while (::PeekMessageW(&message, nullptr, 0u, 0u, PM_REMOVE))
        {
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);

            if (message.message == WM_QUIT)
            {
                g_window.quit = true;
            }
        }

        if (g_window.quit)
        {
            return false;
        }

        // minimized or covered: wait, still handling messages
        if ( (g_window.occluded) &&
             (g_window.swap_chain->Present(0u, DXGI_PRESENT_TEST) ==
              DXGI_STATUS_OCCLUDED) )
        {
            ::Sleep(10);

            continue;
        }

        g_window.occluded = false;

        break;
    }

    if ( (g_window.resize_w != 0u) &&
         (g_window.resize_h != 0u) )
    {
        release(g_window.target);
        g_window.swap_chain->ResizeBuffers(0u, g_window.resize_w,
                                           g_window.resize_h,
                                           DXGI_FORMAT_UNKNOWN, 0u);
        g_window.resize_w = 0u;
        g_window.resize_h = 0u;
        make_target();
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    return true;
}

/*
host_end_frame
  The capture reads the back buffer, so it comes between drawing and the
present that shows the frame.
*/
bool host_end_frame(
    float              _red,
    float              _green,
    float              _blue,
    const std::string& _capture,
    std::string*       _error
)
{
    ImGui::Render();

    const float clear[4] = { _red, _green, _blue, 1.0f };

    g_window.context->OMSetRenderTargets(1u, &g_window.target, nullptr);
    g_window.context->ClearRenderTargetView(g_window.target, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    bool captured = true;

    if ( (!_capture.empty()) &&
         (!capture(_capture)) )
    {
        captured = fail(_error, "could not write " + _capture);
    }

    const HRESULT shown = g_window.swap_chain->Present(1u, 0u);

    g_window.occluded = (shown == DXGI_STATUS_OCCLUDED);

    return captured;
}

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now
