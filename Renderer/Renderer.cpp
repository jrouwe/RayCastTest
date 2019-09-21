#include <pch.h>

#include <Renderer/Renderer.h>
#include <Renderer/Texture.h>
#include <Renderer/FatalErrorIfFailed.h>

#pragma warning (push, 0)
#include <d3dcompiler.h>
#include <ShellScalingApi.h>
#include <DirectXMath.h>
#pragma warning (pop)

#pragma comment(lib, "d3dcompiler.lib")

static Renderer *sRenderer = nullptr;

struct VertexShaderConstantBuffer
{
	DirectX::XMFLOAT4X4		mView;
	DirectX::XMFLOAT4X4		mProjection;
	DirectX::XMFLOAT4X4		mLightView;
	DirectX::XMFLOAT4X4		mLightProjection;
};

struct PixelShaderConstantBuffer
{
	DirectX::XMFLOAT4		mCameraPos;
	DirectX::XMFLOAT4		mLightPos;
};

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_SIZE:
		if (sRenderer != nullptr)
			sRenderer->OnWindowResize();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
Renderer::~Renderer()
{
	// Switch back to windowed mode
	if (mSwapChain != nullptr)
		mSwapChain->SetFullscreenState(FALSE, nullptr);
}

//-----------------------------------------------------------------------------
// Create the Direct3D 11 device and device context.
//-----------------------------------------------------------------------------
void Renderer::Initialize()
{
	// Prevent this window from auto scaling
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(nullptr);
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"TestRayCastClass";
	wcex.hIconSm = nullptr;
	if (!RegisterClassEx(&wcex))
		FatalError("Failed to register window class");

	// Create window
	RECT rc = { 0, 0, 1920, 1080 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	mhWnd = CreateWindow(L"TestRayCastClass", L"TestRayCast", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
		rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, wcex.hInstance, nullptr);
	if (!mhWnd)
		FatalError("Failed to create window");

	// Show window
	ShowWindow(mhWnd, SW_SHOW);

	UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	// If the project is in a debug build, enable debugging via SDK Layers.
	creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// This example only uses feature level 11.0.
	D3D_FEATURE_LEVEL feature_levels[] =
	{
		D3D_FEATURE_LEVEL_11_0
	};

	// Create the Direct3D 11 API device object and a corresponding context.
	HRESULT hr = D3D11CreateDevice(
			nullptr, // default adapter.
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			creation_flags,
			feature_levels,
			ARRAYSIZE(feature_levels),
			D3D11_SDK_VERSION,
			&mDevice,
			nullptr,
			&mContext);

#ifdef _DEBUG
	// Check if creation failed because the Windows 10 SDK is not installed
	if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING)
	{
		// Remove debug flag
		creation_flags &= ~D3D11_CREATE_DEVICE_DEBUG;

		// Try again
		hr = D3D11CreateDevice(
			nullptr, // default adapter.
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			creation_flags,
			feature_levels,
			ARRAYSIZE(feature_levels),
			D3D11_SDK_VERSION,
			&mDevice,
			nullptr,
			&mContext);
	}
#endif

	FatalErrorIfFailed(hr);

	// Create constant buffer
	mVertexShaderConstantBufferProjection = CreateConstantBuffer(sizeof(VertexShaderConstantBuffer));
	mVertexShaderConstantBufferOrtho = CreateConstantBuffer(sizeof(VertexShaderConstantBuffer));
	mPixelShaderConstantBuffer = CreateConstantBuffer(sizeof(PixelShaderConstantBuffer));

	// Set rasterizer state
	CD3D11_DEFAULT def;
	CD3D11_RASTERIZER_DESC raster_desc(def);
	raster_desc.FrontCounterClockwise = true;
	FatalErrorIfFailed(mDevice->CreateRasterizerState(&raster_desc, mRasterizerStateBackFaceCull.GetAddressOf()));
	raster_desc.FrontCounterClockwise = false;
	FatalErrorIfFailed(mDevice->CreateRasterizerState(&raster_desc, mRasterizerStateFrontFaceCull.GetAddressOf()));
	SetBackFaceCulling();
 
	// Create blend modes
	D3D11_BLEND_DESC blend_desc;
	memset(&blend_desc, 0, sizeof(D3D11_RENDER_TARGET_BLEND_DESC)); 
	blend_desc.RenderTarget[0].BlendEnable = false;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; 
	FatalErrorIfFailed(mDevice->CreateBlendState(&blend_desc, &mBlendStateWrite));
	blend_desc.RenderTarget[0].BlendEnable = true;
	blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	FatalErrorIfFailed(mDevice->CreateBlendState(&blend_desc, &mBlendStateAlphaBlend));
	blend_desc.AlphaToCoverageEnable = true;
	FatalErrorIfFailed(mDevice->CreateBlendState(&blend_desc, &mBlendStateAlphaTest));
	SetBlendModeWrite();

	// Create samplers
	D3D11_SAMPLER_DESC sampler_desc;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 1;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.BorderColor[0] = 0;
	sampler_desc.BorderColor[1] = 0;
	sampler_desc.BorderColor[2] = 0;
	sampler_desc.BorderColor[3] = 0;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
	FatalErrorIfFailed(mDevice->CreateSamplerState(&sampler_desc, &mSamplerStateClamp));
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	FatalErrorIfFailed(mDevice->CreateSamplerState(&sampler_desc, &mSamplerStateWrap));
	sampler_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	FatalErrorIfFailed(mDevice->CreateSamplerState(&sampler_desc, &mSamplerStateComparisonLE));

	// Depth test parameters
	D3D11_DEPTH_STENCIL_DESC depth_desc;
	memset(&depth_desc, 0, sizeof(depth_desc));
	depth_desc.DepthEnable = true;
	depth_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depth_desc.DepthFunc = D3D11_COMPARISON_LESS;
	FatalErrorIfFailed(mDevice->CreateDepthStencilState(&depth_desc, &mDepthTestOn));
	depth_desc.DepthEnable = false;
	FatalErrorIfFailed(mDevice->CreateDepthStencilState(&depth_desc, &mDepthTestOff));
	SetDepthTestOn();

	// Initialize device dependent resources
	OnWindowResize();

	// Store global renderer now that we're done initializing
	sRenderer = this;
}

