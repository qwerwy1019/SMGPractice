#include "stdafx.h"
#include "FileHelper.h"
#include "D3DUtil.h"

XMLWriter::XMLWriter(void)
	: _cursor(nullptr)
	, _cursorParent(nullptr)
	, _tabCount(1)
{
}

HRESULT XMLWriter::createDocument(const std::string& name) noexcept
{
	IXMLDOMProcessingInstructionPtr xmlProcessingInstruction;
	HRESULT rv = _document.CreateInstance(__uuidof(DOMDocument));
	if (FAILED(rv))
	{
		assert(false);
		return rv;
	}

	_bstr_t bstr1, bstr2;

	bstr1 = L"xml";
	bstr2 = L"version=\"1.0\"";

	ReturnIfFailed(_document->createProcessingInstruction(bstr1, bstr2, &xmlProcessingInstruction));

	ReturnIfFailed(_document->appendChild(xmlProcessingInstruction, nullptr));

	_bstr_t rootName = name.c_str();

	ReturnIfFailed(_document->createElement(rootName, &_cursorParent));
	ReturnIfFailed(_document->appendChild(_cursorParent, nullptr));
	ReturnIfFailed(insertNewLine());

	_cursor = _cursorParent;
	return S_OK;
}

HRESULT XMLWriter::addNode(const std::string& name) noexcept
{
	assert(_cursorParent != nullptr);
	assert(!name.empty());

	ReturnIfFailed(insertTab());

	HRESULT rv;
	_bstr_t bstrName = name.c_str();
	IXMLDOMElementPtr element;
	rv = _document->createElement(bstrName, &element);
	if (FAILED(rv))
	{
		assert(false);
		return rv;
	}
	IXMLDOMNodePtr outNewChild;
	rv = _cursorParent->appendChild(element, &outNewChild);
	if (FAILED(rv))
	{
		assert(false);
		return rv;
	}
	ReturnIfFailed(insertNewLine());

	DOMNodeType nodeType;
	assert(SUCCEEDED(outNewChild->get_nodeType(&nodeType)) && nodeType == NODE_ELEMENT);
	_cursor = static_cast<IXMLDOMElementPtr>(outNewChild);
	return S_OK;
}

HRESULT XMLWriter::openChildNode(void) noexcept
{
	_cursorParent = _cursor;
	ReturnIfFailed(insertNewLine());
	++_tabCount;

	return S_OK;
}

// hresult 처리를 어떻게 할지 조금 고민해보자 [1/25/2021 qwerw]
HRESULT XMLWriter::closeChildNode(void) noexcept
{
	assert(_cursor != nullptr);


	_cursor = _cursorParent;

	IXMLDOMNodePtr outParent;
	ReturnIfFailed(_cursorParent->get_parentNode(&outParent));
	DOMNodeType nodeType;
	ReturnIfFailed(outParent->get_nodeType(&nodeType));
	if (nodeType == NODE_ELEMENT)
	{
		--_tabCount;
		ReturnIfFailed(insertTab());
		_cursorParent = static_cast<IXMLDOMElementPtr>(outParent);
	}


	return S_OK;
}

HRESULT XMLWriter::writeXmlFile(const std::string& filePath) const noexcept
{
	char fullPath[1024];
	(void)_fullpath(fullPath, filePath.c_str(), 1024);
	std::string fullFilePathStr = fullPath;

	_bstr_t bstrPath = fullFilePathStr.c_str();
	_variant_t variantPath = fullFilePathStr.c_str();
	HRESULT rv = _document->save(variantPath);
	if (FAILED(rv))
	{
		assert(false);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT XMLWriter::insertNewLine(void) noexcept
{
	IXMLDOMTextPtr text;
	_bstr_t newLine = "\n";
	ReturnIfFailed(_document->createTextNode(newLine, &text));
	ReturnIfFailed(_cursorParent->appendChild(text, nullptr));

	return S_OK;
}

HRESULT XMLWriter::insertTab(void) noexcept
{
	IXMLDOMTextPtr text;
	std::string tab(_tabCount, '\t');
	_bstr_t bstrTab = tab.c_str();
	ReturnIfFailed(_document->createTextNode(bstrTab, &text));
	ReturnIfFailed(_cursorParent->appendChild(text, nullptr))
		return S_OK;
}

HRESULT XMLReader::loadXMLFile(const std::string& filePath) noexcept
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
	if (FAILED(rv) || isSuccessful == VARIANT_FALSE)
	{
		assert(false);
		return E_FAIL;
	}
	IXMLDOMElementPtr element;
	rv = _document->get_documentElement(&element);
	if (FAILED(rv))
	{
		assert(false);
		return E_FAIL;
	}
	_node = XMLReaderNode(element);

	return S_OK;
}

XMLReaderNode::XMLReaderNode(IXMLDOMElementPtr node) noexcept
	: _element(node)
{
	
}

std::string XMLReaderNode::getNodeName(void) const noexcept
{
	BSTR bstrNodeName;
	HRESULT rv = _element->get_nodeName(&bstrNodeName);
	if (FAILED(rv))
	{
		assert(false);
	}
	USES_CONVERSION;
	std::string nodeName = W2A(bstrNodeName);
	return nodeName;
}

std::vector<XMLReaderNode> XMLReaderNode::getChildNodes(void) const noexcept
{
	IXMLDOMNodeListPtr nodeList;
	HRESULT rv = _element->get_childNodes(&nodeList);
	assert(SUCCEEDED(rv));

	long listSize;
	rv = nodeList->get_length(&listSize);
	assert(SUCCEEDED(rv));

	std::vector<XMLReaderNode> childNodes(listSize);
	for (long i = 0; i < listSize; ++i)
	{
		IXMLDOMNodePtr node;
		rv = nodeList->get_item(i, &node);
		assert(SUCCEEDED(rv));
		DOMNodeType nodeType;
		rv = node->get_nodeType(&nodeType);
		assert(SUCCEEDED(rv) && nodeType == DOMNodeType::NODE_ELEMENT);

		IXMLDOMElementPtr element = static_cast<IXMLDOMElementPtr>(node);
		childNodes[i] = XMLReaderNode(element);
	}
	return childNodes;
}
