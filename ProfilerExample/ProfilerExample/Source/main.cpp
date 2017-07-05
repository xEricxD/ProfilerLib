#include "Header\ImGuiImpl.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include "Profiler.h"

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

void CreateRenderTarget()
{
  DXGI_SWAP_CHAIN_DESC sd;
  g_pSwapChain->GetDesc(&sd);

  // Create the render target
  ID3D11Texture2D* pBackBuffer;
  D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
  ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
  render_target_view_desc.Format = sd.BufferDesc.Format;
  render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_mainRenderTargetView);
  g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
  pBackBuffer->Release();
}

void CleanupRenderTarget()
{
  if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

HRESULT CreateDeviceD3D(HWND hWnd)
{
  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC sd;
  {
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  }

  UINT createDeviceFlags = 0;
  //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0, };
  if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
    return E_FAIL;

  CreateRenderTarget();

  return S_OK;
}

void CleanupDeviceD3D()
{
  CleanupRenderTarget();
  if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
  if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
  if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

extern LRESULT ImGuiImplWndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (ImGuiImplWndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg)
  {
  case WM_SIZE:
    if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
    {
      ImGuiImplInvalidateDeviceObjects();
      CleanupRenderTarget();
      g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
      CreateRenderTarget();
      ImGuiImplCreateDeviceObjects();
    }
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main(int, char**)
{
  // Create application window
  WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, _T("ImGui Example"), NULL };
  RegisterClassEx(&wc);
  HWND hwnd = CreateWindow(_T("ImGui Example"), _T("ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

  // Initialize Direct3D
  if (CreateDeviceD3D(hwnd) < 0)
  {
    CleanupDeviceD3D();
    UnregisterClass(_T("ImGui Example"), wc.hInstance);
    return 1;
  }

  // Show the window
  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  // Setup ImGui binding
  ImGuiImplInit(hwnd, g_pd3dDevice, g_pd3dDeviceContext);


  // Main loop
  MSG msg;
  ZeroMemory(&msg, sizeof(msg));
  while (msg.message != WM_QUIT)
  {
    if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      continue;
    }
    Profiler::Get()->BeginFrame();

    ImGuiImplNewFrame();
    Profiler::Get()->Render();

    // Rendering
    static ImVec4 clearCol = ImColor(114, 144, 154);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clearCol);

    ImGui::Render();
    g_pSwapChain->Present(0, 0);

    Profiler::Get()->EndFrame();
  }

  ImGuiImplShutdown();
  CleanupDeviceD3D();
  UnregisterClass(_T("ImGui Example"), wc.hInstance);

  return 0;
}
