#define NOMINMAX
#include "stdafx.h"

#include "D3DApp.h"
#include <array>
#include "GeometryGenerator.h"
#include <limits>
#include <Windows.h>
#include <windowsx.h>
#include <wincodec.h>
#include "DirectX/DDSTextureLoader12.h"
#include "MathHelper.h"
#include <DirectXColors.h>
#include <dxgidebug.h>

#include <msxml.h>
#include <atlconv.h>
#include "FileHelper.h"
#include "Material.h"
#include "PreDefines.h"
#include "D3DUtil.h"
#include "UiManager.h"
#include "TypeCommon.h"
#include "SMGFramework.h"
#include "SkinnedData.h"
#include "StageManager.h"
#include "Actor.h"
#include "CharacterInfoManager.h"
#include <algorithm>
#include <corecrt_math.h>
#include "GameObject.h"
#include "ShadowMap.h"
#include "Effect.h"
#include "Camera.h"

void D3DApp::buildShaderResourceViews()
{
	//check(!_textures.empty(), "texture info가 먼저 로드되어야 합니다.");
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
		_srvHeap->GetCPUDescriptorHandleForHeapStart(),
		TEXTURE_SRV_INDEX + _textureLoadedCount,
		_cbvSrcUavDescriptorSize);

	for (int i = _textureLoadedCount; i < _textures.size(); ++i)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format = _textures[i]->_resource->GetDesc().Format;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = _textures[i]->_resource->GetDesc().MipLevels;
		desc.Texture2D.PlaneSlice = 0;
		desc.Texture2D.ResourceMinLODClamp = 0.f;
		_deviceD3d12->CreateShaderResourceView(_textures[i]->_resource.Get(), &desc, handle);
		handle.Offset(1, _cbvSrcUavDescriptorSize);
	}
	_textureLoadedCount = _textures.size();
}

D3DApp::D3DApp()
	: _currentFence(0)
	, _rtvDescriptorSize(0)
	, _dsvDescriptorSize(0)
	, _cbvSrcUavDescriptorSize(0)
	, _frameIndex(0)
	, _currentBackBuffer(0)
	, _textureLoadedCount(0)
	, _projectionMatrix(MathHelper::Identity4x4)
	, _aspectRatio(0)
	, _viewPort()
	, _scissorRect()
	, _sceneBounds(XMFLOAT3(0, 0, 0), 600.f)
	, _mainLightViewMatrix(MathHelper::Identity4x4)
	, _mainLightProjectionMatrix(MathHelper::Identity4x4)
	, _shadowTransform(MathHelper::Identity4x4)
{
	Initialize();
}
D3DApp::~D3DApp()
{
	if (_deviceD3d12 != nullptr)
		flushCommandQueue();

	check(_gameObjects.empty());
	for (int i = 0; i < static_cast<int>(RenderLayer::Count); ++i)
	{
		check(_renderItems[i].empty());
	}
}

void D3DApp::logAdapters(void) noexcept
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	IDXGIFactory* mdxgiFactory = nullptr;
	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&mdxgiFactory);
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());
		adapterList.push_back(adapter);
		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		std::wstring text = to_wstring(i);
		text += L"\n";
		OutputDebugString(text.c_str());
		logAdapterOutput(adapterList[i]);
		adapterList[i]->Release();
		adapterList[i] = nullptr;
	}
	mdxgiFactory->Release();
	mdxgiFactory = nullptr;
}

void D3DApp::logAdapterOutput(IDXGIAdapter* adapter) noexcept
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		
		OutputDebugString(text.c_str());
		logOutputDisplayModes(output, DXGI_FORMAT_B8G8R8A8_UNORM);
		output->Release();
		++i;
	}
}

void D3DApp::logOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) noexcept
{
	UINT count = 0;
	UINT flags = 0;

	output->GetDisplayModeList(format, flags, &count, nullptr);

	vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + to_wstring(x.Width) + L" " +
			L"Height = " + to_wstring(x.Height) + L" " +
			L"Refresh = " + to_wstring(n) + L"/" + to_wstring(d) + L"\n";
		OutputDebugString(text.c_str());
	}
}

void D3DApp::checkFeatureSupport(void) noexcept
{
	D3D_FEATURE_LEVEL featureLevels[3] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelsInfo;
	featureLevelsInfo.NumFeatureLevels = 3;
	featureLevelsInfo.pFeatureLevelRequested = featureLevels;
	 
}

void D3DApp::initDirect3D()
{
	UINT dxgiFactoryFlag = 0;
#if defined(DEBUG) || defined(_DEBUG)
	{
		WComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();

		WComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf())));

		dxgiFactoryFlag = DXGI_CREATE_FACTORY_DEBUG;

		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

		logAdapters();
	}
#endif
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&_factory)));

	HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_deviceD3d12));
	if (FAILED(hardwareResult))
	{
		WComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_deviceD3d12)));
	}
	ThrowIfFailed(_deviceD3d12->CreateFence(_currentFence, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
#if defined DEBUG | defined _DEBUG
	WComPtr<ID3D12InfoQueue> d3d12InfoQueue;
	ThrowIfFailed(_deviceD3d12.As(&d3d12InfoQueue));
	D3D12_MESSAGE_ID hide[] =
	{
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, // d3d d2d clear 문제
	};
	D3D12_INFO_QUEUE_FILTER filter = {};
	filter.DenyList.NumIDs = _countof(hide);
	filter.DenyList.pIDList = hide;
	ThrowIfFailed(d3d12InfoQueue->AddStorageFilterEntries(&filter));
#endif

	_rtvDescriptorSize = _deviceD3d12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_dsvDescriptorSize = _deviceD3d12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_cbvSrcUavDescriptorSize = _deviceD3d12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multiSampleQualityLevels;
	multiSampleQualityLevels.Format = BACK_BUFFER_FORMAT;
	multiSampleQualityLevels.SampleCount = 4;
	multiSampleQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	multiSampleQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(_deviceD3d12->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, 
		&multiSampleQualityLevels, 
		sizeof(multiSampleQualityLevels)));

	_4xMsaaQuality = multiSampleQualityLevels.NumQualityLevels;
	check(_4xMsaaQuality > 0, "Unexpected MSAA quality.");

	createCommandObjects();
	createSwapChain();
	createDescriptorHeaps();

	initShadowMap();
}

