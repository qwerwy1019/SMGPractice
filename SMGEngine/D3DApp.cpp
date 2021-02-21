#define NOMINMAX
#include "stdafx.h"

#include "D3DApp.h"
#include <array>
#include "GeometryGenerator.h"
#include <limits>
#include <stdint.h>
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

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return D3DApp::getApp()->MsgProc(hwnd, msg, wParam, lParam);
}


D3DApp* D3DApp::_app = nullptr;

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

D3DApp::D3DApp(HINSTANCE hInstance)
	: _hInstance(hInstance)
	, _hMainWnd(nullptr)
	, _clientWidth(800)
	, _clientHeight(600)
	, _minimized(false)
	, _maximized(false)
	, _resizing(false)
	, _factory(nullptr)
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
	, _cameraTheta(5.f)
	, _cameraPhi(XM_PIDIV4)
	, _cameraRadius(500.f)
	, _cameraCenterPos(0.f, 0.f, 0.f)
	, _cameraPos(0.f, 0.f, 0.f)
	, _sunTheta(0)
	, _sunPhi(XM_PIDIV2)
	, _viewMatrix(MathHelper::Identity4x4)
	, _projectionMatrix(MathHelper::Identity4x4)
	, _psoType(PSOType::Normal)
	, _viewPort()
	, _scissorRect()
	, _timer()
{
	check(_app == nullptr, "D3DApp class는 한번만 생성되어야 합니다.");
	_app = this;

	_mousePos.x = 0;
	_mousePos.y = 0;
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

D3DApp* D3DApp::getApp(void) noexcept
{
	check(_app != nullptr, "d3d app이 초기화되지 않았습니다.");
	return _app;
}

UIManager* D3DApp::getUIManager(void) noexcept
{
	check(_app->_uiManager != nullptr, "ui manager가 초기화되지 않았습니다.");
	return _app->_uiManager.get();
}

void D3DApp::initMainWindow()
{
	WNDCLASS wc;
	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= MainWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= _hInstance;
	wc.hIcon			= LoadIcon(0, IDI_APPLICATION);
	wc.hCursor			= LoadCursor(0, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName		= 0;
	wc.lpszClassName	= L"MainWnd";

	if (!RegisterClass(&wc))
	{
		ThrowErrCode(ErrCode::InitFail, "RegisterClass Failed.");
	}

	RECT R = { 0, 0, _clientWidth, _clientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	_hMainWnd = CreateWindow(L"MainWnd", WINDOW_CAPTION.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, _hInstance, nullptr);
	if (nullptr == _hMainWnd)
	{
		ThrowErrCode(ErrCode::InitFail, "CreateWindow Failed.");
	}

	ShowWindow(_hMainWnd, SW_SHOW);
	UpdateWindow(_hMainWnd);
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

void D3DApp::createSwapChain(void)
{
	_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.Width = _clientWidth;
	sd.BufferDesc.Height = _clientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = BACK_BUFFER_FORMAT;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	sd.SampleDesc.Count = _4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
	
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	sd.OutputWindow = _hMainWnd;
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
	_uiManager->beforeResize();
	ThrowIfFailed(_commandList->Reset(_commandAlloc.Get(), nullptr), "error!");
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		_swapChainBuffer[i].Reset();
	}
	_depthStencilBuffer.Reset();

	ThrowIfFailed(_swapChain->ResizeBuffers(
		SWAP_CHAIN_BUFFER_COUNT,
		_clientWidth, _clientHeight,
		BACK_BUFFER_FORMAT,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	
	_currentBackBuffer = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		ThrowIfFailed(_swapChain->GetBuffer(i, IID_PPV_ARGS(&_swapChainBuffer[i])));
		_deviceD3d12->CreateRenderTargetView(_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, _rtvDescriptorSize);

		_uiManager->onResize(_swapChainBuffer[i].Get(), i);
		
	}

	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = _clientWidth;
	depthStencilDesc.Height = _clientHeight;
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
	_viewPort.Width = static_cast<float>(_clientWidth);
	_viewPort.Height = static_cast<float>(_clientHeight);
	_viewPort.MinDepth = 0.f;
	_viewPort.MaxDepth = 1.f;

	_scissorRect = { 0, 0, _clientWidth, _clientHeight };

	ThrowIfFailed(_commandList->Close());
	ID3D12CommandList* cmdLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	flushCommandQueue();

	XMMATRIX&& proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&_projectionMatrix, proj);
}

void D3DApp::Update(void)
{
	onKeyboardInput();
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
	_uiManager->drawUI(_currentBackBuffer);
	
	ThrowIfFailed(_swapChain->Present(0, 0));
	_currentBackBuffer = (_currentBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

	_frameResources[_frameIndex]->setFence(++_currentFence);
	_commandQueue->Signal(_fence.Get(), _currentFence);
}

bool D3DApp::isAppPaused(void) const noexcept
{
	return _timer.isTimerStopped();
}

void D3DApp::calculateFrameStats(void) noexcept
{
	check(_hMainWnd != nullptr, "_hMainWnd가 초기화되지 않음");
	static int frameCnt = 0;
	static double timeElapsed = _timer.getTotalTime();

	++frameCnt;
	if (_timer.getTotalTime() - timeElapsed > 1.f)
	{
		std::wstring fpsStr = std::to_wstring(frameCnt);
		std::wstring mspfStr = std::to_wstring(1000.f / frameCnt);

		std::wstring timePosStr = L"";
		std::wstring animString = L"";
		if (!_skinnedInstance.empty())
		{
			timePosStr = std::to_wstring(_skinnedInstance[0]->getTimePosDev());
			USES_CONVERSION;
			animString = A2W(_animationNameListDev[_animationNameIndexDev].c_str());
		}

		std::wstring windowText = WINDOW_CAPTION + L"animTime: " + timePosStr + L" name: " + animString + L" fps: " + fpsStr + L" mspf: " + mspfStr;
		SetWindowText(_hMainWnd, windowText.c_str());

		frameCnt = 0;
		timeElapsed += 1.f;
	}
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

void D3DApp::onMouseDown(WPARAM buttonState, int x, int y) noexcept
{
}

void D3DApp::onMouseUp(WPARAM buttonState, int x, int y) noexcept
{
}

void D3DApp::onMouseMove(WPARAM buttonState, int x, int y) noexcept
{
	if ((buttonState & MK_LBUTTON) != 0)
	{
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - _mousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - _mousePos.y));

		_cameraTheta = MathHelper::AddRadian(_cameraTheta, dx);
		_cameraPhi = MathHelper::AddRadian(_cameraPhi, dy);

		_cameraPhi = min(max(_cameraPhi, 0.1f), MathHelper::Pi - 0.1f);
	}
	else if ((buttonState & MK_RBUTTON) != 0)
	{
		float dx = 0.2f * static_cast<float>(x - _mousePos.x);
		float dy = 0.2f * static_cast<float>(y - _mousePos.y);

		_cameraRadius += dx - dy;
		_cameraRadius = min(max(_cameraRadius, 3.f), 2000.f);
	}

	_mousePos.x = x;
	_mousePos.y = y;
}

void D3DApp::onKeyboardInput(void) noexcept
{
	const float dt = _timer.getDeltaTime();

	{
		if (GetAsyncKeyState(VK_LEFT) && 0x8000)
		{
			_sunTheta -= 1.f * dt;
			_animationNameIndexDev = _animationNameIndexDev - 1;
			if (_animationNameIndexDev < 0)
			{
				_animationNameIndexDev = 0;
			}
			_skinnedInstance[0]->setAnimation(_animationNameListDev[_animationNameIndexDev], 100);
		}

		if (GetAsyncKeyState(VK_RIGHT) && 0x8000)
		{
			_sunTheta += 1.f * dt;
			_animationNameIndexDev = _animationNameIndexDev + 1;
			if (_animationNameIndexDev >= _animationNameListDev.size())
			{
				_animationNameIndexDev = _animationNameListDev.size() - 1;
			}
			_skinnedInstance[0]->setAnimation(_animationNameListDev[_animationNameIndexDev], 100);
		}

		if (GetAsyncKeyState(VK_UP) && 0x8000)
		{
			_sunPhi += 1.f * dt;
		}

		if (GetAsyncKeyState(VK_DOWN) && 0x8000)
		{
			_sunPhi -= 1.f * dt;
		}
	}

	{
		if (GetAsyncKeyState(0x41/*a*/) && 0x8000)
		{
			_cameraCenterPos.x -= 30.f * dt;
		}

		if (GetAsyncKeyState(0x44/*d*/) && 0x8000)
		{
			_cameraCenterPos.x += 30.f * dt;
		}

		if (GetAsyncKeyState(0x57/*w*/) && 0x8000)
		{
			_cameraCenterPos.z += 30.f * dt;
		}

		if (GetAsyncKeyState(0x53/*s*/) && 0x8000)
		{
			_cameraCenterPos.z -= 30.f * dt;
		}

		if (GetAsyncKeyState(0x5A/*z*/) && 0x8000)
		{
			_cameraCenterPos.y += 30.f * dt;
		}

		if (GetAsyncKeyState(0x58/*x*/) && 0x8000)
		{
			_cameraCenterPos.y -= 30.f * dt;
		}
	}
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
	XMVECTOR pos = MathHelper::SphericalToCartesian(_cameraRadius, _cameraPhi, _cameraTheta);
	pos += XMLoadFloat3(&_cameraCenterPos);
	XMStoreFloat3(&_cameraPos, pos);

	XMVECTOR target = XMLoadFloat3(&_cameraCenterPos);
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
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

	__int64 deltaTick = _timer.getDeltaTickCount();
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
	_passConstants._cameraPos = _cameraPos;
	_passConstants._renderTargetSize = XMFLOAT2(static_cast<float>(_clientWidth), static_cast<float>(_clientHeight));
	_passConstants._invRenderTargetSize = XMFLOAT2(1.0f / _clientWidth, 1.0f / _clientHeight);
	_passConstants._nearZ = 1.0f;
	_passConstants._farZ = 200.0f;
	_passConstants._totalTime = _timer.getTotalTime();
	_passConstants._deltaTime = _timer.getDeltaTime();

	_passConstants._fogColor = DirectX::XMFLOAT4(DirectX::Colors::LightSteelBlue);
	_passConstants._fogStart = 20.f;
	_passConstants._fogEnd = 100.f;
	_passConstants._ambientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	
	// 주광
	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.f, _sunPhi, _sunTheta);
	XMStoreFloat3(&_passConstants._lights[0]._direction, lightDir);
	XMStoreFloat3(&_passConstants._lights[0]._position, -lightDir * 30.0f);
	_passConstants._lights[0]._strength = { 0.8f, 0.8f, 0.6f };
	_passConstants._lights[0]._falloffEnd = 200.f;
	//_passConstants._lights[0]._strength = { 0.7f * abs(sinf(3.f * _timer.GetTotalTime())) + 0.3f, 0.f, 0.f };
	// 보조광
	lightDir = -MathHelper::SphericalToCartesian(1.f, _sunPhi, _sunTheta + XM_PIDIV2);
	XMStoreFloat3(&_passConstants._lights[1]._direction, lightDir);
	XMStoreFloat3(&_passConstants._lights[1]._position, -lightDir * 50.f);
	_passConstants._lights[1]._strength = { 1.f, 1.f, 0.9f };
	_passConstants._lights[1]._falloffEnd = 200.f;
	// 역광
	lightDir = -MathHelper::SphericalToCartesian(1.f, _sunPhi, XM_PI + _sunTheta);
	XMStoreFloat3(&_passConstants._lights[2]._direction, lightDir);
	XMStoreFloat3(&_passConstants._lights[2]._position, -lightDir * 50.f);
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

void D3DApp::initializeManagers()
{
	_uiManager = make_unique<UIManager>(_commandQueue.Get(), _deviceD3d12.Get());
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
	// getChildNode(nodeName)을 구현하면 수정할것 [1/28/2021 qwerw]
	const auto& childNodes = rootNode.getChildNodes();
	bool isSkinned = (childNodes.size() == 4);
	int nodeIndex = 0;
	uint16_t skinnedConstantBufferIndex = std::numeric_limits<uint16_t>::max();
	SkinnedModelInstance* skinnedInstance = nullptr;

	if (isSkinned)
	{
		BoneInfo* boneInfo = nullptr;
		AnimationInfo* animationInfo = nullptr;

		if (childNodes[nodeIndex].getNodeName() == "Skeleton")
		{
			string skeletonName;
			childNodes[nodeIndex].loadAttribute("FileName", skeletonName);
			auto boneIt = _boneInfoMap.find(skeletonName);
			if (boneIt == _boneInfoMap.end())
			{
				ThrowErrCode(ErrCode::FileNotFound, "skeleton file : " + skeletonName + "이 _boneInfoMap에 없습니다.");
			}
			boneInfo = boneIt->second.get();

			++nodeIndex;
		}
		else
		{
			ThrowErrCode(ErrCode::NodeNotFound, "Skeleton 노드가 없습니다.");
		}
		
		if (childNodes[nodeIndex].getNodeName() == "Animation")
		{
			string animationInfoName;
			childNodes[nodeIndex].loadAttribute("FileName", animationInfoName);
			auto animIt = _animationInfoMap.find(animationInfoName);
			if (animIt == _animationInfoMap.end())
			{
				ThrowErrCode(ErrCode::FileNotFound, "Animation File : " + animationInfoName + "이 없습니다.");
			}
			animationInfo = animIt->second.get();
			++nodeIndex;
		}
		else
		{
			ThrowErrCode(ErrCode::NodeNotFound, "Animation 노드가 없습니다.");
		}

		skinnedConstantBufferIndex = _skinnedInstance.size();
		unique_ptr<SkinnedModelInstance> newSkinnedInstance(new SkinnedModelInstance(skinnedConstantBufferIndex, boneInfo, animationInfo));
		skinnedInstance = newSkinnedInstance.get();
		_skinnedInstance.emplace_back(move(newSkinnedInstance));
	}
	if (childNodes[nodeIndex].getNodeName() == "Mesh")
	{
		string meshFileName;
		childNodes[nodeIndex].loadAttribute("FileName", meshFileName);
		auto meshIt = _geometries.find(meshFileName);
		if (meshIt == _geometries.end())
		{
			ThrowErrCode(ErrCode::FileNotFound, "Mesh File : " + meshFileName + "이 없습니다.");
		}
		const auto& subMeshMap = meshIt->second->_subMeshMap;
		const auto& subMeshNodes = childNodes[nodeIndex].getChildNodes();
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
		++nodeIndex;
	}
	else
	{
		ThrowErrCode(ErrCode::NodeNotFound, "Mesh 노드가 없습니다.");
	}

	if (childNodes[nodeIndex].getNodeName() == "ActionStates")
	{
		++nodeIndex;
	}
	else
	{
		ThrowErrCode(ErrCode::NodeNotFound, "ActionStates 노드가 없습니다.");
	}
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
	const DirectX::XMFLOAT3& scale,
	const DirectX::XMFLOAT3& rotation,
	const DirectX::XMFLOAT3& transition)
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
		XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
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

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			_timer.Stop();
		}
		else
		{
			_timer.Start();
		}
		return 0;
	case WM_SIZE:
		_clientWidth = LOWORD(lParam);
		_clientHeight = HIWORD(lParam);
		
		if (_deviceD3d12 != nullptr)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				_timer.Stop();
				_minimized = true;
				_maximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				_timer.Start();
				_minimized = false;
				_maximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (_minimized)
				{
					_timer.Start();
					_minimized = false;
					OnResize();
				}
				else if (_maximized)
				{
					_timer.Start();
					_maximized = false;
					OnResize();
				}
				else if (_resizing)
				{
					__noop;
				}
				else
				{
					OnResize();
				}
			}
		}
		return 0;
	case WM_ENTERSIZEMOVE:
		_timer.Stop();
		_resizing = true;
		return 0;
	case WM_EXITSIZEMOVE:
		_timer.Start();
		_resizing = false;
		OnResize();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 800;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 600;
		((MINMAXINFO*)lParam)->ptMaxTrackSize.x = 800;
		((MINMAXINFO*)lParam)->ptMaxTrackSize.y = 600;
		return 0;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if (wParam == VK_F2)
		{
			set4XMsaaState(!_4xMsaaState);
		}
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::Initialize(void)
{
	initMainWindow();
	initDirect3D();
	initializeManagers();
	OnResize();

	ThrowIfFailed(_commandList->Reset(_commandAlloc.Get(), nullptr));

	buildRootSignature();
	buildShaders();
	buildPipelineStateObject();

	// 이 이후로는 맵 전환마다 수행되어야 할듯 [1/21/2021 qwerw]

	_uiManager->loadUI();
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

int D3DApp::Run(void)
{
	MSG msg = { 0 };
	_timer.Reset();
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			_timer.ProgressTick();
			if (!isAppPaused())
			{
				calculateFrameStats();
				//UpdateGameObject();
				Update();

				Draw();

				Sleep(10);
			}
			else
			{
				Sleep(100);
 			}
		}
	}
	return (int)msg.wParam;
}

float D3DApp::aspectRatio(void) const
{
	check(_clientHeight != 0, "zero divide");
	
	return static_cast<float>(_clientWidth) / _clientHeight;
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