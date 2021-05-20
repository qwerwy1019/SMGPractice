#define NOMINMAX
#include "stdafx.h"

#include "D3DApp.h"
#include <array>
#include "GeometryGenerator.h"
#include <limits>
#include <Windows.h>
#include <windowsx.h>
#include "DirectX/DDSTextureLoader12.h"
#include "MathHelper.h"
#include <DirectXColors.h>

#include <msxml.h>
#include <atlconv.h>
#include "FileHelper.h"
#include "Material.h"
#include "PreDefines.h"
#include "D3DUtil.h"
#include "UiManager.h"
#include "TypeCommon.h"
#include "SMGFramework.h"

void D3DApp::loadInfoMap(void)
{
	if (SUCCEEDED(::CoInitialize(nullptr)))
	{
		{
			const string filePath = "../Resources/XmlFiles/Asset/Mesh/marioMesh.xml";
			XMLReader xmlMeshGeometry;

			xmlMeshGeometry.loadXMLFile(filePath);

			unique_ptr<MeshGeometry> mesh(new MeshGeometry);
			mesh->loadXml(xmlMeshGeometry.getRootNode(), _deviceD3d12.Get(), _commandList.Get());

			_geometries["marioMesh"] = move(mesh);
		}
		{
			const string filePath = "../Resources/XmlFiles/Asset/Material/marioMat.xml";
			XMLReader xmlMaterial;
			xmlMaterial.loadXMLFile(filePath);

			loadXmlMaterial(xmlMaterial.getRootNode());
		}
		{
			const string filePath = "../Resources/XmlFiles/Asset/Skeleton/marioSkeleton.xml";
			XMLReader xmlSkeleton;
			xmlSkeleton.loadXMLFile(filePath);

			unique_ptr<BoneInfo> boneInfo(new BoneInfo);
			boneInfo->loadXML(xmlSkeleton.getRootNode());
			_boneInfoMap["marioSkeleton"] = move(boneInfo);
		}
		{
			const string filePath = "../Resources/XmlFiles/Asset/Animation/marioAnim.xml";
			XMLReader xmlAnimation;
			xmlAnimation.loadXMLFile(filePath);

			unique_ptr<AnimationInfo> animationInfo(new AnimationInfo);
			animationInfo->loadXML(xmlAnimation.getRootNode());

			_animationNameListDev = animationInfo->getAnimationNameListDev();

			_animationInfoMap["marioAnim"] = move(animationInfo);
		}
		{
			const string filePath = "../Resources/XmlFiles/Object/marioObj.xml";
			XMLReader xmlGameObject;
			xmlGameObject.loadXMLFile(filePath);

			loadXmlGameObject(xmlGameObject.getRootNode());
		}
		{
			_cameraPosition = XMFLOAT3(200, 20, 0);
			_cameraUpVector = XMFLOAT4(0, 1, 0, 0);
			_cameraFocusPosition = XMFLOAT3(0, 0, 0);
		}
		::CoUninitialize();
	}
}

void D3DApp::buildShaderResourceViews()
{
	check(!_textures.empty(), "texture info가 먼저 로드되어야 합니다.");

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(_srvHeap->GetCPUDescriptorHandleForHeapStart());

	for (auto it = _textures.begin(); it != _textures.end(); ++it)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format = (*it)->_resource->GetDesc().Format;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = (*it)->_resource->GetDesc().MipLevels;
		desc.Texture2D.PlaneSlice = 0;
		desc.Texture2D.ResourceMinLODClamp = 0.f;
		_deviceD3d12->CreateShaderResourceView((*it)->_resource.Get(), &desc, handle);
		handle.Offset(1, _cbvSrcUavDescriptorSize);
	}
}

D3DApp::D3DApp()
	: _factory(nullptr)
	, _swapChain(nullptr)
	, _deviceD3d12(nullptr)
	, _fence(nullptr)
	, _currentFence(0)
	, _commandQueue(nullptr)
	, _commandAlloc(nullptr)
	, _commandList(nullptr)
	, _rtvDescriptorSize(0)
	, _dsvDescriptorSize(0)
	, _cbvSrcUavDescriptorSize(0)
	, _swapChainBuffer()
	, _depthStencilBuffer(nullptr)
	, _frameIndex(0)
	, _rtvHeap(nullptr)
	, _dsvHeap(nullptr)
	, _currentBackBuffer(0)
	, _rootSignature(nullptr)
	, _vertexShader(nullptr)
	, _pixelShader(nullptr)
	, _viewMatrix(MathHelper::Identity4x4)
	, _projectionMatrix(MathHelper::Identity4x4)
	, _psoType(PSOType::Normal)
	, _viewPort()
	, _scissorRect()
{
	Initialize();
}
D3DApp::~D3DApp()
{
	if (_deviceD3d12 != nullptr)
		flushCommandQueue();
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
	}
	mdxgiFactory->Release();
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
#if defined(DEBUG) || defined(_DEBUG)
	{
		WComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();

		logAdapters();
	}
