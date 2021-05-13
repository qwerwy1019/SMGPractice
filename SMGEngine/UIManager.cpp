#include "stdafx.h"
#include "UIManager.h"
#include "D3DUtil.h"
#include "D3DApp.h"
#include "SMGFramework.h"


UIManager::UIManager()
{
}

void UIManager::drawUI()
{
	for (const auto& e : _uiGroups)
	{
		e.second->draw();
	}
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
}

void UIManager::updateUI()
{
	auto groupIt = _uiGroups.find("fpsTextGroup");
	if (groupIt != _uiGroups.end())
	{
		UIGroup* uiGroup = groupIt->second.get();
		UIElementText* textElement = static_cast<UIElementText*>(uiGroup->findElement("fpsText"));
		//std::wstring text = "frame: " + std::to_wstring(D3DApp::getApp()->getSelectedCharacter);
		//textElement->setText(text);
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

void UIGroup::addElement(std::unique_ptr<UIElement>&& uiElement)
{
	_child.emplace_back(std::move(uiElement));
}

void UIGroup::draw(void) const noexcept
{
	D2D1_POINT_2F position = { 0.f, 0.f };

	for (const auto& e : _child)
	{
		e->draw(_position);
	}
}

UIElement::UIElement(const std::string& name, const D2D1_POINT_2F& position, const DirectX::XMFLOAT2& size) noexcept
	: _name(name)
	, _localPosition(position)
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
	check(SMGFramework::Get().getD3DApp() != nullptr, "UI manager가 초기화되지 않았습니다.");
	check(SMGFramework::Get().getD3DApp()->getWriteFactory() != nullptr, "UI manager가 초기화되지 않았습니다.");
	check(formatType != TextFormatType::Count, "format Type이 비정상입니다.");
	check(!text.empty(), "text가 없습니다.");
	check(!name.empty(), "name(key)가 없습니다.");

	setText(text);
}

void UIElementText::draw(const D2D1_POINT_2F& parentPosition) const noexcept
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
