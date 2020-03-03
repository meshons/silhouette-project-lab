#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <wrl.h>
#include <d3d11_2.h>

#include "ThrowOnFail.h"
#include "Math/math.h"

class App
{
	HINSTANCE hInstance;
	int nCmdShow;
	HWND hwnd;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> defaultRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> defaultDepthStencilView;

public:
	App(HINSTANCE hInstance, int nCmdShow);
	~App();

	void start() noexcept;

private:
	void initLogger();
	void openWindow();
	void createD3D();
	void createResources();
	void createSwapChainResources();
	void render();
};