//-----------------------------------------------------------------------------
// Create the swap chain, back buffer and viewport.
//-----------------------------------------------------------------------------
void Renderer::OnWindowResize()
{
	// Set the render target to null as a signal to recreate window resources.
	mRenderTargetView = nullptr;
	mDepthStencilView = nullptr;

	// Get new window size
	RECT rc;
	GetClientRect(mhWnd, &rc);
	mWindowWidth = max<LONG>(rc.right - rc.left, 8);
	mWindowHeight = max<LONG>(rc.bottom - rc.top, 8);

	if (mSwapChain != nullptr)
	{
		// If the swap chain already exists, resize it.
		mSwapChain->ResizeBuffers(
			2, // Double-buffered swap chain.
			mWindowWidth,
			mWindowHeight,
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0);
	}
	else
	{
		// Otherwise, create a new one using the same adapter as the existing
		// Direct3D device.
		DXGI_SWAP_CHAIN_DESC swap_chain_desc;
		memset(&swap_chain_desc, 0, sizeof(swap_chain_desc));
		swap_chain_desc.BufferCount = 2;
		swap_chain_desc.BufferDesc.Width = mWindowWidth;
		swap_chain_desc.BufferDesc.Height = mWindowHeight;
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
		swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.OutputWindow = mhWnd;
		swap_chain_desc.SampleDesc.Count = 1;
		swap_chain_desc.SampleDesc.Quality = 0;
		swap_chain_desc.Windowed = TRUE;
		swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
				
		// This sequence obtains the DXGI factory.
		// First, the DXGI interface for the Direct3D device:
		ComPtr<IDXGIDevice> dxgi_device;
		mDevice.As(&dxgi_device);

		// Then, the adapter hosting the device;
		ComPtr<IDXGIAdapter> dxgi_adapter;
		dxgi_device->GetAdapter(&dxgi_adapter);

		// Then, the factory that created the adapter interface:
		ComPtr<IDXGIFactory1> dxgi_factory;
		dxgi_adapter->GetParent(
			__uuidof(IDXGIFactory1),
			&dxgi_factory);

		// Finally, use the factory to create the swap chain interface:
		dxgi_factory->CreateSwapChain(mDevice.Get(), &swap_chain_desc, &mSwapChain);
	}

	// Get the back buffer resource.
	ComPtr<ID3D11Texture2D> back_buffer;
	mSwapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		&back_buffer);

	// Create a render target view on the back buffer.
	FatalErrorIfFailed(
		mDevice->CreateRenderTargetView(
			back_buffer.Get(),
			nullptr,
			&mRenderTargetView));

	// Create a depth stencil view for use with 3D rendering if needed.
	CD3D11_TEXTURE2D_DESC depth_stencil_desc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		mWindowWidth,
		mWindowHeight,
		1, // This depth stencil view has only one texture.
		1, // Use a single mipmap level.
		D3D11_BIND_DEPTH_STENCIL);

	ComPtr<ID3D11Texture2D> depth_stencil;
	FatalErrorIfFailed(
		mDevice->CreateTexture2D(
		&depth_stencil_desc,
		nullptr,
		&depth_stencil));

	CD3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc(D3D11_DSV_DIMENSION_TEXTURE2D);
	FatalErrorIfFailed(
		mDevice->CreateDepthStencilView(
		depth_stencil.Get(),
		&depth_stencil_view_desc,
		&mDepthStencilView));

	// Set the main buffer as the render target
	SetRenderTarget(nullptr);
}

