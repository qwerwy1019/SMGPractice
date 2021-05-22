#include "stdafx.h"
#include "FileHelper.h"
#include "D3DUtil.h"

XMLWriter::XMLWriter(const std::string& name)
	: _cursor(nullptr)
	, _cursorParent(nullptr)
	, _tabCount(1)
{
	check(!name.empty(), "비정상입니다.");

	IXMLDOMProcessingInstructionPtr xmlProcessingInstruction;
	ThrowIfFailed(_document.CreateInstance(__uuidof(DOMDocument)),
		"writer document create fail");

	_bstr_t bstr1, bstr2;

	bstr1 = L"xml";
	bstr2 = L"version=\"1.0\"";

	ThrowIfFailed(_document->createProcessingInstruction(bstr1, bstr2, &xmlProcessingInstruction),
		"xml version write fail");

	ThrowIfFailed(_document->appendChild(xmlProcessingInstruction, nullptr),
		"xml verstion node append fail");

	_bstr_t rootName = name.c_str();

	ThrowIfFailed(_document->createElement(rootName, &_cursorParent),
		"xml createElement fail");
	ThrowIfFailed(_document->appendChild(_cursorParent, nullptr),
		"xml appendChild fail");

	insertNewLine();

	_cursor = _cursorParent;
}

void XMLWriter::addNode(const std::string& name)
{
	check(_cursorParent != nullptr, "초기화를 확인해주세요.");
	check(!name.empty(), "이름이 비정상입니다");

	insertTab();

	_bstr_t bstrName = name.c_str();
	IXMLDOMElementPtr element;
	ThrowIfFailed(_document->createElement(bstrName, &element), "xml element create fail : " + name);

	IXMLDOMNodePtr outNewChild;
	ThrowIfFailed(_cursorParent->appendChild(element, &outNewChild), "xml append child fail : " + name);

	insertNewLine();
	DOMNodeType nodeType;
	ThrowIfFailed(outNewChild->get_nodeType(&nodeType));

	if (nodeType != NODE_ELEMENT)
	{
		ThrowErrCode(ErrCode::MsXmlError, "nodeType이 이상합니다.");
	}
	_cursor = static_cast<IXMLDOMElementPtr>(outNewChild);
}

void XMLWriter::openChildNode(void)
{
	_cursorParent = _cursor;
	insertNewLine();
	++_tabCount;
}

void XMLWriter::closeChildNode(void)
{
	check(_cursor != nullptr, "초기화를 확인해주세요.");

	_cursor = _cursorParent;

	IXMLDOMNodePtr outParent;

	ThrowIfFailed(_cursorParent->get_parentNode(&outParent), "get_parentNode fail");
	check(outParent != nullptr, "get_parentNode fail");

	DOMNodeType nodeType;
	ThrowIfFailed(outParent->get_nodeType(&nodeType), "get_nodeType fail");

	if (nodeType == NODE_ELEMENT)
	{
		--_tabCount;
		insertTab();
		_cursorParent = static_cast<IXMLDOMElementPtr>(outParent);
	}
}

void XMLWriter::writeXmlFile(const std::string& filePath) const
{
	char fullPath[1024];
	(void)_fullpath(fullPath, filePath.c_str(), 1024);
	std::string fullFilePathStr = fullPath;

	_bstr_t bstrPath = fullFilePathStr.c_str();
	_variant_t variantPath = fullFilePathStr.c_str();

	ThrowIfFailed(_document->save(variantPath), "writeXmlFile fail");
}

void XMLWriter::insertNewLine(void)
{
	check(_document != nullptr, "초기화를 확인해주세요.");
	check(_cursorParent != nullptr, "비정상입니다.");

	IXMLDOMTextPtr text;
	_bstr_t newLine = "\n";
	
	ThrowIfFailed(_document->createTextNode(newLine, &text));

	ThrowIfFailed(_cursorParent->appendChild(text, nullptr));
}

