#include "stdafx.h"
#include "UIManager.h"
#include "D3DUtil.h"
#include "D3DApp.h"


UIManager::UIManager(ID3D12CommandQueue* commandQueueD3d12, ID3D12Device* deviceD3d12)
{
	check(commandQueueD3d12 != nullptr, "D3DApp의 commandQueue가 사용되어야 합니다.");
	check(deviceD3d12 != nullptr, "D3DApp의 deviceD3d12가 사용되어야 합니다.");

	D2D1_FACTORY_OPTIONS options;
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, _d2dFactory.GetAddressOf());

	WComPtr<IDXGIDevice> deviceDxgi;
	WComPtr<ID3D11Device> deviceD3d11;
	WComPtr<ID3D11Device4> deviceD3d114;
	IUnknown* commandQueue = nullptr;

	ThrowIfFailed(commandQueueD3d12->QueryInterface(&commandQueue));
	unsigned __int32 DeviceFlags = ::D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D3D11On12CreateDevice(deviceD3d12,
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

void UIManager::drawUI(int backBufferIndex)
{
	_deviceD3d11On12->AcquireWrappedResources(_backBufferWrapped[backBufferIndex].GetAddressOf(), 1);
	
	_d2dContext->SetTarget(_backBufferBitmap[backBufferIndex].Get());
	_d2dContext->BeginDraw();
	D2D1_POINT_2F position = { 0.f, 0.f };
	
	for (const auto& e : _uiGroups)
	{
		e.second->draw();
	}
	
	_d2dContext->EndDraw();
	_d2dContext->SetTarget(nullptr);

	_deviceD3d11On12->ReleaseWrappedResources(_backBufferWrapped[backBufferIndex].GetAddressOf(), 1);

	_immediateContext->Flush();
}

void UIManager::loadUI()
{	
	std::unique_ptr<UIGroup> uiGroup = std::make_unique<UIGroup>();
	const D2D1_POINT_2F position = { 0, 0 };
	const DirectX::XMFLOAT2 size = { 73, 21 };
	std::unique_ptr<UIElement> uiElement = std::make_unique<UIElementText>("fpsText",
																			position,
																			size,
																			TextFormatType::Normal,
																			L"Hello world!");
	uiGroup->addElement(std::move(uiElement));
	_uiGroups.emplace("fpsTextGroup", std::move(uiGroup));

	_immediateContext->Flush();
}

void UIManager::beforeResize(void)
{
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		_backBufferWrapped[i].Reset();
		_backBufferBitmap[i].Reset();
	}
// 	_text.Reset();
// 	_bitmap.Reset();
// 	_surface.Reset();
	_immediateContext->Flush();
// 	_whiteBrush.Reset();
// 	_transparentBrush.Reset();
// 	_d2dContext->Flush();
}

void UIManager::onResize(ID3D12Resource* swapChainBuffer, UINT index)
{
	constexpr D3D11_RESOURCE_FLAGS resourceFlags = { ::D3D11_BIND_RENDER_TARGET | ::D3D11_BIND_SHADER_RESOURCE };
	ThrowIfFailed(_deviceD3d11On12->CreateWrappedResource(swapChainBuffer,
		&resourceFlags,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT,
		IID_PPV_ARGS(_backBufferWrapped[index].GetAddressOf())
	));

	WComPtr<IDXGISurface> surface;
	ThrowIfFailed(_backBufferWrapped[index]->QueryInterface(surface.GetAddressOf()));

	ThrowIfFailed(_d2dContext->CreateBitmapFromDxgiSurface(surface.Get(), &bitmapProperty, &_backBufferBitmap[index]));
}

void UIManager::afterResize(void)
{

}

const UIElement* UIGroup::findElement(const std::string& name) const noexcept
{
	for (const auto& e : _child)
	{
		if (e->isNameEqual(name))
		{
			return e.get();
		}
	}

	return nullptr;
}

void UIGroup::addElement(std::unique_ptr<UIElement>&& uiElement)
{
	_child.emplace_back(std::move(uiElement));
}

void UIGroup::draw(void) const noexcept
{
	D2D1_POINT_2F position = { 0.f, 0.f };

	for (const auto& e : _child)
	{
		e->draw();
	}
}

UIElement::UIElement(const std::string& name, const D2D1_POINT_2F& position, const DirectX::XMFLOAT2& size) noexcept
	: _name(name)
	, _position(position)
	, _size(size)
{

}

UIElementText::UIElementText(const std::string& name,
	const D2D1_POINT_2F& position,
	const DirectX::XMFLOAT2& size,
	TextFormatType formatType,
	const std::wstring& text)
	: UIElement(name, position, size)
	, _formatType(formatType)
{
	check(D3DApp::getUIManager() != nullptr, "UI manager가 초기화되지 않았습니다.");
	check(D3DApp::getUIManager()->getWriteFactory() != nullptr, "UI manager가 초기화되지 않았습니다.");
	check(formatType != TextFormatType::Count, "format Type이 비정상입니다.");
	check(!text.empty(), "text가 없습니다.");
	check(!name.empty(), "name(key)가 없습니다.");

	IDWriteFactory3* writeFactory = D3DApp::getUIManager()->getWriteFactory();
	IDWriteTextFormat* textFormat = D3DApp::getUIManager()->getTextFormat(formatType);
	writeFactory->CreateTextLayout(
		text.c_str(),
		text.length(),
		textFormat, size.x, size.y, &_textLayout);
}

void UIElementText::draw(void) const noexcept
{
	ID2D1Brush* brush = D3DApp::getUIManager()->getTextBrush(_brushType);
	D3DApp::getUIManager()->getD2dContext()->DrawTextLayout(_position, _textLayout.Get(), brush);
}
