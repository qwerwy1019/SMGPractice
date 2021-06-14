#pragma once
#include "stdafx.h"
#include "D3DUtil.h"
#include <atlconv.h>

class XMLWriter
{
public:
	XMLWriter(const std::string& name);

	template<typename T>
	void addAttribute(const std::string& name, const T& value)
	{
		check(_cursor != nullptr, "cursor가 비정상입니다.");

		_bstr_t bstrName = name.c_str();
		_variant_t variantValue = value;
		IXMLDOMAttributePtr attribute;
		ThrowIfFailed(_document->createAttribute(bstrName, &attribute), "createAttribute fail");
		ThrowIfFailed(attribute->put_value(variantValue), "put_value fail");

		IXMLDOMAttributePtr outAttribute;
		ThrowIfFailed(_cursor->setAttributeNode(attribute, &outAttribute), "setAttributeNode fail");
		if (outAttribute != nullptr)
		{
			ThrowErrCode(ErrCode::InvalidXmlData, "attrName :" + name + "이 중복으로 추가되었습니다.");
		}
	}

	template<>
	void addAttribute(const std::string& name, const DirectX::XMFLOAT2& value)
	{
		check(_cursor != nullptr, "cursor가 비정상입니다.");
		std::string valueStr = std::to_string(value.x) + " " + std::to_string(value.y);
		return addAttribute(name, valueStr.c_str());
	}

	template<>
	void addAttribute(const std::string& name, const DirectX::XMFLOAT3& value)
	{
		check(_cursor != nullptr, "cursor가 비정상입니다.");
		std::string valueStr = std::to_string(value.x) + " " + std::to_string(value.y) + " " + std::to_string(value.z);
		return addAttribute(name, valueStr.c_str());
	}

	template<>
	void addAttribute(const std::string& name, const DirectX::XMFLOAT4& value)
	{
		check(_cursor != nullptr, "cursor가 비정상입니다.");
		std::string valueStr = std::to_string(value.x) + " " + std::to_string(value.y) + " " + std::to_string(value.z) + " " + std::to_string(value.w);
		return addAttribute(name, valueStr.c_str());
	}

	template<typename T>
	void addAttribute(const std::string& name, const std::vector<T>& value)
	{
		check(_cursor != nullptr, "cursor가 비정상입니다.");
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
	void addAttribute(const std::string& name, const std::array<T, Size>& value)
	{
		check(_cursor != nullptr, "cursor가 비정상입니다.");
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
	void addAttribute(const std::string& name, const DirectX::XMFLOAT4X4& value)
	{
		check(_cursor != nullptr, "cursor가 비정상입니다.");
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

	void addNode(const std::string& name);

	void openChildNode(void);

	void closeChildNode(void);

	void writeXmlFile(const std::string& filePath) const;
private:
	void insertNewLine(void);
	void insertTab(void);
	IXMLDOMElementPtr _cursor;
	IXMLDOMElementPtr _cursorParent;
	IXMLDOMDocumentPtr _document;
	int _tabCount;
};

class XMLReaderNode
{
public:
	XMLReaderNode() = default;

	XMLReaderNode(XMLReaderNode&&) = default;
	XMLReaderNode& operator=(XMLReaderNode&&) = default;

	XMLReaderNode(const XMLReaderNode&) = delete;
	XMLReaderNode& operator=(const XMLReaderNode&) = delete;

	XMLReaderNode(IXMLDOMElementPtr node) noexcept;

	template<typename T>
	void loadAttribute(const std::string& attrName, T& outValue) const
	{
		_bstr_t bstrAttrName = attrName.c_str();
		_variant_t out = outValue;
		ThrowIfFailed(_element->getAttribute(bstrAttrName, &out), attrName);

		outValue = out;
	}

	template<>
	void loadAttribute(const std::string& attrName, std::string& outValue) const
	{
		_bstr_t bstrAttrName = attrName.c_str();
		_variant_t out = outValue.c_str();
		ThrowIfFailed(_element->getAttribute(bstrAttrName, &out), attrName);

		USES_CONVERSION;
		outValue = W2A(out.bstrVal);
	}

	template<>
	void loadAttribute(const std::string& attrName, DirectX::XMFLOAT2& outValue) const
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		if (tokenized.size() != 2)
		{
			ThrowErrCode(ErrCode::TokenizeError);
		}
		outValue.x = std::stof(tokenized[0]);
		outValue.y = std::stof(tokenized[1]);
	}

	template<>
	void loadAttribute(const std::string& attrName, DirectX::XMFLOAT3& outValue) const
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		if (tokenized.size() != 3)
		{
			ThrowErrCode(ErrCode::TokenizeError);
		}
		outValue.x = std::stof(tokenized[0]);
		outValue.y = std::stof(tokenized[1]);
		outValue.z = std::stof(tokenized[2]);
	}

	template<>
	void loadAttribute(const std::string& attrName, DirectX::XMFLOAT4& outValue) const
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		if(tokenized.size() != 4)
		{
			ThrowErrCode(ErrCode::TokenizeError);
		}
		outValue.x = std::stof(tokenized[0]);
		outValue.y = std::stof(tokenized[1]);
		outValue.z = std::stof(tokenized[2]);
		outValue.w = std::stof(tokenized[3]);
	}

	template<>
	void loadAttribute(const std::string& attrName, DirectX::XMFLOAT4X4& outValue) const
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		if (tokenized.size() != 16)
		{
			ThrowErrCode(ErrCode::TokenizeError);
		}
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				outValue.m[i][j] = std::stof(tokenized[4 * i + j]);
			}
		}
	}

	template<>
	void loadAttribute(const std::string& attrName, DirectX::XMINT3& outValue) const
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		if (tokenized.size() != 3)
		{
			ThrowErrCode(ErrCode::TokenizeError);
		}
		outValue.x = std::stoi(tokenized[0]);
		outValue.y = std::stoi(tokenized[1]);
		outValue.z = std::stoi(tokenized[2]);
	}

	template<typename T>
	void loadAttribute(const std::string& attrName, std::vector<T>& outValue) const
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
	void loadAttribute(const std::string& attrName, std::array<T, Size>& outValue) const
	{
		std::string outString;
		loadAttribute(attrName, outString);
		const auto& tokenized = D3DUtil::tokenizeString(outString, ' ');
		if (tokenized.size() != Size)
		{
			ThrowErrCode(ErrCode::TokenizeError);
		}
		for (int i = 0; i < tokenized.size(); ++i)
		{
			outValue[i] = D3DUtil::convertTo<T>(tokenized[i]);
		}
	}
	std::string getNodeName(void) const;

	std::vector<XMLReaderNode> getChildNodes(void) const;
	std::unordered_map<std::string, XMLReaderNode> getChildNodesWithName(void) const;
private:
	IXMLDOMElementPtr _element;
};

class XMLReader
{
public:
	XMLReader() = default;
	void loadXMLFile(const std::string& filePath);
	const XMLReaderNode& getRootNode(void) const noexcept { return _node; }
private:
	XMLReaderNode _node;
	IXMLDOMDocumentPtr _document;
};