void D3DApp::initDirect2D(void)
{
	D2D1_FACTORY_OPTIONS options;
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, _d2dFactory.GetAddressOf()));
	ThrowIfFailed(CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(_wicImageFactory.GetAddressOf())
	));
	WComPtr<IDXGIDevice> deviceDxgi;
	WComPtr<ID3D11Device> deviceD3d11;

	unsigned __int32 DeviceFlags = ::D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	ThrowIfFailed(D3D11On12CreateDevice(_deviceD3d12.Get(),
		DeviceFlags,
		nullptr, 
		0, 
		reinterpret_cast<IUnknown**>(_commandQueue.GetAddressOf()),
		1, 
		0x00000001,
		&deviceD3d11,
		&_d3d11Context, 
		nullptr));

	ThrowIfFailed(deviceD3d11.As(&_deviceD3d11On12));

	ThrowIfFailed(_deviceD3d11On12.As(&deviceDxgi));
	_d2dFactory->CreateDevice(deviceDxgi.Get(), &_deviceD2d);

	ThrowIfFailed(_deviceD2d->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_d2dContext));

	ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), (IUnknown**)&_writeFactory));

	{
		ThrowIfFailed(_writeFactory->CreateTextFormat(L"Righteous", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			48.f,
			L"ko-KR",
			&_textFormats[static_cast<int>(TextFormatType::Normal)]));
		ThrowIfFailed(_textFormats[static_cast<int>(TextFormatType::Normal)]->SetTextAlignment(
			DWRITE_TEXT_ALIGNMENT_CENTER));
		ThrowIfFailed(_textFormats[static_cast<int>(TextFormatType::Normal)]->SetParagraphAlignment(
			DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

		ThrowIfFailed(_writeFactory->CreateTextFormat(L"Righteous", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			54.f,
			L"ko-KR",
			&_textFormats[static_cast<int>(TextFormatType::Points)]));
		ThrowIfFailed(_textFormats[static_cast<int>(TextFormatType::Points)]->SetTextAlignment(
			DWRITE_TEXT_ALIGNMENT_LEADING));
		ThrowIfFailed(_textFormats[static_cast<int>(TextFormatType::Points)]->SetParagraphAlignment(
			DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

		ThrowIfFailed(_writeFactory->CreateTextFormat(L"Righteous", nullptr, DWRITE_FONT_WEIGHT_EXTRA_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			50.f,
			L"ko-KR",
			&_textFormats[static_cast<int>(TextFormatType::PointsOutline)]));
		ThrowIfFailed(_textFormats[static_cast<int>(TextFormatType::PointsOutline)]->SetTextAlignment(
			DWRITE_TEXT_ALIGNMENT_LEADING));
		ThrowIfFailed(_textFormats[static_cast<int>(TextFormatType::PointsOutline)]->SetParagraphAlignment(
			DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
	}
	static_assert(3 == static_cast<int>(TextFormatType::Count), "타입 추가시 작성해야함.");

	WComPtr<ID2D1SolidColorBrush> black;
	WComPtr<ID2D1SolidColorBrush> white;
	ThrowIfFailed(_d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &black));
	ThrowIfFailed(_d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &white));

	_textBrushes[static_cast<int>(TextBrushType::Black)] = black;
	_textBrushes[static_cast<int>(TextBrushType::White)] = white;
	static_assert(2 == static_cast<int>(TextBrushType::Count), "타입 추가시 작성해야함.");
}

void D3DApp::flushCommandQueue()
{
	_currentFence++;

	ThrowIfFailed(_commandQueue->Signal(_fence.Get(), _currentFence));

	if (_fence->GetCompletedValue() < _currentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		if (eventHandle == nullptr)
		{
			ThrowIfFailed(E_OUTOFMEMORY, "CreateEventEX Fail");
		}
		ThrowIfFailed(_fence->SetEventOnCompletion(_currentFence, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void D3DApp::set4XMsaaState(bool value)
{
	if (_4xMsaaState != value)
	{
		_4xMsaaState = value;

		createSwapChain();
		OnResize();
	}
}

void D3DApp::createCommandObjects(void)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(_deviceD3d12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)));
	ThrowIfFailed(_deviceD3d12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_commandAlloc.GetAddressOf())));
	ThrowIfFailed(_deviceD3d12->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAlloc.Get(), nullptr, 
		IID_PPV_ARGS(_commandList.GetAddressOf())));

	_commandList->Close();
}

void D3DApp::createSwapChain()
{
	_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.Width = SMGFramework::Get().getClientWidth();
	sd.BufferDesc.Height = SMGFramework::Get().getClientHeight();
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = BACK_BUFFER_FORMAT;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	sd.SampleDesc.Count = _4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
	
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	sd.OutputWindow = SMGFramework::Get().getHWnd();
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(_factory->CreateSwapChain(_commandQueue.Get(), &sd, _swapChain.GetAddressOf()));

	ThrowIfFailed(_factory->MakeWindowAssociation(SMGFramework::Get().getHWnd(), DXGI_MWA_NO_ALT_ENTER));
}

void D3DApp::createDescriptorHeaps(void)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(_deviceD3d12->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(_rtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(_deviceD3d12->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
	srvHeapDesc.NumDescriptors = TEXTURE_SRV_INDEX + TEXTURE_MAX;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	ThrowIfFailed(_deviceD3d12->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(_srvHeap.GetAddressOf())));
}

void D3DApp::OnResize(void)
{
	check(_deviceD3d12 != nullptr, "device가 null입니다. 초기화를 확인하세요.");
	check(_swapChain != nullptr, "swapChain이 null입니다. 초기화를 확인하세요.");
	check(_commandAlloc != nullptr, "commandAlloc이 null입니다. 초기화를 확인하세요.");

	flushCommandQueue();
	
	// reset ui before resize
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		_backBufferWrapped[i].Reset();
		_backBufferBitmap[i].Reset();
	}
	_d3d11Context->Flush();

	ThrowIfFailed(_commandList->Reset(_commandAlloc.Get(), nullptr), "error!");
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		_swapChainBuffer[i].Reset();
	}
	_depthStencilBuffer.Reset();

	const uint32_t width = SMGFramework::Get().getClientWidth();
	const uint32_t height = SMGFramework::Get().getClientHeight();

	ThrowIfFailed(_swapChain->ResizeBuffers(
		SWAP_CHAIN_BUFFER_COUNT,
		width, height,
		BACK_BUFFER_FORMAT,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	
	_currentBackBuffer = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		ThrowIfFailed(_swapChain->GetBuffer(i, IID_PPV_ARGS(&_swapChainBuffer[i])));
		_deviceD3d12->CreateRenderTargetView(_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, _rtvDescriptorSize);

		constexpr D3D11_RESOURCE_FLAGS resourceFlags = { ::D3D11_BIND_RENDER_TARGET | ::D3D11_BIND_SHADER_RESOURCE };
		ThrowIfFailed(_deviceD3d11On12->CreateWrappedResource(_swapChainBuffer[i].Get(),
			&resourceFlags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(_backBufferWrapped[i].GetAddressOf())
		));

		WComPtr<IDXGISurface> surface;
		ThrowIfFailed(_backBufferWrapped[i]->QueryInterface(surface.GetAddressOf()));

		ThrowIfFailed(_d2dContext->CreateBitmapFromDxgiSurface(surface.Get(), &bitmapProperty, &_backBufferBitmap[i]));
	}

	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;

	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	depthStencilDesc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DEPTH_STENCIL_FORMAT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	CD3DX12_HEAP_PROPERTIES defaultHeapProperty(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(_deviceD3d12->CreateCommittedResource(
		&defaultHeapProperty,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(_depthStencilBuffer.GetAddressOf())));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DEPTH_STENCIL_FORMAT;
	dsvDesc.Texture2D.MipSlice = 0;

	_deviceD3d12->CreateDepthStencilView(
		_depthStencilBuffer.Get(),
		&dsvDesc,
		getDepthStencilView());

	CD3DX12_RESOURCE_BARRIER barrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(
		_depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	_commandList->ResourceBarrier(1, &barrierTransition);

	_viewPort.TopLeftX = 0;
	_viewPort.TopLeftY = 0;
	_viewPort.Width = static_cast<float>(width);
	_viewPort.Height = static_cast<float>(height);
	_viewPort.MinDepth = 0.f;
	_viewPort.MaxDepth = 1.f;

	_scissorRect = { 0, 0, static_cast<int>(width), static_cast<int>(height) };

	ThrowIfFailed(_commandList->Close());
	ID3D12CommandList* cmdLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	flushCommandQueue();

	_aspectRatio = static_cast<double>(width) / height;
	XMMATRIX proj = XMMatrixPerspectiveFovLH(FOV_ANGLE, _aspectRatio, NEAR_Z, FAR_Z);
	XMStoreFloat4x4(&_projectionMatrix, proj);

	BoundingFrustum::CreateFromMatrix(_viewFrustumLocal, proj);
}

void D3DApp::Update(void)
{
	_frameIndex = (_frameIndex + 1) % FRAME_RESOURCE_COUNT;
	const UINT64& currentFrameFence = _frameResources[_frameIndex]->getFence();

	if (currentFrameFence != 0 && _fence->GetCompletedValue() < currentFrameFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		if (eventHandle == nullptr)
		{
			ThrowErrCode(ErrCode::CreateD3dBufferFail, "createEvent Fail");
		}
		ThrowIfFailed(_fence->SetEventOnCompletion(currentFrameFence, eventHandle), "fence set fail : " + std::to_string(currentFrameFence));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
	updateObjectConstantBuffer();
	updateSkinnedConstantBuffer();
	updatePassConstantBuffer();
	updateShadowPassConstantBuffer();
	updateMaterialConstantBuffer();

	SMGFramework::getEffectManager()->updateEffectInstanceData(_frameResources[_frameIndex].get());
}

void D3DApp::Draw(void)
{
	ID3D12CommandAllocator* cmdListAlloc = _frameResources[_frameIndex]->getCommandListAlloc();
	ThrowIfFailed(cmdListAlloc->Reset(), "reset in Draw Failed");

	ThrowIfFailed(_commandList->Reset(cmdListAlloc, _pipelineStateObjectMap[PSOType::Shadow].Get()));
	
	ID3D12DescriptorHeap* descriptorHeaps[] = { _srvHeap.Get() };
	_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	_commandList->SetGraphicsRootSignature(_rootSignature.Get());

	CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle(_srvHeap->GetGPUDescriptorHandleForHeapStart());
	textureHandle.Offset(TEXTURE_SRV_INDEX, _cbvSrcUavDescriptorSize);
	_commandList->SetGraphicsRootDescriptorTable(5, textureHandle);

	drawSceneToShadowMap();

	_commandList->RSSetViewports(1, &_viewPort);
	_commandList->RSSetScissorRects(1, &_scissorRect);

	const CD3DX12_RESOURCE_BARRIER& transitionBarrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
			getCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
	_commandList->ResourceBarrier(1, &transitionBarrier1);
	
	
	const float backgroundColor[4] = { _backgroundColor.x, _backgroundColor.y, _backgroundColor.z, 1.f };

	_commandList->ClearRenderTargetView(getCurrentBackBufferView(), backgroundColor, 0, nullptr);
	_commandList->ClearDepthStencilView(getDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	const D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = getCurrentBackBufferView();
	const D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = getDepthStencilView();
	_commandList->OMSetRenderTargets(1, &backBufferView, true, &depthStencilView);

	_commandList->SetGraphicsRootConstantBufferView(3, _frameResources[_frameIndex]->getPassCBVirtualAddress());
	CD3DX12_GPU_DESCRIPTOR_HANDLE shadowMapHandle(_srvHeap->GetGPUDescriptorHandleForHeapStart());
	_commandList->SetGraphicsRootDescriptorTable(4, shadowMapHandle);
	for (auto e : RenderLayers)
	{
		switch (e)
		{
			case RenderLayer::Opaque:
			{
				_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::Normal].Get());
			}
			break;
			case RenderLayer::OpaqueSkinned:
			{
				_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::Skinned].Get());
			}
			break;
			case RenderLayer::AlphaTested:
			{
				_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::BackSideNotCulling].Get());
			}
			break;
			case RenderLayer::Transparent:
			{
				_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::Transparent].Get());
			}
			break;
			case RenderLayer::Shadow:
			{
				_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::Shadow].Get());
			}
			break;
			case RenderLayer::Background:
			{
				_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::Background].Get());
			}
			break;
			case RenderLayer::GameObjectDev:
			{
#if defined(DEBUG) | defined(_DEBUG)
				if (!SMGFramework::Get()._drawCollisionBox)
				{
					continue;
				}
				_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::GameObjectDev].Get());
