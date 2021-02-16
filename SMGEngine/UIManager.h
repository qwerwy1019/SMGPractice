#pragma once
#include "TypeD3d.h"

class Material;

enum class TextFormatType : uint8_t
{
	Normal,
	Count,
};
enum class TextBrushType : uint8_t
{
	Black,
	White,

	Count,
};
static constexpr D2D1_BITMAP_PROPERTIES1 bitmapProperty =
{
	{ BACK_BUFFER_FORMAT, D2D1_ALPHA_MODE_PREMULTIPLIED },
	96.f, 96.f,
	D2D1_BITMAP_OPTIONS_TARGET,
	nullptr
};

class UIElement
{
	// 나중에 이동생성자로 바꿀예정 [2/16/2021 qwerwy]
public:
	UIElement(const std::string& name, const D2D1_POINT_2F& position, const DirectX::XMFLOAT2& size) noexcept;
	bool isNameEqual(const std::string& name) const noexcept { return _name == name; }
	virtual void draw(void) const noexcept = 0;
protected:
	std::string _name;
	D2D1_POINT_2F _position;
	DirectX::XMFLOAT2 _size;
};

class UIGroup
{
public:
	const UIElement* findElement(const std::string& name) const noexcept;
	void addElement(std::unique_ptr<UIElement>&& uiElement);
	void draw(void) const noexcept;
private:
	std::vector<std::unique_ptr<UIElement>> _child;
	//UIElement* _parent;
};

class UIElementText : public UIElement
{
public:
	UIElementText(const std::string& name,
		const D2D1_POINT_2F& position,
		const DirectX::XMFLOAT2& size,
		TextFormatType formatType,
		const std::wstring& text);
	virtual void draw(void) const noexcept override;
private:
	WComPtr<IDWriteTextLayout> _textLayout;
	TextFormatType _formatType;
	TextBrushType _brushType;
};

class UIElementImage : public UIElement
{
	uint16_t _diffuseSRVHeapIndex;
	DirectX::XMFLOAT4 _diffuseAlbedo;
	DirectX::XMFLOAT4 _textureUV;
};

class UIElementButton : public UIElementImage
{
};

class UIManager
{
public:
	UIManager(ID3D12CommandQueue* commandQueueD3d12, ID3D12Device* deviceD3d12);
	void drawUI(int backBufferIndex);
	void loadUI();

	void beforeResize(void);
	void onResize(ID3D12Resource* swapChainBuffer, UINT index);
	void afterResize(void);

	inline IDWriteFactory3* getWriteFactory(void) const noexcept { return _writeFactory.Get(); }
	inline IDWriteTextFormat* getTextFormat(TextFormatType formatType) const noexcept { return _textFormats[static_cast<int>(formatType)].Get(); }
	inline ID2D1Brush* getTextBrush(TextBrushType brushType) const noexcept { return _textBrushes[static_cast<int>(brushType)].Get(); }
	inline ID2D1DeviceContext2* getD2dContext(void) const noexcept { return _d2dContext.Get(); }
private:
	std::unordered_map<std::string, std::unique_ptr<UIGroup>> _uiGroups;

	WComPtr<ID3D11Resource> _backBufferWrapped[SWAP_CHAIN_BUFFER_COUNT];
	WComPtr<ID2D1Bitmap1> _backBufferBitmap[SWAP_CHAIN_BUFFER_COUNT];
	WComPtr<ID2D1Factory3> _d2dFactory;
	WComPtr<ID2D1Device2> _deviceD2d;
	WComPtr<ID2D1DeviceContext2> _d2dContext;

	WComPtr<ID3D11On12Device> _deviceD3d11On12;
	WComPtr<IDWriteFactory3> _writeFactory;
	std::array<WComPtr<IDWriteTextFormat>, static_cast<int>(TextFormatType::Count)> _textFormats;
	std::array<WComPtr<ID2D1Brush>, static_cast<int>(TextBrushType::Count)> _textBrushes;

	WComPtr<ID3D11DeviceContext3> _immediateContext;
};