void Renderer::BeginFrame(const CameraState &inCamera, float inWorldScale)
{
	// Clear the back buffer.
	const float blue[] = { 0.098f, 0.098f, 0.439f, 1.000f };
	mContext->ClearRenderTargetView(
		mRenderTargetView.Get(),
		blue);

	// Clear the depth buffer
	mContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Set the main buffer as the render target
	SetRenderTarget(nullptr);

	// Light position
	Vec3 light_pos(250.0f, 250.0f, 250.0f);

	// Set constants for vertex shader in projection mode
	VertexShaderConstantBuffer *vs = mVertexShaderConstantBufferProjection->Map<VertexShaderConstantBuffer>();

	// Camera projection and view
	DirectX::XMStoreFloat4x4(
		&vs->mProjection,
			DirectX::XMMatrixPerspectiveFovRH(
				inCamera.mFOVY,
				static_cast<float>(GetWindowWidth()) / GetWindowHeight(),
				0.01f * inWorldScale,
				inCamera.mFarPlane * inWorldScale));
	Vec3 tgt = inCamera.mPos + inCamera.mForward;
	DirectX::XMStoreFloat4x4(
		&vs->mView,
		DirectX::XMMatrixLookAtRH(reinterpret_cast<const DirectX::XMVECTOR &>(inCamera.mPos), reinterpret_cast<const DirectX::XMVECTOR &>(tgt), reinterpret_cast<const DirectX::XMVECTOR &>(inCamera.mUp)));

	// Light projection and view
	DirectX::XMStoreFloat4x4(
		&vs->mLightProjection,
			DirectX::XMMatrixPerspectiveFovRH(
				DegreesToRadians(20.0f * inWorldScale),
				1.0f,
				1.0f,
				1000.0f));
	Vec3 light_tgt = Vec3::sZero();
	Vec3 light_up = Vec3(0, 1, 0);
	DirectX::XMStoreFloat4x4(
		&vs->mLightView,
		DirectX::XMMatrixLookAtRH(reinterpret_cast<const DirectX::XMVECTOR &>(light_pos), reinterpret_cast<const DirectX::XMVECTOR &>(light_tgt), reinterpret_cast<const DirectX::XMVECTOR &>(light_up)));

	mVertexShaderConstantBufferProjection->Unmap();

	// Set constants for vertex shader in ortho mode
	vs = mVertexShaderConstantBufferOrtho->Map<VertexShaderConstantBuffer>();

	// Camera projection and view
	DirectX::XMStoreFloat4x4(
		&vs->mProjection,
		DirectX::XMMatrixOrthographicOffCenterRH(
			0.0f,
			float(mWindowWidth),
			float(mWindowHeight),
			0.0f,
			0.0f,
			1.0f));
	DirectX::XMStoreFloat4x4(
		&vs->mView,
		DirectX::XMMatrixIdentity());

	// Light projection and view are unused in ortho mode
	DirectX::XMStoreFloat4x4(
		&vs->mLightView,
		DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(
		&vs->mLightProjection,
		DirectX::XMMatrixIdentity());

	mVertexShaderConstantBufferOrtho->Unmap();

	// Switch to 3d projection mode
	SetProjectionMode();
	
	// Set constants for pixel shader
	PixelShaderConstantBuffer *ps = mPixelShaderConstantBuffer->Map<PixelShaderConstantBuffer>();
	ps->mCameraPos = DirectX::XMFLOAT4(inCamera.mPos.GetX(), inCamera.mPos.GetY(), inCamera.mPos.GetZ(), 0);
	ps->mLightPos = DirectX::XMFLOAT4(light_pos.GetX(), light_pos.GetY(), light_pos.GetZ(), 0);
	mPixelShaderConstantBuffer->Unmap();

	// Set the pixel shader constant buffer data.
	mContext->PSSetConstantBuffers(
		0,  // register 0
		1,  // one constant buffer
		mPixelShaderConstantBuffer->mBuffer.GetAddressOf());
}

void Renderer::EndFrame()
{
	// Present the frame by swapping the back buffer to the screen.
	mSwapChain->Present(1, 0);
}

void Renderer::SetProjectionMode()
{ 
	ID3D11Buffer *buffers[] = { mVertexShaderConstantBufferProjection->mBuffer.Get() };
	mContext->VSSetConstantBuffers(0, 1, buffers); 
}

void Renderer::SetOrthoMode()
{ 
	ID3D11Buffer *buffers[] = { mVertexShaderConstantBufferOrtho->mBuffer.Get() };
	mContext->VSSetConstantBuffers(0, 1, buffers); 
}

Ref<Texture> Renderer::CreateRenderTarget(int inWidth, int inHeight, DXGI_FORMAT inFormat)
{
	return new Texture(this, inWidth, inHeight, inFormat);
}

void Renderer::SetRenderTarget(const Texture *inRenderTarget)
{
	if (inRenderTarget == nullptr)
	{
		// Set render target
		mContext->OMSetRenderTargets(1, mRenderTargetView.GetAddressOf(), mDepthStencilView.Get());

		// Set the rendering viewport to target the entire window
		CD3D11_VIEWPORT viewport(
			0.0f,
			0.0f,
			static_cast<float>(mWindowWidth),
			static_cast<float>(mWindowHeight));
		mContext->RSSetViewports(1, &viewport);
	}
	else
	{
		inRenderTarget->SetAsRenderTarget();
	}
}

void Renderer::CreateVertexShader(const char *inFileName, const D3D11_INPUT_ELEMENT_DESC *inElementDesc, int inElementDescCount, ComPtr<ID3D11VertexShader> &outVertexShader, ComPtr<ID3D11InputLayout> &outInputLayout)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    const D3D_SHADER_MACRO defines[] = 
    {
		{ nullptr, nullptr }
    };

	// Read shader source file
	vector<uint8> data = ReadData(inFileName);

	// Compile source
    ComPtr<ID3DBlob> shader_blob, error_blob;
    HRESULT hr = D3DCompile(&data[0],
							(uint)data.size(),
							inFileName, 
							defines, 
							D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            "main", 
							"vs_5_0",
                            flags, 
							0, 
							shader_blob.GetAddressOf(), 
							error_blob.GetAddressOf());
    if (FAILED(hr))
    {
		// Throw error if compilation failed
        if (error_blob)
            OutputDebugStringA((const char *)error_blob->GetBufferPointer());
		FatalError("Failed to compile vertex shader");
    }    

	// Create shader
	FatalErrorIfFailed(
		mDevice->CreateVertexShader(
			shader_blob->GetBufferPointer(),
			shader_blob->GetBufferSize(),
			nullptr,
			outVertexShader.GetAddressOf()));

	// Get input layout
	FatalErrorIfFailed(
		mDevice->CreateInputLayout(
			inElementDesc,
			inElementDescCount,
			shader_blob->GetBufferPointer(),
			shader_blob->GetBufferSize(),
			&outInputLayout));
}

