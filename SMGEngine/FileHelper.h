#pragma once
#include "stdafx.h"
#include "D3DUtil.h"
#include <atlconv.h>

class XMLWriter
{
public:
	XMLWriter();
	HRESULT createDocument(const std::string& name) noexcept;

	template<typename T>
	HRESULT addAttribute(const std::string& name, const T& value) noexcept
	{
		assert(_cursor != nullptr);

		_bstr_t bstrName = name.c_str();
		_variant_t variantValue = value;
		IXMLDOMAttributePtr attribute;
		ReturnIfFailed(_document->createAttribute(bstrName, &attribute));
		ReturnIfFailed(attribute->put_value(variantValue));

		ReturnIfFailed(_cursor->setAttributeNode(attribute, nullptr));

		return S_OK;
	}

	template<>
	HRESULT addAttribute(const std::string& name, const DirectX::XMFLOAT2& value) noexcept
	{
		assert(_cursor != nullptr);
		std::string valueStr = std::to_string(value.x) + " " + std::to_string(value.y);
		return addAttribute(name, valueStr.c_str());
	}

	template<>
	HRESULT addAttribute(const std::string& name, const DirectX::XMFLOAT3& value) noexcept
	{
		assert(_cursor != nullptr);
		std::string valueStr = std::to_string(value.x) + " " + std::to_string(value.y) + " " + std::to_string(value.z);
		return addAttribute(name, valueStr.c_str());
	}

	template<>
	HRESULT addAttribute(const std::string& name, const DirectX::XMFLOAT4& value) noexcept
	{
		assert(_cursor != nullptr);
		std::string valueStr = std::to_string(value.x) + " " + std::to_string(value.y) + " " + std::to_string(value.z) + " " + std::to_string(value.w);
		return addAttribute(name, valueStr.c_str());
	}

	template<typename T>
	HRESULT addAttribute(const std::string& name, const std::vector<T>& value) noexcept
	{
		assert(_cursor != nullptr);
		std::string valueStr;
		for (int i = 0; i < value.size(); ++i)
		{
			valueStr += std::to_string(value[i]);
			valueStr += " ";
		}
		if (!valueStr.empty())
		{
			valueStr.resize(valueStr.size() - 1);
		}
		return addAttribute(name, valueStr.c_str());
	}

	template<typename T, int Size>
	HRESULT addAttribute(const std::string& name, const std::array<T, Size>& value) noexcept
	{
		assert(_cursor != nullptr);
		std::string valueStr;
		for (int i = 0; i < Size; ++i)
		{
			valueStr += std::to_string(value[i]);
			valueStr += " ";
		}
		valueStr.resize(valueStr.size() - 1);

		return addAttribute(name, valueStr.c_str());
	}

	template<>
	HRESULT addAttribute(const std::string& name, const DirectX::XMFLOAT4X4& value) noexcept
	{
		assert(_cursor != nullptr);
		std::string valueStr;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				valueStr += std::to_string(value.m[i][j]);
				valueStr += " ";
			}
		}
		valueStr.resize(valueStr.size() - 1);
		return addAttribute(name, valueStr.c_str());
	}

	HRESULT addNode(const std::string& name) noexcept;

	HRESULT openChildNode(void) noexcept;

	HRESULT closeChildNode(void) noexcept;

	HRESULT returnToRootNode(void) noexcept;

	HRESULT writeXmlFile(const std::string& filePath) const noexcept;
private:
	HRESULT insertNewLine(void) noexcept;
	HRESULT insertTab(void) noexcept;
	IXMLDOMElementPtr _cursor;
	IXMLDOMElementPtr _cursorParent;
	IXMLDOMDocumentPtr _document;
	int _tabCount;
};

class XMLReaderNode
{
public:
	XMLReaderNode() = default;
	XMLReaderNode(IXMLDOMElementPtr node) noexcept;

	template<typename T>
	void loadAttribute(const std::string& attrName, T& outValue) const noexcept
	{
		_bstr_t bstrAttrName = attrName.c_str();
		_variant_t out = outValue;
		HRESULT rv = _element->getAttribute(bstrAttrName, &out);
		if (FAILED(rv))
		{
			assert(false);
			return;
		}
		outValue = out;
	}

	template<>
	void loadAttribute(const std::string& attrName, std::string& outValue) const noexcept
	{
		_bstr_t bstrAttrName = attrName.c_str();
		_variant_t out = outValue.c_str();
		HRESULT rv = _element->getAttribute(bstrAttrName, &out);
		if (FAILED(rv))
		{
			assert(false);
			return;
		}
		USES_CONVERSION;
		outValue = W2A(out.bstrVal);
	}

	template<>
	void loadAttribute(const std::string& attrName, DirectX::XMFLOAT2& outValue) const noexcept
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		assert(tokenized.size() == 2);
		outValue.x = std::stof(tokenized[0]);
		outValue.y = std::stof(tokenized[1]);
	}

	template<>
	void loadAttribute(const std::string& attrName, DirectX::XMFLOAT3& outValue) const noexcept
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		assert(tokenized.size() == 3);
		outValue.x = std::stof(tokenized[0]);
		outValue.y = std::stof(tokenized[1]);
		outValue.z = std::stof(tokenized[2]);
	}

	template<>
	void loadAttribute(const std::string& attrName, DirectX::XMFLOAT4& outValue) const noexcept
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		assert(tokenized.size() == 4);
		outValue.x = std::stof(tokenized[0]);
		outValue.y = std::stof(tokenized[1]);
		outValue.z = std::stof(tokenized[2]);
		outValue.w = std::stof(tokenized[3]);
	}

	template<>
	void loadAttribute(const std::string& attrName, DirectX::XMFLOAT4X4& outValue) const noexcept
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		assert(tokenized.size() == 16);
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				outValue.m[i][j] = std::stof(tokenized[4 * i + j]);
			}
		}
	}

	template<typename T>
	void loadAttribute(const std::string& attrName, std::vector<T>& outValue) const noexcept
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		outValue.resize(tokenized.size());
		for (int i = 0; i < tokenized.size(); ++i)
		{
			outValue[i] = D3DUtil::convertTo<T>(tokenized[i]);
		}
	}
	template<typename T, int Size>
	void loadAttribute(const std::string& attrName, std::array<T, Size>& outValue) const noexcept
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		assert(tokenized.size() == Size);
		for (int i = 0; i < tokenized.size(); ++i)
		{
			outValue[i] = D3DUtil::convertTo<T>(tokenized[i]);
		}
	}
	std::string getNodeName(void) const noexcept;

	std::vector<XMLReaderNode> getChildNodes(void) const noexcept;

private:
	IXMLDOMElementPtr _element;
};

class XMLReader
{
public:
	XMLReader() = default;
	HRESULT loadXMLFile(const std::string& filePath) noexcept;
	XMLReaderNode getRootNode(void) const noexcept { return _node; }
private:
	XMLReaderNode _node;
	IXMLDOMDocumentPtr _document;
};