#endif
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&_factory)));

	HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_deviceD3d12));
	if (FAILED(hardwareResult))
	{
		WComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_deviceD3d12)));
	}
	ThrowIfFailed(_deviceD3d12->CreateFence(_currentFence, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));

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
}

void D3DApp::initDirect2D(void)
{
	D2D1_FACTORY_OPTIONS options;
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, _d2dFactory.GetAddressOf());

	WComPtr<IDXGIDevice> deviceDxgi;
	WComPtr<ID3D11Device> deviceD3d11;
	WComPtr<ID3D11Device4> deviceD3d114;
	IUnknown* commandQueue = nullptr;

	ThrowIfFailed(_commandQueue->QueryInterface(&commandQueue));
	unsigned __int32 DeviceFlags = ::D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D3D11On12CreateDevice(_deviceD3d12.Get(),
		DeviceFlags, nullptr, 0, &commandQueue, 1, 0x00000001, &deviceD3d11, nullptr, nullptr);

	ThrowIfFailed(deviceD3d11->QueryInterface(deviceD3d114.GetAddressOf()));

	ThrowIfFailed(deviceD3d114->QueryInterface(_deviceD3d11On12.GetAddressOf()));
	ThrowIfFailed(_deviceD3d11On12->QueryInterface(deviceDxgi.GetAddressOf()));
	_d2dFactory->CreateDevice(deviceDxgi.Get(), _deviceD2d.GetAddressOf());

	ThrowIfFailed(_deviceD2d->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_d2dContext));

	deviceD3d114->GetImmediateContext3(_immediateContext.GetAddressOf());

	ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), (IUnknown**)&_writeFactory));

	{
		ThrowIfFailed(_writeFactory->CreateTextFormat(L"굴림체", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			13.f,
			L"ko-KR",
			&_textFormats[0]));
		ThrowIfFailed(_textFormats[0]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));
		ThrowIfFailed(_textFormats[0]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
	}
	static_assert(1 == static_cast<int>(TextFormatType::Count), "타입 추가시 작성해야함.");

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
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(_deviceD3d12->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
	srvHeapDesc.NumDescriptors = 20;
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
	_immediateContext->Flush();

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

	double aspectRatio = static_cast<double>(width) / height;
	XMMATRIX&& proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspectRatio, 1.f, 1000.f);
	XMStoreFloat4x4(&_projectionMatrix, proj);
}

