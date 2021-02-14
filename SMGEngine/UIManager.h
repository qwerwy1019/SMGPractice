#pragma once
#include "TypeD3d.h"

class Material;

class UIElement
{
	DirectX::XMFLOAT2 _position;
	DirectX::XMFLOAT2 _size;

	UIElement* _parent;
	std::vector<std::unique_ptr<UIElement>> _child;
};

class UIElementText : public UIElement
{
	std::wstring _text;
	Font* _font;
};

class UIElementImage : public UIElement
{
	Index16 _diffuseSRVHeapIndex;
	DirectX::XMFLOAT4 _diffuseAlbedo;
	DirectX::XMFLOAT4 _textureUV;
};

class UIElementButton : public UIElementImage
{
};

class UIElementPanel : public UIElementImage
{

};

class UIManager
{
public:

private:
	std::unique_ptr<UIElement> _rootElement;

};