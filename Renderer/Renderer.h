#pragma once

#include <Renderer/ConstantBuffer.h>
#include <Renderer/StructuredBuffer.h>
#include <Core/Reference.h>

// Forward declares
class Texture;

// Parameters for DispatchIndirect
struct DispatchIndirectParams
{
									DispatchIndirectParams() : mThreadGroupCountX(1), mThreadGroupCountY(1), mThreadGroupCountZ(1) { }
									DispatchIndirectParams(int inThreadGroupCountX, int inThreadGroupCountY, int inThreadGroupCountZ) : mThreadGroupCountX(inThreadGroupCountX), mThreadGroupCountY(inThreadGroupCountY), mThreadGroupCountZ(inThreadGroupCountZ) { }

	uint32							mThreadGroupCountX;
	uint32							mThreadGroupCountY;
	uint32							mThreadGroupCountZ;
};

// Camera setup
struct CameraState
{
									CameraState() : mPos(Vec3::sZero()), mForward(0, 0, -1), mUp(0, 1, 0), mFOVY(DegreesToRadians(70.0f)), mFarPlane(100.0f) { }

	Vec3							mPos;								// Camera position
	Vec3							mForward;							// Camera forward vector
	Vec3							mUp;								// Camera up vector
	float							mFOVY;								// Field of view in radians in up direction
	float							mFarPlane;							// Distance of far plane
};

// Creates / manages DirectX objects
class Renderer
{
public:
	// Destructor
									~Renderer();

	// Initialize DirectX
	void							Initialize();

	// Initialize device dependent resources
	void							OnWindowResize();

	// Get window size
	int								GetWindowWidth()					{ return mWindowWidth; }
	int								GetWindowHeight()					{ return mWindowHeight; }

	// Access to the window handle
	HWND							GetWindowHandle() const				{ return mhWnd; }

	// Access to the most important DirectX structures
	ID3D11Device *					GetDevice()							{ return mDevice.Get(); }
	ID3D11DeviceContext *			GetContext()						{ return mContext.Get(); }

	// Start / end drawing a frame
	void							BeginFrame(const CameraState &inCamera, float inWorldScale);
	void							EndFrame();

	// Switch between orthographic and projection mode
	void							SetProjectionMode();
	void							SetOrthoMode();

	// Select culling mode
	void							SetBackFaceCulling()				{ mContext->RSSetState(mRasterizerStateBackFaceCull.Get()); }
	void							SetFrontFaceCulling()				{ mContext->RSSetState(mRasterizerStateFrontFaceCull.Get()); }

	// Turn on / off depth testing while rendering
	void							SetDepthTestOn()					{ mContext->OMSetDepthStencilState(mDepthTestOn.Get(), 0); }
	void							SetDepthTestOff()					{ mContext->OMSetDepthStencilState(mDepthTestOff.Get(), 0); }

	// Set blend mode
	void							SetBlendModeWrite()					{ mContext->OMSetBlendState(mBlendStateWrite.Get(), nullptr, 0xffffffff); }
	void							SetBlendModeAlphaBlend()			{ mContext->OMSetBlendState(mBlendStateAlphaBlend.Get(), nullptr, 0xffffffff); }
	void							SetBlendModeAlphaTest()				{ mContext->OMSetBlendState(mBlendStateAlphaTest.Get(), nullptr, 0xffffffff); }

	// Set sampler state
	void							SetSamplerClamp(int inSlot)			{ mContext->PSSetSamplers(inSlot, 1, mSamplerStateClamp.GetAddressOf()); }
	void							SetSamplerWrap(int inSlot)			{ mContext->PSSetSamplers(inSlot, 1, mSamplerStateWrap.GetAddressOf()); }
	void							SetSamplerComparisonLE(int inSlot)	{ mContext->PSSetSamplers(inSlot, 1, mSamplerStateComparisonLE.GetAddressOf()); }
	
	// Create a texture to render to, pass DXGI_FORMAT_UNKNOWN to only create a depth buffer
	Ref<Texture>					CreateRenderTarget(int inWidth, int inHeight, DXGI_FORMAT inFormat = DXGI_FORMAT_R32G32B32A32_FLOAT);
	void							SetRenderTarget(const Texture *inRenderTarget); // nullptr to set back to the main render target