void D3DApp::Update(void)
{
	updateCamera();

	_frameIndex = (_frameIndex + 1) % FRAME_RESOURCE_COUNT;
	const UINT64& currentFrameFence = _frameResources[_frameIndex]->getFence();

	if (currentFrameFence != 0 && _fence->GetCompletedValue() < currentFrameFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		check(eventHandle != nullptr, "createEvent Fail");
		ThrowIfFailed(_fence->SetEventOnCompletion(currentFrameFence, eventHandle), "fence set fail : " + std::to_string(currentFrameFence));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
	updateObjectConstantBuffer();
	updateSkinnedConstantBuffer();
	updatePassConstantBuffer();
	updateMaterialConstantBuffer();

}

void D3DApp::Draw(void)
{
	ID3D12CommandAllocator* cmdListAlloc = _frameResources[_frameIndex]->getCommandListAlloc();
	ThrowIfFailed(cmdListAlloc->Reset(), "reset in Draw Failed");

	ThrowIfFailed(_commandList->Reset(cmdListAlloc, _pipelineStateObjectMap[_psoType].Get()), "psoType:" + std::to_string((int)_psoType));

	_commandList->RSSetViewports(1, &_viewPort);
	_commandList->RSSetScissorRects(1, &_scissorRect);

	const CD3DX12_RESOURCE_BARRIER& transitionBarrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
			getCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
	_commandList->ResourceBarrier(1, &transitionBarrier1);
	
	_commandList->ClearRenderTargetView(getCurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	_commandList->ClearDepthStencilView(getDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	const D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = getCurrentBackBufferView();
	const D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = getDepthStencilView();
	_commandList->OMSetRenderTargets(1, &backBufferView, true, &depthStencilView);

	ID3D12DescriptorHeap* descriptorHeaps[] = { _srvHeap.Get() };
	_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	_commandList->SetGraphicsRootSignature(_rootSignature.Get());

	_commandList->SetGraphicsRootConstantBufferView(3, _frameResources[_frameIndex]->getPassCBVirtualAddress());

	for (auto e : RenderLayers)
	{
		drawRenderItems(e);
 	}

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
	UINT objectCount = getTotalRenderItemCount();
	UINT materialCount = _materials.size();
	UINT skinnedCount = _skinnedInstance.size();
	for (int i = 0; i < FRAME_RESOURCE_COUNT; ++i)
	{
		auto frameResource = std::make_unique<FrameResource>(_deviceD3d12.Get(), 1, objectCount, materialCount, skinnedCount);
		_frameResources.push_back(std::move(frameResource));
	}
}

void D3DApp::buildConstantGeometry(void)
{

}

void D3DApp::updateCamera(void)
{
	TickCount64 deltaTickCount = SMGFramework::Get().getTimer().getDeltaTickCount();
	XMVECTOR position = XMLoadFloat3(&_cameraPosition);
	XMVECTOR upVector = XMLoadFloat4(&_cameraUpVector);
	XMVECTOR focusPosition = XMLoadFloat3(&_cameraFocusPosition);
	if (0 < _cameraInputLeftTickCount)
	{
		XMVECTOR inputPosition = XMLoadFloat3(&_cameraInputPosition);
		XMVECTOR inputUpVector = XMLoadFloat4(&_cameraInputUpVector);
		XMVECTOR inputFocusPosition = XMLoadFloat3(&_cameraInputFocusPosition);
		if (_cameraInputLeftTickCount <= deltaTickCount)
		{
			position = inputPosition;
			upVector = inputUpVector;
			if (_hasCameraFocusInput)
			{
				focusPosition = inputFocusPosition;
			}
			_cameraInputLeftTickCount = 0;
		}
		else
		{
			float t = (deltaTickCount - _cameraInputLeftTickCount) / static_cast<float>(_cameraInputLeftTickCount);
			position = XMVectorLerp(position, inputPosition, t);
			upVector = XMVectorLerp(upVector, inputUpVector, t);
			if (_hasCameraFocusInput)
			{
				focusPosition = XMVectorLerp(focusPosition, inputFocusPosition, t);
			}
			_cameraInputLeftTickCount -= deltaTickCount;
		}
	}
	XMStoreFloat3(&_cameraPosition, position);
	XMStoreFloat4(&_cameraUpVector, upVector);
	XMStoreFloat3(&_cameraFocusPosition, focusPosition);

	XMMATRIX view = XMMatrixLookAtLH(position, focusPosition, upVector);
	XMStoreFloat4x4(&_viewMatrix, view);
}

void D3DApp::updateObjectConstantBuffer()
{
	auto& currentFrameResource = _frameResources[_frameIndex];
	for (const auto& e : _renderItemsUniquePtrXXX)
	{
		if (e->_dirtyFrames > 0)
		{
			ObjectConstants objectConstants;

			XMMATRIX world = XMLoadFloat4x4(&e->_worldMatrix);
			XMStoreFloat4x4(&objectConstants._world, XMMatrixTranspose(world));

			XMMATRIX textureTransform = XMLoadFloat4x4(&e->_textureTransform);
			XMStoreFloat4x4(&objectConstants._textureTransform, XMMatrixTranspose(textureTransform));
				
			currentFrameResource->setObjectCB(e->_objConstantBufferIndex, objectConstants);
			--e->_dirtyFrames;
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
	XMMATRIX view = XMLoadFloat4x4(&_viewMatrix);
	XMMATRIX proj = XMLoadFloat4x4(&_projectionMatrix);
	XMMATRIX viewProj = view * proj;

	XMVECTOR viewDet = XMMatrixDeterminant(view);
	XMVECTOR projDet = XMMatrixDeterminant(proj);
	XMVECTOR viewProjDet = XMMatrixDeterminant(viewProj);

	XMMATRIX invView = XMMatrixInverse(&viewDet, view);
	XMMATRIX invProj = XMMatrixInverse(&projDet, proj);
	XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);
	
	XMStoreFloat4x4(&_passConstants._view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_passConstants._invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&_passConstants._proj , XMMatrixTranspose(proj));
	XMStoreFloat4x4(&_passConstants._invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&_passConstants._viewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&_passConstants._invViewProj, XMMatrixTranspose(invViewProj));
	_passConstants._cameraPos = _cameraPosition;

	const float width = SMGFramework::Get().getClientWidth();
	const float height = SMGFramework::Get().getClientHeight();
	_passConstants._renderTargetSize = XMFLOAT2(static_cast<float>(width), static_cast<float>(height));
	_passConstants._invRenderTargetSize = XMFLOAT2(1.0f / width, 1.0f / height);
	_passConstants._nearZ = 1.0f;
	_passConstants._farZ = 200.0f;
	_passConstants._totalTime = SMGFramework::Get().getTimer().getTotalTime();
	_passConstants._deltaTime = SMGFramework::Get().getTimer().getDeltaTime();

	_passConstants._fogColor = DirectX::XMFLOAT4(DirectX::Colors::LightSteelBlue);
	_passConstants._fogStart = 20.f;
	_passConstants._fogEnd = 100.f;
	_passConstants._ambientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	
	// 주광
	XMVECTOR lightDir = XMVectorSet(0.f, -1.f, 0.f, 0.f);
	XMStoreFloat3(&_passConstants._lights[0]._direction, lightDir);
	XMStoreFloat3(&_passConstants._lights[0]._position, -lightDir * 30.0f);
	_passConstants._lights[0]._strength = { 0.8f, 0.8f, 0.6f };
	_passConstants._lights[0]._falloffEnd = 200.f;
	//_passConstants._lights[0]._strength = { 0.7f * abs(sinf(3.f * _timer.GetTotalTime())) + 0.3f, 0.f, 0.f };
	// 보조광
	XMVECTOR subLightDir = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	XMStoreFloat3(&_passConstants._lights[1]._direction, subLightDir);
	XMStoreFloat3(&_passConstants._lights[1]._position, -subLightDir * 50.f);
	_passConstants._lights[1]._strength = { 1.f, 1.f, 0.9f };
	_passConstants._lights[1]._falloffEnd = 200.f;
	// 역광
	XMVECTOR backLightDir = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMStoreFloat3(&_passConstants._lights[2]._direction, backLightDir);
	XMStoreFloat3(&_passConstants._lights[2]._position, -backLightDir * 50.f);
	_passConstants._lights[2]._strength = { 1.f, 1.f, 0.9f };
	_passConstants._lights[2]._falloffEnd = 200.f;
	
	_frameResources[_frameIndex]->setPassCB(0, _passConstants);


// 	XMVECTOR shadowPlane = XMVectorSet(0.f, 1.f, 0.f, 0.f);
// 	XMVECTOR shadowDir = -XMLoadFloat3(&_passConstants._lights[0]._direction);
// 	XMMATRIX shadowMatrix = XMMatrixShadow(shadowPlane, shadowDir);
// 	XMMATRIX shadowWorld = XMMatrixTranslation(0.f, 0.5f, 0.f);
// 	XMMATRIX shadowOffsetY = XMMatrixTranslation(0.f, 0.001f, 0.f);
// 	XMStoreFloat4x4(&_shadowItem->_worldMatrix, shadowWorld * shadowMatrix * shadowOffsetY);
// 
// 	_shadowItem->_dirtyFrames = FRAME_RESOURCE_COUNT;
}

void D3DApp::loadXmlMaterial(const XMLReaderNode& rootNode)
{
	const auto& nodes = rootNode.getChildNodes();
	for (int i = 0; i < nodes.size(); ++i)
	{
		string materialName;
		nodes[i].loadAttribute("Name", materialName);
		materialName = "marioMat/" + materialName;
		if (_materials.find(materialName) != _materials.end())
		{
			// 어떻게 처리할까? [1/27/2021 qwerw]
			continue;
		}
		DirectX::XMFLOAT4 diffuseAlbedo;
		DirectX::XMFLOAT3 fresnelR0;
		float roughness;
		nodes[i].loadAttribute("DiffuseAlbedo", diffuseAlbedo);
		nodes[i].loadAttribute("FresnelR0", fresnelR0);
		nodes[i].loadAttribute("Roughness", roughness);

		string textureName;
		nodes[i].loadAttribute("DiffuseTexture", textureName);
		wstring textureNameWstring;
		textureNameWstring.assign(textureName.begin(), textureName.end());
		textureNameWstring = L"../Resources/XmlFiles/Asset/Texture/" + textureNameWstring + L".dds";

		uint16_t diffuseSRVIndex = loadTexture(textureName, textureNameWstring);
		unique_ptr<Material> material(new Material(_materials.size(), diffuseSRVIndex, 0, diffuseAlbedo, fresnelR0, roughness));
		
		_materials.emplace(materialName, move(material));
	}
}

void D3DApp::loadXmlGameObject(const XMLReaderNode& rootNode)
{
	const auto& childNodes = rootNode.getChildNodesWithName();
	bool isSkinned;
	rootNode.loadAttribute("IsSkinned", isSkinned);
	auto childIter = childNodes.end();
	uint16_t skinnedConstantBufferIndex = std::numeric_limits<uint16_t>::max();
	SkinnedModelInstance* skinnedInstance = nullptr;

	if (isSkinned)
	{
		BoneInfo* boneInfo = nullptr;
		AnimationInfo* animationInfo = nullptr;

		childIter = childNodes.find("Skeleton");
		if (childIter == childNodes.end())
		{
			ThrowErrCode(ErrCode::NodeNotFound, "Skeleton 노드가 없습니다.");
		}

		string skeletonName;
		childIter->second.loadAttribute("FileName", skeletonName);
		auto boneIt = _boneInfoMap.find(skeletonName);
		if (boneIt == _boneInfoMap.end())
		{
			ThrowErrCode(ErrCode::FileNotFound, "skeleton file : " + skeletonName + "이 _boneInfoMap에 없습니다.");
		}
		boneInfo = boneIt->second.get();
		
		childIter = childNodes.find("Animation");
		if (childIter == childNodes.end())
		{
			ThrowErrCode(ErrCode::NodeNotFound, "Animation 노드가 없습니다.");
		}
		string animationInfoName;
		childIter->second.loadAttribute("FileName", animationInfoName);
		auto animFileIt = _animationInfoMap.find(animationInfoName);
		if (animFileIt == _animationInfoMap.end())
		{
			ThrowErrCode(ErrCode::FileNotFound, "Animation File : " + animationInfoName + "이 없습니다.");
		}
		animationInfo = animFileIt->second.get();

		skinnedConstantBufferIndex = _skinnedInstance.size();
		unique_ptr<SkinnedModelInstance> newSkinnedInstance(new SkinnedModelInstance(skinnedConstantBufferIndex, boneInfo, animationInfo));
		skinnedInstance = newSkinnedInstance.get();
		_skinnedInstance.emplace_back(move(newSkinnedInstance));
	}

	childIter = childNodes.find("Mesh");
	if (childIter == childNodes.end())
	{
		ThrowErrCode(ErrCode::NodeNotFound, "Mesh 노드가 없습니다.");
	}
	string meshFileName;
	childIter->second.loadAttribute("FileName", meshFileName);
	auto meshIt = _geometries.find(meshFileName);
	if (meshIt == _geometries.end())
	{
		ThrowErrCode(ErrCode::FileNotFound, "Mesh File : " + meshFileName + "이 없습니다.");
	}
	const auto& subMeshMap = meshIt->second->_subMeshMap;
	const auto& subMeshNodes = childIter->second.getChildNodes();
	for (int j = 0; j < subMeshNodes.size(); ++j)
	{
		string subMeshName;
		subMeshNodes[j].loadAttribute("Name", subMeshName);
		const auto& subMeshIt = subMeshMap.find(subMeshName);
		if (subMeshIt == subMeshMap.end())
		{
			ThrowErrCode(ErrCode::SubMeshNotFound, "SubMeshName : " + subMeshName + "이 " + meshFileName + "에 없습니다.");
		}
		string materialFile, materialName;
		subMeshNodes[j].loadAttribute("MaterialFile", materialFile);
		subMeshNodes[j].loadAttribute("MaterialName", materialName);
		auto materialIt = _materials.find(materialFile + '/' + materialName);
		if (materialIt == _materials.end())
		{
			ThrowErrCode(ErrCode::MaterialNotFound, "Material : " + materialFile + '/' + materialName + "이 없습니다.");
		}
		Material* material = materialIt->second.get();
		// material에서 RenderLayer 정할수 있도록 추가해야함 [1/28/2021 qwerw]

		auto renderItem = make_unique<RenderItem>();
		// spawnInfo 작업이 완료되면 이 데이터도 채워져야함 [1/28/2021 qwerw]
// 			XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
// 			XMMATRIX R = XMMatrixRotationY(rotation.y);
// 			XMMATRIX T = XMMatrixTranslation(0.f, 0.f, 0.f);
		renderItem->_worldMatrix = MathHelper::Identity4x4;
		renderItem->_objConstantBufferIndex = _renderItemsUniquePtrXXX.size();
		renderItem->_geometry = meshIt->second.get();
		renderItem->_primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		renderItem->_indexCount = subMeshIt->second._indexCount;
		renderItem->_baseVertexLocation = subMeshIt->second._baseVertexLoaction;
		renderItem->_startIndexLocation = subMeshIt->second._baseIndexLoacation;
		renderItem->_material = material;
		renderItem->_skinnedConstantBufferIndex = skinnedConstantBufferIndex;
		renderItem->_skinnedModelInstance = skinnedInstance;
		_renderItems[static_cast<int>(RenderLayer::Opaque)].push_back(renderItem.get());
		_renderItemsUniquePtrXXX.push_back(move(renderItem));
	}

	childIter = childNodes.find("ActionStates");
	if (childIter == childNodes.end())
	{
		ThrowErrCode(ErrCode::NodeNotFound, "ActionStates 노드가 없습니다.");
	}
}

void D3DApp::drawUI(void)
{
	_deviceD3d11On12->AcquireWrappedResources(_backBufferWrapped[_currentBackBuffer].GetAddressOf(), 1);

	_d2dContext->SetTarget(_backBufferBitmap[_currentBackBuffer].Get());
	_d2dContext->BeginDraw();
	D2D1_POINT_2F position = { 0.f, 0.f };

	SMGFramework::Get().getUIManager()->drawUI();

	_d2dContext->EndDraw();
	_d2dContext->SetTarget(nullptr);

	_deviceD3d11On12->ReleaseWrappedResources(_backBufferWrapped[_currentBackBuffer].GetAddressOf(), 1);

	_immediateContext->Flush();
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

			XMMATRIX matTransform = XMLoadFloat4x4(&mat->getMaterialTransform());
			XMStoreFloat4x4(&matConstants._materialTransform, XMMatrixTranspose(matTransform));

			currentFrameResource->setMaterialCB(mat->getMaterialCBIndex(), matConstants);

			--mat->_dirtyFrames;
		}
	}
}

void D3DApp::drawRenderItems(const RenderLayer renderLayer)
{
	switch (renderLayer)
	{
		case RenderLayer::Opaque:
		{
			__noop;
		}
		break;
		case RenderLayer::AlphaTested:
		{
			_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::BackSideNotCulling].Get());
		}
		break;
		case RenderLayer::Transparent:
		{
			_commandList->SetGraphicsRootConstantBufferView(2,
				_frameResources[_frameIndex]->getPassCBVirtualAddress());
			_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::Transparent].Get());
		}
		break;
		case RenderLayer::Shadow:
		{
			_commandList->SetPipelineState(_pipelineStateObjectMap[PSOType::Shadow].Get());
		}
		break;
		case RenderLayer::Count:
		default:
		{
			static_assert(static_cast<int>(RenderLayer::Count) == 4, "RenderLayer가 어떤 PSO를 사용할지 정해야합니다.");
			static_assert(static_cast<int>(PSOType::Count) == 5, "PSO 타입이 추가되었다면 확인해주세요");
			ThrowErrCode(ErrCode::UndefinedType, "비정상입니다");
		}
	}
	int renderLayerIdx = static_cast<int>(renderLayer);
	UINT objectCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT skinnedCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(SkinnedConstants));
	UINT materialCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	D3D12_GPU_VIRTUAL_ADDRESS objectCBBaseAddress = _frameResources[_frameIndex]->getObjectCBVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS skinnedCBBaseAddress = _frameResources[_frameIndex]->getSkinnedCBVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS materialCBBaseAddress = _frameResources[_frameIndex]->getMaterialCBVirtualAddress();
	
	for (int rIdx = 0; rIdx < _renderItems[renderLayerIdx].size(); ++rIdx)
	{
		auto renderItem = _renderItems[renderLayerIdx][rIdx];
		check(renderItem != nullptr, "비정상입니다.");

		D3D12_VERTEX_BUFFER_VIEW vbv = renderItem->_geometry->getVertexBufferView();
		_commandList->IASetVertexBuffers(0, 1, &vbv);
		D3D12_INDEX_BUFFER_VIEW ibv = renderItem->_geometry->getIndexBufferView();
		_commandList->IASetIndexBuffer(&ibv);
		_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle(_srvHeap->GetGPUDescriptorHandleForHeapStart());
		textureHandle.Offset(renderItem->_material->getDiffuseSRVHeapIndex(), _cbvSrcUavDescriptorSize);
		_commandList->SetGraphicsRootDescriptorTable(0, textureHandle);

		D3D12_GPU_VIRTUAL_ADDRESS objectCBaddress = objectCBBaseAddress + renderItem->_objConstantBufferIndex * objectCBByteSize;
		_commandList->SetGraphicsRootConstantBufferView(1, objectCBaddress);

		D3D12_GPU_VIRTUAL_ADDRESS skinnedCBAdress = skinnedCBBaseAddress + renderItem->_skinnedConstantBufferIndex * skinnedCBByteSize;
		_commandList->SetGraphicsRootConstantBufferView(2, skinnedCBAdress);

		D3D12_GPU_VIRTUAL_ADDRESS materialCBAddress = materialCBBaseAddress + renderItem->_material->getMaterialCBIndex() * materialCBByteSize;
		_commandList->SetGraphicsRootConstantBufferView(4, materialCBAddress);

		_commandList->DrawIndexedInstanced(
			renderItem->_indexCount,
			1,
			renderItem->_startIndexLocation,
			renderItem->_baseVertexLocation,
			0);
	}
}