ComPtr<ID3D11PixelShader> Renderer::CreatePixelShader(const char *inFileName)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    const D3D_SHADER_MACRO defines[] = 
    {
		{ nullptr, nullptr }
    };

	// Read shader source file
	vector<uint8> data = ReadData(inFileName);

	// Compile source
    ComPtr<ID3DBlob> shader_blob, error_blob;
    HRESULT hr = D3DCompile(&data[0],
							(uint)data.size(),
							inFileName, 
							defines, 
							D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            "main", 
							"ps_5_0",
                            flags, 
							0, 
							shader_blob.GetAddressOf(), 
							error_blob.GetAddressOf());
    if (FAILED(hr))
    {
		// Throw error if compilation failed
        if (error_blob)
            OutputDebugStringA((const char *)error_blob->GetBufferPointer());
		FatalError("Failed to compile pixel shader");
    }    

	// Create shader
	ComPtr<ID3D11PixelShader> shader;	
	FatalErrorIfFailed(
		mDevice->CreatePixelShader(
			shader_blob->GetBufferPointer(),
			shader_blob->GetBufferSize(),
			nullptr,
			shader.GetAddressOf()));
	return shader;
}

void Renderer::BindShader(D3D11_PRIMITIVE_TOPOLOGY inType, ID3D11VertexShader *inVertexShader, ID3D11InputLayout *inInputLayout, ID3D11PixelShader *inPixelShader)
{
	// Set primitive type
	mContext->IASetPrimitiveTopology(inType);

	// Set layout
	mContext->IASetInputLayout(inInputLayout);

	// Set the vertex shader
	mContext->VSSetShader(inVertexShader, nullptr, 0);

	// Set the pixel shader
	mContext->PSSetShader(inPixelShader, nullptr, 0);
}