#else
				continue;
#endif
			}
			break;
			case RenderLayer::Count:
			default:
			{
				static_assert(static_cast<int>(RenderLayer::Count) == 7, "RenderLayer가 어떤 PSO를 사용할지 정해야합니다.");
				static_assert(static_cast<int>(PSOType::Count) == 10, "PSO 타입이 추가되었다면 확인해주세요");
				ThrowErrCode(ErrCode::UndefinedType, "비정상입니다");
			}
		}
		drawRenderItems(e, true);
 	}
	drawEffects();
	// drawUI 때문에 삭제함 [2/16/2021 qwerwy]
// 	const CD3DX12_RESOURCE_BARRIER& transitionBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
// 		getCurrentBackBuffer(),
// 		D3D12_RESOURCE_STATE_RENDER_TARGET,
// 		D3D12_RESOURCE_STATE_PRESENT);
// 	_commandList->ResourceBarrier(1, &transitionBarrier2);

	ThrowIfFailed(_commandList->Close());

	ID3D12CommandList* cmdLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	//UI
	drawUI();
	
	ThrowIfFailed(_swapChain->Present(0, 0));
	_currentBackBuffer = (_currentBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

	_frameResources[_frameIndex]->setFence(++_currentFence);
	_commandQueue->Signal(_fence.Get(), _currentFence);
}

void D3DApp::prepareCommandQueue(void)
{
	ThrowIfFailed(_commandList->Reset(_commandAlloc.Get(), nullptr));
}

void D3DApp::executeCommandQueue(void)
{
	buildShaderResourceViews();

	ThrowIfFailed(_commandList->Close());
	ID3D12CommandList* cmdLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	flushCommandQueue();
}

ID3D12Resource* D3DApp::getCurrentBackBuffer() const noexcept
{
	return _swapChainBuffer[_currentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::getCurrentBackBufferView() const noexcept
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		_currentBackBuffer, _rtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::getDepthStencilView() const noexcept
{
	return _dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::buildFrameResources(void)
{
	for (int i = 0; i < FRAME_RESOURCE_COUNT; ++i)
	{
		auto frameResource = std::make_unique<FrameResource>(_deviceD3d12.Get(), 2, OBJECT_MAX, MATERIAL_MAX, SKINNED_INSTANCE_MAX, EFFECT_INSTANCE_MAX);
		_frameResources.push_back(std::move(frameResource));
	}
}

void D3DApp::initShadowMap()
{
	_shadowMap = std::make_unique<ShadowMap>(_deviceD3d12.Get(), 2048, 2048);
	auto srvCpuStart = _srvHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = _srvHeap->GetGPUDescriptorHandleForHeapStart();
	auto dsvCpuStart = _dsvHeap->GetCPUDescriptorHandleForHeapStart();

	_shadowMap->buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart),
								CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart),
								CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, _dsvDescriptorSize));
}

void D3DApp::updateShadowTransform(void) noexcept
{
	auto& mainLight = _passConstants._lights[0];
	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&mainLight._direction);
	XMVECTOR lightPos = -2.0f * _sceneBounds.Radius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&_sceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMStoreFloat3(&_shadowPassConstants._cameraPos, lightPos);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLightSpace;
	XMStoreFloat3(&sphereCenterLightSpace, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum in light space encloses scene.
	float leftX = sphereCenterLightSpace.x - _sceneBounds.Radius;
	float rightX = sphereCenterLightSpace.x + _sceneBounds.Radius;
	float bottomY = sphereCenterLightSpace.y - _sceneBounds.Radius;
	float topY = sphereCenterLightSpace.y + _sceneBounds.Radius;
	float nearZ = sphereCenterLightSpace.z - _sceneBounds.Radius;
	float farZ = sphereCenterLightSpace.z + _sceneBounds.Radius;

	_shadowPassConstants._nearZ = nearZ;
	_shadowPassConstants._farZ = farZ;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(leftX, rightX, bottomY, topY, nearZ, farZ);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView * lightProj * T;
	XMStoreFloat4x4(&_mainLightViewMatrix, lightView);
	XMStoreFloat4x4(&_mainLightProjectionMatrix, lightProj);
	XMStoreFloat4x4(&_shadowTransform, S);
}

void D3DApp::updateShadowPassConstantBuffer(void) noexcept
{
	XMMATRIX view = XMLoadFloat4x4(&_mainLightViewMatrix);
	XMMATRIX proj = XMLoadFloat4x4(&_mainLightProjectionMatrix);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMVECTOR viewDet = XMMatrixDeterminant(view);
	XMMATRIX invView = XMMatrixInverse(&viewDet, view);

	XMVECTOR projDet = XMMatrixDeterminant(proj);
	XMMATRIX invProj = XMMatrixInverse(&projDet, proj);

	XMVECTOR viewProjDet = XMMatrixDeterminant(viewProj);
	XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);

	uint32_t width = _shadowMap->getWidth();
	uint32_t height = _shadowMap->getHeight();

	XMStoreFloat4x4(&_shadowPassConstants._view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_shadowPassConstants._invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&_shadowPassConstants._proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&_shadowPassConstants._invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&_shadowPassConstants._viewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&_shadowPassConstants._invViewProj, XMMatrixTranspose(invViewProj));
	
	_shadowPassConstants._renderTargetSize = XMFLOAT2(static_cast<float>(width), static_cast<float>(height));
	_shadowPassConstants._invRenderTargetSize = XMFLOAT2(1.0f / width, 1.0f / height);

	_frameResources[_frameIndex]->setPassCB(1, _shadowPassConstants);
}

void D3DApp::updateObjectConstantBuffer()
{
	auto& currentFrameResource = _frameResources[_frameIndex];
	for (const auto& e : _gameObjects)
	{
		if (e->popDirtyFrame())
		{
			ObjectConstants objectConstants;

			XMMATRIX world = XMLoadFloat4x4(&e->getWorldMatrix());
			XMStoreFloat4x4(&objectConstants._world, XMMatrixTranspose(world));

			XMMATRIX textureTransform = XMLoadFloat4x4(&e->getTextrueTransformMatrix());
			XMStoreFloat4x4(&objectConstants._textureTransform, XMMatrixTranspose(textureTransform));

			currentFrameResource->setObjectCB(e->getObjectConstantBufferIndex(), objectConstants);
		}
	}
}

void D3DApp::updateSkinnedConstantBuffer(void)
{
	auto& currentFrameResource = _frameResources[_frameIndex];

	TickCount64 deltaTick = SMGFramework::Get().getTimer().getDeltaTickCount();
	for (const auto& e : _skinnedInstance)
	{
		e->updateSkinnedAnimation(deltaTick);
		SkinnedConstants skinnedConstants;
		const auto& matrixes = e->getTransformMatrixes();
		std::copy(std::begin(matrixes), std::end(matrixes), &skinnedConstants._boneTransforms[0]);

		currentFrameResource->setSkinnedCB(e->getIndex(), skinnedConstants);
	}
}

