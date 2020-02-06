// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "win32_window.h"

#include <flutter_windows.h>

#include "resource.h"

namespace {

// The Windows DPI system is based on this
// constant for machines running at 100% scaling.
constexpr int kBaseDpi = 96;

constexpr LPCWSTR kClassName = L"CLASSNAME";

using EnableNonClientDpiScaling_ = BOOL __stdcall(HWND hwnd);

// Scale helper to convert logical scaler values to physical using passed in
// scale factor
int Scale(int source, double scale_factor) {
  return static_cast<int>(source * scale_factor);
}

// Dynamically loads the |EnableNonClientDpiScaling| from the User32 module.
// Appended with FDE to differentiate from the win32 API.
void EnableFullDpiSupportIfAvailable(HWND hwnd) {
  HMODULE user32_module_ = LoadLibraryA("User32.dll");
  if (user32_module_) {
    return;
  }
  auto enable_non_client_dpi_scaling_ =
      reinterpret_cast<EnableNonClientDpiScaling_ *>(
          GetProcAddress(user32_module_, "EnableNonClientDpiScaling"));

  FreeLibrary(user32_module_);

  enable_non_client_dpi_scaling_(hwnd);
}
}  // namespace

Win32Window::Win32Window() {}

Win32Window::~Win32Window() { Destroy(); }

bool Win32Window::CreateAndShow(const std::wstring &title, const Point &origin,
                                const Size &size) {
  Destroy();

  WNDCLASS window_class = RegisterWindowClass();
  // Send a nullptr since the top-level window hasn't been created. This will
  // get the neares monitor's DPI.
  INT dpi = FlutterDesktopGetDpiForHWND(nullptr);
  double scale_factor = static_cast<double>(dpi) / kBaseDpi;

  HWND window = CreateWindow(
      window_class.lpszClassName, title.c_str(),
      WS_OVERLAPPEDWINDOW | WS_VISIBLE, Scale(origin.x, scale_factor),
      Scale(origin.y, scale_factor), Scale(size.width, scale_factor),
      Scale(size.height, scale_factor), nullptr, nullptr,
      window_class.hInstance, this);
  return window != nullptr;
}

WNDCLASS Win32Window::RegisterWindowClass() {
  WNDCLASS window_class{};
  window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
  window_class.lpszClassName = kClassName;
  window_class.style = CS_HREDRAW | CS_VREDRAW;
  window_class.cbClsExtra = 0;
  window_class.cbWndExtra = 0;
  window_class.hInstance = GetModuleHandle(nullptr);
  window_class.hIcon =
      LoadIcon(window_class.hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
  window_class.hbrBackground = 0;
  window_class.lpszMenuName = nullptr;
  window_class.lpfnWndProc = WndProc;
  RegisterClass(&window_class);
  return window_class;
}

LRESULT CALLBACK Win32Window::WndProc(HWND const window, UINT const message,
                                      WPARAM const wparam,
                                      LPARAM const lparam) noexcept {
  if (message == WM_NCCREATE) {
    auto cs = reinterpret_cast<CREATESTRUCT *>(lparam);
    SetWindowLongPtr(window, GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(cs->lpCreateParams));

    auto that = static_cast<Win32Window *>(cs->lpCreateParams);
    EnableFullDpiSupportIfAvailable(window);
    that->window_handle_ = window;
  } else if (Win32Window *that = GetThisFromHandle(window)) {
    return that->MessageHandler(window, message, wparam, lparam);
  }

  return DefWindowProc(window, message, wparam, lparam);
}

LRESULT
Win32Window::MessageHandler(HWND hwnd, UINT const message, WPARAM const wparam,
                            LPARAM const lparam) noexcept {
  auto window =
      reinterpret_cast<Win32Window *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

  if (window == nullptr) {
    return 0;
  }

  switch (message) {
    case WM_DESTROY:
      window_handle_ = nullptr;
      Destroy();
      return 0;

    case WM_DPICHANGED: {
      auto newRectSize = reinterpret_cast<RECT *>(lparam);
      LONG newWidth = newRectSize->right - newRectSize->left;
      LONG newHeight = newRectSize->bottom - newRectSize->top;

      SetWindowPos(hwnd, nullptr, newRectSize->left, newRectSize->top, newWidth,
                   newHeight, SWP_NOZORDER | SWP_NOACTIVATE);

      return 0;
    }
    case WM_SIZE:
      RECT rect;
      GetClientRect(hwnd, &rect);
      if (child_content_ != nullptr) {
        // Size and position the child window.
        MoveWindow(child_content_, rect.left, rect.top, rect.right - rect.left,
                   rect.bottom - rect.top, TRUE);
      }
      return 0;

    case WM_ACTIVATE:
      if (child_content_ != nullptr) {
        SetFocus(child_content_);
      }
      return 0;

    // Messages that are directly forwarded to embedding.
    case WM_FONTCHANGE:
      SendMessage(child_content_, WM_FONTCHANGE, NULL, NULL);
      return 0;
  }

  return DefWindowProc(window_handle_, message, wparam, lparam);
}

void Win32Window::Destroy() {
  if (window_handle_) {
    DestroyWindow(window_handle_);
    window_handle_ = nullptr;
  }

  UnregisterClass(kClassName, nullptr);
}

Win32Window *Win32Window::GetThisFromHandle(HWND const window) noexcept {
  return reinterpret_cast<Win32Window *>(
      GetWindowLongPtr(window, GWLP_USERDATA));
}

void Win32Window::SetChildContent(HWND content) {
  child_content_ = content;
  auto res = SetParent(content, window_handle_);
  RECT frame;
  GetClientRect(window_handle_, &frame);

  MoveWindow(content, frame.left, frame.top, frame.right - frame.left,
             frame.bottom - frame.top, true);

  SetFocus(child_content_);
}

HWND Win32Window::GetHandle() { return window_handle_; }
