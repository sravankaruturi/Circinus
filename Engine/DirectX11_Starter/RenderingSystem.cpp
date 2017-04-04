// -------------------------------------------------------------
//  Base class for a DirectX 11 Game, borrowing from
//  the structure set up by Frank D. Luna in "3D Game
//  Programming with DirectX 11" (d3dApp.h & d3dApp.cpp)
//
//  This version doesn't rely on D3DX or the Effect framework,
//  as these libraries are deprecated.
//
//  You should not need to edit this class at all.
// -------------------------------------------------------------

#include "RenderingSystem.h"
#include "NativeWindow.h"
#include "SceneGraph.h"
#include <WindowsX.h>
#include <sstream>

namespace
{
	// We need a global reference to the DirectX Game so that we can
	// pass OS-level window messages to our application.
	RenderingSystem* renderingSystemInstance = 0;
}

#pragma region Constructor / Destructor

// --------------------------------------------------------
// Constructor - Set up fields and timer
// --------------------------------------------------------
RenderingSystem::RenderingSystem()
	:
	device(0),
	deviceContext(0),
	swapChain(0),
	depthStencilBuffer(0),
	renderTargetView(0),
	depthStencilView(0),
	driverType(D3D_DRIVER_TYPE_HARDWARE),
	featureLevel(D3D_FEATURE_LEVEL_11_0)
{
	// Zero out the viewport struct
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	// Set the global pointer to this RenderingSystem object so we can forward
	// Windows messages to our object's message handling function
	renderingSystemInstance = this;
}

// --------------------------------------------------------
// Destructor - Clean up (release) all DirectX references
// --------------------------------------------------------
RenderingSystem::~RenderingSystem(void)
{
	for (auto i = preBoundCBs.begin(); i != preBoundCBs.end(); ++i)
	{
		i->second.CleanUp();
	}
	for (auto i = meshes.begin(); i != meshes.end(); ++i)
	{
		delete i->second;
	}
	for (auto i = shaders.begin(); i != shaders.end(); ++i)
	{
		delete i->second;
	}
	for (auto i = textures.begin(); i != textures.end(); ++i)
	{
		delete i->second;
	}
	for (auto i = materials.begin(); i != materials.end(); ++i)
	{
		delete *i;
	}

	// Release the core DirectX "stuff" we set up
	ReleaseMacro(renderTargetView);
	ReleaseMacro(depthStencilView);
	ReleaseMacro(swapChain);
	ReleaseMacro(depthStencilBuffer);

	// Restore default device settings
	if (deviceContext)
		deviceContext->ClearState();

	// Release the device context and finally the device itself
	ReleaseMacro(deviceContext);
	ReleaseMacro(device);
}
#pragma endregion

#pragma region Initialization

// --------------------------------------------------------
// Handles the window and Direct3D initialization
// --------------------------------------------------------
bool RenderingSystem::Init(NativeWindow* win)
{
	// Now that the window is ready, initialize
	// DirectX (specifically Direct3D)
	if (!InitDirect3D(win->GetWindowHandle()))
		return false;

	win->SetResizeCallback(std::bind(&RenderingSystem::OnResize, this, std::placeholders::_1, std::placeholders::_2));

	if (!InitPreBoundConstantBuffers())
	{
		return false;
	}

	// Everything was set up properly
	return true;
}


bool RenderingSystem::InitPreBoundConstantBuffers()
{

	ConstantBuffer cb = ConstantBuffer();
	if (!cb.Init(device, sizeof(BuiltinFrameCB)))
		return false;

	preBoundCBs.insert(std::pair<std::string, ConstantBuffer>("FrameConstants", cb));

	return true;
}

void RenderingSystem::UpdatePreBoundConstantBuffers()
{
	for (auto i = preBoundCBs.begin(); i != preBoundCBs.end(); ++i)
	{
		i->second.UploadBuffer(deviceContext);
	}
}

// --------------------------------------------------------
// Create the device, device context, swap chain and 
// necessary buffers to properly draw to the window
// --------------------------------------------------------
bool RenderingSystem::InitDirect3D(void* wndHandle)
{
	UINT createDeviceFlags = 0;

	// Do we want a debug device?
	// A debug device can give us more information when errors or
	// warnings occur, at the cost of a little performance
#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HWND hwnd = (HWND)wndHandle;

	// Set up a swap chain description - 
	// The swap chain is used for double buffering (a.k.a. page flipping)
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferDesc.Width = windowWidth;
	swapChainDesc.BufferDesc.Height = windowHeight;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Create the device and swap chain with the following global function
	// This will also determine the supported feature level (version of DirectX)
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		0,
		driverType,
		0,
		createDeviceFlags,
		0,
		0,
		D3D11_SDK_VERSION,
		&swapChainDesc,		// We pass in the description
		&swapChain,			// And the SWAP CHAIN is created
		&device,			// As well as the DEVICE
		&featureLevel,		// The feature level is determined
		&deviceContext);	// And the CONTEXT is created

							// Was the device created properly?
	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice Failed", 0, 0);
		return false;
	}

	// There are several remaining steps before we can reasonably use DirectX.
	// These steps also need to happen each time the window is resized, 
	// so we simply call the OnResize method here.
	RECT rect;
	GetClientRect(hwnd, &rect);
	OnResize(rect.right, rect.bottom);
	return true;
}
#pragma endregion