ComPtr<ID3D11ComputeShader> Renderer::CreateComputeShader(const char *inFileName, const Defines *inDefines)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PARTIAL_PRECISION | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#else
	flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	vector<D3D_SHADER_MACRO> defines;
	if (inDefines != nullptr)
	{
		defines.resize(inDefines->size() + 1);
		for (uint i = 0; i < (uint)inDefines->size(); ++i)
		{
			defines[i].Name = inDefines->at(i).first.c_str();
			defines[i].Definition = inDefines->at(i).second.c_str();
		}
	}
	else
	{
		defines.resize(1);
	}
	defines.back().Name = nullptr;
	defines.back().Definition = nullptr;

	// Read shader source file
	vector<uint8> data = ReadData(inFileName);

	// Compile source
    ComPtr<ID3DBlob> shader_blob, error_blob;
    HRESULT hr = D3DCompile(&data[0],
							(uint)data.size(),
							inFileName, 
							&defines[0], 
							D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            "main", 
							"cs_5_0",
                            flags, 
							0, 
							shader_blob.GetAddressOf(), 
							error_blob.GetAddressOf());
    if (FAILED(hr))
    {
		// Throw error if compilation failed
        if (error_blob)
            OutputDebugStringA((const char *)error_blob->GetBufferPointer());
		FatalError("Failed to compile compute shader");
    }    

	// Create shader
	ComPtr<ID3D11ComputeShader> shader;	
	FatalErrorIfFailed(
		mDevice->CreateComputeShader(
			shader_blob->GetBufferPointer(),
			shader_blob->GetBufferSize(),
			nullptr,
			shader.GetAddressOf()));
	return shader;
}

void Renderer::CSBindShader(ID3D11ComputeShader *inShader)
{
	mContext->CSSetShader(
		inShader,
		nullptr,
		0);
}

unique_ptr<ConstantBuffer> Renderer::CreateConstantBuffer(uint inBufferSize)
{
	return make_unique<ConstantBuffer>(this, inBufferSize);
}

void Renderer::CSBindConstantBuffers(const ConstantBuffer *inB1, const ConstantBuffer *inB2, const ConstantBuffer *inB3, const ConstantBuffer *inB4)
{
	ID3D11Buffer * const buffers[] =
	{
		inB1 != nullptr? inB1->mBuffer.Get() : nullptr,
		inB2 != nullptr? inB2->mBuffer.Get() : nullptr,
		inB3 != nullptr? inB3->mBuffer.Get() : nullptr,
		inB4 != nullptr? inB4->mBuffer.Get() : nullptr
	};

	// Set constants
	mContext->CSSetConstantBuffers(
		0,
		4,
		buffers);
}

unique_ptr<StructuredBuffer> Renderer::CreateByteAddressBuffer(ECPUAccess inCPUAccess, uint inBufferByteSize, const void *inInitialContents)
{
	assert(IsAligned(inBufferByteSize, 4));
	return make_unique<StructuredBuffer>(this, inCPUAccess, GPU_ACCESS_READ, RAW_YES, APPEND_NO, DISPATCH_NO, 4, inBufferByteSize / 4, inInitialContents);
}

