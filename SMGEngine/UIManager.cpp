#include "stdafx.h"
#include "UIManager.h"
#include "D3DUtil.h"
#include "D3DApp.h"
#include "SMGFramework.h"
#include "FileHelper.h"
#include "UIFunction.h"
#include "MathHelper.h"
#include <d2d1.h>


UIManager::UIManager()
	: _screenSize(0, 0)
{
}

void UIManager::releaseForStageLoad(void) noexcept
{
	_uiGroups.clear();
}

void UIManager::drawUI()
{
	for (const auto& e : _uiGroups)
	{
		e.second->draw();
	}
}

void UIManager::loadXML(const std::string& fileName)
{	
	const std::string filePath = "../Resources/XmlFiles/UI/" + fileName + ".xml";

	XMLReader xmlUI;
	xmlUI.loadXMLFile(filePath);
	xmlUI.getRootNode().loadAttribute("ScreenSize", _screenSize);
	const auto& childNodes = xmlUI.getRootNode().getChildNodes();
	for (const auto& childNode : childNodes)
	{
		std::string name;
		childNode.loadAttribute("Name", name);
		auto it = _uiGroups.emplace(name, std::make_unique<UIGroup>(childNode));
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, name);
		}
	}
}

void UIManager::update()
{
	for (const auto& uiGroup : _uiGroups)
	{
		uiGroup.second->update();
	}
}

UIGroup* UIManager::getGroup(const std::string& groupName) noexcept
{
	auto it = _uiGroups.find(groupName);
	if (it == _uiGroups.end())
	{
		return nullptr;
	}
	return it->second.get();
}

UIGroup::UIGroup(const XMLReaderNode& node)
	: _position(0, 0)
	, _positionOffsetFrom(0, 0)
	, _positionOffsetTo(0, 0)
	, _moveBlendStartTick(0)
	, _moveBlendTick(0)
	, _moveInterpolationType(InterpolationType::Count)
	, _isActive(true)
{
	node.loadAttribute("Position", _position);
	node.loadAttribute("Hide", _hidePositionOffset);
	std::string function;
	node.loadAttribute("UpdateFunction", function);
	_updateFunctionType = getUIFunctionTypeFromString(function);
	if(_updateFunctionType == UIFunctionType::Count)
	{
		ThrowErrCode(ErrCode::UndefinedType, function);
	}
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		_child.emplace_back(UIElement::loadXMLUIElement(childNode));
	}

	bool isHide;
	node.loadAttribute("InitHide", isHide);
	if (isHide)
	{
		_positionOffsetTo = _hidePositionOffset;
		_moveInterpolationType = InterpolationType::Linear;
	}
}

UIElement* UIGroup::findElement(const std::string& name, UIElementType typeCheck) const noexcept
{
	for (const auto& e : _child)
	{
		if (e->isNameEqual(name))
		{
			if (typeCheck != e->getType())
			{
				check(false, "element type error " +
							name +
							std::to_string(static_cast<int>(typeCheck)) +
							std::to_string(static_cast<int>(e->getType())),
							);
				return nullptr;
			}
			return e.get();
		}
	}

	return nullptr;
}

void UIGroup::update(void)
{
	if (_updateFunctionType != UIFunctionType::None)
	{
		UIFunction::execute(_updateFunctionType, this);
	}
}

void UIGroup::draw(void) const noexcept
{
	if (!_isActive)
	{
		return;
	}

	DirectX::XMFLOAT2 position = MathHelper::add(_position, getCurrentPositionOffset());
	
	for (const auto& e : _child)
	{
		e->draw(position);
	}
}

DirectX::XMFLOAT2 UIGroup::getCurrentPositionOffset(void) const noexcept
{
	if (_moveInterpolationType == InterpolationType::Count)
	{
		return XMFLOAT2(0, 0);
	}
	else
	{
		using namespace MathHelper;
		TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
		float t = getInterpolateValue(currentTick, _moveBlendStartTick, _moveBlendTick, _moveInterpolationType);
		return add(mul(_positionOffsetFrom, 1 - t), mul(_positionOffsetTo, t));
	}

}

void UIGroup::setMove(const DirectX::XMFLOAT2& moveVector,
					TickCount64 blendTick, 
					InterpolationType interpolationType) noexcept
{
	check(interpolationType != InterpolationType::Count);

	_positionOffsetFrom = getCurrentPositionOffset();
	_positionOffsetTo = moveVector;
	_moveBlendTick = blendTick;
	_moveBlendStartTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	_moveInterpolationType = interpolationType;
}