void D3DApp::buildConstantBufferViews()
{
}

UINT D3DApp::getTotalRenderItemCount(void) const noexcept
{
	return _renderItemsUniquePtrXXX.size();
}

void D3DApp::buildGameObject(const std::string& meshName, 
	const DirectX::XMFLOAT3& scaling,
	const DirectX::XMFLOAT3& rotation,
	const DirectX::XMFLOAT3& translation)
{
	auto geometry = _geometries.find(meshName);
	if (geometry == _geometries.end())
	{
		ThrowErrCode(ErrCode::MeshNotFound, "mesh " + meshName +"을 찾을 수 없습니다.");\
	}
	const auto& subMeshMap = geometry->second->_subMeshMap;
	for (auto it = subMeshMap.begin(); it != subMeshMap.end(); ++it)
	{
		auto renderItem = make_unique<RenderItem>();
		XMMATRIX S = XMMatrixScaling(scaling.x, scaling.y, scaling.z);
		XMMATRIX R = XMMatrixRotationY(rotation.y);
		XMMATRIX T = XMMatrixTranslation(0.f, 0.f, 0.f);
		XMStoreFloat4x4(&renderItem->_worldMatrix, S * R * T);
		renderItem->_objConstantBufferIndex = _renderItemsUniquePtrXXX.size();
		renderItem->_geometry = geometry->second.get();
		renderItem->_primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		renderItem->_indexCount = it->second._indexCount;
		renderItem->_baseVertexLocation = it->second._baseVertexLoaction;
		renderItem->_startIndexLocation = it->second._baseIndexLoacation;
		renderItem->_material = _materials[meshName + '/' +it->first].get();
		_renderItems[static_cast<int>(RenderLayer::Opaque)].emplace_back(renderItem.get());
		_renderItemsUniquePtrXXX.emplace_back(move(renderItem));
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> D3DApp::getStaticSampler(void) const
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
	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp };
}
void D3DApp::buildRootSignature(void)
{
	CD3DX12_DESCRIPTOR_RANGE textureTable;
	textureTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	slotRootParameter[0].InitAsDescriptorTable(1, &textureTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);
	slotRootParameter[4].InitAsConstantBufferView(3);

	auto staticSampler = getStaticSampler();
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, staticSampler.size(), staticSampler.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
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
	_vertexShader = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", definesForShader, "DefaultVertexShader", "vs_5_0");
	_pixelShader = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", definesForShader, "DefaultPixelShader", "ps_5_0");
}

