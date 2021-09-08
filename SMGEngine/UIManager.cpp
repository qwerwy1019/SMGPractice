#include "stdafx.h"
#include "UIManager.h"
#include "D3DUtil.h"
#include "D3DApp.h"
#include "SMGFramework.h"
#include "FileHelper.h"
#include "UIFunction.h"
#include "MathHelper.h"


UIManager::UIManager()
	: _screenSize(0, 0)
{
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
	: _fromPosition(0, 0)
	, _moveBlendStartTick(0)
	, _moveBlendTick(0)
{
	node.loadAttribute("Position", _toPosition);

	std::string function;
	node.loadAttribute("UpdateFunction", function);
	if (function.empty())
	{
		_updateFunctionType = UIFunctionType::Count;
	}
	else if (function == "setHpUI")
	{
		_updateFunctionType = UIFunctionType::SetHpUI;
	}
	else
	{
		ThrowErrCode(ErrCode::UndefinedType, function);
		static_assert(static_cast<int>(UIFunctionType::Count) == 1);
	}
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		_child.emplace_back(UIElement::loadXMLUIElement(childNode));
	}
}

UIElement* UIGroup::findElement(const std::string& name) const noexcept
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

void UIGroup::update(void)
{
	if (_updateFunctionType != UIFunctionType::Count)
	{
		UIFunction::execute(_updateFunctionType, this);
	}
}

void UIGroup::draw(void) const noexcept
{
	DirectX::XMFLOAT2 position = getCurrentPosition();
	
	for (const auto& e : _child)
	{
		e->draw(position);
	}
}

DirectX::XMFLOAT2 UIGroup::getCurrentPosition(void) const noexcept
{
	if (_moveBlendTick == 0)
	{
		return _toPosition;
	}
	else
	{
		using namespace MathHelper;
		TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
		float t = getInterpolateValue(currentTick, _moveBlendStartTick, _moveBlendTick, _moveInterpolationType);
		return XMFLOAT2(_fromPosition.x * (1 - t) + _toPosition.x * t,
						_fromPosition.y * (1 - t) + _toPosition.y * t);
	}

}

void UIGroup::setMove(const DirectX::XMFLOAT2& toPosition,
					TickCount64 blendTick, 
					InterpolationType interpolationType) noexcept
{
	_toPosition = toPosition;
	_moveBlendTick = blendTick;
	_moveBlendStartTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	_moveInterpolationType = interpolationType;
	_fromPosition = getCurrentPosition();
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
	else
	{
		static_assert(static_cast<int>(UIElementType::Count) == 2);
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
