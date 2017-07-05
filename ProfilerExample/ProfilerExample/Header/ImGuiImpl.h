#ifndef _IMGUI_IMPL_H
#define _IMGUI_IMPL_H

#include "imgui\imgui.h"

// DX11 Implementation for ImGui callback functions

struct ID3D11Device;
struct ID3D11DeviceContext;

IMGUI_API bool        ImGui_Impl_Init(void* hwnd, ID3D11Device* device, ID3D11DeviceContext* device_context);
IMGUI_API void        ImGui_Impl_Shutdown();
IMGUI_API void        ImGui_Impl_NewFrame();

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_Impl_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_Impl_CreateDeviceObjects();

#endif