void XMLWriter::insertTab(void)
{
	IXMLDOMTextPtr text;
	std::string tab(_tabCount, '\t');
	_bstr_t bstrTab = tab.c_str();

	ThrowIfFailed(_document->createTextNode(bstrTab, &text));

	ThrowIfFailed(_cursorParent->appendChild(text, nullptr));
}

void XMLReader::loadXMLFile(const std::string& filePath)
{
	_document.CreateInstance(__uuidof(DOMDocument));
	_document->put_async(VARIANT_FALSE);
	_document->put_validateOnParse(VARIANT_TRUE);
	_document->put_resolveExternals(VARIANT_TRUE);
	
	char fullPath[1024];
	(void)_fullpath(fullPath, filePath.c_str(), 1024);
	std::string fullFilePathStr = fullPath;

	_variant_t pathVariant = fullPath;
	VARIANT_BOOL isSuccessful;
	HRESULT rv = _document->load(pathVariant, &isSuccessful);
	if (rv != S_OK || isSuccessful == VARIANT_FALSE)
	{
		ThrowErrCode(ErrCode::FileNotFound, filePath + "가 없습니다.");
	}
	IXMLDOMElementPtr element;

	ThrowIfFailed(_document->get_documentElement(&element), "xml get_documentElement.");
	_node = XMLReaderNode(element);
}

XMLReaderNode::XMLReaderNode(IXMLDOMElementPtr node) noexcept
	: _element(node)
{
	
}

std::string XMLReaderNode::getNodeName(void) const
{
	BSTR bstrNodeName;
	ThrowIfFailed(_element->get_nodeName(&bstrNodeName));

	USES_CONVERSION;
	std::string nodeName = W2A(bstrNodeName);
	return nodeName;
}

std::vector<XMLReaderNode> XMLReaderNode::getChildNodes(void) const
{
	IXMLDOMNodeListPtr nodeList;
	ThrowIfFailed(_element->get_childNodes(&nodeList));

	long listSize;
	ThrowIfFailed(nodeList->get_length(&listSize));

	std::vector<XMLReaderNode> childNodes;
	childNodes.reserve(listSize);
	for (long i = 0; i < listSize; ++i)
	{
		IXMLDOMNodePtr node;
		ThrowIfFailed(nodeList->get_item(i, &node));

		DOMNodeType nodeType;
		ThrowIfFailed(node->get_nodeType(&nodeType));
		
		if (nodeType != DOMNodeType::NODE_ELEMENT)
		{
			if (nodeType == DOMNodeType::NODE_COMMENT)
			{
				continue;
			}
			ThrowErrCode(ErrCode::TypeIsDifferent, "node type error!");
		}

		IXMLDOMElementPtr element = static_cast<IXMLDOMElementPtr>(node);
		childNodes.emplace_back(element);
	}
	return childNodes;
}

std::unordered_map<std::string, XMLReaderNode> XMLReaderNode::getChildNodesWithName(void) const
{
	IXMLDOMNodeListPtr nodeList;
	ThrowIfFailed(_element->get_childNodes(&nodeList));

	long listSize;
	ThrowIfFailed(nodeList->get_length(&listSize));

	std::unordered_map<std::string, XMLReaderNode> childNodes;
	childNodes.reserve(listSize);
	for (long i = 0; i < listSize; ++i)
	{
		IXMLDOMNodePtr node;
		ThrowIfFailed(nodeList->get_item(i, &node));

		DOMNodeType nodeType;
		ThrowIfFailed(node->get_nodeType(&nodeType));
		if (nodeType != DOMNodeType::NODE_ELEMENT)
		{
			if (nodeType == DOMNodeType::NODE_COMMENT)
			{
				continue;
			}
			ThrowErrCode(ErrCode::TypeIsDifferent, "node type error!");
		}

		IXMLDOMElementPtr element = static_cast<IXMLDOMElementPtr>(node);
		auto elementReaderNode = XMLReaderNode(element);
		childNodes[elementReaderNode.getNodeName()] = std::move(elementReaderNode);
	}
	return childNodes;
}
