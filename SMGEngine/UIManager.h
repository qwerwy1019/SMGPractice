#pragma once
#include "TypeD3d.h"
#include "TypeUI.h"

class Material;


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
	virtual void draw(const D2D1_POINT_2F& parentPosition) const noexcept = 0;
protected:
	std::string _name;
	D2D1_POINT_2F _localPosition;
	DirectX::XMFLOAT2 _size;
};

class UIGroup
{
public:
	UIElement* findElement(const std::string& name) const noexcept;
	void addElement(std::unique_ptr<UIElement>&& uiElement);
	void draw(void) const noexcept;
private:
	std::vector<std::unique_ptr<UIElement>> _child;
	//UIElement* _parent;

	D2D1_POINT_2F _position;
	float _alpha;
};

class UIElementText : public UIElement
{
public:
	UIElementText(const std::string& name,
		const D2D1_POINT_2F& position,
		const DirectX::XMFLOAT2& size,
		TextFormatType formatType,
		const std::wstring& text);
	virtual void draw(const D2D1_POINT_2F& parentPosition) const noexcept override;
	void setText(const std::wstring& text);
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
	UIManager();
	void drawUI();
	void loadUI();
	void updateUI();

private:
	std::unordered_map<std::string, std::unique_ptr<UIGroup>> _uiGroups;
};