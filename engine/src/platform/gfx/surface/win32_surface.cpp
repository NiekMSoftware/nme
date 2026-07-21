//
// Created by niek on 7/18/2026.
//

// TODO: Implement win32 surface

#include "nme/platform/gfx/gfx.h"

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM

namespace nme::gfx {

namespace {

constexpr u32  kMaxSurfaces = 8;
constexpr u32  kEventCap    = 128;      // per surface event ring
constexpr char kClassName[] = "nme_surface_wndclass";

// One window's worth of state. Surface.id is (slot index + 1); 0 == null handle.
struct SurfaceState {
    HWND     hwnd = nullptr;
    Extent2D size = {0, 0};
    bool     should_close = false;

    // TODO: replace with fixed sized ring buffer
    Event ring[kEventCap]{};
    u32 head = 0, tail = 0, count = 0;

    bool push(const Event& e) {
        if (count >= kEventCap) return false;
        ring[tail] = e;
        tail = (tail + 1) % kEventCap;
        ++count;
        return true;
    }
    bool pop(Event* out) {
        if (count == 0) return false;
        *out = ring[head];
        head = (head + 1) % kEventCap;
        --count;
        return true;
    }
};

SurfaceState g_surfaces[kMaxSurfaces];
bool         g_used[kMaxSurfaces] = {};

SurfaceState* state_of(Surface s) {
    if (!valid(s) || s.id > kMaxSurfaces || !g_used[s.id - 1]) return nullptr;
    return &g_surfaces[s.id - 1];
}

LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // Stash the SurfaceState* we passed as lparam so later messages can find it.
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lparam);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    }

    auto* w = reinterpret_cast<SurfaceState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (!w) return DefWindowProcA(hwnd, msg, wparam, lparam);   // pre-NCCREATE msgs

    Event e{};
    switch (msg) {
        case WM_CLOSE: {
            w->should_close = true;
            e.type = EventType::Close;
            w->push(e);
            return 0;
        }

        case WM_SIZE: {
            const Extent2D ext { static_cast<u32>(LOWORD(lparam)), static_cast<u32>(HIWORD(lparam)) };
            w->size = ext;
            if (ext.width && ext.height) {
                e.type = EventType::Resize;
                e.resize = ext;
                w->push(e);
                return 0;
            }
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            e.type     = EventType::KeyDown;
            e.key.code = static_cast<u32>(wparam);
            e.key.down = true;
            w->push(e);
            return 0;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP: {
            e.type     = EventType::KeyUp;
            e.key.code = static_cast<u32>(wparam);
            e.key.down = false;
            w->push(e);
            return 0;
        }

        case WM_MOUSEMOVE: {
            e.type = EventType::MouseMove;
            e.mouse.x = GET_X_LPARAM(lparam);
            e.mouse.y = GET_Y_LPARAM(lparam);
            w->push(e);
            return 0;
        }

        case WM_LBUTTONDOWN: case WM_LBUTTONUP:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: {
            e.type = EventType::MouseButton;
            e.mouse.x = GET_X_LPARAM(lparam);
            e.mouse.y = GET_Y_LPARAM(lparam);
            e.mouse.button = static_cast<u32>(wparam);
            e.mouse.down = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN);
            w->push(e);
            return 0;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS: {
            e.type = EventType::Focus;
            e.focus.gained = (msg == WM_SETFOCUS);
            w->push(e);
            return 0;
        }

        default: {
            return DefWindowProcA(hwnd, msg, wparam, lparam);
        }
    }
}

bool register_class() {
    static bool reg = false;
    if (reg) return true;
    WNDCLASSEXA wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wndproc;
    wc.hInstance     = GetModuleHandleA(nullptr);
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    reg = RegisterClassExA(&wc) != 0;
    return reg;
}

}  // anonymous namespace

Surface create_surface(const WindowDesc* desc, GfxError* out_err) {
    auto fail = [&](const GfxError e) { if (out_err) *out_err = e; return Surface{0}; };

    if (!desc)             return fail(GfxError::InvalidArgs);
    if (!register_class()) return fail(GfxError::Unknown);

    u32 slot = kMaxSurfaces;
    for (u32 i = 0; i < kMaxSurfaces; ++i) {
        if (!g_used[i]) {
            slot = i + 1;
            break;
        }
    }
    if (slot == kMaxSurfaces) return fail(GfxError::OutOfMemory);

    SurfaceState* w = &g_surfaces[slot];
    *w = SurfaceState{};

    const DWORD style = desc->resizable
                        ? WS_OVERLAPPEDWINDOW
                        : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU);

    RECT rc{ 0, 0, static_cast<LONG>(desc->extent.width), static_cast<LONG>(desc->extent.height) };
    AdjustWindowRect(&rc, style, FALSE);

    w->hwnd = CreateWindowExA(
        0, kClassName, desc->title, style,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, GetModuleHandleA(nullptr), w);

    if (!w->hwnd) return fail(GfxError::Unknown);

    g_used[slot] = true;
    w->size      = desc->extent;
    ShowWindow(w->hwnd, SW_SHOW);

    if (out_err) *out_err = GfxError::None;
    return Surface{slot + 1};
}
void destroy_surface(Surface) {}

bool poll_event(Surface, Event*) { return false; }
bool surface_should_close(Surface) { return false; }
Extent2D surface_size(Surface) { return Extent2D{1280, 720}; }
NativeHandle surface_native(Surface) { return {}; }

}  // namespace gfx

// EOF