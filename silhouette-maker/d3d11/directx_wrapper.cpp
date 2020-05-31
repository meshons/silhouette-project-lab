#include "directx_wrapper.h"
#include <d3dcompiler.h>
#include <tchar.h>

#include <ScreenGrab.h>
#include <wincodec.h>
#include <filesystem>

#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1024
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 1024
#endif

directx_wrapper::directx_wrapper(HWND & g_hWnd) {

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE,};
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                         D3D_FEATURE_LEVEL_10_0,};
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    HRESULT hr;
    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++) {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice(
                nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION,
                &device, &g_featureLevel, &device_context
        );

        if (hr == E_INVALIDARG) {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice(
                    nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                    D3D11_SDK_VERSION, &device, &g_featureLevel, &device_context
            );
        }

        if (SUCCEEDED(hr))
            break;
    }
    error_handler << hr;

#if _DEBUG
    error_handler << device->QueryInterface(IID_PPV_ARGS(&d3d11debug));
#endif

    UINT m4xMsaaQuality;
    device->CheckMultisampleQualityLevels(
            DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality
    );

    IDXGIFactory1 * dxgiFactory = nullptr;
    {
        IDXGIDevice * dxgiDevice = nullptr;
        error_handler << device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(&dxgiDevice));

        IDXGIAdapter * adapter = nullptr;
        error_handler << dxgiDevice->GetAdapter(&adapter);
        error_handler << adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&dxgiFactory));
        adapter->Release();
        dxgiDevice->Release();
    }

    IDXGIFactory2 * dxgiFactory2 = nullptr;
    error_handler << dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void **>(&dxgiFactory2));
    if (dxgiFactory2) {
        error_handler << device->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void **>(&device1));
        (void) device_context->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void **>(&device_context1));

        DXGI_SWAP_CHAIN_DESC1 sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.Width = SCREEN_WIDTH;
        sd.Height = SCREEN_HEIGHT;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 4;
        sd.SampleDesc.Quality = m4xMsaaQuality - 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        error_handler << dxgiFactory2->CreateSwapChainForHwnd(device, g_hWnd, &sd, nullptr, nullptr, &swap_chain1);

        hr = swap_chain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void **>(&swap_chain));


        dxgiFactory2->Release();
    } else {
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 1;
        sd.BufferDesc.Width = SCREEN_WIDTH;
        sd.BufferDesc.Height = SCREEN_HEIGHT;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = m4xMsaaQuality - 1;
        sd.Windowed = TRUE;

        error_handler << dxgiFactory->CreateSwapChain(device, &sd, &swap_chain);
    }

    dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    ID3D11Texture2D *pBackBuffer;
    swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    device->CreateRenderTargetView(pBackBuffer, nullptr, &back_buffer);
    pBackBuffer->Release();

    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = SCREEN_WIDTH;
    descDepth.Height = SCREEN_HEIGHT;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 4;
    descDepth.SampleDesc.Quality = m4xMsaaQuality - 1;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    error_handler << device->CreateTexture2D(&descDepth, nullptr, &depth_stencil);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    error_handler << device->CreateDepthStencilView(depth_stencil, 0, &depth_stencil_view);

    device_context->OMSetRenderTargets(1, &back_buffer, depth_stencil_view);

    D3D11_RASTERIZER_DESC rasterDesc;
    rasterDesc.AntialiasedLineEnable = false;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.MultisampleEnable = false;
    rasterDesc.ScissorEnable = false;
    rasterDesc.SlopeScaledDepthBias = 0.0f;

    device->CreateRasterizerState(&rasterDesc, &raster_state);
    device_context->RSSetState(raster_state);

    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.Width = SCREEN_WIDTH;
    viewport.Height = SCREEN_HEIGHT;

    device_context->RSSetViewports(1, &viewport);

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;;
#if defined( DEBUG ) || defined( _DEBUG )
    flags |= D3DCOMPILE_DEBUG;
#endif

    //const D3D_SHADER_MACRO defines[] = {};
    ID3DBlob * errorBlob = nullptr;
    ID3DBlob *VS, *PS;
    error_handler << D3DCompileFromFile(
            L"shaders/shadow_shader_vs.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", flags, 0,
            &VS, &errorBlob
    );
    error_handler << D3DCompileFromFile(
            L"shaders/shadow_shader_fs.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", flags, 0,
            &PS, &errorBlob
    );

    hr = device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, &pVS);
    if (FAILED(hr))
    {
        if (errorBlob != nullptr)
            OutputDebugStringA((LPCSTR)errorBlob->GetBufferPointer());
    }

    if (errorBlob != nullptr)
        errorBlob->Release();

    errorBlob = nullptr;
    device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, &pPS);
    if (FAILED(hr))
    {
        if (errorBlob != nullptr)
            OutputDebugStringA((LPCSTR)errorBlob->GetBufferPointer());
    }

    if (errorBlob != nullptr)
        errorBlob->Release();

    D3D11_INPUT_ELEMENT_DESC ied[] =
            {
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };

    device->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &layout);
    device_context->IASetInputLayout(layout);

    m_Projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.01f, 1000.0f);

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(constant_buffer_struct);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    error_handler << device->CreateBuffer(&bd, nullptr, &constant_buffer);

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    error_handler << device->CreateSamplerState(&sampDesc, &tex_sampler_state);

    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_View = DirectX::XMMatrixLookAtLH(Eye, At, Up);

    manager = new model_manager(g_hWnd, device, device_context);
    manager->start();
    model = nullptr;
}

