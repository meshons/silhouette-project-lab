#include "App.h"

App::App(HINSTANCE hInstance, int nCmdShow): hInstance{hInstance}, nCmdShow{nCmdShow}
{
	initLogger();
}

App::~App()
{
	spdlog::shutdown();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void App::start() noexcept
{
	spdlog::info("silhouette app started");

	openWindow();
	createD3D();

	MSG msg = { 0 };

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else 
		{
			render();
		}
	}

	spdlog::info("app closing");
}

void App::initLogger()
{
	spdlog::init_thread_pool(8192, 1);
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
	auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("log.txt", 1024 * 1024 * 10, 3);
	std::vector<spdlog::sink_ptr> sinks{ stdout_sink, rotating_sink };
	auto logger = std::make_shared<spdlog::async_logger>("default_log", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	spdlog::register_logger(logger);
}

void App::openWindow()
{
	const wchar_t CLASS_NAME[] = L"silhouette generator";

	WNDCLASS wc = {};

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	 hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"silhouette generator",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL)
	{
		spdlog::error("failed to create window");
		return;
	}

	spdlog::info("window opened");
	ShowWindow(hwnd, nCmdShow);
}

void App::createD3D()
{
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	using namespace Microsoft::WRL;

	D3D_FEATURE_LEVEL d3dFeatureLevel;

	ThrowOnFail("Failed to create D3D device.", __FILE__, __LINE__) ^
		D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			creationFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			device.GetAddressOf(),
			&d3dFeatureLevel,
			context.GetAddressOf()
		);

	ComPtr<IDXGIDevice3> dxgiDevice;
	ThrowOnFail("The device is not a DXGI device.", __FILE__, __LINE__) ^
		device.As(&dxgiDevice);

	ComPtr<IDXGIAdapter> dxgiAdapter;
	ThrowOnFail("Failed to get adapter from device.", __FILE__, __LINE__) ^
		dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());

	ComPtr<IDXGIFactory2> dxgiFactory;
	ThrowOnFail("Failed to get factory from adapter.", __FILE__, __LINE__) ^
		dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));

	ComPtr<IDXGISwapChain1> swapChain;
	RECT r;
	GetWindowRect(hwnd, &r);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = r.right - r.left;
	swapChainDesc.Height = r.bottom - r.top;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; 
	swapChainDesc.Flags = 0;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	ThrowOnFail("Failed to create swap chain.", __FILE__, __LINE__) ^
		dxgiFactory->CreateSwapChainForHwnd(
			device.Get(),
			hwnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain
		);

	createSwapChainResources();

	CD3D11_VIEWPORT screenViewport(
		0.0f,
		0.0f,
		swapChainDesc.Width,
		swapChainDesc.Height
	);

	context->RSSetViewports(1, &screenViewport);
}

void App::createResources()
{

}

void App::createSwapChainResources()
{
	using namespace Microsoft::WRL;

	ThrowOnFail("Failed to get swapchain backbuffer.", __FILE__, __LINE__) ^
		swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));

	ThrowOnFail("Failed to create render target view.", __FILE__, __LINE__) ^
		device->CreateRenderTargetView(
			backBuffer.Get(),
			nullptr,
			defaultRenderTargetView.GetAddressOf()
		);

	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		static_cast<UINT>(swapChainDesc.Width),
		static_cast<UINT>(swapChainDesc.Height),
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL
	);

	ThrowOnFail("Failed to create depth stencil texture.", __FILE__, __LINE__) ^
		device->CreateTexture2D(
			&depthStencilDesc,
			nullptr,
			&depthStencil
		);

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	ThrowOnFail("Failed to create depth stencil view.", __FILE__, __LINE__) ^
		device->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			defaultDepthStencilView.GetAddressOf()
		);
}

void App::render()
{
	context->OMSetRenderTargets(1, defaultRenderTargetView.GetAddressOf(), defaultDepthStencilView.Get());

	float clearColor[4] =
	{ 1.0f, 1.0f, 1.0f, 0.0f };
	context->ClearRenderTargetView(
		defaultRenderTargetView.Get(), clearColor);
	context->ClearDepthStencilView(defaultDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0, 0);

	shadedMesh->draw(context);

	using namespace Egg::Math;
	float4x4 perObjectMatrices[3];
	perObjectMatrices[2] =
		//float4x4::translation(float3(0.5, 0, 0)).transpose();
		(firstPersonCam->getViewMatrix()
			* firstPersonCam->getProjMatrix()) /*.transpose()*/;
	perObjectMatrices[1] = float4x4::identity;
	perObjectMatrices[0] = float4x4::identity;
	context->UpdateSubresource(perObjectConstantBuffer.Get(), 0, nullptr, perObjectMatrices, 0, 0);

}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}