void D3DApp::updatePassConstantBuffer()
{
	const Camera* camera = SMGFramework::getCamera();
	XMMATRIX view = XMLoadFloat4x4(&camera->getViewMatrix());
	XMMATRIX proj = XMLoadFloat4x4(&_projectionMatrix);
	XMMATRIX viewProj = view * proj;

	XMVECTOR viewDet = XMMatrixDeterminant(view);
	XMVECTOR projDet = XMMatrixDeterminant(proj);
	XMVECTOR viewProjDet = XMMatrixDeterminant(viewProj);

	XMMATRIX invView = XMMatrixInverse(&viewDet, view);
	XMMATRIX invProj = XMMatrixInverse(&projDet, proj);
	XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);
	XMMATRIX shadowTransform = XMLoadFloat4x4(&_shadowTransform);
	
	XMStoreFloat4x4(&_passConstants._view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_passConstants._invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&_passConstants._proj , XMMatrixTranspose(proj));
	XMStoreFloat4x4(&_passConstants._invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&_passConstants._viewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&_passConstants._invViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&_passConstants._shadowTransform, XMMatrixTranspose(shadowTransform));

	_passConstants._cameraPos = camera->getPosition();
	_passConstants._cameraUpVector = camera->getUpVector();
	_passConstants._cameraRight = camera->getRightVector();

	const float width = static_cast<float>(SMGFramework::Get().getClientWidth());
	const float height = static_cast<float>(SMGFramework::Get().getClientHeight());
	_passConstants._renderTargetSize = XMFLOAT2(static_cast<float>(width), static_cast<float>(height));
	_passConstants._invRenderTargetSize = XMFLOAT2(1.0f / width, 1.0f / height);
	_passConstants._nearZ = NEAR_Z;
	_passConstants._farZ = FAR_Z;
	_passConstants._totalTime = SMGFramework::Get().getTimer().getTotalTime();
	_passConstants._deltaTime = SMGFramework::Get().getTimer().getDeltaTime();

	_passConstants._fogColor = XMFLOAT4(_backgroundColor.x, _backgroundColor.y, _backgroundColor.z, 1.f);
	_passConstants._fogStart = 20.f;
	_passConstants._fogEnd = 5000.f;
	
	_frameResources[_frameIndex]->setPassCB(0, _passConstants);
}

Material* D3DApp::loadXmlMaterial(const std::string& fileName, const std::string& materialName)
{
	std::string materialKey = getMaterialKey(fileName, materialName);
	auto findIt = _materials.find(materialKey);
	if (findIt != _materials.end())
	{
		return findIt->second.get();
	}

	const std::string filePath = "../Resources/XmlFiles/Asset/Material/" + fileName + ".xml";
	XMLReader xmlMaterial;
	xmlMaterial.loadXMLFile(filePath);

	const auto& nodes = xmlMaterial.getRootNode().getChildNodes();
	for (int i = 0; i < nodes.size(); ++i)
	{
		std::string name;
		nodes[i].loadAttribute("Name", name);
		name = fileName + "/" + name;
		if (_materials.find(name) != _materials.end())
		{
			ThrowErrCode(ErrCode::KeyDuplicated, "material 중복 : " + name);
		}
		if (_materials.size() >= MATERIAL_MAX - 1)
		{
			ThrowErrCode(ErrCode::MemoryIsFull, "MaterialConstantBuffer가 " + std::to_string(MATERIAL_MAX) + "를 넘어갑니다.");
		}
		_materials.emplace(name, std::make_unique<Material>(static_cast<int>(_materials.size()), nodes[i]));
	}

	findIt = _materials.find(materialKey);
	if (findIt != _materials.end())
	{
		return findIt->second.get();
	}
	else
	{
		ThrowErrCode(ErrCode::MaterialNotFound, materialKey);
	}
}

BoneInfo* D3DApp::loadXMLBoneInfo(const std::string& fileName)
{
	auto findIt = _boneInfoMap.find(fileName);
	if (findIt != _boneInfoMap.end())
	{
		return findIt->second.get();
	}

	const string filePath = "../Resources/XmlFiles/Asset/Skeleton/" + fileName + ".xml";
	XMLReader xmlSkeleton;
	xmlSkeleton.loadXMLFile(filePath);

	auto it = _boneInfoMap.emplace(fileName, std::make_unique<BoneInfo>(xmlSkeleton.getRootNode()));
	return it.first->second.get();
}

MeshGeometry* D3DApp::loadXMLMeshGeometry(const std::string& fileName)
{
	check(!fileName.empty());
	auto findIt = _geometries.find(fileName);
	if (findIt != _geometries.end())
	{
		return findIt->second.get();
	}

	const string filePath = "../Resources/XmlFiles/Asset/Mesh/" + fileName + ".xml";
	XMLReader xmlMeshGeometry;

	xmlMeshGeometry.loadXMLFile(filePath);

	auto it = _geometries.emplace(fileName, 
		std::make_unique<MeshGeometry>(xmlMeshGeometry.getRootNode(), _deviceD3d12.Get(), _commandList.Get()));
	check(it.second == true);
	return it.first->second.get();
}

AnimationInfo* D3DApp::loadXMLAnimationInfo(const std::string& fileName)
{
	auto findIt = _animationInfoMap.find(fileName);
	if (findIt != _animationInfoMap.end())
	{
		return findIt->second.get();
	}
	const string filePath = "../Resources/XmlFiles/Asset/Animation/" + fileName + ".xml";
	XMLReader xmlAnimation;
	xmlAnimation.loadXMLFile(filePath);

	auto it = _animationInfoMap.emplace(fileName, std::make_unique<AnimationInfo>(xmlAnimation.getRootNode()));
	return it.first->second.get();
}

GameObject* D3DApp::createGameObject(const MeshGeometry* meshGeometry, SkinnedModelInstance* skinnedInstance, uint16_t skinnedBufferIndex) noexcept
{
	check(meshGeometry != nullptr || skinnedInstance == nullptr);
	check(skinnedInstance != nullptr || skinnedBufferIndex == SKINNED_UNDEFINED);

	UINT objCBIndex = popObjectContantBufferIndex();

	_gameObjects.emplace_back(std::make_unique<GameObject>(meshGeometry, objCBIndex, skinnedBufferIndex, skinnedInstance));
	return _gameObjects.back().get();
}

SkinnedModelInstance* D3DApp::createSkinnedInstance(uint16_t& skinnedBufferIndex, const BoneInfo* boneInfo, const AnimationInfo* animationInfo) noexcept
{
	skinnedBufferIndex = popSkinnedContantBufferIndex();

	unique_ptr<SkinnedModelInstance> newSkinnedInstance(std::make_unique<SkinnedModelInstance>(skinnedBufferIndex, boneInfo, animationInfo));
	SkinnedModelInstance* skinnedInstancePtr = newSkinnedInstance.get();
	_skinnedInstance.emplace_back(move(newSkinnedInstance));

	return skinnedInstancePtr;
}
const MeshGeometry* D3DApp::getMeshGeometry(const std::string& meshName) const noexcept
{
	auto it = _geometries.find(meshName);
	if (it != _geometries.end())
	{
		return it->second.get();
	}
	return nullptr;
}
const MeshGeometry* D3DApp::createMeshGeometry(const std::string& meshName, const GeneratedMeshData& meshData)
{
	auto it = _geometries.emplace(meshName, std::make_unique<MeshGeometry>(meshData, _deviceD3d12.Get(), _commandList.Get()));
	if (it.second == false)
	{
		ThrowErrCode(ErrCode::KeyDuplicated, meshName);
	}
	return it.first->second.get();
}

const Material* D3DApp::getMaterial(const std::string& fileName, const std::string& materialName) const noexcept
{
	std::string materialKey = getMaterialKey(fileName, materialName);
	auto findIt = _materials.find(materialKey);
	if (findIt != _materials.end())
	{
		return findIt->second.get();
	}
	return nullptr;
}

GameObject* D3DApp::createObjectFromXML(const std::string& fileName)
{
	const string filePath = "../Resources/XmlFiles/Object/" + fileName + ".xml";
	XMLReader xmlObject;
	xmlObject.loadXMLFile(filePath);

	const auto& childNodes = xmlObject.getRootNode().getChildNodesWithName();
	bool isSkinned;
	xmlObject.getRootNode().loadAttribute("IsSkinned", isSkinned);
	auto childIter = childNodes.end();
	uint16_t skinnedConstantBufferIndex = SKINNED_UNDEFINED;
	SkinnedModelInstance* skinnedInstance = nullptr;

	if (isSkinned)
	{
		childIter = childNodes.find("Skeleton");
		if (childIter == childNodes.end())
		{
			ThrowErrCode(ErrCode::NodeNotFound, "Skeleton 노드가 없습니다.");
		}

		string skeletonName;
		childIter->second.loadAttribute("FileName", skeletonName);
		const BoneInfo* boneInfo = loadXMLBoneInfo(skeletonName);
		
		childIter = childNodes.find("Animation");
		if (childIter == childNodes.end())
		{
			ThrowErrCode(ErrCode::NodeNotFound, "Animation 노드가 없습니다.");
		}
		string animationInfoName;
		childIter->second.loadAttribute("FileName", animationInfoName);
		const AnimationInfo* animationInfo = loadXMLAnimationInfo(animationInfoName);

		skinnedInstance = createSkinnedInstance(skinnedConstantBufferIndex, boneInfo, animationInfo);
	}

	childIter = childNodes.find("Mesh");
	if (childIter == childNodes.end())
	{
		ThrowErrCode(ErrCode::NodeNotFound, "Mesh 노드가 없습니다.");
	}
	string meshFileName;
	const MeshGeometry* mesh = nullptr;
	childIter->second.loadAttribute("FileName", meshFileName);
	if (!meshFileName.empty())
	{
		mesh = loadXMLMeshGeometry(meshFileName);
	}
	
	GameObject* gameObject = createGameObject(mesh, skinnedInstance, skinnedConstantBufferIndex);
	
	if (mesh != nullptr)
	{
		createRenderItems(gameObject, childIter->second);
	}
	return gameObject;
}