bool UIGroup::isMoving(void) const noexcept
{
	if (_moveInterpolationType == InterpolationType::Count)
	{
		return false;
	}
	if (_moveBlendStartTick + _moveBlendTick < SMGFramework::Get().getTimer().getCurrentTickCount())
	{
		return false;
	}
	return true;
}

UIElement::UIElement(const std::string& name, const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size) noexcept
	: _name(name)
	, _localPosition(position)
	, _size(size)
{

}

UIElement::UIElement(const XMLReaderNode& node)
{
	node.loadAttribute("Name", _name);
	node.loadAttribute("Position", _localPosition);
	node.loadAttribute("Size", _size);
}

std::unique_ptr<UIElement> UIElement::loadXMLUIElement(const XMLReaderNode& node)
{
	std::string typeString;
	node.loadAttribute("Type", typeString);
	if (typeString == "Image")
	{
		return std::make_unique<UIElementImage>(node);
	}
	else if (typeString == "Text")
	{
		return std::make_unique<UIElementText>(node);
	}
	else if (typeString == "Iris")
	{
		return std::make_unique<UIElementIris>(node);
	}
	else
	{
		static_assert(static_cast<int>(UIElementType::Count) == 3);
		ThrowErrCode(ErrCode::UndefinedType, typeString);
	}
}

UIElementText::UIElementText(const std::string& name,
	const DirectX::XMFLOAT2& position,
	const DirectX::XMFLOAT2& size,
	TextFormatType formatType,
	TextBrushType brushType,
	const std::wstring& text)
	: UIElement(name, position, size)
	, _formatType(formatType)
	, _brushType(brushType)
{
	check(SMGFramework::Get().getD3DApp() != nullptr, "UI manager가 초기화되지 않았습니다.");
	check(SMGFramework::Get().getD3DApp()->getWriteFactory() != nullptr, "UI manager가 초기화되지 않았습니다.");
	check(formatType != TextFormatType::Count, "format Type이 비정상입니다.");
	check(!text.empty(), "text가 없습니다.");
	check(!name.empty(), "name(key)가 없습니다.");

	setText(text);
}

UIElementText::UIElementText(const XMLReaderNode& node)
	: UIElement(node)
{
	std::wstring text;
	node.loadAttribute("Text", text);

	std::string typeString;
	node.loadAttribute("Brush", typeString);
	if (typeString == "Black")
	{
		_brushType = TextBrushType::Black;
	}
	else if (typeString == "White")
	{
		_brushType = TextBrushType::White;
	}
	else
	{
		ThrowErrCode(ErrCode::UndefinedType, typeString);
		static_assert(static_cast<int>(TextBrushType::Count) == 2);
	}

	node.loadAttribute("Format", typeString);
	if (typeString == "Normal")
	{
		_formatType = TextFormatType::Normal;
	}
	else if (typeString == "Points")
	{
		_formatType = TextFormatType::Points;
	}
	else if (typeString == "PointsOutline")
	{
		_formatType = TextFormatType::PointsOutline;
	}
	else
	{
		ThrowErrCode(ErrCode::UndefinedType, typeString);
		static_assert(static_cast<int>(TextFormatType::Count) == 3);
	}

	setText(text);
}

void UIElementText::draw(const DirectX::XMFLOAT2& parentPosition) const noexcept
{
	ID2D1Brush* brush = SMGFramework::Get().getD3DApp()->getTextBrush(_brushType);
	D2D1_POINT_2F position = { _localPosition.x + parentPosition.x, _localPosition.y + parentPosition.y };
	SMGFramework::Get().getD3DApp()->getD2dContext()->DrawTextLayout(position, _textLayout.Get(), brush);
}

void UIElementText::setText(const std::wstring& text)
{
	IDWriteFactory3* writeFactory = SMGFramework::Get().getD3DApp()->getWriteFactory();
	IDWriteTextFormat* textFormat = SMGFramework::Get().getD3DApp()->getTextFormat(_formatType);
	ThrowIfFailed(writeFactory->CreateTextLayout(
		text.c_str(),
		text.length(),
		textFormat, _size.x, _size.y, &_textLayout));
}

UIElementImage::UIElementImage(const XMLReaderNode& node)
	: UIElement(node)
{
	std::string imageName;
	node.loadAttribute("ImageName", imageName);
	_bitmapImage = SMGFramework::getD3DApp()->loadBitmapImage(imageName);
}

