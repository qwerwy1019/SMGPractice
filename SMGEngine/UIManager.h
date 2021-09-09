#pragma once
#include "TypeD3d.h"
#include "TypeUI.h"
#include "TypeCommon.h"

class Material;
class XMLReaderNode;


static constexpr D2D1_BITMAP_PROPERTIES1 bitmapProperty =
{
	{ BACK_BUFFER_FORMAT, D2D1_ALPHA_MODE_PREMULTIPLIED },
	96.f, 96.f,
	D2D1_BITMAP_OPTIONS_TARGET,
	nullptr
};

class UIElement
{
public:
	UIElement(const XMLReaderNode& node);
	UIElement(const std::string& name, const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size) noexcept;
	virtual UIElementType getType(void) const noexcept = 0;
	virtual ~UIElement() = default;
	bool isNameEqual(const std::string& name) const noexcept { return _name == name; }
	virtual void draw(const DirectX::XMFLOAT2& parentPosition) const noexcept = 0;
	static std::unique_ptr<UIElement> loadXMLUIElement(const XMLReaderNode& node);
protected:
	std::string _name;
	DirectX::XMFLOAT2 _localPosition;
	DirectX::XMFLOAT2 _size;
};

class UIElementText : public UIElement
{
public:
	UIElementText(const XMLReaderNode& node);
	virtual ~UIElementText() = default;
	virtual UIElementType getType(void) const noexcept override { return UIElementType::Text; }
	UIElementText(const std::string& name,
		const DirectX::XMFLOAT2& position,
		const DirectX::XMFLOAT2& size,
		TextFormatType formatType,
		TextBrushType brushType,
		const std::wstring& text);
	virtual void draw(const DirectX::XMFLOAT2& parentPosition) const noexcept override;
	void setText(const std::wstring& text);
private:
	WComPtr<IDWriteTextLayout> _textLayout;
	TextFormatType _formatType;
	TextBrushType _brushType;
};

class UIElementImage : public UIElement
{
public:
	UIElementImage(const XMLReaderNode& node);
	virtual ~UIElementImage() = default;
	virtual UIElementType getType(void) const noexcept override { return UIElementType::Image; }
	virtual void draw(const DirectX::XMFLOAT2& parentPosition) const noexcept override;
	void setImage(const std::string& imageName);
private:
	ID2D1Bitmap* _bitmapImage;
};

class UIElementIris : public UIElement
{
public:
	UIElementIris(const XMLReaderNode& node);
	virtual ~UIElementIris() = default;
	virtual UIElementType getType(void) const noexcept override { return UIElementType::Iris; }
	virtual void draw(const DirectX::XMFLOAT2& parentPosition) const noexcept override;
	void set(bool isIn) noexcept;
	void update(void);
private:
	WComPtr<ID2D1GeometryGroup> _geometryGroup;
	WComPtr<ID2D1EllipseGeometry> _circle;
	WComPtr<ID2D1RectangleGeometry> _screenRectangle;

	TickCount64 _currentTick;
	bool _isIn;

	float _radiusIn;
	float _radiusOut;
	TickCount64 _progressTick;
};
class UIElementButton : public UIElementImage
{
};

class UIGroup
{
public:
	UIGroup(const XMLReaderNode& node);
	UIElement* findElement(const std::string& name) const noexcept;
	void update(void);
	void draw(void) const noexcept;
	UIFunctionType getUpdateFunctionType(void) const noexcept { return _updateFunctionType; }
	DirectX::XMFLOAT2 getCurrentPositionOffset(void) const noexcept;
	const DirectX::XMFLOAT2 getHidePositionOffset(void) const noexcept { return _hidePositionOffset; }
	void setMove(const DirectX::XMFLOAT2& moveVector,
				TickCount64 blendTick,
				InterpolationType interpolationType) noexcept;
	void setActive(bool isActive) noexcept { _isActive = isActive; }
	bool isActive(void) { return _isActive; }
private:
	std::vector<std::unique_ptr<UIElement>> _child;

	DirectX::XMFLOAT2 _position;
	DirectX::XMFLOAT2 _positionOffsetFrom;
	DirectX::XMFLOAT2 _positionOffsetTo;
	TickCount64 _moveBlendTick;
	TickCount64 _moveBlendStartTick;
	InterpolationType _moveInterpolationType;
	UIFunctionType _updateFunctionType;
	DirectX::XMFLOAT2 _hidePositionOffset;

	bool _isActive;
};

class UIManager
{
public:
	UIManager();
	void releaseForStageLoad(void) noexcept;
	void drawUI();
	void loadXML(const std::string& fileName);
	void update();
	UIGroup* getGroup(const std::string& groupName) noexcept;
private:
	std::unordered_map<std::string, std::unique_ptr<UIGroup>> _uiGroups;
	DirectX::XMFLOAT2 _screenSize;
};