#if defined DEBUG | defined _DEBUG
void D3DApp::createGameObjectDev(Actor* actor)
{
	check(actor != nullptr);
	check(actor->getGameObject() != nullptr);
	check(actor->getCharacterInfo() != nullptr);
	
	// collision boundary
	const CharacterInfo* characterInfo = actor->getCharacterInfo();
	GeneratedMeshData meshData;
	switch (characterInfo->getCollisionShape())
	{
		case CollisionShape::Sphere:
		{
			meshData = GeometryGenerator::CreateGeosphere(characterInfo->getRadiusXXX(), 2);
		}
		break;
		case CollisionShape::Box:
		{
			meshData = GeometryGenerator::CreateBox(characterInfo->getSizeXXXX() * 2,
													characterInfo->getSizeYXXX() * 2,
													characterInfo->getSizeZXXX() * 2,
													0);
		}
		break;
		case CollisionShape::Polygon:
		{
			meshData = GeometryGenerator::CreateGeosphere(characterInfo->getRadiusXXX(), 2);
		}
		break;
		case CollisionShape::Count:
		default:
		{
			check(false, "타입 추가시 확인해야함.");
			static_assert(static_cast<int>(CollisionShape::Count) == 4);
		}
	}
	const MeshGeometry* mesh = nullptr;
	auto meshFind = _geometries.find(actor->getCharacterInfo()->getName() + "_CollisionBox");
	if (meshFind == _geometries.end())
	{
		auto meshIt = _geometries.emplace(actor->getCharacterInfo()->getName() + "_CollisionBox",
			std::make_unique<MeshGeometry>(meshData, _deviceD3d12.Get(), _commandList.Get()));
		mesh = meshIt.first->second.get();
	}
	else
	{
		mesh = meshFind->second.get();
	}

 	auto parentObject = const_cast<GameObject*>(actor->getGameObject());
	auto object = createGameObject(mesh, nullptr, SKINNED_UNDEFINED);
 	parentObject->_devObjects.emplace_back(object);
	
	auto renderItem = make_unique<RenderItem>(
		object,
		loadXmlMaterial("devMat", "green"),
		D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
		0,
		RenderLayer::GameObjectDev);

	std::vector<RenderItem*> renderItems;
	renderItems.push_back(renderItem.get());
	object->setRenderItemsXXX(std::move(renderItems));
	_renderItems[static_cast<int>(RenderLayer::GameObjectDev)].emplace_back(std::move(renderItem));
}

void D3DApp::createGameObjectDev(GameObject* gameObject)
{
	// normal vector
	GeneratedMeshData normalLineMeshData;
	
	size_t bufferSize = 0;
	const Vertex* bufferPointer = gameObject->getMeshGeometry()->getVertexBufferXXX(bufferSize);
	normalLineMeshData._vertices.reserve(bufferSize * 2);
	normalLineMeshData._indices.reserve(bufferSize * 2);
	for (int i = 0; i < bufferSize; ++i)
	{
		Vertex from, to;
		from._position = MathHelper::add(bufferPointer[i]._position, MathHelper::mul(bufferPointer[i]._normal, -100));
		from._textureCoord = DirectX::XMFLOAT2(0, 0);
		to._position = MathHelper::add(bufferPointer[i]._position, MathHelper::mul(bufferPointer[i]._normal, 200));
		to._textureCoord = DirectX::XMFLOAT2(1, 1);
		normalLineMeshData._vertices.push_back(from);
		normalLineMeshData._vertices.push_back(to);
		normalLineMeshData._indices.push_back(2 * i);
		normalLineMeshData._indices.push_back(2 * i + 1);
	}

	auto meshIt = _geometries.emplace("NormalVector" + std::to_string(gameObject->getObjectConstantBufferIndex()),
		std::make_unique<MeshGeometry>(normalLineMeshData, _deviceD3d12.Get(), _commandList.Get()));

	auto object = createGameObject(meshIt.first->second.get(), nullptr, SKINNED_UNDEFINED);
	gameObject->_devObjects.emplace_back(object);

	auto renderItem = make_unique<RenderItem>(
		object,
		loadXmlMaterial("devMat", "blueToRed"),
		D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
		0,
		RenderLayer::GameObjectDev);

	std::vector<RenderItem*> renderItems;
	renderItems.push_back(renderItem.get());
	object->setRenderItemsXXX(std::move(renderItems));
	_renderItems[static_cast<int>(renderItem->_renderLayer)].emplace_back(std::move(renderItem));
}
#endif

void D3DApp::releaseItemsForStageLoad(bool isReload) noexcept
{
	flushCommandQueue();
	Sleep(100);

	for (int i = 0; i < static_cast<int>(RenderLayer::Count); ++i)
	{
		_renderItems[i].clear();
	}
	_gameObjects.clear();
	_skinnedInstance.clear();
	_objectCBReturned = std::queue<UINT>();
	_objectCBIndexCount = 0;
	_skinnedCBReturned = std::queue<uint16_t>();
	_skinnedCBIndexCount = 0;

	if (!isReload)
	{
		_textures.clear();
		_textureIndexMap.clear();
		_textureLoadedCount = 0;
		_materials.clear();
		_boneInfoMap.clear();
		_animationInfoMap.clear();
		_geometries.clear();
		_bitmapImages.clear();
	}
}

void D3DApp::createRenderItems(GameObject* gameObject, const XMLReaderNode& node)
{
	check(gameObject->getMeshGeometry() != nullptr);

	const auto& subMeshList = gameObject->getMeshGeometry()->_subMeshList;
	const auto& subMeshNodes = node.getChildNodes();

	std::vector<RenderItem*> renderItems;
	if (subMeshList.size() > std::numeric_limits<uint8_t>::max())
	{
		ThrowErrCode(ErrCode::Overflow, "subMesh 갯수가 255개를 넘음");
	}
	renderItems.reserve(subMeshNodes.size());

	for (int j = 0; j < subMeshNodes.size(); ++j)
	{
		string subMeshName;
		subMeshNodes[j].loadAttribute("Name", subMeshName);
		auto subMeshIt = find(subMeshList.begin(), subMeshList.end(), subMeshName);
		if (subMeshIt == subMeshList.end())
		{
			ThrowErrCode(ErrCode::SubMeshNotFound, "SubMeshName : " + subMeshName + "이 없습니다.");
		}
		string materialFile, materialName;
		subMeshNodes[j].loadAttribute("MaterialFile", materialFile);
		subMeshNodes[j].loadAttribute("MaterialName", materialName);
		Material* material = loadXmlMaterial(materialFile, materialName);
		if ((gameObject->isSkinnedAnimationObject()) != (material->getRenderLayer() == RenderLayer::OpaqueSkinned))
		{
			ThrowErrCode(ErrCode::NotSkinnedMaterial, materialFile + "/" + materialName);
		}

		RenderLayer renderLayer = material->getRenderLayer();
		uint8_t subMeshIndex = subMeshIt - subMeshList.begin();
		auto renderItem = make_unique<RenderItem>(
			gameObject,
			material,
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
			subMeshIndex,
			renderLayer);

		renderItems.push_back(renderItem.get());
		_renderItems[static_cast<int>(renderLayer)].emplace_back(std::move(renderItem));
	}

	gameObject->setRenderItemsXXX(std::move(renderItems));
}

void D3DApp::drawUI(void)
{
	_deviceD3d11On12->AcquireWrappedResources(_backBufferWrapped[_currentBackBuffer].GetAddressOf(), 1);

	_d2dContext->SetTarget(_backBufferBitmap[_currentBackBuffer].Get());
	_d2dContext->BeginDraw();

	SMGFramework::Get().getUIManager()->drawUI();

	_d2dContext->EndDraw();
	_d2dContext->SetTarget(nullptr);

	_deviceD3d11On12->ReleaseWrappedResources(_backBufferWrapped[_currentBackBuffer].GetAddressOf(), 1);

	_d3d11Context->Flush();
}

