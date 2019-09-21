#include <pch.h>

#include <Renderer/TriangleRendererImp.h>
#include <Renderer/Renderer.h>
#include <Renderer/RenderPrimitive.h>

TriangleRendererImp::TriangleRendererImp(Renderer *inRenderer) :
	mRenderer(inRenderer)
{
	// Create input layout
	const D3D11_INPUT_ELEMENT_DESC vertex_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "INSTANCE_TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_INV_TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_INV_TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_INV_TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 96, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_INV_TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 112, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 128, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	// Load vertex shader
	mRenderer->CreateVertexShader("Shaders/TriangleVertexShader.hlsl", vertex_desc, ARRAYSIZE(vertex_desc), mVertexShader, mInputLayout);
	
	// Load pixel shader
	mPixelShader = mRenderer->CreatePixelShader("Shaders/TrianglePixelShader.hlsl");
	
	// Load depth vertex shader
	mRenderer->CreateVertexShader("Shaders/TriangleDepthVertexShader.hlsl", vertex_desc, ARRAYSIZE(vertex_desc), mDepthVertexShader, mInputLayout);
	
	// Load depth pixel shader
	mDepthPixelShader = mRenderer->CreatePixelShader("Shaders/TriangleDepthPixelShader.hlsl");

	// Create depth only texture (no color buffer, as seen from light)
	mDepthTexture = mRenderer->CreateRenderTarget(4096, 4096, DXGI_FORMAT_UNKNOWN);

	// Create instances buffer
	mInstancesBuffer = new RenderInstances(mRenderer);
}

TriangleRenderer::Batch TriangleRendererImp::CreateTriangleBatch(const TriangleList &inTriangles) 
{
	vector<Vertex> vertices;

	// Create render vertices
	vertices.resize(3 * inTriangles.size());
	for (size_t t = 0; t < inTriangles.size(); ++t)
	{
		Vec3 vtx[3];
		for (int v = 0; v < 3; ++v)
			vtx[v] = Vec3::sLoadFloat3Unsafe(inTriangles[t].mV[v]);

		Float3 normal;
		((vtx[1] - vtx[0]).Cross(vtx[2] - vtx[0])).Normalized().StoreFloat3(&normal);

		for (size_t v = 0; v < 3; ++v)
		{
			size_t idx = t * 3 + v;
			vertices[idx].mPosition = inTriangles[t].mV[v];
			vertices[idx].mNormal = normal;
			vertices[idx].mUV = Float2(0, 0);
			vertices[idx].mColor = Color::sWhite;
		}
	}

	BatchImpl *primitive = new BatchImpl(mRenderer, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	primitive->CreateVertexBuffer((int)vertices.size(), sizeof(Vertex), vertices.data());
	return primitive;
}

void TriangleRendererImp::DrawTriangleBatch(const Mat44 &inModelMatrix, const Color &inModelColor, Batch inBatch)
{
	mPrimitives[inBatch].push_back({ inModelMatrix, inModelMatrix.GetDirectionPreservingMatrix(), inModelColor });
	++mNumInstances;
}

void TriangleRendererImp::DrawTriangleBatchBackFacing(const Mat44 &inModelMatrix, const Color &inModelColor, Batch inBatch)
{
	mPrimitivesBackFacing[inBatch].push_back({ inModelMatrix, inModelMatrix.GetDirectionPreservingMatrix(), inModelColor });
	++mNumInstances;
}

void TriangleRendererImp::Draw()
{
	// Clear the shadow map texture with color 1 which indicates max depth
	mDepthTexture->ClearRenderTarget(Vec4::sReplicate(1.0f));

	// Render to shadow map texture first
	mRenderer->SetRenderTarget(mDepthTexture);

	// No blending, we're only writing depth values
	mRenderer->SetBlendModeWrite();

	// Front face culling, we want to render the back side of the geometry for casting shadows
	mRenderer->SetFrontFaceCulling();

	// Resize instances buffer and copy all instance data into it
	if (mNumInstances > 0)
	{
		mInstancesBuffer->CreateBuffer(mNumInstances, sizeof(Instance));
		Instance *instance = reinterpret_cast<Instance *>(mInstancesBuffer->Lock());
		for (InstanceMap::value_type &v : mPrimitives)
		{
			memcpy(instance, &v.second[0], sizeof(Instance) * v.second.size());
			instance += v.second.size();
		}
		for (InstanceMap::value_type &v : mPrimitivesBackFacing)
		{
			memcpy(instance, &v.second[0], sizeof(Instance) * v.second.size());
			instance += v.second.size();
		}
		mInstancesBuffer->Unlock();
	}

	// Bind depth shader
	mRenderer->BindShader(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, mDepthVertexShader.Get(), mInputLayout.Get(), mDepthPixelShader.Get());	

	// Draw all primitives as seen from the light
	int start_instance = 0;
	for (InstanceMap::value_type &v : mPrimitives)
	{
		int num_instances = (int)v.second.size();
		mInstancesBuffer->Draw(static_cast<BatchImpl *>(v.first.GetPtr()), start_instance, num_instances);
		start_instance += num_instances;
	}

	// Back face culling, we want to render the front side of back facing geometry
	mRenderer->SetBackFaceCulling();

	// Draw all primitives as seen from the light
	for (InstanceMap::value_type &v : mPrimitivesBackFacing)
	{
		int num_instances = (int)v.second.size();
		mInstancesBuffer->Draw(static_cast<BatchImpl *>(v.first.GetPtr()), start_instance, num_instances);
		start_instance += num_instances;
	}
	assert(start_instance == mNumInstances);

	// Switch to the main viewport
	mRenderer->SetRenderTarget(nullptr);

	// Bind the normal shader
	mRenderer->BindShader(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, mVertexShader.Get(), mInputLayout.Get(), mPixelShader.Get());

	// Bind the shadow map texture
	mDepthTexture->Bind(0);
	mRenderer->SetSamplerComparisonLE(0);

	// Draw all primitives
	start_instance = 0;
	for (InstanceMap::value_type &v : mPrimitives)
	{
		int num_instances = (int)v.second.size();
		mInstancesBuffer->Draw(static_cast<BatchImpl *>(v.first.GetPtr()), start_instance, num_instances);
		start_instance += num_instances;
	}

	// Front face culling, the next batch needs to render inside out
	mRenderer->SetFrontFaceCulling();

	// Draw all back primitives
	for (InstanceMap::value_type &v : mPrimitivesBackFacing)
	{
		int num_instances = (int)v.second.size();
		mInstancesBuffer->Draw(static_cast<BatchImpl *>(v.first.GetPtr()), start_instance, num_instances);
		start_instance += num_instances;
	}
	assert(start_instance == mNumInstances);

	// Back face culling, this is the default
	mRenderer->SetBackFaceCulling();

	// Unbind the shadow map texture to avoid warning that we're setting it as render target while still bound
	mDepthTexture->UnBind(0);
}

void TriangleRendererImp::Clear()
{ 
	// Move primitives to draw back to the free list
	mPrimitives.clear(); 
	mPrimitivesBackFacing.clear();
	mNumInstances = 0;
}
