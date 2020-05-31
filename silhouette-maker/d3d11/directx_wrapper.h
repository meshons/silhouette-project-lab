#ifndef SILHOUETTE_DIRECTX_WRAPPER_H
#define SILHOUETTE_DIRECTX_WRAPPER_H

#include <d3d11_4.h>
#include <stdexcept>
#include <system_error>
#include <DirectXMath.h>
#include <wrl.h>

#include "directx_hresult_handler.h"
#include "assimp/model_loader.h"
#include "safe_release.h"
#include "assimp/model_manager.h"

struct constant_buffer_struct {
    DirectX::XMMATRIX mWorld;
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProjection;
};

class directx_wrapper
{
    ID3D11Device * device = nullptr;
    ID3D11Device1 * device1 = nullptr;
    IDXGISwapChain * swap_chain = nullptr;
    IDXGISwapChain1 * swap_chain1 = nullptr;
    ID3D11DeviceContext * device_context = nullptr;
    ID3D11DeviceContext1 * device_context1 = nullptr;
    ID3D11RenderTargetView * back_buffer = nullptr;
    ID3D11VertexShader * pVS = nullptr;
    ID3D11PixelShader * pPS = nullptr;
    ID3D11InputLayout * layout = nullptr;
    ID3D11Buffer * constant_buffer = nullptr;
    ID3D11Texture2D * depth_stencil = nullptr;
    ID3D11DepthStencilView * depth_stencil_view = nullptr;
    ID3D11SamplerState * tex_sampler_state = nullptr;
    ID3D11RasterizerState * raster_state = nullptr;

    D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
    D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;
    ID3D11Debug* d3d11debug = nullptr;

    DirectX::XMMATRIX m_World;
    DirectX::XMMATRIX m_View;
    DirectX::XMMATRIX m_Projection;

    directx_hresult_handler error_handler;

    model_manager * manager;
    model_loader * model;
public:
    directx_wrapper(HWND & g_hWnd);

    ~directx_wrapper();
    void clean();

    bool render_frame();
};

directx_hresult_handler& operator<<(directx_hresult_handler& handler, const HRESULT & result);

#endif //SILHOUETTE_DIRECTX_WRAPPER_H