void D3DApp::drawSceneToShadowMap(void)
{
	_commandList->RSSetViewports(1, &_shadowMap->getViewPort());
	_commandList->RSSetScissorRects(1, &_shadowMap->getScissorRect());

	CD3DX12_RESOURCE_BARRIER depthWriteBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_shadowMap->getResource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	_commandList->ResourceBarrier(1, &depthWriteBarrier);
	auto dsv = _shadowMap->getDsv();
	_commandList->ClearDepthStencilView(dsv,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
	
	_commandList->OMSetRenderTargets(0, nullptr, false, &dsv);

	UINT passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	auto shadowPassCBAddress = _frameResources[_frameIndex]->getPassCBVirtualAddress() + 1 * passCBByteSize;
	_commandList->SetGraphicsRootConstantBufferView(3, shadowPassCBAddress);

	_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::Shadow].Get());

	drawRenderItems(RenderLayer::Opaque, false);
	drawRenderItems(RenderLayer::AlphaTested, false);

	_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::ShadowSkinned].Get());
	drawRenderItems(RenderLayer::OpaqueSkinned, false);

	static_assert(static_cast<int>(RenderLayer::Count) == 7, "그림자가 생겨야하는 레이어라면 추가해주세요.");

	CD3DX12_RESOURCE_BARRIER readBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_shadowMap->getResource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
	_commandList->ResourceBarrier(1, &readBarrier);
}

void D3DApp::drawEffects(void)
{
	_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::Effect].Get());

	auto meshGeometry = SMGFramework::getEffectManager()->getMeshGeometry();
	D3D12_VERTEX_BUFFER_VIEW vbv = meshGeometry->getVertexBufferView();
	_commandList->IASetVertexBuffers(0, 1, &vbv);
	D3D12_INDEX_BUFFER_VIEW ibv = meshGeometry->getIndexBufferView();
	_commandList->IASetIndexBuffer(&ibv);
	_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	auto instanceBufferAddress = _frameResources[_frameIndex]->getEffectBufferVirtualAddress();
	_commandList->SetGraphicsRootShaderResourceView(6, instanceBufferAddress);
	const auto& subMesh = meshGeometry->_subMeshList[0];
	_commandList->DrawIndexedInstanced(
		subMesh._indexCount,
		SMGFramework::getEffectManager()->getInstanceCount(),
		subMesh._baseIndexLoacation,
		subMesh._baseVertexLoaction,
		0);
}

std::string D3DApp::getMaterialKey(const std::string& fileName, const std::string& materialName) const noexcept
{
	return fileName + "/" + materialName;
}

void D3DApp::removeRenderItem(const RenderLayer renderLayer, const RenderItem* renderItem) noexcept
{
	auto& renderItems = _renderItems[static_cast<int>(renderLayer)];
	for (int i = 0; i < renderItems.size(); ++i)
	{
		if (renderItems[i].get() == renderItem)
		{
			renderItems[i] = std::move(renderItems.back());
			renderItems.pop_back();
			return;
		}
	}
	check(false);
}

void D3DApp::removeSkinnedInstance(const SkinnedModelInstance* skinnedInstance) noexcept
{
	for (int i = 0; i < _skinnedInstance.size(); ++i)
	{
		if (_skinnedInstance[i].get() == skinnedInstance)
		{
			_skinnedInstance[i] = std::move(_skinnedInstance.back());
			_skinnedInstance.pop_back();
			return;
		}
	}
	check(false);
}

void D3DApp::removeGameObject(const GameObject* gameObject) noexcept
{
	for (int i = 0; i < _gameObjects.size(); ++i)
	{
		if (_gameObjects[i].get() == gameObject)
		{
			_gameObjects[i] = std::move(_gameObjects.back());
			_gameObjects.pop_back();
			return;
		}
	}
	check(false);
}

bool XM_CALLCONV D3DApp::checkCulled(const DirectX::BoundingBox& box, FXMMATRIX world) const noexcept
{
	XMVECTOR worldDet = XMMatrixDeterminant(world);
	XMMATRIX invWorld = XMMatrixInverse(&worldDet, world);
	XMMATRIX viewToLocal = XMMatrixMultiply(XMLoadFloat4x4(&SMGFramework::getCamera()->getInvViewMatrix()), invWorld);

	BoundingFrustum localSpaceFrustum;
	_viewFrustumLocal.Transform(localSpaceFrustum, viewToLocal);

	if (localSpaceFrustum.Contains(box) != DirectX::DISJOINT)
	{
		return false;
	}
	return true;
}

void D3DApp::setLight(const std::vector<Light>& lights, const DirectX::XMFLOAT4& ambientLight) noexcept
{
	check(0 < lights.size() && lights.size() <= MAX_LIGHT_COUNT);
	for (int i = 0; i < lights.size(); ++i)
	{
		_passConstants._lights[i] = lights[i];
	}
	_passConstants._ambientLight = ambientLight;
	updateShadowTransform();
}

void D3DApp::setBackgroundColor(const DirectX::XMFLOAT3& color) noexcept
{
	_backgroundColor = color;
}

void D3DApp::updateMaterialConstantBuffer(void)
{
	auto& currentFrameResource = _frameResources[_frameIndex];
	for (auto it = _materials.begin(); it != _materials.end(); ++it)
	{
		Material* mat = it->second.get();
		if (mat->_dirtyFrames > 0)
		{
			MaterialConstants matConstants;
			matConstants._diffuseAlbedo = mat->getDiffuseAlbedo();
			matConstants._fresnelR0 = mat->getFresnelR0();
			matConstants._roughness = mat->getRoughness();
			matConstants._diffuseMapIndex = mat->getDiffuseSRVHeapIndex();

			XMMATRIX matTransform = XMLoadFloat4x4(&mat->getMaterialTransform());
			XMStoreFloat4x4(&matConstants._materialTransform, XMMatrixTranspose(matTransform));

			currentFrameResource->setMaterialCB(mat->getMaterialCBIndex(), matConstants);

			--mat->_dirtyFrames;
		}
	}
}

UINT D3DApp::popObjectContantBufferIndex(void)
{
	if (_objectCBReturned.empty())
	{
		if (_objectCBIndexCount >= OBJECT_MAX - 1)
		{
			ThrowErrCode(ErrCode::MemoryIsFull, "ObjectConstantBuffer가 " + std::to_string(OBJECT_MAX) + "를 넘어갑니다.");
		}
		return _objectCBIndexCount++;
	}
	else
	{
		UINT rv = _objectCBReturned.front();
		_objectCBReturned.pop();

		return rv;
	}
}

uint16_t D3DApp::popSkinnedContantBufferIndex(void)
{
	if (_skinnedCBReturned.empty())
	{
		if (_skinnedCBIndexCount >= SKINNED_INSTANCE_MAX - 1)
		{
			ThrowErrCode(ErrCode::MemoryIsFull, "SkinnedConstantBuffer가 " + std::to_string(SKINNED_INSTANCE_MAX) + "를 넘어갑니다.");
		}
		return _skinnedCBIndexCount++;
	}
	else
	{
		uint16_t rv = _skinnedCBReturned.front();
		_skinnedCBReturned.pop();

		return rv;
	}
}

void D3DApp::pushObjectContantBufferIndex(UINT index) noexcept
{
	_objectCBReturned.push(index);
}

void D3DApp::pushSkinnedContantBufferIndex(uint16_t index) noexcept
{
	_skinnedCBReturned.push(index);
}