void UIElementImage::draw(const DirectX::XMFLOAT2& parentPosition) const noexcept
{
	float upperLeftX = _localPosition.x + parentPosition.x;
	float upperLeftY = _localPosition.y + parentPosition.y;
	float bottomRightX = upperLeftX + _size.x;
	float bottomRightY = upperLeftY + _size.y;
	SMGFramework::getD3DApp()->getD2dContext()->DrawBitmap(
		_bitmapImage,
		D2D1::RectF(upperLeftX, upperLeftY, bottomRightX, bottomRightY));
}

void UIElementImage::setImage(const std::string& imageName)
{
	_bitmapImage = SMGFramework::getD3DApp()->loadBitmapImage(imageName);
}

UIElementIris::UIElementIris(const XMLReaderNode& node)
	: UIElement(node)
{
	node.loadAttribute("RadiusIn", _radiusIn);
	node.loadAttribute("RadiusOut", _radiusOut);
	node.loadAttribute("InitTick", _currentTick);
	node.loadAttribute("InitIn", _isIn);
	node.loadAttribute("ProgressTick", _progressTick);

	const D2D1_RECT_F rect = D2D1::Rect(
		_localPosition.x - _size.x,
		_localPosition.y - _size.y,
		_localPosition.x + _size.x, 
		_localPosition.y + _size.y);

	ThrowIfFailed(SMGFramework::getD3DApp()->getD2dFactory()->CreateRectangleGeometry(
		rect, &_screenRectangle));

	const D2D1_ELLIPSE circle = D2D1::Ellipse(D2D1::Point2F(_localPosition.x, _localPosition.y), _size.x, _size.y);
	ThrowIfFailed(SMGFramework::getD3DApp()->getD2dFactory()->CreateEllipseGeometry(
		circle,
		&_circle));

	ID2D1Geometry* geometries[] =
	{
		_circle.Get(),
		_screenRectangle.Get(),
	};

	ThrowIfFailed(SMGFramework::getD3DApp()->getD2dFactory()->CreateGeometryGroup(
		D2D1_FILL_MODE_ALTERNATE,
		geometries,
		2,
		&_geometryGroup
	));
}

void UIElementIris::draw(const DirectX::XMFLOAT2& parentPosition) const noexcept
{
	ID2D1Brush* brush = SMGFramework::Get().getD3DApp()->getTextBrush(TextBrushType::Black);
	auto transformMatrix = D2D1::Matrix3x2F::Translation(parentPosition.x, parentPosition.y);
	WComPtr<ID2D1TransformedGeometry> transformedGeometry;

	SMGFramework::getD3DApp()->getD2dFactory()->CreateTransformedGeometry(
		_geometryGroup.Get(),
		transformMatrix,
		&transformedGeometry);

	SMGFramework::getD3DApp()->getD2dContext()->FillGeometry(transformedGeometry.Get(), brush);
}

void UIElementIris::set(bool isIn) noexcept
{
	_isIn = isIn;
}

void UIElementIris::update(void)
{
	if (_isIn)
	{
		if (_currentTick == _progressTick)
		{
			return;
		}
		TickCount64 deltaTick = SMGFramework::Get().getTimer().getDeltaTickCount();
		_currentTick = std::min(_currentTick + deltaTick, _progressTick);
	}
	else
	{
		if (_currentTick == 0)
		{
			return;
		}
		TickCount64 deltaTick = SMGFramework::Get().getTimer().getDeltaTickCount();
		if (_currentTick < deltaTick)
		{
			_currentTick = 0;
		}
		else
		{
			_currentTick -= deltaTick;
		}
	}
	
	D2D1_POINT_2F center = { _localPosition.x, _localPosition.y };
	float t = MathHelper::getInterpolateValue(
		_currentTick, 0, _progressTick, InterpolationType::Linear);
	float radius = _radiusIn * (1 - t) + _radiusOut * t;
	const D2D1_ELLIPSE circle = D2D1::Ellipse(center, radius, radius);
	ThrowIfFailed(SMGFramework::getD3DApp()->getD2dFactory()->CreateEllipseGeometry(circle, &_circle));
	ID2D1Geometry* geometries[] =
	{
		_circle.Get(),
		_screenRectangle.Get(),
	};

	ThrowIfFailed(SMGFramework::getD3DApp()->getD2dFactory()->CreateGeometryGroup(
		D2D1_FILL_MODE_ALTERNATE,
		geometries,
		2,
		&_geometryGroup
	));
}
