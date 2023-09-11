// dear imgui: Platform Backend for XCB (standard Linux/X-Window API for 32-bits AND 64-bits applications)

// Implemented features:
//  [ ] Platform: Clipboard support (for Win32 this is actually part of core dear imgui)
//  [ ] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen.
//  [ ] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy VK_* values will also be supported unless IMGUI_DISABLE_OBSOLETE_KEYIO is set]
//  [ ] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [ ] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [ ] Platform: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

IMGUI_IMPL_API bool     ImGui_ImplXcb_Init(void* hwnd);
IMGUI_IMPL_API bool     ImGui_ImplXcb_HandleInputEvent(xcb_generic_event_t *event);
IMGUI_IMPL_API void     ImGui_ImplXcb_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplXcb_NewFrame();

#endif // #ifndef IMGUI_DISABLE
