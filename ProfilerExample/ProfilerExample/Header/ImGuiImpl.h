#ifndef _IMGUI_IMPL_H
#define _IMGUI_IMPL_H

#include "imgui\imgui.h"

// DX11 Implementation for ImGui callback functions
struct ID3D11Device;
struct ID3D11DeviceContext;

IMGUI_API bool        ImGuiImplInit(void* hwnd, ID3D11Device* device, ID3D11DeviceContext* device_context);
IMGUI_API void        ImGuiImplShutdown();
IMGUI_API void        ImGuiImplNewFrame();

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGuiImplInvalidateDeviceObjects();
IMGUI_API bool        ImGuiImplCreateDeviceObjects();

#endif