void D3DApp::drawRenderItems(const RenderLayer renderLayer, bool checkCulled)
{
	int renderLayerIdx = static_cast<int>(renderLayer);
	UINT objectCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT skinnedCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(SkinnedConstants));
	UINT materialCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	D3D12_GPU_VIRTUAL_ADDRESS objectCBBaseAddress = _frameResources[_frameIndex]->getObjectCBVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS skinnedCBBaseAddress = _frameResources[_frameIndex]->getSkinnedCBVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS materialCBBaseAddress = _frameResources[_frameIndex]->getMaterialCBVirtualAddress();
	
	for (int rIdx = 0; rIdx < _renderItems[renderLayerIdx].size(); ++rIdx)
	{
		auto renderItem = _renderItems[renderLayerIdx][rIdx].get();
		check(renderItem != nullptr, "비정상입니다.");
		if (checkCulled && renderItem->_parentObject->isCulled())
		{
			continue;
		}

		auto meshGeometry = renderItem->_parentObject->getMeshGeometry();
		D3D12_VERTEX_BUFFER_VIEW vbv = meshGeometry->getVertexBufferView();
		_commandList->IASetVertexBuffers(0, 1, &vbv);
		D3D12_INDEX_BUFFER_VIEW ibv = meshGeometry->getIndexBufferView();
		_commandList->IASetIndexBuffer(&ibv);
		_commandList->IASetPrimitiveTopology(renderItem->_primitive);

		D3D12_GPU_VIRTUAL_ADDRESS objectCBaddress = objectCBBaseAddress + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(renderItem->_parentObject->getObjectConstantBufferIndex()) * objectCBByteSize;
		_commandList->SetGraphicsRootConstantBufferView(0, objectCBaddress);

		auto skinnedIndex = renderItem->_parentObject->getSkinnedConstantBufferIndex();
		if (skinnedIndex != SKINNED_UNDEFINED)
		{
			D3D12_GPU_VIRTUAL_ADDRESS skinnedCBAdress = skinnedCBBaseAddress + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(skinnedIndex) * skinnedCBByteSize;
			_commandList->SetGraphicsRootConstantBufferView(1, skinnedCBAdress);
		}
		D3D12_GPU_VIRTUAL_ADDRESS materialCBAddress = materialCBBaseAddress + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(renderItem->_material->getMaterialCBIndex()) * materialCBByteSize;
		_commandList->SetGraphicsRootConstantBufferView(2, materialCBAddress);
		
		const auto& subMesh = meshGeometry->_subMeshList[renderItem->_subMeshIndex];
		_commandList->DrawIndexedInstanced(
			subMesh._indexCount,
			1,
			subMesh._baseIndexLoacation,
			subMesh._baseVertexLoaction,
			0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> D3DApp::getStaticSampler(void) const
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
		);
	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);
	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);
	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8
	);
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8
	);
	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6,
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		0.0f,
		16,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK
	);

	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp, shadow };
}
void D3DApp::buildRootSignature(void)
{
	CD3DX12_DESCRIPTOR_RANGE shadowMapTable;
	shadowMapTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE textureTable;
	textureTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, TEXTURE_MAX, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[7];

	slotRootParameter[0].InitAsConstantBufferView(0); // objectConstant
	slotRootParameter[1].InitAsConstantBufferView(1); // skinnedConstant
	slotRootParameter[2].InitAsConstantBufferView(2); // materialDatas
	slotRootParameter[3].InitAsConstantBufferView(3); // passConstant
	slotRootParameter[4].InitAsDescriptorTable(1, &shadowMapTable, D3D12_SHADER_VISIBILITY_PIXEL); // shadowMap
	slotRootParameter[5].InitAsDescriptorTable(1, &textureTable, D3D12_SHADER_VISIBILITY_PIXEL); // textures
	slotRootParameter[6].InitAsShaderResourceView(1, 1); // instanceDatas

	auto staticSampler = getStaticSampler();
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(7, slotRootParameter, staticSampler.size(), staticSampler.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	WComPtr<ID3DBlob> serializedRootSig = nullptr;
	WComPtr<ID3DBlob> errorBlob = nullptr;
	
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr, "RootSignature serialize fail!");

	ThrowIfFailed(_deviceD3d12->CreateRootSignature(
		0, 
		serializedRootSig->GetBufferPointer(), 
		serializedRootSig->GetBufferSize(), 
		IID_PPV_ARGS(_rootSignature.GetAddressOf())), "create root signature fail");
}

void D3DApp::buildShaders(void)
{
	_shaders["defaultVS"]			= D3DUtil::CompileShader(L"Shaders\\Default.hlsl", definesForShader, "DefaultVertexShader", "vs_5_1");
	_shaders["skinnedVS"]			= D3DUtil::CompileShader(L"Shaders\\Default.hlsl", definesForSkinnedVertexShader, "DefaultVertexShader", "vs_5_1");
	_shaders["defaultPS"]			= D3DUtil::CompileShader(L"Shaders\\Default.hlsl", definesForShader, "DefaultPixelShader", "ps_5_1");
	_shaders["shadowVS"]			= D3DUtil::CompileShader(L"Shaders\\Shadow.hlsl", definesForShader, "ShadowVertexShader", "vs_5_1");
	_shaders["shadowAlphaTestPS"]	= D3DUtil::CompileShader(L"Shaders\\Shadow.hlsl", definesForAlphaTestShader, "ShadowPixelShader", "ps_5_1");
	_shaders["shadowSkinnedVS"]		= D3DUtil::CompileShader(L"Shaders\\Shadow.hlsl", definesForSkinnedVertexShader, "ShadowVertexShader", "vs_5_1");
	_shaders["effectVS"]			= D3DUtil::CompileShader(L"Shaders\\Effect.hlsl", definesForShader, "EffectVertexShader", "vs_5_1");
	_shaders["effectGS"]			= D3DUtil::CompileShader(L"Shaders\\Effect.hlsl", definesForShader, "EffectGeoShader", "gs_5_1");
	_shaders["effectPS"]			= D3DUtil::CompileShader(L"Shaders\\Effect.hlsl", definesForShader, "EffectPixelShader", "ps_5_1");
	_shaders["alphaTestPS"]			= D3DUtil::CompileShader(L"Shaders\\Default.hlsl", definesForAlphaTestShader, "DefaultPixelShader", "ps_5_1");
	_shaders["backgroundVS"]		= D3DUtil::CompileShader(L"Shaders\\Background.hlsl", definesForShader, "BackgroundVertexShader", "vs_5_1");
	_shaders["backgroundPS"]		= D3DUtil::CompileShader(L"Shaders\\Background.hlsl", definesForShader, "BackgroundPixelShader", "ps_5_1");
}

void D3DApp::buildPipelineStateObject(void)
{
	check(_pipelineStateObjectMap.size() == 0, "pso가 이미 생성되어있습니다");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescNormal;
	ZeroMemory(&psoDescNormal, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
	D3D12_DEPTH_STENCIL_DESC transparentStencilDesc;
	D3D12_DEPTH_STENCIL_DESC uiDepthStencilesc;

	// psoDescNormal
	{
		psoDescNormal.pRootSignature = _rootSignature.Get();
		psoDescNormal.VS = 
		{ 
			reinterpret_cast<BYTE*>(_shaders["defaultVS"]->GetBufferPointer()),
			_shaders["defaultVS"]->GetBufferSize()
		};
		psoDescNormal.PS =
		{ 
			reinterpret_cast<BYTE*>(_shaders["defaultPS"]->GetBufferPointer()),
			_shaders["defaultPS"]->GetBufferSize()
		};
		psoDescNormal.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDescNormal.SampleMask = UINT_MAX;
		psoDescNormal.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDescNormal.InputLayout = { VERTEX_INPUT_LAYOUT, VERTEX_INPUT_DESC_SIZE };
		psoDescNormal.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDescNormal.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDescNormal.NumRenderTargets = 1;
		psoDescNormal.RTVFormats[0] = BACK_BUFFER_FORMAT;
		psoDescNormal.DSVFormat = DEPTH_STENCIL_FORMAT;
		psoDescNormal.SampleDesc.Count = _4xMsaaState ? 4 : 1;
		psoDescNormal.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
	}
	// blendDesc
	{
		blendDesc.BlendEnable = true;
		blendDesc.LogicOpEnable = false;
		blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	// transparentStencilDesc
	{
		transparentStencilDesc.DepthEnable = true;
		transparentStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		transparentStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		transparentStencilDesc.StencilEnable = true;
		transparentStencilDesc.StencilReadMask = 0xff;
		transparentStencilDesc.StencilWriteMask = 0xff;

		transparentStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		transparentStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		transparentStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		transparentStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

		transparentStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		transparentStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		transparentStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		transparentStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	}
	//uiDepthStencilesc
	{
		uiDepthStencilesc.DepthEnable = false;
	}
	{

		auto normalPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::Normal, nullptr));
		check(normalPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Normal");
		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescNormal, IID_PPV_ARGS(&normalPSO.first->second)),
						"PSO 생성 실패! PSOType::Normal");
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescSkinned = psoDescNormal;
		psoDescSkinned.InputLayout = { SKINNED_VERTEX_INPUT_LAYOUT, SKINNED_VERTEX_INPUT_DESC_SIZE };
		psoDescSkinned.VS = 
		{
			reinterpret_cast<BYTE*>(_shaders["skinnedVS"]->GetBufferPointer()),
			_shaders["skinnedVS"]->GetBufferSize()
		};
		psoDescSkinned.PS =
		{
			reinterpret_cast<BYTE*>(_shaders["alphaTestPS"]->GetBufferPointer()),
			_shaders["alphaTestPS"]->GetBufferSize()
		};
		auto skinnedPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::Skinned, nullptr));
		check(skinnedPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Skinned");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescSkinned, IID_PPV_ARGS(&skinnedPSO.first->second)),
			"PSO 생성 실패! PSOType::Skinned");
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescBackSideNotCulling = psoDescNormal;
		psoDescBackSideNotCulling.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDescBackSideNotCulling.PS =
		{
			reinterpret_cast<BYTE*>(_shaders["alphaTestPS"]->GetBufferPointer()),
			_shaders["alphaTestPS"]->GetBufferSize()
		};
		auto backSideNotCullingPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::BackSideNotCulling, nullptr));
		check(backSideNotCullingPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::BackSideNotCulling");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescBackSideNotCulling, IID_PPV_ARGS(&backSideNotCullingPSO.first->second)),
						"PSO 생성 실패! PSOType::BackSideNotCulling");
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescTransparent = psoDescNormal;


		psoDescTransparent.BlendState.RenderTarget[0] = blendDesc;
		psoDescTransparent.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		auto transparentPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::Transparent, nullptr));
		check(transparentPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Transparent");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescTransparent, IID_PPV_ARGS(&transparentPSO.first->second)),
					"PSO 생성 실패! PSOType::Transparent");
	}

	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescShadow = psoDescNormal;

		psoDescShadow.RasterizerState.DepthBias = 10000;
		psoDescShadow.RasterizerState.DepthBiasClamp = 0.f;
 		psoDescShadow.RasterizerState.SlopeScaledDepthBias = 0.05f;
		psoDescShadow.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDescShadow.VS =
		{
			reinterpret_cast<BYTE*>(_shaders["shadowVS"]->GetBufferPointer()),
			_shaders["shadowVS"]->GetBufferSize()
		};
		psoDescShadow.PS =
		{
			reinterpret_cast<BYTE*>(_shaders["shadowAlphaTestPS"]->GetBufferPointer()),
			_shaders["shadowAlphaTestPS"]->GetBufferSize()
		};

		psoDescShadow.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDescShadow.NumRenderTargets = 0;

		auto shadowPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::Shadow, nullptr));
		check(shadowPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Shadow");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescShadow, IID_PPV_ARGS(&shadowPSO.first->second)),
					"PSO 생성 실패! PSOType::Shadow");

		psoDescShadow.InputLayout = { SKINNED_VERTEX_INPUT_LAYOUT, SKINNED_VERTEX_INPUT_DESC_SIZE };
		psoDescShadow.VS = 
		{
			reinterpret_cast<BYTE*>(_shaders["shadowSkinnedVS"]->GetBufferPointer()),
			_shaders["shadowSkinnedVS"]->GetBufferSize()
		};
		auto shadowSkinnedPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::ShadowSkinned, nullptr));
		check(shadowSkinnedPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::ShadowSkinned");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescShadow, IID_PPV_ARGS(&shadowSkinnedPSO.first->second)),
			"PSO 생성 실패! PSOType::ShadowSkinned");
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescEffect = psoDescNormal;
		psoDescEffect.VS = 
		{
			reinterpret_cast<BYTE*>(_shaders["effectVS"]->GetBufferPointer()),
			_shaders["effectVS"]->GetBufferSize()
		};
		psoDescEffect.GS =
		{
			reinterpret_cast<BYTE*>(_shaders["effectGS"]->GetBufferPointer()),
			_shaders["effectGS"]->GetBufferSize()
		};
		psoDescEffect.PS =
		{
			reinterpret_cast<BYTE*>(_shaders["effectPS"]->GetBufferPointer()),
			_shaders["effectPS"]->GetBufferSize()
		};
		psoDescEffect.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		psoDescEffect.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		psoDescEffect.BlendState.RenderTarget[0] = blendDesc;
		psoDescEffect.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		auto effectPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::Effect, nullptr));
		check(effectPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Effect");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescEffect, IID_PPV_ARGS(&effectPSO.first->second)),
			"PSO 생성 실패! PSOType::Effect");

	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescBackground = psoDescNormal;

		psoDescBackground.BlendState.RenderTarget[0] = blendDesc;
		psoDescBackground.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDescBackground.VS = 
		{
			reinterpret_cast<BYTE*>(_shaders["backgroundVS"]->GetBufferPointer()),
			_shaders["backgroundVS"]->GetBufferSize()
		};
		psoDescBackground.PS =
		{
			reinterpret_cast<BYTE*>(_shaders["backgroundPS"]->GetBufferPointer()),
			_shaders["backgroundPS"]->GetBufferSize()
		};
		psoDescBackground.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		auto backgroundPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::Background, nullptr));
		check(backgroundPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Background");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescBackground, IID_PPV_ARGS(&backgroundPSO.first->second)),
			"PSO 생성 실패! PSOType::Background");
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescUI = psoDescNormal;

		auto UIPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::UI, nullptr));
		check(UIPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Shadow");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescUI, IID_PPV_ARGS(&UIPSO.first->second)),
			"PSO 생성 실패! PSOType::UIPSO");
	}