	// Vertex & pixel shaders
	void							CreateVertexShader(const char *inFileName, const D3D11_INPUT_ELEMENT_DESC *inElementDesc, int inElementDescCount, ComPtr<ID3D11VertexShader> &outVertexShader, ComPtr<ID3D11InputLayout> &outInputLayout);
	ComPtr<ID3D11PixelShader>		CreatePixelShader(const char *inFileName);
	void							BindShader(D3D11_PRIMITIVE_TOPOLOGY inType, ID3D11VertexShader *inVertexShader, ID3D11InputLayout *inInputLayout, ID3D11PixelShader *inPixelShader);

	// Compute shaders
	using Define = pair<string, string>;
	using Defines = vector<Define>;
	ComPtr<ID3D11ComputeShader>		CreateComputeShader(const char *inFileName, const Defines *inDefines = nullptr);
	void							CSBindShader(ID3D11ComputeShader *inShader);

	// Constant buffers
	unique_ptr<ConstantBuffer>		CreateConstantBuffer(uint inBufferSize);
	void							CSBindConstantBuffers(const ConstantBuffer *inB1, const ConstantBuffer *inB2 = nullptr, const ConstantBuffer *inB3 = nullptr, const ConstantBuffer *inB4 = nullptr);

	// Structured buffers
	unique_ptr<StructuredBuffer>	CreateByteAddressBuffer(ECPUAccess inCPUAccess, uint inBufferByteSize, const void *inInitialContents = nullptr);
	unique_ptr<StructuredBuffer>	CreateStructuredBuffer(ECPUAccess inCPUAccess, uint inElementSize, uint inElementCount, const void *inInitialContents = nullptr);
	void							CSBindRBuffers(StructuredBuffer *inB1, StructuredBuffer *inB2 = nullptr, StructuredBuffer *inB3 = nullptr, StructuredBuffer *inB4 = nullptr);

	// RW Structured buffers
	unique_ptr<StructuredBuffer>	CreateRWByteAddressBuffer(ECPUAccess inCPUAccess, uint inBufferByteSize, const void *inInitialContents = nullptr);
	unique_ptr<StructuredBuffer>	CreateRWByteAddressBufferDispatchIndirect(ECPUAccess inCPUAccess, uint inBufferByteSize, const void *inInitialContents = nullptr);
	unique_ptr<StructuredBuffer>	CreateRWStructuredBuffer(ECPUAccess inCPUAccess, uint inElementSize, uint inElementCount, const void *inInitialContents = nullptr);
	unique_ptr<StructuredBuffer>	CreateAppendConsumeStructuredBuffer(ECPUAccess inCPUAccess, uint inElementSize, uint inElementCount, const void *inInitialContents = nullptr);
	void							CSBindRWBuffers(StructuredBuffer *inB1, StructuredBuffer *inB2 = nullptr, StructuredBuffer *inB3 = nullptr, StructuredBuffer *inB4 = nullptr);

	// Dispatch compute work
	void							Dispatch(int inThreadGroupCountX, int inThreadGroupCountY, int inThreadGroupCountZ);

	// Dispatch compute work, thread group counts are in DispatchIndirectParams struct located at inBufferOffset
	void							DispatchIndirect(StructuredBuffer *inBuffer, int inBufferOffset);

private:
	HWND							mhWnd;
	int								mWindowWidth;
	int								mWindowHeight;
	ComPtr<ID3D11Device>			mDevice;
	ComPtr<ID3D11DeviceContext>		mContext;
	ComPtr<IDXGISwapChain>			mSwapChain;
	ComPtr<ID3D11RenderTargetView>	mRenderTargetView;
	ComPtr<ID3D11DepthStencilView>	mDepthStencilView;
	ComPtr<ID3D11RasterizerState>	mRasterizerStateBackFaceCull;
	ComPtr<ID3D11RasterizerState>	mRasterizerStateFrontFaceCull;
	ComPtr<ID3D11BlendState>		mBlendStateWrite;
	ComPtr<ID3D11BlendState>		mBlendStateAlphaBlend;
	ComPtr<ID3D11BlendState>		mBlendStateAlphaTest;
	ComPtr<ID3D11SamplerState>		mSamplerStateClamp;
	ComPtr<ID3D11SamplerState>		mSamplerStateWrap;
	ComPtr<ID3D11SamplerState>		mSamplerStateComparisonLE;
	ComPtr<ID3D11DepthStencilState>	mDepthTestOn;
	ComPtr<ID3D11DepthStencilState>	mDepthTestOff;
	unique_ptr<ConstantBuffer>		mVertexShaderConstantBufferProjection;
	unique_ptr<ConstantBuffer>		mVertexShaderConstantBufferOrtho;
	unique_ptr<ConstantBuffer>		mPixelShaderConstantBuffer;
};