#pragma region Window Resizing

// --------------------------------------------------------
// When the window is resized, the underlying buffers (textures) must
// also be resized to match.  
// --------------------------------------------------------
void RenderingSystem::OnResize(int windowWidth, int windowHeight)
{
	// Release any existing views, since we'll be destroying
	// the corresponding buffers.
	ReleaseMacro(renderTargetView);
	ReleaseMacro(depthStencilView);
	ReleaseMacro(depthStencilBuffer);

	// Resize the swap chain to match the new window dimensions
	HR(swapChain->ResizeBuffers(
		1,
		windowWidth,
		windowHeight,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		0));

	// Recreate the render target view that points to the swap chain's buffer
	ID3D11Texture2D* backBuffer;
	HR(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	HR(device->CreateRenderTargetView(backBuffer, 0, &renderTargetView));
	ReleaseMacro(backBuffer);

	// Set up the description of the texture to use for the depth buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = windowWidth;
	depthStencilDesc.Height = windowHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;

	// Create the depth/stencil buffer and corresponding view
	HR(device->CreateTexture2D(&depthStencilDesc, 0, &depthStencilBuffer));
	HR(device->CreateDepthStencilView(depthStencilBuffer, 0, &depthStencilView));

	// Bind these views to the pipeline, so rendering properly 
	// uses the underlying textures
	deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

	// Update the viewport to match the new window size and set it on the device
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)windowWidth;
	viewport.Height = (float)windowHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	deviceContext->RSSetViewports(1, &viewport);

	// Recalculate the aspect ratio, since it probably changed
	aspectRatio = (float)windowWidth / windowHeight;
}
#pragma endregion

#pragma region Draw

void RenderingSystem::DrawScene(Camera* cam, SceneGraph* scene)
{
	float clearColor[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
	deviceContext->ClearRenderTargetView(renderTargetView, clearColor);
	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	std::list<Renderable>& list = scene->renderables;
	for (auto i = list.begin(); i != list.end(); ++i)
	{
		i->material->Apply(deviceContext);
		UINT strides[] = { sizeof(Vertex) };
		UINT offsets[] = { 0 };
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		deviceContext->IASetInputLayout(i->material->shader->layout);
		deviceContext->IASetVertexBuffers(0, 1, &(i->mesh->vertexBuffer), strides, offsets);
		deviceContext->IASetIndexBuffer(i->mesh->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		deviceContext->DrawIndexed(i->mesh->indexCount, 0, 0);
	}

	swapChain->Present(0, 0);
}

#pragma endregion

#pragma region Resources

Mesh * RenderingSystem::CreateMesh(const char* filename)
{
	if (meshes.find(filename) != meshes.end())
	{
		return meshes[filename];
	}

	Mesh* mesh = new Mesh();
	mesh->LoadFromFile(filename, device);

	if (mesh->IsValid())
	{
		meshes.insert(std::pair<std::string, Mesh*>(filename, mesh));
		return mesh;
	}

	delete mesh;
	return mesh;
}

Shader * RenderingSystem::CreateShader(const wchar_t * filename)
{
	if (shaders.find(filename) != shaders.end())
	{
		return shaders[filename];
	}

	Shader* shader = new Shader();
	shader->LoadShaderFromCSO(device, filename);

	if (shader->IsValid())
	{
		shaders.insert(std::pair<std::wstring, Shader*>(filename, shader));
		return shader;
	}

	delete shader;
	return nullptr;
}

Texture * RenderingSystem::CreateTexture(const wchar_t * filename)
{
	if (textures.find(filename) != textures.end())
	{
		return textures[filename];
	}

	Texture* tex = new Texture();
	tex->LoadTextureFromFile(filename, device);

	if (tex->IsValid())
	{
		textures.insert(std::pair<std::wstring, Texture*>(filename, tex));
		return tex;
	}

	delete tex;
	return nullptr;
}

Material * RenderingSystem::CreateMaterial(Shader * shader)
{
	Material* mat = new Material();
	mat->InitWithShader(device, shader, &preBoundCBs);

	if (mat->IsValid())
	{
		materials.push_back(mat);
		return mat;
	}

	delete mat;
	return nullptr;
}

#pragma endregion