directx_wrapper::~directx_wrapper() {
    clean();
}

bool directx_wrapper::render_frame() {
    if (!model && !manager->has_next_model())
    {
        return false;
    }

	if (!model)
	{
        model = manager->next();
	}

    static float t = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0)
        timeStart = timeCur;
    t = (timeCur - timeStart) / 1000.0f;

    float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    device_context->ClearRenderTargetView(back_buffer, clearColor);
    device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    if (model != nullptr) {
        auto scaleAndCenter = model->getMaxScaleAndCenter();

        //m_World = DirectX::XMMatrixRotationY(-t);
        auto rotation = model->getRotation();
        auto blockTransform = model->getBlockTransformation();

    	m_World = DirectX::XMMatrixTranslation(
            -scaleAndCenter.second.X,
            -scaleAndCenter.second.Y,
            -scaleAndCenter.second.Z
        );

        m_World = m_World * DirectX::XMMatrixRotationX(rotation.X);
        m_World = m_World * DirectX::XMMatrixRotationY(rotation.Y);
        m_World = m_World * DirectX::XMMatrixRotationZ(rotation.Z);
    	

        m_World = m_World * DirectX::XMMatrixScaling(scaleAndCenter.first, scaleAndCenter.first, scaleAndCenter.first);
        m_World = m_World * DirectX::XMMatrixTranslation(
            blockTransform.X,
            blockTransform.Y,
            blockTransform.Z
        );
        //m_World = m_World * DirectX::XMMatrixScaling(0.05, 0.05, 0.05);
    } 
	
    constant_buffer_struct cb;
    cb.mWorld = XMMatrixTranspose(m_World);
    cb.mView = XMMatrixTranspose(m_View);
    cb.mProjection = XMMatrixTranspose(m_Projection);
    device_context->UpdateSubresource(constant_buffer, 0, nullptr, &cb, 0, 0);

    device_context->VSSetShader(pVS, 0, 0);
    device_context->VSSetConstantBuffers(0, 1, &constant_buffer);
    device_context->PSSetShader(pPS, 0, 0);
    device_context->PSSetSamplers(0, 1, &tex_sampler_state);

	if (model)
		model->Draw(device_context);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBufferTex;
    HRESULT hr = swap_chain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&backBufferTex);
    if ( SUCCEEDED(hr) ) {
        // Write out the render target as JPG
    	if (model )
    	{
	        std::wstring file = L"output/";
	        auto model_path = std::filesystem::path(model->getName());
            auto model_name = model_path.filename().string();

	        auto output_path = std::filesystem::current_path()
	                .append("output").append(model_name + model->getState());

	        if (!std::filesystem::exists(output_path))
	        {
	            std::filesystem::create_directory(output_path);
	        }

            auto dir_name = output_path.filename().string();

	        file += std::wstring(dir_name.begin(), dir_name.end());
	        file += L"/";
	        file += std::wstring(model_name.begin(), model_name.end());
            file += std::to_wstring(model->getBlockState());
            file += L"_";
            file += std::to_wstring(model->getRotationState());
	        file += std::wstring(L".jpg");

	        SaveWICTextureToFile(
	                device_context,
	                backBufferTex.Get(),
	                GUID_ContainerFormatJpeg,
	                file.c_str()
	                );
        }
    }

	if (model)
		model->rotate();
    swap_chain->Present(0, 0);

    if (model && model->modelEnd()) {
        model = manager->next();
    }

    return true;
}

void directx_wrapper::clean() {
    if (swap_chain)
        swap_chain->SetFullscreenState(FALSE, nullptr);

    if (manager) {
        manager->stop();
        delete manager;
        manager = nullptr;
    }

    safe_release(tex_sampler_state);
    safe_release(constant_buffer);
    safe_release(layout);
    safe_release(pVS);
    safe_release(pPS);
    safe_release(raster_state);
    safe_release(depth_stencil_view);
    safe_release(depth_stencil);
    safe_release(back_buffer);
    safe_release(swap_chain);
    safe_release(device_context1);
    safe_release(device1);
    safe_release(device_context);
#if _DEBUG
    if (d3d11debug) {
        OutputDebugString(TEXT("Dumping DirectX 11 live objects.\n"));
        d3d11debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        safe_release(d3d11debug);
    } else {
        OutputDebugString(TEXT("Unable to dump live objects: no DirectX 11 debug interface available.\n"));
    }
#endif
    safe_release(device);
}