void D3DApp::buildPipelineStateObject(void)
{
	check(_pipelineStateObjectMap.size() == 0, "pso가 이미 생성되어있습니다");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescNormal;
	ZeroMemory(&psoDescNormal, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
	D3D12_DEPTH_STENCIL_DESC transparentStencilDesc;
	D3D12_DEPTH_STENCIL_DESC shadowDesc;
	D3D12_DEPTH_STENCIL_DESC uiDepthStencilesc;

	// psoDescNormal
	{
		psoDescNormal.pRootSignature = _rootSignature.Get();
		psoDescNormal.VS = { reinterpret_cast<BYTE*>(_vertexShader->GetBufferPointer()), _vertexShader->GetBufferSize() };
		psoDescNormal.PS = { reinterpret_cast<BYTE*>(_pixelShader->GetBufferPointer()), _pixelShader->GetBufferSize() };
		psoDescNormal.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDescNormal.SampleMask = UINT_MAX;
		psoDescNormal.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDescNormal.InputLayout = { SKINNED_VERTEX_INPUT_LAYOUT, SKINNED_VERTEX_INPUT_DESC_SIZE };
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
	//shadowDesc
	{
		uiDepthStencilesc.DepthEnable = false;
	}
	//shadowDesc
	{
		shadowDesc.DepthEnable = true;
		shadowDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		shadowDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		shadowDesc.StencilEnable = true;
		shadowDesc.StencilReadMask = 0xff;
		shadowDesc.StencilWriteMask = 0xff;

		shadowDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		shadowDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		shadowDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
		shadowDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

		shadowDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		shadowDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		shadowDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
		shadowDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	}
	{

		auto normalPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::Normal, nullptr));
		check(normalPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Normal");
		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescNormal, IID_PPV_ARGS(&normalPSO.first->second)),
						"PSO 생성 실패! PSOType::Normal");
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescBackSideNotCulling = psoDescNormal;
		psoDescBackSideNotCulling.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		auto backSideNotCullingPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::BackSideNotCulling, nullptr));
		check(backSideNotCullingPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::BackSideNotCulling");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescBackSideNotCulling, IID_PPV_ARGS(&backSideNotCullingPSO.first->second)),
						"PSO 생성 실패! PSOType::BackSideNotCulling");
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescTransparent = psoDescNormal;


		psoDescTransparent.BlendState.RenderTarget[0] = blendDesc;
		psoDescTransparent.DepthStencilState = transparentStencilDesc;
		auto transparentPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::Transparent, nullptr));
		check(transparentPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Transparent");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescTransparent, IID_PPV_ARGS(&transparentPSO.first->second)),
					"PSO 생성 실패! PSOType::Transparent");
	}

	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescShadow = psoDescNormal;


		psoDescShadow.DepthStencilState.DepthEnable = false;
		auto shadowPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::Shadow, nullptr));
		check(shadowPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Shadow");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescShadow, IID_PPV_ARGS(&shadowPSO.first->second)),
					"PSO 생성 실패! PSOType::Shadow");
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescUI = psoDescNormal;


		psoDescUI.BlendState.RenderTarget[0] = blendDesc;
		psoDescUI.DepthStencilState = shadowDesc;
		auto UIPSO = _pipelineStateObjectMap.insert(make_pair(PSOType::UI, nullptr));
		check(UIPSO.second == true, "PSO가 중복 생성되었습니다 PSOType::Shadow");

		ThrowIfFailed(_deviceD3d12->CreateGraphicsPipelineState(&psoDescUI, IID_PPV_ARGS(&UIPSO.first->second)),
			"PSO 생성 실패! PSOType::UIPSO");
	}
	static_assert(static_cast<int>(PSOType::Count) == 5, "PSO 타입이 추가되었다면 확인해주세요.");
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

	loadInfoMap();
	buildShaderResourceViews();

	//buildConstantGeometry();
	//BuildMaterials();
	//buildGameObjects();
	buildFrameResources();
	buildConstantBufferViews();

	ThrowIfFailed(_commandList->Close());
	ID3D12CommandList* cmdLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	flushCommandQueue();

	return true;
}

uint16_t D3DApp::loadTexture(const string& textureName, const wstring& fileName)
{
	if (std::numeric_limits<uint16_t>::max() <= _textures.size())
	{
		ThrowErrCode(ErrCode::Overflow, "Texture index가 범위를 넘어갑니다.");
	}

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
	texture->_index = textureSRVIndex;
	_textures.emplace_back(std::move(texture));
	
	return textureSRVIndex;
}

RenderItem::RenderItem() noexcept
	: _worldMatrix(MathHelper::Identity4x4)
	, _textureTransform(MathHelper::Identity4x4)
	, _dirtyFrames(FRAME_RESOURCE_COUNT)
	, _objConstantBufferIndex(-1)
	, _geometry(nullptr)
	, _primitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	, _indexCount(0)
	, _startIndexLocation(0)
	, _baseVertexLocation(0)
	, _material(nullptr)
	, _skinnedConstantBufferIndex(std::numeric_limits<uint16_t>::max())
	, _skinnedModelInstance(nullptr)
{

}