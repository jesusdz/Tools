#define TOOLS_WINDOW
#include "tools.h"

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#include <d3d12.h>          // D3D12 interface
#include <dxgi1_6.h>        // DirectX graphics infrastructure (adapters, presentation, etc)
#include <d3dcompiler.h>    // Utilities to compile HLSL code at runtime
#include <d3d12sdklayers.h>
//#include <d3dx12.h>

#include <stdint.h>

/*
NOTE:
When using runtime compiled HLSL shaders using any of the D3DCompiler functions, do not
forget to link against the d3dcompiler.lib library and copy the D3dcompiler_47.dll to
the same folder as the binary executable when distributing your project.

A redistributable version of the D3dcompiler_47.dll file can be found in the Windows 10
SDK installation folder at C:\Program Files (x86)\Windows Kits\10\Redist\D3D\.

For more information, refer to the MSDN blog post at:
https://blogs.msdn.microsoft.com/chuckw/2012/05/07/hlsl-fxc-and-d3dcompile/
 */

static const UINT FRAME_COUNT = 2;

struct Vertex
{
	float3 position;
	float4 color;
};

struct GfxDevice
{
	ComPtr<ID3D12Device2> g_Device;
	ComPtr<IDXGISwapChain4> g_SwapChain;
	ComPtr<ID3D12Resource> g_BackBuffers[FRAME_COUNT];
	ComPtr<ID3D12CommandQueue> g_CommandQueue;
	ComPtr<ID3D12GraphicsCommandList> g_CommandList;
	ComPtr<ID3D12CommandAllocator> g_CommandAllocators[FRAME_COUNT];
	ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap; // Render Target Views (RTV)
	UINT g_RTVDescriptorSize;
	UINT g_CurrentBackBufferIndex;

	// Synchronization objects
	ComPtr<ID3D12Fence> g_Fence;
	uint64_t g_FenceValue = 0;
	uint64_t g_FrameFenceValues[FRAME_COUNT];
	HANDLE g_FenceEvent;

	// By default, enable V-Sync.
	// Can be toggled with the V key.
	bool g_VSync = true;
	bool g_TearingSupported = false;

	// By default, use windowed mode.
	// Can be toggled with the Alt+Enter or F11
	bool g_Fullscreen = false;
};

#define ThrowIfFailed(hRes) if( FAILED( hRes ) ) { return false; }

bool InitializeGraphics(Arena &arena, Window &window, GfxDevice &gfxDevice)
{
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();

	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	const bool useWarp = false;
	if (useWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ( (dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
					dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory &&
					SUCCEEDED( D3D12CreateDevice( dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr) ) )
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	ComPtr<ID3D12Device2> device;
	ThrowIfFailed(D3D12CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = ARRAY_COUNT(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = ARRAY_COUNT(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = ARRAY_COUNT(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	// Command queue
	ComPtr<ID3D12CommandQueue> commandQueue;
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));
	}


	// Query tearing support
	BOOL allowTearing = FALSE;
	{
		// Rather than create the DXGI 1.5 factory interface directly, we create the
		// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the
		// graphics debugging tools which will not support the 1.5 factory interface
		// until a future update.
		ComPtr<IDXGIFactory4> factory4;
		if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
		{
			ComPtr<IDXGIFactory5> factory5;
			if (SUCCEEDED(factory4.As(&factory5)))
			{
				if (FAILED(factory5->CheckFeatureSupport(
								DXGI_FEATURE_PRESENT_ALLOW_TEARING,
								&allowTearing, sizeof(allowTearing))))
				{
					allowTearing = FALSE;
				}
			}
		}
	}


	// Create the swapchain
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t bufferCount = FRAME_COUNT;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
				commandQueue.Get(),
				window.hWnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
	ThrowIfFailed(dxgiFactory->MakeWindowAssociation(window.hWnd, DXGI_MWA_NO_ALT_ENTER));

	ComPtr<IDXGISwapChain4> swapChain;
	ThrowIfFailed(swapChain1.As(&swapChain));


	// Descriptor heap
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = FRAME_COUNT;
		desc.Type = descriptorHeapType;

		ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvDescriptorHeap)));
	}


	// Render target views (RTV)
	ComPtr<ID3D12Resource> backBuffers[FRAME_COUNT] = {};
	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (int i = 0; i < FRAME_COUNT; ++i)
		{
			ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));
			device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}
	}


	// Command allocators
	ComPtr<ID3D12CommandAllocator> commandAllocators[FRAME_COUNT];
	{
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		for (u32 i = 0; i < FRAME_COUNT; ++i)
		{
			ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocators[i])));
		}
	}


	// Command list
	ComPtr<ID3D12GraphicsCommandList> commandList;
	{
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(device->CreateCommandList(0, type, commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&commandList)));
		ThrowIfFailed(commandList->Close()); // For conveniency, so the first command in the render loop can be Reset
	}


	// Populate device
	gfxDevice.g_Device = device;
	gfxDevice.g_SwapChain = swapChain;
	gfxDevice.g_RTVDescriptorHeap = rtvDescriptorHeap;
	gfxDevice.g_RTVDescriptorSize = rtvDescriptorSize;
	for (u32 i = 0; i < FRAME_COUNT; ++i) { gfxDevice.g_BackBuffers[i] = backBuffers[i]; }
	gfxDevice.g_CommandQueue = commandQueue;
	for (u32 i = 0; i < FRAME_COUNT; ++i) { gfxDevice.g_CommandAllocators[i] = commandAllocators[i]; }
	gfxDevice.g_CommandList = commandList;
	gfxDevice.g_CurrentBackBufferIndex = 0;

	// Synchronization objects
	//ComPtr<ID3D12Fence> g_Fence;
	//uint64_t g_FenceValue = 0;
	//uint64_t g_FrameFenceValues[FRAME_COUNT];
	//HANDLE g_FenceEvent;

	//bool g_VSync = true;
	gfxDevice.g_TearingSupported = allowTearing;
	//bool g_Fullscreen = false;

	return true;
}

void CleanupGraphics(GfxDevice &gfxDevice)
{
}

void RenderGraphics(GfxDevice &gfxDevice)
{
	// IDXGISwapChain::Present
}

int main()
{
	// Create Window
	Window window = {};
	if ( !InitializeWindow(window) )
	{
		LOG(Error, "InitializeWindow failed!\n");
		return -1;
	}

	// Allocate base memory
	u32 baseMemorySize = MB(64);
	byte *baseMemory = (byte*)AllocateVirtualMemory(baseMemorySize);
	Arena arena = MakeArena(baseMemory, baseMemorySize);

	// Initialize graphics
	GfxDevice gfxDevice = {};
	if ( !InitializeGraphics(arena, window, gfxDevice) )
	{
		LOG(Error, "InitializeGraphics failed!\n");
		return -1;
	}

	// Application loop
	while ( 1 )
	{
		ProcessWindowEvents(window);

		if (window.flags & WindowFlags_Exiting)
		{
			break;
		}

		RenderGraphics(gfxDevice);
	}

	CleanupGraphics(gfxDevice);

	CleanupWindow(window);

	PrintArenaUsage(arena);

	return 0;
}

