#include "nme/platform/gfx/gfx.h"
#include "nme/platform/collections/ring_buffer.h"

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
    RingBuffer<Event> events{};
};

SurfaceState g_surfaces[kMaxSurfaces];
bool         g_used[kMaxSurfaces] = {};

SurfaceState* state_of(const Surface s) {
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
            ring_buffer_push(&w->events, e);
            return 0;
        }

        case WM_SIZE: {
            const Extent2D ext { static_cast<u32>(LOWORD(lparam)), static_cast<u32>(HIWORD(lparam)) };
            w->size = ext;
            if (ext.width && ext.height) {
                e.type = EventType::Resize;
                e.resize = ext;
                ring_buffer_push(&w->events, e);
                return 0;
            }
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            e.type     = EventType::KeyDown;
            e.key.code = static_cast<u32>(wparam);
            e.key.down = true;
            ring_buffer_push(&w->events, e);
            return 0;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP: {
            e.type     = EventType::KeyUp;
            e.key.code = static_cast<u32>(wparam);
            e.key.down = false;
            ring_buffer_push(&w->events, e);
            return 0;
        }

        case WM_MOUSEMOVE: {
            e.type = EventType::MouseMove;
            e.mouse.x = GET_X_LPARAM(lparam);
            e.mouse.y = GET_Y_LPARAM(lparam);
            ring_buffer_push(&w->events, e);
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
            ring_buffer_push(&w->events, e);
            return 0;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS: {
            e.type = EventType::Focus;
            e.focus.gained = (msg == WM_SETFOCUS);
            ring_buffer_push(&w->events, e);
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

GfxResult<Surface> create_surface(const WindowDesc* desc, const Allocator& alloc) {
    const auto fail = [](const GfxError e) { return result_err<Surface, GfxError>(e); };

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

    // init ring buffer
    ring_buffer_init(&w->events, alloc, kEventCap);

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

    return result_ok<Surface, GfxError>(Surface{slot + 1});
}

void destroy_surface(const Surface s) {
    SurfaceState* w = state_of(s);
    if (!w) return;
    if (w->hwnd) DestroyWindow(w->hwnd);
    ring_buffer_destroy(&w->events);
    *w = SurfaceState{};
    g_used[s.id - 1] = false;
}

bool poll_event(const Surface s, Event* out) {
    SurfaceState* w = state_of(s);
    if (!w || !out) return false;

    if (ring_buffer_pop(&w->events, out)) return true;       // fast path: something queued

    MSG msg;
    while (PeekMessageA(&msg, w->hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);         // wndproc enqueues into w
        if (ring_buffer_pop(&w->events, out)) return true;
    }
    return false;                       // queue drained
}

bool surface_should_close(const Surface s) {
    const SurfaceState* w = state_of(s);
    return w ? w->should_close : true;
}

Extent2D surface_size(const Surface s) {
    const SurfaceState* w = state_of(s);
    return w ? w->size : Extent2D{0, 0};
}

NativeHandle surface_native(const Surface s) {
    const SurfaceState* w = state_of(s);
    return w ? static_cast<NativeHandle>(w->hwnd) : nullptr;
}

}  // namespace gfx

// EOF