#if defined(DEBUG) | defined(_DEBUG)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescGameObjectDev = psoDescNormal;

		psoDescGameObjectDev.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDescGameObjectDev.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		auto gameObjectDevPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::GameObjectDev, nullptr));
		check(gameObjectDevPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Shadow");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescGameObjectDev, IID_PPV_ARGS(&gameObjectDevPSO.first->second)),
			"PSO 생성 실패! PSOType::GameObjectDev");
	}
#endif
	static_assert(static_cast<int>(PSOType::Count) == 10, "PSO 타입이 추가되었다면 확인해주세요.");
}

bool D3DApp::Initialize(void)
{
	initDirect3D();
	initDirect2D();

	OnResize();

	ThrowIfFailed(_commandList->Reset(_commandAlloc.Get(), nullptr));

	buildRootSignature();
	buildShaders();
	buildPipelineStateObject();

	// 이 이후로는 맵 전환마다 수행되어야 할듯 [1/21/2021 qwerw]

	//loadInfoMap();
	buildShaderResourceViews();

	//buildConstantGeometry();
	//BuildMaterials();
	//buildGameObjects();
	buildFrameResources();

	ThrowIfFailed(_commandList->Close());
	ID3D12CommandList* cmdLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	flushCommandQueue();

	return true;
}

uint16_t D3DApp::loadTexture(const string& textureName)
{
	auto it = _textureIndexMap.find(textureName);
	if (it != _textureIndexMap.end())
	{
		return it->second;
	}
	if (std::numeric_limits<uint16_t>::max() <= _textures.size())
	{
		ThrowErrCode(ErrCode::Overflow, "Texture index가 범위를 넘어갑니다.");
	}
	std::wstring fileName;
	fileName.assign(textureName.begin(), textureName.end());
	fileName = L"../Resources/XmlFiles/Asset/Texture/" + fileName + L".dds";

	auto texture = std::make_unique<Texture>();
	texture->_name = textureName;
	texture->_fileName = fileName;

	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;

	ThrowIfFailed(
		LoadDDSTextureFromFile(
			_deviceD3d12.Get(),
			texture->_fileName.c_str(),
			texture->_resource.GetAddressOf(),
			ddsData,
			subresources),
		"LoadDDSTexture Fail : " + textureName);

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->_resource.Get(), 0,
		static_cast<UINT>(subresources.size()));

	// Create the GPU upload buffer.
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

	auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	ThrowIfFailed(
		_deviceD3d12->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(texture->_uploader.GetAddressOf())), "texture upload fail");

	UpdateSubresources(_commandList.Get(), texture->_resource.Get(), texture->_uploader.Get(),
		0, 0, static_cast<UINT>(subresources.size()), subresources.data());

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture->_resource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	_commandList->ResourceBarrier(1, &barrier);

	uint16_t textureSRVIndex = static_cast<uint16_t>(_textures.size());
	_textures.emplace_back(std::move(texture));
	_textureIndexMap.emplace(textureName, textureSRVIndex);
	return textureSRVIndex;
}

RenderItem::RenderItem(const GameObject* parentObject, 
						const Material* material, 
						D3D12_PRIMITIVE_TOPOLOGY primitive,
						uint8_t subMeshIndex,
						RenderLayer renderLayer) noexcept
	: _parentObject(parentObject)
	, _material(material)
	, _primitive(primitive)
	, _renderLayer(renderLayer)
	, _subMeshIndex(subMeshIndex)
{
}

const SubMeshGeometry& RenderItem::getSubMesh() const noexcept
{
	return _parentObject->getMeshGeometry()->_subMeshList[_subMeshIndex];
}

void RenderItem::changeMaterial(const Material* material) noexcept
{
	check(material != nullptr);
	_material = material;
}

ID2D1Bitmap* D3DApp::loadBitmapImage(const std::string& resourceName)
{
	auto findIt = _bitmapImages.find(resourceName);
	if (findIt != _bitmapImages.end())
	{
		return findIt->second.Get();
	}
	WComPtr<IWICBitmapDecoder> decoder = nullptr;
	WComPtr<IWICBitmapFrameDecode> source = nullptr;
	WComPtr<IWICFormatConverter> converter = nullptr;
	WComPtr<ID2D1Bitmap> outBitmap = nullptr;

	std::wstring fileName;
	fileName.assign(resourceName.begin(), resourceName.end());
	fileName = L"../Resources/XmlFiles/Asset/Texture/" + fileName + L".png";

	ThrowIfFailed(_wicImageFactory->CreateDecoderFromFilename(
		fileName.c_str(),
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		decoder.GetAddressOf()
	), resourceName);
	ThrowIfFailed(decoder->GetFrame(0, &source));
	ThrowIfFailed(_wicImageFactory->CreateFormatConverter(&converter));

	ThrowIfFailed(converter->Initialize(
		source.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.f,
		WICBitmapPaletteTypeMedianCut
	));

	_d2dContext->CreateBitmapFromWicBitmap(
		converter.Get(),
		nullptr,
		outBitmap.GetAddressOf()
	);

	auto it = _bitmapImages.emplace(resourceName, outBitmap);
	if (it.second == false)
	{
		ThrowErrCode(ErrCode::KeyDuplicated, resourceName);
	}
	return it.first->second.Get();
}