unique_ptr<StructuredBuffer> Renderer::CreateStructuredBuffer(ECPUAccess inCPUAccess, uint inElementSize, uint inElementCount, const void *inInitialContents)
{
	return make_unique<StructuredBuffer>(this, inCPUAccess, GPU_ACCESS_READ, RAW_NO, APPEND_NO, DISPATCH_NO, inElementSize, inElementCount, inInitialContents);
}

void Renderer::CSBindRBuffers(StructuredBuffer *inB1, StructuredBuffer *inB2, StructuredBuffer *inB3, StructuredBuffer *inB4)
{
	ID3D11ShaderResourceView * const srvs[] =
	{
		inB1 != nullptr? inB1->mSRV.Get() : nullptr,
		inB2 != nullptr? inB2->mSRV.Get() : nullptr,
		inB3 != nullptr? inB3->mSRV.Get() : nullptr,
		inB4 != nullptr? inB4->mSRV.Get() : nullptr
	};

	// Set buffers
	mContext->CSSetShaderResources(
		0,
		4,
		srvs);
}

unique_ptr<StructuredBuffer> Renderer::CreateRWByteAddressBuffer(ECPUAccess inCPUAccess, uint inBufferByteSize, const void *inInitialContents)
{
	assert(IsAligned(inBufferByteSize, 4));
	return make_unique<StructuredBuffer>(this, inCPUAccess, GPU_ACCESS_READ_WRITE, RAW_YES, APPEND_NO, DISPATCH_NO, 4, inBufferByteSize / 4, inInitialContents);
}

unique_ptr<StructuredBuffer> Renderer::CreateRWByteAddressBufferDispatchIndirect(ECPUAccess inCPUAccess, uint inBufferByteSize, const void *inInitialContents)
{
	assert(IsAligned(inBufferByteSize, 4));
	return make_unique<StructuredBuffer>(this, inCPUAccess, GPU_ACCESS_READ_WRITE, RAW_YES, APPEND_NO, DISPATCH_YES, 4, inBufferByteSize / 4, inInitialContents);
}

unique_ptr<StructuredBuffer> Renderer::CreateRWStructuredBuffer(ECPUAccess inCPUAccess, uint inElementSize, uint inElementCount, const void *inInitialContents)
{
	return make_unique<StructuredBuffer>(this, inCPUAccess, GPU_ACCESS_READ_WRITE, RAW_NO, APPEND_NO, DISPATCH_NO, inElementSize, inElementCount, inInitialContents);
}

unique_ptr<StructuredBuffer> Renderer::CreateAppendConsumeStructuredBuffer(ECPUAccess inCPUAccess, uint inElementSize, uint inElementCount, const void *inInitialContents)
{
	return make_unique<StructuredBuffer>(this, inCPUAccess, GPU_ACCESS_READ_WRITE, RAW_NO, APPEND_YES, DISPATCH_NO, inElementSize, inElementCount, inInitialContents);
}

void Renderer::CSBindRWBuffers(StructuredBuffer *inB1, StructuredBuffer *inB2, StructuredBuffer *inB3, StructuredBuffer *inB4)
{
	ID3D11UnorderedAccessView * const uavs[] =
	{
		inB1 != nullptr? inB1->mUAV.Get() : nullptr,
		inB2 != nullptr? inB2->mUAV.Get() : nullptr,
		inB3 != nullptr? inB3->mUAV.Get() : nullptr,
		inB4 != nullptr? inB4->mUAV.Get() : nullptr
	};

	// Set views
	mContext->CSSetUnorderedAccessViews(
		0,
		4,
		uavs,
		nullptr);
}

void Renderer::Dispatch(int inThreadGroupCountX, int inThreadGroupCountY, int inThreadGroupCountZ)
{
	mContext->Dispatch(inThreadGroupCountX, inThreadGroupCountY, inThreadGroupCountZ);
}

void Renderer::DispatchIndirect(StructuredBuffer *inBuffer, int inBufferOffset)
{
	mContext->DispatchIndirect(inBuffer->mBuffer.Get(), inBufferOffset);
}
